#include "ui/analysis_chart_widget.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QClipboard>
#include <QMenu>
#include <QMimeData>
#include <QTest>
#include <QTimer>

class AnalysisChartWidgetTest final : public QObject {
    Q_OBJECT

private slots:
    void doubleClickDoesNotOpenGraphProperties();
    void contextMenuProvidesGraphPropertiesAction();
    void browseModeKeepsChartSurface();
    void copyToClipboardPublishesImageMime();
    void copyIncludesVisibilityFootnoteWhenHidden();
    void ctrlCOnSurfaceCopiesChart();
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

void AnalysisChartWidgetTest::browseModeKeepsChartSurface()
{
    AnalysisChartWidget widget;
    widget.set_model(make_chart_model());
    widget.resize(640, 420);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    const auto* surface = widget.findChild<QWidget*>(QStringLiteral("chart_surface"));
    QVERIFY(surface != nullptr);
    QVERIFY(surface->width() >= 600);
    QVERIFY(widget.findChild<QWidget*>(QStringLiteral("graph_properties_panel")) == nullptr);
}

void AnalysisChartWidgetTest::copyToClipboardPublishesImageMime()
{
    AnalysisChartWidget widget;
    widget.set_model(make_chart_model());
    widget.resize(640, 420);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QVERIFY(widget.copy_to_clipboard());
    const QMimeData* mime = QApplication::clipboard()->mimeData();
    QVERIFY(mime != nullptr);
    QVERIFY(mime->hasImage());
    QVERIFY(mime->hasFormat(QStringLiteral("image/png")));
    QVERIFY(mime->hasFormat(QStringLiteral("image/bmp")));
    QVERIFY(!QApplication::clipboard()->image().isNull());
}

void AnalysisChartWidgetTest::copyIncludesVisibilityFootnoteWhenHidden()
{
    AnalysisChartWidget widget;
    widget.set_model(make_chart_model());
    widget.resize(640, 420);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QVERIFY(widget.copy_to_clipboard());
    const int base_height = QApplication::clipboard()->image().height();

    widget.set_row_visibility_summary(1, 2, 30, 35);
    QVERIFY(widget.copy_to_clipboard());
    const QImage with_footnote = QApplication::clipboard()->image();
    QVERIFY(!with_footnote.isNull());
    QVERIFY(with_footnote.height() > base_height);
}

void AnalysisChartWidgetTest::ctrlCOnSurfaceCopiesChart()
{
    AnalysisChartWidget widget;
    widget.set_model(make_chart_model());
    widget.resize(640, 420);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QWidget* surface = widget.findChild<QWidget*>(QStringLiteral("chart_surface"));
    QVERIFY(surface != nullptr);

    QApplication::clipboard()->clear();
    QTest::mouseClick(surface, Qt::LeftButton, Qt::NoModifier, QPoint(120, 120));
    QVERIFY(surface->hasFocus());
    QTest::keyClick(surface, Qt::Key_C, Qt::ControlModifier);

    const QMimeData* mime = QApplication::clipboard()->mimeData();
    QVERIFY(mime != nullptr);
    QVERIFY(mime->hasImage());
    QVERIFY(mime->hasFormat(QStringLiteral("image/png")));
}

QTEST_MAIN(AnalysisChartWidgetTest)
#include "analysis_chart_widget_test.moc"
