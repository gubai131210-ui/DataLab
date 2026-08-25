#include "domain/statistics/nhpp_repairable.h"

#include <algorithm>
#include <cmath>

namespace datalab::domain::statistics {

NhppRepairableResult fit_nhpp_crow_amsaa(
    const std::vector<double>& failure_times,
    const std::vector<std::size_t>& source_rows,
    const NhppRepairableOptions& options)
{
    NhppRepairableResult result;
    result.source_rows = source_rows;
    result.failure_times = failure_times;

    if (failure_times.empty()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "nhpp_empty",
            "可修复 NHPP 需要至少一个累积失效时间。"});
        return result;
    }

    for (double t : failure_times) {
        if (!(t > 0.0) || !std::isfinite(t)) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "nhpp_bad_times",
                "失效时间必须为正有限数，且满足 0 < t1 ≤ … ≤ T（complete-case）。"});
            return result;
        }
    }

    std::vector<std::pair<double, std::size_t>> paired;
    paired.reserve(failure_times.size());
    for (std::size_t i = 0; i < failure_times.size(); ++i) {
        const std::size_t src =
            i < source_rows.size() ? source_rows[i] : i;
        paired.push_back({failure_times[i], src});
    }
    std::sort(paired.begin(), paired.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<double> sorted;
    std::vector<std::size_t> sorted_sources;
    sorted.reserve(paired.size());
    sorted_sources.reserve(paired.size());
    for (const auto& item : paired) {
        sorted.push_back(item.first);
        sorted_sources.push_back(item.second);
    }
    result.failure_times = sorted;
    result.source_rows = sorted_sources;
    result.failure_count = sorted.size();

    double T = options.truncation_time.has_value()
        ? *options.truncation_time
        : sorted.back();
    if (!(T > 0.0) || !std::isfinite(T) || T < sorted.back()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "nhpp_bad_T",
            "截尾时间 T 必须 ≥ 最大失效时间且为正有限数。"});
        return result;
    }
    result.truncation_time = T;

    const double n = static_cast<double>(sorted.size());
    double sum_ln = 0.0;
    for (double ti : sorted) {
        if (!(T > ti) && !(T == ti && ti > 0.0)) {
            // When T == ti, ln(T/ti)=0 contributes nothing; allow equality.
        }
        if (!(ti > 0.0)) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "nhpp_bad_times",
                "失效时间必须为正。"});
            return result;
        }
        const double ratio = T / ti;
        if (!(ratio > 0.0) || !std::isfinite(ratio)) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "nhpp_mle_failed",
                "无法计算 Σ ln(T/ti)。"});
            return result;
        }
        sum_ln += std::log(ratio);
    }
    if (!(sum_ln > 0.0) || !std::isfinite(sum_ln)) {
        // All failures at T → denominator 0; honesty path.
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "nhpp_degenerate",
            "Σ ln(T/ti) = 0：无法估计 β（例如所有失效恰在截尾点）。"});
        return result;
    }

    const double beta = n / sum_ln;
    if (!std::isfinite(beta) || !(beta > 0.0)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "nhpp_mle_failed",
            "β MLE 无效。"});
        return result;
    }
    const double lambda = n / std::pow(T, beta);
    if (!std::isfinite(lambda) || !(lambda > 0.0)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "nhpp_mle_failed",
            "λ MLE 无效。"});
        return result;
    }
    result.beta = beta;
    result.lambda = lambda;

    const std::size_t grid = std::max<std::size_t>(2, options.intensity_grid_points);
    for (std::size_t i = 1; i <= grid; ++i) {
        const double t = T * static_cast<double>(i) / static_cast<double>(grid);
        NhppIntensityPoint point;
        point.t = t;
        point.mean_function = lambda * std::pow(t, beta);
        if (t > 0.0) {
            point.intensity = lambda * beta * std::pow(t, beta - 1.0);
        }
        result.intensity_curve.push_back(point);
    }

    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "nhpp_scope",
        "Crow–AMSAA 幂律 NHPP：参数为 MLE；Duane 图仅供趋势参考，"
        "不宣称 ROCOF 合格或过程已稳定。"});
    return result;
}

}  // namespace datalab::domain::statistics
