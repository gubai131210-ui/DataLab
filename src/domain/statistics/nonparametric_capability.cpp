#include "domain/statistics/nonparametric_capability.h"

#include "domain/statistics/quality_visuals.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

namespace datalab::domain::statistics {
namespace {

double normal_cdf(double z)
{
    return 0.5 * std::erfc(-z / std::sqrt(2.0));
}

double empirical_percentile(std::vector<double> sorted, double p)
{
    if (sorted.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    std::sort(sorted.begin(), sorted.end());
    const double n = static_cast<double>(sorted.size());
    const double w = p * (n + 1.0);
    const std::size_t y = static_cast<std::size_t>(std::floor(w));
    const double z = w - static_cast<double>(y);
    if (y == 0) {
        return sorted.front();
    }
    if (y >= sorted.size()) {
        return sorted.back();
    }
    return (1.0 - z) * sorted[y - 1] + z * sorted[y];
}

double median_value(const std::vector<double>& sorted)
{
    return empirical_percentile(sorted, 0.5);
}

}  // namespace

NonparametricCapabilityResult compute_nonparametric_capability(
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows,
    const SpecificationLimits& specifications,
    double tolerance_k)
{
    NonparametricCapabilityResult result;
    result.tolerance_k = tolerance_k;
    result.source_rows = source_rows;

    if (values.size() < 10) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "nonparam_cap_n",
            "非参数能力至少需要 10 个有效观测。"});
        return result;
    }
    if (!specifications.lower.has_value() || !specifications.upper.has_value()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "nonparam_cap_specs",
            "非参数 Cnp 需要 LSL 与 USL。"});
        return result;
    }
    const double lsl = *specifications.lower;
    const double usl = *specifications.upper;
    if (!(lsl < usl)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "nonparam_cap_specs",
            "LSL 必须小于 USL。"});
        return result;
    }
    if (!(tolerance_k > 0.0)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "nonparam_cap_tolerance",
            "容差 K 必须为正。"});
        return result;
    }

    result.sample_size = values.size();
    std::vector<double> sorted = values;
    result.median = median_value(sorted);

    const double half = tolerance_k / 2.0;
    const double p_lower = normal_cdf(-half);
    const double p_upper = normal_cdf(half);
    result.lower_percentile = empirical_percentile(sorted, p_lower);
    result.upper_percentile = empirical_percentile(sorted, p_upper);

    const double spread = result.upper_percentile - result.lower_percentile;
    if (spread > 0.0) {
        result.cnp = (usl - lsl) / spread;
    }
    const double lower_side = result.median - result.lower_percentile;
    const double upper_side = result.upper_percentile - result.median;
    if (lower_side > 0.0) {
        result.cnpl = (result.median - lsl) / lower_side;
    }
    if (upper_side > 0.0) {
        result.cnpu = (usl - result.median) / upper_side;
    }
    if (result.cnpl.has_value() && result.cnpu.has_value()) {
        result.cnpk = std::min(*result.cnpl, *result.cnpu);
    } else if (result.cnpl.has_value()) {
        result.cnpk = *result.cnpl;
    } else if (result.cnpu.has_value()) {
        result.cnpk = *result.cnpu;
    }

    std::size_t below = 0;
    std::size_t above = 0;
    for (double v : values) {
        if (v < lsl) {
            ++below;
        }
        if (v > usl) {
            ++above;
        }
    }
    const double ppm_scale = 1.0e6 / static_cast<double>(values.size());
    result.observed_ppm_below = static_cast<double>(below) * ppm_scale;
    result.observed_ppm_above = static_cast<double>(above) * ppm_scale;
    result.observed_ppm_total = static_cast<double>(below + above) * ppm_scale;

    const HistogramResult histogram_result = histogram(values);
    result.histogram_edges = histogram_result.edges;
    result.histogram_counts = histogram_result.counts;

    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "nonparam_cap_scope",
        "经验分位数能力（Cnp/Cnpk）；非正态假设；非 Minitab golden。"});
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::warning, "assumption_not_verified",
        "能力指标未验证过程稳定性；数值仅供调查，不能单独作为过程合格结论。"});
    return result;
}

}  // namespace datalab::domain::statistics
