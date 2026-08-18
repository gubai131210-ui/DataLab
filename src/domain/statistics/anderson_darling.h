#pragma once

#include <cstddef>
#include <optional>
#include <vector>

namespace datalab::domain::statistics {

struct AndersonDarlingSummary {
    std::optional<double> statistic;
    std::optional<double> adjusted;
};

// Computes A² from ascending CDF values Z_(1)..Z_(n) at ordered observations.
AndersonDarlingSummary anderson_darling_from_sorted_cdf(
    const std::vector<double>& sorted_cdf);

// Stephens (1986) normal A²* p-value from adjusted statistic.
double anderson_darling_p_value_normal(double adjusted);

// Generic Stephens-style adjustment used for reporting A²*.
double anderson_darling_adjusted_statistic(double statistic, std::size_t sample_size);

}  // namespace datalab::domain::statistics
