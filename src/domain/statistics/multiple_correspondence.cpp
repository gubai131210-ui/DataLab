#include "domain/statistics/multiple_correspondence.h"

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

}  // namespace

MultipleCorrespondenceResult multiple_correspondence_analyze(
    const std::vector<std::vector<std::string>>& categorical_columns,
    const std::vector<std::size_t>& source_rows,
    const MultipleCorrespondenceOptions& options)
{
    MultipleCorrespondenceResult result;
    if (categorical_columns.size() < 3 || categorical_columns.size() > 6) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "mca_variable_count",
                       "多重对应分析需要 3～6 列分类变量。");
        return result;
    }
    const std::size_t n = categorical_columns.front().size();
    for (const auto& column : categorical_columns) {
        if (column.size() != n) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "mca_length_mismatch", "分类列长度不一致。");
            return result;
        }
    }
    result.observation_count = n;
    result.variable_count = categorical_columns.size();
    if (!source_rows.empty()) {
        result.observation_source_rows = source_rows;
    } else {
        result.observation_source_rows.resize(n);
        std::iota(result.observation_source_rows.begin(),
                  result.observation_source_rows.end(), 0);
    }

    struct CategoryRef {
        std::size_t variable_index = 0;
        std::string label;
    };
    std::vector<CategoryRef> categories;
    std::map<std::string, std::size_t> category_index;
    for (std::size_t v = 0; v < categorical_columns.size(); ++v) {
        std::set<std::string> levels;
        for (const std::string& value : categorical_columns[v]) {
            if (!value.empty()) {
                levels.insert(value);
            }
        }
        for (const std::string& level : levels) {
            const std::string key = std::to_string(v) + ":" + level;
            category_index[key] = categories.size();
            categories.push_back({v, level});
        }
    }
    result.category_count = categories.size();
    if (categories.size() < 4) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "mca_insufficient_categories", "指示矩阵类别数不足。");
        return result;
    }

    Matrix indicator(n, std::vector<double>(categories.size(), 0.0));
    for (std::size_t row = 0; row < n; ++row) {
        for (std::size_t v = 0; v < categorical_columns.size(); ++v) {
            const std::string& value = categorical_columns[v][row];
            if (value.empty()) {
                continue;
            }
            const std::string key = std::to_string(v) + ":" + value;
            const auto found = category_index.find(key);
            if (found != category_index.end()) {
                indicator[row][found->second] = 1.0;
            }
        }
    }

    Matrix burt(categories.size(), std::vector<double>(categories.size(), 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t a = 0; a < categories.size(); ++a) {
            if (indicator[i][a] == 0.0) {
                continue;
            }
            for (std::size_t b = 0; b < categories.size(); ++b) {
                if (indicator[i][b] == 0.0) {
                    continue;
                }
                burt[a][b] += 1.0;
            }
        }
    }

    std::vector<double> masses(categories.size(), 0.0);
    for (std::size_t a = 0; a < categories.size(); ++a) {
        masses[a] = burt[a][a] / static_cast<double>(n);
    }

    Matrix normalized(categories.size(), std::vector<double>(categories.size(), 0.0));
    for (std::size_t a = 0; a < categories.size(); ++a) {
        const double ma = masses[a];
        const double inv_a = ma > 0.0 ? 1.0 / std::sqrt(ma) : 0.0;
        for (std::size_t b = 0; b < categories.size(); ++b) {
            const double mb = masses[b];
            const double inv_b = mb > 0.0 ? 1.0 / std::sqrt(mb) : 0.0;
            const double expected = ma * mb;
            const double residual = burt[a][b] / static_cast<double>(n) - expected;
            normalized[a][b] = inv_a * residual * inv_b;
        }
    }

    std::vector<double> eigenvalues;
    Matrix eigenvectors;
    symmetric_eigen_decomposition(normalized, eigenvalues, eigenvectors);

    std::vector<std::pair<double, std::size_t>> order;
    for (std::size_t k = 0; k < eigenvalues.size(); ++k) {
        order.emplace_back(std::max(0.0, eigenvalues[k]), k);
    }
    std::sort(order.begin(), order.end(),
              [](const auto& left, const auto& right) { return left.first > right.first; });

    result.total_inertia = std::accumulate(
        eigenvalues.cbegin(), eigenvalues.cend(), 0.0);
    result.chi_square = result.total_inertia * static_cast<double>(n);
    std::size_t df = 0;
    for (std::size_t v = 0; v < categorical_columns.size(); ++v) {
        std::set<std::string> levels;
        for (const std::string& value : categorical_columns[v]) {
            if (!value.empty()) {
                levels.insert(value);
            }
        }
        if (levels.size() > 1) {
            df += levels.size() - 1;
        }
    }
    result.chi_square_df = df > 0 ? df : 0;
    if (result.chi_square_df > 0) {
        result.chi_square_p_value =
            chi_square_upper_tail(result.chi_square, result.chi_square_df);
    }

    const std::size_t max_components = std::min(
        options.component_count, categories.size() > 1 ? categories.size() - 1 : 0);
    result.component_count = max_components;
    result.inertia_per_component.resize(max_components);
    for (std::size_t dim = 0; dim < max_components; ++dim) {
        result.inertia_per_component[dim] = order[dim].first;
    }

    if (options.include_column_contributions) {
        for (std::size_t a = 0; a < categories.size(); ++a) {
            CorrespondenceContributionRow row;
            row.label = "V" + std::to_string(categories[a].variable_index + 1)
                + ":" + categories[a].label;
            row.mass = masses[a];
            row.inertia = 0.0;
            for (std::size_t dim = 0; dim < max_components; ++dim) {
                const std::size_t k = order[dim].second;
                const double sigma2 = order[dim].first;
                const double coord = row.mass > 0.0
                    ? eigenvectors[a][k] * std::sqrt(sigma2) / std::sqrt(row.mass) : 0.0;
                row.coordinates.push_back(coord);
                const double contr = sigma2 > 0.0
                    ? coord * coord * row.mass / sigma2 : 0.0;
                row.contributions.push_back(contr);
            }
            double quality = 0.0;
            for (double contr : row.contributions) {
                quality += contr;
            }
            row.quality = quality;
            result.column_contributions.push_back(std::move(row));
        }
    }

    return result;
}

}  // namespace datalab::domain::statistics
