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
    std::optional<double> cp_lower;
    std::optional<double> cp_upper;
    std::optional<double> cpl;
    std::optional<double> cpl_lower;
    std::optional<double> cpl_upper;
    std::optional<double> cpu;
    std::optional<double> cpu_lower;
    std::optional<double> cpu_upper;
    std::optional<double> cpk;
    std::optional<double> cpk_lower;
    std::optional<double> cpk_upper;
    std::optional<double> cpm;
    std::optional<double> pp;
    std::optional<double> pp_lower;
    std::optional<double> pp_upper;
    std::optional<double> ppl;
    std::optional<double> ppl_lower;
    std::optional<double> ppl_upper;
    std::optional<double> ppu;
    std::optional<double> ppu_lower;
    std::optional<double> ppu_upper;
    std::optional<double> ppk;
    std::optional<double> ppk_lower;
    std::optional<double> ppk_upper;
    double within_degrees_of_freedom = 0.0;
    double overall_degrees_of_freedom = 0.0;
    std::string capability_ci_method;
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
    std::optional<double> transform_p_value;
    std::optional<double> transform_anderson_darling;
    std::string nonnormal_distribution;
    std::optional<double> fitted_shape;
    std::optional<double> fitted_scale;
    std::vector<double> transformed_values;
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

void fill_capability_index_intervals(
    ProcessCapabilityResult& result, double confidence_level = 0.95);

}  // namespace datalab::domain::statistics
