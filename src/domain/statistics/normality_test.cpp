#include "domain/statistics/normality_test.h"

#include "domain/quality_diagnostics.h"
#include "domain/statistics/descriptive_statistics.h"

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

double anderson_darling_p_value(double adjusted)
{
    if (adjusted > 0.6) {
        return std::exp(1.2937 - 5.709 * adjusted + 0.0186 * adjusted * adjusted);
    }
    if (adjusted > 0.34) {
        return std::exp(0.9177 - 4.279 * adjusted - 1.38 * adjusted * adjusted);
    }
    if (adjusted > 0.2) {
        return 1.0 - std::exp(-8.318 + 42.796 * adjusted - 59.938 * adjusted * adjusted);
    }
    return 1.0 - std::exp(-13.436 + 101.14 * adjusted - 223.73 * adjusted * adjusted);
}

void note(NormalityTestResult& result, const char* code, const char* message)
{
    result.diagnostics.push_back(message);
    add_warning(result.messages, code, message);
}

}  // namespace

NormalityTestResult normality_test(const std::vector<double>& observations)
{
    std::vector<std::size_t> source_rows(observations.size());
    std::iota(source_rows.begin(), source_rows.end(), 0);
    return normality_test(observations, source_rows);
}

NormalityTestResult normality_test(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows)
{
    NormalityTestResult result;
    result.evidence.method_version = "2";
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
             "Anderson-Darling 至少需要 2 个观测；当前仅返回概率图点。");
        result.evidence.not_computed_reason = "insufficient_data";
        result.decision = "not_computed";
        return result;
    }
    result.sample_standard_deviation = *summary->sample_standard_deviation;
    if (!(result.sample_standard_deviation > 0.0)) {
        note(result, "zero_variance", "常量样本无法计算 Anderson-Darling 统计量。");
        result.evidence.not_computed_reason = "zero_variance";
        result.decision = "not_computed";
        return result;
    }
    if (valid.size() < 3) {
        note(result, "insufficient_data",
             "Anderson-Darling 至少需要 3 个有效观测；n<3 时不计算 AD 与 p 值。");
        result.evidence.not_computed_reason = "insufficient_data";
        result.decision = "not_computed";
        return result;
    }
    if (valid.size() < 8) {
        result.sample_size_warning = true;
        note(result, "assumption_not_verified",
             "n<8 时 Anderson-Darling 近似较粗糙，未拒绝正态假设不能当作已验证正态。");
    }

    std::vector<double> ordered = valid;
    std::sort(ordered.begin(), ordered.end());
    constexpr double kEpsilon = 1.0e-12;
    double sum = 0.0;
    for (std::size_t index = 0; index < ordered.size(); ++index) {
        const double cdf = std::clamp(
            normal_cdf(ordered[index], result.mean, result.sample_standard_deviation),
            kEpsilon, 1.0 - kEpsilon);
        const double left_weight = static_cast<double>(2 * index + 1);
        const double right_weight =
            static_cast<double>(2 * ordered.size() + 1 - 2 * (index + 1));
        sum += left_weight * std::log(cdf)
            + right_weight * std::log(1.0 - cdf);
    }
    const double statistic = -static_cast<double>(ordered.size())
        - sum / static_cast<double>(ordered.size());
    const double adjusted = statistic * (1.0 + 0.75 / ordered.size()
        + 2.25 / (ordered.size() * ordered.size()));
    result.anderson_darling = statistic;
    result.adjusted_anderson_darling = adjusted;
    result.p_value = std::clamp(anderson_darling_p_value(adjusted), 0.0, 1.0);
    result.decision = *result.p_value < result.alpha ? "reject" : "fail_to_reject";
    return result;
}

}  // namespace datalab::domain::statistics
