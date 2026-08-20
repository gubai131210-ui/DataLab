#include "domain/statistics/variance_tests.h"

#include "domain/statistics/normal_distribution.h"
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
                       VarianceRobustMethod::brown_forsythe_median);
}

LeveneTestResult levene_mean_two_variances(
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

LeveneTestResult levene_k_groups(
    const std::vector<std::vector<double>>& groups,
    double confidence_level,
    VarianceRobustMethod method)
{
    LeveneTestResult result;
    result.method = method;
    result.group_count = groups.size();
    result.confidence_level = confidence_level;
    if (!valid_confidence(confidence_level)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    if (groups.size() < 2) {
        add_error(result.diagnostics, "insufficient_groups",
                  "Levene 检验至少需要两个有效组。");
        return result;
    }
    std::vector<std::vector<double>> deviations;
    deviations.reserve(groups.size());
    std::size_t total = 0;
    for (const auto& group : groups) {
        std::vector<double> values;
        if (!finite_observations(group, values, result.diagnostics) || values.size() < 2) {
            add_error(result.diagnostics, "insufficient_observations",
                      "Levene 检验要求每组至少有两个有效观测。");
            return result;
        }
        const double center = group_center(values, method);
        std::vector<double> group_deviations;
        group_deviations.reserve(values.size());
        for (const double value : values) {
            group_deviations.push_back(std::abs(value - center));
        }
        total += group_deviations.size();
        deviations.push_back(std::move(group_deviations));
    }
    result.total_count = total;
    std::vector<double> group_means;
    double grand = 0.0;
    for (const auto& group : deviations) {
        const double mean = std::accumulate(group.cbegin(), group.cend(), 0.0)
            / static_cast<double>(group.size());
        group_means.push_back(mean);
        grand += mean * static_cast<double>(group.size());
    }
    grand /= static_cast<double>(total);
    double between = 0.0;
    double within = 0.0;
    for (std::size_t index = 0; index < deviations.size(); ++index) {
        between += static_cast<double>(deviations[index].size())
            * (group_means[index] - grand) * (group_means[index] - grand);
        for (const double value : deviations[index]) {
            within += (value - group_means[index]) * (value - group_means[index]);
        }
    }
    result.numerator_degrees_of_freedom = static_cast<double>(groups.size() - 1);
    result.denominator_degrees_of_freedom = static_cast<double>(
        total - groups.size());
    if (within == 0.0) {
        if (between == 0.0) {
            result.f_statistic = 0.0;
            result.p_value = 1.0;
            result.p_value_two_sided = 1.0;
        } else {
            add_error(result.diagnostics, "zero_within_variance",
                      "绝对偏差的组内平方和为 0，无法计算有限 F 统计量。");
        }
        return result;
    }
    result.f_statistic = (between / result.numerator_degrees_of_freedom)
        / (within / result.denominator_degrees_of_freedom);
    result.p_value = f_right_tail(
        result.f_statistic, result.numerator_degrees_of_freedom,
        result.denominator_degrees_of_freedom);
    result.p_value_two_sided = result.p_value;
    return result;
}

double trimmed_mean(const std::vector<double>& values)
{
    if (values.empty()) {
        return 0.0;
    }
    if (values.size() < 5) {
        return std::accumulate(values.cbegin(), values.cend(), 0.0)
            / static_cast<double>(values.size());
    }
    const double trim_proportion =
        1.0 / (2.0 * std::sqrt(static_cast<double>(values.size()) - 4.0));
    auto sorted = values;
    std::sort(sorted.begin(), sorted.end());
    const auto drop = static_cast<std::size_t>(
        std::floor(trim_proportion * static_cast<double>(sorted.size())));
    const std::size_t begin = std::min(drop, sorted.size() / 2);
    const std::size_t end = sorted.size() - begin;
    if (end <= begin) {
        return sorted[sorted.size() / 2];
    }
    double sum = 0.0;
    for (std::size_t index = begin; index < end; ++index) {
        sum += sorted[index];
    }
    return sum / static_cast<double>(end - begin);
}

double fourth_moment_about(const std::vector<double>& values, const double center)
{
    double sum = 0.0;
    for (const double value : values) {
        const double d = value - center;
        sum += d * d * d * d;
    }
    return sum;
}

double pooled_kurtosis_at_rho(
    const std::vector<double>& first,
    const std::vector<double>& second,
    const double s1,
    const double s2,
    const double rho,
    const double m1,
    const double m2)
{
    const double n1 = static_cast<double>(first.size());
    const double n2 = static_cast<double>(second.size());
    const double rho2 = rho * rho;
    const double rho4 = rho2 * rho2;
    const double numerator = (n1 + n2)
        * (fourth_moment_about(first, m1) + rho4 * fourth_moment_about(second, m2));
    const double denom_inner = (n1 - 1.0) * s1 * s1 + rho2 * (n2 - 1.0) * s2 * s2;
    if (!(denom_inner > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return numerator / (denom_inner * denom_inner);
}

double bonett_se_ln_sd_ratio(
    const double gamma_hat,
    const std::size_t n1,
    const std::size_t n2)
{
    // SE of ln(S1/S2) = 0.5 * SE of ln(S1²/S2²)
    const double var_ln_var_ratio =
        (gamma_hat - 1.0) / static_cast<double>(n1 - 1)
        + (gamma_hat - 1.0) / static_cast<double>(n2 - 1);
    if (!(var_ln_var_ratio > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return 0.5 * std::sqrt(var_ln_var_ratio);
}

double equalizer_constant(const std::size_t n1, const std::size_t n2, const double z)
{
    const double a = static_cast<double>(n1) / (static_cast<double>(n1) - z * z);
    const double b = static_cast<double>(n2) / (static_cast<double>(n2) - z * z);
    if (!(a > 0.0) || !(b > 0.0)) {
        return 1.0;
    }
    return std::sqrt(a / b);
}

double bonett_z_at_rho(
    const double ln_ratio,
    const double rho,
    const std::vector<double>& first,
    const std::vector<double>& second,
    const double s1,
    const double s2,
    const double m1,
    const double m2,
    const double equalizer)
{
    if (!(rho > 0.0)) {
        return std::numeric_limits<double>::infinity();
    }
    const double gamma = pooled_kurtosis_at_rho(first, second, s1, s2, rho, m1, m2);
    if (!std::isfinite(gamma) || gamma <= 1.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double se = bonett_se_ln_sd_ratio(gamma, first.size(), second.size());
    if (!std::isfinite(se) || !(se > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (ln_ratio - std::log(rho)) / (equalizer * se);
}

BonettVarianceResult bonett_two_variances(
    const std::vector<double>& first_input,
    const std::vector<double>& second_input,
    const double confidence_level,
    const TestAlternative alternative)
{
    BonettVarianceResult result;
    result.confidence_level = confidence_level;
    if (!valid_confidence(confidence_level)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    std::vector<double> first;
    std::vector<double> second;
    if (!finite_observations(first_input, first, result.diagnostics)
        || !finite_observations(second_input, second, result.diagnostics)) {
        return result;
    }
    if (first.size() < 2 || second.size() < 2) {
        add_error(result.diagnostics, "insufficient_observations",
                  "Bonett 等方差要求每组至少 2 个观测。");
        return result;
    }
    if (first.size() < 5 || second.size() < 5) {
        add_warning(result.diagnostics, "bonett_small_sample_trim",
                    "组样本量小于 5，修整均值退回算术均值；Bonett 小样本表现可能不稳定。");
    }
    result.first_count = first.size();
    result.second_count = second.size();
    const double v1 = sample_variance(first);
    const double v2 = sample_variance(second);
    if (!(v1 > 0.0) || !(v2 > 0.0)) {
        add_error(result.diagnostics, "zero_sample_variance",
                  "样本方差为 0，无法计算 Bonett 比率区间。");
        return result;
    }
    result.first_standard_deviation = std::sqrt(v1);
    result.second_standard_deviation = std::sqrt(v2);
    result.standard_deviation_ratio =
        result.first_standard_deviation / result.second_standard_deviation;
    const double m1 = trimmed_mean(first);
    const double m2 = trimmed_mean(second);
    const double alpha = 1.0 - confidence_level;
    const double z_crit = alternative == TestAlternative::two_sided
        ? standard_normal_quantile(1.0 - alpha * 0.5)
        : standard_normal_quantile(1.0 - alpha);
    if (!(std::isfinite(z_crit)) || z_crit * z_crit >= static_cast<double>(first.size())
        || z_crit * z_crit >= static_cast<double>(second.size())) {
        add_error(result.diagnostics, "bonett_equalizer_undefined",
                  "样本量相对置信水平过小，无法计算 Bonett equalizer。");
        return result;
    }
    const double equalizer = equalizer_constant(first.size(), second.size(), z_crit);
    const double ln_ratio = std::log(result.standard_deviation_ratio);
    const double z0 = bonett_z_at_rho(
        ln_ratio, 1.0, first, second, result.first_standard_deviation,
        result.second_standard_deviation, m1, m2, equalizer);
    if (!std::isfinite(z0)) {
        add_error(result.diagnostics, "bonett_statistic_undefined",
                  "无法计算 Bonett 统计量（峰度估计无效）。");
        return result;
    }
    result.z_statistic = z0;
    const double cdf = standard_normal_cdf(z0);
    if (alternative == TestAlternative::less) {
        result.p_value = cdf;
    } else if (alternative == TestAlternative::greater) {
        result.p_value = 1.0 - cdf;
    } else {
        result.p_value = std::clamp(
            2.0 * (1.0 - standard_normal_cdf(std::abs(z0))), 0.0, 1.0);
    }

    // Invert |z(ρ)| = z_crit for two-sided CI of σ1/σ2.
    auto root_side = [&](const bool lower_side) -> std::optional<double> {
        double lo = lower_side ? 1.0e-8 : result.standard_deviation_ratio;
        double hi = lower_side ? result.standard_deviation_ratio : 1.0e8;
        if (!(lo < hi)) {
            return std::nullopt;
        }
        double z_lo = bonett_z_at_rho(
            ln_ratio, lo, first, second, result.first_standard_deviation,
            result.second_standard_deviation, m1, m2, equalizer);
        double z_hi = bonett_z_at_rho(
            ln_ratio, hi, first, second, result.first_standard_deviation,
            result.second_standard_deviation, m1, m2, equalizer);
        if (!std::isfinite(z_lo) || !std::isfinite(z_hi)) {
            return std::nullopt;
        }
        // z(ρ) decreases in ρ; lower root where z = +z_crit, upper where z = -z_crit
        const double target = lower_side ? z_crit : -z_crit;
        if ((z_lo - target) * (z_hi - target) > 0.0) {
            // Expand bracket
            for (int expand = 0; expand < 40; ++expand) {
                if (lower_side) {
                    lo *= 0.5;
                    z_lo = bonett_z_at_rho(
                        ln_ratio, lo, first, second, result.first_standard_deviation,
                        result.second_standard_deviation, m1, m2, equalizer);
                } else {
                    hi *= 2.0;
                    z_hi = bonett_z_at_rho(
                        ln_ratio, hi, first, second, result.first_standard_deviation,
                        result.second_standard_deviation, m1, m2, equalizer);
                }
                if (!std::isfinite(z_lo) || !std::isfinite(z_hi)) {
                    return std::nullopt;
                }
                if ((z_lo - target) * (z_hi - target) <= 0.0) {
                    break;
                }
            }
        }
        if ((z_lo - target) * (z_hi - target) > 0.0) {
            return std::nullopt;
        }
        for (int iter = 0; iter < 80; ++iter) {
            const double mid = 0.5 * (lo + hi);
            const double z_mid = bonett_z_at_rho(
                ln_ratio, mid, first, second, result.first_standard_deviation,
                result.second_standard_deviation, m1, m2, equalizer);
            if (!std::isfinite(z_mid)) {
                return std::nullopt;
            }
            if ((z_lo - target) * (z_mid - target) <= 0.0) {
                hi = mid;
                z_hi = z_mid;
            } else {
                lo = mid;
                z_lo = z_mid;
            }
        }
        return 0.5 * (lo + hi);
    };

    if (alternative == TestAlternative::two_sided) {
        result.confidence_lower = root_side(true);
        result.confidence_upper = root_side(false);
        if (!result.confidence_lower.has_value() || !result.confidence_upper.has_value()) {
            add_warning(result.diagnostics, "bonett_ci_not_found",
                        "未能数值反解 Bonett 标准差比置信区间。");
        }
    } else if (alternative == TestAlternative::less) {
        result.confidence_upper = root_side(false);
    } else {
        result.confidence_lower = root_side(true);
    }
    return result;
}

BartlettVarianceResult bartlett_k_groups(
    const std::vector<std::vector<double>>& groups,
    double confidence_level)
{
    BartlettVarianceResult result;
    result.confidence_level = confidence_level;
    if (!valid_confidence(confidence_level)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    if (groups.size() < 2) {
        add_error(result.diagnostics, "insufficient_groups",
                  "Bartlett 等方差至少需要 2 组。");
        return result;
    }

    std::vector<std::size_t> counts;
    std::vector<double> variances;
    counts.reserve(groups.size());
    variances.reserve(groups.size());
    for (const auto& group : groups) {
        std::vector<double> cleaned;
        if (!finite_observations(group, cleaned, result.diagnostics)) {
            return result;
        }
        if (cleaned.size() < 2) {
            add_error(result.diagnostics, "insufficient_group_size",
                      "Bartlett 要求每组至少 2 个有效观测。");
            return result;
        }
        const double variance = sample_variance(cleaned);
        if (!(variance > 0.0)) {
            add_error(result.diagnostics, "zero_group_variance",
                      "Bartlett 要求每组样本方差大于 0。");
            return result;
        }
        counts.push_back(cleaned.size());
        variances.push_back(variance);
        result.total_count += cleaned.size();
    }

    result.group_count = groups.size();
    const double k = static_cast<double>(result.group_count);
    double nu = 0.0;
    double pooled = 0.0;
    double sum_inv_nu = 0.0;
    double sum_nu_ln_s2 = 0.0;
    for (std::size_t index = 0; index < variances.size(); ++index) {
        const double nu_i = static_cast<double>(counts[index] - 1);
        nu += nu_i;
        pooled += nu_i * variances[index];
        sum_inv_nu += 1.0 / nu_i;
        sum_nu_ln_s2 += nu_i * std::log(variances[index]);
    }
    pooled /= nu;
    if (!(pooled > 0.0)) {
        add_error(result.diagnostics, "zero_pooled_variance",
                  "合并方差为 0，无法计算 Bartlett 统计量。");
        return result;
    }
    const double q = nu * std::log(pooled) - sum_nu_ln_s2;
    const double c = 1.0 + (1.0 / (3.0 * (k - 1.0))) * (sum_inv_nu - 1.0 / nu);
    if (!(c > 0.0) || !std::isfinite(c) || !std::isfinite(q)) {
        add_error(result.diagnostics, "bartlett_statistic_undefined",
                  "Bartlett 统计量无法计算。");
        return result;
    }
    result.chi_square_statistic = q / c;
    result.degrees_of_freedom = k - 1.0;
    add_warning(result.diagnostics, "bartlett_normality_sensitive",
                "Bartlett 对正态偏离敏感；稳健场景宜对照中位数 Levene。");
    // Right-tail P(χ²_df ≥ statistic) = regularized_gamma_q(df/2, x/2)
    result.p_value = regularized_gamma_q(
        result.degrees_of_freedom / 2.0, result.chi_square_statistic / 2.0);
    return result;
}

}  // namespace datalab::domain::statistics
