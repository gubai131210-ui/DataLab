#include "domain/statistics/simple_correspondence.h"

#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <utility>

namespace datalab::domain::statistics {
namespace {

using Matrix = std::vector<std::vector<double>>;

double regularized_gamma_q(double shape, double value)
{
    if (!(shape > 0.0) || value < 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (value == 0.0) {
        return 1.0;
    }
    constexpr int max_iterations = 200;
    constexpr double epsilon = 3.0e-14;
    constexpr double tiny = 1.0e-300;
    if (value < shape + 1.0) {
        double term = 1.0 / shape;
        double sum = term;
        for (int index = 1; index <= max_iterations; ++index) {
            term *= value / (shape + static_cast<double>(index));
            sum += term;
            if (std::abs(term) < std::abs(sum) * epsilon) {
                break;
            }
        }
        return std::clamp(1.0 - std::exp(-value + shape * std::log(value) - std::lgamma(shape)) * sum,
                          0.0, 1.0);
    }
    double c = value + 1.0 - shape;
    double d = 1.0 / (tiny + c);
    double h = d;
    for (int index = 1; index <= max_iterations; ++index) {
        const double an = -static_cast<double>(index) * (static_cast<double>(index) - shape);
        c += 2.0;
        d = an * d + c;
        if (std::abs(d) < tiny) {
            d = tiny;
        }
        d = 1.0 / d;
        const double delta = c * d;
        h *= delta;
        if (std::abs(delta - 1.0) < epsilon) {
            break;
        }
    }
    return std::clamp(std::exp(-value + shape * std::log(value) - std::lgamma(shape)) * h,
                      0.0, 1.0);
}

double chi_square_upper_tail(double chi2, std::size_t df)
{
    if (df == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return regularized_gamma_q(static_cast<double>(df) / 2.0, chi2 / 2.0);
}

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const std::string& message)
{
    diagnostics.push_back({severity, std::string{code}, message, {}});
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
    for (std::size_t iteration = 0; iteration < 200; ++iteration) {
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
    return true;
}

Matrix multiply(const Matrix& left, const Matrix& right)
{
    const std::size_t n = left.size();
    const std::size_t m = right.empty() ? 0 : right.front().size();
    const std::size_t inner = right.size();
    Matrix product(n, std::vector<double>(m, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < m; ++j) {
            for (std::size_t k = 0; k < inner; ++k) {
                product[i][j] += left[i][k] * right[k][j];
            }
        }
    }
    return product;
}

Matrix transpose(const Matrix& input)
{
    if (input.empty()) {
        return {};
    }
    const std::size_t rows = input.size();
    const std::size_t cols = input.front().size();
    Matrix output(cols, std::vector<double>(rows, 0.0));
    for (std::size_t i = 0; i < rows; ++i) {
        for (std::size_t j = 0; j < cols; ++j) {
            output[j][i] = input[i][j];
        }
    }
    return output;
}

}  // namespace

SimpleCorrespondenceResult simple_correspondence_analyze(
    const std::vector<std::string>& row_variable,
    const std::vector<std::string>& column_variable,
    const std::vector<std::size_t>& source_rows,
    const SimpleCorrespondenceOptions& options)
{
    SimpleCorrespondenceResult result;
    if (row_variable.size() != column_variable.size() || row_variable.empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "sca_length_mismatch", "行变量与列变量长度不一致或为空。");
        return result;
    }
    result.observation_count = row_variable.size();
    if (!source_rows.empty()) {
        result.observation_source_rows = source_rows;
    } else {
        result.observation_source_rows.resize(row_variable.size());
        std::iota(result.observation_source_rows.begin(),
                  result.observation_source_rows.end(), 0);
    }

    std::vector<std::string> row_levels;
    std::vector<std::string> col_levels;
    std::map<std::string, std::size_t> row_index;
    std::map<std::string, std::size_t> col_index;
    for (std::size_t i = 0; i < row_variable.size(); ++i) {
        if (row_variable[i].empty() || column_variable[i].empty()) {
            continue;
        }
        if (row_index.find(row_variable[i]) == row_index.end()) {
            row_index[row_variable[i]] = row_levels.size();
            row_levels.push_back(row_variable[i]);
        }
        if (col_index.find(column_variable[i]) == col_index.end()) {
            col_index[column_variable[i]] = col_levels.size();
            col_levels.push_back(column_variable[i]);
        }
    }
    const std::size_t I = row_levels.size();
    const std::size_t J = col_levels.size();
    result.row_level_count = I;
    result.column_level_count = J;
    if (I < 2 || J < 2) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "sca_insufficient_levels",
                       "简单对应分析需要行、列各至少 2 个水平。");
        return result;
    }

    Matrix counts(I, std::vector<double>(J, 0.0));
    for (std::size_t i = 0; i < row_variable.size(); ++i) {
        if (row_variable[i].empty() || column_variable[i].empty()) {
            continue;
        }
        counts[row_index[row_variable[i]]][col_index[column_variable[i]]] += 1.0;
    }

    double n = 0.0;
    for (const auto& row : counts) {
        n += std::accumulate(row.cbegin(), row.cend(), 0.0);
    }
    if (n <= 0.0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "sca_empty_table", "列联表总计为零。");
        return result;
    }

    std::vector<double> row_sums(I, 0.0);
    std::vector<double> col_sums(J, 0.0);
    for (std::size_t i = 0; i < I; ++i) {
        row_sums[i] = std::accumulate(counts[i].cbegin(), counts[i].cend(), 0.0);
    }
    for (std::size_t j = 0; j < J; ++j) {
        for (std::size_t i = 0; i < I; ++i) {
            col_sums[j] += counts[i][j];
        }
    }

    double chi2 = 0.0;
    for (std::size_t i = 0; i < I; ++i) {
        for (std::size_t j = 0; j < J; ++j) {
            const double expected = row_sums[i] * col_sums[j] / n;
            if (expected > 0.0) {
                const double diff = counts[i][j] - expected;
                chi2 += diff * diff / expected;
            }
        }
    }
    result.chi_square = chi2;
    result.total_inertia = chi2 / n;
    result.chi_square_df = (I > 1 && J > 1) ? (I - 1) * (J - 1) : 0;
    if (result.chi_square_df > 0) {
        result.chi_square_p_value = chi_square_upper_tail(chi2, result.chi_square_df);
    }

    Matrix s_matrix(I, std::vector<double>(J, 0.0));
    for (std::size_t i = 0; i < I; ++i) {
        const double row_mass = row_sums[i] / n;
        const double dr_inv_sqrt = row_mass > 0.0 ? 1.0 / std::sqrt(row_mass) : 0.0;
        for (std::size_t j = 0; j < J; ++j) {
            const double col_mass = col_sums[j] / n;
            const double dc_inv_sqrt = col_mass > 0.0 ? 1.0 / std::sqrt(col_mass) : 0.0;
            const double residual = counts[i][j] / n - row_mass * (col_sums[j] / n);
            s_matrix[i][j] = dr_inv_sqrt * residual * dc_inv_sqrt;
        }
    }

    const Matrix s_transpose = transpose(s_matrix);
    const Matrix gram_row = multiply(s_matrix, s_transpose);
    const Matrix gram_col = multiply(s_transpose, s_matrix);

    std::vector<double> row_eigenvalues;
    Matrix row_eigenvectors;
    symmetric_eigen_decomposition(gram_row, row_eigenvalues, row_eigenvectors);

    std::vector<double> col_eigenvalues;
    Matrix col_eigenvectors;
    symmetric_eigen_decomposition(gram_col, col_eigenvalues, col_eigenvectors);

    std::vector<std::pair<double, std::size_t>> row_order;
    for (std::size_t k = 0; k < row_eigenvalues.size(); ++k) {
        row_order.emplace_back(std::max(0.0, row_eigenvalues[k]), k);
    }
    std::sort(row_order.begin(), row_order.end(),
              [](const auto& left, const auto& right) { return left.first > right.first; });

    const std::size_t max_components = std::min<std::size_t>(
        options.component_count, std::min(I, J) - 1);
    result.component_count = max_components;
    result.singular_values.resize(max_components);
    result.inertia_per_component.resize(max_components);
    for (std::size_t dim = 0; dim < max_components; ++dim) {
        const double eigen = row_order[dim].first;
        result.singular_values[dim] = std::sqrt(std::max(0.0, eigen));
        result.inertia_per_component[dim] = eigen;
    }

    std::vector<std::vector<double>> row_coords(I, std::vector<double>(max_components, 0.0));
    std::vector<std::vector<double>> col_coords(J, std::vector<double>(max_components, 0.0));
    for (std::size_t dim = 0; dim < max_components; ++dim) {
        const std::size_t k = row_order[dim].second;
        const double sigma = result.singular_values[dim];
        for (std::size_t i = 0; i < I; ++i) {
            const double mass = row_sums[i] / n;
            row_coords[i][dim] = mass > 0.0
                ? row_eigenvectors[i][k] * sigma / std::sqrt(mass) : 0.0;
        }
        for (std::size_t j = 0; j < J; ++j) {
            const double mass = col_sums[j] / n;
            col_coords[j][dim] = mass > 0.0
                ? col_eigenvectors[j][k] * sigma / std::sqrt(mass) : 0.0;
        }
    }

    if (options.include_row_contributions) {
        for (std::size_t i = 0; i < I; ++i) {
            CorrespondenceContributionRow row;
            row.label = row_levels[i];
            row.mass = row_sums[i] / n;
            row.inertia = 0.0;
            for (std::size_t dim = 0; dim < max_components; ++dim) {
                row.coordinates.push_back(row_coords[i][dim]);
            }
            for (std::size_t dim = 0; dim < max_components; ++dim) {
                const double sigma2 = result.inertia_per_component[dim];
                const double contr = sigma2 > 0.0
                    ? row_coords[i][dim] * row_coords[i][dim] * row.mass / sigma2 : 0.0;
                row.contributions.push_back(contr);
                row.inertia += contr * sigma2 / (row.mass > 0.0 ? row.mass : 1.0);
            }
            double quality = 0.0;
            double row_inertia = 0.0;
            for (std::size_t dim = 0; dim < row_eigenvalues.size(); ++dim) {
                const double sigma2 = std::max(0.0, row_eigenvalues[dim]);
                const double coord = row_eigenvectors[i][dim] * std::sqrt(sigma2)
                    / std::sqrt(row.mass > 0.0 ? row.mass : 1.0);
                row_inertia += coord * coord * row.mass;
            }
            for (std::size_t dim = 0; dim < max_components; ++dim) {
                quality += row.contributions[dim];
            }
            row.quality = row_inertia > 0.0 ? quality * result.total_inertia / row_inertia : 0.0;
            result.row_contributions.push_back(std::move(row));
        }
    }

    if (options.include_column_contributions) {
        for (std::size_t j = 0; j < J; ++j) {
            CorrespondenceContributionRow col;
            col.label = col_levels[j];
            col.mass = col_sums[j] / n;
            col.inertia = 0.0;
            for (std::size_t dim = 0; dim < max_components; ++dim) {
                col.coordinates.push_back(col_coords[j][dim]);
            }
            for (std::size_t dim = 0; dim < max_components; ++dim) {
                const double sigma2 = result.inertia_per_component[dim];
                const double contr = sigma2 > 0.0
                    ? col_coords[j][dim] * col_coords[j][dim] * col.mass / sigma2 : 0.0;
                col.contributions.push_back(contr);
            }
            double quality = 0.0;
            double col_inertia = 0.0;
            for (std::size_t dim = 0; dim < col_eigenvalues.size(); ++dim) {
                const double sigma2 = std::max(0.0, col_eigenvalues[dim]);
                const double coord = col_eigenvectors[j][dim] * std::sqrt(sigma2)
                    / std::sqrt(col.mass > 0.0 ? col.mass : 1.0);
                col_inertia += coord * coord * col.mass;
            }
            for (std::size_t dim = 0; dim < max_components; ++dim) {
                quality += col.contributions[dim];
            }
            col.quality = col_inertia > 0.0 ? quality * result.total_inertia / col_inertia : 0.0;
            result.column_contributions.push_back(std::move(col));
        }
    }

    return result;
}

}  // namespace datalab::domain::statistics
