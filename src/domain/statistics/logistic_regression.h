#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct LogisticCoefficient {
    std::string term;
    double coefficient = 0.0;
    double standard_error = std::numeric_limits<double>::quiet_NaN();
    double z_statistic = std::numeric_limits<double>::quiet_NaN();
    double p_value = std::numeric_limits<double>::quiet_NaN();
    double odds_ratio = std::numeric_limits<double>::quiet_NaN();
    double confidence_lower = std::numeric_limits<double>::quiet_NaN();
    double confidence_upper = std::numeric_limits<double>::quiet_NaN();
};

struct LogisticObservation {
    int response = 0;
    double linear_predictor = 0.0;
    double probability = 0.0;
    double pearson_residual = 0.0;
    double deviance_residual = 0.0;
    double leverage = 0.0;
    bool high_leverage = false;
};

struct LogisticRegressionResult {
    std::size_t observation_count = 0;
    std::size_t predictor_count = 0;
    std::size_t iteration_count = 0;
    bool converged = false;
    bool complete_separation = false;
    std::vector<LogisticCoefficient> coefficients;
    std::vector<LogisticObservation> observations;
    double log_likelihood = std::numeric_limits<double>::quiet_NaN();
    double deviance = std::numeric_limits<double>::quiet_NaN();
    double aic = std::numeric_limits<double>::quiet_NaN();
    double bic = std::numeric_limits<double>::quiet_NaN();
    std::optional<double> hosmer_lemeshow_statistic;
    std::optional<double> hosmer_lemeshow_p;
    std::size_t hosmer_lemeshow_groups = 0;
    std::optional<std::size_t> hosmer_lemeshow_df;
    std::string hosmer_lemeshow_status = "not_computed";
    std::vector<DiagnosticMessage> diagnostics;
};

// Fits a binary logistic model with an intercept using IRLS.
LogisticRegressionResult fit_logistic_regression(
    const std::vector<int>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& predictor_labels = {},
    double confidence_level = 0.95,
    std::size_t max_iterations = 100,
    double tolerance = 1.0e-8);

// Returns NaN when the result or predictor row has an incompatible shape.
double predict_logistic_probability(
    const LogisticRegressionResult& result,
    const std::vector<double>& predictors);

std::vector<double> predict_logistic_probabilities(
    const LogisticRegressionResult& result,
    const std::vector<std::vector<double>>& predictors);

}  // namespace datalab::domain::statistics
