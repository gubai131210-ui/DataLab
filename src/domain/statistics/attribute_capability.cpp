#include "domain/statistics/attribute_capability.h"

#include "domain/quality_diagnostics.h"
#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace datalab::domain::statistics {
namespace {

constexpr double kEpsilon = 1.0e-12;

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

bool to_count(double value, std::size_t& count)
{
    if (!std::isfinite(value) || value < 0.0) {
        return false;
    }
    const double rounded = std::round(value);
    if (std::abs(value - rounded) > 1.0e-9) {
        return false;
    }
    count = static_cast<std::size_t>(rounded);
    return true;
}

std::optional<double> resolve_binomial_target(
    std::optional<double> target,
    std::vector<DiagnosticMessage>& diagnostics)
{
    if (!target.has_value()) {
        return std::nullopt;
    }
    if (!std::isfinite(*target) || *target <= 0.0) {
        add_warning(diagnostics, "invalid_target",
                    "目标不合格品率必须为正数；已忽略该目标。");
        return std::nullopt;
    }
    if (*target <= 1.0) {
        return target;
    }
    if (*target <= 100.0) {
        add_warning(diagnostics, "target_interpreted_as_percent",
                    "目标值大于 1，已按百分数除以 100 解释为不合格品率。");
        return *target / 100.0;
    }
    add_warning(diagnostics, "invalid_target",
                "目标不合格品率超出 (0, 100]，已忽略该目标。");
    return std::nullopt;
}

std::optional<double> resolve_poisson_target(
    std::optional<double> target,
    std::vector<DiagnosticMessage>& diagnostics)
{
    if (!target.has_value()) {
        return std::nullopt;
    }
    if (!std::isfinite(*target) || *target < 0.0) {
        add_warning(diagnostics, "invalid_target",
                    "目标 DPU 必须为非负数；已忽略该目标。");
        return std::nullopt;
    }
    return target;
}

bool collect_counts(
    const std::vector<AttributeSample>& samples,
    bool binomial,
    AttributeCapabilityResult& result)
{
    for (const AttributeSample& sample : samples) {
        std::size_t defectives = 0;
        std::size_t inspected = 0;
        if (!to_count(sample.defectives, defectives)
            || !to_count(sample.inspected, inspected)
            || inspected == 0) {
            add_error(result.diagnostics, "invalid_attribute_count",
                      binomial
                          ? "不合格品数和检验数必须是正检验数上的非负整数。"
                          : "缺陷数和单位数必须是正单位数上的非负整数。");
            continue;
        }
        if (binomial && defectives > inspected) {
            add_error(result.diagnostics, "defectives_exceed_inspected",
                      "不合格品数不能大于检验数。");
            continue;
        }
        result.defectives.push_back(defectives);
        result.inspected.push_back(inspected);
        result.source_rows.push_back(sample.source_row);
        result.defectives_total += static_cast<double>(defectives);
        result.inspected_total += static_cast<double>(inspected);
    }
    result.sample_count = result.defectives.size();
    return result.sample_count > 0;
}

void fill_cumulative(
    AttributeCapabilityResult& result,
    bool percent)
{
    double defectives = 0.0;
    double inspected = 0.0;
    result.cumulative_values.clear();
    result.cumulative_values.reserve(result.defectives.size());
    for (std::size_t index = 0; index < result.defectives.size(); ++index) {
        defectives += static_cast<double>(result.defectives[index]);
        inspected += static_cast<double>(result.inspected[index]);
        const double rate = inspected > 0.0 ? defectives / inspected : 0.0;
        result.cumulative_values.push_back(percent ? 100.0 * rate : rate);
    }
}

std::optional<double> process_z_from_p(double proportion)
{
    if (!(proportion > 0.0 && proportion < 1.0)) {
        return std::nullopt;
    }
    const double z = standard_normal_quantile(1.0 - proportion);
    if (!std::isfinite(z)) {
        return std::nullopt;
    }
    return z;
}

}  // namespace

ConfidenceInterval garwood_rate(
    double defectives_total,
    double exposure,
    double alpha)
{
    ConfidenceInterval interval;
    if (!(exposure > 0.0) || defectives_total < 0.0) {
        return interval;
    }
    if (defectives_total <= 0.0) {
        interval.lower = 0.0;
    } else {
        const double lower = chi_square_quantile(
            alpha / 2.0, 2.0 * defectives_total);
        if (std::isfinite(lower)) {
            interval.lower = lower / (2.0 * exposure);
        }
    }
    const double upper = chi_square_quantile(
        1.0 - alpha / 2.0, 2.0 * (defectives_total + 1.0));
    if (std::isfinite(upper)) {
        interval.upper = upper / (2.0 * exposure);
    }
    return interval;
}

ConfidenceInterval clopper_pearson_interval(
    double defectives,
    double inspected,
    double alpha)
{
    ConfidenceInterval interval;
    if (!(inspected > 0.0) || defectives < 0.0 || defectives > inspected) {
        return interval;
    }
    const double upper_probability = 1.0 - alpha / 2.0;
    if (defectives <= 0.0) {
        interval.lower = 0.0;
    } else {
        const double v1 = 2.0 * defectives;
        const double v2 = 2.0 * (inspected - defectives + 1.0);
        const double critical = f_quantile(upper_probability, v2, v1);
        if (std::isfinite(critical) && critical > 0.0) {
            interval.lower = 1.0 / (1.0 + (inspected - defectives + 1.0)
                / defectives * critical);
        }
    }
    if (defectives >= inspected) {
        interval.upper = 1.0;
    } else {
        const double v3 = 2.0 * (defectives + 1.0);
        const double v4 = 2.0 * (inspected - defectives);
        const double critical = f_quantile(upper_probability, v3, v4);
        if (std::isfinite(critical) && critical > 0.0) {
            interval.upper = 1.0 / (1.0 + (inspected - defectives)
                / ((defectives + 1.0) * critical));
        }
    }
    return interval;
}

AttributeCapabilityResult binomial_capability(
    const std::vector<AttributeSample>& samples,
    const std::size_t missing_count,
    const std::optional<double> target,
    const double confidence_level)
{
    AttributeCapabilityResult result;
    result.method = "binomial";
    result.missing_count = missing_count;
    result.assumption_status = "not_verified";
    const double alpha = (confidence_level > 0.0 && confidence_level < 1.0)
        ? 1.0 - confidence_level : 0.05;
    result.target = resolve_binomial_target(target, result.diagnostics);
    if (!collect_counts(samples, true, result)) {
        add_error(result.diagnostics, "insufficient_data",
                  "二项过程能力至少需要一个有效子组。");
        return result;
    }
    result.average_p = result.defectives_total / result.inspected_total;
    result.percent_defective = 100.0 * *result.average_p;
    result.ppm_defective = 1.0e6 * *result.average_p;
    result.process_z = process_z_from_p(*result.average_p);
    if (!result.process_z.has_value()) {
        add_warning(result.diagnostics, "process_z_not_computed",
                    "Average P 为 0 或 1 时 Process Z 未定义。");
    }
    result.average_p_interval = clopper_pearson_interval(
        result.defectives_total, result.inspected_total, alpha);
    if (result.average_p_interval.lower.has_value()) {
        result.percent_defective_interval.lower = 100.0 * *result.average_p_interval.lower;
        result.ppm_interval.lower = 1.0e6 * *result.average_p_interval.lower;
    }
    if (result.average_p_interval.upper.has_value()) {
        result.percent_defective_interval.upper = 100.0 * *result.average_p_interval.upper;
        result.ppm_interval.upper = 1.0e6 * *result.average_p_interval.upper;
    }
    if (result.average_p_interval.upper.has_value()) {
        result.process_z_interval.lower = process_z_from_p(*result.average_p_interval.upper);
    }
    if (result.average_p_interval.lower.has_value()) {
        result.process_z_interval.upper = process_z_from_p(*result.average_p_interval.lower);
    }
    fill_cumulative(result, true);
    add_warning(result.diagnostics, "binomial_assumption_not_verified",
                "二项过程能力未验证独立性、恒定 p 与稳定性，不能写成过程合格。");
    return result;
}

AttributeCapabilityResult poisson_capability(
    const std::vector<AttributeSample>& samples,
    const std::size_t missing_count,
    const std::optional<double> target,
    const double confidence_level)
{
    AttributeCapabilityResult result;
    result.method = "poisson";
    result.missing_count = missing_count;
    result.assumption_status = "not_verified";
    const double alpha = (confidence_level > 0.0 && confidence_level < 1.0)
        ? 1.0 - confidence_level : 0.05;
    result.target = resolve_poisson_target(target, result.diagnostics);
    if (!collect_counts(samples, false, result)) {
        add_error(result.diagnostics, "insufficient_data",
                  "泊松过程能力至少需要一个有效子组。");
        return result;
    }
    result.mean_dpu = result.defectives_total / result.inspected_total;
    result.mean_defective = result.defectives_total
        / static_cast<double>(result.sample_count);
    double minimum = std::numeric_limits<double>::infinity();
    double maximum = -std::numeric_limits<double>::infinity();
    for (std::size_t index = 0; index < result.defectives.size(); ++index) {
        const double dpu = static_cast<double>(result.defectives[index])
            / static_cast<double>(result.inspected[index]);
        minimum = std::min(minimum, dpu);
        maximum = std::max(maximum, dpu);
    }
    result.minimum_dpu = minimum;
    result.maximum_dpu = maximum;
    result.mean_dpu_interval = garwood_rate(
        result.defectives_total, result.inspected_total, alpha);
    result.mean_defective_interval = garwood_rate(
        result.defectives_total, static_cast<double>(result.sample_count), alpha);
    fill_cumulative(result, false);
    add_warning(result.diagnostics, "poisson_assumption_not_verified",
                "泊松过程能力未验证独立性、恒定 DPU 与稳定性，不能写成过程合格。");
    return result;
}

}  // namespace datalab::domain::statistics
