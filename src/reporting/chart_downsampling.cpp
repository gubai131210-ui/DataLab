#include "reporting/chart_downsampling.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace datalab::reporting {

std::vector<std::size_t> chart_downsample_indices(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const QRectF& plot,
    std::size_t max_points)
{
    const std::size_t count = std::min(x.size(), y.size());
    if (count == 0) {
        return {};
    }
    if (max_points == 0) {
        max_points = static_cast<std::size_t>(std::max(256.0, plot.width() * 3.0));
    }
    if (count <= max_points) {
        std::vector<std::size_t> indices(count);
        for (std::size_t index = 0; index < count; ++index) {
            indices[index] = index;
        }
        return indices;
    }

    auto x_range = std::minmax_element(x.cbegin(), x.cbegin() + count);
    const double x_min = *x_range.first;
    const double x_max = *x_range.second;
    const double span = x_max - x_min;
    const std::size_t bucket_count = std::max<std::size_t>(1, max_points / 4);
    std::vector<std::size_t> first(bucket_count, count);
    std::vector<std::size_t> last(bucket_count, count);
    std::vector<std::size_t> minimum(bucket_count, count);
    std::vector<std::size_t> maximum(bucket_count, count);
    for (std::size_t index = 0; index < count; ++index) {
        if (!std::isfinite(x[index]) || !std::isfinite(y[index])) {
            continue;
        }
        const double fraction = span > 0.0
            ? std::clamp((x[index] - x_min) / span, 0.0, 1.0) : 0.0;
        const std::size_t bucket = std::min(
            bucket_count - 1,
            static_cast<std::size_t>(fraction * static_cast<double>(bucket_count)));
        if (first[bucket] == count) {
            first[bucket] = index;
        }
        last[bucket] = index;
        if (minimum[bucket] == count || y[index] < y[minimum[bucket]]) {
            minimum[bucket] = index;
        }
        if (maximum[bucket] == count || y[index] > y[maximum[bucket]]) {
            maximum[bucket] = index;
        }
    }

    std::vector<std::size_t> indices;
    indices.reserve(std::min(count, max_points));
    for (std::size_t bucket = 0; bucket < bucket_count; ++bucket) {
        const std::size_t candidates[] = {
            first[bucket], minimum[bucket], maximum[bucket], last[bucket]};
        for (const std::size_t index : candidates) {
            if (index == count) {
                continue;
            }
            if (std::find(indices.cbegin(), indices.cend(), index) == indices.cend()) {
                indices.push_back(index);
            }
        }
    }
    std::sort(indices.begin(), indices.end());
    return indices;
}

}  // namespace datalab::reporting
