#include "reporting/chart_plot_context.h"

#include "reporting/chart_geometry.h"

#include <QLineF>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace {

QVector<double> finite_values(const std::vector<double>& values)
{
    QVector<double> result;
    for (const double value : values) {
        if (std::isfinite(value)) {
            result.append(value);
        }
    }
    return result;
}

void finalize_mapper(
    ChartPlotContext& context,
    const ChartModel& model,
    double x_min,
    double x_max,
    double y_min,
    double y_max)
{
    chart_apply_custom_x_range(x_min, x_max, model);
    chart_apply_custom_y_range(y_min, y_max, model);
    context.mapper.set_data_range(x_min, x_max, y_min, y_max);
    context.mapper.zoom(model.view.zoom_factor, context.plot.center());
    context.mapper.pan(model.view.pan_offset);
}

}  // namespace

QRectF chart_plot_area(const QRectF& area, ChartKind kind)
{
    return chart_geometry::plot_rect(area, kind);
}

void chart_apply_custom_y_range(double& y_min, double& y_max, const ChartModel& model)
{
    const bool custom_min = model.y_min.has_value() && std::isfinite(*model.y_min);
    const bool custom_max = model.y_max.has_value() && std::isfinite(*model.y_max);
    if (custom_min) {
        y_min = *model.y_min;
    }
    if (custom_max) {
        y_max = *model.y_max;
    }
    if (custom_min && custom_max && !(y_min < y_max)) {
        const double mid = 0.5 * (y_min + y_max);
        y_min = mid - 1.0;
        y_max = mid + 1.0;
    }
}

void chart_apply_custom_x_range(double& x_min, double& x_max, const ChartModel& model)
{
    const bool custom_min = model.x_min.has_value() && std::isfinite(*model.x_min);
    const bool custom_max = model.x_max.has_value() && std::isfinite(*model.x_max);
    if (custom_min) {
        x_min = *model.x_min;
    }
    if (custom_max) {
        x_max = *model.x_max;
    }
    if (custom_min && custom_max && !(x_min < x_max)) {
        const double mid = 0.5 * (x_min + x_max);
        x_min = mid - 1.0;
        x_max = mid + 1.0;
    }
}

ChartPlotContext build_histogram_plot_context(const ChartModel& model, const QRectF& area)
{
    ChartPlotContext context;
    if (model.histogram_edges.size() < 2 || model.histogram_counts.empty()) {
        return context;
    }
    context.plot = chart_plot_area(area, ChartKind::Histogram);
    context.mapper = ChartCoordinateMapper(context.plot);
    const double x_min = model.histogram_edges.front();
    const double x_max = model.histogram_edges.back();
    double y_max = *std::max_element(
        model.histogram_counts.cbegin(), model.histogram_counts.cend());
    if (y_max <= 0.0) {
        y_max = 1.0;
    }
    y_max *= 1.15;
    finalize_mapper(context, model, x_min, x_max, 0.0, y_max);
    context.valid = true;
    return context;
}

ChartPlotContext build_control_plot_context(const ChartModel& model, const QRectF& area)
{
    ChartPlotContext context;
    const ChartSeries* first_series = nullptr;
    for (const ChartSeries& series : model.series) {
        if (!series.values.empty() || !series.lower.empty() || !series.upper.empty()) {
            first_series = &series;
            break;
        }
    }
  QVector<double> all_values = finite_values(model.values);
    for (const double value : finite_values(model.lower)) {
        all_values.append(value);
    }
    for (const double value : finite_values(model.upper)) {
        all_values.append(value);
    }
    for (const double value : finite_values(model.center)) {
        all_values.append(value);
    }
    if (first_series != nullptr) {
        for (const double value : finite_values(first_series->values)) {
            all_values.append(value);
        }
        for (const double value : finite_values(first_series->lower)) {
            all_values.append(value);
        }
        for (const double value : finite_values(first_series->upper)) {
            all_values.append(value);
        }
    }
    for (const ChartSeries& series : model.series) {
        for (const double value : finite_values(series.values)) {
            all_values.append(value);
        }
        for (const double value : finite_values(series.lower)) {
            all_values.append(value);
        }
        for (const double value : finite_values(series.upper)) {
            all_values.append(value);
        }
    }
    if (all_values.isEmpty()) {
        return context;
    }
    auto [minimum_it, maximum_it] = std::minmax_element(all_values.cbegin(), all_values.cend());
    double y_min = *minimum_it;
    double y_max = *maximum_it;
    if (qFuzzyCompare(y_min, y_max)) {
        y_min -= 1.0;
        y_max += 1.0;
    } else {
        const double padding = (y_max - y_min) * 0.08;
        y_min -= padding;
        y_max += padding;
    }

    const auto x_at = [&](std::size_t index) {
        return model.x_values.size() == model.values.size()
            ? model.x_values[index] : static_cast<double>(index);
    };
    const auto series_x_at = [](const ChartSeries& series, std::size_t index) {
        return index < series.x_values.size()
            ? series.x_values[index] : static_cast<double>(index);
    };
    double x_min = model.values.empty() && first_series != nullptr
        ? series_x_at(*first_series, 0) : x_at(0);
    double x_max = x_min;
    if (!model.values.empty() && model.x_values.size() == model.values.size()) {
        const auto x_range = std::minmax_element(
            model.x_values.cbegin(), model.x_values.cend());
        x_min = *x_range.first;
        x_max = *x_range.second;
    } else if (!model.values.empty()) {
        x_max = x_at(model.values.size() - 1);
    } else if (first_series != nullptr && !first_series->values.empty()) {
        x_max = series_x_at(*first_series, first_series->values.size() - 1);
    }
    for (const ChartSeries& series : model.series) {
        if (series.values.empty()) {
            continue;
        }
        double series_min = series_x_at(series, 0);
        double series_max = series_min;
        if (series.x_values.size() == series.values.size()) {
            const auto range = std::minmax_element(
                series.x_values.cbegin(), series.x_values.cend());
            series_min = *range.first;
            series_max = *range.second;
        } else {
            series_max = series_x_at(series, series.values.size() - 1);
        }
        x_min = std::min(x_min, series_min);
        x_max = std::max(x_max, series_max);
    }

    context.plot = chart_plot_area(area, ChartKind::Control);
    context.mapper = ChartCoordinateMapper(context.plot);
    finalize_mapper(context, model, x_min, std::max(x_min + 1.0, x_max), y_min, y_max);
    context.valid = true;
    return context;
}

ChartPlotContext build_scatter_plot_context(const ChartModel& model, const QRectF& area)
{
    ChartPlotContext context;
    const std::size_t count = std::min(model.x_values.size(), model.values.size());
    double x_min = 0.0;
    double x_max = 1.0;
    double y_min = 0.0;
    double y_max = 1.0;
    bool have_range = false;
    if (count > 0) {
        const auto x_range = std::minmax_element(
            model.x_values.cbegin(), model.x_values.cbegin() + count);
        const auto y_range = std::minmax_element(
            model.values.cbegin(), model.values.cbegin() + count);
        x_min = *x_range.first;
        x_max = *x_range.second;
        y_min = *y_range.first;
        y_max = *y_range.second;
        have_range = true;
    }
    for (const ChartSeries& series : model.series) {
        const std::size_t series_count = series.values.size();
        const std::size_t interval_count = std::max(series.lower.size(), series.upper.size());
        if (series_count == 0 && interval_count == 0) {
            continue;
        }
        have_range = true;
        if (series_count > 0) {
            const auto series_y_range = std::minmax_element(
                series.values.cbegin(), series.values.cend());
            y_min = std::min(y_min, *series_y_range.first);
            y_max = std::max(y_max, *series_y_range.second);
        }
        if (series.lower.size() == interval_count && interval_count > 0) {
            const auto range = std::minmax_element(
                series.lower.cbegin(), series.lower.cend());
            y_min = std::min(y_min, *range.first);
            y_max = std::max(y_max, *range.second);
        }
        if (series.upper.size() == interval_count && interval_count > 0) {
            const auto range = std::minmax_element(
                series.upper.cbegin(), series.upper.cend());
            y_min = std::min(y_min, *range.first);
            y_max = std::max(y_max, *range.second);
        }
        if (series.x_values.size() == series_count) {
            const auto series_x_range = std::minmax_element(
                series.x_values.cbegin(), series.x_values.cend());
            x_min = std::min(x_min, *series_x_range.first);
            x_max = std::max(x_max, *series_x_range.second);
        }
    }
    if (!have_range) {
        return context;
    }
    if (model.kind == ChartKind::Area) {
        y_min = std::min(0.0, y_min);
        y_max = std::max(0.0, y_max);
    }
    if (x_min == x_max) {
        x_min -= 1.0;
        x_max += 1.0;
    }
    if (y_min == y_max) {
        y_min -= 1.0;
        y_max += 1.0;
    }
    const double x_padding = 0.05 * (x_max - x_min);
    const double y_padding = 0.05 * (y_max - y_min);
    x_min -= x_padding;
    x_max += x_padding;
    y_min -= y_padding;
    y_max += y_padding;

    context.plot = chart_plot_area(area, model.kind);
    context.mapper = ChartCoordinateMapper(context.plot);
    finalize_mapper(context, model, x_min, x_max, y_min, y_max);
    context.valid = true;
    return context;
}

ChartPlotContext build_pareto_plot_context(const ChartModel& model, const QRectF& area)
{
    ChartPlotContext context;
    if (model.category_values.empty()) {
        return context;
    }
    context.plot = chart_plot_area(area, ChartKind::Pareto);
    if (context.plot.width() <= 1.0 || context.plot.height() <= 1.0) {
        return context;
    }
    const bool show_cumulative = !model.cumulative_percent.empty();
    const double maximum_count = *std::max_element(
        model.category_values.cbegin(), model.category_values.cend());
    const double total_count = std::accumulate(
        model.category_values.cbegin(), model.category_values.cend(), 0.0);
    const double reference = !model.center.empty() && std::isfinite(model.center.front())
        ? model.center.front() : 0.0;
    const double scale_top = show_cumulative
        ? std::max({1.0, maximum_count, total_count})
        : std::max({1.0, maximum_count, reference});
    const double tick_step = std::max(1.0, std::ceil(scale_top / 5.0));
    const double y_max = tick_step * 5.0;
    context.mapper = ChartCoordinateMapper(context.plot);
    context.mapper.set_data_range(
        -0.5,
        static_cast<double>(model.category_values.size()) - 0.5,
        0.0,
        y_max);
    context.valid = true;
    return context;
}

ChartPlotContext build_category_index_plot_context(
    const ChartModel& model,
    const QRectF& area,
    const std::vector<double>& values,
    double y_minimum,
    double y_maximum)
{
    ChartPlotContext context;
    if (values.empty()) {
        return context;
    }
    context.plot = chart_plot_area(area, model.kind);
    context.mapper = ChartCoordinateMapper(context.plot);
    context.mapper.set_data_range(
        -0.5,
        static_cast<double>(values.size()) - 0.5,
        y_minimum,
        y_maximum);
    context.valid = true;
    return context;
}

ChartPlotContext build_box_plot_context(const ChartModel& model, const QRectF& area)
{
    ChartPlotContext context;
    const std::size_t box_count = std::min({
        model.box_min.size(), model.box_q1.size(), model.box_median.size(),
        model.box_q3.size(), model.box_max.size()});
    if (box_count == 0) {
        return context;
    }
    QVector<double> all_values;
    for (const auto& series : {model.box_min, model.box_max, model.box_q1, model.box_q3}) {
        for (const double value : series) {
            all_values.append(value);
        }
    }
    for (const ChartSeries& series : model.series) {
        for (const double value : series.values) {
            all_values.append(value);
        }
    }
    if (all_values.isEmpty()) {
        return context;
    }
    auto [minimum_it, maximum_it] = std::minmax_element(all_values.cbegin(), all_values.cend());
    double minimum = *minimum_it;
    double maximum = *maximum_it;
    const double padding = std::max(0.08 * (maximum - minimum), 1.0e-6);
    context.plot = chart_plot_area(area, ChartKind::BoxPlot);
    context.mapper = ChartCoordinateMapper(context.plot);
    context.mapper.set_data_range(
        -0.5,
        static_cast<double>(box_count) - 0.5,
        minimum - padding,
        maximum + padding);
    context.valid = true;
    return context;
}

ChartPlotContext build_violin_plot_context(const ChartModel& model, const QRectF& area)
{
    ChartPlotContext context;
    const std::size_t box_count = std::min({
        model.box_min.size(), model.box_q1.size(), model.box_median.size(),
        model.box_q3.size(), model.box_max.size()});
    if (box_count == 0) {
        return context;
    }
    QVector<double> all_values;
    for (const auto& series : {model.box_min, model.box_max}) {
        for (const double value : series) {
            all_values.append(value);
        }
    }
    for (const ChartSeries& series : model.series) {
        for (const double value : series.values) {
            all_values.append(value);
        }
    }
    if (all_values.isEmpty()) {
        return context;
    }
    auto [minimum_it, maximum_it] = std::minmax_element(all_values.cbegin(), all_values.cend());
    const double padding = std::max(0.08 * (*maximum_it - *minimum_it), 1.0e-6);
    context.plot = chart_plot_area(area, ChartKind::Violin);
    context.mapper = ChartCoordinateMapper(context.plot);
    context.mapper.set_data_range(
        -0.5,
        static_cast<double>(box_count) - 0.5,
        *minimum_it - padding,
        *maximum_it + padding);
    context.valid = true;
    return context;
}

ChartPlotContext build_interval_plot_context(const ChartModel& model, const QRectF& area)
{
    ChartPlotContext context;
    const std::size_t count = std::min({
        model.values.size(), model.interval_lower.size(), model.interval_upper.size()});
    if (count == 0) {
        return context;
    }
    double minimum = *std::min_element(
        model.interval_lower.cbegin(), model.interval_lower.cbegin() + count);
    double maximum = *std::max_element(
        model.interval_upper.cbegin(), model.interval_upper.cbegin() + count);
    if (qFuzzyCompare(minimum, maximum)) {
        minimum -= 1.0;
        maximum += 1.0;
    }
    const double padding = (maximum - minimum) * 0.08;
    context.plot = chart_plot_area(area, ChartKind::Interval);
    context.mapper = ChartCoordinateMapper(context.plot);
    context.mapper.set_data_range(
        -0.5,
        static_cast<double>(count) - 0.5,
        minimum - padding,
        maximum + padding);
    context.valid = true;
    return context;
}

ChartPlotContext build_ecdf_plot_context(const ChartModel& model, const QRectF& area)
{
    ChartPlotContext context;
    const std::size_t count = std::min(model.x_values.size(), model.values.size());
    if (count == 0) {
        return context;
    }
    double x_min = *std::min_element(model.x_values.cbegin(), model.x_values.cbegin() + count);
    double x_max = *std::max_element(model.x_values.cbegin(), model.x_values.cbegin() + count);
    if (qFuzzyCompare(x_min, x_max)) {
        x_min -= 1.0;
        x_max += 1.0;
    }
    context.plot = chart_plot_area(area, ChartKind::Ecdf);
    context.mapper = ChartCoordinateMapper(context.plot);
    context.mapper.set_data_range(x_min, x_max, 0.0, 1.05);
    context.valid = true;
    return context;
}

ChartMatrixGridLayout build_correlation_grid_layout(const ChartModel& model, const QRectF& area)
{
    ChartMatrixGridLayout layout;
    const std::size_t count = model.matrix_values.size();
    if (count == 0 || model.matrix_labels.size() < count) {
        return layout;
    }
    const double cell = std::min(
        82.0,
        std::max(42.0, std::min(area.width(), area.height())
                 / static_cast<double>(count + 1))) * std::max(1.0, model.view.zoom_factor);
    layout.viewport = chart_plot_area(area, ChartKind::Correlation);
    layout.origin = QPointF(area.left() + 120.0, area.top() + 52.0)
        + model.view.pan_offset;
    layout.cell_width = cell;
    layout.cell_height = cell;
    layout.column_count = count;
    layout.row_count = count;
    layout.first_column = layout.origin.x() < layout.viewport.left()
        ? static_cast<std::size_t>(std::max(0.0, std::ceil(
            (layout.viewport.left() - layout.origin.x()) / cell))) : 0;
    layout.first_row = layout.origin.y() < layout.viewport.top()
        ? static_cast<std::size_t>(std::max(0.0, std::ceil(
            (layout.viewport.top() - layout.origin.y()) / cell))) : 0;
    layout.first_column = std::min(layout.first_column, count);
    layout.first_row = std::min(layout.first_row, count);
    layout.visible_column_count = std::min(
        count - layout.first_column,
        static_cast<std::size_t>(std::max(0.0, std::ceil(
            (layout.viewport.right() - layout.origin.x()) / cell))));
    layout.visible_row_count = std::min(
        count - layout.first_row,
        static_cast<std::size_t>(std::max(0.0, std::ceil(
            (layout.viewport.bottom() - layout.origin.y()) / cell))));
    layout.valid = true;
    return layout;
}

ChartMatrixGridLayout build_heatmap_grid_layout(const ChartModel& model, const QRectF& area)
{
    ChartMatrixGridLayout layout;
    const std::vector<QString>& columns = model.matrix_labels;
    const std::vector<QString>& rows = model.categories.empty()
        ? model.matrix_labels : model.categories;
    if (columns.empty() || rows.empty() || model.matrix_values.empty()) {
        return layout;
    }
    const double zoom = std::max(1.0, model.view.zoom_factor);
    layout.cell_width = std::max(18.0, std::min(
        72.0, (area.width() - 140.0) / static_cast<double>(columns.size()))) * zoom;
    layout.cell_height = std::max(18.0, std::min(
        48.0, (area.height() - 90.0) / static_cast<double>(rows.size()))) * zoom;
    layout.viewport = chart_plot_area(area, ChartKind::Heatmap);
    layout.origin = QPointF(area.left() + 110.0, area.top() + 52.0)
        + model.view.pan_offset;
    layout.column_count = columns.size();
    layout.row_count = rows.size();
    layout.first_column = layout.origin.x() < layout.viewport.left()
        ? static_cast<std::size_t>(std::max(0.0, std::ceil(
            (layout.viewport.left() - layout.origin.x()) / layout.cell_width))) : 0;
    layout.first_row = layout.origin.y() < layout.viewport.top()
        ? static_cast<std::size_t>(std::max(0.0, std::ceil(
            (layout.viewport.top() - layout.origin.y()) / layout.cell_height))) : 0;
    layout.first_column = std::min(layout.first_column, layout.column_count);
    layout.first_row = std::min(layout.first_row, layout.row_count);
    layout.visible_column_count = std::min(
        layout.column_count - layout.first_column,
        static_cast<std::size_t>(std::max(0.0, std::ceil(
            (layout.viewport.right() - layout.origin.x()) / layout.cell_width))));
    layout.visible_row_count = std::min(
        layout.row_count - layout.first_row,
        static_cast<std::size_t>(std::max(0.0, std::ceil(
            (layout.viewport.bottom() - layout.origin.y()) / layout.cell_height))));
    layout.valid = true;
    return layout;
}

ChartMatrixGridLayout build_matrix_grid_layout(const ChartModel& model, const QRectF& area)
{
    ChartMatrixGridLayout layout;
    const std::size_t count = model.matrix_labels.size();
    if (count == 0 || model.matrix_values.size() < count) {
        return layout;
    }
    const double zoom = std::max(1.0, model.view.zoom_factor);
    const double cell = std::max(18.0, std::min(area.width(), area.height())
        / static_cast<double>(count + 0.4)) * zoom;
    layout.viewport = chart_plot_area(area, ChartKind::Matrix);
    layout.origin = QPointF(area.left() + 36.0, area.top() + 40.0)
        + model.view.pan_offset;
    layout.cell_width = cell;
    layout.cell_height = cell;
    layout.column_count = count;
    layout.row_count = count;
    layout.first_column = layout.origin.x() < layout.viewport.left()
        ? static_cast<std::size_t>(std::max(0.0, std::ceil(
            (layout.viewport.left() - layout.origin.x()) / cell))) : 0;
    layout.first_row = layout.origin.y() < layout.viewport.top()
        ? static_cast<std::size_t>(std::max(0.0, std::ceil(
            (layout.viewport.top() - layout.origin.y()) / cell))) : 0;
    layout.first_column = std::min(layout.first_column, count);
    layout.first_row = std::min(layout.first_row, count);
    layout.visible_column_count = std::min(count - layout.first_column,
        static_cast<std::size_t>(std::max(0.0, std::ceil(
            (layout.viewport.right() - layout.origin.x()) / cell))));
    layout.visible_row_count = std::min(count - layout.first_row,
        static_cast<std::size_t>(std::max(0.0, std::ceil(
            (layout.viewport.bottom() - layout.origin.y()) / cell))));
    layout.valid = true;
    return layout;
}

double chart_hit_tolerance_pixels(const ChartModel& model, double base_tolerance)
{
    const double zoom = std::max(model.view.zoom_factor, 1.0e-6);
    return base_tolerance / zoom;
}

std::optional<std::size_t> chart_hit_test_point_series(
    const ChartModel& model,
    const QRectF& area,
    const QPoint& position,
    double base_tolerance)
{
    ChartPlotContext context;
    if (model.kind == ChartKind::Control) {
        context = build_control_plot_context(model, area);
    } else {
        context = build_scatter_plot_context(model, area);
    }
    if (!context.valid) {
        return std::nullopt;
    }
    const double tolerance = chart_hit_tolerance_pixels(model, base_tolerance);
    const auto x_at = [&](std::size_t index, const std::vector<double>& xs) {
        return xs.size() > index ? xs[index] : static_cast<double>(index);
    };
    std::optional<std::size_t> result;
    double distance = tolerance;
    const auto test_series = [&](const std::vector<double>& xs,
                                 const std::vector<double>& ys,
                                 std::size_t flat_offset) {
        const std::size_t count = std::min(xs.size(), ys.size());
        for (std::size_t index = 0; index < count; ++index) {
            const QPointF point = context.mapper.to_pixel(x_at(index, xs), ys[index]);
            if (std::abs(point.x() - position.x()) > tolerance) {
                continue;
            }
            const double current = QLineF(point, position).length();
            if (current < distance) {
                distance = current;
                result = flat_offset + index;
            }
        }
    };
    const bool line_visible = (model.kind == ChartKind::Control
        || model.kind == ChartKind::TimeSeries || model.kind == ChartKind::Area)
        && model.value_style.line_width > 0.0;
    if (!model.values.empty() && model.value_style.visible
        && (model.value_style.point_style != ChartPointStyle::None || line_visible)) {
        test_series(model.x_values, model.values, 0);
    }
    if (model.values.empty() && !model.series.empty()) {
        std::size_t flat_offset = 0;
        for (const ChartSeries& series : model.series) {
            if (series.style.visible) {
                test_series(series.x_values, series.values, flat_offset);
            }
            flat_offset += std::min(series.x_values.size(), series.values.size());
        }
    }
    return result;
}
