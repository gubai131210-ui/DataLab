#include "domain/statistics/hypothesis_tests.h"

#include "domain/statistics/descriptive_statistics.h"
#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

void add_error(std::vector<DiagnosticMessage>& diagnostics, const char* code, const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

bool valid_confidence(double confidence_level)
{
    return confidence_level > 0.0 && confidence_level < 1.0;
}

double beta_continued_fraction(double a, double b, double x)
{
    constexpr int max_iterations = 200;
    constexpr double epsilon = 3.0e-14;
    constexpr double tiny = 1.0e-300;
    const double qab = a + b;
    const double qap = a + 1.0;
    const double qam = a - 1.0;
    double c = 1.0;
    double d = 1.0 - qab * x / qap;
    d = std::abs(d) < tiny ? tiny : d;
    d = 1.0 / d;
    double h = d;
    for (int iteration = 1; iteration <= max_iterations; ++iteration) {
        const double m = static_cast<double>(iteration);
        const double m2 = 2.0 * m;
        double numerator = m * (b - m) * x / ((qam + m2) * (a + m2));
        d = 1.0 + numerator * d;
        d = std::abs(d) < tiny ? tiny : d;
        c = 1.0 + numerator / c;
        c = std::abs(c) < tiny ? tiny : c;
        d = 1.0 / d;
        h *= d * c;
        numerator = -(a + m) * (qab + m) * x
            / ((a + m2) * (qap + m2));
        d = 1.0 + numerator * d;
        d = std::abs(d) < tiny ? tiny : d;
        c = 1.0 + numerator / c;
        c = std::abs(c) < tiny ? tiny : c;
        d = 1.0 / d;
        const double delta = d * c;
        h *= delta;
        if (std::abs(delta - 1.0) < epsilon) {
            break;
        }
    }
    return h;
}

double regularized_beta(double x, double a, double b)
{
    if (x <= 0.0) {
        return 0.0;
    }
    if (x >= 1.0) {
        return 1.0;
    }
    const double logarithm = std::lgamma(a + b) - std::lgamma(a) - std::lgamma(b)
        + a * std::log(x) + b * std::log1p(-x);
    const double factor = std::exp(logarithm);
    if (x < (a + 1.0) / (a + b + 2.0)) {
        return factor * beta_continued_fraction(a, b, x) / a;
    }
    return 1.0 - factor * beta_continued_fraction(b, a, 1.0 - x) / b;
}

double two_sided_p_value(double statistic, double degrees_of_freedom)
{
    return std::clamp(1.0 - student_t_cdf(std::abs(statistic), degrees_of_freedom), 0.0, 1.0)
        * 2.0;
}

void fill_confidence_interval(
    TTestResult& result,
    TestAlternative alternative)
{
    if (!(result.standard_error >= 0.0) || !result.p_value.has_value()) {
        return;
    }
    const double alpha = 1.0 - result.confidence_level;
    const double critical = alternative == TestAlternative::two_sided
        ? student_t_quantile(1.0 - alpha / 2.0, result.degrees_of_freedom)
        : student_t_quantile(1.0 - alpha, result.degrees_of_freedom);
    if (alternative == TestAlternative::less) {
        result.confidence_upper = result.difference + critical * result.standard_error;
    } else if (alternative == TestAlternative::greater) {
        result.confidence_lower = result.difference - critical * result.standard_error;
    } else {
        result.confidence_lower = result.difference - critical * result.standard_error;
        result.confidence_upper = result.difference + critical * result.standard_error;
    }
}

TTestResult summary(const std::vector<double>& observations)
{
    TTestResult result;
    result.count = observations.size();
    if (observations.size() < 2) {
        add_error(result.diagnostics, "insufficient_observations",
                  "t 检验至少需要两个有效观测。");
        return result;
    }
    const double total = std::accumulate(observations.cbegin(), observations.cend(), 0.0);
    result.mean = total / static_cast<double>(observations.size());
    double sum_squared = 0.0;
    for (const double observation : observations) {
        sum_squared += (observation - result.mean) * (observation - result.mean);
    }
    result.sample_standard_deviation = std::sqrt(
        sum_squared / static_cast<double>(observations.size() - 1));
    result.standard_error = result.sample_standard_deviation
        / std::sqrt(static_cast<double>(observations.size()));
    return result;
}

}  // namespace

double student_t_cdf(double value, double degrees_of_freedom)
{
    if (!(degrees_of_freedom > 0.0) || !std::isfinite(value)) {
        return value > 0.0 ? 1.0 : 0.0;
    }
    if (value == 0.0) {
        return 0.5;
    }
    const double x = degrees_of_freedom
        / (degrees_of_freedom + value * value);
    const double beta = regularized_beta(x, degrees_of_freedom / 2.0, 0.5);
    return value > 0.0 ? 1.0 - 0.5 * beta : 0.5 * beta;
}

double student_t_quantile(double probability, double degrees_of_freedom)
{
    if (!(probability > 0.0 && probability < 1.0) || !(degrees_of_freedom > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double lower = -100.0;
    double upper = 100.0;
    for (int iteration = 0; iteration < 160; ++iteration) {
        const double middle = 0.5 * (lower + upper);
        if (student_t_cdf(middle, degrees_of_freedom) < probability) {
            lower = middle;
        } else {
            upper = middle;
        }
    }
    return 0.5 * (lower + upper);
}

double f_right_tail(double value, double numerator_df, double denominator_df)
{
    if (!(value >= 0.0) || !(numerator_df > 0.0) || !(denominator_df > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double x = denominator_df / (denominator_df + numerator_df * value);
    return regularized_beta(x, denominator_df / 2.0, numerator_df / 2.0);
}

TTestResult one_sample_t_test(
    const std::vector<double>& observations,
    double hypothesized_mean,
    double confidence_level,
    TestAlternative alternative)
{
    TTestResult result = summary(observations);
    result.hypothesized_mean = hypothesized_mean;
    result.confidence_level = confidence_level;
    if (!valid_confidence(confidence_level)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    if (!result.diagnostics.empty()) {
        return result;
    }
    result.difference = result.mean - hypothesized_mean;
    result.degrees_of_freedom = static_cast<double>(result.count - 1);
    if (result.standard_error == 0.0) {
        if (result.difference == 0.0) {
            result.t_statistic = 0.0;
            result.p_value = 1.0;
        } else {
            add_error(result.diagnostics, "zero_variance",
                      "样本标准差为 0，无法计算非零均值差异的 t 检验。");
            return result;
        }
    } else {
        result.t_statistic = result.difference / result.standard_error;
        const double cdf = student_t_cdf(result.t_statistic, result.degrees_of_freedom);
        if (alternative == TestAlternative::less) {
            result.p_value = cdf;
        } else if (alternative == TestAlternative::greater) {
            result.p_value = 1.0 - cdf;
        } else {
            result.p_value = two_sided_p_value(result.t_statistic, result.degrees_of_freedom);
        }
    }
    fill_confidence_interval(result, alternative);
    return result;
}

TwoSampleTTestResult two_sample_t_test(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double confidence_level,
    TestAlternative alternative,
    VarianceMethod variance_method)
{
    TwoSampleTTestResult result;
    result.first = summary(first);
    result.second = summary(second);
    result.confidence_level = confidence_level;
    result.variance_method = variance_method;
    if (!valid_confidence(confidence_level)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    if (!result.first.diagnostics.empty() || !result.second.diagnostics.empty()) {
        add_error(result.diagnostics, "insufficient_group_data",
                  "双样本 t 检验的每组至少需要两个有效观测。");
        return result;
    }
    result.mean_difference = result.first.mean - result.second.mean;
    const double first_variance = result.first.sample_standard_deviation
        * result.first.sample_standard_deviation;
    const double second_variance = result.second.sample_standard_deviation
        * result.second.sample_standard_deviation;
    const double first_size = static_cast<double>(first.size());
    const double second_size = static_cast<double>(second.size());
    if (variance_method == VarianceMethod::pooled) {
        const double pooled_variance = (
            static_cast<double>(first.size() - 1) * first_variance
            + static_cast<double>(second.size() - 1) * second_variance)
            / static_cast<double>(first.size() + second.size() - 2);
        result.standard_error_difference = std::sqrt(
            pooled_variance * (1.0 / first_size + 1.0 / second_size));
        result.degrees_of_freedom = static_cast<double>(first.size() + second.size() - 2);
    } else {
        const double first_term = first_variance / first_size;
        const double second_term = second_variance / second_size;
        result.standard_error_difference = std::sqrt(first_term + second_term);
        const double denominator = first_term * first_term / (first_size - 1.0)
            + second_term * second_term / (second_size - 1.0);
        result.degrees_of_freedom = denominator > 0.0
            ? (first_term + second_term) * (first_term + second_term) / denominator
            : 0.0;
    }
    if (result.standard_error_difference == 0.0) {
        if (result.mean_difference == 0.0) {
            result.t_statistic = 0.0;
            result.p_value = 1.0;
        } else {
            add_error(result.diagnostics, "zero_variance",
                      "两组方差均为 0 且均值不同，无法计算有限 t 统计量。");
            return result;
        }
    } else {
        result.t_statistic = result.mean_difference / result.standard_error_difference;
        const double cdf = student_t_cdf(result.t_statistic, result.degrees_of_freedom);
        if (alternative == TestAlternative::less) {
            result.p_value = cdf;
        } else if (alternative == TestAlternative::greater) {
            result.p_value = 1.0 - cdf;
        } else {
            result.p_value = two_sided_p_value(result.t_statistic, result.degrees_of_freedom);
        }
    }
    const double alpha = 1.0 - confidence_level;
    const double critical = alternative == TestAlternative::two_sided
        ? student_t_quantile(1.0 - alpha / 2.0, result.degrees_of_freedom)
        : student_t_quantile(1.0 - alpha, result.degrees_of_freedom);
    if (alternative == TestAlternative::less) {
        result.confidence_upper = result.mean_difference
            + critical * result.standard_error_difference;
    } else if (alternative == TestAlternative::greater) {
        result.confidence_lower = result.mean_difference
            - critical * result.standard_error_difference;
    } else {
        result.confidence_lower = result.mean_difference
            - critical * result.standard_error_difference;
        result.confidence_upper = result.mean_difference
            + critical * result.standard_error_difference;
    }
    return result;
}

AnovaResult one_way_anova(
    const std::vector<std::vector<double>>& groups,
    const std::vector<std::string>& labels)
{
    AnovaResult result;
    if (groups.size() < 2) {
        add_error(result.diagnostics, "insufficient_groups",
                  "单因素 ANOVA 至少需要两个有效组。");
        return result;
    }
    if (!labels.empty() && labels.size() != groups.size()) {
        add_error(result.diagnostics, "invalid_group_labels",
                  "组标签数量必须与有效组数量一致。");
        return result;
    }
    for (std::size_t index = 0; index < groups.size(); ++index) {
        if (groups[index].empty()) {
            add_error(result.diagnostics, "empty_group",
                      "ANOVA 不允许存在没有有效观测的组。");
            return result;
        }
        AnovaGroupSummary summary;
        summary.label = labels.empty() ? std::to_string(index + 1) : labels[index];
        summary.count = groups[index].size();
        summary.mean = std::accumulate(groups[index].cbegin(), groups[index].cend(), 0.0)
            / static_cast<double>(summary.count);
        if (summary.count > 1) {
            double sum_squared = 0.0;
            for (const double value : groups[index]) {
                sum_squared += (value - summary.mean) * (value - summary.mean);
            }
            summary.sample_standard_deviation = std::sqrt(
                sum_squared / static_cast<double>(summary.count - 1));
        }
        result.groups.push_back(summary);
        result.total_count += summary.count;
    }
    result.grand_mean = 0.0;
    for (const auto& group : result.groups) {
        result.grand_mean += static_cast<double>(group.count) * group.mean;
    }
    result.grand_mean /= static_cast<double>(result.total_count);
    for (const auto& group : result.groups) {
        result.between_sum_of_squares += static_cast<double>(group.count)
            * (group.mean - result.grand_mean) * (group.mean - result.grand_mean);
    }
    for (std::size_t index = 0; index < groups.size(); ++index) {
        for (const double value : groups[index]) {
            result.total_sum_of_squares += (value - result.grand_mean)
                * (value - result.grand_mean);
            result.error_sum_of_squares += (value - result.groups[index].mean)
                * (value - result.groups[index].mean);
        }
    }
    result.between_degrees_of_freedom = result.groups.size() - 1;
    result.error_degrees_of_freedom = result.total_count - result.groups.size();
    result.total_degrees_of_freedom = result.total_count - 1;
    if (result.error_degrees_of_freedom == 0 || result.error_sum_of_squares == 0.0) {
        add_error(result.diagnostics, "zero_error_variance",
                  "组内误差平方和为 0，无法计算有限 F 统计量。");
        return result;
    }
    result.between_mean_square = result.between_sum_of_squares
        / static_cast<double>(result.between_degrees_of_freedom);
    result.error_mean_square = result.error_sum_of_squares
        / static_cast<double>(result.error_degrees_of_freedom);
    result.f_statistic = result.between_mean_square / result.error_mean_square;
    result.p_value = f_right_tail(
        result.f_statistic,
        static_cast<double>(result.between_degrees_of_freedom),
        static_cast<double>(result.error_degrees_of_freedom));
    return result;
}

}  // namespace datalab::domain::statistics
