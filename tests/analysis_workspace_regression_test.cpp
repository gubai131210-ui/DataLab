#include "domain/column_extract.h"
#include "ui/page_renderer.h"
#include "ui/report_table_model.h"
#include "ui/worksheet_model.h"
#include "ui/worksheet_sort_filter_proxy.h"
#include "reporting/chart_plot_context.h"

#include <QLabel>
#include <QTabWidget>
#include <QTableView>
#include <QComboBox>
#include <QtTest/QtTest>
#include <memory>

class AnalysisWorkspaceRegressionTest final : public QObject {
    Q_OBJECT
private slots:
    void editingAndReplacingMissingCellsAgreeWithExtraction();
    void longTablesDoNotPrecedeChartsAndAreBuiltOnDemand();
    void reportStillIncludesAllTables();
    void numericResultsSortByValue();
    void calculationErrorsRemainVisibleBeforeOrdinaryWarnings();
    void hiddenScatterPointsCannotBeHit();
};

void AnalysisWorkspaceRegressionTest::editingAndReplacingMissingCellsAgreeWithExtraction()
{
    datalab::domain::DataTable table;
    table.columns = {"Value", "Other"};
    table.rows = {{"*", "2"}, {"3", "4"}};
    datalab::domain::populate_data_table_contract(table);
    table.row_ids = {40, 90};
    WorksheetModel model;
    model.set_table(table);
    QVERIFY(model.setData(model.index(0, 0), QStringLiteral("7")));
    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toString(), QStringLiteral("7"));
    QCOMPARE(model.data(model.index(0, 1), Qt::DisplayRole).toString(), QStringLiteral("2"));
    QCOMPARE(datalab::domain::extract_numeric_column(model.table(), 0, {}).values.size(), std::size_t(2));
    auto replacement = model.table();
    replacement.rows[0][0] = "";
    model.replace_table(replacement);
    replacement = model.table();
    replacement.rows[0][0] = "8";
    model.replace_table(replacement);
    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toString(), QStringLiteral("8"));
    QCOMPARE(model.table().row_ids[0], datalab::domain::RowId(40));
    QVERIFY(datalab::domain::validate_data_table_contract(model.table()).empty());
}

static datalab::domain::OutputPage long_result()
{
    datalab::domain::OutputPage page;
    page.title = "Long result";
    datalab::domain::StatisticTable table;
    table.title = "Observations";
    table.headers = {"Value"};
    table.rows.assign(1000, {"2"});
    page.tables = {table, table};
    datalab::domain::PlotSpec plot;
    plot.values = {1.0, 2.0, 3.0};
    page.plots.push_back(plot);
    return page;
}

void AnalysisWorkspaceRegressionTest::longTablesDoNotPrecedeChartsAndAreBuiltOnDemand()
{
    page_renderer::PageRenderOptions options;
    options.interactive_charts = true;
    std::unique_ptr<QWidget> view(page_renderer::build_page_widget(long_result(), nullptr, options));
    auto* sections = view->findChild<QTabWidget*>(QStringLiteral("analysis_result_sections"));
    QVERIFY(sections != nullptr);
    QCOMPARE(sections->tabText(0), QStringLiteral("图表"));
    QVERIFY(view->findChildren<QTableView*>().empty());
    sections->setCurrentIndex(1);
    QCOMPARE(view->findChildren<QTableView*>().size(), 1);
    QVERIFY(view->findChild<QTableView*>()->minimumHeight() < 1000);
    auto* selector = view->findChild<QComboBox*>(QStringLiteral("result_table_selector"));
    QVERIFY(selector != nullptr);
    selector->setCurrentIndex(1);
    QCOMPARE(view->findChildren<QTableView*>().size(), 2);
}

void AnalysisWorkspaceRegressionTest::reportStillIncludesAllTables()
{
    std::unique_ptr<QWidget> view(page_renderer::build_page_widget(long_result(), nullptr));
    QCOMPARE(view->findChildren<QTableView*>().size(), 2);
    QVERIFY(view->findChild<QTabWidget*>(QStringLiteral("analysis_result_sections")) == nullptr);
}

void AnalysisWorkspaceRegressionTest::numericResultsSortByValue()
{
    datalab::ui::ReportTableModel model;
    datalab::domain::StatisticTable table;
    table.headers = {"Value"};
    table.rows = {{"10"}, {"2"}, {"-1"}, {"*"}};
    model.set_table(table);
    datalab::ui::WorksheetSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.sort(0, Qt::AscendingOrder);
    QCOMPARE(proxy.index(0, 0).data().toString(), QStringLiteral("-1"));
    QCOMPARE(proxy.index(1, 0).data().toString(), QStringLiteral("2"));
    QCOMPARE(proxy.index(2, 0).data().toString(), QStringLiteral("10"));
    QCOMPARE(proxy.index(3, 0).data().toString(), QStringLiteral("*"));
}

void AnalysisWorkspaceRegressionTest::calculationErrorsRemainVisibleBeforeOrdinaryWarnings()
{
    auto page = long_result();
    using Severity = datalab::domain::DiagnosticMessage::Severity;
    page.diagnostics = {{Severity::warning, "ordinary", "ordinary warning"},
                        {Severity::error, "failure", "calculation failed"}};
    page_renderer::PageRenderOptions options;
    options.interactive_charts = true;
    std::unique_ptr<QWidget> view(page_renderer::build_page_widget(page, nullptr, options));
    auto* banner = view->findChild<QLabel*>(QStringLiteral("analysis_warning_summary"));
    QVERIFY(banner != nullptr);
    QVERIFY(banner->wordWrap());
    QVERIFY(banner->text().contains(QStringLiteral("calculation failed")));
    QVERIFY(!banner->text().contains(QStringLiteral("ordinary warning")));
}

void AnalysisWorkspaceRegressionTest::hiddenScatterPointsCannotBeHit()
{
    ChartModel model;
    model.kind = ChartKind::Scatter;
    model.x_values = {1.0, 2.0};
    model.values = {3.0, 4.0};
    const QRectF area(0, 0, 640, 480);
    const auto context = build_scatter_plot_context(model, area);
    QVERIFY(context.valid);
    const QPoint point = context.mapper.to_pixel(1.0, 3.0).toPoint();
    QVERIFY(chart_hit_test_point_series(model, area, point).has_value());
    model.value_style.visible = false;
    QVERIFY(!chart_hit_test_point_series(model, area, point).has_value());
    model.value_style.visible = true;
    model.value_style.point_style = ChartPointStyle::None;
    model.kind = ChartKind::TimeSeries;
    QVERIFY(chart_hit_test_point_series(model, area, point).has_value());
}

QTEST_MAIN(AnalysisWorkspaceRegressionTest)
#include "analysis_workspace_regression_test.moc"
