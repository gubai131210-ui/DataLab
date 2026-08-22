#pragma once

// Hartigan & Hartigan (1985) dip test of unimodality — formula_reference research screen.
// Not vendor_oracle / golden; Monte Carlo p-values use Uniform(0,1) null only.
// Never authorizes process capability pass/fail.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct HartiganDipResult {
    std::size_t n = 0;
    double dip = 0.0;
    std::optional<double> p_value;  // Uniform-null MC; nullopt if mc_reps==0 or n small
    std::size_t mc_reps = 0;
    std::string status = "not_run";  // not_run | insufficient_n | consistent | evidence_against
    std::string evidence_type = "formula_reference";
    std::string algorithm_id = "hartigan_dip_1985";
};

// Compute Hartigan dip for finite observations. Optional Monte Carlo p-value under
// Uniform(0,1) null (same n). mc_reps==0 skips p-value. seed is deterministic.
HartiganDipResult compute_hartigan_dip(
    const std::vector<double>& observations,
    std::size_t mc_reps = 199,
    std::uint64_t seed = 20260821ull);

}  // namespace datalab::domain::statistics
