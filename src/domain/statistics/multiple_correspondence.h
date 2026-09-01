#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/simple_correspondence.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct MultipleCorrespondenceOptions {
    std::size_t component_count = 2;
    bool include_column_contributions = true;
};

struct MultipleCorrespondenceResult {
    std::size_t observation_count = 0;
    std::size_t variable_count = 0;
    std::size_t category_count = 0;
    std::size_t component_count = 0;
    double total_inertia = 0.0;
    double chi_square = 0.0;
    std::size_t chi_square_df = 0;
    std::optional<double> chi_square_p_value;
    std::vector<double> inertia_per_component;
    std::vector<CorrespondenceContributionRow> column_contributions;
    std::vector<std::size_t> observation_source_rows;
    std::string algorithm_id = "multiple_correspondence_burt";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

MultipleCorrespondenceResult multiple_correspondence_analyze(
    const std::vector<std::vector<std::string>>& categorical_columns,
    const std::vector<std::size_t>& source_rows = {},
    const MultipleCorrespondenceOptions& options = {});

}  // namespace datalab::domain::statistics
