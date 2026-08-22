#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

namespace datalab::domain::statistics {

struct IntervalObservation {
    double left = 0.0;
    double right = 0.0;  // +inf → use infinity
    std::size_t source_row = 0;
};

struct KmIntervalPoint {
    double time = 0.0;
    double survival = 1.0;
    double mass = 0.0;
};

struct KmIntervalResult {
    std::size_t observation_count = 0;
    std::size_t exact_count = 0;
    std::size_t left_censored_count = 0;
    std::size_t right_censored_count = 0;
    std::size_t interval_censored_count = 0;
    std::size_t iteration_count = 0;
    bool converged = false;
    bool identifiable = false;
    std::optional<double> median_life;
    std::vector<KmIntervalPoint> points;
    std::vector<DiagnosticMessage> diagnostics;
};

// Turnbull NPMLE for interval-censored data (includes left/right as special intervals).
KmIntervalResult kaplan_meier_interval(
    const std::vector<IntervalObservation>& observations,
    std::size_t max_iterations = 200,
    double tolerance = 1.0e-8);

}  // namespace datalab::domain::statistics
