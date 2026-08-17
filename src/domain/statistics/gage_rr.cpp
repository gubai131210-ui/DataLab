#include "domain/statistics/gage_rr.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

void add_error(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

struct Cell {
    std::vector<double> values;
};

}  // namespace

GageRrResult crossed_gage_rr(
    const std::vector<double>& measurements,
    const std::vector<std::string>& parts,
    const std::vector<std::string>& operators,
    double tolerance)
{
    GageRrResult result;
    result.tolerance = tolerance;
    if (measurements.size() < 4 || measurements.size() != parts.size()
        || measurements.size() != operators.size()) {
        add_error(result.diagnostics, "invalid_gage_shape",
                  "Gage R&R 要求测量值、零件和操作员列长度一致且至少有四条记录。");
        return result;
    }
    std::map<std::string, std::size_t> part_index;
    std::map<std::string, std::size_t> operator_index;
    for (std::size_t index = 0; index < measurements.size(); ++index) {
        if (!std::isfinite(measurements[index]) || parts[index].empty()
            || operators[index].empty()) {
            add_error(result.diagnostics, "invalid_gage_row",
                      "测量值、零件和操作员标签必须有效。");
            return result;
        }
        part_index.emplace(parts[index], part_index.size());
        operator_index.emplace(operators[index], operator_index.size());
    }
    result.part_count = part_index.size();
    result.operator_count = operator_index.size();
    if (result.part_count < 2 || result.operator_count < 2) {
        add_error(result.diagnostics, "insufficient_gage_levels",
                  "Crossed Gage R&R 至少需要两个零件和两个操作员。");
        return result;
    }
    std::vector<std::vector<Cell>> cells(
        result.part_count, std::vector<Cell>(result.operator_count));
    for (std::size_t index = 0; index < measurements.size(); ++index) {
        cells[part_index.at(parts[index])][operator_index.at(operators[index])]
            .values.push_back(measurements[index]);
    }
    const std::size_t expected_replicates = cells.front().front().values.size();
    if (expected_replicates < 2) {
        add_error(result.diagnostics, "insufficient_replicates",
                  "每个零件与操作员组合至少需要两次重复测量。");
        return result;
    }
    for (const auto& part_cells : cells) {
        for (const Cell& cell : part_cells) {
            if (cell.values.size() != expected_replicates) {
                add_error(result.diagnostics, "unbalanced_gage_design",
                          "每个零件×操作员组合必须具有相同的重复次数。");
                return result;
            }
        }
    }
    result.replicate_count = expected_replicates;
    const double grand_mean = std::accumulate(
        measurements.cbegin(), measurements.cend(), 0.0)
        / static_cast<double>(measurements.size());
    std::vector<double> part_means(result.part_count, 0.0);
    std::vector<double> operator_means(result.operator_count, 0.0);
    std::vector<std::vector<double>> cell_means(
        result.part_count, std::vector<double>(result.operator_count, 0.0));
    for (std::size_t part = 0; part < result.part_count; ++part) {
        for (std::size_t oper = 0; oper < result.operator_count; ++oper) {
            const Cell& cell = cells[part][oper];
            cell_means[part][oper] = std::accumulate(
                cell.values.cbegin(), cell.values.cend(), 0.0)
                / static_cast<double>(cell.values.size());
            part_means[part] += cell_means[part][oper];
            operator_means[oper] += cell_means[part][oper];
        }
    }
    for (double& mean : part_means) {
        mean /= static_cast<double>(result.operator_count);
    }
    for (double& mean : operator_means) {
        mean /= static_cast<double>(result.part_count);
    }
    double ss_part = 0.0;
    for (const double mean : part_means) {
        ss_part += static_cast<double>(result.operator_count * expected_replicates)
            * (mean - grand_mean) * (mean - grand_mean);
    }
    double ss_operator = 0.0;
    for (const double mean : operator_means) {
        ss_operator += static_cast<double>(result.part_count * expected_replicates)
            * (mean - grand_mean) * (mean - grand_mean);
    }
    double ss_interaction = 0.0;
    double ss_repeatability = 0.0;
    for (std::size_t part = 0; part < result.part_count; ++part) {
        for (std::size_t oper = 0; oper < result.operator_count; ++oper) {
            const double interaction = cell_means[part][oper]
                - part_means[part] - operator_means[oper] + grand_mean;
            ss_interaction += static_cast<double>(expected_replicates)
                * interaction * interaction;
            for (const double value : cells[part][oper].values) {
                ss_repeatability += (value - cell_means[part][oper])
                    * (value - cell_means[part][oper]);
            }
        }
    }
    const std::size_t df_part = result.part_count - 1;
    const std::size_t df_operator = result.operator_count - 1;
    const std::size_t df_interaction = df_part * df_operator;
    const std::size_t df_repeatability = result.part_count
        * result.operator_count * (expected_replicates - 1);
    const double ms_part = ss_part / static_cast<double>(df_part);
    const double ms_operator = ss_operator / static_cast<double>(df_operator);
    const double ms_interaction = df_interaction > 0
        ? ss_interaction / static_cast<double>(df_interaction) : 0.0;
    const double ms_repeatability = ss_repeatability
        / static_cast<double>(df_repeatability);
    result.anova_rows = {
        {"Part", df_part, ss_part, ms_part,
         ms_interaction > 0.0 ? ms_part / ms_interaction : 0.0},
        {"Operator", df_operator, ss_operator, ms_operator,
         ms_interaction > 0.0 ? ms_operator / ms_interaction : 0.0},
        {"Part * Operator", df_interaction, ss_interaction, ms_interaction,
         ms_repeatability > 0.0 ? ms_interaction / ms_repeatability : 0.0},
        {"Repeatability", df_repeatability, ss_repeatability, ms_repeatability, 0.0}};
    const double repeatability = std::max(0.0, ms_repeatability);
    const double interaction = std::max(0.0,
        (ms_interaction - ms_repeatability)
            / static_cast<double>(expected_replicates));
    const double operator_variance = std::max(0.0,
        (ms_operator - ms_interaction)
            / static_cast<double>(result.part_count * expected_replicates));
    const double part_variance = std::max(0.0,
        (ms_part - ms_interaction)
            / static_cast<double>(result.operator_count * expected_replicates));
    const double reproducibility = interaction + operator_variance;
    const double gage_rr = repeatability + reproducibility;
    const double total = gage_rr + part_variance;
    const std::vector<std::pair<std::string, double>> components = {
        {"Repeatability", repeatability},
        {"Reproducibility", reproducibility},
        {"Total Gage R&R", gage_rr},
        {"Part-To-Part", part_variance},
        {"Total Variation", total}};
    for (const auto& [source, variance] : components) {
        GageVarianceComponent component;
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
        add_error(result.diagnostics, "zero_total_variation",
                  "所有测量值相同，无法估计 Gage R&R 方差分量。");
    }
    return result;
}

}  // namespace datalab::domain::statistics
