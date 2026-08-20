#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/control_charts.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

class EwmaCusumOutputTest final : public QObject {
    Q_OBJECT

private slots:
    void ewmaFormulaReferenceLimitsVaryByPoint();
    void ewmaServiceBuildsPointTableAndFacts();
    void ewmaHistoricalSigmaLabel();
    void cusumFormulaReferenceNoSignalOnShortSeries();
    void cusumServiceListsAllSignals();
    void cusumFirDomainOption();
};

void EwmaCusumOutputTest::ewmaFormulaReferenceLimitsVaryByPoint()
{
    // # source: formula_reference — docs/research/spc-control-charts.md §6 hand example.
    datalab::domain::statistics::EwmaOptions options;
    options.lambda = 0.2;
    options.limit_sigma = 3.0;
    options.historical_mean = 10.0;
    options.historical_sigma = 1.0;
    options.special_causes.enabled_tests = {1};
    options.special_causes.policy = "explicit";
    const auto chart = datalab::domain::statistics::ControlCharts::ewma_chart(
        {10.0, 11.0, 10.0}, options);
    QCOMPARE(chart.plotted_values.size(), std::size_t{3});
    QVERIFY(std::abs(chart.plotted_values[1] - 10.2) < 1.0e-12);
    QVERIFY(std::abs(chart.plotted_values[2] - 10.16) < 1.0e-12);
    QVERIFY(chart.point_sigma.size() >= 2);
    QVERIFY(std::abs(chart.point_sigma[1] - 0.2) < 1.0e-12);
    QVERIFY(std::abs(chart.point_sigma[2] - 0.2939) < 1.0e-3);
    QVERIFY(std::abs(chart.lower_control_limit[1] - 9.4) < 1.0e-12);
    QVERIFY(std::abs(chart.upper_control_limit[1] - 10.6) < 1.0e-12);
}

void EwmaCusumOutputTest::ewmaServiceBuildsPointTableAndFacts()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    table.rows = {{"10"}, {"11"}, {"10"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.control.ewma_lambda = 0.2;
    configuration.control.ewma_limit_sigma = 3.0;
    configuration.control.historical_center = 10.0;
    configuration.control.historical_sigma = 1.0;
    configuration.control.enabled_special_cause_tests = {1};
    configuration.control.special_cause_rule_policy = "explicit";
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::ewma(table, configuration);
    const auto parameters = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "EWMA 参数";
        });
    const auto point_table = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "EWMA 逐点统计";
        });
    QVERIFY(parameters != page.tables.cend());
    QVERIFY(point_table != page.tables.cend());
    QCOMPARE(point_table->rows.size(), std::size_t{3});
    QCOMPARE(point_table->headers.size(), std::size_t{9});
    QVERIFY(page.facts.spc.has_value());
    QVERIFY(page.facts.spc->sigma_within.has_value());
    QVERIFY(page.facts.spc->out_of_control_count.has_value());
}

void EwmaCusumOutputTest::ewmaHistoricalSigmaLabel()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    table.rows = {{"10"}, {"10.1"}, {"9.9"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.control.historical_center = 10.0;
    configuration.control.historical_sigma = 1.0;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::ewma(table, configuration);
    QVERIFY(page.parameter_summary.find("历史参数") != std::string::npos);
    const auto parameters = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "EWMA 参数";
        });
    QVERIFY(parameters != page.tables.cend());
    const auto source_row = std::find_if(
        parameters->rows.cbegin(), parameters->rows.cend(),
        [](const std::vector<std::string>& row) { return row.front() == "参数来源"; });
    QVERIFY(source_row != parameters->rows.cend());
    QCOMPARE((*source_row)[1], std::string("历史参数"));
}

void EwmaCusumOutputTest::cusumFormulaReferenceNoSignalOnShortSeries()
{
    // # source: formula_reference — docs/research/spc-control-charts.md §7 hand example.
    datalab::domain::statistics::CusumOptions options;
    options.target = 10.0;
    options.sigma = 1.0;
    options.k = 0.5;
    options.h = 4.0;
    const auto chart = datalab::domain::statistics::ControlCharts::cusum_chart(
        {10.0, 11.0, 10.0, 12.0}, options);
    QVERIFY(chart.upper_signal_points.empty());
    QVERIFY(chart.lower_signal_points.empty());
    QVERIFY(std::abs(chart.primary.plotted_values[1] - 0.5) < 1.0e-12);
}

void EwmaCusumOutputTest::cusumServiceListsAllSignals()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    for (int index = 0; index < 12; ++index) {
        table.rows.push_back({std::to_string(15.0)});
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.control.cusum_target = 10.0;
    configuration.control.cusum_sigma = 1.0;
    configuration.control.cusum_k = 0.5;
    configuration.control.cusum_h = 4.0;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::cusum(table, configuration);
    const auto point_table = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "CUSUM 逐点统计";
        });
    const auto signal_table = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "CUSUM 信号";
        });
    QVERIFY(point_table != page.tables.cend());
    QVERIFY(signal_table != page.tables.cend());
    QVERIFY(signal_table->rows.size() > 1);
    QVERIFY(page.facts.spc.has_value());
    QVERIFY(page.facts.spc->out_of_control_count.has_value());
    QVERIFY(*page.facts.spc->out_of_control_count >= std::size_t{1});

    datalab::domain::OutputPage interpreted = page;
    datalab::application::InterpretationService::enrich(interpreted);
    for (const auto& section : interpreted.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("必须删") == std::string::npos);
        }
    }
}

void EwmaCusumOutputTest::cusumFirDomainOption()
{
    datalab::domain::statistics::CusumOptions options;
    options.target = 10.0;
    options.sigma = 1.0;
    options.k = 0.5;
    options.h = 4.0;
    const auto plain = datalab::domain::statistics::ControlCharts::cusum_chart(
        {10.0, 10.0, 10.0}, options);
    options.fast_initial_response = true;
    const auto fir = datalab::domain::statistics::ControlCharts::cusum_chart(
        {10.0, 10.0, 10.0}, options);
    QVERIFY(fir.primary.plotted_values.front()
            > plain.primary.plotted_values.front());
}

QTEST_APPLESS_MAIN(EwmaCusumOutputTest)

#include "ewma_cusum_output_test.moc"
