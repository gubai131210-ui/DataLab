#include "application/doe_pages.h"

#include "application/output_builder.h"

#include "domain/column_extract.h"
#include "domain/statistics/normal_probability.h"
#include "domain/statistics/quality_visuals.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <sstream>
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

void add_zero_residual_reference(PlotSpec& plot)
{
    if (plot.x_values.empty()) {
        return;
    }
    const auto [x_min, x_max] = std::minmax_element(
        plot.x_values.cbegin(), plot.x_values.cend());
    PlotSeries zero;
    zero.label = "残差 = 0";
    zero.x_values = {*x_min, *x_max};
    zero.values = {0.0, 0.0};
    zero.line_width = 1.0;
    plot.series.push_back(std::move(zero));
}

void append_doe_residual_plots(
    domain::OutputPage& page,
    const datalab::domain::statistics::DoeFactorialDesign& design,
    const datalab::domain::statistics::DoeResponseAnalysisResult& fit)
{
    std::vector<double> fitted;
    std::vector<double> residuals;
    std::vector<double> order;
    std::vector<std::size_t> source_rows;
    for (const auto& row : fit.residuals) {
        if (!std::isfinite(row.residual) || row.run_index >= design.runs.size()) {
            continue;
        }
        fitted.push_back(row.fitted);
        residuals.push_back(row.residual);
        order.push_back(static_cast<double>(design.runs[row.run_index].run_order + 1));
        source_rows.push_back(design.runs[row.run_index].standard_order);
    }
    if (residuals.empty()) {
        return;
    }
    PlotSpec residual_plot;
    residual_plot.kind = PlotKind::scatter;
    residual_plot.title = "残差与拟合值";
    residual_plot.x_axis_title = "拟合值";
    residual_plot.y_axis_title = "残差";
    residual_plot.x_values = fitted;
    residual_plot.values = residuals;
    residual_plot.source_rows = source_rows;
    add_zero_residual_reference(residual_plot);
    page.plots.push_back(std::move(residual_plot));

    PlotSpec order_plot;
    order_plot.kind = PlotKind::scatter;
    order_plot.title = "残差与观测顺序";
    order_plot.x_axis_title = "观测顺序";
    order_plot.y_axis_title = "残差";
    order_plot.x_values = order;
    order_plot.values = residuals;
    order_plot.source_rows = source_rows;
    add_zero_residual_reference(order_plot);
    page.plots.push_back(std::move(order_plot));

    PlotSpec residual_probability;
    residual_probability.kind = PlotKind::probability;
    residual_probability.title = "残差正态概率图";
    residual_probability.x_axis_title = "理论分位数";
    residual_probability.y_axis_title = "残差";
    const datalab::domain::statistics::NormalProbabilityResult probability =
        datalab::domain::statistics::normal_probability_plot(residuals, source_rows);
    residual_probability.x_values = probability.theoretical_quantiles;
    residual_probability.values = probability.ordered_values;
    residual_probability.source_rows = probability.source_rows;
    page.plots.push_back(std::move(residual_probability));

    const datalab::domain::statistics::HistogramResult bins =
        datalab::domain::statistics::histogram(residuals, 0);
    PlotSpec hist;
    hist.kind = PlotKind::histogram;
    hist.title = "残差直方图";
    hist.x_axis_title = "残差";
    hist.y_axis_title = "频数";
    hist.histogram_edges = bins.edges;
    hist.histogram_counts = bins.counts;
    hist.values = residuals;
    hist.source_rows = source_rows;
    page.plots.push_back(std::move(hist));
}

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
    PlotSeries series;
    series.label = design.factors[factor_index].name;
    series.show_points = true;
    series.x_values = plot.x_values;
    series.values = plot.values;
    plot.series = {std::move(series)};
    const std::string low_label = design.factors[factor_index].low_level.empty()
        ? "Low" : design.factors[factor_index].low_level;
    const std::string high_label = design.factors[factor_index].high_level.empty()
        ? "High" : design.factors[factor_index].high_level;
    plot.categories = {low_label, high_label};
    plot.point_labels = {low_label, high_label};
    return plot;
}

std::pair<double, double> cube_project(int x, int y, int z)
{
    return {static_cast<double>(x) + 0.45 * static_cast<double>(z),
            static_cast<double>(y) + 0.32 * static_cast<double>(z)};
}

PlotSpec cube_plot(const datalab::domain::statistics::DoeFactorialDesign& design,
                   const std::vector<double>& responses,
                   const std::string& response_label)
{
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    const std::size_t factor_count = design.factors.size();
    plot.title = factor_count == 2 ? "立方图（方形）" : "立方图";
    plot.x_axis_title = design.factors.empty() ? "Factor" : design.factors[0].name;
    plot.y_axis_title = factor_count >= 2 ? design.factors[1].name : response_label;
    std::map<std::vector<int>, std::pair<double, std::size_t>> sums;
    for (std::size_t row = 0; row < design.runs.size() && row < responses.size(); ++row) {
        if (!std::isfinite(responses[row]) || design.runs[row].center_point) {
            continue;
        }
        std::vector<int> key = design.runs[row].coded_levels;
        if (key.size() > 3) {
            key.resize(3);
        }
        auto& entry = sums[key];
        entry.first += responses[row];
        ++entry.second;
    }
    std::vector<int> xs;
    std::vector<int> ys;
    std::vector<int> zs;
    if (factor_count == 2) {
        xs = {-1, 1, -1, 1};
        ys = {-1, -1, 1, 1};
        zs = {0, 0, 0, 0};
    } else {
        xs = {-1, 1, -1, 1, -1, 1, -1, 1};
        ys = {-1, -1, 1, 1, -1, -1, 1, 1};
        zs = {-1, -1, -1, -1, 1, 1, 1, 1};
    }
    for (std::size_t index = 0; index < xs.size(); ++index) {
        std::vector<int> key = {xs[index], ys[index]};
        if (factor_count >= 3) {
            key.push_back(zs[index]);
        }
        const auto found = sums.find(key);
        const double mean = found == sums.end() || found->second.second == 0
            ? 0.0
            : found->second.first / static_cast<double>(found->second.second);
        const auto projected = cube_project(xs[index], ys[index], zs[index]);
        plot.x_values.push_back(projected.first);
        plot.values.push_back(projected.second);
        std::ostringstream label;
        label.setf(std::ios::fixed);
        label.precision(3);
        label << mean;
        plot.point_labels.push_back(label.str());
    }
    auto add_edge = [&](std::size_t from, std::size_t to) {
        PlotSeries edge;
        edge.label = "棱";
        edge.x_values = {plot.x_values[from], plot.x_values[to]};
        edge.values = {plot.values[from], plot.values[to]};
        plot.series.push_back(std::move(edge));
    };
    if (factor_count == 2) {
        add_edge(0, 1);
        add_edge(1, 3);
        add_edge(3, 2);
        add_edge(2, 0);
    } else {
        add_edge(0, 1);
        add_edge(1, 3);
        add_edge(3, 2);
        add_edge(2, 0);
        add_edge(4, 5);
        add_edge(5, 7);
        add_edge(7, 6);
        add_edge(6, 4);
        add_edge(0, 4);
        add_edge(1, 5);
        add_edge(2, 6);
        add_edge(3, 7);
    }
    return plot;
}

PlotSpec standardized_effects_pareto(
    const datalab::domain::statistics::DoeResponseAnalysisResult& fit)
{
    PlotSpec plot;
    plot.kind = PlotKind::pareto;
    plot.title = fit.pareto_method == "lenth_pse"
        ? "效应 Pareto"
        : "标准化效应 Pareto";
    plot.x_axis_title = "项";
    plot.y_axis_title = fit.pareto_method == "lenth_pse" ? "|效应|" : "|t|";
    struct Item {
        std::string term;
        double magnitude = 0.0;
    };
    std::vector<Item> items;
    for (std::size_t index = 1; index < fit.term_names.size(); ++index) {
        Item item;
        item.term = fit.term_names[index];
        if (fit.pareto_method == "lenth_pse") {
            item.magnitude = index < fit.effects.size()
                ? std::abs(fit.effects[index]) : 0.0;
        } else {
            item.magnitude = index < fit.t_statistics.size()
                ? std::abs(fit.t_statistics[index]) : 0.0;
        }
        items.push_back(item);
    }
    std::sort(items.begin(), items.end(), [](const Item& left, const Item& right) {
        return left.magnitude > right.magnitude;
    });
    for (const Item& item : items) {
        plot.categories.push_back(item.term);
        plot.category_values.push_back(item.magnitude);
    }
    if (std::isfinite(fit.pareto_reference) && fit.pareto_reference > 0.0
        && !plot.category_values.empty()) {
        plot.center.assign(plot.category_values.size(), fit.pareto_reference);
    }
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
        column_label(table, *configuration.doe.response_column);
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
    if (!fit.term_names.empty() && fit.term_names.size() > 1) {
        page.plots.push_back(standardized_effects_pareto(fit));
    }
    if (design.factors.size() == 2 || design.factors.size() == 3) {
        page.plots.push_back(cube_plot(design, responses, response_label));
    } else if (design.factors.size() >= 4) {
        page.diagnostics.push_back({
            datalab::domain::DiagnosticMessage::Severity::info,
            "cube_plot_requires_2_or_3_factors",
            "立方图仅支持 2 或 3 个因子（当前 "
                + std::to_string(design.factors.size())
                + " 个）；请用主效应图、交互图、等值线/曲面图查看高维设计。"});
    }
    for (std::size_t factor = 0; factor < design.factors.size(); ++factor) {
        page.plots.push_back(main_effect_plot(factor, design, responses, response_label));
    }
    for (std::size_t first = 0; first < design.factors.size(); ++first) {
        for (std::size_t second = first + 1; second < design.factors.size(); ++second) {
            page.plots.push_back(
                interaction_plot(first, second, design, responses, response_label));
        }
    }
    if (design.factors.size() >= 2) {
        auto resolve_factor = [&](const std::string& name,
                                  const std::size_t fallback) -> std::size_t {
            if (name.empty()) {
                return fallback;
            }
            for (std::size_t index = 0; index < design.factors.size(); ++index) {
                if (design.factors[index].name == name) {
                    return index;
                }
            }
            return design.factors.size();
        };
        std::size_t x_index = resolve_factor(configuration.doe.contour_x_factor, 0);
        std::size_t y_index = resolve_factor(configuration.doe.contour_y_factor, 1);
        if (x_index >= design.factors.size() || y_index >= design.factors.size()
            || x_index == y_index) {
            page.diagnostics.push_back({
                datalab::domain::DiagnosticMessage::Severity::warning,
                "invalid_contour_factors",
                "等值线/曲面的 X/Y 因子无效或相同；请指定两个不同的因子名。"});
        } else {
            std::vector<std::string> held_actuals;
            std::vector<datalab::domain::DiagnosticMessage> hold_diagnostics;
            const std::vector<double> hold_coded =
                datalab::domain::statistics::resolve_contour_hold_coded(
                    design, x_index, y_index, configuration.doe.contour_hold_actual,
                    held_actuals, hold_diagnostics);
            page.diagnostics.insert(page.diagnostics.end(),
                                    hold_diagnostics.cbegin(), hold_diagnostics.cend());
            const auto grid = datalab::domain::statistics::evaluate_coded_grid(
                fit, design, x_index, y_index, 25, &hold_coded);
            page.diagnostics.insert(page.diagnostics.end(),
                                    grid.diagnostics.cbegin(), grid.diagnostics.cend());
            if (!page.facts.doe.has_value()) {
                page.facts.doe = datalab::domain::DoeFacts{};
            }
            page.facts.doe->contour_x_factor = design.factors[x_index].name;
            page.facts.doe->contour_y_factor = design.factors[y_index].name;
            page.facts.doe->held_factor_names = grid.held_factor_names;
            page.facts.doe->held_actual_values = held_actuals;
            page.facts.doe->held_coded_values = grid.held_coded_values;
            page.facts.doe->contour_plot_available =
                !grid.x.empty() && !grid.y.empty() && !grid.z.empty();
            if (!grid.x.empty() && !grid.y.empty() && !grid.z.empty()) {
                PlotSpec contour;
                contour.kind = PlotKind::contour;
                contour.title = "等值线图 - " + design.factors[x_index].name
                    + " vs " + design.factors[y_index].name;
                contour.x_axis_title = design.factors[x_index].name + "（编码）";
                contour.y_axis_title = design.factors[y_index].name + "（编码）";
                contour.contour_x = grid.x;
                contour.contour_y = grid.y;
                contour.matrix_values = grid.z;
                double minimum = grid.z.front().front();
                double maximum = grid.z.front().front();
                for (const auto& row : grid.z) {
                    for (const double value : row) {
                        minimum = std::min(minimum, value);
                        maximum = std::max(maximum, value);
                    }
                }
                contour.color_min = minimum;
                contour.color_max = maximum;
                page.plots.push_back(contour);
                PlotSpec surface = contour;
                surface.kind = PlotKind::surface;
                surface.title = "响应曲面图 - " + design.factors[x_index].name
                    + " vs " + design.factors[y_index].name;
                page.plots.push_back(std::move(surface));
            }
        }
    } else {
        page.diagnostics.push_back({
            datalab::domain::DiagnosticMessage::Severity::info,
            "contour_requires_two_factors",
            "等值线/曲面图需要至少两个连续因子。"});
    }
    append_doe_residual_plots(page, design, fit);
    if (!page.facts.doe.has_value()) {
        page.facts.doe = datalab::domain::DoeFacts{};
    }
    page.facts.doe->residual_count = fit.residuals.size();
    page.facts.doe->has_p_value = fit.residual_degrees_of_freedom > 0;
    page.facts.doe->factor_count = design.factors.size();
    page.facts.doe->cube_plot_available =
        design.factors.size() == 2 || design.factors.size() == 3;
    page.facts.doe->pareto_method = fit.pareto_method;
    if (std::isfinite(fit.pareto_reference) && fit.pareto_reference > 0.0) {
        page.facts.doe->pareto_reference = fit.pareto_reference;
    }
    double largest = -1.0;
    for (std::size_t index = 1; index < fit.term_names.size(); ++index) {
        const double magnitude = fit.pareto_method == "lenth_pse"
            ? (index < fit.effects.size() ? std::abs(fit.effects[index]) : 0.0)
            : (index < fit.t_statistics.size() ? std::abs(fit.t_statistics[index]) : 0.0);
        if (magnitude > largest) {
            largest = magnitude;
            page.facts.doe->largest_standardized_effect_term = fit.term_names[index];
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
