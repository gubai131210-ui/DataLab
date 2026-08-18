#include "domain/statistics/process_capability.h"

#include "domain/quality_diagnostics.h"
#include "domain/statistics/descriptive_statistics.h"
#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <limits>

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
    return result;
}

}  // namespace datalab::domain::statistics
