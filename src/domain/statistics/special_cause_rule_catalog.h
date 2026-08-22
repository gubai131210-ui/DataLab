#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/control_charts.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace datalab::domain::statistics {

// Stable status ids for special-cause rule evidence (user labels are localized).
inline constexpr const char* kSpecialCauseStatusNotTriggered = "not_triggered";
inline constexpr const char* kSpecialCauseStatusTriggered = "triggered";
inline constexpr const char* kSpecialCauseStatusNotVerified = "not_verified";
inline constexpr const char* kSpecialCauseStatusNotApplicable = "not_applicable";
inline constexpr const char* kSpecialCauseStatusCalculationFailed = "calculation_failed";

struct SpecialCauseRuleSpec {
    int number = 0;
    int default_window_size = 0;
    const char* rule_id = "";
    const char* display_name = "";
    const char* short_explanation = "";
    const char* window = "";
    const char* threshold = "";
    const char* comparison_direction = "";
    const char* suggested_action = "";
};

const std::vector<SpecialCauseRuleSpec>& special_cause_rule_catalog();
const SpecialCauseRuleSpec* find_special_cause_rule_by_number(int number);
const SpecialCauseRuleSpec* find_special_cause_rule_by_id(std::string_view rule_id);

std::string special_cause_rule_display_name(int number);
std::string special_cause_rule_status_label(std::string_view status_id);

// Formats enabled/selected rule numbers as readable names (never "Test N" alone).
std::string format_special_cause_rule_names(const std::vector<int>& tests);

// Formats per-point triggered rules; empty → "未触发".
std::string format_triggered_special_cause_rules(const std::vector<int>& tests);

// Primary rule label for a point; number<=0 → "未触发".
std::string format_primary_special_cause_rule(int test_number);

std::vector<RuleEvidence> build_special_cause_rule_evidences(
    const ControlChartResult& result,
    ControlChartKind kind,
    const SpecialCauseSelection& selection);

StatisticTable special_cause_rule_evidence_table(
    const std::vector<RuleEvidence>& rules);

}  // namespace datalab::domain::statistics
