#include "domain/statistics/proportion_test.h"

#include "domain/statistics/attribute_capability.h"
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

void add_warning(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::warning, code, message});
}

double log_combination(std::size_t n, std::size_t k)
{
    if (k > n) {
        return -std::numeric_limits<double>::infinity();
    }
    const std::size_t reduced = std::min(k, n - k);
    double value = 0.0;
    for (std::size_t index = 1; index <= reduced; ++index) {
        value += std::log(static_cast<double>(n - reduced + index))
            - std::log(static_cast<double>(index));
    }
    return value;
}

double binomial_pmf(std::size_t x, std::size_t n, double p)
{
    if (p == 0.0) {
        return x == 0 ? 1.0 : 0.0;
    }
    if (p == 1.0) {
        return x == n ? 1.0 : 0.0;
    }
    return std::exp(log_combination(n, x)
        + static_cast<double>(x) * std::log(p)
        + static_cast<double>(n - x) * std::log(1.0 - p));
}

double binomial_cdf_le(std::size_t x, std::size_t n, double p)
{
    if (x >= n) {
        return 1.0;
    }
    double sum = 0.0;
    for (std::size_t index = 0; index <= x; ++index) {
        sum += binomial_pmf(index, n, p);
    }
    return std::clamp(sum, 0.0, 1.0);
}

double binomial_cdf_ge(std::size_t x, std::size_t n, double p)
{
    if (x == 0) {
        return 1.0;
    }
    if (x > n) {
        return 0.0;
    }
    return std::clamp(1.0 - binomial_cdf_le(x - 1, n, p), 0.0, 1.0);
}

}  // namespace

OneProportionResult one_proportion_test(
    std::size_t events,
    std::size_t trials,
    double hypothesized,
    double confidence_level,
    TestAlternative alternative,
    ProportionMethod method,
    std::size_t row_count,
    std::size_t missing_count)
{
    OneProportionResult result;
    result.events = events;
    result.trials = trials;
    result.row_count = row_count;
    result.missing_count = missing_count;
    result.hypothesized = hypothesized;
    result.confidence_level = confidence_level;
    result.alternative = alternative;
    if (method == ProportionMethod::normal) {
        result.method = "normal";
        result.ci_method = "wald";
    } else if (method == ProportionMethod::wilson) {
        result.method = "wilson";
        result.ci_method = "wilson_score";
    } else if (method == ProportionMethod::agresti_coull) {
        result.method = "agresti_coull";
        result.ci_method = "agresti_coull";
    } else {
        result.method = "exact";
        result.ci_method = "clopper_pearson";
    }
    if (!(confidence_level > 0.0 && confidence_level < 1.0)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    if (trials == 0 || events > trials) {
        add_error(result.diagnostics, "invalid_proportion_counts",
                  "单比例检验要求试验数大于 0，且事件数不能超过试验数。");
        return result;
    }
    if (!(hypothesized > 0.0 && hypothesized < 1.0)) {
        add_error(result.diagnostics, "invalid_hypothesized_proportion",
                  "假设比例必须位于 (0, 1) 内。");
        return result;
    }
    result.proportion = static_cast<double>(events) / static_cast<double>(trials);
    if (row_count > 1) {
        add_warning(result.diagnostics, "summarized_from_multiple_rows",
                    "已将多行事件数/试验数求和后再做单比例检验。");
    }
    if (missing_count > 0) {
        add_warning(result.diagnostics, "missing_values",
                    "跳过了缺失或非法单元格。");
    }
    const double alpha = 1.0 - confidence_level;
    const double interval_alpha = alternative == TestAlternative::two_sided
        ? alpha : std::min(0.999999, 2.0 * alpha);
    if (method == ProportionMethod::exact) {
        const auto interval = clopper_pearson_interval(
            static_cast<double>(events), static_cast<double>(trials), interval_alpha);
        if (alternative == TestAlternative::less) {
            result.confidence_upper = interval.upper;
        } else if (alternative == TestAlternative::greater) {
            result.confidence_lower = interval.lower;
        } else {
            result.confidence_lower = interval.lower;
            result.confidence_upper = interval.upper;
        }
        const double lower_tail = binomial_cdf_le(events, trials, hypothesized);
        const double upper_tail = binomial_cdf_ge(events, trials, hypothesized);
        if (alternative == TestAlternative::less) {
            result.p_value = lower_tail;
        } else if (alternative == TestAlternative::greater) {
            result.p_value = upper_tail;
        } else {
            result.p_value = std::min(1.0, 2.0 * std::min(lower_tail, upper_tail));
        }
        return result;
    }

    const double n = static_cast<double>(trials);
    const double null_variance = hypothesized * (1.0 - hypothesized) / n;
    if (!(null_variance > 0.0)) {
        add_error(result.diagnostics, "zero_proportion_variance",
                  "假设比例标准误为 0，无法计算正态近似检验。");
        return result;
    }
    result.z_statistic = (result.proportion - hypothesized) / std::sqrt(null_variance);
    const double cdf = standard_normal_cdf(*result.z_statistic);
    result.p_value = alternative == TestAlternative::less
        ? cdf
        : alternative == TestAlternative::greater
            ? 1.0 - cdf
            : std::clamp(2.0 * (1.0 - standard_normal_cdf(std::abs(*result.z_statistic))),
                         0.0, 1.0);

    const double critical = alternative == TestAlternative::two_sided
        ? standard_normal_quantile(0.5 + confidence_level / 2.0)
        : standard_normal_quantile(confidence_level);

    if (method == ProportionMethod::wilson) {
        const double z2 = critical * critical;
        const double denom = 1.0 + z2 / n;
        const double center = (result.proportion + z2 / (2.0 * n)) / denom;
        const double half = critical
            * std::sqrt(result.proportion * (1.0 - result.proportion) / n
                        + z2 / (4.0 * n * n))
            / denom;
        double lower = center - half;
        double upper = center + half;
        if (events == 0) {
            lower = 0.0;
        }
        if (events == trials) {
            upper = 1.0;
        }
        lower = std::clamp(lower, 0.0, 1.0);
        upper = std::clamp(upper, 0.0, 1.0);
        if (alternative == TestAlternative::less) {
            result.confidence_upper = upper;
        } else if (alternative == TestAlternative::greater) {
            result.confidence_lower = lower;
        } else {
            result.confidence_lower = lower;
            result.confidence_upper = upper;
        }
        return result;
    }

    if (method == ProportionMethod::agresti_coull) {
        const double z2 = critical * critical;
        const double n_tilde = n + z2;
        const double p_tilde = (static_cast<double>(events) + z2 / 2.0) / n_tilde;
        const double se_tilde = std::sqrt(p_tilde * (1.0 - p_tilde) / n_tilde);
        double lower = p_tilde - critical * se_tilde;
        double upper = p_tilde + critical * se_tilde;
        if (events == 0) {
            lower = 0.0;
        }
        if (events == trials) {
            upper = 1.0;
        }
        lower = std::clamp(lower, 0.0, 1.0);
        upper = std::clamp(upper, 0.0, 1.0);
        if (alternative == TestAlternative::less) {
            result.confidence_upper = upper;
        } else if (alternative == TestAlternative::greater) {
            result.confidence_lower = lower;
        } else {
            result.confidence_lower = lower;
            result.confidence_upper = upper;
        }
        return result;
    }

    const double sample_variance =
        result.proportion * (1.0 - result.proportion) / n;
    if (!(sample_variance > 0.0)) {
        add_error(result.diagnostics, "zero_proportion_variance",
                  "样本比例为 0 或 1，无法计算 Wald 置信区间。");
        return result;
    }
    const double se = std::sqrt(sample_variance);
    if (alternative == TestAlternative::less) {
        result.confidence_upper = result.proportion + critical * se;
    } else if (alternative == TestAlternative::greater) {
        result.confidence_lower = result.proportion - critical * se;
    } else {
        result.confidence_lower = result.proportion - critical * se;
        result.confidence_upper = result.proportion + critical * se;
    }
    const double expected_events = n * hypothesized;
    const double expected_nonevents = n * (1.0 - hypothesized);
    if (expected_events < 5.0 || expected_nonevents < 5.0) {
        add_warning(result.diagnostics, "small_count_normal_approximation",
                    "n p0 或 n(1-p0) 小于 5，正态近似可能不准确。");
    }
    return result;
}

}  // namespace datalab::domain::statistics
