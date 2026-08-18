#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class ArimaModel {
    arima_0_1_0,
    ar_1,
    ma_1
};

struct ArimaOrder {
    int p = 0;
    int d = 0;
    int q = 0;
};

struct ArimaResult {
    ArimaModel model = ArimaModel::arima_0_1_0;
    ArimaOrder order;
    double intercept = 0.0;
    double coefficient = 0.0;
    double drift = 0.0;
    double log_likelihood = 0.0;
    double sse = 0.0;
    double aic = 0.0;
    double aicc = 0.0;
    double bic = 0.0;
    std::vector<double> ar_coefficients;
    std::vector<double> ma_coefficients;
    std::vector<double> fitted;
    std::vector<double> residuals;
    std::vector<double> forecasts;
    std::vector<double> lower;
    std::vector<double> upper;
    std::vector<DiagnosticMessage> diagnostics;
};

std::string arima_order_label(const ArimaOrder& order);

ArimaResult fit_arima(
    const std::vector<double>& observations,
    ArimaModel model,
    std::size_t forecast_periods = 1);

ArimaResult fit_arima_order(
    const std::vector<double>& observations,
    const ArimaOrder& order,
    std::size_t forecast_periods = 1);

std::vector<ArimaResult> fit_arima_candidates(
    const std::vector<double>& observations,
    std::size_t forecast_periods = 1,
    int max_p = 3,
    int max_q = 3,
    int max_d = 2);

}  // namespace datalab::domain::statistics
