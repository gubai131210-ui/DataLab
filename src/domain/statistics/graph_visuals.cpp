#include "domain/statistics/graph_visuals.h"

#include "domain/graph_assembly.h"
#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/normal_probability.h"
#include "domain/statistics/quality_visuals.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <set>

namespace datalab::domain::statistics {
namespace {

void add_error(std::vector<DiagnosticMessage>& diagnostics,
               const char* code,
               const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

bool valid_confidence(const double value)
{
    return value > 0.0 && value < 1.0 && std::isfinite(value);
}

std::size_t group_index(std::vector<std::string>& labels, const std::string& value)
{
    const auto found = std::find(labels.cbegin(), labels.cend(), value);
    if (found != labels.cend()) {
        return static_cast<std::size_t>(std::distance(labels.cbegin(), found));
    }
    labels.push_back(value);
    return labels.size() - 1;
}

}  // namespace

ScatterPlotResult scatter_plot(
    const std::vector<double>& x_values,
    const std::vector<double>& y_values,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& point_groups,
    const std::vector<std::string>& point_labels)
{
    ScatterPlotResult result;
    const std::size_t count = std::min({x_values.size(), y_values.size(), source_rows.size()});
    for (std::size_t index = 0; index < count; ++index) {
        if (!std::isfinite(x_values[index]) || !std::isfinite(y_values[index])) {
            continue;
        }
        result.x_values.push_back(x_values[index]);
        result.y_values.push_back(y_values[index]);
        result.source_rows.push_back(source_rows[index]);
        if (point_groups.size() == count) {
            result.point_groups.push_back(point_groups[index]);
        }
        if (point_labels.size() == count) {
            result.point_labels.push_back(point_labels[index]);
        }
    }
    if (result.x_values.size() < 2) {
        add_error(result.diagnostics, "insufficient_points",
                  "散点图至少需要两个有效的完整观测行。");
    }
    return result;
}

IntervalPlotResult interval_plot(
    const std::vector<double>& values,
    const std::vector<std::string>& groups,
    const std::vector<std::size_t>& source_rows,
    const double confidence_level)
{
    IntervalPlotResult result;
    if (!valid_confidence(confidence_level)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    const std::size_t count = std::min({values.size(), groups.size(), source_rows.size()});
    std::vector<std::vector<double>> grouped_values;
    for (std::size_t index = 0; index < count; ++index) {
        if (!std::isfinite(values[index])) {
            continue;
        }
        const std::size_t group = group_index(result.labels, groups[index]);
        if (group >= grouped_values.size()) {
            grouped_values.emplace_back();
            result.source_rows.emplace_back();
        }
        grouped_values[group].push_back(values[index]);
        result.source_rows[group].push_back(source_rows[index]);
    }
    const double probability = 0.5 + confidence_level / 2.0;
    for (const std::vector<double>& observations : grouped_values) {
        const std::size_t n = observations.size();
        result.counts.push_back(n);
        const double mean = n == 0
            ? 0.0
            : std::accumulate(observations.cbegin(), observations.cend(), 0.0)
                / static_cast<double>(n);
        result.means.push_back(mean);
        if (n < 2) {
            result.lower.push_back(mean);
            result.upper.push_back(mean);
            add_error(result.diagnostics, "insufficient_group_points",
                      "区间图存在有效观测数少于 2 的分组，无法估计标准误。");
            continue;
        }
        double squared_error = 0.0;
        for (const double value : observations) {
            squared_error += (value - mean) * (value - mean);
        }
        const double standard_deviation =
            std::sqrt(squared_error / static_cast<double>(n - 1));
        const double standard_error = standard_deviation / std::sqrt(static_cast<double>(n));
        const double critical = student_t_quantile(
            probability, static_cast<double>(n - 1));
        result.lower.push_back(mean - critical * standard_error);
        result.upper.push_back(mean + critical * standard_error);
    }
    if (result.labels.empty()) {
        add_error(result.diagnostics, "no_valid_groups",
                  "区间图没有可用于计算的有效分组。");
    }
    return result;
}

BubblePlotResult bubble_plot(
    const std::vector<double>& x_values,
    const std::vector<double>& y_values,
    const std::vector<double>& sizes,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& point_groups,
    const std::vector<std::string>& point_labels)
{
    BubblePlotResult result;
    const std::size_t count =
        std::min({x_values.size(), y_values.size(), sizes.size(), source_rows.size()});
    for (std::size_t index = 0; index < count; ++index) {
        if (!std::isfinite(x_values[index]) || !std::isfinite(y_values[index])
            || !std::isfinite(sizes[index])) {
            continue;
        }
        if (sizes[index] < 0.0) {
            add_error(result.diagnostics, "negative_bubble_size",
                      "气泡大小不能为负数。");
            continue;
        }
        result.points.x_values.push_back(x_values[index]);
        result.points.y_values.push_back(y_values[index]);
        result.points.source_rows.push_back(source_rows[index]);
        result.sizes.push_back(sizes[index]);
        if (point_groups.size() == count) {
            result.points.point_groups.push_back(point_groups[index]);
        }
        if (point_labels.size() == count) {
            result.points.point_labels.push_back(point_labels[index]);
        }
    }
    if (result.points.x_values.empty()) {
        add_error(result.diagnostics, "no_valid_bubbles",
                  "气泡图没有可用于绘制的完整观测行。");
    }
    result.points.diagnostics = result.diagnostics;
    return result;
}

GraphCorrelationResult correlation_plot(
    const std::vector<std::vector<double>>& columns,
    const std::vector<std::string>& labels,
    const std::string& method,
    const double confidence_level)
{
    GraphCorrelationResult result;
    result.labels = labels;
    const CorrelationMethod selected = method == "spearman"
        ? CorrelationMethod::spearman : CorrelationMethod::pearson;
    result.correlation = correlation_matrix(columns, selected, confidence_level);
    if (columns.size() < 2) {
        add_error(result.correlation.diagnostics, "insufficient_variables",
                  "相关图至少需要选择两个连续变量。");
    }
    return result;
}

EcdfPlotResult ecdf_plot(
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows)
{
    EcdfPlotResult result;
    const std::size_t count = std::min(values.size(), source_rows.size());
    std::vector<std::pair<double, std::size_t>> ordered;
    ordered.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        if (std::isfinite(values[index])) {
            ordered.push_back({values[index], source_rows[index]});
        }
    }
    if (ordered.empty()) {
        add_error(result.diagnostics, "insufficient_points",
                  "经验累积分布图至少需要一个有效观测。");
        return result;
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const auto& left, const auto& right) {
                  return left.first < right.first;
              });
    const double n = static_cast<double>(ordered.size());
    for (std::size_t index = 0; index < ordered.size(); ) {
        std::size_t end = index + 1;
        while (end < ordered.size() && ordered[end].first == ordered[index].first) {
            ++end;
        }
        result.values.push_back(ordered[index].first);
        result.counts.push_back(end - index);
        result.proportions.push_back(static_cast<double>(end) / n);
        result.source_rows.push_back(ordered[index].second);
        index = end;
    }
    return result;
}

ProbabilityPlotResult probability_plot(
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows)
{
    ProbabilityPlotResult result;
    const std::size_t count = std::min(values.size(), source_rows.size());
    std::vector<std::pair<double, std::size_t>> ordered;
    for (std::size_t index = 0; index < count; ++index) {
        if (std::isfinite(values[index])) {
            ordered.push_back({values[index], source_rows[index]});
        }
    }
    if (ordered.size() < 3) {
        add_error(result.diagnostics, "insufficient_points",
                  "正态概率图至少需要三个有效观测。");
        return result;
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const auto& left, const auto& right) {
                  return left.first < right.first;
              });
    std::vector<double> observations;
    observations.reserve(ordered.size());
    for (const auto& item : ordered) {
        observations.push_back(item.first);
        result.source_rows.push_back(item.second);
    }
    const auto probability = normal_probability_plot(observations);
    result.theoretical_quantiles = probability.theoretical_quantiles;
    result.ordered_values = probability.ordered_values;
    result.correlation = probability.correlation;
    const double n = static_cast<double>(observations.size());
    result.location = std::accumulate(
        observations.cbegin(), observations.cend(), 0.0) / n;
    double squared = 0.0;
    for (const double value : observations) {
        squared += (value - result.location) * (value - result.location);
    }
    result.scale = n > 1.0 ? std::sqrt(squared / (n - 1.0)) : 0.0;
    double residual_squared = 0.0;
    double mean_z = 0.0;
    for (const double z : result.theoretical_quantiles) {
        mean_z += z;
    }
    mean_z /= n;
    double sum_zz = 0.0;
    for (std::size_t index = 0; index < observations.size(); ++index) {
        const double fitted = result.location + result.scale * result.theoretical_quantiles[index];
        result.fitted.push_back(fitted);
        residual_squared += (observations[index] - fitted) * (observations[index] - fitted);
        const double dz = result.theoretical_quantiles[index] - mean_z;
        sum_zz += dz * dz;
    }
    const double residual_df = std::max(1.0, n - 2.0);
    const double residual_sigma = std::sqrt(residual_squared / residual_df);
    const double critical = student_t_quantile(0.975, residual_df);
    for (std::size_t index = 0; index < observations.size(); ++index) {
        const double dz = result.theoretical_quantiles[index] - mean_z;
        const double se = residual_sigma * std::sqrt(
            1.0 / n + (sum_zz > 0.0 ? dz * dz / sum_zz : 0.0));
        result.lower.push_back(result.fitted[index] - critical * se);
        result.upper.push_back(result.fitted[index] + critical * se);
    }
    return result;
}

MatrixScatterResult matrix_scatter_plot(
    const std::vector<std::vector<double>>& columns,
    const std::vector<std::string>& labels,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& groups)
{
    MatrixScatterResult result;
    result.labels = labels;
    result.columns = columns;
    result.source_rows = source_rows;
    result.groups = groups;
    if (columns.size() < 2) {
        add_error(result.diagnostics, "insufficient_variables",
                  "矩阵图至少需要两个连续变量。");
    }
    return result;
}

MarginalPlotResult marginal_plot(
    const std::vector<double>& x_values,
    const std::vector<double>& y_values,
    const std::vector<std::size_t>& source_rows,
    const int bin_count)
{
    MarginalPlotResult result;
    result.points = scatter_plot(x_values, y_values, source_rows);
    result.x_histogram = histogram(result.points.x_values, bin_count);
    result.y_histogram = histogram(result.points.y_values, bin_count);
    return result;
}

ParallelPlotResult parallel_plot(
    const std::vector<std::vector<double>>& columns,
    const std::vector<std::string>& labels,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& groups)
{
    ParallelPlotResult result;
    result.labels = labels;
    if (columns.size() < 2) {
        add_error(result.diagnostics, "insufficient_variables",
                  "平行坐标图至少需要两个连续变量。");
        return result;
    }
    const std::size_t row_count = columns.front().size();
    result.minima.assign(columns.size(), 0.0);
    result.maxima.assign(columns.size(), 1.0);
    for (std::size_t column = 0; column < columns.size(); ++column) {
        double minimum = std::numeric_limits<double>::infinity();
        double maximum = -std::numeric_limits<double>::infinity();
        for (const double value : columns[column]) {
            if (!std::isfinite(value)) {
                continue;
            }
            minimum = std::min(minimum, value);
            maximum = std::max(maximum, value);
        }
        if (!std::isfinite(minimum) || !std::isfinite(maximum)) {
            continue;
        }
        result.minima[column] = minimum;
        result.maxima[column] = maximum <= minimum ? minimum + 1.0 : maximum;
    }
    for (std::size_t row = 0; row < row_count; ++row) {
        std::vector<double> observation;
        observation.reserve(columns.size());
        for (const std::vector<double>& column : columns) {
            observation.push_back(row < column.size() ? column[row]
                                                      : std::numeric_limits<double>::quiet_NaN());
        }
        result.rows.push_back(std::move(observation));
        if (row < source_rows.size()) {
            result.source_rows.push_back(source_rows[row]);
        }
        if (row < groups.size()) {
            result.groups.push_back(groups[row]);
        }
    }
    return result;
}

HeatmapPlotResult heatmap_from_correlation(const GraphCorrelationResult& source)
{
    HeatmapPlotResult result;
    result.row_labels = source.labels;
    result.column_labels = source.labels;
    result.values = source.correlation.coefficients;
    result.counts = source.correlation.counts;
    result.color_min = -1.0;
    result.color_max = 1.0;
    result.diagnostics = source.correlation.diagnostics;
    return result;
}

HeatmapPlotResult heatmap_from_categories(
    const std::vector<std::string>& rows,
    const std::vector<std::string>& columns,
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows)
{
    HeatmapPlotResult result;
    const std::size_t count = std::min({rows.size(), columns.size(), values.size()});
    std::vector<std::string> row_order;
    std::vector<std::string> column_order;
    std::map<std::pair<std::string, std::string>, std::pair<double, std::size_t>> cells;
    std::map<std::pair<std::string, std::string>, std::vector<std::size_t>> cell_rows;
    for (std::size_t index = 0; index < count; ++index) {
        if (!std::isfinite(values[index])) {
            continue;
        }
        datalab::domain::stable_group_index(row_order, rows[index]);
        datalab::domain::stable_group_index(column_order, columns[index]);
        auto& cell = cells[{rows[index], columns[index]}];
        cell.first += values[index];
        ++cell.second;
        if (index < source_rows.size()) {
            cell_rows[{rows[index], columns[index]}].push_back(source_rows[index]);
        }
    }
    if (row_order.empty() || column_order.empty()) {
        add_error(result.diagnostics, "no_valid_cells",
                  "热图没有可用于聚合的完整观测。");
        return result;
    }
    result.row_labels = row_order;
    result.column_labels = column_order;
    result.values.assign(row_order.size(), std::vector<double>(column_order.size(), 0.0));
    result.counts.assign(row_order.size(), std::vector<std::size_t>(column_order.size(), 0));
    result.cell_source_rows.assign(
        row_order.size(),
        std::vector<std::vector<std::size_t>>(column_order.size()));
    double minimum = std::numeric_limits<double>::infinity();
    double maximum = -std::numeric_limits<double>::infinity();
    for (std::size_t row = 0; row < row_order.size(); ++row) {
        for (std::size_t column = 0; column < column_order.size(); ++column) {
            const auto key = std::make_pair(row_order[row], column_order[column]);
            const auto found = cells.find(key);
            if (found == cells.end() || found->second.second == 0) {
                continue;
            }
            const double mean = found->second.first
                / static_cast<double>(found->second.second);
            result.values[row][column] = mean;
            result.counts[row][column] = found->second.second;
            result.cell_source_rows[row][column] = cell_rows[key];
            minimum = std::min(minimum, mean);
            maximum = std::max(maximum, mean);
        }
    }
    if (!std::isfinite(minimum) || !std::isfinite(maximum)) {
        minimum = 0.0;
        maximum = 1.0;
    }
    if (maximum <= minimum) {
        maximum = minimum + 1.0;
    }
    result.color_min = minimum;
    result.color_max = maximum;
    return result;
}

TimeSeriesPlotResult time_series_plot(
    const std::vector<double>& times,
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& time_labels,
    const std::vector<std::string>& groups)
{
    TimeSeriesPlotResult result;
    const std::size_t count = std::min({times.size(), values.size(), source_rows.size()});
    std::vector<std::size_t> order(count);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](std::size_t left, std::size_t right) {
        return times[left] < times[right];
    });
    bool duplicate_time = false;
    for (std::size_t position = 0; position < order.size(); ++position) {
        const std::size_t index = order[position];
        if (!std::isfinite(times[index]) || !std::isfinite(values[index])) {
            continue;
        }
        if (!result.x_values.empty() && times[index] == result.x_values.back()) {
            duplicate_time = true;
        }
        result.x_values.push_back(times[index]);
        result.y_values.push_back(values[index]);
        result.source_rows.push_back(source_rows[index]);
        if (index < time_labels.size()) {
            result.time_labels.push_back(time_labels[index]);
        }
        if (index < groups.size()) {
            result.groups.push_back(groups[index]);
        }
    }
    if (result.x_values.size() < 2) {
        add_error(result.diagnostics, "insufficient_points",
                  "时间序列图至少需要两个有效观测。");
    }
    if (duplicate_time) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "duplicate_time",
            "存在重复时间点；图形按时间排序后保留全部观测。"});
    }
    if (result.x_values.size() >= 3) {
        std::vector<double> gaps;
        for (std::size_t index = 1; index < result.x_values.size(); ++index) {
            const double gap = result.x_values[index] - result.x_values[index - 1];
            if (gap > 0.0) {
                gaps.push_back(gap);
            }
        }
        if (gaps.size() >= 2) {
            std::vector<double> ordered_gaps = gaps;
            std::sort(ordered_gaps.begin(), ordered_gaps.end());
            const double median = ordered_gaps[ordered_gaps.size() / 2];
            if (median > 0.0 && ordered_gaps.back() > 2.5 * median) {
                result.diagnostics.push_back({
                    DiagnosticMessage::Severity::warning, "irregular_interval",
                    "时间间隔不规则；图形按时间排序，但不把间隔当作等距。"});
            }
        }
    }
    return result;
}

ContourPlotResult contour_plot(
    const std::vector<double>& x_values,
    const std::vector<double>& y_values,
    const std::vector<double>& z_values,
    const int level_count)
{
    ContourPlotResult result;
    const std::size_t count = std::min({x_values.size(), y_values.size(), z_values.size()});
    std::set<double> unique_x;
    std::set<double> unique_y;
    std::map<std::pair<double, double>, double> cells;
    for (std::size_t index = 0; index < count; ++index) {
        if (!std::isfinite(x_values[index]) || !std::isfinite(y_values[index])
            || !std::isfinite(z_values[index])) {
            continue;
        }
        unique_x.insert(x_values[index]);
        unique_y.insert(y_values[index]);
        cells[{x_values[index], y_values[index]}] = z_values[index];
    }
    result.x.assign(unique_x.begin(), unique_x.end());
    result.y.assign(unique_y.begin(), unique_y.end());
    if (result.x.size() < 2 || result.y.size() < 2
        || cells.size() != result.x.size() * result.y.size()) {
        add_error(result.diagnostics, "irregular_grid",
                  "等值线图需要完整的规则 X/Y/Z 网格。");
        return result;
    }
    result.z.assign(result.y.size(), std::vector<double>(result.x.size(), 0.0));
    double minimum = std::numeric_limits<double>::infinity();
    double maximum = -std::numeric_limits<double>::infinity();
    for (std::size_t row = 0; row < result.y.size(); ++row) {
        for (std::size_t column = 0; column < result.x.size(); ++column) {
            const double value = cells[{result.x[column], result.y[row]}];
            result.z[row][column] = value;
            minimum = std::min(minimum, value);
            maximum = std::max(maximum, value);
        }
    }
    const int levels = std::max(2, std::min(level_count, 20));
    for (int index = 0; index < levels; ++index) {
        const double fraction = static_cast<double>(index) / static_cast<double>(levels - 1);
        result.levels.push_back(minimum + fraction * (maximum - minimum));
    }
    return result;
}

PiePlotResult pie_plot(
    const std::vector<std::string>& categories,
    const std::vector<double>& weights,
    const double other_threshold_percent,
    const std::vector<std::size_t>& source_rows)
{
    PiePlotResult result;
    const std::size_t count = std::min(categories.size(), weights.size());
    std::vector<std::string> order;
    std::map<std::string, double> totals;
    std::map<std::string, std::vector<std::size_t>> members;
    double total = 0.0;
    for (std::size_t index = 0; index < count; ++index) {
        if (!std::isfinite(weights[index])) {
            continue;
        }
        if (weights[index] < 0.0) {
            add_error(result.diagnostics, "negative_weight",
                      "饼图权重不能为负数。");
            continue;
        }
        if (weights[index] == 0.0) {
            continue;
        }
        const std::string label = categories[index].empty() ? "(空)" : categories[index];
        if (totals.find(label) == totals.end()) {
            order.push_back(label);
        }
        totals[label] += weights[index];
        total += weights[index];
        if (index < source_rows.size()) {
            members[label].push_back(source_rows[index]);
        }
    }
    if (total <= 0.0) {
        add_error(result.diagnostics, "zero_total",
                  "饼图各类别合计必须大于 0。");
        return result;
    }
    double other = 0.0;
    std::vector<std::size_t> other_members;
    const double threshold = std::clamp(other_threshold_percent, 0.0, 100.0);
    for (const std::string& label : order) {
        const double value = totals[label];
        const double percent = 100.0 * value / total;
        if (percent < threshold && order.size() > 1) {
            other += value;
            other_members.insert(other_members.end(),
                                 members[label].begin(), members[label].end());
            continue;
        }
        result.labels.push_back(label);
        result.values.push_back(value);
        result.percents.push_back(percent);
        result.member_source_rows.push_back(members[label]);
    }
    if (other > 0.0) {
        result.labels.push_back("Other");
        result.values.push_back(other);
        result.percents.push_back(100.0 * other / total);
        result.member_source_rows.push_back(std::move(other_members));
    }
    return result;
}

}  // namespace datalab::domain::statistics
