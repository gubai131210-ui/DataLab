#include "domain/statistics/special_cause_rule_catalog.h"
#include "domain/statistics/control_charts.h"
#include "infrastructure/output_serialization.h"

#include <QTest>

#include <set>
#include <string>

using datalab::domain::statistics::ControlChartKind;
using datalab::domain::statistics::ControlChartResult;
using datalab::domain::statistics::SpecialCauseSelection;
using datalab::domain::statistics::apply_special_cause_tests;
using datalab::domain::statistics::build_special_cause_rule_evidences;
using datalab::domain::statistics::find_special_cause_rule_by_id;
using datalab::domain::statistics::format_special_cause_rule_names;
using datalab::domain::statistics::format_triggered_special_cause_rules;
using datalab::domain::statistics::kSpecialCauseStatusNotApplicable;
using datalab::domain::statistics::kSpecialCauseStatusNotTriggered;
using datalab::domain::statistics::kSpecialCauseStatusTriggered;
using datalab::domain::statistics::special_cause_rule_catalog;
using datalab::domain::statistics::special_cause_rule_display_name;
using datalab::domain::statistics::special_cause_rule_status_label;

class SpecialCauseRuleCatalogTest final : public QObject {
    Q_OBJECT

private slots:
    void catalogHasEightStableIds();
    void displayNamesAreConcrete();
    void formatNeverUsesBareTestNumbers();
    void triggeredEmptyIsNotTriggered();
    void evidencesCoverStatuses();
    void ruleEvidenceJsonRoundTrip();
};

void SpecialCauseRuleCatalogTest::catalogHasEightStableIds()
{
    QCOMPARE(special_cause_rule_catalog().size(), std::size_t{8});
    const std::set<std::string> expected = {
        "beyond_control_limit",
        "nine_same_side",
        "six_point_trend",
        "fourteen_alternating",
        "two_of_three_beyond_2sigma",
        "four_of_five_beyond_1sigma",
        "fifteen_within_1sigma",
        "eight_beyond_1sigma"};
    std::set<std::string> actual;
    for (const auto& rule : special_cause_rule_catalog()) {
        actual.insert(rule.rule_id);
        QVERIFY(find_special_cause_rule_by_id(rule.rule_id) != nullptr);
    }
    QCOMPARE(actual, expected);
}

void SpecialCauseRuleCatalogTest::displayNamesAreConcrete()
{
    QCOMPARE(special_cause_rule_display_name(1), std::string("单点超出 3σ 控制限"));
    QCOMPARE(special_cause_rule_display_name(2), std::string("连续 9 点位于中心线同侧"));
    QCOMPARE(special_cause_rule_display_name(3), std::string("连续 6 点持续单调趋势"));
    QCOMPARE(special_cause_rule_display_name(4), std::string("连续 14 点上下交替"));
    QCOMPARE(special_cause_rule_display_name(5), std::string("3 点中至少 2 点同侧超过 2σ"));
    QCOMPARE(special_cause_rule_display_name(6), std::string("5 点中至少 4 点同侧超过 1σ"));
    QCOMPARE(special_cause_rule_display_name(7), std::string("连续 15 点全部落在 1σ 内"));
    QCOMPARE(special_cause_rule_display_name(8), std::string("连续 8 点全部位于 1σ 外且同侧"));
}

void SpecialCauseRuleCatalogTest::formatNeverUsesBareTestNumbers()
{
    const std::string text = format_special_cause_rule_names({1, 5});
    QVERIFY(text.find("Test 1") == std::string::npos);
    QVERIFY(text.find("Test 5") == std::string::npos);
    QVERIFY(text.find("单点超出 3σ 控制限") != std::string::npos);
    QVERIFY(text.find("3 点中至少 2 点同侧超过 2σ") != std::string::npos);
}

void SpecialCauseRuleCatalogTest::triggeredEmptyIsNotTriggered()
{
    QCOMPARE(format_triggered_special_cause_rules({}), std::string("未触发"));
    QCOMPARE(special_cause_rule_status_label(kSpecialCauseStatusTriggered),
             std::string("已触发"));
    QCOMPARE(special_cause_rule_status_label(kSpecialCauseStatusNotTriggered),
             std::string("未触发"));
}

void SpecialCauseRuleCatalogTest::evidencesCoverStatuses()
{
    ControlChartResult chart;
    chart.plotted_values.assign(3, 0.0);
    chart.center_line.assign(3, 0.0);
    chart.lower_control_limit.assign(3, -3.0);
    chart.upper_control_limit.assign(3, 3.0);
    chart.point_sigma.assign(3, 1.0);
    chart.source_rows = {10, 11, 12};
    chart.plotted_values[0] = 4.0;
    SpecialCauseSelection selection;
    selection.enabled_tests = {1};
    selection.policy = "explicit";
    apply_special_cause_tests(chart, ControlChartKind::individuals, selection);

    const auto evidences = build_special_cause_rule_evidences(
        chart, ControlChartKind::ewma, selection);
    QCOMPARE(evidences.size(), std::size_t{8});
    bool saw_triggered = false;
    bool saw_not_applicable = false;
    bool saw_not_verified = false;
    for (const auto& evidence : evidences) {
        if (evidence.id == "beyond_control_limit") {
            QCOMPARE(evidence.status, std::string(kSpecialCauseStatusTriggered));
            QVERIFY(!evidence.plotted_points.empty());
            saw_triggered = true;
        } else if (evidence.id == "nine_same_side") {
            QCOMPARE(evidence.status, std::string(kSpecialCauseStatusNotApplicable));
            QVERIFY(!evidence.not_applicable_reason.empty());
            saw_not_applicable = true;
        } else if (evidence.status == kSpecialCauseStatusNotVerified) {
            saw_not_verified = true;
        }
        QVERIFY(evidence.name.find("Test ") == std::string::npos);
    }
    QVERIFY(saw_triggered);
    QVERIFY(saw_not_applicable);
    QVERIFY(saw_not_verified);
}

void SpecialCauseRuleCatalogTest::ruleEvidenceJsonRoundTrip()
{
    ControlChartResult chart;
    chart.plotted_values = {0.0, 0.0, 4.0};
    chart.center_line = {0.0, 0.0, 0.0};
    chart.lower_control_limit = {-3.0, -3.0, -3.0};
    chart.upper_control_limit = {3.0, 3.0, 3.0};
    chart.point_sigma = {1.0, 1.0, 1.0};
    chart.source_rows = {0, 1, 2};
    SpecialCauseSelection selection;
    selection.enabled_tests = {1};
    selection.policy = "explicit";
    apply_special_cause_tests(chart, ControlChartKind::individuals, selection);

    datalab::domain::OutputPage page;
    page.id = "spc_rules";
    page.title = "rules";
    page.method_name = "I-MR Chart";
    page.facts.spc = datalab::domain::SpcFacts{};
    page.facts.spc->rules = build_special_cause_rule_evidences(
        chart, ControlChartKind::individuals, selection);
    page.facts.spc->enabled_special_cause_rule_ids = {"beyond_control_limit"};

    const QJsonObject json = datalab::infrastructure::output_page_to_json(page);
    const datalab::domain::OutputPage restored =
        datalab::infrastructure::output_page_from_json(json);
    QVERIFY(restored.facts.spc.has_value());
    QCOMPARE(restored.facts.spc->enabled_special_cause_rule_ids.front(),
             std::string("beyond_control_limit"));
    QVERIFY(!restored.facts.spc->rules.empty());
    QCOMPARE(restored.facts.spc->rules.front().id, page.facts.spc->rules.front().id);
    QCOMPARE(restored.facts.spc->rules.front().name, page.facts.spc->rules.front().name);
    QCOMPARE(restored.facts.spc->rules.front().window, page.facts.spc->rules.front().window);
}

QTEST_APPLESS_MAIN(SpecialCauseRuleCatalogTest)

#include "special_cause_rule_catalog_test.moc"
