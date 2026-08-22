#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class CartTask {
    classification,
    regression
};

struct CartTreeOptions {
    CartTask task = CartTask::classification;
    std::size_t max_depth = 5;
    std::size_t min_leaf = 5;
};

struct CartNode {
    int id = -1;
    int parent_id = -1;
    std::size_t depth = 0;
    bool is_leaf = true;
    std::size_t n = 0;
    double impurity = 0.0;
    double prediction = 0.0;  // class index or mean
    std::string prediction_label;
    std::optional<std::size_t> split_variable;
    std::optional<double> split_threshold;
    int left_child = -1;
    int right_child = -1;
};

struct CartTreeResult {
    CartTask task = CartTask::classification;
    std::size_t observation_count = 0;
    std::size_t predictor_count = 0;
    std::size_t max_depth = 0;
    std::size_t node_count = 0;
    std::size_t leaf_count = 0;
    double train_metric = 0.0;  // accuracy or RMSE
    std::string top_variable;
    std::vector<std::size_t> valid_rows;
    std::vector<CartNode> nodes;
    std::vector<double> fitted;
    std::vector<std::string> fitted_labels;
    std::vector<std::string> class_labels;
    std::vector<double> variable_importance;
    std::vector<std::string> predictor_names;
    std::vector<std::vector<std::size_t>> confusion;  // classification only
    std::vector<DiagnosticMessage> diagnostics;
};

// predictors: row-major [n][p]; response numeric for regression, or class
// indices 0..C-1 for classification (class_labels maps index → name).
CartTreeResult fit_cart_tree(
    const std::vector<std::vector<double>>& predictors,
    const std::vector<double>& response,
    const std::vector<std::string>& class_labels,
    const std::vector<std::string>& predictor_names,
    const CartTreeOptions& options = {});

}  // namespace datalab::domain::statistics
