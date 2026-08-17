#pragma once

#include "ui/chart_model.h"

#include <QRectF>

class QPainter;

class ChartRenderer final {
public:
    static void render(QPainter& painter, const QRectF& area, const ChartModel& model);
};
