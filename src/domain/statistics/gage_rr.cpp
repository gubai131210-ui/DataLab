#include "domain/statistics/gage_rr.h"

#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <optional>

namespace datalab::domain::statistics {
namespace {

void add_error(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

void add_warning(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::warning, code, message});
}

GageVarianceComponent make_component(
    const std::string& source,
    double raw_variance,
    double total,
    double tolerance,
    double multiplier,
    std::vector<DiagnosticMessage>& diagnostics)
{
    GageVarianceComponent component;
    component.source = source;
    component.raw_variance_component = raw_variance;
    component.truncated = raw_variance < 0.0;
    component.variance_component = std::max(0.0, raw_variance);
    if (component.truncated) {
        add_warning(diagnostics, "negative_variance_component",
                    "方差分量原始估计为负，已截断为 0。");
    }
    component.standard_deviation = std::sqrt(component.variance_component);
    component.percent_contribution = total > 0.0
        ? component.variance_component / total * 100.0 : 0.0;
    component.study_variation = multiplier * component.standard_deviation;
    component.percent_study_variation = total > 0.0
        ? component.standard_deviation / std::sqrt(total) * 100.0 : 0.0;
    if (std::isfinite(tolerance) && tolerance > 0.0) {
        component.percent_tolerance_available = true;
        component.percent_tolerance = component.study_variation / tolerance * 100.0;
    }
    return component;
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
    result.method = "anova";
    result.evidence.method_version = "2";
    result.evidence.assumption_status = "not_verified";
    result.design_balanced = true;
    if (!std::isfinite(tolerance) || tolerance < 0.0) {
        add_error(result.diagnostics, "invalid_tolerance",
                  "公差必须为有限非负数；NaN、无穷或负数不可用。");
        return result;
    }
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
                result.design_balanced = false;
                result.rules.push_back({
                    "design_balance", "triggered",
                    "零件×操作员单元重复次数不一致。", {},
                    "交叉设计需要平衡重复后才能解释方差分量。"});
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
    const double f_part = ms_interaction > 0.0 ? ms_part / ms_interaction : 0.0;
    const double f_operator = ms_interaction > 0.0 ? ms_operator / ms_interaction : 0.0;
    const double f_interaction = ms_repeatability > 0.0
        ? ms_interaction / ms_repeatability : 0.0;
    const std::optional<double> p_part = ms_interaction > 0.0
        ? std::optional<double>(f_right_tail(
              f_part, static_cast<double>(df_part),
              static_cast<double>(df_interaction)))
        : std::nullopt;
    const std::optional<double> p_operator = ms_interaction > 0.0
        ? std::optional<double>(f_right_tail(
              f_operator, static_cast<double>(df_operator),
              static_cast<double>(df_interaction)))
        : std::nullopt;
    const std::optional<double> p_interaction = ms_repeatability > 0.0
        ? std::optional<double>(f_right_tail(
              f_interaction, static_cast<double>(df_interaction),
              static_cast<double>(df_repeatability)))
        : std::nullopt;
    result.anova_rows = {
        {"Part", df_part, ss_part, ms_part, f_part, p_part},
        {"Operator", df_operator, ss_operator, ms_operator, f_operator, p_operator},
        {"Part * Operator", df_interaction, ss_interaction, ms_interaction,
         f_interaction, p_interaction},
        {"Repeatability", df_repeatability, ss_repeatability, ms_repeatability, 0.0,
         std::nullopt}};
    result.interaction_p_value = p_interaction;
    result.interaction_retained = true;
    result.interaction_reduction_recommended =
        p_interaction.has_value() && *p_interaction > 0.25;
    if (result.interaction_reduction_recommended) {
        add_warning(result.diagnostics, "interaction_pooling_investigation",
                    "Part×Operator 交互 p>0.25，传统 AIAG 流程可考虑将交互并入重复性；"
                    "当前结果保留完整交互模型，不自动缩减。");
    }
    const double raw_repeatability = ms_repeatability;
    const double raw_interaction = (ms_interaction - ms_repeatability)
        / static_cast<double>(expected_replicates);
    const double raw_operator = (ms_operator - ms_interaction)
        / static_cast<double>(result.part_count * expected_replicates);
    const double raw_part = (ms_part - ms_interaction)
        / static_cast<double>(result.operator_count * expected_replicates);
    const double repeatability = std::max(0.0, raw_repeatability);
    const double interaction = std::max(0.0, raw_interaction);
    const double operator_variance = std::max(0.0, raw_operator);
    const double part_variance = std::max(0.0, raw_part);
    const double reproducibility = interaction + operator_variance;
    const double gage_rr = repeatability + reproducibility;
    const double total = gage_rr + part_variance;
    result.negative_variance_truncated =
        raw_repeatability < 0.0 || raw_interaction < 0.0
        || raw_operator < 0.0 || raw_part < 0.0;
    result.variance_components.push_back(make_component(
        "Repeatability", raw_repeatability, total, tolerance,
        result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        "Operator", raw_operator, total, tolerance,
        result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        "Operator * Part", raw_interaction, total, tolerance,
        result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        "Reproducibility", reproducibility, total, tolerance,
        result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        "Total Gage R&R", gage_rr, total, tolerance,
        result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        "Part-To-Part", raw_part, total, tolerance,
        result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        "Total Variation", total, total, tolerance,
        result.study_var_multiplier, result.diagnostics));
    if (gage_rr > 0.0) {
        result.ndc = std::floor(1.41 * std::sqrt(part_variance / gage_rr));
        if (result.ndc < 1.0) {
            result.ndc = 1.0;
        }
        result.ndc_available = true;
        if (result.ndc < 5.0) {
            add_warning(result.diagnostics, "ndc_investigation",
                        "ndc<5 只作为调查提示，不是量具不合格的绝对结论。");
        }
    } else {
        add_error(result.diagnostics, "not_estimable",
                  "Gage 标准差为 0，ndc 不可估计。");
    }
    if (total == 0.0) {
        add_error(result.diagnostics, "zero_total_variation",
                  "所有测量值相同，无法估计 Gage R&R 方差分量。");
    }
    result.evidence.valid_count = measurements.size();
    result.rules.push_back({
        "design_balance", "not_triggered",
        "交叉设计单元重复次数平衡。", {},
        "平衡设计是 ANOVA 方差分量解释的前提。"});
    result.rules.push_back({
        "interaction_model",
        result.interaction_reduction_recommended ? "triggered" : "not_triggered",
        result.interaction_reduction_recommended
            ? "交互项 p>0.25，可考虑缩减，但当前保留完整模型。"
            : "当前保留 Part×Operator 交互模型。",
        {},
        "交互是否缩减必须回显；本实现不自动并入重复性。"});
    result.rules.push_back({
        "negative_variance",
        result.negative_variance_truncated ? "triggered" : "not_triggered",
        result.negative_variance_truncated
            ? "存在负方差分量，已截断为 0，并保留原始估计。"
            : "方差分量原始估计均非负。",
        {},
        "截断后的分量用于 %Contribution；解释时同时查看原始值。"});
    result.rules.push_back({
        "percent_metrics", "not_triggered",
        "%Contribution 基于方差，%Study Var 基于标准差，口径不同。",
        {},
        "不要把 %Contribution 与 %Study Var 当成同一个百分比。"});
    if (tolerance <= 0.0) {
        result.rules.push_back({
            "invalid_tolerance", "triggered",
            "未提供有效公差，%Tolerance 不可用。", {},
            "只有有限正公差才能计算 %Tolerance。"});
    }
    return result;
}

}  // namespace datalab::domain::statistics
