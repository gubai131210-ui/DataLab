#pragma once

#include "domain/statistics/process_capability.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct BatchCapabilityRow {
    std::string batch_id;
    std::size_t sample_size = 0;
    double mean = 0.0;
    double within_standard_deviation = 0.0;
    std::optional<double> cp;
    std::optional<double> cpk;
    std::optional<double> pp;
    std::optional<double> ppk;
    std::vector<std::size_t> source_rows;
};

struct BatchCapabilityResult {
    std::size_t batch_count = 0;
    std::size_t skipped_batch_count = 0;
    std::size_t total_observations = 0;
    std::vector<BatchCapabilityRow> batches;
    std::vector<DiagnosticMessage> diagnostics;
};

BatchCapabilityResult compute_batch_capability(
    const std::vector<double>& values,
    const std::vector<std::string>& batch_labels,
    const std::vector<std::size_t>& source_rows,
    const SpecificationLimits& specifications,
    std::size_t min_batch_size = 2);

}  // namespace datalab::domain::statistics
