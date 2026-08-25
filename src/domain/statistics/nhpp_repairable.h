#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct NhppRepairableOptions {
    std::optional<double> truncation_time;  // T; default = max(ti)
    std::size_t intensity_grid_points = 10;
};

struct NhppIntensityPoint {
    double t = 0.0;
    double intensity = 0.0;  // λ(t) = λ β t^{β-1}
    double mean_function = 0.0;  // M(t) = λ t^β
};

struct NhppRepairableResult {
    std::size_t failure_count = 0;  // n
    double truncation_time = 0.0;  // T
    std::optional<double> beta;
    std::optional<double> lambda;
    std::vector<double> failure_times;
    std::vector<std::size_t> source_rows;
    std::vector<NhppIntensityPoint> intensity_curve;
    std::string algorithm_id = "nhpp_crow_amsaa_mle";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

// Crow–AMSAA power-law NHPP MLE on cumulative failure times 0 < t1 < … ≤ T.
NhppRepairableResult fit_nhpp_crow_amsaa(
    const std::vector<double>& failure_times,
    const std::vector<std::size_t>& source_rows = {},
    const NhppRepairableOptions& options = {});

}  // namespace datalab::domain::statistics
