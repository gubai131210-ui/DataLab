#include "application/doe_pages.h"

#include "application/output_builder.h"

#include "domain/column_extract.h"
#include "domain/statistics/normal_probability.h"
#include "domain/statistics/quality_visuals.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <map>
#include <optional>
#include <set>
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
    const bool plackett_burman = design.design_kind == "plackett_burman";
    const bool taguchi = design.design_kind == "taguchi_orthogonal";
    const bool fractional =
        !plackett_burman && !taguchi
        && (design.design_kind == "fractional" || design.fraction_p > 0);
    if (plackett_burman) {
        page.title = "Plackett–Burman 设计";
        page.method_name = "Plackett-Burman Design";
    } else if (taguchi) {
        page.title = "Taguchi 正交设计";
        page.method_name = "Taguchi Orthogonal Design";
    } else {
        page.title = fractional ? "2 水平部分析因设计" : "2 水平全因子设计";
        page.method_name = fractional ? "2-Level Fractional Factorial Design"
                                      : "2-Level Factorial Design";
    }
    page.configuration = configuration;
    page.parameter_summary = "因子数 k = " + std::to_string(factors.size())
        + "    p = " + std::to_string(design.fraction_p)
        + "    运行数 = " + std::to_string(design.runs.size());
    if (design.resolution > 0) {
        page.parameter_summary += "    分辨度 = " + std::to_string(design.resolution);
    }
    page.diagnostics = design.diagnostics;

    StatisticTable info;
    info.title = "设计信息";
    info.headers = {"Property", "Value"};
    if (plackett_burman) {
        info.rows.push_back({"Design", "Plackett-Burman"});
    } else if (taguchi) {
        info.rows.push_back({"Design", "Taguchi Orthogonal"});
    } else {
        info.rows.push_back(
            {"Design", fractional ? "2^(k-p) fractional" : "2^k full"});
    }
    info.rows.push_back({"Factors (k)", std::to_string(factors.size())});
    info.rows.push_back({"Fraction (p)", std::to_string(design.fraction_p)});
    info.rows.push_back({"Runs", std::to_string(design.runs.size())});
    if (design.resolution > 0) {
        info.rows.push_back({"Resolution", std::to_string(design.resolution)});
    }
    page.tables.push_back(std::move(info));

    if (!design.generators.empty()) {
        StatisticTable generators;
        generators.title = "设计生成器";
        generators.headers = {"Generator"};
        for (const auto& generator : design.generators) {
            generators.rows.push_back({generator});
        }
        page.tables.push_back(std::move(generators));
    }
    if (!design.defining_relation.empty()) {
        StatisticTable relation;
        relation.title = "定义关系";
        relation.headers = {"Word"};
        for (const auto& word : design.defining_relation) {
            relation.rows.push_back({word});
        }
        page.tables.push_back(std::move(relation));
    }
    if (!design.alias_lines.empty()) {
        StatisticTable aliases;
        aliases.title = "别名结构";
        aliases.headers = {"Alias Chain"};
        for (const auto& line : design.alias_lines) {
            aliases.rows.push_back({line});
        }
        page.tables.push_back(std::move(aliases));
    }

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

    page.facts.doe = datalab::domain::DoeFacts{};
    page.facts.doe->factor_count = factors.size();
    page.facts.doe->design_kind = design.design_kind;
    page.facts.doe->fraction_p = design.fraction_p;
    page.facts.doe->resolution = design.resolution;
    page.facts.doe->run_count = design.runs.size();
    if (!design.generators.empty()) {
        std::string joined;
        for (std::size_t index = 0; index < design.generators.size(); ++index) {
            if (index > 0) {
                joined += ";";
            }
            joined += design.generators[index];
        }
        page.facts.doe->generator_text = joined;
    }

    const std::string source_id = page.id.empty() ? "factorial_design" : page.id;
    page.worksheet_export = factorial_design_to_worksheet(design, source_id);
    page.diagnostics.push_back({
        datalab::domain::DiagnosticMessage::Severity::info, "doe_worksheet_export_ready",
        "已生成可写入工作表的设计矩阵（实际水平 + 空 Response 列）；"
        "UI 可一键替换活动工作表以录入响应。"});
    return page;
}

domain::OutputPage response_surface_design_page(
    const domain::AnalysisConfiguration& configuration,
    const domain::statistics::ResponseSurfaceDesign& design)
{
    domain::OutputPage page;
    page.id = new_id("doe_rsd");
    const bool is_bbd = design.design_kind_id == "bbd";
    page.title = is_bbd ? "Box–Behnken 设计" : "中心复合设计 (CCD)";
    page.method_name = is_bbd ? "Box-Behnken Design" : "Central Composite Design";
    page.configuration = configuration;
    page.parameter_summary = "因子数 = " + std::to_string(design.factor_count)
        + "    运行数 = " + std::to_string(design.run_count)
        + "    α = " + format_number(design.alpha);
    if (!is_bbd) {
        page.parameter_summary += "    变体 = " + design.ccd_variant_id;
    }
    page.diagnostics = design.diagnostics;

    StatisticTable info;
    info.title = "设计信息";
    info.headers = {"Property", "Value"};
    info.rows.push_back({"Design", design.design_kind_id});
    if (!is_bbd) {
        info.rows.push_back({"CCD variant", design.ccd_variant_id});
        info.rows.push_back({"Alpha", format_number(design.alpha)});
    }
    info.rows.push_back({"Factors", std::to_string(design.factor_count)});
    info.rows.push_back({"Runs", std::to_string(design.run_count)});
    info.rows.push_back({"Cube points", std::to_string(design.cube_count)});
    info.rows.push_back({"Star points", std::to_string(design.star_count)});
    info.rows.push_back({"Edge midpoints", std::to_string(design.edge_count)});
    info.rows.push_back({"Center points", std::to_string(design.center_count)});
    info.rows.push_back({"Randomized", design.randomized ? "true" : "false"});
    info.rows.push_back({"Seed", std::to_string(design.random_seed)});
    info.rows.push_back({"Evidence", "formula_reference"});
    page.tables.push_back(std::move(info));

    StatisticTable factors_table;
    factors_table.title = "因素定义";
    factors_table.headers = {"ID", "Name", "Unit", "Low", "High", "Center"};
    for (const auto& factor : design.factors) {
        factors_table.rows.push_back({
            factor.id,
            factor.name.empty() ? factor.id : factor.name,
            factor.unit,
            format_number(factor.low),
            format_number(factor.high),
            format_number(datalab::domain::statistics::factor_center(factor))});
    }
    page.tables.push_back(std::move(factors_table));

    StatisticTable design_table;
    design_table.title = "设计矩阵";
    design_table.headers = {
        "Run ID", "Standard Order", "Run Order", "Block", "Point Type"};
    for (const auto& factor : design.factors) {
        const std::string label = factor.name.empty() ? factor.id : factor.name;
        design_table.headers.push_back(label + " (coded)");
        design_table.headers.push_back(label + " (actual)");
    }
    for (const auto& run : design.runs) {
        std::vector<std::string> row = {
            run.run_id,
            std::to_string(run.standard_order),
            std::to_string(run.run_order),
            std::to_string(run.block),
            run.point_type};
        for (std::size_t index = 0; index < design.factors.size(); ++index) {
            const double coded = index < run.coded_levels.size() ? run.coded_levels[index] : 0.0;
            const double actual =
                index < run.actual_levels.size() ? run.actual_levels[index] : 0.0;
            row.push_back(format_number(coded));
            row.push_back(format_number(actual));
        }
        design_table.rows.push_back(std::move(row));
    }
    page.tables.push_back(std::move(design_table));

    domain::DesignGenerationFacts facts;
    facts.design_kind = design.design_kind_id;
    facts.ccd_variant = design.ccd_variant_id;
    facts.design_source_id = configuration.response_surface_design.design_source_id.empty()
        ? page.id
        : configuration.response_surface_design.design_source_id;
    facts.factor_count = design.factor_count;
    facts.run_count = design.run_count;
    facts.cube_count = design.cube_count;
    facts.star_count = design.star_count;
    facts.edge_count = design.edge_count;
    facts.center_count = design.center_count;
    facts.alpha = design.alpha;
    facts.allow_beyond_range = configuration.response_surface_design.allow_beyond_range;
    facts.beyond_range_detected = design.beyond_range_detected;
    facts.randomized = design.randomized;
    facts.random_seed = design.random_seed;
    facts.evidence_type = "formula_reference";
    page.facts.design_generation = std::move(facts);

    page.facts.doe = domain::DoeFacts{};
    page.facts.doe->factor_count = design.factor_count;
    page.facts.doe->design_kind = design.design_kind_id;
    page.facts.doe->run_count = design.run_count;
    page.worksheet_export = response_surface_design_to_worksheet(
        design,
        page.facts.design_generation.has_value()
            ? page.facts.design_generation->design_source_id
            : configuration.response_surface_design.design_source_id);
    page.diagnostics.push_back({
        datalab::domain::DiagnosticMessage::Severity::info, "doe_worksheet_export_ready",
        "已生成可写入工作表的设计矩阵（实际水平 + 空 Response 列）；"
        "UI 可一键替换活动工作表以录入响应。"});
    return page;
}

domain::DataTable response_surface_design_to_worksheet(
    const domain::statistics::ResponseSurfaceDesign& design,
    const std::string& design_source_id)
{
    domain::DataTable table;
    table.name = design_source_id.empty()
        ? (design.design_kind_id + "_worksheet")
        : (design_source_id + "_worksheet");
    table.source_path = "generated:response_surface_design";
    table.import_metadata.provider_id = "datalab.doe";
    table.import_metadata.source_object = design.design_kind_id;
    table.import_metadata.object_kind = "design_matrix";
    table.import_metadata.row_id_is_synthetic = true;
    table.import_metadata.filter_summary =
        "design_source_id=" + design_source_id;

    table.columns = {"RunID", "StdOrder", "RunOrder", "Block", "PointType"};
    for (const auto& factor : design.factors) {
        const std::string label = factor.name.empty() ? factor.id : factor.name;
        table.columns.push_back(label);
    }
    table.columns.push_back("Response");

    table.column_types.assign(table.columns.size(), domain::ColumnType::unknown);
    table.column_types[0] = domain::ColumnType::categorical;  // RunID
    table.column_types[1] = domain::ColumnType::numeric;      // StdOrder
    table.column_types[2] = domain::ColumnType::numeric;      // RunOrder
    table.column_types[3] = domain::ColumnType::numeric;      // Block
    table.column_types[4] = domain::ColumnType::categorical;  // PointType
    for (std::size_t index = 0; index < design.factors.size(); ++index) {
        table.column_types[5 + index] = domain::ColumnType::numeric;
    }
    table.column_types.back() = domain::ColumnType::numeric;  // Response empty

    for (std::size_t run_index = 0; run_index < design.runs.size(); ++run_index) {
        const auto& run = design.runs[run_index];
        std::vector<std::string> row = {
            run.run_id,
            std::to_string(run.standard_order),
            std::to_string(run.run_order),
            std::to_string(run.block),
            run.point_type};
        for (std::size_t index = 0; index < design.factors.size(); ++index) {
            const double actual =
                index < run.actual_levels.size() ? run.actual_levels[index] : 0.0;
            row.push_back(format_number(actual));
        }
        row.push_back("");  // Response placeholder for experiment entry
        table.rows.push_back(std::move(row));
        table.row_ids.push_back(static_cast<domain::RowId>(run_index + 1));
    }
    return table;
}

namespace {

std::string factorial_actual_level(
    const domain::statistics::DoeFactor& factor, const int coded)
{
    if (coded < 0) {
        return factor.low_level.empty() ? "-1" : factor.low_level;
    }
    if (coded > 0) {
        return factor.high_level.empty() ? "1" : factor.high_level;
    }
    if (!factor.mid_level.empty()) {
        return factor.mid_level;
    }
    const auto low = parse_numeric_cell(factor.low_level);
    const auto high = parse_numeric_cell(factor.high_level);
    if (low.has_value() && high.has_value()) {
        return format_number((*low + *high) / 2.0);
    }
    return "0";
}

std::string factorial_worksheet_default_name(
    const domain::statistics::DoeFactorialDesign& design)
{
    if (design.design_kind == "plackett_burman") {
        return "plackett_burman_worksheet";
    }
    if (design.design_kind == "taguchi_orthogonal") {
        return "taguchi_orthogonal_worksheet";
    }
    const bool fractional =
        design.design_kind == "fractional" || design.fraction_p > 0;
    return fractional ? "fractional_factorial_worksheet"
                      : "full_factorial_worksheet";
}

std::string factorial_worksheet_source_object(
    const domain::statistics::DoeFactorialDesign& design)
{
    if (!design.design_kind.empty()) {
        return design.design_kind;
    }
    const bool fractional = design.fraction_p > 0;
    return fractional ? "2_level_fractional_factorial" : "2_level_full_factorial";
}

}  // namespace

domain::DataTable factorial_design_to_worksheet(
    const domain::statistics::DoeFactorialDesign& design,
    const std::string& design_source_id)
{
    domain::DataTable table;
    table.name = design_source_id.empty()
        ? factorial_worksheet_default_name(design)
        : (design_source_id + "_worksheet");
    table.source_path = "generated:factorial_design";
    table.import_metadata.provider_id = "datalab.doe";
    table.import_metadata.source_object = factorial_worksheet_source_object(design);
    table.import_metadata.object_kind = "design_matrix";
    table.import_metadata.row_id_is_synthetic = true;
    table.import_metadata.filter_summary =
        "design_source_id=" + design_source_id
        + ";design_kind=" + table.import_metadata.source_object
        + ";p=" + std::to_string(design.fraction_p);

    table.columns = {"RunID", "StdOrder", "RunOrder", "Block", "PointType"};
    for (const auto& factor : design.factors) {
        table.columns.push_back(factor.name.empty() ? "Factor" : factor.name);
    }
    table.columns.push_back("Response");

    table.column_types.assign(table.columns.size(), domain::ColumnType::unknown);
    table.column_types[0] = domain::ColumnType::categorical;
    table.column_types[1] = domain::ColumnType::numeric;
    table.column_types[2] = domain::ColumnType::numeric;
    table.column_types[3] = domain::ColumnType::numeric;
    table.column_types[4] = domain::ColumnType::categorical;
    for (std::size_t index = 0; index < design.factors.size(); ++index) {
        table.column_types[5 + index] = domain::ColumnType::categorical;
    }
    table.column_types.back() = domain::ColumnType::numeric;

    for (std::size_t run_index = 0; run_index < design.runs.size(); ++run_index) {
        const auto& run = design.runs[run_index];
        const std::string point_type = run.center_point ? "center" : "factorial";
        std::vector<std::string> row = {
            "R" + std::to_string(run_index + 1),
            std::to_string(run.standard_order),
            std::to_string(run.run_order),
            std::to_string(run.block),
            point_type};
        for (std::size_t index = 0; index < design.factors.size(); ++index) {
            const int coded =
                index < run.coded_levels.size() ? run.coded_levels[index] : 0;
            row.push_back(factorial_actual_level(design.factors[index], coded));
        }
        row.push_back("");
        table.rows.push_back(std::move(row));
        table.row_ids.push_back(static_cast<domain::RowId>(run_index + 1));
    }
    return table;
}

namespace {

std::string ascii_lower(std::string text)
{
    for (char& ch : text) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return text;
}

std::optional<std::size_t> find_column_ci(
    const domain::DataTable& table, const std::string& name)
{
    const std::string needle = ascii_lower(name);
    for (std::size_t index = 0; index < table.columns.size(); ++index) {
        if (ascii_lower(table.columns[index]) == needle) {
            return index;
        }
    }
    return std::nullopt;
}

bool nearly_equal_level(double left, double right)
{
    const double scale = std::max({1.0, std::abs(left), std::abs(right)});
    return std::abs(left - right) <= 1e-9 * scale;
}

bool cell_matches_level(const std::string& cell, const std::string& level)
{
    if (cell == level) {
        return true;
    }
    const auto cell_value = parse_numeric_cell(cell);
    const auto level_value = parse_numeric_cell(level);
    return cell_value.has_value() && level_value.has_value()
        && nearly_equal_level(*cell_value, *level_value);
}

bool is_center_point_type(const std::string& point_type)
{
    const std::string lower = ascii_lower(point_type);
    return lower == "center" || lower == "centre" || lower == "cp";
}

bool is_factorial_point_type(const std::string& point_type)
{
    const std::string lower = ascii_lower(point_type);
    return lower == "factorial" || lower == "cube" || lower == "corner";
}

std::optional<int> parse_coded_factor_cell(
    const std::string& value,
    const std::string& low,
    const std::string& high)
{
    if (cell_matches_level(value, low)) {
        return -1;
    }
    if (cell_matches_level(value, high)) {
        return 1;
    }
    const auto numeric = parse_numeric_cell(value);
    if (numeric.has_value()) {
        if (nearly_equal_level(*numeric, -1.0)) {
            return -1;
        }
        if (nearly_equal_level(*numeric, 1.0)) {
            return 1;
        }
        if (nearly_equal_level(*numeric, 0.0)) {
            return 0;
        }
        const auto low_value = parse_numeric_cell(low);
        const auto high_value = parse_numeric_cell(high);
        if (low_value.has_value() && high_value.has_value()) {
            const double mid = (*low_value + *high_value) / 2.0;
            if (nearly_equal_level(*numeric, mid)) {
                return 0;
            }
        }
    }
    if (value == "0") {
        return 0;
    }
    return std::nullopt;
}

std::optional<std::size_t> parse_positive_index_cell(const std::string& text)
{
    const auto numeric = parse_numeric_cell(text);
    if (!numeric.has_value() || *numeric < 1.0
        || !nearly_equal_level(*numeric, std::floor(*numeric))) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(*numeric);
}

}  // namespace

ImportedFactorialRuns import_factorial_runs_from_worksheet(
    const domain::DataTable& table,
    const domain::AnalysisConfiguration& configuration)
{
    ImportedFactorialRuns imported;
    for (std::size_t factor = 0; factor < configuration.doe.factor_columns.size();
         ++factor) {
        const std::size_t column = configuration.doe.factor_columns[factor];
        const std::string low = factor < configuration.doe.low_levels.size()
            ? configuration.doe.low_levels[factor]
            : "-1";
        const std::string high = factor < configuration.doe.high_levels.size()
            ? configuration.doe.high_levels[factor]
            : "+1";
        imported.design.factors.push_back({
            column < table.columns.size() ? table.columns[column]
                                          : domain::column_label(table, column),
            low,
            high});
    }

    const auto point_type_column = find_column_ci(table, "PointType");
    const auto block_column = find_column_ci(table, "Block");
    const auto std_order_column = find_column_ci(table, "StdOrder");
    const auto run_order_column = find_column_ci(table, "RunOrder");
    std::set<std::size_t> excluded(
        configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend());

    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (excluded.count(row_index) != 0) {
            continue;
        }
        const auto& row = table.rows[row_index];
        std::optional<std::string> point_type;
        if (point_type_column.has_value() && *point_type_column < row.size()) {
            point_type = row[*point_type_column];
        }

        std::vector<int> levels;
        levels.reserve(configuration.doe.factor_columns.size());
        bool valid_levels = true;
        for (std::size_t factor = 0;
             factor < configuration.doe.factor_columns.size(); ++factor) {
            const std::size_t column = configuration.doe.factor_columns[factor];
            const std::string value = column < row.size() ? row[column] : "";
            const std::string low = factor < configuration.doe.low_levels.size()
                ? configuration.doe.low_levels[factor]
                : "-1";
            const std::string high = factor < configuration.doe.high_levels.size()
                ? configuration.doe.high_levels[factor]
                : "+1";
            if (point_type.has_value() && is_center_point_type(*point_type)) {
                levels.push_back(0);
                continue;
            }
            const auto coded = parse_coded_factor_cell(value, low, high);
            if (!coded.has_value()) {
                valid_levels = false;
                break;
            }
            if (point_type.has_value() && is_factorial_point_type(*point_type)
                && *coded == 0) {
                valid_levels = false;
                break;
            }
            levels.push_back(*coded);
        }
        if (!valid_levels || levels.size() != configuration.doe.factor_columns.size()) {
            ++imported.skipped_level_rows;
            continue;
        }

        const bool all_zero = std::all_of(
            levels.cbegin(), levels.cend(), [](int level) { return level == 0; });
        const bool any_zero = std::any_of(
            levels.cbegin(), levels.cend(), [](int level) { return level == 0; });
        bool center_point = false;
        if (point_type.has_value() && is_center_point_type(*point_type)) {
            center_point = true;
        } else if (all_zero) {
            center_point = true;
        } else if (any_zero) {
            ++imported.skipped_level_rows;
            continue;
        }

        domain::statistics::DoeRun run;
        run.center_point = center_point;
        run.coded_levels = std::move(levels);
        if (std_order_column.has_value() && *std_order_column < row.size()) {
            run.standard_order =
                parse_positive_index_cell(row[*std_order_column]).value_or(row_index + 1);
        } else {
            run.standard_order = row_index + 1;
        }
        if (run_order_column.has_value() && *run_order_column < row.size()) {
            run.run_order =
                parse_positive_index_cell(row[*run_order_column]).value_or(
                    imported.design.runs.size() + 1);
        } else {
            run.run_order = imported.design.runs.size() + 1;
        }
        if (block_column.has_value() && *block_column < row.size()) {
            run.block = parse_positive_index_cell(row[*block_column]).value_or(1);
        } else {
            run.block = 1;
        }
        if (center_point) {
            ++imported.center_run_count;
        }
        imported.design.runs.push_back(std::move(run));
        imported.source_rows.push_back(row_index);
    }
    return imported;
}

}  // namespace datalab::application
