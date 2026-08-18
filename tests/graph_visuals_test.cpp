#include "domain/graph_assembly.h"
#include "domain/statistics/graph_visuals.h"
#include "domain/statistics/quality_visuals.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <limits>

class GraphVisualsTest final : public QObject {
    Q_OBJECT

private slots:
    void scatterPreservesCompleteRows();
    void intervalUsesStableGroupOrderAndTInterval();
    void bubbleRejectsNegativeSize();
    void correlationSupportsSpearman();
    void ecdfUsesUniqueSortedSteps();
    void pieMergesSmallCategories();
    void pieRejectsNegativeWeight();
    void contourRejectsIrregularGrid();
    void contourAcceptsRegularGrid();
    void heatmapFixesCorrelationColorRange();
    void tukeyBoxplotMarksOutliers();
    void probabilityPlotHasFitAndBands();
    void timeSeriesDiagnosesIrregularGaps();
    void histogramManualBinsAndSharedEdges();
    void assembleSkipsMissingAndKeepsRowMap();
};

void GraphVisualsTest::scatterPreservesCompleteRows()
{
    const auto result = datalab::domain::statistics::scatter_plot(
        {1.0, 2.0}, {3.0, 4.0}, {5, 9}, {"A", "B"}, {"r5", "r9"});
    QCOMPARE(result.x_values, std::vector<double>({1.0, 2.0}));
    QCOMPARE(result.source_rows, std::vector<std::size_t>({5, 9}));
    QCOMPARE(result.point_groups, std::vector<std::string>({"A", "B"}));
}

void GraphVisualsTest::intervalUsesStableGroupOrderAndTInterval()
{
    const auto result = datalab::domain::statistics::interval_plot(
        {1.0, 3.0, 10.0, 14.0}, {"B", "B", "A", "A"}, {0, 1, 2, 3});
    QCOMPARE(result.labels, std::vector<std::string>({"B", "A"}));
    QCOMPARE(result.counts, std::vector<std::size_t>({2, 2}));
    QCOMPARE(result.means, std::vector<double>({2.0, 12.0}));
    QVERIFY(result.lower[0] < result.means[0]);
    QVERIFY(result.upper[1] > result.means[1]);
}

void GraphVisualsTest::bubbleRejectsNegativeSize()
{
    const auto result = datalab::domain::statistics::bubble_plot(
        {1.0, 2.0}, {3.0, 4.0}, {5.0, -1.0}, {0, 1});
    QCOMPARE(result.points.x_values.size(), std::size_t(1));
    QVERIFY(!result.diagnostics.empty());
    QCOMPARE(result.diagnostics.front().code, std::string("negative_bubble_size"));
}

void GraphVisualsTest::correlationSupportsSpearman()
{
    const auto result = datalab::domain::statistics::correlation_plot(
        {{1.0, 2.0, 3.0}, {3.0, 2.0, 1.0}},
        {"X", "Y"}, "spearman");
    QCOMPARE(result.labels, std::vector<std::string>({"X", "Y"}));
    QCOMPARE(result.correlation.counts[0][1], std::size_t(3));
    QVERIFY(std::abs(result.correlation.coefficients[0][1] + 1.0) < 1.0e-12);
}

void GraphVisualsTest::ecdfUsesUniqueSortedSteps()
{
    const auto result = datalab::domain::statistics::ecdf_plot(
        {1.0, 2.0, 2.0, 3.0}, {0, 1, 2, 3});
    QCOMPARE(result.values, std::vector<double>({1.0, 2.0, 3.0}));
    QCOMPARE(result.counts, std::vector<std::size_t>({1, 2, 1}));
    QCOMPARE(result.proportions.back(), 1.0);
    QCOMPARE(result.proportions.front(), 0.25);
}

void GraphVisualsTest::pieMergesSmallCategories()
{
    const auto result = datalab::domain::statistics::pie_plot(
        {"A", "B", "C"}, {90.0, 8.0, 2.0}, 10.0);
    QCOMPARE(result.labels.front(), std::string("A"));
    QVERIFY(std::find(result.labels.begin(), result.labels.end(), "Other")
            != result.labels.end());
}

void GraphVisualsTest::pieRejectsNegativeWeight()
{
    const auto result = datalab::domain::statistics::pie_plot(
        {"A", "B"}, {4.0, -1.0}, 0.0);
    QCOMPARE(result.diagnostics.front().code, std::string("negative_weight"));
    QCOMPARE(result.labels, std::vector<std::string>({"A"}));
}

void GraphVisualsTest::contourRejectsIrregularGrid()
{
    const auto result = datalab::domain::statistics::contour_plot(
        {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 2.0, 3.0});
    QVERIFY(!result.diagnostics.empty());
    QCOMPARE(result.diagnostics.front().code, std::string("irregular_grid"));
}

void GraphVisualsTest::contourAcceptsRegularGrid()
{
    const auto result = datalab::domain::statistics::contour_plot(
        {0.0, 1.0, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}, {1.0, 2.0, 3.0, 4.0}, 3);
    QVERIFY(result.diagnostics.empty());
    QCOMPARE(result.x.size(), std::size_t{2});
    QCOMPARE(result.y.size(), std::size_t{2});
    QCOMPARE(result.z[0][0], 1.0);
    QCOMPARE(result.z[1][1], 4.0);
    QCOMPARE(result.levels.size(), std::size_t{3});
}

void GraphVisualsTest::heatmapFixesCorrelationColorRange()
{
    const auto correlation = datalab::domain::statistics::correlation_plot(
        {{1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}}, {"X", "Y"}, "pearson");
    const auto result = datalab::domain::statistics::heatmap_from_correlation(correlation);
    QCOMPARE(result.color_min, -1.0);
    QCOMPARE(result.color_max, 1.0);
}

void GraphVisualsTest::tukeyBoxplotMarksOutliers()
{
    const auto result = datalab::domain::statistics::box_plot_summary({1.0, 2.0, 3.0, 4.0, 100.0});
    QCOMPARE(result.count, std::size_t{5});
    QCOMPARE(result.whisker_low, 1.0);
    QVERIFY(result.whisker_high < 100.0);
    QCOMPARE(result.outliers, std::vector<double>({100.0}));
    const auto constant = datalab::domain::statistics::box_plot_summary({5.0, 5.0, 5.0});
    QCOMPARE(constant.iqr, 0.0);
    QVERIFY(constant.outliers.empty());
    QCOMPARE(constant.whisker_low, 5.0);
}

void GraphVisualsTest::probabilityPlotHasFitAndBands()
{
    const auto result = datalab::domain::statistics::probability_plot(
        {1.0, 2.0, 3.0, 4.0, 5.0}, {0, 1, 2, 3, 4});
    QCOMPARE(result.ordered_values.size(), std::size_t{5});
    QCOMPARE(result.fitted.size(), std::size_t{5});
    QCOMPARE(result.lower.size(), std::size_t{5});
    QVERIFY(result.scale > 0.0);
    QVERIFY(result.lower[2] < result.fitted[2]);
    QVERIFY(result.upper[2] > result.fitted[2]);
}

void GraphVisualsTest::timeSeriesDiagnosesIrregularGaps()
{
    const auto result = datalab::domain::statistics::time_series_plot(
        {1.0, 2.0, 3.0, 20.0}, {4.0, 5.0, 6.0, 7.0}, {0, 1, 2, 3});
    QVERIFY(std::any_of(result.diagnostics.cbegin(), result.diagnostics.cend(),
                        [](const datalab::domain::DiagnosticMessage& item) {
                            return item.code == "irregular_interval";
                        }));
}

void GraphVisualsTest::histogramManualBinsAndSharedEdges()
{
    const auto bins = datalab::domain::statistics::histogram({1.0, 2.0, 3.0, 4.0}, 2);
    QCOMPARE(bins.counts.size(), std::size_t{2});
    QCOMPARE(bins.edges.front(), 1.0);
    QCOMPARE(bins.edges.back(), 4.0);
    const auto grouped = datalab::domain::statistics::histogram_with_edges({1.0, 1.5}, bins.edges);
    QCOMPARE(grouped.counts.size(), bins.counts.size());
    QVERIFY(grouped.counts.front() >= 2.0);
}

void GraphVisualsTest::assembleSkipsMissingAndKeepsRowMap()
{
    datalab::domain::DataTable table;
    table.columns = {"X", "Y", "G"};
    table.rows = {{"1", "2", "A"}, {"*", "3", "B"}, {"4", "5", "A"}};
    const auto assembled = datalab::domain::assemble_graph_columns(
        table, 0, 1, {}, 2, {}, {}, true);
    QCOMPARE(assembled.source_rows, std::vector<std::size_t>({0, 2}));
    QCOMPARE(assembled.skipped_count, std::size_t{1});
    QCOMPARE(assembled.groups, std::vector<std::string>({"A", "A"}));

    const auto matrix = datalab::domain::assemble_numeric_matrix(
        table, {0, 1}, 2, {}, false);
    QCOMPARE(matrix.source_rows.size(), std::size_t{3});
    QVERIFY(std::isnan(matrix.columns[0][1]));
}

QTEST_APPLESS_MAIN(GraphVisualsTest)
#include "graph_visuals_test.moc"
