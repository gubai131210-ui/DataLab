#include "domain/statistics/probit_reliability.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

using Matrix = std::vector<std::vector<double>>;
constexpr double kProbabilityFloor = 1.0e-15;
constexpr double kPivotTolerance = 1.0e-12;

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const char* message)
{
    diagnostics.push_back({severity, code, message});
}

double logistic(double eta)
{
    if (eta >= 0.0) {
        const double exp_negative = std::exp(-eta);
        return 1.0 / (1.0 + exp_negative);
    }
    const double exp_positive = std::exp(eta);
    return exp_positive / (1.0 + exp_positive);
}

bool invert_matrix(const Matrix& input, Matrix& inverse)
{
    const std::size_t size = input.size();
    if (size == 0) {
        return false;
    }
    Matrix augmented(size, std::vector<double>(size * 2, 0.0));
    double scale = 0.0;
    for (std::size_t row = 0; row < size; ++row) {
        if (input[row].size() != size) {
            return false;
        }
        for (std::size_t column = 0; column < size; ++column) {
            augmented[row][column] = input[row][column];
            scale = std::max(scale, std::abs(input[row][column]));
        }
        augmented[row][size + row] = 1.0;
    }
    const double pivot_limit = std::max(1.0, scale) * kPivotTolerance;
    for (std::size_t column = 0; column < size; ++column) {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < size; ++row) {
            if (std::abs(augmented[row][column])
                > std::abs(augmented[pivot][column])) {
                pivot = row;
            }
        }
        if (!std::isfinite(augmented[pivot][column])
            || std::abs(augmented[pivot][column]) <= pivot_limit) {
            return false;
        }
        std::swap(augmented[pivot], augmented[column]);
        const double divisor = augmented[column][column];
        for (double& value : augmented[column]) {
            value /= divisor;
        }
        for (std::size_t row = 0; row < size; ++row) {
            if (row == column) {
                continue;
            }
            const double factor = augmented[row][column];
            for (std::size_t value = 0; value < size * 2; ++value) {
                augmented[row][value] -= factor * augmented[column][value];
            }
        }
    }
    inverse.assign(size, std::vector<double>(size, 0.0));
    for (std::size_t row = 0; row < size; ++row) {
        for (std::size_t column = 0; column < size; ++column) {
            inverse[row][column] = augmented[row][size + column];
        }
    }
    return true;
}

bool solve_linear_system(const Matrix& input, const std::vector<double>& rhs,
                         std::vector<double>& solution)
{
    Matrix inverse;
    if (rhs.size() != input.size() || !invert_matrix(input, inverse)) {
        return false;
    }
    solution.assign(rhs.size(), 0.0);
    for (std::size_t row = 0; row < rhs.size(); ++row) {
        for (std::size_t column = 0; column < rhs.size(); ++column) {
            solution[row] += inverse[row][column] * rhs[column];
        }
    }
    return std::all_of(solution.cbegin(), solution.cend(),
                       [](double value) { return std::isfinite(value); });
}

double normal_quantile(double probability)
{
    double lower = -9.0;
    double upper = 9.0;
    for (int iteration = 0; iteration < 80; ++iteration) {
        const double middle = (lower + upper) / 2.0;
        const double cdf = 0.5 * std::erfc(-middle / std::sqrt(2.0));
        if (cdf < probability) {
            lower = middle;
        } else {
            upper = middle;
        }
    }
    return (lower + upper) / 2.0;
}

double two_sided_normal_p(double z)
{
    return std::erfc(std::abs(z) / std::sqrt(2.0));
}

void compute_ld50(ProbitReliabilityResult& result, double confidence_level,
                  const Matrix& covariance)
{
    if (result.coefficients.size() < 2 || covariance.size() < 2) {
        return;
    }
    const double intercept = result.coefficients[0].coefficient;
    const double slope = result.coefficients[1].coefficient;
    if (!std::isfinite(slope) || std::abs(slope) <= 1.0e-12) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                       "ld50_not_identified",
                       "应力系数接近 0，无法估计 LD50。");
        return;
    }
    const double ld50 = -intercept / slope;
    if (!std::isfinite(ld50)) {
        return;
    }
    result.ld50 = ld50;

    const double var_intercept = std::max(0.0, covariance[0][0]);
    const double var_slope = std::max(0.0, covariance[1][1]);
    const double cov_intercept_slope = covariance[0][1];
    const double gradient_intercept = -1.0 / slope;
    const double gradient_slope = intercept / (slope * slope);
    const double variance = gradient_intercept * gradient_intercept * var_intercept
        + gradient_slope * gradient_slope * var_slope
        + 2.0 * gradient_intercept * gradient_slope * cov_intercept_slope;
    if (!(variance > 0.0) || !std::isfinite(variance)) {
        return;
    }
    const double standard_error = std::sqrt(variance);
    result.ld50_standard_error = standard_error;
    const double critical = normal_quantile(0.5 + confidence_level / 2.0);
    result.ld50_confidence_lower = ld50 - critical * standard_error;
    result.ld50_confidence_upper = ld50 + critical * standard_error;
}

}  // namespace

ProbitReliabilityResult fit_probit_reliability(
    const std::vector<std::size_t>& events,
    const std::vector<std::size_t>& trials,
    const std::vector<double>& stress,
    double confidence_level,
    std::size_t max_iterations,
    double tolerance)
{
    ProbitReliabilityResult result;
    result.link = "logit";
    if (events.size() < 3 || events.size() != trials.size()
        || events.size() != stress.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_probit_shape",
                       "Probit 可靠性分析至少需要三个 complete-case 行，且事件数、试验数与应力列长度一致。");
        return result;
    }
    if (!(confidence_level > 0.0 && confidence_level < 1.0)
        || !(tolerance > 0.0) || max_iterations == 0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_probit_options",
                       "置信水平、收敛容差和最大迭代次数必须有效。");
        return result;
    }

    for (std::size_t row = 0; row < events.size(); ++row) {
        if (trials[row] == 0 || events[row] > trials[row] || !std::isfinite(stress[row])) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_probit_row",
                           "事件数必须为非负且不超过试验数；应力必须为有限数值。");
            return result;
        }
    }

    result.observation_count = events.size();
    const std::size_t parameter_count = 2;
    Matrix design(events.size(), std::vector<double>(parameter_count, 1.0));
    for (std::size_t row = 0; row < events.size(); ++row) {
        design[row][1] = stress[row];
    }

    std::vector<double> coefficients(parameter_count, 0.0);
    Matrix information;
    for (std::size_t iteration = 0; iteration < max_iterations; ++iteration) {
        information.assign(parameter_count,
                           std::vector<double>(parameter_count, 0.0));
        std::vector<double> score(parameter_count, 0.0);
        double maximum_step = 0.0;
        for (std::size_t row = 0; row < events.size(); ++row) {
            const double eta = std::inner_product(
                design[row].cbegin(), design[row].cend(), coefficients.cbegin(), 0.0);
            const double probability = logistic(std::clamp(eta, -700.0, 700.0));
            const double weight = static_cast<double>(trials[row])
                * std::max(kProbabilityFloor, probability * (1.0 - probability));
            const double residual = static_cast<double>(events[row])
                - static_cast<double>(trials[row]) * probability;
            for (std::size_t first = 0; first < parameter_count; ++first) {
                score[first] += design[row][first] * residual;
                for (std::size_t second = 0; second < parameter_count; ++second) {
                    information[first][second] +=
                        design[row][first] * weight * design[row][second];
                }
            }
        }
        std::vector<double> step;
        if (!solve_linear_system(information, score, step)) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "rank_deficient_design",
                           "Probit 信息矩阵秩亏，无法进行 IRLS 更新。");
            return result;
        }
        for (std::size_t index = 0; index < parameter_count; ++index) {
            coefficients[index] += step[index];
            maximum_step = std::max(maximum_step, std::abs(step[index]));
        }
        result.iteration_count = iteration + 1;
        if (maximum_step < tolerance) {
            result.converged = true;
            break;
        }
    }
    if (!result.converged) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "non_convergence",
                       "IRLS 在最大迭代次数内未达到收敛容差。");
    }

    information.assign(parameter_count,
                       std::vector<double>(parameter_count, 0.0));
    result.log_likelihood = 0.0;
    result.deviance = 0.0;
    result.observations.reserve(events.size());
    for (std::size_t row = 0; row < events.size(); ++row) {
        const double eta = std::inner_product(
            design[row].cbegin(), design[row].cend(), coefficients.cbegin(), 0.0);
        const double probability = logistic(std::clamp(eta, -700.0, 700.0));
        const double safe_probability = std::clamp(
            probability, kProbabilityFloor, 1.0 - kProbabilityFloor);
        const double n = static_cast<double>(trials[row]);
        const double y = static_cast<double>(events[row]);
        result.log_likelihood += y * std::log(safe_probability)
            + (n - y) * std::log1p(-safe_probability);
        const double saturated = y * std::log(y == 0.0 ? 1.0 : y / n)
            + (n - y) * std::log((n - y) == 0.0 ? 1.0 : (n - y) / n);
        result.deviance += 2.0 * (saturated - (y * std::log(safe_probability)
            + (n - y) * std::log1p(-safe_probability)));
        const double variance = std::max(
            kProbabilityFloor, n * safe_probability * (1.0 - safe_probability));
        const double pearson = (y - n * safe_probability) / std::sqrt(variance);
        const double proportion = n > 0.0 ? y / n : 0.0;
        result.observations.push_back({
            stress[row], events[row], trials[row], proportion, probability, eta, pearson});
        const double weight = n * std::max(kProbabilityFloor,
                                           safe_probability * (1.0 - safe_probability));
        for (std::size_t first = 0; first < parameter_count; ++first) {
            for (std::size_t second = 0; second < parameter_count; ++second) {
                information[first][second] +=
                    design[row][first] * weight * design[row][second];
            }
        }
    }

    Matrix covariance;
    if (!invert_matrix(information, covariance)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "rank_deficient_covariance",
                       "无法计算 Probit 系数协方差矩阵。");
        return result;
    }

    const double critical = normal_quantile(0.5 + confidence_level / 2.0);
    const std::vector<std::string> labels = {"Intercept", "Stress"};
    result.coefficients.reserve(parameter_count);
    for (std::size_t index = 0; index < parameter_count; ++index) {
        ProbitReliabilityCoefficient coefficient;
        coefficient.term = labels[index];
        coefficient.coefficient = coefficients[index];
        coefficient.standard_error = std::sqrt(std::max(0.0, covariance[index][index]));
        coefficient.z_statistic = coefficient.standard_error > 0.0
            ? coefficient.coefficient / coefficient.standard_error : 0.0;
        coefficient.p_value = two_sided_normal_p(coefficient.z_statistic);
        coefficient.confidence_lower = coefficient.coefficient
            - critical * coefficient.standard_error;
        coefficient.confidence_upper = coefficient.coefficient
            + critical * coefficient.standard_error;
        result.coefficients.push_back(coefficient);
    }

    result.aic = -2.0 * result.log_likelihood
        + 2.0 * static_cast<double>(parameter_count);
    compute_ld50(result, confidence_level, covariance);
    add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                   "probit_link",
                   "本轮使用 logit 链接拟合二项比例；非 Minitab probit golden。");
    return result;
}

}  // namespace datalab::domain::statistics
