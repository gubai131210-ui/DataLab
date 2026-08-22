#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace datalab::domain::statistics {

struct BootstrapMeanOptions {
    std::size_t replicates = 2000;
    double confidence_level = 0.95;
    std::uint32_t seed = 1;
};

struct BootstrapMeanResult {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::size_t replicates = 0;
    double confidence_level = 0.95;
    std::string method = "percentile";
    std::optional<double> sample_mean;
    std::optional<double> ci_lower;
    std::optional<double> ci_upper;
    std::vector<double> bootstrap_means;
    std::vector<DiagnosticMessage> diagnostics;
};

BootstrapMeanResult bootstrap_mean_ci(
    const std::vector<double>& sample,
    const BootstrapMeanOptions& options = {});

}  // namespace datalab::domain::statistics
