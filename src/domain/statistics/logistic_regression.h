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
    std::optional<double> vif;
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
    double leverage_threshold = 0.0;
    std::optional<double> maximum_leverage;
    std::optional<double> maximum_vif;
    std::size_t concordant_pairs = 0;
    std::size_t discordant_pairs = 0;
    std::size_t tied_pairs = 0;
    std::optional<double> pairs_concordance_percent;
    std::size_t true_positive = 0;
    std::size_t true_negative = 0;
    std::size_t false_positive = 0;
    std::size_t false_negative = 0;
    std::vector<DiagnosticMessage> diagnostics;
};

struct LogisticStepwiseStep {
    std::size_t step = 0;
    std::string action;  // enter / remove / start / stop
    std::string term;
    std::optional<double> deviance;
    std::optional<double> aic;
    std::optional<double> aicc;
    std::optional<double> bic;
    std::optional<double> enter_p_value;
    std::optional<double> remove_p_value;
};

struct LogisticStepwiseResult {
    std::string method = "stepwise";
    std::string criterion = "alpha";
    double alpha_enter = 0.15;
    double alpha_remove = 0.15;
    std::size_t observation_count = 0;
    std::size_t candidate_count = 0;
    std::size_t best_step_index = 0;
    std::vector<std::string> selected_terms;
    std::vector<LogisticStepwiseStep> steps;
    LogisticRegressionResult final_model;
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

LogisticStepwiseResult fit_logistic_stepwise(
    const std::vector<int>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& predictor_labels = {},
    const std::string& method = "stepwise",
    double alpha_enter = 0.15,
    double alpha_remove = 0.15,
    double confidence_level = 0.95,
    std::size_t max_iterations = 100,
    double tolerance = 1.0e-8);

}  // namespace datalab::domain::statistics
