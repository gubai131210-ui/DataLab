#pragma once

#include "domain/statistics/gage_rr.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct ExpandedGageUnbalancedOptions {
    bool include_additional_factor = false;
    bool additional_random = true;
    bool operator_random = true;
    bool part_random = true;
    double study_var_multiplier = 6.0;
};

struct ExpandedGageUnbalancedResult {
    std::size_t observation_count = 0;
    std::size_t part_count = 0;
    std::size_t operator_count = 0;
    std::size_t additional_level_count = 0;
    bool design_balanced = true;
    bool has_additional_factor = false;
    double tolerance = 0.0;
    double ndc = 0.0;
    bool ndc_available = false;
    double gage_rr_percent_study_var = 0.0;
    std::vector<GageAnovaRow> anova_rows;
    std::vector<GageVarianceComponent> variance_components;
    std::vector<std::size_t> observation_source_rows;
    std::string algorithm_id = "expanded_gage_unbalanced_glm_varcomp";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

ExpandedGageUnbalancedResult expanded_gage_unbalanced_analyze(
    const std::vector<double>& measurements,
    const std::vector<std::string>& parts,
    const std::vector<std::string>& operators,
    const std::vector<std::string>& additional,
    double tolerance = 0.0,
    const std::vector<std::size_t>& source_rows = {},
    const ExpandedGageUnbalancedOptions& options = {});

}  // namespace datalab::domain::statistics
