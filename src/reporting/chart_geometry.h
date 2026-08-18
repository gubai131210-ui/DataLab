#pragma once

#include "reporting/chart_model.h"

#include <QRectF>

namespace chart_geometry {

inline QRectF plot_rect(const QRectF& area, const ChartKind kind)
{
    return kind == ChartKind::Pareto
        ? area.adjusted(64.0, 42.0, -120.0, -178.0)
        : area.adjusted(58.0, 42.0, -96.0, -48.0);
}

}  // namespace chart_geometry
