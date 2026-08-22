#include "domain/row_visibility.h"

#include <algorithm>

namespace datalab::domain {

std::unordered_set<std::size_t> to_row_set(const std::vector<std::size_t>& rows)
{
    return std::unordered_set<std::size_t>(rows.begin(), rows.end());
}

bool is_row_excluded(
    std::size_t row,
    const std::unordered_set<std::size_t>& excluded_set)
{
    return excluded_set.find(row) != excluded_set.end();
}

bool is_row_hidden(
    std::size_t row,
    const std::unordered_set<std::size_t>& hidden_set)
{
    return hidden_set.find(row) != hidden_set.end();
}

RowVisibilitySummary summarize_row_visibility(
    std::size_t total_rows,
    const std::vector<std::size_t>& hidden_rows,
    const std::vector<std::size_t>& excluded_rows)
{
    RowVisibilitySummary summary;
    summary.total_rows = total_rows;
    const auto hidden = to_row_set(hidden_rows);
    const auto excluded = to_row_set(excluded_rows);

    // Overlap is allowed but must remain distinguishable in diagnostics.
    std::size_t overlap = 0;
    for (const std::size_t row : hidden) {
        if (excluded.count(row) != 0) {
            ++overlap;
        }
    }
    if (overlap > 0) {
        summary.diagnostics.push_back({
            DiagnosticMessage::Severity::info, "row_visibility_overlap",
            "存在同时标记为 hidden 与 excluded 的行；分析以 excluded 为准，"
            "显示同时不展示。禁止合并为一个 bool。"});
    }

    for (std::size_t row = 0; row < total_rows; ++row) {
        const bool ex = is_row_excluded(row, excluded);
        const bool hi = is_row_hidden(row, hidden);
        if (ex) {
            ++summary.excluded_count;
        } else {
            ++summary.analysis_eligible_count;
            summary.analysis_rows.push_back(row);
        }
        if (hi && !ex) {
            ++summary.hidden_count;
        }
        if (!ex && !hi) {
            ++summary.visible_count;
            ++summary.display_eligible_count;
            summary.display_rows.push_back(row);
        } else if (!ex && hi) {
            // hidden-only: still counted in analysis_eligible above
        }
    }

    summary.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "row_visibility_contract",
        "hidden 只影响显示；excluded 影响分析输入。两者必须可区分。"});
    return summary;
}

}  // namespace datalab::domain
