#include "domain/statistics/process_capability.h"

#include "domain/statistics/descriptive_statistics.h"
#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>

namespace datalab::domain::statistics {
namespace {

void add_error(ProcessCapabilityResult& result, const char* code, const char* message)
{
    result.diagnostics.push_back(
        {DiagnosticMessage::Severity::error, code, message});
}

std::optional<double> one_sided(double numerator, double sigma)
{
    if (!(sigma > 0.0) || !std::isfinite(sigma) || !std::isfinite(numerator)) {
        return std::nullopt;
    }
    return numerator / (3.0 * sigma);
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

    if (!specifications.lower.has_value() && !specifications.upper.has_value()) {
        add_error(result, "missing_specifications", "LSL or USL is required.");
        return result;
    }
    if (specifications.lower.has_value() && specifications.upper.has_value()) {
        if (!std::isfinite(*specifications.lower) || !std::isfinite(*specifications.upper)
            || *specifications.lower >= *specifications.upper) {
            add_error(result, "invalid_specifications", "LSL must be less than USL.");
            return result;
        }
    }

    const bool has_both = specifications.lower.has_value() && specifications.upper.has_value();
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
        add_error(result, "invalid_within_sigma", "Within sigma must be positive.");
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
        result.expected_ppm_overall_total =
            result.expected_ppm_overall_below.value_or(0.0)
            + result.expected_ppm_overall_above.value_or(0.0);
    } else {
        add_error(result, "invalid_overall_sigma", "Overall sigma must be positive.");
    }

    return result;
}

ProcessCapabilityResult ProcessCapability::calculate(
    const std::vector<double>& observations,
    double within_standard_deviation,
    const SpecificationLimits& specifications)
{
    const auto descriptive = DescriptiveStatistics::calculate(observations);
    if (!descriptive.has_value()) {
        ProcessCapabilityResult result;
        add_error(result, "empty_data", "Capability analysis requires numeric observations.");
        return result;
    }
    ProcessCapabilityResult result = calculate(
        descriptive->mean,
        within_standard_deviation,
        descriptive->sample_standard_deviation.value_or(descriptive->population_standard_deviation),
        specifications);
    result.sample_size = descriptive->count;

    std::size_t below = 0;
    std::size_t above = 0;
    for (const double value : observations) {
        if (specifications.lower.has_value() && value < *specifications.lower) {
            ++below;
        }
        if (specifications.upper.has_value() && value > *specifications.upper) {
            ++above;
        }
    }
    const double n = static_cast<double>(observations.size());
    result.observed_ppm_below = 1.0e6 * static_cast<double>(below) / n;
    result.observed_ppm_above = 1.0e6 * static_cast<double>(above) / n;
    result.observed_ppm_total = *result.observed_ppm_below + *result.observed_ppm_above;
    return result;
}

}  // namespace datalab::domain::statistics
