#include "domain/statistics/variance_tests.h"

#include "domain/statistics/normality_test.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

constexpr double kEpsilon = 1.0e-12;

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

bool valid_confidence(double value)
{
    return value > 0.0 && value < 1.0;
}

bool finite_observations(
    const std::vector<double>& input,
    std::vector<double>& output,
    std::vector<DiagnosticMessage>& diagnostics)
{
    output = input;
    if (!std::all_of(output.cbegin(), output.cend(),
                     [](double value) { return std::isfinite(value); })) {
        add_error(diagnostics, "non_finite_observation",
                  "方差检验要求所有观测都是有限数值。");
        return false;
    }
    return true;
}

double sample_variance(const std::vector<double>& values)
{
    const double mean = std::accumulate(values.cbegin(), values.cend(), 0.0)
        / static_cast<double>(values.size());
    double sum = 0.0;
    for (const double value : values) {
        const double difference = value - mean;
        sum += difference * difference;
    }
    return sum / static_cast<double>(values.size() - 1);
}

double regularized_gamma_q(double shape, double value)
{
    if (!(shape > 0.0) || value < 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (value == 0.0) {
        return 1.0;
    }
    constexpr int kMaxIterations = 200;
    if (value < shape + 1.0) {
        double term = 1.0 / shape;
        double sum = term;
        for (int index = 1; index <= kMaxIterations; ++index) {
            term *= value / (shape + static_cast<double>(index));
            sum += term;
            if (std::abs(term) < std::abs(sum) * kEpsilon) {
                break;
            }
        }
        return std::clamp(
            1.0 - sum * std::exp(-value + shape * std::log(value) - std::lgamma(shape)),
            0.0, 1.0);
    }
    double factor = 1.0;
    double sum = 1.0;
    for (int index = 1; index <= kMaxIterations; ++index) {
        factor *= (shape - static_cast<double>(index)) / value;
        sum += factor;
        if (std::abs(factor) < std::abs(sum) * kEpsilon) {
            break;
        }
    }
    return std::clamp(
        std::exp(-value + shape * std::log(value) - std::lgamma(shape)) * sum,
        0.0, 1.0);
}

double chi_square_left_tail(double value, double degrees_of_freedom)
{
    return std::clamp(
        1.0 - regularized_gamma_q(degrees_of_freedom / 2.0, value / 2.0),
        0.0, 1.0);
}

double chi_square_quantile(double probability, double degrees_of_freedom)
{
    if (!(probability > 0.0 && probability < 1.0)
        || !(degrees_of_freedom > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double lower = 0.0;
    double upper = std::max(1.0, degrees_of_freedom);
    while (chi_square_left_tail(upper, degrees_of_freedom) < probability) {
        upper *= 2.0;
    }
    for (int index = 0; index < 160; ++index) {
        const double middle = (lower + upper) / 2.0;
        if (chi_square_left_tail(middle, degrees_of_freedom) < probability) {
            lower = middle;
        } else {
            upper = middle;
        }
    }
    return (lower + upper) / 2.0;
}

double f_left_tail(double value, double numerator_df, double denominator_df)
{
    if (value <= 0.0) {
        return 0.0;
    }
    return f_right_tail(1.0 / value, denominator_df, numerator_df);
}

double f_quantile(double probability, double numerator_df, double denominator_df)
{
    if (!(probability > 0.0 && probability < 1.0)
        || !(numerator_df > 0.0) || !(denominator_df > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double lower = 0.0;
    double upper = 1.0;
    while (f_left_tail(upper, numerator_df, denominator_df) < probability) {
        upper *= 2.0;
    }
    for (int index = 0; index < 160; ++index) {
        const double middle = (lower + upper) / 2.0;
        if (f_left_tail(middle, numerator_df, denominator_df) < probability) {
            lower = middle;
        } else {
            upper = middle;
        }
    }
    return (lower + upper) / 2.0;
}

void add_normality_risk(
    const std::vector<double>& observations,
    std::vector<DiagnosticMessage>& diagnostics,
    const char* group_name)
{
    const NormalityTestResult normality = normality_test(observations);
    if (normality.p_value.has_value() && *normality.p_value < 0.05) {
        add_warning(diagnostics, "normality_risk",
                    group_name);
    } else if (!normality.p_value.has_value()) {
        add_warning(diagnostics, "normality_not_assessed",
                    "样本不足或零方差，无法完成正态性风险诊断。");
    }
}

void fill_alternative_p_values(
    double left_tail,
    double right_tail,
    std::optional<double>& selected,
    std::optional<double>& less,
    std::optional<double>& greater,
    std::optional<double>& two_sided,
    TestAlternative alternative)
{
    less = std::clamp(left_tail, 0.0, 1.0);
    greater = std::clamp(right_tail, 0.0, 1.0);
    two_sided = std::clamp(2.0 * std::min(*less, *greater), 0.0, 1.0);
    selected = alternative == TestAlternative::less
        ? less : alternative == TestAlternative::greater ? greater : two_sided;
}

void fill_ratio_confidence_interval(
    double first_variance,
    double second_variance,
    double confidence_level,
    double first_df,
    double second_df,
    std::optional<double>& lower,
    std::optional<double>& upper)
{
    const double alpha = 1.0 - confidence_level;
    const double lower_critical = f_quantile(1.0 - alpha / 2.0, first_df, second_df);
    const double upper_critical = f_quantile(alpha / 2.0, first_df, second_df);
    const double ratio = first_variance / second_variance;
    lower = ratio / lower_critical;
    upper = ratio / upper_critical;
}

double group_center(const std::vector<double>& group, VarianceRobustMethod method)
{
    if (method == VarianceRobustMethod::levene_mean) {
        return std::accumulate(group.cbegin(), group.cend(), 0.0)
            / static_cast<double>(group.size());
    }
    std::vector<double> ordered = group;
    std::sort(ordered.begin(), ordered.end());
    const std::size_t middle = ordered.size() / 2;
    return ordered.size() % 2 == 0
        ? (ordered[middle - 1] + ordered[middle]) / 2.0
        : ordered[middle];
}

LeveneTestResult robust_test(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double confidence_level,
    TestAlternative alternative,
    VarianceRobustMethod method)
{
    LeveneTestResult result;
    result.method = method;
    result.group_count = 2;
    result.confidence_level = confidence_level;
    std::vector<double> first_values;
    std::vector<double> second_values;
    const bool first_valid = finite_observations(first, first_values, result.diagnostics);
    const bool second_valid = finite_observations(second, second_values, result.diagnostics);
    if (!valid_confidence(confidence_level)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    if (!first_valid || !second_valid || first.size() < 2 || second.size() < 2) {
        add_error(result.diagnostics, "insufficient_observations",
                  "Levene/Brown-Forsythe 检验要求每组至少有两个有效观测。");
        return result;
    }
    result.total_count = first.size() + second.size();
    add_normality_risk(first_values, result.diagnostics,
                       "第一组正态性风险较高，Levene 类检验结果应结合图形诊断解释。");
    add_normality_risk(second_values, result.diagnostics,
                       "第二组正态性风险较高，Levene 类检验结果应结合图形诊断解释。");

    const double first_center = group_center(first_values, method);
    const double second_center = group_center(second_values, method);
    std::vector<double> first_deviations;
    std::vector<double> second_deviations;
    for (const double value : first_values) {
        first_deviations.push_back(std::abs(value - first_center));
    }
    for (const double value : second_values) {
        second_deviations.push_back(std::abs(value - second_center));
    }
    const double first_mean = std::accumulate(
        first_deviations.cbegin(), first_deviations.cend(), 0.0)
        / static_cast<double>(first_deviations.size());
    const double second_mean = std::accumulate(
        second_deviations.cbegin(), second_deviations.cend(), 0.0)
        / static_cast<double>(second_deviations.size());
    const double grand_mean = (first_mean * first.size() + second_mean * second.size())
        / static_cast<double>(result.total_count);
    const double between = static_cast<double>(first.size())
        * (first_mean - grand_mean) * (first_mean - grand_mean)
        + static_cast<double>(second.size())
        * (second_mean - grand_mean) * (second_mean - grand_mean);
    double within = 0.0;
    for (const double value : first_deviations) {
        within += (value - first_mean) * (value - first_mean);
    }
    for (const double value : second_deviations) {
        within += (value - second_mean) * (value - second_mean);
    }
    result.numerator_degrees_of_freedom = 1.0;
    result.denominator_degrees_of_freedom = static_cast<double>(result.total_count - 2);
    if (within == 0.0) {
        if (between == 0.0) {
            result.f_statistic = 0.0;
            fill_alternative_p_values(1.0, 1.0, result.p_value,
                                      result.p_value_less, result.p_value_greater,
                                      result.p_value_two_sided, alternative);
        } else {
            add_error(result.diagnostics, "zero_within_variance",
                      "绝对偏差的组内平方和为 0，无法计算有限 F 统计量。");
        }
        return result;
    }
    result.f_statistic = between
        / (within / result.denominator_degrees_of_freedom);
    const double right_tail = f_right_tail(
        result.f_statistic, result.numerator_degrees_of_freedom,
        result.denominator_degrees_of_freedom);
    fill_alternative_p_values(
        1.0 - right_tail, right_tail, result.p_value,
        result.p_value_less, result.p_value_greater,
        result.p_value_two_sided, alternative);

    // A ratio interval is a stable supplemental interval; it is not the
    // confidence interval for the transformed Levene location parameters.
    const double first_variance = sample_variance(first_values);
    const double second_variance = sample_variance(second_values);
    if (first_variance > 0.0 && second_variance > 0.0) {
        fill_ratio_confidence_interval(
            first_variance, second_variance, confidence_level,
            static_cast<double>(first.size() - 1),
            static_cast<double>(second.size() - 1),
            result.confidence_lower, result.confidence_upper);
        add_warning(result.diagnostics, "raw_variance_ratio_interval",
                    "该置信区间是原始方差比的 F 区间，仅作为 Levene 类检验的稳定补充。");
    }
    return result;
}

}  // namespace

OneVarianceTestResult chi_square_one_variance_test(
    const std::vector<double>& observations,
    double hypothesized_variance,
    double confidence_level,
    TestAlternative alternative)
{
    OneVarianceTestResult result;
    result.count = observations.size();
    result.hypothesized_variance = hypothesized_variance;
    result.confidence_level = confidence_level;
    std::vector<double> values;
    if (!finite_observations(observations, values, result.diagnostics)
        || !valid_confidence(confidence_level)
        || !(hypothesized_variance > 0.0)) {
        if (!valid_confidence(confidence_level)) {
            add_error(result.diagnostics, "invalid_confidence_level",
                      "置信水平必须大于 0 且小于 1。");
        }
        if (!(hypothesized_variance > 0.0)) {
            add_error(result.diagnostics, "invalid_hypothesized_variance",
                      "待检验的总体方差必须大于 0。");
        }
        return result;
    }
    if (values.size() < 2) {
        add_error(result.diagnostics, "insufficient_observations",
                  "一方差卡方检验至少需要两个有效观测。");
        return result;
    }
    result.sample_variance = sample_variance(values);
    result.sample_standard_deviation = std::sqrt(result.sample_variance);
    result.degrees_of_freedom = static_cast<double>(values.size() - 1);
    add_normality_risk(values, result.diagnostics,
                       "样本正态性风险较高，一方差卡方检验对正态性敏感。");
    if (result.sample_variance == 0.0) {
        result.chi_square_statistic = 0.0;
        fill_alternative_p_values(0.0, 1.0, result.p_value,
                                  result.p_value_less, result.p_value_greater,
                                  result.p_value_two_sided, alternative);
    } else {
        result.chi_square_statistic = result.degrees_of_freedom
            * result.sample_variance / hypothesized_variance;
        const double right_tail = regularized_gamma_q(
            result.degrees_of_freedom / 2.0,
            result.chi_square_statistic / 2.0);
        fill_alternative_p_values(
            1.0 - right_tail, right_tail, result.p_value,
            result.p_value_less, result.p_value_greater,
            result.p_value_two_sided, alternative);
    }
    const double alpha = 1.0 - confidence_level;
    const double lower_quantile = chi_square_quantile(1.0 - alpha / 2.0,
                                                       result.degrees_of_freedom);
    const double upper_quantile = chi_square_quantile(alpha / 2.0,
                                                       result.degrees_of_freedom);
    result.confidence_lower = result.degrees_of_freedom * result.sample_variance
        / lower_quantile;
    result.confidence_upper = result.degrees_of_freedom * result.sample_variance
        / upper_quantile;
    return result;
}

TwoVarianceFTestResult f_test_two_variances(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double confidence_level,
    TestAlternative alternative)
{
    TwoVarianceFTestResult result;
    result.first_count = first.size();
    result.second_count = second.size();
    result.confidence_level = confidence_level;
    std::vector<double> first_values;
    std::vector<double> second_values;
    const bool first_valid = finite_observations(first, first_values, result.diagnostics);
    const bool second_valid = finite_observations(second, second_values, result.diagnostics);
    if (!valid_confidence(confidence_level)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    if (!first_valid || !second_valid || first.size() < 2 || second.size() < 2) {
        add_error(result.diagnostics, "insufficient_observations",
                  "两方差 F 检验要求每组至少有两个有效观测。");
        return result;
    }
    result.first_variance = sample_variance(first_values);
    result.second_variance = sample_variance(second_values);
    add_normality_risk(first_values, result.diagnostics,
                       "第一组正态性风险较高，经典 F 检验对正态性敏感。");
    add_normality_risk(second_values, result.diagnostics,
                       "第二组正态性风险较高，经典 F 检验对正态性敏感。");
    if (!(result.first_variance > 0.0) || !(result.second_variance > 0.0)) {
        add_error(result.diagnostics, "zero_variance",
                  "经典 F 检验要求两组样本方差都大于 0。");
        return result;
    }
    result.variance_ratio = result.first_variance / result.second_variance;
    result.f_statistic = result.variance_ratio;
    result.numerator_degrees_of_freedom = static_cast<double>(first.size() - 1);
    result.denominator_degrees_of_freedom = static_cast<double>(second.size() - 1);
    const double right_tail = f_right_tail(
        result.f_statistic, result.numerator_degrees_of_freedom,
        result.denominator_degrees_of_freedom);
    fill_alternative_p_values(
        f_left_tail(result.f_statistic, result.numerator_degrees_of_freedom,
                    result.denominator_degrees_of_freedom),
        right_tail, result.p_value, result.p_value_less,
        result.p_value_greater, result.p_value_two_sided, alternative);
    fill_ratio_confidence_interval(
        result.first_variance, result.second_variance, confidence_level,
        result.numerator_degrees_of_freedom, result.denominator_degrees_of_freedom,
        result.confidence_lower, result.confidence_upper);
    return result;
}

LeveneTestResult levene_two_variances(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double confidence_level,
    TestAlternative alternative)
{
    return robust_test(first, second, confidence_level, alternative,
                       VarianceRobustMethod::levene_mean);
}

LeveneTestResult brown_forsythe_two_variances(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double confidence_level,
    TestAlternative alternative)
{
    return robust_test(first, second, confidence_level, alternative,
                       VarianceRobustMethod::brown_forsythe_median);
}

}  // namespace datalab::domain::statistics
