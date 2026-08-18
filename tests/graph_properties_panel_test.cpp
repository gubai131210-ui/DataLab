#include "ui/graph_properties_panel.h"

#include <QAbstractScrollArea>
#include <QListWidget>
#include <QPushButton>
#include <QTest>

class GraphPropertiesPanelTest final : public QObject {
    Q_OBJECT

private slots:
    void controlPanelShowsReferenceObjects();
    void scatterPanelHidesReferenceObjects();
    void panelHasScrollableContentAndFixedActions();
};

ChartModel make_model(const ChartKind kind)
{
    ChartModel model;
    model.kind = kind;
    model.values = {1.0, 2.0, 3.0};
    model.x_values = {1.0, 2.0, 3.0};
    if (kind == ChartKind::Control) {
        model.center = {2.0, 2.0, 2.0};
        model.lower = {1.0, 1.0, 1.0};
        model.upper = {3.0, 3.0, 3.0};
    }
    return model;
}

void GraphPropertiesPanelTest::controlPanelShowsReferenceObjects()
{
    GraphPropertiesPanel panel(make_model(ChartKind::Control));
    const auto* objects = panel.findChild<QListWidget*>(
        QStringLiteral("graph_object_list"));
    QVERIFY(objects != nullptr);
    QVERIFY(objects->findItems(QStringLiteral("控制线 / CL"), Qt::MatchExactly).size() == 1);
    QVERIFY(objects->findItems(QStringLiteral("控制线 / LCL"), Qt::MatchExactly).size() == 1);
    QVERIFY(objects->findItems(QStringLiteral("控制线 / UCL"), Qt::MatchExactly).size() == 1);
}

void GraphPropertiesPanelTest::scatterPanelHidesReferenceObjects()
{
    GraphPropertiesPanel panel(make_model(ChartKind::Scatter));
    const auto* objects = panel.findChild<QListWidget*>(
        QStringLiteral("graph_object_list"));
    QVERIFY(objects != nullptr);
    QVERIFY(objects->findItems(QStringLiteral("控制线 / CL"), Qt::MatchExactly).empty());
}

void GraphPropertiesPanelTest::panelHasScrollableContentAndFixedActions()
{
    GraphPropertiesPanel panel(make_model(ChartKind::Scatter));
    const auto* scroll = panel.findChild<QScrollArea*>(
        QStringLiteral("graph_properties_scroll"));
    QVERIFY(scroll != nullptr);
    QVERIFY(scroll->widgetResizable());
    QVERIFY(panel.findChild<QPushButton*>(QStringLiteral("close_panel")) != nullptr);
    QVERIFY(panel.findChild<QPushButton*>(QStringLiteral("restore_defaults")) != nullptr);
    QVERIFY(panel.findChild<QPushButton*>(QStringLiteral("apply_changes")) != nullptr);
}

QTEST_MAIN(GraphPropertiesPanelTest)
#include "graph_properties_panel_test.moc"
