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

double chi_square_right_tail(double value, double degrees_of_freedom)
{
    if (!(value >= 0.0) || !(degrees_of_freedom > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double shape = degrees_of_freedom / 2.0;
    const double x = value / 2.0;
    if (x == 0.0) {
        return 1.0;
    }
    double term = 1.0 / shape;
    double sum = term;
    for (int index = 1; index < 200; ++index) {
        term *= x / (shape + index);
        sum += term;
        if (std::abs(term) < std::abs(sum) * 1.0e-14) {
            break;
        }
    }
    if (x < shape + 1.0) {
        return std::clamp(1.0 - sum * std::exp(-x + shape * std::log(x)
            - std::lgamma(shape)), 0.0, 1.0);
    }
    double continued = 1.0;
    double factor = 1.0;
    for (int index = 1; index < 200; ++index) {
        factor *= (shape - index) / x;
        continued += factor;
        if (std::abs(factor) < std::abs(continued) * 1.0e-14) {
            break;
        }
    }
    return std::clamp(std::exp(-x + shape * std::log(x)
        - std::lgamma(shape)) * continued, 0.0, 1.0);
}

std::vector<std::optional<double>> compute_vif(
    const std::vector<std::vector<double>>& predictors)
{
    if (predictors.empty() || predictors.front().empty()) {
        return {};
    }
    const std::size_t n = predictors.size();
    const std::size_t p = predictors.front().size();
    std::vector<std::optional<double>> vif(p, std::nullopt);
    if (n <= p) {
        return vif;
    }
    for (std::size_t target = 0; target < p; ++target) {
        Matrix design(n, std::vector<double>(p, 1.0));
        std::vector<double> y(n, 0.0);
        for (std::size_t row = 0; row < n; ++row) {
            y[row] = predictors[row][target];
            std::size_t col = 1;
            for (std::size_t feature = 0; feature < p; ++feature) {
                if (feature == target) {
                    continue;
                }
                design[row][col++] = predictors[row][feature];
            }
        }
        Matrix normal(p, std::vector<double>(p, 0.0));
        std::vector<double> rhs(p, 0.0);
        for (std::size_t row = 0; row < n; ++row) {
            for (std::size_t i = 0; i < p; ++i) {
                rhs[i] += design[row][i] * y[row];
                for (std::size_t j = 0; j < p; ++j) {
                    normal[i][j] += design[row][i] * design[row][j];
                }
            }
        }
        std::vector<double> beta;
        if (!solve_linear_system(normal, rhs, beta)) {
            continue;
        }
        double y_mean = std::accumulate(y.cbegin(), y.cend(), 0.0) / static_cast<double>(n);
        double ss_tot = 0.0;
        double ss_res = 0.0;
        for (std::size_t row = 0; row < n; ++row) {
            double fitted = 0.0;
            for (std::size_t i = 0; i < p; ++i) {
                fitted += design[row][i] * beta[i];
            }
            const double d_tot = y[row] - y_mean;
            const double d_res = y[row] - fitted;
            ss_tot += d_tot * d_tot;
            ss_res += d_res * d_res;
        }
        if (!(ss_tot > 0.0)) {
            continue;
        }
        const double r2 = std::clamp(1.0 - ss_res / ss_tot, 0.0, 1.0 - 1.0e-12);
        vif[target] = 1.0 / (1.0 - r2);
    }
    return vif;
}

void compute_hosmer_lemeshow(LogisticRegressionResult& result)
{
    result.hosmer_lemeshow_status = "not_computed";
    if (result.observations.size() < 20 || result.complete_separation || !result.converged) {
        result.hosmer_lemeshow_status = "not_computed";
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                       "hosmer_lemeshow_not_computed",
                       "Hosmer–Lemeshow 在样本量不足、未收敛或完全分离时不计算。");
        return;
    }
    std::vector<std::size_t> order(result.observations.size());
    for (std::size_t index = 0; index < order.size(); ++index) {
        order[index] = index;
    }
    std::sort(order.begin(), order.end(), [&](std::size_t first, std::size_t second) {
        return result.observations[first].probability
            < result.observations[second].probability;
    });
    const std::size_t groups = 10;
    const std::size_t count = result.observations.size();
    double statistic = 0.0;
    std::size_t used_groups = 0;
    for (std::size_t group = 0; group < groups; ++group) {
        const std::size_t start = group * count / groups;
        const std::size_t end = (group + 1) * count / groups;
        if (end <= start) {
            continue;
        }
        double observed = 0.0;
        double expected = 0.0;
        for (std::size_t index = start; index < end; ++index) {
            const LogisticObservation& observation = result.observations[order[index]];
            observed += static_cast<double>(observation.response);
            expected += observation.probability;
        }
        const double n = static_cast<double>(end - start);
        const double mean_p = expected / n;
        const double variance = n * mean_p * (1.0 - mean_p);
        if (variance <= 1.0e-12) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                           "hosmer_lemeshow_not_computed",
                           "Hosmer–Lemeshow 分组期望方差过小，检验不可用。");
            return;
        }
        const double residual = observed - expected;
        statistic += residual * residual / variance;
        ++used_groups;
    }
    if (used_groups < 6) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                       "hosmer_lemeshow_not_computed",
                       "Hosmer–Lemeshow 有效组数不足，检验不可用。");
        return;
    }
    result.hosmer_lemeshow_statistic = statistic;
    result.hosmer_lemeshow_groups = used_groups;
    result.hosmer_lemeshow_df = used_groups - 2;
    result.hosmer_lemeshow_p = chi_square_right_tail(
        statistic, static_cast<double>(used_groups) - 2.0);
    result.hosmer_lemeshow_status = "computed";
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
        result.complete_separation = true;
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
    for (std::size_t row = 0; row < result.observations.size(); ++row) {
        const double weight = std::max(
            kProbabilityFloor,
            result.observations[row].probability
                * (1.0 - result.observations[row].probability));
        double quadratic = 0.0;
        for (std::size_t first = 0; first < parameter_count; ++first) {
            double inner = 0.0;
            for (std::size_t second = 0; second < parameter_count; ++second) {
                inner += covariance[first][second] * design[row][second];
            }
            quadratic += design[row][first] * inner;
        }
        result.observations[row].leverage = weight * quadratic;
    }
    const double leverage_threshold = std::min(
        3.0 * static_cast<double>(predictor_count + 1)
            / static_cast<double>(response.size()),
        0.99);
    result.leverage_threshold = leverage_threshold;
    double maximum_leverage = 0.0;
    for (auto& observation : result.observations) {
        observation.high_leverage = observation.leverage > leverage_threshold;
        maximum_leverage = std::max(maximum_leverage, observation.leverage);
    }
    if (!result.observations.empty()) {
        result.maximum_leverage = maximum_leverage;
    }
    const std::vector<std::optional<double>> vif = compute_vif(predictors);
    std::optional<double> maximum_vif;
    for (std::size_t feature = 0; feature < predictor_count; ++feature) {
        if (feature + 1 < result.coefficients.size()) {
            result.coefficients[feature + 1].vif = vif[feature];
        }
        if (vif[feature].has_value()) {
            maximum_vif = maximum_vif.has_value()
                ? std::max(*maximum_vif, *vif[feature]) : *vif[feature];
        }
    }
    if (maximum_vif.has_value()) {
        result.maximum_vif = maximum_vif;
    }
    compute_hosmer_lemeshow(result);
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
