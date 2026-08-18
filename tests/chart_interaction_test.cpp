#include "reporting/chart_interaction.h"

#include <QTest>

class ChartInteractionTest final : public QObject {
    Q_OBJECT

private slots:
    void parallelTooltipIncludesObservationValues();
    void contourTooltipIncludesCellValue();
};

void ChartInteractionTest::parallelTooltipIncludesObservationValues()
{
    ChartModel model;
    model.matrix_labels = {QStringLiteral("温度"), QStringLiteral("压力")};
    model.matrix_values = {{10.0, 2.5}};
    model.point_groups = {QStringLiteral("A")};

    const QString tooltip = chart_interaction::tooltip_text(
        model, {chart_interaction::HitKind::ParallelObservation, 0});
    QVERIFY(tooltip.contains(QStringLiteral("观测 1")));
    QVERIFY(tooltip.contains(QStringLiteral("温度")));
    QVERIFY(tooltip.contains(QStringLiteral("分组: A")));
}

void ChartInteractionTest::contourTooltipIncludesCellValue()
{
    ChartModel model;
    model.contour_x = {0.0, 1.0, 2.0};
    model.contour_y = {10.0, 20.0};
    model.matrix_values = {{3.5, 4.5}};

    const QString tooltip = chart_interaction::tooltip_text(
        model, {chart_interaction::HitKind::ContourCell, 1});
    QVERIFY(tooltip.contains(QStringLiteral("网格单元")));
    QVERIFY(tooltip.contains(QStringLiteral("值: 4.5")));
}

QTEST_MAIN(ChartInteractionTest)
#include "chart_interaction_test.moc"
