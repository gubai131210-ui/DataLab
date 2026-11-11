#include "reporting/chart_plot_context.h"

#include "reporting/chart_geometry.h"
#include "reporting/chart_series_contract.h"

#include <QLineF>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <utility>

namespace {

using datalab::reporting::chart_series_is_visible;

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

double aggregate_range_padding(double minimum, double maximum)
{
    const double scale = maximum > minimum ? maximum - minimum
        : minimum != 0.0 ? std::abs(minimum) : 1.0;
    const double padding = scale * 0.08;
    return padding > 0.0 ? padding
        : std::abs(minimum - std::nextafter(minimum, 0.0));
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
    context.mapper.pan(chart_pan_offset(model, context.plot));
}

}  // namespace

QRectF chart_plot_area(const QRectF& area, ChartKind kind)
{
    auto plot = chart_geometry::plot_rect(area, kind);
    if (kind == ChartKind::Correlation || kind == ChartKind::Heatmap) {
        plot.setLeft(std::max(plot.left(), area.left() + (kind == ChartKind::Correlation ? 120.0 : 110.0)));
        plot.setTop(std::max(plot.top(), area.top() + 52.0));
    }
    return plot;
}

ChartMarginalPlotLayout chart_marginal_plot_layout(const QRectF& area)
{
    ChartMarginalPlotLayout layout;
    layout.scatter_canvas = area.adjusted(0.0, 30.0, 18.0, 0.0);
    layout.scatter = chart_plot_area(layout.scatter_canvas, ChartKind::Scatter);
    layout.x_histogram = QRectF(layout.scatter.left(), area.top() + 36.0,
        layout.scatter.width(), layout.scatter.top() - area.top() - 40.0);
    layout.y_histogram = QRectF(layout.scatter.right() + 8.0, layout.scatter.top(),
        58.0, layout.scatter.height());
    layout.valid = layout.scatter.width() > 1.0 && layout.scatter.height() > 1.0;
    return layout;
}

QPointF chart_pan_offset(const ChartModel& model, const QRectF& plot)
{
    const auto size = model.view.pan_reference_size;
    if (size.width() <= 0.0 || size.height() <= 0.0) return model.view.pan_offset;
    return {model.view.pan_offset.x() * plot.width() / size.width(),
            model.view.pan_offset.y() * plot.height() / size.height()};
}

void chart_set_pan_offset(ChartModel& model, const QRectF& plot, const QPointF& pixels)
{
    model.view.pan_offset = pixels;
    model.view.pan_reference_size = plot.size();
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
    double x_min = std::numeric_limits<double>::infinity(), x_max = -x_min;
    double y_min = x_min, y_max = -x_min;
    const auto extend = [&](const std::vector<double>& ys, const std::vector<double>& xs) {
        for (std::size_t i = 0; i < ys.size(); ++i) {
            const double x = i < xs.size() ? xs[i] : static_cast<double>(i);
            if (!std::isfinite(x) || !std::isfinite(ys[i])) continue;
            x_min = std::min(x_min, x); x_max = std::max(x_max, x);
            y_min = std::min(y_min, ys[i]); y_max = std::max(y_max, ys[i]);
        }
    };
    if (chart_series_is_visible(model.value_style)) extend(model.values, model.x_values);
    if (model.lower_style.visible) extend(model.lower, model.x_values);
    if (model.upper_style.visible) extend(model.upper, model.x_values);
    if (model.center_style.visible) extend(model.center, model.x_values);
    for (const auto& series : model.series) {
        if (!chart_series_is_visible(series.style)) continue;
        extend(series.values, series.x_values);
        extend(series.lower, series.x_values);
        extend(series.upper, series.x_values);
    }
    if (!std::isfinite(x_min) || !std::isfinite(y_min)) return context;
    if (y_min == y_max) { y_min -= 1.0; y_max += 1.0; }
    else { const double padding = (y_max - y_min) * 0.08; y_min -= padding; y_max += padding; }
    context.plot = chart_plot_area(area, model.kind);
    context.mapper = ChartCoordinateMapper(context.plot);
    finalize_mapper(context, model, x_min, std::max(x_min + 1.0, x_max), y_min, y_max);
    context.valid = context.plot.width() > 1.0 && context.plot.height() > 1.0;
    return context;
}

static ChartPlotContext build_xy_plot_context(
    const ChartModel& model, const QRectF& area, double padding, bool include_series)
{
    ChartPlotContext context;
    context.primary_mirrors_series = datalab::reporting::chart_primary_mirrors_series(model);
    double x_min = 0.0;
    double x_max = 1.0;
    double y_min = 0.0;
    double y_max = 1.0;
    bool have_range = false;
    const auto extend = [&](double x, double y) {
        if (!std::isfinite(x) || !std::isfinite(y)) return;
        if (!have_range) { x_min = x_max = x; y_min = y_max = y; have_range = true; }
        x_min = std::min(x_min, x); x_max = std::max(x_max, x);
        y_min = std::min(y_min, y); y_max = std::max(y_max, y);
    };
    std::size_t base_count = model.values.size();
    if (model.kind == ChartKind::Bubble) base_count = std::min(base_count, model.bubble_sizes.size());
    if (!model.x_values.empty()) base_count = std::min(base_count, model.x_values.size());
    if (chart_series_is_visible(model.value_style) && !context.primary_mirrors_series) {
        for (std::size_t i = 0; i < base_count; ++i) {
            extend(model.x_values.empty() ? static_cast<double>(i) : model.x_values[i], model.values[i]);
        }
    }
    if (include_series) for (const ChartSeries& series : model.series) {
        if (!chart_series_is_visible(series.style)) continue;
        const std::size_t count = std::max({series.values.size(), series.lower.size(), series.upper.size()});
        for (std::size_t i = 0; i < count; ++i) {
            const double x = i < series.x_values.size() ? series.x_values[i] : static_cast<double>(i);
            if (i < series.values.size()) extend(x, series.values[i]);
            if (i < series.lower.size()) extend(x, series.lower[i]);
            if (i < series.upper.size()) extend(x, series.upper[i]);
        }
    }
    if (!have_range) {
        return context;
    }
    if (model.kind == ChartKind::Area || model.kind == ChartKind::Density) {
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
    const double x_padding = padding * (x_max - x_min);
    const double y_padding = padding * (y_max - y_min);
    x_min -= x_padding;
    x_max += x_padding;
    y_min -= y_padding;
    y_max += y_padding;

    context.plot = chart_plot_area(area, model.kind);
    context.mapper = ChartCoordinateMapper(context.plot);
    finalize_mapper(context, model, x_min, x_max, y_min, y_max);
    context.valid = context.plot.width() > 1.0 && context.plot.height() > 1.0;
    return context;
}

ChartPlotContext build_scatter_plot_context(const ChartModel& model, const QRectF& area)
{
    return build_xy_plot_context(model, area, 0.05, true);
}

ChartPlotContext build_probability_plot_context(const ChartModel& model, const QRectF& area)
{
    return build_xy_plot_context(model, area, 0.08, false);
}

ChartPlotContext build_pareto_plot_context(const ChartModel& model, const QRectF& area)
{
    ChartPlotContext context;
    const auto& values = model.category_values.empty() ? model.values : model.category_values;
    const bool grouped = !model.series.empty() && !model.categories.empty();
    if (values.empty() && !grouped) {
        return context;
    }
    context.plot = chart_plot_area(area, ChartKind::Pareto);
    if (context.plot.width() <= 1.0 || context.plot.height() <= 1.0) {
        return context;
    }
    const bool show_cumulative = !model.cumulative_percent.empty();
    double maximum_count = values.empty() ? 0.0 : *std::max_element(values.cbegin(), values.cend());
    double minimum_count = values.empty() ? 0.0 : std::min(0.0, *std::min_element(values.cbegin(), values.cend()));
    if (grouped) for (const auto& series : model.series) {
        if (!chart_series_is_visible(series.style)) continue;
        for (double value : series.values) if (std::isfinite(value)) {
            maximum_count = std::max(maximum_count, value);
            minimum_count = std::min(minimum_count, value);
        }
    }
    const double total_count = std::accumulate(
        values.cbegin(), values.cend(), 0.0);
    const double reference = !model.center.empty() && std::isfinite(model.center.front())
        ? model.center.front() : 0.0;
    const double scale_top = show_cumulative
        ? std::max({1.0, maximum_count, total_count})
        : std::max({1.0, maximum_count, reference});
    const double tick_step = std::max(1.0, std::ceil(scale_top / 5.0));
    const double y_max = tick_step * 5.0;
    context.mapper = ChartCoordinateMapper(context.plot);
    finalize_mapper(context, model,
        -0.5,
        static_cast<double>(grouped ? model.categories.size() : values.size()) - 0.5,
        minimum_count,
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
    finalize_mapper(context, model,
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
        for (const double value : finite_values(series)) {
            all_values.append(value);
        }
    }
    for (const ChartSeries& series : model.series) {
        if (!chart_series_is_visible(series.style)) continue;
        for (const double value : finite_values(series.values)) {
            all_values.append(value);
        }
    }
    if (all_values.isEmpty()) {
        return context;
    }
    auto [minimum_it, maximum_it] = std::minmax_element(all_values.cbegin(), all_values.cend());
    double minimum = *minimum_it;
    double maximum = *maximum_it;
    const double padding = aggregate_range_padding(minimum, maximum);
    context.plot = chart_plot_area(area, ChartKind::BoxPlot);
    context.mapper = ChartCoordinateMapper(context.plot);
    finalize_mapper(context, model,
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
        for (const double value : finite_values(series)) {
            all_values.append(value);
        }
    }
    for (const ChartSeries& series : model.series) {
        if (!chart_series_is_visible(series.style)) continue;
        for (const double value : finite_values(series.values)) {
            all_values.append(value);
        }
    }
    if (all_values.isEmpty()) {
        return context;
    }
    auto [minimum_it, maximum_it] = std::minmax_element(all_values.cbegin(), all_values.cend());
    const double padding = aggregate_range_padding(*minimum_it, *maximum_it);
    context.plot = chart_plot_area(area, ChartKind::Violin);
    context.mapper = ChartCoordinateMapper(context.plot);
    finalize_mapper(context, model,
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
    finalize_mapper(context, model,
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
    finalize_mapper(context, model, x_min, x_max, 0.0, 1.05);
    context.valid = true;
    return context;
}

ChartPlotContext build_contour_plot_context(const ChartModel& model, const QRectF& area)
{
    ChartPlotContext context;
    if (model.contour_x.size() < 2 || model.contour_y.size() < 2) return context;
    context.plot = chart_plot_area(area, model.kind);
    context.mapper = ChartCoordinateMapper(context.plot);
    finalize_mapper(context, model, model.contour_x.front(), model.contour_x.back(),
                    model.contour_y.front(), model.contour_y.back());
    context.valid = context.plot.width() > 1.0 && context.plot.height() > 1.0;
    return context;
}

ChartPlotContext build_parallel_plot_context(const ChartModel& model, const QRectF& area)
{
    ChartPlotContext context;
    if (model.matrix_labels.size() < 2 || model.matrix_values.empty()) return context;
    context.plot = chart_plot_area(area, model.kind);
    context.mapper = ChartCoordinateMapper(context.plot);
    finalize_mapper(context, model, 0.0, static_cast<double>(model.matrix_labels.size() - 1), 0.0, 1.0);
    context.valid = context.plot.width() > 1.0 && context.plot.height() > 1.0;
    return context;
}

std::optional<QPointF> chart_parallel_point(
    const ChartModel& model, const ChartPlotContext& context, std::size_t row, std::size_t axis)
{
    if (!context.valid || row >= model.matrix_values.size() || axis >= model.matrix_labels.size()
        || axis >= model.matrix_values[row].size() || !std::isfinite(model.matrix_values[row][axis])) return {};
    const double minimum = axis < model.lower.size() ? model.lower[axis] : 0.0;
    const double maximum = axis < model.upper.size() ? model.upper[axis] : 1.0;
    if (!std::isfinite(minimum) || !std::isfinite(maximum)) return {};
    const double fraction = maximum > minimum
        ? std::clamp((model.matrix_values[row][axis] - minimum) / (maximum - minimum), 0.0, 1.0) : 0.5;
    return context.mapper.to_pixel(static_cast<double>(axis), fraction);
}

static std::optional<QPointF> surface_projection(
    const ChartModel& model, double z_min, double z_max, std::size_t row, std::size_t column)
{
    if (row >= model.contour_y.size() || column >= model.contour_x.size()
        || row >= model.matrix_values.size() || column >= model.matrix_values[row].size()) return {};
    const double z = model.matrix_values[row][column];
    if (!std::isfinite(z) || !std::isfinite(model.contour_x[column])
        || !std::isfinite(model.contour_y[row])) return {};
    const double dx = model.contour_x.back() - model.contour_x.front();
    const double dy = model.contour_y.back() - model.contour_y.front();
    if (!(dx > 0.0) || !(dy > 0.0) || !(z_max > z_min)) return {};
    const double x = (model.contour_x[column] - model.contour_x.front()) / dx;
    const double y = (model.contour_y[row] - model.contour_y.front()) / dy;
    return QPointF((x - y) * 0.8660254037844386, (x + y) * 0.5 + (z - z_min) / (z_max - z_min) * 0.65);
}

ChartPlotContext build_surface_plot_context(const ChartModel& model, const QRectF& area)
{
    ChartPlotContext context;
    if (model.contour_x.size() < 2 || model.contour_y.size() < 2) return context;
    context.surface_z_min = std::numeric_limits<double>::infinity();
    context.surface_z_max = -std::numeric_limits<double>::infinity();
    for (std::size_t row = 0; row < std::min(model.contour_y.size(), model.matrix_values.size()); ++row)
        for (std::size_t column = 0; column < std::min(model.contour_x.size(), model.matrix_values[row].size()); ++column) {
            const double z = model.matrix_values[row][column];
            if (!std::isfinite(z)) continue;
            context.surface_z_min = std::min(context.surface_z_min, z);
            context.surface_z_max = std::max(context.surface_z_max, z);
        }
    if (!std::isfinite(context.surface_z_min) || !std::isfinite(context.surface_z_max)) return context;
    if (context.surface_z_min == context.surface_z_max) { context.surface_z_min -= 1.0; context.surface_z_max += 1.0; }
    double u_min = std::numeric_limits<double>::infinity(), u_max = -u_min;
    double v_min = u_min, v_max = -u_min;
    for (std::size_t row = 0; row < model.contour_y.size(); ++row)
        for (std::size_t column = 0; column < model.contour_x.size(); ++column) {
            const auto point = surface_projection(model, context.surface_z_min, context.surface_z_max, row, column);
            if (!point.has_value()) continue;
            u_min = std::min(u_min, point->x()); u_max = std::max(u_max, point->x());
            v_min = std::min(v_min, point->y()); v_max = std::max(v_max, point->y());
        }
    if (!std::isfinite(u_min) || !std::isfinite(v_min)) return context;
    context.plot = chart_plot_area(area, model.kind);
    context.mapper = ChartCoordinateMapper(context.plot);
    finalize_mapper(context, model, u_min, u_max, v_min, v_max);
    context.valid = context.plot.width() > 1.0 && context.plot.height() > 1.0;
    return context;
}

std::optional<QPointF> chart_surface_point(
    const ChartModel& model, const ChartPlotContext& context, std::size_t row, std::size_t column)
{
    const auto point = surface_projection(model, context.surface_z_min, context.surface_z_max, row, column);
    return context.valid && point.has_value()
        ? std::optional<QPointF>(context.mapper.to_pixel(point->x(), point->y())) : std::nullopt;
}

ChartPlotContext build_chart_plot_context(const ChartModel& model, const QRectF& area)
{
    switch (model.kind) {
    case ChartKind::Control: return build_control_plot_context(model, area);
    case ChartKind::Histogram: return build_histogram_plot_context(model, area);
    case ChartKind::Pareto: case ChartKind::Bar: return build_pareto_plot_context(model, area);
    case ChartKind::BoxPlot: return build_box_plot_context(model, area);
    case ChartKind::Violin: return build_violin_plot_context(model, area);
    case ChartKind::Interval: return build_interval_plot_context(model, area);
    case ChartKind::Ecdf: return build_ecdf_plot_context(model, area);
    case ChartKind::Probability: return build_probability_plot_context(model, area);
    case ChartKind::Bubble: return build_xy_plot_context(model, area, 0.05, false);
    case ChartKind::Marginal: {
        const auto layout = chart_marginal_plot_layout(area);
        return layout.valid ? build_scatter_plot_context(model, layout.scatter_canvas)
                            : ChartPlotContext{};
    }
    case ChartKind::Parallel: return build_parallel_plot_context(model, area);
    case ChartKind::Contour: case ChartKind::Hexbin: return build_contour_plot_context(model, area);
    case ChartKind::Surface: return build_surface_plot_context(model, area);
    case ChartKind::Scatter: case ChartKind::TimeSeries: case ChartKind::Area: case ChartKind::Density:
        return build_scatter_plot_context(model, area);
    default: return {};
    }
}

static void finalize_grid(ChartMatrixGridLayout& grid)
{
    const auto visible_range = [](double start, double end, double origin, double cell, std::size_t count) {
        const double first = std::clamp(std::floor((start - origin) / cell), 0.0, static_cast<double>(count));
        const double last = std::clamp(std::ceil((end - origin) / cell), first, static_cast<double>(count));
        return std::make_pair(static_cast<std::size_t>(first), static_cast<std::size_t>(last - first));
    };
    const auto columns = visible_range(grid.viewport.left(), grid.viewport.right(), grid.origin.x(), grid.cell_width, grid.column_count);
    const auto rows = visible_range(grid.viewport.top(), grid.viewport.bottom(), grid.origin.y(), grid.cell_height, grid.row_count);
    grid.first_column = columns.first; grid.visible_column_count = columns.second;
    grid.first_row = rows.first; grid.visible_row_count = rows.second;
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
                 / static_cast<double>(count + 1))) * std::clamp(model.view.zoom_factor, 0.25, 20.0);
    layout.viewport = chart_plot_area(area, ChartKind::Correlation);
    layout.origin = QPointF(area.left() + 120.0, area.top() + 52.0)
        + chart_pan_offset(model, layout.viewport);
    layout.cell_width = cell;
    layout.cell_height = cell;
    layout.column_count = count;
    layout.row_count = count;
    if (layout.viewport.width() <= 1.0 || layout.viewport.height() <= 1.0) return layout;
    finalize_grid(layout);
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
    const double zoom = std::clamp(model.view.zoom_factor, 0.25, 20.0);
    layout.cell_width = std::max(18.0, std::min(
        72.0, (area.width() - 140.0) / static_cast<double>(columns.size()))) * zoom;
    layout.cell_height = std::max(18.0, std::min(
        48.0, (area.height() - 90.0) / static_cast<double>(rows.size()))) * zoom;
    layout.viewport = chart_plot_area(area, ChartKind::Heatmap);
    layout.origin = QPointF(area.left() + 110.0, area.top() + 52.0)
        + chart_pan_offset(model, layout.viewport);
    layout.column_count = columns.size();
    layout.row_count = rows.size();
    if (layout.viewport.width() <= 1.0 || layout.viewport.height() <= 1.0) return layout;
    finalize_grid(layout);
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
    const double zoom = std::clamp(model.view.zoom_factor, 0.25, 20.0);
    const double cell = std::max(18.0, std::min(area.width(), area.height())
        / static_cast<double>(count + 0.4) * zoom);
    layout.viewport = chart_plot_area(area, ChartKind::Matrix);
    layout.origin = QPointF(area.left() + 36.0, area.top() + 40.0)
        + chart_pan_offset(model, layout.viewport);
    layout.cell_width = cell;
    layout.cell_height = cell;
    layout.column_count = count;
    layout.row_count = count;
    if (layout.viewport.width() <= 1.0 || layout.viewport.height() <= 1.0) return layout;
    finalize_grid(layout);
    layout.valid = true;
    return layout;
}

double chart_hit_tolerance_pixels(const ChartModel& model, double base_tolerance)
{
    Q_UNUSED(model)
    return std::max(1.0, base_tolerance);
}

std::optional<std::size_t> chart_hit_test_point_series(
    const ChartModel& model,
    const QRectF& area,
    const QPoint& position,
    double base_tolerance)
{
    ChartPlotContext context;
    context = build_chart_plot_context(model, area);
    if (!context.valid || !context.plot.contains(position)) {
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
        const std::size_t count = xs.empty() ? ys.size() : std::min(xs.size(), ys.size());
        for (std::size_t index = 0; index < count; ++index) {
            if (!std::isfinite(x_at(index, xs)) || !std::isfinite(ys[index])) continue;
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
    if (!model.values.empty() && chart_series_is_visible(model.value_style)
        && (model.value_style.point_style != ChartPointStyle::None || line_visible)) {
        test_series(model.x_values, model.values, 0);
    }
    if (model.values.empty() && !model.series.empty()) {
        std::size_t flat_offset = 0;
        for (const ChartSeries& series : model.series) {
            if (chart_series_is_visible(series.style)) {
                test_series(series.x_values, series.values, flat_offset);
            }
            flat_offset += series.values.size();
        }
    }
    return result;
}
