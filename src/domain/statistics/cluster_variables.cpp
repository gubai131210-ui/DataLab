#include "domain/statistics/cluster_variables.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>

namespace datalab::domain::statistics {
namespace {

double pearson_correlation(
    const std::vector<double>& left,
    const std::vector<double>& right)
{
    if (left.size() != right.size() || left.size() < 2) {
        return 0.0;
    }
    const double n = static_cast<double>(left.size());
    double mean_left = 0.0;
    double mean_right = 0.0;
    for (std::size_t index = 0; index < left.size(); ++index) {
        mean_left += left[index];
        mean_right += right[index];
    }
    mean_left /= n;
    mean_right /= n;
    double numerator = 0.0;
    double denom_left = 0.0;
    double denom_right = 0.0;
    for (std::size_t index = 0; index < left.size(); ++index) {
        const double delta_left = left[index] - mean_left;
        const double delta_right = right[index] - mean_right;
        numerator += delta_left * delta_right;
        denom_left += delta_left * delta_left;
        denom_right += delta_right * delta_right;
    }
    const double denominator = std::sqrt(denom_left * denom_right);
    if (!(denominator > 0.0)) {
        return 0.0;
    }
    return numerator / denominator;
}

double linkage_distance(
    const std::string& linkage,
    const std::map<int, std::vector<int>>& members,
    const std::vector<std::vector<double>>& distance,
    int left,
    int right)
{
    double best = std::numeric_limits<double>::infinity();
    double sum = 0.0;
    std::size_t pair_count = 0;
    for (int i : members.at(left)) {
        for (int j : members.at(right)) {
            const double value = distance[static_cast<std::size_t>(i)]
                                         [static_cast<std::size_t>(j)];
            if (linkage == "single") {
                best = std::min(best, value);
            } else if (linkage == "average") {
                sum += value;
                ++pair_count;
            } else {
                best = std::max(best, value);
            }
        }
    }
    if (linkage == "average") {
        return pair_count > 0 ? sum / static_cast<double>(pair_count) : 0.0;
    }
    return best;
}

}  // namespace

ClusterVariablesResult cluster_variables_analyze(
    const std::vector<std::vector<double>>& data,
    const std::vector<std::string>& variable_labels,
    const ClusterVariablesOptions& options)
{
    ClusterVariablesResult result;
    result.linkage = options.linkage.empty() ? "complete" : options.linkage;
    if (data.empty() || data.front().empty()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "cluster_var_empty",
            "变量聚类需要非空数值矩阵。"});
        return result;
    }
    const std::size_t observations = data.size();
    const std::size_t variables = data.front().size();
    result.observation_count = observations;
    result.variable_count = variables;
    if (variables < 3) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "cluster_var_min_vars",
            "变量聚类至少需要 3 个数值变量。"});
        return result;
    }
    for (std::size_t column = 0; column < variables; ++column) {
        result.variable_labels.push_back(
            column < variable_labels.size()
                ? variable_labels[column]
                : ("V" + std::to_string(column + 1)));
    }

    std::vector<std::vector<double>> columns(variables);
    for (std::size_t column = 0; column < variables; ++column) {
        columns[column].reserve(observations);
        for (std::size_t row = 0; row < observations; ++row) {
            if (column >= data[row].size() || !std::isfinite(data[row][column])) {
                result.diagnostics.push_back({
                    DiagnosticMessage::Severity::error, "cluster_var_nonfinite",
                    "变量列含非有限值。"});
                return result;
            }
            columns[column].push_back(data[row][column]);
        }
    }

    result.correlation_matrix.assign(
        variables, std::vector<double>(variables, 0.0));
    result.distance_matrix.assign(
        variables, std::vector<double>(variables, 0.0));
    for (std::size_t i = 0; i < variables; ++i) {
        result.correlation_matrix[i][i] = 1.0;
        result.distance_matrix[i][i] = 0.0;
        for (std::size_t j = i + 1; j < variables; ++j) {
            const double rho = pearson_correlation(columns[i], columns[j]);
            result.correlation_matrix[i][j] = rho;
            result.correlation_matrix[j][i] = rho;
            const double distance = options.use_absolute_correlation
                ? 1.0 - std::abs(rho) : 1.0 - rho;
            result.distance_matrix[i][j] = distance;
            result.distance_matrix[j][i] = distance;
            result.max_distance = std::max(result.max_distance, distance);
        }
    }
    if (!(result.max_distance > 0.0)) {
        result.max_distance = 1.0;
    }

    std::map<int, std::vector<int>> members;
    std::set<int> active;
    for (std::size_t index = 0; index < variables; ++index) {
        members[static_cast<int>(index)] = {static_cast<int>(index)};
        active.insert(static_cast<int>(index));
    }
    int next_id = static_cast<int>(variables);
    while (active.size() > 1) {
        double best = std::numeric_limits<double>::infinity();
        int best_left = -1;
        int best_right = -1;
        for (auto left = active.begin(); left != active.end(); ++left) {
            auto right = left;
            ++right;
            for (; right != active.end(); ++right) {
                const double distance = linkage_distance(
                    result.linkage, members, result.distance_matrix,
                    *left, *right);
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
        ClusterVariablesMerge merge;
        merge.step = result.merges.size() + 1;
        merge.left_id = best_left;
        merge.right_id = best_right;
        merge.new_id = next_id;
        merge.height = best;
        merge.similarity = 100.0 * (1.0 - best / result.max_distance);
        auto label_for = [&](int id) {
            if (id >= 0 && static_cast<std::size_t>(id) < variables) {
                return result.variable_labels[static_cast<std::size_t>(id)];
            }
            return std::string("C") + std::to_string(id);
        };
        merge.left_label = label_for(best_left);
        merge.right_label = label_for(best_right);
        result.merges.push_back(merge);

        std::vector<int> combined = members[best_left];
        combined.insert(combined.end(),
                        members[best_right].begin(), members[best_right].end());
        members[next_id] = std::move(combined);
        members.erase(best_left);
        members.erase(best_right);
        active.erase(best_left);
        active.erase(best_right);
        active.insert(next_id);
        ++next_id;
    }

    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "cluster_var_linkage",
        "Pearson 相关距离 + " + result.linkage + " 连结；非 Minitab golden。"});
    return result;
}

}  // namespace datalab::domain::statistics
