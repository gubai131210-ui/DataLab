#pragma once

#include "reporting/chart_model.h"

#include <QRectF>
#include <QSize>

class QPainter;
class QPixmap;

class ChartRenderer final {
public:
    static void render(QPainter& painter, const QRectF& area, const ChartModel& model);
    static QPixmap render_to_pixmap(
        const ChartModel& model,
        const QSize& size,
        qreal device_pixel_ratio = 1.0);
};
