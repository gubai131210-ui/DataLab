#include "application/doe_pages.h"

#include "application/output_builder.h"

#include "domain/column_extract.h"

#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace datalab::application {

namespace {

using datalab::domain::PlotKind;
using datalab::domain::PlotSeries;
using datalab::domain::PlotSeriesRole;
using datalab::domain::PlotSpec;
using datalab::domain::StatisticTable;
using datalab::domain::column_label;

using datalab::domain::statistics::DoeAnovaRow;

// DOE 标准 ANOVA 行（Source/SS/DF/MS/F/P-Value）；source_prefix 用于区组行加 "Block: " 前缀。
void append_anova_row(StatisticTable& table, const DoeAnovaRow& row,
                      const std::string& source_prefix = {})
{
    table.rows.push_back({source_prefix + row.source, format_number(row.sum_of_squares),
        std::to_string(row.degrees_of_freedom), format_number(row.mean_square),
        format_number(row.f_statistic), format_optional(row.p_value)});
}

void append_anova_rows(StatisticTable& table,
                       const std::vector<DoeAnovaRow>& rows,
                       const std::string& source_prefix = {})
{
    for (const auto& row : rows) {
        append_anova_row(table, row, source_prefix);
    }
}

// 主效应图：低/高水平响应均值连线。
PlotSpec main_effect_plot(std::size_t factor_index,
                          const datalab::domain::statistics::DoeFactorialDesign& design,
                          const std::vector<double>& responses,
                          const std::string& response_label)
{
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "主效应图 - " + design.factors[factor_index].name;
    plot.x_axis_title = design.factors[factor_index].name;
    plot.y_axis_title = response_label;
    plot.x_values = {-1.0, 1.0};
    plot.values = {0.0, 0.0};
    double low_sum = 0.0;
    double high_sum = 0.0;
    std::size_t low_count = 0;
    std::size_t high_count = 0;
    for (std::size_t row = 0; row < design.runs.size(); ++row) {
        if (!std::isfinite(responses[row])) {
            continue;
        }
        if (design.runs[row].coded_levels[factor_index] < 0) {
            low_sum += responses[row];
            ++low_count;
        } else if (design.runs[row].coded_levels[factor_index] > 0) {
            high_sum += responses[row];
            ++high_count;
        }
    }
    plot.values = {low_count == 0 ? 0.0 : low_sum / low_count,
                   high_count == 0 ? 0.0 : high_sum / high_count};
    return plot;
}

// 交互作用图：两因子四象限响应均值连线（低/高两组序列）。
PlotSpec interaction_plot(std::size_t first, std::size_t second,
                          const datalab::domain::statistics::DoeFactorialDesign& design,
                          const std::vector<double>& responses,
                          const std::string& response_label)
{
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "交互作用图 - " + design.factors[first].name + "*"
        + design.factors[second].name;
    plot.x_axis_title = design.factors[first].name
        + "（按 " + design.factors[second].name + " 分组）";
    plot.y_axis_title = response_label;
    plot.x_values = {-1.0, 1.0, -1.0, 1.0};
    plot.values.assign(4, 0.0);
    std::array<double, 4> sums{};
    std::array<std::size_t, 4> counts{};
    for (std::size_t row = 0; row < design.runs.size(); ++row) {
        if (!std::isfinite(responses[row])) {
            continue;
        }
        const int a = design.runs[row].coded_levels[first];
        const int b = design.runs[row].coded_levels[second];
        const std::size_t index = (b > 0 ? 2U : 0U) + (a > 0 ? 1U : 0U);
        sums[index] += responses[row];
        ++counts[index];
    }
    for (std::size_t index = 0; index < 4; ++index) {
        plot.values[index] = counts[index] == 0 ? 0.0
            : sums[index] / static_cast<double>(counts[index]);
    }
    PlotSeries low_group;
    low_group.role = PlotSeriesRole::interaction_first;
    low_group.label = design.factors[second].name + " = Low";
    low_group.x_values = {-1.0, 1.0};
    low_group.values = {plot.values[0], plot.values[1]};
    low_group.show_points = true;
    PlotSeries high_group;
    high_group.role = PlotSeriesRole::interaction_second;
    high_group.label = design.factors[second].name + " = High";
    high_group.x_values = {-1.0, 1.0};
    high_group.values = {plot.values[2], plot.values[3]};
    high_group.show_points = true;
    plot.series = {std::move(low_group), std::move(high_group)};
    return plot;
}

}  // namespace

domain::OutputPage doe_response_page(
    const domain::DataTable& table,
    const domain::AnalysisConfiguration& configuration,
    const domain::statistics::DoeFactorialDesign& design,
    const std::vector<double>& responses,
    const domain::statistics::DoeResponseAnalysisResult& fit)
{
    const std::string response_label =
        column_label(table, *configuration.doe_response_column);
    domain::OutputPage page;
    page.id = new_id("doe_response");
    page.title = "DOE 响应分析";
    page.method_name = "2-Level Factorial Response Analysis";
    page.configuration = configuration;
    page.parameter_summary = "响应 = " + response_label
        + "    因子数 = " + std::to_string(design.factors.size())
        + "    有效运行数 = " + std::to_string(fit.residuals.size());
    page.diagnostics = design.diagnostics;
    page.diagnostics.insert(page.diagnostics.end(), fit.diagnostics.cbegin(),
                            fit.diagnostics.cend());
    StatisticTable coefficients;
    coefficients.title = "系数与效应";
    coefficients.headers = {"Term", "Coefficient", "Effect"};
    for (std::size_t index = 0; index < fit.term_names.size(); ++index) {
        coefficients.rows.push_back({
            fit.term_names[index], format_number(fit.coefficients[index]),
            format_number(fit.effects[index])});
    }
    page.tables.push_back(std::move(coefficients));
    StatisticTable anova;
    anova.title = "DOE ANOVA";
    anova.headers = {"Source", "SS", "DF", "MS", "F", "P-Value"};
    append_anova_rows(anova, fit.anova_rows);
    page.tables.push_back(std::move(anova));
    if (!fit.model_anova_rows.empty() || !fit.block_anova_rows.empty()) {
        StatisticTable model_terms;
        model_terms.title = "模型项与区组";
        model_terms.headers = {"Source", "SS", "DF", "MS", "F", "P-Value"};
        append_anova_rows(model_terms, fit.model_anova_rows);
        append_anova_rows(model_terms, fit.block_anova_rows, "Block: ");
        page.tables.push_back(std::move(model_terms));
    }
    if (fit.pure_error_anova_row.has_value()
        || fit.lack_of_fit_anova_row.has_value()) {
        StatisticTable fit_diagnostics;
        fit_diagnostics.title = "纯误差与失拟";
        fit_diagnostics.headers = {"Source", "SS", "DF", "MS", "F", "P-Value"};
        if (fit.pure_error_anova_row.has_value()) {
            append_anova_row(fit_diagnostics, *fit.pure_error_anova_row);
        }
        if (fit.lack_of_fit_anova_row.has_value()) {
            append_anova_row(fit_diagnostics, *fit.lack_of_fit_anova_row);
        }
        page.tables.push_back(std::move(fit_diagnostics));
    }
    if (fit.center_points.count > 0) {
        StatisticTable curvature;
        curvature.title = "中心点与曲率";
        curvature.headers = {"Center N", "Center Mean", "Factorial Mean",
                             "Difference", "SS", "DF", "F", "P-Value"};
        curvature.rows.push_back({
            std::to_string(fit.center_points.count),
            format_number(fit.center_points.mean),
            format_number(fit.curvature.factorial_mean),
            format_number(fit.curvature.difference),
            format_number(fit.curvature.sum_of_squares),
            std::to_string(fit.curvature.degrees_of_freedom),
            format_number(fit.curvature.f_statistic),
            format_optional(fit.curvature.p_value)});
        page.tables.push_back(std::move(curvature));
    }
    StatisticTable residuals;
    residuals.title = "残差诊断";
    residuals.headers = {"Run", "Response", "Fitted", "Residual", "Standardized Residual"};
    for (const auto& row : fit.residuals) {
        residuals.rows.push_back({std::to_string(row.run_index),
            format_number(row.response), format_number(row.fitted),
            format_number(row.residual), format_number(row.standardized_residual)});
    }
    page.tables.push_back(std::move(residuals));
    for (std::size_t factor = 0; factor < design.factors.size(); ++factor) {
        page.plots.push_back(main_effect_plot(factor, design, responses, response_label));
    }
    for (std::size_t first = 0; first < design.factors.size(); ++first) {
        for (std::size_t second = first + 1; second < design.factors.size(); ++second) {
            page.plots.push_back(
                interaction_plot(first, second, design, responses, response_label));
        }
    }
    return page;
}

domain::OutputPage doe_design_page(
    const domain::AnalysisConfiguration& configuration,
    const std::vector<domain::statistics::DoeFactor>& factors,
    const domain::statistics::DoeFactorialDesign& design)
{
    domain::OutputPage page;
    page.id = new_id("doe");
    page.title = "2 水平全因子设计";
    page.method_name = "2-Level Factorial Design";
    page.configuration = configuration;
    page.parameter_summary = "因子数 = " + std::to_string(factors.size())
        + "    运行数 = " + std::to_string(design.runs.size());
    page.diagnostics = design.diagnostics;
    StatisticTable design_table;
    design_table.title = "设计矩阵";
    design_table.headers = {"Standard Order", "Run Order", "Block"};
    for (const auto& factor : factors) {
        design_table.headers.push_back(factor.name);
    }
    for (const auto& run : design.runs) {
        std::vector<std::string> row = {
            std::to_string(run.standard_order), std::to_string(run.run_order),
            std::to_string(run.block)};
        for (const int level : run.coded_levels) {
            row.push_back(std::to_string(level));
        }
        design_table.rows.push_back(std::move(row));
    }
    page.tables.push_back(std::move(design_table));
    return page;
}

}  // namespace datalab::application
