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
    auto dual = spec.compute(
        subgroups,
        datalab::domain::statistics::special_cause_selection_from_configuration(
            configuration.control.enabled_special_cause_tests,
            configuration.control.special_cause_rule_policy));
    std::vector<std::string> stages;
    if (configuration.control.stage_column.has_value()) {
        const std::vector<std::string> stage_values = extract_text_column(
            table, *configuration.control.stage_column);
        for (const auto& rows : input->source_rows) {
            const std::size_t row = rows.front();
            const std::string stage = row < stage_values.size() ? stage_values[row] : "";
            if (is_missing_cell(stage)) {
                return error_page(spec.title, spec.method_name,
                                  "阶段列存在缺失标签，请补齐原始数据。");
            }
            stages.push_back(stage);
        }
        dual.primary.phase_labels = stages;
        dual.secondary.phase_labels = stages;
        const auto special_causes =
            datalab::domain::statistics::special_cause_selection_from_configuration(
                configuration.control.enabled_special_cause_tests,
                configuration.control.special_cause_rule_policy);
        const auto secondary_kind = spec.secondary_short == "S"
            ? datalab::domain::statistics::ControlChartKind::stdev
            : datalab::domain::statistics::ControlChartKind::range;
        datalab::domain::statistics::apply_special_cause_tests(
            dual.primary, datalab::domain::statistics::ControlChartKind::xbar,
            special_causes);
        datalab::domain::statistics::apply_special_cause_tests(
            dual.secondary, secondary_kind, special_causes);
    }
    OutputPage page;
    page.id = new_id(spec.id_prefix);
    page.title = spec.title;
    page.method_name = spec.method_name;
    page.configuration = configuration;
    page.diagnostics = dual.diagnostics;
    page.diagnostics.insert(
        page.diagnostics.end(), dual.primary.diagnostics.begin(), dual.primary.diagnostics.end());
    page.diagnostics.insert(
        page.diagnostics.end(), dual.secondary.diagnostics.begin(), dual.secondary.diagnostics.end());
    page.parameter_summary = "变量: " + extracted.name
        + (spec.use_config_subgroup_size_in_summary
            ? "    子组大小 = " + std::to_string(configuration.control.subgroup_size.value_or(5))
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
        {spec.secondary_short + " Test 1 超限点数", std::to_string(dual.secondary.test1_points.size())},
        {"规则策略", configuration.control.special_cause_rule_policy == "explicit"
            ? "用户指定" : "默认全选适用规则"},
        {"Xbar 启用测试", datalab::domain::statistics::format_special_cause_tests(
            datalab::domain::statistics::resolve_special_cause_tests(
                datalab::domain::statistics::special_cause_selection_from_configuration(
                    configuration.control.enabled_special_cause_tests,
                    configuration.control.special_cause_rule_policy),
                datalab::domain::statistics::ControlChartKind::xbar))},
        {spec.secondary_short + " 适用规则", "Test 1–4"}};
    page.tables.push_back(table_out);
    std::vector<std::size_t> subgroup_rows;
    for (std::size_t index = 0; index < subgroups.size(); ++index) {
        subgroup_rows.push_back(input->source_rows[index].front());
    }
    page.tables.push_back(subgroup_dual_point_table(
        dual.primary, dual.secondary, subgroups, subgroup_rows, input->labels, stages,
        spec.subgroup_table_title, spec.secondary_short));
    dual.primary.source_rows = subgroup_rows;
    dual.secondary.source_rows = subgroup_rows;
    page.plots.push_back(control_plot("Xbar 图", "子组均值", dual.primary, subgroup_rows));
    page.plots.push_back(control_plot(
        spec.secondary_plot_title, spec.secondary_axis, dual.secondary, subgroup_rows));
    std::size_t out_of_control_union = 0;
    for (std::size_t index = 0; index < subgroups.size(); ++index) {
        const bool xbar_failed = std::find(
            dual.primary.test1_points.cbegin(), dual.primary.test1_points.cend(), index)
            != dual.primary.test1_points.cend();
        const bool secondary_failed = std::find(
            dual.secondary.test1_points.cbegin(), dual.secondary.test1_points.cend(), index)
            != dual.secondary.test1_points.cend();
        if (xbar_failed || secondary_failed) {
            ++out_of_control_union;
        }
    }
    page.facts.spc = datalab::domain::SpcFacts{};
    page.facts.spc->sigma_within = dual.sigma;
    page.facts.spc->out_of_control_count = out_of_control_union;
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
    const auto special_causes =
        datalab::domain::statistics::special_cause_selection_from_configuration(
            configuration.control.enabled_special_cause_tests,
            configuration.control.special_cause_rule_policy);
    std::vector<std::string> stages;
    if (configuration.control.stage_column.has_value()) {
        const std::vector<std::string> stage_values = extract_text_column(
            table, *configuration.control.stage_column);
        for (const std::size_t row : data->source_rows) {
            const std::string stage = row < stage_values.size() ? stage_values[row] : "";
            if (is_missing_cell(stage)) {
                return error_page(spec.title, spec.method_name,
                                  "阶段列存在缺失标签，请补齐原始数据。");
            }
            stages.push_back(stage);
        }
    }
    auto chart = spec.compute(data->counts, data->denominators, special_causes);
    chart.source_rows = data->source_rows;
    if (!stages.empty()) {
        chart.phase_labels = stages;
        datalab::domain::statistics::apply_special_cause_tests(
            chart, datalab::domain::statistics::ControlChartKind::attribute, special_causes);
    }
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
    StatisticTable parameters;
    parameters.title = spec.title + " 方法与参数";
    parameters.headers = {"指标", "数值"};
    const auto enabled = datalab::domain::statistics::resolve_special_cause_tests(
        special_causes,
        datalab::domain::statistics::ControlChartKind::attribute);
    parameters.rows = {
        {"有效子组数", std::to_string(chart.plotted_values.size())},
        {"规则策略", configuration.control.special_cause_rule_policy == "explicit"
            ? "用户指定" : "默认全选适用规则"},
        {"启用测试", datalab::domain::statistics::format_special_cause_tests(enabled)},
        {"Test 1 超限点数", std::to_string(chart.test1_points.size())},
        {"判定口径", "Tests 1–8；超过 kσ 使用严格大于，窗口不跨阶段或缺失断点"}};
    page.tables.push_back(parameters);
    page.tables.push_back(attribute_chart_table(
        spec.title + "逐子组统计", data->counts, data->denominators, chart,
        spec.count_header, spec.denominator_header, spec.rate_header, stages));
    page.plots.push_back(control_plot(spec.plot_title, spec.y_axis, chart, data->source_rows));
    page.facts.spc = datalab::domain::SpcFacts{};
    page.facts.spc->out_of_control_count = chart.test1_points.size();
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
    std::vector<std::string> stages;
    if (effective.control.stage_column.has_value()) {
        const std::vector<std::string> stage_values = extract_text_column(
            table, *effective.control.stage_column);
        for (const std::size_t row : data->source_rows) {
            const std::string stage = row < stage_values.size() ? stage_values[row] : "";
            if (is_missing_cell(stage)) {
                return error_page(spec.title, spec.method_name,
                                  "阶段列存在缺失标签，请补齐原始数据。");
            }
            stages.push_back(stage);
        }
    }
    datalab::domain::statistics::LaneyChartOptions options;
    options.enabled_special_cause_tests = effective.control.enabled_special_cause_tests;
    options.special_cause_rule_policy = effective.control.special_cause_rule_policy;
    options.historical_center = effective.control.historical_center;
    options.historical_sigma_z = effective.control.historical_sigma_z;
    options.phase_labels = stages;
    auto chart = spec.compute(data->counts, data->denominators, options);
    chart.source_rows = data->source_rows;
    if (!stages.empty()) {
        chart.phase_labels = stages;
        datalab::domain::statistics::apply_special_cause_tests(
            chart, datalab::domain::statistics::ControlChartKind::laney,
            datalab::domain::statistics::special_cause_selection_from_configuration(
                effective.control.enabled_special_cause_tests,
                effective.control.special_cause_rule_policy));
    }
    if (chart.plotted_values.empty()) {
        return error_page(spec.title, spec.method_name, chart.diagnostics.empty()
            ? spec.title + "没有可显示的数据。" : chart.diagnostics.front().message);
    }
    OutputPage page;
    page.id = new_id(spec.id_prefix);
    page.title = spec.title;
    page.method_name = spec.method_name;
    page.configuration = effective;
    page.diagnostics = chart.diagnostics;
    const bool historical_center = effective.control.historical_center.has_value();
    const bool historical_sigma = effective.control.historical_sigma_z.has_value();
    page.parameter_summary = "分布 = " + spec.distribution_text + "    "
        + spec.center_label + " = " + format_number(chart.center_line.front())
        + (historical_center ? "（历史参数）" : "（估计）")
        + "    Sigma Z = " + format_number(chart.sigma_z)
        + (historical_sigma ? "（历史参数）" : "（估计）");
    StatisticTable parameters;
    parameters.title = spec.title + " 参数";
    parameters.headers = {"指标", "数值"};
    parameters.rows = {
        {spec.center_label, format_number(chart.center_line.front())
            + (historical_center ? "（历史参数）" : "")},
        {"Sigma Z", format_number(chart.sigma_z)
            + (historical_sigma ? "（历史参数）" : "")},
        {"MR̄(Z)", format_number(chart.moving_ranges.size() > 1
            ? std::accumulate(chart.moving_ranges.cbegin() + 1, chart.moving_ranges.cend(), 0.0)
                / static_cast<double>(chart.moving_ranges.size() - 1) : 0.0)},
        {"有效子组数", std::to_string(chart.plotted_values.size())},
        {"Test 1 超限点数", std::to_string(chart.test1_points.size())}};
    if (spec.include_enabled_tests_row) {
        const auto enabled = datalab::domain::statistics::resolve_special_cause_tests(
            datalab::domain::statistics::special_cause_selection_from_configuration(
                effective.control.enabled_special_cause_tests,
                effective.control.special_cause_rule_policy),
            datalab::domain::statistics::ControlChartKind::laney);
        parameters.rows.push_back({"规则策略",
            effective.control.special_cause_rule_policy == "explicit"
                ? "用户指定" : "默认全选适用规则"});
        parameters.rows.push_back({"启用测试",
            datalab::domain::statistics::format_special_cause_tests(enabled)});
    }
    page.tables.push_back(parameters);
    page.tables.push_back(laney_chart_table(
        data->counts, data->denominators, chart, stages,
        spec.count_header, spec.denominator_header));
    PlotSpec plot = control_plot(spec.title, spec.y_axis, chart, data->source_rows);
    plot.subtitle = "Sigma Z = " + format_number(chart.sigma_z);
    plot.sigma_z = chart.sigma_z;
    page.plots.push_back(plot);
    page.facts.spc = datalab::domain::SpcFacts{};
    page.facts.spc->sigma_z = chart.sigma_z;
    page.facts.spc->out_of_control_count = chart.test1_points.size();
    return page;
}

}  // namespace datalab::application
