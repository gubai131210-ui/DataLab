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

struct CapabilityMixtureComponent {
    double weight = 0.0;
    double mean = 0.0;
    double sd = 0.0;
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
    // CAP stability prerequisite screen (I-MR Rule 1); never alone verifies assumptions.
    std::string stability_screen_status = "not_run";  // not_run | clear | signals | insufficient_n
    std::size_t stability_out_of_control_count = 0;
    // CAP bimodality histogram screen; suspected ≠ mixture model fit.
    std::string bimodality_screen_status = "not_run";  // not_run | clear | suspected | insufficient_n
    std::size_t bimodality_peak_count = 0;
    // CAP Hartigan dip screen (formula_reference); never opens pass/fail; not mixture fit.
    std::string hartigan_dip_status = "not_run";  // not_run | insufficient_n | consistent | evidence_against
    double hartigan_dip_statistic = 0.0;
    std::optional<double> hartigan_dip_p_value;
    // CAP Gaussian mixture EM+BIC (k=1..k_max); preferred ≠ process fail; never opens pass/fail.
    std::string mixture_status = "not_run";
    // not_run | insufficient_n | failed | not_preferred | preferred_2comp | preferred_kcomp
    int mixture_k_selected = 1;
    int mixture_k_max = 4;
    double mixture_weight1 = 0.0;
    double mixture_mean1 = 0.0;
    double mixture_mean2 = 0.0;
    double mixture_sd1 = 0.0;
    double mixture_sd2 = 0.0;
    double mixture_delta_bic = 0.0;
    std::string mixture_algorithm_id = "gaussian_mixture_k_bic";
    std::string mixture_evidence_type = "formula_reference";
    std::vector<CapabilityMixtureComponent> mixture_components;
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

// Preliminary I-MR Rule-1 screen only. Clear ≠ verified stability; never sets
// assumption_status to "verified" or invents pass/fail authorization.
void apply_capability_stability_screen(
    ProcessCapabilityResult& result,
    const std::vector<double>& observations);

// Histogram peak/valley screen for suspected bimodality under a single-distribution
// capability fit. Suspected → evidence_against; clear ≠ unimodality proof.
// evidence_type remains formula_reference (not vendor_oracle / golden).
void apply_capability_bimodality_screen(
    ProcessCapabilityResult& result,
    const std::vector<double>& observations);

// Hartigan & Hartigan (1985) dip of unimodality — formula_reference research screen.
// evidence_against → assumption evidence_against; never opens pass/fail; not a mixture model.
void apply_capability_hartigan_dip_screen(
    ProcessCapabilityResult& result,
    const std::vector<double>& observations);

// Gaussian mixture EM + BIC search k=1..k_max (formula_reference / gaussian_mixture_k_bic).
// preferred_2comp | preferred_kcomp → assumption evidence_against; never opens pass/fail; not vendor_oracle.
void apply_capability_mixture_screen(
    ProcessCapabilityResult& result,
    const std::vector<double>& observations);

void fill_capability_index_intervals(
    ProcessCapabilityResult& result, double confidence_level = 0.95);

}  // namespace datalab::domain::statistics
