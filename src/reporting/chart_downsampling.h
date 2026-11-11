#pragma once

#include <QRectF>

#include <cstddef>
#include <vector>

class ChartCoordinateMapper;

namespace datalab::reporting {

// Returns source indices in draw order.  Each horizontal pixel bucket keeps
// the first, last, minimum-y and maximum-y observations, preserving spikes
// while bounding painter work for large time-series/scatter charts.
std::vector<std::size_t> chart_downsample_indices(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const QRectF& plot,
    std::size_t max_points = 0);

// Samples the current X view, retaining adjacent boundary observations for lines.
std::vector<std::size_t> chart_downsample_indices(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const QRectF& plot,
    const ChartCoordinateMapper& mapper,
    std::size_t max_points = 0);

// Point-only charts sample both visible axes, with a margin for edge markers.
std::vector<std::size_t> chart_visible_point_indices(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const QRectF& plot,
    const ChartCoordinateMapper& mapper,
    std::size_t max_points = 0);

// Downsampling may skip valid observations, but must never bridge a missing one.
bool chart_samples_are_connected(
    const std::vector<double>& x,
    const std::vector<double>& y,
    std::size_t first,
    std::size_t last);

}  // namespace datalab::reporting
