#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct CorrespondenceContributionRow {
    std::string label;
    double quality = 0.0;
    double mass = 0.0;
    double inertia = 0.0;
    std::vector<double> coordinates;
    std::vector<double> contributions;
};

struct SimpleCorrespondenceOptions {
    std::size_t component_count = 2;
    bool include_row_contributions = true;
    bool include_column_contributions = true;
};

struct SimpleCorrespondenceResult {
    std::size_t observation_count = 0;
    std::size_t row_level_count = 0;
    std::size_t column_level_count = 0;
    std::size_t component_count = 0;
    double total_inertia = 0.0;
    double chi_square = 0.0;
    std::size_t chi_square_df = 0;
    std::optional<double> chi_square_p_value;
    std::vector<double> singular_values;
    std::vector<double> inertia_per_component;
    std::vector<CorrespondenceContributionRow> row_contributions;
    std::vector<CorrespondenceContributionRow> column_contributions;
    std::vector<std::size_t> observation_source_rows;
    std::string algorithm_id = "simple_correspondence_svd";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

SimpleCorrespondenceResult simple_correspondence_analyze(
    const std::vector<std::string>& row_variable,
    const std::vector<std::string>& column_variable,
    const std::vector<std::size_t>& source_rows = {},
    const SimpleCorrespondenceOptions& options = {});

}  // namespace datalab::domain::statistics
