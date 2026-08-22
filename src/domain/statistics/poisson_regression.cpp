#include "domain/statistics/poisson_regression.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

using Matrix = std::vector<std::vector<double>>;
constexpr double kMuFloor = 1.0e-12;
constexpr double kPivotTolerance = 1.0e-12;

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
            if (std::abs(augmented[row][column]) > std::abs(augmented[pivot][column])) {
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

double poisson_log_likelihood(double y, double mu)
{
    // log P(Y=y) = y log μ − μ − log(y!) ; Stirling for large y optional — use lgamma.
    return y * std::log(mu) - mu - std::lgamma(y + 1.0);
}

double poisson_deviance_term(double y, double mu)
{
    if (y <= 0.0) {
        return 2.0 * mu;
    }
    return 2.0 * (y * std::log(y / mu) - (y - mu));
}

}  // namespace

PoissonRegressionResult fit_poisson_regression(
    const std::vector<double>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& predictor_labels,
    double confidence_level,
    std::size_t max_iterations,
    double tolerance)
{
    PoissonRegressionResult result;
    if (response.size() < 3 || predictors.size() != response.size()
        || predictors.empty() || predictors.front().empty()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "poisson_invalid_shape",
            "Poisson 回归至少需要三个观测和一个预测变量。"});
        return result;
    }
    const std::size_t p = predictors.front().size();
    result.predictor_count = p;
    for (std::size_t row = 0; row < response.size(); ++row) {
        if (!std::isfinite(response[row]) || response[row] < 0.0
            || predictors[row].size() != p) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "poisson_invalid_response",
                "响应必须为非负有限值，且预测矩阵规整。"});
            return result;
        }
        for (double value : predictors[row]) {
            if (!std::isfinite(value)) {
                result.diagnostics.push_back({
                    DiagnosticMessage::Severity::error, "poisson_nonfinite_predictor",
                    "预测变量含非有限值。"});
                return result;
            }
        }
    }

    const std::size_t n = response.size();
    const std::size_t columns = p + 1;
    std::vector<double> beta(columns, 0.0);
    const double mean_y =
        std::accumulate(response.cbegin(), response.cend(), 0.0) / static_cast<double>(n);
    beta[0] = std::log(std::max(mean_y, kMuFloor));

    double previous_deviance = std::numeric_limits<double>::infinity();
    Matrix xtwx(columns, std::vector<double>(columns, 0.0));
    for (std::size_t iteration = 0; iteration < max_iterations; ++iteration) {
        std::vector<double> xtwz(columns, 0.0);
        for (std::size_t i = 0; i < columns; ++i) {
            std::fill(xtwx[i].begin(), xtwx[i].end(), 0.0);
        }
        double deviance = 0.0;
        double log_likelihood = 0.0;
        for (std::size_t row = 0; row < n; ++row) {
            double eta = beta[0];
            for (std::size_t j = 0; j < p; ++j) {
                eta += beta[j + 1] * predictors[row][j];
            }
            const double mu = std::max(std::exp(eta), kMuFloor);
            const double weight = mu;
            const double z = eta + (response[row] - mu) / mu;
            deviance += poisson_deviance_term(response[row], mu);
            log_likelihood += poisson_log_likelihood(response[row], mu);

            std::vector<double> x(columns, 1.0);
            for (std::size_t j = 0; j < p; ++j) {
                x[j + 1] = predictors[row][j];
            }
            for (std::size_t i = 0; i < columns; ++i) {
                xtwz[i] += weight * x[i] * z;
                for (std::size_t j = 0; j < columns; ++j) {
                    xtwx[i][j] += weight * x[i] * x[j];
                }
            }
        }

        std::vector<double> updated;
        if (!solve_linear_system(xtwx, xtwz, updated)) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "poisson_singular",
                "IRLS 信息矩阵奇异。"});
            return result;
        }
        beta = updated;
        result.iteration_count = iteration + 1;
        result.deviance = deviance;
        result.log_likelihood = log_likelihood;
        if (std::abs(previous_deviance - deviance) < tolerance) {
            result.converged = true;
            break;
        }
        previous_deviance = deviance;
    }
    if (!result.converged) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "poisson_max_iterations",
            "已达最大迭代次数。"});
    }

    Matrix covariance;
    if (!invert_matrix(xtwx, covariance)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "poisson_se_failed",
            "无法计算系数协方差。"});
        covariance.assign(columns, std::vector<double>(columns, 0.0));
    }
    const double z_crit = normal_quantile(0.5 + 0.5 * confidence_level);
    result.coefficients.clear();
    for (std::size_t index = 0; index < columns; ++index) {
        PoissonCoefficient coefficient;
        coefficient.term = index == 0
            ? "Intercept"
            : (index - 1 < predictor_labels.size()
                   ? predictor_labels[index - 1]
                   : ("X" + std::to_string(index)));
        coefficient.coefficient = beta[index];
        coefficient.standard_error = std::sqrt(std::max(0.0, covariance[index][index]));
        coefficient.z_statistic =
            coefficient.standard_error > 0.0
                ? coefficient.coefficient / coefficient.standard_error
                : 0.0;
        coefficient.p_value = two_sided_normal_p(coefficient.z_statistic);
        coefficient.confidence_lower =
            coefficient.coefficient - z_crit * coefficient.standard_error;
        coefficient.confidence_upper =
            coefficient.coefficient + z_crit * coefficient.standard_error;
        result.coefficients.push_back(coefficient);
    }

    result.observation_count = n;
    result.aic = -2.0 * result.log_likelihood + 2.0 * static_cast<double>(columns);
    result.observations.clear();
    for (std::size_t row = 0; row < n; ++row) {
        PoissonObservation observation;
        observation.response = response[row];
        observation.linear_predictor = beta[0];
        for (std::size_t j = 0; j < p; ++j) {
            observation.linear_predictor += beta[j + 1] * predictors[row][j];
        }
        observation.fitted = std::max(std::exp(observation.linear_predictor), kMuFloor);
        observation.pearson_residual =
            (response[row] - observation.fitted) / std::sqrt(observation.fitted);
        result.observations.push_back(observation);
    }
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "poisson_link",
        "默认 natural log 链；IRLS；非 Minitab golden。"});
    return result;
}

}  // namespace datalab::domain::statistics
