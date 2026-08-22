#include "domain/statistics/ordinal_logistic.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

double logistic(double eta)
{
    if (eta >= 0.0) {
        return 1.0 / (1.0 + std::exp(-eta));
    }
    const double e = std::exp(eta);
    return e / (1.0 + e);
}

double two_sided_normal_p(double z)
{
    return std::erfc(std::abs(z) / std::sqrt(2.0));
}

bool invert_matrix(
    std::vector<std::vector<double>> matrix,
    std::vector<std::vector<double>>& inverse)
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

double log_likelihood(
    const std::vector<std::size_t>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<double>& theta,
    const std::vector<double>& beta)
{
    const std::size_t k_minus = theta.size();
    double ll = 0.0;
    for (std::size_t i = 0; i < response.size(); ++i) {
        const std::size_t y = response[i];
        double xb = 0.0;
        for (std::size_t j = 0; j < beta.size(); ++j) {
            xb += beta[j] * predictors[i][j];
        }
        double gamma_prev = 0.0;
        double gamma_curr = 1.0;
        if (y > 0) {
            gamma_prev = logistic(theta[y - 1] + xb);
        }
        if (y < k_minus) {
            gamma_curr = logistic(theta[y] + xb);
        }
        const double pi = std::max(gamma_curr - gamma_prev, 1.0e-15);
        ll += std::log(pi);
    }
    return ll;
}

}  // namespace

OrdinalLogisticResult fit_ordinal_logistic(
    const std::vector<std::size_t>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& category_labels,
    const std::vector<std::string>& predictor_labels,
    std::size_t max_iterations,
    double tolerance)
{
    OrdinalLogisticResult result;
    result.category_labels = category_labels;
    if (response.size() < 10 || predictors.size() != response.size()
        || predictors.empty() || predictors.front().empty()
        || category_labels.size() < 3) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "ordinal_invalid",
            "有序 Logistic 需要 ≥10 观测、≥3 有序水平与 ≥1 预测变量。"});
        return result;
    }
    const std::size_t k = category_labels.size();
    const std::size_t p = predictors.front().size();
    result.category_count = k;
    result.predictor_count = p;
    result.observation_count = response.size();
    for (std::size_t y : response) {
        if (y >= k) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "ordinal_bad_code",
                "响应类别编码越界。"});
            return result;
        }
    }

    std::vector<double> theta(k - 1, 0.0);
    for (std::size_t cut = 0; cut < k - 1; ++cut) {
        theta[cut] = -1.0 + 2.0 * static_cast<double>(cut) / static_cast<double>(k - 2);
    }
    std::vector<double> beta(p, 0.0);
    const std::size_t dim = (k - 1) + p;
    double previous_ll = -std::numeric_limits<double>::infinity();

    for (std::size_t iter = 0; iter < max_iterations; ++iter) {
        std::vector<double> score(dim, 0.0);
        std::vector<std::vector<double>> hessian(dim, std::vector<double>(dim, 0.0));
        // Numerical gradient + Hessian via central differences on LL.
        const double eps = 1.0e-5;
        auto pack = [&](std::vector<double>& th, std::vector<double>& be) {
            std::vector<double> v = th;
            v.insert(v.end(), be.begin(), be.end());
            return v;
        };
        auto unpack = [&](const std::vector<double>& v) {
            for (std::size_t i = 0; i < k - 1; ++i) {
                theta[i] = v[i];
            }
            for (std::size_t j = 0; j < p; ++j) {
                beta[j] = v[k - 1 + j];
            }
        };
        std::vector<double> params = pack(theta, beta);
        const double ll0 = log_likelihood(response, predictors, theta, beta);
        for (std::size_t i = 0; i < dim; ++i) {
            std::vector<double> plus = params;
            std::vector<double> minus = params;
            plus[i] += eps;
            minus[i] -= eps;
            unpack(plus);
            const double llp = log_likelihood(response, predictors, theta, beta);
            unpack(minus);
            const double llm = log_likelihood(response, predictors, theta, beta);
            score[i] = (llp - llm) / (2.0 * eps);
            for (std::size_t j = i; j < dim; ++j) {
                std::vector<double> pp = params;
                std::vector<double> pm = params;
                std::vector<double> mp = params;
                std::vector<double> mm = params;
                pp[i] += eps;
                pp[j] += eps;
                pm[i] += eps;
                pm[j] -= eps;
                mp[i] -= eps;
                mp[j] += eps;
                mm[i] -= eps;
                mm[j] -= eps;
                unpack(pp);
                const double llpp = log_likelihood(response, predictors, theta, beta);
                unpack(pm);
                const double llpm = log_likelihood(response, predictors, theta, beta);
                unpack(mp);
                const double llmp = log_likelihood(response, predictors, theta, beta);
                unpack(mm);
                const double llmm = log_likelihood(response, predictors, theta, beta);
                const double hij =
                    (llpp - llpm - llmp + llmm) / (4.0 * eps * eps);
                hessian[i][j] = hij;
                hessian[j][i] = hij;
            }
            unpack(params);
        }
        // Solve Hessian * delta = -score  (Newton on LL → use -H for maximization)
        std::vector<std::vector<double>> neg_h = hessian;
        for (std::size_t i = 0; i < dim; ++i) {
            for (std::size_t j = 0; j < dim; ++j) {
                neg_h[i][j] = -hessian[i][j];
            }
        }
        std::vector<std::vector<double>> inv;
        if (!invert_matrix(neg_h, inv)) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "ordinal_singular",
                "有序 Logistic 信息矩阵奇异。"});
            unpack(params);
            break;
        }
        std::vector<double> delta(dim, 0.0);
        for (std::size_t i = 0; i < dim; ++i) {
            for (std::size_t j = 0; j < dim; ++j) {
                delta[i] += inv[i][j] * score[j];
            }
        }
        for (std::size_t i = 0; i < dim; ++i) {
            params[i] += delta[i];
        }
        // Enforce ordered thresholds.
        for (std::size_t cut = 1; cut < k - 1; ++cut) {
            if (params[cut] < params[cut - 1] + 1.0e-4) {
                params[cut] = params[cut - 1] + 1.0e-4;
            }
        }
        unpack(params);
        const double ll = log_likelihood(response, predictors, theta, beta);
        result.iteration_count = iter + 1;
        result.log_likelihood = ll;
        if (std::abs(ll - previous_ll) < tolerance) {
            result.converged = true;
            break;
        }
        previous_ll = ll;
        (void)ll0;
    }
    if (!result.converged) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "ordinal_max_iter",
            "已达最大迭代次数。"});
    }

    // SE from final inverse information (-H)^{-1}
    std::vector<double> params;
    params.insert(params.end(), theta.begin(), theta.end());
    params.insert(params.end(), beta.begin(), beta.end());
    const double eps = 1.0e-5;
    std::vector<std::vector<double>> hessian(dim, std::vector<double>(dim, 0.0));
    auto unpack_local = [&](const std::vector<double>& v) {
        for (std::size_t i = 0; i < k - 1; ++i) {
            theta[i] = v[i];
        }
        for (std::size_t j = 0; j < p; ++j) {
            beta[j] = v[k - 1 + j];
        }
    };
    for (std::size_t i = 0; i < dim; ++i) {
        for (std::size_t j = i; j < dim; ++j) {
            std::vector<double> pp = params;
            std::vector<double> pm = params;
            std::vector<double> mp = params;
            std::vector<double> mm = params;
            pp[i] += eps;
            pp[j] += eps;
            pm[i] += eps;
            pm[j] -= eps;
            mp[i] -= eps;
            mp[j] += eps;
            mm[i] -= eps;
            mm[j] -= eps;
            unpack_local(pp);
            const double llpp = log_likelihood(response, predictors, theta, beta);
            unpack_local(pm);
            const double llpm = log_likelihood(response, predictors, theta, beta);
            unpack_local(mp);
            const double llmp = log_likelihood(response, predictors, theta, beta);
            unpack_local(mm);
            const double llmm = log_likelihood(response, predictors, theta, beta);
            const double hij = (llpp - llpm - llmp + llmm) / (4.0 * eps * eps);
            hessian[i][j] = hij;
            hessian[j][i] = hij;
        }
    }
    unpack_local(params);
    std::vector<std::vector<double>> neg_h = hessian;
    for (std::size_t i = 0; i < dim; ++i) {
        for (std::size_t j = 0; j < dim; ++j) {
            neg_h[i][j] = -hessian[i][j];
        }
    }
    std::vector<std::vector<double>> cov;
    const bool have_cov = invert_matrix(neg_h, cov);
    auto push_coef = [&](const std::string& name, double est, std::size_t index,
                         std::vector<OrdinalCoefficient>& out) {
        OrdinalCoefficient c;
        c.term = name;
        c.estimate = est;
        c.standard_error =
            have_cov ? std::sqrt(std::max(0.0, cov[index][index])) : 0.0;
        c.z_statistic =
            c.standard_error > 0.0 ? c.estimate / c.standard_error : 0.0;
        c.p_value = two_sided_normal_p(c.z_statistic);
        out.push_back(c);
    };
    for (std::size_t cut = 0; cut < k - 1; ++cut) {
        push_coef("Const(" + std::to_string(cut + 1) + ")", theta[cut], cut,
                  result.thresholds);
    }
    for (std::size_t j = 0; j < p; ++j) {
        const std::string name = j < predictor_labels.size()
            ? predictor_labels[j]
            : ("X" + std::to_string(j + 1));
        push_coef(name, beta[j], k - 1 + j, result.slopes);
    }
    result.aic = -2.0 * result.log_likelihood + 2.0 * static_cast<double>(dim);
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "ordinal_link",
        "比例优势 logit；非 Minitab golden。"});
    return result;
}

}  // namespace datalab::domain::statistics
