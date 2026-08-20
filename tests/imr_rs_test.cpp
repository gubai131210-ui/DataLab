#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/control_charts.h"
#include "domain/statistics/process_capability.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

class ImrRsTest final : public QObject {
    Q_OBJECT

private slots:
    void individualsMatchImrOnSubgroupMeans();
    void sigmaMatchesBetweenWithinForN5();
    void n9UsesSChart();
    void rejectsMissingSubgroups();
    void hoverRowsAreSubgroupFirstRows();
    void imrPointTableKeepsSourceRowsAndHistoricalLabel();
    void imrStageLabelsBreakMovingRange();
    void imrRsSubgroupPointTable();
    void sigmaMatchesBetweenWithinForN9();
};

void ImrRsTest::individualsMatchImrOnSubgroupMeans()
{
    // # source: formula_reference — I/MR of subgroup means equals I-MR on those means.
    const std::vector<std::vector<double>> subgroups{
        {10.0, 11.0, 12.0, 13.0, 14.0},
        {12.0, 13.0, 14.0, 15.0, 16.0}};
    const auto triple =
        datalab::domain::statistics::ControlCharts::imr_rs_triple(subgroups);
    const auto dual =
        datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(
            {12.0, 14.0});
    QCOMPARE(triple.individuals.plotted_values.size(), dual.primary.plotted_values.size());
    QCOMPARE(triple.moving_range.plotted_values.size(), dual.secondary.plotted_values.size());
    for (std::size_t index = 0; index < dual.primary.plotted_values.size(); ++index) {
        QVERIFY(std::abs(triple.individuals.plotted_values[index]
                         - dual.primary.plotted_values[index])
                < 1.0e-12);
        QVERIFY(std::abs(triple.moving_range.plotted_values[index]
                         - dual.secondary.plotted_values[index])
                < 1.0e-12);
    }
    QVERIFY(std::abs(triple.sigma_xbar - dual.sigma) < 1.0e-12);
}

void ImrRsTest::sigmaMatchesBetweenWithinForN5()
{
    // # source: formula_reference — n=5 uses R̄/d2, same as between_within.
    const std::vector<std::vector<double>> subgroups{
        {10.0, 11.0, 12.0, 13.0, 14.0},
        {12.0, 13.0, 14.0, 15.0, 16.0},
        {11.0, 12.0, 13.0, 14.0, 15.0}};
    std::vector<double> observations;
    for (const auto& subgroup : subgroups) {
        observations.insert(observations.end(), subgroup.cbegin(), subgroup.cend());
    }
    const auto triple =
        datalab::domain::statistics::ControlCharts::imr_rs_triple(subgroups);
    const auto capability =
        datalab::domain::statistics::ProcessCapability::calculate_between_within(
            observations, subgroups, {});
    QVERIFY(capability.subgroup_within_standard_deviation.has_value());
    QVERIFY(capability.between_standard_deviation.has_value());
    QVERIFY(capability.between_within_standard_deviation.has_value());
    QVERIFY(std::abs(triple.sigma_within
                     - *capability.subgroup_within_standard_deviation)
            < 1.0e-12);
    QVERIFY(std::abs(triple.sigma_between - *capability.between_standard_deviation)
            < 1.0e-12);
    QVERIFY(std::abs(triple.sigma_between_within
                     - *capability.between_within_standard_deviation)
            < 1.0e-12);
    QCOMPARE(triple.within_chart, std::string("range"));
    QCOMPARE(triple.within.plotted_values.size(), subgroups.size());
}

void ImrRsTest::n9UsesSChart()
{
    std::vector<std::vector<double>> subgroups(2);
    for (std::size_t value = 1; value <= 9; ++value) {
        subgroups[0].push_back(static_cast<double>(value));
        subgroups[1].push_back(static_cast<double>(value + 2));
    }
    const auto triple =
        datalab::domain::statistics::ControlCharts::imr_rs_triple(subgroups);
    QCOMPARE(triple.within_chart, std::string("stdev"));
    QCOMPARE(triple.method, std::string("S̄ / c4"));
    QCOMPARE(triple.within.plotted_values.size(), std::size_t{2});

    datalab::domain::DataTable table;
    table.columns = {"Y"};
    for (const auto& subgroup : subgroups) {
        for (const double value : subgroup) {
            table.rows.push_back({std::to_string(value)});
        }
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.selection.measurement_column = 0;
    configuration.control.subgroup_size = 9;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::imr_rs(table, configuration);
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title == "S 图";
                        }));
}

void ImrRsTest::rejectsMissingSubgroups()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    table.rows = {{"1"}, {"2"}, {"3"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.selection.measurement_column = 0;
    configuration.control.subgroup_size = 1;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::imr_rs(table, configuration);
    QVERIFY(page.plots.empty());
    QVERIFY(std::any_of(
        page.diagnostics.cbegin(), page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.severity
                == datalab::domain::DiagnosticMessage::Severity::error;
        }));
}

void ImrRsTest::hoverRowsAreSubgroupFirstRows()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    for (int value = 0; value < 10; ++value) {
        table.rows.push_back({std::to_string(10 + value)});
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.selection.measurement_column = 0;
    configuration.control.subgroup_size = 5;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::imr_rs(table, configuration);
    QCOMPARE(page.plots.size(), std::size_t{3});
    for (const auto& plot : page.plots) {
        QCOMPARE(plot.source_rows.size(), std::size_t{2});
        QCOMPARE(plot.source_rows[0], std::size_t{0});
        QCOMPARE(plot.source_rows[1], std::size_t{5});
    }
    QVERIFY(page.facts.spc.has_value());
    QVERIFY(page.facts.spc->sigma_within.has_value());
    datalab::domain::OutputPage interpreted = page;
    datalab::application::InterpretationService::enrich(interpreted);
    for (const auto& section : interpreted.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
        }
    }
}

void ImrRsTest::imrPointTableKeepsSourceRowsAndHistoricalLabel()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    table.rows = {{"*"}, {"10"}, {"10.1"}, {"9.9"}, {"10.2"}, {"10.0"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.control.historical_center = 10.0;
    configuration.control.historical_sigma = 1.0;
    configuration.control.enabled_special_cause_tests = {1};
    configuration.control.special_cause_rule_policy = "explicit";
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::individuals_moving_range(
            table, configuration);
    const auto point = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "I-MR 逐点统计";
        });
    QVERIFY(point != page.tables.cend());
    QCOMPARE(point->rows.size(), std::size_t{5});
    QCOMPARE(point->rows.front()[0], std::string("2"));
    QVERIFY(page.parameter_summary.find("历史参数") != std::string::npos);
    QCOMPARE(page.plots.front().source_rows.front(), std::size_t{1});
}

void ImrRsTest::imrStageLabelsBreakMovingRange()
{
    datalab::domain::DataTable table;
    table.columns = {"Y", "Stage"};
    table.rows = {
        {"10.0", "A"}, {"10.1", "A"}, {"10.2", "A"},
        {"20.0", "B"}, {"20.1", "B"}, {"20.2", "B"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.control.stage_column = 1;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::individuals_moving_range(
            table, configuration);
    QCOMPARE(page.plots.front().point_groups.size(), std::size_t{6});
    QCOMPARE(page.plots.front().point_groups[2], std::string("A"));
    QCOMPARE(page.plots.front().point_groups[3], std::string("B"));
    const auto point = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "I-MR 逐点统计";
        });
    QVERIFY(point != page.tables.cend());
    QCOMPARE(point->rows[3][1], std::string("B"));
    QCOMPARE(point->rows[3][6], std::string("*"));
}

void ImrRsTest::imrRsSubgroupPointTable()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    for (int value = 0; value < 10; ++value) {
        table.rows.push_back({std::to_string(10 + value)});
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.selection.measurement_column = 0;
    configuration.control.subgroup_size = 5;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::imr_rs(table, configuration);
    const auto points = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "I-MR-R/S 逐子组统计";
        });
    QVERIFY(points != page.tables.cend());
    QCOMPARE(points->rows.size(), std::size_t{2});
    QCOMPARE(points->rows.front()[0], std::string("1"));
    QCOMPARE(points->rows[1][0], std::string("6"));
}

void ImrRsTest::sigmaMatchesBetweenWithinForN9()
{
    // # source: formula_reference — n>=9 uses S̄/c4 for both I-MR-R/S and between/within.
    std::vector<std::vector<double>> subgroups(3);
    for (std::size_t value = 1; value <= 9; ++value) {
        subgroups[0].push_back(static_cast<double>(value));
        subgroups[1].push_back(static_cast<double>(value + 1));
        subgroups[2].push_back(static_cast<double>(value + 2));
    }
    std::vector<double> observations;
    for (const auto& subgroup : subgroups) {
        observations.insert(observations.end(), subgroup.cbegin(), subgroup.cend());
    }
    const auto triple =
        datalab::domain::statistics::ControlCharts::imr_rs_triple(subgroups);
    const auto capability =
        datalab::domain::statistics::ProcessCapability::calculate_between_within(
            observations, subgroups, {});
    QCOMPARE(triple.method, std::string("S̄ / c4"));
    QCOMPARE(capability.within_sigma_method, std::string("S̄ / c4"));
    QVERIFY(std::abs(triple.sigma_within
                     - *capability.subgroup_within_standard_deviation)
            < 1.0e-12);
    QVERIFY(std::abs(triple.sigma_between_within
                     - *capability.between_within_standard_deviation)
            < 1.0e-12);
}

QTEST_APPLESS_MAIN(ImrRsTest)

#include "imr_rs_test.moc"
