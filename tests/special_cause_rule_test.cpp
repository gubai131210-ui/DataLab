#include "domain/statistics/control_charts.h"
#include "ui/analysis_commands.h"
#include "ui/analysis_setup_dialog.h"

#include <QCheckBox>
#include <QLabel>
#include <QTest>

#include <cmath>
#include <limits>
#include <vector>

using datalab::domain::statistics::ControlChartKind;
using datalab::domain::statistics::ControlChartResult;
using datalab::domain::statistics::ControlCharts;
using datalab::domain::statistics::CusumOptions;
using datalab::domain::statistics::EwmaOptions;
using datalab::domain::statistics::SpecialCauseSelection;
using datalab::domain::statistics::apply_special_cause_tests;
using datalab::domain::statistics::applicable_special_cause_tests;
using datalab::domain::statistics::default_special_cause_tests;
using datalab::domain::statistics::parse_special_cause_tests;
using datalab::domain::statistics::resolve_special_cause_tests;

class SpecialCauseRuleTest final : public QObject {
    Q_OBJECT

private:
    static ControlChartResult synthetic(const std::vector<double>& values)
    {
        ControlChartResult result;
        result.plotted_values = values;
        result.center_line.assign(values.size(), 0.0);
        result.lower_control_limit.assign(values.size(), -3.0);
        result.upper_control_limit.assign(values.size(), 3.0);
        result.point_sigma.assign(values.size(), 1.0);
        return result;
    }

private slots:
    void applicableRulesMatchChartKind();
    void parsesLegacyTextAndNone();
    void defaultAllApplicableUnlessExplicit();
    void filtersInapplicableRules();
    void detectsEachMinitabRule();
    void equalValuesBreakTrendAndAlternation();
    void testEightDoesNotRequireBothSides();
    void phaseLabelsBreakWindows();
    void overlappingWindowsMergeTriggeredTests();
    void missingValuesBreakWindows();
    void exactThreeSigmaDoesNotTriggerTestOne();
    void cusumDoesNotApplyShewhartTests();
    void dialogDefaultsSelectAllApplicable();
    void dialogGraysInapplicableRules();
};

void SpecialCauseRuleTest::applicableRulesMatchChartKind()
{
    QCOMPARE(applicable_special_cause_tests(ControlChartKind::individuals),
             (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8}));
    QCOMPARE(applicable_special_cause_tests(ControlChartKind::attribute),
             (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8}));
    QCOMPARE(applicable_special_cause_tests(ControlChartKind::range),
             (std::vector<int>{1, 2, 3, 4}));
    QCOMPARE(applicable_special_cause_tests(ControlChartKind::moving_range),
             (std::vector<int>{1, 2, 3, 4}));
    QCOMPARE(applicable_special_cause_tests(ControlChartKind::ewma),
             (std::vector<int>{1}));
    QVERIFY(applicable_special_cause_tests(ControlChartKind::cusum).empty());
    QCOMPARE(default_special_cause_tests(ControlChartKind::xbar).size(), std::size_t{8});
}

void SpecialCauseRuleTest::parsesLegacyTextAndNone()
{
    QCOMPARE(parse_special_cause_tests("1,2 3;8"), (std::vector<int>{1, 2, 3, 8}));
    QVERIFY(parse_special_cause_tests("none").empty());
    QVERIFY(parse_special_cause_tests("NONE").empty());
}

void SpecialCauseRuleTest::defaultAllApplicableUnlessExplicit()
{
    SpecialCauseSelection all_default;
    all_default.policy = "default_all_applicable";
    QCOMPARE(resolve_special_cause_tests(all_default, ControlChartKind::individuals).size(),
             std::size_t{8});

    SpecialCauseSelection legacy;
    legacy.enabled_tests = {1};
    QCOMPARE(resolve_special_cause_tests(legacy, ControlChartKind::laney),
             (std::vector<int>{1}));

    SpecialCauseSelection cleared;
    cleared.policy = "explicit";
    QVERIFY(resolve_special_cause_tests(cleared, ControlChartKind::attribute).empty());
}

void SpecialCauseRuleTest::filtersInapplicableRules()
{
    SpecialCauseSelection selection{{1, 5, 8}, "explicit"};
    std::vector<datalab::domain::DiagnosticMessage> diagnostics;
    const auto enabled = resolve_special_cause_tests(
        selection, ControlChartKind::moving_range, &diagnostics);
    QCOMPARE(enabled, (std::vector<int>{1}));
    QVERIFY(!diagnostics.empty());
}

void SpecialCauseRuleTest::detectsEachMinitabRule()
{
    SpecialCauseSelection only;
    only.policy = "explicit";

    only.enabled_tests = {1};
    auto t1 = synthetic({0, 0, 0, 3.1});
    apply_special_cause_tests(t1, ControlChartKind::individuals, only);
    QCOMPARE(t1.special_cause_points[0], (std::vector<std::size_t>{3}));

    only.enabled_tests = {2};
    auto t2 = synthetic({1, 1, 1, 1, 1, 1, 1, 1, 1});
    apply_special_cause_tests(t2, ControlChartKind::individuals, only);
    QCOMPARE(t2.special_cause_points[1].size(), std::size_t{9});

    only.enabled_tests = {3};
    auto t3 = synthetic({0, 1, 2, 3, 4, 5});
    apply_special_cause_tests(t3, ControlChartKind::individuals, only);
    QCOMPARE(t3.special_cause_points[2].size(), std::size_t{6});

    only.enabled_tests = {4};
    auto t4 = synthetic({1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1});
    apply_special_cause_tests(t4, ControlChartKind::individuals, only);
    QCOMPARE(t4.special_cause_points[3].size(), std::size_t{14});

    only.enabled_tests = {5};
    auto t5 = synthetic({2.1, 2.2, 0});
    apply_special_cause_tests(t5, ControlChartKind::individuals, only);
    QCOMPARE(t5.special_cause_points[4].size(), std::size_t{3});

    only.enabled_tests = {6};
    auto t6 = synthetic({1.1, 1.2, 1.3, 1.4, 0});
    apply_special_cause_tests(t6, ControlChartKind::individuals, only);
    QCOMPARE(t6.special_cause_points[5].size(), std::size_t{5});

    only.enabled_tests = {7};
    auto t7 = synthetic({
        0.1, -0.2, 0.3, -0.4, 0.0, 0.2, -0.1, 0.4, -0.3, 0.1, -0.2, 0.3, -0.4, 0.2, 0.0});
    apply_special_cause_tests(t7, ControlChartKind::individuals, only);
    QCOMPARE(t7.special_cause_points[6].size(), std::size_t{15});

    only.enabled_tests = {8};
    auto t8 = synthetic({1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8});
    apply_special_cause_tests(t8, ControlChartKind::individuals, only);
    QCOMPARE(t8.special_cause_points[7].size(), std::size_t{8});
}

void SpecialCauseRuleTest::equalValuesBreakTrendAndAlternation()
{
    SpecialCauseSelection only{{3}, "explicit"};
    auto trend = synthetic({0, 1, 1, 2, 3, 4});
    apply_special_cause_tests(trend, ControlChartKind::individuals, only);
    QVERIFY(trend.special_cause_points[2].empty());

    only.enabled_tests = {4};
    auto alternating = synthetic({1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, 1});
    apply_special_cause_tests(alternating, ControlChartKind::individuals, only);
    QVERIFY(alternating.special_cause_points[3].empty());
}

void SpecialCauseRuleTest::testEightDoesNotRequireBothSides()
{
    SpecialCauseSelection only{{8}, "explicit"};
    auto same_side = synthetic({1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8});
    apply_special_cause_tests(same_side, ControlChartKind::individuals, only);
    QVERIFY(!same_side.special_cause_points[7].empty());
}

void SpecialCauseRuleTest::phaseLabelsBreakWindows()
{
    SpecialCauseSelection only{{2}, "explicit"};
    auto result = synthetic({1, 1, 1, 1, 1, 1, 1, 1, 1});
    result.phase_labels = {"A", "A", "A", "A", "B", "B", "B", "B", "B"};
    apply_special_cause_tests(result, ControlChartKind::individuals, only);
    QVERIFY(result.special_cause_points[1].empty());
}

void SpecialCauseRuleTest::overlappingWindowsMergeTriggeredTests()
{
    SpecialCauseSelection both{{1, 5}, "explicit"};
    auto result = synthetic({3.1, 2.2, 0.0});
    apply_special_cause_tests(result, ControlChartKind::individuals, both);
    QCOMPARE(result.triggered_tests[0], (std::vector<int>{1, 5}));
    QCOMPARE(result.primary_test_by_point[0], 1);
    QCOMPARE(result.triggered_tests[1], (std::vector<int>{5}));

    SpecialCauseSelection run{{2}, "explicit"};
    auto longer = synthetic({1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
    apply_special_cause_tests(longer, ControlChartKind::individuals, run);
    QCOMPARE(longer.special_cause_points[1].size(), std::size_t{10});
    QCOMPARE(longer.triggered_tests[9], (std::vector<int>{2}));
}

void SpecialCauseRuleTest::missingValuesBreakWindows()
{
    SpecialCauseSelection only{{2}, "explicit"};
    auto result = synthetic({1, 1, 1, 1, std::numeric_limits<double>::quiet_NaN(), 1, 1, 1, 1});
    apply_special_cause_tests(result, ControlChartKind::individuals, only);
    QVERIFY(result.special_cause_points[1].empty());
}

void SpecialCauseRuleTest::exactThreeSigmaDoesNotTriggerTestOne()
{
    SpecialCauseSelection only{{1}, "explicit"};
    auto result = synthetic({0, 0, 0, 3.0});
    apply_special_cause_tests(result, ControlChartKind::individuals, only);
    QVERIFY(result.special_cause_points[0].empty());
}

void SpecialCauseRuleTest::cusumDoesNotApplyShewhartTests()
{
    CusumOptions options;
    const auto chart = ControlCharts::cusum_chart({0.0, 0.1, -0.1, 0.2}, options);
    QVERIFY(chart.primary.special_cause_points.empty()
            || chart.primary.special_cause_points[0].empty());
    QVERIFY(chart.primary.triggered_tests.empty()
            || chart.primary.triggered_tests.front().empty());

    EwmaOptions ewma;
    ewma.special_causes = {{1, 5, 8}, "explicit"};
    const auto ewma_chart = ControlCharts::ewma_chart({0.0, 0.0, 0.0, 0.0}, ewma);
    bool ignored = false;
    for (const auto& diagnostic : ewma_chart.diagnostics) {
        ignored = ignored || diagnostic.code == "test_not_applicable";
    }
    QVERIFY(ignored);
}

void SpecialCauseRuleTest::dialogDefaultsSelectAllApplicable()
{
    AnalysisSetupDialog dialog(QStringLiteral("测试"), {QStringLiteral("C1")});
    analysis_commands::InputSpec spec{
        QStringLiteral("tests"), QStringLiteral("特殊原因测试"), QStringLiteral("individuals")};
    dialog.add_input(spec);
    QCOMPARE(dialog.line_text(QStringLiteral("tests")), QStringLiteral("1 2 3 4 5 6 7 8"));
    QVERIFY(dialog.findChild<QCheckBox*>() != nullptr);
    dialog.reset_defaults();
    QCOMPARE(dialog.line_text(QStringLiteral("tests")), QStringLiteral("1 2 3 4 5 6 7 8"));
}

void SpecialCauseRuleTest::dialogGraysInapplicableRules()
{
    auto enabled_count = [](AnalysisSetupDialog& dialog) {
        int enabled = 0;
        for (auto* check : dialog.findChildren<QCheckBox*>()) {
            enabled += check->isEnabled() ? 1 : 0;
        }
        return enabled;
    };

    AnalysisSetupDialog range_dialog(QStringLiteral("测试"), {QStringLiteral("C1")});
    range_dialog.add_input({
        QStringLiteral("tests"), QStringLiteral("特殊原因测试"), QStringLiteral("range")});
    QCOMPARE(enabled_count(range_dialog), 4);
    QCOMPARE(range_dialog.line_text(QStringLiteral("tests")), QStringLiteral("1 2 3 4"));

    AnalysisSetupDialog ewma_dialog(QStringLiteral("测试"), {QStringLiteral("C1")});
    ewma_dialog.add_input({
        QStringLiteral("tests"), QStringLiteral("特殊原因测试"), QStringLiteral("ewma")});
    QCOMPARE(enabled_count(ewma_dialog), 1);
    QCOMPARE(ewma_dialog.line_text(QStringLiteral("tests")), QStringLiteral("1"));

    AnalysisSetupDialog cusum_dialog(QStringLiteral("测试"), {QStringLiteral("C1")});
    cusum_dialog.add_input({
        QStringLiteral("tests"), QStringLiteral("特殊原因测试"), QStringLiteral("cusum")});
    QCOMPARE(enabled_count(cusum_dialog), 0);
    QCOMPARE(cusum_dialog.line_text(QStringLiteral("tests")), QStringLiteral("none"));
}

QTEST_MAIN(SpecialCauseRuleTest)
#include "special_cause_rule_test.moc"
