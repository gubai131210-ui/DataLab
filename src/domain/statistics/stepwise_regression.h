#pragma once

#include "domain/statistics/regression.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct StepwiseStep {
    std::size_t step = 0;
    std::string action;  // enter / remove / start / stop
    std::string term;
    double r_squared = 0.0;
    double adjusted_r_squared = 0.0;
    double error_sum_of_squares = 0.0;
    std::optional<double> entered_p_value;
    std::optional<double> removed_p_value;
    std::optional<double> aic;
    std::optional<double> aicc;
    std::optional<double> bic;
};

struct StepwiseRegressionResult {
    std::string method = "stepwise";
    std::string criterion = "alpha";
    double alpha_enter = 0.15;
    double alpha_remove = 0.15;
    std::size_t observation_count = 0;
    std::size_t candidate_count = 0;
    std::size_t best_step_index = 0;
    std::vector<std::string> selected_terms;
    std::vector<StepwiseStep> steps;
    RegressionResult final_model;
    std::vector<DiagnosticMessage> diagnostics;
};

StepwiseRegressionResult fit_stepwise_regression(
    const std::vector<double>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& predictor_labels,
    const std::string& method = "stepwise",
    double alpha_enter = 0.15,
    double alpha_remove = 0.15,
    double confidence_level = 0.95,
    const std::vector<std::size_t>& source_rows = {});

}  // namespace datalab::domain::statistics
