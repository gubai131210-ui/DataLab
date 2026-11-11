#include "reporting/chart_plot_context.h"
#include "reporting/chart_renderer.h"

#include <QtTest/QtTest>

class ChartPlotContextTest final : public QObject {
    Q_OBJECT

private slots:
    void histogramMapperMatchesRendererYRange();
    void histogramBinDataXMapsToCorrectIndex();
    void controlMapperIncludesLimitSeries();
    void controlMapperUsesRealDataRangeNotDefault();
    void correlationGridUsesRendererOrigin();
    void hitToleranceUsesScreenPixels();
    void boxPlotContextMatchesRendererRange();
};

void ChartPlotContextTest::histogramMapperMatchesRendererYRange()
{
    ChartModel model;
    model.kind = ChartKind::Histogram;
    model.histogram_edges = {0.0, 1.0, 2.0, 3.0};
    model.histogram_counts = {4.0, 10.0, 2.0};

    const QRectF area(0.0, 0.0, 640.0, 480.0);
    const ChartPlotContext context = build_histogram_plot_context(model, area);

    QVERIFY(context.valid);
    QCOMPARE(context.mapper.y_min(), 0.0);
    QCOMPARE(context.mapper.y_max(), 11.5);
}

void ChartPlotContextTest::histogramBinDataXMapsToCorrectIndex()
{
    ChartModel model;
    model.kind = ChartKind::Histogram;
    model.histogram_edges = {0.0, 1.0, 2.0, 3.0};
    model.histogram_counts = {4.0, 10.0, 2.0};

    const QRectF area(0.0, 0.0, 640.0, 480.0);
    const ChartPlotContext context = build_histogram_plot_context(model, area);
    QVERIFY(context.valid);

    const double x = context.mapper.to_data(context.mapper.to_pixel(1.5, 0.0)).x();
    QVERIFY(x >= 1.0 && x < 2.0);
}

void ChartPlotContextTest::controlMapperIncludesLimitSeries()
{
    ChartModel model;
    model.kind = ChartKind::Control;
    model.values = {1.0, 2.0, 3.0};
    model.lower = {0.5, 0.5, 0.5};
    model.upper = {4.0, 4.0, 4.0};

    const QRectF area(0.0, 0.0, 640.0, 480.0);
    const ChartPlotContext context = build_control_plot_context(model, area);

    QVERIFY(context.valid);
    QVERIFY(context.mapper.y_min() < 1.0);
    QVERIFY(context.mapper.y_max() > 3.0);
}

void ChartPlotContextTest::controlMapperUsesRealDataRangeNotDefault()
{
    ChartModel model;
    model.kind = ChartKind::Control;
    model.values = {20.1, 20.3, 20.4, 20.2};

    const QRectF area(0.0, 0.0, 640.0, 480.0);
    const ChartPlotContext context = build_control_plot_context(model, area);

    QVERIFY(context.valid);
    QVERIFY(context.mapper.y_min() < 20.1);
    QVERIFY(context.mapper.y_max() > 20.4);
    QVERIFY(context.mapper.y_max() != 1.0);
    QVERIFY(!(context.mapper.y_min() == 0.0 && context.mapper.y_max() == 1.0));
}

void ChartPlotContextTest::correlationGridUsesRendererOrigin()
{
    ChartModel model;
    model.kind = ChartKind::Correlation;
    model.matrix_labels = {QStringLiteral("A"), QStringLiteral("B")};
    model.matrix_values = {{1.0, 0.5}, {0.5, 1.0}};

    const QRectF area(0.0, 0.0, 640.0, 480.0);
    const ChartMatrixGridLayout layout = build_correlation_grid_layout(model, area);

    QVERIFY(layout.valid);
    QCOMPARE(layout.origin.x(), area.left() + 120.0);
    QCOMPARE(layout.origin.y(), area.top() + 52.0);
}

void ChartPlotContextTest::hitToleranceUsesScreenPixels()
{
    ChartModel model;
    model.view.zoom_factor = 2.0;
    QCOMPARE(chart_hit_tolerance_pixels(model, 28.0), 28.0);
}

void ChartPlotContextTest::boxPlotContextMatchesRendererRange()
{
    ChartModel model;
    model.kind = ChartKind::BoxPlot;
    model.box_min = {1.0, 2.0};
    model.box_q1 = {2.0, 3.0};
    model.box_median = {3.0, 4.0};
    model.box_q3 = {4.0, 5.0};
    model.box_max = {10.0, 6.0};

    const QRectF area(0.0, 0.0, 640.0, 480.0);
    const ChartPlotContext context = build_box_plot_context(model, area);

    QVERIFY(context.valid);
    QVERIFY(context.mapper.y_min() < 1.0);
    QVERIFY(context.mapper.y_max() > 10.0);
}

QTEST_MAIN(ChartPlotContextTest)
#include "chart_plot_context_test.moc"
