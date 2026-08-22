#include "ui/output_workspace.h"
#include "ui/analysis_chart_widget.h"

#include "domain/quality_types.h"

#include <QApplication>
#include <QScrollArea>
#include <QSignalSpy>
#include <QTest>

class OutputWorkspaceTest final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void copyShortcutOnPageEmitsSignal();
    void lastFocusedChartPreferredForCopy();
    void cleanupTestCase();

private:
    int argc_ = 1;
    char argv0_[2] = {'t', '\0'};
    char* argv_[1] = {argv0_};
    QApplication* app_ = nullptr;
};

void OutputWorkspaceTest::initTestCase()
{
    app_ = new QApplication(argc_, argv_);
}

void OutputWorkspaceTest::cleanupTestCase()
{
    delete app_;
    app_ = nullptr;
}

void OutputWorkspaceTest::copyShortcutOnPageEmitsSignal()
{
    OutputWorkspace workspace;
    workspace.show();
    QVERIFY(QTest::qWaitForWindowExposed(&workspace));

    datalab::domain::OutputPage page;
    page.id = "test_page";
    page.title = "Test";
    page.method_name = "Test method";
    page.parameter_summary = "n=3";
    datalab::domain::StatisticTable table;
    table.title = "Summary";
    table.headers = {"x"};
    table.rows = {{"1"}};
    page.tables.push_back(table);

    QSignalSpy copy_spy(&workspace, &OutputWorkspace::copy_chart_requested);
    workspace.add_page(page);
    QVERIFY(workspace.count() > 0);

    QWidget* scroll = workspace.currentWidget();
    QVERIFY(scroll != nullptr);
    auto* scroll_area = qobject_cast<QScrollArea*>(scroll);
    QVERIFY(scroll_area != nullptr);
    QWidget* page_widget = scroll_area->widget();
    QVERIFY(page_widget != nullptr);
    page_widget->setFocus(Qt::OtherFocusReason);
    QVERIFY(page_widget->hasFocus() || scroll_area->isAncestorOf(QApplication::focusWidget()));
    QTest::keyClick(page_widget, Qt::Key_C, Qt::ControlModifier);
    QCOMPARE(copy_spy.count(), 1);
}

void OutputWorkspaceTest::lastFocusedChartPreferredForCopy()
{
    OutputWorkspace workspace;
    workspace.show();
    QVERIFY(QTest::qWaitForWindowExposed(&workspace));

    datalab::domain::OutputPage page;
    page.id = "sixpack_like";
    page.title = "Two plots";
    page.method_name = "Test";
    datalab::domain::PlotSpec plot_a;
    plot_a.title = "A";
    plot_a.kind = datalab::domain::PlotKind::scatter;
    plot_a.values = {1.0, 2.0, 3.0};
    plot_a.x_values = {1.0, 2.0, 3.0};
    datalab::domain::PlotSpec plot_b;
    plot_b.title = "B";
    plot_b.kind = datalab::domain::PlotKind::scatter;
    plot_b.values = {2.0, 3.0, 4.0};
    plot_b.x_values = {1.0, 2.0, 3.0};
    page.plots.push_back(plot_a);
    page.plots.push_back(plot_b);

    workspace.add_page(page);
    const auto charts = workspace.findChildren<AnalysisChartWidget*>();
    QCOMPARE(charts.size(), 2);

    QWidget* surface = charts[1]->findChild<QWidget*>(QStringLiteral("chart_surface"));
    QVERIFY(surface != nullptr);
    QTest::mouseClick(surface, Qt::LeftButton, Qt::NoModifier, QPoint(80, 80));
    QCOMPARE(workspace.chart_for_copy(), charts[1]);
}

QTEST_MAIN(OutputWorkspaceTest)
#include "output_workspace_test.moc"
