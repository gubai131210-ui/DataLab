#include "domain/statistics/hierarchical_cluster.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>

namespace datalab::domain::statistics {
namespace {

double euclidean(const std::vector<double>& left, const std::vector<double>& right)
{
    double sum = 0.0;
    for (std::size_t index = 0; index < left.size() && index < right.size(); ++index) {
        const double delta = left[index] - right[index];
        sum += delta * delta;
    }
    return std::sqrt(sum);
}

}  // namespace

HierarchicalClusterResult cluster_observations_complete(
    const std::vector<std::vector<double>>& rows,
    const HierarchicalClusterOptions& options)
{
    HierarchicalClusterResult result;
    result.standardized = options.standardize;
    result.cluster_count = options.cluster_count;
    if (rows.empty() || rows.front().empty() || options.cluster_count < 2) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "hclust_invalid",
            "层次聚类需要非空矩阵且 k≥2。"});
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
    if (data.size() < options.cluster_count) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "hclust_insufficient_n",
            "有效观测数必须 ≥ k。"});
        return result;
    }

    if (options.standardize) {
        std::vector<double> means(variables, 0.0);
        std::vector<double> scales(variables, 1.0);
        for (const auto& row : data) {
            for (std::size_t column = 0; column < variables; ++column) {
                means[column] += row[column];
            }
        }
        for (std::size_t column = 0; column < variables; ++column) {
            means[column] /= static_cast<double>(data.size());
        }
        for (std::size_t column = 0; column < variables; ++column) {
            double ss = 0.0;
            for (const auto& row : data) {
                const double delta = row[column] - means[column];
                ss += delta * delta;
            }
            const double variance = ss / static_cast<double>(data.size() - 1);
            scales[column] = variance > 0.0 ? std::sqrt(variance) : 1.0;
        }
        for (auto& row : data) {
            for (std::size_t column = 0; column < variables; ++column) {
                row[column] = (row[column] - means[column]) / scales[column];
            }
        }
    }

    const std::size_t n = data.size();
    // Cluster ids: 0..n-1 leaves; new ids n, n+1, ...
    std::map<int, std::vector<std::size_t>> members;
    for (std::size_t index = 0; index < n; ++index) {
        members[static_cast<int>(index)] = {index};
    }
    std::set<int> active;
    for (std::size_t index = 0; index < n; ++index) {
        active.insert(static_cast<int>(index));
    }

    auto complete_distance = [&](int left, int right) {
        double max_distance = 0.0;
        for (std::size_t i : members[left]) {
            for (std::size_t j : members[right]) {
                max_distance = std::max(max_distance, euclidean(data[i], data[j]));
            }
        }
        return max_distance;
    };

    int next_id = static_cast<int>(n);
    while (active.size() > 1) {
        double best = std::numeric_limits<double>::infinity();
        int best_left = -1;
        int best_right = -1;
        for (auto left = active.begin(); left != active.end(); ++left) {
            auto right = left;
            ++right;
            for (; right != active.end(); ++right) {
                const double distance = complete_distance(*left, *right);
                if (distance < best) {
                    best = distance;
                    best_left = *left;
                    best_right = *right;
                }
            }
        }
        if (best_left < 0 || best_right < 0) {
            break;
        }
        HierarchicalMerge merge;
        merge.step = result.merges.size() + 1;
        merge.left_id = best_left;
        merge.right_id = best_right;
        merge.new_id = next_id;
        merge.height = best;
        result.merges.push_back(merge);

        std::vector<std::size_t> combined = members[best_left];
        combined.insert(combined.end(), members[best_right].begin(), members[best_right].end());
        members[next_id] = std::move(combined);
        members.erase(best_left);
        members.erase(best_right);
        active.erase(best_left);
        active.erase(best_right);
        active.insert(next_id);
        ++next_id;
    }

    // Cut into k clusters: stop when active size would be k — equivalently take
    // the last (n-k) merges' resulting partition.
    const std::size_t merges_to_apply = n > options.cluster_count
        ? n - options.cluster_count : 0;
    std::map<int, std::vector<std::size_t>> cut_members;
    for (std::size_t index = 0; index < n; ++index) {
        cut_members[static_cast<int>(index)] = {index};
    }
    for (std::size_t step = 0; step < merges_to_apply && step < result.merges.size(); ++step) {
        const auto& merge = result.merges[step];
        std::vector<std::size_t> combined = cut_members[merge.left_id];
        combined.insert(combined.end(),
                        cut_members[merge.right_id].begin(),
                        cut_members[merge.right_id].end());
        cut_members.erase(merge.left_id);
        cut_members.erase(merge.right_id);
        cut_members[merge.new_id] = std::move(combined);
    }
    result.assignments.assign(n, 0);
    result.cluster_sizes.clear();
    std::size_t label = 0;
    for (const auto& entry : cut_members) {
        result.cluster_sizes.push_back(entry.second.size());
        for (std::size_t index : entry.second) {
            result.assignments[index] = label;
        }
        ++label;
    }
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "hclust_linkage",
        "Complete linkage + 欧氏距离；非 Minitab golden。"});
    return result;
}

}  // namespace datalab::domain::statistics
