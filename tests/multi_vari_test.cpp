#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/multi_vari.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <string>
#include <vector>

class MultiVariTest final : public QObject {
    Q_OBJECT

private slots:
    void computesFactorAndCellMeans();
    void diagnosesInsufficientCoverage();
    void serviceSkipsMissingAndBuildsTables();
    void interpretationDoesNotClaimCapability();
    void supportsFourFactorsWithSourceRows();
};

void MultiVariTest::computesFactorAndCellMeans()
{
    // # source: formula_reference
    const std::vector<double> measurements = {10.0, 12.0, 11.0, 20.0, 22.0, 24.0};
    const std::vector<std::vector<std::string>> factors = {
        {"A", "1"}, {"A", "1"}, {"A", "2"},
        {"B", "1"}, {"B", "2"}, {"B", "2"}};
    const std::vector<std::size_t> source_rows = {0, 1, 2, 3, 4, 5};
    const auto result = datalab::domain::statistics::multi_vari_chart(
        measurements, factors, source_rows, {"零件", "操作者"});
    QCOMPARE(result.valid_count, std::size_t{6});
    QCOMPARE(result.factor_count, std::size_t{2});
    QCOMPARE(result.possible_combinations, std::size_t{4});
    QCOMPARE(result.observed_combinations, std::size_t{4});
    QCOMPARE(result.combination_coverage, 1.0);
    QVERIFY(result.plot_available);
    QCOMPARE(result.factor_means.size(), std::size_t{4});
    QCOMPARE(result.factor_means[0].level, std::string{"A"});
    QCOMPARE(result.factor_means[0].mean, 11.0);
    QCOMPARE(result.factor_means[1].level, std::string{"B"});
    QCOMPARE(result.factor_means[1].mean, 22.0);
    QCOMPARE(result.factor_means[2].level, std::string{"1"});
    QCOMPARE(result.factor_means[2].mean, 14.0);
    QCOMPARE(result.factor_means[3].level, std::string{"2"});
    QCOMPARE(result.factor_means[3].mean, 19.0);
    QCOMPARE(result.points.size(), std::size_t{6});
    QCOMPARE(result.points.front().source_row, std::size_t{0});
}

void MultiVariTest::diagnosesInsufficientCoverage()
{
    const std::vector<double> measurements = {10.0, 20.0};
    const std::vector<std::vector<std::string>> factors = {
        {"A", "1"}, {"B", "2"}};
    const auto result = datalab::domain::statistics::multi_vari_chart(
        measurements, factors, {0, 1}, {"零件", "操作者"});
    QVERIFY(!result.plot_available);
    QVERIFY(std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "insufficient_combination_coverage";
        }));
}

void MultiVariTest::serviceSkipsMissingAndBuildsTables()
{
    datalab::domain::DataTable table;
    table.columns = {"零件", "操作者", "测量"};
    table.rows = {
        {"A", "1", "10"},
        {"A", "1", "12"},
        {"A", "2", "*"},
        {"A", "2", "11"},
        {"B", "1", "20"},
        {"B", "", "21"},
        {"B", "2", "22"},
        {"B", "2", "24"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.selection.measurement_column = 2;
    configuration.graph.variable_columns = {0, 1};
    configuration.chart_type = "multi_vari";
    const auto page = datalab::application::AnalysisService::multi_vari(
        table, configuration);
    QCOMPARE(page.method_name, std::string{"Multi-Vari Chart"});
    QVERIFY(page.facts.multi_vari.has_value());
    QCOMPARE(page.facts.multi_vari->valid_count, std::size_t{6});
    QCOMPARE(page.facts.multi_vari->missing_count, std::size_t{2});
    QCOMPARE(page.facts.multi_vari->factor_count, std::size_t{2});
    QVERIFY(std::any_of(
        page.diagnostics.cbegin(), page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "missing_values";
        }));
    QVERIFY(std::any_of(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table) {
            return table.title == "因子均值";
        }));
    QVERIFY(std::any_of(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table) {
            return table.title == "单元均值";
        }));
    QCOMPARE(page.plots.size(), std::size_t{1});
    QCOMPARE(page.plots.front().source_rows.front(), std::size_t{0});
}

void MultiVariTest::interpretationDoesNotClaimCapability()
{
    datalab::domain::OutputPage page;
    page.method_name = "Multi-Vari Chart";
    datalab::domain::MultiVariFacts facts;
    facts.factor_count = 2;
    facts.valid_count = 6;
    facts.missing_count = 2;
    facts.combination_coverage = 1.0;
    facts.factor_names = {"零件", "操作者"};
    page.facts.multi_vari = facts;
    datalab::application::InterpretationService::enrich(page);
    QVERIFY(!page.interpretation.empty());
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
        }
    }
    const auto conclusion = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(conclusion != page.interpretation.cend());
    QVERIFY(std::any_of(
        conclusion->bullets.cbegin(), conclusion->bullets.cend(),
        [](const std::string& bullet) {
            return bullet.find("2 个因子") != std::string::npos;
        }));
}

void MultiVariTest::supportsFourFactorsWithSourceRows()
{
    // # source: formula_reference — 2^4 full factorial one obs each → coverage 100%
    std::vector<double> measurements;
    std::vector<std::vector<std::string>> factors;
    std::vector<std::size_t> source_rows;
    std::size_t row = 0;
    for (const char* a : {"A1", "A2"}) {
        for (const char* b : {"B1", "B2"}) {
            for (const char* c : {"C1", "C2"}) {
                for (const char* d : {"D1", "D2"}) {
                    measurements.push_back(static_cast<double>(row + 1));
                    factors.push_back({a, b, c, d});
                    source_rows.push_back(row);
                    ++row;
                }
            }
        }
    }
    const auto result = datalab::domain::statistics::multi_vari_chart(
        measurements, factors, source_rows, {"F0", "F1", "F2", "F3"});
    QCOMPARE(result.factor_count, std::size_t{4});
    QCOMPARE(result.valid_count, std::size_t{16});
    QCOMPARE(result.possible_combinations, std::size_t{16});
    QCOMPARE(result.observed_combinations, std::size_t{16});
    QVERIFY(result.plot_available);
    QCOMPARE(result.points.back().source_row, std::size_t{15});
    // Outer factor D2 shifts x beyond the 3-factor block width.
    double max_x = 0.0;
    for (const auto& point : result.points) {
        max_x = std::max(max_x, point.x_position);
    }
    QVERIFY(max_x > 3.0);

    datalab::domain::DataTable table;
    table.columns = {"F0", "F1", "F2", "F3", "Y"};
    for (std::size_t index = 0; index < measurements.size(); ++index) {
        table.rows.push_back({
            factors[index][0], factors[index][1], factors[index][2],
            factors[index][3], std::to_string(measurements[index])});
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.selection.measurement_column = 4;
    configuration.graph.variable_columns = {0, 1, 2, 3};
    const auto page = datalab::application::AnalysisService::multi_vari(
        table, configuration);
    QVERIFY(page.facts.multi_vari.has_value());
    QCOMPARE(page.facts.multi_vari->factor_count, std::size_t{4});
    QVERIFY(!page.plots.empty());
    QCOMPARE(page.plots.front().source_rows.size(), std::size_t{16});
}

QTEST_APPLESS_MAIN(MultiVariTest)
#include "multi_vari_test.moc"
