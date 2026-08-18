#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct GageAnovaRow {
    std::string source;
    std::size_t degrees_of_freedom = 0;
    double sum_of_squares = 0.0;
    double mean_square = 0.0;
    double f_statistic = 0.0;
    std::optional<double> p_value;
};

struct GageVarianceComponent {
    std::string source;
    double raw_variance_component = 0.0;
    double variance_component = 0.0;
    bool truncated = false;
    double standard_deviation = 0.0;
    double percent_contribution = 0.0;
    double study_variation = 0.0;
    double percent_study_variation = 0.0;
    double percent_tolerance = 0.0;
    bool percent_tolerance_available = false;
};

struct GageRrResult {
    std::size_t part_count = 0;
    std::size_t operator_count = 0;
    std::size_t replicate_count = 0;
    double tolerance = 0.0;
    double ndc = 0.0;
    double study_var_multiplier = 6.0;
    std::string method = "anova";
    bool ndc_available = false;
    bool design_balanced = true;
    bool interaction_retained = true;
    std::optional<double> interaction_p_value;
    bool interaction_reduction_recommended = false;
    bool negative_variance_truncated = false;
    QualityEvidence evidence;
    std::vector<GageAnovaRow> anova_rows;
    std::vector<GageVarianceComponent> variance_components;
    std::vector<RuleEvidence> rules;
    std::vector<DiagnosticMessage> diagnostics;
};

GageRrResult crossed_gage_rr(
    const std::vector<double>& measurements,
    const std::vector<std::string>& parts,
    const std::vector<std::string>& operators,
    double tolerance = 0.0);

}  // namespace datalab::domain::statistics
