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
    Scatter
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

struct ChartSeries {
    ChartSeriesRole role = ChartSeriesRole::Generic;
    QString label;
    std::vector<double> values;
    std::vector<double> x_values;
    std::vector<double> lower;
    std::vector<double> upper;
    double line_width = 1.8;
    bool show_points = false;
};

struct ChartModel final {
    ChartKind kind = ChartKind::Control;
    QString title = QStringLiteral("控制图");
    QString x_axis_title = QStringLiteral("观测序号");
    QString y_axis_title = QStringLiteral("测量值");
    QString center_label = QStringLiteral("CL");
    QString subtitle;
    bool show_grid = true;
    bool show_legend = true;
    double line_width = 1.8;
    std::vector<double> values;
    std::vector<double> x_values;
    std::vector<double> center;
    std::vector<double> lower;
    std::vector<double> upper;
    std::vector<ChartSeries> series;
    std::vector<std::size_t> source_rows;
    std::vector<std::vector<std::size_t>> special_cause_points;
    double sigma_z = 0.0;
    std::vector<std::size_t> selected_points;
    std::optional<std::size_t> hovered_point;
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
    double zoom_factor = 1.0;
    QPointF pan_offset;
};
