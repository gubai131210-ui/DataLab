#include "ui/chart_coordinate_mapper.h"

#include <QtTest/QtTest>

class ChartGeometryTest final : public QObject {
    Q_OBJECT

private slots:
    void mapsDataToPlotAndBack();
    void zoomKeepsBoundsValid();
};

void ChartGeometryTest::mapsDataToPlotAndBack()
{
    ChartCoordinateMapper mapper(QRectF(0.0, 0.0, 100.0, 100.0));
    mapper.set_data_range(0.0, 10.0, 0.0, 100.0);

    const QPointF pixel = mapper.to_pixel(5.0, 50.0);
    const QPointF value = mapper.to_data(pixel);

    QVERIFY(qFuzzyCompare(pixel.x(), 50.0));
    QVERIFY(qFuzzyCompare(pixel.y(), 50.0));
    QVERIFY(qFuzzyCompare(value.x(), 5.0));
    QVERIFY(qFuzzyCompare(value.y(), 50.0));
}

void ChartGeometryTest::zoomKeepsBoundsValid()
{
    ChartCoordinateMapper mapper(QRectF(0.0, 0.0, 100.0, 100.0));
    mapper.set_data_range(0.0, 10.0, 0.0, 100.0);
    mapper.zoom(2.0, QPointF(50.0, 50.0));

    QVERIFY(mapper.x_min() > 0.0);
    QVERIFY(mapper.x_max() < 10.0);
    QVERIFY(mapper.y_min() > 0.0);
    QVERIFY(mapper.y_max() < 100.0);
}

QTEST_APPLESS_MAIN(ChartGeometryTest)

#include "chart_geometry_test.moc"
