#include "reporting/chart_adapter.h"
#include "reporting/chart_renderer.h"

#include <QColor>
#include <QGuiApplication>
#include <QImage>
#include <QtTest/QtTest>

class ChartRendererTest final : public QObject {
    Q_OBJECT

private slots:
    void renderToPixmapProducesNonEmptyImage();
    void themePresetChangesBackgroundColor();
    void printThemeKeepsLightBackground();
    void unknownPresetFallsBackToDefault();
    void customYRangeAndDataRegionFillRender();
    void histogramCurveLegendRoundTrips();
};

ChartModel sample_control_model()
{
    ChartModel model;
    model.title = QStringLiteral("测试控制图");
    model.values = {1.0, 2.0, 3.0, 2.5};
    model.center = {2.0, 2.0, 2.0, 2.0};
    model.lower = {1.0, 1.0, 1.0, 1.0};
    model.upper = {3.0, 3.0, 3.0, 3.0};
    model.source_rows = {0, 1, 2, 3};
    return model;
}

QRgb corner_pixel(const ChartModel& model)
{
    const QPixmap pixmap = ChartRenderer::render_to_pixmap(model, QSize(320, 160), 1.0);
    const QImage image = pixmap.toImage();
    return image.pixel(0, 0);
}

void ChartRendererTest::renderToPixmapProducesNonEmptyImage()
{
    ChartModel model = sample_control_model();
    const QPixmap pixmap = ChartRenderer::render_to_pixmap(model, QSize(640, 320), 2.0);
    QVERIFY(!pixmap.isNull());
    QCOMPARE(pixmap.devicePixelRatio(), 2.0);
    QCOMPARE(pixmap.width(), 1280);
    QCOMPARE(pixmap.height(), 640);
}

void ChartRendererTest::themePresetChangesBackgroundColor()
{
    ChartModel light = sample_control_model();
    light.theme_preset = QStringLiteral("default");
    ChartModel dark = sample_control_model();
    dark.theme_preset = QStringLiteral("dark");
    const QRgb light_pixel = corner_pixel(light);
    const QRgb dark_pixel = corner_pixel(dark);
    QVERIFY(light_pixel != dark_pixel);
    QCOMPARE(QColor(dark_pixel).red(), 0x1e);
    QCOMPARE(QColor(dark_pixel).green(), 0x1e);
    QCOMPARE(QColor(dark_pixel).blue(), 0x1e);
}

void ChartRendererTest::printThemeKeepsLightBackground()
{
    ChartModel print_model = sample_control_model();
    print_model.theme_preset = QStringLiteral("print");
    const QColor print_color(corner_pixel(print_model));
    QVERIFY(print_color.red() >= 250);
    QVERIFY(print_color.green() >= 250);
    QVERIFY(print_color.blue() >= 250);
}

void ChartRendererTest::unknownPresetFallsBackToDefault()
{
    ChartModel fallback = sample_control_model();
    fallback.theme_preset = QStringLiteral("unknown-theme");
    ChartModel def = sample_control_model();
    def.theme_preset = QStringLiteral("default");
    QCOMPARE(corner_pixel(fallback), corner_pixel(def));
}

void ChartRendererTest::customYRangeAndDataRegionFillRender()
{
    ChartModel model = sample_control_model();
    model.y_min = 0.0;
    model.y_max = 10.0;
    model.x_min = 0.0;
    model.x_max = 5.0;
    model.data_region_fill = QStringLiteral("#fff3e0");
    const QPixmap pixmap = ChartRenderer::render_to_pixmap(model, QSize(320, 160), 1.0);
    QVERIFY(!pixmap.isNull());
    ChartModel missing;
    missing.theme_preset = QStringLiteral("default");
    missing.values = {1.0, 2.0};
    const QPixmap auto_scale = ChartRenderer::render_to_pixmap(missing, QSize(320, 160), 1.0);
    QVERIFY(!auto_scale.isNull());
}

void ChartRendererTest::histogramCurveLegendRoundTrips()
{
    datalab::domain::PlotSpec plot;
    plot.kind = datalab::domain::PlotKind::histogram;
    plot.histogram_edges = {0.0, 1.0, 2.0};
    plot.histogram_counts = {1.0, 2.0};
    plot.process_mean = 1.0;
    plot.within_sigma = 0.5;
    plot.overall_sigma = 0.6;
    plot.lsl = 0.0;
    plot.usl = 2.0;
    plot.target = 1.0;
    datalab::domain::PlotSeries within;
    within.label = "Within";
    within.style.color = "#455a64";
    plot.series.push_back(within);
    datalab::domain::PlotSeries overall;
    overall.label = "Overall";
    overall.style.color = "#c62828";
    plot.series.push_back(overall);

    const ChartModel model = chart_model_from_plot(plot);
    QCOMPARE(model.series.size(), std::size_t{2});
    QCOMPARE(model.series[0].label, QStringLiteral("Within"));
    QCOMPARE(model.series[1].label, QStringLiteral("Overall"));
    QCOMPARE(model.lsl, plot.lsl);
    const datalab::domain::PlotSpec restored = plot_from_chart_model(model);
    QCOMPARE(restored.series.size(), std::size_t{2});
    QCOMPARE(restored.series[0].label, std::string{"Within"});
    QCOMPARE(restored.series[1].label, std::string{"Overall"});
    const QPixmap pixmap = ChartRenderer::render_to_pixmap(model, QSize(320, 200), 1.0);
    QVERIFY(!pixmap.isNull());
}

QTEST_MAIN(ChartRendererTest)

#include "chart_renderer_test.moc"
