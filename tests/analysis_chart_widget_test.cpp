#include "ui/analysis_chart_widget.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QTest>
#include <QTimer>

class AnalysisChartWidgetTest final : public QObject {
    Q_OBJECT

private slots:
    void doubleClickDoesNotOpenGraphProperties();
    void contextMenuProvidesGraphPropertiesAction();
};

ChartModel make_chart_model()
{
    ChartModel model;
    model.kind = ChartKind::Scatter;
    model.values = {1.0, 2.0, 3.0};
    model.x_values = {1.0, 2.0, 3.0};
    return model;
}

void AnalysisChartWidgetTest::doubleClickDoesNotOpenGraphProperties()
{
    AnalysisChartWidget widget;
    widget.set_model(make_chart_model());
    widget.resize(640, 420);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QTest::mouseDClick(&widget, Qt::LeftButton, Qt::NoModifier, QPoint(320, 210));
    QVERIFY(QApplication::activeModalWidget() == nullptr);
}

void AnalysisChartWidgetTest::contextMenuProvidesGraphPropertiesAction()
{
    AnalysisChartWidget widget;
    widget.set_model(make_chart_model());
    widget.resize(640, 420);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    bool has_edit_action = false;
    const QPoint local_position(320, 210);
    QContextMenuEvent event(
        QContextMenuEvent::Mouse,
        local_position,
        widget.mapToGlobal(local_position));
    QTimer::singleShot(0, [&has_edit_action]() {
        auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (menu != nullptr) {
            for (QAction* action : menu->actions()) {
                has_edit_action = action->text().contains(QStringLiteral("编辑图形"));
            }
            menu->close();
        }
    });
    QApplication::sendEvent(&widget, &event);

    QVERIFY(event.isAccepted());
    QVERIFY(has_edit_action);
}

QTEST_MAIN(AnalysisChartWidgetTest)
#include "analysis_chart_widget_test.moc"
