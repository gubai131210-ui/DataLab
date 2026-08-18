#include "domain/statistics/anderson_darling.h"

#include <algorithm>
#include <cmath>

namespace datalab::domain::statistics {

double anderson_darling_p_value_normal(double adjusted)
{
    if (adjusted > 0.6) {
        return std::exp(1.2937 - 5.709 * adjusted + 0.0186 * adjusted * adjusted);
    }
    if (adjusted > 0.34) {
        return std::exp(0.9177 - 4.279 * adjusted - 1.38 * adjusted * adjusted);
    }
    if (adjusted > 0.2) {
        return 1.0 - std::exp(-8.318 + 42.796 * adjusted - 59.938 * adjusted * adjusted);
    }
    return 1.0 - std::exp(-13.436 + 101.14 * adjusted - 223.73 * adjusted * adjusted);
}

double anderson_darling_adjusted_statistic(double statistic, std::size_t sample_size)
{
    return statistic * (1.0 + 0.75 / static_cast<double>(sample_size)
        + 2.25 / (static_cast<double>(sample_size) * static_cast<double>(sample_size)));
}

AndersonDarlingSummary anderson_darling_from_sorted_cdf(
    const std::vector<double>& sorted_cdf)
{
    AndersonDarlingSummary summary;
    if (sorted_cdf.size() < 3) {
        return summary;
    }
    constexpr double kEpsilon = 1.0e-12;
    double sum = 0.0;
    for (std::size_t index = 0; index < sorted_cdf.size(); ++index) {
        const double cdf = std::clamp(sorted_cdf[index], kEpsilon, 1.0 - kEpsilon);
        const double left_weight = static_cast<double>(2 * index + 1);
        const double right_weight =
            static_cast<double>(2 * sorted_cdf.size() + 1 - 2 * (index + 1));
        sum += left_weight * std::log(cdf)
            + right_weight * std::log(1.0 - cdf);
    }
    const double statistic = -static_cast<double>(sorted_cdf.size())
        - sum / static_cast<double>(sorted_cdf.size());
    summary.statistic = statistic;
    summary.adjusted = anderson_darling_adjusted_statistic(statistic, sorted_cdf.size());
    return summary;
}

}  // namespace datalab::domain::statistics
