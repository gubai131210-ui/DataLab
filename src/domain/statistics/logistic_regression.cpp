#include "domain/statistics/logistic_regression.h"

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

std::string term_name(std::size_t index, const std::vector<std::string>& labels)
{
    if (index == 0) {
        return "Intercept";
    }
    return labels.empty() ? "X" + std::to_string(index) : labels[index - 1];
}

}  // namespace

LogisticRegressionResult fit_logistic_regression(
    const std::vector<int>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& predictor_labels,
    double confidence_level,
    std::size_t max_iterations,
    double tolerance)
{
    LogisticRegressionResult result;
    if (response.size() < 3 || predictors.size() != response.size()
        || predictors.empty() || predictors.front().empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_logistic_shape",
                       "二元 Logistic 回归至少需要三个观测和一个预测变量。");
        return result;
    }
    if (!(confidence_level > 0.0 && confidence_level < 1.0)
        || !(tolerance > 0.0) || max_iterations == 0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_logistic_options",
                       "置信水平、收敛容差和最大迭代次数必须有效。");
        return result;
    }
    const std::size_t predictor_count = predictors.front().size();
    if (!predictor_labels.empty() && predictor_labels.size() != predictor_count) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_predictor_labels",
                       "预测变量标签数量必须与预测变量列数一致。");
        return result;
    }
    for (std::size_t row = 0; row < response.size(); ++row) {
        if ((response[row] != 0 && response[row] != 1)
            || predictors[row].size() != predictor_count) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           predictors[row].size() != predictor_count
                               ? "ragged_predictor_matrix" : "invalid_binary_response",
                           predictors[row].size() != predictor_count
                               ? "每行预测变量必须具有相同列数。"
                               : "响应变量必须全部为 0 或 1。");
            return result;
        }
        for (double value : predictors[row]) {
            if (!std::isfinite(value)) {
                add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                               "invalid_predictor", "预测变量必须全部为有限数值。");
                return result;
            }
        }
    }

    result.observation_count = response.size();
    result.predictor_count = predictor_count;
    const std::size_t parameter_count = predictor_count + 1;
    if (response.size() <= parameter_count) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_logistic_degrees_of_freedom",
                       "Logistic 回归需要多于参数数量的观测。");
        return result;
    }

    Matrix design(response.size(), std::vector<double>(parameter_count, 1.0));
    for (std::size_t row = 0; row < response.size(); ++row) {
        for (std::size_t column = 0; column < predictor_count; ++column) {
            design[row][column + 1] = predictors[row][column];
        }
    }

    std::vector<double> coefficients(parameter_count, 0.0);
    Matrix information;
    bool overflow_seen = false;
    for (std::size_t iteration = 0; iteration < max_iterations; ++iteration) {
        information.assign(parameter_count,
                           std::vector<double>(parameter_count, 0.0));
        std::vector<double> score(parameter_count, 0.0);
        double maximum_step = 0.0;
        for (std::size_t row = 0; row < response.size(); ++row) {
            double eta = std::inner_product(
                design[row].cbegin(), design[row].cend(), coefficients.cbegin(), 0.0);
            if (!std::isfinite(eta) || std::abs(eta) > 700.0) {
                overflow_seen = true;
            }
            const double probability = logistic(std::clamp(eta, -700.0, 700.0));
            const double weight = std::max(kProbabilityFloor,
                                           probability * (1.0 - probability));
            const double residual = static_cast<double>(response[row]) - probability;
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
                           "Logistic 信息矩阵秩亏，无法进行 IRLS 更新。");
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
    if (overflow_seen) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "numeric_overflow",
                       "线性预测量达到数值稳定性边界，概率已进行安全截断。");
    }
    if (!result.converged) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "non_convergence",
                       "IRLS 在最大迭代次数内未达到收敛容差。");
    }

    std::vector<double> probabilities(response.size(), 0.0);
    std::size_t boundary_count = 0;
    bool boundary_matches_response = true;
    result.observations.reserve(response.size());
    result.log_likelihood = 0.0;
    result.deviance = 0.0;
    for (std::size_t row = 0; row < response.size(); ++row) {
        const double eta = std::inner_product(
            design[row].cbegin(), design[row].cend(), coefficients.cbegin(), 0.0);
        const double probability = logistic(std::clamp(eta, -700.0, 700.0));
        probabilities[row] = probability;
        const double safe_probability = std::clamp(
            probability, kProbabilityFloor, 1.0 - kProbabilityFloor);
        const double y = static_cast<double>(response[row]);
        result.log_likelihood += y * std::log(safe_probability)
            + (1.0 - y) * std::log1p(-safe_probability);
        const double deviance_contribution = y * std::log(y == 0.0 ? 1.0
                : y / safe_probability)
            + (1.0 - y) * std::log((y == 1.0 ? 1.0
                : (1.0 - y) / (1.0 - safe_probability)));
        result.deviance += 2.0 * deviance_contribution;
        const double variance = std::max(kProbabilityFloor,
                                          safe_probability * (1.0 - safe_probability));
        const double deviance_residual = std::copysign(
            std::sqrt(std::max(0.0, 2.0 * deviance_contribution)), y - probability);
        result.observations.push_back({
            response[row], eta, probability, (y - probability) / std::sqrt(variance),
            deviance_residual});
        if (probability <= kProbabilityFloor || probability >= 1.0 - kProbabilityFloor) {
            ++boundary_count;
            if ((probability > 0.5 ? 1 : 0) != response[row]) {
                boundary_matches_response = false;
            }
        }
    }
    if (boundary_count > 0 && boundary_matches_response) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "complete_separation",
                       "预测变量完全分离了 0/1 响应，极大似然估计可能不存在。");
    } else if (boundary_count > 0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "quasi_separation",
                       "预测概率达到边界，模型可能存在准分离。");
    }

    // Rebuild the information matrix at the final coefficient vector.
    information.assign(parameter_count,
                       std::vector<double>(parameter_count, 0.0));
    for (std::size_t row = 0; row < response.size(); ++row) {
        const double eta = std::inner_product(
            design[row].cbegin(), design[row].cend(), coefficients.cbegin(), 0.0);
        const double probability = logistic(std::clamp(eta, -700.0, 700.0));
        const double weight = std::max(
            kProbabilityFloor, probability * (1.0 - probability));
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
                       "无法计算 Logistic 系数协方差矩阵。");
        return result;
    }
    const double critical = normal_quantile(0.5 + confidence_level / 2.0);
    result.coefficients.reserve(parameter_count);
    for (std::size_t index = 0; index < parameter_count; ++index) {
        LogisticCoefficient coefficient;
        coefficient.term = term_name(index, predictor_labels);
        coefficient.coefficient = coefficients[index];
        coefficient.standard_error = std::sqrt(std::max(0.0, covariance[index][index]));
        coefficient.z_statistic = coefficient.standard_error > 0.0
            ? coefficient.coefficient / coefficient.standard_error : 0.0;
        coefficient.p_value = two_sided_normal_p(coefficient.z_statistic);
        coefficient.odds_ratio = std::exp(std::clamp(coefficient.coefficient, -700.0, 700.0));
        coefficient.confidence_lower = coefficient.coefficient
            - critical * coefficient.standard_error;
        coefficient.confidence_upper = coefficient.coefficient
            + critical * coefficient.standard_error;
        result.coefficients.push_back(coefficient);
    }
    const double parameter_count_value = static_cast<double>(parameter_count);
    result.aic = -2.0 * result.log_likelihood + 2.0 * parameter_count_value;
    result.bic = -2.0 * result.log_likelihood
        + std::log(static_cast<double>(response.size())) * parameter_count_value;
    return result;
}

double predict_logistic_probability(
    const LogisticRegressionResult& result,
    const std::vector<double>& predictors)
{
    if (result.coefficients.size() != predictors.size() + 1) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double eta = result.coefficients.front().coefficient;
    for (std::size_t index = 0; index < predictors.size(); ++index) {
        if (!std::isfinite(predictors[index])) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        eta += result.coefficients[index + 1].coefficient * predictors[index];
    }
    return logistic(std::clamp(eta, -700.0, 700.0));
}

std::vector<double> predict_logistic_probabilities(
    const LogisticRegressionResult& result,
    const std::vector<std::vector<double>>& predictors)
{
    std::vector<double> probabilities;
    probabilities.reserve(predictors.size());
    for (const auto& row : predictors) {
        probabilities.push_back(predict_logistic_probability(result, row));
    }
    return probabilities;
}

}  // namespace datalab::domain::statistics
