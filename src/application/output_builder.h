#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/control_charts.h"
#include "domain/statistics/descriptive_statistics.h"
#include "domain/statistics/hypothesis_tests.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::application {

// ---- 数字格式化（全应用统一格式策略） ----

std::string format_number(double value, int digits = 6);
std::string format_optional(const std::optional<double>& value, int digits = 6);
std::optional<double> parse_numeric_cell(const std::string& text);

// ---- 页面装配辅助 ----

std::string new_id(const std::string& prefix);
domain::OutputPage error_page(
    const std::string& title,
    const std::string& method,
    const std::string& message);
void append_diagnostics(
    std::vector<domain::DiagnosticMessage>& target,
    const std::vector<domain::DiagnosticMessage>& source,
    const std::string& prefix);

// ---- 配置字符串 → 领域枚举 / 文案 ----

domain::statistics::TestAlternative parse_alternative(const std::string& value);
domain::statistics::VarianceMethod parse_variance_method(const std::string& value);
std::string alternative_label(const std::string& value);
bool append_nonnegative_counts(
    const std::vector<double>& values,
    std::vector<std::size_t>& counts);

// ---- 通用表格 / 控制图构建器 ----

domain::StatisticTable t_test_table(
    const std::string& title,
    const domain::statistics::TTestResult& result,
    const std::string& variable);

domain::StatisticTable descriptive_table(
    const std::vector<domain::statistics::DescriptiveStatisticsResult>& rows);

domain::StatisticTable attribute_chart_table(
    const std::string& title,
    const std::vector<std::size_t>& counts,
    const std::vector<std::size_t>& denominators,
    const domain::statistics::ControlChartResult& chart,
    const std::string& count_header,
    const std::string& denominator_header,
    const std::string& rate_header);

domain::StatisticTable laney_chart_table(
    const std::vector<std::size_t>& counts,
    const std::vector<std::size_t>& denominators,
    const domain::statistics::ControlChartResult& chart,
    const std::vector<std::string>& stages,
    const std::string& count_header,
    const std::string& denominator_header);

domain::PlotSpec control_plot(
    const std::string& title,
    const std::string& y_axis,
    const domain::statistics::ControlChartResult& chart,
    const std::vector<std::size_t>& source_rows);

}  // namespace datalab::application
