#include "domain/statistics/mixed_effects_reml.h"

#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
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

Matrix build_fixed_design(
    const std::vector<std::string>& fixed_a,
    const std::vector<std::string>& fixed_b,
    const std::vector<double>& covariate,
    std::vector<std::string>& term_labels)
{
    const std::size_t n = fixed_a.empty() ? 0 : fixed_a.size();
    std::set<std::string> la;
    std::set<std::string> lb;
    for (std::size_t i = 0; i < n; ++i) {
        if (!fixed_a.empty()) {
            la.insert(fixed_a[i]);
        }
        if (!fixed_b.empty() && i < fixed_b.size()) {
            lb.insert(fixed_b[i]);
        }
    }
    std::vector<std::string> levels_a(la.cbegin(), la.cend());
    std::vector<std::string> levels_b(lb.cbegin(), lb.cend());
    std::sort(levels_a.begin(), levels_a.end());
    std::sort(levels_b.begin(), levels_b.end());

    term_labels.clear();
    term_labels.push_back("Intercept");
    std::size_t k = 1;
    for (std::size_t i = 1; i < levels_a.size(); ++i) {
        term_labels.push_back("FA[" + levels_a[i] + "]");
        ++k;
    }
    for (std::size_t i = 1; i < levels_b.size(); ++i) {
        term_labels.push_back("FB[" + levels_b[i] + "]");
        ++k;
    }
  if (!covariate.empty()) {
        term_labels.push_back("Covariate");
        ++k;
    }
    (void)k;

    Matrix X(n, std::vector<double>(term_labels.size(), 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        X[i][0] = 1.0;
        for (std::size_t j = 1; j < levels_a.size(); ++j) {
            if (!fixed_a.empty() && fixed_a[i] == levels_a[j]) {
                X[i][j] = 1.0;
            }
        }
        const std::size_t base_b = levels_a.size();
        for (std::size_t j = 1; j < levels_b.size(); ++j) {
            if (i < fixed_b.size() && fixed_b[i] == levels_b[j]) {
                X[i][base_b + j - 1] = 1.0;
            }
        }
        if (!covariate.empty() && i < covariate.size()) {
            X[i][term_labels.size() - 1] = covariate[i];
        }
    }
    return X;
}

double reml_log_likelihood(
    double sigma2,
    double sigma_u2,
    const std::vector<double>& y,
    const Matrix& X,
    const std::vector<std::size_t>& group_index,
    std::size_t group_count)
{
    const std::size_t n = y.size();
    const std::size_t p = X.empty() ? 0 : X.front().size();
    if (sigma2 <= 0.0 || sigma_u2 < 0.0 || n == 0) {
        return -1.0e30;
    }
    Matrix V(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        V[i][i] = sigma2;
        for (std::size_t j = 0; j < n; ++j) {
            if (group_index[i] == group_index[j]) {
                V[i][j] += sigma_u2;
            }
        }
    }
    Matrix V_inv;
    if (!invert_matrix(V, V_inv)) {
        return -1.0e30;
    }
    Matrix XtVinvX(p, std::vector<double>(p, 0.0));
    std::vector<double> XtVinvy(p, 0.0);
    double yVinvy = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            yVinvy += y[i] * V_inv[i][j] * y[j];
            for (std::size_t c = 0; c < p; ++c) {
                for (std::size_t d = 0; d < p; ++d) {
                    XtVinvX[c][d] += X[i][c] * V_inv[i][j] * X[j][d];
                }
                XtVinvy[c] += X[i][c] * V_inv[i][j] * y[j];
            }
        }
    }
    Matrix XtVinvX_inv;
    if (!invert_matrix(XtVinvX, XtVinvX_inv)) {
        return -1.0e30;
    }
    double quad = yVinvy;
    for (std::size_t c = 0; c < p; ++c) {
        for (std::size_t d = 0; d < p; ++d) {
            quad -= XtVinvy[c] * XtVinvX_inv[c][d] * XtVinvy[d];
        }
    }
    double log_det = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        log_det += std::log(std::max(V[i][i], 1.0e-12));
    }
    double log_xt = 0.0;
    for (std::size_t c = 0; c < p; ++c) {
        log_xt += std::log(std::max(XtVinvX[c][c], 1.0e-12));
    }
    return -0.5 * (static_cast<double>(n - p) * std::log(2.0 * 3.14159265358979323846)
        + log_det + quad + log_xt);
}

}  // namespace

MixedEffectsRemlResult mixed_effects_reml_analyze(
    const std::vector<double>& response,
    const std::vector<std::string>& random_factor,
    const std::vector<std::string>& fixed_a,
    const std::vector<std::string>& fixed_b,
    const std::vector<double>& covariate,
    const std::vector<std::size_t>& source_rows,
    const MixedEffectsRemlOptions& options)
{
    (void)options;
    MixedEffectsRemlResult result;
    if (response.empty() || random_factor.empty()
        || response.size() != random_factor.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "length_mismatch", "响应与随机因子列长度须一致。");
        return result;
    }

    std::vector<double> y;
    std::vector<std::string> rf;
    std::vector<std::string> fa;
    std::vector<std::string> fb;
    std::vector<double> cov;
    const bool has_fixed_a = !fixed_a.empty() && fixed_a.size() == response.size();
    const bool has_fixed_b = !fixed_b.empty() && fixed_b.size() == response.size();
    const bool has_cov = !covariate.empty() && covariate.size() == response.size();

    for (std::size_t i = 0; i < response.size(); ++i) {
        if (random_factor[i].empty() || !std::isfinite(response[i])) {
            continue;
        }
        if (has_fixed_a && fixed_a[i].empty()) {
            continue;
        }
        if (has_cov && !std::isfinite(covariate[i])) {
            continue;
        }
        y.push_back(response[i]);
        rf.push_back(random_factor[i]);
        if (has_fixed_a) {
            fa.push_back(fixed_a[i]);
        } else {
            fa.push_back("All");
        }
        if (has_fixed_b) {
            fb.push_back(fixed_b[i]);
        }
        if (has_cov) {
            cov.push_back(covariate[i]);
        }
        const std::size_t source = i < source_rows.size() ? source_rows[i] : i;
        result.observation_source_rows.push_back(source);
    }

    result.observation_count = y.size();
    if (result.observation_count < 4) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_observations", "有效观测不足。");
        return result;
    }

    std::map<std::string, std::size_t> group_map;
    std::vector<std::size_t> group_index;
    for (const auto& g : rf) {
        if (!group_map.count(g)) {
            group_map[g] = group_map.size();
        }
        group_index.push_back(group_map[g]);
    }
    result.random_level_count = group_map.size();
    if (result.random_level_count < 2) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_random_levels", "随机因子至少需要 2 个水平。");
        return result;
    }

    std::vector<std::string> term_labels;
    const Matrix X = build_fixed_design(fa, fb, cov, term_labels);

    double sigma2 = 1.0;
    double sigma_u2 = 0.5;
    double best_ll = reml_log_likelihood(sigma2, sigma_u2, y, X, group_index,
                                         result.random_level_count);
    for (int iter = 0; iter < 30; ++iter) {
        double best_local = best_ll;
        double best_s2 = sigma2;
        double best_su2 = sigma_u2;
        for (double ds2 : {-0.2, -0.1, 0.0, 0.1, 0.2}) {
            for (double dsu : {-0.2, -0.1, 0.0, 0.1, 0.2}) {
                const double trial_s2 = std::max(1.0e-6, sigma2 * std::exp(ds2));
                const double trial_su2 = std::max(0.0, sigma_u2 * std::exp(dsu));
                const double ll = reml_log_likelihood(
                    trial_s2, trial_su2, y, X, group_index, result.random_level_count);
                if (ll > best_local) {
                    best_local = ll;
                    best_s2 = trial_s2;
                    best_su2 = trial_su2;
                }
            }
        }
        sigma2 = best_s2;
        sigma_u2 = best_su2;
        if (best_local - best_ll < 1.0e-4) {
            result.converged = true;
            break;
        }
        best_ll = best_local;
    }

    result.residual_variance = sigma2;
    result.random_variance = sigma_u2;

    const std::size_t n = y.size();
    const std::size_t p = X.front().size();
    Matrix V(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        V[i][i] = sigma2;
        for (std::size_t j = 0; j < n; ++j) {
            if (group_index[i] == group_index[j]) {
                V[i][j] += sigma_u2;
            }
        }
    }
    Matrix V_inv;
    if (!invert_matrix(V, V_inv)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "singular_v_matrix", "V 矩阵奇异。");
        return result;
    }

    Matrix XtVinvX(p, std::vector<double>(p, 0.0));
    std::vector<double> XtVinvy(p, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            for (std::size_t c = 0; c < p; ++c) {
                for (std::size_t d = 0; d < p; ++d) {
                    XtVinvX[c][d] += X[i][c] * V_inv[i][j] * X[j][d];
                }
                XtVinvy[c] += X[i][c] * V_inv[i][j] * y[j];
            }
        }
    }
    Matrix XtVinvX_inv;
    if (!invert_matrix(XtVinvX, XtVinvX_inv)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "singular_fixed_model", "固定效应模型奇异。");
        return result;
    }

    std::vector<double> beta(p, 0.0);
    for (std::size_t c = 0; c < p; ++c) {
        for (std::size_t d = 0; d < p; ++d) {
            beta[c] += XtVinvX_inv[c][d] * XtVinvy[d];
        }
    }

    const double total_var = sigma2 + sigma_u2;
    MixedEffectsVarianceRow residual_row;
    residual_row.source = "Residual";
    residual_row.variance_component = sigma2;
    residual_row.percent_contribution = total_var > 0.0 ? 100.0 * sigma2 / total_var : 0.0;
    result.variance_components.push_back(residual_row);

    MixedEffectsVarianceRow random_row;
    random_row.source = "Random Factor";
    random_row.variance_component = sigma_u2;
    random_row.percent_contribution = total_var > 0.0 ? 100.0 * sigma_u2 / total_var : 0.0;
    result.variance_components.push_back(random_row);

    for (std::size_t c = 0; c < p; ++c) {
        MixedEffectsFixedRow row;
        row.term = term_labels[c];
        row.coefficient = beta[c];
        row.standard_error = std::sqrt(std::max(0.0, XtVinvX_inv[c][c]));
        if (row.standard_error > 0.0) {
            row.t_statistic = row.coefficient / row.standard_error;
            const double t_abs = std::abs(row.t_statistic);
            row.p_value = 2.0 * (1.0 - 0.5 * std::erfc(-t_abs / std::sqrt(2.0)));
        }
        result.fixed_effects.push_back(row);
    }

    if (!result.converged) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "reml_not_converged", "REML 迭代未完全收敛。");
    }

    return result;
}

}  // namespace datalab::domain::statistics
