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
    std::size_t source_row = 0;
    double response = 0.0;
    double fitted = 0.0;
    double residual = 0.0;
    double standardized_residual = 0.0;
    double internally_standardized_residual = 0.0;
    double studentized_residual = 0.0;
    double deleted_studentized_residual = 0.0;
    double leverage = 0.0;
    double cooks_distance = 0.0;
    double dfits = 0.0;
    bool is_outlier = false;
    bool is_high_leverage = false;
    bool is_influential = false;
    bool unusual_r = false;
    bool unusual_x = false;
    std::vector<std::string> diagnostic_flags;
};

struct RegressionDiagnosticsSummary {
    double durbin_watson = 0.0;
    std::optional<NormalityTestResult> residual_normality;
    std::size_t outlier_count = 0;
    std::size_t high_leverage_count = 0;
    std::size_t influential_count = 0;
    std::vector<std::size_t> flagged_observations;
    std::vector<double> residual_vs_fitted_x;
    std::vector<double> residual_vs_fitted_y;
    std::vector<double> residual_vs_order_x;
    std::vector<double> residual_vs_order_y;
    std::string durbin_watson_order = "input_order";
    std::optional<double> durbin_watson_dl;
    std::optional<double> durbin_watson_du;
    std::string durbin_watson_decision = "not_computed";
    std::vector<AssumptionCheck> assumptions;
    std::vector<RuleEvidence> rules;
};

struct RegressionAnovaEffect {
    std::string term;
    std::optional<double> sequential_sum_of_squares;
    std::optional<double> adjusted_sum_of_squares;
    std::size_t degrees_of_freedom = 1;
    std::optional<double> mean_square;
    std::optional<double> f_statistic;
    std::optional<double> p_value;
    bool estimable = true;
};

struct RegressionBandPoint {
    double x = 0.0;
    double fitted = 0.0;
    double se_fit = 0.0;
    double se_pred = 0.0;
    double ci_lower = 0.0;
    double ci_upper = 0.0;
    double pi_lower = 0.0;
    double pi_upper = 0.0;
};

struct RegressionResult {
    std::size_t observation_count = 0;
    std::size_t predictor_count = 0;
    double residual_standard_deviation = 0.0;
    std::vector<std::vector<double>> xtx_inverse;
    std::vector<double> simple_predictor_values;
    double confidence_level = 0.95;
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
    QualityEvidence evidence;
    std::vector<RegressionCoefficient> coefficients;
    std::vector<RegressionAnovaEffect> anova_effects;
    std::vector<RegressionObservation> observations;
    RegressionDiagnosticsSummary diagnostics_summary;
    std::vector<DiagnosticMessage> diagnostics;
};

RegressionResult fit_linear_regression(
    const std::vector<double>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& predictor_labels = {},
    double confidence_level = 0.95,
    const std::vector<std::size_t>& source_rows = {});

// Simple-regression (one predictor) fitted-line CI/PI grid.
// Empty when predictor_count != 1, error_df <= 0, or (X'X)^{-1} missing.
std::vector<RegressionBandPoint> fitted_line_bands(
    const RegressionResult& result,
    std::size_t grid_count = 40);

}  // namespace datalab::domain::statistics
