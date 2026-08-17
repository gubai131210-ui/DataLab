#include "domain/statistics/regression.h"

#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

using Matrix = std::vector<std::vector<double>>;

void add_error(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

bool invert_matrix(const Matrix& input, Matrix& inverse)
{
    const std::size_t size = input.size();
    inverse.assign(size, std::vector<double>(size, 0.0));
    Matrix augmented(size, std::vector<double>(size * 2, 0.0));
    for (std::size_t row = 0; row < size; ++row) {
        if (input[row].size() != size) {
            return false;
        }
        for (std::size_t column = 0; column < size; ++column) {
            augmented[row][column] = input[row][column];
        }
        augmented[row][size + row] = 1.0;
    }
    for (std::size_t column = 0; column < size; ++column) {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < size; ++row) {
            if (std::abs(augmented[row][column])
                > std::abs(augmented[pivot][column])) {
                pivot = row;
            }
        }
        if (std::abs(augmented[pivot][column]) < 1.0e-12) {
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
    for (std::size_t row = 0; row < size; ++row) {
        for (std::size_t column = 0; column < size; ++column) {
            inverse[row][column] = augmented[row][size + column];
        }
    }
    return true;
}

double two_sided_p(double statistic, double degrees_of_freedom)
{
    return std::clamp(2.0 * (1.0 - student_t_cdf(
        std::abs(statistic), degrees_of_freedom)), 0.0, 1.0);
}

}  // namespace

RegressionResult fit_linear_regression(
    const std::vector<double>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& predictor_labels,
    double confidence_level)
{
    RegressionResult result;
    if (response.size() < 3 || predictors.empty()
        || predictors.size() != response.size()) {
        add_error(result.diagnostics, "invalid_regression_shape",
                  "回归至少需要三个观测、一个响应列和一个预测列。");
        return result;
    }
    if (!(confidence_level > 0.0 && confidence_level < 1.0)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    const std::size_t predictor_count = predictors.front().size();
    if (predictor_count == 0
        || (!predictor_labels.empty() && predictor_labels.size() != predictor_count)) {
        add_error(result.diagnostics, "invalid_predictor_count",
                  "预测变量数量和标签数量必须有效且一致。");
        return result;
    }
    for (const auto& row : predictors) {
        if (row.size() != predictor_count) {
            add_error(result.diagnostics, "ragged_predictor_matrix",
                      "每行预测变量必须具有相同列数。");
            return result;
        }
    }
    result.observation_count = response.size();
    result.predictor_count = predictor_count;
    const std::size_t parameter_count = predictor_count + 1;
    if (result.observation_count <= parameter_count) {
        add_error(result.diagnostics, "insufficient_error_degrees_of_freedom",
                  "回归需要正的误差自由度。");
        return result;
    }

    Matrix design(result.observation_count,
                  std::vector<double>(parameter_count, 1.0));
    for (std::size_t row = 0; row < result.observation_count; ++row) {
        if (!std::isfinite(response[row])) {
            add_error(result.diagnostics, "invalid_response",
                      "响应变量必须全部为有限数值。");
            return result;
        }
        for (std::size_t column = 0; column < predictor_count; ++column) {
            if (!std::isfinite(predictors[row][column])) {
                add_error(result.diagnostics, "invalid_predictor",
                          "预测变量必须全部为有限数值。");
                return result;
            }
            design[row][column + 1] = predictors[row][column];
        }
    }

    Matrix q(result.observation_count,
             std::vector<double>(parameter_count, 0.0));
    Matrix r(parameter_count, std::vector<double>(parameter_count, 0.0));
    for (std::size_t column = 0; column < parameter_count; ++column) {
        std::vector<double> vector(result.observation_count, 0.0);
        for (std::size_t row = 0; row < result.observation_count; ++row) {
            vector[row] = design[row][column];
        }
        for (std::size_t previous = 0; previous < column; ++previous) {
            double projection = 0.0;
            for (std::size_t row = 0; row < result.observation_count; ++row) {
                projection += q[row][previous] * vector[row];
            }
            r[previous][column] = projection;
            for (std::size_t row = 0; row < result.observation_count; ++row) {
                vector[row] -= projection * q[row][previous];
            }
        }
        double norm = 0.0;
        for (const double value : vector) {
            norm += value * value;
        }
        norm = std::sqrt(norm);
        if (norm < 1.0e-10) {
            add_error(result.diagnostics, "rank_deficient_design",
                      "设计矩阵存在完全共线或常量预测变量。");
            return result;
        }
        r[column][column] = norm;
        for (std::size_t row = 0; row < result.observation_count; ++row) {
            q[row][column] = vector[row] / norm;
        }
    }

    std::vector<double> q_transposed_y(parameter_count, 0.0);
    for (std::size_t column = 0; column < parameter_count; ++column) {
        for (std::size_t row = 0; row < result.observation_count; ++row) {
            q_transposed_y[column] += q[row][column] * response[row];
        }
    }
    std::vector<double> coefficients(parameter_count, 0.0);
    for (std::size_t index = parameter_count; index-- > 0;) {
        double value = q_transposed_y[index];
        for (std::size_t next = index + 1; next < parameter_count; ++next) {
            value -= r[index][next] * coefficients[next];
        }
        coefficients[index] = value / r[index][index];
    }

    double response_mean = std::accumulate(
        response.cbegin(), response.cend(), 0.0)
        / static_cast<double>(result.observation_count);
    result.total_sum_of_squares = 0.0;
    result.error_sum_of_squares = 0.0;
    result.observations.reserve(result.observation_count);
    std::vector<double> leverages(result.observation_count, 0.0);
    for (std::size_t row = 0; row < result.observation_count; ++row) {
        double fitted = 0.0;
        for (std::size_t column = 0; column < parameter_count; ++column) {
            fitted += design[row][column] * coefficients[column];
            leverages[row] += q[row][column] * q[row][column];
        }
        const double residual = response[row] - fitted;
        result.total_sum_of_squares += (response[row] - response_mean)
            * (response[row] - response_mean);
        result.error_sum_of_squares += residual * residual;
        RegressionObservation observation;
        observation.response = response[row];
        observation.fitted = fitted;
        observation.residual = residual;
        observation.leverage = leverages[row];
        result.observations.push_back(observation);
    }
    result.regression_sum_of_squares = result.total_sum_of_squares
        - result.error_sum_of_squares;
    const double error_df = static_cast<double>(
        result.observation_count - parameter_count);
    result.error_mean_square = result.error_sum_of_squares / error_df;
    result.residual_standard_deviation = std::sqrt(result.error_mean_square);
    result.regression_mean_square = result.regression_sum_of_squares
        / static_cast<double>(predictor_count);
    result.r_squared = result.total_sum_of_squares > 0.0
        ? 1.0 - result.error_sum_of_squares / result.total_sum_of_squares : 0.0;
    result.adjusted_r_squared = 1.0
        - (1.0 - result.r_squared)
            * static_cast<double>(result.observation_count - 1) / error_df;
    result.f_statistic = result.error_mean_square > 0.0
        ? result.regression_mean_square / result.error_mean_square : 0.0;
    result.model_p_value = f_right_tail(
        result.f_statistic, static_cast<double>(predictor_count), error_df);

    Matrix cross_product(parameter_count,
                         std::vector<double>(parameter_count, 0.0));
    for (const auto& row : design) {
        for (std::size_t first = 0; first < parameter_count; ++first) {
            for (std::size_t second = 0; second < parameter_count; ++second) {
                cross_product[first][second] += row[first] * row[second];
            }
        }
    }
    Matrix cross_inverse;
    if (!invert_matrix(cross_product, cross_inverse)) {
        add_error(result.diagnostics, "singular_covariance_matrix",
                  "无法计算回归系数协方差矩阵。");
        return result;
    }
    const double critical = student_t_quantile(
        0.5 + confidence_level / 2.0, error_df);
    for (std::size_t index = 0; index < parameter_count; ++index) {
        RegressionCoefficient coefficient;
        coefficient.term = index == 0 ? "Constant"
            : predictor_labels.empty()
                ? "X" + std::to_string(index)
                : predictor_labels[index - 1];
        coefficient.coefficient = coefficients[index];
        coefficient.standard_error = std::sqrt(
            std::max(0.0, result.error_mean_square * cross_inverse[index][index]));
        coefficient.t_statistic = coefficient.standard_error > 0.0
            ? coefficient.coefficient / coefficient.standard_error : 0.0;
        coefficient.p_value = two_sided_p(coefficient.t_statistic, error_df);
        coefficient.confidence_lower = coefficient.coefficient
            - critical * coefficient.standard_error;
        coefficient.confidence_upper = coefficient.coefficient
            + critical * coefficient.standard_error;
        if (index > 0) {
            coefficient.vif = cross_inverse[index][index]
                * std::max(0.0, cross_product[index][index]);
        }
        result.coefficients.push_back(coefficient);
    }
    for (auto& observation : result.observations) {
        const double residual_scale = result.residual_standard_deviation
            * std::sqrt(std::max(1.0e-15, 1.0 - observation.leverage));
        observation.standardized_residual = residual_scale > 0.0
            ? observation.residual / residual_scale : 0.0;
        observation.studentized_residual = observation.standardized_residual;
        observation.cooks_distance = result.error_mean_square > 0.0
            ? observation.standardized_residual * observation.standardized_residual
                * observation.leverage / (static_cast<double>(parameter_count)
                    * std::max(1.0e-15, 1.0 - observation.leverage))
            : 0.0;
        observation.dfits = observation.standardized_residual
            * std::sqrt(std::max(0.0, observation.leverage
                / std::max(1.0e-15, 1.0 - observation.leverage)));
    }
    const double residual_degrees_of_freedom = error_df - 1.0;
    for (auto& observation : result.observations) {
        const double one_minus_leverage = std::max(
            1.0e-15, 1.0 - observation.leverage);
        const double deleted_residual = observation.residual / one_minus_leverage;
        const double deleted_error_sum_of_squares =
            result.error_sum_of_squares
            - observation.residual * observation.residual / one_minus_leverage;
        const double deleted_mean_square =
            residual_degrees_of_freedom > 0.0
                ? std::max(0.0, deleted_error_sum_of_squares
                    / residual_degrees_of_freedom)
                : 0.0;
        const double deleted_scale = std::sqrt(deleted_mean_square);
        observation.deleted_studentized_residual = deleted_scale > 0.0
            ? deleted_residual / deleted_scale : 0.0;
    }
    result.press = 0.0;
    for (const auto& observation : result.observations) {
        const double deleted_residual = observation.residual
            / std::max(1.0e-15, 1.0 - observation.leverage);
        result.press += deleted_residual * deleted_residual;
    }
    result.predicted_r_squared = result.total_sum_of_squares > 0.0
        ? 1.0 - result.press / result.total_sum_of_squares : 0.0;

    // The input row order is the observation order used by Durbin-Watson.
    double successive_difference_sum = 0.0;
    for (std::size_t index = 1; index < result.observations.size(); ++index) {
        const double difference = result.observations[index].residual
            - result.observations[index - 1].residual;
        successive_difference_sum += difference * difference;
    }
    result.durbin_watson =
        result.error_sum_of_squares > 0.0
            ? successive_difference_sum / result.error_sum_of_squares : 0.0;
    result.diagnostics_summary.durbin_watson = result.durbin_watson;

    std::vector<double> residuals;
    residuals.reserve(result.observations.size());
    for (const auto& observation : result.observations) {
        residuals.push_back(observation.residual);
    }
    result.diagnostics_summary.residual_normality = normality_test(residuals);

    const double observation_count =
        static_cast<double>(result.observation_count);
    const double parameter_count_value = static_cast<double>(parameter_count);
    const double leverage_threshold = 2.0 * parameter_count_value
        / observation_count;
    const double cooks_threshold = 4.0 / observation_count;
    const double dfits_threshold = 2.0 * std::sqrt(
        parameter_count_value / observation_count);
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        auto& observation = result.observations[index];
        observation.is_outlier = std::abs(observation.deleted_studentized_residual)
            > 3.0;
        observation.is_high_leverage = observation.leverage > leverage_threshold;
        observation.is_influential = observation.cooks_distance > cooks_threshold
            || std::abs(observation.dfits) > dfits_threshold;
        if (observation.is_outlier) {
            observation.diagnostic_flags.push_back("outlier");
            ++result.diagnostics_summary.outlier_count;
        }
        if (observation.is_high_leverage) {
            observation.diagnostic_flags.push_back("high_leverage");
            ++result.diagnostics_summary.high_leverage_count;
        }
        if (observation.is_influential) {
            observation.diagnostic_flags.push_back("influential");
            ++result.diagnostics_summary.influential_count;
        }
        if (!observation.diagnostic_flags.empty()) {
            result.diagnostics_summary.flagged_observations.push_back(index);
        }
    }
    return result;
}

}  // namespace datalab::domain::statistics
