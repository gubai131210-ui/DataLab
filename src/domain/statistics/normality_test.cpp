#include "domain/statistics/normality_test.h"

#include "domain/quality_diagnostics.h"
#include "domain/statistics/anderson_darling.h"
#include "domain/statistics/descriptive_statistics.h"
#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

double normal_cdf(double value, double mean, double standard_deviation)
{
    return 0.5 * (1.0 + std::erf(
        (value - mean) / (standard_deviation * std::sqrt(2.0))));
}

void note(NormalityTestResult& result, const char* code, const char* message)
{
    result.diagnostics.push_back(message);
    add_warning(result.messages, code, message);
}

double ryan_joiner_critical(std::size_t n, double alpha)
{
    const double nn = static_cast<double>(n);
    const double u = 1.0 / std::sqrt(nn);
    const double v = 1.0 / nn;
    const double w = 1.0 / (nn * nn);
    if (alpha <= 0.01) {
        return 0.9963 - 0.0211 * u - 1.4106 * v + 3.5152 * w;
    }
    if (alpha <= 0.05) {
        return 1.0063 - 0.1288 * u - 0.6118 * v + 1.3505 * w;
    }
    return 1.0071 - 0.1371 * u - 0.3682 * v + 0.7780 * w;
}

void compute_anderson_darling(NormalityTestResult& result, std::vector<double> ordered)
{
    std::vector<double> cdf;
    cdf.reserve(ordered.size());
    for (const double value : ordered) {
        cdf.push_back(normal_cdf(value, result.mean, result.sample_standard_deviation));
    }
    const auto ad = anderson_darling_from_sorted_cdf(cdf);
    result.anderson_darling = ad.statistic;
    result.adjusted_anderson_darling = ad.adjusted;
    if (ad.adjusted.has_value()) {
        result.p_value = std::clamp(
            anderson_darling_p_value_normal(*ad.adjusted), 0.0, 1.0);
        result.decision = *result.p_value < result.alpha ? "reject" : "fail_to_reject";
    }
}

void compute_ryan_joiner(NormalityTestResult& result, std::vector<double> ordered)
{
    const std::size_t n = ordered.size();
    std::vector<double> ranks(n, 0.0);
    std::size_t start = 0;
    while (start < n) {
        std::size_t end = start + 1;
        while (end < n && ordered[end] == ordered[start]) {
            ++end;
        }
        const double rank = 0.5 * (static_cast<double>(start + 1)
            + static_cast<double>(end));
        for (std::size_t index = start; index < end; ++index) {
            ranks[index] = rank;
        }
        start = end;
    }

    std::vector<double> scores(n, 0.0);
    double sum_b2 = 0.0;
    double sum_yb = 0.0;
    const double nn = static_cast<double>(n);
    for (std::size_t index = 0; index < n; ++index) {
        const double p = (ranks[index] - 0.375) / (nn + 0.25);
        const double clipped = std::clamp(p, 1.0e-12, 1.0 - 1.0e-12);
        scores[index] = standard_normal_quantile(clipped);
        sum_b2 += scores[index] * scores[index];
        sum_yb += ordered[index] * scores[index];
    }
    const double s2 = result.sample_standard_deviation * result.sample_standard_deviation;
    if (!(s2 > 0.0) || !(sum_b2 > 0.0)) {
        note(result, "ryan_joiner_not_computed",
             "Ryan–Joiner 统计量无法计算（方差或正态得分为零）。");
        result.decision = "not_computed";
        return;
    }
    const double rp = sum_yb / std::sqrt(s2 * (nn - 1.0) * sum_b2);
    result.ryan_joiner_r = rp;

    const double c10 = ryan_joiner_critical(n, 0.10);
    const double c05 = ryan_joiner_critical(n, 0.05);
    const double c01 = ryan_joiner_critical(n, 0.01);
    if (rp > c10) {
        result.p_value = 0.10;
        note(result, "ryan_joiner_p_gt_0_10",
             "Ryan–Joiner：R 高于 α=0.10 临界，报告 p>0.10（存储 0.10）。");
    } else if (rp <= c01) {
        result.p_value = 0.01;
        note(result, "ryan_joiner_p_lt_0_01",
             "Ryan–Joiner：R 不高于 α=0.01 临界，报告 p<0.01（存储 0.01）。");
    } else if (rp > c05) {
        const double t = (c10 - rp) / (c10 - c05);
        result.p_value = std::clamp(0.10 - t * 0.05, 0.05, 0.10);
    } else {
        const double t = (c05 - rp) / (c05 - c01);
        result.p_value = std::clamp(0.05 - t * 0.04, 0.01, 0.05);
    }
    result.decision = *result.p_value < result.alpha ? "reject" : "fail_to_reject";
}

}  // namespace

NormalityTestResult normality_test(const std::vector<double>& observations)
{
    std::vector<std::size_t> source_rows(observations.size());
    std::iota(source_rows.begin(), source_rows.end(), 0);
    return normality_test(observations, source_rows, "anderson_darling");
}

NormalityTestResult normality_test(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows)
{
    return normality_test(observations, source_rows, "anderson_darling");
}

NormalityTestResult normality_test(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows,
    const std::string& method)
{
    NormalityTestResult result;
    result.method = (method == "ryan_joiner") ? "ryan_joiner" : "anderson_darling";
    result.evidence.method_version = "3";
    result.evidence.assumption_status = "not_verified";
    result.evidence.alpha = result.alpha;
    result.probability_plot = normal_probability_plot(observations, source_rows);

    std::vector<double> valid;
    valid.reserve(observations.size());
    for (const double value : observations) {
        if (std::isfinite(value)) {
            valid.push_back(value);
        } else {
            ++result.missing_count;
        }
    }
    result.count = valid.size();
    result.evidence.valid_count = valid.size();
    result.evidence.missing_count = result.missing_count;
    result.evidence.source_rows = source_rows;

    const auto summary = DescriptiveStatistics::calculate(valid);
    if (!summary.has_value()) {
        note(result, "insufficient_data", "正态性检验需要至少一个有效数值观测。");
        result.evidence.not_computed_reason = "insufficient_data";
        result.decision = "not_computed";
        return result;
    }
    result.mean = summary->mean;
    if (!summary->sample_standard_deviation.has_value()) {
        note(result, "insufficient_data",
             "正态性检验至少需要 2 个观测；当前仅返回概率图点。");
        result.evidence.not_computed_reason = "insufficient_data";
        result.decision = "not_computed";
        return result;
    }
    result.sample_standard_deviation = *summary->sample_standard_deviation;
    if (!(result.sample_standard_deviation > 0.0)) {
        note(result, "zero_variance", "常量样本无法计算正态性检验统计量。");
        result.evidence.not_computed_reason = "zero_variance";
        result.decision = "not_computed";
        return result;
    }
    if (valid.size() < 3) {
        note(result, "insufficient_data",
             "正态性检验至少需要 3 个有效观测；n<3 时不计算统计量与 p 值。");
        result.evidence.not_computed_reason = "insufficient_data";
        result.decision = "not_computed";
        return result;
    }
    if (valid.size() < 8) {
        result.sample_size_warning = true;
        note(result, "assumption_not_verified",
             "n<8 时正态性近似较粗糙，未拒绝正态假设不能当作已验证正态。");
    }

    std::vector<double> ordered = valid;
    std::sort(ordered.begin(), ordered.end());
    if (result.method == "ryan_joiner") {
        compute_ryan_joiner(result, std::move(ordered));
    } else {
        compute_anderson_darling(result, std::move(ordered));
    }
    return result;
}

}  // namespace datalab::domain::statistics
