#include "application/analysis_service.h"
#include "application/chart_pages.h"
#include "application/column_assembly.h"
#include "application/doe_pages.h"
#include "application/output_builder.h"

#include "domain/column_extract.h"
#include "domain/statistics/control_charts.h"
#include "domain/statistics/descriptive_statistics.h"
#include "domain/statistics/process_capability.h"
#include "domain/statistics/quality_visuals.h"
#include "domain/statistics/normal_probability.h"
#include "domain/statistics/normality_test.h"
#include "domain/statistics/correlation.h"
#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/inference_extensions.h"
#include "domain/statistics/regression.h"
#include "domain/statistics/box_cox.h"
#include "domain/statistics/gage_rr.h"
#include "domain/statistics/msa_type1.h"
#include "domain/statistics/reliability.h"
#include "domain/statistics/t_power.h"
#include "domain/statistics/nonparametric_tests.h"
#include "domain/statistics/time_series.h"
#include "domain/statistics/arima.h"
#include "domain/statistics/two_factor_anova.h"
#include "domain/statistics/logistic_regression.h"
#include "domain/statistics/variance_tests.h"
#include "domain/statistics/time_series_decomposition.h"
#include "domain/statistics/doe_factorial.h"
#include "domain/statistics/nested_gage_rr.h"
#include "domain/statistics/attribute_agreement.h"
#include "domain/statistics/seasonal_forecasting.h"
#include "domain/statistics/pca.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <map>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <limits>
#include <set>

namespace datalab::application {
namespace {

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::DiagnosticMessage;
using datalab::domain::ExtractedNumericColumn;
using datalab::domain::OutputPage;
using datalab::domain::PlotKind;
using datalab::domain::PlotSpec;
using datalab::domain::StatisticTable;
using datalab::domain::SpecificationLimits;
using datalab::domain::column_label;
using datalab::domain::extract_numeric_column;
using datalab::domain::extract_text_column;
using datalab::domain::is_missing_cell;

// ---- capability 家族 / logistic 的页面装配辅助（阶段 2.3 薄壳化）----

// 组装配过程能力页正文（Process Data / Performance PPM / Potential Within /
// Overall 四表 + 直方图），capability() 只负责校验、Within σ 与附加图/指标。
datalab::domain::OutputPage build_capability_content(
    const datalab::domain::AnalysisConfiguration& configuration,
    const datalab::domain::ExtractedNumericColumn& extracted,
    const datalab::domain::statistics::ProcessCapabilityResult& capability_result,
    int subgroup_size,
    const std::string& within_method)
{
    datalab::domain::OutputPage page;
    page.id = new_id("cap");
    page.title = "正态过程能力分析";
    page.method_name = "Normal Capability Analysis";
    page.configuration = configuration;
    page.diagnostics = capability_result.diagnostics;
    page.parameter_summary =
        "变量: " + extracted.name
        + "    子组大小 = " + std::to_string(subgroup_size)
        + "    Within σ: " + within_method;

    StatisticTable process;
    process.title = "Process Data";
    process.headers = {"项目", "数值"};
    process.rows = {
        {"LSL", format_optional(configuration.specifications.lower)},
        {"Target", format_optional(configuration.specifications.target)},
        {"USL", format_optional(configuration.specifications.upper)},
        {"Sample Mean", format_number(capability_result.mean)},
        {"Sample N", std::to_string(capability_result.sample_size)},
        {"StDev (Within)", format_number(capability_result.within_standard_deviation)},
        {"StDev (Overall)", format_number(capability_result.overall_standard_deviation)}};
    page.tables.push_back(process);

    StatisticTable ppm;
    ppm.title = "Performance (PPM)";
    ppm.headers = {"", "观测", "期望 Within", "期望 Overall"};
    ppm.rows = {
        {"低于 LSL",
         format_optional(capability_result.observed_ppm_below, 4),
         format_optional(capability_result.expected_ppm_within_below, 4),
         format_optional(capability_result.expected_ppm_overall_below, 4)},
        {"高于 USL",
         format_optional(capability_result.observed_ppm_above, 4),
         format_optional(capability_result.expected_ppm_within_above, 4),
         format_optional(capability_result.expected_ppm_overall_above, 4)},
        {"合计",
         format_optional(capability_result.observed_ppm_total, 4),
         format_optional(capability_result.expected_ppm_within_total, 4),
         format_optional(capability_result.expected_ppm_overall_total, 4)}};
    page.tables.push_back(ppm);

    StatisticTable within;
    within.title = "Potential (Within) Capability";
    within.headers = {"指标", "数值"};
    within.rows = {
        {"Cp", format_optional(capability_result.cp)},
        {"CPL", format_optional(capability_result.cpl)},
        {"CPU", format_optional(capability_result.cpu)},
        {"Cpk", format_optional(capability_result.cpk)}};
    page.tables.push_back(within);

    StatisticTable overall;
    overall.title = "Overall Capability";
    overall.headers = {"指标", "数值"};
    overall.rows = {
        {"Pp", format_optional(capability_result.pp)},
        {"PPL", format_optional(capability_result.ppl)},
        {"PPU", format_optional(capability_result.ppu)},
        {"Ppk", format_optional(capability_result.ppk)}};
    page.tables.push_back(overall);

    const auto bins = datalab::domain::statistics::histogram(extracted.values, 0);
    PlotSpec hist;
    hist.kind = PlotKind::histogram;
    hist.title = "过程能力直方图";
    hist.x_axis_title = extracted.name;
    hist.y_axis_title = "频数";
    hist.histogram_edges = bins.edges;
    hist.histogram_counts = bins.counts;
    hist.values = extracted.values;
    hist.source_rows = extracted.source_rows;
    hist.lsl = configuration.specifications.lower;
    hist.usl = configuration.specifications.upper;
    hist.target = configuration.specifications.target;
    hist.process_mean = capability_result.mean;
    hist.within_sigma = capability_result.within_standard_deviation;
    hist.overall_sigma = capability_result.overall_standard_deviation;
    page.plots.push_back(hist);
    return page;
}

// 正态概率图（含参考线），capability_sixpack 使用。
PlotSpec probability_plot_spec(
    const datalab::domain::statistics::NormalProbabilityResult& probability,
    const std::string& variable_name)
{
    PlotSpec plot;
    plot.kind = PlotKind::probability;
    plot.title = "正态概率图";
    plot.x_axis_title = "理论标准正态分位数";
    plot.y_axis_title = variable_name;
    plot.values = probability.ordered_values;
    plot.x_values = probability.theoretical_quantiles;
    plot.source_rows.resize(probability.ordered_values.size());
    std::iota(plot.source_rows.begin(), plot.source_rows.end(), 0);
    plot.center.resize(probability.ordered_values.size());
    if (probability.theoretical_quantiles.size() >= 2) {
        const double x0 = probability.theoretical_quantiles.front();
        const double x1 = probability.theoretical_quantiles.back();
        const double y0 = probability.ordered_values.front();
        const double y1 = probability.ordered_values.back();
        const double slope = (x1 == x0) ? 0.0 : (y1 - y0) / (x1 - x0);
        for (std::size_t index = 0; index < plot.center.size(); ++index) {
            plot.center[index] =
                y0 + slope * (probability.theoretical_quantiles[index] - x0);
        }
    }
    return plot;
}

// "最后 25 个子组 / 最近 25 个观测"图，capability_sixpack 使用。
PlotSpec last_points_plot(
    const datalab::domain::ExtractedNumericColumn& extracted,
    const datalab::domain::statistics::ControlChartResult& primary,
    int subgroup_size)
{
    const bool by_subgroups = subgroup_size > 1;
    PlotSpec plot;
    plot.kind = PlotKind::control;
    plot.title = by_subgroups ? "最后 25 个子组" : "最近 25 个观测";
    plot.x_axis_title = "样本";
    plot.y_axis_title = "测量值";
    if (by_subgroups) {
        const std::size_t group_count = extracted.values.size() / subgroup_size;
        const std::size_t first_group = group_count > 25 ? group_count - 25 : 0;
        for (std::size_t group = first_group; group < group_count; ++group) {
            for (std::size_t offset = 0; offset < subgroup_size; ++offset) {
                const std::size_t source = group * subgroup_size + offset;
                plot.values.push_back(extracted.values[source]);
                plot.x_values.push_back(static_cast<double>(group - first_group + 1));
                plot.source_rows.push_back(extracted.source_rows[source]);
            }
        }
    } else {
        plot = control_plot("最近 25 个观测", "测量值", primary, extracted.source_rows);
        const std::size_t first = plot.values.size() > 25 ? plot.values.size() - 25 : 0;
        plot.values.erase(plot.values.begin(),
                          plot.values.begin() + static_cast<std::ptrdiff_t>(first));
        plot.source_rows.erase(plot.source_rows.begin(),
                               plot.source_rows.begin() + static_cast<std::ptrdiff_t>(first));
    }
    return plot;
}

// Logistic 回归 complete-case 导入（事件解析 + 预测变量行），logistic_regression 使用。
struct LogisticImport {
    std::vector<int> response;
    std::vector<std::vector<double>> predictors;
    std::vector<std::size_t> source_rows;
};

LogisticImport logistic_import_rows(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    LogisticImport result;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row_index)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        const auto& row = table.rows[row_index];
        const std::size_t response_column = *configuration.logistic_response_column;
        if (response_column >= row.size()) {
            continue;
        }
        int event = -1;
        if (const auto numeric = parse_numeric_cell(row[response_column]);
            numeric.has_value() && (*numeric == 0.0 || *numeric == 1.0)) {
            event = static_cast<int>(*numeric);
        } else if (!configuration.logistic_event_level.empty()) {
            event = row[response_column] == configuration.logistic_event_level ? 1 : 0;
        }
        std::vector<double> predictor_row;
        bool complete = event >= 0;
        for (const std::size_t column : configuration.logistic_predictor_columns) {
            if (column >= row.size()) {
                complete = false;
                break;
            }
            const auto value = parse_numeric_cell(row[column]);
            if (!value.has_value()) {
                complete = false;
                break;
            }
            predictor_row.push_back(*value);
        }
        if (complete) {
            result.response.push_back(event);
            result.predictors.push_back(std::move(predictor_row));
            result.source_rows.push_back(row_index);
        }
    }
    return result;
}

}  // namespace

OutputPage AnalysisService::descriptive(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    OutputPage page;
    page.id = new_id("desc");
    page.title = "显示描述性统计";
    page.method_name = "Display Descriptive Statistics";
    page.configuration = configuration;

    std::vector<datalab::domain::statistics::DescriptiveStatisticsResult> rows;
    const std::vector<std::size_t> columns = configuration.variable_columns.empty()
        ? std::vector<std::size_t>{configuration.selection.measurement_column}
        : configuration.variable_columns;

    for (const std::size_t column : columns) {
        const ExtractedNumericColumn extracted =
            extract_numeric_column(table, column, configuration.excluded_rows);
        if (configuration.by_column.has_value()) {
            const std::vector<std::string> groups = extract_text_column(table, *configuration.by_column);
            std::map<std::string, std::vector<double>> grouped;
            std::map<std::string, std::size_t> missing;
            for (std::size_t index = 0; index < extracted.values.size(); ++index) {
                const std::size_t row = extracted.source_rows[index];
                const std::string label = row < groups.size() ? groups[row] : "*";
                grouped[label].push_back(extracted.values[index]);
            }
            for (std::size_t row = 0; row < table.rows.size(); ++row) {
                if (std::find(configuration.excluded_rows.begin(), configuration.excluded_rows.end(), row)
                    != configuration.excluded_rows.end()) {
                    continue;
                }
                if (column >= table.rows[row].size() || is_missing_cell(table.rows[row][column])) {
                    const std::string label = row < groups.size() ? groups[row] : "*";
                    ++missing[label];
                }
            }
            for (auto& [label, values] : grouped) {
                auto stats = datalab::domain::statistics::DescriptiveStatistics::calculate(
                    values, missing[label], values.size() + missing[label]);
                if (stats.has_value()) {
                    stats->group_label = extracted.name + " / " + label;
                    rows.push_back(*stats);
                }
            }
        } else {
            auto stats = datalab::domain::statistics::DescriptiveStatistics::calculate(
                extracted.values, extracted.missing_count, extracted.total_count);
            if (stats.has_value()) {
                stats->group_label = extracted.name;
                rows.push_back(*stats);
            }
        }
    }
    if (rows.empty()) {
        return error_page(page.title, page.method_name, "所选列没有足够的数值观测。");
    }
    page.tables.push_back(descriptive_table(rows));
    page.parameter_summary = "变量数 = " + std::to_string(columns.size());
    return page;
}

OutputPage AnalysisService::normality_test(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::size_t column = first_variable(configuration);
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, column, configuration.excluded_rows);
    const auto result = datalab::domain::statistics::normality_test(extracted.values);

    OutputPage page;
    page.id = new_id("normality");
    page.title = "正态性检验";
    page.method_name = "Normality Test";
    page.configuration = configuration;
    page.parameter_summary = "变量: " + extracted.name
        + "    方法: Anderson-Darling    缺失值 N* = "
        + std::to_string(extracted.missing_count);
    for (const std::string& diagnostic : result.diagnostics) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "normality", diagnostic});
    }
    StatisticTable table_out;
    table_out.title = "正态性检验";
    table_out.headers = {"变量", "N", "N*", "Mean", "StDev", "AD", "P-Value", "结论"};
    std::string conclusion = "无法检验";
    if (result.p_value.has_value()) {
        conclusion = *result.p_value < 0.05
            ? "拒绝正态分布假设" : "不能拒绝正态分布假设";
    }
    table_out.rows.push_back({
        extracted.name,
        std::to_string(result.count),
        std::to_string(extracted.missing_count),
        format_number(result.mean),
        result.sample_standard_deviation > 0.0
            ? format_number(result.sample_standard_deviation) : "*",
        result.anderson_darling.has_value() ? format_number(*result.anderson_darling) : "*",
        result.p_value.has_value() ? format_number(*result.p_value) : "*",
        conclusion});
    page.tables.push_back(table_out);

    PlotSpec plot;
    plot.kind = PlotKind::probability;
    plot.title = extracted.name + " 的正态概率图";
    plot.x_axis_title = "标准正态分位数";
    plot.y_axis_title = extracted.name;
    plot.values = result.probability_plot.ordered_values;
    plot.x_values = result.probability_plot.theoretical_quantiles;
    plot.line_width = 1.4;
    page.plots.push_back(plot);
    return page;
}

OutputPage AnalysisService::correlation(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() < 2) {
        return error_page("相关分析", "Correlation", "请选择至少两列数值变量。");
    }
    std::vector<std::vector<double>> columns;
    std::vector<std::string> names;
    for (const std::size_t column : configuration.variable_columns) {
        const ExtractedNumericColumn extracted =
            extract_numeric_column(table, column, configuration.excluded_rows);
        columns.push_back(extracted.values);
        names.push_back(extracted.name);
    }
    const bool spearman = configuration.correlation_method == "spearman";
    const auto result = datalab::domain::statistics::correlation_matrix(
        columns,
        spearman ? datalab::domain::statistics::CorrelationMethod::spearman
                 : datalab::domain::statistics::CorrelationMethod::pearson,
        configuration.confidence_level);
    OutputPage page;
    page.id = new_id("correlation");
    page.title = spearman ? "Spearman 秩相关" : "Pearson 相关";
    page.method_name = "Correlation";
    page.configuration = configuration;
    page.parameter_summary = "方法 = " + std::string(spearman ? "Spearman" : "Pearson")
        + "    置信水平 = " + format_number(configuration.confidence_level)
        + "    有效变量数 = " + std::to_string(columns.size());
    page.diagnostics = result.diagnostics;
    for (std::size_t index = 0; index < configuration.variable_columns.size(); ++index) {
        const ExtractedNumericColumn extracted = extract_numeric_column(
            table, configuration.variable_columns[index], configuration.excluded_rows);
        if (extracted.missing_count > 0) {
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::warning,
                "missing_values",
                names[index] + " 跳过 " + std::to_string(extracted.missing_count)
                    + " 个缺失或非法单元格。"});
        }
    }
    StatisticTable coefficients;
    coefficients.title = "相关系数矩阵";
    coefficients.headers.push_back("变量");
    coefficients.headers.insert(coefficients.headers.end(), names.cbegin(), names.cend());
    for (std::size_t row = 0; row < names.size(); ++row) {
        std::vector<std::string> values;
        values.push_back(names[row]);
        for (std::size_t column = 0; column < names.size(); ++column) {
            values.push_back(format_number(result.coefficients[row][column]));
        }
        coefficients.rows.push_back(values);
    }
    page.tables.push_back(coefficients);
    StatisticTable pair_table;
    pair_table.title = "相关分析详细结果";
    pair_table.headers = {"变量 1", "变量 2", "N", "相关系数", "P-Value", "置信区间"};
    for (const auto& pair : result.pairs) {
        const std::string interval = pair.confidence_lower.has_value()
            && pair.confidence_upper.has_value()
            ? "[" + format_number(*pair.confidence_lower) + ", "
                + format_number(*pair.confidence_upper) + "]" : "*";
        pair_table.rows.push_back({
            pair.first_column < names.size() ? names[pair.first_column] : "*",
            pair.second_column < names.size() ? names[pair.second_column] : "*",
            std::to_string(pair.count),
            format_number(pair.coefficient),
            pair.p_value.has_value() ? format_number(*pair.p_value) : "*",
            interval});
        append_diagnostics(page.diagnostics, pair.diagnostics, "相关分析：");
    }
    page.tables.push_back(pair_table);
    if (columns.size() == 2 && columns[0].size() == columns[1].size()) {
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = names[0] + " 与 " + names[1] + " 的散点图";
        plot.x_axis_title = names[0];
        plot.y_axis_title = names[1];
        plot.x_values = columns[0];
        plot.values = columns[1];
        page.plots.push_back(plot);
    }
    return page;
}

OutputPage AnalysisService::one_sample_t(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (!configuration.hypothesis_mean.has_value()) {
        return error_page("单样本 t 检验", "One-Sample T", "请指定假设均值。");
    }
    const auto result = datalab::domain::statistics::one_sample_t_test(
        extracted.values,
        *configuration.hypothesis_mean,
        configuration.confidence_level,
        parse_alternative(configuration.alternative));
    OutputPage page;
    page.id = new_id("one_sample_t");
    page.title = "单样本 t 检验";
    page.method_name = "One-Sample T";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    假设均值 = " + format_number(*configuration.hypothesis_mean)
        + "    备择：总体均值 " + alternative_label(configuration.alternative)
        + " 假设均值";
    append_diagnostics(page.diagnostics, result.diagnostics, "单样本 t：");
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "跳过 " + std::to_string(extracted.missing_count)
                + " 个缺失或非法单元格。"});
    }
    page.tables.push_back(t_test_table("单样本 t 检验", result, extracted.name));
    return page;
}

OutputPage AnalysisService::two_sample_t(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() < 2) {
        return error_page("双样本 t 检验", "Two-Sample T", "请选择两列独立样本变量。");
    }
    const ExtractedNumericColumn first = extract_numeric_column(
        table, configuration.variable_columns[0], configuration.excluded_rows);
    const ExtractedNumericColumn second = extract_numeric_column(
        table, configuration.variable_columns[1], configuration.excluded_rows);
    const auto result = datalab::domain::statistics::two_sample_t_test(
        first.values,
        second.values,
        configuration.confidence_level,
        parse_alternative(configuration.alternative),
        parse_variance_method(configuration.variance_method));
    OutputPage page;
    page.id = new_id("two_sample_t");
    page.title = "双样本 t 检验";
    page.method_name = "Two-Sample T";
    page.configuration = configuration;
    page.parameter_summary = "方法 = "
        + std::string(configuration.variance_method == "pooled" ? "合并方差" : "Welch")
        + "    置信水平 = " + format_number(configuration.confidence_level);
    append_diagnostics(page.diagnostics, result.diagnostics, "双样本 t：");
    if (first.missing_count > 0 || second.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "双样本 t 检验跳过缺失或非法单元格；组 1 = "
                + std::to_string(first.missing_count) + "，组 2 = "
                + std::to_string(second.missing_count) + "。"});
    }
    StatisticTable groups;
    groups.title = "双样本描述统计";
    groups.headers = {"组", "N", "Mean", "StDev"};
    groups.rows = {
        {first.name, std::to_string(result.first.count), format_number(result.first.mean),
         format_number(result.first.sample_standard_deviation)},
        {second.name, std::to_string(result.second.count), format_number(result.second.mean),
         format_number(result.second.sample_standard_deviation)}};
    page.tables.push_back(groups);
    StatisticTable test;
    test.title = "双样本 t 检验";
    test.headers = {"均值差", "SE 差值", "T", "DF", "P-Value", "置信区间"};
    const std::string interval = result.confidence_lower.has_value()
        && result.confidence_upper.has_value()
        ? "[" + format_number(*result.confidence_lower) + ", "
            + format_number(*result.confidence_upper) + "]" : "*";
    test.rows.push_back({
        format_number(result.mean_difference),
        format_number(result.standard_error_difference),
        format_number(result.t_statistic),
        format_number(result.degrees_of_freedom),
        result.p_value.has_value() ? format_number(*result.p_value) : "*",
        interval});
    page.tables.push_back(test);
    return page;
}

OutputPage AnalysisService::one_way_anova(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.by_column.has_value()) {
        return error_page("单因素 ANOVA", "One-Way ANOVA", "请选择因子/分组列。");
    }
    const ExtractedNumericColumn response = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    const std::vector<std::string> labels = extract_text_column(
        table, *configuration.by_column);
    std::map<std::string, std::vector<double>> grouped;
    for (std::size_t index = 0; index < response.values.size(); ++index) {
        const std::size_t row = response.source_rows[index];
        if (row >= labels.size() || is_missing_cell(labels[row])) {
            return error_page("单因素 ANOVA", "One-Way ANOVA",
                              "因子列存在缺失标签，无法进行分组。原始行 "
                                  + std::to_string(row + 1));
        }
        grouped[labels[row]].push_back(response.values[index]);
    }
    std::vector<std::vector<double>> groups;
    std::vector<std::string> group_labels;
    for (const auto& [label, values] : grouped) {
        group_labels.push_back(label);
        groups.push_back(values);
    }
    const auto result = datalab::domain::statistics::one_way_anova(groups, group_labels);
    OutputPage page;
    page.id = new_id("anova");
    page.title = "单因素 ANOVA";
    page.method_name = "One-Way ANOVA";
    page.configuration = configuration;
    page.parameter_summary = "响应 = " + response.name
        + "    因子 = " + column_label(table, *configuration.by_column);
    page.diagnostics = result.diagnostics;
    if (response.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "ANOVA 跳过 " + std::to_string(response.missing_count)
                + " 个缺失或非法响应值。"});
    }
    StatisticTable means;
    means.title = "组均值";
    means.headers = {"组", "N", "Mean", "StDev"};
    for (const auto& group : result.groups) {
        means.rows.push_back({
            group.label, std::to_string(group.count), format_number(group.mean),
            format_number(group.sample_standard_deviation)});
    }
    page.tables.push_back(means);
    StatisticTable anova;
    anova.title = "方差分析";
    anova.headers = {"来源", "DF", "SS", "MS", "F", "P-Value"};
    anova.rows = {
        {"组间", std::to_string(result.between_degrees_of_freedom),
         format_number(result.between_sum_of_squares), format_number(result.between_mean_square),
         format_number(result.f_statistic), result.p_value.has_value()
             ? format_number(*result.p_value) : "*"},
        {"误差", std::to_string(result.error_degrees_of_freedom),
         format_number(result.error_sum_of_squares), format_number(result.error_mean_square),
         "", ""},
        {"合计", std::to_string(result.total_degrees_of_freedom),
         format_number(result.total_sum_of_squares), "", "", ""}};
    page.tables.push_back(anova);
    const auto tukey = datalab::domain::statistics::tukey_multiple_comparisons(
        groups, group_labels, configuration.confidence_level);
    append_diagnostics(page.diagnostics, tukey.diagnostics, "Tukey: ");
    if (!tukey.comparisons.empty()) {
        StatisticTable comparisons;
        comparisons.title = "Tukey 同时比较";
        comparisons.headers = {"差值", "Difference", "SE Difference", "q",
                               "同时置信区间", "Adjusted P-Value"};
        for (const auto& comparison : tukey.comparisons) {
            comparisons.rows.push_back({
                comparison.first_label + " - " + comparison.second_label,
                format_number(comparison.mean_difference),
                format_number(comparison.standard_error),
                format_number(comparison.q_statistic),
                "[" + format_number(comparison.confidence_lower) + ", "
                    + format_number(comparison.confidence_upper) + "]",
                format_number(comparison.adjusted_p_value)});
        }
        page.tables.push_back(comparisons);
    }
    return page;
}

OutputPage AnalysisService::paired_t(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() < 2) {
        return error_page("配对 t 检验", "Paired t", "请选择两列配对测量值。");
    }
    const auto first = extract_numeric_column(
        table, configuration.variable_columns[0], configuration.excluded_rows);
    const auto second = extract_numeric_column(
        table, configuration.variable_columns[1], configuration.excluded_rows);
    const auto aligned = align_complete_rows({first, second});
    const std::vector<double>& first_values = aligned[0];
    const std::vector<double>& second_values = aligned[1];
    const auto result = datalab::domain::statistics::paired_t_test(
        first_values, second_values, configuration.confidence_level,
        parse_alternative(configuration.alternative));
    OutputPage page;
    page.id = new_id("paired_t");
    page.title = "配对 t 检验";
    page.method_name = "Paired t";
    page.configuration = configuration;
    page.parameter_summary = "第一列 = " + first.name + "    第二列 = " + second.name;
    page.diagnostics = result.diagnostics;
    StatisticTable output;
    output.title = "配对差值统计";
    output.headers = {"N", "Mean Difference", "StDev", "SE", "T", "DF",
                      "P-Value", "置信区间"};
    std::string interval = "*";
    if (result.confidence_lower.has_value() && result.confidence_upper.has_value()) {
        interval = "[" + format_number(*result.confidence_lower) + ", "
            + format_number(*result.confidence_upper) + "]";
    } else if (result.confidence_lower.has_value()) {
        interval = "[" + format_number(*result.confidence_lower) + ", +∞)";
    } else if (result.confidence_upper.has_value()) {
        interval = "(-∞, " + format_number(*result.confidence_upper) + "]";
    }
    output.rows.push_back({
        std::to_string(result.count), format_number(result.mean_difference),
        format_number(result.sample_standard_deviation), format_number(result.standard_error),
        format_number(result.t_statistic), format_number(result.degrees_of_freedom),
        result.p_value.has_value() ? format_number(*result.p_value) : "*", interval});
    page.tables.push_back(output);
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "配对测量散点图";
    plot.x_axis_title = first.name;
    plot.y_axis_title = second.name;
    plot.x_values = first_values;
    plot.values = second_values;
    page.plots.push_back(plot);
    return page;
}

OutputPage AnalysisService::regression(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() < 2) {
        return error_page("线性回归", "Linear Regression",
                          "请选择一个响应变量和至少一个预测变量。");
    }
    const auto response = extract_numeric_column(
        table, configuration.variable_columns.front(), configuration.excluded_rows);
    std::vector<ExtractedNumericColumn> predictors;
    for (std::size_t index = 1; index < configuration.variable_columns.size(); ++index) {
        predictors.push_back(extract_numeric_column(
            table, configuration.variable_columns[index], configuration.excluded_rows));
    }
    std::vector<ExtractedNumericColumn> columns;
    columns.reserve(predictors.size() + 1);
    columns.push_back(response);
    columns.insert(columns.end(), predictors.begin(), predictors.end());
    const auto aligned = align_complete_rows(columns);
    const std::vector<double>& response_values = aligned[0];
    std::vector<std::vector<double>> predictor_values;
    predictor_values.reserve(aligned.size() - 1);
    for (std::size_t index = 1; index < aligned.size(); ++index) {
        predictor_values.push_back(aligned[index]);
    }
    std::vector<std::string> labels;
    for (std::size_t index = 1; index < configuration.variable_columns.size(); ++index) {
        labels.push_back(column_label(table, configuration.variable_columns[index]));
    }
    const auto result = datalab::domain::statistics::fit_linear_regression(
        response_values, predictor_values, labels, configuration.confidence_level);
    OutputPage page;
    page.id = new_id("regression");
    page.title = "线性回归";
    page.method_name = "Linear Regression";
    page.configuration = configuration;
    page.parameter_summary = "响应 = " + response.name
        + "    预测变量数 = " + std::to_string(predictors.size());
    page.diagnostics = result.diagnostics;
    if (response.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "回归使用 complete-case，跳过响应缺失或预测变量不完整的行。"});
    }
    StatisticTable summary_table;
    summary_table.title = "模型摘要";
    summary_table.headers = {"S", "R-sq", "R-sq(adj)", "R-sq(pred)", "PRESS", "F",
                             "P-Value", "Durbin-Watson", "异常", "高杠杆", "影响"};
    summary_table.rows.push_back({
        format_number(result.residual_standard_deviation),
        format_number(result.r_squared), format_number(result.adjusted_r_squared),
        format_number(result.predicted_r_squared), format_number(result.press),
        format_number(result.f_statistic),
        result.model_p_value.has_value() ? format_number(*result.model_p_value) : "*",
        format_number(result.diagnostics_summary.durbin_watson),
        std::to_string(result.diagnostics_summary.outlier_count),
        std::to_string(result.diagnostics_summary.high_leverage_count),
        std::to_string(result.diagnostics_summary.influential_count)});
    page.tables.push_back(summary_table);
    StatisticTable coefficient_table;
    coefficient_table.title = "系数";
    coefficient_table.headers = {"项", "Coef", "SE Coef", "T", "P-Value", "置信区间", "VIF"};
    for (const auto& coefficient : result.coefficients) {
        coefficient_table.rows.push_back({
            coefficient.term, format_number(coefficient.coefficient),
            format_number(coefficient.standard_error), format_number(coefficient.t_statistic),
            coefficient.p_value.has_value() ? format_number(*coefficient.p_value) : "*",
            coefficient.confidence_lower.has_value() && coefficient.confidence_upper.has_value()
                ? "[" + format_number(*coefficient.confidence_lower) + ", "
                    + format_number(*coefficient.confidence_upper) + "]" : "*",
            coefficient.vif.has_value() ? format_number(*coefficient.vif) : ""});
    }
    page.tables.push_back(coefficient_table);
    StatisticTable anova_table;
    anova_table.title = "回归方差分析";
    anova_table.headers = {"来源", "DF", "SS", "MS", "F", "P-Value"};
    anova_table.rows = {
        {"回归", std::to_string(result.predictor_count),
         format_number(result.regression_sum_of_squares),
         format_number(result.regression_mean_square), format_number(result.f_statistic),
         result.model_p_value.has_value() ? format_number(*result.model_p_value) : "*"},
        {"误差", std::to_string(result.observation_count - result.predictor_count - 1),
         format_number(result.error_sum_of_squares), format_number(result.error_mean_square), "", ""},
        {"合计", std::to_string(result.observation_count - 1),
         format_number(result.total_sum_of_squares), "", "", ""}};
    page.tables.push_back(anova_table);
    StatisticTable diagnostics;
    diagnostics.title = "拟合与诊断";
    diagnostics.headers = {"观测", "拟合值", "残差", "标准化残差", "删除学生化残差",
                           "杠杆值", "Cook 距离", "DFITS", "诊断标记"};
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        const auto& observation = result.observations[index];
        diagnostics.rows.push_back({
            std::to_string(index + 1), format_number(observation.fitted),
            format_number(observation.residual), format_number(observation.standardized_residual),
            format_number(observation.deleted_studentized_residual),
            format_number(observation.leverage), format_number(observation.cooks_distance),
            format_number(observation.dfits),
            observation.diagnostic_flags.empty() ? "" :
                observation.diagnostic_flags.front()});
    }
    page.tables.push_back(diagnostics);
    PlotSpec residual_plot;
    residual_plot.kind = PlotKind::scatter;
    residual_plot.title = "残差与拟合值";
    residual_plot.x_axis_title = "拟合值";
    residual_plot.y_axis_title = "残差";
    for (const auto& observation : result.observations) {
        residual_plot.x_values.push_back(observation.fitted);
        residual_plot.values.push_back(observation.residual);
    }
    page.plots.push_back(residual_plot);
    PlotSpec order_plot;
    order_plot.kind = PlotKind::scatter;
    order_plot.title = "残差与观测顺序";
    order_plot.x_axis_title = "观测顺序";
    order_plot.y_axis_title = "残差";
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        order_plot.x_values.push_back(static_cast<double>(index + 1));
        order_plot.values.push_back(result.observations[index].residual);
    }
    page.plots.push_back(order_plot);
    if (!predictors.empty()) {
        PlotSpec predictor_plot;
        predictor_plot.kind = PlotKind::scatter;
        predictor_plot.title = "残差与预测变量";
        predictor_plot.x_axis_title = labels.front();
        predictor_plot.y_axis_title = "残差";
        for (std::size_t index = 0; index < predictor_values.size()
             && index < result.observations.size(); ++index) {
            predictor_plot.x_values.push_back(predictor_values[index].front());
            predictor_plot.values.push_back(result.observations[index].residual);
        }
        page.plots.push_back(predictor_plot);
    }
    if (result.diagnostics_summary.residual_normality.has_value()) {
        const auto& normality = *result.diagnostics_summary.residual_normality;
        page.interpretation.push_back({
            "残差正态性",
            {"Anderson-Darling = " + format_number(normality.anderson_darling.value_or(0.0))
                 + "，请结合正态概率图和 P 值判断残差正态性。"},
            DiagnosticMessage::Severity::info});
    }
    return page;
}

OutputPage AnalysisService::two_proportions(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.first_events_column.has_value()
        || !configuration.first_trials_column.has_value()
        || !configuration.second_events_column.has_value()
        || !configuration.second_trials_column.has_value()) {
        return error_page("两比例检验", "2 Proportions",
                          "请选择两组事件数和试验数列。");
    }
    const auto first_events = extract_numeric_column(
        table, *configuration.first_events_column, configuration.excluded_rows);
    const auto first_trials = extract_numeric_column(
        table, *configuration.first_trials_column, configuration.excluded_rows);
    const auto second_events = extract_numeric_column(
        table, *configuration.second_events_column, configuration.excluded_rows);
    const auto second_trials = extract_numeric_column(
        table, *configuration.second_trials_column, configuration.excluded_rows);
    std::vector<std::size_t> values;
    auto count_at = [](const ExtractedNumericColumn& column) {
        std::vector<std::size_t> counts;
        append_nonnegative_counts(column.values, counts);
        return counts;
    };
    const auto first_event_values = count_at(first_events);
    const auto first_trial_values = count_at(first_trials);
    const auto second_event_values = count_at(second_events);
    const auto second_trial_values = count_at(second_trials);
    if (first_event_values.size() != 1 || first_trial_values.size() != 1
        || second_event_values.size() != 1 || second_trial_values.size() != 1) {
        return error_page("两比例检验", "2 Proportions",
                          "两比例检验当前要求每组使用一行汇总计数。");
    }
    const auto result = datalab::domain::statistics::two_proportions_test(
        first_event_values.front(), first_trial_values.front(),
        second_event_values.front(), second_trial_values.front(),
        configuration.confidence_level, parse_alternative(configuration.alternative));
    OutputPage page;
    page.id = new_id("two_proportions");
    page.title = "两比例检验";
    page.method_name = "2 Proportions";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    page.parameter_summary = "第一组 = " + first_events.name + "/" + first_trials.name
        + "    第二组 = " + second_events.name + "/" + second_trials.name;
    StatisticTable output;
    output.title = "两比例检验";
    output.headers = {"组", "事件数", "试验数", "比例"};
    output.rows.push_back({"第一组", std::to_string(result.first_events),
                           std::to_string(result.first_trials),
                           format_number(result.first_proportion)});
    output.rows.push_back({"第二组", std::to_string(result.second_events),
                           std::to_string(result.second_trials),
                           format_number(result.second_proportion)});
    output.rows.push_back({"差值", "", "", format_number(result.difference)});
    page.tables.push_back(output);
    StatisticTable test;
    test.title = "检验结果";
    test.headers = {"Z", "P-Value", "置信区间", "Fisher P-Value"};
    const std::string interval = result.confidence_lower.has_value()
        && result.confidence_upper.has_value()
        ? "[" + format_number(*result.confidence_lower) + ", "
            + format_number(*result.confidence_upper) + "]" : "*";
    test.rows.push_back(std::vector<std::string>{
        format_number(result.z_statistic),
        result.p_value.has_value() ? format_number(*result.p_value) : "*",
        interval,
        result.fisher_p_value.has_value()
            ? format_number(*result.fisher_p_value) : "*"});
    page.tables.push_back(test);
    return page;
}

OutputPage AnalysisService::chi_square(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.row_category_column.has_value()
        || !configuration.column_category_column.has_value()) {
        return error_page("列联表卡方", "Chi-Square Association",
                          "请选择行分类列和列分类列。");
    }
    const auto rows = extract_text_column(table, *configuration.row_category_column);
    const auto columns = extract_text_column(table, *configuration.column_category_column);
    std::map<std::string, std::size_t> row_indices;
    std::map<std::string, std::size_t> column_indices;
    std::vector<std::vector<double>> observed;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (row >= rows.size() || row >= columns.size()
            || is_missing_cell(rows[row]) || is_missing_cell(columns[row])) {
            continue;
        }
        const auto row_result = row_indices.emplace(rows[row], row_indices.size());
        const auto column_result = column_indices.emplace(columns[row], column_indices.size());
        if (row_result.second) {
            observed.emplace_back(column_indices.size(), 0.0);
        }
        for (auto& values : observed) {
            values.resize(column_indices.size(), 0.0);
        }
        observed[row_result.first->second][column_result.first->second] += 1.0;
    }
    std::vector<std::string> row_labels(row_indices.size());
    for (const auto& [label, index] : row_indices) {
        row_labels[index] = label;
    }
    std::vector<std::string> column_labels(column_indices.size());
    for (const auto& [label, index] : column_indices) {
        column_labels[index] = label;
    }
    const auto result = datalab::domain::statistics::chi_square_association(
        observed, row_labels, column_labels);
    OutputPage page;
    page.id = new_id("chi_square");
    page.title = "列联表卡方";
    page.method_name = "Chi-Square Association";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    StatisticTable output;
    output.title = "卡方检验";
    output.headers = {"Pearson χ²", "DF", "P-Value", "Likelihood Ratio χ²", "P-Value"};
    output.rows.push_back({
        format_number(result.pearson_statistic), format_number(result.degrees_of_freedom),
        result.p_value.has_value() ? format_number(*result.p_value) : "*",
        format_number(result.likelihood_ratio_statistic),
        result.likelihood_ratio_p_value.has_value()
            ? format_number(*result.likelihood_ratio_p_value) : "*"});
    page.tables.push_back(output);
    StatisticTable cells;
    cells.title = "单元格统计";
    cells.headers = {"行", "列", "Observed", "Expected", "Raw Residual",
                     "Standardized Residual", "Adjusted Residual", "Contribution"};
    for (const auto& cell : result.cells) {
        cells.rows.push_back({
            cell.row_label, cell.column_label, format_number(cell.observed),
            format_number(cell.expected), format_number(cell.raw_residual),
            format_number(cell.standardized_residual), format_number(cell.adjusted_residual),
            format_number(cell.contribution)});
    }
    page.tables.push_back(cells);
    return page;
}

OutputPage AnalysisService::box_cox(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto extracted = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    std::optional<double> requested_lambda;
    if (configuration.hypothesis_mean.has_value()) {
        requested_lambda = configuration.hypothesis_mean;
    }
    const auto result = datalab::domain::statistics::box_cox_transform(
        extracted.values, requested_lambda);
    OutputPage page;
    page.id = new_id("box_cox");
    page.title = "Box-Cox 变换";
    page.method_name = "Box-Cox Transformation";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    lambda = " + format_number(result.lambda);
    page.diagnostics = result.diagnostics;
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "Box-Cox 跳过 " + std::to_string(extracted.missing_count)
                + " 个缺失或非法观测。"});
    }
    StatisticTable summary;
    summary.title = "变换参数";
    summary.headers = {"N", "Lambda", "Transformed StDev"};
    summary.rows.push_back({
        std::to_string(result.transformed_values.size()),
        format_number(result.lambda),
        format_number(result.transformed_standard_deviation)});
    page.tables.push_back(summary);
    if (configuration.specifications.lower.has_value()
        || configuration.specifications.upper.has_value()) {
        const auto transform_limit = [&result](double value) {
            return result.lambda == 0.0
                ? std::log(value)
                : (std::pow(value, result.lambda) - 1.0) / result.lambda;
        };
        SpecificationLimits transformed_specifications;
        if (configuration.specifications.lower.has_value()) {
            transformed_specifications.lower = transform_limit(
                *configuration.specifications.lower);
        }
        if (configuration.specifications.upper.has_value()) {
            transformed_specifications.upper = transform_limit(
                *configuration.specifications.upper);
        }
        if (configuration.specifications.target.has_value()) {
            transformed_specifications.target = transform_limit(
                *configuration.specifications.target);
        }
        const auto capability = datalab::domain::statistics::ProcessCapability::calculate(
            result.transformed_values, result.transformed_standard_deviation,
            transformed_specifications);
        StatisticTable capability_table;
        capability_table.title = "变换后过程能力";
        capability_table.headers = {"指标", "数值"};
        capability_table.rows = {
            {"Cp", format_optional(capability.cp)},
            {"Cpk", format_optional(capability.cpk)},
            {"Pp", format_optional(capability.pp)},
            {"Ppk", format_optional(capability.ppk)}};
        page.tables.push_back(capability_table);
        append_diagnostics(page.diagnostics, capability.diagnostics, "能力分析: ");
    }
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "Box-Cox 变换后序列";
    plot.x_axis_title = "观测序号";
    plot.y_axis_title = "变换值";
    for (std::size_t index = 0; index < result.transformed_values.size(); ++index) {
        plot.x_values.push_back(static_cast<double>(index + 1));
        plot.values.push_back(result.transformed_values[index]);
    }
    page.plots.push_back(plot);
    return page;
}

OutputPage AnalysisService::gage_rr(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.gage_measurement_column.has_value()
        || !configuration.gage_part_column.has_value()
        || !configuration.gage_operator_column.has_value()) {
        return error_page("Crossed Gage R&R", "Crossed Gage R&R",
                          "请选择测量值、零件和操作员列。");
    }
    const auto measurements = extract_numeric_column(
        table, *configuration.gage_measurement_column, configuration.excluded_rows);
    const auto parts = extract_text_column(table, *configuration.gage_part_column);
    const auto operators = extract_text_column(table, *configuration.gage_operator_column);
    std::map<std::size_t, double> measurement_by_row;
    for (std::size_t index = 0; index < measurements.values.size(); ++index) {
        measurement_by_row[measurements.source_rows[index]] = measurements.values[index];
    }
    std::vector<double> values;
    std::vector<std::string> part_values;
    std::vector<std::string> operator_values;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        const auto measurement = measurement_by_row.find(row);
        if (measurement == measurement_by_row.end()
            || row >= parts.size() || row >= operators.size()
            || is_missing_cell(parts[row]) || is_missing_cell(operators[row])) {
            continue;
        }
        values.push_back(measurement->second);
        part_values.push_back(parts[row]);
        operator_values.push_back(operators[row]);
    }
    const double tolerance = configuration.specifications.lower.has_value()
        && configuration.specifications.upper.has_value()
        ? *configuration.specifications.upper - *configuration.specifications.lower : 0.0;
    const auto result = datalab::domain::statistics::crossed_gage_rr(
        values, part_values, operator_values, tolerance);
    OutputPage page;
    page.id = new_id("gage_rr");
    page.title = "Crossed Gage R&R";
    page.method_name = "Crossed Gage R&R (ANOVA)";
    page.configuration = configuration;
    page.parameter_summary = "测量值 = " + measurements.name
        + "    零件 = " + column_label(table, *configuration.gage_part_column)
        + "    操作员 = " + column_label(table, *configuration.gage_operator_column);
    page.diagnostics = result.diagnostics;
    StatisticTable anova;
    anova.title = "Gage R&R 方差分析";
    anova.headers = {"来源", "DF", "SS", "MS", "F"};
    for (const auto& row : result.anova_rows) {
        anova.rows.push_back({
            row.source, std::to_string(row.degrees_of_freedom),
            format_number(row.sum_of_squares), format_number(row.mean_square),
            format_number(row.f_statistic)});
    }
    page.tables.push_back(anova);
    StatisticTable components;
    components.title = "方差分量";
    components.headers = {"来源", "VarComp", "StdDev", "%Contribution",
                          "Study Var", "%Study Var", "%Tolerance"};
    for (const auto& component : result.variance_components) {
        components.rows.push_back({
            component.source, format_number(component.variance_component),
            format_number(component.standard_deviation),
            format_number(component.percent_contribution),
            format_number(component.study_variation),
            format_number(component.percent_study_variation),
            format_number(component.percent_tolerance)});
    }
    page.tables.push_back(components);
    StatisticTable summary;
    summary.title = "Gage R&R 摘要";
    summary.headers = {"零件数", "操作员数", "重复次数", "ndc"};
    summary.rows.push_back({
        std::to_string(result.part_count), std::to_string(result.operator_count),
        std::to_string(result.replicate_count), format_number(result.ndc)});
    page.tables.push_back(summary);
    return page;
}

OutputPage AnalysisService::mann_whitney(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() != 2) {
        return error_page("Mann-Whitney 检验", "Mann-Whitney", "请选择正好两列独立样本。");
    }
    const auto first = extract_numeric_column(
        table, configuration.variable_columns[0], configuration.excluded_rows);
    const auto second = extract_numeric_column(
        table, configuration.variable_columns[1], configuration.excluded_rows);
    const auto result = datalab::domain::statistics::mann_whitney(
        first.values, second.values, parse_alternative(configuration.alternative));
    OutputPage page;
    page.id = new_id("mann_whitney");
    page.title = "Mann-Whitney 检验";
    page.method_name = "Mann-Whitney U";
    page.configuration = configuration;
    page.parameter_summary = first.name + " vs " + second.name;
    page.diagnostics = result.diagnostics;
    StatisticTable output;
    output.title = "秩和检验";
    output.headers = {"第一组 N", "第二组 N", "秩和", "Z", "P-Value", "位置差异"};
    output.rows.push_back({std::to_string(result.first_count), std::to_string(result.second_count),
        format_number(result.rank_sum), format_number(result.z_statistic),
        format_optional(result.p_value), format_optional(result.location_difference)});
    page.tables.push_back(output);
    return page;
}

OutputPage AnalysisService::wilcoxon_signed_rank(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() != 2) {
        return error_page("Wilcoxon 符号秩检验", "Wilcoxon signed-rank", "请选择正好两列配对样本。");
    }
    const auto first = extract_numeric_column(
        table, configuration.variable_columns[0], configuration.excluded_rows);
    const auto second = extract_numeric_column(
        table, configuration.variable_columns[1], configuration.excluded_rows);
    const auto result = datalab::domain::statistics::wilcoxon_signed_rank(
        first.values, second.values, parse_alternative(configuration.alternative));
    OutputPage page;
    page.id = new_id("wilcoxon");
    page.title = "Wilcoxon 符号秩检验";
    page.method_name = "Wilcoxon signed-rank";
    page.configuration = configuration;
    page.parameter_summary = first.name + " vs " + second.name;
    page.diagnostics = result.diagnostics;
    StatisticTable output;
    output.title = "符号秩检验";
    output.headers = {"非零差值 N", "正秩和", "负秩和", "Z", "P-Value"};
    output.rows.push_back({std::to_string(result.count), format_number(result.positive_rank_sum),
        format_number(result.negative_rank_sum), format_number(result.z_statistic),
        format_optional(result.p_value)});
    page.tables.push_back(output);
    return page;
}

OutputPage AnalysisService::kruskal_wallis(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.empty() || !configuration.by_column.has_value()) {
        return error_page("Kruskal-Wallis 检验", "Kruskal-Wallis", "请选择测量列和分组列。");
    }
    const auto extracted = extract_numeric_column(
        table, configuration.variable_columns.front(), configuration.excluded_rows);
    const auto labels = extract_text_column(table, *configuration.by_column);
    std::map<std::string, std::vector<double>> grouped;
    for (std::size_t index = 0; index < extracted.values.size(); ++index) {
        const std::size_t row = extracted.source_rows[index];
        if (row < labels.size() && !is_missing_cell(labels[row])) {
            grouped[labels[row]].push_back(extracted.values[index]);
        }
    }
    std::vector<std::vector<double>> groups;
    std::vector<std::string> group_labels;
    for (auto& [label, values] : grouped) {
        if (!values.empty()) {
            group_labels.push_back(label);
            groups.push_back(std::move(values));
        }
    }
    const auto result = datalab::domain::statistics::kruskal_wallis(groups, group_labels);
    OutputPage page;
    page.id = new_id("kruskal_wallis");
    page.title = "Kruskal-Wallis 检验";
    page.method_name = "Kruskal-Wallis";
    page.configuration = configuration;
    page.parameter_summary = "测量值 = " + extracted.name;
    page.diagnostics = result.diagnostics;
    StatisticTable output;
    output.title = "Kruskal-Wallis 结果";
    output.headers = {"组别", "N", "中位数", "平均秩"};
    for (const auto& group : result.groups) {
        output.rows.push_back({group.label, std::to_string(group.count),
            format_number(group.median), format_number(group.mean_rank)});
    }
    page.tables.push_back(output);
    StatisticTable summary;
    summary.title = "检验统计量";
    summary.headers = {"H", "调整后 H", "DF", "P-Value"};
    summary.rows.push_back({format_number(result.h_statistic),
        format_number(result.adjusted_h_statistic), format_number(result.degrees_of_freedom),
        format_optional(result.p_value)});
    page.tables.push_back(summary);
    return page;
}

OutputPage AnalysisService::ewma(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto extracted = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    datalab::domain::statistics::EwmaOptions options;
    options.lambda = configuration.ewma_lambda;
    options.limit_sigma = configuration.ewma_limit_sigma;
    options.historical_mean = configuration.historical_center;
    const auto chart = datalab::domain::statistics::ControlCharts::ewma_chart(
        extracted.values, options);
    OutputPage page;
    page.id = new_id("ewma");
    page.title = "EWMA 控制图";
    page.method_name = "EWMA Chart";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    λ = " + format_number(options.lambda)
        + "    控制限倍数 = " + format_number(options.limit_sigma);
    page.diagnostics = chart.diagnostics;
    page.plots.push_back(control_plot("EWMA 控制图", "EWMA", chart, extracted.source_rows));
    return page;
}

OutputPage AnalysisService::cusum(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto extracted = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    datalab::domain::statistics::CusumOptions options;
    options.target = configuration.cusum_target;
    options.sigma = configuration.cusum_sigma;
    options.k = configuration.cusum_k;
    options.h = configuration.cusum_h;
    options.fast_initial_response = configuration.cusum_fast_initial_response;
    const auto chart = datalab::domain::statistics::ControlCharts::cusum_chart(
        extracted.values, options);
    OutputPage page;
    page.id = new_id("cusum");
    page.title = "CUSUM 控制图";
    page.method_name = "CUSUM Chart";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    目标 = " + format_number(options.target)
        + "    k = " + format_number(options.k)
        + "    h = " + format_number(options.h);
    page.diagnostics = chart.diagnostics;
    page.plots.push_back(control_plot("CUSUM 控制图", "CUSUM", chart.primary, extracted.source_rows));
    return page;
}

OutputPage AnalysisService::time_series_smoothing(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto extracted = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    const bool double_method = configuration.smoothing_method != "single";
    const auto result = double_method
        ? datalab::domain::statistics::double_exponential_smoothing(
            extracted.values, configuration.smoothing_alpha, configuration.smoothing_gamma,
            static_cast<std::size_t>(std::max(1, configuration.forecast_periods)))
        : datalab::domain::statistics::single_exponential_smoothing(
            extracted.values, configuration.smoothing_alpha,
            static_cast<std::size_t>(std::max(1, configuration.forecast_periods)));
    OutputPage page;
    page.id = new_id("time_series");
    page.title = "时间序列平滑";
    page.method_name = double_method ? "Double Exponential Smoothing" : "Single Exponential Smoothing";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name;
    page.diagnostics = result.diagnostics;
    StatisticTable output;
    output.title = "平滑与预测";
    output.headers = {"序号", "拟合值", "预测值", "下限", "上限"};
    for (std::size_t index = 0; index < extracted.values.size(); ++index) {
        output.rows.push_back({std::to_string(index + 1), format_number(result.fitted[index]),
            "", "", ""});
    }
    for (std::size_t index = 0; index < result.forecasts.size(); ++index) {
        output.rows.push_back({std::to_string(extracted.values.size() + index + 1), "",
            format_number(result.forecasts[index]), format_number(result.lower[index]),
            format_number(result.upper[index])});
    }
    page.tables.push_back(output);
    StatisticTable metrics;
    metrics.title = "预测误差";
    metrics.headers = {"MAD", "MSD", "MAPE (%)"};
    metrics.rows.push_back({format_number(result.mad), format_number(result.msd),
        format_number(result.mape)});
    page.tables.push_back(metrics);
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "实际值、拟合值与预测区间";
    plot.x_axis_title = "观测序号";
    plot.y_axis_title = extracted.name;
    datalab::domain::PlotSeries actual;
    actual.role = datalab::domain::PlotSeriesRole::actual;
    actual.label = "实际值";
    actual.values = extracted.values;
    actual.show_points = true;
    datalab::domain::PlotSeries fitted;
    fitted.role = datalab::domain::PlotSeriesRole::fitted;
    fitted.label = "拟合值";
    fitted.values = result.fitted;
    datalab::domain::PlotSeries forecast;
    forecast.role = datalab::domain::PlotSeriesRole::forecast;
    forecast.label = "预测值";
    forecast.values = result.forecasts;
    forecast.x_values.resize(result.forecasts.size());
    for (std::size_t index = 0; index < forecast.x_values.size(); ++index) {
        forecast.x_values[index] = static_cast<double>(extracted.values.size() + index);
    }
    datalab::domain::PlotSeries confidence;
    confidence.role = datalab::domain::PlotSeriesRole::confidence_band;
    confidence.label = "预测区间";
    confidence.lower = result.lower;
    confidence.upper = result.upper;
    confidence.x_values = forecast.x_values;
    plot.series = {std::move(actual), std::move(fitted), std::move(forecast),
                   std::move(confidence)};
    page.plots.push_back(std::move(plot));
    return page;
}

OutputPage AnalysisService::arima(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.arima_value_column.has_value()) {
        return error_page("ARIMA", "ARIMA", "请选择时间序列数值列。");
    }
    const auto values = extract_numeric_column(
        table, *configuration.arima_value_column, configuration.excluded_rows);
    if (values.values.empty()) {
        return error_page("ARIMA", "ARIMA", "时间序列没有有效数值观测。");
    }
    if (configuration.arima_time_column.has_value()) {
        const auto time = extract_numeric_column(
            table, *configuration.arima_time_column, configuration.excluded_rows);
        if (time.values.size() != values.values.size()) {
            return error_page("ARIMA", "ARIMA", "时间列与数值列的有效行不一致。");
        }
        for (std::size_t index = 1; index < time.values.size(); ++index) {
            if (!(time.values[index] > time.values[index - 1])) {
                return error_page("ARIMA", "ARIMA",
                                  "时间列必须严格递增，且不能包含重复时间点。");
            }
        }
    }
    const std::size_t forecast_periods = configuration.forecast_periods > 0
        ? static_cast<std::size_t>(configuration.forecast_periods) : 1;
    const auto candidates = datalab::domain::statistics::fit_arima_candidates(
        values.values, forecast_periods);
    if (candidates.empty()) {
        return error_page("ARIMA", "ARIMA", "没有可拟合的候选模型。");
    }
    auto criterion_value = [&](const datalab::domain::statistics::ArimaResult& result) {
        if (configuration.arima_selection_criterion == "aic") {
            return result.aic;
        }
        if (configuration.arima_selection_criterion == "bic") {
            return result.bic;
        }
        return result.aicc;
    };
    const auto best = std::min_element(
        candidates.cbegin(), candidates.cend(),
        [&](const auto& first, const auto& second) {
            return criterion_value(first) < criterion_value(second);
        });
    auto model_name = [](datalab::domain::statistics::ArimaModel model) {
        switch (model) {
        case datalab::domain::statistics::ArimaModel::ar_1:
            return std::string("AR(1)");
        case datalab::domain::statistics::ArimaModel::ma_1:
            return std::string("MA(1)");
        case datalab::domain::statistics::ArimaModel::arima_0_1_0:
        default:
            return std::string("ARIMA(0,1,0)");
        }
    };
    OutputPage page;
    page.id = new_id("arima");
    page.title = "ARIMA 基础预测";
    page.method_name = "ARIMA";
    page.configuration = configuration;
    page.parameter_summary = "响应列 = "
        + column_label(table, *configuration.arima_value_column)
        + "    选择准则 = " + configuration.arima_selection_criterion;
    for (const auto& candidate : candidates) {
        page.diagnostics.insert(page.diagnostics.end(),
                                candidate.diagnostics.cbegin(),
                                candidate.diagnostics.cend());
    }
    StatisticTable comparison;
    comparison.title = "候选模型比较";
    comparison.headers = {"模型", "SSE", "AIC", "AICc", "BIC"};
    for (const auto& candidate : candidates) {
        comparison.rows.push_back({
            model_name(candidate.model), format_number(candidate.sse),
            format_number(candidate.aic), format_number(candidate.aicc),
            format_number(candidate.bic)});
    }
    page.tables.push_back(comparison);
    StatisticTable forecast;
    forecast.title = "模型摘要与预测";
    forecast.headers = {"最优模型", "截距", "系数/漂移", "预测期", "Forecast", "Lower", "Upper"};
    for (std::size_t index = 0; index < best->forecasts.size(); ++index) {
        forecast.rows.push_back({
            index == 0 ? model_name(best->model) : "",
            index == 0 ? format_number(best->intercept) : "",
            index == 0 ? format_number(best->coefficient + best->drift) : "",
            std::to_string(index + 1),
            format_number(best->forecasts[index]),
            index < best->lower.size() ? format_number(best->lower[index]) : "*",
            index < best->upper.size() ? format_number(best->upper[index]) : "*"});
    }
    page.tables.push_back(forecast);
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "ARIMA 拟合与预测";
    plot.x_axis_title = "Period";
    plot.y_axis_title = column_label(table, *configuration.arima_value_column);
    plot.x_values.reserve(best->fitted.size() + best->forecasts.size());
    plot.values.reserve(best->fitted.size() + best->forecasts.size());
    for (std::size_t index = 0; index < best->fitted.size(); ++index) {
        plot.x_values.push_back(static_cast<double>(index + 1));
        plot.values.push_back(best->fitted[index]);
    }
    for (std::size_t index = 0; index < best->forecasts.size(); ++index) {
        plot.x_values.push_back(static_cast<double>(best->fitted.size() + index + 1));
        plot.values.push_back(best->forecasts[index]);
    }
    datalab::domain::PlotSeries actual;
    actual.role = datalab::domain::PlotSeriesRole::actual;
    actual.label = "实际值";
    actual.values = values.values;
    actual.show_points = true;
    actual.x_values.resize(values.values.size());
    for (std::size_t index = 0; index < actual.x_values.size(); ++index) {
        actual.x_values[index] = static_cast<double>(index + 1);
    }
    datalab::domain::PlotSeries fitted_series;
    fitted_series.role = datalab::domain::PlotSeriesRole::fitted;
    fitted_series.label = "拟合值";
    fitted_series.values = best->fitted;
    fitted_series.x_values.resize(best->fitted.size());
    for (std::size_t index = 0; index < fitted_series.x_values.size(); ++index) {
        fitted_series.x_values[index] = static_cast<double>(index + 1);
    }
    datalab::domain::PlotSeries forecast_series;
    forecast_series.role = datalab::domain::PlotSeriesRole::forecast;
    forecast_series.label = "预测值";
    forecast_series.values = best->forecasts;
    forecast_series.lower = best->lower;
    forecast_series.upper = best->upper;
    forecast_series.x_values.resize(best->forecasts.size());
    for (std::size_t index = 0; index < forecast_series.x_values.size(); ++index) {
        forecast_series.x_values[index] =
            static_cast<double>(best->fitted.size() + index + 1);
    }
    datalab::domain::PlotSeries confidence;
    confidence.role = datalab::domain::PlotSeriesRole::confidence_band;
    confidence.label = "预测区间";
    confidence.lower = best->lower;
    confidence.upper = best->upper;
    confidence.x_values = forecast_series.x_values;
    plot.series = {std::move(actual), std::move(fitted_series),
                   std::move(forecast_series), std::move(confidence)};
    page.plots.push_back(plot);
    return page;
}

OutputPage AnalysisService::two_factor_anova(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.anova_response_column.has_value()
        || !configuration.anova_factor_a_column.has_value()
        || !configuration.anova_factor_b_column.has_value()) {
        return error_page("双因素 ANOVA", "Two-Factor ANOVA",
                          "请选择响应变量、因子 A 和因子 B。");
    }
    datalab::domain::statistics::TwoFactorAnovaInput input;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row_index)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        const auto& row = table.rows[row_index];
        const std::size_t response_column = *configuration.anova_response_column;
        const std::size_t factor_a_column = *configuration.anova_factor_a_column;
        const std::size_t factor_b_column = *configuration.anova_factor_b_column;
        if (response_column >= row.size() || factor_a_column >= row.size()
            || factor_b_column >= row.size()) {
            continue;
        }
        const auto response = parse_numeric_cell(row[response_column]);
        if (!response.has_value() || is_missing_cell(row[factor_a_column])
            || is_missing_cell(row[factor_b_column])) {
            continue;
        }
        input.response.push_back(*response);
        input.factor_a.push_back(row[factor_a_column]);
        input.factor_b.push_back(row[factor_b_column]);
    }
    input.encoding = configuration.anova_factor_encoding == "effect"
        ? datalab::domain::statistics::AnovaFactorEncoding::effect
        : datalab::domain::statistics::AnovaFactorEncoding::reference;
    const auto result = datalab::domain::statistics::two_factor_anova(input);
    OutputPage page;
    page.id = new_id("two_factor_anova");
    page.title = "双因素 ANOVA";
    page.method_name = "Two-Factor ANOVA";
    page.configuration = configuration;
    page.parameter_summary = "响应 = "
        + column_label(table, *configuration.anova_response_column)
        + "    因子 A = " + column_label(table, *configuration.anova_factor_a_column)
        + "    因子 B = " + column_label(table, *configuration.anova_factor_b_column);
    page.diagnostics = result.diagnostics;
    StatisticTable effects;
    effects.title = "ANOVA 表";
    effects.headers = {"来源", "Seq SS", "Adj SS", "DF", "MS", "F", "P-Value"};
    for (const auto& effect : result.effects) {
        effects.rows.push_back({
            effect.term, format_number(effect.sequential_sum_of_squares),
            format_number(effect.adjusted_sum_of_squares),
            std::to_string(effect.degrees_of_freedom), format_number(effect.mean_square),
            format_number(effect.f_statistic),
            effect.p_value.has_value() ? format_number(*effect.p_value) : "*"});
    }
    effects.rows.push_back({"Error", "", "", std::to_string(result.error_degrees_of_freedom),
                            format_number(result.error_mean_square),
                            "", ""});
    page.tables.push_back(effects);
    StatisticTable means_a;
    means_a.title = "因子 A 均值";
    means_a.headers = {"水平", "N", "均值"};
    for (const auto& mean : result.factor_a_means) {
        means_a.rows.push_back({mean.level, std::to_string(mean.count), format_number(mean.mean)});
    }
    page.tables.push_back(means_a);
    StatisticTable means_b;
    means_b.title = "因子 B 均值";
    means_b.headers = {"水平", "N", "均值"};
    for (const auto& mean : result.factor_b_means) {
        means_b.rows.push_back({mean.level, std::to_string(mean.count), format_number(mean.mean)});
    }
    page.tables.push_back(means_b);
    StatisticTable interaction;
    interaction.title = "交互均值";
    interaction.headers = {"因子 A", "因子 B", "N", "均值"};
    for (const auto& mean : result.interaction_means) {
        interaction.rows.push_back({mean.factor_a_level, mean.factor_b_level,
                                    std::to_string(mean.count), format_number(mean.mean)});
    }
    page.tables.push_back(interaction);
    return page;
}

OutputPage AnalysisService::logistic_regression(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.logistic_response_column.has_value()
        || configuration.logistic_predictor_columns.empty()) {
        return error_page("二元 Logistic 回归", "Binary Logistic Regression",
                          "请选择二元响应列和至少一个预测变量。");
    }
    const LogisticImport imported = logistic_import_rows(table, configuration);
    const std::vector<int>& response = imported.response;
    const std::vector<std::vector<double>>& predictors = imported.predictors;
    const std::vector<std::size_t>& source_rows = imported.source_rows;
    std::vector<std::string> labels;
    for (const std::size_t column : configuration.logistic_predictor_columns) {
        labels.push_back(column_label(table, column));
    }
    const auto result = datalab::domain::statistics::fit_logistic_regression(
        response, predictors, labels, configuration.confidence_level,
        static_cast<std::size_t>(std::max(1, configuration.logistic_max_iterations)),
        configuration.logistic_tolerance);
    OutputPage page;
    page.id = new_id("logistic");
    page.title = "二元 Logistic 回归";
    page.method_name = "Binary Logistic Regression";
    page.configuration = configuration;
    page.parameter_summary = "响应 = "
        + column_label(table, *configuration.logistic_response_column)
        + "    事件水平 = " + configuration.logistic_event_level
        + "    预测变量数 = " + std::to_string(labels.size());
    page.diagnostics = result.diagnostics;
    if (source_rows.size() < table.rows.size()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "Logistic 回归使用 complete-case，缺失或非法行已排除。"});
    }
    StatisticTable summary;
    summary.title = "模型摘要";
    summary.headers = {"N", "迭代次数", "收敛", "Log-Likelihood", "Deviance", "AIC", "BIC"};
    summary.rows.push_back({
        std::to_string(result.observation_count),
        std::to_string(result.iteration_count),
        result.converged ? "是" : "否",
        format_number(result.log_likelihood), format_number(result.deviance),
        format_number(result.aic), format_number(result.bic)});
    page.tables.push_back(summary);
    StatisticTable coefficients;
    coefficients.title = "系数与 Odds Ratio";
    coefficients.headers = {"项", "Coef", "SE Coef", "Z", "P-Value",
                            "Odds Ratio", "95% CI"};
    for (const auto& coefficient : result.coefficients) {
        coefficients.rows.push_back({
            coefficient.term, format_number(coefficient.coefficient),
            format_number(coefficient.standard_error),
            format_number(coefficient.z_statistic),
            format_number(coefficient.p_value),
            format_number(coefficient.odds_ratio),
            "[" + format_number(coefficient.confidence_lower) + ", "
                + format_number(coefficient.confidence_upper) + "]"});
    }
    page.tables.push_back(coefficients);
    StatisticTable fitted;
    fitted.title = "拟合与残差";
    fitted.headers = {"原始行", "响应", "预测概率", "Pearson 残差", "Deviance 残差"};
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        const auto& observation = result.observations[index];
        fitted.rows.push_back({
            index < source_rows.size() ? std::to_string(source_rows[index] + 1) : "*",
            std::to_string(observation.response), format_number(observation.probability),
            format_number(observation.pearson_residual),
            format_number(observation.deviance_residual)});
    }
    page.tables.push_back(fitted);
    PlotSpec probability;
    probability.kind = PlotKind::scatter;
    probability.title = "预测概率";
    probability.x_axis_title = "观测顺序";
    probability.y_axis_title = "事件概率";
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        probability.x_values.push_back(static_cast<double>(index + 1));
        probability.values.push_back(result.observations[index].probability);
        probability.source_rows.push_back(
            index < source_rows.size() ? source_rows[index] : index);
    }
    page.plots.push_back(probability);
    return page;
}

OutputPage AnalysisService::variance_test(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.variance_first_column.has_value()) {
        return error_page("方差检验", "Variance Test", "请选择第一样本列。");
    }
    const auto first = extract_numeric_column(
        table, *configuration.variance_first_column, configuration.excluded_rows);
    const auto alternative = parse_alternative(configuration.variance_alternative);
    OutputPage page;
    page.id = new_id("variance");
    page.title = "方差检验";
    page.method_name = "Variance Test";
    page.configuration = configuration;
    page.parameter_summary = "第一样本 = "
        + column_label(table, *configuration.variance_first_column)
        + "    方法 = " + configuration.variance_test_method;
    StatisticTable summary;
    summary.title = "方差检验结果";
    summary.headers = {"方法", "N", "统计量", "DF", "P-Value", "置信区间"};
    if (!configuration.variance_second_column.has_value()) {
        if (!configuration.hypothesized_variance.has_value()) {
            return error_page("一方差检验", "1 Variance",
                              "一方差检验需要输入假设方差。");
        }
        const auto result = datalab::domain::statistics::chi_square_one_variance_test(
            first.values, *configuration.hypothesized_variance,
            configuration.confidence_level, alternative);
        page.diagnostics = result.diagnostics;
        summary.rows.push_back({
            "Chi-Square", std::to_string(result.count),
            format_number(result.chi_square_statistic),
            format_number(result.degrees_of_freedom),
            result.p_value.has_value() ? format_number(*result.p_value) : "*",
            result.confidence_lower.has_value() && result.confidence_upper.has_value()
                ? "[" + format_number(*result.confidence_lower) + ", "
                    + format_number(*result.confidence_upper) + "]" : "*"});
    } else {
        const auto second = extract_numeric_column(
            table, *configuration.variance_second_column, configuration.excluded_rows);
        const auto f_result = datalab::domain::statistics::f_test_two_variances(
            first.values, second.values, configuration.confidence_level, alternative);
        page.diagnostics = f_result.diagnostics;
        summary.rows.push_back({
            "F-test", std::to_string(f_result.first_count) + " / "
                + std::to_string(f_result.second_count),
            format_number(f_result.f_statistic),
            format_number(f_result.numerator_degrees_of_freedom) + " / "
                + format_number(f_result.denominator_degrees_of_freedom),
            f_result.p_value.has_value() ? format_number(*f_result.p_value) : "*",
            f_result.confidence_lower.has_value() && f_result.confidence_upper.has_value()
                ? "[" + format_number(*f_result.confidence_lower) + ", "
                    + format_number(*f_result.confidence_upper) + "]" : "*"});
        const auto robust = configuration.variance_test_method == "brown_forsythe"
            ? datalab::domain::statistics::brown_forsythe_two_variances(
                first.values, second.values, configuration.confidence_level, alternative)
            : datalab::domain::statistics::levene_two_variances(
                first.values, second.values, configuration.confidence_level, alternative);
        page.diagnostics.insert(page.diagnostics.end(), robust.diagnostics.cbegin(),
                                robust.diagnostics.cend());
        summary.rows.push_back({
            configuration.variance_test_method == "brown_forsythe"
                ? "Brown-Forsythe" : "Levene",
            std::to_string(robust.total_count),
            format_number(robust.f_statistic),
            format_number(robust.numerator_degrees_of_freedom) + " / "
                + format_number(robust.denominator_degrees_of_freedom),
            robust.p_value.has_value() ? format_number(*robust.p_value) : "*", "*"});
    }
    page.tables.push_back(summary);
    return page;
}

OutputPage AnalysisService::time_series_decomposition(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.decomposition_value_column.has_value()
        || configuration.decomposition_seasonal_period < 1) {
        return error_page("时间序列分解", "Time Series Decomposition",
                          "请选择时间序列值列并输入正整数季节周期。");
    }
    const auto values = extract_numeric_column(
        table, *configuration.decomposition_value_column, configuration.excluded_rows);
    std::vector<double> time;
    if (configuration.decomposition_time_column.has_value()) {
        const auto extracted = extract_numeric_column(
            table, *configuration.decomposition_time_column, configuration.excluded_rows);
        time = extracted.values;
    } else {
        time.reserve(values.values.size());
        for (std::size_t index = 0; index < values.values.size(); ++index) {
            time.push_back(static_cast<double>(index + 1));
        }
    }
    const auto result = datalab::domain::statistics::decompose_time_series(
        {time, values.values},
        {configuration.decomposition_model == "multiplicative"
             ? datalab::domain::statistics::DecompositionModel::multiplicative
             : datalab::domain::statistics::DecompositionModel::additive,
         static_cast<std::size_t>(configuration.decomposition_seasonal_period),
         static_cast<std::size_t>(std::max(1, configuration.forecast_periods))});
    OutputPage page;
    page.id = new_id("decomposition");
    page.title = "时间序列分解";
    page.method_name = "Time Series Decomposition";
    page.configuration = configuration;
    page.parameter_summary = "值列 = "
        + column_label(table, *configuration.decomposition_value_column)
        + "    周期 = " + std::to_string(configuration.decomposition_seasonal_period)
        + "    模型 = " + configuration.decomposition_model;
    page.diagnostics = result.diagnostics;
    StatisticTable summary;
    summary.title = "分解模型摘要";
    summary.headers = {"Trend Intercept", "Trend Slope", "MAD", "MSD", "MAPE"};
    summary.rows.push_back({
        format_number(result.trend_intercept), format_number(result.trend_slope),
        format_number(result.mad), format_number(result.msd), format_number(result.mape)});
    page.tables.push_back(summary);
    StatisticTable detail;
    detail.title = "分解明细";
    detail.headers = {"Time", "Observed", "Moving Average", "Trend", "Fitted", "Residual"};
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        detail.rows.push_back({
            index < result.time.size() ? format_number(result.time[index]) : "*",
            format_number(result.observations[index]),
            index < result.centered_moving_average.size()
                ? format_number(result.centered_moving_average[index]) : "*",
            index < result.trend.size() ? format_number(result.trend[index]) : "*",
            index < result.fitted.size() ? format_number(result.fitted[index]) : "*",
            index < result.residuals.size() ? format_number(result.residuals[index]) : "*"});
    }
    page.tables.push_back(detail);
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "时间序列分解拟合";
    plot.x_axis_title = "Time";
    plot.y_axis_title = column_label(table, *configuration.decomposition_value_column);
    plot.x_values = result.time;
    plot.values = result.observations;
    datalab::domain::PlotSeries observed;
    observed.role = datalab::domain::PlotSeriesRole::actual;
    observed.label = "实际值";
    observed.x_values = result.time;
    observed.values = result.observations;
    observed.show_points = true;
    datalab::domain::PlotSeries trend;
    trend.role = datalab::domain::PlotSeriesRole::trend;
    trend.label = "趋势";
    trend.x_values = result.time;
    trend.values = result.trend;
    datalab::domain::PlotSeries fitted;
    fitted.role = datalab::domain::PlotSeriesRole::fitted;
    fitted.label = "拟合值";
    fitted.x_values = result.time;
    fitted.values = result.fitted;
    datalab::domain::PlotSeries remainder;
    remainder.role = datalab::domain::PlotSeriesRole::remainder;
    remainder.label = "残差";
    remainder.x_values = result.time;
    remainder.values = result.residuals;
    plot.series = {std::move(observed), std::move(trend), std::move(fitted),
                   std::move(remainder)};
    page.plots.push_back(plot);
    return page;
}

OutputPage AnalysisService::doe_factorial(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.doe_response_column.has_value()
        || !configuration.doe_factor_columns.empty()) {
        if (!configuration.doe_response_column.has_value()
            || configuration.doe_factor_columns.empty()) {
            return error_page("DOE 响应分析", "DOE Response Analysis",
                              "请选择响应列和至少一个设计因子列。");
        }
        if (configuration.doe_factor_columns.size()
            >= std::numeric_limits<std::size_t>::digits) {
            return error_page("DOE 响应分析", "DOE Response Analysis",
                              "因子数量过大，无法建立 2 水平模型。");
        }
        if (*configuration.doe_response_column >= table.columns.size()) {
            return error_page("DOE 响应分析", "DOE Response Analysis",
                              "响应列索引超出当前数据表范围。");
        }
        datalab::domain::statistics::DoeFactorialDesign imported_design;
        for (const std::size_t column : configuration.doe_factor_columns) {
            if (column >= table.columns.size()) {
                return error_page("DOE 响应分析", "DOE Response Analysis",
                                  "设计因子列索引超出当前数据表范围。");
            }
            imported_design.factors.push_back({
                column_label(table, column), "-1", "+1"});
        }
        std::set<std::size_t> excluded(
            configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend());
        std::vector<double> responses;
        for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
            if (excluded.count(row_index) != 0) {
                continue;
            }
            const auto& row = table.rows[row_index];
            if (*configuration.doe_response_column >= row.size()) {
                continue;
            }
            std::vector<int> levels;
            bool valid_levels = true;
            for (std::size_t factor = 0;
                 factor < configuration.doe_factor_columns.size(); ++factor) {
                const std::size_t column = configuration.doe_factor_columns[factor];
                const std::string value = column < row.size() ? row[column] : "";
                const auto numeric = parse_numeric_cell(value);
                if (numeric.has_value() && (*numeric == -1.0 || *numeric == 1.0)) {
                    levels.push_back(static_cast<int>(*numeric));
                } else {
                    const std::string low = factor < configuration.doe_low_levels.size()
                        ? configuration.doe_low_levels[factor] : "-1";
                    const std::string high = factor < configuration.doe_high_levels.size()
                        ? configuration.doe_high_levels[factor] : "+1";
                    levels.push_back(value == low ? -1 : value == high ? 1 : 0);
                    if (levels.back() == 0) {
                        valid_levels = false;
                    }
                }
            }
            if (!valid_levels) {
                imported_design.diagnostics.push_back({
                    datalab::domain::DiagnosticMessage::Severity::warning,
                    "missing_doe_run", "存在缺少有效因子水平的运行，已跳过。"});
                continue;
            }
            datalab::domain::statistics::DoeRun run;
            run.standard_order = row_index;
            run.run_order = imported_design.runs.size();
            run.coded_levels = std::move(levels);
            imported_design.runs.push_back(std::move(run));
            responses.push_back(parse_numeric_cell(row[*configuration.doe_response_column])
                                    .value_or(std::numeric_limits<double>::quiet_NaN()));
        }
        const auto fit = datalab::domain::statistics::fit_response_analysis(
            imported_design, responses,
            column_label(table, *configuration.doe_response_column));
        return doe_response_page(table, configuration, imported_design, responses, fit);
    }
    std::vector<datalab::domain::statistics::DoeFactor> factors;
    for (std::size_t index = 0; index < configuration.doe_factor_names.size(); ++index) {
        datalab::domain::statistics::DoeFactor factor;
        factor.name = configuration.doe_factor_names[index];
        factor.low_level = index < configuration.doe_low_levels.size()
            ? configuration.doe_low_levels[index] : "-1";
        factor.high_level = index < configuration.doe_high_levels.size()
            ? configuration.doe_high_levels[index] : "+1";
        factors.push_back(std::move(factor));
    }
    const auto design = datalab::domain::statistics::generate_2_level_factorial({
        factors,
        configuration.doe_center_point_count,
        configuration.doe_block_count,
        configuration.doe_randomize,
        configuration.doe_random_seed});
    return doe_design_page(configuration, factors, design);
}

OutputPage AnalysisService::msa_type1(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.gage_measurement_column.has_value()) {
        return error_page("MSA Type 1", "MSA Type 1",
                          "请选择测量值列，并在配置中提供参考值。");
    }
    const auto measurements = extract_numeric_column(
        table, *configuration.gage_measurement_column, configuration.excluded_rows);
    if (configuration.msa_mode == "stability") {
        const auto result = datalab::domain::statistics::gage_stability(measurements.values);
        OutputPage page;
        page.id = new_id("msa_stability");
        page.title = "Gage Stability";
        page.method_name = "Stability / Gage Run Chart";
        page.configuration = configuration;
        page.parameter_summary = "测量值 = " + measurements.name;
        page.diagnostics = result.diagnostics;
        StatisticTable summary;
        summary.title = "Stability 统计";
        summary.headers = {"Center", "Sigma", "LCL", "UCL", "Out of Control"};
        summary.rows.push_back({format_number(result.center), format_number(result.sigma),
                                format_number(result.lower_control_limit),
                                format_number(result.upper_control_limit),
                                std::to_string(result.out_of_control.size())});
        page.tables.push_back(std::move(summary));
        PlotSpec plot;
        plot.kind = PlotKind::control;
        plot.title = "Gage Stability Run Chart";
        plot.x_axis_title = "观测序号";
        plot.y_axis_title = "测量值";
        plot.values = result.values;
        plot.center.assign(result.values.size(), result.center);
        plot.lower.assign(result.values.size(), result.lower_control_limit);
        plot.upper.assign(result.values.size(), result.upper_control_limit);
        page.plots.push_back(std::move(plot));
        return page;
    }
    if (configuration.msa_mode == "bias_linearity") {
        if (!configuration.msa_reference_column.has_value()) {
            return error_page("Bias/Linearity", "Bias/Linearity", "请选择参考值列。");
        }
        const auto references = extract_numeric_column(
            table, *configuration.msa_reference_column, configuration.excluded_rows);
        const auto result = datalab::domain::statistics::bias_linearity(
            references.values, measurements.values, configuration.confidence_level);
        OutputPage page;
        page.id = new_id("msa_bias_linearity");
        page.title = "Bias/Linearity";
        page.method_name = "MSA Bias and Linearity";
        page.configuration = configuration;
        page.parameter_summary = "测量值 = " + measurements.name
            + "    参考值 = " + references.name;
        page.diagnostics = result.diagnostics;
        StatisticTable summary;
        summary.title = "Bias/Linearity 回归";
        summary.headers = {"Intercept", "Slope", "Slope SE", "Slope CI Low",
                           "Slope CI High", "R-Squared", "Bias Low", "Bias High"};
        summary.rows.push_back({
            format_number(result.intercept), format_number(result.slope),
            format_number(result.slope_standard_error),
            format_number(result.slope_ci_lower), format_number(result.slope_ci_upper),
            format_number(result.r_squared), format_number(result.bias_at_low),
            format_number(result.bias_at_high)});
        page.tables.push_back(std::move(summary));
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "Bias versus Reference";
        plot.x_axis_title = "参考值";
        plot.y_axis_title = "Bias";
        plot.x_values = references.values;
        plot.values.reserve(references.values.size());
        for (std::size_t i = 0; i < references.values.size()
             && i < measurements.values.size(); ++i) {
            plot.values.push_back(measurements.values[i] - references.values[i]);
        }
        page.plots.push_back(std::move(plot));
        return page;
    }
    if (!configuration.msa_reference_value.has_value()) {
        return error_page("MSA Type 1", "MSA Type 1", "请输入参考值。");
    }
    const auto result = datalab::domain::statistics::msa_type1(
        measurements.values, *configuration.msa_reference_value,
        configuration.gage_tolerance, configuration.confidence_level);
    OutputPage page;
    page.id = new_id("msa_type1");
    page.title = "MSA Type 1 Gage";
    page.method_name = "MSA Type 1 Gage + Bias";
    page.configuration = configuration;
    page.parameter_summary = "测量值 = " + measurements.name
        + "    参考值 = " + format_number(*configuration.msa_reference_value);
    page.diagnostics = result.diagnostics;
    StatisticTable table_result;
    table_result.title = "Type 1 Gage 结果";
    table_result.headers = {
        "N", "Mean", "StdDev", "Bias", "SE Bias", "T", "DF", "P",
        "Bias CI Low", "Bias CI High", "Cg", "Cgk", "%Tolerance"};
    table_result.rows.push_back({
        std::to_string(result.count), format_number(result.mean),
        format_number(result.standard_deviation), format_number(result.bias),
        format_number(result.bias_standard_error), format_number(result.t_statistic),
        format_number(result.degrees_of_freedom), format_number(result.p_value),
        format_number(result.bias_ci_lower), format_number(result.bias_ci_upper),
        format_number(result.cg), format_number(result.cgk),
        format_number(result.percent_tolerance)});
    page.tables.push_back(std::move(table_result));
    PlotSpec plot;
    plot.kind = PlotKind::control;
    plot.title = "Gage Run Chart";
    plot.x_axis_title = "测量序号";
    plot.y_axis_title = "测量值";
    plot.center_label = "Reference";
    plot.values = measurements.values;
    plot.center.assign(plot.values.size(), *configuration.msa_reference_value);
    page.plots.push_back(std::move(plot));
    return page;
}

OutputPage AnalysisService::reliability(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.reliability_time_column.has_value()
        || !configuration.reliability_event_column.has_value()) {
        return error_page("Reliability", "Reliability", "请选择寿命列和失效/删失指示列。");
    }
    const auto times = extract_numeric_column(
        table, *configuration.reliability_time_column, configuration.excluded_rows);
    const auto event_text = extract_text_column(
        table, *configuration.reliability_event_column);
    std::vector<double> aligned_times;
    std::vector<bool> events;
    std::vector<int> aligned_groups;
    std::vector<std::string> group_levels;
    const auto group_text = configuration.reliability_group_column.has_value()
        ? extract_text_column(table, *configuration.reliability_group_column)
        : std::vector<std::string>{};
    for (std::size_t index = 0; index < times.source_rows.size(); ++index) {
        const std::size_t row = times.source_rows[index];
        if (row >= event_text.size() || is_missing_cell(event_text[row])) continue;
        const std::string value = event_text[row];
        const bool event = value == "1" || value == "1.0" || value == "true"
            || value == "TRUE" || value == "fail" || value == "Failure";
        if (!group_text.empty()) {
            if (row >= group_text.size()) {
                continue;
            }
            const std::string& label = group_text[row];
            if (is_missing_cell(label)) {
                continue;
            }
            auto level = std::find(group_levels.begin(), group_levels.end(), label);
            if (level == group_levels.end()) {
                group_levels.push_back(label);
                level = group_levels.end() - 1;
            }
            if (group_levels.size() > 2) {
                return error_page("Reliability", "Reliability",
                                  "Log-rank 分组目前只支持两个非空水平。");
            }
            aligned_groups.push_back(
                static_cast<int>(std::distance(group_levels.begin(), level)));
        }
        aligned_times.push_back(times.values[index]);
        events.push_back(event);
    }
    const std::string model = configuration.reliability_model;
    OutputPage page;
    page.id = new_id("reliability");
    page.title = "Reliability Analysis";
    page.method_name = model == "weibull" ? "Weibull Lifetime"
        : model == "exponential" ? "Exponential Lifetime" : "Kaplan-Meier";
    page.configuration = configuration;
    page.parameter_summary = "寿命列 = " + times.name
        + "    模型 = " + model;
    if (model == "weibull") {
        const auto result = datalab::domain::statistics::fit_weibull(aligned_times, events);
        page.diagnostics = result.diagnostics;
        StatisticTable summary;
        summary.title = "Weibull 参数";
        summary.headers = {
            "Shape", "Scale", "B10", "B50", "B90", "LogLik", "AIC", "BIC",
            "Failures", "Observations"};
        summary.rows.push_back({format_number(result.shape), format_number(result.scale),
                                result.b10.has_value() ? format_number(*result.b10) : "*",
                                result.b50.has_value() ? format_number(*result.b50) : "*",
                                result.b90.has_value() ? format_number(*result.b90) : "*",
                                format_number(result.log_likelihood),
                                format_number(result.aic), format_number(result.bic),
                                std::to_string(result.failures),
                                std::to_string(result.observations)});
        page.tables.push_back(std::move(summary));
    } else if (model == "exponential") {
        const auto result = datalab::domain::statistics::fit_exponential(aligned_times, events);
        page.diagnostics = result.diagnostics;
        StatisticTable summary;
        summary.title = "Exponential 参数";
        summary.headers = {
            "Rate", "Mean Life", "B10", "B50", "B90", "LogLik", "AIC", "BIC",
            "Failures", "Observations"};
        summary.rows.push_back({
            format_number(result.rate), format_number(result.mean_life),
            result.b10.has_value() ? format_number(*result.b10) : "*",
            result.b50.has_value() ? format_number(*result.b50) : "*",
            result.b90.has_value() ? format_number(*result.b90) : "*",
            format_number(result.log_likelihood), format_number(result.aic),
            format_number(result.bic), std::to_string(result.failures),
            std::to_string(result.observations)});
        page.tables.push_back(std::move(summary));
    } else {
        const auto result = datalab::domain::statistics::kaplan_meier(aligned_times, events);
        page.diagnostics = result.diagnostics;
        StatisticTable summary;
        summary.title = "Kaplan-Meier 生存表";
        summary.headers = {
            "Time", "At Risk", "Failures", "Censored", "Survival", "SE",
            "CI Lower", "CI Upper"};
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "Kaplan-Meier Survival Curve";
        plot.x_axis_title = "Time";
        plot.y_axis_title = "Survival";
        for (const auto& point : result.points) {
            summary.rows.push_back({format_number(point.time), std::to_string(point.at_risk),
                                    std::to_string(point.failures), std::to_string(point.censored),
                                    format_number(point.survival),
                                    format_number(point.standard_error),
                                    format_number(point.confidence_lower),
                                    format_number(point.confidence_upper)});
            plot.x_values.push_back(point.time);
            plot.values.push_back(point.survival);
            plot.lower.push_back(point.confidence_lower);
            plot.upper.push_back(point.confidence_upper);
        }
        page.tables.push_back(std::move(summary));
        page.plots.push_back(std::move(plot));
        if (aligned_groups.size() == aligned_times.size() && group_levels.size() == 2) {
            const auto log_rank = datalab::domain::statistics::log_rank_test(
                aligned_times, events, aligned_groups);
            page.diagnostics.insert(page.diagnostics.end(),
                                    log_rank.diagnostics.cbegin(),
                                    log_rank.diagnostics.cend());
            StatisticTable comparison;
            comparison.title = "Log-rank 分组比较";
            comparison.headers = {"Chi-Square", "DF", "P-Value",
                                  "Group 1 Failures", "Group 2 Failures"};
            comparison.rows.push_back({
                format_number(log_rank.chi_square),
                format_number(log_rank.degrees_of_freedom),
                format_number(log_rank.p_value),
                std::to_string(log_rank.group_one_failures),
                std::to_string(log_rank.group_two_failures)});
            page.tables.push_back(std::move(comparison));
        }
    }
    return page;
}

OutputPage AnalysisService::t_power(
    const DataTable&,
    const AnalysisConfiguration& configuration)
{
    const bool two_sample = configuration.power_mode.find("two_sample") == 0;
    const bool calculate_power = configuration.power_mode.find("_power") != std::string::npos;
    datalab::domain::statistics::TPowerResult result;
    if (configuration.power_mode.find("anova") == 0) {
        result = configuration.power_mode.find("_power") != std::string::npos
            ? datalab::domain::statistics::one_way_anova_power(
                configuration.power_sample_size, configuration.power_group_count,
                configuration.power_effect_size, configuration.power_alpha)
            : datalab::domain::statistics::one_way_anova_sample_size(
                configuration.power_group_count, configuration.power_effect_size,
                configuration.power_target, configuration.power_alpha);
    } else if (configuration.power_mode.find("one_proportion") == 0) {
        const auto alternative = configuration.alternative == "greater"
            ? datalab::domain::statistics::PowerAlternative::greater
            : configuration.alternative == "less"
                ? datalab::domain::statistics::PowerAlternative::less
                : datalab::domain::statistics::PowerAlternative::two_sided;
        result = calculate_power
            ? datalab::domain::statistics::one_sample_proportion_power(
                configuration.power_sample_size, configuration.power_null_proportion,
                configuration.power_second_proportion, configuration.power_alpha, alternative)
            : datalab::domain::statistics::one_sample_proportion_sample_size(
                configuration.power_null_proportion, configuration.power_second_proportion,
                configuration.power_target, configuration.power_alpha, alternative);
    } else if (configuration.power_mode.find("two_proportion") == 0) {
        const auto alternative = configuration.alternative == "greater"
            ? datalab::domain::statistics::PowerAlternative::greater
            : configuration.alternative == "less"
                ? datalab::domain::statistics::PowerAlternative::less
                : datalab::domain::statistics::PowerAlternative::two_sided;
        const auto variance_method = configuration.power_variance_method == "unpooled"
            ? datalab::domain::statistics::ProportionVarianceMethod::unpooled
            : datalab::domain::statistics::ProportionVarianceMethod::pooled;
        result = calculate_power
            ? datalab::domain::statistics::two_proportion_power(
                configuration.power_sample_size, configuration.power_null_proportion,
                configuration.power_second_proportion, configuration.power_alpha,
                alternative, variance_method)
            : datalab::domain::statistics::two_proportion_sample_size(
                configuration.power_null_proportion, configuration.power_second_proportion,
                configuration.power_target, configuration.power_alpha,
                alternative, variance_method);
    } else if (calculate_power) {
        result = two_sample
            ? datalab::domain::statistics::two_sample_t_power(
                configuration.power_sample_size, configuration.power_effect_size,
                configuration.power_alpha)
            : datalab::domain::statistics::one_sample_t_power(
                configuration.power_sample_size, configuration.power_effect_size,
                configuration.power_alpha);
    } else {
        result = two_sample
            ? datalab::domain::statistics::two_sample_t_sample_size(
                configuration.power_effect_size, configuration.power_target,
                configuration.power_alpha)
            : datalab::domain::statistics::one_sample_t_sample_size(
                configuration.power_effect_size, configuration.power_target,
                configuration.power_alpha);
    }
    OutputPage page;
    page.id = new_id("t_power");
    page.title = "T 功效与样本量";
    page.method_name = calculate_power ? "T Test Power" : "T Test Sample Size";
    page.configuration = configuration;
    page.parameter_summary = "模式 = " + configuration.power_mode;
    page.diagnostics = result.diagnostics;
    StatisticTable summary;
    summary.title = "功效计算结果";
    summary.headers = {
        "Sample Size", "Per Group", "Total N", "Effect Size", "Power",
        "DF", "Noncentrality", "Critical", "Alpha"};
    summary.rows.push_back({
        std::to_string(result.sample_size),
        std::to_string(result.sample_size_per_group),
        std::to_string(result.total_sample_size),
        format_number(result.effect_size), format_number(result.power),
        format_number(result.degrees_of_freedom),
        format_number(result.noncentrality_parameter),
        format_number(result.critical_value),
        format_number(configuration.power_alpha)});
    page.tables.push_back(std::move(summary));
    return page;
}

OutputPage AnalysisService::nested_gage_rr(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.nested_gage_measurement_column.has_value()
        || !configuration.nested_gage_part_column.has_value()
        || !configuration.nested_gage_operator_column.has_value()) {
        return error_page("Nested Gage R&R", "Nested Gage R&R",
                          "请选择测量值、部件和操作者列。");
    }
    const auto measurements = extract_numeric_column(
        table, *configuration.nested_gage_measurement_column, configuration.excluded_rows);
    const auto parts = extract_text_column(table, *configuration.nested_gage_part_column);
    const auto operators = extract_text_column(table, *configuration.nested_gage_operator_column);
    std::vector<std::string> aligned_parts;
    std::vector<std::string> aligned_operators;
    for (const std::size_t source_row : measurements.source_rows) {
        if (source_row < parts.size() && source_row < operators.size()) {
            aligned_parts.push_back(parts[source_row]);
            aligned_operators.push_back(operators[source_row]);
        }
    }
    const auto result = datalab::domain::statistics::nested_gage_rr(
        measurements.values, aligned_parts, aligned_operators, configuration.gage_tolerance);
    OutputPage page;
    page.id = new_id("nested_gage");
    page.title = "Nested Gage R&R";
    page.method_name = "Nested Gage R&R";
    page.configuration = configuration;
    page.parameter_summary = "部件数 = " + std::to_string(result.part_count)
        + "    操作者数 = " + std::to_string(result.operator_count);
    page.diagnostics = result.diagnostics;
    StatisticTable anova;
    anova.title = "Nested Gage R&R ANOVA";
    anova.headers = {"Source", "DF", "SS", "MS", "F"};
    for (const auto& row : result.anova_rows) {
        anova.rows.push_back({row.source, std::to_string(row.degrees_of_freedom),
            format_number(row.sum_of_squares), format_number(row.mean_square),
            format_number(row.f_statistic)});
    }
    page.tables.push_back(std::move(anova));
    StatisticTable components;
    components.title = "Variance Components";
    components.headers = {"Source", "Variance", "Std Dev", "%Contribution",
                          "Study Var", "%Study Var", "%Tolerance"};
    for (const auto& row : result.variance_components) {
        components.rows.push_back({row.source, format_number(row.variance_component),
            format_number(row.standard_deviation), format_number(row.percent_contribution),
            format_number(row.study_variation), format_number(row.percent_study_variation),
            format_number(row.percent_tolerance)});
    }
    page.tables.push_back(std::move(components));
    return page;
}

OutputPage AnalysisService::attribute_agreement(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.attribute_rating_column.has_value()
        || !configuration.attribute_part_column.has_value()
        || !configuration.attribute_appraiser_column.has_value()) {
        return error_page("属性一致性分析", "Attribute Agreement Analysis",
                          "请选择评级、部件和评估者列。");
    }
    const auto ratings = extract_text_column(table, *configuration.attribute_rating_column);
    const auto parts = extract_text_column(table, *configuration.attribute_part_column);
    const auto appraisers = extract_text_column(table, *configuration.attribute_appraiser_column);
    std::vector<std::string> standards;
    if (configuration.attribute_standard_column.has_value()) {
        standards = extract_text_column(table, *configuration.attribute_standard_column);
    }
    const auto result = datalab::domain::statistics::attribute_agreement(
        ratings, parts, appraisers, standards, configuration.confidence_level);
    OutputPage page;
    page.id = new_id("attribute_agreement");
    page.title = "属性一致性分析";
    page.method_name = "Attribute Agreement Analysis";
    page.configuration = configuration;
    page.parameter_summary = "部件数 = " + std::to_string(result.item_count)
        + "    评估者数 = " + std::to_string(result.evaluator_count);
    page.diagnostics = result.diagnostics;
    StatisticTable within;
    within.title = "评估者内一致性";
    within.headers = {"Evaluator", "N", "Agreement %", "Kappa", "95% CI"};
    for (const auto& row : result.within_evaluator) {
        within.rows.push_back({row.evaluator, std::to_string(row.estimate.valid_count),
            format_number(row.estimate.agreement_percent), format_number(row.estimate.kappa),
            "[" + format_number(row.estimate.kappa_ci_low) + ", "
                + format_number(row.estimate.kappa_ci_high) + "]"});
    }
    page.tables.push_back(std::move(within));
    StatisticTable between;
    between.title = "评估者间一致性";
    between.headers = {"Evaluator 1", "Evaluator 2", "N", "Agreement %", "Kappa"};
    for (const auto& row : result.between_evaluator) {
        between.rows.push_back({row.first_evaluator, row.second_evaluator,
            std::to_string(row.estimate.valid_count),
            format_number(row.estimate.agreement_percent),
            format_number(row.estimate.kappa)});
    }
    page.tables.push_back(std::move(between));
    return page;
}

OutputPage AnalysisService::seasonal_forecasting(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::size_t value_column = configuration.decomposition_value_column.value_or(
        configuration.selection.measurement_column);
    const auto extracted = extract_numeric_column(table, value_column, configuration.excluded_rows);
    const auto result = datalab::domain::statistics::fit_seasonal_forecasting(
        extracted.values,
        {configuration.seasonal_error_model == "multiplicative"
             ? datalab::domain::statistics::SeasonalErrorModel::multiplicative
             : datalab::domain::statistics::SeasonalErrorModel::additive,
         configuration.seasonal_trend_model == "none"
             ? datalab::domain::statistics::TrendModel::none
             : configuration.seasonal_trend_model == "multiplicative"
                 ? datalab::domain::statistics::TrendModel::multiplicative
                 : datalab::domain::statistics::TrendModel::additive,
         configuration.seasonal_period, static_cast<std::size_t>(
             std::max(1, configuration.forecast_periods)), configuration.seasonal_damped_trend,
         configuration.smoothing_alpha, configuration.seasonal_beta, configuration.smoothing_gamma,
         configuration.seasonal_damping_phi, configuration.confidence_level});
    OutputPage page;
    page.id = new_id("seasonal_forecast");
    page.title = "季节性预测";
    page.method_name = "Holt-Winters Seasonal Forecasting";
    page.configuration = configuration;
    page.parameter_summary = "周期 = " + std::to_string(configuration.seasonal_period)
        + "    模型 = " + configuration.seasonal_error_model;
    page.diagnostics = result.diagnostics;
    StatisticTable metrics;
    metrics.title = "预测准确度";
    metrics.headers = {"N", "MAD", "MSD", "MAPE", "RMSE", "MASE"};
    metrics.rows.push_back({std::to_string(result.metrics.count),
        format_number(result.metrics.mad), format_number(result.metrics.msd),
        format_number(result.metrics.mape), format_number(result.metrics.rmse),
        format_number(result.metrics.mase)});
    page.tables.push_back(std::move(metrics));
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "季节性拟合与预测";
    plot.x_axis_title = "Order";
    plot.y_axis_title = column_label(table, value_column);
    plot.values = extracted.values;
    for (std::size_t index = 0; index < extracted.values.size(); ++index) {
        plot.x_values.push_back(static_cast<double>(index + 1));
    }
    page.plots.push_back(std::move(plot));
    return page;
}

OutputPage AnalysisService::pca(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::vector<std::size_t>& columns = configuration.pca_variable_columns.empty()
        ? configuration.variable_columns : configuration.pca_variable_columns;
    if (columns.size() < 2) {
        return error_page("PCA 主成分分析", "Principal Component Analysis",
                          "至少需要选择两个数值变量。");
    }
    std::vector<std::vector<double>> rows;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column : columns) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (valid) {
            rows.push_back(std::move(values));
            source_rows.push_back(row);
        }
    }
    const auto result = datalab::domain::statistics::principal_component_analysis(
        rows, {configuration.pca_mode == "standardized"
                   ? datalab::domain::statistics::PcaMode::standardized
                   : datalab::domain::statistics::PcaMode::covariance,
               configuration.pca_component_count, 100, 1.0e-10,
               configuration.pca_anomaly_quantile});
    OutputPage page;
    page.id = new_id("pca");
    page.title = "主成分分析";
    page.method_name = "Principal Component Analysis";
    page.configuration = configuration;
    page.parameter_summary = "观测数 = " + std::to_string(result.observation_count)
        + "    变量数 = " + std::to_string(result.variable_count);
    page.diagnostics = result.diagnostics;
    StatisticTable eigen;
    eigen.title = "特征值与解释率";
    eigen.headers = {"Component", "Eigenvalue", "Explained %", "Cumulative %"};
    for (std::size_t index = 0; index < result.eigenvalues.size(); ++index) {
        eigen.rows.push_back({std::to_string(index + 1), format_number(result.eigenvalues[index]),
            format_number(result.explained_variance_ratio[index] * 100.0),
            format_number(result.cumulative_explained_variance_ratio[index] * 100.0)});
    }
    page.tables.push_back(std::move(eigen));
    StatisticTable loadings;
    loadings.title = "主成分载荷";
    loadings.headers = {"Variable"};
    for (std::size_t index = 0; index < result.retained_component_count; ++index) {
        loadings.headers.push_back("PC" + std::to_string(index + 1));
    }
    for (std::size_t variable = 0; variable < result.loadings.size(); ++variable) {
        std::vector<std::string> row = {column_label(table, columns[variable])};
        for (std::size_t component = 0;
             component < result.loadings[variable].size(); ++component) {
            row.push_back(format_number(result.loadings[variable][component]));
        }
        loadings.rows.push_back(std::move(row));
    }
    page.tables.push_back(std::move(loadings));
    if (result.scores.size() >= 2 && result.retained_component_count >= 2) {
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "PCA 得分图";
        plot.x_axis_title = "PC1";
        plot.y_axis_title = "PC2";
        for (const auto& score : result.scores) {
            plot.x_values.push_back(score[0]);
            plot.values.push_back(score[1]);
        }
        plot.source_rows = source_rows;
        page.plots.push_back(std::move(plot));
    }
    return page;
}

OutputPage AnalysisService::individuals_moving_range(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (extracted.values.size() < 2) {
        return error_page("I-MR 控制图", "I-MR Chart", "I-MR 至少需要两个数值观测。");
    }

    datalab::domain::statistics::IndividualsMovingRangeOptions options;
    options.moving_range_length = std::max(2, configuration.moving_range_length);
    options.method = configuration.sigma_method == "median_moving_range"
        ? datalab::domain::statistics::SigmaEstimateMethod::median_moving_range
        : datalab::domain::statistics::SigmaEstimateMethod::average_moving_range;
    const auto dual = datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(
        extracted.values, options);

    OutputPage page;
    page.id = new_id("imr");
    page.title = "I-MR 控制图";
    page.method_name = "I-MR Chart";
    page.configuration = configuration;
    page.diagnostics = dual.diagnostics;
    page.diagnostics.insert(
        page.diagnostics.end(), dual.primary.diagnostics.begin(), dual.primary.diagnostics.end());
    page.parameter_summary =
        "变量: " + extracted.name
        + "    移动极差长度 = " + std::to_string(options.moving_range_length)
        + "    σ = MR / d2 = " + format_number(dual.sigma);
    StatisticTable table_out;
    table_out.title = "I-MR 参数";
    table_out.headers = {"指标", "数值"};
    table_out.rows = {
        {"N", std::to_string(extracted.values.size())},
        {"N*", std::to_string(extracted.missing_count)},
        {"均值", format_number(dual.primary.center_line.empty() ? 0.0 : dual.primary.center_line.front())},
        {"MR̄", format_number(dual.average_moving_range)},
        {"σ (within)", format_number(dual.sigma)},
        {"Test 1 超限点数", std::to_string(dual.primary.test1_points.size())}};
    page.tables.push_back(table_out);
    page.plots.push_back(control_plot("单值图 (I)", "测量值", dual.primary, extracted.source_rows));
    page.plots.push_back(control_plot("移动极差图 (MR)", "移动极差", dual.secondary, extracted.source_rows));
    return page;
}

OutputPage AnalysisService::xbar_range(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    DualSubgroupChartSpec spec;
    spec.title = "Xbar-R 控制图";
    spec.method_name = "Xbar-R Chart";
    spec.id_prefix = "xbarr";
    spec.sigma_label = "R̄";
    spec.sigma_expression = "R̄ / d2";
    spec.secondary_short = "R";
    spec.secondary_plot_title = "R 图";
    spec.secondary_axis = "子组极差";
    spec.parameter_table_title = "Xbar-R 参数";
    spec.subgroup_table_title = "Xbar-R 逐子组统计";
    spec.compute = [](const std::vector<std::vector<double>>& subgroups) {
        return datalab::domain::statistics::ControlCharts::xbar_range_dual(subgroups);
    };
    spec.validate = [](const std::vector<std::vector<double>>& subgroups) -> std::string {
        return subgroups.front().size() > 8
            ? "Xbar-R 适用于子组大小不超过 8；较大子组请使用 Xbar-S。" : "";
    };
    return subgroup_dual_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::xbar_s(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    DualSubgroupChartSpec spec;
    spec.title = "Xbar-S 控制图";
    spec.method_name = "Xbar-S Chart";
    spec.id_prefix = "xbars";
    spec.sigma_label = "S̄";
    spec.sigma_expression = "S̄ / c4";
    spec.secondary_short = "S";
    spec.secondary_plot_title = "S 图";
    spec.secondary_axis = "子组标准差";
    spec.parameter_table_title = "Xbar-S 参数";
    spec.subgroup_table_title = "Xbar-S 逐子组统计";
    spec.use_config_subgroup_size_in_summary = true;
    spec.compute = [](const std::vector<std::vector<double>>& subgroups) {
        return datalab::domain::statistics::ControlCharts::xbar_s_dual(subgroups);
    };
    return subgroup_dual_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::p_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    AttributeChartSpec spec;
    spec.title = "P 图";
    spec.method_name = "P Chart";
    spec.id_prefix = "pchart";
    spec.count_header = "不合格品数";
    spec.denominator_header = "检验数";
    spec.rate_header = "不合格品率";
    spec.plot_title = "P 图";
    spec.y_axis = "不合格品率";
    spec.parameter_summary = "分布 = 二项分布    p̄ = Σ不合格品数 / Σ检验数    "
        "Test 1 = 超出 3σ 控制限的点";
    spec.assemble = [](const DataTable& table,
                       const AnalysisConfiguration& configuration,
                       std::string& error) -> std::optional<AttributeChartData> {
        const std::size_t defect_column = configuration.selection.defect_count_column.value_or(
            first_variable(configuration));
        const ExtractedNumericColumn defectives =
            extract_numeric_column(table, defect_column, configuration.excluded_rows);
        AttributeChartData data;
        data.source_rows = defectives.source_rows;
        if (configuration.inspected_constant.has_value()) {
            if (!append_nonnegative_counts(defectives.values, data.counts)) {
                error = "不合格品数必须是非负整数。";
                return std::nullopt;
            }
            for (std::size_t index = 0; index < data.counts.size(); ++index) {
                data.denominators.push_back(*configuration.inspected_constant);
            }
        } else if (configuration.selection.inspected_count_column.has_value()) {
            const ExtractedNumericColumn inspected = extract_numeric_column(
                table, *configuration.selection.inspected_count_column, configuration.excluded_rows);
            const std::size_t count = std::min(defectives.values.size(), inspected.values.size());
            if (!append_nonnegative_counts(
                    std::vector<double>(defectives.values.begin(), defectives.values.begin() + count),
                    data.counts)
                || !append_nonnegative_counts(
                    std::vector<double>(inspected.values.begin(), inspected.values.begin() + count),
                    data.denominators)) {
                error = "不合格品数和检验数必须是非负整数。";
                return std::nullopt;
            }
        }
        if (data.counts.empty()) {
            error = "请指定不合格品数列和检验数（常数或列）。";
            return std::nullopt;
        }
        return data;
    };
    spec.compute = [](const std::vector<std::size_t>& defectives,
                      const std::vector<std::size_t>& inspected) {
        return datalab::domain::statistics::ControlCharts::p_chart(defectives, inspected);
    };
    return attribute_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::np_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    AttributeChartSpec spec;
    spec.title = "NP 图";
    spec.method_name = "NP Chart";
    spec.id_prefix = "np";
    spec.count_header = "不合格品数";
    spec.denominator_header = "检验数";
    spec.rate_header = "不合格品数";
    spec.plot_title = "NP 图";
    spec.y_axis = "不合格品数";
    spec.parameter_summary = "分布 = 二项分布    np̄_i = n_i p̄    "
        "Test 1 = 超出 3σ 控制限的点";
    spec.assemble = [](const DataTable& table,
                       const AnalysisConfiguration& configuration,
                       std::string& error) -> std::optional<AttributeChartData> {
        const std::size_t defect_column = configuration.selection.defect_count_column.value_or(
            first_variable(configuration));
        const ExtractedNumericColumn defects =
            extract_numeric_column(table, defect_column, configuration.excluded_rows);
        if (!configuration.inspected_constant.has_value()
            && !configuration.selection.inspected_count_column.has_value()) {
            error = "NP 图需要固定检验数或检验数列。";
            return std::nullopt;
        }
        AttributeChartData data;
        data.source_rows = defects.source_rows;
        if (!append_nonnegative_counts(defects.values, data.counts)) {
            error = "不合格品数必须是非负整数。";
            return std::nullopt;
        }
        for (std::size_t index = 0; index < data.counts.size(); ++index) {
            data.denominators.push_back(configuration.inspected_constant.value_or(1));
        }
        if (!configuration.inspected_constant.has_value()
            && configuration.selection.inspected_count_column.has_value()) {
            const ExtractedNumericColumn inspected = extract_numeric_column(
                table, *configuration.selection.inspected_count_column, configuration.excluded_rows);
            data.denominators.clear();
            if (!append_nonnegative_counts(inspected.values, data.denominators)) {
                error = "检验数必须是非负整数。";
                return std::nullopt;
            }
        }
        return data;
    };
    spec.compute = [](const std::vector<std::size_t>& defectives,
                      const std::vector<std::size_t>& inspected) {
        return datalab::domain::statistics::ControlCharts::np_chart(defectives, inspected);
    };
    return attribute_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::c_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    AttributeChartSpec spec;
    spec.title = "C 图";
    spec.method_name = "C Chart";
    spec.id_prefix = "cchart";
    spec.count_header = "缺陷数";
    spec.denominator_header = "单位数";
    spec.rate_header = "缺陷数";
    spec.plot_title = "C 图";
    spec.y_axis = "缺陷数";
    spec.parameter_summary = "分布 = 泊松分布    c̄ = 缺陷数均值    "
        "C 图要求每个子组单位数相同    Test 1 = 超出 3σ 控制限的点";
    spec.assemble = [](const DataTable& table,
                       const AnalysisConfiguration& configuration,
                       std::string& error) -> std::optional<AttributeChartData> {
        const ExtractedNumericColumn defects = extract_numeric_column(
            table, configuration.selection.defect_count_column.value_or(first_variable(configuration)),
            configuration.excluded_rows);
        AttributeChartData data;
        data.source_rows = defects.source_rows;
        if (!append_nonnegative_counts(defects.values, data.counts)) {
            error = "缺陷数必须是非负整数。";
            return std::nullopt;
        }
        data.denominators.assign(
            data.counts.size(), configuration.inspected_constant.value_or(1));
        return data;
    };
    spec.compute = [](const std::vector<std::size_t>& counts,
                      const std::vector<std::size_t>& denominators) {
        return datalab::domain::statistics::ControlCharts::c_chart(
            counts, denominators.empty() ? 1 : denominators.front());
    };
    return attribute_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::u_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    AttributeChartSpec spec;
    spec.title = "U 图";
    spec.method_name = "U Chart";
    spec.id_prefix = "uchart";
    spec.count_header = "缺陷数";
    spec.denominator_header = "单位数";
    spec.rate_header = "单位缺陷数";
    spec.plot_title = "U 图";
    spec.y_axis = "单位缺陷数";
    spec.parameter_summary = "分布 = 泊松分布    ū = Σ缺陷数 / Σ单位数    "
        "Test 1 = 超出 3σ 控制限的点";
    spec.assemble = [](const DataTable& table,
                       const AnalysisConfiguration& configuration,
                       std::string& error) -> std::optional<AttributeChartData> {
        const ExtractedNumericColumn defects = extract_numeric_column(
            table, configuration.selection.defect_count_column.value_or(first_variable(configuration)),
            configuration.excluded_rows);
        if (!configuration.selection.inspected_count_column.has_value()) {
            error = "U 图需要单位数列。";
            return std::nullopt;
        }
        const ExtractedNumericColumn units = extract_numeric_column(
            table, *configuration.selection.inspected_count_column, configuration.excluded_rows);
        AttributeChartData data;
        data.source_rows = defects.source_rows;
        const std::size_t count = std::min(defects.values.size(), units.values.size());
        if (!append_nonnegative_counts(
                std::vector<double>(defects.values.begin(), defects.values.begin() + count),
                data.counts)
            || !append_nonnegative_counts(
                std::vector<double>(units.values.begin(), units.values.begin() + count),
                data.denominators)) {
            error = "缺陷数和单位数必须是非负整数。";
            return std::nullopt;
        }
        return data;
    };
    spec.compute = [](const std::vector<std::size_t>& defects,
                      const std::vector<std::size_t>& units) {
        return datalab::domain::statistics::ControlCharts::u_chart(defects, units);
    };
    return attribute_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::laney_p_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    LaneyChartSpec spec;
    spec.title = "Laney P' 图";
    spec.method_name = "Laney P' Chart";
    spec.id_prefix = "laneyp";
    spec.distribution_text = "二项分布";
    spec.center_label = "p̄";
    spec.count_header = "不合格品数";
    spec.denominator_header = "检验数";
    spec.y_axis = "不合格品率";
    spec.include_enabled_tests_row = true;
    spec.assemble = [](const DataTable& table,
                       const AnalysisConfiguration& effective,
                       std::string& error) -> std::optional<AttributeChartData> {
        const std::size_t defect_column = effective.selection.defect_count_column.value_or(
            first_variable(effective));
        const ExtractedNumericColumn defectives =
            extract_numeric_column(table, defect_column, effective.excluded_rows);
        AttributeChartData data;
        data.source_rows = defectives.source_rows;
        if (!append_nonnegative_counts(defectives.values, data.counts)) {
            error = "不合格品数必须是非负整数。";
            return std::nullopt;
        }
        if (effective.inspected_constant.has_value()) {
            data.denominators.assign(data.counts.size(), *effective.inspected_constant);
        } else if (effective.selection.inspected_count_column.has_value()) {
            const ExtractedNumericColumn inspected = extract_numeric_column(
                table, *effective.selection.inspected_count_column, effective.excluded_rows);
            if (!append_nonnegative_counts(inspected.values, data.denominators)
                || data.denominators.size() != data.counts.size()) {
                error = "检验数必须是有效的非负整数列。";
                return std::nullopt;
            }
        } else {
            error = "请指定检验数列或检验数常数。";
            return std::nullopt;
        }
        return data;
    };
    spec.compute = [](const std::vector<std::size_t>& defectives,
                      const std::vector<std::size_t>& inspected,
                      const datalab::domain::statistics::LaneyChartOptions& options) {
        return datalab::domain::statistics::ControlCharts::laney_p_chart(
            defectives, inspected, options);
    };
    return laney_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::laney_u_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    LaneyChartSpec spec;
    spec.title = "Laney U' 图";
    spec.method_name = "Laney U' Chart";
    spec.id_prefix = "laneyu";
    spec.distribution_text = "泊松分布";
    spec.center_label = "ū";
    spec.count_header = "缺陷数";
    spec.denominator_header = "单位数";
    spec.y_axis = "单位缺陷数";
    spec.assemble = [spec](const DataTable& table,
                           const AnalysisConfiguration& effective,
                           std::string& error) -> std::optional<AttributeChartData> {
        const std::size_t defect_column = effective.selection.defect_count_column.value_or(
            first_variable(effective));
        const ExtractedNumericColumn defects =
            extract_numeric_column(table, defect_column, effective.excluded_rows);
        if (!effective.selection.inspected_count_column.has_value()) {
            error = spec.title + "需要单位数列。";
            return std::nullopt;
        }
        const ExtractedNumericColumn units = extract_numeric_column(
            table, *effective.selection.inspected_count_column, effective.excluded_rows);
        AttributeChartData data;
        data.source_rows = defects.source_rows;
        if (!append_nonnegative_counts(defects.values, data.counts)
            || !append_nonnegative_counts(units.values, data.denominators)
            || data.counts.size() != data.denominators.size()) {
            error = "缺陷数和单位数必须是有效的非负整数列。";
            return std::nullopt;
        }
        return data;
    };
    spec.compute = [](const std::vector<std::size_t>& defects,
                      const std::vector<std::size_t>& units,
                      const datalab::domain::statistics::LaneyChartOptions& options) {
        return datalab::domain::statistics::ControlCharts::laney_u_chart(
            defects, units, options);
    };
    return laney_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::capability(
    const DataTable& table,
    const AnalysisConfiguration& configuration,
    std::vector<double>* capability_indices)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (extracted.values.size() < 2) {
        return error_page("正态过程能力", "Normal Capability Analysis", "过程能力至少需要两个数值观测。");
    }
    if (!configuration.specifications.lower.has_value()
        && !configuration.specifications.upper.has_value()) {
        return error_page("正态过程能力", "Normal Capability Analysis", "请输入 LSL 或 USL。");
    }

    const int subgroup_size = configuration.subgroup_size.value_or(1);
    double within_sigma = 0.0;
    std::string within_method = "样本标准差";
    if (subgroup_size <= 1) {
        datalab::domain::statistics::IndividualsMovingRangeOptions options;
        options.moving_range_length = std::max(2, configuration.moving_range_length);
        options.method = configuration.sigma_method == "median_moving_range"
            ? datalab::domain::statistics::SigmaEstimateMethod::median_moving_range
            : datalab::domain::statistics::SigmaEstimateMethod::average_moving_range;
        const auto dual = datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(
            extracted.values, options);
        within_sigma = dual.sigma;
        within_method = "平均移动极差 / d2";
    } else {
        const auto subgroups = datalab::domain::statistics::build_subgroups(
            extracted.values, subgroup_size);
        const auto dual = datalab::domain::statistics::ControlCharts::xbar_range_dual(subgroups);
        within_sigma = dual.sigma;
        within_method = "R̄ / d2";
    }

    const auto capability_result = datalab::domain::statistics::ProcessCapability::calculate(
        extracted.values, within_sigma, configuration.specifications);
    OutputPage page = build_capability_content(
        configuration, extracted, capability_result, subgroup_size, within_method);
    if (capability_indices != nullptr) {
        if (capability_result.cp.has_value()) {
            capability_indices->push_back(*capability_result.cp);
        }
        if (capability_result.cpk.has_value()) {
            capability_indices->push_back(*capability_result.cpk);
        }
        if (capability_result.pp.has_value()) {
            capability_indices->push_back(*capability_result.pp);
        }
        if (capability_result.ppk.has_value()) {
            capability_indices->push_back(*capability_result.ppk);
        }
    }

    if (subgroup_size <= 1) {
        datalab::domain::statistics::IndividualsMovingRangeOptions options;
        options.moving_range_length = std::max(2, configuration.moving_range_length);
        const auto dual = datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(
            extracted.values, options);
        page.plots.push_back(control_plot("I 图", "测量值", dual.primary, extracted.source_rows));
    }
    return page;
}

OutputPage AnalysisService::capability_sixpack(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (extracted.values.size() < 2) {
        return error_page("过程能力 Sixpack", "Capability Sixpack", "Sixpack 至少需要两个数值观测。");
    }
    std::vector<double> capability_indices;
    OutputPage capability_page = capability(table, configuration, &capability_indices);
    if (capability_page.tables.empty() && capability_page.plots.empty()) {
        capability_page.title = "过程能力 Sixpack";
        capability_page.method_name = "Capability Sixpack";
        return capability_page;
    }

    const int subgroup_size = configuration.subgroup_size.value_or(1);
    std::vector<PlotSpec> primary_plots;
    datalab::domain::statistics::ControlChartResult primary;
    datalab::domain::statistics::ControlChartResult secondary;
    std::vector<std::size_t> subgroup_rows;
    if (subgroup_size > 1) {
        const auto subgroups = datalab::domain::statistics::build_subgroups(
            extracted.values, subgroup_size);
        const auto xbar_r =
            datalab::domain::statistics::ControlCharts::xbar_range_dual(subgroups);
        for (std::size_t index = 0; index < subgroups.size(); ++index) {
            const std::size_t source = index * subgroup_size;
            subgroup_rows.push_back(source < extracted.source_rows.size()
                                        ? extracted.source_rows[source] : source);
        }
        primary = xbar_r.primary;
        secondary = xbar_r.secondary;
        primary_plots.push_back(control_plot("Xbar 控制图", "子组均值", primary, subgroup_rows));
    } else {
        datalab::domain::statistics::IndividualsMovingRangeOptions options;
        options.moving_range_length = std::max(2, configuration.moving_range_length);
        const auto imr =
            datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(
                extracted.values, options);
        primary = imr.primary;
        secondary = imr.secondary;
        primary_plots.push_back(control_plot("I 控制图", "测量值", primary, extracted.source_rows));
    }
    primary_plots.push_back(control_plot(
        subgroup_size > 1 ? "R 控制图" : "MR 控制图",
        subgroup_size > 1 ? "子组极差" : "移动极差",
        secondary,
        subgroup_size > 1 ? subgroup_rows : extracted.source_rows));
    const auto probability =
        datalab::domain::statistics::normal_probability_plot(extracted.values);

    capability_page.id = new_id("sixpack");
    capability_page.title = "过程能力 Sixpack";
    capability_page.method_name = "Capability Sixpack";
    capability_page.configuration = configuration;
    PlotSpec capability_histogram;
    for (const auto& plot : capability_page.plots) {
        if (plot.kind == PlotKind::histogram) {
            capability_histogram = plot;
            break;
        }
    }
    capability_page.plots.clear();
    if (!primary_plots.empty()) {
        capability_page.plots.push_back(std::move(primary_plots[0]));
    }
    capability_page.plots.push_back(capability_histogram);
    if (primary_plots.size() > 1) {
        capability_page.plots.push_back(std::move(primary_plots[1]));
    }
    capability_page.plots.push_back(probability_plot_spec(probability, extracted.name));
    capability_page.plots.push_back(last_points_plot(extracted, primary, subgroup_size));
    PlotSpec capability_plot;
    capability_plot.kind = PlotKind::control;
    capability_plot.title = "能力图";
    capability_plot.x_axis_title = "指标";
    capability_plot.y_axis_title = "能力指数";
    for (std::size_t index = 0; index < capability_indices.size(); ++index) {
        capability_plot.values.push_back(capability_indices[index]);
        capability_plot.x_values.push_back(static_cast<double>(index));
    }
    capability_page.plots.push_back(capability_plot);
    capability_page.parameter_summary +=
        "    正态概率图相关系数 = " + format_number(probability.correlation);
    return capability_page;
}

OutputPage AnalysisService::histogram(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (extracted.values.empty()) {
        return error_page("直方图", "Histogram", "所选列没有数值观测。");
    }
    const auto bins = datalab::domain::statistics::histogram(extracted.values, 0);
    OutputPage page;
    page.id = new_id("hist");
    page.title = "直方图";
    page.method_name = "Histogram";
    page.configuration = configuration;
    page.parameter_summary = "变量: " + extracted.name + "    N = " + std::to_string(extracted.values.size());
    PlotSpec hist;
    hist.kind = PlotKind::histogram;
    hist.title = extracted.name;
    hist.x_axis_title = extracted.name;
    hist.y_axis_title = "频数";
    hist.histogram_edges = bins.edges;
    hist.histogram_counts = bins.counts;
    hist.values = extracted.values;
    hist.source_rows = extracted.source_rows;
    page.plots.push_back(hist);
    return page;
}

OutputPage AnalysisService::boxplot(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (extracted.values.empty()) {
        return error_page("箱线图", "Boxplot", "所选列没有数值观测。");
    }
    OutputPage page;
    page.id = new_id("box");
    page.title = "箱线图";
    page.method_name = "Boxplot";
    page.configuration = configuration;
    PlotSpec plot;
    plot.kind = PlotKind::boxplot;
    plot.title = extracted.name;
    plot.y_axis_title = extracted.name;
    plot.x_axis_title = "分组";
    plot.source_rows = extracted.source_rows;

    if (configuration.by_column.has_value()) {
        const std::vector<std::string> groups = extract_text_column(table, *configuration.by_column);
        std::map<std::string, std::vector<double>> grouped;
        for (std::size_t index = 0; index < extracted.values.size(); ++index) {
            const std::size_t row = extracted.source_rows[index];
            grouped[row < groups.size() ? groups[row] : "*"].push_back(extracted.values[index]);
        }
        for (const auto& [label, values] : grouped) {
            const auto summary = datalab::domain::statistics::box_plot_summary(values);
            plot.box_labels.push_back(label);
            plot.box_min.push_back(summary.minimum);
            plot.box_q1.push_back(summary.first_quartile);
            plot.box_median.push_back(summary.median);
            plot.box_q3.push_back(summary.third_quartile);
            plot.box_max.push_back(summary.maximum);
        }
    } else {
        const auto summary = datalab::domain::statistics::box_plot_summary(extracted.values);
        plot.box_labels.push_back(extracted.name);
        plot.box_min.push_back(summary.minimum);
        plot.box_q1.push_back(summary.first_quartile);
        plot.box_median.push_back(summary.median);
        plot.box_q3.push_back(summary.third_quartile);
        plot.box_max.push_back(summary.maximum);
    }
    page.plots.push_back(plot);
    page.parameter_summary = "变量: " + extracted.name;
    return page;
}

OutputPage AnalysisService::pareto(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::size_t column = first_variable(configuration);
    std::map<std::string, std::size_t> counts;
    std::vector<DiagnosticMessage> diagnostics;
    if (configuration.selection.defect_count_column.has_value()) {
        const ExtractedNumericColumn counts_col = extract_numeric_column(
            table, *configuration.selection.defect_count_column, configuration.excluded_rows);
        const std::vector<std::string> names = extract_text_column(table, column);
        for (std::size_t index = 0; index < counts_col.values.size(); ++index) {
            const std::size_t row = counts_col.source_rows[index];
            const std::string name = row < names.size() ? names[row] : "?";
            if (is_missing_cell(name)) {
                diagnostics.push_back({
                    DiagnosticMessage::Severity::warning,
                    "missing_pareto_category",
                    "已忽略类别为空或 * 的汇总行。"});
                continue;
            }
            const double count = counts_col.values[index];
            if (count < 0.0 || std::floor(count) != count) {
                diagnostics.push_back({
                    DiagnosticMessage::Severity::warning,
                    "invalid_pareto_count",
                    "已忽略负数或非整数缺陷计数。"});
                continue;
            }
            counts[name] += static_cast<std::size_t>(count);
        }
    } else {
        for (std::size_t row = 0; row < table.rows.size(); ++row) {
            if (std::find(configuration.excluded_rows.begin(), configuration.excluded_rows.end(), row)
                != configuration.excluded_rows.end()) {
                continue;
            }
            if (column >= table.rows[row].size() || is_missing_cell(table.rows[row][column])) {
                continue;
            }
            ++counts[table.rows[row][column]];
        }
    }
    std::vector<std::pair<std::string, std::size_t>> pairs(counts.begin(), counts.end());
    datalab::domain::statistics::ParetoOptions options;
    options.other_threshold_percent = configuration.pareto_other_threshold_percent;
    const auto items = datalab::domain::statistics::pareto(pairs, options);
    if (items.empty()) {
        return error_page("柏拉图", "Pareto Chart", "没有可用于柏拉图的类别数据。");
    }
    OutputPage page;
    page.id = new_id("pareto");
    page.title = "柏拉图";
    page.method_name = "Pareto Chart";
    page.configuration = configuration;
    page.diagnostics = diagnostics;
    page.parameter_summary = "类别列: " + column_label(table, column)
        + "    总计数 = " + std::to_string(
            std::accumulate(pairs.begin(), pairs.end(), std::size_t{0},
                            [](std::size_t total, const auto& item) {
                                return total + item.second;
                            }));
    if (configuration.pareto_other_threshold_percent.has_value()) {
        page.parameter_summary += "    Other 阈值 = "
            + format_number(*configuration.pareto_other_threshold_percent, 4) + "%";
    }
    StatisticTable table_out;
    table_out.title = "缺陷计数";
    table_out.headers = {"类别", "计数", "Percent", "Cum %"};
    PlotSpec plot;
    plot.kind = PlotKind::pareto;
    plot.title = column < table.columns.size()
        ? table.columns[column] + " 的 Pareto 图"
        : "C" + std::to_string(column + 1) + " 的 Pareto 图";
    plot.x_axis_title = "类别";
    plot.y_axis_title = "计数";
    for (const auto& item : items) {
        table_out.rows.push_back({
            item.category,
            std::to_string(item.count),
            format_number(item.percent, 4),
            format_number(item.cumulative_percent, 4)});
        plot.categories.push_back(item.category);
        plot.category_values.push_back(static_cast<double>(item.count));
        plot.cumulative_percent.push_back(item.cumulative_percent);
    }
    page.tables.push_back(table_out);
    page.plots.push_back(plot);
    return page;
}

}  // namespace datalab::application
