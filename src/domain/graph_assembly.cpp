#include "domain/graph_assembly.h"

#include "domain/column_extract.h"

#include <algorithm>
#include <limits>

namespace datalab::domain {

std::size_t stable_group_index(std::vector<std::string>& labels, const std::string& value)
{
    const auto found = std::find(labels.cbegin(), labels.cend(), value);
    if (found != labels.cend()) {
        return static_cast<std::size_t>(std::distance(labels.cbegin(), found));
    }
    labels.push_back(value);
    return labels.size() - 1;
}

AssembledGraphColumns assemble_graph_columns(
    const DataTable& table,
    const std::size_t first_column,
    const std::optional<std::size_t>& second_column,
    const std::optional<std::size_t>& third_column,
    const std::optional<std::size_t>& group_column,
    const std::optional<std::size_t>& label_column,
    const std::vector<std::size_t>& excluded_rows,
    const bool require_numeric_first)
{
    AssembledGraphColumns result;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (std::find(excluded_rows.cbegin(), excluded_rows.cend(), row_index)
            != excluded_rows.cend()) {
            continue;
        }
        const auto& row = table.rows[row_index];
        const auto cell_at = [&](const std::size_t column) -> std::string {
            return column < row.size() ? row[column] : std::string();
        };
        double first = 0.0;
        if (require_numeric_first) {
            if (first_column >= row.size() || is_missing_cell(cell_at(first_column))
                || !parse_finite_number(cell_at(first_column), first)) {
                ++result.skipped_count;
                continue;
            }
        }
        double second = 0.0;
        if (second_column.has_value()) {
            if (*second_column >= row.size() || is_missing_cell(cell_at(*second_column))
                || !parse_finite_number(cell_at(*second_column), second)) {
                ++result.skipped_count;
                continue;
            }
        }
        double third = 0.0;
        if (third_column.has_value()) {
            if (*third_column >= row.size() || is_missing_cell(cell_at(*third_column))
                || !parse_finite_number(cell_at(*third_column), third)) {
                ++result.skipped_count;
                continue;
            }
        }
        if (require_numeric_first) {
            result.first.push_back(first);
        }
        if (second_column.has_value()) {
            result.second.push_back(second);
        }
        if (third_column.has_value()) {
            result.third.push_back(third);
        }
        result.source_rows.push_back(row_index);
        if (group_column.has_value()) {
            result.groups.push_back(cell_at(*group_column));
        }
        if (label_column.has_value()) {
            result.labels.push_back(cell_at(*label_column));
        }
        result.categories.push_back(cell_at(first_column));
    }
    return result;
}

AssembledMatrixColumns assemble_numeric_matrix(
    const DataTable& table,
    const std::vector<std::size_t>& columns,
    const std::optional<std::size_t>& group_column,
    const std::vector<std::size_t>& excluded_rows,
    const bool complete_case)
{
    AssembledMatrixColumns result;
    result.columns.assign(columns.size(), {});
    for (const std::size_t column : columns) {
        result.names.push_back(column_label(table, column));
        if (column < table.columns.size()) {
            result.names.back() = table.columns[column];
        }
    }
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (std::find(excluded_rows.cbegin(), excluded_rows.cend(), row_index)
            != excluded_rows.cend()) {
            continue;
        }
        std::vector<double> values;
        values.reserve(columns.size());
        bool any_finite = false;
        bool complete = true;
        for (const std::size_t column : columns) {
            double value = 0.0;
            if (column >= table.rows[row_index].size()
                || is_missing_cell(table.rows[row_index][column])
                || !parse_finite_number(table.rows[row_index][column], value)) {
                complete = false;
                values.push_back(std::numeric_limits<double>::quiet_NaN());
                continue;
            }
            any_finite = true;
            values.push_back(value);
        }
        if ((complete_case && !complete) || !any_finite) {
            ++result.skipped_count;
            continue;
        }
        for (std::size_t index = 0; index < values.size(); ++index) {
            result.columns[index].push_back(values[index]);
        }
        result.source_rows.push_back(row_index);
        if (group_column.has_value()) {
            result.groups.push_back(
                *group_column < table.rows[row_index].size()
                    ? table.rows[row_index][*group_column] : std::string());
        }
    }
    return result;
}

FacetPartitionResult partition_facet_levels(
    const std::vector<std::string>& facet_labels,
    int max_panels)
{
    FacetPartitionResult result;
    const int capped = std::clamp(max_panels, 1, 12);
    std::vector<std::string> order;
    std::vector<std::vector<std::size_t>> members;
    for (std::size_t index = 0; index < facet_labels.size(); ++index) {
        const std::string level =
            facet_labels[index].empty() ? "(空)" : facet_labels[index];
        const std::size_t panel = stable_group_index(order, level);
        if (panel >= members.size()) {
            members.resize(panel + 1);
        }
        members[panel].push_back(index);
    }
    result.level_count = order.size();
    const std::size_t keep = std::min(order.size(), static_cast<std::size_t>(capped));
    for (std::size_t i = 0; i < keep; ++i) {
        FacetPanel panel;
        panel.level = order[i];
        panel.member_indices = members[i];
        result.panels.push_back(std::move(panel));
    }
    if (order.size() > keep) {
        result.truncated_levels = order.size() - keep;
        DiagnosticMessage diagnostic;
        diagnostic.severity = DiagnosticMessage::Severity::warning;
        diagnostic.code = "facet_levels_truncated";
        diagnostic.message =
            "分面水平数 = " + std::to_string(order.size())
            + "，受控 Graph Builder 最多显示 " + std::to_string(capped)
            + " 个面板；已截断 " + std::to_string(result.truncated_levels)
            + " 个水平（非自由拼版）。";
        result.diagnostics.push_back(std::move(diagnostic));
    }
    DiagnosticMessage note;
    note.severity = DiagnosticMessage::Severity::info;
    note.code = "facet_controlled_panels";
    note.message =
        "使用受控分面面板（facet），不是自由像素拼版；"
        "by/分组仍是图内着色，与分面列不同。";
    result.diagnostics.push_back(std::move(note));
    return result;
}

}  // namespace datalab::domain
