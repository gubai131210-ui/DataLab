#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <vector>

namespace datalab::domain::statistics {

struct ForecastResult {
    std::vector<double> fitted;
    std::vector<double> forecasts;
    std::vector<double> lower;
    std::vector<double> upper;
    double mad = 0.0;
    double msd = 0.0;
    double mape = 0.0;
    std::vector<DiagnosticMessage> diagnostics;
};

ForecastResult single_exponential_smoothing(
    const std::vector<double>& observations,
    double alpha,
    std::size_t forecast_periods = 1);

ForecastResult double_exponential_smoothing(
    const std::vector<double>& observations,
    double alpha,
    double gamma,
    std::size_t forecast_periods = 1);

}  // namespace datalab::domain::statistics
