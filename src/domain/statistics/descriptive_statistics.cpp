#include "domain/statistics/descriptive_statistics.h"

#include <algorithm>
#include <cmath>

namespace datalab::domain::statistics {

std::optional<DescriptiveStatisticsResult> DescriptiveStatistics::calculate(
    const std::vector<double>& observations)
{
    return calculate(observations, 0, observations.size());
}

std::optional<DescriptiveStatisticsResult> DescriptiveStatistics::calculate(
    const std::vector<double>& observations,
    std::size_t missing_count,
    std::size_t total_count)
{
    if (observations.empty()) {
        return std::nullopt;
    }

    for (const double observation : observations) {
        if (!std::isfinite(observation)) {
            return std::nullopt;
        }
    }

    DescriptiveStatisticsResult result;
    result.count = observations.size();
    result.missing_count = missing_count;
    result.total_count = total_count == 0 ? observations.size() + missing_count : total_count;
    result.minimum = observations.front();
    result.maximum = observations.front();

    long double sum = 0.0L;
    for (const double observation : observations) {
        sum += observation;
        result.minimum = std::min(result.minimum, observation);
        result.maximum = std::max(result.maximum, observation);
    }

    result.mean = static_cast<double>(sum / observations.size());
    result.sum = static_cast<double>(sum);
    std::vector<double> ordered = observations;
    std::sort(ordered.begin(), ordered.end());
    const auto percentile = [&ordered](double position) {
        const double index = position * static_cast<double>(ordered.size() - 1);
        const std::size_t lower_index = static_cast<std::size_t>(index);
        const std::size_t upper_index = std::min(lower_index + 1, ordered.size() - 1);
        const double fraction = index - static_cast<double>(lower_index);
        return ordered[lower_index]
            + fraction * (ordered[upper_index] - ordered[lower_index]);
    };
    result.median = percentile(0.5);
    result.first_quartile = percentile(0.25);
    result.third_quartile = percentile(0.75);
    result.interquartile_range = result.third_quartile - result.first_quartile;
    result.range = result.maximum - result.minimum;

    long double squared_deviations = 0.0L;
    for (const double observation : observations) {
        const long double deviation =
            static_cast<long double>(observation) - result.mean;
        squared_deviations += deviation * deviation;
    }

    const long double population_variance =
        squared_deviations / observations.size();
    result.population_standard_deviation =
        std::sqrt(static_cast<double>(population_variance));
    result.variance = observations.size() >= 2
        ? static_cast<double>(squared_deviations / (observations.size() - 1))
        : 0.0;

    if (observations.size() >= 2) {
        const long double sample_variance =
            squared_deviations / (observations.size() - 1);
        result.sample_standard_deviation =
            std::sqrt(static_cast<double>(sample_variance));
        result.standard_error_of_mean =
            *result.sample_standard_deviation / std::sqrt(static_cast<double>(observations.size()));
    }

    return result;
}

}  // namespace datalab::domain::statistics
