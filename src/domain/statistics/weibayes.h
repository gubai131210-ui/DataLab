#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct WeibayesOptions {
    double shape_prior = 2.0;  // fixed β
};

struct WeibayesPercentile {
    double percentile = 0.0;  // 10 / 50 / 90
    double life = 0.0;
};

struct WeibayesResult {
    std::size_t n = 0;
    std::size_t failure_count = 0;   // r
    std::size_t censored_count = 0;
    double shape_prior = 2.0;
    std::optional<double> scale;  // η
    bool zero_failure_bound = false;
    std::vector<WeibayesPercentile> percentiles;
    std::vector<std::size_t> source_rows;
    std::string algorithm_id = "weibayes_fixed_shape";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

// times > 0; events true = exact failure, false = right-censored.
// η = (Σ t_i^β / r)^{1/β} when r ≥ 1; r = 0 → honesty / bound path (no η claim).
WeibayesResult fit_weibayes(
    const std::vector<double>& times,
    const std::vector<bool>& events,
    const std::vector<std::size_t>& source_rows = {},
    const WeibayesOptions& options = {});

}  // namespace datalab::domain::statistics
