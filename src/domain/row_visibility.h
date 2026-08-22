#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <unordered_set>
#include <vector>

namespace datalab::domain {

enum class RowVisibilityKind {
    visible,
    hidden,    // display-only; still eligible for analysis unless also excluded
    excluded   // removed from analysis input
};

struct RowVisibilitySummary {
    std::size_t total_rows = 0;
    std::size_t visible_count = 0;
    std::size_t hidden_count = 0;
    std::size_t excluded_count = 0;
    std::size_t analysis_eligible_count = 0;  // not excluded
    std::size_t display_eligible_count = 0;   // not hidden and not excluded
    std::vector<std::size_t> analysis_rows;
    std::vector<std::size_t> display_rows;
    std::vector<DiagnosticMessage> diagnostics;
};

// hidden and excluded are independent sets. A row in both is treated as excluded
// for analysis and as non-display for preview (excluded wins for analysis).
RowVisibilitySummary summarize_row_visibility(
    std::size_t total_rows,
    const std::vector<std::size_t>& hidden_rows,
    const std::vector<std::size_t>& excluded_rows);

bool is_row_excluded(
    std::size_t row,
    const std::unordered_set<std::size_t>& excluded_set);

bool is_row_hidden(
    std::size_t row,
    const std::unordered_set<std::size_t>& hidden_set);

std::unordered_set<std::size_t> to_row_set(const std::vector<std::size_t>& rows);

}  // namespace datalab::domain
