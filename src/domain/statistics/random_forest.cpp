#include "domain/statistics/random_forest.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <random>

namespace datalab::domain::statistics {
namespace {

std::size_t majority_index(const std::vector<std::size_t>& votes)
{
    std::size_t best = 0;
    for (std::size_t index = 1; index < votes.size(); ++index) {
        if (votes[index] > votes[best]) {
            best = index;
        }
    }
    return best;
}

}  // namespace

RandomForestResult fit_random_forest(
    const std::vector<std::vector<double>>& predictors,
    const std::vector<double>& response,
    const std::vector<std::string>& class_labels,
    const std::vector<std::string>& predictor_names,
    const RandomForestOptions& options)
{
    RandomForestResult result;
    result.task = options.task;
    result.n_trees = options.n_trees;
    result.max_depth = options.max_depth;
    result.predictor_names = predictor_names;
    result.class_labels = class_labels;
    result.disclosure =
        "Bagging CART ensemble; NOT TreeNet / Minitab Random Forests aligned.";

    if (predictors.empty() || predictors.size() != response.size()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "rf_empty",
            "随机森林需要预测矩阵与响应对齐且非空。"});
        return result;
    }
    const std::size_t predictor_count = predictors.front().size();
    result.predictor_count = predictor_count;
    if (predictor_count == 0 || options.n_trees == 0) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "rf_invalid",
            "至少需要一个预测变量且树数 ≥ 1。"});
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
            DiagnosticMessage::Severity::error, "rf_insufficient_n",
            "有效观测过少，无法拟合随机森林。"});
        return result;
    }

    result.variable_importance.assign(predictor_count, 0.0);
    result.fitted.assign(keep.size(), 0.0);
    if (options.task == CartTask::classification) {
        result.fitted_labels.assign(keep.size(), "");
        result.confusion.assign(
            class_labels.size(), std::vector<std::size_t>(class_labels.size(), 0));
    }

    std::vector<std::vector<std::size_t>> class_votes;
    std::vector<double> regression_sum;
    std::vector<std::size_t> vote_count;
    if (options.task == CartTask::classification) {
        class_votes.assign(keep.size(), std::vector<std::size_t>(class_labels.size(), 0));
    } else {
        regression_sum.assign(keep.size(), 0.0);
        vote_count.assign(keep.size(), 0);
    }

    std::vector<std::vector<std::size_t>> oob_class_votes;
    std::vector<double> oob_reg_sum;
    std::vector<std::size_t> oob_count;
    if (options.compute_oob) {
        if (options.task == CartTask::classification) {
            oob_class_votes.assign(
                keep.size(), std::vector<std::size_t>(class_labels.size(), 0));
        } else {
            oob_reg_sum.assign(keep.size(), 0.0);
            oob_count.assign(keep.size(), 0);
        }
    }

    CartTreeOptions tree_options;
    tree_options.task = options.task;
    tree_options.max_depth = options.max_depth;
    tree_options.min_leaf = options.min_leaf;

    std::mt19937 rng(options.seed == 0 ? 1u : options.seed);
    std::uniform_int_distribution<std::size_t> pick(0, keep.size() - 1);

    std::size_t trees_ok = 0;
    for (std::size_t tree = 0; tree < options.n_trees; ++tree) {
        std::vector<std::size_t> bootstrap_local;
        bootstrap_local.reserve(keep.size());
        std::vector<char> in_bag(keep.size(), 0);
        for (std::size_t i = 0; i < keep.size(); ++i) {
            const std::size_t local = pick(rng);
            bootstrap_local.push_back(local);
            in_bag[local] = 1;
        }

        std::vector<std::vector<double>> boot_x;
        std::vector<double> boot_y;
        boot_x.reserve(bootstrap_local.size());
        boot_y.reserve(bootstrap_local.size());
        for (std::size_t local : bootstrap_local) {
            const std::size_t global = keep[local];
            boot_x.push_back(predictors[global]);
            boot_y.push_back(response[global]);
        }

        const CartTreeResult tree_fit = fit_cart_tree(
            boot_x, boot_y, class_labels, predictor_names, tree_options);
        if (tree_fit.nodes.empty()) {
            continue;
        }
        ++trees_ok;
        for (std::size_t v = 0; v < tree_fit.variable_importance.size()
             && v < result.variable_importance.size();
             ++v) {
            result.variable_importance[v] += tree_fit.variable_importance[v];
        }

        // Predict every original keep-row with this tree (train aggregate).
        for (std::size_t local = 0; local < keep.size(); ++local) {
            const std::size_t global = keep[local];
            // Walk tree on full row.
            int node_id = 0;
            double prediction = 0.0;
            std::string label;
            while (node_id >= 0
                   && static_cast<std::size_t>(node_id) < tree_fit.nodes.size()) {
                const CartNode& node = tree_fit.nodes[static_cast<std::size_t>(node_id)];
                if (node.is_leaf || !node.split_variable.has_value()
                    || !node.split_threshold.has_value()) {
                    prediction = node.prediction;
                    label = node.prediction_label;
                    break;
                }
                const std::size_t var = *node.split_variable;
                const double x =
                    var < predictors[global].size() ? predictors[global][var] : 0.0;
                node_id = x <= *node.split_threshold ? node.left_child : node.right_child;
            }

            if (options.task == CartTask::classification) {
                const auto cls = static_cast<std::size_t>(std::llround(prediction));
                if (cls < class_votes[local].size()) {
                    ++class_votes[local][cls];
                }
                if (options.compute_oob && !in_bag[local] && cls < oob_class_votes[local].size()) {
                    ++oob_class_votes[local][cls];
                }
                (void)label;
            } else {
                regression_sum[local] += prediction;
                ++vote_count[local];
                if (options.compute_oob && !in_bag[local]) {
                    oob_reg_sum[local] += prediction;
                    ++oob_count[local];
                }
            }
        }
    }

    if (trees_ok == 0) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "rf_no_trees",
            "所有 bootstrap 树拟合失败。"});
        return result;
    }
    result.n_trees = trees_ok;

    const double inv_trees = 1.0 / static_cast<double>(trees_ok);
    for (double& importance : result.variable_importance) {
        importance *= inv_trees;
    }
    std::size_t top = 0;
    for (std::size_t index = 1; index < result.variable_importance.size(); ++index) {
        if (result.variable_importance[index] > result.variable_importance[top]) {
            top = index;
        }
    }
    result.top_variable = top < predictor_names.size() ? predictor_names[top]
                                                       : ("X" + std::to_string(top + 1));

    if (options.task == CartTask::classification) {
        std::size_t correct = 0;
        for (std::size_t local = 0; local < keep.size(); ++local) {
            const std::size_t predicted = majority_index(class_votes[local]);
            result.fitted[local] = static_cast<double>(predicted);
            result.fitted_labels[local] =
                predicted < class_labels.size() ? class_labels[predicted] : "?";
            const auto actual =
                static_cast<std::size_t>(std::llround(response[keep[local]]));
            if (actual < result.confusion.size()
                && predicted < result.confusion[actual].size()) {
                ++result.confusion[actual][predicted];
            }
            if (predicted == actual) {
                ++correct;
            }
        }
        result.train_metric =
            static_cast<double>(correct) / static_cast<double>(keep.size());

        if (options.compute_oob) {
            std::size_t oob_n = 0;
            std::size_t oob_correct = 0;
            for (std::size_t local = 0; local < keep.size(); ++local) {
                std::size_t total = 0;
                for (std::size_t v : oob_class_votes[local]) {
                    total += v;
                }
                if (total == 0) {
                    continue;
                }
                const std::size_t predicted = majority_index(oob_class_votes[local]);
                const auto actual =
                    static_cast<std::size_t>(std::llround(response[keep[local]]));
                ++oob_n;
                if (predicted == actual) {
                    ++oob_correct;
                }
            }
            if (oob_n > 0) {
                result.oob_metric =
                    static_cast<double>(oob_correct) / static_cast<double>(oob_n);
            }
        }
    } else {
        double sse = 0.0;
        for (std::size_t local = 0; local < keep.size(); ++local) {
            const double pred = vote_count[local] > 0
                ? regression_sum[local] / static_cast<double>(vote_count[local])
                : 0.0;
            result.fitted[local] = pred;
            const double delta = response[keep[local]] - pred;
            sse += delta * delta;
        }
        result.train_metric = std::sqrt(sse / static_cast<double>(keep.size()));

        if (options.compute_oob) {
            double oob_sse = 0.0;
            std::size_t oob_n = 0;
            for (std::size_t local = 0; local < keep.size(); ++local) {
                if (oob_count[local] == 0) {
                    continue;
                }
                const double pred =
                    oob_reg_sum[local] / static_cast<double>(oob_count[local]);
                const double delta = response[keep[local]] - pred;
                oob_sse += delta * delta;
                ++oob_n;
            }
            if (oob_n > 0) {
                result.oob_metric = std::sqrt(oob_sse / static_cast<double>(oob_n));
            }
        }
    }

    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "rf_disclosure",
        result.disclosure});
    return result;
}

}  // namespace datalab::domain::statistics
