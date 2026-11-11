#include "reporting/chart_downsampling.h"

#include "reporting/chart_coordinate_mapper.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <set>
#include <utility>

namespace datalab::reporting {
namespace {

using Window = std::optional<std::pair<double, double>>;

bool finite_sample(const std::vector<double>& x, const std::vector<double>& y,
                   std::size_t index)
{
    return std::isfinite(x[index]) && std::isfinite(y[index]);
}

bool sample_in_view(const std::vector<double>& x, const std::vector<double>& y,
                    std::size_t index, std::size_t count, const Window& window,
                    const Window& y_window)
{
    if (!finite_sample(x, y, index)) return false;
    if (y_window.has_value()) {
        return (!window.has_value() || (x[index] >= window->first && x[index] <= window->second))
            && y[index] >= y_window->first && y[index] <= y_window->second;
    }
    if (!window.has_value()) return true;
    const double minimum = window->first;
    const double maximum = window->second;
    if (x[index] >= minimum && x[index] <= maximum) return true;
    // Include both endpoints when a visible line crosses the view boundary.
    if (index > 0 && finite_sample(x, y, index - 1)
        && std::min(x[index], x[index - 1]) <= maximum
        && std::max(x[index], x[index - 1]) >= minimum) return true;
    return index + 1 < count && finite_sample(x, y, index + 1)
        && std::min(x[index], x[index + 1]) <= maximum
        && std::max(x[index], x[index + 1]) >= minimum;
}

double bucket_fraction(double value, double minimum, double maximum)
{
    if (!(minimum < maximum)) return 0.0;
    const double span = maximum - minimum;
    // Halving first also handles a finite range whose subtraction would overflow.
    const double fraction = std::isfinite(span)
        ? (value - minimum) / span
        : (value * 0.5 - minimum * 0.5) / (maximum * 0.5 - minimum * 0.5);
    return std::isfinite(fraction) ? std::clamp(fraction, 0.0, 1.0)
                                 : value <= minimum ? 0.0 : 1.0;
}

std::size_t sample_budget(std::size_t count, const QRectF& plot, std::size_t budget)
{
    if (budget == 0) {
        const double width = std::isfinite(plot.width()) ? std::max(0.0, plot.width()) : 0.0;
        budget = static_cast<std::size_t>(std::min(static_cast<double>(count),
                                                 std::max(256.0, width * 3.0)));
    }
    return std::min(budget, count);
}

std::vector<std::size_t> sample_points(
    const std::vector<double>& x, const std::vector<double>& y,
    const QRectF& plot, std::size_t budget, const Window& x_window, const Window& y_window)
{
    const std::size_t count = std::min(x.size(), y.size());
    std::array<std::size_t, 4> extrema{count, count, count, count};
    std::size_t eligible_count = 0;
    for (std::size_t index = 0; index < count; ++index) {
        if (!sample_in_view(x, y, index, count, x_window, y_window)) continue;
        if (eligible_count++ == 0) extrema.fill(index);
        if (x[index] < x[extrema[0]]) extrema[0] = index;
        if (x[index] > x[extrema[1]]) extrema[1] = index;
        if (y[index] < y[extrema[2]]) extrema[2] = index;
        if (y[index] > y[extrema[3]]) extrema[3] = index;
    }
    if (eligible_count == 0) return {};
    std::vector<std::size_t> indices;
    if (eligible_count <= budget) {
        for (std::size_t index = 0; index < count; ++index)
            if (sample_in_view(x, y, index, count, x_window, y_window)) indices.push_back(index);
        return indices;
    }
    indices.assign(extrema.begin(), extrema.end());
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    if (indices.size() >= budget) {
        indices.resize(budget);
        return indices;
    }
    // A 2-D grid retains vertical distributions that an X-only envelope loses.
    const std::size_t grid_budget = budget - indices.size();
    const double aspect = plot.height() > 0.0 && std::isfinite(plot.width() / plot.height())
        ? std::clamp(plot.width() / plot.height(), 1.0 / 64.0, 64.0) : 1.0;
    const std::size_t columns = std::clamp(static_cast<std::size_t>(
        std::sqrt(static_cast<double>(grid_budget) * aspect)), std::size_t{1}, grid_budget);
    const std::size_t rows = std::max<std::size_t>(1, grid_budget / columns);
    std::vector<std::size_t> cells(columns * rows, count);
    for (std::size_t index = 0; index < count; ++index) {
        if (!sample_in_view(x, y, index, count, x_window, y_window)) continue;
        const std::size_t column = std::min(columns - 1, static_cast<std::size_t>(
            bucket_fraction(x[index], x_window->first, x_window->second) * columns));
        const std::size_t row = std::min(rows - 1, static_cast<std::size_t>(
            bucket_fraction(y[index], y_window->first, y_window->second) * rows));
        if (cells[row * columns + column] == count) cells[row * columns + column] = index;
    }
    for (std::size_t index : cells) if (index != count) indices.push_back(index);
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    return indices;
}

std::vector<std::size_t> sample_indices(
    const std::vector<double>& x, const std::vector<double>& y,
    const QRectF& plot, std::size_t budget, const Window& window,
    const Window& y_window = std::nullopt)
{
    const std::size_t count = std::min(x.size(), y.size());
    if (count == 0) return {};
    budget = sample_budget(count, plot, budget);
    std::size_t eligible_count = 0;
    double minimum_x = std::numeric_limits<double>::infinity();
    double maximum_x = -minimum_x;
    for (std::size_t index = 0; index < count; ++index) {
        if (!sample_in_view(x, y, index, count, window, y_window)) continue;
        ++eligible_count;
        minimum_x = std::min(minimum_x, x[index]);
        maximum_x = std::max(maximum_x, x[index]);
    }
    if (eligible_count == 0) return {};
    std::vector<std::size_t> indices;
    indices.reserve(std::min(eligible_count, budget));
    if (eligible_count <= budget) {
        for (std::size_t index = 0; index < count; ++index) {
            if (sample_in_view(x, y, index, count, window, y_window)) indices.push_back(index);
        }
        return indices;
    }
    if (window.has_value()) {
        minimum_x = window->first;
        maximum_x = window->second;
    }
    const std::size_t bucket_count = std::max<std::size_t>(1, budget / 4);
    std::vector<std::size_t> first(bucket_count, count);
    std::vector<std::size_t> last(bucket_count, count);
    std::vector<std::size_t> minimum(bucket_count, count);
    std::vector<std::size_t> maximum(bucket_count, count);
    for (std::size_t index = 0; index < count; ++index) {
        if (!sample_in_view(x, y, index, count, window, y_window)) continue;
        const double fraction = bucket_fraction(x[index], minimum_x, maximum_x);
        const std::size_t bucket = std::min(bucket_count - 1,
            static_cast<std::size_t>(fraction * static_cast<double>(bucket_count)));
        if (first[bucket] == count) first[bucket] = index;
        last[bucket] = index;
        if (minimum[bucket] == count || y[index] < y[minimum[bucket]]) minimum[bucket] = index;
        if (maximum[bucket] == count || y[index] > y[maximum[bucket]]) maximum[bucket] = index;
    }
    for (std::size_t bucket = 0; bucket < bucket_count; ++bucket) {
        std::array<std::size_t, 4> candidates{
            first[bucket], minimum[bucket], maximum[bucket], last[bucket]};
        std::sort(candidates.begin(), candidates.end());
        const auto end = std::unique(candidates.begin(), candidates.end());
        for (auto it = candidates.begin(); it != end && *it != count; ++it) {
            indices.push_back(*it);
        }
    }
    std::sort(indices.begin(), indices.end());
    if (indices.size() > budget) {
        // Budgets below four cannot retain all four extrema candidates.
        std::vector<std::size_t> limited;
        for (std::size_t index = 0; index < budget; ++index) {
            limited.push_back(indices[budget == 1 ? 0
                : index * (indices.size() - 1) / (budget - 1)]);
        }
        return limited;
    }
    return indices;
}

} // namespace

std::vector<std::size_t> chart_downsample_indices(
    const std::vector<double>& x, const std::vector<double>& y,
    const QRectF& plot, std::size_t max_points)
{
    return sample_indices(x, y, plot, max_points, std::nullopt);
}

std::vector<std::size_t> chart_downsample_indices(
    const std::vector<double>& x, const std::vector<double>& y,
    const QRectF& plot, const ChartCoordinateMapper& mapper, std::size_t max_points)
{
    const Window window = std::isfinite(mapper.x_min()) && std::isfinite(mapper.x_max())
        && mapper.x_min() < mapper.x_max()
        ? Window(std::make_pair(mapper.x_min(), mapper.x_max())) : std::nullopt;
    return sample_indices(x, y, plot, max_points, window);
}

std::vector<std::size_t> chart_visible_point_indices(
    const std::vector<double>& x, const std::vector<double>& y,
    const QRectF& plot, const ChartCoordinateMapper& mapper, std::size_t max_points)
{
    const std::size_t budget = sample_budget(std::min(x.size(), y.size()), plot, max_points);
    if (budget == 0) return {};
    const QPointF inner_first = mapper.to_data(plot.topLeft());
    const QPointF inner_last = mapper.to_data(plot.bottomRight());
    // Current marker/halo sizes are at most 12/8 logical pixels. Keep nearby
    // offscreen centers so an edge marker does not disappear during sampling.
    const QPointF first = mapper.to_data(plot.topLeft() - QPointF(24.0, 24.0));
    const QPointF last = mapper.to_data(plot.bottomRight() + QPointF(24.0, 24.0));
    if (!std::isfinite(first.x()) || !std::isfinite(first.y())
        || !std::isfinite(last.x()) || !std::isfinite(last.y())
        || !std::isfinite(inner_first.x()) || !std::isfinite(inner_first.y())
        || !std::isfinite(inner_last.x()) || !std::isfinite(inner_last.y())) {
        return sample_indices(x, y, plot, max_points, std::nullopt);
    }
    const Window x_window = std::make_pair(std::min(first.x(), last.x()), std::max(first.x(), last.x()));
    const Window y_window = std::make_pair(std::min(first.y(), last.y()), std::max(first.y(), last.y()));
    const Window inner_x = std::make_pair(std::min(inner_first.x(), inner_last.x()),
                                         std::max(inner_first.x(), inner_last.x()));
    const Window inner_y = std::make_pair(std::min(inner_first.y(), inner_last.y()),
                                         std::max(inner_first.y(), inner_last.y()));
    const auto visible = sample_points(x, y, plot, budget, inner_x, inner_y);
    if (visible.size() == budget) return visible;
    std::set<std::size_t> chosen(visible.begin(), visible.end());
    const auto nearby = sample_indices(x, y, plot, budget, x_window, y_window);
    // True visible points have priority over the optional offscreen marker margin.
    for (bool inside : {true, false}) {
        for (std::size_t index : nearby) {
            if (chosen.size() == budget) break;
            if (sample_in_view(x, y, index, std::min(x.size(), y.size()), inner_x, inner_y) == inside)
                chosen.insert(index);
        }
    }
    return {chosen.begin(), chosen.end()};
}

bool chart_samples_are_connected(
    const std::vector<double>& x, const std::vector<double>& y,
    std::size_t first, std::size_t last)
{
    if (first >= last || last >= std::min(x.size(), y.size())) return false;
    for (std::size_t index = first; index <= last; ++index) {
        if (!finite_sample(x, y, index)) return false;
    }
    return true;
}

} // namespace datalab::reporting
