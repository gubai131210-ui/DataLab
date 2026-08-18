#include "domain/quality_types.h"
#include "reporting/chart_adapter.h"

#include <QtTest/QtTest>

class ChartModelRoundtripTest final : public QObject {
    Q_OBJECT

private slots:
    void preservesDataAndStyles();
};

void ChartModelRoundtripTest::preservesDataAndStyles()
{
    datalab::domain::PlotSpec source;
    source.kind = datalab::domain::PlotKind::scatter;
    source.title = "测试散点图";
    source.values = {1.0, 2.0, 3.0};
    source.x_values = {10.0, 20.0, 30.0};
    source.source_rows = {4, 8, 12};
    source.value_style.color = "#008577";
    source.value_style.point_style = datalab::domain::PlotPointStyle::square;
    source.value_style.line_width = 2.5;
    source.legend_font_size = 11;
    source.histogram_edges_y = {0.0, 1.0};
    source.color_min = -1.0;
    source.color_max = 1.0;

    datalab::domain::PlotSeries series;
    series.role = datalab::domain::PlotSeriesRole::fitted;
    series.label = "拟合线";
    series.values = {1.1, 2.1, 2.9};
    series.style.visible = false;
    series.style.color = "#c62828";
    series.style.line_style = datalab::domain::PlotLineStyle::dot;
    source.series.push_back(series);

    const ChartModel model = chart_model_from_plot(source);
    QCOMPARE(model.title, QStringLiteral("测试散点图"));
    QCOMPARE(model.value_style.color, QStringLiteral("#008577"));
    QCOMPARE(model.value_style.point_style, ChartPointStyle::Square);
    QCOMPARE(model.legend_font_size, 11);
    QCOMPARE(model.series.size(), std::size_t(1));
    QVERIFY(!model.series.front().style.visible);
    QCOMPARE(model.series.front().style.line_style, ChartLineStyle::Dot);

    const datalab::domain::PlotSpec restored = plot_from_chart_model(model);
    QCOMPARE(restored.title, source.title);
    QCOMPARE(restored.values, source.values);
    QCOMPARE(restored.x_values, source.x_values);
    QCOMPARE(restored.source_rows, source.source_rows);
    QCOMPARE(restored.histogram_edges_y, source.histogram_edges_y);
    QCOMPARE(*restored.color_min, -1.0);
    QCOMPARE(restored.value_style.color, source.value_style.color);
    QCOMPARE(restored.value_style.point_style, source.value_style.point_style);
    QCOMPARE(restored.series.size(), std::size_t(1));
    QCOMPARE(restored.series.front().style.color, series.style.color);
    QCOMPARE(restored.series.front().style.line_style, series.style.line_style);
    QVERIFY(!restored.series.front().style.visible);
}

QTEST_APPLESS_MAIN(ChartModelRoundtripTest)

#include "chart_model_roundtrip_test.moc"
