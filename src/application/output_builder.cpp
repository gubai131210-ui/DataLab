#include "application/output_builder.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace datalab::application {

using datalab::domain::StatisticTable;

namespace {

std::string triggered_tests_text(
    const datalab::domain::statistics::ControlChartResult& chart,
    std::size_t index)
{
    if (index >= chart.triggered_tests.size()
        || chart.triggered_tests[index].empty()) {
        return {};
    }
    std::ostringstream stream;
    for (std::size_t offset = 0; offset < chart.triggered_tests[index].size(); ++offset) {
        if (offset > 0) {
            stream << ",";
        }
        stream << "Test " << chart.triggered_tests[index][offset];
    }
    return stream.str();
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
    domain::StatisticTable table;
    table.title = "规则证据";
    table.headers = {"规则", "状态", "证据", "关联行", "建议"};
    for (const auto& rule : rules) {
        std::string rows;
        for (std::size_t index = 0; index < rule.related_rows.size(); ++index) {
            if (index != 0) {
                rows += ",";
            }
            rows += std::to_string(rule.related_rows[index] + 1);
        }
        table.rows.push_back({
            rule.id, rule.status, rule.message, rows, rule.suggested_action});
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
    const std::string& rate_header)
{
    StatisticTable table;
    table.title = title;
    table.headers = {
        "原始行", "子组", count_header, denominator_header, rate_header, "中心线", "LCL", "UCL",
        "触发测试", "最小测试"};
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
            std::to_string(count),
            std::to_string(denominator),
            format_number(chart.plotted_values[index]),
            format_number(center),
            format_number(lower),
            format_number(upper),
            tests,
            primary_test > 0 ? "Test " + std::to_string(primary_test) : ""});
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
                    "Z", "MR", "中心线", "LCL", "UCL", "Test 1", "Test 2",
                    "Test 3", "Test 4", "触发测试", "最小测试"};
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
            triggered_tests_text(chart, index),
            index < chart.primary_test_by_point.size()
                && chart.primary_test_by_point[index] > 0
                ? "Test " + std::to_string(chart.primary_test_by_point[index]) : ""});
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
    return plot;
}

}  // namespace datalab::application
