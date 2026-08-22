#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace datalab::domain::statistics {

struct IsolationForestOptions {
    std::size_t tree_count = 100;
    std::size_t max_samples = 256;
    std::uint32_t seed = 1;
    double score_quantile = 0.90;  // mark anomaly when score >= empirical quantile
};

struct IsolationForestResult {
    std::size_t observation_count = 0;
    std::size_t variable_count = 0;
    std::size_t tree_count = 0;
    std::size_t max_samples = 0;
    double score_threshold = 0.0;
    std::size_t anomaly_count = 0;
    std::vector<std::size_t> valid_rows;
    std::vector<double> scores;
    std::vector<bool> anomaly;
    std::vector<DiagnosticMessage> diagnostics;
};

IsolationForestResult isolation_forest(
    const std::vector<std::vector<double>>& rows,
    const IsolationForestOptions& options = {});

}  // namespace datalab::domain::statistics
