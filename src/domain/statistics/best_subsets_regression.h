#pragma once

#include "domain/statistics/regression.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct BestSubsetsModelSummary {
    std::size_t predictor_count = 0;
    std::vector<bool> predictors_in_model;
    std::vector<std::string> term_labels;
    double r_squared = 0.0;
    double adjusted_r_squared = 0.0;
    double mallows_cp = 0.0;
    double s = 0.0;
};

struct BestSubsetsRegressionResult {
    std::size_t observation_count = 0;
    std::size_t candidate_count = 0;
    std::size_t models_per_size = 1;
    std::size_t min_predictors = 1;
    std::size_t max_predictors = 0;
    std::optional<double> full_model_mse;
    std::vector<BestSubsetsModelSummary> model_summaries;
    std::optional<BestSubsetsModelSummary> best_overall;
    std::vector<DiagnosticMessage> diagnostics;
};

BestSubsetsRegressionResult fit_best_subsets_regression(
    const std::vector<double>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& predictor_labels,
    std::size_t min_predictors = 1,
    std::size_t max_predictors = 0,
    std::size_t models_per_size = 1,
    double confidence_level = 0.95,
    const std::vector<std::size_t>& source_rows = {});

}  // namespace datalab::domain::statistics
