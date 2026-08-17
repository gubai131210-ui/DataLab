#include "application/chart_pages.h"

#include "application/column_assembly.h"
#include "application/output_builder.h"
#include "domain/column_extract.h"

#include <algorithm>
#include <numeric>

namespace datalab::application {

namespace {

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::ExtractedNumericColumn;
using datalab::domain::OutputPage;
using datalab::domain::PlotSpec;
using datalab::domain::StatisticTable;
using datalab::domain::extract_numeric_column;
using datalab::domain::extract_text_column;
using datalab::domain::is_missing_cell;

}  // namespace

OutputPage subgroup_dual_chart_page(
    const DataTable& table,
    const AnalysisConfiguration& configuration,
    const DualSubgroupChartSpec& spec)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    std::string subgroup_error;
    const auto input = build_strict_subgroups(table, extracted, configuration, subgroup_error);
    if (!input.has_value()) {
        return error_page(spec.title, spec.method_name, subgroup_error);
    }
    const std::vector<std::vector<double>>& subgroups = input->values;
    if (spec.validate && !spec.validate(subgroups).empty()) {
        return error_page(spec.title, spec.method_name, spec.validate(subgroups));
    }
    const auto dual = spec.compute(subgroups);
    OutputPage page;
    page.id = new_id(spec.id_prefix);
    page.title = spec.title;
    page.method_name = spec.method_name;
    page.configuration = configuration;
    page.diagnostics = dual.diagnostics;
    page.parameter_summary = "变量: " + extracted.name
        + (spec.use_config_subgroup_size_in_summary
            ? "    子组大小 = " + std::to_string(configuration.subgroup_size.value_or(5))
            : "    子组数 = " + std::to_string(subgroups.size()))
        + "    σ = " + spec.sigma_expression + " = " + format_number(dual.sigma);
    StatisticTable table_out;
    table_out.title = spec.parameter_table_title;
    table_out.headers = {"指标", "数值"};
    table_out.rows = {
        {"子组数", std::to_string(subgroups.size())},
        {"X̄", format_number(dual.primary.center_line.empty() ? 0.0 : dual.primary.center_line.front())},
        {spec.sigma_label, format_number(dual.average_moving_range)},
        {"σ (within)", format_number(dual.sigma)},
        {"Xbar Test 1 超限点数", std::to_string(dual.primary.test1_points.size())},
        {spec.secondary_short + " Test 1 超限点数", std::to_string(dual.secondary.test1_points.size())}};
    page.tables.push_back(table_out);
    std::vector<std::size_t> subgroup_rows;
    StatisticTable subgroup_table;
    subgroup_table.title = spec.subgroup_table_title;
    subgroup_table.headers = {"子组", "N", "Xbar", spec.secondary_short, "Xbar CL", "Xbar LCL",
                              "Xbar UCL", spec.secondary_short + " CL",
                              spec.secondary_short + " LCL", spec.secondary_short + " UCL", "Test 1"};
    for (std::size_t index = 0; index < subgroups.size(); ++index) {
        subgroup_rows.push_back(input->source_rows[index].front());
        const bool xbar_failed = std::find(
            dual.primary.test1_points.cbegin(), dual.primary.test1_points.cend(), index)
            != dual.primary.test1_points.cend();
        const bool secondary_failed = std::find(
            dual.secondary.test1_points.cbegin(), dual.secondary.test1_points.cend(), index)
            != dual.secondary.test1_points.cend();
        subgroup_table.rows.push_back({
            input->labels[index], std::to_string(subgroups[index].size()),
            format_number(dual.primary.plotted_values[index]),
            format_number(dual.secondary.plotted_values[index]),
            format_number(dual.primary.center_line[index]),
            format_number(dual.primary.lower_control_limit[index]),
            format_number(dual.primary.upper_control_limit[index]),
            format_number(dual.secondary.center_line[index]),
            format_number(dual.secondary.lower_control_limit[index]),
            format_number(dual.secondary.upper_control_limit[index]),
            (xbar_failed || secondary_failed) ? "是" : ""});
    }
    page.tables.push_back(subgroup_table);
    page.plots.push_back(control_plot("Xbar 图", "子组均值", dual.primary, subgroup_rows));
    page.plots.push_back(control_plot(
        spec.secondary_plot_title, spec.secondary_axis, dual.secondary, subgroup_rows));
    return page;
}

OutputPage attribute_chart_page(
    const DataTable& table,
    const AnalysisConfiguration& configuration,
    const AttributeChartSpec& spec)
{
    std::string error;
    const auto data = spec.assemble(table, configuration, error);
    if (!data.has_value()) {
        return error_page(spec.title, spec.method_name, error);
    }
    const auto chart = spec.compute(data->counts, data->denominators);
    if (chart.plotted_values.empty()) {
        return error_page(spec.title, spec.method_name, chart.diagnostics.empty()
            ? spec.title + "没有可显示的数据。" : chart.diagnostics.front().message);
    }
    OutputPage page;
    page.id = new_id(spec.id_prefix);
    page.title = spec.title;
    page.method_name = spec.method_name;
    page.configuration = configuration;
    page.diagnostics = chart.diagnostics;
    page.parameter_summary = spec.parameter_summary;
    page.tables.push_back(attribute_chart_table(
        spec.title + "逐子组统计", data->counts, data->denominators, chart,
        spec.count_header, spec.denominator_header, spec.rate_header));
    page.plots.push_back(control_plot(spec.plot_title, spec.y_axis, chart, data->source_rows));
    return page;
}

OutputPage laney_chart_page(
    const DataTable& table,
    const AnalysisConfiguration& configuration,
    const LaneyChartSpec& spec)
{
    AnalysisConfiguration effective = configuration;
    if (!effective.included_rows.empty()) {
        effective.excluded_rows.clear();
        for (std::size_t row = 0; row < table.rows.size(); ++row) {
            if (std::find(effective.included_rows.cbegin(), effective.included_rows.cend(), row)
                == effective.included_rows.cend()) {
                effective.excluded_rows.push_back(row);
            }
        }
    }
    std::string error;
    const auto data = spec.assemble(table, effective, error);
    if (!data.has_value()) {
        return error_page(spec.title, spec.method_name, error);
    }
    datalab::domain::statistics::LaneyChartOptions options;
    options.enabled_special_cause_tests = effective.enabled_special_cause_tests;
    options.historical_center = effective.historical_center;
    options.historical_sigma_z = effective.historical_sigma_z;
    const auto chart = spec.compute(data->counts, data->denominators, options);
    if (chart.plotted_values.empty()) {
        return error_page(spec.title, spec.method_name, chart.diagnostics.empty()
            ? spec.title + "没有可显示的数据。" : chart.diagnostics.front().message);
    }
    std::vector<std::string> stages;
    if (effective.stage_column.has_value()) {
        const std::vector<std::string> stage_values = extract_text_column(
            table, *effective.stage_column);
        for (const std::size_t row : data->source_rows) {
            const std::string stage = row < stage_values.size() ? stage_values[row] : "";
            if (is_missing_cell(stage)) {
                return error_page(spec.title, spec.method_name,
                                  "阶段列存在缺失标签，请补齐原始数据。");
            }
            stages.push_back(stage);
        }
    }
    OutputPage page;
    page.id = new_id(spec.id_prefix);
    page.title = spec.title;
    page.method_name = spec.method_name;
    page.configuration = effective;
    page.diagnostics = chart.diagnostics;
    page.parameter_summary = "分布 = " + spec.distribution_text + "    "
        + spec.center_label + " = " + format_number(chart.center_line.front())
        + "    Sigma Z = " + format_number(chart.sigma_z)
        + (effective.historical_sigma_z.has_value() ? "（历史参数）" : "（估计）");
    StatisticTable parameters;
    parameters.title = spec.title + " 参数";
    parameters.headers = {"指标", "数值"};
    parameters.rows = {
        {spec.center_label, format_number(chart.center_line.front())},
        {"Sigma Z", format_number(chart.sigma_z)},
        {"MR̄(Z)", format_number(chart.moving_ranges.size() > 1
            ? std::accumulate(chart.moving_ranges.cbegin() + 1, chart.moving_ranges.cend(), 0.0)
                / static_cast<double>(chart.moving_ranges.size() - 1) : 0.0)},
        {"有效子组数", std::to_string(chart.plotted_values.size())}};
    if (spec.include_enabled_tests_row) {
        parameters.rows.push_back({"启用测试", effective.enabled_special_cause_tests.empty()
            ? "无" : "Test " + std::to_string(effective.enabled_special_cause_tests.front())});
    }
    page.tables.push_back(parameters);
    page.tables.push_back(laney_chart_table(
        data->counts, data->denominators, chart, stages,
        spec.count_header, spec.denominator_header));
    PlotSpec plot = control_plot(spec.title, spec.y_axis, chart, data->source_rows);
    plot.subtitle = "Sigma Z = " + format_number(chart.sigma_z);
    page.plots.push_back(plot);
    return page;
}

}  // namespace datalab::application
