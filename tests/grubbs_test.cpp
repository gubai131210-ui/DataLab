#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/grubbs_test.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

class GrubbsTest final : public QObject {
    Q_OBJECT

private slots:
    void statisticMatchesHandCalculation();
    void rejectsSmallSampleAndZeroVariance();
    void buildsServiceCompleteCaseContract();
    void interpretationDoesNotClaimDeletion();
};

void GrubbsTest::statisticMatchesHandCalculation()
{
    // # source: formula_reference — G = max|y-ȳ|/s for {1,2,3,4,10}.
    // ȳ = 4, s² = 12.5, s = √12.5, G = 6/√12.5.
    const auto result = datalab::domain::statistics::grubbs_outlier_test(
        {1.0, 2.0, 3.0, 4.0, 10.0}, {10, 11, 12, 13, 14});
    QCOMPARE(result.n, std::size_t{5});
    QVERIFY(qAbs(result.mean - 4.0) < 1.0e-12);
    QVERIFY(qAbs(result.sample_standard_deviation - std::sqrt(12.5)) < 1.0e-12);
    QVERIFY(result.g_statistic.has_value());
    QVERIFY(qAbs(*result.g_statistic - 6.0 / std::sqrt(12.5)) < 1.0e-12);
    QCOMPARE(result.direction, std::string{"largest"});
    QCOMPARE(*result.outlier_value, 10.0);
    QCOMPARE(*result.source_row, std::size_t{14});
    QVERIFY(result.p_value.has_value());
    QVERIFY(*result.p_value > 0.0);
    QVERIFY(*result.p_value <= 1.0);
}

void GrubbsTest::rejectsSmallSampleAndZeroVariance()
{
    const auto too_small = datalab::domain::statistics::grubbs_outlier_test({1.0, 2.0});
    QVERIFY(!too_small.g_statistic.has_value());
    QVERIFY(std::any_of(
        too_small.diagnostics.cbegin(), too_small.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "insufficient_observations";
        }));

    const auto flat = datalab::domain::statistics::grubbs_outlier_test({1.0, 1.0, 1.0});
    QVERIFY(!flat.g_statistic.has_value());
    QVERIFY(std::any_of(
        flat.diagnostics.cbegin(), flat.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "zero_variance";
        }));
}

void GrubbsTest::buildsServiceCompleteCaseContract()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    table.rows = {{"1"}, {"*"}, {"2"}, {"3"}, {"4"}, {"10"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.selection.measurement_column = 0;
    configuration.inference.confidence_level = 0.95;
    configuration.inference.alternative = "two_sided";
    const auto page =
        datalab::application::AnalysisService::outlier_test(table, configuration);
    QCOMPARE(page.method_name, std::string{"Outlier Test"});
    QVERIFY(page.facts.outlier_test.has_value());
    QCOMPARE(page.facts.outlier_test->n, std::size_t{5});
    QCOMPARE(page.facts.outlier_test->missing_count, std::size_t{1});
    QCOMPARE(*page.facts.outlier_test->source_row, std::size_t{5});
    QCOMPARE(*page.facts.outlier_test->outlier_value, 10.0);
    QVERIFY(!page.plots.empty());
    QCOMPARE(page.plots.front().kind, datalab::domain::PlotKind::scatter);
    QCOMPARE(page.plots.front().source_rows, (std::vector<std::size_t>{0, 2, 3, 4, 5}));
    QCOMPARE(page.plots.front().values.size(), page.plots.front().source_rows.size());
    QCOMPARE(page.tables.front().title, std::string{"异常值检验"});
}

void GrubbsTest::interpretationDoesNotClaimDeletion()
{
    datalab::domain::OutputPage page;
    page.method_name = "Outlier Test";
    datalab::domain::OutlierTestFacts facts;
    facts.n = 5;
    facts.g_statistic = 1.7;
    facts.p_value = 0.02;
    facts.alpha = 0.05;
    facts.assumption_status = "not_verified";
    page.facts.outlier_test = facts;
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("必须删除") == std::string::npos);
            QVERIFY(bullet.find("已确认异常") == std::string::npos);
            QVERIFY(bullet.find("已证明") == std::string::npos);
        }
    }
    const auto conclusion = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(conclusion != page.interpretation.cend());
    QVERIFY(conclusion->bullets.front().find("工程调查") != std::string::npos);
}

QTEST_APPLESS_MAIN(GrubbsTest)

#include "grubbs_test.moc"
