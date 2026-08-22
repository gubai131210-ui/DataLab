#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/control_charts.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class ZoneZmrMaOutputTest final : public QObject {
    Q_OBJECT

private slots:
    void zoneFormulaReferenceNoSignalWithinOneSigma();
    void zoneServiceBuildsScoreTableAndFacts();
    void zmrFormulaReferenceConstantColumnNearZero();
    void zmrServiceBuildsDualChartsAndFacts();
    void movingAverageFormulaReferenceConstantColumnInControl();
    void movingAverageServiceBuildsPointTableAndFacts();
    void factsRoundTripThroughSerialization();
    void interpretationAvoidsStabilityClaims();
};

void ZoneZmrMaOutputTest::zoneFormulaReferenceNoSignalWithinOneSigma()
{
    // # source: formula_reference — all points within ±1σ → no Jaehn score ≥ 8.
    const std::vector<double> observations = {10.0, 10.1, 9.9, 10.05, 9.95, 10.02};
    datalab::domain::statistics::ZoneChartOptions options;
    const auto result = datalab::domain::statistics::ControlCharts::zone_chart(
        observations, options);
    QCOMPARE(result.signal_points.size(), std::size_t{0});
    for (const double score : result.zone_scores) {
        QVERIFY(score < 8.0);
    }
}

void ZoneZmrMaOutputTest::zoneServiceBuildsScoreTableAndFacts()
{
    DataTable table;
    table.columns = {"Value"};
    table.rows = {{"10"}, {"10.1"}, {"9.9"}, {"10.05"}, {"9.95"}};
    AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.control.moving_range_length = 2;
    const auto page = datalab::application::AnalysisService::zone_chart(table, configuration);
    QVERIFY(page.facts.zone_chart.has_value());
    QVERIFY(page.facts.spc.has_value());
    QCOMPARE(page.facts.zone_chart->signal_count, std::size_t{0});
    QCOMPARE(page.plots.size(), std::size_t{2});
    bool has_score_table = false;
    for (const auto& table_out : page.tables) {
        if (table_out.title == "区域图逐点统计") {
            has_score_table = true;
            QVERIFY(!table_out.rows.empty());
        }
    }
    QVERIFY(has_score_table);
}

void ZoneZmrMaOutputTest::zmrFormulaReferenceConstantColumnNearZero()
{
    // # source: formula_reference — constant column → Z ≈ 0.
    const std::vector<double> observations = {5.0, 5.0, 5.0, 5.0, 5.0};
    const auto result = datalab::domain::statistics::ControlCharts::z_mr_chart(
        observations, {});
    QCOMPARE(result.z_values.size(), observations.size());
    for (const double z : result.z_values) {
        QVERIFY(std::abs(z) < 1.0e-10);
    }
}

void ZoneZmrMaOutputTest::zmrServiceBuildsDualChartsAndFacts()
{
    DataTable table;
    table.columns = {"Value", "Product"};
    table.rows = {{"5", "A"}, {"5.1", "A"}, {"4.9", "B"}, {"5", "B"}};
    AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.by_column = 1;
    const auto page = datalab::application::AnalysisService::z_mr(table, configuration);
    QVERIFY(page.facts.z_mr.has_value());
    QCOMPARE(page.facts.z_mr->group_count, std::size_t{2});
    QCOMPARE(page.plots.size(), std::size_t{2});
    bool has_point_table = false;
    for (const auto& table_out : page.tables) {
        if (table_out.title == "Z-MR 逐点统计") {
            has_point_table = true;
        }
    }
    QVERIFY(has_point_table);
}

void ZoneZmrMaOutputTest::movingAverageFormulaReferenceConstantColumnInControl()
{
    // # source: formula_reference — constant column → MA = constant, no Test 1 breach.
    const std::vector<double> observations = {3.0, 3.0, 3.0, 3.0, 3.0};
    datalab::domain::statistics::MovingAverageOptions options;
    options.window = 3;
    const auto chart = datalab::domain::statistics::ControlCharts::moving_average_chart(
        observations, options);
    QCOMPARE(chart.test1_points.size(), std::size_t{0});
    for (std::size_t index = 0; index < chart.plotted_values.size(); ++index) {
        if (std::isfinite(chart.plotted_values[index])) {
            QCOMPARE(chart.plotted_values[index], 3.0);
        }
    }
}

void ZoneZmrMaOutputTest::movingAverageServiceBuildsPointTableAndFacts()
{
    DataTable table;
    table.columns = {"Value"};
    for (int index = 0; index < 6; ++index) {
        table.rows.push_back({"3"});
    }
    AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.control.ma_window = 3;
    const auto page = datalab::application::AnalysisService::moving_average(
        table, configuration);
    QVERIFY(page.facts.moving_average.has_value());
    QCOMPARE(page.facts.moving_average->window, 3);
    QVERIFY(page.facts.spc.has_value());
    QCOMPARE(page.facts.spc->out_of_control_count, std::size_t{0});
    QCOMPARE(page.plots.size(), std::size_t{1});
}

void ZoneZmrMaOutputTest::factsRoundTripThroughSerialization()
{
    datalab::domain::OutputPage page;
    page.id = "zone_zmr_ma_test";
    page.title = "Test";
    page.configuration.control.ma_window = 4;
    page.facts.zone_chart = datalab::domain::ZoneChartFacts{};
    page.facts.zone_chart->n = 5;
    page.facts.zone_chart->signal_count = 1;
    page.facts.zone_chart->signal_threshold = 8.0;
    page.facts.z_mr = datalab::domain::ZmrFacts{};
    page.facts.z_mr->group_count = 2;
    page.facts.z_mr->z_out_of_control_count = 0;
    page.facts.moving_average = datalab::domain::MovingAverageChartFacts{};
    page.facts.moving_average->window = 4;
    page.facts.moving_average->out_of_control_count = 0;
    page.facts.spc = datalab::domain::SpcFacts{};
    page.facts.spc->out_of_control_count = 0;

    const QJsonObject json = datalab::infrastructure::output_page_to_json(page);
    const auto restored = datalab::infrastructure::output_page_from_json(json);
    QVERIFY(restored.facts.zone_chart.has_value());
    QCOMPARE(restored.facts.zone_chart->signal_count, std::size_t{1});
    QVERIFY(restored.facts.z_mr.has_value());
    QCOMPARE(restored.facts.z_mr->group_count, std::size_t{2});
    QVERIFY(restored.facts.moving_average.has_value());
    QCOMPARE(restored.facts.moving_average->window, 4);
    QCOMPARE(restored.configuration.control.ma_window, 4);
}

void ZoneZmrMaOutputTest::interpretationAvoidsStabilityClaims()
{
    DataTable table;
    table.columns = {"Value"};
    table.rows = {{"10"}, {"12"}, {"14"}, {"16"}, {"18"}, {"20"}};
    AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    const auto page = datalab::application::AnalysisService::zone_chart(table, configuration);
    datalab::domain::OutputPage enriched = page;
    datalab::application::InterpretationService::enrich(enriched);
    for (const auto& section : enriched.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("过程已失控") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
            QVERIFY(bullet.find("必须接收") == std::string::npos);
            QVERIFY(bullet.find("已证明同均值") == std::string::npos);
            QVERIFY(bullet.find("必须删除") == std::string::npos);
        }
    }
}

QTEST_MAIN(ZoneZmrMaOutputTest)
#include "zone_zmr_ma_output_test.moc"
