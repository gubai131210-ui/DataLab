#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/cart_tree.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct RandomForestOptions {
    CartTask task = CartTask::classification;
    std::size_t n_trees = 50;
    std::size_t max_depth = 5;
    std::size_t min_leaf = 5;
    std::uint32_t seed = 1;
    bool compute_oob = true;
};

struct RandomForestResult {
    CartTask task = CartTask::classification;
    std::size_t observation_count = 0;
    std::size_t predictor_count = 0;
    std::size_t n_trees = 0;
    std::size_t max_depth = 0;
    double train_metric = 0.0;  // accuracy or RMSE
    std::optional<double> oob_metric;
    std::string top_variable;
    std::string disclosure =
        "Bagging CART ensemble; NOT TreeNet / Minitab Random Forests aligned.";
    std::vector<std::size_t> valid_rows;
    std::vector<double> fitted;
    std::vector<std::string> fitted_labels;
    std::vector<std::string> class_labels;
    std::vector<double> variable_importance;
    std::vector<std::string> predictor_names;
    std::vector<std::vector<std::size_t>> confusion;
    std::vector<DiagnosticMessage> diagnostics;
};

RandomForestResult fit_random_forest(
    const std::vector<std::vector<double>>& predictors,
    const std::vector<double>& response,
    const std::vector<std::string>& class_labels,
    const std::vector<std::string>& predictor_names,
    const RandomForestOptions& options = {});

}  // namespace datalab::domain::statistics
