#include "domain/statistics/nested_gage_rr.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <utility>

namespace datalab::domain::statistics {
namespace {

void add_error(std::vector<DiagnosticMessage>& diagnostics, const char* code,
               const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

struct Cell {
    std::vector<double> values;
};

}  // namespace

NestedGageRrResult nested_gage_rr(
    const std::vector<double>& measurements,
    const std::vector<std::string>& parts,
    const std::vector<std::string>& operators,
    double tolerance)
{
    NestedGageRrResult result;
    result.tolerance = tolerance;
    if (measurements.size() < 4 || measurements.size() != parts.size()
        || measurements.size() != operators.size() || tolerance < 0.0
        || !std::isfinite(tolerance)) {
        add_error(result.diagnostics, "invalid_nested_gage_shape",
                  "Nested Gage R&R 要求长度一致、至少四条记录且公差非负。");
        return result;
    }

    std::map<std::string, std::size_t> operator_index;
    std::map<std::string, std::size_t> part_index;
    std::vector<std::size_t> part_operator;
    for (std::size_t i = 0; i < measurements.size(); ++i) {
        if (!std::isfinite(measurements[i]) || parts[i].empty()
            || operators[i].empty()) {
            add_error(result.diagnostics, "invalid_nested_gage_row",
                      "测量值、零件和操作员标签必须有效。");
            return result;
        }
        operator_index.emplace(operators[i], operator_index.size());
        const auto part_inserted = part_index.emplace(parts[i], part_index.size());
        if (part_inserted.second) {
            part_operator.push_back(operator_index.at(operators[i]));
        } else if (part_operator[part_inserted.first->second]
                   != operator_index.at(operators[i])) {
            add_error(result.diagnostics, "part_has_multiple_operators",
                      "Nested Gage R&R 中每个零件必须只属于一个操作员。");
            return result;
        }
    }
    result.operator_count = operator_index.size();
    result.part_count = part_index.size();
    if (result.operator_count < 2 || result.part_count < 2) {
        add_error(result.diagnostics, "insufficient_nested_gage_levels",
                  "Nested Gage R&R 至少需要两个操作员和两个零件。");
        return result;
    }

    std::vector<Cell> cells(result.part_count);
    for (std::size_t i = 0; i < measurements.size(); ++i) {
        cells[part_index.at(parts[i])].values.push_back(measurements[i]);
    }
    const std::size_t replicates = cells.front().values.size();
    if (replicates < 2) {
        add_error(result.diagnostics, "insufficient_nested_replicates",
                  "每个零件至少需要两次重复测量。");
        return result;
    }
    for (const Cell& cell : cells) {
        if (cell.values.size() != replicates) {
            add_error(result.diagnostics, "unbalanced_nested_gage_design",
                      "每个零件必须具有相同的重复次数。");
            return result;
        }
    }
    result.replicate_count = replicates;
    const std::size_t first_operator_parts = static_cast<std::size_t>(
        std::count(part_operator.cbegin(), part_operator.cend(), 0U));
    if (first_operator_parts == 0) {
        add_error(result.diagnostics, "empty_nested_operator",
                  "每个操作员必须至少有一个零件。");
        return result;
    }
    for (std::size_t op = 0; op < result.operator_count; ++op) {
        if (std::count(part_operator.cbegin(), part_operator.cend(), op)
            != static_cast<std::ptrdiff_t>(first_operator_parts)) {
            add_error(result.diagnostics, "unbalanced_nested_operator",
                      "每个操作员必须分配相同数量的零件。");
            return result;
        }
    }
    result.parts_per_operator = first_operator_parts;

    const double grand_mean = std::accumulate(
        measurements.cbegin(), measurements.cend(), 0.0) / measurements.size();
    std::vector<double> part_means(result.part_count);
    std::vector<double> operator_means(result.operator_count);
    for (std::size_t part = 0; part < result.part_count; ++part) {
        part_means[part] = std::accumulate(cells[part].values.cbegin(),
                                           cells[part].values.cend(), 0.0)
            / replicates;
        operator_means[part_operator[part]] += part_means[part];
    }
    for (double& mean : operator_means) {
        mean /= static_cast<double>(result.parts_per_operator);
    }

    double ss_operator = 0.0;
    for (const double mean : operator_means) {
        ss_operator += static_cast<double>(result.part_count * replicates
                                           / result.operator_count)
            * (mean - grand_mean) * (mean - grand_mean);
    }
    double ss_part = 0.0;
    double ss_repeatability = 0.0;
    for (std::size_t part = 0; part < result.part_count; ++part) {
        ss_part += static_cast<double>(replicates)
            * (part_means[part] - operator_means[part_operator[part]])
            * (part_means[part] - operator_means[part_operator[part]]);
        for (const double value : cells[part].values) {
            ss_repeatability += (value - part_means[part])
                * (value - part_means[part]);
        }
    }
    const std::size_t df_operator = result.operator_count - 1;
    const std::size_t df_part = result.part_count - result.operator_count;
    const std::size_t df_repeatability = result.part_count * (replicates - 1);
    const double ms_operator = ss_operator / df_operator;
    const double ms_part = ss_part / df_part;
    const double ms_repeatability = ss_repeatability / df_repeatability;
    result.anova_rows = {
        {"Operator", df_operator, ss_operator, ms_operator,
         ms_part > 0.0 ? ms_operator / ms_part : 0.0},
        {"Part(Operator)", df_part, ss_part, ms_part,
         ms_repeatability > 0.0 ? ms_part / ms_repeatability : 0.0},
        {"Repeatability", df_repeatability, ss_repeatability,
         ms_repeatability, 0.0}};

    const double repeatability = std::max(0.0, ms_repeatability);
    const double part_variance = std::max(
        0.0, (ms_part - ms_repeatability) / static_cast<double>(replicates));
    const double operator_variance = std::max(
        0.0, (ms_operator - ms_part)
            / static_cast<double>(result.parts_per_operator * replicates));
    const double gage_rr = repeatability + operator_variance;
    const double total = gage_rr + part_variance;
    const std::vector<std::pair<std::string, double>> components = {
        {"Repeatability", repeatability},
        {"Reproducibility", operator_variance},
        {"Total Gage R&R", gage_rr},
        {"Part-To-Part", part_variance},
        {"Total Variation", total}};
    for (const auto& [source, variance] : components) {
        NestedGageVarianceComponent component;
        component.source = source;
        component.variance_component = variance;
        component.standard_deviation = std::sqrt(variance);
        component.percent_contribution = total > 0.0 ? variance / total * 100.0 : 0.0;
        component.study_variation = 6.0 * component.standard_deviation;
        component.percent_study_variation = total > 0.0
            ? component.standard_deviation / std::sqrt(total) * 100.0 : 0.0;
        component.percent_tolerance = tolerance > 0.0
            ? component.study_variation / tolerance * 100.0 : 0.0;
        result.variance_components.push_back(component);
    }
    result.ndc = gage_rr > 0.0
        ? std::floor(1.41 * std::sqrt(part_variance / gage_rr)) : 0.0;
    if (total == 0.0) {
        add_error(result.diagnostics, "zero_nested_total_variation",
                  "所有测量值相同，无法估计 Nested Gage R&R 方差分量。");
    }
    return result;
}

}  // namespace datalab::domain::statistics
