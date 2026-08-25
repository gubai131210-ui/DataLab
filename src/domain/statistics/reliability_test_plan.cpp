#include "domain/statistics/reliability_test_plan.h"

#include <cmath>
#include <limits>

namespace datalab::domain::statistics {
namespace {

double binom_cdf_leq_r(std::size_t n, std::size_t r, double p_fail)
{
    // Σ_{k=0..r} C(n,k) p^k (1-p)^{n-k} via recursive term ratios.
    if (r >= n) {
        return 1.0;
    }
    const double q = 1.0 - p_fail;  // success / reliability
    if (!(p_fail >= 0.0) || !(p_fail <= 1.0) || !(q >= 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    // term for k=0
    double term = std::pow(q, static_cast<double>(n));
    double sum = term;
    for (std::size_t k = 0; k < r; ++k) {
        term *= (static_cast<double>(n - k) / static_cast<double>(k + 1))
            * (p_fail / q);
        sum += term;
        if (!std::isfinite(sum)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
    }
    return sum;
}

std::size_t search_n_with_failures(
    double R_test, double CL, std::size_t r, std::size_t n_min)
{
    const double p_fail = 1.0 - R_test;
    const double threshold = 1.0 - CL;
    const std::size_t n_cap = 100000;
    for (std::size_t n = std::max(n_min, r); n <= n_cap; ++n) {
        const double cdf = binom_cdf_leq_r(n, r, p_fail);
        if (std::isfinite(cdf) && cdf <= threshold) {
            return n;
        }
    }
    return 0;
}

}  // namespace

ReliabilityTestPlanResult plan_reliability_demonstration(
    const ReliabilityTestPlanOptions& options)
{
    ReliabilityTestPlanResult result;
    result.shape_beta = options.shape_beta;
    result.target_reliability = options.target_reliability;
    result.confidence_level = options.confidence_level;
    result.test_time = options.test_time;
    result.mission_time = options.mission_time;
    result.allowed_failures = options.allowed_failures;

    if (!(options.shape_beta > 0.0) || !std::isfinite(options.shape_beta)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "rtp_bad_beta",
            "Weibull 形状 β 假设必须为正有限数。"});
        return result;
    }
    if (!(options.target_reliability > 0.0)
        || !(options.target_reliability < 1.0)
        || !std::isfinite(options.target_reliability)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "rtp_bad_R",
            "目标可靠度 R 必须满足 0 < R < 1。"});
        return result;
    }
    if (!(options.confidence_level > 0.0)
        || !(options.confidence_level < 1.0)
        || !std::isfinite(options.confidence_level)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "rtp_bad_CL",
            "置信水平 CL 必须满足 0 < CL < 1。"});
        return result;
    }
    if (!(options.test_time > 0.0) || !std::isfinite(options.test_time)
        || !(options.mission_time > 0.0) || !std::isfinite(options.mission_time)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "rtp_bad_times",
            "试验时长 T0 与任务时长 tm 必须为正有限数。"});
        return result;
    }

    const double delta = std::pow(
        options.test_time / options.mission_time, options.shape_beta);
    if (!std::isfinite(delta) || !(delta > 0.0)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "rtp_bad_delta",
            "时间比调整 δ=(T0/tm)^β 无效。"});
        return result;
    }
    result.time_ratio_delta = delta;
    const double R_test = std::pow(options.target_reliability, delta);
    if (!std::isfinite(R_test) || !(R_test > 0.0) || !(R_test < 1.0)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "rtp_bad_Rtest",
            "等效试验可靠度 R^δ 无效。"});
        return result;
    }
    result.test_reliability = R_test;

    if (options.allowed_failures == 0) {
        // n = ceil( ln(1-CL) / (δ ln R) ) = ceil( ln(1-CL) / ln(R_test) )
        const double numer = std::log(1.0 - options.confidence_level);
        const double denom = std::log(R_test);
        if (!std::isfinite(numer) || !std::isfinite(denom) || denom == 0.0) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "rtp_zero_fail_failed",
                "零失效样本量公式无法求值。"});
            return result;
        }
        const double raw = numer / denom;
        if (!std::isfinite(raw) || !(raw > 0.0)) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "rtp_zero_fail_failed",
                "零失效样本量计算结果无效。"});
            return result;
        }
        result.sample_size = static_cast<std::size_t>(std::ceil(raw));
    } else {
        const std::size_t n = search_n_with_failures(
            R_test, options.confidence_level, options.allowed_failures, 1);
        if (n == 0) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "rtp_search_failed",
                "在搜索上限内未找到满足二项累积的样本量 n。"});
            return result;
        }
        result.sample_size = n;
    }

    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "rtp_assumptions",
        "β 为工程假设而非数据估计；演示型试验计划不宣称寿命已达标。"});
    return result;
}

}  // namespace datalab::domain::statistics
