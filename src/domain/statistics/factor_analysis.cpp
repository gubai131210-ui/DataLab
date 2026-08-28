#include "domain/statistics/factor_analysis.h"

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

bool finite_row(const std::vector<double>& row)
{
    return std::all_of(row.cbegin(), row.cend(), [](double value) {
        return std::isfinite(value);
    });
}

bool symmetric_eigen_decomposition(
    const Matrix& input,
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
    constexpr double epsilon = 1.0e-10;
    constexpr std::size_t max_iterations = 100;
    for (std::size_t iteration = 0; iteration < max_iterations; ++iteration) {
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
        const double cross = matrix[pivot_row][pivot_column];
        matrix[pivot_row][pivot_row] =
            cosine * cosine * pivot_diagonal
            + sine * sine * column_diagonal
            - 2.0 * cosine * sine * cross;
        matrix[pivot_column][pivot_column] =
            sine * sine * pivot_diagonal
            + cosine * cosine * column_diagonal
            + 2.0 * cosine * sine * cross;
        matrix[pivot_row][pivot_column] = 0.0;
        matrix[pivot_column][pivot_row] = 0.0;
        for (std::size_t index = 0; index < dimension; ++index) {
            const double row_value = eigenvectors[index][pivot_row];
            const double column_value = eigenvectors[index][pivot_column];
            eigenvectors[index][pivot_row] =
                cosine * row_value - sine * column_value;
            eigenvectors[index][pivot_column] =
                sine * row_value + cosine * column_value;
        }
    }
    eigenvalues.resize(dimension);
    for (std::size_t index = 0; index < dimension; ++index) {
        eigenvalues[index] = matrix[index][index];
    }
    return true;
}

Matrix varimax_rotation(Matrix loadings, std::size_t max_iter = 25)
{
    const std::size_t variables = loadings.size();
    const std::size_t factors = loadings.empty() ? 0 : loadings.front().size();
    if (factors < 2) {
        return loadings;
    }
    Matrix rotation(factors, std::vector<double>(factors, 0.0));
    for (std::size_t index = 0; index < factors; ++index) {
        rotation[index][index] = 1.0;
    }
    for (std::size_t iter = 0; iter < max_iter; ++iter) {
        Matrix gradient(factors, std::vector<double>(factors, 0.0));
        for (std::size_t p = 0; p < factors; ++p) {
            for (std::size_t q = p + 1; q < factors; ++q) {
                double numerator = 0.0;
                double denominator = 0.0;
                for (std::size_t row = 0; row < variables; ++row) {
                    double u = 0.0;
                    double v = 0.0;
                    for (std::size_t k = 0; k < factors; ++k) {
                        u += loadings[row][k] * rotation[k][p];
                        v += loadings[row][k] * rotation[k][q];
                    }
                    numerator += u * v * (u * u - v * v);
                    denominator += u * u + v * v;
                }
                if (denominator > 1.0e-12) {
                    const double angle = 0.25 * std::atan2(numerator, denominator);
                    const double cosine = std::cos(angle);
                    const double sine = std::sin(angle);
                    for (std::size_t k = 0; k < factors; ++k) {
                        const double rp = rotation[k][p];
                        const double rq = rotation[k][q];
                        rotation[k][p] = cosine * rp - sine * rq;
                        rotation[k][q] = sine * rp + cosine * rq;
                    }
                }
            }
        }
    }
    Matrix rotated(variables, std::vector<double>(factors, 0.0));
    for (std::size_t row = 0; row < variables; ++row) {
        for (std::size_t col = 0; col < factors; ++col) {
            for (std::size_t k = 0; k < factors; ++k) {
                rotated[row][col] += loadings[row][k] * rotation[k][col];
            }
        }
    }
    return rotated;
}

}  // namespace

FactorAnalysisResult factor_analysis_extract(
    const std::vector<std::vector<double>>& rows,
    const std::vector<std::string>& variable_names,
    const std::vector<std::size_t>&,
    const FactorAnalysisOptions& options)
{
    FactorAnalysisResult result;
    result.variable_names = variable_names;
    result.varimax_applied = options.varimax_rotation;

    std::vector<std::vector<double>> valid_rows;
    for (const auto& row : rows) {
        if (finite_row(row)) {
            valid_rows.push_back(row);
        }
    }
    result.observation_count = valid_rows.size();
    result.variable_count = variable_names.size();
    if (result.variable_count < 3 || valid_rows.size() < 3) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_factor_data",
                       "因子分析需要至少 3 个变量与 3 个完整观测。");
        return result;
    }

    std::vector<double> means(result.variable_count, 0.0);
    std::vector<double> scales(result.variable_count, 1.0);
    for (std::size_t col = 0; col < result.variable_count; ++col) {
        double sum = 0.0;
        for (const auto& row : valid_rows) {
            sum += col < row.size() ? row[col] : 0.0;
        }
        means[col] = sum / static_cast<double>(valid_rows.size());
        double var = 0.0;
        for (const auto& row : valid_rows) {
            const double d = (col < row.size() ? row[col] : 0.0) - means[col];
            var += d * d;
        }
        scales[col] = valid_rows.size() > 1
            ? std::sqrt(var / static_cast<double>(valid_rows.size() - 1)) : 1.0;
        if (scales[col] < 1.0e-12) {
            scales[col] = 1.0;
        }
    }

    Matrix correlation(result.variable_count,
                     std::vector<double>(result.variable_count, 0.0));
    for (std::size_t i = 0; i < result.variable_count; ++i) {
        for (std::size_t j = 0; j < result.variable_count; ++j) {
            double sum = 0.0;
            for (const auto& row : valid_rows) {
                const double xi = (i < row.size() ? row[i] : 0.0) - means[i];
                const double xj = (j < row.size() ? row[j] : 0.0) - means[j];
                sum += xi * xj;
            }
            correlation[i][j] = sum
                / (static_cast<double>(valid_rows.size() - 1) * scales[i] * scales[j]);
        }
    }

    Matrix eigenvectors;
    if (!symmetric_eigen_decomposition(correlation, result.eigenvalues, eigenvectors)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "eigen_decomposition_failed", "相关阵特征分解失败。");
        return result;
    }

    std::vector<std::size_t> order(result.eigenvalues.size());
    for (std::size_t i = 0; i < order.size(); ++i) {
        order[i] = i;
    }
    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return result.eigenvalues[a] > result.eigenvalues[b];
    });

    std::size_t retain = options.factor_count;
    if (retain == 0 && options.use_kaiser_rule) {
        retain = 0;
        for (double value : result.eigenvalues) {
            if (value > 1.0) {
                ++retain;
            }
        }
    }
    if (retain == 0) {
        retain = std::min<std::size_t>(2, result.variable_count);
    }
    retain = std::min(retain, result.variable_count);
    result.retained_factor_count = retain;

    Matrix loadings(result.variable_count, std::vector<double>(retain, 0.0));
    for (std::size_t row = 0; row < result.variable_count; ++row) {
        for (std::size_t f = 0; f < retain; ++f) {
            const std::size_t eigen_index = order[f];
            loadings[row][f] =
                std::sqrt(std::max(0.0, result.eigenvalues[eigen_index]))
                * eigenvectors[row][eigen_index];
        }
    }
    if (options.varimax_rotation) {
        loadings = varimax_rotation(loadings);
    }

    const double total_variance = std::accumulate(
        result.eigenvalues.cbegin(), result.eigenvalues.cend(), 0.0);
    double cumulative = 0.0;
    for (std::size_t f = 0; f < retain; ++f) {
        const std::size_t eigen_index = order[f];
        FactorVarianceRow row;
        row.factor_index = f + 1;
        row.eigenvalue = result.eigenvalues[eigen_index];
        row.percent_variance = total_variance > 0.0
            ? 100.0 * row.eigenvalue / total_variance : 0.0;
        cumulative += row.percent_variance;
        row.cumulative_percent = cumulative;
        result.variance_explained.push_back(row);
    }

    for (std::size_t row = 0; row < result.variable_count; ++row) {
        FactorLoadingRow loading_row;
        loading_row.variable = row < variable_names.size()
            ? variable_names[row] : ("V" + std::to_string(row + 1));
        loading_row.loadings = loadings[row];
        double comm = 0.0;
        for (double value : loadings[row]) {
            comm += value * value;
        }
        loading_row.communality = comm;
        result.loadings_table.push_back(loading_row);
    }

    return result;
}

}  // namespace datalab::domain::statistics
