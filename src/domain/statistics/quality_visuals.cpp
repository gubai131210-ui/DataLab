#include "domain/statistics/quality_visuals.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace datalab::domain::statistics {

BoxPlotSummary box_plot_summary(const std::vector<double>& observations)
{
    BoxPlotSummary result;
    if (observations.empty()) {
        return result;
    }
    std::vector<double> ordered = observations;
    std::sort(ordered.begin(), ordered.end());
    const auto percentile = [&ordered](double position) {
        const double index = position * static_cast<double>(ordered.size() - 1);
        const std::size_t lower = static_cast<std::size_t>(index);
        const std::size_t upper = std::min(lower + 1, ordered.size() - 1);
        const double fraction = index - static_cast<double>(lower);
        return ordered[lower] + fraction * (ordered[upper] - ordered[lower]);
    };
    result.minimum = ordered.front();
    result.first_quartile = percentile(0.25);
    result.median = percentile(0.5);
    result.third_quartile = percentile(0.75);
    result.maximum = ordered.back();
    result.count = ordered.size();
    result.iqr = result.third_quartile - result.first_quartile;
    const double fence_low = result.first_quartile - 1.5 * result.iqr;
    const double fence_high = result.third_quartile + 1.5 * result.iqr;
    result.whisker_low = result.minimum;
    result.whisker_high = result.maximum;
    bool found_low = false;
    for (const double value : ordered) {
        if (value < fence_low) {
            result.outliers.push_back(value);
            continue;
        }
        if (!found_low) {
            result.whisker_low = value;
            found_low = true;
        }
        if (value > fence_high) {
            result.outliers.push_back(value);
        } else {
            result.whisker_high = value;
        }
    }
    return result;
}

std::vector<ParetoItem> pareto(
    const std::vector<std::pair<std::string, std::size_t>>& counts)
{
    return pareto(counts, {});
}

std::vector<ParetoItem> pareto(
    const std::vector<std::pair<std::string, std::size_t>>& counts,
    const ParetoOptions& options)
{
    std::vector<ParetoItem> result;
    result.reserve(counts.size());
    std::size_t total = 0;
    for (const auto& item : counts) {
        if (!item.first.empty() && item.first != "*") {
            total += item.second;
            result.push_back({item.first, item.second, 0.0, 0.0});
        }
    }
    std::sort(result.begin(), result.end(), [](const ParetoItem& left, const ParetoItem& right) {
        if (left.count != right.count) {
            return left.count > right.count;
        }
        return left.category < right.category;
    });

    if (options.other_threshold_percent.has_value()) {
        const double threshold = std::clamp(*options.other_threshold_percent, 0.0, 100.0);
        std::size_t other_count = 0;
        std::vector<ParetoItem> retained;
        retained.reserve(result.size());
        std::size_t accumulated = 0;
        bool combining = false;
        for (const ParetoItem& item : result) {
            // Minitab keeps categories until Cum% first surpasses the threshold,
            // then merges every remaining category into Other.
            if (combining) {
                other_count += item.count;
                continue;
            }
            retained.push_back(item);
            accumulated += item.count;
            const double cumulative = total == 0
                ? 0.0
                : 100.0 * static_cast<double>(accumulated) / static_cast<double>(total);
            if (cumulative > threshold) {
                combining = true;
            }
        }
        if (other_count > 0) {
            retained.push_back({"Other", other_count, 0.0, 0.0});
        }
        result = std::move(retained);
    }

    std::size_t accumulated = 0;
    for (ParetoItem& item : result) {
        accumulated += item.count;
        item.percent = total == 0
            ? 0.0
            : 100.0 * static_cast<double>(item.count) / static_cast<double>(total);
        item.cumulative_percent = total == 0
            ? 0.0
            : 100.0 * static_cast<double>(accumulated) / static_cast<double>(total);
    }
    return result;
}

HistogramResult histogram(const std::vector<double>& observations, int bin_count)
{
    HistogramResult result;
    if (observations.empty()) {
        return result;
    }
    const auto [minimum_it, maximum_it] =
        std::minmax_element(observations.begin(), observations.end());
    double minimum = *minimum_it;
    double maximum = *maximum_it;
    if (maximum <= minimum) {
        maximum = minimum + 1.0;
    }
    int bins = bin_count;
    if (bins <= 0) {
        bins = static_cast<int>(std::ceil(std::log2(static_cast<double>(observations.size())) + 1.0));
        bins = std::max(5, std::min(bins, 30));
    }
    const double width = (maximum - minimum) / static_cast<double>(bins);
    result.edges.resize(static_cast<std::size_t>(bins) + 1);
    result.counts.assign(static_cast<std::size_t>(bins), 0.0);
    for (int index = 0; index <= bins; ++index) {
        result.edges[static_cast<std::size_t>(index)] = minimum + width * static_cast<double>(index);
    }
    result.edges.back() = maximum;
    for (const double value : observations) {
        int bin = static_cast<int>((value - minimum) / width);
        if (bin >= bins) {
            bin = bins - 1;
        }
        if (bin < 0) {
            bin = 0;
        }
        result.counts[static_cast<std::size_t>(bin)] += 1.0;
    }
    return result;
}

HistogramResult histogram_with_edges(
    const std::vector<double>& observations,
    const std::vector<double>& edges)
{
    HistogramResult result;
    result.edges = edges;
    if (edges.size() < 2) {
        return result;
    }
    result.counts.assign(edges.size() - 1, 0.0);
    for (const double value : observations) {
        if (!std::isfinite(value) || value < edges.front() || value > edges.back()) {
            continue;
        }
        auto it = std::upper_bound(edges.cbegin(), edges.cend(), value);
        std::size_t bin = static_cast<std::size_t>(std::distance(edges.cbegin(), it));
        if (bin == 0) {
            bin = 1;
        }
        --bin;
        if (bin >= result.counts.size()) {
            bin = result.counts.size() - 1;
        }
        result.counts[bin] += 1.0;
    }
    return result;
}

}  // namespace datalab::domain::statistics
