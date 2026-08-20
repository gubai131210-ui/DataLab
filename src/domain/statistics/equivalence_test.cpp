#include "domain/statistics/equivalence_test.h"

#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace datalab::domain::statistics {
namespace {

void add_error(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

void fill_tost(
    EquivalenceTestResult& result,
    double difference,
    double standard_error,
    double degrees_of_freedom,
    double lower,
    double upper,
    double confidence_level)
{
    result.difference = difference;
    result.standard_error = standard_error;
    result.degrees_of_freedom = degrees_of_freedom;
    result.lower = lower;
    result.upper = upper;
    result.confidence_level = confidence_level;
    result.alpha = 1.0 - confidence_level;
    if (!(confidence_level > 0.0 && confidence_level < 1.0)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return;
    }
    if (!(lower < upper) || !std::isfinite(lower) || !std::isfinite(upper)) {
        add_error(result.diagnostics, "invalid_equivalence_limits",
                  "等价下限必须小于等价上限。");
        return;
    }
    if (!(standard_error > 0.0) || !(degrees_of_freedom > 0.0)) {
        add_error(result.diagnostics, "zero_variance",
                  "标准误为 0 或自由度不可用，无法计算 TOST。");
        return;
    }
    result.t_lower = (difference - lower) / standard_error;
    result.t_upper = (difference - upper) / standard_error;
    result.p_lower = 1.0 - student_t_cdf(result.t_lower, degrees_of_freedom);
    result.p_upper = student_t_cdf(result.t_upper, degrees_of_freedom);
    const double critical = student_t_quantile(1.0 - result.alpha, degrees_of_freedom);
    result.confidence_lower = difference - critical * standard_error;
    result.confidence_upper = difference + critical * standard_error;
    result.both_pvalues_below_alpha = result.p_lower.has_value()
        && result.p_upper.has_value()
        && *result.p_lower <= result.alpha
        && *result.p_upper <= result.alpha;
    result.within_limits = result.both_pvalues_below_alpha;
}

void fill_z_tost(
    EquivalenceTestResult& result,
    double difference,
    double standard_error,
    double lower,
    double upper,
    double confidence_level)
{
    result.difference = difference;
    result.standard_error = standard_error;
    result.degrees_of_freedom = 0.0;
    result.lower = lower;
    result.upper = upper;
    result.confidence_level = confidence_level;
    result.alpha = 1.0 - confidence_level;
    result.ci_method = "wald_z_tost";
    if (!(confidence_level > 0.0 && confidence_level < 1.0)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return;
    }
    if (!(lower < upper) || !std::isfinite(lower) || !std::isfinite(upper)) {
        add_error(result.diagnostics, "invalid_equivalence_limits",
                  "等价下限必须小于等价上限。");
        return;
    }
    if (!(standard_error > 0.0) || !std::isfinite(standard_error)) {
        add_error(result.diagnostics, "zero_variance",
                  "标准误为 0，无法计算比例 z-TOST。");
        return;
    }
    result.t_lower = (difference - lower) / standard_error;
    result.t_upper = (difference - upper) / standard_error;
    result.p_lower = 1.0 - standard_normal_cdf(result.t_lower);
    result.p_upper = standard_normal_cdf(result.t_upper);
    const double critical = standard_normal_quantile(1.0 - result.alpha);
    result.confidence_lower = difference - critical * standard_error;
    result.confidence_upper = difference + critical * standard_error;
    result.both_pvalues_below_alpha = result.p_lower.has_value()
        && result.p_upper.has_value()
        && *result.p_lower <= result.alpha
        && *result.p_upper <= result.alpha;
    result.within_limits = result.both_pvalues_below_alpha;
}

}  // namespace

EquivalenceTestResult one_sample_equivalence_test(
    const std::vector<double>& observations,
    double target,
    double lower,
    double upper,
    double confidence_level)
{
    EquivalenceTestResult result;
    result.kind = "one_sample";
    const TTestResult summary = one_sample_t_test(
        observations, target, confidence_level, TestAlternative::two_sided);
    result.diagnostics = summary.diagnostics;
    result.first_count = summary.count;
    result.first_mean = summary.mean;
    result.first_standard_deviation = summary.sample_standard_deviation;
    if (!result.diagnostics.empty()) {
        return result;
    }
    fill_tost(result, summary.difference, summary.standard_error,
              summary.degrees_of_freedom, lower, upper, confidence_level);
    return result;
}

EquivalenceTestResult two_sample_equivalence_test(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double lower,
    double upper,
    double confidence_level,
    VarianceMethod variance_method)
{
    EquivalenceTestResult result;
    result.kind = "two_sample";
    result.variance_method = variance_method;
    const TwoSampleTTestResult summary = two_sample_t_test(
        first, second, confidence_level, TestAlternative::two_sided, variance_method);
    result.diagnostics = summary.diagnostics;
    result.first_count = summary.first.count;
    result.second_count = summary.second.count;
    result.first_mean = summary.first.mean;
    result.second_mean = summary.second.mean;
    result.first_standard_deviation = summary.first.sample_standard_deviation;
    result.second_standard_deviation = summary.second.sample_standard_deviation;
    if (!result.diagnostics.empty()) {
        return result;
    }
    fill_tost(result, summary.mean_difference, summary.standard_error_difference,
              summary.degrees_of_freedom, lower, upper, confidence_level);
    return result;
}

EquivalenceTestResult paired_equivalence_test(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double lower,
    double upper,
    double confidence_level)
{
    EquivalenceTestResult result;
    result.kind = "paired";
    const PairedTTestResult summary = paired_t_test(
        first, second, confidence_level, TestAlternative::two_sided);
    result.diagnostics = summary.diagnostics;
    result.first_count = summary.count;
    result.second_count = summary.count;
    result.first_mean = summary.mean_difference;
    result.first_standard_deviation = summary.sample_standard_deviation;
    if (!result.diagnostics.empty()) {
        return result;
    }
    fill_tost(result, summary.mean_difference, summary.standard_error,
              summary.degrees_of_freedom, lower, upper, confidence_level);
    return result;
}

EquivalenceTestResult one_proportion_equivalence_test(
    const std::size_t events,
    const std::size_t trials,
    const double hypothesized,
    const double lower,
    const double upper,
    const double confidence_level,
    const std::size_t row_count,
    const std::size_t missing_count)
{
    EquivalenceTestResult result;
    result.kind = "one_proportion";
    result.ci_method = "wald_z_tost";
    result.first_count = trials;
    result.second_count = row_count;
    if (trials == 0 || events > trials) {
        add_error(result.diagnostics, "invalid_binomial_counts",
                  "试验数必须为正，且事件数不能超过试验数。");
        return result;
    }
    if (!(hypothesized >= 0.0 && hypothesized <= 1.0) || !std::isfinite(hypothesized)) {
        add_error(result.diagnostics, "invalid_hypothesized_proportion",
                  "假设比例必须在 0 与 1 之间。");
        return result;
    }
    const double proportion = static_cast<double>(events) / static_cast<double>(trials);
    result.first_mean = proportion;
    result.second_mean = hypothesized;
    result.first_standard_deviation = static_cast<double>(events);
    result.second_standard_deviation = static_cast<double>(missing_count);
    const double se = std::sqrt(proportion * (1.0 - proportion) / static_cast<double>(trials));
    fill_z_tost(result, proportion - hypothesized, se, lower, upper, confidence_level);
    return result;
}

EquivalenceTestResult two_proportion_equivalence_test(
    const std::size_t first_events,
    const std::size_t first_trials,
    const std::size_t second_events,
    const std::size_t second_trials,
    const double lower,
    const double upper,
    const double confidence_level,
    const std::size_t first_row_count,
    const std::size_t second_row_count,
    const std::size_t first_missing,
    const std::size_t second_missing)
{
    EquivalenceTestResult result;
    result.kind = "two_proportion";
    result.ci_method = "wald_z_tost";
    result.first_count = first_trials;
    result.second_count = second_trials;
    if (first_trials == 0 || second_trials == 0
        || first_events > first_trials || second_events > second_trials) {
        add_error(result.diagnostics, "invalid_binomial_counts",
                  "两组试验数必须为正，且事件数不能超过试验数。");
        return result;
    }
    const double p1 = static_cast<double>(first_events) / static_cast<double>(first_trials);
    const double p2 = static_cast<double>(second_events)
        / static_cast<double>(second_trials);
    result.first_mean = p1;
    result.second_mean = p2;
    result.first_standard_deviation = static_cast<double>(first_events);
    result.second_standard_deviation = static_cast<double>(second_events);
    (void)first_row_count;
    (void)second_row_count;
    (void)first_missing;
    (void)second_missing;
    const double se = std::sqrt(
        p1 * (1.0 - p1) / static_cast<double>(first_trials)
        + p2 * (1.0 - p2) / static_cast<double>(second_trials));
    fill_z_tost(result, p1 - p2, se, lower, upper, confidence_level);
    return result;
}

EquivalenceTestResult two_sample_equivalence_ratio_test(
    const std::vector<double>& test_sample,
    const std::vector<double>& reference_sample,
    const double lower,
    const double upper,
    const double confidence_level,
    const VarianceMethod variance_method,
    const bool log_transform)
{
    EquivalenceTestResult result;
    result.kind = "two_sample_ratio";
    result.ci_method = log_transform ? "tost_ratio_log_1_minus_alpha"
                                     : "tost_ratio_1_minus_alpha";
    result.variance_method = variance_method;
    result.confidence_level = confidence_level;
    result.alpha = 1.0 - confidence_level;
    result.lower = lower;
    result.upper = upper;

    if (!(confidence_level > 0.0 && confidence_level < 1.0)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    if (!(lower > 0.0) || !(lower < upper) || !std::isfinite(lower)
        || !std::isfinite(upper)) {
        add_error(result.diagnostics, "invalid_equivalence_limits",
                  "比值等价下限必须大于 0 且小于上限。");
        return result;
    }

    if (log_transform) {
        const auto all_positive = [](const std::vector<double>& values) {
            return std::all_of(values.cbegin(), values.cend(), [](const double value) {
                return std::isfinite(value) && value > 0.0;
            });
        };
        if (test_sample.empty() || reference_sample.empty()
            || !all_positive(test_sample) || !all_positive(reference_sample)) {
            add_error(result.diagnostics, "nonpositive_for_log_ratio",
                      "对数均值比 TOST 要求检验列与参考列的全部观测均为正值。");
            return result;
        }
        const TwoSampleTTestResult raw_summary = two_sample_t_test(
            test_sample, reference_sample, confidence_level,
            TestAlternative::two_sided, variance_method);
        result.first_count = raw_summary.first.count;
        result.second_count = raw_summary.second.count;
        result.first_mean = raw_summary.first.mean;
        result.second_mean = raw_summary.second.mean;
        result.first_standard_deviation = raw_summary.first.sample_standard_deviation;
        result.second_standard_deviation = raw_summary.second.sample_standard_deviation;

        std::vector<double> log_test;
        std::vector<double> log_reference;
        log_test.reserve(test_sample.size());
        log_reference.reserve(reference_sample.size());
        for (const double value : test_sample) {
            log_test.push_back(std::log(value));
        }
        for (const double value : reference_sample) {
            log_reference.push_back(std::log(value));
        }
        EquivalenceTestResult log_result = two_sample_equivalence_test(
            log_test, log_reference, std::log(lower), std::log(upper),
            confidence_level, variance_method);
        result.diagnostics = log_result.diagnostics;
        if (!result.diagnostics.empty()) {
            return result;
        }
        result.standard_error = log_result.standard_error;
        result.degrees_of_freedom = log_result.degrees_of_freedom;
        result.t_lower = log_result.t_lower;
        result.t_upper = log_result.t_upper;
        result.p_lower = log_result.p_lower;
        result.p_upper = log_result.p_upper;
        result.both_pvalues_below_alpha = log_result.both_pvalues_below_alpha;
        result.within_limits = log_result.within_limits;
        result.difference = std::exp(log_result.difference);
        if (log_result.confidence_lower.has_value()) {
            result.confidence_lower = std::exp(*log_result.confidence_lower);
        }
        if (log_result.confidence_upper.has_value()) {
            result.confidence_upper = std::exp(*log_result.confidence_upper);
        }
        return result;
    }

    const TwoSampleTTestResult summary = two_sample_t_test(
        test_sample, reference_sample, confidence_level,
        TestAlternative::two_sided, variance_method);
    result.diagnostics = summary.diagnostics;
    result.first_count = summary.first.count;
    result.second_count = summary.second.count;
    result.first_mean = summary.first.mean;
    result.second_mean = summary.second.mean;
    result.first_standard_deviation = summary.first.sample_standard_deviation;
    result.second_standard_deviation = summary.second.sample_standard_deviation;
    if (!result.diagnostics.empty()) {
        return result;
    }
    if (!(result.second_mean > 0.0) || !std::isfinite(result.second_mean)) {
        add_error(result.diagnostics, "nonpositive_reference_mean",
                  "参考均值必须为正，才能计算均值比 TOST。");
        return result;
    }
    if (!std::isfinite(result.first_mean)) {
        add_error(result.diagnostics, "invalid_test_mean",
                  "检验均值不可用，无法计算均值比。");
        return result;
    }

    const double n1 = static_cast<double>(result.first_count);
    const double n2 = static_cast<double>(result.second_count);
    const double s1 = result.first_standard_deviation;
    const double s2 = result.second_standard_deviation;
    const double v1 = s1 * s1;
    const double v2 = s2 * s2;
    const double y1 = result.first_mean;
    const double y2 = result.second_mean;
    const double ratio = y1 / y2;
    result.difference = ratio;

    const auto se_at = [&](const double delta) -> double {
        if (variance_method == VarianceMethod::pooled) {
            const double pooled_variance =
                ((n1 - 1.0) * v1 + (n2 - 1.0) * v2) / (n1 + n2 - 2.0);
            return std::sqrt(pooled_variance * (1.0 / n1 + delta * delta / n2));
        }
        return std::sqrt(v1 / n1 + delta * delta * v2 / n2);
    };

    double degrees = 0.0;
    if (variance_method == VarianceMethod::pooled) {
        degrees = n1 + n2 - 2.0;
    } else {
        const double a = v1 / n1;
        const double b = ratio * ratio * v2 / n2;
        const double denom = a * a / (n1 - 1.0) + b * b / (n2 - 1.0);
        degrees = denom > 0.0 ? (a + b) * (a + b) / denom : 0.0;
    }
    result.degrees_of_freedom = degrees;
    const double se_lower = se_at(lower);
    const double se_upper = se_at(upper);
    result.standard_error = se_at(ratio);
    if (!(se_lower > 0.0) || !(se_upper > 0.0) || !(degrees > 0.0)) {
        add_error(result.diagnostics, "zero_variance",
                  "标准误为 0 或自由度不可用，无法计算均值比 TOST。");
        return result;
    }

    result.t_lower = (y1 - lower * y2) / se_lower;
    result.t_upper = (y1 - upper * y2) / se_upper;
    result.p_lower = 1.0 - student_t_cdf(result.t_lower, degrees);
    result.p_upper = student_t_cdf(result.t_upper, degrees);
    result.both_pvalues_below_alpha = result.p_lower.has_value()
        && result.p_upper.has_value()
        && *result.p_lower <= result.alpha
        && *result.p_upper <= result.alpha;
    result.within_limits = result.both_pvalues_below_alpha;

    const double t_crit = student_t_quantile(1.0 - result.alpha, degrees);
    const double t2 = t_crit * t_crit;
    double term_ref = 0.0;
    double term_test = 0.0;
    if (variance_method == VarianceMethod::pooled) {
        const double pooled_variance =
            ((n1 - 1.0) * v1 + (n2 - 1.0) * v2) / (n1 + n2 - 2.0);
        term_ref = pooled_variance / n2;
        term_test = pooled_variance / n1;
    } else {
        term_ref = v2 / n2;
        term_test = v1 / n1;
    }
    const double A = y2 * y2 - t2 * term_ref;
    const double B = y1 * y2;
    const double C = y1 * y1 - t2 * term_test;
    if (!(A > 0.0)) {
        add_error(result.diagnostics, "fieller_unavailable",
                  "Fieller 条件不满足，无法计算比值置信区间。");
        return result;
    }
    const double disc = B * B - A * C;
    if (!(disc >= 0.0) || !std::isfinite(disc)) {
        add_error(result.diagnostics, "fieller_unavailable",
                  "Fieller 判别式无效，无法计算比值置信区间。");
        return result;
    }
    const double root1 = (B - std::sqrt(disc)) / A;
    const double root2 = (B + std::sqrt(disc)) / A;
    result.confidence_lower = std::min(root1, root2);
    result.confidence_upper = std::max(root1, root2);
    return result;
}

}  // namespace datalab::domain::statistics
