#pragma once

#include <QPointF>
#include <QRectF>

class ChartCoordinateMapper final {
public:
    explicit ChartCoordinateMapper(const QRectF& plot_rect = {});

    void set_plot_rect(const QRectF& plot_rect);
    void set_data_range(double x_min, double x_max, double y_min, double y_max);
    void zoom(double factor, const QPointF& anchor);
    void pan(const QPointF& delta_pixels);

    QPointF to_pixel(double x, double y) const;
    QPointF to_data(const QPointF& pixel) const;

    double x_min() const;
    double x_max() const;
    double y_min() const;
    double y_max() const;

private:
    QRectF plot_rect_;
    double x_min_ = 0.0;
    double x_max_ = 1.0;
    double y_min_ = 0.0;
    double y_max_ = 1.0;
};
