#include "application/analysis_service.h"
#include "domain/statistics/control_charts.h"
#include "ui/analysis_commands.h"
#include "ui/analysis_setup_dialog.h"

#include <QCheckBox>
#include <QLabel>
#include <QTest>

#include <algorithm>
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
    void testSevenBoundaryExcludesExactSigma();
    void testSevenXbarBoundaryExcludesExactSigma();
    void imrHistoricalExactSigmaDoesNotTriggerTestSeven();
    void imrAndXbarServiceTestSevenBoundary();
    void testTwoBoundaryEightSameSideNoTrigger();
    void testFourBoundaryThirteenAlternatingFails();
    void testFiveBoundaryExactTwoSigmaAndSinglePoint();
    void testSixBoundaryExactOneSigmaAndThreePoints();
    void testEightBoundaryExactOneSigmaNoTrigger();
    void testEightSameSideAllAboveOneSigma();
    void imrAndXbarServiceTestTwoBoundary();
    void imrAndXbarServiceTestFiveSixEightBoundary();
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

void SpecialCauseRuleTest::testSevenBoundaryExcludesExactSigma()
{
    // # source: formula_reference — Test 7 uses |y-CL| < σ; equal to 1σ is not inside.
    SpecialCauseSelection only{{7}, "explicit"};
    auto inside = synthetic({
        0.1, -0.2, 0.3, -0.4, 0.0, 0.2, -0.1, 0.4, -0.3, 0.1, -0.2, 0.3, -0.4, 0.2, 0.0});
    apply_special_cause_tests(inside, ControlChartKind::individuals, only);
    QCOMPARE(inside.special_cause_points[6].size(), std::size_t{15});

    auto on_boundary = synthetic({
        1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    apply_special_cause_tests(on_boundary, ControlChartKind::individuals, only);
    QVERIFY(on_boundary.special_cause_points[6].empty());
}

void SpecialCauseRuleTest::testSevenXbarBoundaryExcludesExactSigma()
{
    // # source: formula_reference — same <σ rule on Xbar plotted means.
    SpecialCauseSelection only{{7}, "explicit"};
    auto inside = synthetic({
        0.1, -0.2, 0.3, -0.4, 0.0, 0.2, -0.1, 0.4, -0.3, 0.1, -0.2, 0.3, -0.4, 0.2, 0.0});
    apply_special_cause_tests(inside, ControlChartKind::xbar, only);
    QCOMPARE(inside.special_cause_points[6].size(), std::size_t{15});

    auto on_boundary = synthetic({
        1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    apply_special_cause_tests(on_boundary, ControlChartKind::xbar, only);
    QVERIFY(on_boundary.special_cause_points[6].empty());
}

void SpecialCauseRuleTest::imrHistoricalExactSigmaDoesNotTriggerTestSeven()
{
    // # source: formula_reference — I-MR with known σ; a point on the 1σ ring
    // is not inside the Test 7 window.
    datalab::domain::statistics::IndividualsMovingRangeOptions options;
    options.historical_mean = 0.0;
    options.historical_sigma = 1.0;
    options.special_causes = {{7}, "explicit"};
    std::vector<double> inside(15, 0.4);
    const auto inside_chart =
        ControlCharts::individuals_moving_range_dual(inside, options);
    QCOMPARE(inside_chart.primary.special_cause_points[6].size(), std::size_t{15});

    std::vector<double> on_boundary(15, 0.4);
    on_boundary[0] = 1.0;
    const auto boundary_chart =
        ControlCharts::individuals_moving_range_dual(on_boundary, options);
    QVERIFY(boundary_chart.primary.special_cause_points[6].empty());
}

namespace {

bool plot_triggered_test(const datalab::domain::PlotSpec& plot, int test)
{
    for (const auto& tests : plot.triggered_tests) {
        if (std::find(tests.cbegin(), tests.cend(), test) != tests.cend()) {
            return true;
        }
    }
    return false;
}

}  // namespace

void SpecialCauseRuleTest::imrAndXbarServiceTestSevenBoundary()
{
    // # source: formula_reference — service I-MR / Xbar-R with Test 7 only.
    datalab::domain::DataTable imr_inside;
    imr_inside.columns = {"Y"};
    for (int index = 0; index < 15; ++index) {
        imr_inside.rows.push_back({index % 2 == 0 ? "10.0" : "10.1"});
    }
    datalab::domain::AnalysisConfiguration imr_config;
    imr_config.variable_columns = {0};
    imr_config.selection.measurement_column = 0;
    imr_config.control.enabled_special_cause_tests = {7};
    imr_config.control.special_cause_rule_policy = "explicit";
    const auto imr_inside_page =
        datalab::application::AnalysisService::individuals_moving_range(
            imr_inside, imr_config);
    QVERIFY(!imr_inside_page.plots.empty());
    QCOMPARE(imr_inside_page.plots.front().source_rows.size(), std::size_t{15});
    QCOMPARE(imr_inside_page.plots.front().source_rows.front(), std::size_t{0});
    QVERIFY(plot_triggered_test(imr_inside_page.plots.front(), 7));

    datalab::domain::DataTable imr_outside = imr_inside;
    imr_outside.rows[0] = {"50.0"};
    const auto imr_outside_page =
        datalab::application::AnalysisService::individuals_moving_range(
            imr_outside, imr_config);
    QVERIFY(!plot_triggered_test(imr_outside_page.plots.front(), 7));
    QCOMPARE(imr_outside_page.plots.front().source_rows.front(), std::size_t{0});

    datalab::domain::DataTable xbar_inside;
    xbar_inside.columns = {"Y"};
    for (int subgroup = 0; subgroup < 15; ++subgroup) {
        xbar_inside.rows.push_back({"10.0"});
        xbar_inside.rows.push_back({"10.2"});
    }
    datalab::domain::AnalysisConfiguration xbar_config;
    xbar_config.variable_columns = {0};
    xbar_config.selection.measurement_column = 0;
    xbar_config.control.subgroup_size = 2;
    xbar_config.control.enabled_special_cause_tests = {7};
    xbar_config.control.special_cause_rule_policy = "explicit";
    const auto xbar_inside_page =
        datalab::application::AnalysisService::xbar_range(xbar_inside, xbar_config);
    QVERIFY(!xbar_inside_page.plots.empty());
    QCOMPARE(xbar_inside_page.plots.front().source_rows.size(), std::size_t{15});
    QCOMPARE(xbar_inside_page.plots.front().source_rows[1], std::size_t{2});
    QVERIFY(plot_triggered_test(xbar_inside_page.plots.front(), 7));

    // Shift one subgroup mean so |ȳ − CL| ≥ σ_xbar (A2·R̄/3). Range stays 0.2.
    datalab::domain::DataTable xbar_boundary = xbar_inside;
    xbar_boundary.rows[0] = {"10.20"};
    xbar_boundary.rows[1] = {"10.40"};
    const auto xbar_boundary_page =
        datalab::application::AnalysisService::xbar_range(xbar_boundary, xbar_config);
    QVERIFY(!plot_triggered_test(xbar_boundary_page.plots.front(), 7));
}

void SpecialCauseRuleTest::testTwoBoundaryEightSameSideNoTrigger()
{
    // # source: formula_reference — Test 2 needs 9 consecutive same-side points.
    SpecialCauseSelection only{{2}, "explicit"};
    auto eight = synthetic({1, 1, 1, 1, 1, 1, 1, 1});
    apply_special_cause_tests(eight, ControlChartKind::individuals, only);
    QVERIFY(eight.special_cause_points[1].empty());

    auto nine = synthetic({1, 1, 1, 1, 1, 1, 1, 1, 1});
    apply_special_cause_tests(nine, ControlChartKind::individuals, only);
    QCOMPARE(nine.special_cause_points[1].size(), std::size_t{9});

    auto reset = synthetic({1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1});
    apply_special_cause_tests(reset, ControlChartKind::individuals, only);
    QVERIFY(reset.special_cause_points[1].empty());
}

void SpecialCauseRuleTest::testFourBoundaryThirteenAlternatingFails()
{
    // # source: formula_reference — Test 4 needs 14 strictly alternating points.
    SpecialCauseSelection only{{4}, "explicit"};
    auto broken = synthetic({1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, 1});
    apply_special_cause_tests(broken, ControlChartKind::individuals, only);
    QVERIFY(broken.special_cause_points[3].empty());

    auto strict = synthetic({1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1});
    apply_special_cause_tests(strict, ControlChartKind::individuals, only);
    QCOMPARE(strict.special_cause_points[3].size(), std::size_t{14});
}

void SpecialCauseRuleTest::testFiveBoundaryExactTwoSigmaAndSinglePoint()
{
    // # source: formula_reference — Test 5 uses strict >2σ; exactly 2σ does not count.
    SpecialCauseSelection only{{5}, "explicit"};
    auto on_boundary = synthetic({2.0, 0.0, 0.0});
    apply_special_cause_tests(on_boundary, ControlChartKind::individuals, only);
    QVERIFY(on_boundary.special_cause_points[4].empty());

    auto one_outside = synthetic({2.1, 0.0, 0.0});
    apply_special_cause_tests(one_outside, ControlChartKind::individuals, only);
    QVERIFY(one_outside.special_cause_points[4].empty());

    auto triggers = synthetic({2.1, 2.2, 0.0});
    apply_special_cause_tests(triggers, ControlChartKind::individuals, only);
    QCOMPARE(triggers.special_cause_points[4].size(), std::size_t{3});
}

void SpecialCauseRuleTest::testSixBoundaryExactOneSigmaAndThreePoints()
{
    // # source: formula_reference — Test 6 uses strict >1σ; need 4 of 5 on same side.
    SpecialCauseSelection only{{6}, "explicit"};
    auto on_boundary = synthetic({1.0, 1.0, 1.0, 1.0, 0.0});
    apply_special_cause_tests(on_boundary, ControlChartKind::individuals, only);
    QVERIFY(on_boundary.special_cause_points[5].empty());

    auto three_outside = synthetic({1.1, 1.1, 1.1, 0.0, 0.0});
    apply_special_cause_tests(three_outside, ControlChartKind::individuals, only);
    QVERIFY(three_outside.special_cause_points[5].empty());

    auto triggers = synthetic({1.1, 1.2, 1.3, 1.4, 0.0});
    apply_special_cause_tests(triggers, ControlChartKind::individuals, only);
    QCOMPARE(triggers.special_cause_points[5].size(), std::size_t{5});
}

void SpecialCauseRuleTest::testEightBoundaryExactOneSigmaNoTrigger()
{
    // # source: formula_reference — Test 8 uses strict >1σ; exactly 1σ does not count.
    SpecialCauseSelection only{{8}, "explicit"};
    auto on_boundary = synthetic({1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0});
    apply_special_cause_tests(on_boundary, ControlChartKind::individuals, only);
    QVERIFY(on_boundary.special_cause_points[7].empty());
}

void SpecialCauseRuleTest::testEightSameSideAllAboveOneSigma()
{
    // # source: formula_reference — Test 8 does not require alternating sides.
    SpecialCauseSelection only{{8}, "explicit"};
    auto same_side = synthetic({1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8});
    apply_special_cause_tests(same_side, ControlChartKind::individuals, only);
    QCOMPARE(same_side.special_cause_points[7].size(), std::size_t{8});
    apply_special_cause_tests(same_side, ControlChartKind::xbar, only);
    QCOMPARE(same_side.special_cause_points[7].size(), std::size_t{8});
}

void SpecialCauseRuleTest::imrAndXbarServiceTestTwoBoundary()
{
    // # source: formula_reference — service I-MR / Xbar-R with Test 2 only.
    auto build_imr = [](const std::vector<std::string>& values) {
        datalab::domain::DataTable table;
        table.columns = {"Y"};
        for (const auto& value : values) {
            table.rows.push_back({value});
        }
        datalab::domain::AnalysisConfiguration config;
        config.variable_columns = {0};
        config.selection.measurement_column = 0;
        config.control.enabled_special_cause_tests = {2};
        config.control.special_cause_rule_policy = "explicit";
        return datalab::application::AnalysisService::individuals_moving_range(table, config);
    };

    std::vector<std::string> baseline;
    for (int index = 0; index < 10; ++index) {
        baseline.push_back(index % 2 == 0 ? "10.0" : "10.2");
    }
    auto eight_above = baseline;
    for (int index = 0; index < 8; ++index) {
        eight_above.push_back("10.5");
    }
    const auto eight_page = build_imr(eight_above);
    QVERIFY(!eight_page.plots.empty());
    QVERIFY(!plot_triggered_test(eight_page.plots.front(), 2));

    auto nine_above = baseline;
    for (int index = 0; index < 9; ++index) {
        nine_above.push_back("10.5");
    }
    const auto nine_page = build_imr(nine_above);
    QVERIFY(plot_triggered_test(nine_page.plots.front(), 2));

    datalab::domain::DataTable xbar_table;
    xbar_table.columns = {"Y"};
    for (int subgroup = 0; subgroup < 8; ++subgroup) {
        xbar_table.rows.push_back({"10.5"});
        xbar_table.rows.push_back({"10.7"});
    }
    datalab::domain::AnalysisConfiguration xbar_config;
    xbar_config.variable_columns = {0};
    xbar_config.selection.measurement_column = 0;
    xbar_config.control.subgroup_size = 2;
    xbar_config.control.enabled_special_cause_tests = {2};
    xbar_config.control.special_cause_rule_policy = "explicit";
    const auto xbar_eight =
        datalab::application::AnalysisService::xbar_range(xbar_table, xbar_config);
    QVERIFY(!plot_triggered_test(xbar_eight.plots.front(), 2));

    xbar_table.rows.push_back({"10.5"});
    xbar_table.rows.push_back({"10.7"});
    const auto xbar_nine =
        datalab::application::AnalysisService::xbar_range(xbar_table, xbar_config);
    QVERIFY(plot_triggered_test(xbar_nine.plots.front(), 2));
}

void SpecialCauseRuleTest::imrAndXbarServiceTestFiveSixEightBoundary()
{
    // # source: formula_reference — service layer for Tests 5, 6, 8 boundaries.
    auto build_imr = [](const std::vector<std::string>& values, int test) {
        datalab::domain::DataTable table;
        table.columns = {"Y"};
        for (const auto& value : values) {
            table.rows.push_back({value});
        }
        datalab::domain::AnalysisConfiguration config;
        config.variable_columns = {0};
        config.selection.measurement_column = 0;
        config.control.enabled_special_cause_tests = {test};
        config.control.special_cause_rule_policy = "explicit";
        return datalab::application::AnalysisService::individuals_moving_range(table, config);
    };

    std::vector<std::string> baseline;
    for (int index = 0; index < 12; ++index) {
        baseline.push_back(index % 2 == 0 ? "10.0" : "10.2");
    }

    auto test_five_fail = baseline;
    test_five_fail.push_back("10.0");
    test_five_fail.push_back("10.0");
    test_five_fail.push_back("10.0");
    const auto five_fail_page = build_imr(test_five_fail, 5);
    QVERIFY(!plot_triggered_test(five_fail_page.plots.front(), 5));

    auto test_five_pass = baseline;
    test_five_pass.push_back("10.0");
    test_five_pass.push_back("12.0");
    test_five_pass.push_back("12.1");
    const auto five_pass_page = build_imr(test_five_pass, 5);
    QVERIFY(plot_triggered_test(five_pass_page.plots.front(), 5));

    auto test_six_fail = baseline;
    for (int index = 0; index < 5; ++index) {
        test_six_fail.push_back("10.0");
    }
    const auto six_fail_page = build_imr(test_six_fail, 6);
    QVERIFY(!plot_triggered_test(six_fail_page.plots.front(), 6));

    auto test_six_pass = baseline;
    test_six_pass.push_back("10.6");
    test_six_pass.push_back("10.7");
    test_six_pass.push_back("10.8");
    test_six_pass.push_back("10.9");
    test_six_pass.push_back("10.0");
    const auto six_pass_page = build_imr(test_six_pass, 6);
    QVERIFY(plot_triggered_test(six_pass_page.plots.front(), 6));

    auto test_eight_fail = baseline;
    for (int index = 0; index < 8; ++index) {
        test_eight_fail.push_back("10.2");
    }
    const auto eight_fail_page = build_imr(test_eight_fail, 8);
    QVERIFY(!plot_triggered_test(eight_fail_page.plots.front(), 8));

    auto test_eight_pass = baseline;
    for (int index = 0; index < 8; ++index) {
        test_eight_pass.push_back("10.6");
    }
    const auto eight_pass_page = build_imr(test_eight_pass, 8);
    QVERIFY(plot_triggered_test(eight_pass_page.plots.front(), 8));
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
