#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace datalab::domain::statistics {

struct NonparametricCapabilityResult {
    std::size_t sample_size = 0;
    double median = 0.0;
    double lower_percentile = 0.0;
    double upper_percentile = 0.0;
    double tolerance_k = 6.0;
    std::optional<double> cnp;
    std::optional<double> cnpl;
    std::optional<double> cnpu;
    std::optional<double> cnpk;
    std::optional<double> observed_ppm_below;
    std::optional<double> observed_ppm_above;
    std::optional<double> observed_ppm_total;
    std::vector<double> histogram_edges;
    std::vector<double> histogram_counts;
    std::vector<std::size_t> source_rows;
    std::vector<DiagnosticMessage> diagnostics;
};

NonparametricCapabilityResult compute_nonparametric_capability(
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows,
    const SpecificationLimits& specifications,
    double tolerance_k = 6.0);

}  // namespace datalab::domain::statistics
