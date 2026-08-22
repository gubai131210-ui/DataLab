#include "application/output_builder.h"

#include "domain/statistics/special_cause_rule_catalog.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <string>

namespace datalab::application {

using datalab::domain::StatisticTable;

namespace {

std::string triggered_tests_text(
    const datalab::domain::statistics::ControlChartResult& chart,
    std::size_t index)
{
    if (index >= chart.triggered_tests.size()) {
        return datalab::domain::statistics::format_triggered_special_cause_rules({});
    }
    return datalab::domain::statistics::format_triggered_special_cause_rules(
        chart.triggered_tests[index]);
}

std::string primary_test_text(int primary_test)
{
    return datalab::domain::statistics::format_primary_special_cause_rule(primary_test);
}

std::string merged_dual_triggered_tests(
    const datalab::domain::statistics::ControlChartResult& primary,
    const datalab::domain::statistics::ControlChartResult& secondary,
    std::size_t index,
    const std::string& secondary_short)
{
    const std::string xbar_tests = triggered_tests_text(primary, index);
    const std::string secondary_tests = triggered_tests_text(secondary, index);
    const bool xbar_active =
        xbar_tests != datalab::domain::statistics::format_triggered_special_cause_rules({});
    const bool secondary_active =
        secondary_tests
        != datalab::domain::statistics::format_triggered_special_cause_rules({});
    if (!xbar_active && !secondary_active) {
        return "未触发";
    }
    std::ostringstream stream;
    if (xbar_active) {
        stream << "Xbar: " << xbar_tests;
    }
    if (secondary_active) {
        if (xbar_active) {
            stream << "; ";
        }
        stream << secondary_short << ": " << secondary_tests;
    }
    return stream.str();
}

std::string merged_dual_minimum_test(
    const datalab::domain::statistics::ControlChartResult& primary,
    const datalab::domain::statistics::ControlChartResult& secondary,
    std::size_t index,
    const std::string& secondary_short)
{
    const int xbar_test = index < primary.primary_test_by_point.size()
        ? primary.primary_test_by_point[index] : 0;
    const int secondary_test = index < secondary.primary_test_by_point.size()
        ? secondary.primary_test_by_point[index] : 0;
    if (xbar_test > 0 && secondary_test > 0) {
        const int minimum = std::min(xbar_test, secondary_test);
        return datalab::domain::statistics::format_primary_special_cause_rule(minimum);
    }
    if (xbar_test > 0) {
        return datalab::domain::statistics::format_primary_special_cause_rule(xbar_test);
    }
    if (secondary_test > 0) {
        return secondary_short + ": "
            + datalab::domain::statistics::format_primary_special_cause_rule(secondary_test);
    }
    const bool xbar_failed = std::find(
        primary.test1_points.cbegin(), primary.test1_points.cend(), index)
        != primary.test1_points.cend();
    const bool secondary_failed = std::find(
        secondary.test1_points.cbegin(), secondary.test1_points.cend(), index)
        != secondary.test1_points.cend();
    if (xbar_failed || secondary_failed) {
        const std::string name =
            datalab::domain::statistics::special_cause_rule_display_name(1);
        if (xbar_failed && secondary_failed) {
            return name;
        }
        if (xbar_failed) {
            return name;
        }
        return secondary_short + ": " + name;
    }
    return "未触发";
}

std::string cusum_signal_label(
    std::size_t index,
    const std::vector<std::size_t>& upper_signal_points,
    const std::vector<std::size_t>& lower_signal_points)
{
    const bool upper = std::find(
        upper_signal_points.cbegin(), upper_signal_points.cend(), index)
        != upper_signal_points.cend();
    const bool lower = std::find(
        lower_signal_points.cbegin(), lower_signal_points.cend(), index)
        != lower_signal_points.cend();
    if (upper && lower) {
        return "上/下";
    }
    if (upper) {
        return "上侧";
    }
    if (lower) {
        return "下侧";
    }
    return "无";
}

}  // namespace

std::string format_number(double value, int digits)
{
    if (!std::isfinite(value)) {
        return "*";
    }
    std::ostringstream stream;
    stream << std::setprecision(digits) << value;
    return stream.str();
}

std::optional<double> parse_numeric_cell(const std::string& text)
{
    if (text.empty() || text == "*" || text == "NA" || text == "NaN") {
        return std::nullopt;
    }
    try {
        std::size_t consumed = 0;
        const double value = std::stod(text, &consumed);
        if (consumed != text.size() || !std::isfinite(value)) {
            return std::nullopt;
        }
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

std::string format_optional(const std::optional<double>& value, int digits)
{
    return value.has_value() ? format_number(*value, digits) : "*";
}

std::string new_id(const std::string& prefix)
{
    static int counter = 0;
    ++counter;
    return prefix + "_" + std::to_string(counter);
}

domain::OutputPage error_page(
    const std::string& title,
    const std::string& method,
    const std::string& message)
{
    domain::OutputPage page;
    page.id = new_id("err");
    page.title = title;
    page.method_name = method;
    page.diagnostics.push_back({domain::DiagnosticMessage::Severity::error, "analysis", message});
    return page;
}

void append_diagnostics(
    std::vector<domain::DiagnosticMessage>& target,
    const std::vector<domain::DiagnosticMessage>& source,
    const std::string& prefix)
{
    for (const auto& diagnostic : source) {
        target.push_back({
            diagnostic.severity,
            diagnostic.code,
            prefix + diagnostic.message});
    }
}

void append_rule_table(
    domain::OutputPage& page,
    const std::vector<domain::RuleEvidence>& rules)
{
    if (rules.empty()) {
        return;
    }
    const bool special_cause = std::any_of(
        rules.begin(), rules.end(),
        [](const domain::RuleEvidence& rule) {
            return !rule.window.empty() || rule.id.find('_') != std::string::npos;
        });
    if (special_cause) {
        page.tables.push_back(
            datalab::domain::statistics::special_cause_rule_evidence_table(rules));
        return;
    }
    domain::StatisticTable table;
    table.title = "规则证据";
    table.headers = {"规则", "名称", "状态", "证据", "关联行", "建议"};
    for (const auto& rule : rules) {
        std::string rows;
        for (std::size_t index = 0; index < rule.related_rows.size(); ++index) {
            if (index != 0) {
                rows += ",";
            }
            rows += std::to_string(rule.related_rows[index] + 1);
        }
        table.rows.push_back({
            rule.id,
            rule.name.empty() ? rule.id : rule.name,
            rule.status,
            rule.message,
            rows,
            rule.suggested_action});
    }
    page.tables.push_back(std::move(table));
}

domain::statistics::TestAlternative parse_alternative(const std::string& value)
{
    if (value == "less") {
        return domain::statistics::TestAlternative::less;
    }
    if (value == "greater") {
        return domain::statistics::TestAlternative::greater;
    }
    return domain::statistics::TestAlternative::two_sided;
}

domain::statistics::VarianceMethod parse_variance_method(const std::string& value)
{
    return value == "pooled"
        ? domain::statistics::VarianceMethod::pooled
        : domain::statistics::VarianceMethod::welch;
}

std::string alternative_label(const std::string& value)
{
    if (value == "less") {
        return "小于";
    }
    if (value == "greater") {
        return "大于";
    }
    return "不等于";
}

bool append_nonnegative_counts(
    const std::vector<double>& values,
    std::vector<std::size_t>& counts)
{
    for (const double value : values) {
        if (!std::isfinite(value) || value < 0.0
            || value != std::floor(value)
            || value > static_cast<double>(std::numeric_limits<std::size_t>::max())) {
            return false;
        }
        counts.push_back(static_cast<std::size_t>(value));
    }
    return true;
}

StatisticTable t_test_table(
    const std::string& title,
    const domain::statistics::TTestResult& result,
    const std::string& variable)
{
    StatisticTable table;
    table.title = title;
    table.headers = {"变量", "N", "Mean", "StDev", "SE Mean", "假设均值",
                     "差值", "T", "DF", "P-Value", "置信区间"};
    const std::string interval = result.confidence_lower.has_value()
        && result.confidence_upper.has_value()
        ? "[" + format_number(*result.confidence_lower) + ", "
            + format_number(*result.confidence_upper) + "]"
        : result.confidence_lower.has_value()
            ? "[" + format_number(*result.confidence_lower) + ", +∞)"
            : result.confidence_upper.has_value()
                ? "(-∞, " + format_number(*result.confidence_upper) + "]" : "*";
    table.rows.push_back({
        variable,
        std::to_string(result.count),
        format_number(result.mean),
        format_number(result.sample_standard_deviation),
        format_number(result.standard_error),
        format_number(result.hypothesized_mean),
        format_number(result.difference),
        format_number(result.t_statistic),
        format_number(result.degrees_of_freedom),
        result.p_value.has_value() ? format_number(*result.p_value) : "*",
        interval});
    return table;
}

StatisticTable descriptive_table(
    const std::vector<domain::statistics::DescriptiveStatisticsResult>& rows)
{
    StatisticTable table;
    table.title = "描述统计";
    table.headers = {
        "变量", "N", "N*", "Mean", "SE Mean", "StDev", "Variance",
        "Minimum", "Q1", "Median", "Q3", "IQR", "Maximum", "Range", "Sum",
        "Skewness", "Excess Kurtosis"};
    for (const auto& row : rows) {
        table.rows.push_back({
            row.group_label,
            std::to_string(row.count),
            std::to_string(row.missing_count),
            format_number(row.mean),
            format_optional(row.standard_error_of_mean),
            format_optional(row.sample_standard_deviation),
            format_number(row.variance),
            format_number(row.minimum),
            format_number(row.first_quartile),
            format_number(row.median),
            format_number(row.third_quartile),
            format_number(row.interquartile_range),
            format_number(row.maximum),
            format_number(row.range),
            format_number(row.sum),
            format_optional(row.skewness),
            format_optional(row.excess_kurtosis)});
    }
    return table;
}

StatisticTable attribute_chart_table(
    const std::string& title,
    const std::vector<std::size_t>& counts,
    const std::vector<std::size_t>& denominators,
    const domain::statistics::ControlChartResult& chart,
    const std::string& count_header,
    const std::string& denominator_header,
    const std::string& rate_header,
    const std::vector<std::string>& stages)
{
    StatisticTable table;
    table.title = title;
    table.headers = {
        "原始行", "子组", "阶段", count_header, denominator_header, rate_header, "中心线", "LCL",
        "UCL", "触发规则", "主要规则"};
    for (std::size_t index = 0; index < chart.plotted_values.size(); ++index) {
        const std::size_t count = index < counts.size() ? counts[index] : 0;
        const std::size_t denominator = index < denominators.size() ? denominators[index] : 0;
        const double center = index < chart.center_line.size() ? chart.center_line[index] : 0.0;
        const double lower = index < chart.lower_control_limit.size()
            ? chart.lower_control_limit[index] : 0.0;
        const double upper = index < chart.upper_control_limit.size()
            ? chart.upper_control_limit[index] : 0.0;
        const std::string tests = triggered_tests_text(chart, index);
        const int primary_test = index < chart.primary_test_by_point.size()
            ? chart.primary_test_by_point[index] : 0;
        table.rows.push_back({
            index < chart.source_rows.size()
                ? std::to_string(chart.source_rows[index] + 1) : "*",
            std::to_string(index + 1),
            index < stages.size() ? stages[index] : "",
            std::to_string(count),
            std::to_string(denominator),
            format_number(chart.plotted_values[index]),
            format_number(center),
            format_number(lower),
            format_number(upper),
            tests,
            primary_test_text(primary_test)});
    }
    return table;
}

StatisticTable laney_chart_table(
    const std::vector<std::size_t>& counts,
    const std::vector<std::size_t>& denominators,
    const domain::statistics::ControlChartResult& chart,
    const std::vector<std::string>& stages,
    const std::string& count_header,
    const std::string& denominator_header)
{
    StatisticTable table;
    table.title = "Laney 图逐子组统计";
    table.headers = {"原始行", "子组", "阶段", count_header, denominator_header, "绘制值",
                    "Z", "MR", "中心线", "LCL", "UCL"};
    for (const auto& rule : datalab::domain::statistics::special_cause_rule_catalog()) {
        table.headers.push_back(rule.display_name);
    }
    table.headers.push_back("触发规则");
    table.headers.push_back("主要规则");
    for (std::size_t index = 0; index < chart.plotted_values.size(); ++index) {
        const auto test_failed = [&](std::size_t test) {
            return test < chart.special_cause_points.size()
                && std::find(chart.special_cause_points[test].cbegin(),
                             chart.special_cause_points[test].cend(), index)
                    != chart.special_cause_points[test].cend();
        };
        table.rows.push_back({
            index < chart.source_rows.size()
                ? std::to_string(chart.source_rows[index] + 1) : "*",
            std::to_string(index + 1),
            index < stages.size() ? stages[index] : "",
            index < counts.size() ? std::to_string(counts[index]) : "*",
            index < denominators.size() ? std::to_string(denominators[index]) : "*",
            format_number(chart.plotted_values[index]),
            index < chart.standardized_values.size()
                ? format_number(chart.standardized_values[index]) : "*",
            index < chart.moving_ranges.size() ? format_number(chart.moving_ranges[index]) : "*",
            index < chart.center_line.size() ? format_number(chart.center_line[index]) : "*",
            index < chart.lower_control_limit.size()
                ? format_number(chart.lower_control_limit[index]) : "*",
            index < chart.upper_control_limit.size()
                ? format_number(chart.upper_control_limit[index]) : "*",
            test_failed(0) ? "是" : "",
            test_failed(1) ? "是" : "",
            test_failed(2) ? "是" : "",
            test_failed(3) ? "是" : "",
            test_failed(4) ? "是" : "",
            test_failed(5) ? "是" : "",
            test_failed(6) ? "是" : "",
            test_failed(7) ? "是" : "",
            triggered_tests_text(chart, index),
            primary_test_text(index < chart.primary_test_by_point.size() ? chart.primary_test_by_point[index] : 0)});
    }
    return table;
}

StatisticTable individuals_point_table(
    const domain::statistics::ControlChartResult& individuals,
    const domain::statistics::ControlChartResult& moving_range,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& stages)
{
    StatisticTable table;
    table.title = "I-MR 逐点统计";
    table.headers = {"原始行", "阶段", "观测值", "I CL", "I LCL", "I UCL", "MR",
                     "触发规则", "主要规则"};
    for (std::size_t index = 0; index < individuals.plotted_values.size(); ++index) {
        table.rows.push_back({
            index < source_rows.size()
                ? std::to_string(source_rows[index] + 1) : "*",
            index < stages.size() ? stages[index] : "",
            format_number(individuals.plotted_values[index]),
            index < individuals.center_line.size()
                ? format_number(individuals.center_line[index]) : "*",
            index < individuals.lower_control_limit.size()
                ? format_number(individuals.lower_control_limit[index]) : "*",
            index < individuals.upper_control_limit.size()
                ? format_number(individuals.upper_control_limit[index]) : "*",
            index < moving_range.plotted_values.size()
                ? format_number(moving_range.plotted_values[index]) : "*",
            triggered_tests_text(individuals, index),
            primary_test_text(index < individuals.primary_test_by_point.size() ? individuals.primary_test_by_point[index] : 0)});
    }
    return table;
}

StatisticTable subgroup_dual_point_table(
    const domain::statistics::ControlChartResult& primary,
    const domain::statistics::ControlChartResult& secondary,
    const std::vector<std::vector<double>>& subgroups,
    const std::vector<std::size_t>& subgroup_source_rows,
    const std::vector<std::string>& labels,
    const std::vector<std::string>& stages,
    const std::string& title,
    const std::string& secondary_short)
{
    StatisticTable table;
    table.title = title;
    table.headers = {"原始行", "子组", "阶段", "N", "Xbar", secondary_short,
                     "Xbar CL", "Xbar LCL", "Xbar UCL",
                     secondary_short + " CL",
                     secondary_short + " LCL",
                     secondary_short + " UCL", "触发规则", "主要规则"};
    for (std::size_t index = 0; index < subgroups.size(); ++index) {
        table.rows.push_back({
            index < subgroup_source_rows.size()
                ? std::to_string(subgroup_source_rows[index] + 1) : "*",
            index < labels.size() ? labels[index] : std::to_string(index + 1),
            index < stages.size() ? stages[index] : "",
            std::to_string(subgroups[index].size()),
            index < primary.plotted_values.size()
                ? format_number(primary.plotted_values[index]) : "*",
            index < secondary.plotted_values.size()
                ? format_number(secondary.plotted_values[index]) : "*",
            index < primary.center_line.size()
                ? format_number(primary.center_line[index]) : "*",
            index < primary.lower_control_limit.size()
                ? format_number(primary.lower_control_limit[index]) : "*",
            index < primary.upper_control_limit.size()
                ? format_number(primary.upper_control_limit[index]) : "*",
            index < secondary.center_line.size()
                ? format_number(secondary.center_line[index]) : "*",
            index < secondary.lower_control_limit.size()
                ? format_number(secondary.lower_control_limit[index]) : "*",
            index < secondary.upper_control_limit.size()
                ? format_number(secondary.upper_control_limit[index]) : "*",
            merged_dual_triggered_tests(primary, secondary, index, secondary_short),
            merged_dual_minimum_test(primary, secondary, index, secondary_short)});
    }
    return table;
}

StatisticTable ewma_point_table(
    const domain::statistics::ControlChartResult& chart,
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows)
{
    StatisticTable table;
    table.title = "EWMA 逐点统计";
    table.headers = {"原始行", "观测值", "EWMA", "σ", "CL", "LCL", "UCL",
                     "触发规则", "主要规则"};
    for (std::size_t index = 0; index < chart.plotted_values.size(); ++index) {
        table.rows.push_back({
            index < source_rows.size()
                ? std::to_string(source_rows[index] + 1) : "*",
            index < observations.size()
                ? format_number(observations[index]) : "*",
            format_number(chart.plotted_values[index]),
            index < chart.point_sigma.size()
                ? format_number(chart.point_sigma[index]) : "*",
            index < chart.center_line.size()
                ? format_number(chart.center_line[index]) : "*",
            index < chart.lower_control_limit.size()
                ? format_number(chart.lower_control_limit[index]) : "*",
            index < chart.upper_control_limit.size()
                ? format_number(chart.upper_control_limit[index]) : "*",
            triggered_tests_text(chart, index),
            primary_test_text(index < chart.primary_test_by_point.size() ? chart.primary_test_by_point[index] : 0)});
    }
    return table;
}

StatisticTable rare_event_point_table(
    const std::string& title,
    const domain::statistics::ControlChartResult& chart,
    const std::vector<std::size_t>& source_rows)
{
    StatisticTable table;
    table.title = title;
    table.headers = {"原始行", "间隔", "CL", "LCL", "UCL", "触发规则", "主要规则"};
    for (std::size_t index = 0; index < chart.plotted_values.size(); ++index) {
        table.rows.push_back({
            index < source_rows.size()
                ? std::to_string(source_rows[index] + 1) : "*",
            format_number(chart.plotted_values[index]),
            index < chart.center_line.size()
                ? format_number(chart.center_line[index]) : "*",
            index < chart.lower_control_limit.size()
                && std::isfinite(chart.lower_control_limit[index])
                ? format_number(chart.lower_control_limit[index]) : "*",
            index < chart.upper_control_limit.size()
                && std::isfinite(chart.upper_control_limit[index])
                ? format_number(chart.upper_control_limit[index]) : "*",
            triggered_tests_text(chart, index),
            primary_test_text(index < chart.primary_test_by_point.size() ? chart.primary_test_by_point[index] : 0)});
    }
    return table;
}

StatisticTable cusum_point_table(
    const domain::statistics::TimeWeightedControlChartResult& chart,
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows)
{
    StatisticTable table;
    table.title = "CUSUM 逐点统计";
    table.headers = {"原始行", "观测值", "C+", "C−", "信号"};
    const std::size_t count = chart.primary.plotted_values.size();
    for (std::size_t index = 0; index < count; ++index) {
        table.rows.push_back({
            index < source_rows.size()
                ? std::to_string(source_rows[index] + 1) : "*",
            index < observations.size()
                ? format_number(observations[index]) : "*",
            format_number(chart.primary.plotted_values[index]),
            index < chart.secondary.plotted_values.size()
                ? format_number(chart.secondary.plotted_values[index]) : "*",
            cusum_signal_label(
                index, chart.upper_signal_points, chart.lower_signal_points)});
    }
    return table;
}

StatisticTable cusum_signal_table(
    const domain::statistics::TimeWeightedControlChartResult& chart,
    const std::vector<std::size_t>& source_rows)
{
    StatisticTable table;
    table.title = "CUSUM 信号";
    table.headers = {"方向", "原始行", "观测序号", "累计值"};
    auto append_signal = [&](const char* direction,
                             const std::vector<std::size_t>& signal_points,
                             const domain::statistics::ControlChartResult& side) {
        for (const std::size_t index : signal_points) {
            table.rows.push_back({
                direction,
                index < source_rows.size()
                    ? std::to_string(source_rows[index] + 1) : "*",
                std::to_string(index + 1),
                index < side.plotted_values.size()
                    ? format_number(side.plotted_values[index]) : "*"});
        }
    };
    append_signal("上侧", chart.upper_signal_points, chart.primary);
    append_signal("下侧", chart.lower_signal_points, chart.secondary);
    if (table.rows.empty()) {
        table.rows.push_back({"无", "*", "*", "*"});
    }
    return table;
}

StatisticTable zone_point_table(
    const domain::statistics::ZoneChartResult& chart,
    const std::vector<std::size_t>& source_rows)
{
    StatisticTable table;
    table.title = "区域图逐点统计";
    table.headers = {"原始行", "观测值", "Z", "区域带", "累计得分", "Jaehn 信号"};
    const auto band_label = [](int band) -> std::string {
        switch (band) {
        case 3:
            return ">3σ";
        case 2:
            return "2–3σ";
        case 1:
            return "1–2σ";
        default:
            return "≤1σ";
        }
    };
    std::set<std::size_t> signals(chart.signal_points.cbegin(), chart.signal_points.cend());
    for (std::size_t index = 0; index < chart.individuals.plotted_values.size(); ++index) {
        const double z = chart.sigma > 0.0
            ? (chart.individuals.plotted_values[index] - chart.center) / chart.sigma
            : 0.0;
        table.rows.push_back({
            index < source_rows.size()
                ? std::to_string(source_rows[index] + 1) : "*",
            format_number(chart.individuals.plotted_values[index]),
            format_number(z),
            index < chart.zone_band.size() ? band_label(chart.zone_band[index]) : "*",
            index < chart.zone_scores.size()
                ? format_number(chart.zone_scores[index]) : "*",
            signals.count(index) != 0 ? "是" : ""});
    }
    return table;
}

StatisticTable zmr_point_table(
    const domain::statistics::ZmrChartResult& chart,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& group_labels)
{
    StatisticTable table;
    table.title = "Z-MR 逐点统计";
    table.headers = {"原始行", "组", "Z", "Z CL", "Z LCL", "Z UCL",
                     "MR(Z)", "MR CL", "MR UCL", "触发规则", "主要规则"};
    for (std::size_t index = 0; index < chart.z_values.size(); ++index) {
        table.rows.push_back({
            index < source_rows.size()
                ? std::to_string(source_rows[index] + 1) : "*",
            index < group_labels.size() ? group_labels[index] : "",
            format_number(chart.z_values[index]),
            index < chart.z_chart.center_line.size()
                ? format_number(chart.z_chart.center_line[index]) : "*",
            index < chart.z_chart.lower_control_limit.size()
                ? format_number(chart.z_chart.lower_control_limit[index]) : "*",
            index < chart.z_chart.upper_control_limit.size()
                ? format_number(chart.z_chart.upper_control_limit[index]) : "*",
            index < chart.mr_chart.plotted_values.size()
                ? format_number(chart.mr_chart.plotted_values[index]) : "*",
            index < chart.mr_chart.center_line.size()
                ? format_number(chart.mr_chart.center_line[index]) : "*",
            index < chart.mr_chart.upper_control_limit.size()
                ? format_number(chart.mr_chart.upper_control_limit[index]) : "*",
            triggered_tests_text(chart.z_chart, index),
            primary_test_text(index < chart.z_chart.primary_test_by_point.size() ? chart.z_chart.primary_test_by_point[index] : 0)});
    }
    return table;
}

StatisticTable moving_average_point_table(
    const domain::statistics::ControlChartResult& chart,
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows)
{
    StatisticTable table;
    table.title = "移动平均逐点统计";
    table.headers = {"原始行", "观测值", "MA", "σ/√w", "CL", "LCL", "UCL",
                     "触发规则", "主要规则"};
    for (std::size_t index = 0; index < chart.plotted_values.size(); ++index) {
        if (!std::isfinite(chart.plotted_values[index])) {
            continue;
        }
        table.rows.push_back({
            index < source_rows.size()
                ? std::to_string(source_rows[index] + 1) : "*",
            index < observations.size()
                ? format_number(observations[index]) : "*",
            format_number(chart.plotted_values[index]),
            index < chart.point_sigma.size()
                ? format_number(chart.point_sigma[index]) : "*",
            index < chart.center_line.size()
                ? format_number(chart.center_line[index]) : "*",
            index < chart.lower_control_limit.size()
                ? format_number(chart.lower_control_limit[index]) : "*",
            index < chart.upper_control_limit.size()
                ? format_number(chart.upper_control_limit[index]) : "*",
            triggered_tests_text(chart, index),
            primary_test_text(index < chart.primary_test_by_point.size() ? chart.primary_test_by_point[index] : 0)});
    }
    return table;
}

domain::PlotSpec control_plot(
    const std::string& title,
    const std::string& y_axis,
    const domain::statistics::ControlChartResult& chart,
    const std::vector<std::size_t>& source_rows)
{
    domain::PlotSpec plot;
    plot.kind = domain::PlotKind::control;
    plot.title = title;
    plot.x_axis_title = "观测序号";
    plot.y_axis_title = y_axis;
    if (y_axis == "移动极差" || y_axis == "子组极差") {
        plot.center_label = "R̄";
    } else if (y_axis == "子组标准差") {
        plot.center_label = "S̄";
    } else if (y_axis == "不合格品率") {
        plot.center_label = "p̄";
    } else if (y_axis == "不合格品数") {
        plot.center_label = "np̄";
    } else if (y_axis == "缺陷数") {
        plot.center_label = "c̄";
    } else if (y_axis == "单位缺陷数") {
        plot.center_label = "ū";
    } else {
        plot.center_label = "X̄";
    }
    plot.values = chart.plotted_values;
    plot.center = chart.center_line;
    plot.lower = chart.lower_control_limit;
    plot.upper = chart.upper_control_limit;
    plot.source_rows = source_rows;
    plot.special_cause_points = chart.special_cause_points;
    plot.triggered_tests = chart.triggered_tests;
    plot.primary_test_by_point = chart.primary_test_by_point;
    plot.signal_direction = chart.signal_direction;
    plot.sigma_z = chart.sigma_z;
    if (!chart.phase_labels.empty()) {
        plot.point_groups = chart.phase_labels;
        plot.point_labels = chart.phase_labels;
    }
    return plot;
}

void attach_special_cause_rules(
    domain::SpcFacts& spc,
    const domain::statistics::ControlChartResult& chart,
    domain::statistics::ControlChartKind kind,
    const domain::statistics::SpecialCauseSelection& selection)
{
    const auto enabled =
        datalab::domain::statistics::resolve_special_cause_tests(selection, kind);
    spc.enabled_special_cause_tests = enabled;
    spc.enabled_special_cause_rule_ids.clear();
    for (const int number : enabled) {
        if (const auto* spec =
                datalab::domain::statistics::find_special_cause_rule_by_number(number)) {
            spc.enabled_special_cause_rule_ids.push_back(spec->rule_id);
        }
    }
    if (spc.rule_policy.empty()) {
        spc.rule_policy = selection.policy;
    }
    spc.rules = datalab::domain::statistics::build_special_cause_rule_evidences(
        chart, kind, selection);
}

void append_special_cause_rule_table(
    domain::OutputPage& page,
    const domain::SpcFacts& spc)
{
    if (spc.rules.empty()) {
        return;
    }
    append_rule_table(page, spc.rules);
}

}  // namespace datalab::application
