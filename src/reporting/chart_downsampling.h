#pragma once

#include <QRectF>

#include <cstddef>
#include <vector>

namespace datalab::reporting {

// Returns source indices in draw order.  Each horizontal pixel bucket keeps
// the first, last, minimum-y and maximum-y observations, preserving spikes
// while bounding painter work for large time-series/scatter charts.
std::vector<std::size_t> chart_downsample_indices(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const QRectF& plot,
    std::size_t max_points = 0);

}  // namespace datalab::reporting
