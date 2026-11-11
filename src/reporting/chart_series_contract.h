#pragma once

#include "reporting/chart_model.h"

#include <algorithm>
#include <cmath>

namespace datalab::reporting {

inline double chart_series_opacity(const ChartSeriesStyle& style)
{
    return std::isfinite(style.opacity) ? std::clamp(style.opacity, 0.0, 1.0) : 0.0;
}

// Painting, ranges, hit testing and brushing must agree on transparent series.
inline bool chart_series_is_visible(const ChartSeriesStyle& style)
{
    return style.visible && chart_series_opacity(style) > 0.0;
}

// Some multi-variable time charts flatten their series into values solely for
// source-row linking. Exact coordinate equality identifies that existing contract.
bool chart_primary_mirrors_series(const ChartModel& model);

} // namespace datalab::reporting
