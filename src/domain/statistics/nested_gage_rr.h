#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct NestedGageAnovaRow {
    std::string source;
    std::size_t degrees_of_freedom = 0;
    double sum_of_squares = 0.0;
    double mean_square = 0.0;
    double f_statistic = 0.0;
};

struct NestedGageVarianceComponent {
    std::string source;
    double variance_component = 0.0;
    double standard_deviation = 0.0;
    double percent_contribution = 0.0;
    double study_variation = 0.0;
    double percent_study_variation = 0.0;
    double percent_tolerance = 0.0;
};

struct NestedGageRrResult {
    std::size_t part_count = 0;
    std::size_t operator_count = 0;
    std::size_t parts_per_operator = 0;
    std::size_t replicate_count = 0;
    double tolerance = 0.0;
    double ndc = 0.0;
    std::vector<NestedGageAnovaRow> anova_rows;
    std::vector<NestedGageVarianceComponent> variance_components;
    std::vector<DiagnosticMessage> diagnostics;
};

// Computes a balanced nested design where each part belongs to one operator.
NestedGageRrResult nested_gage_rr(
    const std::vector<double>& measurements,
    const std::vector<std::string>& parts,
    const std::vector<std::string>& operators,
    double tolerance = 0.0);

}  // namespace datalab::domain::statistics
