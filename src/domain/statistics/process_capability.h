#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class CapabilityMethod {
    normal,
    johnson,
    non_normal,
    between_within
};

struct ProcessCapabilityResult {
    double mean = 0.0;
    double within_standard_deviation = 0.0;
    double overall_standard_deviation = 0.0;
    std::optional<double> subgroup_within_standard_deviation;
    std::optional<double> between_standard_deviation;
    std::optional<double> between_within_standard_deviation;
    std::size_t sample_size = 0;
    std::optional<double> cp;
    std::optional<double> cpl;
    std::optional<double> cpu;
    std::optional<double> cpk;
    std::optional<double> cpm;
    std::optional<double> pp;
    std::optional<double> ppl;
    std::optional<double> ppu;
    std::optional<double> ppk;
    std::optional<double> observed_ppm_below;
    std::optional<double> observed_ppm_above;
    std::optional<double> observed_ppm_total;
    std::optional<double> expected_ppm_within_below;
    std::optional<double> expected_ppm_within_above;
    std::optional<double> expected_ppm_within_total;
    std::optional<double> expected_ppm_overall_below;
    std::optional<double> expected_ppm_overall_above;
    std::optional<double> expected_ppm_overall_total;
    std::optional<double> z_lsl;
    std::optional<double> z_usl;
    std::optional<double> z_bench;
    std::string specification_mode;
    std::string within_sigma_method;
    std::string overall_sigma_method = "sample_standard_deviation";
    std::string between_sigma_method;
    std::string between_within_sigma_method;
    std::string capability_method = "normal";
    std::string johnson_family;
    QualityEvidence evidence;
    std::vector<DiagnosticMessage> diagnostics;
};

class ProcessCapability final {
public:
    static ProcessCapabilityResult calculate(
        double mean,
        double within_standard_deviation,
        double overall_standard_deviation,
        const SpecificationLimits& specifications);

    static ProcessCapabilityResult calculate(
        const std::vector<double>& observations,
        double within_standard_deviation,
        const SpecificationLimits& specifications);

    static ProcessCapabilityResult calculate_johnson(
        const std::vector<double>& observations,
        const SpecificationLimits& specifications,
        double p_criterion = 0.10);

    static ProcessCapabilityResult calculate_nonnormal(
        const std::vector<double>& observations,
        const SpecificationLimits& specifications,
        const std::string& distribution);

    static ProcessCapabilityResult calculate_between_within(
        const std::vector<double>& observations,
        const std::vector<std::vector<double>>& subgroups,
        const SpecificationLimits& specifications);
};

}  // namespace datalab::domain::statistics
