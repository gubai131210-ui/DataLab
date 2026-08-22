#pragma once

// Fine–Gray subdistribution hazard regression (IPCW / Geskus-style weighted partial likelihood).
// formula_reference:
//   fine_gray_binary_ipcw | fine_gray_continuous_ipcw | fine_gray_multi_ipcw
// Not cause-specific Cox, not vendor_oracle, not pinned R survival::finegray.

#include "domain/quality_types.h"
#include "domain/statistics/censoring_contract.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct FineGrayTerm {
    std::string name;
    std::optional<double> mean;  // centering mean (continuous/multi)
    std::optional<double> beta;
    std::optional<double> se_beta;
    std::optional<double> hazard_ratio;  // exp(beta); binary: level1 vs 0; continuous/multi: per +1
    std::optional<double> p_value;
};

struct FineGrayResult {
    bool ran = false;
    bool converged = false;
    std::string kind;  // binary | continuous | multi
    std::string target_failure_mode;
    std::string covariate_name;  // single-covariate display; multi uses terms[].name
    std::string group_level_0;  // binary only
    std::string group_level_1;  // binary only
    std::size_t n = 0;
    std::size_t target_failures = 0;
    std::size_t competing_failures = 0;
    std::size_t right_censored = 0;
    std::optional<double> covariate_mean;  // single continuous
    std::optional<double> beta;
    std::optional<double> se_beta;
    std::optional<double> hazard_ratio;
    std::optional<double> p_value;
    std::vector<FineGrayTerm> terms;  // always filled when coefficients exist (p≥1)
    int iterations = 0;
    std::string not_computed_reason;
    std::string evidence_type = "formula_reference";
    std::string algorithm_id = "fine_gray_continuous_ipcw";
    std::vector<DiagnosticMessage> diagnostics;
};

using FineGrayBinaryResult = FineGrayResult;

// Exact + right only. x_values must align 1:1 with observations (non-finite → drop row).
// Continuous covariate is mean-centered; HR is per +1 original unit.
FineGrayResult fine_gray_continuous(
    const std::vector<CensoringObservation>& observations,
    const std::vector<double>& x_values,
    const std::string& target_failure_mode = {},
    const std::string& covariate_name = "x");

// Exactly two distinct non-empty group labels → x in {0,1}.
FineGrayResult fine_gray_binary(
    const std::vector<CensoringObservation>& observations,
    const std::string& target_failure_mode = {});

// Multi-covariate: x_matrix rows align 1:1 with observations; each inner vector length = p.
// Columns are mean-centered independently. Max p = 5; requires target_failures ≥ 5p.
FineGrayResult fine_gray_multi(
    const std::vector<CensoringObservation>& observations,
    const std::vector<std::vector<double>>& x_matrix,
    const std::vector<std::string>& covariate_names,
    const std::string& target_failure_mode = {});

}  // namespace datalab::domain::statistics
