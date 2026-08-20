#pragma once

#include <QPointF>
#include <QString>

#include <cstddef>
#include <optional>
#include <vector>

enum class ChartKind {
    Control,
    Histogram,
    BoxPlot,
    Pareto,
    Probability,
    Scatter,
    Interval,
    Correlation,
    Bubble,
    Ecdf,
    Matrix,
    Marginal,
    Parallel,
    Heatmap,
    TimeSeries,
    Area,
    Contour,
    Pie,
    Surface
};

enum class ChartSeriesRole {
    Generic,
    Actual,
    Fitted,
    Forecast,
    InteractionFirst,
    InteractionSecond,
    ConfidenceBand,
    Trend,
    Seasonal,
    Remainder,
    Component
};

enum class ChartLineStyle {
    Solid,
    Dash,
    Dot,
    DashDot
};

enum class ChartPointStyle {
    None,
    Circle,
    Square,
    Triangle,
    Cross
};

struct ChartSeriesStyle {
    bool visible = true;
    QString color = QStringLiteral("#455a64");
    QString fill_color;
    ChartLineStyle line_style = ChartLineStyle::Solid;
    ChartPointStyle point_style = ChartPointStyle::None;
    double line_width = 1.8;
    double point_size = 3.5;
    double opacity = 1.0;
};

struct ChartReferenceStyle {
    bool visible = true;
    QString label;
    QString color = QStringLiteral("#455a64");
    ChartLineStyle line_style = ChartLineStyle::Dash;
    double line_width = 1.2;
};

struct ChartViewState {
    std::vector<std::size_t> selected_points;
    std::optional<std::size_t> hovered_point;
    double zoom_factor = 1.0;
    QPointF pan_offset;
};

struct ChartSeries {
    ChartSeriesRole role = ChartSeriesRole::Generic;
    QString label;
    std::vector<double> values;
    std::vector<double> x_values;
    std::vector<double> lower;
    std::vector<double> upper;
    double line_width = 1.8;
    bool show_points = false;
    ChartSeriesStyle style;
};

struct ChartModel final {
    int schema_version = 1;
    ChartKind kind = ChartKind::Control;
    QString title = QStringLiteral("控制图");
    QString x_axis_title = QStringLiteral("观测序号");
    QString y_axis_title = QStringLiteral("测量值");
    QString center_label = QStringLiteral("CL");
    QString subtitle;
    bool show_grid = true;
    bool show_legend = true;
    double line_width = 1.8;
    int legend_font_size = 8;
    int title_font_size = 11;
    int axis_font_size = 9;
    QString theme_preset = QStringLiteral("default");
    QString grid_color = QStringLiteral("#e3e7eb");
    ChartSeriesStyle value_style{
        true, QStringLiteral("#1565c0"), {}, ChartLineStyle::Solid,
        ChartPointStyle::Circle, 1.8, 3.5, 1.0};
    ChartReferenceStyle center_style{
        true, QStringLiteral("CL"), QStringLiteral("#2e7d32"),
        ChartLineStyle::Dash, 1.2};
    ChartReferenceStyle lower_style{
        true, QStringLiteral("LCL"), QStringLiteral("#d32f2f"),
        ChartLineStyle::Dash, 1.0};
    ChartReferenceStyle upper_style{
        true, QStringLiteral("UCL"), QStringLiteral("#d32f2f"),
        ChartLineStyle::Dash, 1.0};
    std::vector<double> values;
    std::vector<double> x_values;
    std::vector<double> center;
    std::vector<double> lower;
    std::vector<double> upper;
    std::vector<ChartSeries> series;
    std::vector<std::size_t> source_rows;
    std::vector<std::vector<std::size_t>> special_cause_points;
    std::vector<std::vector<int>> triggered_tests;
    std::vector<int> primary_test_by_point;
    std::vector<int> signal_direction;
    double sigma_z = 0.0;
    ChartViewState view;
    std::vector<double> histogram_edges;
    std::vector<double> histogram_counts;
    std::optional<double> lsl;
    std::optional<double> usl;
    std::optional<double> target;
    std::optional<double> process_mean;
    std::optional<double> within_sigma;
    std::optional<double> overall_sigma;
    std::vector<QString> categories;
    std::vector<double> category_values;
    std::vector<double> cumulative_percent;
    std::vector<double> box_min;
    std::vector<double> box_q1;
    std::vector<double> box_median;
    std::vector<double> box_q3;
    std::vector<double> box_max;
    std::vector<QString> box_labels;
    std::vector<double> interval_lower;
    std::vector<double> interval_upper;
    std::vector<std::size_t> interval_counts;
    std::vector<QString> point_labels;
    std::vector<QString> point_groups;
    std::vector<double> bubble_sizes;
    std::vector<QString> matrix_labels;
    std::vector<std::vector<double>> matrix_values;
    std::vector<std::vector<std::size_t>> matrix_counts;
    std::vector<std::vector<double>> matrix_p_values;
    std::vector<double> histogram_edges_y;
    std::vector<double> histogram_counts_y;
    std::vector<double> contour_x;
    std::vector<double> contour_y;
    std::vector<double> contour_levels;
    std::optional<double> color_min;
    std::optional<double> color_max;
    std::optional<double> y_min;
    std::optional<double> y_max;
    std::optional<double> x_min;
    std::optional<double> x_max;
    QString data_region_fill;
};
