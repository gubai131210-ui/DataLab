#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/normal_probability.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct DistributionIdentificationCandidate {
    std::string distribution;
    std::optional<double> location;
    std::optional<double> scale;
    std::optional<double> shape;
    std::optional<double> anderson_darling;
    std::optional<double> adjusted_anderson_darling;
    std::optional<double> p_value;
    std::string decision = "not_computed";
    std::string status = "not_computed";
    NormalProbabilityResult probability_plot;
};

struct DistributionIdentificationResult {
    std::vector<DistributionIdentificationCandidate> candidates;
    std::vector<DiagnosticMessage> diagnostics;
};

DistributionIdentificationResult identify_individual_distributions(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows = {},
    double alpha = 0.05);

}  // namespace datalab::domain::statistics
