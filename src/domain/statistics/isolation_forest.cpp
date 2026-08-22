#include "domain/statistics/isolation_forest.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>

namespace datalab::domain::statistics {
namespace {

struct IsoNode {
    bool is_leaf = true;
    std::size_t split_variable = 0;
    double split_value = 0.0;
    int left = -1;
    int right = -1;
    std::size_t size = 0;
};

double average_path_length(std::size_t n)
{
    if (n <= 1) {
        return 0.0;
    }
    if (n == 2) {
        return 1.0;
    }
    const double nn = static_cast<double>(n);
    const double harmonic = std::log(nn - 1.0) + 0.5772156649;
    return 2.0 * harmonic - 2.0 * (nn - 1.0) / nn;
}

int build_tree(
    std::vector<IsoNode>& nodes,
    const std::vector<std::vector<double>>& sample,
    std::vector<std::size_t> indices,
    std::size_t depth,
    std::size_t max_depth,
    std::mt19937& rng)
{
    IsoNode node;
    node.size = indices.size();
    const int id = static_cast<int>(nodes.size());
    nodes.push_back(node);
    if (indices.size() <= 1 || depth >= max_depth || sample.empty()) {
        nodes[static_cast<std::size_t>(id)].is_leaf = true;
        return id;
    }
    const std::size_t variables = sample.front().size();
    std::uniform_int_distribution<std::size_t> var_dist(0, variables - 1);
    const std::size_t variable = var_dist(rng);
    double min_value = std::numeric_limits<double>::infinity();
    double max_value = -std::numeric_limits<double>::infinity();
    for (std::size_t index : indices) {
        min_value = std::min(min_value, sample[index][variable]);
        max_value = std::max(max_value, sample[index][variable]);
    }
    if (!(max_value > min_value)) {
        nodes[static_cast<std::size_t>(id)].is_leaf = true;
        return id;
    }
    std::uniform_real_distribution<double> split_dist(min_value, max_value);
    const double split = split_dist(rng);
    std::vector<std::size_t> left;
    std::vector<std::size_t> right;
    for (std::size_t index : indices) {
        if (sample[index][variable] < split) {
            left.push_back(index);
        } else {
            right.push_back(index);
        }
    }
    if (left.empty() || right.empty()) {
        nodes[static_cast<std::size_t>(id)].is_leaf = true;
        return id;
    }
    nodes[static_cast<std::size_t>(id)].is_leaf = false;
    nodes[static_cast<std::size_t>(id)].split_variable = variable;
    nodes[static_cast<std::size_t>(id)].split_value = split;
    nodes[static_cast<std::size_t>(id)].left =
        build_tree(nodes, sample, std::move(left), depth + 1, max_depth, rng);
    nodes[static_cast<std::size_t>(id)].right =
        build_tree(nodes, sample, std::move(right), depth + 1, max_depth, rng);
    return id;
}

double path_length(
    const std::vector<IsoNode>& nodes,
    int root,
    const std::vector<double>& row)
{
    double length = 0.0;
    int current = root;
    while (current >= 0 && current < static_cast<int>(nodes.size())) {
        const IsoNode& node = nodes[static_cast<std::size_t>(current)];
        if (node.is_leaf) {
            length += average_path_length(node.size);
            break;
        }
        ++length;
        current = row[node.split_variable] < node.split_value ? node.left : node.right;
    }
    return length;
}

}  // namespace

IsolationForestResult isolation_forest(
    const std::vector<std::vector<double>>& rows,
    const IsolationForestOptions& options)
{
    IsolationForestResult result;
    result.tree_count = options.tree_count;
    if (rows.empty() || rows.front().empty() || options.tree_count == 0) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "iforest_empty",
            "Isolation Forest 需要非空矩阵与 tree_count ≥ 1。"});
        return result;
    }
    const std::size_t variables = rows.front().size();
    result.variable_count = variables;
    std::vector<std::vector<double>> data;
    for (std::size_t row = 0; row < rows.size(); ++row) {
        if (rows[row].size() != variables) {
            continue;
        }
        bool finite = true;
        for (double value : rows[row]) {
            if (!std::isfinite(value)) {
                finite = false;
                break;
            }
        }
        if (!finite) {
            continue;
        }
        data.push_back(rows[row]);
        result.valid_rows.push_back(row);
    }
    result.observation_count = data.size();
    if (data.size() < 2 || variables < 2) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "iforest_insufficient",
            "至少需要 2 个观测与 2 个变量。"});
        return result;
    }

    const std::size_t max_samples =
        std::min(options.max_samples == 0 ? data.size() : options.max_samples, data.size());
    result.max_samples = max_samples;
    const double c_n = average_path_length(max_samples);
    std::mt19937 rng(options.seed);
    std::vector<double> expected_lengths(data.size(), 0.0);

    for (std::size_t tree = 0; tree < options.tree_count; ++tree) {
        std::vector<std::size_t> sample_index(data.size());
        std::iota(sample_index.begin(), sample_index.end(), 0);
        std::shuffle(sample_index.begin(), sample_index.end(), rng);
        sample_index.resize(max_samples);
        std::vector<std::vector<double>> sample;
        sample.reserve(max_samples);
        for (std::size_t index : sample_index) {
            sample.push_back(data[index]);
        }
        std::vector<std::size_t> all(max_samples);
        std::iota(all.begin(), all.end(), 0);
        const std::size_t max_depth =
            static_cast<std::size_t>(std::ceil(std::log2(std::max<std::size_t>(2, max_samples))));
        std::vector<IsoNode> nodes;
        const int root = build_tree(nodes, sample, std::move(all), 0, max_depth, rng);
        for (std::size_t row = 0; row < data.size(); ++row) {
            expected_lengths[row] += path_length(nodes, root, data[row]);
        }
    }

    result.scores.assign(data.size(), 0.0);
    for (std::size_t row = 0; row < data.size(); ++row) {
        const double mean_length =
            expected_lengths[row] / static_cast<double>(options.tree_count);
        result.scores[row] =
            c_n > 0.0 ? std::pow(2.0, -mean_length / c_n) : 0.0;
    }
    std::vector<double> ordered = result.scores;
    std::sort(ordered.begin(), ordered.end());
    const double q = std::clamp(options.score_quantile, 0.5, 0.999);
    const std::size_t q_index = static_cast<std::size_t>(
        std::min(ordered.size() - 1,
                 static_cast<std::size_t>(std::floor(q * static_cast<double>(ordered.size())))));
    result.score_threshold = ordered[q_index];
    result.anomaly.assign(data.size(), false);
    for (std::size_t row = 0; row < data.size(); ++row) {
        if (result.scores[row] >= result.score_threshold) {
            result.anomaly[row] = true;
            ++result.anomaly_count;
        }
    }
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "iforest_scope",
        "自研 Isolation Forest；多元异常辅助；非单变量 Grubbs/Dixon；非 TreeNet。"});
    return result;
}

}  // namespace datalab::domain::statistics
