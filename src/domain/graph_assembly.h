#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain {

struct AssembledGraphColumns {
    std::vector<double> first;
    std::vector<double> second;
    std::vector<double> third;
    std::vector<std::size_t> source_rows;
    std::vector<std::string> groups;
    std::vector<std::string> labels;
    std::vector<std::string> categories;
    std::size_t skipped_count = 0;
};

struct AssembledMatrixColumns {
    std::vector<std::string> names;
    std::vector<std::vector<double>> columns;
    std::vector<std::size_t> source_rows;
    std::vector<std::string> groups;
    std::size_t skipped_count = 0;
};

AssembledGraphColumns assemble_graph_columns(
    const DataTable& table,
    std::size_t first_column,
    const std::optional<std::size_t>& second_column,
    const std::optional<std::size_t>& third_column,
    const std::optional<std::size_t>& group_column,
    const std::optional<std::size_t>& label_column,
    const std::vector<std::size_t>& excluded_rows,
    bool require_numeric_first = true);

AssembledGraphColumns assemble_time_series_columns(
    const DataTable& table,
    std::size_t time_column,
    std::size_t value_column,
    const std::optional<std::size_t>& group_column,
    const std::optional<std::size_t>& label_column,
    const std::vector<std::size_t>& excluded_rows);

AssembledMatrixColumns assemble_numeric_matrix(
    const DataTable& table,
    const std::vector<std::size_t>& columns,
    const std::optional<std::size_t>& group_column,
    const std::vector<std::size_t>& excluded_rows,
    bool complete_case = true);

std::size_t stable_group_index(std::vector<std::string>& labels, const std::string& value);

struct FacetPanel {
    std::string level;
    std::vector<std::size_t> member_indices;  // indices into parallel source arrays
};

struct FacetPartitionResult {
    std::vector<FacetPanel> panels;
    std::size_t level_count = 0;
    std::size_t truncated_levels = 0;
    std::vector<DiagnosticMessage> diagnostics;
};

// Stable first-seen level order. max_panels clamps productized facet grid (1..12).
FacetPartitionResult partition_facet_levels(
    const std::vector<std::string>& facet_labels,
    int max_panels);

}  // namespace datalab::domain
