#include "application/analysis_service.h"
#include "domain/statistics/control_charts.h"
#include "infrastructure/csv_importer.h"

#include <QtTest/QtTest>

class MinitabFormulaGoldenTest final : public QObject {
    Q_OBJECT

private slots:
    void imrMatchesMinitabMovingRangeMethod();
    void capabilityWithinSigmaMatchesImr();
    void pistonRingFixtureRunsImr();
};

void MinitabFormulaGoldenTest::imrMatchesMinitabMovingRangeMethod()
{
    // Minitab I-MR default: CL = mean, sigma = MR_bar / d2(2), d2(2)=1.128
    const std::vector<double> values{1.0, 2.0, 3.0, 2.0};
    const auto dual =
        datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(values);
    QCOMPARE(dual.average_moving_range, 1.0);
    QVERIFY(qAbs(dual.sigma - 1.0 / 1.128) < 1.0e-12);
    QVERIFY(qAbs(dual.primary.upper_control_limit.front() - (2.0 + 3.0 / 1.128)) < 1.0e-12);
    QVERIFY(qAbs(dual.secondary.upper_control_limit.back() - 3.267 * 1.0) < 0.002);
}

void MinitabFormulaGoldenTest::capabilityWithinSigmaMatchesImr()
{
    datalab::domain::DataTable table;
    table.columns = {"x"};
    table.rows = {{"1"}, {"2"}, {"3"}, {"2"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.control.subgroup_size = 1;
    configuration.specifications.lower = -10.0;
    configuration.specifications.upper = 10.0;
    const auto capability = datalab::application::AnalysisService::capability(table, configuration);
    const auto imr = datalab::application::AnalysisService::individuals_moving_range(table, configuration);
    QVERIFY(!capability.tables.empty());
    QVERIFY(!imr.tables.empty());
    QString within;
    QString imr_sigma;
    for (const auto& row : capability.tables.front().rows) {
        if (row.size() >= 2 && row[0] == "StDev (Within)") {
            within = QString::fromStdString(row[1]);
        }
    }
    for (const auto& row : imr.tables.front().rows) {
        if (row.size() >= 2 && row[0] == "σ (within)") {
            imr_sigma = QString::fromStdString(row[1]);
        }
    }
    QCOMPARE(within, imr_sigma);
}

void MinitabFormulaGoldenTest::pistonRingFixtureRunsImr()
{
    QString error;
    const QString path = QStringLiteral(
        DATALAB_SOURCE_DIR "/tests/fixtures/minitab/converted/PistonRingDiameter.csv");
    const auto table = datalab::infrastructure::CsvImporter::import_file(path, &error);
    if (!table.has_value()) {
        QSKIP("PistonRingDiameter.csv is not available");
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    const auto page = datalab::application::AnalysisService::individuals_moving_range(*table, configuration);
    QVERIFY(!page.plots.empty());
    QCOMPARE(page.plots.size(), std::size_t{2});
    QVERIFY(page.plots.front().values.size() == table->rows.size()
            || page.plots.front().values.size() + 1 >= table->rows.size());
}

QTEST_APPLESS_MAIN(MinitabFormulaGoldenTest)

#include "minitab_formula_golden_test.moc"
