#pragma once

#include <vector>

namespace datalab::domain::statistics {

struct NormalProbabilityResult {
    std::vector<double> theoretical_quantiles;
    std::vector<double> ordered_values;
    double correlation = 0.0;
};

NormalProbabilityResult normal_probability_plot(
    const std::vector<double>& observations);

}  // namespace datalab::domain::statistics
