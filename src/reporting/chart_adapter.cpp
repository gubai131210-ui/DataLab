#include "reporting/chart_adapter.h"

#include <utility>

namespace {

ChartSeriesRole chart_series_role(datalab::domain::PlotSeriesRole role)
{
    using datalab::domain::PlotSeriesRole;
    switch (role) {
    case PlotSeriesRole::actual: return ChartSeriesRole::Actual;
    case PlotSeriesRole::fitted: return ChartSeriesRole::Fitted;
    case PlotSeriesRole::forecast: return ChartSeriesRole::Forecast;
    case PlotSeriesRole::interaction_first: return ChartSeriesRole::InteractionFirst;
    case PlotSeriesRole::interaction_second: return ChartSeriesRole::InteractionSecond;
    case PlotSeriesRole::confidence_band: return ChartSeriesRole::ConfidenceBand;
    case PlotSeriesRole::trend: return ChartSeriesRole::Trend;
    case PlotSeriesRole::seasonal: return ChartSeriesRole::Seasonal;
    case PlotSeriesRole::remainder: return ChartSeriesRole::Remainder;
    case PlotSeriesRole::component: return ChartSeriesRole::Component;
    case PlotSeriesRole::generic:
    default: return ChartSeriesRole::Generic;
    }
}

datalab::domain::PlotLineStyle plot_line_style(ChartLineStyle style)
{
    using datalab::domain::PlotLineStyle;
    switch (style) {
    case ChartLineStyle::Dash: return PlotLineStyle::dash;
    case ChartLineStyle::Dot: return PlotLineStyle::dot;
    case ChartLineStyle::DashDot: return PlotLineStyle::dash_dot;
    case ChartLineStyle::Solid:
    default: return PlotLineStyle::solid;
    }
}

ChartLineStyle chart_line_style(datalab::domain::PlotLineStyle style)
{
    using datalab::domain::PlotLineStyle;
    switch (style) {
    case PlotLineStyle::dash: return ChartLineStyle::Dash;
    case PlotLineStyle::dot: return ChartLineStyle::Dot;
    case PlotLineStyle::dash_dot: return ChartLineStyle::DashDot;
    case PlotLineStyle::solid:
    default: return ChartLineStyle::Solid;
    }
}

datalab::domain::PlotPointStyle plot_point_style(ChartPointStyle style)
{
    using datalab::domain::PlotPointStyle;
    switch (style) {
    case ChartPointStyle::Circle: return PlotPointStyle::circle;
    case ChartPointStyle::Square: return PlotPointStyle::square;
    case ChartPointStyle::Triangle: return PlotPointStyle::triangle;
    case ChartPointStyle::Cross: return PlotPointStyle::cross;
    case ChartPointStyle::None:
    default: return PlotPointStyle::none;
    }
}

ChartPointStyle chart_point_style(datalab::domain::PlotPointStyle style)
{
    using datalab::domain::PlotPointStyle;
    switch (style) {
    case PlotPointStyle::circle: return ChartPointStyle::Circle;
    case PlotPointStyle::square: return ChartPointStyle::Square;
    case PlotPointStyle::triangle: return ChartPointStyle::Triangle;
    case PlotPointStyle::cross: return ChartPointStyle::Cross;
    case PlotPointStyle::none:
    default: return ChartPointStyle::None;
    }
}

ChartSeriesStyle chart_series_style(const datalab::domain::PlotSeriesStyle& source)
{
    ChartSeriesStyle result;
    result.visible = source.visible;
    result.color = QString::fromStdString(source.color);
    result.fill_color = QString::fromStdString(source.fill_color);
    result.line_style = chart_line_style(source.line_style);
    result.point_style = chart_point_style(source.point_style);
    result.line_width = source.line_width;
    result.point_size = source.point_size;
    result.opacity = source.opacity;
    return result;
}

QString default_series_color(const ChartSeriesRole role)
{
    switch (role) {
    case ChartSeriesRole::Actual: return QStringLiteral("#1565c0");
    case ChartSeriesRole::Fitted: return QStringLiteral("#2e7d32");
    case ChartSeriesRole::Forecast: return QStringLiteral("#ef6c00");
    case ChartSeriesRole::InteractionFirst: return QStringLiteral("#1565c0");
    case ChartSeriesRole::InteractionSecond: return QStringLiteral("#c62828");
    case ChartSeriesRole::Trend: return QStringLiteral("#6a1b9a");
    case ChartSeriesRole::Seasonal: return QStringLiteral("#00838f");
    case ChartSeriesRole::Remainder: return QStringLiteral("#5d4037");
    case ChartSeriesRole::ConfidenceBand: return QStringLiteral("#90caf9");
    case ChartSeriesRole::Component:
    case ChartSeriesRole::Generic:
    default: return QStringLiteral("#455a64");
    }
}

datalab::domain::PlotSeriesStyle plot_series_style(const ChartSeriesStyle& source)
{
    datalab::domain::PlotSeriesStyle result;
    result.visible = source.visible;
    result.color = source.color.toStdString();
    result.fill_color = source.fill_color.toStdString();
    result.line_style = plot_line_style(source.line_style);
    result.point_style = plot_point_style(source.point_style);
    result.line_width = source.line_width;
    result.point_size = source.point_size;
    result.opacity = source.opacity;
    return result;
}

ChartReferenceStyle chart_reference_style(
    const datalab::domain::PlotReferenceStyle& source)
{
    return {
        source.visible,
        QString::fromStdString(source.label),
        QString::fromStdString(source.color),
        chart_line_style(source.line_style),
        source.line_width};
}

datalab::domain::PlotReferenceStyle plot_reference_style(
    const ChartReferenceStyle& source)
{
    return {
        source.visible,
        source.label.toStdString(),
        source.color.toStdString(),
        plot_line_style(source.line_style),
        source.line_width};
}

}  // namespace

ChartModel chart_model_from_plot(const datalab::domain::PlotSpec& plot)
{
    ChartModel model;
    switch (plot.kind) {
    case datalab::domain::PlotKind::histogram:
        model.kind = ChartKind::Histogram;
        break;
    case datalab::domain::PlotKind::boxplot:
        model.kind = ChartKind::BoxPlot;
        break;
    case datalab::domain::PlotKind::pareto:
        model.kind = ChartKind::Pareto;
        break;
    case datalab::domain::PlotKind::probability:
        model.kind = ChartKind::Probability;
        break;
    case datalab::domain::PlotKind::scatter:
        model.kind = ChartKind::Scatter;
        break;
    case datalab::domain::PlotKind::interval:
        model.kind = ChartKind::Interval;
        break;
    case datalab::domain::PlotKind::correlation:
        model.kind = ChartKind::Correlation;
        break;
    case datalab::domain::PlotKind::bubble:
        model.kind = ChartKind::Bubble;
        break;
    case datalab::domain::PlotKind::ecdf:
        model.kind = ChartKind::Ecdf;
        break;
    case datalab::domain::PlotKind::matrix:
        model.kind = ChartKind::Matrix;
        break;
    case datalab::domain::PlotKind::marginal:
        model.kind = ChartKind::Marginal;
        break;
    case datalab::domain::PlotKind::parallel:
        model.kind = ChartKind::Parallel;
        break;
    case datalab::domain::PlotKind::heatmap:
        model.kind = ChartKind::Heatmap;
        break;
    case datalab::domain::PlotKind::time_series:
        model.kind = ChartKind::TimeSeries;
        break;
    case datalab::domain::PlotKind::area:
        model.kind = ChartKind::Area;
        break;
    case datalab::domain::PlotKind::contour:
        model.kind = ChartKind::Contour;
        break;
    case datalab::domain::PlotKind::pie:
        model.kind = ChartKind::Pie;
        break;
    case datalab::domain::PlotKind::control:
    default:
        model.kind = ChartKind::Control;
        break;
    }
    model.title = QString::fromStdString(plot.title);
    model.x_axis_title = QString::fromStdString(plot.x_axis_title);
    model.y_axis_title = QString::fromStdString(plot.y_axis_title);
    model.center_label = QString::fromStdString(plot.center_label);
    model.subtitle = QString::fromStdString(plot.subtitle);
    model.show_grid = plot.show_grid;
    model.show_legend = plot.show_legend;
    model.line_width = plot.line_width;
    model.legend_font_size = plot.legend_font_size;
    model.grid_color = QString::fromStdString(plot.grid_color);
    model.value_style = chart_series_style(plot.value_style);
    model.center_style = chart_reference_style(plot.center_style);
    model.lower_style = chart_reference_style(plot.lower_style);
    model.upper_style = chart_reference_style(plot.upper_style);
    if (model.center_style.label.isEmpty() || model.center_style.label == QStringLiteral("CL")) {
        model.center_style.label = model.center_label;
    }
    if (model.lower_style.label.isEmpty()) {
        model.lower_style.label = QStringLiteral("LCL");
    }
    if (model.upper_style.label.isEmpty()) {
        model.upper_style.label = QStringLiteral("UCL");
    }
    model.values = plot.values;
    model.x_values = plot.x_values;
    model.center = plot.center;
    model.lower = plot.lower;
    model.upper = plot.upper;
    for (const datalab::domain::PlotSeries& source : plot.series) {
        ChartSeries series;
        series.role = chart_series_role(source.role);
        series.label = QString::fromStdString(source.label);
        series.values = source.values;
        series.x_values = source.x_values;
        series.lower = source.lower;
        series.upper = source.upper;
        series.line_width = source.line_width;
        series.style = chart_series_style(source.style);
        if (series.style.color == QStringLiteral("#455a64")) {
            series.style.color = default_series_color(series.role);
        }
        if (qFuzzyCompare(source.style.line_width, 1.8)) {
            series.style.line_width = source.line_width;
        }
        series.show_points = source.show_points;
        if (series.show_points && series.style.point_style == ChartPointStyle::None) {
            series.style.point_style = ChartPointStyle::Circle;
        }
        model.series.push_back(std::move(series));
    }
    model.source_rows = plot.source_rows;
    model.special_cause_points = plot.special_cause_points;
    model.triggered_tests = plot.triggered_tests;
    model.primary_test_by_point = plot.primary_test_by_point;
    model.signal_direction = plot.signal_direction;
    model.sigma_z = plot.sigma_z;
    model.histogram_edges = plot.histogram_edges;
    model.histogram_counts = plot.histogram_counts;
    model.lsl = plot.lsl;
    model.usl = plot.usl;
    model.target = plot.target;
    model.process_mean = plot.process_mean;
    model.within_sigma = plot.within_sigma;
    model.overall_sigma = plot.overall_sigma;
    for (const std::string& category : plot.categories) {
        model.categories.push_back(QString::fromStdString(category));
    }
    model.category_values = plot.category_values;
    model.cumulative_percent = plot.cumulative_percent;
    model.box_min = plot.box_min;
    model.box_q1 = plot.box_q1;
    model.box_median = plot.box_median;
    model.box_q3 = plot.box_q3;
    model.box_max = plot.box_max;
    for (const std::string& label : plot.box_labels) {
        model.box_labels.push_back(QString::fromStdString(label));
    }
    model.interval_lower = plot.interval_lower;
    model.interval_upper = plot.interval_upper;
    model.interval_counts = plot.interval_counts;
    for (const std::string& value : plot.point_labels) {
        model.point_labels.push_back(QString::fromStdString(value));
    }
    for (const std::string& value : plot.point_groups) {
        model.point_groups.push_back(QString::fromStdString(value));
    }
    model.bubble_sizes = plot.bubble_sizes;
    for (const std::string& value : plot.matrix_labels) {
        model.matrix_labels.push_back(QString::fromStdString(value));
    }
    model.matrix_values = plot.matrix_values;
    model.matrix_counts = plot.matrix_counts;
    model.matrix_p_values = plot.matrix_p_values;
    model.histogram_edges_y = plot.histogram_edges_y;
    model.histogram_counts_y = plot.histogram_counts_y;
    model.contour_x = plot.contour_x;
    model.contour_y = plot.contour_y;
    model.contour_levels = plot.contour_levels;
    model.color_min = plot.color_min;
    model.color_max = plot.color_max;
    return model;
}

datalab::domain::PlotSpec plot_from_chart_model(const ChartModel& model)
{
    datalab::domain::PlotSpec plot;
    switch (model.kind) {
    case ChartKind::Histogram: plot.kind = datalab::domain::PlotKind::histogram; break;
    case ChartKind::BoxPlot: plot.kind = datalab::domain::PlotKind::boxplot; break;
    case ChartKind::Pareto: plot.kind = datalab::domain::PlotKind::pareto; break;
    case ChartKind::Probability: plot.kind = datalab::domain::PlotKind::probability; break;
    case ChartKind::Scatter: plot.kind = datalab::domain::PlotKind::scatter; break;
    case ChartKind::Interval: plot.kind = datalab::domain::PlotKind::interval; break;
    case ChartKind::Correlation: plot.kind = datalab::domain::PlotKind::correlation; break;
    case ChartKind::Bubble: plot.kind = datalab::domain::PlotKind::bubble; break;
    case ChartKind::Ecdf: plot.kind = datalab::domain::PlotKind::ecdf; break;
    case ChartKind::Matrix: plot.kind = datalab::domain::PlotKind::matrix; break;
    case ChartKind::Marginal: plot.kind = datalab::domain::PlotKind::marginal; break;
    case ChartKind::Parallel: plot.kind = datalab::domain::PlotKind::parallel; break;
    case ChartKind::Heatmap: plot.kind = datalab::domain::PlotKind::heatmap; break;
    case ChartKind::TimeSeries: plot.kind = datalab::domain::PlotKind::time_series; break;
    case ChartKind::Area: plot.kind = datalab::domain::PlotKind::area; break;
    case ChartKind::Contour: plot.kind = datalab::domain::PlotKind::contour; break;
    case ChartKind::Pie: plot.kind = datalab::domain::PlotKind::pie; break;
    case ChartKind::Control:
    default: plot.kind = datalab::domain::PlotKind::control; break;
    }
    plot.title = model.title.toStdString();
    plot.x_axis_title = model.x_axis_title.toStdString();
    plot.y_axis_title = model.y_axis_title.toStdString();
    plot.center_label = model.center_label.toStdString();
    plot.subtitle = model.subtitle.toStdString();
    plot.show_grid = model.show_grid;
    plot.show_legend = model.show_legend;
    plot.line_width = model.line_width;
    plot.legend_font_size = model.legend_font_size;
    plot.grid_color = model.grid_color.toStdString();
    plot.value_style = plot_series_style(model.value_style);
    plot.center_style = plot_reference_style(model.center_style);
    plot.lower_style = plot_reference_style(model.lower_style);
    plot.upper_style = plot_reference_style(model.upper_style);
    plot.values = model.values;
    plot.x_values = model.x_values;
    plot.center = model.center;
    plot.lower = model.lower;
    plot.upper = model.upper;
    for (const ChartSeries& source : model.series) {
        datalab::domain::PlotSeries series;
        series.role = static_cast<datalab::domain::PlotSeriesRole>(
            static_cast<int>(source.role));
        series.label = source.label.toStdString();
        series.values = source.values;
        series.x_values = source.x_values;
        series.lower = source.lower;
        series.upper = source.upper;
        series.line_width = source.style.line_width;
        series.style = plot_series_style(source.style);
        series.show_points = source.show_points;
        plot.series.push_back(std::move(series));
    }
    plot.source_rows = model.source_rows;
    plot.special_cause_points = model.special_cause_points;
    plot.triggered_tests = model.triggered_tests;
    plot.primary_test_by_point = model.primary_test_by_point;
    plot.signal_direction = model.signal_direction;
    plot.sigma_z = model.sigma_z;
    plot.histogram_edges = model.histogram_edges;
    plot.histogram_counts = model.histogram_counts;
    plot.lsl = model.lsl;
    plot.usl = model.usl;
    plot.target = model.target;
    plot.process_mean = model.process_mean;
    plot.within_sigma = model.within_sigma;
    plot.overall_sigma = model.overall_sigma;
    for (const QString& category : model.categories) {
        plot.categories.push_back(category.toStdString());
    }
    plot.category_values = model.category_values;
    plot.cumulative_percent = model.cumulative_percent;
    plot.box_min = model.box_min;
    plot.box_q1 = model.box_q1;
    plot.box_median = model.box_median;
    plot.box_q3 = model.box_q3;
    plot.box_max = model.box_max;
    for (const QString& label : model.box_labels) {
        plot.box_labels.push_back(label.toStdString());
    }
    plot.interval_lower = model.interval_lower;
    plot.interval_upper = model.interval_upper;
    plot.interval_counts = model.interval_counts;
    for (const QString& value : model.point_labels) {
        plot.point_labels.push_back(value.toStdString());
    }
    for (const QString& value : model.point_groups) {
        plot.point_groups.push_back(value.toStdString());
    }
    plot.bubble_sizes = model.bubble_sizes;
    for (const QString& value : model.matrix_labels) {
        plot.matrix_labels.push_back(value.toStdString());
    }
    plot.matrix_values = model.matrix_values;
    plot.matrix_counts = model.matrix_counts;
    plot.matrix_p_values = model.matrix_p_values;
    plot.histogram_edges_y = model.histogram_edges_y;
    plot.histogram_counts_y = model.histogram_counts_y;
    plot.contour_x = model.contour_x;
    plot.contour_y = model.contour_y;
    plot.contour_levels = model.contour_levels;
    plot.color_min = model.color_min;
    plot.color_max = model.color_max;
    return plot;
}
