#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/quality_types.h"
#include "domain/statistics/control_charts.h"
#include "domain/statistics/inference_extensions.h"
#include "domain/statistics/process_capability.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

class CapabilityCiGofRareEventPowerTest final : public QObject {
    Q_OBJECT

private slots:
    void capabilityIntervalsMatchChiSquareAndBissell();
    void capabilityOneSidedLeavesMissingSideEmpty();
    void capabilityZeroSigmaDoesNotInventIntervals();
    void chiSquareGofMatchesMinitabExample();
    void gAndTChartsHaveLimitsTest1AndSourceRows();
    void capabilityTablesIncludeIntervalColumns();
    void chiSquareAssociationHeatmapUnchanged();
    void chiSquareGofServiceCountsMissingAndRejectsBadProportions();
    void tPowerFillsFactsActualPowerAndCurve();
    void interpretationAvoidsPassLanguage();
};

void CapabilityCiGofRareEventPowerTest::capabilityIntervalsMatchChiSquareAndBissell()
{
    // # source: formula_reference
    // Cp=1, ν=24, α=0.05: χ²_{0.025,24}≈12.40115, χ²_{0.975,24}≈39.36408
    // Cp·√(χ²/ν) → [0.7188, 1.2807]
    // Bissell: 1 ± 1.959964·√(1/(9·25)+1/(2·24)) → [0.6884, 1.3116]
    using datalab::domain::statistics::ProcessCapability;
    using datalab::domain::statistics::fill_capability_index_intervals;
    datalab::domain::SpecificationLimits specs;
    specs.lower = 7.0;
    specs.upper = 13.0;
    auto result = ProcessCapability::calculate(10.0, 1.0, 1.0, specs);
    result.sample_size = 25;
    fill_capability_index_intervals(result, 0.95);
    QVERIFY(result.cp.has_value());
    QCOMPARE(*result.cp, 1.0);
    QVERIFY(result.cpk.has_value());
    QCOMPARE(*result.cpk, 1.0);
    QVERIFY(result.cp_lower.has_value());
    QVERIFY(result.cp_upper.has_value());
    QVERIFY(result.cpk_lower.has_value());
    QVERIFY(result.cpk_upper.has_value());
    QVERIFY(qAbs(*result.cp_lower - 0.7188) < 0.005);
    QVERIFY(qAbs(*result.cp_upper - 1.2807) < 0.005);
    QVERIFY(qAbs(*result.cpk_lower - 0.6884) < 0.005);
    QVERIFY(qAbs(*result.cpk_upper - 1.3116) < 0.005);
    QCOMPARE(result.capability_ci_method, std::string("chi_square_cp_pp_bissell_cpk_ppk"));
}

void CapabilityCiGofRareEventPowerTest::capabilityOneSidedLeavesMissingSideEmpty()
{
    using datalab::domain::statistics::ProcessCapability;
    using datalab::domain::statistics::fill_capability_index_intervals;
    datalab::domain::SpecificationLimits specs;
    specs.lower = 7.0;
    auto result = ProcessCapability::calculate(10.0, 1.0, 1.0, specs);
    result.sample_size = 25;
    fill_capability_index_intervals(result, 0.95);
    QVERIFY(result.cpl.has_value());
    QVERIFY(result.cpl_lower.has_value());
    QVERIFY(!result.cpu.has_value());
    QVERIFY(!result.cpu_lower.has_value());
    QVERIFY(!result.cpu_upper.has_value());
    QVERIFY(!result.cp.has_value());
}

void CapabilityCiGofRareEventPowerTest::capabilityZeroSigmaDoesNotInventIntervals()
{
    using datalab::domain::statistics::ProcessCapability;
    datalab::domain::SpecificationLimits specs;
    specs.lower = 0.0;
    specs.upper = 10.0;
    const auto result = ProcessCapability::calculate(
        std::vector<double>(8, 5.0), 0.0, specs);
    QVERIFY(!result.cp.has_value());
    QVERIFY(!result.cp_lower.has_value());
    QVERIFY(!result.cpk_lower.has_value());
}

void CapabilityCiGofRareEventPowerTest::chiSquareGofMatchesMinitabExample()
{
    // # source: formula_reference  Minitab GOF example N=40, p=0.1/0.2/0.3/0.4
    // E=(4,8,12,16); χ²=0.25+6.125+1/3+2.25=8.9583; DF=3
    const auto result = datalab::domain::statistics::chi_square_goodness_of_fit(
        {"A", "B", "C", "D"}, {5.0, 15.0, 10.0, 10.0}, {0.1, 0.2, 0.3, 0.4});
    QVERIFY(qAbs(result.pearson_statistic - 8.9583) < 1.0e-4);
    QCOMPARE(result.degrees_of_freedom, 3.0);
    QVERIFY(result.p_value.has_value());
    const auto mismatched = datalab::domain::statistics::chi_square_goodness_of_fit(
        {"A", "B"}, {10.0, 10.0}, {0.5});
    QVERIFY(!mismatched.diagnostics.empty());
}

void CapabilityCiGofRareEventPowerTest::gAndTChartsHaveLimitsTest1AndSourceRows()
{
    std::vector<double> intervals(20, 1.0);
    intervals.push_back(100.0);
    std::vector<datalab::domain::RowId> rows;
    rows.reserve(intervals.size());
    for (std::size_t index = 0; index < intervals.size(); ++index) {
        rows.push_back(static_cast<datalab::domain::RowId>(3 + 2 * index));
    }
    const auto g = datalab::domain::statistics::ControlCharts::g_chart(
        intervals, rows, {});
    QVERIFY(!g.upper_control_limit.empty());
    QVERIFY(std::isfinite(g.upper_control_limit.front()));
    QVERIFY(std::isfinite(g.center_line.front()));
    QVERIFY(!g.test1_points.empty());
    QCOMPARE(g.source_rows, rows);
    QVERIFY(g.source_rows.front() != datalab::domain::RowId{0});

    const auto t = datalab::domain::statistics::ControlCharts::t_chart(
        {1.2, 0.0, 2.5, 3.1, 4.0}, {10, 11, 12, 13, 14}, {});
    QVERIFY(std::any_of(
        t.diagnostics.cbegin(), t.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "zero_interval_regression_used";
        }));
    QVERIFY(!t.upper_control_limit.empty());
    QVERIFY(std::isfinite(t.upper_control_limit.front()));
}

void CapabilityCiGofRareEventPowerTest::capabilityTablesIncludeIntervalColumns()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    for (int index = 0; index < 12; ++index) {
        table.rows.push_back({std::to_string(9.5 + 0.1 * index)});
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.specifications.lower = 7.0;
    configuration.specifications.upper = 13.0;
    const auto page = datalab::application::AnalysisService::capability(table, configuration);
    bool found_potential = false;
    bool found_overall = false;
    for (const auto& statistic_table : page.tables) {
        if (statistic_table.title.find("Potential") != std::string::npos) {
            found_potential = true;
            QCOMPARE(statistic_table.headers,
                     (std::vector<std::string>{"指标", "估计", "下限", "上限"}));
        }
        if (statistic_table.title.find("Overall") != std::string::npos
            && statistic_table.headers.size() == 4) {
            found_overall = true;
            QCOMPARE(statistic_table.headers,
                     (std::vector<std::string>{"指标", "估计", "下限", "上限"}));
        }
    }
    QVERIFY(found_potential);
    QVERIFY(found_overall);

    const auto sixpack =
        datalab::application::AnalysisService::capability_sixpack(table, configuration);
    QCOMPARE(sixpack.plots.size(), std::size_t{6});
    QCOMPARE(sixpack.plots[0].title, std::string{"I 图"});
    QCOMPARE(sixpack.plots[1].title, std::string{"过程能力直方图"});
    QCOMPARE(sixpack.plots[2].title, std::string{"MR 图"});
    QCOMPARE(sixpack.plots[3].title, std::string{"正态概率图"});
    QCOMPARE(sixpack.plots[4].title, std::string{"最近 25 个观测"});
    QCOMPARE(sixpack.plots[5].title, std::string{"能力图"});
}

void CapabilityCiGofRareEventPowerTest::chiSquareAssociationHeatmapUnchanged()
{
    datalab::domain::DataTable table;
    table.columns = {"Row", "Col"};
    table.rows = {{"A", "X"}, {"A", "Y"}, {"B", "X"}, {"B", "Y"}, {"A", "X"}, {"*", "Y"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.inference.row_category_column = 0;
    configuration.inference.column_category_column = 1;
    const auto page = datalab::application::AnalysisService::chi_square(table, configuration);
    QCOMPARE(page.tables.size(), std::size_t{3});
    QVERIFY(page.facts.chi_square.has_value());
    QVERIFY(page.facts.chi_square->plot_available);
    QVERIFY(!page.plots.empty());
}

void CapabilityCiGofRareEventPowerTest::chiSquareGofServiceCountsMissingAndRejectsBadProportions()
{
    datalab::domain::DataTable table;
    table.columns = {"Cat"};
    table.rows = {{"A"}, {"*"}, {"B"}, {"A"}, {"C"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.inference.gof_category_column = 0;
    configuration.inference.expected_proportions = "0.5,0.5";
    const auto page = datalab::application::AnalysisService::chi_square_gof(table, configuration);
    QVERIFY(page.facts.chi_square_gof.has_value());
    QCOMPARE(page.facts.chi_square_gof->missing_count, std::size_t{1});
    QCOMPARE(page.facts.chi_square_gof->category_count, std::size_t{3});
    QVERIFY(page.facts.chi_square_gof->minimum_expected_count.has_value());
    QVERIFY(!page.facts.chi_square_gof->validity_status.empty());
    QVERIFY(std::any_of(
        page.diagnostics.cbegin(), page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "proportion_count_mismatch";
        }));
}

void CapabilityCiGofRareEventPowerTest::tPowerFillsFactsActualPowerAndCurve()
{
    datalab::domain::DataTable table;
    datalab::domain::AnalysisConfiguration configuration;
    configuration.power.mode = "one_sample_sample_size";
    configuration.power.effect_size = 0.5;
    configuration.power.target = 0.8;
    configuration.power.alpha = 0.05;
    const auto page = datalab::application::AnalysisService::t_power(table, configuration);
    QCOMPARE(page.tables.size(), std::size_t{1});
    QVERIFY(std::find(page.tables.front().headers.cbegin(),
                      page.tables.front().headers.cend(),
                      "Actual Power")
            != page.tables.front().headers.cend());
    QVERIFY(page.facts.power.has_value());
    QVERIFY(page.facts.power->actual_power.has_value());
    QVERIFY(!page.plots.empty());
    QVERIFY(!page.plots.front().series.empty());
    QVERIFY(page.plots.front().series.front().values.size() > 2);
}

void CapabilityCiGofRareEventPowerTest::interpretationAvoidsPassLanguage()
{
    datalab::domain::OutputPage page;
    page.method_name = "T Test Sample Size";
    page.configuration.power.alpha = 0.05;
    page.configuration.power.target = 0.8;
    page.facts.power = datalab::domain::PowerFacts{};
    page.facts.power->effect_size = 0.5;
    page.facts.power->actual_power = 0.81;
    page.facts.power->power = 0.81;
    page.facts.power->sample_size = 34;
    page.facts.power->target = 0.8;
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
            QVERIFY(bullet.find("通过") == std::string::npos);
            QVERIFY(bullet.find("已证明") == std::string::npos);
            QVERIFY(bullet.find("足够") == std::string::npos);
        }
    }
}

QTEST_APPLESS_MAIN(CapabilityCiGofRareEventPowerTest)
#include "capability_ci_gof_rare_event_power_test.moc"
