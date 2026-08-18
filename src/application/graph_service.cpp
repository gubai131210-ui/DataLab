#include "application/graph_service.h"

#include "domain/column_extract.h"
#include "domain/graph_assembly.h"
#include "domain/statistics/graph_visuals.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace datalab::application {
namespace {

using datalab::domain::AnalysisConfiguration;
using datalab::domain::AssembledGraphColumns;
using datalab::domain::AssembledMatrixColumns;
using datalab::domain::DataTable;
using datalab::domain::DiagnosticMessage;
using datalab::domain::OutputPage;
using datalab::domain::PlotKind;
using datalab::domain::PlotSpec;
using datalab::domain::assemble_graph_columns;
using datalab::domain::assemble_numeric_matrix;
using datalab::domain::parse_finite_number;

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

AssembledGraphColumns xy_columns(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    return assemble_graph_columns(
        table, graph.x_column.value_or(0), graph.y_column, graph.size_column,
        graph.by_column, graph.label_column, configuration.excluded_rows, true);
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
    const auto result = domain::statistics::scatter_plot(
        columns.first, columns.second, columns.source_rows, columns.groups, columns.labels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "散点图";
    page.method_name = "Scatter Plot";
    page.parameter_summary = "X = " + label(table, *graph.x_column)
        + "    Y = " + label(table, *graph.y_column)
        + "    N = " + count_text(result.x_values.size());
    page.diagnostics = result.diagnostics;
    PlotSpec plot = base_plot(PlotKind::scatter, "散点图",
                              label(table, *graph.x_column), label(table, *graph.y_column));
    plot.x_values = result.x_values;
    plot.values = result.y_values;
    plot.source_rows = result.source_rows;
    plot.point_groups = result.point_groups;
    plot.point_labels = result.point_labels;
    page.plots.push_back(std::move(plot));
    return page;
}

OutputPage GraphService::interval(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.y_column.has_value() || !graph.by_column.has_value()) {
        return error_page("区间散点图", "请选择响应变量和分类变量。");
    }
    const AssembledGraphColumns columns = assemble_graph_columns(
        table, *graph.y_column, {}, {}, graph.by_column, {}, configuration.excluded_rows, true);
    const auto result = domain::statistics::interval_plot(
        columns.first, columns.groups, columns.source_rows, graph.confidence_level);
    OutputPage page;
    page.configuration = configuration;
    page.title = "区间散点图";
    page.method_name = "Interval Plot";
    page.parameter_summary = "响应 = " + label(table, *graph.y_column)
        + "    分组 = " + label(table, *graph.by_column)
        + "    置信水平 = " + std::to_string(graph.confidence_level);
    page.diagnostics = result.diagnostics;
    PlotSpec plot = base_plot(PlotKind::interval, "区间散点图",
                              label(table, *graph.by_column), label(table, *graph.y_column));
    plot.categories = result.labels;
    plot.values = result.means;
    plot.interval_lower = result.lower;
    plot.interval_upper = result.upper;
    plot.interval_counts = result.counts;
    for (const auto& rows : result.source_rows) {
        if (!rows.empty()) {
            plot.source_rows.push_back(rows.front());
        }
    }
    page.plots.push_back(std::move(plot));
    return page;
}

OutputPage GraphService::correlation(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (graph.variable_columns.size() < 2) {
        return error_page("相关图", "请至少选择两个连续变量。");
    }
    const AssembledMatrixColumns assembled = assemble_numeric_matrix(
        table, graph.variable_columns, {}, configuration.excluded_rows);
    const auto result = domain::statistics::correlation_plot(
        assembled.columns, assembled.names, graph.correlation_method, graph.confidence_level);
    OutputPage page;
    page.configuration = configuration;
    page.title = "相关图";
    page.method_name = "Correlation Plot";
    page.parameter_summary = "方法 = " + graph.correlation_method
        + "    变量数 = " + count_text(assembled.names.size())
        + "    N = " + count_text(assembled.source_rows.size());
    page.diagnostics = result.correlation.diagnostics;
    PlotSpec plot = base_plot(PlotKind::correlation, "相关图", "变量", "变量");
    plot.matrix_labels = result.labels;
    plot.matrix_values = result.correlation.coefficients;
    plot.matrix_counts = result.correlation.counts;
    plot.color_min = -1.0;
    plot.color_max = 1.0;
    for (const auto& pair : result.correlation.pairs) {
        if (pair.p_value.has_value()) {
            if (plot.matrix_p_values.size() < assembled.names.size()) {
                plot.matrix_p_values.assign(
                    assembled.names.size(),
                    std::vector<double>(assembled.names.size(), 0.0));
            }
            plot.matrix_p_values[pair.first_column][pair.second_column] = *pair.p_value;
            plot.matrix_p_values[pair.second_column][pair.first_column] = *pair.p_value;
        }
    }
    page.plots.push_back(std::move(plot));
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
    const AssembledGraphColumns columns = xy_columns(table, configuration);
    const auto result = domain::statistics::bubble_plot(
        columns.first, columns.second, columns.third, columns.source_rows,
        columns.groups, columns.labels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "气泡图";
    page.method_name = "Bubble Plot";
    page.parameter_summary = "X = " + label(table, *graph.x_column)
        + "    Y = " + label(table, *graph.y_column)
        + "    大小 = " + label(table, *graph.size_column)
        + "    N = " + count_text(result.points.x_values.size());
    page.diagnostics = result.diagnostics;
    PlotSpec plot = base_plot(PlotKind::bubble, "气泡图",
                              label(table, *graph.x_column), label(table, *graph.y_column));
    plot.x_values = result.points.x_values;
    plot.values = result.points.y_values;
    plot.bubble_sizes = result.sizes;
    plot.source_rows = result.points.source_rows;
    plot.point_groups = result.points.point_groups;
    plot.point_labels = result.points.point_labels;
    page.plots.push_back(std::move(plot));
    return page;
}

OutputPage GraphService::probability(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.y_column.has_value()) {
        return error_page("正态概率图", "请选择连续变量。");
    }
    const AssembledGraphColumns columns = assemble_graph_columns(
        table, *graph.y_column, {}, {}, {}, {}, configuration.excluded_rows, true);
    const auto result = domain::statistics::probability_plot(
        columns.first, columns.source_rows);
    OutputPage page;
    page.configuration = configuration;
    page.title = "正态概率图";
    page.method_name = "Normal Probability Plot";
    page.parameter_summary = "变量 = " + label(table, *graph.y_column)
        + "    N = " + count_text(result.ordered_values.size())
        + "    位置(均值) = " + std::to_string(result.location)
        + "    尺度(样本标准差) = " + std::to_string(result.scale)
        + "    相关系数 = " + std::to_string(result.correlation);
    page.diagnostics = result.diagnostics;
    PlotSpec plot = base_plot(PlotKind::probability, "正态概率图",
                              "理论分位数", label(table, *graph.y_column));
    plot.subtitle = "直线性不能单独作为正态性证明";
    plot.x_values = result.theoretical_quantiles;
    plot.values = result.ordered_values;
    plot.center = result.fitted;
    plot.lower = result.lower;
    plot.upper = result.upper;
    plot.source_rows = result.source_rows;
    page.plots.push_back(std::move(plot));
    return page;
}

OutputPage GraphService::ecdf(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.y_column.has_value()) {
        return error_page("经验累积分布图", "请选择连续变量。");
    }
    const AssembledGraphColumns columns = assemble_graph_columns(
        table, *graph.y_column, {}, {}, {}, {}, configuration.excluded_rows, true);
    const auto result = domain::statistics::ecdf_plot(columns.first, columns.source_rows);
    OutputPage page;
    page.configuration = configuration;
    page.title = "经验累积分布图";
    page.method_name = "Empirical CDF";
    page.parameter_summary = "变量 = " + label(table, *graph.y_column)
        + "    N = " + count_text(columns.first.size())
        + "    阶梯点数 = " + count_text(result.values.size());
    page.diagnostics = result.diagnostics;
    PlotSpec plot = base_plot(PlotKind::ecdf, "经验累积分布图",
                              label(table, *graph.y_column), "累计比例");
    plot.x_values = result.values;
    plot.values = result.proportions;
    plot.source_rows = result.source_rows;
    plot.interval_counts = result.counts;
    page.plots.push_back(std::move(plot));
    return page;
}

OutputPage GraphService::matrix(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (graph.variable_columns.size() < 2) {
        return error_page("矩阵图", "请至少选择两个连续变量。");
    }
    const AssembledMatrixColumns assembled = assemble_numeric_matrix(
        table, graph.variable_columns, graph.by_column, configuration.excluded_rows);
    const auto result = domain::statistics::matrix_scatter_plot(
        assembled.columns, assembled.names, assembled.source_rows, assembled.groups);
    OutputPage page;
    page.configuration = configuration;
    page.title = "矩阵图";
    page.method_name = "Matrix Plot";
    page.parameter_summary = "变量数 = " + count_text(assembled.names.size())
        + "    N = " + count_text(assembled.source_rows.size());
    page.diagnostics = result.diagnostics;
    PlotSpec plot = base_plot(PlotKind::matrix, "矩阵图", "变量", "变量");
    plot.matrix_labels = result.labels;
    plot.matrix_values = result.columns;
    plot.source_rows = result.source_rows;
    plot.point_groups = result.groups;
    page.plots.push_back(std::move(plot));
    return page;
}

OutputPage GraphService::marginal(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.x_column.has_value() || !graph.y_column.has_value()) {
        return error_page("边际图", "请选择 X 变量和 Y 变量。");
    }
    const AssembledGraphColumns columns = xy_columns(table, configuration);
    const auto result = domain::statistics::marginal_plot(
        columns.first, columns.second, columns.source_rows, graph.bin_count);
    OutputPage page;
    page.configuration = configuration;
    page.title = "边际图";
    page.method_name = "Marginal Plot";
    page.parameter_summary = "X = " + label(table, *graph.x_column)
        + "    Y = " + label(table, *graph.y_column)
        + "    N = " + count_text(result.points.x_values.size());
    page.diagnostics = result.points.diagnostics;
    PlotSpec plot = base_plot(PlotKind::marginal, "边际图",
                              label(table, *graph.x_column), label(table, *graph.y_column));
    plot.x_values = result.points.x_values;
    plot.values = result.points.y_values;
    plot.source_rows = result.points.source_rows;
    plot.histogram_edges = result.x_histogram.edges;
    plot.histogram_counts = result.x_histogram.counts;
    plot.histogram_edges_y = result.y_histogram.edges;
    plot.histogram_counts_y = result.y_histogram.counts;
    page.plots.push_back(std::move(plot));
    return page;
}

OutputPage GraphService::parallel(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (graph.variable_columns.size() < 2) {
        return error_page("平行坐标图", "请至少选择两个连续变量。");
    }
    const AssembledMatrixColumns assembled = assemble_numeric_matrix(
        table, graph.variable_columns, graph.by_column, configuration.excluded_rows, false);
    const auto result = domain::statistics::parallel_plot(
        assembled.columns, assembled.names, assembled.source_rows, assembled.groups);
    OutputPage page;
    page.configuration = configuration;
    page.title = "平行坐标图";
    page.method_name = "Parallel Coordinates Plot";
    page.parameter_summary = "变量数 = " + count_text(assembled.names.size())
        + "    N = " + count_text(result.rows.size())
        + "    坐标已按各变量最小-最大范围标准化";
    page.diagnostics = result.diagnostics;
    PlotSpec plot = base_plot(PlotKind::parallel, "平行坐标图", "变量", "标准化值");
    plot.matrix_labels = result.labels;
    plot.matrix_values = result.rows;
    plot.lower = result.minima;
    plot.upper = result.maxima;
    plot.source_rows = result.source_rows;
    plot.point_groups = result.groups;
    page.plots.push_back(std::move(plot));
    return page;
}

OutputPage GraphService::heatmap(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    OutputPage page;
    page.configuration = configuration;
    page.title = "热图";
    page.method_name = "Heatmap";
    PlotSpec plot = base_plot(PlotKind::heatmap, "热图", "", "");
    if (graph.variable_columns.size() >= 2 && graph.color_scale != "category") {
        const AssembledMatrixColumns assembled = assemble_numeric_matrix(
            table, graph.variable_columns, {}, configuration.excluded_rows);
        const auto correlation = domain::statistics::correlation_plot(
            assembled.columns, assembled.names, graph.correlation_method, graph.confidence_level);
        const auto result = domain::statistics::heatmap_from_correlation(correlation);
        page.parameter_summary = "颜色范围固定为相关系数 [-1, 1]    方法 = "
            + graph.correlation_method;
        page.diagnostics = result.diagnostics;
        plot.matrix_labels = result.row_labels;
        plot.matrix_values = result.values;
        plot.matrix_counts = result.counts;
        plot.color_min = result.color_min;
        plot.color_max = result.color_max;
        plot.x_axis_title = "变量";
        plot.y_axis_title = "变量";
        page.plots.push_back(std::move(plot));
        return page;
    }
    if (!graph.x_column.has_value() || !graph.y_column.has_value()
        || !graph.z_column.has_value()) {
        return error_page("热图", "请选择多个连续变量，或行类别、列类别和数值。");
    }
    std::vector<std::string> rows;
    std::vector<std::string> columns;
    std::vector<double> values;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
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
        rows.push_back(table.rows[row][*graph.y_column]);
        columns.push_back(table.rows[row][*graph.x_column]);
        values.push_back(value);
        source_rows.push_back(row);
    }
    const auto result = domain::statistics::heatmap_from_categories(
        rows, columns, values, source_rows);
    page.parameter_summary = "单元格为组内均值    颜色范围 = ["
        + std::to_string(result.color_min) + ", " + std::to_string(result.color_max) + "]";
    page.diagnostics = result.diagnostics;
    plot.matrix_labels = result.column_labels;
    plot.categories = result.row_labels;
    plot.matrix_values = result.values;
    plot.matrix_counts = result.counts;
    plot.color_min = result.color_min;
    plot.color_max = result.color_max;
    plot.x_axis_title = label(table, *graph.x_column);
    plot.y_axis_title = label(table, *graph.y_column);
    page.plots.push_back(std::move(plot));
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
    const AssembledGraphColumns columns = assemble_graph_columns(
        table, time_column, graph.y_column, {}, graph.by_column, graph.label_column,
        configuration.excluded_rows, true);
    const auto result = domain::statistics::time_series_plot(
        columns.first, columns.second, columns.source_rows, columns.categories, columns.groups);
    OutputPage page;
    page.configuration = configuration;
    page.title = "时间序列图";
    page.method_name = "Time Series Plot";
    page.parameter_summary = "时间 = " + label(table, time_column)
        + "    数值 = " + label(table, *graph.y_column)
        + "    N = " + count_text(result.x_values.size());
    page.diagnostics = result.diagnostics;
    PlotSpec plot = base_plot(PlotKind::time_series, "时间序列图",
                              label(table, time_column), label(table, *graph.y_column));
    plot.x_values = result.x_values;
    plot.values = result.y_values;
    plot.source_rows = result.source_rows;
    plot.point_groups = result.groups;
    plot.point_labels = result.time_labels;
    if (!graph.connect_missing) {
        plot.value_style.line_width = 0.0;
    }
    page.plots.push_back(std::move(plot));
    return page;
}

OutputPage GraphService::area(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    OutputPage page = time_series(table, configuration);
    if (page.plots.empty()) {
        return page;
    }
    page.title = "区域图";
    page.method_name = "Area Plot";
    page.plots.front().kind = PlotKind::area;
    page.plots.front().title = "区域图";
    page.parameter_summary += "    面积表示相邻观测之间的数值区间，不是置信区间";
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
    const AssembledGraphColumns columns = assemble_graph_columns(
        table, *graph.x_column, graph.y_column, graph.z_column, {}, {},
        configuration.excluded_rows, true);
    const auto result = domain::statistics::contour_plot(
        columns.first, columns.second, columns.third, graph.contour_levels);
    OutputPage page;
    page.configuration = configuration;
    page.title = "等值线图";
    page.method_name = "Contour Plot";
    page.parameter_summary = "X = " + label(table, *graph.x_column)
        + "    Y = " + label(table, *graph.y_column)
        + "    Z = " + label(table, *graph.z_column)
        + "    网格 = " + count_text(result.x.size()) + " × " + count_text(result.y.size());
    page.diagnostics = result.diagnostics;
    PlotSpec plot = base_plot(PlotKind::contour, "等值线图",
                              label(table, *graph.x_column), label(table, *graph.y_column));
    plot.contour_x = result.x;
    plot.contour_y = result.y;
    plot.contour_levels = result.levels;
    plot.matrix_values = result.z;
    if (!result.levels.empty()) {
        plot.color_min = result.levels.front();
        plot.color_max = result.levels.back();
    }
    page.plots.push_back(std::move(plot));
    return page;
}

OutputPage GraphService::pie(
    const DataTable& table, const AnalysisConfiguration& configuration)
{
    const auto& graph = configuration.graph;
    if (!graph.x_column.has_value()) {
        return error_page("饼图", "请选择分类变量。");
    }
    std::vector<std::string> categories;
    std::vector<double> weights;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
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
        categories.push_back(table.rows[row][*graph.x_column]);
        weights.push_back(weight);
    }
    const auto result = domain::statistics::pie_plot(
        categories, weights, graph.other_threshold_percent);
    OutputPage page;
    page.configuration = configuration;
    page.title = "饼图";
    page.method_name = "Pie Chart";
    page.parameter_summary = "类别 = " + label(table, *graph.x_column)
        + "    小类别合并阈值 = " + std::to_string(graph.other_threshold_percent) + "%";
    page.diagnostics = result.diagnostics;
    PlotSpec plot = base_plot(PlotKind::pie, "饼图", "", "组成比例");
    plot.categories = result.labels;
    plot.category_values = result.values;
    plot.cumulative_percent = result.percents;
    page.plots.push_back(std::move(plot));
    return page;
}

}  // namespace datalab::application
