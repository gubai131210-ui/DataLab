#include "domain/statistics/multi_vari.h"

#include <algorithm>
#include <map>
#include <utility>

namespace datalab::domain::statistics {
namespace {

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const char* message)
{
    diagnostics.push_back({severity, code, message});
}

std::size_t index_of(const std::vector<std::string>& values, const std::string& value)
{
    const auto found = std::find(values.cbegin(), values.cend(), value);
    if (found == values.cend()) {
        return values.size();
    }
    return static_cast<std::size_t>(std::distance(values.cbegin(), found));
}

std::vector<std::string> unique_levels(
    const std::vector<std::vector<std::string>>& factor_levels,
    const std::size_t factor)
{
    std::vector<std::string> levels;
    for (const auto& row : factor_levels) {
        if (factor >= row.size()) {
            continue;
        }
        if (index_of(levels, row[factor]) == levels.size()) {
            levels.push_back(row[factor]);
        }
    }
    return levels;
}

std::string cell_key(const std::vector<std::string>& levels)
{
    std::string key;
    for (std::size_t index = 0; index < levels.size(); ++index) {
        if (index > 0) {
            key += "\x1f";
        }
        key += levels[index];
    }
    return key;
}

double x_position(
    const std::vector<std::size_t>& indices,
    const std::vector<std::size_t>& counts)
{
    double x = static_cast<double>(indices.empty() ? 0 : indices[0]);
    if (counts.size() >= 3 && indices.size() >= 3) {
        x += static_cast<double>(indices[2])
            * (static_cast<double>(counts[0]) + 1.0);
    }
    if (counts.size() >= 4 && indices.size() >= 4) {
        const double n0 = static_cast<double>(counts[0]);
        const double n2 = counts.size() >= 3 ? static_cast<double>(counts[2]) : 1.0;
        const double block3 = (n0 + 1.0) * std::max(n2, 1.0);
        x += static_cast<double>(indices[3]) * (block3 + 1.0);
    }
    if (counts.size() >= 2 && counts[1] > 1 && indices.size() >= 2) {
        x += (static_cast<double>(indices[1])
              - (static_cast<double>(counts[1]) - 1.0) / 2.0)
            * 0.25;
    }
    return x;
}

}  // namespace

MultiVariResult multi_vari_chart(
    const std::vector<double>& measurements,
    const std::vector<std::vector<std::string>>& factor_levels,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& factor_names)
{
    MultiVariResult result;
    result.factor_names = factor_names;
    result.factor_count = factor_names.size();
    const std::size_t count = std::min(
        {measurements.size(), factor_levels.size(), source_rows.size()});
    if (result.factor_count < 2 || result.factor_count > 4) {
        add_diagnostic(
            result.diagnostics, DiagnosticMessage::Severity::error,
            "insufficient_factors",
            "Multi-Vari 图需要 2～4 个因子列。");
        return result;
    }
    for (std::size_t index = 0; index < count; ++index) {
        if (factor_levels[index].size() < result.factor_count) {
            continue;
        }
        MultiVariPoint point;
        point.source_row = source_rows[index];
        point.measurement = measurements[index];
        point.factor_levels = factor_levels[index];
        result.points.push_back(std::move(point));
    }
    result.valid_count = result.points.size();
    if (result.valid_count == 0) {
        add_diagnostic(
            result.diagnostics, DiagnosticMessage::Severity::error,
            "insufficient_observations",
            "没有可用于 Multi-Vari 图的 complete-case 观测。");
        return result;
    }

    std::vector<std::vector<std::string>> levels_by_factor;
    std::vector<std::size_t> level_counts;
    result.possible_combinations = 1;
    for (std::size_t factor = 0; factor < result.factor_count; ++factor) {
        auto levels = unique_levels(factor_levels, factor);
        if (levels.size() < 2) {
            add_diagnostic(
                result.diagnostics, DiagnosticMessage::Severity::error,
                "insufficient_factor_levels",
                "每个因子至少需要 2 个水平。");
            return result;
        }
        result.possible_combinations *= levels.size();
        level_counts.push_back(levels.size());
        levels_by_factor.push_back(std::move(levels));
    }

    struct Accumulator {
        double sum = 0.0;
        std::size_t count = 0;
        std::vector<std::string> levels;
    };
    std::map<std::string, Accumulator> cells;
    std::vector<std::map<std::string, Accumulator>> factor_accumulators(result.factor_count);
    for (auto& point : result.points) {
        std::vector<std::size_t> indices(result.factor_count, 0);
        for (std::size_t factor = 0; factor < result.factor_count; ++factor) {
            indices[factor] = index_of(levels_by_factor[factor], point.factor_levels[factor]);
            auto& accumulator = factor_accumulators[factor][point.factor_levels[factor]];
            accumulator.sum += point.measurement;
            ++accumulator.count;
            if (accumulator.levels.empty()) {
                accumulator.levels = {point.factor_levels[factor]};
            }
        }
        point.x_position = x_position(indices, level_counts);
        Accumulator& cell = cells[cell_key(point.factor_levels)];
        cell.sum += point.measurement;
        ++cell.count;
        if (cell.levels.empty()) {
            cell.levels = point.factor_levels;
        }
    }

    result.observed_combinations = cells.size();
    result.combination_coverage =
        static_cast<double>(result.observed_combinations)
        / static_cast<double>(result.possible_combinations);
    result.plot_available = result.combination_coverage >= 0.60;
    if (!result.plot_available) {
        add_diagnostic(
            result.diagnostics, DiagnosticMessage::Severity::error,
            "insufficient_combination_coverage",
            "因子水平组合覆盖不足 60%，不绘制 Multi-Vari 图。");
    }

    for (std::size_t factor = 0; factor < result.factor_count; ++factor) {
        for (const auto& level : levels_by_factor[factor]) {
            const auto& accumulator = factor_accumulators[factor][level];
            MultiVariFactorMean mean;
            mean.factor_name = factor_names[factor];
            mean.level = level;
            mean.count = accumulator.count;
            mean.mean = accumulator.count == 0
                ? 0.0
                : accumulator.sum / static_cast<double>(accumulator.count);
            result.factor_means.push_back(std::move(mean));
        }
    }

    std::map<std::string, MultiVariMeanSeries> series_by_group;
    for (auto& [key, cell] : cells) {
        MultiVariCellMean mean;
        mean.levels = cell.levels;
        mean.count = cell.count;
        mean.mean = cell.count == 0
            ? 0.0 : cell.sum / static_cast<double>(cell.count);
        std::vector<std::size_t> indices(result.factor_count, 0);
        for (std::size_t factor = 0; factor < result.factor_count
             && factor < cell.levels.size(); ++factor) {
            indices[factor] = index_of(levels_by_factor[factor], cell.levels[factor]);
        }
        mean.x_position = x_position(indices, level_counts);
        if (result.factor_count >= 2 && cell.levels.size() >= 2) {
            std::string label = cell.levels[1];
            if (result.factor_count >= 3 && cell.levels.size() >= 3) {
                label += " / " + cell.levels[2];
            }
            if (result.factor_count >= 4 && cell.levels.size() >= 4) {
                label += " / " + cell.levels[3];
            }
            auto& series = series_by_group[label];
            if (series.label.empty()) {
                series.label = "均值 " + label;
            }
            series.x_values.push_back(mean.x_position);
            series.y_values.push_back(mean.mean);
        }
        result.cell_means.push_back(std::move(mean));
    }
    for (auto& [label, series] : series_by_group) {
        std::vector<std::size_t> order(series.x_values.size());
        for (std::size_t index = 0; index < order.size(); ++index) {
            order[index] = index;
        }
        std::sort(order.begin(), order.end(), [&](std::size_t left, std::size_t right) {
            return series.x_values[left] < series.x_values[right];
        });
        MultiVariMeanSeries sorted;
        sorted.label = series.label;
        for (const std::size_t index : order) {
            sorted.x_values.push_back(series.x_values[index]);
            sorted.y_values.push_back(series.y_values[index]);
        }
        result.mean_series.push_back(std::move(sorted));
    }
    return result;
}

}  // namespace datalab::domain::statistics
