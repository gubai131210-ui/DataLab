#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/control_charts.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

class XbarOutputTest final : public QObject {
    Q_OBJECT

private slots:
    void subgroupTableKeepsSourceRowsAndStages();
    void secondaryRangeFailureAppearsInTriggeredTests();
    void spcFactsExposeSigmaWithinAndUnionCount();
    void imrPopulatesSpcFacts();
};

void XbarOutputTest::subgroupTableKeepsSourceRowsAndStages()
{
    datalab::domain::DataTable table;
    table.columns = {"Y", "Stage"};
    for (int subgroup = 0; subgroup < 3; ++subgroup) {
        for (int point = 0; point < 5; ++point) {
            const int row = subgroup * 5 + point;
            table.rows.push_back({
                std::to_string(10 + point),
                subgroup < 2 ? "A" : "B"});
        }
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.selection.measurement_column = 0;
    configuration.control.subgroup_size = 5;
    configuration.control.stage_column = 1;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::xbar_range(table, configuration);
    const auto subgroup_table = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Xbar-R 逐子组统计";
        });
    QVERIFY(subgroup_table != page.tables.cend());
    QCOMPARE(subgroup_table->headers.size(), std::size_t{14});
    QCOMPARE(subgroup_table->rows.size(), std::size_t{3});
    QCOMPARE(subgroup_table->rows.front()[0], std::string("1"));
    QCOMPARE(subgroup_table->rows[2][2], std::string("B"));
    QCOMPARE(page.plots.front().source_rows.size(), std::size_t{3});
    QCOMPARE(page.plots.front().source_rows.front(), std::size_t{0});
}

void XbarOutputTest::secondaryRangeFailureAppearsInTriggeredTests()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    for (int subgroup = 0; subgroup < 10; ++subgroup) {
        if (subgroup < 9) {
            for (int point = 0; point < 5; ++point) {
                table.rows.push_back({"10"});
            }
        } else {
            table.rows.push_back({"1"});
            table.rows.push_back({"19"});
            table.rows.push_back({"10"});
            table.rows.push_back({"10"});
            table.rows.push_back({"10"});
        }
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.selection.measurement_column = 0;
    configuration.control.subgroup_size = 5;
    configuration.control.enabled_special_cause_tests = {1};
    configuration.control.special_cause_rule_policy = "explicit";
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::xbar_range(table, configuration);
    const auto subgroup_table = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Xbar-R 逐子组统计";
        });
    QVERIFY(subgroup_table != page.tables.cend());
    const std::string& triggered = subgroup_table->rows.back()[12];
    QVERIFY2(triggered.find("R: Test 1") != std::string::npos,
             triggered.c_str());
    QCOMPARE(subgroup_table->rows.back()[13], std::string("R: Test 1"));
}

void XbarOutputTest::spcFactsExposeSigmaWithinAndUnionCount()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    for (int subgroup = 0; subgroup < 10; ++subgroup) {
        if (subgroup < 9) {
            for (int point = 0; point < 5; ++point) {
                table.rows.push_back({"10"});
            }
        } else {
            table.rows.push_back({"1"});
            table.rows.push_back({"19"});
            table.rows.push_back({"10"});
            table.rows.push_back({"10"});
            table.rows.push_back({"10"});
        }
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.selection.measurement_column = 0;
    configuration.control.subgroup_size = 5;
    configuration.control.enabled_special_cause_tests = {1};
    configuration.control.special_cause_rule_policy = "explicit";
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::xbar_range(table, configuration);
    QVERIFY(page.facts.spc.has_value());
    QVERIFY(page.facts.spc->sigma_within.has_value());
    QVERIFY(*page.facts.spc->sigma_within > 0.0);
    QVERIFY(page.facts.spc->out_of_control_count.has_value());
    QCOMPARE(*page.facts.spc->out_of_control_count, std::size_t{1});

    datalab::domain::OutputPage interpreted = page;
    datalab::application::InterpretationService::enrich(interpreted);
    for (const auto& section : interpreted.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
        }
    }
}

void XbarOutputTest::imrPopulatesSpcFacts()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    table.rows = {{"10"}, {"10.1"}, {"9.9"}, {"10.2"}, {"10.0"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.control.enabled_special_cause_tests = {1};
    configuration.control.special_cause_rule_policy = "explicit";
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::individuals_moving_range(
            table, configuration);
    QVERIFY(page.facts.spc.has_value());
    QVERIFY(page.facts.spc->sigma_within.has_value());
    QVERIFY(page.facts.spc->out_of_control_count.has_value());
}

QTEST_APPLESS_MAIN(XbarOutputTest)

#include "xbar_output_test.moc"
