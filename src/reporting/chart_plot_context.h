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

QRectF chart_plot_area(const QRectF& area, ChartKind kind);

void chart_apply_custom_y_range(double& y_min, double& y_max, const ChartModel& model);
void chart_apply_custom_x_range(double& x_min, double& x_max, const ChartModel& model);

ChartPlotContext build_histogram_plot_context(const ChartModel& model, const QRectF& area);
ChartPlotContext build_control_plot_context(const ChartModel& model, const QRectF& area);
ChartPlotContext build_scatter_plot_context(const ChartModel& model, const QRectF& area);
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
