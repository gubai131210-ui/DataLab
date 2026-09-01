#include "domain/statistics/manova_one_way.h"

#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

using Matrix = std::vector<std::vector<double>>;

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const std::string& message)
{
    diagnostics.push_back({severity, std::string{code}, message, {}});
}

bool invert_matrix(Matrix matrix, Matrix& inverse)
{
    const std::size_t n = matrix.size();
    if (n == 0) {
        return false;
    }
    inverse.assign(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        inverse[i][i] = 1.0;
    }
    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivot = col;
        for (std::size_t row = col + 1; row < n; ++row) {
            if (std::abs(matrix[row][col]) > std::abs(matrix[pivot][col])) {
                pivot = row;
            }
        }
        if (std::abs(matrix[pivot][col]) < 1.0e-12) {
            return false;
        }
        std::swap(matrix[pivot], matrix[col]);
        std::swap(inverse[pivot], inverse[col]);
        const double div = matrix[col][col];
        for (std::size_t j = 0; j < n; ++j) {
            matrix[col][j] /= div;
            inverse[col][j] /= div;
        }
        for (std::size_t row = 0; row < n; ++row) {
            if (row == col) {
                continue;
            }
            const double factor = matrix[row][col];
            for (std::size_t j = 0; j < n; ++j) {
                matrix[row][j] -= factor * matrix[col][j];
                inverse[row][j] -= factor * inverse[col][j];
            }
        }
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

Matrix outer_product(const std::vector<double>& left, const std::vector<double>& right)
{
    Matrix result(left.size(), std::vector<double>(right.size(), 0.0));
    for (std::size_t i = 0; i < left.size(); ++i) {
        for (std::size_t j = 0; j < right.size(); ++j) {
            result[i][j] = left[i] * right[j];
        }
    }
    return result;
}

Matrix add_matrices(const Matrix& a, const Matrix& b)
{
    Matrix result = a;
    for (std::size_t i = 0; i < result.size(); ++i) {
        for (std::size_t j = 0; j < result[i].size(); ++j) {
            result[i][j] += b[i][j];
        }
    }
    return result;
}

bool symmetric_eigen(
    Matrix input,
    std::vector<double>& eigenvalues,
    Matrix& eigenvectors)
{
    const std::size_t dimension = input.size();
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
                if (std::abs(input[row][column]) > largest) {
                    largest = std::abs(input[row][column]);
                    pivot_row = row;
                    pivot_column = column;
                }
            }
        }
        if (largest <= epsilon) {
            break;
        }
        const double diagonal_difference =
            input[pivot_column][pivot_column] - input[pivot_row][pivot_row];
        const double angle = 0.5 * std::atan2(
            2.0 * input[pivot_row][pivot_column], diagonal_difference);
        const double cosine = std::cos(angle);
        const double sine = std::sin(angle);
        for (std::size_t index = 0; index < dimension; ++index) {
            if (index == pivot_row || index == pivot_column) {
                continue;
            }
            const double row_value = input[index][pivot_row];
            const double column_value = input[index][pivot_column];
            input[index][pivot_row] = cosine * row_value - sine * column_value;
            input[pivot_row][index] = input[index][pivot_row];
            input[index][pivot_column] = sine * row_value + cosine * column_value;
            input[pivot_column][index] = input[index][pivot_column];
        }
        const double pivot_diagonal = input[pivot_row][pivot_row];
        const double column_diagonal = input[pivot_column][pivot_column];
        const double off_diagonal = input[pivot_row][pivot_column];
        input[pivot_row][pivot_row] = cosine * cosine * pivot_diagonal
            - 2.0 * sine * cosine * off_diagonal + sine * sine * column_diagonal;
        input[pivot_column][pivot_column] = sine * sine * pivot_diagonal
            + 2.0 * sine * cosine * off_diagonal + cosine * cosine * column_diagonal;
        input[pivot_row][pivot_column] = 0.0;
        input[pivot_column][pivot_row] = 0.0;
        for (std::size_t index = 0; index < dimension; ++index) {
            const double row_value = eigenvectors[index][pivot_row];
            const double column_value = eigenvectors[index][pivot_column];
            eigenvectors[index][pivot_row] = cosine * row_value - sine * column_value;
            eigenvectors[index][pivot_column] = sine * row_value + cosine * column_value;
        }
    }
    eigenvalues.resize(dimension);
    for (std::size_t index = 0; index < dimension; ++index) {
        eigenvalues[index] = std::max(0.0, input[index][index]);
    }
    std::vector<std::size_t> order(dimension);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](std::size_t left, std::size_t right) {
        return eigenvalues[left] > eigenvalues[right];
    });
    std::vector<double> sorted_values(dimension);
    for (std::size_t component = 0; component < dimension; ++component) {
        sorted_values[component] = eigenvalues[order[component]];
    }
    eigenvalues = std::move(sorted_values);
    return true;
}

ManovaTestRow make_test(
    const std::string& name,
    double value,
    double f_value,
    double num_df,
    double den_df,
    bool approximate)
{
    ManovaTestRow row;
    row.test_name = name;
    row.value = value;
    row.f_statistic = f_value;
    row.numerator_df = num_df;
    row.denominator_df = den_df;
    row.approximate = approximate;
    if (den_df > 0.0 && num_df > 0.0 && f_value > 0.0) {
        row.p_value = f_right_tail(f_value, num_df, den_df);
    }
    return row;
}

}  // namespace

ManovaOneWayResult manova_one_way_analyze(
    const std::vector<std::vector<double>>& responses,
    const std::vector<std::string>& factor,
    const std::vector<std::size_t>& source_rows,
    const ManovaOneWayOptions& options)
{
    ManovaOneWayResult result;
    if (responses.empty() || factor.empty() || responses.size() != factor.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "length_mismatch", "响应矩阵与因子列长度须一致。");
        return result;
    }
    const std::size_t p = responses.front().size();
    if (p < 2 || p > 4) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_response_count", "响应变量数须为 2～4。");
        return result;
    }

    std::map<std::string, std::vector<std::vector<double>>> groups;
    for (std::size_t index = 0; index < responses.size(); ++index) {
        if (factor[index].empty()) {
            continue;
        }
        if (responses[index].size() != p) {
            continue;
        }
        bool valid = true;
        for (double value : responses[index]) {
            if (!std::isfinite(value)) {
                valid = false;
                break;
            }
        }
        if (!valid) {
            continue;
        }
        groups[factor[index]].push_back(responses[index]);
        const std::size_t source = index < source_rows.size() ? source_rows[index] : index;
        result.observation_source_rows.push_back(source);
    }

    result.observation_count = result.observation_source_rows.size();
    result.response_count = p;
    result.group_count = groups.size();
    if (result.group_count < 2) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_groups", "因子至少需要 2 个组。");
        return result;
    }
    for (const auto& [group, rows] : groups) {
        if (rows.size() < 2) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "insufficient_group_size",
                           "每组至少需要 2 个观测（组 " + group + "）。");
            return result;
        }
    }

    std::vector<double> grand_mean(p, 0.0);
    for (const auto& [group, rows] : groups) {
        for (const auto& row : rows) {
            for (std::size_t j = 0; j < p; ++j) {
                grand_mean[j] += row[j];
            }
        }
    }
    for (double& mean : grand_mean) {
        mean /= static_cast<double>(result.observation_count);
    }

    Matrix H(p, std::vector<double>(p, 0.0));
    Matrix E(p, std::vector<double>(p, 0.0));
    for (const auto& [group, rows] : groups) {
        std::vector<double> group_mean(p, 0.0);
        for (const auto& row : rows) {
            for (std::size_t j = 0; j < p; ++j) {
                group_mean[j] += row[j];
            }
        }
        for (double& mean : group_mean) {
            mean /= static_cast<double>(rows.size());
        }
        std::vector<double> diff(p);
        for (std::size_t j = 0; j < p; ++j) {
            diff[j] = group_mean[j] - grand_mean[j];
        }
        const Matrix outer = outer_product(diff, diff);
        for (std::size_t i = 0; i < p; ++i) {
            for (std::size_t j = 0; j < p; ++j) {
                H[i][j] += static_cast<double>(rows.size()) * outer[i][j];
            }
        }
        ManovaGroupMeanVector mean_row;
        mean_row.group = group;
        mean_row.count = rows.size();
        mean_row.means = group_mean;
        result.group_means.push_back(mean_row);

        for (const auto& row : rows) {
            std::vector<double> within(p);
            for (std::size_t j = 0; j < p; ++j) {
                within[j] = row[j] - group_mean[j];
            }
            const Matrix within_outer = outer_product(within, within);
            E = add_matrices(E, within_outer);
        }
    }

    Matrix e_inv;
    if (!invert_matrix(E, e_inv)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "singular_e_matrix", "组内 SSCP 矩阵 E 奇异，无法求逆。");
        return result;
    }
    const Matrix e_inv_h = multiply(e_inv, H);
    std::vector<double> eigenvalues;
    Matrix eigenvectors;
    if (!symmetric_eigen(e_inv_h, eigenvalues, eigenvectors)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "eigen_failed", "E^{-1}H 特征分解失败。");
        return result;
    }

    const double lambda_sum = std::accumulate(
        eigenvalues.cbegin(), eigenvalues.cend(), 0.0);
    double wilks = 1.0;
    double pillai = 0.0;
    for (double lambda : eigenvalues) {
        wilks *= 1.0 / (1.0 + lambda);
        pillai += lambda / (1.0 + lambda);
    }
    const double lawley = lambda_sum;
    const double roy = eigenvalues.empty() ? 0.0 : eigenvalues.front();

    const double g = static_cast<double>(result.group_count);
    const double n = static_cast<double>(result.observation_count);
    const double q = static_cast<double>(result.group_count - 1);
    const double s = std::min(p, static_cast<std::size_t>(result.group_count - 1));
    const double df1 = static_cast<double>(s * p);
    const double df2 = n - g - static_cast<double>(p) + 1.0;

    if (options.wilks && s > 0) {
        const double t = std::sqrt((df1 * df1 * df2 * df2 - 4.0) / (df1 * df1 + df2 * df2 - 5.0));
        const double df1_w = df1;
        const double df2_w = df2;
        const double f_w = ((1.0 - wilks) / wilks) * (df2_w / df1_w);
        result.test_rows.push_back(make_test(
            "Wilks' Lambda", wilks, f_w, df1_w, df2_w, s > 2));
    }
    if (options.pillai) {
        const double f_p = (pillai / (1.0 - pillai)) * (df2 / df1);
        result.test_rows.push_back(make_test(
            "Pillai's Trace", pillai, f_p, df1, df2, s > 2));
    }
    if (options.lawley_hotelling) {
        const double f_lh = (lawley / static_cast<double>(s)) * (df2 / df1);
        result.test_rows.push_back(make_test(
            "Lawley-Hotelling", lawley, f_lh, df1, df2, s > 2));
    }
    if (options.roy) {
        const double f_r = roy * df2 / df1;
        result.test_rows.push_back(make_test(
            "Roy's Largest Root", roy, f_r, df1, df2, s > 2));
    }

    const double total_lambda = std::accumulate(
        eigenvalues.cbegin(), eigenvalues.cend(), 0.0);
    for (std::size_t i = 0; i < eigenvalues.size(); ++i) {
        ManovaEigenRow row;
        row.index = i + 1;
        row.eigenvalue = eigenvalues[i];
        row.proportion = total_lambda > 0.0 ? eigenvalues[i] / total_lambda : 0.0;
        result.eigenvalues.push_back(row);
    }

    return result;
}

}  // namespace datalab::domain::statistics
