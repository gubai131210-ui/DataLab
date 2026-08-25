#include "domain/statistics/weibayes.h"

#include "domain/statistics/reliability.h"

#include <cmath>

namespace datalab::domain::statistics {

WeibayesResult fit_weibayes(
    const std::vector<double>& times,
    const std::vector<bool>& events,
    const std::vector<std::size_t>& source_rows,
    const WeibayesOptions& options)
{
    WeibayesResult result;
    result.shape_prior = options.shape_prior;
    result.source_rows = source_rows;

    if (!(options.shape_prior > 0.0) || !std::isfinite(options.shape_prior)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "weibayes_bad_shape",
            "Weibayes 形状先验 β 必须为正有限数。"});
        return result;
    }
    if (times.empty() || times.size() != events.size()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "weibayes_empty",
            "Weibayes 需要非空且对齐的时间与事件指示。"});
        return result;
    }

    const double beta = options.shape_prior;
    double sum_t_beta = 0.0;
    std::size_t failures = 0;
    std::size_t censored = 0;
    for (std::size_t index = 0; index < times.size(); ++index) {
        if (!(times[index] > 0.0) || !std::isfinite(times[index])) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "weibayes_bad_time",
                "时间必须为正有限数（complete-case）。"});
            return result;
        }
        sum_t_beta += std::pow(times[index], beta);
        if (events[index]) {
            ++failures;
        } else {
            ++censored;
        }
    }
    result.n = times.size();
    result.failure_count = failures;
    result.censored_count = censored;

    if (failures == 0) {
        result.zero_failure_bound = true;
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "weibayes_zero_failures",
            "失效数 r = 0：不做 η 点估计；仅保留形状先验与删失摘要（少失效诚实边界）。"});
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::info, "weibayes_limits",
            "无失效路径不宣称寿命已达标；需更多失效或工程先验复核。"});
        return result;
    }

    const double eta = std::pow(sum_t_beta / static_cast<double>(failures), 1.0 / beta);
    if (!std::isfinite(eta) || !(eta > 0.0)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "weibayes_scale_failed",
            "无法计算特征寿命 η。"});
        return result;
    }
    result.scale = eta;

    for (const double percentile : {10.0, 50.0, 90.0}) {
        WeibayesPercentile row;
        row.percentile = percentile;
        row.life = percentile_life_weibull(beta, eta, percentile);
        result.percentiles.push_back(row);
    }
    return result;
}

}  // namespace datalab::domain::statistics
