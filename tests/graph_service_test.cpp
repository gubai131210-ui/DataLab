#include "application/analysis_service.h"
#include "application/graph_service.h"
#include "domain/quality_types.h"

#include <QtTest/QtTest>

class GraphServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void pieAndEcdfProducePlots();
    void contourDiagnosesIrregularGrid();
    void probabilityAndMatrixProducePlots();
    void heatmapFixesColorRange();
    void histogramAndBoxplotUseDomainRules();
};

namespace {

datalab::domain::DataTable sample_table()
{
    datalab::domain::DataTable table;
    table.columns = {"Time", "X", "Y", "Z", "Group"};
    table.rows = {
        {"1", "1", "2", "3", "A"},
        {"2", "2", "3", "4", "A"},
        {"3", "3", "1", "5", "B"},
        {"4", "4", "0", "6", "B"},
        {"20", "100", "0", "7", "B"}};
    return table;
}

}  // namespace

void GraphServiceTest::pieAndEcdfProducePlots()
{
    const auto table = sample_table();
    datalab::domain::AnalysisConfiguration pie;
    pie.graph.graph_kind = "pie";
    pie.graph.x_column = 4;
    const auto pie_page = datalab::application::GraphService::run(table, pie);
    QCOMPARE(pie_page.plots.size(), std::size_t{1});
    QCOMPARE(pie_page.plots.front().kind, datalab::domain::PlotKind::pie);
    QVERIFY(pie_page.plots.front().categories.size() >= 2);

    datalab::domain::AnalysisConfiguration ecdf;
    ecdf.graph.graph_kind = "ecdf";
    ecdf.graph.y_column = 1;
    const auto ecdf_page = datalab::application::GraphService::run(table, ecdf);
    QCOMPARE(ecdf_page.plots.front().kind, datalab::domain::PlotKind::ecdf);
    QCOMPARE(ecdf_page.plots.front().values.back(), 1.0);
}

void GraphServiceTest::contourDiagnosesIrregularGrid()
{
    const auto table = sample_table();
    datalab::domain::AnalysisConfiguration configuration;
    configuration.graph.graph_kind = "contour";
    configuration.graph.x_column = 1;
    configuration.graph.y_column = 2;
    configuration.graph.z_column = 3;
    const auto page = datalab::application::GraphService::run(table, configuration);
    QVERIFY(!page.diagnostics.empty());
    QCOMPARE(page.diagnostics.front().code, std::string("irregular_grid"));
}

void GraphServiceTest::probabilityAndMatrixProducePlots()
{
    const auto table = sample_table();
    datalab::domain::AnalysisConfiguration probability;
    probability.graph.graph_kind = "probability";
    probability.graph.y_column = 1;
    const auto probability_page = datalab::application::GraphService::run(table, probability);
    QCOMPARE(probability_page.plots.front().kind, datalab::domain::PlotKind::probability);
    QCOMPARE(probability_page.plots.front().center.size(),
             probability_page.plots.front().values.size());
    QVERIFY(!probability_page.plots.front().lower.empty());

    datalab::domain::AnalysisConfiguration matrix;
    matrix.graph.graph_kind = "matrix";
    matrix.graph.variable_columns = {1, 2, 3};
    const auto matrix_page = datalab::application::GraphService::run(table, matrix);
    QCOMPARE(matrix_page.plots.front().kind, datalab::domain::PlotKind::matrix);
    QCOMPARE(matrix_page.plots.front().matrix_labels.size(), std::size_t{3});
}

void GraphServiceTest::heatmapFixesColorRange()
{
    const auto table = sample_table();
    datalab::domain::AnalysisConfiguration configuration;
    configuration.graph.graph_kind = "heatmap";
    configuration.graph.variable_columns = {1, 2, 3};
    const auto page = datalab::application::GraphService::run(table, configuration);
    QCOMPARE(page.plots.front().kind, datalab::domain::PlotKind::heatmap);
    QCOMPARE(*page.plots.front().color_min, -1.0);
    QCOMPARE(*page.plots.front().color_max, 1.0);
}

void GraphServiceTest::histogramAndBoxplotUseDomainRules()
{
    const auto table = sample_table();
    datalab::domain::AnalysisConfiguration histogram;
    histogram.variable_columns = {1};
    histogram.graph.bin_count = 4;
    const auto histogram_page = datalab::application::AnalysisService::histogram(table, histogram);
    QCOMPARE(histogram_page.plots.front().kind, datalab::domain::PlotKind::histogram);
    QCOMPARE(histogram_page.plots.front().histogram_counts.size(), std::size_t{4});
    QVERIFY(histogram_page.parameter_summary.find("手工") != std::string::npos);

    datalab::domain::AnalysisConfiguration boxplot;
    boxplot.variable_columns = {1};
    boxplot.by_column = 4;
    const auto box_page = datalab::application::AnalysisService::boxplot(table, boxplot);
    QCOMPARE(box_page.plots.front().box_labels, std::vector<std::string>({"A", "B"}));
    QVERIFY(!box_page.tables.empty());
    QCOMPARE(box_page.plots.front().box_min.size(), std::size_t{2});
}

QTEST_APPLESS_MAIN(GraphServiceTest)
#include "graph_service_test.moc"
