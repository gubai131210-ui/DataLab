#include "domain/statistics/gaussian_mixture_2.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <utility>

namespace datalab::domain::statistics {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kMinSd = 1e-8;
constexpr double kPreferDeltaBic = 2.0;

double normal_log_pdf(double x, double mean, double sd)
{
    const double s = std::max(sd, kMinSd);
    const double z = (x - mean) / s;
    return -0.5 * std::log(2.0 * kPi) - std::log(s) - 0.5 * z * z;
}

double normal_pdf(double x, double mean, double sd)
{
    return std::exp(normal_log_pdf(x, mean, sd));
}

int free_params_for_k(int k)
{
    // k means, k sds, (k-1) free weights
    return 3 * k - 1;
}

struct EmFit {
    bool ok = false;
    bool converged = false;
    int iterations = 0;
    double log_likelihood = 0.0;
    double bic = 0.0;
    std::vector<GaussianMixtureComponent> components;
    std::string fail_reason;
};

EmFit fit_em_k(
    const std::vector<double>& x,
    int k,
    int max_iterations,
    double tol)
{
    EmFit fit;
    const std::size_t n = x.size();
    if (k < 1 || n == 0) {
        fit.fail_reason = "invalid_k";
        return fit;
    }

    const double mean = std::accumulate(x.begin(), x.end(), 0.0) / static_cast<double>(n);
    double var = 0.0;
    for (double value : x) {
        const double d = value - mean;
        var += d * d;
    }
    var /= static_cast<double>(n);
    const double sd = std::sqrt(std::max(var, kMinSd * kMinSd));

    if (k == 1) {
        GaussianMixtureComponent c;
        c.weight = 1.0;
        c.mean = mean;
        c.sd = sd;
        fit.components.push_back(c);
        double ll = 0.0;
        for (double value : x) {
            ll += normal_log_pdf(value, mean, sd);
        }
        fit.log_likelihood = ll;
        fit.bic = -2.0 * ll + 2.0 * std::log(static_cast<double>(n));
        fit.ok = true;
        fit.converged = true;
        fit.iterations = 1;
        return fit;
    }

    std::vector<double> sorted = x;
    std::sort(sorted.begin(), sorted.end());
    std::vector<GaussianMixtureComponent> comps(static_cast<std::size_t>(k));
    for (int j = 0; j < k; ++j) {
        const double frac = (static_cast<double>(j) + 0.5) / static_cast<double>(k);
        const std::size_t idx = std::min(
            n - 1,
            static_cast<std::size_t>(frac * static_cast<double>(n)));
        comps[static_cast<std::size_t>(j)].mean = sorted[idx];
        comps[static_cast<std::size_t>(j)].sd = sd;
        comps[static_cast<std::size_t>(j)].weight = 1.0 / static_cast<double>(k);
    }
    if (!(comps.back().mean > comps.front().mean)) {
        for (int j = 0; j < k; ++j) {
            const double offset =
                (static_cast<double>(j) - 0.5 * static_cast<double>(k - 1)) * sd;
            comps[static_cast<std::size_t>(j)].mean = mean + offset;
        }
    }

    std::vector<double> resp(n * static_cast<std::size_t>(k), 0.0);
    double prev_ll = -std::numeric_limits<double>::infinity();
    bool converged = false;
    int iters = 0;
    double ll = 0.0;

    for (iters = 1; iters <= max_iterations; ++iters) {
        ll = 0.0;
        for (std::size_t i = 0; i < n; ++i) {
            double dens = 0.0;
            for (int j = 0; j < k; ++j) {
                const auto& c = comps[static_cast<std::size_t>(j)];
                dens += c.weight * normal_pdf(x[i], c.mean, c.sd);
            }
            if (!(dens > 0.0) || !std::isfinite(dens)) {
                fit.fail_reason = "density_degenerate";
                return fit;
            }
            for (int j = 0; j < k; ++j) {
                const auto& c = comps[static_cast<std::size_t>(j)];
                resp[i * static_cast<std::size_t>(k) + static_cast<std::size_t>(j)] =
                    (c.weight * normal_pdf(x[i], c.mean, c.sd)) / dens;
            }
            ll += std::log(dens);
        }

        for (int j = 0; j < k; ++j) {
            double sum_r = 0.0;
            double sum_x = 0.0;
            for (std::size_t i = 0; i < n; ++i) {
                const double r =
                    resp[i * static_cast<std::size_t>(k) + static_cast<std::size_t>(j)];
                sum_r += r;
                sum_x += r * x[i];
            }
            const double nj = std::max(sum_r, kMinSd);
            comps[static_cast<std::size_t>(j)].weight =
                std::clamp(sum_r / static_cast<double>(n), 0.02, 0.98);
            comps[static_cast<std::size_t>(j)].mean = sum_x / nj;
            double v = 0.0;
            for (std::size_t i = 0; i < n; ++i) {
                const double r =
                    resp[i * static_cast<std::size_t>(k) + static_cast<std::size_t>(j)];
                const double d = x[i] - comps[static_cast<std::size_t>(j)].mean;
                v += r * d * d;
            }
            comps[static_cast<std::size_t>(j)].sd =
                std::sqrt(std::max(v / nj, kMinSd * kMinSd));
        }
        // Renormalize weights
        double wsum = 0.0;
        for (const auto& c : comps) {
            wsum += c.weight;
        }
        if (wsum > 0.0) {
            for (auto& c : comps) {
                c.weight /= wsum;
            }
        }

        if (std::isfinite(prev_ll)
            && std::fabs(ll - prev_ll) < tol * (1.0 + std::fabs(ll))) {
            converged = true;
            break;
        }
        prev_ll = ll;
    }

    // Sort components by mean for stable reporting
    std::sort(comps.begin(), comps.end(),
              [](const GaussianMixtureComponent& a, const GaussianMixtureComponent& b) {
                  return a.mean < b.mean;
              });

    fit.ok = true;
    fit.converged = converged;
    fit.iterations = iters;
    fit.log_likelihood = ll;
    fit.components = std::move(comps);
    fit.bic = -2.0 * ll
        + static_cast<double>(free_params_for_k(k)) * std::log(static_cast<double>(n));
    return fit;
}

bool weights_ok(const std::vector<GaussianMixtureComponent>& comps, double lo, double hi)
{
    for (const auto& c : comps) {
        if (!(c.weight > lo && c.weight < hi)) {
            return false;
        }
    }
    return true;
}

}  // namespace

GaussianMixture2Result fit_gaussian_mixture_2(
    const std::vector<double>& observations,
    const int max_iterations,
    const double tol)
{
    GaussianMixture2Result result;
    result.evidence_type = "formula_reference";
    result.algorithm_id = "gaussian_mixture_2_em";

    std::vector<double> x;
    x.reserve(observations.size());
    for (double value : observations) {
        if (std::isfinite(value)) {
            x.push_back(value);
        }
    }
    const std::size_t n = x.size();
    if (n < 30) {
        result.status = "insufficient_n";
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "mixture2_insufficient_n",
            "观测不足（n<30），无法做二维高斯混合 EM；不得把不足样本写成已排除混合。"});
        return result;
    }

    const EmFit one = fit_em_k(x, 1, max_iterations, tol);
    const EmFit two = fit_em_k(x, 2, max_iterations, tol);
    result.log_likelihood_1 = one.log_likelihood;
    result.bic_1 = one.bic;

    if (!two.ok || two.fail_reason == "density_degenerate") {
        result.status = "failed";
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "mixture2_em_failed",
            "二维高斯混合 EM 密度退化，未能收敛；不得伪造混合拟合。"});
        return result;
    }

    result.ok = true;
    result.converged = two.converged;
    result.iterations = two.iterations;
    result.log_likelihood_2 = two.log_likelihood;
    result.bic_2 = two.bic;
    result.delta_bic = result.bic_1 - result.bic_2;
    if (two.components.size() >= 2) {
        result.weight1 = two.components[0].weight;
        result.weight2 = two.components[1].weight;
        result.mean1 = two.components[0].mean;
        result.mean2 = two.components[1].mean;
        result.sd1 = two.components[0].sd;
        result.sd2 = two.components[1].sd;
    }

    const bool w_ok = result.weight1 > 0.1 && result.weight1 < 0.9;
    if (!two.converged) {
        result.status = "failed";
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "mixture2_not_converged",
            "二维高斯混合 EM 未在迭代上限内收敛；不得把未收敛结果写成已确认混合。"});
    } else if (result.delta_bic >= kPreferDeltaBic && w_ok) {
        result.status = "preferred_2comp";
    } else {
        result.status = "not_preferred";
    }

    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info,
        "mixture2_scope",
        "二维高斯混合 = formula_reference / gaussian_mixture_2_em（EM + BIC vs 单正态）；"
        "多 k 搜索见 gaussian_mixture_k_bic；不是 vendor_oracle，不得写成过程合格判定。"});
    return result;
}

GaussianMixtureSearchResult fit_gaussian_mixture_search(
    const std::vector<double>& observations,
    int k_max,
    const int max_iterations,
    const double tol)
{
    GaussianMixtureSearchResult result;
    result.evidence_type = "formula_reference";
    result.algorithm_id = "gaussian_mixture_k_bic";
    k_max = std::clamp(k_max, 2, 5);
    result.k_max = k_max;

    std::vector<double> x;
    x.reserve(observations.size());
    for (double value : observations) {
        if (std::isfinite(value)) {
            x.push_back(value);
        }
    }
    const std::size_t n = x.size();
    if (n < 30) {
        result.status = "insufficient_n";
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "mixture_k_insufficient_n",
            "观测不足（n<30），无法做多 k 高斯混合 BIC 搜索；不得把不足样本写成已排除混合。"});
        return result;
    }

    result.bic_by_k.assign(static_cast<std::size_t>(k_max) + 1, 0.0);
    int best_k = 1;
    double best_bic = std::numeric_limits<double>::infinity();
    EmFit best_fit;
    int last_iters = 0;

    for (int k = 1; k <= k_max; ++k) {
        if (n < static_cast<std::size_t>(15 * k) && k > 1) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::info,
                "mixture_k_skipped_sparse",
                "跳过 k=" + std::to_string(k) + "（n < 15k）；不得把跳过写成已排除该 k。"});
            continue;
        }
        const EmFit fit = fit_em_k(x, k, max_iterations, tol);
        if (!fit.ok) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::warning,
                "mixture_k_em_failed",
                "k=" + std::to_string(k) + " 高斯混合 EM 失败（"
                    + fit.fail_reason + "）；该 k 不参与 BIC 优选。"});
            continue;
        }
        if (!fit.converged && k > 1) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::warning,
                "mixture_k_not_converged",
                "k=" + std::to_string(k) + " 高斯混合 EM 未收敛；该 k 不参与 BIC 优选。"});
            continue;
        }
        result.bic_by_k[static_cast<std::size_t>(k)] = fit.bic;
        last_iters = std::max(last_iters, fit.iterations);
        if (fit.bic < best_bic) {
            best_bic = fit.bic;
            best_k = k;
            best_fit = fit;
        }
        if (k == 1) {
            result.bic_1 = fit.bic;
        }
    }

    if (!best_fit.ok) {
        result.status = "failed";
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "mixture_k_search_failed",
            "多 k 高斯混合 BIC 搜索未得到可用拟合；不得伪造混合结论。"});
        return result;
    }

    result.ok = true;
    result.converged = best_fit.converged;
    result.iterations = last_iters;
    result.k_selected = best_k;
    result.components = best_fit.components;
    result.bic_selected = best_fit.bic;
    result.delta_bic = result.bic_1 - result.bic_selected;

    const bool w_ok = best_k == 1
        || weights_ok(result.components, 0.05, 0.95);
    if (best_k >= 2 && result.delta_bic >= kPreferDeltaBic && w_ok) {
        result.status = (best_k == 2) ? "preferred_2comp" : "preferred_kcomp";
    } else {
        result.status = "not_preferred";
    }

    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info,
        "mixture_k_scope",
        "多 k 高斯混合 = formula_reference / gaussian_mixture_k_bic（EM + BIC，k=1.."
            + std::to_string(k_max) + "；选定 k=" + std::to_string(best_k)
            + "）；不是非高斯混合，不是 vendor_oracle，不得写成过程合格判定。"});
    return result;
}

}  // namespace datalab::domain::statistics
