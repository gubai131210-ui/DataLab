#include "ui/graph_properties_dialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QTableWidget>
#include <QTabWidget>
#include <QTest>

class GraphPropertiesDialogTest final : public QObject {
    Q_OBJECT

private slots:
    void controlChartShowsReferenceTab();
    void scatterChartHidesReferenceTab();
    void previewIsRightOfTabs();
    void layoutHasSafeMinimumAndScrollableSeries();
    void seriesTableShowsSeriesLabels();
};

ChartModel make_model(ChartKind kind)
{
    ChartModel model;
    model.kind = kind;
    model.title = QStringLiteral("测试图");
    model.x_axis_title = QStringLiteral("X");
    model.y_axis_title = QStringLiteral("Y");
    ChartSeries series;
    series.label = QStringLiteral("系列 1");
    model.series.push_back(series);
    model.values = {1.0, 2.0, 3.0};
    model.x_values = {1.0, 2.0, 3.0};
    if (kind == ChartKind::Control) {
        model.center = {2.0, 2.0, 2.0};
        model.lower = {1.0, 1.0, 1.0};
        model.upper = {3.0, 3.0, 3.0};
    }
    return model;
}

void GraphPropertiesDialogTest::controlChartShowsReferenceTab()
{
    GraphPropertiesDialog dialog(make_model(ChartKind::Control));
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    const auto* tabs = dialog.findChild<QTabWidget*>();
    QVERIFY(tabs != nullptr);
    bool has_reference = false;
    for (int index = 0; index < tabs->count(); ++index) {
        if (tabs->tabText(index) == QStringLiteral("参考线")) {
            has_reference = true;
        }
    }
    QVERIFY(has_reference);
    dialog.close();
}

void GraphPropertiesDialogTest::scatterChartHidesReferenceTab()
{
    GraphPropertiesDialog dialog(make_model(ChartKind::Scatter));
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    const auto* tabs = dialog.findChild<QTabWidget*>();
    QVERIFY(tabs != nullptr);
    for (int index = 0; index < tabs->count(); ++index) {
        QVERIFY(tabs->tabText(index) != QStringLiteral("参考线"));
    }
    dialog.close();
}

void GraphPropertiesDialogTest::previewIsRightOfTabs()
{
    GraphPropertiesDialog dialog(make_model(ChartKind::Scatter));
    dialog.resize(1080, 700);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    const auto* tabs = dialog.findChild<QTabWidget*>();
    const auto labels = dialog.findChildren<QLabel*>();
    const QLabel* preview = nullptr;
    for (QLabel* label : labels) {
        if (label->minimumWidth() >= 280) {
            preview = label;
            break;
        }
    }
    QVERIFY(tabs != nullptr);
    QVERIFY(preview != nullptr);
    QVERIFY(preview->geometry().x() > tabs->geometry().x());
    dialog.close();
}

void GraphPropertiesDialogTest::layoutHasSafeMinimumAndScrollableSeries()
{
    GraphPropertiesDialog dialog(make_model(ChartKind::Scatter));
    QVERIFY(dialog.minimumSize().width() >= 960);
    QVERIFY(dialog.minimumSize().height() >= 620);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    const auto* buttons = dialog.findChild<QDialogButtonBox*>(
        QStringLiteral("dialog_buttons"));
    const auto* series = dialog.findChild<QTableWidget*>(
        QStringLiteral("series_table"));
    QVERIFY(buttons != nullptr);
    QVERIFY(series != nullptr);
    QVERIFY(buttons->isVisible());
    QVERIFY(series->horizontalScrollBarPolicy() == Qt::ScrollBarAsNeeded);
    QVERIFY(series->verticalScrollBarPolicy() == Qt::ScrollBarAsNeeded);
    dialog.close();
}

void GraphPropertiesDialogTest::seriesTableShowsSeriesLabels()
{
    ChartModel model = make_model(ChartKind::Scatter);
    model.series[0].label = QStringLiteral("实际");
    ChartSeries fitted;
    fitted.label = QStringLiteral("拟合");
    model.series.push_back(fitted);
    GraphPropertiesDialog dialog(model);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    const auto* series = dialog.findChild<QTableWidget*>(QStringLiteral("series_table"));
    QVERIFY(series != nullptr);
    QCOMPARE(series->rowCount(), 2);
    QCOMPARE(series->item(0, 1)->text(), QStringLiteral("实际"));
    QCOMPARE(series->item(1, 1)->text(), QStringLiteral("拟合"));
    dialog.close();
}

QTEST_MAIN(GraphPropertiesDialogTest)
#include "graph_properties_dialog_test.moc"
