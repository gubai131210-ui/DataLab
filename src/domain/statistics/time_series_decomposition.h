#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <vector>

namespace datalab::domain::statistics {

enum class DecompositionModel {
    additive,
    multiplicative
};

struct TimeSeriesDecompositionInput {
    std::vector<double> time;
    std::vector<double> observations;
};

struct TimeSeriesDecompositionOptions {
    DecompositionModel model = DecompositionModel::additive;
    std::size_t seasonal_period = 1;
    std::size_t forecast_periods = 1;
};

struct TimeSeriesDecompositionResult {
    DecompositionModel model = DecompositionModel::additive;
    std::size_t seasonal_period = 0;
    std::vector<double> time;
    std::vector<double> observations;
    std::vector<double> centered_moving_average;
    std::vector<double> seasonal_indices;
    std::vector<double> trend;
    std::vector<double> fitted;
    std::vector<double> residuals;
    std::vector<double> forecasts;
    double trend_intercept = 0.0;
    double trend_slope = 0.0;
    double mad = 0.0;
    double msd = 0.0;
    double mape = 0.0;
    std::vector<DiagnosticMessage> diagnostics;
};

// Decomposes an equally spaced series using centered moving averages and a
// linear trend on the deseasonalized observations. Invalid input is reported
// through diagnostics and produces an otherwise empty result.
TimeSeriesDecompositionResult decompose_time_series(
    const TimeSeriesDecompositionInput& input,
    const TimeSeriesDecompositionOptions& options);

}  // namespace datalab::domain::statistics
