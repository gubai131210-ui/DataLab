#include "domain/statistics/kmeans.h"

#include <cmath>
#include <limits>

namespace datalab::domain::statistics {
namespace {

double squared_distance(
    const std::vector<double>& left,
    const std::vector<double>& right)
{
    double sum = 0.0;
    for (std::size_t index = 0; index < left.size() && index < right.size(); ++index) {
        const double delta = left[index] - right[index];
        sum += delta * delta;
    }
    return sum;
}

}  // namespace

KMeansResult cluster_kmeans(
    const std::vector<std::vector<double>>& rows,
    const KMeansOptions& options)
{
    KMeansResult result;
    result.standardized = options.standardize;
    result.cluster_count = options.cluster_count;

    if (options.cluster_count < 2) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "kmeans_invalid_k",
            "K-Means 需要 k ≥ 2。"});
        return result;
    }
    if (rows.empty() || rows.front().empty()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "kmeans_empty",
            "K-Means 需要至少一行有效数值观测。"});
        return result;
    }

    const std::size_t variable_count = rows.front().size();
    std::vector<std::vector<double>> data;
    data.reserve(rows.size());
    for (std::size_t row = 0; row < rows.size(); ++row) {
        if (rows[row].size() != variable_count) {
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
    result.variable_count = variable_count;
    if (data.size() < options.cluster_count) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "kmeans_insufficient_n",
            "有效观测数必须 ≥ k。"});
        return result;
    }

    result.means.assign(variable_count, 0.0);
    result.scales.assign(variable_count, 1.0);
    for (const auto& row : data) {
        for (std::size_t column = 0; column < variable_count; ++column) {
            result.means[column] += row[column];
        }
    }
    for (std::size_t column = 0; column < variable_count; ++column) {
        result.means[column] /= static_cast<double>(data.size());
    }
    if (options.standardize) {
        for (std::size_t column = 0; column < variable_count; ++column) {
            double ss = 0.0;
            for (const auto& row : data) {
                const double delta = row[column] - result.means[column];
                ss += delta * delta;
            }
            const double variance = ss / static_cast<double>(data.size() - 1);
            if (!(variance > 0.0) || !std::isfinite(variance)) {
                result.diagnostics.push_back({
                    DiagnosticMessage::Severity::warning, "kmeans_constant_column",
                    "变量 " + std::to_string(column + 1) + " 方差为 0，标准化尺度取 1。"});
                result.scales[column] = 1.0;
            } else {
                result.scales[column] = std::sqrt(variance);
            }
        }
        for (auto& row : data) {
            for (std::size_t column = 0; column < variable_count; ++column) {
                row[column] = (row[column] - result.means[column]) / result.scales[column];
            }
        }
    }

    result.centroids.assign(options.cluster_count, std::vector<double>(variable_count, 0.0));
    for (std::size_t cluster = 0; cluster < options.cluster_count; ++cluster) {
        result.centroids[cluster] = data[cluster];
    }
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "kmeans_init",
        "初始质心取前 k 个有效观测（分析尺度）；Lloyd 迭代（分配→更新质心）。"});

    result.assignments.assign(data.size(), options.cluster_count);
    result.distances_to_centroid.assign(data.size(), 0.0);
    const std::size_t max_iterations =
        options.max_iterations == 0 ? 100 : options.max_iterations;

    for (std::size_t iteration = 0; iteration < max_iterations; ++iteration) {
        bool moved = false;
        for (std::size_t observation = 0; observation < data.size(); ++observation) {
            std::size_t best = 0;
            double best_ss = std::numeric_limits<double>::infinity();
            for (std::size_t cluster = 0; cluster < options.cluster_count; ++cluster) {
                const double ss = squared_distance(data[observation], result.centroids[cluster]);
                if (ss < best_ss) {
                    best_ss = ss;
                    best = cluster;
                }
            }
            if (result.assignments[observation] != best) {
                moved = true;
                result.assignments[observation] = best;
            }
            result.distances_to_centroid[observation] = std::sqrt(best_ss);
        }

        std::vector<std::vector<double>> sums(
            options.cluster_count, std::vector<double>(variable_count, 0.0));
        std::vector<std::size_t> counts(options.cluster_count, 0);
        for (std::size_t observation = 0; observation < data.size(); ++observation) {
            const std::size_t cluster = result.assignments[observation];
            ++counts[cluster];
            for (std::size_t column = 0; column < variable_count; ++column) {
                sums[cluster][column] += data[observation][column];
            }
        }
        for (std::size_t cluster = 0; cluster < options.cluster_count; ++cluster) {
            if (counts[cluster] == 0) {
                continue;
            }
            for (std::size_t column = 0; column < variable_count; ++column) {
                result.centroids[cluster][column] =
                    sums[cluster][column] / static_cast<double>(counts[cluster]);
            }
        }
        result.cluster_sizes = counts;
        result.iterations = iteration + 1;
        if (!moved) {
            result.converged = true;
            break;
        }
    }
    if (!result.converged) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "kmeans_max_iterations",
            "已达最大迭代次数，分配可能尚未完全稳定。"});
    }

    result.within_ss.assign(options.cluster_count, 0.0);
    result.total_within_ss = 0.0;
    result.cluster_sizes.assign(options.cluster_count, 0);
    for (std::size_t observation = 0; observation < data.size(); ++observation) {
        const std::size_t cluster = result.assignments[observation];
        ++result.cluster_sizes[cluster];
        const double ss = squared_distance(data[observation], result.centroids[cluster]);
        result.within_ss[cluster] += ss;
        result.total_within_ss += ss;
        result.distances_to_centroid[observation] = std::sqrt(ss);
    }
    return result;
}

}  // namespace datalab::domain::statistics
