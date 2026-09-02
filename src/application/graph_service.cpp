#include "application/graph_service.h"
#include "application/analysis_service.h"
#include "application/computation_trace_attach.h"

#include "domain/column_extract.h"
#include "domain/graph_assembly.h"
#include "domain/row_visibility.h"
#include "domain/statistics/graph_visuals.h"
#include "domain/statistics/eda_plots.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <unordered_set>

namespace datalab::application {
namespace {

using datalab::domain::AnalysisConfiguration;
using datalab::domain::GraphConfiguration;
using datalab::domain::GraphGalleryFacts;
using datalab::domain::ChiSquareMosaicLinkFacts;
using datalab::domain::ChiSquareMosaicLinkConfiguration;
using datalab::domain::AssembledGraphColumns;
using datalab::domain::AssembledMatrixColumns;
using datalab::domain::DataTable;
using datalab::domain::DiagnosticMessage;
using datalab::domain::EdaPlotFacts;
using datalab::domain::OutputPage;
using datalab::domain::PlotKind;
using datalab::domain::PlotSpec;
using datalab::domain::assemble_graph_columns;
using datalab::domain::assemble_time_series_columns;
using datalab::domain::assemble_numeric_matrix;
using datalab::domain::extract_numeric_column;
using datalab::domain::extract_text_column;
using datalab::domain::ExtractedNumericColumn;
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

std::vector<std::size_t> panel_source_rows_from_member_indices(
    const AssembledGraphColumns& facet_base,
    const std::vector<std::size_t>& member_indices)
{
    std::vector<std::size_t> panel_source_rows;
    panel_source_rows.reserve(member_indices.size());
    for (const std::size_t index : member_indices) {
        if (index < facet_base.source_rows.size()) {
            panel_source_rows.push_back(facet_base.source_rows[index]);
        }
    }
    return panel_source_rows;
}

AssembledGraphColumns slice_graph_columns_by_source_rows(
    const AssembledGraphColumns& columns,
    const std::vector<std::size_t>& panel_source_rows)
{
    AssembledGraphColumns slice;
    for (const std::size_t source_row : panel_source_rows) {
        for (std::size_t index = 0; index < columns.source_rows.size(); ++index) {
            if (columns.source_rows[index] != source_row) {
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
            slice.source_rows.push_back(source_row);
            break;
        }
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
    OutputPage page;
    std::string command_id = "scatter_plot";
    if (kind == "interval") {
        page = interval(table, configuration);
        command_id = "interval_plot";
    } else if (kind == "correlation") {
        page = correlation(table, configuration);
        command_id = "correlation_plot";
    } else if (kind == "bubble") {
        page = bubble(table, configuration);
        command_id = "bubble_plot";
    } else if (kind == "probability") {
        page = probability(table, configuration);
        command_id = "probability_plot";
    } else if (kind == "ecdf") {
        page = ecdf(table, configuration);
        command_id = "ecdf_plot";
    } else if (kind == "matrix") {
        page = matrix(table, configuration);
        command_id = "matrix_plot";
    } else if (kind == "marginal") {
        page = marginal(table, configuration);
        command_id = "marginal_plot";
    } else if (kind == "parallel") {
        page = parallel(table, configuration);
        command_id = "parallel_plot";
    } else if (kind == "dotplot") {
        page = dotplot(table, configuration);
        command_id = "dotplot";
    } else if (kind == "heatmap") {
        page = heatmap(table, configuration);
        command_id = "heatmap_plot";
    } else if (kind == "time_series") {
        page = time_series(table, configuration);
        command_id = "time_series_plot";
    } else if (kind == "area") {
        page = area(table, configuration);
        command_id = "area_plot";
    } else if (kind == "contour") {
        page = contour(table, configuration);
        command_id = "contour_plot";
    } else if (kind == "pie") {
        page = pie(table, configuration);
        command_id = "pie_plot";
    } else if (kind == "density") {
        page = density(table, configuration);
        command_id = "density_plot";
    } else if (kind == "hexbin") {
        page = hexbin(table, configuration);
        command_id = "hexbin_plot";
    } else if (kind == "violin") {
        page = violin(table, configuration);
        command_id = "violin_plot";
    } else if (kind == "bar") {
        page = bar(table, configuration);
        command_id = "bar_chart";
    } else if (kind == "simplex") {
        page = simplex_design_plot(table, configuration);
        command_id = "simplex_design_plot";
    } else if (kind == "mosaic") {
        page = mosaic(table, configuration);
        command_id = "mosaic_plot";
    } else if (kind == "histogram") {
        page = histogram(table, configuration);
        command_id = "histogram";
    } else if (kind == "box") {
        page = box(table, configuration);
        command_id = "boxplot";
    } else {
        page = scatter(table, configuration);
        command_id = "scatter_plot";
    }
    if (page.analysis_command_id.empty()) {
        page.analysis_command_id = command_id;
    }
    if (page.computation_traces.empty()) {
        attach_computation_traces(page, page.analysis_command_id);
    }
    return page;
}

OutputPage GraphService::scatter(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.x_column.has_value()) {
        return error_page("散点图", "请选择 X 变量。");
    }
    const std::vector<std::size_t> y_columns =
        !graph.variable_columns.empty()
            ? graph.variable_columns
            : (graph.y_column.has_value()
                   ? std::vector<std::size_t>{*graph.y_column}
                   : std::vector<std::size_t>{});
    if (y_columns.empty()) {
        return error_page("散点图", "请选择 Y 变量。");
    }

    if (y_columns.size() > 1 && !graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "散点图";
        page.method_name = "Scatter Plot";
        PlotSpec plot = base_plot(
            PlotKind::scatter, "散点图",
            label(table, *graph.x_column), "Y");
        plot.values.clear();
        plot.x_values.clear();
        static const char* k_series_colors[] = {
            "#1565c0", "#c62828", "#2e7d32", "#6a1b9a", "#ef6c00", "#00838f"};
        std::size_t display_n = 0;
        std::size_t analysis_n = 0;
        for (std::size_t index = 0; index < y_columns.size(); ++index) {
            const std::size_t y_column = y_columns[index];
            const AssembledGraphColumns columns = assemble_graph_columns(
                table, *graph.x_column, y_column, graph.size_column,
                graph.by_column, graph.label_column, configuration.excluded_rows, true);
            analysis_n = std::max(analysis_n, columns.source_rows.size());
            AssembledGraphColumns display_columns =
                filter_hidden_from_columns(columns, configuration.hidden_rows);
            display_n += display_columns.first.size();
            datalab::domain::PlotSeries series;
            series.role = datalab::domain::PlotSeriesRole::generic;
            series.label = label(table, y_column);
            series.style.color = k_series_colors[index % 6];
            series.style.visible = true;
            series.show_points = true;
            series.style.point_style = datalab::domain::PlotPointStyle::circle;
            series.style.point_size = 3.5;
            series.x_values = display_columns.first;
            series.values = display_columns.second;
            plot.series.push_back(std::move(series));
        }
        page.plots.push_back(std::move(plot));
        page.parameter_summary = "X = " + label(table, *graph.x_column)
            + "    Y 列数 = " + count_text(y_columns.size())
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_n);
        attach_visibility_facts(page, "scatter", table, configuration, display_n, analysis_n);
        return page;
    }

    if (y_columns.size() > 1 && graph.facet_column.has_value()) {
        const AssembledGraphColumns facet_base = assemble_graph_columns(
            table, *graph.x_column, y_columns.front(), graph.size_column,
            graph.by_column, graph.label_column, configuration.excluded_rows, true);
        std::vector<std::string> facet_labels;
        facet_labels.reserve(facet_base.source_rows.size());
        for (const std::size_t row : facet_base.source_rows) {
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
        std::size_t analysis_n = 0;
        static const char* k_series_colors[] = {
            "#1565c0", "#c62828", "#2e7d32", "#6a1b9a", "#ef6c00", "#00838f"};
        for (const auto& panel : partition.panels) {
            const std::vector<std::size_t> panel_source_rows =
                panel_source_rows_from_member_indices(facet_base, panel.member_indices);
            PlotSpec plot = base_plot(
                PlotKind::scatter,
                "散点图 · " + panel.level,
                label(table, *graph.x_column),
                "Y");
            plot.subtitle = "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel_source_rows.size());
            for (std::size_t y_index = 0; y_index < y_columns.size(); ++y_index) {
                const std::size_t y_column = y_columns[y_index];
                const AssembledGraphColumns columns = assemble_graph_columns(
                    table, *graph.x_column, y_column, graph.size_column,
                    graph.by_column, graph.label_column, configuration.excluded_rows, true);
                analysis_n = std::max(analysis_n, columns.source_rows.size());
                const AssembledGraphColumns slice =
                    slice_graph_columns_by_source_rows(columns, panel_source_rows);
                AssembledGraphColumns display_slice =
                    filter_hidden_from_columns(slice, configuration.hidden_rows);
                display_n += display_slice.first.size();
                datalab::domain::PlotSeries series;
                series.role = datalab::domain::PlotSeriesRole::generic;
                series.label = label(table, y_column);
                series.style.color = k_series_colors[y_index % 6];
                series.style.visible = true;
                series.show_points = true;
                series.style.point_style = datalab::domain::PlotPointStyle::circle;
                series.style.point_size = 3.5;
                series.x_values = display_slice.first;
                series.values = display_slice.second;
                plot.series.push_back(std::move(series));
            }
            page.plots.push_back(std::move(plot));
        }
        page.diagnostics.insert(
            page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
        page.parameter_summary = "X = " + label(table, *graph.x_column)
            + "    Y 列数 = " + count_text(y_columns.size())
            + "    分面 = " + label(table, *graph.facet_column)
            + "    面板 = " + count_text(page.plots.size())
            + "/" + count_text(partition.level_count)
            + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_n);
        attach_visibility_facts(page, "scatter", table, configuration, display_n, analysis_n);
        apply_facet_facts(page, partition, graph.facet_max_panels);
        return page;
    }

    if (!graph.y_column.has_value()) {
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
        domain::statistics::BubblePlotOptions bubble_options;
        bubble_options.size_mode = graph.bubble_size_mode;
        auto result = domain::statistics::bubble_plot(
            analysis_slice.first, analysis_slice.second, analysis_slice.third,
            analysis_slice.source_rows, analysis_slice.groups, analysis_slice.labels,
            bubble_options);
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

    domain::statistics::ParallelPlotOptions plot_options;
    plot_options.y_scale_mode = graph.y_scale_mode;
    plot_options.sort_by_variation = graph.sort_by_variation;
    plot_options.parallel_layout = graph.parallel_layout;

    const auto push_parallel_plot = [&](OutputPage& page,
                                        const AssembledMatrixColumns& analysis_slice,
                                        const std::string& title,
                                        const std::string& subtitle) {
        AssembledMatrixColumns display_slice =
            filter_hidden_from_matrix(analysis_slice, configuration.hidden_rows);
        const auto analysis_result = domain::statistics::parallel_plot(
            analysis_slice.columns, analysis_slice.names,
            analysis_slice.source_rows, analysis_slice.groups, plot_options);
        const auto result = domain::statistics::parallel_plot(
            display_slice.columns, display_slice.names,
            display_slice.source_rows, display_slice.groups, plot_options);
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
            + "    y_scale = " + graph.y_scale_mode
            + "    layout = " + graph.parallel_layout
            + "    sort_variation = " + (graph.sort_by_variation ? "yes" : "no")
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        page.analysis_command_id = "parallel_plot";
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
        + "    y_scale = " + graph.y_scale_mode
        + "    layout = " + graph.parallel_layout
        + "    sort_variation = " + (graph.sort_by_variation ? "yes" : "no")
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    page.analysis_command_id = "parallel_plot";
    attach_visibility_facts(page, "parallel", table, configuration, display_n,
                            analysis_assembled.source_rows.size());
    apply_facet_facts(page, partition, graph.facet_max_panels);
    return page;
}

OutputPage GraphService::dotplot(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    const std::vector<std::size_t> y_columns =
        !graph.variable_columns.empty()
            ? graph.variable_columns
            : (graph.y_column.has_value()
                   ? std::vector<std::size_t>{*graph.y_column}
                   : std::vector<std::size_t>{});
    if (y_columns.empty()) {
        return error_page("点图", "请选择 Y 变量。");
    }

    OutputPage page;
    page.configuration = configuration;
    page.title = "点图";
    page.method_name = "Dotplot";
    std::size_t display_n = 0;

    for (std::size_t column : y_columns) {
        const auto values_col = extract_numeric_column(
            table, column, configuration.excluded_rows);
        if (values_col.values.empty()) {
            continue;
        }
        std::vector<std::string> groups;
        if (graph.by_column.has_value()) {
            for (const std::size_t row : values_col.source_rows) {
                groups.push_back(table.rows[row][*graph.by_column]);
            }
        }
        domain::statistics::DotplotOptions options;
        options.layout_mode = graph.dotplot_layout_mode;
        options.jitter = graph.dotplot_jitter;
        const auto result = domain::statistics::dotplot(
            values_col.values, values_col.source_rows, groups, options);

        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());

        PlotSpec plot = base_plot(
            PlotKind::scatter,
            y_columns.size() > 1 ? "点图 · " + label(table, column) : "点图",
            "位置",
            label(table, column));
        plot.subtitle = "layout = " + result.layout_mode
            + (result.jitter ? "    jitter = yes" : "    jitter = no");
        plot.values = result.values;
        plot.x_values = result.positions;
        for (std::size_t i = 0; i < result.jitter_offsets.size(); ++i) {
            plot.x_values[i] += result.jitter_offsets[i];
        }
        plot.source_rows = result.source_rows;
        plot.point_groups = result.groups;
        page.plots.push_back(std::move(plot));
        display_n += result.values.size();
    }

    if (page.plots.empty()) {
        return error_page("点图", "所选列没有数值观测。");
    }

    page.parameter_summary = "Y 列数 = " + count_text(y_columns.size())
        + "    图数 = " + count_text(page.plots.size())
        + "    显示 N = " + count_text(display_n)
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    page.analysis_command_id = "dotplot";
    attach_visibility_facts(page, "dotplot", table, configuration, display_n, display_n);
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
    const std::vector<std::size_t> value_columns =
        !graph.variable_columns.empty()
            ? graph.variable_columns
            : (graph.y_column.has_value()
                   ? std::vector<std::size_t>{*graph.y_column}
                   : std::vector<std::size_t>{});
    if (value_columns.empty()) {
        return error_page("时间序列图", "请选择数值变量。");
    }
    const std::size_t time_column = graph.time_column.value_or(
        graph.x_column.value_or(value_columns.front()));

    if (value_columns.size() > 1 && !graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "时间序列图";
        page.method_name = "Time Series Plot";
        PlotSpec plot = base_plot(
            PlotKind::time_series, "时间序列图",
            label(table, time_column), "数值");
        static const char* k_series_colors[] = {
            "#1565c0", "#c62828", "#2e7d32", "#6a1b9a", "#ef6c00", "#00838f"};
        std::size_t display_n = 0;
        std::size_t analysis_n = 0;
        for (std::size_t index = 0; index < value_columns.size(); ++index) {
            const std::size_t value_column = value_columns[index];
            const AssembledGraphColumns columns = assemble_time_series_columns(
                table, time_column, value_column, graph.by_column,
                graph.label_column, configuration.excluded_rows);
            analysis_n = std::max(analysis_n, columns.source_rows.size());
            AssembledGraphColumns display_columns =
                filter_hidden_from_columns(columns, configuration.hidden_rows);
            display_n += display_columns.first.size();
            datalab::domain::PlotSeries series;
            series.role = datalab::domain::PlotSeriesRole::generic;
            series.label = label(table, value_column);
            series.style.color = k_series_colors[index % 6];
            series.style.visible = true;
            series.style.line_width = graph.connect_missing ? 1.8 : 0.0;
            series.show_points = true;
            series.style.point_style = datalab::domain::PlotPointStyle::circle;
            series.style.point_size = 3.5;
            series.x_values = display_columns.first;
            series.values = display_columns.second;
            plot.series.push_back(std::move(series));
            for (std::size_t point = 0; point < display_columns.first.size(); ++point) {
                plot.x_values.push_back(display_columns.first[point]);
                plot.values.push_back(display_columns.second[point]);
                plot.source_rows.push_back(display_columns.source_rows[point]);
            }
        }
        page.plots.push_back(std::move(plot));
        page.parameter_summary = "时间 = " + label(table, time_column)
            + "    数值列数 = " + count_text(value_columns.size())
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_n);
        attach_visibility_facts(page, "time_series", table, configuration, display_n, analysis_n);
        return page;
    }

    if (value_columns.size() > 1 && graph.facet_column.has_value()) {
        const AssembledGraphColumns facet_base = assemble_time_series_columns(
            table, time_column, value_columns.front(), graph.by_column,
            graph.label_column, configuration.excluded_rows);
        const auto facet_labels = facet_labels_for_rows(
            table, facet_base.source_rows, *graph.facet_column);
        const auto partition = domain::partition_facet_levels(
            facet_labels, graph.facet_max_panels);
        OutputPage page;
        page.configuration = configuration;
        page.title = "时间序列图（分面）";
        page.method_name = "Faceted Time Series Plot";
        std::size_t display_n = 0;
        std::size_t analysis_n = 0;
        static const char* k_series_colors[] = {
            "#1565c0", "#c62828", "#2e7d32", "#6a1b9a", "#ef6c00", "#00838f"};
        for (const auto& panel : partition.panels) {
            const std::vector<std::size_t> panel_source_rows =
                panel_source_rows_from_member_indices(facet_base, panel.member_indices);
            PlotSpec plot = base_plot(
                PlotKind::time_series,
                "时间序列图 · " + panel.level,
                label(table, time_column),
                "数值");
            plot.subtitle = "facet = " + panel.level
                + "    分析 N(水平) = " + count_text(panel_source_rows.size());
            for (std::size_t value_index = 0; value_index < value_columns.size(); ++value_index) {
                const std::size_t value_column = value_columns[value_index];
                const AssembledGraphColumns columns = assemble_time_series_columns(
                    table, time_column, value_column, graph.by_column,
                    graph.label_column, configuration.excluded_rows);
                analysis_n = std::max(analysis_n, columns.source_rows.size());
                const AssembledGraphColumns slice =
                    slice_graph_columns_by_source_rows(columns, panel_source_rows);
                AssembledGraphColumns display_slice =
                    filter_hidden_from_columns(slice, configuration.hidden_rows);
                display_n += display_slice.first.size();
                datalab::domain::PlotSeries series;
                series.role = datalab::domain::PlotSeriesRole::generic;
                series.label = label(table, value_column);
                series.style.color = k_series_colors[value_index % 6];
                series.style.visible = true;
                series.style.line_width = graph.connect_missing ? 1.8 : 0.0;
                series.show_points = true;
                series.style.point_style = datalab::domain::PlotPointStyle::circle;
                series.style.point_size = 3.5;
                series.x_values = display_slice.first;
                series.values = display_slice.second;
                plot.series.push_back(std::move(series));
                for (std::size_t point = 0; point < display_slice.first.size(); ++point) {
                    plot.x_values.push_back(display_slice.first[point]);
                    plot.values.push_back(display_slice.second[point]);
                    plot.source_rows.push_back(display_slice.source_rows[point]);
                }
            }
            page.plots.push_back(std::move(plot));
        }
        page.diagnostics.insert(
            page.diagnostics.end(), partition.diagnostics.begin(), partition.diagnostics.end());
        page.parameter_summary = "时间 = " + label(table, time_column)
            + "    数值列数 = " + count_text(value_columns.size())
            + "    分面 = " + label(table, *graph.facet_column)
            + "    面板 = " + count_text(page.plots.size())
            + "/" + count_text(partition.level_count)
            + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_n);
        attach_visibility_facts(page, "time_series", table, configuration, display_n, analysis_n);
        apply_facet_facts(page, partition, graph.facet_max_panels);
        return page;
    }

    if (!graph.y_column.has_value()) {
        return error_page("时间序列图", "请选择数值变量。");
    }
    const AssembledGraphColumns analysis_columns = assemble_time_series_columns(
        table, time_column, *graph.y_column, graph.by_column, graph.label_column,
        configuration.excluded_rows);

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
        plot.categories = result.time_labels;
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
    const std::vector<std::size_t> variable_columns =
        !graph.variable_columns.empty()
            ? graph.variable_columns
            : (graph.y_column.has_value()
                   ? std::vector<std::size_t>{*graph.y_column}
                   : std::vector<std::size_t>{});
    if (variable_columns.empty()) {
        return error_page("小提琴图", "请选择响应变量。");
    }

    const bool has_by = graph.by_column.has_value();
    const bool multiple_variables = variable_columns.size() > 1;

    const auto push_violin_plot = [&](OutputPage& page,
                                      const AssembledGraphColumns& analysis_slice,
                                      const std::string& title,
                                      const std::string& subtitle,
                                      const std::string& y_axis_title) {
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
            PlotKind::violin, title, has_by ? "分组" : "变量", y_axis_title);
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

    const auto build_slice_from_extracted = [&](const ExtractedNumericColumn& extracted) {
        AssembledGraphColumns slice;
        slice.first = extracted.values;
        slice.source_rows = extracted.source_rows;
        if (has_by) {
            const std::vector<std::string> by_values =
                extract_text_column(table, *graph.by_column);
            for (const std::size_t row : extracted.source_rows) {
                slice.groups.push_back(row < by_values.size() ? by_values[row] : "*");
            }
        }
        return slice;
    };

    const auto append_variable_violins = [&](PlotSpec& plot,
                                             const ExtractedNumericColumn& extracted) {
        AssembledGraphColumns slice = build_slice_from_extracted(extracted);
        AssembledGraphColumns display_slice =
            filter_hidden_from_columns(slice, configuration.hidden_rows);
        const auto result = domain::statistics::violin_plot(
            display_slice.first, display_slice.groups, display_slice.source_rows);
        for (const auto& group : result.groups) {
            const std::string group_label = has_by ? group.label : extracted.name;
            plot.box_labels.push_back(group_label);
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
            density.label = group_label;
            density.x_values = group.density_values;
            density.values = group.density_y;
            plot.series.push_back(std::move(density));
        }
        return std::make_pair(display_slice.first.size(), result);
    };

    if (graph.facet_column.has_value()) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "小提琴图（分面）";
        page.method_name = "Faceted Violin Plot";
        std::size_t display_n = 0;
        std::size_t analysis_n = 0;
        std::size_t analysis_group_count = 0;
        domain::FacetPartitionResult last_partition;
        for (const std::size_t column : variable_columns) {
            const AssembledGraphColumns analysis_columns = assemble_graph_columns(
                table, column, {}, {}, graph.by_column, {}, configuration.excluded_rows, true);
            analysis_n = std::max(analysis_n, analysis_columns.first.size());
            const auto facet_labels = facet_labels_for_rows(
                table, analysis_columns.source_rows, *graph.facet_column);
            const auto partition = domain::partition_facet_levels(
                facet_labels, graph.facet_max_panels);
            last_partition = partition;
            page.diagnostics.insert(
                page.diagnostics.end(),
                partition.diagnostics.begin(),
                partition.diagnostics.end());
            const std::string y_label = label(table, column);
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
                const std::string panel_title = multiple_variables
                    ? "小提琴图 · " + y_label + " · " + panel.level
                    : "小提琴图 · " + panel.level;
                const auto pushed = push_violin_plot(
                    page,
                    slice,
                    panel_title,
                    "facet = " + panel.level
                        + "    分析 N(水平) = " + count_text(panel.member_indices.size()),
                    y_label);
                display_n += pushed.first;
                analysis_group_count = std::max(analysis_group_count, pushed.second.first);
            }
        }
        page.parameter_summary = "变量数 = " + count_text(variable_columns.size())
            + (graph.by_column.has_value()
                   ? "    分组 = " + label(table, *graph.by_column) : "")
            + "    分面 = " + label(table, *graph.facet_column)
            + "    面板 = " + count_text(page.plots.size())
            + "/" + count_text(last_partition.level_count)
            + "    max_panels = " + std::to_string(std::clamp(graph.facet_max_panels, 1, 12))
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_n)
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "violin", table, configuration, display_n,
                                analysis_n, analysis_group_count);
        apply_facet_facts(page, last_partition, graph.facet_max_panels);
        return page;
    }

    if (has_by && multiple_variables) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "小提琴图";
        page.method_name = "Violin Plot";
        std::size_t display_n = 0;
        std::size_t analysis_n = 0;
        std::size_t analysis_group_count = 0;
        double bandwidth = 0.0;
        for (const std::size_t column : variable_columns) {
            const ExtractedNumericColumn extracted =
                extract_numeric_column(table, column, configuration.excluded_rows);
            if (extracted.values.empty()) {
                continue;
            }
            analysis_n = std::max(analysis_n, extracted.source_rows.size());
            const auto pushed = push_violin_plot(
                page,
                build_slice_from_extracted(extracted),
                extracted.name,
                "",
                extracted.name);
            display_n += pushed.first;
            analysis_group_count = std::max(analysis_group_count, pushed.second.first);
            bandwidth = pushed.second.second;
        }
        if (page.plots.empty()) {
            return error_page("小提琴图", "所选列没有数值观测。");
        }
        page.parameter_summary = "变量数 = " + count_text(variable_columns.size())
            + "    分组 = " + label(table, *graph.by_column)
            + "    显示组数 = " + std::to_string(analysis_group_count)
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_n)
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "violin", table, configuration, display_n,
                                analysis_n, analysis_group_count);
        if (page.facts.eda.has_value()) {
            page.facts.eda->bandwidth = bandwidth;
            page.facts.eda->category_count = analysis_group_count;
        }
        return page;
    }

    if (!has_by && multiple_variables) {
        OutputPage page;
        page.configuration = configuration;
        page.title = "小提琴图";
        page.method_name = "Violin Plot";
        PlotSpec plot;
        plot.kind = PlotKind::violin;
        plot.title = "小提琴图";
        plot.x_axis_title = "变量";
        plot.y_axis_title = "数值";
        std::size_t display_n = 0;
        std::size_t analysis_n = 0;
        double bandwidth = 0.0;
        for (const std::size_t column : variable_columns) {
            const ExtractedNumericColumn extracted =
                extract_numeric_column(table, column, configuration.excluded_rows);
            if (extracted.values.empty()) {
                continue;
            }
            analysis_n = std::max(analysis_n, extracted.source_rows.size());
            const auto appended = append_variable_violins(plot, extracted);
            display_n += appended.first;
            page.diagnostics.insert(
                page.diagnostics.end(),
                appended.second.diagnostics.begin(),
                appended.second.diagnostics.end());
            bandwidth = appended.second.bandwidth;
        }
        if (plot.box_labels.empty()) {
            return error_page("小提琴图", "所选列没有数值观测。");
        }
        page.plots.push_back(std::move(plot));
        page.parameter_summary = "变量数 = " + count_text(variable_columns.size())
            + "    显示 N = " + count_text(display_n)
            + "    分析 N = " + count_text(analysis_n)
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        attach_visibility_facts(page, "violin", table, configuration, display_n, analysis_n,
                                plot.box_labels.size());
        if (page.facts.eda.has_value()) {
            page.facts.eda->bandwidth = bandwidth;
            page.facts.eda->category_count = plot.box_labels.size();
        }
        return page;
    }

    const AssembledGraphColumns analysis_columns = assemble_graph_columns(
        table, variable_columns.front(), {}, {}, graph.by_column, {}, configuration.excluded_rows,
        true);
    OutputPage page;
    page.configuration = configuration;
    page.title = "小提琴图";
    page.method_name = "Violin Plot";
    const auto pushed = push_violin_plot(
        page,
        analysis_columns,
        "小提琴图",
        "",
        label(table, variable_columns.front()));
    page.parameter_summary = "响应 = " + label(table, variable_columns.front())
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

OutputPage GraphService::bar(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.x_column.has_value()) {
        return error_page("条形图", "请选择分类变量。");
    }
    const auto excluded = to_row_set(configuration.excluded_rows);
    const auto hidden = to_row_set(configuration.hidden_rows);

    const std::vector<std::size_t> measure_columns =
        !graph.variable_columns.empty()
            ? graph.variable_columns
            : (graph.y_column.has_value() && !graph.weight_column.has_value()
                   ? std::vector<std::size_t>{*graph.y_column}
                   : std::vector<std::size_t>{});

    if (!measure_columns.empty()) {
        std::vector<std::string> category_order;
        std::map<std::string, std::size_t> category_index;
        std::vector<std::vector<double>> sums;
        const auto ensure_category = [&](const std::string& category) -> std::size_t {
            const auto found = category_index.find(category);
            if (found != category_index.end()) {
                return found->second;
            }
            const std::size_t index = category_order.size();
            category_index.emplace(category, index);
            category_order.push_back(category);
            sums.emplace_back(measure_columns.size(), 0.0);
            return index;
        };

        std::size_t analysis_rows = 0;
        for (std::size_t row = 0; row < table.rows.size(); ++row) {
            if (excluded.count(row) != 0) {
                continue;
            }
            if (*graph.x_column >= table.rows[row].size()) {
                continue;
            }
            const std::string category = table.rows[row][*graph.x_column];
            const std::size_t category_slot = ensure_category(category);
            bool has_measure = false;
            for (std::size_t measure_index = 0; measure_index < measure_columns.size();
                 ++measure_index) {
                const std::size_t column = measure_columns[measure_index];
                if (column >= table.rows[row].size()) {
                    continue;
                }
                double value = 0.0;
                if (!parse_finite_number(table.rows[row][column], value)) {
                    continue;
                }
                sums[category_slot][measure_index] += value;
                has_measure = true;
            }
            if (has_measure) {
                ++analysis_rows;
            }
        }

        if (category_order.empty()) {
            return error_page("条形图", "所选列没有有效数值。");
        }

        PlotSpec plot = base_plot(
            PlotKind::bar, "条形图", label(table, *graph.x_column), "数值");
        static const char* k_series_colors[] = {
            "#1565c0", "#c62828", "#2e7d32", "#6a1b9a", "#ef6c00", "#00838f"};
        for (std::size_t measure_index = 0; measure_index < measure_columns.size();
             ++measure_index) {
            datalab::domain::PlotSeries series;
            series.role = datalab::domain::PlotSeriesRole::generic;
            series.label = label(table, measure_columns[measure_index]);
            series.style.color = k_series_colors[measure_index % 6];
            series.style.visible = true;
            for (const std::vector<double>& row_sums : sums) {
                series.values.push_back(row_sums[measure_index]);
            }
            plot.series.push_back(std::move(series));
        }
        plot.categories = category_order;
        if (measure_columns.size() == 1 && !plot.series.empty()) {
            plot.category_values = plot.series.front().values;
        }

        OutputPage page;
        page.configuration = configuration;
        page.title = "条形图";
        page.method_name = "Bar Chart";
        page.parameter_summary = "类别 = " + label(table, *graph.x_column)
            + "    数值列数 = " + count_text(measure_columns.size())
            + "    类别数 = " + count_text(category_order.size())
            + "    分析行 = " + count_text(analysis_rows)
            + "    excluded = " + count_text(configuration.excluded_rows.size())
            + "    hidden = " + count_text(configuration.hidden_rows.size());
        page.plots.push_back(std::move(plot));
        attach_visibility_facts(page, "bar", table, configuration, analysis_rows, analysis_rows);
        return page;
    }

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

OutputPage GraphService::simplex_design_plot(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (graph.variable_columns.size() < 3 || graph.variable_columns.size() > 4) {
        return error_page("混料三角图", "请选择 3～4 个分量列。");
    }
    const AssembledMatrixColumns assembled = assemble_numeric_matrix(
        table, graph.variable_columns, {}, configuration.excluded_rows);
    std::vector<std::vector<double>> component_rows;
    component_rows.reserve(assembled.source_rows.size());
    for (std::size_t row_index = 0; row_index < assembled.source_rows.size(); ++row_index) {
        std::vector<double> row;
        for (const auto& column : assembled.columns) {
            row.push_back(row_index < column.size() ? column[row_index] : 0.0);
        }
        component_rows.push_back(std::move(row));
    }
    std::vector<std::string> point_labels;
    if (graph.simplex_label_mode == "label_column" && graph.label_column.has_value()) {
        for (const std::size_t source_row : assembled.source_rows) {
            if (source_row < table.rows.size()
                && *graph.label_column < table.rows[source_row].size()) {
                point_labels.push_back(table.rows[source_row][*graph.label_column]);
            } else {
                point_labels.emplace_back();
            }
        }
    }

    domain::statistics::SimplexDesignPlotOptions options;
    options.layout_mode = graph.simplex_layout_mode;
    options.unit_mode = graph.simplex_unit_mode;
    options.label_mode = graph.simplex_label_mode;

    const auto hidden = to_row_set(configuration.hidden_rows);
    const auto analysis_result = domain::statistics::simplex_design_plot(
        component_rows, assembled.source_rows, point_labels, options);

    OutputPage page;
    page.configuration = configuration;
    page.title = "混料三角图";
    page.method_name = "Simplex Design Plot";
    page.diagnostics = analysis_result.diagnostics;
    page.analysis_command_id = "simplex_design_plot";
    std::size_t display_n = 0;
    for (const auto& panel : analysis_result.panels) {
        std::vector<double> xs;
        std::vector<double> ys;
        std::vector<std::size_t> rows;
        std::vector<std::string> labels;
        for (std::size_t index = 0; index < panel.points.source_rows.size(); ++index) {
            const std::size_t source_row = panel.points.source_rows[index];
            if (hidden.count(source_row) != 0) {
                continue;
            }
            if (index < panel.points.x_values.size()) {
                xs.push_back(panel.points.x_values[index]);
            }
            if (index < panel.points.y_values.size()) {
                ys.push_back(panel.points.y_values[index]);
            }
            rows.push_back(source_row);
            if (index < panel.points.point_labels.size()) {
                labels.push_back(panel.points.point_labels[index]);
            }
        }
        display_n += xs.size();
        PlotSpec plot = base_plot(
            PlotKind::scatter, "混料三角图 · " + panel.title, "X", "Y");
        plot.subtitle = "layout = " + analysis_result.layout_mode
            + "    q = " + std::to_string(analysis_result.component_count);
        plot.x_values = std::move(xs);
        plot.values = std::move(ys);
        plot.source_rows = std::move(rows);
        plot.point_labels = std::move(labels);
        page.plots.push_back(std::move(plot));
    }
    page.parameter_summary = "分量数 = " + count_text(graph.variable_columns.size())
        + "    布局 = " + graph.simplex_layout_mode
        + "    显示 N = " + count_text(display_n)
        + "    分析 N = " + count_text(assembled.source_rows.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());
    attach_visibility_facts(page, "simplex", table, configuration, display_n,
                            assembled.source_rows.size());
    return page;
}

OutputPage GraphService::mosaic(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (graph.variable_columns.size() < 2 || graph.variable_columns.size() > 3) {
        return error_page("马赛克图", "请选择 2～3 个分类列。");
    }
    const auto excluded = to_row_set(configuration.excluded_rows);
    const auto hidden = to_row_set(configuration.hidden_rows);
    const std::size_t column_count = graph.variable_columns.size();
    std::vector<std::vector<std::string>> analysis_categories(column_count);
    std::vector<std::vector<std::string>> display_categories(column_count);
    std::vector<std::size_t> analysis_rows;
    std::vector<std::size_t> display_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (excluded.count(row) != 0) {
            continue;
        }
        std::vector<std::string> row_values;
        bool complete = true;
        for (const std::size_t column_index : graph.variable_columns) {
            if (column_index >= table.rows[row].size()
                || table.rows[row][column_index].empty()) {
                complete = false;
                break;
            }
            row_values.push_back(table.rows[row][column_index]);
        }
        if (!complete) {
            continue;
        }
        analysis_rows.push_back(row);
        for (std::size_t col = 0; col < column_count; ++col) {
            analysis_categories[col].push_back(row_values[col]);
        }
        if (hidden.count(row) == 0) {
            display_rows.push_back(row);
            for (std::size_t col = 0; col < column_count; ++col) {
                display_categories[col].push_back(row_values[col]);
            }
        }
    }

    domain::statistics::MosaicPlotOptions options;
    options.measure_mode = graph.mosaic_measure_mode;
    options.sort_mode = graph.mosaic_sort_mode;
    options.max_combinations =
        static_cast<std::size_t>(std::clamp(graph.mosaic_max_combinations, 5, 30));

    const auto analysis_mosaic = domain::statistics::mosaic_plot(
        analysis_categories, analysis_rows, options);
    const auto result = domain::statistics::mosaic_plot(
        display_categories, display_rows, options);

    OutputPage page;
    page.configuration = configuration;
    page.title = "马赛克图";
    page.method_name = "Mosaic Plot";
    page.diagnostics = result.diagnostics;
    page.diagnostics.insert(
        page.diagnostics.end(),
        analysis_mosaic.diagnostics.begin(),
        analysis_mosaic.diagnostics.end());
    page.analysis_command_id = "mosaic_plot";
    page.parameter_summary = "分类列 = " + count_text(column_count)
        + "    组合 = " + count_text(result.combination_count)
        + "    measure = " + graph.mosaic_measure_mode
        + "    显示 N = " + count_text(display_rows.size())
        + "    分析 N = " + count_text(analysis_rows.size())
        + "    excluded = " + count_text(configuration.excluded_rows.size())
        + "    hidden = " + count_text(configuration.hidden_rows.size());

    PlotSpec plot = base_plot(PlotKind::bar, "马赛克图", "组合", "值");
    plot.categories = result.combination_labels;
    plot.category_values = result.values;
    plot.member_source_rows = result.member_source_rows;
    plot.source_rows.assign(
        plot.member_source_rows.size(), std::numeric_limits<std::size_t>::max());
    for (std::size_t index = 0; index < plot.member_source_rows.size(); ++index) {
        if (!plot.member_source_rows[index].empty()) {
            plot.source_rows[index] = plot.member_source_rows[index].front();
        }
    }
    page.plots.push_back(std::move(plot));
    attach_visibility_facts(page, "mosaic", table, configuration, display_rows.size(),
                            analysis_rows.size());
    return page;
}

namespace {

std::string gallery_kind_title(const std::string& kind)
{
    if (kind == "scatter") {
        return "散点图";
    }
    if (kind == "bar") {
        return "条形图";
    }
    if (kind == "box") {
        return "箱线图";
    }
    if (kind == "histogram") {
        return "直方图";
    }
    if (kind == "dotplot") {
        return "点图";
    }
    return kind;
}

AnalysisConfiguration build_gallery_delegated(const AnalysisConfiguration& configuration)
{
    AnalysisConfiguration delegated = configuration;
    const auto& gallery = configuration.graph_gallery;
    delegated.graph = GraphConfiguration{};
    delegated.variable_columns.clear();
    delegated.by_column.reset();
    delegated.graph.variable_columns.clear();

    const auto gallery_y_columns = [&]() -> std::vector<std::size_t> {
        if (!gallery.y_columns.empty()) {
            return gallery.y_columns;
        }
        if (gallery.y_column.has_value()) {
            return {*gallery.y_column};
        }
        return {};
    };

    if (gallery.gallery_kind == "scatter") {
        delegated.graph.graph_kind = "scatter";
        delegated.graph.x_column = gallery.x_column;
        const std::vector<std::size_t> y_columns = gallery_y_columns();
        delegated.graph.variable_columns = y_columns;
        if (!y_columns.empty()) {
            delegated.graph.y_column = y_columns.front();
        }
        if (gallery.by_column.has_value()) {
            delegated.graph.by_column = gallery.by_column;
        }
    } else if (gallery.gallery_kind == "bar") {
        delegated.graph.graph_kind = "bar";
        delegated.graph.x_column = gallery.category_column;
        if (gallery.weight_column.has_value()) {
            delegated.graph.weight_column = gallery.weight_column;
        }
    } else if (gallery.gallery_kind == "box") {
        delegated.graph.graph_kind = "box";
        delegated.variable_columns = gallery_y_columns();
        if (gallery.by_column.has_value()) {
            delegated.by_column = gallery.by_column;
        }
    } else if (gallery.gallery_kind == "histogram") {
        delegated.graph.graph_kind = "histogram";
        delegated.variable_columns = gallery_y_columns();
        delegated.graph.bin_count = gallery.bin_count;
        if (gallery.by_column.has_value()) {
            delegated.by_column = gallery.by_column;
            delegated.graph.by_column = gallery.by_column;
        }
    } else if (gallery.gallery_kind == "dotplot") {
        delegated.graph.graph_kind = "dotplot";
        const std::vector<std::size_t> y_columns = gallery_y_columns();
        delegated.graph.variable_columns = y_columns;
        if (!y_columns.empty()) {
            delegated.graph.y_column = y_columns.front();
        }
        if (gallery.by_column.has_value()) {
            delegated.graph.by_column = gallery.by_column;
        }
        delegated.graph.dotplot_layout_mode = gallery.dotplot_layout_mode;
        delegated.graph.dotplot_jitter = gallery.dotplot_jitter;
    } else {
        delegated.graph.graph_kind = gallery.gallery_kind;
    }
    return delegated;
}

}  // namespace

OutputPage GraphService::histogram(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    return AnalysisService::histogram(table, configuration);
}

OutputPage GraphService::box(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    return AnalysisService::boxplot(table, configuration);
}

OutputPage GraphService::graph_gallery(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& gallery = configuration.graph_gallery;
    if (gallery.gallery_kind == "scatter") {
        const bool has_y = !gallery.y_columns.empty() || gallery.y_column.has_value();
        if (!gallery.x_column.has_value() || !has_y) {
            return error_page(
                "探索性图形",
                "散点图需要 X 与至少一个 Y 数值列。");
        }
    } else if (gallery.gallery_kind == "bar") {
        if (!gallery.category_column.has_value()) {
            return error_page(
                "探索性图形",
                "条形图需要类别列。");
        }
    } else if (gallery.gallery_kind == "box"
               || gallery.gallery_kind == "histogram"
               || gallery.gallery_kind == "dotplot") {
        const bool has_y = !gallery.y_columns.empty() || gallery.y_column.has_value();
        if (!has_y) {
            return error_page(
                "探索性图形",
                "所选图型需要至少一个 Y 数值列。");
        }
    } else {
        return error_page(
            "探索性图形",
            "不支持的画廊图型：" + gallery.gallery_kind);
    }

    const AnalysisConfiguration delegated = build_gallery_delegated(configuration);
    OutputPage page = run(table, delegated);
    const std::string delegated_command = page.analysis_command_id;
    page.configuration = configuration;
    page.analysis_command_id = "graph_gallery";
    page.title = "探索性图形 · " + gallery_kind_title(gallery.gallery_kind);
    page.method_name = "Graph Gallery";

    GraphGalleryFacts facts;
    facts.gallery_kind = gallery.gallery_kind;
    facts.delegated_command_id = delegated_command;
    facts.plot_count = page.plots.size();
    facts.evidence_type = "graph_reference";
    page.facts.graph_gallery = facts;

    if (page.computation_traces.empty()) {
        attach_computation_traces(page, "graph_gallery");
    }
    return page;
}

OutputPage GraphService::chi_square_mosaic_link(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const ChiSquareMosaicLinkConfiguration& link = configuration.chi_square_mosaic_link;
    if (!link.row_category_column.has_value() || !link.column_category_column.has_value()) {
        return error_page("卡方–马赛克联动", "请选择行分类列和列分类列。");
    }
    if (*link.row_category_column == *link.column_category_column) {
        return error_page("卡方–马赛克联动", "行分类列与列分类列必须不同。");
    }

    AnalysisConfiguration chi_config = configuration;
    chi_config.chart_type = "chi_square";
    chi_config.inference.row_category_column = link.row_category_column;
    chi_config.inference.column_category_column = link.column_category_column;
    OutputPage chi_page = AnalysisService::chi_square(table, chi_config);

    AnalysisConfiguration mosaic_config = configuration;
    mosaic_config.chart_type = "mosaic_plot";
    mosaic_config.graph = GraphConfiguration{};
    mosaic_config.graph.graph_kind = "mosaic";
    mosaic_config.graph.variable_columns = {
        *link.row_category_column, *link.column_category_column};
    if (link.third_category_column.has_value()
        && *link.third_category_column != *link.row_category_column
        && *link.third_category_column != *link.column_category_column) {
        mosaic_config.graph.variable_columns.push_back(*link.third_category_column);
    }
    mosaic_config.graph.mosaic_measure_mode = link.mosaic_measure_mode;
    mosaic_config.graph.mosaic_sort_mode = link.mosaic_sort_mode;
    mosaic_config.graph.mosaic_max_combinations =
        std::clamp(link.mosaic_max_combinations, 5, 30);
    OutputPage mosaic_page = mosaic(table, mosaic_config);

    OutputPage page;
    page.configuration = configuration;
    page.analysis_command_id = "chi_square_mosaic_link";
    page.title = "卡方–马赛克联动";
    page.method_name = "Chi-Square Mosaic Link";
    page.diagnostics = chi_page.diagnostics;
    page.diagnostics.insert(
        page.diagnostics.end(),
        mosaic_page.diagnostics.begin(),
        mosaic_page.diagnostics.end());

    for (const auto& table_block : chi_page.tables) {
        if (link.table_mode == "counts_only"
            && table_block.title != "观察频数") {
            continue;
        }
        if (!link.include_percent_tables
            && (table_block.title == "行百分比"
                || table_block.title == "列百分比"
                || table_block.title == "合计百分比")) {
            continue;
        }
        if (!link.include_cell_statistics && table_block.title == "单元格统计") {
            continue;
        }
        page.tables.push_back(table_block);
    }

    if (link.include_adjusted_residual_heatmap) {
        for (const auto& plot : chi_page.plots) {
            if (plot.title == "调整残差热图" || plot.title == "观察频数热图") {
                page.plots.push_back(plot);
            }
        }
    }
    if (link.include_mosaic_plot) {
        for (const auto& plot : mosaic_page.plots) {
            page.plots.push_back(plot);
        }
    }

    page.parameter_summary = "行 = "
        + std::to_string(*link.row_category_column)
        + "    列 = " + std::to_string(*link.column_category_column)
        + "    table_mode = " + link.table_mode
        + "    mosaic_measure = " + link.mosaic_measure_mode
        + "    表数 = " + std::to_string(page.tables.size())
        + "    图数 = " + std::to_string(page.plots.size());

    ChiSquareMosaicLinkFacts facts;
    facts.category_column_count = mosaic_config.graph.variable_columns.size();
    facts.table_count = page.tables.size();
    facts.plot_count = page.plots.size();
    facts.table_mode = link.table_mode;
    facts.mosaic_available = link.include_mosaic_plot && !mosaic_page.plots.empty();
    if (facts.mosaic_available && !mosaic_page.plots.empty()) {
        facts.mosaic_combination_count = mosaic_page.plots.front().categories.size();
    }
    if (chi_page.facts.chi_square.has_value()) {
        const auto& chi_facts = *chi_page.facts.chi_square;
        facts.chi_square_available = true;
        facts.chi_square_statistic = chi_facts.statistic;
        facts.chi_square_p_value = chi_facts.p_value;
        facts.max_abs_adjusted_residual = chi_facts.max_abs_adjusted_residual;
        facts.residual_heatmap_available =
            link.include_adjusted_residual_heatmap && chi_facts.residual_heatmap_available;
        page.facts.chi_square = chi_facts;
    }
    page.facts.chi_square_mosaic_link = facts;

    if (page.computation_traces.empty()) {
        attach_computation_traces(page, "chi_square_mosaic_link");
    }
    return page;
}

}  // namespace datalab::application
