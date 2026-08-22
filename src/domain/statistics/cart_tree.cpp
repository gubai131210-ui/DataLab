#include "domain/statistics/cart_tree.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

double gini_impurity(const std::vector<std::size_t>& counts, std::size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    double impurity = 1.0;
    for (std::size_t count : counts) {
        const double p = static_cast<double>(count) / static_cast<double>(n);
        impurity -= p * p;
    }
    return impurity;
}

double sse_impurity(const std::vector<double>& values)
{
    if (values.empty()) {
        return 0.0;
    }
    const double mean =
        std::accumulate(values.cbegin(), values.cend(), 0.0)
        / static_cast<double>(values.size());
    double sse = 0.0;
    for (double value : values) {
        const double delta = value - mean;
        sse += delta * delta;
    }
    return sse;
}

std::size_t majority_class(const std::vector<std::size_t>& counts)
{
    std::size_t best = 0;
    for (std::size_t index = 1; index < counts.size(); ++index) {
        if (counts[index] > counts[best]) {
            best = index;
        }
    }
    return best;
}

}  // namespace

CartTreeResult fit_cart_tree(
    const std::vector<std::vector<double>>& predictors,
    const std::vector<double>& response,
    const std::vector<std::string>& class_labels,
    const std::vector<std::string>& predictor_names,
    const CartTreeOptions& options)
{
    CartTreeResult result;
    result.task = options.task;
    result.max_depth = options.max_depth;
    result.predictor_names = predictor_names;
    result.class_labels = class_labels;

    if (predictors.empty() || predictors.size() != response.size()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "cart_empty",
            "CART 需要预测矩阵与响应对齐且非空。"});
        return result;
    }
    const std::size_t predictor_count = predictors.front().size();
    result.predictor_count = predictor_count;
    if (predictor_count == 0) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "cart_no_predictors",
            "至少需要一个数值预测变量。"});
        return result;
    }

    std::vector<std::size_t> keep;
    keep.reserve(response.size());
    for (std::size_t row = 0; row < response.size(); ++row) {
        if (!std::isfinite(response[row]) || predictors[row].size() != predictor_count) {
            continue;
        }
        bool finite = true;
        for (double value : predictors[row]) {
            if (!std::isfinite(value)) {
                finite = false;
                break;
            }
        }
        if (!finite) {
            continue;
        }
        if (options.task == CartTask::classification) {
            const auto class_index = static_cast<std::size_t>(std::llround(response[row]));
            if (class_index >= class_labels.size()
                || std::abs(response[row] - static_cast<double>(class_index)) > 1.0e-9) {
                continue;
            }
        }
        keep.push_back(row);
    }
    result.valid_rows = keep;
    result.observation_count = keep.size();
    if (keep.size() < options.min_leaf * 2) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "cart_insufficient_n",
            "有效观测过少，无法按 min_leaf 分裂。"});
        return result;
    }

    result.variable_importance.assign(predictor_count, 0.0);
    const std::size_t class_count =
        options.task == CartTask::classification ? class_labels.size() : 0;

    struct WorkItem {
        std::vector<std::size_t> indices;
        int parent_id = -1;
        std::size_t depth = 0;
        bool is_left = true;
    };

    std::vector<WorkItem> stack;
    stack.push_back({keep, -1, 0, true});

    while (!stack.empty()) {
        WorkItem item = std::move(stack.back());
        stack.pop_back();
        CartNode node;
        node.id = static_cast<int>(result.nodes.size());
        node.parent_id = item.parent_id;
        node.depth = item.depth;
        node.n = item.indices.size();

        if (item.parent_id >= 0 && item.parent_id < static_cast<int>(result.nodes.size())) {
            if (item.is_left) {
                result.nodes[static_cast<std::size_t>(item.parent_id)].left_child = node.id;
            } else {
                result.nodes[static_cast<std::size_t>(item.parent_id)].right_child = node.id;
            }
        }

        if (options.task == CartTask::classification) {
            std::vector<std::size_t> counts(class_count, 0);
            for (std::size_t index : item.indices) {
                const auto class_index =
                    static_cast<std::size_t>(std::llround(response[index]));
                ++counts[class_index];
            }
            node.impurity = gini_impurity(counts, item.indices.size());
            const std::size_t predicted = majority_class(counts);
            node.prediction = static_cast<double>(predicted);
            node.prediction_label =
                predicted < class_labels.size() ? class_labels[predicted] : "?";
        } else {
            std::vector<double> values;
            values.reserve(item.indices.size());
            for (std::size_t index : item.indices) {
                values.push_back(response[index]);
            }
            node.impurity = sse_impurity(values);
            node.prediction =
                std::accumulate(values.cbegin(), values.cend(), 0.0)
                / static_cast<double>(values.size());
            node.prediction_label = std::to_string(node.prediction);
        }

        bool can_split = item.depth < options.max_depth
            && item.indices.size() >= options.min_leaf * 2
            && node.impurity > 1.0e-12;

        std::size_t best_variable = 0;
        double best_threshold = 0.0;
        double best_gain = 0.0;
        std::vector<std::size_t> best_left;
        std::vector<std::size_t> best_right;

        if (can_split) {
            for (std::size_t variable = 0; variable < predictor_count; ++variable) {
                std::vector<std::size_t> ordered = item.indices;
                std::sort(ordered.begin(), ordered.end(),
                          [&](std::size_t left, std::size_t right) {
                              return predictors[left][variable]
                                  < predictors[right][variable];
                          });
                for (std::size_t split = options.min_leaf;
                     split + options.min_leaf <= ordered.size(); ++split) {
                    const double left_value = predictors[ordered[split - 1]][variable];
                    const double right_value = predictors[ordered[split]][variable];
                    if (!(right_value > left_value)) {
                        continue;
                    }
                    const double threshold = 0.5 * (left_value + right_value);
                    std::vector<std::size_t> left(ordered.begin(),
                                                  ordered.begin() + static_cast<std::ptrdiff_t>(split));
                    std::vector<std::size_t> right(ordered.begin() + static_cast<std::ptrdiff_t>(split),
                                                   ordered.end());
                    double child_impurity = 0.0;
                    if (options.task == CartTask::classification) {
                        std::vector<std::size_t> left_counts(class_count, 0);
                        std::vector<std::size_t> right_counts(class_count, 0);
                        for (std::size_t index : left) {
                            ++left_counts[static_cast<std::size_t>(
                                std::llround(response[index]))];
                        }
                        for (std::size_t index : right) {
                            ++right_counts[static_cast<std::size_t>(
                                std::llround(response[index]))];
                        }
                        child_impurity =
                            (static_cast<double>(left.size())
                                 * gini_impurity(left_counts, left.size())
                             + static_cast<double>(right.size())
                                 * gini_impurity(right_counts, right.size()))
                            / static_cast<double>(item.indices.size());
                    } else {
                        std::vector<double> left_values;
                        std::vector<double> right_values;
                        left_values.reserve(left.size());
                        right_values.reserve(right.size());
                        for (std::size_t index : left) {
                            left_values.push_back(response[index]);
                        }
                        for (std::size_t index : right) {
                            right_values.push_back(response[index]);
                        }
                        child_impurity = sse_impurity(left_values) + sse_impurity(right_values);
                    }
                    const double gain = node.impurity - child_impurity;
                    if (gain > best_gain) {
                        best_gain = gain;
                        best_variable = variable;
                        best_threshold = threshold;
                        best_left = std::move(left);
                        best_right = std::move(right);
                    }
                }
            }
        }

        if (best_gain > 1.0e-12 && !best_left.empty() && !best_right.empty()) {
            node.is_leaf = false;
            node.split_variable = best_variable;
            node.split_threshold = best_threshold;
            result.variable_importance[best_variable] += best_gain;
            result.nodes.push_back(node);
            stack.push_back({std::move(best_right), node.id, item.depth + 1, false});
            stack.push_back({std::move(best_left), node.id, item.depth + 1, true});
        } else {
            node.is_leaf = true;
            result.nodes.push_back(node);
            ++result.leaf_count;
        }
    }

    result.node_count = result.nodes.size();
    double importance_sum = 0.0;
    for (double value : result.variable_importance) {
        importance_sum += value;
    }
    if (importance_sum > 0.0) {
        for (double& value : result.variable_importance) {
            value /= importance_sum;
        }
        const auto top = static_cast<std::size_t>(
            std::distance(result.variable_importance.cbegin(),
                          std::max_element(result.variable_importance.cbegin(),
                                           result.variable_importance.cend())));
        if (top < predictor_names.size()) {
            result.top_variable = predictor_names[top];
        } else {
            result.top_variable = "X" + std::to_string(top + 1);
        }
    }

    auto predict_one = [&](const std::vector<double>& row) -> const CartNode& {
        const CartNode* current = &result.nodes.front();
        while (!current->is_leaf) {
            const std::size_t variable = *current->split_variable;
            const int child = row[variable] <= *current->split_threshold
                ? current->left_child : current->right_child;
            if (child < 0 || child >= static_cast<int>(result.nodes.size())) {
                break;
            }
            current = &result.nodes[static_cast<std::size_t>(child)];
        }
        return *current;
    };

    result.fitted.assign(response.size(), std::numeric_limits<double>::quiet_NaN());
    result.fitted_labels.assign(response.size(), "");
    if (options.task == CartTask::classification) {
        result.confusion.assign(class_count, std::vector<std::size_t>(class_count, 0));
    }
    std::size_t correct = 0;
    double sse = 0.0;
    for (std::size_t index : keep) {
        const CartNode& leaf = predict_one(predictors[index]);
        result.fitted[index] = leaf.prediction;
        result.fitted_labels[index] = leaf.prediction_label;
        if (options.task == CartTask::classification) {
            const auto actual = static_cast<std::size_t>(std::llround(response[index]));
            const auto predicted = static_cast<std::size_t>(std::llround(leaf.prediction));
            if (actual < class_count && predicted < class_count) {
                ++result.confusion[actual][predicted];
            }
            if (actual == predicted) {
                ++correct;
            }
        } else {
            const double delta = response[index] - leaf.prediction;
            sse += delta * delta;
        }
    }
    if (options.task == CartTask::classification) {
        result.train_metric =
            keep.empty() ? 0.0
                         : static_cast<double>(correct) / static_cast<double>(keep.size());
    } else {
        result.train_metric =
            keep.empty() ? 0.0
                         : std::sqrt(sse / static_cast<double>(keep.size()));
    }
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "cart_scope",
        "自研 CART 单树；非 Minitab TreeNet/Random Forests 数值对齐；本轮无成本复杂度剪枝。"});
    return result;
}

}  // namespace datalab::domain::statistics
