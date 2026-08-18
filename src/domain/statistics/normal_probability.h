#pragma once

#include <cstddef>
#include <vector>

namespace datalab::domain::statistics {

struct NormalProbabilityResult {
    std::vector<double> theoretical_quantiles;
    std::vector<double> ordered_values;
    std::vector<std::size_t> source_rows;
    double correlation = 0.0;
};

NormalProbabilityResult normal_probability_plot(
    const std::vector<double>& observations);

NormalProbabilityResult normal_probability_plot(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows);

}  // namespace datalab::domain::statistics
