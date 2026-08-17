#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <vector>

namespace datalab::domain::statistics {

enum class ArimaModel {
    arima_0_1_0,
    ar_1,
    ma_1
};

struct ArimaResult {
    ArimaModel model = ArimaModel::arima_0_1_0;
    double intercept = 0.0;
    double coefficient = 0.0;
    double drift = 0.0;
    double log_likelihood = 0.0;
    double sse = 0.0;
    double aic = 0.0;
    double aicc = 0.0;
    double bic = 0.0;
    std::vector<double> fitted;
    std::vector<double> residuals;
    std::vector<double> forecasts;
    std::vector<double> lower;
    std::vector<double> upper;
    std::vector<DiagnosticMessage> diagnostics;
};

// Fits one of the supported non-seasonal ARIMA candidates and forecasts recursively.
// The interval bounds are normal-approximation 95% prediction intervals.
ArimaResult fit_arima(
    const std::vector<double>& observations,
    ArimaModel model,
    std::size_t forecast_periods = 1);

// Fits all supported candidates in fixed order. Invalid candidates retain diagnostics.
std::vector<ArimaResult> fit_arima_candidates(
    const std::vector<double>& observations,
    std::size_t forecast_periods = 1);

}  // namespace datalab::domain::statistics
