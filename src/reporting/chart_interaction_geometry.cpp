#include "reporting/chart_interaction.h"
#include "reporting/chart_plot_context.h"
#include "reporting/chart_series_contract.h"

#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <set>

namespace chart_interaction {
namespace {

using datalab::reporting::chart_series_is_visible;

enum class Shape { point, rectangle, segment, path };
struct Mark {
    Hit hit;
    Shape shape = Shape::point;
    QRectF viewport;
    QRectF bounds;
    QLineF segment;
    QPainterPath path;
};
using Visitor = std::function<void(const Mark&)>;

bool finite_point(const QPointF& point)
{
    return std::isfinite(point.x()) && std::isfinite(point.y());
}

void point_mark(const Visitor& visitor, Hit hit, const QPointF& point, double radius, const QRectF& viewport)
{
    if (!finite_point(point) || !std::isfinite(radius)) return;
    Mark mark;
    mark.hit = hit; mark.viewport = viewport;
    radius = std::max(1.0, radius);
    mark.bounds = QRectF(point.x() - radius, point.y() - radius, 2.0 * radius, 2.0 * radius);
    visitor(mark);
}

void rectangle_mark(const Visitor& visitor, Hit hit, const QRectF& bounds, const QRectF& viewport)
{
    if (!finite_point(bounds.topLeft()) || !finite_point(bounds.bottomRight())) return;
    Mark mark;
    mark.hit = hit; mark.shape = Shape::rectangle; mark.bounds = bounds.normalized(); mark.viewport = viewport;
    visitor(mark);
}

void segment_mark(const Visitor& visitor, Hit hit, const QPointF& first, const QPointF& second, const QRectF& viewport)
{
    if (!finite_point(first) || !finite_point(second)) return;
    Mark mark;
    mark.hit = hit; mark.shape = Shape::segment; mark.segment = QLineF(first, second);
    mark.bounds = QRectF(first, second).normalized(); mark.viewport = viewport;
    visitor(mark);
}

void path_mark(const Visitor& visitor, Hit hit, const QPainterPath& path, const QRectF& viewport)
{
    Mark mark;
    mark.hit = hit; mark.shape = Shape::path; mark.path = path;
    mark.bounds = path.boundingRect(); mark.viewport = viewport;
    visitor(mark);
}

ChartMatrixGridLayout matrix_layout(const ChartModel& model, const QRectF& area)
{
    if (model.kind == ChartKind::Correlation) return build_correlation_grid_layout(model, area);
    if (model.kind == ChartKind::Heatmap) return build_heatmap_grid_layout(model, area);
    return build_matrix_grid_layout(model, area);
}

void visit_marks(const ChartModel& model, const QRectF& area, const Visitor& visitor,
                 const std::optional<QRectF>& query = std::nullopt)
{
    if (model.kind == ChartKind::Pie) {
        double total = 0.0;
        for (double value : model.category_values) if (std::isfinite(value) && value > 0.0) total += value;
        if (!(total > 0.0)) return;
        const double size = std::min(area.width(), area.height()) * 0.55;
        const QRectF pie(area.center().x() - size / 2.0, area.center().y() - size / 2.2, size, size);
        double start = 90.0;
        for (std::size_t i = 0; i < model.category_values.size(); ++i) {
            const double value = model.category_values[i];
            if (!std::isfinite(value) || value <= 0.0) continue;
            const double span = -360.0 * value / total;
            QPainterPath path;
            path.moveTo(pie.center()); path.arcTo(pie, start, span); path.closeSubpath();
            path_mark(visitor, {HitKind::Slice, i}, path, area);
            start += span;
        }
        return;
    }
    if (model.kind == ChartKind::Matrix || model.kind == ChartKind::Heatmap || model.kind == ChartKind::Correlation) {
        const auto grid = matrix_layout(model, area);
        if (!grid.valid) return;
        for (std::size_t row = grid.first_row; row < grid.first_row + grid.visible_row_count; ++row)
            for (std::size_t column = grid.first_column; column < grid.first_column + grid.visible_column_count; ++column) {
                const double gap = model.kind == ChartKind::Matrix ? 4.0 : 2.0;
                const QRectF cell(grid.origin.x() + column * grid.cell_width, grid.origin.y() + row * grid.cell_height,
                                  grid.cell_width - gap, grid.cell_height - gap);
                if (query.has_value() && !cell.intersected(grid.viewport).intersects(*query)) continue;
                if (model.kind != ChartKind::Matrix) {
                    if (row >= model.matrix_values.size() || column >= model.matrix_values[row].size()
                        || !std::isfinite(model.matrix_values[row][column])) continue;
                    Hit hit{HitKind::Cell, row * grid.column_count + column};
                    hit.matrix_row = row; hit.matrix_column = column;
                    rectangle_mark(visitor, hit, cell, grid.viewport);
                    continue;
                }
                if (row == column || row >= model.matrix_values.size() || column >= model.matrix_values.size()) continue;
                const auto& xs = model.matrix_values[column];
                const auto& ys = model.matrix_values[row];
                const std::size_t count = std::min(xs.size(), ys.size());
                double x_min = std::numeric_limits<double>::infinity(), x_max = -x_min;
                double y_min = x_min, y_max = -x_min;
                for (std::size_t i = 0; i < count; ++i) if (std::isfinite(xs[i]) && std::isfinite(ys[i])) {
                    x_min = std::min(x_min, xs[i]); x_max = std::max(x_max, xs[i]);
                    y_min = std::min(y_min, ys[i]); y_max = std::max(y_max, ys[i]);
                }
                if (!std::isfinite(x_min)) continue;
                ChartCoordinateMapper mapper(cell.adjusted(4.0, 4.0, -4.0, -4.0));
                mapper.set_data_range(x_min, x_min == x_max ? x_min + 1.0 : x_max,
                                      y_min, y_min == y_max ? y_min + 1.0 : y_max);
                for (std::size_t i = 0; i < count; ++i) {
                    Hit hit{HitKind::MatrixPoint, i}; hit.matrix_row = row; hit.matrix_column = column;
                    point_mark(visitor, hit, mapper.to_pixel(xs[i], ys[i]), 1.8, cell.intersected(grid.viewport));
                }
            }
        return;
    }
    const ChartPlotContext context = build_chart_plot_context(model, area);
    if (!context.valid || context.plot.width() <= 1.0 || context.plot.height() <= 1.0) return;
    const auto& mapper = context.mapper;
    const auto& plot = context.plot;
    if (model.kind == ChartKind::Histogram) {
        const std::size_t count = std::min(model.histogram_counts.size(), model.histogram_edges.size() - 1);
        for (std::size_t i = 0; i < count; ++i) {
            if (!(model.histogram_counts[i] > 0.0)) continue;
            rectangle_mark(visitor, {HitKind::Bar, i}, QRectF(mapper.to_pixel(model.histogram_edges[i], model.histogram_counts[i]),
                mapper.to_pixel(model.histogram_edges[i + 1], 0.0)), plot);
        }
        return;
    }
    if (model.kind == ChartKind::Bar || model.kind == ChartKind::Pareto) {
        const bool grouped = !model.series.empty() && !model.categories.empty();
        if (grouped) {
            const double cluster = std::min(48.0, plot.width() / model.categories.size() * 0.7);
            const double width = cluster / model.series.size() * 0.85;
            for (std::size_t category = 0; category < model.categories.size(); ++category)
                for (std::size_t s = 0; s < model.series.size(); ++s) {
                    const auto& series = model.series[s];
                    if (!chart_series_is_visible(series.style) || category >= series.values.size() || !std::isfinite(series.values[category])) continue;
                    const double slot = category + (static_cast<double>(s) - (model.series.size() - 1) / 2.0) * (width / cluster);
                    const QPointF top = mapper.to_pixel(slot, series.values[category]);
                    const QPointF bottom = mapper.to_pixel(slot, 0.0);
                    Hit hit{HitKind::Bar, category}; hit.series_index = s; hit.sample_index = category;
                    rectangle_mark(visitor, hit, QRectF(top + QPointF(-width / 2.0, 0.0), bottom + QPointF(width / 2.0, 0.0)), plot);
                }
        } else {
            const auto& values = model.category_values.empty() ? model.values : model.category_values;
            if (values.empty()) return;
            const double width = std::min(36.0, plot.width() / values.size() * 0.6);
            for (std::size_t i = 0; i < values.size(); ++i) {
                const QPointF top = mapper.to_pixel(static_cast<double>(i), values[i]);
                const QPointF bottom = mapper.to_pixel(static_cast<double>(i), 0.0);
                rectangle_mark(visitor, {HitKind::Bar, i}, QRectF(top + QPointF(-width / 2.0, 0.0), bottom + QPointF(width / 2.0, 0.0)), plot);
            }
        }
        return;
    }
    if (model.kind == ChartKind::BoxPlot || model.kind == ChartKind::Violin) {
        const std::size_t count = std::min({model.box_min.size(), model.box_q1.size(), model.box_median.size(), model.box_q3.size(), model.box_max.size()});
        if (count == 0) return;
        const double width = model.kind == ChartKind::Violin ? std::min(18.0, plot.width() / count * 0.15)
                                                             : std::min(40.0, plot.width() / count * 0.4);
        for (std::size_t i = 0; i < count; ++i) {
            Hit hit{HitKind::Box, i};
            const QPointF q1 = mapper.to_pixel(i, model.box_q1[i]), q3 = mapper.to_pixel(i, model.box_q3[i]);
            rectangle_mark(visitor, hit, QRectF(q3 + QPointF(-width / 2.0, 0.0), q1 + QPointF(width / 2.0, 0.0)), plot);
            segment_mark(visitor, hit, mapper.to_pixel(i, model.box_min[i]), mapper.to_pixel(i, model.box_max[i]), plot);
            if (model.kind == ChartKind::BoxPlot) {
                for (double value : {model.box_min[i], model.box_max[i]}) {
                    const QPointF point = mapper.to_pixel(i, value);
                    segment_mark(visitor, hit, point - QPointF(8.0, 0.0), point + QPointF(8.0, 0.0), plot);
                }
            }
            if (model.kind == ChartKind::Violin && i < model.series.size() && chart_series_is_visible(model.series[i].style)) {
                const auto& density = model.series[i];
                const std::size_t n = std::min(density.x_values.size(), density.values.size());
                if (n < 2) continue;
                QPainterPath path;
                for (std::size_t j = 0; j < n; ++j) {
                    const QPointF point = mapper.to_pixel(i - 0.35 * density.x_values[j], density.values[j]);
                    if (j == 0) path.moveTo(point); else path.lineTo(point);
                }
                for (std::size_t j = n; j-- > 0;) path.lineTo(mapper.to_pixel(i + 0.35 * density.x_values[j], density.values[j]));
                path.closeSubpath(); path_mark(visitor, hit, path, plot);
            }
        }
        if (model.kind == ChartKind::BoxPlot) for (const auto& series : model.series) {
            if (!chart_series_is_visible(series.style)) continue;
            for (std::size_t j = 0; j < std::min(series.x_values.size(), series.values.size()); ++j) {
                if (!std::isfinite(series.x_values[j]) || series.x_values[j] < -0.5
                    || series.x_values[j] >= static_cast<double>(count) - 0.5) continue;
                const long long group = std::llround(series.x_values[j]);
                if (group >= 0 && static_cast<std::size_t>(group) < count)
                    point_mark(visitor, {HitKind::Box, static_cast<std::size_t>(group)}, mapper.to_pixel(series.x_values[j], series.values[j]), 3.0, plot);
            }
        }
        return;
    }
    if (model.kind == ChartKind::Interval) {
        const std::size_t count = std::min({model.values.size(), model.interval_lower.size(), model.interval_upper.size()});
        for (std::size_t i = 0; i < count; ++i) {
            const Hit hit{HitKind::Interval, i};
            segment_mark(visitor, hit, mapper.to_pixel(i, model.interval_lower[i]), mapper.to_pixel(i, model.interval_upper[i]), plot);
            for (double value : {model.interval_lower[i], model.interval_upper[i]}) {
                const auto point = mapper.to_pixel(i, value);
                segment_mark(visitor, hit, point - QPointF(5.0, 0.0), point + QPointF(5.0, 0.0), plot);
            }
            point_mark(visitor, hit, mapper.to_pixel(i, model.values[i]), 4.0, plot);
        }
        return;
    }
    if (model.kind == ChartKind::Contour || model.kind == ChartKind::Hexbin) {
        const std::size_t columns = model.contour_x.size() - 1;
        for (std::size_t row = 0; row + 1 < model.contour_y.size() && row < model.matrix_values.size(); ++row)
            for (std::size_t column = 0; column < columns && column < model.matrix_values[row].size(); ++column) {
                if (!std::isfinite(model.matrix_values[row][column])) continue;
                rectangle_mark(visitor, {HitKind::ContourCell, row * columns + column},
                    QRectF(mapper.to_pixel(model.contour_x[column], model.contour_y[row + 1]),
                           mapper.to_pixel(model.contour_x[column + 1], model.contour_y[row])), plot);
            }
        return;
    }
    if (model.kind == ChartKind::Parallel) {
        for (std::size_t row = 0; row < model.matrix_values.size(); ++row) {
            std::optional<QPointF> previous;
            for (std::size_t axis = 0; axis < model.matrix_labels.size(); ++axis) {
                const auto point = chart_parallel_point(model, context, row, axis);
                if (previous.has_value() && point.has_value()) segment_mark(visitor, {HitKind::ParallelObservation, row}, *previous, *point, plot);
                previous = point;
            }
        }
        return;
    }
    if (model.kind == ChartKind::Surface) {
        for (std::size_t row = 0; row < model.contour_y.size(); ++row)
            for (std::size_t column = 0; column < model.contour_x.size(); ++column) {
                const auto point = chart_surface_point(model, context, row, column);
                if (!point.has_value()) continue;
                Hit hit{HitKind::SurfacePoint, row * model.contour_x.size() + column};
                hit.matrix_row = row; hit.matrix_column = column;
                point_mark(visitor, hit, *point, 3.0, plot);
            }
        return;
    }
    double size_min = std::numeric_limits<double>::infinity(), size_max = -size_min;
    if (model.kind == ChartKind::Bubble) for (double size : model.bubble_sizes) if (std::isfinite(size)) {
        size_min = std::min(size_min, size); size_max = std::max(size_max, size);
    }
    std::size_t count = model.values.size();
    if (!model.x_values.empty() && model.kind != ChartKind::Control) count = std::min(count, model.x_values.size());
    if (model.kind == ChartKind::Bubble) count = std::min(count, model.bubble_sizes.size());
    const bool line_kind = model.kind == ChartKind::Control || model.kind == ChartKind::TimeSeries
        || model.kind == ChartKind::Area || model.kind == ChartKind::Density || model.kind == ChartKind::Ecdf;
    if (chart_series_is_visible(model.value_style) && !context.primary_mirrors_series
        && (model.value_style.point_style != ChartPointStyle::None
                                     || (line_kind && model.value_style.line_width > 0.0))) {
        for (std::size_t i = 0; i < count; ++i) {
            double radius = std::clamp(model.value_style.point_size, 1.0, 12.0);
            if (model.kind == ChartKind::Bubble) {
                if (!std::isfinite(model.bubble_sizes[i])) continue;
                const double fraction = size_max > size_min ? (model.bubble_sizes[i] - size_min) / (size_max - size_min) : 0.5;
                radius = 4.0 + std::sqrt(std::clamp(fraction, 0.0, 1.0)) * 18.0;
            }
            point_mark(visitor, {HitKind::Point, i}, mapper.to_pixel(i < model.x_values.size() ? model.x_values[i] : static_cast<double>(i), model.values[i]), radius, plot);
        }
    }
    if (model.kind == ChartKind::Probability || model.kind == ChartKind::Bubble || model.kind == ChartKind::Ecdf) return;
    std::size_t offset = model.values.size();
    for (std::size_t s = 0; s < model.series.size(); ++s) {
        const auto& series = model.series[s];
        if (chart_series_is_visible(series.style)
            && ((series.show_points && series.style.point_style != ChartPointStyle::None)
                || series.style.line_width > 0.0))
            for (std::size_t i = 0; i < series.values.size(); ++i) {
                Hit hit{HitKind::SeriesPoint, offset + i}; hit.series_index = s; hit.sample_index = i;
                point_mark(visitor, hit, mapper.to_pixel(i < series.x_values.size() ? series.x_values[i] : static_cast<double>(i), series.values[i]),
                           std::clamp(series.style.point_size, 1.0, 12.0), plot);
            }
        offset += series.values.size();
    }
    if (model.kind == ChartKind::Control) {
        const std::vector<double>* lines[] = {&model.center, &model.lower, &model.upper};
        const ChartReferenceStyle styles[] = {model.center_style, model.lower_style, model.upper_style};
        for (std::size_t line = 0; line < 3; ++line) {
            if (!styles[line].visible) continue;
            for (std::size_t i = 1; i < std::min(lines[line]->size(), model.values.size()); ++i) {
                Hit hit{HitKind::ReferenceLine, line}; hit.sample_index = i;
                segment_mark(visitor, hit,
                    mapper.to_pixel(i - 1 < model.x_values.size() ? model.x_values[i - 1] : static_cast<double>(i - 1), (*lines[line])[i - 1]),
                    mapper.to_pixel(i < model.x_values.size() ? model.x_values[i] : static_cast<double>(i), (*lines[line])[i]), plot);
            }
        }
    }
}

double segment_distance(const QLineF& line, const QPointF& point)
{
    const QPointF delta = line.p2() - line.p1();
    const double squared = QPointF::dotProduct(delta, delta);
    const double t = squared > 0.0 ? std::clamp(QPointF::dotProduct(point - line.p1(), delta) / squared, 0.0, 1.0) : 0.0;
    return QLineF(line.p1() + t * delta, point).length();
}

double distance(const Mark& mark, const QPointF& point)
{
    if (mark.shape == Shape::point) return std::max(0.0, QLineF(mark.bounds.center(), point).length() - mark.bounds.width() / 2.0);
    if (mark.shape == Shape::segment) return segment_distance(mark.segment, point);
    if (mark.shape == Shape::path) {
        if (mark.path.contains(point)) return 0.0;
        QPainterPathStroker stroker; stroker.setWidth(8.0);
        return stroker.createStroke(mark.path).contains(point) ? 3.0 : std::numeric_limits<double>::infinity();
    }
    const QPointF nearest(std::clamp(point.x(), mark.bounds.left(), mark.bounds.right()),
                          std::clamp(point.y(), mark.bounds.top(), mark.bounds.bottom()));
    return QLineF(nearest, point).length();
}

int point_priority(const ChartModel& model, const Hit& hit)
{
    if (hit.kind == HitKind::Point) return 2;
    if (hit.kind == HitKind::SeriesPoint && hit.series_index.has_value()
        && *hit.series_index < model.series.size()) {
        const ChartSeries& series = model.series[*hit.series_index];
        if (series.role == ChartSeriesRole::Actual
            && series.values.size() == model.source_rows.size()) return 3;
        return series.role == ChartSeriesRole::Generic ? 2 : 1;
    }
    return 2;
}

bool intersects(const Mark& mark, const QRectF& selection)
{
    const QRectF clipped = selection.intersected(mark.viewport);
    if (clipped.isEmpty()) return false;
    if (mark.shape == Shape::point) return clipped.contains(mark.bounds.center());
    if (mark.shape == Shape::rectangle) return clipped.intersects(mark.bounds);
    QPainterPath box; box.addRect(clipped);
    if (mark.shape == Shape::path) return mark.path.intersects(box);
    QPainterPath line; line.moveTo(mark.segment.p1()); line.lineTo(mark.segment.p2());
    QPainterPathStroker stroker; stroker.setWidth(2.0);
    return stroker.createStroke(line).intersects(box);
}
} // namespace

std::optional<Hit> hit_test(const ChartModel& model, const QRectF& area, const QPointF& position)
{
    std::optional<Hit> best;
    double best_distance = 6.0;
    visit_marks(model, area, [&](const Mark& mark) {
        if (!mark.viewport.contains(position)) return;
        if (mark.hit.kind == HitKind::Cell && !mark.bounds.contains(position)) return;
        double gap = distance(mark, position);
        if (mark.hit.kind == HitKind::ReferenceLine) gap += 1.0;
        if (gap < best_distance || (gap == best_distance
            && (!best.has_value() || point_priority(model, mark.hit) >= point_priority(model, *best)))) {
            best_distance = gap;
            best = mark.hit;
        }
    }, QRectF(position - QPointF(6.0, 6.0), QSizeF(12.0, 12.0)));
    return best;
}

std::vector<std::size_t> select_rectangle(const ChartModel& model, const QRectF& area, const QRectF& selection)
{
    std::set<std::size_t> selected;
    visit_marks(model, area, [&](const Mark& mark) {
        if (mark.hit.kind != HitKind::ReferenceLine && intersects(mark, selection.normalized())) selected.insert(mark.hit.index);
    }, selection.normalized());
    return {selected.begin(), selected.end()};
}

bool supports_navigation(ChartKind kind) { return kind != ChartKind::Pie; }

void paint_selection(QPainter& painter, const ChartModel& model, const QRectF& area)
{
    // Matrix renderer already includes selected observations in its downsampled
    // point set. Avoid a second complete scan through every visible matrix cell.
    if (model.kind == ChartKind::Matrix) return;
    if (model.view.selected_points.empty() && !model.view.hovered_point.has_value()) return;
    const std::set<std::size_t> selected(model.view.selected_points.begin(), model.view.selected_points.end());
    visit_marks(model, area, [&](const Mark& mark) {
        if (mark.hit.kind == HitKind::ReferenceLine) return;
        const bool hovered = model.view.hovered_point == mark.hit.index;
        if (!hovered && selected.count(mark.hit.index) == 0) return;
        painter.save();
        painter.setClipRect(mark.viewport, Qt::IntersectClip);
        painter.setPen(QPen(hovered ? QColor("#1565c0") : QColor("#e67e22"), 2.0));
        painter.setBrush(Qt::NoBrush);
        if (mark.shape == Shape::point) painter.drawEllipse(mark.bounds.adjusted(-3.0, -3.0, 3.0, 3.0));
        else if (mark.shape == Shape::rectangle) painter.drawRect(mark.bounds);
        else if (mark.shape == Shape::segment) painter.drawLine(mark.segment);
        else painter.drawPath(mark.path);
        painter.restore();
    });
}

bool zoom_at(ChartModel& model, const QRectF& area, const QPointF& anchor, double factor)
{
    if (!supports_navigation(model.kind) || !std::isfinite(factor) || factor <= 0.0) return false;
    const auto old_grid = matrix_layout(model, area);
    const auto old_context = build_chart_plot_context(model, area);
    const bool grid_kind = model.kind == ChartKind::Matrix || model.kind == ChartKind::Heatmap || model.kind == ChartKind::Correlation;
    if ((grid_kind && (!old_grid.valid || !old_grid.viewport.contains(anchor)))
        || (!grid_kind && (!old_context.valid || !old_context.plot.contains(anchor)))) return false;
    const double next_zoom = std::clamp(model.view.zoom_factor * factor, 0.25, 20.0);
    if (next_zoom == model.view.zoom_factor) return false;
    if (grid_kind) {
        const QPointF cell((anchor.x() - old_grid.origin.x()) / old_grid.cell_width,
                            (anchor.y() - old_grid.origin.y()) / old_grid.cell_height);
        model.view.zoom_factor = next_zoom;
        const auto next = matrix_layout(model, area);
        chart_set_pan_offset(model, next.viewport, chart_pan_offset(model, next.viewport)
            + anchor - QPointF(next.origin.x() + cell.x() * next.cell_width, next.origin.y() + cell.y() * next.cell_height));
    } else {
        const QPointF data = old_context.mapper.to_data(anchor);
        model.view.zoom_factor = next_zoom;
        const auto next = build_chart_plot_context(model, area);
        chart_set_pan_offset(model, next.plot, chart_pan_offset(model, next.plot)
            + anchor - next.mapper.to_pixel(data.x(), data.y()));
    }
    return true;
}

bool zoom_to_rectangle(ChartModel& model, const QRectF& area, const QRectF& selection)
{
    const bool grid_kind = model.kind == ChartKind::Matrix || model.kind == ChartKind::Heatmap || model.kind == ChartKind::Correlation;
    if (grid_kind) {
        const auto grid = matrix_layout(model, area);
        if (!grid.valid) return false;
        const auto clipped = selection.normalized().intersected(grid.viewport);
        if (clipped.width() < 4.0 || clipped.height() < 4.0) return false;
        const auto center = clipped.center();
        zoom_at(model, area, center, std::min(grid.viewport.width() / clipped.width(), grid.viewport.height() / clipped.height()));
        chart_set_pan_offset(model, grid.viewport, chart_pan_offset(model, grid.viewport) + grid.viewport.center() - center);
        return true;
    }
    const auto context = build_chart_plot_context(model, area);
    if (!context.valid || !supports_navigation(model.kind)) return false;
    const QRectF clipped = selection.normalized().intersected(context.plot);
    if (clipped.width() < 4.0 || clipped.height() < 4.0) return false;
    const QPointF first = context.mapper.to_data(clipped.topLeft());
    const QPointF second = context.mapper.to_data(clipped.bottomRight());
    model.x_min = std::min(first.x(), second.x()); model.x_max = std::max(first.x(), second.x());
    model.y_min = std::min(first.y(), second.y()); model.y_max = std::max(first.y(), second.y());
    model.view.zoom_factor = 1.0; model.view.pan_offset = {}; model.view.pan_reference_size = {};
    return true;
}
} // namespace chart_interaction
