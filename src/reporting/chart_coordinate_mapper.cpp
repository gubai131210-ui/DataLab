#include "chart_coordinate_mapper.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>

ChartCoordinateMapper::ChartCoordinateMapper(const QRectF& plot_rect)
    : plot_rect_(plot_rect)
{
}

void ChartCoordinateMapper::set_plot_rect(const QRectF& plot_rect)
{
    plot_rect_ = plot_rect;
}

void ChartCoordinateMapper::set_data_range(
    double x_min, double x_max, double y_min, double y_max)
{
    if (x_max <= x_min) {
        x_max = x_min + 1.0;
    }
    if (y_max <= y_min) {
        y_max = y_min + 1.0;
    }
    x_min_ = x_min;
    x_max_ = x_max;
    y_min_ = y_min;
    y_max_ = y_max;
}

void ChartCoordinateMapper::zoom(double factor, const QPointF& anchor)
{
    if (factor <= 0.0 || !qIsFinite(factor) || factor == 1.0
        || plot_rect_.width() <= 0.0 || plot_rect_.height() <= 0.0
        || !std::isfinite(anchor.x()) || !std::isfinite(anchor.y())) {
        return;
    }

    const QPointF anchor_data = to_data(anchor);
    const double new_x_range = (x_max_ - x_min_) / factor;
    const double new_y_range = (y_max_ - y_min_) / factor;
    const double x_ratio = plot_rect_.width() > 0.0
        ? (anchor.x() - plot_rect_.left()) / plot_rect_.width()
        : 0.5;
    const double y_ratio = plot_rect_.height() > 0.0
        ? (plot_rect_.bottom() - anchor.y()) / plot_rect_.height()
        : 0.5;

    const double x_min = anchor_data.x() - new_x_range * x_ratio;
    const double x_max = x_min + new_x_range;
    const double y_min = anchor_data.y() - new_y_range * y_ratio;
    const double y_max = y_min + new_y_range;
    // A zoom limit is determined by representable doubles, not by an absolute
    // data-unit floor. Keep the previous view if either axis would collapse.
    if (!std::isfinite(x_min) || !std::isfinite(x_max) || !(x_min < x_max)
        || !std::isfinite(y_min) || !std::isfinite(y_max) || !(y_min < y_max)) {
        return;
    }
    x_min_ = x_min;
    x_max_ = x_max;
    y_min_ = y_min;
    y_max_ = y_max;
}

void ChartCoordinateMapper::pan(const QPointF& delta_pixels)
{
    if (plot_rect_.width() <= 0.0 || plot_rect_.height() <= 0.0) {
        return;
    }
    if (!std::isfinite(delta_pixels.x()) || !std::isfinite(delta_pixels.y())) {
        return;
    }
    const double x_delta = delta_pixels.x() / plot_rect_.width() * (x_max_ - x_min_);
    const double y_delta = delta_pixels.y() / plot_rect_.height() * (y_max_ - y_min_);
    const double x_min = x_min_ - x_delta;
    const double x_max = x_max_ - x_delta;
    const double y_min = y_min_ + y_delta;
    const double y_max = y_max_ + y_delta;
    if (!std::isfinite(x_min) || !std::isfinite(x_max) || !(x_min < x_max)
        || !std::isfinite(y_min) || !std::isfinite(y_max) || !(y_min < y_max)) {
        return;
    }
    x_min_ = x_min;
    x_max_ = x_max;
    y_min_ = y_min;
    y_max_ = y_max;
}

QPointF ChartCoordinateMapper::to_pixel(double x, double y) const
{
    const double x_ratio = (x - x_min_) / (x_max_ - x_min_);
    const double y_ratio = (y - y_min_) / (y_max_ - y_min_);
    return QPointF(
        plot_rect_.left() + x_ratio * plot_rect_.width(),
        plot_rect_.bottom() - y_ratio * plot_rect_.height());
}

QPointF ChartCoordinateMapper::to_data(const QPointF& pixel) const
{
    const double x_ratio = (pixel.x() - plot_rect_.left()) / plot_rect_.width();
    const double y_ratio = (plot_rect_.bottom() - pixel.y()) / plot_rect_.height();
    return QPointF(
        x_min_ + x_ratio * (x_max_ - x_min_),
        y_min_ + y_ratio * (y_max_ - y_min_));
}

double ChartCoordinateMapper::x_min() const { return x_min_; }
double ChartCoordinateMapper::x_max() const { return x_max_; }
double ChartCoordinateMapper::y_min() const { return y_min_; }
double ChartCoordinateMapper::y_max() const { return y_max_; }
