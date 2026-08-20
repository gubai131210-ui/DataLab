#include "domain/statistics/process_capability.h"

#include "domain/quality_diagnostics.h"
#include "domain/statistics/control_charts.h"
#include "domain/statistics/descriptive_statistics.h"
#include "domain/statistics/johnson_transform.h"
#include "domain/statistics/normal_distribution.h"
#include "domain/statistics/reliability.h"
#include "domain/statistics/spc_constants.h"
#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <string>

namespace datalab::domain::statistics {
namespace {

std::optional<double> one_sided(double numerator, double sigma)
{
    if (!(sigma > 0.0) || !std::isfinite(sigma) || !std::isfinite(numerator)) {
        return std::nullopt;
    }
    return numerator / (3.0 * sigma);
}

bool finite_limit(const std::optional<double>& value)
{
    return value.has_value() && std::isfinite(*value);
}

}  // namespace

ProcessCapabilityResult ProcessCapability::calculate(
    double mean,
    double within_standard_deviation,
    double overall_standard_deviation,
    const SpecificationLimits& specifications)
{
    ProcessCapabilityResult result;
    result.mean = mean;
    result.within_standard_deviation = within_standard_deviation;
    result.overall_standard_deviation = overall_standard_deviation;
    result.evidence.method_version = "2";
    result.evidence.assumption_status = "not_verified";
    result.evidence.parameter_source = "estimated";
    result.within_sigma_method = "user_or_estimated_within";
    add_warning(result.diagnostics, "assumption_not_verified",
                "能力指标未验证过程稳定性和正态性；数值仅供调查，不能单独作为过程合格结论。");

    if (!specifications.lower.has_value() && !specifications.upper.has_value()) {
        add_error(result.diagnostics, "missing_specifications", "LSL or USL is required.");
        result.evidence.not_computed_reason = "missing_specifications";
        return result;
    }
    if (specifications.lower.has_value() && !std::isfinite(*specifications.lower)) {
        add_error(result.diagnostics, "invalid_specification", "LSL 必须为有限数。");
        result.evidence.not_computed_reason = "invalid_specification";
        return result;
    }
    if (specifications.upper.has_value() && !std::isfinite(*specifications.upper)) {
        add_error(result.diagnostics, "invalid_specification", "USL 必须为有限数。");
        result.evidence.not_computed_reason = "invalid_specification";
        return result;
    }
    if (specifications.target.has_value() && !std::isfinite(*specifications.target)) {
        add_error(result.diagnostics, "invalid_target", "Target 必须为有限数，不能静默跳过。");
        result.evidence.not_computed_reason = "invalid_target";
        return result;
    }
    if (finite_limit(specifications.lower) && finite_limit(specifications.upper)
        && *specifications.lower >= *specifications.upper) {
        add_error(result.diagnostics, "invalid_specification", "LSL must be less than USL.");
        result.evidence.not_computed_reason = "invalid_specification";
        return result;
    }
    if (!std::isfinite(mean)) {
        add_error(result.diagnostics, "non_finite_input", "过程均值必须为有限数。");
        result.evidence.not_computed_reason = "non_finite_input";
        return result;
    }

    const bool has_both = finite_limit(specifications.lower) && finite_limit(specifications.upper);
    result.specification_mode = has_both
        ? "bilateral"
        : (specifications.lower.has_value() ? "lower_only" : "upper_only");

    if (within_standard_deviation > 0.0 && std::isfinite(within_standard_deviation)) {
        if (has_both) {
            result.cp = (*specifications.upper - *specifications.lower)
                / (6.0 * within_standard_deviation);
        }
        if (specifications.lower.has_value()) {
            result.cpl = one_sided(mean - *specifications.lower, within_standard_deviation);
        }
        if (specifications.upper.has_value()) {
            result.cpu = one_sided(*specifications.upper - mean, within_standard_deviation);
        }
        if (result.cpl.has_value() && result.cpu.has_value()) {
            result.cpk = std::min(*result.cpl, *result.cpu);
        } else {
            result.cpk = result.cpl.has_value() ? result.cpl : result.cpu;
        }
        if (specifications.lower.has_value()) {
            result.expected_ppm_within_below =
                expected_ppm_below(mean, within_standard_deviation, *specifications.lower);
        }
        if (specifications.upper.has_value()) {
            result.expected_ppm_within_above =
                expected_ppm_above(mean, within_standard_deviation, *specifications.upper);
        }
        result.expected_ppm_within_total =
            result.expected_ppm_within_below.value_or(0.0)
            + result.expected_ppm_within_above.value_or(0.0);
    } else {
        add_error(result.diagnostics, "invalid_within_sigma", "Within sigma must be positive.");
        result.evidence.not_computed_reason = "invalid_within_sigma";
    }

    if (overall_standard_deviation > 0.0 && std::isfinite(overall_standard_deviation)) {
        if (has_both) {
            result.pp = (*specifications.upper - *specifications.lower)
                / (6.0 * overall_standard_deviation);
        }
        if (specifications.lower.has_value()) {
            result.ppl = one_sided(mean - *specifications.lower, overall_standard_deviation);
        }
        if (specifications.upper.has_value()) {
            result.ppu = one_sided(*specifications.upper - mean, overall_standard_deviation);
        }
        if (result.ppl.has_value() && result.ppu.has_value()) {
            result.ppk = std::min(*result.ppl, *result.ppu);
        } else {
            result.ppk = result.ppl.has_value() ? result.ppl : result.ppu;
        }
        if (specifications.lower.has_value()) {
            result.expected_ppm_overall_below =
                expected_ppm_below(mean, overall_standard_deviation, *specifications.lower);
        }
        if (specifications.upper.has_value()) {
            result.expected_ppm_overall_above =
                expected_ppm_above(mean, overall_standard_deviation, *specifications.upper);
        }
        if (specifications.lower.has_value()) {
            result.z_lsl = (mean - *specifications.lower) / overall_standard_deviation;
        }
        if (specifications.upper.has_value()) {
            result.z_usl = (*specifications.upper - mean) / overall_standard_deviation;
        }
        if (result.z_lsl.has_value() && result.z_usl.has_value()) {
            result.z_bench = std::min(*result.z_lsl, *result.z_usl);
        } else {
            result.z_bench = result.z_lsl.has_value() ? result.z_lsl : result.z_usl;
        }
        if (specifications.target.has_value() && has_both) {
            const double centered_variance =
                overall_standard_deviation * overall_standard_deviation
                + (mean - *specifications.target) * (mean - *specifications.target);
            if (centered_variance > 0.0) {
                result.cpm = (*specifications.upper - *specifications.lower)
                    / (6.0 * std::sqrt(centered_variance));
            }
        }
        result.expected_ppm_overall_total =
            result.expected_ppm_overall_below.value_or(0.0)
            + result.expected_ppm_overall_above.value_or(0.0);
    } else {
        add_error(result.diagnostics, "invalid_overall_sigma", "Overall sigma must be positive.");
        if (result.evidence.not_computed_reason.empty()) {
            result.evidence.not_computed_reason = "invalid_overall_sigma";
        }
    }

    return result;
}

ProcessCapabilityResult ProcessCapability::calculate(
    const std::vector<double>& observations,
    double within_standard_deviation,
    const SpecificationLimits& specifications)
{
    std::vector<double> valid;
    valid.reserve(observations.size());
    std::size_t missing = 0;
    for (const double value : observations) {
        if (std::isfinite(value)) {
            valid.push_back(value);
        } else {
            ++missing;
        }
    }
    const auto descriptive = DescriptiveStatistics::calculate(valid, missing, observations.size());
    if (!descriptive.has_value()) {
        ProcessCapabilityResult result;
        result.evidence.method_version = "2";
        result.evidence.missing_count = missing;
        result.evidence.not_computed_reason = "empty_data";
        add_error(result.diagnostics, "empty_data", "Capability analysis requires numeric observations.");
        return result;
    }
    ProcessCapabilityResult result = calculate(
        descriptive->mean,
        within_standard_deviation,
        descriptive->sample_standard_deviation.value_or(descriptive->population_standard_deviation),
        specifications);
    result.sample_size = descriptive->count;
    result.evidence.valid_count = descriptive->count;
    result.evidence.missing_count = missing;
    result.overall_sigma_method = "sample_standard_deviation";

    if (valid.empty()) {
        return result;
    }
    std::size_t below = 0;
    std::size_t above = 0;
    for (const double value : valid) {
        if (specifications.lower.has_value() && std::isfinite(*specifications.lower)
            && value < *specifications.lower) {
            ++below;
        }
        if (specifications.upper.has_value() && std::isfinite(*specifications.upper)
            && value > *specifications.upper) {
            ++above;
        }
    }
    const double n = static_cast<double>(valid.size());
    result.observed_ppm_below = 1.0e6 * static_cast<double>(below) / n;
    result.observed_ppm_above = 1.0e6 * static_cast<double>(above) / n;
    result.observed_ppm_total = *result.observed_ppm_below + *result.observed_ppm_above;
    result.overall_degrees_of_freedom =
        result.sample_size > 1 ? static_cast<double>(result.sample_size - 1) : 0.0;
    result.within_degrees_of_freedom = result.overall_degrees_of_freedom;
    fill_capability_index_intervals(result);
    return result;
}

namespace {

void fill_observed_ppm(
    ProcessCapabilityResult& result,
    const std::vector<double>& valid,
    const SpecificationLimits& specifications)
{
    if (valid.empty()) {
        return;
    }
    std::size_t below = 0;
    std::size_t above = 0;
    for (const double value : valid) {
        if (specifications.lower.has_value() && std::isfinite(*specifications.lower)
            && value < *specifications.lower) {
            ++below;
        }
        if (specifications.upper.has_value() && std::isfinite(*specifications.upper)
            && value > *specifications.upper) {
            ++above;
        }
    }
    const double n = static_cast<double>(valid.size());
    result.observed_ppm_below = 1.0e6 * static_cast<double>(below) / n;
    result.observed_ppm_above = 1.0e6 * static_cast<double>(above) / n;
    result.observed_ppm_total = *result.observed_ppm_below + *result.observed_ppm_above;
}

void clear_within_indices(ProcessCapabilityResult& result)
{
    result.cp.reset();
    result.cp_lower.reset();
    result.cp_upper.reset();
    result.cpl.reset();
    result.cpl_lower.reset();
    result.cpl_upper.reset();
    result.cpu.reset();
    result.cpu_lower.reset();
    result.cpu_upper.reset();
    result.cpk.reset();
    result.cpk_lower.reset();
    result.cpk_upper.reset();
    result.cpm.reset();
    result.expected_ppm_within_below.reset();
    result.expected_ppm_within_above.reset();
    result.expected_ppm_within_total.reset();
    result.within_standard_deviation = 0.0;
    result.within_sigma_method = "not_applicable";
    result.within_degrees_of_freedom = 0.0;
}

}  // namespace

ProcessCapabilityResult ProcessCapability::calculate_johnson(
    const std::vector<double>& observations,
    const SpecificationLimits& specifications,
    double p_criterion)
{
    std::vector<double> valid;
    for (const double value : observations) {
        if (std::isfinite(value)) {
            valid.push_back(value);
        }
    }
    JohnsonTransformResult transform = fit_johnson_transform(valid, p_criterion);
    ProcessCapabilityResult result;
    result.capability_method = "johnson";
    result.johnson_family = johnson_family_name(transform.parameters.family);
    result.transform_p_value = transform.found ? std::optional<double>(transform.p_value)
                                             : std::nullopt;
    result.transform_anderson_darling = transform.found
        ? std::optional<double>(transform.anderson_darling) : std::nullopt;
    result.diagnostics = transform.diagnostics;
    result.evidence.method_version = "2";
    result.evidence.assumption_status = "not_verified";
    result.evidence.valid_count = valid.size();
    result.sample_size = valid.size();
    if (!transform.found) {
        result.evidence.not_computed_reason = "johnson_transform_not_found";
        fill_observed_ppm(result, valid, specifications);
        return result;
    }

    SpecificationLimits transformed_specs;
    bool spec_outside = false;
    if (specifications.lower.has_value()) {
        const auto z = johnson_transform_value(transform.parameters, *specifications.lower);
        if (z.has_value()) {
            transformed_specs.lower = z;
        } else {
            spec_outside = true;
        }
    }
    if (specifications.upper.has_value()) {
        const auto z = johnson_transform_value(transform.parameters, *specifications.upper);
        if (z.has_value()) {
            transformed_specs.upper = z;
        } else {
            spec_outside = true;
        }
    }
    if (specifications.target.has_value()) {
        transformed_specs.target =
            johnson_transform_value(transform.parameters, *specifications.target);
    }
    if (!transformed_specs.lower.has_value() && !transformed_specs.upper.has_value()) {
        add_error(result.diagnostics, "johnson_spec_outside_support",
                  "规格限落在 Johnson 变换定义域外，无法计算 Pp/Ppk。");
        result.evidence.not_computed_reason = "johnson_spec_outside_support";
        fill_observed_ppm(result, valid, specifications);
        return result;
    }
    if (spec_outside) {
        add_warning(result.diagnostics, "johnson_spec_outside_support",
                    "至少一个规格限落在变换定义域外；越界侧的百分位回推未实现，"
                    "仅输出仍可变换规格的 overall 指数。");
    }

    const auto descriptive = DescriptiveStatistics::calculate(transform.transformed);
    if (!descriptive.has_value()
        || !descriptive->sample_standard_deviation.has_value()) {
        add_error(result.diagnostics, "johnson_transform_not_found",
                  "变换后样本标准差不可用。");
        result.evidence.not_computed_reason = "empty_data";
        return result;
    }
    result = calculate(
        descriptive->mean,
        *descriptive->sample_standard_deviation,
        *descriptive->sample_standard_deviation,
        transformed_specs);
    result.capability_method = "johnson";
    result.johnson_family = johnson_family_name(transform.parameters.family);
    result.transform_p_value = std::optional<double>(transform.p_value);
    result.transform_anderson_darling = std::optional<double>(transform.anderson_darling);
    result.diagnostics.insert(result.diagnostics.end(),
                              transform.diagnostics.cbegin(),
                              transform.diagnostics.cend());
    result.sample_size = valid.size();
    result.evidence.valid_count = valid.size();
    result.transformed_values = transform.transformed;
    clear_within_indices(result);
    add_warning(result.diagnostics, "within_not_applicable_after_johnson",
                "Johnson 变换路径只报告 overall Pp/Ppk，不报告 within Cp/Cpk。");
    fill_observed_ppm(result, valid, specifications);
    result.overall_sigma_method = "johnson_transformed_sample_sd";
    result.overall_degrees_of_freedom =
        result.sample_size > 1 ? static_cast<double>(result.sample_size - 1) : 0.0;
    fill_capability_index_intervals(result);
    return result;
}

ProcessCapabilityResult ProcessCapability::calculate_nonnormal(
    const std::vector<double>& observations,
    const SpecificationLimits& specifications,
    const std::string& distribution)
{
    ProcessCapabilityResult result;
    result.capability_method = "non_normal";
    result.nonnormal_distribution = distribution;
    result.evidence.method_version = "2";
    result.evidence.assumption_status = "not_verified";
    result.within_sigma_method = "not_applicable";
    result.overall_sigma_method = distribution + "_z_score";
    std::vector<double> valid;
    for (const double value : observations) {
        if (std::isfinite(value)) {
            valid.push_back(value);
        }
    }
    result.sample_size = valid.size();
    result.evidence.valid_count = valid.size();
    if (valid.size() < 2) {
        add_error(result.diagnostics, "empty_data",
                  "非正态能力至少需要两个有限观测。");
        result.evidence.not_computed_reason = "empty_data";
        return result;
    }
    if (!specifications.lower.has_value() && !specifications.upper.has_value()) {
        add_error(result.diagnostics, "missing_specifications", "LSL or USL is required.");
        result.evidence.not_computed_reason = "missing_specifications";
        return result;
    }
    const bool has_both = specifications.lower.has_value()
        && specifications.upper.has_value();
    result.specification_mode = has_both
        ? "bilateral"
        : (specifications.lower.has_value() ? "lower_only" : "upper_only");

    std::vector<double> positive;
    for (const double value : valid) {
        if (value > 0.0) {
            positive.push_back(value);
        }
    }
    if (positive.size() < 2) {
        add_error(result.diagnostics, "non_positive_observation",
                  "Weibull/Lognormal 非正态能力要求观测值为正数。");
        result.evidence.not_computed_reason = "non_positive_observation";
        return result;
    }
    const std::vector<bool> events(positive.size(), true);
    const bool use_lognormal = distribution == "lognormal";
    double weibull_shape = 0.0;
    double weibull_scale = 0.0;
    double lognormal_location = 0.0;
    double lognormal_scale = 0.0;
    if (use_lognormal) {
        const LognormalResult fitted = fit_lognormal(positive, events);
        result.diagnostics.insert(result.diagnostics.end(),
                                  fitted.diagnostics.cbegin(),
                                  fitted.diagnostics.cend());
        if (!fitted.identifiable || !fitted.converged) {
            result.evidence.not_computed_reason = "distribution_not_identifiable";
            fill_observed_ppm(result, valid, specifications);
            return result;
        }
        lognormal_location = fitted.location;
        lognormal_scale = fitted.scale;
        result.fitted_shape = lognormal_scale;
        result.fitted_scale = lognormal_location;
        result.mean = std::exp(fitted.location + 0.5 * fitted.scale * fitted.scale);
        result.overall_standard_deviation =
            result.mean * std::sqrt(std::exp(fitted.scale * fitted.scale) - 1.0);
    } else {
        const WeibullResult fitted = fit_weibull(positive, events);
        result.diagnostics.insert(result.diagnostics.end(),
                                  fitted.diagnostics.cbegin(),
                                  fitted.diagnostics.cend());
        if (!fitted.identifiable || !fitted.converged) {
            result.evidence.not_computed_reason = "distribution_not_identifiable";
            fill_observed_ppm(result, valid, specifications);
            return result;
        }
        weibull_shape = fitted.shape;
        weibull_scale = fitted.scale;
        result.fitted_shape = weibull_shape;
        result.fitted_scale = weibull_scale;
        result.mean = fitted.scale * std::tgamma(1.0 + 1.0 / fitted.shape);
        result.overall_standard_deviation = 0.0;
    }

    const auto cdf = [&](double x) -> double {
        if (!(x > 0.0)) {
            return 0.0;
        }
        if (use_lognormal) {
            return standard_normal_cdf((std::log(x) - lognormal_location) / lognormal_scale);
        }
        return 1.0 - std::exp(-std::pow(x / weibull_scale, weibull_shape));
    };
    const auto clamped_quantile = [](double probability) {
        const double clamped = std::clamp(probability, 1.0e-12, 1.0 - 1.0e-12);
        return standard_normal_quantile(clamped);
    };

    if (specifications.lower.has_value() && std::isfinite(*specifications.lower)) {
        const double probability = cdf(*specifications.lower);
        result.z_lsl = clamped_quantile(probability);
        result.ppl = -(*result.z_lsl) / 3.0;
        result.expected_ppm_overall_below = 1.0e6 * probability;
    }
    if (specifications.upper.has_value() && std::isfinite(*specifications.upper)) {
        const double probability = cdf(*specifications.upper);
        result.z_usl = clamped_quantile(probability);
        result.ppu = *result.z_usl / 3.0;
        result.expected_ppm_overall_above = 1.0e6 * (1.0 - probability);
    }
    if (result.ppl.has_value() && result.ppu.has_value()) {
        result.ppk = std::min(*result.ppl, *result.ppu);
        result.pp = (*result.z_usl - *result.z_lsl) / 6.0;
    } else {
        result.ppk = result.ppl.has_value() ? result.ppl : result.ppu;
    }
    if (result.z_lsl.has_value() && result.z_usl.has_value()) {
        result.z_bench = std::min(*result.z_lsl, *result.z_usl);
    } else {
        result.z_bench = result.z_lsl.has_value() ? result.z_lsl : result.z_usl;
    }
    result.expected_ppm_overall_total =
        result.expected_ppm_overall_below.value_or(0.0)
        + result.expected_ppm_overall_above.value_or(0.0);
    fill_observed_ppm(result, valid, specifications);
    add_warning(result.diagnostics, "nonnormal_z_score_formula_reference",
                "非正态能力使用拟合分布 CDF 的 Z-score 法计算 Pp/Ppk；"
                "不报告 Cp/Cpk。数值是公式参考，不是 Minitab 导出。");
    add_warning(result.diagnostics, "assumption_not_verified",
                "能力指标未验证过程稳定性；数值仅供调查，不能单独作为过程合格结论。");
    return result;
}

ProcessCapabilityResult ProcessCapability::calculate_between_within(
    const std::vector<double>& observations,
    const std::vector<std::vector<double>>& subgroups,
    const SpecificationLimits& specifications)
{
    ProcessCapabilityResult failure;
    failure.evidence.method_version = "2";
    if (subgroups.size() < 2) {
        add_error(failure.diagnostics, "insufficient_subgroups",
                  "组间/组内能力至少需要两个子组。");
        failure.evidence.not_computed_reason = "insufficient_subgroups";
        return failure;
    }
    const std::size_t subgroup_size = subgroups.front().size();
    if (subgroup_size < 2) {
        add_error(failure.diagnostics, "invalid_subgroup_size",
                  "各子组必须至少包含两个观测。");
        failure.evidence.not_computed_reason = "invalid_subgroup_size";
        return failure;
    }
    for (const auto& subgroup : subgroups) {
        if (subgroup.size() != subgroup_size) {
            add_error(failure.diagnostics, "unequal_subgroups",
                      "各子组必须具有相同观测数。");
            failure.evidence.not_computed_reason = "unequal_subgroups";
            return failure;
        }
    }

    const std::optional<double> d2_pair = SpcConstants::d2(2);
    const WithinSubgroupSigmaEstimate within =
        estimate_within_subgroup_sigma(subgroups);
    if (!within.ok) {
        add_error(failure.diagnostics, within.error_code.c_str(),
                  within.error_message.c_str());
        failure.evidence.not_computed_reason = within.error_code;
        return failure;
    }
    if (!d2_pair.has_value()) {
        add_error(failure.diagnostics, "unsupported_subgroup_size",
                  "子组大小超出无偏常数表范围。");
        failure.evidence.not_computed_reason = "unsupported_subgroup_size";
        return failure;
    }

    std::vector<double> subgroup_means;
    subgroup_means.reserve(subgroups.size());
    for (const auto& subgroup : subgroups) {
        subgroup_means.push_back(
            std::accumulate(subgroup.begin(), subgroup.end(), 0.0)
            / static_cast<double>(subgroup.size()));
    }
    const double within_sigma = within.sigma;

    double moving_range_sum = 0.0;
    for (std::size_t index = 1; index < subgroup_means.size(); ++index) {
        moving_range_sum += std::abs(subgroup_means[index] - subgroup_means[index - 1]);
    }
    const double sigma_xbar = (moving_range_sum
        / static_cast<double>(subgroup_means.size() - 1)) / *d2_pair;
    const double within_variance = within_sigma * within_sigma;
    const double raw_between_variance =
        sigma_xbar * sigma_xbar - within_variance / static_cast<double>(subgroup_size);
    const bool truncated = raw_between_variance < 0.0;
    const double between_variance = truncated ? 0.0 : raw_between_variance;
    const double between_sigma = std::sqrt(between_variance);
    const double between_within_sigma = std::sqrt(between_variance + within_variance);

    std::vector<double> valid;
    valid.reserve(observations.size());
    std::size_t missing = 0;
    for (const double value : observations) {
        if (std::isfinite(value)) {
            valid.push_back(value);
        } else {
            ++missing;
        }
    }
    const auto descriptive = DescriptiveStatistics::calculate(valid, missing, observations.size());
    if (!descriptive.has_value() || !descriptive->sample_standard_deviation.has_value()) {
        add_error(failure.diagnostics, "empty_data", "组间/组内能力需要有效数值观测。");
        failure.evidence.not_computed_reason = "empty_data";
        return failure;
    }

    ProcessCapabilityResult result = calculate(
        descriptive->mean,
        between_within_sigma,
        *descriptive->sample_standard_deviation,
        specifications);
    result.sample_size = descriptive->count;
    result.evidence.valid_count = descriptive->count;
    result.evidence.missing_count = missing;
    result.capability_method = "between_within";
    result.subgroup_within_standard_deviation = within_sigma;
    result.between_standard_deviation = between_sigma;
    result.between_within_standard_deviation = between_within_sigma;
    result.within_sigma_method = within.method;
    result.between_sigma_method = "MR̄(子组均值) / d2(2)";
    result.between_within_sigma_method = "sqrt(σ²_B + σ²_within)";
    result.overall_sigma_method = "sample_standard_deviation";
    if (truncated) {
        add_warning(result.diagnostics, "between_variance_truncated",
                    "估计的组间方差为负，已截断为 0；σ_B 可能低估。");
    }
    add_warning(result.diagnostics, "assumption_not_verified",
                "能力指标未验证过程稳定性和正态性；数值仅供调查，不能单独作为过程合格结论。");
    fill_observed_ppm(result, valid, specifications);
    result.overall_degrees_of_freedom =
        result.sample_size > 1 ? static_cast<double>(result.sample_size - 1) : 0.0;
    result.within_degrees_of_freedom = result.overall_degrees_of_freedom;
    if (result.within_sigma_method.find("R") != std::string::npos) {
        add_warning(result.diagnostics, "ci_df_used_sample_n",
                    "组间/组内能力区间自由度使用 N−1，不是 Minitab Rbar/Sbar 调整 ν。");
    }
    fill_capability_index_intervals(result);
    return result;
}

namespace {

double chi_square_left_tail_local(double value, double degrees_of_freedom)
{
    if (!(degrees_of_freedom > 0.0) || value < 0.0 || !std::isfinite(value)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double shape = degrees_of_freedom / 2.0;
    const double scaled = value / 2.0;
    if (scaled == 0.0) {
        return 0.0;
    }
    constexpr int kMaxIterations = 200;
    constexpr double kEpsilon = 1.0e-14;
    if (scaled < shape + 1.0) {
        double term = 1.0 / shape;
        double sum = term;
        for (int index = 1; index <= kMaxIterations; ++index) {
            term *= scaled / (shape + static_cast<double>(index));
            sum += term;
            if (std::abs(term) < std::abs(sum) * kEpsilon) {
                break;
            }
        }
        return std::clamp(
            sum * std::exp(-scaled + shape * std::log(scaled) - std::lgamma(shape)),
            0.0, 1.0);
    }
    double factor = 1.0;
    double sum = 1.0;
    for (int index = 1; index <= kMaxIterations; ++index) {
        factor *= (shape - static_cast<double>(index)) / scaled;
        sum += factor;
        if (std::abs(factor) < std::abs(sum) * kEpsilon) {
            break;
        }
    }
    return std::clamp(
        1.0 - std::exp(-scaled + shape * std::log(scaled) - std::lgamma(shape)) * sum,
        0.0, 1.0);
}

double chi_square_quantile_local(double probability, double degrees_of_freedom)
{
    if (!(probability > 0.0 && probability < 1.0) || !(degrees_of_freedom > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double lower = 0.0;
    double upper = std::max(1.0, degrees_of_freedom);
    while (chi_square_left_tail_local(upper, degrees_of_freedom) < probability) {
        upper *= 2.0;
        if (!std::isfinite(upper)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
    }
    for (int index = 0; index < 160; ++index) {
        const double middle = (lower + upper) / 2.0;
        if (chi_square_left_tail_local(middle, degrees_of_freedom) < probability) {
            lower = middle;
        } else {
            upper = middle;
        }
    }
    return (lower + upper) / 2.0;
}

void assign_chi_square_interval(
    const std::optional<double>& estimate,
    double nu,
    double alpha,
    std::optional<double>& lower,
    std::optional<double>& upper)
{
    lower.reset();
    upper.reset();
    if (!estimate.has_value() || !std::isfinite(*estimate) || !(nu > 0.0)
        || !(alpha > 0.0 && alpha < 1.0)) {
        return;
    }
    const double chi_low = chi_square_quantile_local(alpha / 2.0, nu);
    const double chi_high = chi_square_quantile_local(1.0 - alpha / 2.0, nu);
    if (!(chi_low > 0.0) || !(chi_high > 0.0)) {
        return;
    }
    lower = *estimate * std::sqrt(chi_low / nu);
    upper = *estimate * std::sqrt(chi_high / nu);
}

void assign_bissell_interval(
    const std::optional<double>& estimate,
    double n,
    double nu,
    double z,
    std::optional<double>& lower,
    std::optional<double>& upper)
{
    lower.reset();
    upper.reset();
    if (!estimate.has_value() || !std::isfinite(*estimate) || !(n > 0.0) || !(nu > 0.0)
        || !std::isfinite(z)) {
        return;
    }
    const double variance = 1.0 / (9.0 * n) + (*estimate) * (*estimate) / (2.0 * nu);
    if (!(variance > 0.0)) {
        return;
    }
    const double half = z * std::sqrt(variance);
    lower = *estimate - half;
    upper = *estimate + half;
}

}  // namespace

void fill_capability_index_intervals(
    ProcessCapabilityResult& result, double confidence_level)
{
    result.capability_ci_method.clear();
    if (!(confidence_level > 0.0 && confidence_level < 1.0) || result.sample_size < 2) {
        add_warning(result.diagnostics, "capability_ci_not_computed",
                    "样本量不足或置信水平非法，未计算能力指数区间。");
        return;
    }
    const double alpha = 1.0 - confidence_level;
    const double n = static_cast<double>(result.sample_size);
    if (!(result.overall_degrees_of_freedom > 0.0)) {
        result.overall_degrees_of_freedom = n - 1.0;
    }
    if (!(result.within_degrees_of_freedom > 0.0)
        && result.capability_method != "johnson"
        && result.capability_method != "non_normal") {
        result.within_degrees_of_freedom = n - 1.0;
    }
    const double z = standard_normal_quantile(1.0 - alpha / 2.0);
    assign_chi_square_interval(
        result.cp, result.within_degrees_of_freedom, alpha,
        result.cp_lower, result.cp_upper);
    assign_bissell_interval(
        result.cpl, n, result.within_degrees_of_freedom, z,
        result.cpl_lower, result.cpl_upper);
    assign_bissell_interval(
        result.cpu, n, result.within_degrees_of_freedom, z,
        result.cpu_lower, result.cpu_upper);
    assign_bissell_interval(
        result.cpk, n, result.within_degrees_of_freedom, z,
        result.cpk_lower, result.cpk_upper);
    assign_chi_square_interval(
        result.pp, result.overall_degrees_of_freedom, alpha,
        result.pp_lower, result.pp_upper);
    assign_bissell_interval(
        result.ppl, n, result.overall_degrees_of_freedom, z,
        result.ppl_lower, result.ppl_upper);
    assign_bissell_interval(
        result.ppu, n, result.overall_degrees_of_freedom, z,
        result.ppu_lower, result.ppu_upper);
    assign_bissell_interval(
        result.ppk, n, result.overall_degrees_of_freedom, z,
        result.ppk_lower, result.ppk_upper);
    result.capability_ci_method = "chi_square_cp_pp_bissell_cpk_ppk";
}

}  // namespace datalab::domain::statistics
