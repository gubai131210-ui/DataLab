#include "domain/statistics/nominal_logistic.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

using Matrix = std::vector<std::vector<double>>;

constexpr double kProbFloor = 1.0e-15;
constexpr double kPivotTol = 1.0e-12;

double two_sided_normal_p(double z)
{
    return std::erfc(std::abs(z) / std::sqrt(2.0));
}

double normal_quantile(double p)
{
    double lower = -9.0;
    double upper = 9.0;
    for (int i = 0; i < 80; ++i) {
        const double mid = (lower + upper) / 2.0;
        const double cdf = 0.5 * std::erfc(-mid / std::sqrt(2.0));
        if (cdf < p) {
            lower = mid;
        } else {
            upper = mid;
        }
    }
    return (lower + upper) / 2.0;
}

double chi_square_right_tail(double value, double df)
{
    if (!(value >= 0.0) || !(df > 0.0)) {
        return 1.0;
    }
    const double shape = df / 2.0;
    const double x = value / 2.0;
    double term = 1.0 / shape;
    double sum = term;
    for (int i = 1; i < 200; ++i) {
        term *= x / (shape + static_cast<double>(i));
        sum += term;
        if (std::abs(term) < std::abs(sum) * 1.0e-14) {
            break;
        }
    }
    return std::clamp(1.0 - sum * std::exp(-x + shape * std::log(x)
        - std::lgamma(shape)), 0.0, 1.0);
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
    double scale = 0.0;
    for (const auto& row : matrix) {
        for (double v : row) {
            scale = std::max(scale, std::abs(v));
        }
    }
    const double pivot_limit = std::max(1.0, scale) * kPivotTol;
    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivot = col;
        for (std::size_t row = col + 1; row < n; ++row) {
            if (std::abs(matrix[row][col]) > std::abs(matrix[pivot][col])) {
                pivot = row;
            }
        }
        if (std::abs(matrix[pivot][col]) <= pivot_limit) {
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

void compute_probabilities(
    const std::vector<double>& params,
    std::size_t k_minus,
    std::size_t p,
    const std::vector<double>& x,
    std::vector<double>& pi)
{
    pi.assign(k_minus + 1, 0.0);
    double denom = 1.0;
    for (std::size_t logit = 0; logit < k_minus; ++logit) {
        double eta = params[logit * (p + 1)];
        for (std::size_t j = 0; j < p; ++j) {
            eta += params[logit * (p + 1) + 1 + j] * x[j];
        }
        const double e = std::exp(std::clamp(eta, -30.0, 30.0));
        pi[logit] = e;
        denom += e;
    }
    for (std::size_t logit = 0; logit < k_minus; ++logit) {
        pi[logit] /= denom;
    }
    pi[k_minus] = 1.0 / denom;
}

std::size_t param_index(std::size_t logit, std::size_t term, std::size_t p)
{
    return logit * (p + 1) + term;
}

void build_score_hessian(
    const std::vector<std::size_t>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<double>& params,
    std::size_t k_minus,
    std::size_t p,
    std::vector<double>& score,
    Matrix& hessian)
{
    const std::size_t dim = k_minus * (p + 1);
    score.assign(dim, 0.0);
    hessian.assign(dim, std::vector<double>(dim, 0.0));
    std::vector<double> pi;
    for (std::size_t i = 0; i < response.size(); ++i) {
        compute_probabilities(params, k_minus, p, predictors[i], pi);
        const std::size_t y = response[i];
        for (std::size_t k = 0; k < k_minus; ++k) {
            for (std::size_t t = 0; t <= p; ++t) {
                const double x_it = t == 0 ? 1.0 : predictors[i][t - 1];
                const std::size_t idx_k = param_index(k, t, p);
                score[idx_k] += x_it * ((y == k ? 1.0 : 0.0) - pi[k]);
                for (std::size_t l = 0; l < k_minus; ++l) {
                    for (std::size_t u = 0; u <= p; ++u) {
                        const double x_iu = u == 0 ? 1.0 : predictors[i][u - 1];
                        const std::size_t idx_l = param_index(l, u, p);
                        hessian[idx_k][idx_l] -= x_it * x_iu * pi[k]
                            * ((k == l ? 1.0 : 0.0) - pi[l]);
                    }
                }
            }
        }
    }
}

double log_likelihood(
    const std::vector<std::size_t>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<double>& params,
    std::size_t k_minus,
    std::size_t p)
{
    double ll = 0.0;
    std::vector<double> pi;
    for (std::size_t i = 0; i < response.size(); ++i) {
        compute_probabilities(params, k_minus, p, predictors[i], pi);
        ll += std::log(std::max(pi[response[i]], kProbFloor));
    }
    return ll;
}

}  // namespace

NominalLogisticResult fit_nominal_logistic(
    const std::vector<std::size_t>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& category_labels,
    const std::vector<std::string>& predictor_labels,
    std::size_t max_iterations,
    double tolerance,
    double confidence_level)
{
    NominalLogisticResult result;
    result.category_labels = category_labels;
    if (response.size() < 10 || predictors.size() != response.size()
        || predictors.empty() || predictors.front().empty()
        || category_labels.size() < 3) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "nominal_invalid",
            "名义 Logistic 需要 ≥10 观测、≥3 名义水平与 ≥1 预测变量。"});
        return result;
    }
    const std::size_t k = category_labels.size();
    const std::size_t p = predictors.front().size();
    const std::size_t k_minus = k - 1;
    result.category_count = k;
    result.predictor_count = p;
    result.logit_count = k_minus;
    result.observation_count = response.size();
    result.reference_category = category_labels.back();
    for (std::size_t y : response) {
        if (y >= k) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "nominal_bad_code",
                "响应类别编码越界。"});
            return result;
        }
    }

    const std::size_t dim = k_minus * (p + 1);
    std::vector<double> params(dim, 0.0);
    double previous_ll = -std::numeric_limits<double>::infinity();

    for (std::size_t iter = 0; iter < max_iterations; ++iter) {
        std::vector<double> score;
        Matrix hessian;
        build_score_hessian(response, predictors, params, k_minus, p, score, hessian);

        Matrix cov;
        if (!invert_matrix(hessian, cov)) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "nominal_singular",
                "信息矩阵不可逆；检查完全分离或样本量。"});
            return result;
        }

        std::vector<double> delta(dim, 0.0);
        for (std::size_t a = 0; a < dim; ++a) {
            for (std::size_t b = 0; b < dim; ++b) {
                delta[a] += cov[a][b] * score[b];
            }
        }

        const double ll0 = log_likelihood(response, predictors, params, k_minus, p);
        double step = 1.0;
        std::vector<double> trial = params;
        for (std::size_t a = 0; a < dim; ++a) {
            trial[a] += step * delta[a];
        }
        double trial_ll = log_likelihood(response, predictors, trial, k_minus, p);
        while (trial_ll < ll0 && step > 1.0e-4) {
            step *= 0.5;
            for (std::size_t a = 0; a < dim; ++a) {
                trial[a] = params[a] + step * delta[a];
            }
            trial_ll = log_likelihood(response, predictors, trial, k_minus, p);
        }
        params = trial;
        result.iteration_count = iter + 1;

        if (std::abs(trial_ll - previous_ll) < tolerance) {
            result.converged = true;
            result.log_likelihood = trial_ll;
            result.aic = -2.0 * trial_ll + 2.0 * static_cast<double>(dim);

            build_score_hessian(response, predictors, params, k_minus, p, score, hessian);
            if (!invert_matrix(hessian, cov)) {
                cov.assign(dim, std::vector<double>(dim, 0.0));
            }

            const double z_crit = normal_quantile(0.5 + confidence_level / 2.0);
            for (std::size_t logit = 0; logit < k_minus; ++logit) {
                const std::string logit_label =
                    category_labels[logit] + "/" + result.reference_category;
                for (std::size_t t = 0; t <= p; ++t) {
                    const std::size_t idx = param_index(logit, t, p);
                    NominalCoefficient coef;
                    coef.logit_label = logit_label;
                    coef.term = t == 0 ? "Constant"
                        : (t - 1 < predictor_labels.size()
                               ? predictor_labels[t - 1]
                               : ("X" + std::to_string(t)));
                    coef.estimate = params[idx];
                    coef.standard_error = std::sqrt(std::max(cov[idx][idx], 0.0));
                    coef.z_statistic = coef.standard_error > 0.0
                        ? coef.estimate / coef.standard_error : 0.0;
                    coef.p_value = two_sided_normal_p(coef.z_statistic);
                    coef.odds_ratio = std::exp(coef.estimate);
                    coef.confidence_lower = coef.estimate - z_crit * coef.standard_error;
                    coef.confidence_upper = coef.estimate + z_crit * coef.standard_error;
                    result.coefficients.push_back(coef);
                }
            }

            std::vector<double> null_params(dim, 0.0);
            const double ll_null =
                log_likelihood(response, predictors, null_params, k_minus, p);
            result.g_statistic = 2.0 * (trial_ll - ll_null);
            result.g_df = static_cast<double>(k_minus * p);
            if (result.g_df > 0.0 && result.g_statistic > 0.0) {
                result.g_p_value = chi_square_right_tail(result.g_statistic, result.g_df);
            }
            break;
        }
        previous_ll = trial_ll;
    }

    if (!result.converged) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "nominal_no_converge",
            "名义 Logistic 未在最大迭代内收敛。"});
    }
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "nominal_method",
        "基线类别 multinomial logit IRLS（解析得分/信息矩阵）。"});
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "nominal_scope",
        "广义 logit（基线=末水平）；非有序 Logistic；非 Minitab golden。"});
    return result;
}

}  // namespace datalab::domain::statistics
