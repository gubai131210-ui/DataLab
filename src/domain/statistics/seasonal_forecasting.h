#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <vector>

namespace datalab::domain::statistics {

enum class SeasonalErrorModel {
    additive,
    multiplicative
};

enum class TrendModel {
    none,
    additive,
    multiplicative
};

struct SeasonalForecastingOptions {
    SeasonalErrorModel error_model = SeasonalErrorModel::additive;
    TrendModel trend_model = TrendModel::additive;
    std::size_t seasonal_period = 1;
    std::size_t forecast_periods = 1;
    bool damped_trend = false;
    double alpha = 0.2;
    double beta = 0.1;
    double gamma = 0.2;
    double damping_phi = 0.98;
    double confidence_level = 0.95;
};

struct ForecastMetrics {
    double mad = 0.0;
    double msd = 0.0;
    double mape = 0.0;
    double rmse = 0.0;
    double mase = 0.0;
    std::size_t count = 0;
};

struct SeasonalForecastingResult {
    SeasonalForecastingOptions options;
    double level = 0.0;
    double trend = 0.0;
    std::vector<double> seasonal;
    std::vector<double> fitted;
    std::vector<double> residuals;
    std::vector<double> forecasts;
    std::vector<double> lower;
    std::vector<double> upper;
    ForecastMetrics metrics;
    std::vector<DiagnosticMessage> diagnostics;
};

SeasonalForecastingResult fit_seasonal_forecasting(
    const std::vector<double>& observations,
    const SeasonalForecastingOptions& options = {});

struct RollingOriginOptions {
    std::size_t initial_training_size = 0;
    std::size_t horizon = 1;
    std::size_t step = 1;
    SeasonalForecastingOptions model_options;
};

struct RollingOriginResult {
    ForecastMetrics metrics;
    std::vector<double> actuals;
    std::vector<double> forecasts;
    std::vector<double> lower;
    std::vector<double> upper;
    std::vector<std::size_t> origins;
    std::vector<DiagnosticMessage> diagnostics;
};

RollingOriginResult rolling_origin_validate(
    const std::vector<double>& observations,
    const RollingOriginOptions& options);

struct SarimaOrder {
    std::size_t p = 0;
    std::size_t d = 0;
    std::size_t q = 0;
    std::size_t seasonal_p = 0;
    std::size_t seasonal_d = 0;
    std::size_t seasonal_q = 0;
    std::size_t seasonal_period = 1;
};

struct FixedSarimaParameters {
    SarimaOrder order;
    std::vector<double> ar;
    std::vector<double> ma;
    std::vector<double> seasonal_ar;
    std::vector<double> seasonal_ma;
    double intercept = 0.0;
    double innovation_variance = 1.0;
};

struct SarimaForecastingResult {
    FixedSarimaParameters parameters;
    std::vector<double> forecasts;
    std::vector<double> lower;
    std::vector<double> upper;
    ForecastMetrics metrics;
    std::vector<DiagnosticMessage> diagnostics;
};

// The interface is intentionally ready for fixed-parameter SARIMA.
// Candidate search uses multiplicative CSS, not Minitab TSERIES back-forecast LS.
SarimaForecastingResult forecast_fixed_sarima(
    const std::vector<double>& observations,
    const FixedSarimaParameters& parameters,
    std::size_t forecast_periods,
    double confidence_level = 0.95);

struct SarimaCandidateResult {
    SarimaOrder order;
    double sse = 0.0;
    double aic = 0.0;
    double aicc = 0.0;
    double bic = 0.0;
    std::vector<DiagnosticMessage> diagnostics;
};

std::string sarima_order_label(const SarimaOrder& order);

std::vector<SarimaCandidateResult> fit_best_sarima_candidates(
    const std::vector<double>& observations,
    std::size_t seasonal_period,
    int max_p = 2,
    int max_q = 2,
    int max_d = 1,
    int max_seasonal_p = 2,
    int max_seasonal_q = 2,
    int max_seasonal_d = 1);

}  // namespace datalab::domain::statistics
