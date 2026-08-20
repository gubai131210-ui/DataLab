#include "domain/statistics/poisson_rate.h"

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

double poisson_pmf(std::size_t k, double lambda)
{
    if (!(lambda >= 0.0) || !std::isfinite(lambda)) {
        return 0.0;
    }
    if (lambda == 0.0) {
        return k == 0 ? 1.0 : 0.0;
    }
    return std::exp(-lambda + static_cast<double>(k) * std::log(lambda)
        - std::lgamma(static_cast<double>(k) + 1.0));
}

double poisson_cdf_le(std::size_t x, double lambda)
{
    double sum = 0.0;
    for (std::size_t index = 0; index <= x; ++index) {
        sum += poisson_pmf(index, lambda);
    }
    return std::clamp(sum, 0.0, 1.0);
}

double poisson_cdf_ge(std::size_t x, double lambda)
{
    if (x == 0) {
        return 1.0;
    }
    return std::clamp(1.0 - poisson_cdf_le(x - 1, lambda), 0.0, 1.0);
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

OnePoissonRateResult one_poisson_rate_test(
    std::size_t events,
    double exposure,
    double hypothesized,
    double confidence_level,
    TestAlternative alternative,
    ProportionMethod method,
    std::size_t row_count,
    std::size_t missing_count)
{
    OnePoissonRateResult result;
    result.events = events;
    result.exposure = exposure;
    result.row_count = row_count;
    result.missing_count = missing_count;
    result.hypothesized = hypothesized;
    result.confidence_level = confidence_level;
    result.alternative = alternative;
    result.method = method == ProportionMethod::normal ? "normal" : "exact";
    if (!(confidence_level > 0.0 && confidence_level < 1.0)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    if (!(exposure > 0.0) || !std::isfinite(exposure)) {
        add_error(result.diagnostics, "invalid_poisson_exposure",
                  "单样本泊松率要求观测长度大于 0。");
        return result;
    }
    if (!(hypothesized > 0.0) || !std::isfinite(hypothesized)) {
        add_error(result.diagnostics, "invalid_hypothesized_rate",
                  "假设发生率必须大于 0。");
        return result;
    }
    result.rate = static_cast<double>(events) / exposure;
    if (row_count > 1) {
        add_warning(result.diagnostics, "summarized_from_multiple_rows",
                    "已将多行缺陷数/观测长度求和后再做单样本泊松率检验。");
    }
    if (missing_count > 0) {
        add_warning(result.diagnostics, "missing_values",
                    "跳过了缺失或非法单元格。");
    }
    const double alpha = 1.0 - confidence_level;
    const double interval_alpha = alternative == TestAlternative::two_sided
        ? alpha : std::min(0.999999, 2.0 * alpha);
    if (method == ProportionMethod::exact) {
        const auto interval = garwood_rate(
            static_cast<double>(events), exposure, interval_alpha);
        if (alternative == TestAlternative::less) {
            result.confidence_upper = interval.upper;
        } else if (alternative == TestAlternative::greater) {
            result.confidence_lower = interval.lower;
        } else {
            result.confidence_lower = interval.lower;
            result.confidence_upper = interval.upper;
        }
        const double mean0 = hypothesized * exposure;
        const double lower_tail = poisson_cdf_le(events, mean0);
        const double upper_tail = poisson_cdf_ge(events, mean0);
        if (alternative == TestAlternative::less) {
            result.p_value = lower_tail;
        } else if (alternative == TestAlternative::greater) {
            result.p_value = upper_tail;
        } else {
            result.p_value = std::min(1.0, 2.0 * std::min(lower_tail, upper_tail));
        }
        return result;
    }

    const double null_variance = hypothesized / exposure;
    if (!(null_variance > 0.0)) {
        add_error(result.diagnostics, "zero_rate_variance",
                  "假设发生率标准误为 0，无法计算正态近似检验。");
        return result;
    }
    result.z_statistic = (result.rate - hypothesized) / std::sqrt(null_variance);
    const double cdf = standard_normal_cdf(*result.z_statistic);
    result.p_value = alternative == TestAlternative::less
        ? cdf
        : alternative == TestAlternative::greater
            ? 1.0 - cdf
            : std::clamp(2.0 * (1.0 - standard_normal_cdf(std::abs(*result.z_statistic))),
                         0.0, 1.0);
    if (!(result.rate > 0.0)) {
        add_error(result.diagnostics, "zero_rate_variance",
                  "样本率为 0，无法计算 Wald 置信区间。");
        return result;
    }
    const double se = std::sqrt(result.rate / exposure);
    const double critical = alternative == TestAlternative::two_sided
        ? standard_normal_quantile(0.5 + confidence_level / 2.0)
        : standard_normal_quantile(confidence_level);
    if (alternative == TestAlternative::less) {
        result.confidence_upper = result.rate + critical * se;
    } else if (alternative == TestAlternative::greater) {
        result.confidence_lower = result.rate - critical * se;
    } else {
        result.confidence_lower = result.rate - critical * se;
        result.confidence_upper = result.rate + critical * se;
    }
    if (events <= 10) {
        add_warning(result.diagnostics, "small_count_normal_approximation",
                    "总发生数不大于 10，正态近似可能不准确。");
    }
    return result;
}

TwoPoissonRateResult two_poisson_rate_test(
    std::size_t first_events,
    double first_exposure,
    std::size_t second_events,
    double second_exposure,
    double confidence_level,
    TestAlternative alternative,
    ProportionMethod method,
    const std::string& comparison)
{
    TwoPoissonRateResult result;
    result.first_events = first_events;
    result.first_exposure = first_exposure;
    result.second_events = second_events;
    result.second_exposure = second_exposure;
    result.confidence_level = confidence_level;
    result.alternative = alternative;
    result.method = method == ProportionMethod::normal ? "normal" : "exact";
    result.comparison = comparison == "ratio" ? "ratio" : "difference";
    result.ci_method = result.comparison == "ratio" ? "log_wald" : "wald_difference";
    if (!(confidence_level > 0.0 && confidence_level < 1.0)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    if (!(first_exposure > 0.0) || !(second_exposure > 0.0)
        || !std::isfinite(first_exposure) || !std::isfinite(second_exposure)) {
        add_error(result.diagnostics, "invalid_poisson_exposure",
                  "双样本泊松率要求两组观测长度都大于 0。");
        return result;
    }
    result.first_rate = static_cast<double>(first_events) / first_exposure;
    result.second_rate = static_cast<double>(second_events) / second_exposure;
    result.difference = result.first_rate - result.second_rate;
    if (result.second_rate > 0.0) {
        result.ratio = result.first_rate / result.second_rate;
    }

    if (result.comparison == "ratio") {
        if (first_events == 0 || second_events == 0) {
            add_warning(result.diagnostics, "zero_events_for_rate_ratio",
                        "率比的 log-Wald 区间需要两组事件数都大于 0。");
        } else {
            const double log_ratio = std::log(*result.ratio);
            const double se = std::sqrt(1.0 / static_cast<double>(first_events)
                + 1.0 / static_cast<double>(second_events));
            result.z_statistic = log_ratio / se;
            const double cdf = standard_normal_cdf(*result.z_statistic);
            result.p_value = alternative == TestAlternative::less
                ? cdf
                : alternative == TestAlternative::greater
                    ? 1.0 - cdf
                    : std::clamp(
                          2.0 * (1.0 - standard_normal_cdf(std::abs(*result.z_statistic))),
                          0.0, 1.0);
            const double critical = alternative == TestAlternative::two_sided
                ? standard_normal_quantile(0.5 + confidence_level / 2.0)
                : standard_normal_quantile(confidence_level);
            if (alternative == TestAlternative::less) {
                result.confidence_upper = std::exp(log_ratio + critical * se);
            } else if (alternative == TestAlternative::greater) {
                result.confidence_lower = std::exp(log_ratio - critical * se);
            } else {
                result.confidence_lower = std::exp(log_ratio - critical * se);
                result.confidence_upper = std::exp(log_ratio + critical * se);
            }
        }
        if (method == ProportionMethod::exact && first_events + second_events > 0) {
            const double p0 = first_exposure / (first_exposure + second_exposure);
            const std::size_t total = first_events + second_events;
            const double lower_tail = binomial_cdf_le(first_events, total, p0);
            const double upper_tail = binomial_cdf_ge(first_events, total, p0);
            if (alternative == TestAlternative::less) {
                result.p_value = lower_tail;
            } else if (alternative == TestAlternative::greater) {
                result.p_value = upper_tail;
            } else {
                result.p_value = std::min(1.0, 2.0 * std::min(lower_tail, upper_tail));
            }
            result.method = "exact";
            if (first_events > 0 && second_events > 0) {
                result.ci_method = "log_wald";
            }
        }
        return result;
    }

    if (method == ProportionMethod::exact) {
        const double p0 = first_exposure / (first_exposure + second_exposure);
        const std::size_t total = first_events + second_events;
        const double lower_tail = binomial_cdf_le(first_events, total, p0);
        const double upper_tail = binomial_cdf_ge(first_events, total, p0);
        if (alternative == TestAlternative::less) {
            result.p_value = lower_tail;
        } else if (alternative == TestAlternative::greater) {
            result.p_value = upper_tail;
        } else {
            result.p_value = std::min(1.0, 2.0 * std::min(lower_tail, upper_tail));
        }
        return result;
    }

    const double se_squared = result.first_rate / first_exposure
        + result.second_rate / second_exposure;
    if (!(se_squared > 0.0)) {
        add_error(result.diagnostics, "zero_rate_variance",
                  "两组率的 Wald 标准误为 0，无法计算正态近似。");
        return result;
    }
    const double se = std::sqrt(se_squared);
    result.z_statistic = result.difference / se;
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
    if (alternative == TestAlternative::less) {
        result.confidence_upper = result.difference + critical * se;
    } else if (alternative == TestAlternative::greater) {
        result.confidence_lower = result.difference - critical * se;
    } else {
        result.confidence_lower = result.difference - critical * se;
        result.confidence_upper = result.difference + critical * se;
    }
    if (first_events + second_events <= 10) {
        add_warning(result.diagnostics, "small_count_normal_approximation",
                    "两组总发生数不大于 10，正态近似可能不准确。");
    }
    return result;
}

}  // namespace datalab::domain::statistics
