#include "domain/statistics/ccf.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace datalab::domain::statistics {

CcfResult compute_ccf(
    const std::vector<double>& x,
    const std::vector<double>& y,
    std::size_t max_lag,
    double alpha)
{
    CcfResult result;
    result.alpha = alpha;
    const std::size_t n = std::min(x.size(), y.size());
    std::vector<double> xs;
    std::vector<double> ys;
    for (std::size_t i = 0; i < n; ++i) {
        if (std::isfinite(x[i]) && std::isfinite(y[i])) {
            xs.push_back(x[i]);
            ys.push_back(y[i]);
        } else {
            ++result.missing_count;
        }
    }
    result.n = xs.size();
    if (xs.size() < 5) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "ccf_insufficient",
            "CCF 至少需要约 5 对有效观测。"});
        return result;
    }
    if (max_lag == 0) {
        max_lag = std::min<std::size_t>(40, xs.size() / 4);
    }
    max_lag = std::min(max_lag, xs.size() - 2);
    result.max_lag = max_lag;
    const double mean_x =
        std::accumulate(xs.cbegin(), xs.cend(), 0.0) / static_cast<double>(xs.size());
    const double mean_y =
        std::accumulate(ys.cbegin(), ys.cend(), 0.0) / static_cast<double>(ys.size());
    double ssx = 0.0;
    double ssy = 0.0;
    for (std::size_t i = 0; i < xs.size(); ++i) {
        ssx += (xs[i] - mean_x) * (xs[i] - mean_x);
        ssy += (ys[i] - mean_y) * (ys[i] - mean_y);
    }
    if (!(ssx > 0.0) || !(ssy > 0.0)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "ccf_zero_variance",
            "某一序列方差为 0，无法计算 CCF。"});
        return result;
    }
    const double denom = std::sqrt(ssx * ssy);
    const double z = alpha <= 0.01 ? 2.57582930355
        : (alpha >= 0.1 ? 1.64485362695 : 1.95996398454);
    result.band_half_width = z / std::sqrt(static_cast<double>(xs.size()));

    for (int lag = -static_cast<int>(max_lag); lag <= static_cast<int>(max_lag); ++lag) {
        double num = 0.0;
        if (lag >= 0) {
            for (std::size_t t = 0; t + static_cast<std::size_t>(lag) < xs.size(); ++t) {
                num += (xs[t] - mean_x)
                    * (ys[t + static_cast<std::size_t>(lag)] - mean_y);
            }
        } else {
            const std::size_t shift = static_cast<std::size_t>(-lag);
            for (std::size_t t = shift; t < xs.size(); ++t) {
                num += (xs[t] - mean_x) * (ys[t - shift] - mean_y);
            }
        }
        result.lags.push_back(static_cast<double>(lag));
        result.ccf.push_back(num / denom);
    }
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "ccf_band",
        "带宽为白噪声固定 ±z/√n；非预白化；非 Minitab golden。"});
    return result;
}

}  // namespace datalab::domain::statistics
