#include "domain/statistics/pca.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

using Matrix = std::vector<std::vector<double>>;

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const char* message)
{
    diagnostics.push_back({severity, code, message});
}

double quantile(std::vector<double> values, double probability)
{
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const double position = probability
        * static_cast<double>(values.size() - 1);
    const std::size_t lower = static_cast<std::size_t>(position);
    const std::size_t upper = std::min(lower + 1, values.size() - 1);
    return values[lower] + (position - static_cast<double>(lower))
        * (values[upper] - values[lower]);
}

bool finite_row(const std::vector<double>& row)
{
    return std::all_of(row.cbegin(), row.cend(), [](double value) {
        return std::isfinite(value);
    });
}

bool symmetric_eigen_decomposition(
    const Matrix& input,
    std::size_t max_iterations,
    double tolerance,
    std::vector<double>& eigenvalues,
    Matrix& eigenvectors)
{
    const std::size_t dimension = input.size();
    Matrix matrix = input;
    eigenvectors.assign(dimension, std::vector<double>(dimension, 0.0));
    for (std::size_t index = 0; index < dimension; ++index) {
        eigenvectors[index][index] = 1.0;
    }

    if (dimension == 0) {
        eigenvalues.clear();
        return true;
    }
    const std::size_t iterations = std::max<std::size_t>(1, max_iterations);
    const double epsilon = std::max(tolerance, 1.0e-14);
    bool converged = false;
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        std::size_t pivot_row = 0;
        std::size_t pivot_column = 0;
        double largest = 0.0;
        for (std::size_t row = 0; row < dimension; ++row) {
            for (std::size_t column = row + 1; column < dimension; ++column) {
                if (std::abs(matrix[row][column]) > largest) {
                    largest = std::abs(matrix[row][column]);
                    pivot_row = row;
                    pivot_column = column;
                }
            }
        }
        if (largest <= epsilon) {
            converged = true;
            break;
        }

        const double diagonal_difference =
            matrix[pivot_column][pivot_column] - matrix[pivot_row][pivot_row];
        const double angle = 0.5 * std::atan2(
            2.0 * matrix[pivot_row][pivot_column], diagonal_difference);
        const double cosine = std::cos(angle);
        const double sine = std::sin(angle);
        for (std::size_t index = 0; index < dimension; ++index) {
            if (index == pivot_row || index == pivot_column) {
                continue;
            }
            const double row_value = matrix[index][pivot_row];
            const double column_value = matrix[index][pivot_column];
            matrix[index][pivot_row] = cosine * row_value - sine * column_value;
            matrix[pivot_row][index] = matrix[index][pivot_row];
            matrix[index][pivot_column] = sine * row_value + cosine * column_value;
            matrix[pivot_column][index] = matrix[index][pivot_column];
        }
        const double pivot_diagonal = matrix[pivot_row][pivot_row];
        const double column_diagonal = matrix[pivot_column][pivot_column];
        const double off_diagonal = matrix[pivot_row][pivot_column];
        matrix[pivot_row][pivot_row] = cosine * cosine * pivot_diagonal
            - 2.0 * sine * cosine * off_diagonal
            + sine * sine * column_diagonal;
        matrix[pivot_column][pivot_column] = sine * sine * pivot_diagonal
            + 2.0 * sine * cosine * off_diagonal
            + cosine * cosine * column_diagonal;
        matrix[pivot_row][pivot_column] = 0.0;
        matrix[pivot_column][pivot_row] = 0.0;
        for (std::size_t index = 0; index < dimension; ++index) {
            const double row_value = eigenvectors[index][pivot_row];
            const double column_value = eigenvectors[index][pivot_column];
            eigenvectors[index][pivot_row] = cosine * row_value - sine * column_value;
            eigenvectors[index][pivot_column] = sine * row_value + cosine * column_value;
        }
    }

    eigenvalues.resize(dimension);
    for (std::size_t index = 0; index < dimension; ++index) {
        eigenvalues[index] = matrix[index][index];
    }
    std::vector<std::size_t> order(dimension);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](std::size_t left, std::size_t right) {
        return eigenvalues[left] > eigenvalues[right];
    });
    std::vector<double> sorted_values(dimension);
    Matrix sorted_vectors(dimension, std::vector<double>(dimension, 0.0));
    for (std::size_t component = 0; component < dimension; ++component) {
        sorted_values[component] = std::max(0.0, eigenvalues[order[component]]);
        for (std::size_t variable = 0; variable < dimension; ++variable) {
            sorted_vectors[variable][component] = eigenvectors[variable][order[component]];
        }
    }
    eigenvalues = std::move(sorted_values);
    eigenvectors = std::move(sorted_vectors);
    return converged;
}

}  // namespace

PcaResult principal_component_analysis(
    const std::vector<std::vector<double>>& rows,
    const PcaOptions& options)
{
    PcaResult result;
    result.mode = options.mode;
    if (rows.empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "empty_input", "PCA 输入不能为空。");
        return result;
    }
    result.variable_count = rows.front().size();
    if (result.variable_count == 0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "empty_variables", "PCA 至少需要一个变量。");
        return result;
    }
    if (options.anomaly_quantile <= 0.0 || options.anomaly_quantile >= 1.0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_anomaly_quantile", "异常分位数必须大于 0 且小于 1。");
        return result;
    }
    for (std::size_t row = 0; row < rows.size(); ++row) {
        if (rows[row].size() != result.variable_count) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "inconsistent_dimensions", "PCA 输入各行必须具有相同的变量数。");
            return result;
        }
        if (!finite_row(rows[row])) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                           "missing_row_excluded", "包含缺失或非有限值的观测行已被排除。");
            continue;
        }
        result.valid_rows.push_back(row);
    }
    result.observation_count = result.valid_rows.size();
    result.means.assign(result.variable_count, 0.0);
    result.scales.assign(result.variable_count, 1.0);
    if (result.observation_count < 2) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_samples", "PCA 至少需要两个完整观测。");
        return result;
    }

    for (std::size_t row : result.valid_rows) {
        for (std::size_t variable = 0; variable < result.variable_count; ++variable) {
            result.means[variable] += rows[row][variable];
        }
    }
    for (double& mean : result.means) {
        mean /= static_cast<double>(result.observation_count);
    }

    Matrix centered(result.observation_count,
                    std::vector<double>(result.variable_count, 0.0));
    for (std::size_t observation = 0; observation < result.observation_count;
         ++observation) {
        const std::vector<double>& row = rows[result.valid_rows[observation]];
        for (std::size_t variable = 0; variable < result.variable_count; ++variable) {
            centered[observation][variable] = row[variable] - result.means[variable];
        }
    }
    for (std::size_t variable = 0; variable < result.variable_count; ++variable) {
        double sum_squares = 0.0;
        for (const std::vector<double>& row : centered) {
            sum_squares += row[variable] * row[variable];
        }
        result.scales[variable] = std::sqrt(
            sum_squares / static_cast<double>(result.observation_count - 1));
        if (result.scales[variable] <= 1.0e-12) {
            result.scales[variable] = 1.0;
            result.constant_columns.push_back(variable);
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                           "constant_column", "常量列的尺度被设为 1，其方差方向为 0。");
        }
    }

    Matrix transformed = centered;
    for (std::vector<double>& row : transformed) {
        for (std::size_t variable = 0; variable < result.variable_count; ++variable) {
            if (options.mode == PcaMode::standardized) {
                row[variable] /= result.scales[variable];
            }
        }
    }
    Matrix covariance(result.variable_count,
                      std::vector<double>(result.variable_count, 0.0));
    for (std::size_t first = 0; first < result.variable_count; ++first) {
        for (std::size_t second = first; second < result.variable_count; ++second) {
            double value = 0.0;
            for (const std::vector<double>& row : transformed) {
                value += row[first] * row[second];
            }
            value /= static_cast<double>(result.observation_count - 1);
            covariance[first][second] = value;
            covariance[second][first] = value;
        }
    }
    if (options.mode == PcaMode::standardized) {
        result.correlation_matrix = covariance;
    } else {
        result.covariance_matrix = covariance;
        result.correlation_matrix.assign(
            result.variable_count, std::vector<double>(result.variable_count, 0.0));
        for (std::size_t first = 0; first < result.variable_count; ++first) {
            for (std::size_t second = 0; second < result.variable_count; ++second) {
                const double denominator = result.scales[first] * result.scales[second];
                result.correlation_matrix[first][second] =
                    denominator > 0.0 ? covariance[first][second] / denominator : 0.0;
            }
        }
    }
    if (options.mode == PcaMode::standardized) {
        result.covariance_matrix.assign(
            result.variable_count, std::vector<double>(result.variable_count, 0.0));
        for (std::size_t first = 0; first < result.variable_count; ++first) {
            for (std::size_t second = 0; second < result.variable_count; ++second) {
                result.covariance_matrix[first][second] =
                    covariance[first][second] * result.scales[first] * result.scales[second];
            }
        }
    }

    Matrix eigenvectors;
    result.converged = symmetric_eigen_decomposition(
        covariance, options.max_iterations, options.tolerance,
        result.eigenvalues, eigenvectors);
    if (!result.converged) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "eigendecomposition_not_converged",
                       "对称特征分解在规定迭代次数内未收敛。");
        return result;
    }
    const std::size_t all_components = result.eigenvalues.size();
    result.retained_component_count = all_components;
    if (options.component_count > 0) {
        result.retained_component_count =
            std::min(options.component_count, result.retained_component_count);
    }
    const double total_variance = std::accumulate(
        result.eigenvalues.cbegin(), result.eigenvalues.cend(), 0.0);
    result.explained_variance_ratio.assign(all_components, 0.0);
    result.cumulative_explained_variance_ratio.assign(all_components, 0.0);
    double cumulative = 0.0;
    for (std::size_t component = 0; component < all_components; ++component) {
        if (total_variance > 0.0) {
            result.explained_variance_ratio[component] =
                result.eigenvalues[component] / total_variance;
        }
        cumulative += result.explained_variance_ratio[component];
        result.cumulative_explained_variance_ratio[component] = cumulative;
    }

    result.coefficients.assign(
        result.variable_count, std::vector<double>(result.retained_component_count, 0.0));
    result.loadings.assign(
        result.variable_count, std::vector<double>(result.retained_component_count, 0.0));
    for (std::size_t variable = 0; variable < result.variable_count; ++variable) {
        for (std::size_t component = 0; component < result.retained_component_count;
             ++component) {
            result.coefficients[variable][component] = eigenvectors[variable][component];
            result.loadings[variable][component] =
                eigenvectors[variable][component] * std::sqrt(result.eigenvalues[component]);
        }
    }
    result.scores.assign(
        result.observation_count, std::vector<double>(result.retained_component_count, 0.0));
    result.hotelling_t2.assign(result.observation_count, 0.0);
    result.q_residuals.assign(result.observation_count, 0.0);
    std::vector<double> t2_values;
    std::vector<double> q_values;
    for (std::size_t observation = 0; observation < result.observation_count;
         ++observation) {
        double squared_total = 0.0;
        for (double value : transformed[observation]) {
            squared_total += value * value;
        }
        double squared_reconstructed = 0.0;
        for (std::size_t component = 0; component < result.retained_component_count;
             ++component) {
            double score = 0.0;
            for (std::size_t variable = 0; variable < result.variable_count; ++variable) {
                score += transformed[observation][variable]
                    * eigenvectors[variable][component];
            }
            result.scores[observation][component] = score;
            if (result.eigenvalues[component] > 1.0e-12) {
                result.hotelling_t2[observation] += score * score
                    / result.eigenvalues[component];
            }
            squared_reconstructed += score * score;
        }
        result.q_residuals[observation] =
            std::max(0.0, squared_total - squared_reconstructed);
        t2_values.push_back(result.hotelling_t2[observation]);
        q_values.push_back(result.q_residuals[observation]);
    }
    result.hotelling_t2_limit = quantile(t2_values, options.anomaly_quantile);
    result.q_residual_limit = quantile(q_values, options.anomaly_quantile);
    result.hotelling_t2_anomaly.assign(result.observation_count, false);
    result.q_residual_anomaly.assign(result.observation_count, false);
    result.anomaly.assign(result.observation_count, false);
    for (std::size_t observation = 0; observation < result.observation_count;
         ++observation) {
        result.hotelling_t2_anomaly[observation] =
            result.hotelling_t2[observation] > result.hotelling_t2_limit;
        result.q_residual_anomaly[observation] =
            result.q_residuals[observation] > result.q_residual_limit;
        result.anomaly[observation] = result.hotelling_t2_anomaly[observation]
            || result.q_residual_anomaly[observation];
    }
    add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                   "empirical_anomaly_quantile",
                   "T²/Q 限使用样本经验分位，不是 Minitab T² 控制图 UCL，也不使用 "
                   "Jackson–Mudholkar 解析限。");
    return result;
}

PcaResult pca(
    const std::vector<std::vector<double>>& rows,
    const PcaOptions& options)
{
    return principal_component_analysis(rows, options);
}

}  // namespace datalab::domain::statistics
