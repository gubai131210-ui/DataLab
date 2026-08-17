#pragma once

#include "domain/statistics/normal_probability.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct NormalityTestResult {
    std::size_t count = 0;
    double mean = 0.0;
    double sample_standard_deviation = 0.0;
    std::optional<double> anderson_darling;
    std::optional<double> p_value;
    std::vector<std::string> diagnostics;
    NormalProbabilityResult probability_plot;
};

NormalityTestResult normality_test(const std::vector<double>& observations);

}  // namespace datalab::domain::statistics
