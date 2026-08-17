#pragma once

#include "domain/statistics/normality_test.h"
#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct RegressionCoefficient {
    std::string term;
    double coefficient = 0.0;
    double standard_error = 0.0;
    double t_statistic = 0.0;
    std::optional<double> p_value;
    std::optional<double> confidence_lower;
    std::optional<double> confidence_upper;
    std::optional<double> vif;
};

struct RegressionObservation {
    double response = 0.0;
    double fitted = 0.0;
    double residual = 0.0;
    double standardized_residual = 0.0;
    // Internal studentized residual; kept alongside the legacy field.
    double studentized_residual = 0.0;
    // Externally studentized residual based on the leave-one-out variance.
    double deleted_studentized_residual = 0.0;
    double leverage = 0.0;
    double cooks_distance = 0.0;
    double dfits = 0.0;
    bool is_outlier = false;
    bool is_high_leverage = false;
    bool is_influential = false;
    std::vector<std::string> diagnostic_flags;
};

struct RegressionDiagnosticsSummary {
    double durbin_watson = 0.0;
    std::optional<NormalityTestResult> residual_normality;
    std::size_t outlier_count = 0;
    std::size_t high_leverage_count = 0;
    std::size_t influential_count = 0;
    std::vector<std::size_t> flagged_observations;
};

struct RegressionResult {
    std::size_t observation_count = 0;
    std::size_t predictor_count = 0;
    double residual_standard_deviation = 0.0;
    double r_squared = 0.0;
    double adjusted_r_squared = 0.0;
    double predicted_r_squared = 0.0;
    double press = 0.0;
    double regression_sum_of_squares = 0.0;
    double error_sum_of_squares = 0.0;
    double total_sum_of_squares = 0.0;
    double regression_mean_square = 0.0;
    double error_mean_square = 0.0;
    double f_statistic = 0.0;
    double durbin_watson = 0.0;
    std::optional<double> model_p_value;
    std::vector<RegressionCoefficient> coefficients;
    std::vector<RegressionObservation> observations;
    RegressionDiagnosticsSummary diagnostics_summary;
    std::vector<DiagnosticMessage> diagnostics;
};

RegressionResult fit_linear_regression(
    const std::vector<double>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& predictor_labels = {},
    double confidence_level = 0.95);

}  // namespace datalab::domain::statistics
