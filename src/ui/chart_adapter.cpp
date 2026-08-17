#include "ui/chart_adapter.h"

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

datalab::domain::PlotSeriesRole plot_series_role(ChartSeriesRole role)
{
    using datalab::domain::PlotSeriesRole;
    switch (role) {
    case ChartSeriesRole::Actual: return PlotSeriesRole::actual;
    case ChartSeriesRole::Fitted: return PlotSeriesRole::fitted;
    case ChartSeriesRole::Forecast: return PlotSeriesRole::forecast;
    case ChartSeriesRole::InteractionFirst: return PlotSeriesRole::interaction_first;
    case ChartSeriesRole::InteractionSecond: return PlotSeriesRole::interaction_second;
    case ChartSeriesRole::ConfidenceBand: return PlotSeriesRole::confidence_band;
    case ChartSeriesRole::Trend: return PlotSeriesRole::trend;
    case ChartSeriesRole::Seasonal: return PlotSeriesRole::seasonal;
    case ChartSeriesRole::Remainder: return PlotSeriesRole::remainder;
    case ChartSeriesRole::Component: return PlotSeriesRole::component;
    case ChartSeriesRole::Generic:
    default: return PlotSeriesRole::generic;
    }
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
        series.show_points = source.show_points;
        model.series.push_back(std::move(series));
    }
    model.source_rows = plot.source_rows;
    model.special_cause_points = plot.special_cause_points;
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
    return model;
}

datalab::domain::PlotSpec plot_from_chart_model(const ChartModel& model)
{
    datalab::domain::PlotSpec plot;
    switch (model.kind) {
    case ChartKind::Histogram:
        plot.kind = datalab::domain::PlotKind::histogram;
        break;
    case ChartKind::BoxPlot:
        plot.kind = datalab::domain::PlotKind::boxplot;
        break;
    case ChartKind::Pareto:
        plot.kind = datalab::domain::PlotKind::pareto;
        break;
    case ChartKind::Probability:
        plot.kind = datalab::domain::PlotKind::probability;
        break;
    case ChartKind::Scatter:
        plot.kind = datalab::domain::PlotKind::scatter;
        break;
    case ChartKind::Control:
    default:
        plot.kind = datalab::domain::PlotKind::control;
        break;
    }
    plot.title = model.title.toStdString();
    plot.x_axis_title = model.x_axis_title.toStdString();
    plot.y_axis_title = model.y_axis_title.toStdString();
    plot.center_label = model.center_label.toStdString();
    plot.subtitle = model.subtitle.toStdString();
    plot.show_grid = model.show_grid;
    plot.show_legend = model.show_legend;
    plot.line_width = model.line_width;
    plot.values = model.values;
    plot.x_values = model.x_values;
    plot.center = model.center;
    plot.lower = model.lower;
    plot.upper = model.upper;
    for (const ChartSeries& source : model.series) {
        datalab::domain::PlotSeries series;
        series.role = plot_series_role(source.role);
        series.label = source.label.toStdString();
        series.values = source.values;
        series.x_values = source.x_values;
        series.lower = source.lower;
        series.upper = source.upper;
        series.line_width = source.line_width;
        series.show_points = source.show_points;
        plot.series.push_back(std::move(series));
    }
    plot.source_rows = model.source_rows;
    plot.special_cause_points = model.special_cause_points;
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
    return plot;
}
