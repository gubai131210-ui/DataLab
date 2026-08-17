#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/control_charts.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace datalab::application {

// ---- 子组双图页面（Xbar-R / Xbar-S）----

struct DualSubgroupChartSpec {
    std::string title;                // "Xbar-R 控制图"
    std::string method_name;          // "Xbar-R Chart"
    std::string id_prefix;            // "xbarr"
    std::string sigma_label;          // "R̄" / "S̄"（参数表行标签）
    std::string sigma_expression;     // "R̄ / d2" / "S̄ / c4"（parameter_summary）
    std::string secondary_short;      // "R" / "S"（逐子组表列名前缀）
    std::string secondary_plot_title; // "R 图" / "S 图"
    std::string secondary_axis;       // "子组极差" / "子组标准差"
    std::string parameter_table_title;   // "Xbar-R 参数" / "Xbar-S 参数"
    std::string subgroup_table_title;    // "Xbar-R 逐子组统计" / "Xbar-S 逐子组统计"
    // xbar_s 的 parameter_summary 用"子组大小 = value_or(5)"，xbar_range 用"子组数 = 实际数量"
    bool use_config_subgroup_size_in_summary = false;
    std::function<domain::statistics::DualControlChartResult(
        const std::vector<std::vector<double>>&)> compute;
    // 返回错误消息；空串表示通过。空函数表示不做额外校验。
    std::function<std::string(const std::vector<std::vector<double>>&)> validate = {};
};

domain::OutputPage subgroup_dual_chart_page(
    const domain::DataTable& table,
    const domain::AnalysisConfiguration& configuration,
    const DualSubgroupChartSpec& spec);

// ---- 属性控制图页面（P / NP / C / U）----

struct AttributeChartData {
    std::vector<std::size_t> counts;
    std::vector<std::size_t> denominators;
    std::vector<std::size_t> source_rows;
};

struct AttributeChartSpec {
    std::string title;              // "P 图"
    std::string method_name;        // "P Chart"
    std::string id_prefix;          // "pchart"
    std::string count_header;       // "不合格品数" / "缺陷数"
    std::string denominator_header; // "检验数" / "单位数"
    std::string rate_header;        // "不合格品率" / "不合格品数" / "缺陷数" / "单位缺陷数"
    std::string plot_title;         // "P 图"（控制图标题）
    std::string y_axis;             // "不合格品率" / "不合格品数" / "缺陷数" / "单位缺陷数"
    std::string parameter_summary;
    // 数据装配：返回 counts/denominators/source_rows；失败返回 nullopt 并写 error。
    std::function<std::optional<AttributeChartData>(
        const domain::DataTable&,
        const domain::AnalysisConfiguration&,
        std::string&)> assemble;
    std::function<domain::statistics::ControlChartResult(
        const std::vector<std::size_t>&,
        const std::vector<std::size_t>&)> compute;
};

domain::OutputPage attribute_chart_page(
    const domain::DataTable& table,
    const domain::AnalysisConfiguration& configuration,
    const AttributeChartSpec& spec);

// ---- Laney 图页面（Laney P' / Laney U'）----

struct LaneyChartSpec {
    std::string title;              // "Laney P' 图"
    std::string method_name;        // "Laney P' Chart"
    std::string id_prefix;          // "laneyp"
    std::string distribution_text;  // "二项分布" / "泊松分布"
    std::string center_label;       // "p̄" / "ū"（参数表行标签）
    std::string count_header;       // "不合格品数" / "缺陷数"
    std::string denominator_header; // "检验数" / "单位数"
    std::string y_axis;             // "不合格品率" / "单位缺陷数"
    bool include_enabled_tests_row = false;  // laney_p 有"启用测试"行
    std::function<std::optional<AttributeChartData>(
        const domain::DataTable&,
        const domain::AnalysisConfiguration&,
        std::string&)> assemble;
    std::function<domain::statistics::ControlChartResult(
        const std::vector<std::size_t>&,
        const std::vector<std::size_t>&,
        const domain::statistics::LaneyChartOptions&)> compute;
};

domain::OutputPage laney_chart_page(
    const domain::DataTable& table,
    const domain::AnalysisConfiguration& configuration,
    const LaneyChartSpec& spec);

}  // namespace datalab::application
