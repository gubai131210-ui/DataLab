#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace datalab::domain::statistics {

struct BootstrapTwoSampleOptions {
    std::size_t replicates = 2000;
    double confidence_level = 0.95;
    std::uint32_t seed = 1;
};

struct BootstrapTwoSampleResult {
    std::size_t n_first = 0;
    std::size_t n_second = 0;
    std::size_t missing_count = 0;
    std::size_t replicates = 0;
    double confidence_level = 0.95;
    std::string method = "percentile";
    std::optional<double> mean_first;
    std::optional<double> mean_second;
    std::optional<double> mean_difference;
    std::optional<double> ci_lower;
    std::optional<double> ci_upper;
    std::vector<double> bootstrap_differences;
    std::vector<DiagnosticMessage> diagnostics;
};

BootstrapTwoSampleResult bootstrap_two_sample_mean_difference_ci(
    const std::vector<double>& first_sample,
    const std::vector<double>& second_sample,
    const BootstrapTwoSampleOptions& options = {});

}  // namespace datalab::domain::statistics
