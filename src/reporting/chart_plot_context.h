#pragma once

#include "reporting/chart_coordinate_mapper.h"
#include "reporting/chart_model.h"

#include <QPointF>
#include <QRectF>

#include <optional>
#include <vector>

struct ChartPlotContext {
    QRectF plot;
    ChartCoordinateMapper mapper;
    bool valid = false;
    bool primary_mirrors_series = false;
    double surface_z_min = 0.0;
    double surface_z_max = 1.0;
};

struct ChartMatrixGridLayout {
    QRectF viewport;
    QPointF origin;
    double cell_width = 0.0;
    double cell_height = 0.0;
    std::size_t column_count = 0;
    std::size_t row_count = 0;
    std::size_t first_column = 0;
    std::size_t first_row = 0;
    std::size_t visible_column_count = 0;
    std::size_t visible_row_count = 0;
    bool valid = false;
};

struct ChartMarginalPlotLayout {
    QRectF scatter_canvas;
    QRectF scatter;
    QRectF x_histogram;
    QRectF y_histogram;
    bool valid = false;
};

QRectF chart_plot_area(const QRectF& area, ChartKind kind);
ChartMarginalPlotLayout chart_marginal_plot_layout(const QRectF& area);
QPointF chart_pan_offset(const ChartModel& model, const QRectF& plot);
void chart_set_pan_offset(ChartModel& model, const QRectF& plot, const QPointF& pixels);

void chart_apply_custom_y_range(double& y_min, double& y_max, const ChartModel& model);
void chart_apply_custom_x_range(double& x_min, double& x_max, const ChartModel& model);

ChartPlotContext build_histogram_plot_context(const ChartModel& model, const QRectF& area);
ChartPlotContext build_control_plot_context(const ChartModel& model, const QRectF& area);
ChartPlotContext build_scatter_plot_context(const ChartModel& model, const QRectF& area);
// One coordinate contract for painting, pointer hit testing and navigation.
ChartPlotContext build_chart_plot_context(const ChartModel& model, const QRectF& area);
ChartPlotContext build_probability_plot_context(const ChartModel& model, const QRectF& area);
ChartPlotContext build_contour_plot_context(const ChartModel& model, const QRectF& area);
ChartPlotContext build_parallel_plot_context(const ChartModel& model, const QRectF& area);
ChartPlotContext build_surface_plot_context(const ChartModel& model, const QRectF& area);
std::optional<QPointF> chart_surface_point(
    const ChartModel& model, const ChartPlotContext& context, std::size_t row, std::size_t column);
std::optional<QPointF> chart_parallel_point(
    const ChartModel& model, const ChartPlotContext& context, std::size_t row, std::size_t axis);
ChartPlotContext build_pareto_plot_context(const ChartModel& model, const QRectF& area);
ChartPlotContext build_category_index_plot_context(
    const ChartModel& model,
    const QRectF& area,
    const std::vector<double>& values,
    double y_minimum,
    double y_maximum);
ChartPlotContext build_box_plot_context(const ChartModel& model, const QRectF& area);
ChartPlotContext build_violin_plot_context(const ChartModel& model, const QRectF& area);
ChartPlotContext build_interval_plot_context(const ChartModel& model, const QRectF& area);
ChartPlotContext build_ecdf_plot_context(const ChartModel& model, const QRectF& area);

ChartMatrixGridLayout build_correlation_grid_layout(const ChartModel& model, const QRectF& area);
ChartMatrixGridLayout build_heatmap_grid_layout(const ChartModel& model, const QRectF& area);
ChartMatrixGridLayout build_matrix_grid_layout(const ChartModel& model, const QRectF& area);

double chart_hit_tolerance_pixels(const ChartModel& model, double base_tolerance = 28.0);

std::optional<std::size_t> chart_hit_test_point_series(
    const ChartModel& model,
    const QRectF& area,
    const QPoint& position,
    double base_tolerance = 28.0);
