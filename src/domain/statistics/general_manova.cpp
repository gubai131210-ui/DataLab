#include "domain/statistics/general_manova.h"

#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <set>

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

Matrix subtract_matrices(const Matrix& a, const Matrix& b)
{
    Matrix result = a;
    for (std::size_t i = 0; i < result.size(); ++i) {
        for (std::size_t j = 0; j < result[i].size(); ++j) {
            result[i][j] -= b[i][j];
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

std::vector<ManovaTestRow> manova_tests_from_he(
    const Matrix& H,
    const Matrix& E,
    const GeneralManovaOptions& options,
    std::size_t q_effect,
    std::size_t n,
    std::size_t p)
{
    std::vector<ManovaTestRow> rows;
    Matrix e_inv;
    if (!invert_matrix(E, e_inv)) {
        return rows;
    }
    const Matrix e_inv_h = multiply(e_inv, H);
    std::vector<double> eigenvalues;
    Matrix eigenvectors;
    if (!symmetric_eigen(e_inv_h, eigenvalues, eigenvectors)) {
        return rows;
    }
    double wilks = 1.0;
    double pillai = 0.0;
    for (double lambda : eigenvalues) {
        wilks *= 1.0 / (1.0 + lambda);
        pillai += lambda / (1.0 + lambda);
    }
    const double lawley = std::accumulate(
        eigenvalues.cbegin(), eigenvalues.cend(), 0.0);
    const double roy = eigenvalues.empty() ? 0.0 : eigenvalues.front();
    const double s = std::min(p, q_effect);
    const double df1 = static_cast<double>(s * p);
    const double df2 = n - static_cast<double>(q_effect) - static_cast<double>(p) + 1.0;
    if (df2 <= 0.0 || s == 0) {
        return rows;
    }
    if (options.wilks && s > 0) {
        const double f_w = ((1.0 - wilks) / wilks) * (df2 / df1);
        rows.push_back(make_test("Wilks' Lambda", wilks, f_w, df1, df2, s > 2));
    }
    if (options.pillai) {
        const double f_p = (pillai / (1.0 - pillai)) * (df2 / df1);
        rows.push_back(make_test("Pillai's Trace", pillai, f_p, df1, df2, s > 2));
    }
    if (options.lawley_hotelling) {
        const double f_lh = (lawley / static_cast<double>(s)) * (df2 / df1);
        rows.push_back(make_test("Lawley-Hotelling", lawley, f_lh, df1, df2, s > 2));
    }
    if (options.roy) {
        const double f_r = roy * df2 / df1;
        rows.push_back(make_test("Roy's Largest Root", roy, f_r, df1, df2, s > 2));
    }
    return rows;
}

Matrix build_design(
    const std::vector<std::string>& factor_a,
    const std::vector<std::string>& factor_b,
    const std::vector<double>& covariate,
    bool include_interaction,
    std::vector<std::string>& column_labels,
    std::vector<std::vector<std::size_t>>& effect_column_groups)
{
    const std::size_t n = factor_a.size();
    std::set<std::string> levels_a;
    std::set<std::string> levels_b;
    for (std::size_t i = 0; i < n; ++i) {
        levels_a.insert(factor_a[i]);
        if (!factor_b.empty() && i < factor_b.size()) {
            levels_b.insert(factor_b[i]);
        }
    }
    std::vector<std::string> la(levels_a.cbegin(), levels_a.cend());
    std::vector<std::string> lb(levels_b.cbegin(), levels_b.cend());
    std::sort(la.begin(), la.end());
    std::sort(lb.begin(), lb.end());

    column_labels.clear();
    effect_column_groups.clear();
    column_labels.push_back("Intercept");
    std::size_t col = 1;

    std::vector<std::size_t> cols_a;
    for (std::size_t i = 1; i < la.size(); ++i) {
        column_labels.push_back("A[" + la[i] + "]");
        cols_a.push_back(col++);
    }
    if (!cols_a.empty()) {
        effect_column_groups.push_back(cols_a);
    }

    std::vector<std::size_t> cols_b;
    if (!lb.empty()) {
        for (std::size_t i = 1; i < lb.size(); ++i) {
            column_labels.push_back("B[" + lb[i] + "]");
            cols_b.push_back(col++);
        }
        if (!cols_b.empty()) {
            effect_column_groups.push_back(cols_b);
        }
    }

    std::vector<std::size_t> cols_ab;
    if (include_interaction && !lb.empty() && la.size() > 1 && lb.size() > 1) {
        for (std::size_t ia = 1; ia < la.size(); ++ia) {
            for (std::size_t ib = 1; ib < lb.size(); ++ib) {
                column_labels.push_back("AB[" + la[ia] + "×" + lb[ib] + "]");
                cols_ab.push_back(col++);
            }
        }
        if (!cols_ab.empty()) {
            effect_column_groups.push_back(cols_ab);
        }
    }

    std::vector<std::size_t> cols_cov;
    if (!covariate.empty()) {
        column_labels.push_back("Covariate");
        cols_cov.push_back(col++);
        effect_column_groups.push_back(cols_cov);
    }

    const std::size_t k = column_labels.size();
    Matrix X(n, std::vector<double>(k, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        X[i][0] = 1.0;
        for (std::size_t j = 1; j < la.size(); ++j) {
            if (factor_a[i] == la[j]) {
                X[i][j] = 1.0;
            }
        }
        if (!lb.empty() && i < factor_b.size()) {
            std::size_t base = la.size();
            for (std::size_t j = 1; j < lb.size(); ++j) {
                if (factor_b[i] == lb[j]) {
                    X[i][base + j - 1] = 1.0;
                }
            }
            if (include_interaction) {
                std::size_t ab_base = la.size() + lb.size() - 1;
                for (std::size_t ia = 1; ia < la.size(); ++ia) {
                    for (std::size_t ib = 1; ib < lb.size(); ++ib) {
                        if (factor_a[i] == la[ia] && factor_b[i] == lb[ib]) {
                            const std::size_t idx = ab_base
                                + (ia - 1) * (lb.size() - 1) + (ib - 1);
                            X[i][idx] = 1.0;
                        }
                    }
                }
            }
        }
        if (!covariate.empty() && i < covariate.size()) {
            X[i][k - 1] = covariate[i];
        }
    }
    return X;
}

Matrix sscp_from_projection(
    const Matrix& Y,
    const Matrix& X_full,
    const Matrix& X_reduced)
{
    const Matrix Xt = transpose(X_full);
    const Matrix Xr = transpose(X_reduced);
    Matrix XtX = multiply(Xt, X_full);
    Matrix XrXr = multiply(Xr, X_reduced);
    Matrix XtX_inv;
    Matrix XrXr_inv;
    if (!invert_matrix(XtX, XtX_inv) || !invert_matrix(XrXr, XrXr_inv)) {
        return Matrix(Y.front().size(), std::vector<double>(Y.front().size(), 0.0));
    }
    const Matrix P_full = multiply(multiply(X_full, XtX_inv), Xt);
    const Matrix P_red = multiply(multiply(X_reduced, XrXr_inv), Xr);
    const Matrix diff = subtract_matrices(P_full, P_red);
    const Matrix Yt = transpose(Y);
    const Matrix Yt_diff = multiply(Yt, diff);
  return multiply(Yt_diff, Y);
}

std::vector<std::string> effect_names(
    bool has_b,
    bool has_interaction,
    bool has_covariate)
{
    std::vector<std::string> names;
    names.push_back("Factor A");
    if (has_b) {
        names.push_back("Factor B");
    }
    if (has_interaction) {
        names.push_back("Interaction");
    }
    if (has_covariate) {
        names.push_back("Covariate");
    }
    return names;
}

}  // namespace

GeneralManovaResult general_manova_analyze(
    const std::vector<std::vector<double>>& responses,
    const std::vector<std::string>& factor_a,
    const std::vector<std::string>& factor_b,
    const std::vector<double>& covariate,
    const std::vector<std::size_t>& source_rows,
    const GeneralManovaOptions& options)
{
    GeneralManovaResult result;
    if (responses.empty() || factor_a.empty() || responses.size() != factor_a.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "length_mismatch", "响应矩阵与因子 A 列长度须一致。");
        return result;
    }
    const std::size_t p = responses.front().size();
    if (p < 2 || p > 4) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_response_count", "响应变量数须为 2～4。");
        return result;
    }
    const bool has_b = !factor_b.empty() && factor_b.size() == factor_a.size();
    const bool has_cov = !covariate.empty() && covariate.size() == factor_a.size();

    std::vector<std::vector<double>> Y;
    std::vector<std::string> fa;
    std::vector<std::string> fb;
    std::vector<double> cov;
    for (std::size_t index = 0; index < responses.size(); ++index) {
        if (factor_a[index].empty()) {
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
        if (has_b && (index >= factor_b.size() || factor_b[index].empty())) {
            continue;
        }
        if (has_cov) {
            if (!std::isfinite(covariate[index])) {
                continue;
            }
        }
        Y.push_back(responses[index]);
        fa.push_back(factor_a[index]);
        if (has_b) {
            fb.push_back(factor_b[index]);
        }
        if (has_cov) {
            cov.push_back(covariate[index]);
        }
        const std::size_t source = index < source_rows.size() ? source_rows[index] : index;
        result.observation_source_rows.push_back(source);
    }

    result.observation_count = result.observation_source_rows.size();
    result.response_count = p;
    if (result.observation_count < static_cast<std::size_t>(p + 2)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_observations", "有效观测不足。");
        return result;
    }

    std::vector<double> grand_mean(p, 0.0);
    for (const auto& row : Y) {
        for (std::size_t j = 0; j < p; ++j) {
            grand_mean[j] += row[j];
        }
    }
    for (double& mean : grand_mean) {
        mean /= static_cast<double>(result.observation_count);
    }
    for (auto& row : Y) {
        for (std::size_t j = 0; j < p; ++j) {
            row[j] -= grand_mean[j];
        }
    }

    std::vector<std::string> col_labels;
    std::vector<std::vector<std::size_t>> effect_groups;
    const Matrix X = build_design(
        fa, has_b ? fb : std::vector<std::string>{},
        has_cov ? cov : std::vector<double>{},
        options.include_interaction && has_b,
        col_labels, effect_groups);

    const Matrix Xt = transpose(X);
    Matrix XtX = multiply(Xt, X);
    Matrix XtX_inv;
    if (!invert_matrix(XtX, XtX_inv)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "singular_design", "设计矩阵奇异。");
        return result;
    }
    const Matrix P_full = multiply(multiply(X, XtX_inv), Xt);
    Matrix YtY(p, std::vector<double>(p, 0.0));
    for (const auto& row : Y) {
        YtY = add_matrices(YtY, outer_product(row, row));
    }
    Matrix Y_hat = multiply(P_full, Y);
    Matrix residual(p, std::vector<double>(p, 0.0));
    for (std::size_t i = 0; i < result.observation_count; ++i) {
        std::vector<double> diff(p);
        for (std::size_t j = 0; j < p; ++j) {
            diff[j] = Y[i][j] - Y_hat[i][j];
        }
        residual = add_matrices(residual, outer_product(diff, diff));
    }
    const Matrix E = residual;

    const std::vector<std::string> names = effect_names(
        has_b, options.include_interaction && has_b, has_cov);
    for (std::size_t effect = 0; effect < effect_groups.size(); ++effect) {
        std::vector<std::size_t> reduced_cols;
        for (std::size_t c = 0; c < col_labels.size(); ++c) {
            bool in_effect = false;
            for (std::size_t ec : effect_groups[effect]) {
                if (ec == c) {
                    in_effect = true;
                    break;
                }
            }
            if (!in_effect) {
                reduced_cols.push_back(c);
            }
        }
        Matrix X_red(result.observation_count,
                     std::vector<double>(reduced_cols.size(), 0.0));
        for (std::size_t i = 0; i < result.observation_count; ++i) {
            for (std::size_t j = 0; j < reduced_cols.size(); ++j) {
                X_red[i][j] = X[i][reduced_cols[j]];
            }
        }
        const Matrix H = sscp_from_projection(Y, X, X_red);
        GeneralManovaEffectTest effect_test;
        effect_test.effect_name = effect < names.size() ? names[effect] : "Effect";
        const std::size_t q = effect_groups[effect].size();
        effect_test.test_rows = manova_tests_from_he(
            H, E, options, q, result.observation_count, p);
        if (!effect_test.test_rows.empty()) {
            result.effect_tests.push_back(effect_test);
        }
    }

    std::map<std::string, std::vector<std::vector<double>>> cells;
    for (std::size_t i = 0; i < fa.size(); ++i) {
        std::string label = fa[i];
        if (has_b) {
            label += "|" + fb[i];
        }
        cells[label].push_back(Y[i]);
    }
    for (const auto& [label, rows] : cells) {
        GeneralManovaCellMean cell;
        cell.cell_label = label;
        cell.count = rows.size();
        cell.means.assign(p, 0.0);
        for (const auto& row : rows) {
            for (std::size_t j = 0; j < p; ++j) {
                cell.means[j] += row[j] + grand_mean[j];
            }
        }
        for (double& mean : cell.means) {
            mean /= static_cast<double>(cell.count);
        }
        result.cell_means.push_back(cell);
    }

    return result;
}

}  // namespace datalab::domain::statistics
