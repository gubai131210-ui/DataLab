#include "application/graph_service.h"

#include "domain/column_extract.h"
#include "domain/graph_assembly.h"
#include "domain/row_visibility.h"
#include "domain/statistics/graph_visuals.h"
#include "domain/statistics/eda_plots.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_set>

namespace datalab::application {
namespace {

using datalab::domain::AnalysisConfiguration;
using datalab::domain::AssembledGraphColumns;
using datalab::domain::AssembledMatrixColumns;
using datalab::domain::DataTable;
using datalab::domain::DiagnosticMessage;
using datalab::domain::EdaPlotFacts;
using datalab::domain::OutputPage;
using datalab::domain::PlotKind;
using datalab::domain::PlotSpec;
using datalab::domain::assemble_graph_columns;
using datalab::domain::assemble_numeric_matrix;
using datalab::domain::parse_finite_number;
using datalab::domain::summarize_row_visibility;
using datalab::domain::to_row_set;

OutputPage error_page(const std::string& title, const std::string& message)
{
    OutputPage page;
    page.title = title;
    page.method_name = title;
    page.diagnostics.push_back(
        {DiagnosticMessage::Severity::error, "graph_input", message});
    return page;
}

std::string label(const DataTable& table, const std::size_t column)
{
    return column < table.columns.size()
        ? table.columns[column] : "C" + std::to_string(column + 1);
}

std::string count_text(const std::size_t count)
{
    return std::to_string(count);
}

void attach_visibility_facts(
    OutputPage& page,
    const std::string& kind,
    const DataTable& table,
    const AnalysisConfiguration& configuration,
    std::size_t plotted_n,
    std::size_t analysis_n = 0,
    std::size_t analysis_category_count = 0)
{
    const auto visibility = summarize_row_visibility(
        table.rows.size(), configuration.hidden_rows, configuration.excluded_rows);
    page.diagnostics.insert(page.diagnostics.end(),
                            visibility.diagnostics.begin(), visibility.diagnostics.end());
    EdaPlotFacts facts;
    facts.kind = kind;
    facts.n = plotted_n;
    facts.hidden_count = visibility.hidden_count;
    facts.excluded_count = visibility.excluded_count;
    facts.analysis_eligible_n = visibility.analysis_eligible_count;
    facts.display_eligible_n = visibility.display_eligible_count;
    facts.hidden_excluded_distinct = true;
    facts.analysis_n = analysis_n == 0 ? visibility.analysis_eligible_count : analysis_n;
    facts.analysis_category_count = analysis_category_count;
    page.facts.eda = std::move(facts);
}

void apply_facet_facts(
    OutputPage& page,
    const domain::FacetPartitionResult& partition,
    int facet_max_panels)
{
    if (!page.facts.eda.has_value()) {
        return;
    }
    page.facts.eda->facet_enabled = true;
    page.facts.eda->facet_panel_count = page.plots.size();
    page.facts.eda->facet_level_count = partition.level_count;
    page.facts.eda->facet_truncated_levels = partition.truncated_levels;
    page.facts.eda->facet_max_panels = std::clamp(facet_max_panels, 1, 12);
}

std::vector<std::string> facet_labels_for_rows(
    const DataTable& table,
    const std::vector<std::size_t>& source_rows,
    std::size_t facet_column)
{
    std::vector<std::string> labels;
    labels.reserve(source_rows.size());
    for (const std::size_t row : source_rows) {
        if (row >= table.rows.size() || facet_column >= table.rows[row].size()) {
            labels.emplace_back();
        } else {
            labels.push_back(table.rows[row][facet_column]);
        }
    }
    return labels;
}

AssembledGraphColumns xy_columns(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    // Analysis/complete-case assembly uses excluded only — never fold hidden into excluded.
    return assemble_graph_columns(
        table, graph.x_column.value_or(0), graph.y_column, graph.size_column,
        graph.by_column, graph.label_column, configuration.excluded_rows, true);
}

template <typename PlotResult>
void filter_hidden_from_plot_points(
    PlotResult& result,
    const std::vector<std::size_t>& hidden_rows)
{
    if (hidden_rows.empty() || result.source_rows.empty()) {
        return;
    }
    const auto hidden = to_row_set(hidden_rows);
    std::vector<double> x;
    std::vector<double> y;
    std::vector<std::size_t> rows;
    std::vector<std::string> groups;
    std::vector<std::string> labels;
    x.reserve(result.source_rows.size());
    y.reserve(result.source_rows.size());
    rows.reserve(result.source_rows.size());
    for (std::size_t index = 0; index < result.source_rows.size(); ++index) {
        if (hidden.count(result.source_rows[index]) != 0) {
            continue;
        }
        if (index < result.x_values.size()) {
            x.push_back(result.x_values[index]);
        }
        if (index < result.y_values.size()) {
            y.push_back(result.y_values[index]);
        }
        rows.push_back(result.source_rows[index]);
        if (index < result.point_groups.size()) {
            groups.push_back(result.point_groups[index]);
        }
        if (index < result.point_labels.size()) {
            labels.push_back(result.point_labels[index]);
        }
    }
    result.x_values = std::move(x);
    result.y_values = std::move(y);
    result.source_rows = std::move(rows);
    result.point_groups = std::move(groups);
    result.point_labels = std::move(labels);
}

void filter_hidden_from_bubble_sizes(
    domain::statistics::BubblePlotResult& result,
    const std::vector<std::size_t>& hidden_rows)
{
    if (hidden_rows.empty() || result.points.source_rows.empty()) {
        return;
    }
    const auto hidden = to_row_set(hidden_rows);
    std::vector<double> kept_sizes;
    kept_sizes.reserve(result.points.source_rows.size());
    for (std::size_t index = 0; index < result.points.source_rows.size(); ++index) {
        if (hidden.count(result.points.source_rows[index]) != 0) {
            continue;
        }
        if (index < result.sizes.size()) {
            kept_sizes.push_back(result.sizes[index]);
        }
    }
    filter_hidden_from_plot_points(result.points, hidden_rows);
    result.sizes = std::move(kept_sizes);
}

AssembledGraphColumns filter_hidden_from_columns(
    const AssembledGraphColumns& columns,
    const std::vector<std::size_t>& hidden_rows)
{
    if (hidden_rows.empty() || columns.source_rows.empty()) {
        return columns;
    }
    const auto hidden = to_row_set(hidden_rows);
    AssembledGraphColumns filtered;
    filtered.first.reserve(columns.first.size());
    filtered.second.reserve(columns.second.size());
    filtered.third.reserve(columns.third.size());
    filtered.groups.reserve(columns.groups.size());
    filtered.labels.reserve(columns.labels.size());
    filtered.categories.reserve(columns.categories.size());
    filtered.source_rows.reserve(columns.source_rows.size());
    for (std::size_t index = 0; index < columns.source_rows.size(); ++index) {
        if (hidden.count(columns.source_rows[index]) != 0) {
            continue;
        }
        if (index < columns.first.size()) {
            filtered.first.push_back(columns.first[index]);
        }
        if (index < columns.second.size()) {
            filtered.second.push_back(columns.second[index]);
        }
        if (index < columns.third.size()) {
            filtered.third.push_back(columns.third[index]);
        }
        if (index < columns.groups.size()) {
            filtered.groups.push_back(columns.groups[index]);
        }
        if (index < columns.labels.size()) {
            filtered.labels.push_back(columns.labels[index]);
        }
        if (index < columns.categories.size()) {
            filtered.categories.push_back(columns.categories[index]);
        }
        filtered.source_rows.push_back(columns.source_rows[index]);
    }
    filtered.skipped_count = columns.skipped_count;
    return filtered;
}

AssembledMatrixColumns filter_hidden_from_matrix(
    const AssembledMatrixColumns& assembled,
    const std::vector<std::size_t>& hidden_rows)
{
    if (hidden_rows.empty() || assembled.source_rows.empty()) {
        return assembled;
    }
    const auto hidden = to_row_set(hidden_rows);
    AssembledMatrixColumns filtered;
    filtered.names = assembled.names;
    filtered.columns.resize(assembled.columns.size());
    filtered.source_rows.reserve(assembled.source_rows.size());
    filtered.groups.reserve(assembled.groups.size());
    for (std::size_t index = 0; index < assembled.source_rows.size(); ++index) {
        if (hidden.count(assembled.source_rows[index]) != 0) {
            continue;
        }
        filtered.source_rows.push_back(assembled.source_rows[index]);
        if (index < assembled.groups.size()) {
            filtered.groups.push_back(assembled.groups[index]);
        }
        for (std::size_t column = 0; column < assembled.columns.size(); ++column) {
            if (index < assembled.columns[column].size()) {
                filtered.columns[column].push_back(assembled.columns[column][index]);
            }
        }
    }
    filtered.skipped_count = assembled.skipped_count;
    return filtered;
}

AssembledMatrixColumns slice_matrix_columns(
    const AssembledMatrixColumns& assembled,
    const std::vector<std::size_t>& member_indices)
{
    AssembledMatrixColumns slice;
    slice.names = assembled.names;
    slice.columns.resize(assembled.columns.size());
    slice.source_rows.reserve(member_indices.size());
    for (const std::size_t index : member_indices) {
        if (index >= assembled.source_rows.size()) {
            continue;
        }
        slice.source_rows.push_back(assembled.source_rows[index]);
        if (index < assembled.groups.size()) {
            slice.groups.push_back(assembled.groups[index]);
        }
        for (std::size_t column = 0; column < assembled.columns.size(); ++column) {
            if (index < assembled.columns[column].size()) {
                slice.columns[column].push_back(assembled.columns[column][index]);
            }
        }
    }
    slice.skipped_count = assembled.skipped_count;
    return slice;
}

AssembledGraphColumns slice_graph_columns(
    const AssembledGraphColumns& columns,
    const std::vector<std::size_t>& member_indices)
{
    AssembledGraphColumns slice;
    for (const std::size_t index : member_indices) {
        if (index >= columns.source_rows.size()) {
            continue;
        }
        if (index < columns.first.size()) {
            slice.first.push_back(columns.first[index]);
        }
        if (index < columns.second.size()) {
            slice.second.push_back(columns.second[index]);
        }
        if (index < columns.third.size()) {
            slice.third.push_back(columns.third[index]);
        }
        if (index < columns.groups.size()) {
            slice.groups.push_back(columns.groups[index]);
        }
        if (index < columns.labels.size()) {
            slice.labels.push_back(columns.labels[index]);
        }
        if (index < columns.categories.size()) {
            slice.categories.push_back(columns.categories[index]);
        }
        slice.source_rows.push_back(columns.source_rows[index]);
    }
    slice.skipped_count = columns.skipped_count;
    return slice;
}

PlotSpec base_plot(const PlotKind kind, const std::string& title,
                   const std::string& x_title, const std::string& y_title)
{
    PlotSpec plot;
    plot.kind = kind;
    plot.title = title;
    plot.x_axis_title = x_title;
    plot.y_axis_title = y_title;
    return plot;
}

}  // namespace

OutputPage GraphService::run(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const std::string& kind = configuration.graph.graph_kind;
    if (kind == "interval") {
        return interval(table, configuration);
    }
    if (kind == "correlation") {
        return correlation(table, configuration);
    }
    if (kind == "bubble") {
        return bubble(table, configuration);
    }
    if (kind == "probability") {
        return probability(table, configuration);
    }
    if (kind == "ecdf") {
        return ecdf(table, configuration);
    }
    if (kind == "matrix") {
        return matrix(table, configuration);
    }
    if (kind == "marginal") {
        return marginal(table, configuration);
    }
    if (kind == "parallel") {
        return parallel(table, configuration);
    }
    if (kind == "heatmap") {
        return heatmap(table, configuration);
    }
    if (kind == "time_series") {
        return time_series(table, configuration);
    }
    if (kind == "area") {
        return area(table, configuration);
    }
    if (kind == "contour") {
        return contour(table, configuration);
    }
    if (kind == "pie") {
        return pie(table, configuration);
    }
    if (kind == "density") {
        return density(table, configuration);
    }
    if (kind == "hexbin") {
        return hexbin(table, configuration);
    }
    if (kind == "violin") {
        return violin(table, configuration);
    }
    if (kind == "bar") {
        return bar(table, configuration);
    }
    return scatter(table, configuration);
}

OutputPage GraphService::scatter(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.x_column.has_value() || !graph.y_column.has_value()) {
        return error_page("散点图", "请选择 X 变量和 Y 变量。");
    }
    const AssembledGraphColumns columns = xy_columns(table, configuration);
    const std::size_t analysis_n = columns.source_rows.size();

    if (!graph.facet_column.has_value()) {
        auto result = domain::statistics::scatter_plot(
            columns.first, columns.second, columns.source_rows, columns.groups,
            columns.labels);
        filter_hidden_from_plot_points(result, configuration.hidden_rows);
        OutputPage page;
        page.configuration = configuration;
        page.title = "散点图";
        page.method_name = "Scatter Plot";
        page.parameter_summary = "X = " + label(table, *graph.x_column)
            + "    Y = " + label(table, *graph.y_column)
            + "    显示 N = " + count_text(result.x_values.size())
            + "    分析 N = " + count_text(analysis_n)
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        page.diagnostics = result.diagnostics;
        PlotSpec plot = base_plot(PlotKind::scatter, "散点图",
                                  label(table, *graph.x_column), label(table, *graph.y_column));
        plot.x_values = result.x_values;
        plot.values = result.y_values;
        plot.source_rows = result.source_rows;
        plot.point_groups = result.point_groups;
        plot.point_labels = result.point_labels;
        page.plots.push_back(std::move(plot));
        attach_visibility_facts(page, "scatter", table, configuration, result.x_values.size(),
                                analysis_n);
        return page;
    }

    // Controlled facet: separate panels per level (≠ by_column within-plot grouping).
    std::vector<std::string> facet_labels;
    facet_labels.reserve(columns.source_rows.size());
    for (const std::size_t row : columns.source_rows) {
        if (row >= table.rows.size() || *graph.facet_column >= table.rows[row].size()) {
            facet_labels.emplace_back();
        } else {
            facet_labels.push_back(table.rows[row][*graph.facet_column]);
        }
    }
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "散点图（分面）";
    page.method_name = "Faceted Scatter Plot";
    std::size_t display_n = 0;
    const auto hidden = to_row_set(configuration.hidden_rows);
    for (const auto& panel : partition.panels) {
        std::vector<double> xs;
        std::vector<double> ys;
        std::vector<std::size_t> rows;
        std::vector<std::string> groups;
        std::vector<std::string> labels;
        for (const std::size_t index : panel.member_indices) {
            if (index >= columns.source_rows.size()) {
                continue;
            }
            const std::size_t source_row = columns.source_rows[index];
            if (hidden.count(source_row) != 0) {
                continue;
            }
            if (index < columns.first.size()) {
                xs.push_back(columns.first[index]);
            }
            if (index < columns.second.size()) {
                ys.push_back(columns.second[index]);
            }
            rows.push_back(source_row);
            if (index < columns.groups.size()) {
                groups.push_back(columns.groups[index]);
            }
            if (index < columns.labels.size()) {
                labels.push_back(columns.labels[index]);
            }
        }
        display_n += xs.size();
        auto result = domain::statistics::scatter_plot(xs, ys, rows, groups, labels);
        PlotSpec plot = base_plot(
            PlotKind::scatter,
            "散点图 · " + panel.level,
            label(table, *graph.x_column),
            label(table, *graph.y_column));
        plot.subtitle = "facet = " + panel.level
            + "    分析 N(水平) = " + count_text(panel.member_indices.size())
            + "    显示 N = " + count_text(xs.size());
        plot.x_values = result.x_values;
        plot.values = result.y_values;
        plot.source_rows = result.source_rows;
        plot.point_groups = result.point_groups;
        plot.point_labels = result.point_labels;
        page.plots.push_back(std::move(plot));
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "X = " + label(table, *graph.x_column)
        + "    Y = " + label(table, *graph.y_column)
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_n)
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "scatter", table, configuration, display_n, analysis_n);
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::interval(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.y_column.has_value() || !graph.by_column.has_value()) {
        return error_page("区间散点图", "请选择响应变量和分类变量。");
    }
    const AssembledGraphColumns analysis_columns = assemble_graph_columns(
        table, *graph.y_column, {}, {}, graph.by_column, {}, configuration.excluded_rows, true);

    const auto push_interval_plot = [&](OutputPage& page,
                                        const AssembledGraphColumns& analysis_slice,
                                        const std::string& title,
                                        const std::string& subtitle) {
        AssembledGraphColumns display_slice =
            filter_hidden_from_columns(analysis_slice, configuration.hidden_rows);
        const auto analysis_result = domain::statistics::interval_plot(
            analysis_slice.first, analysis_slice.groups, analysis_slice.source_rows,
            graph.confidence_level);
        const auto result = domain::statistics::interval_plot(
            display_slice.first, display_slice.groups, display_slice.source_rows,
            graph.confidence_level);
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
        page.diagnostics.insert(
            page.diagnostics.end(),
            analysis_result.diagnostics.begin(),
            analysis_result.diagnostics.end());
        PlotSpec plot = base_plot(
            PlotKind::interval, title,
            label(table, *graph.by_column), label(table, *graph.y_column));
        plot.subtitle = subtitle;
        plot.categories = result.labels;
        plot.values = result.means;
        plot.interval_lower = result.lower;
        plot.interval_upper = result.upper;
        plot.interval_counts = result.counts;
        for (const auto& rows : result.source_rows) {
            if (!rows.empty()) {
                plot.source_rows.push_back(rows.front());
            }
            plot.member_source_rows.push_back(rows);
        }
        page.plots.push_back(std::move(plot));
        return std::make_pair(display_slice.source_rows.size(), analysis_result.labels.size());
    };

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "区间散点图";
        page.method_name = "Interval Plot";
        const auto counts = push_interval_plot(page, analysis_columns, "区间散点图", "");
        page.parameter_summary = "响应 = " + label(table, *graph.y_column)
            + "    分组 = " + label(table, *graph.by_column)
            + "    置信水平 = " + std::to_string(graph.confidence_level)
            + "    显示 N = " + count_text(counts.first)
            + "    分析 N = " + count_text(analysis_columns.first.size())
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "interval", table, configuration,
                                counts.first, analysis_columns.first.size(),
                                counts.second);
        if (page.facts.eda.has_value()) {
            page.facts.eda->category_count = counts.second;
        }
        return page;
    }

    const auto facet_labels = facet_labels_for_rows(
        table, analysis_columns.source_rows, *graph.facet_column);
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "区间散点图（分面）";
    page.method_name = "Faceted Interval Plot";
    std::size_t display_n = 0;
    std::size_t analysis_category_count = 0;
    for (const auto& panel : partition.panels) {
        AssembledGraphColumns slice;
        for (const std::size_t index : panel.member_indices) {
            if (index >= analysis_columns.source_rows.size()) {
                continue;
            }
            if (index < analysis_columns.first.size()) {
                slice.first.push_back(analysis_columns.first[index]);
            }
            if (index < analysis_columns.groups.size()) {
                slice.groups.push_back(analysis_columns.groups[index]);
            }
            slice.source_rows.push_back(analysis_columns.source_rows[index]);
        }
        const auto counts = push_interval_plot(
            page,
            slice,
            "区间散点图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size()));
        display_n += counts.first;
        analysis_category_count = std::max(analysis_category_count, counts.second);
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "响应 = " + label(table, *graph.y_column)
        + "    分组 = " + label(table, *graph.by_column)
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_columns.first.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "interval", table, configuration, display_n,
                            analysis_columns.first.size(), analysis_category_count);
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::correlation(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (graph.variable_columns.size() < 2) {
        return error_page("相关图", "请至少选择两个连续变量。");
    }
    const AssembledMatrixColumns analysis_assembled = assemble_numeric_matrix(
        table, graph.variable_columns, {}, configuration.excluded_rows);

    const auto push_correlation_plot = [&](OutputPage& page,
                                           const AssembledMatrixColumns& analysis_slice,
                                           const std::string& title,
                                           const std::string& subtitle) {
        AssembledMatrixColumns display_slice =
            filter_hidden_from_matrix(analysis_slice, configuration.hidden_rows);
        const auto analysis_result = domain::statistics::correlation_plot(
            analysis_slice.columns, analysis_slice.names, graph.correlation_method,
            graph.confidence_level);
        const auto result = domain::statistics::correlation_plot(
            display_slice.columns, display_slice.names, graph.correlation_method,
            graph.confidence_level);
        page.diagnostics.insert(
            page.diagnostics.end(),
            result.correlation.diagnostics.begin(),
            result.correlation.diagnostics.end());
        page.diagnostics.insert(
            page.diagnostics.end(),
            analysis_result.correlation.diagnostics.begin(),
            analysis_result.correlation.diagnostics.end());
        PlotSpec plot = base_plot(PlotKind::correlation, title, "变量", "变量");
        plot.subtitle = subtitle;
        plot.matrix_labels = result.labels;
        plot.matrix_values = result.correlation.coefficients;
        plot.matrix_counts = result.correlation.counts;
        plot.color_min = -1.0;
        plot.color_max = 1.0;
        for (const auto& pair : result.correlation.pairs) {
            if (pair.p_value.has_value()) {
                if (plot.matrix_p_values.size() < display_slice.names.size()) {
                    plot.matrix_p_values.assign(
                        display_slice.names.size(),
                        std::vector<double>(display_slice.names.size(), 0.0));
                }
                plot.matrix_p_values[pair.first_column][pair.second_column] = *pair.p_value;
                plot.matrix_p_values[pair.second_column][pair.first_column] = *pair.p_value;
            }
        }
        page.plots.push_back(std::move(plot));
        return display_slice.source_rows.size();
    };

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "相关图";
        page.method_name = "Correlation Plot";
        const std::size_t display_n =
            push_correlation_plot(page, analysis_assembled, "相关图", "");
        page.parameter_summary = "方法 = " + graph.correlation_method
            + "    变量数 = " + count_text(analysis_assembled.names.size())
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_assembled.source_rows.size())
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "correlation", table, configuration, display_n,
                                analysis_assembled.source_rows.size());
        return page;
    }

    const auto facet_labels = facet_labels_for_rows(
        table, analysis_assembled.source_rows, *graph.facet_column);
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "相关图（分面）";
    page.method_name = "Faceted Correlation Plot";
    std::size_t display_n = 0;
    for (const auto& panel : partition.panels) {
        display_n += push_correlation_plot(
            page,
            slice_matrix_columns(analysis_assembled, panel.member_indices),
            "相关图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size())
                + "    相关矩阵热图单元格不是观测层；不伪造 per-cell member_source_rows");
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "方法 = " + graph.correlation_method
        + "    变量数 = " + count_text(analysis_assembled.names.size())
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_assembled.source_rows.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "correlation", table, configuration, display_n,
                            analysis_assembled.source_rows.size());
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::bubble(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.x_column.has_value() || !graph.y_column.has_value()
        || !graph.size_column.has_value()) {
        return error_page("气泡图", "请选择 X、Y 和气泡大小变量。");
    }
    const AssembledGraphColumns analysis_columns = xy_columns(table, configuration);

    const auto push_bubble_plot = [&](OutputPage& page,
                                      const AssembledGraphColumns& analysis_slice,
                                      const std::string& title,
                                      const std::string& subtitle) {
        auto result = domain::statistics::bubble_plot(
            analysis_slice.first, analysis_slice.second, analysis_slice.third,
            analysis_slice.source_rows, analysis_slice.groups, analysis_slice.labels);
        const std::size_t analysis_n = result.points.source_rows.size();
        filter_hidden_from_bubble_sizes(result, configuration.hidden_rows);
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
        PlotSpec plot = base_plot(
            PlotKind::bubble, title,
            label(table, *graph.x_column), label(table, *graph.y_column));
        plot.subtitle = subtitle;
        plot.x_values = result.points.x_values;
        plot.values = result.points.y_values;
        plot.bubble_sizes = result.sizes;
        plot.source_rows = result.points.source_rows;
        plot.point_groups = result.points.point_groups;
        plot.point_labels = result.points.point_labels;
        page.plots.push_back(std::move(plot));
        return std::make_pair(result.points.x_values.size(), analysis_n);
    };

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "气泡图";
        page.method_name = "Bubble Plot";
        const auto counts = push_bubble_plot(page, analysis_columns, "气泡图", "");
        page.parameter_summary = "X = " + label(table, *graph.x_column)
            + "    Y = " + label(table, *graph.y_column)
            + "    大小 = " + label(table, *graph.size_column)
            + "    显示 N = " + count_text(counts.first)
            + "    分析 N = " + count_text(counts.second)
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "bubble", table, configuration, counts.first,
                                counts.second);
        return page;
    }

    const auto facet_labels = facet_labels_for_rows(
        table, analysis_columns.source_rows, *graph.facet_column);
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "气泡图（分面）";
    page.method_name = "Faceted Bubble Plot";
    std::size_t display_n = 0;
    for (const auto& panel : partition.panels) {
        const auto counts = push_bubble_plot(
            page,
            slice_graph_columns(analysis_columns, panel.member_indices),
            "气泡图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size()));
        display_n += counts.first;
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "X = " + label(table, *graph.x_column)
        + "    Y = " + label(table, *graph.y_column)
        + "    大小 = " + label(table, *graph.size_column)
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_columns.source_rows.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "bubble", table, configuration, display_n,
                            analysis_columns.source_rows.size());
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::probability(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.y_column.has_value()) {
        return error_page("正态概率图", "请选择连续变量。");
    }
    const AssembledGraphColumns analysis_columns = assemble_graph_columns(
        table, *graph.y_column, {}, {}, {}, {}, configuration.excluded_rows, true);

    const auto push_probability_plot = [&](OutputPage& page,
                                           const AssembledGraphColumns& analysis_slice,
                                           const std::string& title,
                                           const std::string& subtitle) {
        AssembledGraphColumns display_slice =
            filter_hidden_from_columns(analysis_slice, configuration.hidden_rows);
        const auto analysis_fit = domain::statistics::probability_plot(
            analysis_slice.first, analysis_slice.source_rows);
        const auto result = domain::statistics::probability_plot(
            display_slice.first, display_slice.source_rows);
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
        page.diagnostics.insert(
            page.diagnostics.end(),
            analysis_fit.diagnostics.begin(),
            analysis_fit.diagnostics.end());
        PlotSpec plot = base_plot(
            PlotKind::probability, title, "理论分位数", label(table, *graph.y_column));
        plot.subtitle = subtitle.empty()
            ? "直线性不能单独作为正态性证明；显示拟合省略 hidden，分析拟合保留 hidden"
            : subtitle;
        plot.x_values = result.theoretical_quantiles;
        plot.values = result.ordered_values;
        plot.center = result.fitted;
        plot.lower = result.lower;
        plot.upper = result.upper;
        plot.source_rows = result.source_rows;
        page.plots.push_back(std::move(plot));
        return std::make_pair(result.ordered_values.size(), analysis_fit.ordered_values.size());
    };

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "正态概率图";
        page.method_name = "Normal Probability Plot";
        const auto counts =
            push_probability_plot(page, analysis_columns, "正态概率图", "");
        page.parameter_summary = "变量 = " + label(table, *graph.y_column)
            + "    显示 N = " + count_text(counts.first)
            + "    分析 N = " + count_text(counts.second)
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "probability", table, configuration, counts.first,
                                counts.second);
        return page;
    }

    const auto facet_labels = facet_labels_for_rows(
        table, analysis_columns.source_rows, *graph.facet_column);
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "正态概率图（分面）";
    page.method_name = "Faceted Normal Probability Plot";
    std::size_t display_n = 0;
    for (const auto& panel : partition.panels) {
        const auto counts = push_probability_plot(
            page,
            slice_graph_columns(analysis_columns, panel.member_indices),
            "正态概率图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size())
                + "    直线性不能单独作为正态性证明");
        display_n += counts.first;
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "变量 = " + label(table, *graph.y_column)
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_columns.source_rows.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "probability", table, configuration, display_n,
                            analysis_columns.source_rows.size());
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::ecdf(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.y_column.has_value()) {
        return error_page("经验累积分布图", "请选择连续变量。");
    }
    const AssembledGraphColumns analysis_columns = assemble_graph_columns(
        table, *graph.y_column, {}, {}, {}, {}, configuration.excluded_rows, true);

    const auto push_ecdf_plot = [&](OutputPage& page,
                                    const AssembledGraphColumns& analysis_slice,
                                    const std::string& title,
                                    const std::string& subtitle) {
        AssembledGraphColumns display_slice =
            filter_hidden_from_columns(analysis_slice, configuration.hidden_rows);
        const auto analysis_ecdf =
            domain::statistics::ecdf_plot(analysis_slice.first, analysis_slice.source_rows);
        const auto result =
            domain::statistics::ecdf_plot(display_slice.first, display_slice.source_rows);
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
        page.diagnostics.insert(
            page.diagnostics.end(),
            analysis_ecdf.diagnostics.begin(),
            analysis_ecdf.diagnostics.end());
        PlotSpec plot = base_plot(
            PlotKind::ecdf, title, label(table, *graph.y_column), "累计比例");
        plot.subtitle = subtitle;
        plot.x_values = result.values;
        plot.values = result.proportions;
        plot.source_rows = result.source_rows;
        plot.interval_counts = result.counts;
        page.plots.push_back(std::move(plot));
        return display_slice.first.size();
    };

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "经验累积分布图";
        page.method_name = "Empirical CDF";
        const std::size_t display_n =
            push_ecdf_plot(page, analysis_columns, "经验累积分布图", "");
        page.parameter_summary = "变量 = " + label(table, *graph.y_column)
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_columns.first.size())
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "ecdf", table, configuration, display_n,
                                analysis_columns.first.size());
        return page;
    }

    const auto facet_labels = facet_labels_for_rows(
        table, analysis_columns.source_rows, *graph.facet_column);
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "经验累积分布图（分面）";
    page.method_name = "Faceted Empirical CDF";
    std::size_t display_n = 0;
    for (const auto& panel : partition.panels) {
        display_n += push_ecdf_plot(
            page,
            slice_graph_columns(analysis_columns, panel.member_indices),
            "经验累积分布图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size()));
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "变量 = " + label(table, *graph.y_column)
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_columns.first.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "ecdf", table, configuration, display_n,
                            analysis_columns.first.size());
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::matrix(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (graph.variable_columns.size() < 2) {
        return error_page("矩阵图", "请至少选择两个连续变量。");
    }
    const AssembledMatrixColumns analysis_assembled = assemble_numeric_matrix(
        table, graph.variable_columns, graph.by_column, configuration.excluded_rows);

    const auto push_matrix_plot = [&](OutputPage& page,
                                      const AssembledMatrixColumns& analysis_slice,
                                      const std::string& title,
                                      const std::string& subtitle) {
        AssembledMatrixColumns display_slice =
            filter_hidden_from_matrix(analysis_slice, configuration.hidden_rows);
        const auto analysis_result = domain::statistics::matrix_scatter_plot(
            analysis_slice.columns, analysis_slice.names,
            analysis_slice.source_rows, analysis_slice.groups);
        const auto result = domain::statistics::matrix_scatter_plot(
            display_slice.columns, display_slice.names,
            display_slice.source_rows, display_slice.groups);
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
        page.diagnostics.insert(
            page.diagnostics.end(),
            analysis_result.diagnostics.begin(),
            analysis_result.diagnostics.end());
        PlotSpec plot = base_plot(PlotKind::matrix, title, "变量", "变量");
        plot.subtitle = subtitle;
        plot.matrix_labels = result.labels;
        plot.matrix_values = result.columns;
        plot.source_rows = result.source_rows;
        plot.point_groups = result.groups;
        page.plots.push_back(std::move(plot));
        return display_slice.source_rows.size();
    };

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "矩阵图";
        page.method_name = "Matrix Plot";
        const std::size_t display_n =
            push_matrix_plot(page, analysis_assembled, "矩阵图", "");
        page.parameter_summary = "变量数 = " + count_text(analysis_assembled.names.size())
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_assembled.source_rows.size())
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "matrix", table, configuration, display_n,
                                analysis_assembled.source_rows.size());
        return page;
    }

    const auto facet_labels = facet_labels_for_rows(
        table, analysis_assembled.source_rows, *graph.facet_column);
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "矩阵图（分面）";
    page.method_name = "Faceted Matrix Plot";
    std::size_t display_n = 0;
    for (const auto& panel : partition.panels) {
        display_n += push_matrix_plot(
            page,
            slice_matrix_columns(analysis_assembled, panel.member_indices),
            "矩阵图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size()));
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "变量数 = " + count_text(analysis_assembled.names.size())
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_assembled.source_rows.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "matrix", table, configuration, display_n,
                            analysis_assembled.source_rows.size());
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::marginal(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.x_column.has_value() || !graph.y_column.has_value()) {
        return error_page("边际图", "请选择 X 变量和 Y 变量。");
    }
    const AssembledGraphColumns analysis_columns = xy_columns(table, configuration);

    const auto push_marginal_plot = [&](OutputPage& page,
                                        const AssembledGraphColumns& analysis_slice,
                                        const std::string& title,
                                        const std::string& subtitle) {
        AssembledGraphColumns display_slice =
            filter_hidden_from_columns(analysis_slice, configuration.hidden_rows);
        const auto analysis_result = domain::statistics::marginal_plot(
            analysis_slice.first, analysis_slice.second, analysis_slice.source_rows,
            graph.bin_count);
        const auto result = domain::statistics::marginal_plot(
            display_slice.first, display_slice.second, display_slice.source_rows,
            graph.bin_count);
        page.diagnostics.insert(
            page.diagnostics.end(),
            result.points.diagnostics.begin(),
            result.points.diagnostics.end());
        page.diagnostics.insert(
            page.diagnostics.end(),
            analysis_result.points.diagnostics.begin(),
            analysis_result.points.diagnostics.end());
        PlotSpec plot = base_plot(
            PlotKind::marginal, title,
            label(table, *graph.x_column), label(table, *graph.y_column));
        plot.subtitle = subtitle;
        plot.x_values = result.points.x_values;
        plot.values = result.points.y_values;
        plot.source_rows = result.points.source_rows;
        plot.histogram_edges = result.x_histogram.edges;
        plot.histogram_counts = result.x_histogram.counts;
        plot.histogram_edges_y = result.y_histogram.edges;
        plot.histogram_counts_y = result.y_histogram.counts;
        page.plots.push_back(std::move(plot));
        return std::make_pair(
            result.points.x_values.size(), analysis_result.points.x_values.size());
    };

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "边际图";
        page.method_name = "Marginal Plot";
        const auto counts = push_marginal_plot(page, analysis_columns, "边际图", "");
        page.parameter_summary = "X = " + label(table, *graph.x_column)
            + "    Y = " + label(table, *graph.y_column)
            + "    显示 N = " + count_text(counts.first)
            + "    分析 N = " + count_text(counts.second)
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "marginal", table, configuration, counts.first,
                                counts.second);
        return page;
    }

    const auto facet_labels = facet_labels_for_rows(
        table, analysis_columns.source_rows, *graph.facet_column);
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "边际图（分面）";
    page.method_name = "Faceted Marginal Plot";
    std::size_t display_n = 0;
    for (const auto& panel : partition.panels) {
        const auto counts = push_marginal_plot(
            page,
            slice_graph_columns(analysis_columns, panel.member_indices),
            "边际图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size()));
        display_n += counts.first;
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "X = " + label(table, *graph.x_column)
        + "    Y = " + label(table, *graph.y_column)
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_columns.source_rows.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "marginal", table, configuration, display_n,
                            analysis_columns.source_rows.size());
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::parallel(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (graph.variable_columns.size() < 2) {
        return error_page("平行坐标图", "请至少选择两个连续变量。");
    }
    const AssembledMatrixColumns analysis_assembled = assemble_numeric_matrix(
        table, graph.variable_columns, graph.by_column, configuration.excluded_rows, false);

    const auto push_parallel_plot = [&](OutputPage& page,
                                        const AssembledMatrixColumns& analysis_slice,
                                        const std::string& title,
                                        const std::string& subtitle) {
        AssembledMatrixColumns display_slice =
            filter_hidden_from_matrix(analysis_slice, configuration.hidden_rows);
        const auto analysis_result = domain::statistics::parallel_plot(
            analysis_slice.columns, analysis_slice.names,
            analysis_slice.source_rows, analysis_slice.groups);
        const auto result = domain::statistics::parallel_plot(
            display_slice.columns, display_slice.names,
            display_slice.source_rows, display_slice.groups);
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
        page.diagnostics.insert(
            page.diagnostics.end(),
            analysis_result.diagnostics.begin(),
            analysis_result.diagnostics.end());
        PlotSpec plot = base_plot(PlotKind::parallel, title, "变量", "标准化值");
        plot.subtitle = subtitle;
        plot.matrix_labels = result.labels;
        plot.matrix_values = result.rows;
        plot.lower = result.minima;
        plot.upper = result.maxima;
        plot.source_rows = result.source_rows;
        plot.point_groups = result.groups;
        page.plots.push_back(std::move(plot));
        return result.rows.size();
    };

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "平行坐标图";
        page.method_name = "Parallel Coordinates Plot";
        const std::size_t display_n =
            push_parallel_plot(page, analysis_assembled, "平行坐标图", "");
        page.parameter_summary = "变量数 = " + count_text(analysis_assembled.names.size())
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_assembled.source_rows.size())
            + "    坐标已按各变量最小-最大范围标准化"
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "parallel", table, configuration, display_n,
                                analysis_assembled.source_rows.size());
        return page;
    }

    const auto facet_labels = facet_labels_for_rows(
        table, analysis_assembled.source_rows, *graph.facet_column);
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "平行坐标图（分面）";
    page.method_name = "Faceted Parallel Coordinates Plot";
    std::size_t display_n = 0;
    for (const auto& panel : partition.panels) {
        display_n += push_parallel_plot(
            page,
            slice_matrix_columns(analysis_assembled, panel.member_indices),
            "平行坐标图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size()));
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "变量数 = " + count_text(analysis_assembled.names.size())
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_assembled.source_rows.size())
        + "    坐标已按各变量最小-最大范围标准化"
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "parallel", table, configuration, display_n,
                            analysis_assembled.source_rows.size());
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::heatmap(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;

    const auto fill_category_members = [](PlotSpec& plot,
                                          const domain::statistics::HeatmapPlotResult& result) {
        const std::size_t ncols = result.column_labels.size();
        plot.member_source_rows.assign(result.row_labels.size() * ncols, {});
        plot.source_rows.assign(plot.member_source_rows.size(),
                                std::numeric_limits<std::size_t>::max());
        for (std::size_t row = 0; row < result.cell_source_rows.size(); ++row) {
            for (std::size_t col = 0; col < result.cell_source_rows[row].size(); ++col) {
                const std::size_t index = row * ncols + col;
                if (index >= plot.member_source_rows.size()) {
                    continue;
                }
                plot.member_source_rows[index] = result.cell_source_rows[row][col];
                if (!plot.member_source_rows[index].empty()) {
                    plot.source_rows[index] = plot.member_source_rows[index].front();
                }
            }
        }
    };

    if (graph.variable_columns.size() >= 2 && graph.color_scale != "category") {
        const AssembledMatrixColumns analysis_assembled = assemble_numeric_matrix(
            table, graph.variable_columns, {}, configuration.excluded_rows);

        const auto push_corr_heatmap = [&](OutputPage& page,
                                           const AssembledMatrixColumns& analysis_slice,
                                           const std::string& title,
                                           const std::string& subtitle) {
            AssembledMatrixColumns display_slice =
                filter_hidden_from_matrix(analysis_slice, configuration.hidden_rows);
            const auto analysis_correlation = domain::statistics::correlation_plot(
                analysis_slice.columns, analysis_slice.names, graph.correlation_method,
                graph.confidence_level);
            const auto correlation = domain::statistics::correlation_plot(
                display_slice.columns, display_slice.names, graph.correlation_method,
                graph.confidence_level);
            const auto result = domain::statistics::heatmap_from_correlation(correlation);
            page.diagnostics.insert(
                page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
            page.diagnostics.insert(
                page.diagnostics.end(),
                analysis_correlation.correlation.diagnostics.begin(),
                analysis_correlation.correlation.diagnostics.end());
            PlotSpec plot = base_plot(PlotKind::heatmap, title, "变量", "变量");
            plot.subtitle = subtitle;
            plot.matrix_labels = result.row_labels;
            plot.matrix_values = result.values;
            plot.matrix_counts = result.counts;
            plot.color_min = result.color_min;
            plot.color_max = result.color_max;
            // Correlation cells are not observation strata — do not invent member rows.
            page.plots.push_back(std::move(plot));
            return display_slice.source_rows.size();
        };

        if (!graph.facet_column.has_value()) {
            OutputPage page;
            page.configuration = configuration;
            page.title = "热图";
            page.method_name = "Heatmap";
            const std::size_t display_n =
                push_corr_heatmap(page, analysis_assembled, "热图", "");
            page.parameter_summary = "颜色范围固定为相关系数 [-1, 1]    方法 = "
                + graph.correlation_method
                + "    显示 N = " + count_text(display_n)
                + "    分析 N = " + count_text(analysis_assembled.source_rows.size())
                + "    excluded = " + count_text(configuration.excluded_rows.size())
                + "    hidden = " + count_text(configuration.hidden_rows.size());
            attach_visibility_facts(page, "heatmap", table, configuration, display_n,
                                    analysis_assembled.source_rows.size());
            return page;
        }

        const auto facet_labels = facet_labels_for_rows(
            table, analysis_assembled.source_rows, *graph.facet_column);
        const auto partition = domain::partition_facet_levels(
            facet_labels, graph.facet_max_panels);
        OutputPage page;
        page.configuration = configuration;
        page.title = "热图（分面）";
        page.method_name = "Faceted Correlation Heatmap";
        std::size_t display_n = 0;
        for (const auto& panel : partition.panels) {
            display_n += push_corr_heatmap(
                page,
                slice_matrix_columns(analysis_assembled, panel.member_indices),
                "热图 · " + panel.level,
                "facet = " + panel.level
                    + "    分析 N(水平) = " + count_text(panel.member_indices.size())
                    + "    相关矩阵单元格不是观测层");
        }
        page.diagnostics.insert(
            page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
        page.parameter_summary = "颜色范围固定为相关系数 [-1, 1]    方法 = "
            + graph.correlation_method
            + "    分面 = " + label(table, *graph.facet_column)
            + "    面板 = " + count_text(page.plots.size())
            + "/" + count_text(partition.level_count)
            + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_assembled.source_rows.size())
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "heatmap", table, configuration, display_n,
                                analysis_assembled.source_rows.size());
        apply_facet_facts(page, partition, graph.facet_max_panels);
        return page;
    }

    if (!graph.x_column.has_value() || !graph.y_column.has_value()
        || !graph.z_column.has_value()) {
        return error_page("热图", "请选择多个连续变量，或行类别、列类别和数值。");
    }

    struct CategoryObs {
        std::string row_label;
        std::string col_label;
        double value = 0.0;
        std::size_t source_row = 0;
        std::string facet;
    };
    const auto excluded = to_row_set(configuration.excluded_rows);
    std::vector<CategoryObs> analysis_obs;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (excluded.count(row) != 0) {
            continue;
        }
        if (*graph.x_column >= table.rows[row].size()
            || *graph.y_column >= table.rows[row].size()
            || *graph.z_column >= table.rows[row].size()) {
            continue;
        }
        double value = 0.0;
        if (!parse_finite_number(table.rows[row][*graph.z_column], value)) {
            continue;
        }
        CategoryObs obs;
        obs.row_label = table.rows[row][*graph.y_column];
        obs.col_label = table.rows[row][*graph.x_column];
        obs.value = value;
        obs.source_row = row;
        if (graph.facet_column.has_value()
            && *graph.facet_column < table.rows[row].size()) {
            obs.facet = table.rows[row][*graph.facet_column];
        }
        analysis_obs.push_back(std::move(obs));
    }

    const auto push_category_heatmap = [&](OutputPage& page,
                                           const std::vector<CategoryObs>& panel_obs,
                                           const std::string& title,
                                           const std::string& subtitle) {
        const auto hidden = to_row_set(configuration.hidden_rows);
        std::vector<std::string> analysis_rows;
        std::vector<std::string> analysis_columns;
        std::vector<double> analysis_values;
        std::vector<std::size_t> analysis_source_rows;
        std::vector<std::string> display_rows;
        std::vector<std::string> display_columns;
        std::vector<double> display_values;
        std::vector<std::size_t> display_source_rows;
        for (const auto& obs : panel_obs) {
            analysis_rows.push_back(obs.row_label);
            analysis_columns.push_back(obs.col_label);
            analysis_values.push_back(obs.value);
            analysis_source_rows.push_back(obs.source_row);
            if (hidden.count(obs.source_row) == 0) {
                display_rows.push_back(obs.row_label);
                display_columns.push_back(obs.col_label);
                display_values.push_back(obs.value);
                display_source_rows.push_back(obs.source_row);
            }
        }
        const auto analysis_result = domain::statistics::heatmap_from_categories(
            analysis_rows, analysis_columns, analysis_values, analysis_source_rows);
        const auto result = domain::statistics::heatmap_from_categories(
            display_rows, display_columns, display_values, display_source_rows);
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
        page.diagnostics.insert(
            page.diagnostics.end(),
            analysis_result.diagnostics.begin(),
            analysis_result.diagnostics.end());
        PlotSpec plot = base_plot(
            PlotKind::heatmap, title,
            label(table, *graph.x_column), label(table, *graph.y_column));
        plot.subtitle = subtitle;
        plot.matrix_labels = result.column_labels;
        plot.categories = result.row_labels;
        plot.matrix_values = result.values;
        plot.matrix_counts = result.counts;
        plot.color_min = result.color_min;
        plot.color_max = result.color_max;
        fill_category_members(plot, result);
        page.plots.push_back(std::move(plot));
        return display_source_rows.size();
    };

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "热图";
        page.method_name = "Heatmap";
        const std::size_t display_n =
            push_category_heatmap(page, analysis_obs, "热图", "");
        page.parameter_summary = "单元格为组内均值"
            + std::string("    显示 N = ") + count_text(display_n)
            + "    分析 N = " + count_text(analysis_obs.size())
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "heatmap", table, configuration, display_n,
                                analysis_obs.size());
        return page;
    }

    std::vector<std::string> facet_labels;
    facet_labels.reserve(analysis_obs.size());
    for (const auto& obs : analysis_obs) {
        facet_labels.push_back(obs.facet);
    }
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "热图（分面）";
    page.method_name = "Faceted Category Heatmap";
    std::size_t display_n = 0;
    for (const auto& panel : partition.panels) {
        std::vector<CategoryObs> panel_obs;
        for (const std::size_t index : panel.member_indices) {
            if (index < analysis_obs.size()) {
                panel_obs.push_back(analysis_obs[index]);
            }
        }
        display_n += push_category_heatmap(
            page,
            panel_obs,
            "热图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size()));
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "单元格为组内均值"
        + std::string("    分面 = ") + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_obs.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "heatmap", table, configuration, display_n,
                            analysis_obs.size());
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::time_series(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.y_column.has_value()) {
        return error_page("时间序列图", "请选择数值变量。");
    }
    const std::size_t time_column = graph.time_column.value_or(
        graph.x_column.value_or(*graph.y_column));
    const AssembledGraphColumns analysis_columns = assemble_graph_columns(
        table, time_column, graph.y_column, {}, graph.by_column, graph.label_column,
        configuration.excluded_rows, true);

    const auto push_ts_plot = [&](OutputPage& page,
                                  const AssembledGraphColumns& analysis_slice,
                                  const std::string& title,
                                  const std::string& subtitle) {
        AssembledGraphColumns display_slice =
            filter_hidden_from_columns(analysis_slice, configuration.hidden_rows);
        const auto analysis_result = domain::statistics::time_series_plot(
            analysis_slice.first, analysis_slice.second, analysis_slice.source_rows,
            analysis_slice.categories, analysis_slice.groups);
        const auto result = domain::statistics::time_series_plot(
            display_slice.first, display_slice.second, display_slice.source_rows,
            display_slice.categories, display_slice.groups);
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
        page.diagnostics.insert(
            page.diagnostics.end(),
            analysis_result.diagnostics.begin(),
            analysis_result.diagnostics.end());
        PlotSpec plot = base_plot(
            PlotKind::time_series, title,
            label(table, time_column), label(table, *graph.y_column));
        plot.subtitle = subtitle;
        plot.x_values = result.x_values;
        plot.values = result.y_values;
        plot.source_rows = result.source_rows;
        plot.point_groups = result.groups;
        plot.point_labels = result.time_labels;
        if (!graph.connect_missing) {
            plot.value_style.line_width = 0.0;
        }
        page.plots.push_back(std::move(plot));
        return result.x_values.size();
    };

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "时间序列图";
        page.method_name = "Time Series Plot";
        const std::size_t display_n =
            push_ts_plot(page, analysis_columns, "时间序列图", "");
        page.parameter_summary = "时间 = " + label(table, time_column)
            + "    数值 = " + label(table, *graph.y_column)
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_columns.source_rows.size())
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "time_series", table, configuration, display_n,
                                analysis_columns.source_rows.size());
        return page;
    }

    const auto facet_labels = facet_labels_for_rows(
        table, analysis_columns.source_rows, *graph.facet_column);
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "时间序列图（分面）";
    page.method_name = "Faceted Time Series Plot";
    std::size_t display_n = 0;
    for (const auto& panel : partition.panels) {
        AssembledGraphColumns slice;
        for (const std::size_t index : panel.member_indices) {
            if (index >= analysis_columns.source_rows.size()) {
                continue;
            }
            if (index < analysis_columns.first.size()) {
                slice.first.push_back(analysis_columns.first[index]);
            }
            if (index < analysis_columns.second.size()) {
                slice.second.push_back(analysis_columns.second[index]);
            }
            if (index < analysis_columns.groups.size()) {
                slice.groups.push_back(analysis_columns.groups[index]);
            }
            if (index < analysis_columns.categories.size()) {
                slice.categories.push_back(analysis_columns.categories[index]);
            }
            if (index < analysis_columns.labels.size()) {
                slice.labels.push_back(analysis_columns.labels[index]);
            }
            slice.source_rows.push_back(analysis_columns.source_rows[index]);
        }
        display_n += push_ts_plot(
            page,
            slice,
            "时间序列图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size()));
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "时间 = " + label(table, time_column)
        + "    数值 = " + label(table, *graph.y_column)
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_columns.source_rows.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "time_series", table, configuration, display_n,
                            analysis_columns.source_rows.size());
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::area(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    OutputPage page = time_series(table, configuration);
    if (page.plots.empty()) {
        return page;
    }
    const bool faceted = configuration.graph.facet_column.has_value();
    page.title = faceted ? "区域图（分面）" : "面积区域图";
    page.method_name = faceted ? "Faceted Area Plot" : "Area Plot";
    for (auto& plot : page.plots) {
        plot.kind = PlotKind::area;
        const std::string from = "时间序列图";
        const std::string to = faceted ? "区域图" : "面积区域图";
        const auto pos = plot.title.find(from);
        if (pos != std::string::npos) {
            plot.title.replace(pos, from.size(), to);
        }
    }
    page.parameter_summary += "    面积表示相邻观测之间的数值区间，不是置信区间";
    if (page.facts.eda.has_value()) {
        page.facts.eda->kind = "area";
    }
    return page;
}

OutputPage GraphService::contour(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.x_column.has_value() || !graph.y_column.has_value()
        || !graph.z_column.has_value()) {
        return error_page("等值线图", "请选择 X、Y 和 Z 变量。");
    }
    const AssembledGraphColumns analysis_columns = assemble_graph_columns(
        table, *graph.x_column, graph.y_column, graph.z_column, {}, {},
        configuration.excluded_rows, true);

    const auto push_contour_plot = [&](OutputPage& page,
                                       const AssembledGraphColumns& analysis_slice,
                                       const std::string& title,
                                       const std::string& subtitle) {
        AssembledGraphColumns display_slice =
            filter_hidden_from_columns(analysis_slice, configuration.hidden_rows);
        const auto analysis_result = domain::statistics::contour_plot(
            analysis_slice.first, analysis_slice.second, analysis_slice.third,
            graph.contour_levels);
        const auto result = domain::statistics::contour_plot(
            display_slice.first, display_slice.second, display_slice.third,
            graph.contour_levels);
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
        page.diagnostics.insert(
            page.diagnostics.end(),
            analysis_result.diagnostics.begin(),
            analysis_result.diagnostics.end());
        PlotSpec plot = base_plot(
            PlotKind::contour, title,
            label(table, *graph.x_column), label(table, *graph.y_column));
        plot.subtitle = subtitle;
        plot.contour_x = result.x;
        plot.contour_y = result.y;
        plot.contour_levels = result.levels;
        plot.matrix_values = result.z;
        if (!result.levels.empty()) {
            plot.color_min = result.levels.front();
            plot.color_max = result.levels.back();
        }
        page.plots.push_back(std::move(plot));
        return display_slice.source_rows.size();
    };

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "等值线图";
        page.method_name = "Contour Plot";
        const std::size_t display_n =
            push_contour_plot(page, analysis_columns, "等值线图", "");
        page.parameter_summary = "X = " + label(table, *graph.x_column)
            + "    Y = " + label(table, *graph.y_column)
            + "    Z = " + label(table, *graph.z_column)
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_columns.source_rows.size())
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "contour", table, configuration, display_n,
                                analysis_columns.source_rows.size());
        return page;
    }

    const auto facet_labels = facet_labels_for_rows(
        table, analysis_columns.source_rows, *graph.facet_column);
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "等值线图（分面）";
    page.method_name = "Faceted Contour Plot";
    std::size_t display_n = 0;
    for (const auto& panel : partition.panels) {
        AssembledGraphColumns slice;
        for (const std::size_t index : panel.member_indices) {
            if (index >= analysis_columns.source_rows.size()) {
                continue;
            }
            if (index < analysis_columns.first.size()) {
                slice.first.push_back(analysis_columns.first[index]);
            }
            if (index < analysis_columns.second.size()) {
                slice.second.push_back(analysis_columns.second[index]);
            }
            if (index < analysis_columns.third.size()) {
                slice.third.push_back(analysis_columns.third[index]);
            }
            slice.source_rows.push_back(analysis_columns.source_rows[index]);
        }
        display_n += push_contour_plot(
            page,
            slice,
            "等值线图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size()));
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "X = " + label(table, *graph.x_column)
        + "    Y = " + label(table, *graph.y_column)
        + "    Z = " + label(table, *graph.z_column)
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_columns.source_rows.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "contour", table, configuration, display_n,
                            analysis_columns.source_rows.size());
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::pie(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.x_column.has_value()) {
        return error_page("饼图", "请选择分类变量。");
    }
    const auto excluded = to_row_set(configuration.excluded_rows);
    const auto hidden = to_row_set(configuration.hidden_rows);
    std::vector<std::string> analysis_categories;
    std::vector<double> analysis_weights;
    std::vector<std::string> display_categories;
    std::vector<double> display_weights;
    std::vector<std::size_t> display_source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (excluded.count(row) != 0) {
            continue;
        }
        if (*graph.x_column >= table.rows[row].size()) {
            continue;
        }
        double weight = 1.0;
        if (graph.weight_column.has_value()) {
            if (*graph.weight_column >= table.rows[row].size()
                || !parse_finite_number(table.rows[row][*graph.weight_column], weight)) {
                continue;
            }
        }
        const std::string category = table.rows[row][*graph.x_column];
        analysis_categories.push_back(category);
        analysis_weights.push_back(weight);
        if (hidden.count(row) == 0) {
            display_categories.push_back(category);
            display_weights.push_back(weight);
            display_source_rows.push_back(row);
        }
    }
    const auto analysis_pie = domain::statistics::pie_plot(
        analysis_categories, analysis_weights, graph.other_threshold_percent);
    const auto result = domain::statistics::pie_plot(
        display_categories, display_weights, graph.other_threshold_percent,
        display_source_rows);
    OutputPage page;
    page.configuration = configuration;
    page.title = "饼图";
    page.method_name = "Pie Chart";
    page.parameter_summary = "类别 = " + label(table, *graph.x_column)
        + "    小类别合并阈值 = " + std::to_string(graph.other_threshold_percent) + "%"
        + "    显示 N = " + count_text(display_categories.size())
        + "    分析 N = " + count_text(analysis_categories.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    page.diagnostics = result.diagnostics;
    page.diagnostics.insert(
        page.diagnostics.end(), analysis_pie.diagnostics.begin(), analysis_pie.diagnostics.end());
    PlotSpec plot = base_plot(PlotKind::pie, "饼图", "", "组成比例");
    plot.categories = result.labels;
    plot.category_values = result.values;
    plot.cumulative_percent = result.percents;
    plot.member_source_rows = result.member_source_rows;
    plot.source_rows.assign(
        plot.member_source_rows.size(), std::numeric_limits<std::size_t>::max());
    for (std::size_t i = 0; i < plot.member_source_rows.size(); ++i) {
        if (!plot.member_source_rows[i].empty()) {
            plot.source_rows[i] = plot.member_source_rows[i].front();
        }
    }
    page.plots.push_back(std::move(plot));
    attach_visibility_facts(page, "pie", table, configuration, display_categories.size(),
                            analysis_categories.size(), analysis_pie.labels.size());
    if (page.facts.eda.has_value()) {
        page.facts.eda->category_count = result.labels.size();
        page.facts.eda->has_cumulative_percent = true;
    }
    return page;
}

OutputPage GraphService::density(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.x_column.has_value()) {
        return error_page("密度图", "请选择连续变量。");
    }
    // Analysis assembly: excluded only. Display curve omits hidden.
    const AssembledGraphColumns analysis_columns = assemble_graph_columns(
        table, *graph.x_column, {}, {}, {}, {}, configuration.excluded_rows, true);
    const AssembledGraphColumns display_columns =
        filter_hidden_from_columns(analysis_columns, configuration.hidden_rows);
    const auto analysis_kde = domain::statistics::gaussian_kde(analysis_columns.first);

    const auto push_density_plot = [&](OutputPage& page,
                                       const std::string& title,
                                       const std::string& subtitle,
                                       const std::vector<double>& values,
                                       double* bandwidth_out) -> std::size_t {
        const auto result = domain::statistics::gaussian_kde(values);
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
        PlotSpec plot = base_plot(PlotKind::density, title,
                                  label(table, *graph.x_column), "密度");
        plot.subtitle = subtitle;
        plot.x_values = result.x;
        plot.values = result.density;
        // KDE grid points are not discrete observation marks — do not map
        // source_rows / member_source_rows by curve index (would invent linkage).
        plot.source_rows.clear();
        plot.member_source_rows.clear();
        page.plots.push_back(std::move(plot));
        if (bandwidth_out != nullptr) {
            *bandwidth_out = result.bandwidth;
        }
        return result.n;
    };

    DiagnosticMessage density_mark_note;
    density_mark_note.severity = DiagnosticMessage::Severity::info;
    density_mark_note.code = "density_curve_not_discrete_marks";
    density_mark_note.message =
        "密度曲线是连续估计网格，不是离散观测标记；"
        "点选曲线不会映射到单条工作表行（hidden/excluded 仍分别计入分析/显示 N）。";

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "密度图";
        page.method_name = "Density Plot";
        double bandwidth = 0.0;
        const std::size_t display_n = push_density_plot(
            page, "密度图", "", display_columns.first, &bandwidth);
        page.parameter_summary = "变量 = " + label(table, *graph.x_column)
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_kde.n)
            + "    h = " + std::to_string(bandwidth)
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        page.diagnostics.insert(
            page.diagnostics.end(),
            analysis_kde.diagnostics.begin(),
            analysis_kde.diagnostics.end());
        page.diagnostics.push_back(density_mark_note);
        attach_visibility_facts(page, "density", table, configuration, display_n,
                                analysis_kde.n);
        if (page.facts.eda.has_value()) {
            page.facts.eda->bandwidth = bandwidth;
        }
        return page;
    }

    const auto facet_labels = facet_labels_for_rows(
        table, analysis_columns.source_rows, *graph.facet_column);
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "密度图（分面）";
    page.method_name = "Faceted Density Plot";
    const auto hidden = to_row_set(configuration.hidden_rows);
    std::size_t display_n = 0;
    for (const auto& panel : partition.panels) {
        std::vector<double> values;
        for (const std::size_t index : panel.member_indices) {
            if (index >= analysis_columns.source_rows.size()
                || index >= analysis_columns.first.size()) {
                continue;
            }
            const std::size_t source_row = analysis_columns.source_rows[index];
            if (hidden.count(source_row) != 0) {
                continue;
            }
            values.push_back(analysis_columns.first[index]);
        }
        display_n += push_density_plot(
            page,
            "密度图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size())
                + "    显示 N = " + count_text(values.size()),
            values,
            nullptr);
    }
    page.diagnostics.insert(
        page.diagnostics.end(),
        analysis_kde.diagnostics.begin(),
        analysis_kde.diagnostics.end());
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.diagnostics.push_back(density_mark_note);
    page.parameter_summary = "变量 = " + label(table, *graph.x_column)
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_kde.n)
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "density", table, configuration, display_n, analysis_kde.n);
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::hexbin(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.x_column.has_value() || !graph.y_column.has_value()) {
        return error_page("Hexbin", "请选择 X 变量和 Y 变量。");
    }
    const AssembledGraphColumns analysis_columns = xy_columns(table, configuration);

    const auto push_hexbin_plot = [&](OutputPage& page,
                                      const AssembledGraphColumns& analysis_slice,
                                      const std::string& title,
                                      const std::string& subtitle) {
        AssembledGraphColumns display_slice =
            filter_hidden_from_columns(analysis_slice, configuration.hidden_rows);
        const auto analysis_hex = domain::statistics::hexbin_rectangular(
            analysis_slice.first, analysis_slice.second, analysis_slice.source_rows,
            graph.bin_count);
        const auto result = domain::statistics::hexbin_rectangular(
            display_slice.first, display_slice.second, display_slice.source_rows,
            graph.bin_count);
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
        page.diagnostics.insert(
            page.diagnostics.end(),
            analysis_hex.diagnostics.begin(),
            analysis_hex.diagnostics.end());
        PlotSpec plot = base_plot(
            PlotKind::hexbin, title,
            label(table, *graph.x_column), label(table, *graph.y_column));
        plot.subtitle = subtitle;
        plot.contour_x = result.x_edges;
        plot.contour_y = result.y_edges;
        plot.matrix_values = result.counts;
        plot.color_min = 0.0;
        plot.color_max = result.max_count;
        const std::size_t ncols = result.x_bins;
        plot.member_source_rows.assign(result.y_bins * ncols, {});
        plot.source_rows.assign(plot.member_source_rows.size(),
                                std::numeric_limits<std::size_t>::max());
        for (std::size_t row = 0; row < result.cell_source_rows.size(); ++row) {
            for (std::size_t col = 0; col < result.cell_source_rows[row].size(); ++col) {
                const std::size_t index = row * ncols + col;
                if (index >= plot.member_source_rows.size()) {
                    continue;
                }
                plot.member_source_rows[index] = result.cell_source_rows[row][col];
                if (!plot.member_source_rows[index].empty()) {
                    plot.source_rows[index] = plot.member_source_rows[index].front();
                }
            }
        }
        page.plots.push_back(std::move(plot));
        return std::make_pair(result.n, analysis_hex.n);
    };

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "Hexbin / 二维分箱";
        page.method_name = "Hexbin Plot";
        const auto counts = push_hexbin_plot(page, analysis_columns, "二维分箱散点", "");
        page.parameter_summary = "X = " + label(table, *graph.x_column)
            + "    Y = " + label(table, *graph.y_column)
            + "    显示 N = " + count_text(counts.first)
            + "    分析 N = " + count_text(counts.second)
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "hexbin", table, configuration, counts.first,
                                counts.second);
        if (page.facts.eda.has_value() && !page.plots.empty()) {
            page.facts.eda->x_bins =
                page.plots.front().contour_x.empty()
                    ? 0
                    : page.plots.front().contour_x.size() - 1;
            page.facts.eda->y_bins =
                page.plots.front().contour_y.empty()
                    ? 0
                    : page.plots.front().contour_y.size() - 1;
        }
        return page;
    }

    const auto facet_labels = facet_labels_for_rows(
        table, analysis_columns.source_rows, *graph.facet_column);
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "Hexbin（分面）";
    page.method_name = "Faceted Hexbin Plot";
    std::size_t display_n = 0;
    for (const auto& panel : partition.panels) {
        AssembledGraphColumns slice;
        for (const std::size_t index : panel.member_indices) {
            if (index >= analysis_columns.source_rows.size()) {
                continue;
            }
            if (index < analysis_columns.first.size()) {
                slice.first.push_back(analysis_columns.first[index]);
            }
            if (index < analysis_columns.second.size()) {
                slice.second.push_back(analysis_columns.second[index]);
            }
            slice.source_rows.push_back(analysis_columns.source_rows[index]);
        }
        const auto counts = push_hexbin_plot(
            page,
            slice,
            "二维分箱 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size()));
        display_n += counts.first;
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "X = " + label(table, *graph.x_column)
        + "    Y = " + label(table, *graph.y_column)
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_columns.first.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "hexbin", table, configuration, display_n,
                            analysis_columns.first.size());
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::violin(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.y_column.has_value()) {
        return error_page("小提琴图", "请选择响应变量。");
    }
    const AssembledGraphColumns analysis_columns = assemble_graph_columns(
        table, *graph.y_column, {}, {}, graph.by_column, {}, configuration.excluded_rows, true);

    const auto push_violin_plot = [&](OutputPage& page,
                                      const AssembledGraphColumns& analysis_slice,
                                      const std::string& title,
                                      const std::string& subtitle) {
        AssembledGraphColumns display_slice =
            filter_hidden_from_columns(analysis_slice, configuration.hidden_rows);
        const auto analysis_violin = domain::statistics::violin_plot(
            analysis_slice.first, analysis_slice.groups, analysis_slice.source_rows);
        const auto result = domain::statistics::violin_plot(
            display_slice.first, display_slice.groups, display_slice.source_rows);
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
        page.diagnostics.insert(
            page.diagnostics.end(),
            analysis_violin.diagnostics.begin(),
            analysis_violin.diagnostics.end());
        PlotSpec plot = base_plot(
            PlotKind::violin, title, "分组", label(table, *graph.y_column));
        plot.subtitle = subtitle;
        for (const auto& group : result.groups) {
            plot.box_labels.push_back(group.label);
            plot.box_min.push_back(group.whisker_low);
            plot.box_q1.push_back(group.q1);
            plot.box_median.push_back(group.median);
            plot.box_q3.push_back(group.q3);
            plot.box_max.push_back(group.whisker_high);
            if (!group.source_rows.empty()) {
                plot.source_rows.push_back(group.source_rows.front());
            }
            plot.member_source_rows.push_back(group.source_rows);
            domain::PlotSeries density;
            density.label = group.label;
            density.x_values = group.density_values;
            density.values = group.density_y;
            plot.series.push_back(std::move(density));
        }
        page.plots.push_back(std::move(plot));
        return std::make_pair(
            display_slice.first.size(),
            std::make_pair(analysis_violin.groups.size(), result.bandwidth));
    };

    if (!graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "小提琴图";
        page.method_name = "Violin Plot";
        const auto pushed = push_violin_plot(page, analysis_columns, "小提琴图", "");
        page.parameter_summary = "响应 = " + label(table, *graph.y_column)
            + (graph.by_column.has_value()
                   ? "    分组 = " + label(table, *graph.by_column) : "")
            + "    显示组数 = " + std::to_string(pushed.second.first)
            + "    显示 N = " + count_text(pushed.first)
            + "    分析 N = " + count_text(analysis_columns.first.size())
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "violin", table, configuration, pushed.first,
                                analysis_columns.first.size(), pushed.second.first);
        if (page.facts.eda.has_value()) {
            page.facts.eda->bandwidth = pushed.second.second;
            page.facts.eda->category_count = pushed.second.first;
        }
        return page;
    }

    const auto facet_labels = facet_labels_for_rows(
        table, analysis_columns.source_rows, *graph.facet_column);
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "小提琴图（分面）";
    page.method_name = "Faceted Violin Plot";
    std::size_t display_n = 0;
    std::size_t analysis_group_count = 0;
    for (const auto& panel : partition.panels) {
        AssembledGraphColumns slice;
        for (const std::size_t index : panel.member_indices) {
            if (index >= analysis_columns.source_rows.size()) {
                continue;
            }
            if (index < analysis_columns.first.size()) {
                slice.first.push_back(analysis_columns.first[index]);
            }
            if (index < analysis_columns.groups.size()) {
                slice.groups.push_back(analysis_columns.groups[index]);
            }
            slice.source_rows.push_back(analysis_columns.source_rows[index]);
        }
        const auto pushed = push_violin_plot(
            page,
            slice,
            "小提琴图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size()));
        display_n += pushed.first;
        analysis_group_count = std::max(analysis_group_count, pushed.second.first);
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.parameter_summary = "响应 = " + label(table, *graph.y_column)
        + (graph.by_column.has_value()
               ? "    分组 = " + label(table, *graph.by_column) : "")
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_columns.first.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "violin", table, configuration, display_n,
                            analysis_columns.first.size(), analysis_group_count);
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::bar(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.x_column.has_value()) {
        return error_page("条形图", "请选择分类变量。");
    }
    const auto excluded = to_row_set(configuration.excluded_rows);
    const auto hidden = to_row_set(configuration.hidden_rows);

    struct BarObservation {
        std::string category;
        double weight = 1.0;
        std::size_t row = 0;
        std::string facet;
    };
    std::vector<BarObservation> analysis_obs;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (excluded.count(row) != 0) {
            continue;
        }
        if (*graph.x_column >= table.rows[row].size()) {
            continue;
        }
        double weight = 1.0;
        if (graph.weight_column.has_value()) {
            if (*graph.weight_column >= table.rows[row].size()
                || !parse_finite_number(table.rows[row][*graph.weight_column], weight)) {
                continue;
            }
        }
        BarObservation obs;
        obs.category = table.rows[row][*graph.x_column];
        obs.weight = weight;
        obs.row = row;
        if (graph.facet_column.has_value()
            && *graph.facet_column < table.rows[row].size()) {
            obs.facet = table.rows[row][*graph.facet_column];
        }
        analysis_obs.push_back(std::move(obs));
    }

    const auto build_bar_plot = [&](const std::vector<BarObservation>& panel_obs,
                                    const std::string& title,
                                    const std::string& subtitle,
                                    PlotSpec* out_plot,
                                    std::size_t* display_n,
                                    std::size_t* display_category_count,
                                    std::vector<DiagnosticMessage>* diagnostics) {
        std::vector<std::string> analysis_categories;
        std::vector<double> analysis_weights;
        std::vector<std::string> display_categories;
        std::vector<double> display_weights;
        std::vector<std::size_t> display_source_rows;
        for (const auto& obs : panel_obs) {
            analysis_categories.push_back(obs.category);
            analysis_weights.push_back(obs.weight);
            if (hidden.count(obs.row) == 0) {
                display_categories.push_back(obs.category);
                display_weights.push_back(obs.weight);
                display_source_rows.push_back(obs.row);
            }
        }
        const auto analysis_result = domain::statistics::bar_chart_counts(
            analysis_categories,
            graph.weight_column.has_value() ? &analysis_weights : nullptr);
        const auto display_result = domain::statistics::bar_chart_counts(
            display_categories,
            graph.weight_column.has_value() ? &display_weights : nullptr);
        if (diagnostics != nullptr) {
            diagnostics->insert(
                diagnostics->end(),
                display_result.diagnostics.begin(),
                display_result.diagnostics.end());
            diagnostics->insert(
                diagnostics->end(),
                analysis_result.diagnostics.begin(),
                analysis_result.diagnostics.end());
        }
        PlotSpec plot = base_plot(PlotKind::bar, title, "类别", "计数");
        plot.subtitle = subtitle;
        plot.categories = display_result.categories;
        plot.values = display_result.values;
        plot.member_source_rows.assign(display_result.categories.size(), {});
        for (std::size_t index = 0; index < display_categories.size(); ++index) {
            const auto found = std::find(display_result.categories.begin(),
                                         display_result.categories.end(),
                                         display_categories[index]);
            if (found == display_result.categories.end()) {
                continue;
            }
            const std::size_t category_index = static_cast<std::size_t>(
                std::distance(display_result.categories.begin(), found));
            plot.member_source_rows[category_index].push_back(display_source_rows[index]);
        }
        for (const auto& members : plot.member_source_rows) {
            if (!members.empty()) {
                plot.source_rows.push_back(members.front());
            }
        }
        if (out_plot != nullptr) {
            *out_plot = std::move(plot);
        }
        if (display_n != nullptr) {
            *display_n = display_categories.size();
        }
        if (display_category_count != nullptr) {
            *display_category_count = display_result.categories.size();
        }
        return analysis_result.categories.size();
    };

    DiagnosticMessage bar_visibility_note{
        DiagnosticMessage::Severity::info, "bar_hidden_excluded_distinct",
        "条形图显示计数省略 hidden；分析口径计数保留 hidden、仅省略 excluded。"
        "两者不得合并为一个 bool。"};

    if (!graph.facet_column.has_value()) {
        PlotSpec plot;
        std::size_t display_n = 0;
        std::size_t display_category_count = 0;
        std::vector<DiagnosticMessage> diagnostics;
        const std::size_t analysis_category_count = build_bar_plot(
            analysis_obs, "条形图", "", &plot, &display_n, &display_category_count,
            &diagnostics);
        OutputPage page;
        page.configuration = configuration;
        page.title = "条形图";
        page.method_name = "Bar Chart";
        page.parameter_summary = "类别 = " + label(table, *graph.x_column)
            + "    显示类别数 = " + std::to_string(display_category_count)
            + "    分析类别数 = " + std::to_string(analysis_category_count)
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        page.diagnostics = std::move(diagnostics);
        page.diagnostics.push_back(bar_visibility_note);
        page.plots.push_back(std::move(plot));
        domain::EdaPlotFacts facts;
        facts.kind = "bar";
        facts.n = display_n;
        facts.category_count = display_category_count;
        facts.sorted_by_count = false;
        const auto visibility = summarize_row_visibility(
            table.rows.size(), configuration.hidden_rows, configuration.excluded_rows);
        page.diagnostics.insert(page.diagnostics.end(),
                                visibility.diagnostics.begin(), visibility.diagnostics.end());
        facts.hidden_count = visibility.hidden_count;
        facts.excluded_count = visibility.excluded_count;
        facts.analysis_eligible_n = visibility.analysis_eligible_count;
        facts.display_eligible_n = visibility.display_eligible_count;
        facts.hidden_excluded_distinct = true;
        facts.analysis_n = analysis_obs.size();
        facts.analysis_category_count = analysis_category_count;
        page.facts.eda = std::move(facts);
        return page;
    }

    std::vector<std::string> facet_labels;
    facet_labels.reserve(analysis_obs.size());
    for (const auto& obs : analysis_obs) {
        facet_labels.push_back(obs.facet);
    }
    const auto partition = domain::partition_facet_levels(
        facet_labels, graph.facet_max_panels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "条形图（分面）";
    page.method_name = "Faceted Bar Chart";
    std::size_t display_n = 0;
    std::size_t analysis_category_count = 0;
    for (const auto& panel : partition.panels) {
        std::vector<BarObservation> panel_obs;
        for (const std::size_t index : panel.member_indices) {
            if (index < analysis_obs.size()) {
                panel_obs.push_back(analysis_obs[index]);
            }
        }
        PlotSpec plot;
        std::size_t panel_display_n = 0;
        std::size_t panel_display_cats = 0;
        const std::size_t panel_analysis_cats = build_bar_plot(
            panel_obs,
            "条形图 · " + panel.level,
            "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel.member_indices.size()),
            &plot,
            &panel_display_n,
            &panel_display_cats,
            &page.diagnostics);
        display_n += panel_display_n;
        analysis_category_count = std::max(analysis_category_count, panel_analysis_cats);
        page.plots.push_back(std::move(plot));
        (void)panel_display_cats;
    }
    page.diagnostics.insert(
        page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
    page.diagnostics.push_back(bar_visibility_note);
    page.parameter_summary = "类别 = " + label(table, *graph.x_column)
        + "    分面 = " + label(table, *graph.facet_column)
        + "    面板 = " + count_text(page.plots.size())
        + "/" + count_text(partition.level_count)
        + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(analysis_obs.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "bar", table, configuration, display_n, analysis_obs.size(),
                            analysis_category_count);
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

}  // namespace datalab::application
