#include "reporting/chart_coordinate_mapper.h"
#include "reporting/chart_geometry.h"

#include <QtTest/QtTest>

class ChartGeometryTest final : public QObject {
    Q_OBJECT

private slots:
    void mapsDataToPlotAndBack();
    void zoomKeepsBoundsValid();
    void usesExpandedParetoLabelArea();
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

void ChartGeometryTest::usesExpandedParetoLabelArea()
{
    const QRectF normal = chart_geometry::plot_rect(QRectF(0.0, 0.0, 500.0, 400.0),
                                                    ChartKind::Control);
    const QRectF pareto = chart_geometry::plot_rect(QRectF(0.0, 0.0, 500.0, 400.0),
                                                    ChartKind::Pareto);

    QVERIFY(pareto.bottom() < normal.bottom());
    QVERIFY(pareto.left() > normal.left());
    QVERIFY(pareto.right() < normal.right());
}

QTEST_APPLESS_MAIN(ChartGeometryTest)

#include "chart_geometry_test.moc"
