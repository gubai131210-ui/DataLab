#include "domain/statistics/special_cause_rule_catalog.h"

#include <algorithm>
#include <sstream>

namespace datalab::domain::statistics {
namespace {

std::string join_sizes(const std::vector<std::size_t>& values)
{
    std::ostringstream stream;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            stream << ',';
        }
        stream << (values[index] + 1);
    }
    return stream.str();
}

std::string join_row_ids(const std::vector<RowId>& values)
{
    std::ostringstream stream;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            stream << ',';
        }
        stream << (values[index] + 1);
    }
    return stream.str();
}

bool chart_supports_rule(ControlChartKind kind, int number)
{
    const std::vector<int> applicable = applicable_special_cause_tests(kind);
    return std::find(applicable.begin(), applicable.end(), number) != applicable.end();
}

}  // namespace

const std::vector<SpecialCauseRuleSpec>& special_cause_rule_catalog()
{
    static const std::vector<SpecialCauseRuleSpec> catalog = {
        {1, 1, "beyond_control_limit",
         "单点超出 3σ 控制限",
         "任一点低于 LCL 或高于 UCL；有限控制限使用严格越界比较。表示该点与当前控制模型不一致，不等于根因已确认。",
         "单点",
         "LCL / UCL（约 3σ）",
         "y < LCL 或 y > UCL（严格）",
         "复核测量、批次、设备或取样条件，并关联原始观测行。"},
        {2, 9, "nine_same_side",
         "连续 9 点位于中心线同侧",
         "连续 9 个可用点全部在中心线同一侧；跨缺失或阶段断点不形成窗口。提示均值偏移、分层或阶段变化。",
         "连续 9 点",
         "同侧 9 点",
         "全部位于中心线同一侧（中心线上点打断）",
         "调查均值偏移、分层或阶段变化，检查窗口首尾相关行。"},
        {3, 6, "six_point_trend",
         "连续 6 点持续单调趋势",
         "连续 6 点严格递增或严格递减；相等值不构成趋势。提示磨损、漂移或时间相关结构。",
         "连续 6 点",
         "严格单调 6 点",
         "严格递增或严格递减（相等打断）",
         "调查磨损、漂移或时间相关结构，核对趋势窗口点。"},
        {4, 14, "fourteen_alternating",
         "连续 14 点上下交替",
         "连续 14 点相邻差值符号严格交替；零差值打断规则。提示周期、人机/设备交替或过度调整。",
         "连续 14 点",
         "相邻符号严格交替",
         "相邻差分符号交替（零差分打断）",
         "调查周期、交替作业或过度调整，核对交替窗口。"},
        {5, 3, "two_of_three_beyond_2sigma",
         "3 点中至少 2 点同侧超过 2σ",
         "3 点窗口内至少 2 点位于中心线同侧且距离严格大于 2σ；提示较小但系统性的偏移。",
         "连续 3 点窗口",
         "同侧且 |y−CL| > 2σ，至少 2 点",
         "同侧且严格大于 2σ",
         "调查较小系统性偏移，核对命中点标准化距离。"},
        {6, 5, "four_of_five_beyond_1sigma",
         "5 点中至少 4 点同侧超过 1σ",
         "5 点窗口内至少 4 点位于同侧且距离严格大于 1σ；提示过程均值发生小幅移动。",
         "连续 5 点窗口",
         "同侧且 |y−CL| > 1σ，至少 4 点",
         "同侧且严格大于 1σ",
         "调查过程均值小幅移动，核对命中点标准化距离。"},
        {7, 15, "fifteen_within_1sigma",
         "连续 15 点全部落在 1σ 内",
         "连续 15 个点的绝对中心线距离严格小于 σ；提示控制限可能过宽或数据存在分层。",
         "连续 15 点",
         "|y−CL| < σ",
         "严格落在 ±1σ 以内",
         "调查控制限是否过宽或数据分层，核对 σ 来源。"},
        {8, 8, "eight_beyond_1sigma",
         "连续 8 点全部位于 1σ 外且同侧",
         "连续 8 点均在中心线同一侧且绝对距离严格大于 σ；提示混合总体或双群模式。",
         "连续 8 点",
         "同侧且 |y−CL| > σ",
         "同侧且严格大于 1σ",
         "调查混合总体或双群模式，按分组/批次复核。"},
    };
    return catalog;
}

const SpecialCauseRuleSpec* find_special_cause_rule_by_number(int number)
{
    for (const SpecialCauseRuleSpec& spec : special_cause_rule_catalog()) {
        if (spec.number == number) {
            return &spec;
        }
    }
    return nullptr;
}

const SpecialCauseRuleSpec* find_special_cause_rule_by_id(std::string_view rule_id)
{
    for (const SpecialCauseRuleSpec& spec : special_cause_rule_catalog()) {
        if (rule_id == spec.rule_id) {
            return &spec;
        }
    }
    return nullptr;
}

std::string special_cause_rule_display_name(int number)
{
    if (const SpecialCauseRuleSpec* spec = find_special_cause_rule_by_number(number)) {
        return spec->display_name;
    }
    return "未知特殊原因规则";
}

std::string special_cause_rule_status_label(std::string_view status_id)
{
    if (status_id == kSpecialCauseStatusTriggered) {
        return "已触发";
    }
    if (status_id == kSpecialCauseStatusNotTriggered) {
        return "未触发";
    }
    if (status_id == kSpecialCauseStatusNotVerified) {
        return "未验证";
    }
    if (status_id == kSpecialCauseStatusNotApplicable) {
        return "不适用";
    }
    if (status_id == kSpecialCauseStatusCalculationFailed) {
        return "计算失败";
    }
    return std::string(status_id);
}

std::string format_special_cause_rule_names(const std::vector<int>& tests)
{
    if (tests.empty()) {
        return "无";
    }
    std::ostringstream stream;
    for (std::size_t index = 0; index < tests.size(); ++index) {
        if (index > 0) {
            stream << ", ";
        }
        stream << special_cause_rule_display_name(tests[index]);
    }
    return stream.str();
}

std::string format_triggered_special_cause_rules(const std::vector<int>& tests)
{
    if (tests.empty()) {
        return "未触发";
    }
    return format_special_cause_rule_names(tests);
}

std::string format_primary_special_cause_rule(int test_number)
{
    if (test_number <= 0) {
        return "未触发";
    }
    return special_cause_rule_display_name(test_number);
}

std::vector<RuleEvidence> build_special_cause_rule_evidences(
    const ControlChartResult& result,
    ControlChartKind kind,
    const SpecialCauseSelection& selection)
{
    const std::vector<int> enabled =
        resolve_special_cause_tests(selection, kind, nullptr);
    std::vector<RuleEvidence> evidences;
    evidences.reserve(special_cause_rule_catalog().size());

    for (const SpecialCauseRuleSpec& spec : special_cause_rule_catalog()) {
        RuleEvidence evidence;
        evidence.id = spec.rule_id;
        evidence.name = spec.display_name;
        evidence.window = spec.window;
        evidence.threshold = spec.threshold;
        evidence.comparison_direction = spec.comparison_direction;
        evidence.suggested_action = spec.suggested_action;
        evidence.message = spec.short_explanation;

        if (!chart_supports_rule(kind, spec.number)) {
            evidence.status = kSpecialCauseStatusNotApplicable;
            evidence.not_applicable_reason =
                "此规则不适用于控制图类型 " + control_chart_kind_name(kind) + "。";
            evidences.push_back(std::move(evidence));
            continue;
        }

        if (std::find(enabled.begin(), enabled.end(), spec.number) == enabled.end()) {
            evidence.status = kSpecialCauseStatusNotVerified;
            evidence.not_verified_reason = "规则未启用，本次分析未验证该特殊原因模式。";
            evidences.push_back(std::move(evidence));
            continue;
        }

        if (result.plotted_values.empty()) {
            evidence.status = kSpecialCauseStatusCalculationFailed;
            evidence.calculation_failed_reason = "没有可绘制的控制图点，无法判定规则。";
            evidences.push_back(std::move(evidence));
            continue;
        }

        const std::size_t slot = static_cast<std::size_t>(spec.number - 1);
        if (slot < result.special_cause_points.size()) {
            evidence.plotted_points = result.special_cause_points[slot];
        }
        for (const std::size_t point : evidence.plotted_points) {
            if (point < result.source_rows.size()) {
                evidence.related_rows.push_back(result.source_rows[point]);
            }
        }

        if (!evidence.plotted_points.empty()) {
            evidence.status = kSpecialCauseStatusTriggered;
            evidence.message = std::string(spec.short_explanation)
                + " 触发图点序号（1-based）: "
                + join_sizes(evidence.plotted_points) + "。";
        } else {
            evidence.status = kSpecialCauseStatusNotTriggered;
            evidence.message = std::string(spec.short_explanation) + " 当前未触发。";
        }
        evidences.push_back(std::move(evidence));
    }
    return evidences;
}

StatisticTable special_cause_rule_evidence_table(const std::vector<RuleEvidence>& rules)
{
    StatisticTable table;
    table.title = "特殊原因规则证据";
    table.headers = {
        "规则ID", "规则名称", "状态", "判定窗口", "阈值", "比较方向",
        "解释", "触发图点", "原始行", "建议动作",
        "不适用原因", "未验证原因", "计算失败原因"};
    table.column_kinds = {
        "rule_id", "text", "status", "text", "text", "text",
        "text", "text", "row_id", "text",
        "text", "text", "text"};
    for (const RuleEvidence& rule : rules) {
        table.rule_ids.push_back(rule.id);
        table.row_ids.push_back(rule.related_rows.empty() ? 0 : rule.related_rows.front());
        table.rows.push_back({
            rule.id,
            rule.name.empty() ? rule.id : rule.name,
            special_cause_rule_status_label(rule.status),
            rule.window,
            rule.threshold,
            rule.comparison_direction,
            rule.message,
            join_sizes(rule.plotted_points),
            join_row_ids(rule.related_rows),
            rule.suggested_action,
            rule.not_applicable_reason,
            rule.not_verified_reason,
            rule.calculation_failed_reason});
    }
    return table;
}

}  // namespace datalab::domain::statistics
