#include "domain/column_extract.h"
#include "infrastructure/output_serialization.h"
#include "infrastructure/table_fingerprint.h"
#include "reporting/chart_coordinate_mapper.h"
#include "reporting/chart_plot_context.h"
#include "reporting/chart_renderer.h"
#include "ui/analysis_chart_widget.h"
#include "ui/output_workspace.h"

#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QMetaObject>
#include <QTabBar>
#include <QTest>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
using datalab::domain::DataTable;
using datalab::domain::OutputPage;
using datalab::infrastructure::table_fingerprint;

DataTable worksheet(const std::string& name)
{
    DataTable table;
    table.name = name;
    table.source_path = "source.csv";
    table.columns = {"X", "Y"};
    table.rows = {{"1", "2"}, {"3", "4"}};
    return table;
}

OutputPage result(const DataTable& table, const std::string& id)
{
    OutputPage page;
    page.id = id;
    page.title = id;
    page.source_data_fingerprint = table_fingerprint(table);
    page.source_worksheet_name = table.name;
    datalab::domain::PlotSpec plot;
    plot.title = id;
    plot.kind = datalab::domain::PlotKind::scatter;
    plot.x_values = {1.0, 3.0};
    plot.values = {2.0, 4.0};
    plot.source_rows = {0, 1};
    page.plots.push_back(plot);
    return page;
}

AnalysisChartWidget* chart_on(OutputWorkspace& workspace, int index)
{
    return workspace.widget(index)->findChild<AnalysisChartWidget*>();
}
}

class AnalysisSourceBindingTest final : public QObject {
    Q_OBJECT
private slots:
    void fingerprintsTrackSourceAndRowOrder();
    void fingerprintsRespectBoundariesAndCanonicalPadding();
    void bindingsSurviveSerializationAndLegacyResultsStayUnbound();
    void chartRowsOnlyReachTheMatchingWorksheet();
    void worksheetSelectionOnlyHighlightsMatchingResults();
    void closedPagesCannotWriteIntoSurvivingDuplicateIds();
    void viewRangeKeepsSmallUnitsAndIdentityZoom();
    void smallAggregateRangesKeepTheirScale();
    void invalidNavigationDoesNotPoisonTheMapper();
};

void AnalysisSourceBindingTest::fingerprintsTrackSourceAndRowOrder()
{
    const auto original = worksheet("A");
    const auto fingerprint = table_fingerprint(original);
    QVERIFY(!fingerprint.empty());
    auto modified = original;
    modified.name = "B";
    QVERIFY(table_fingerprint(modified) != fingerprint);
    modified = original;
    std::swap(modified.rows[0], modified.rows[1]);
    QVERIFY(table_fingerprint(modified) != fingerprint);
    modified = original;
    modified.rows[0][0] = "99";
    QVERIFY(table_fingerprint(modified) != fingerprint);
    modified.rows[0][0] = "1";
    QCOMPARE(table_fingerprint(modified), fingerprint);
    modified.columns[0] = "Other";
    QVERIFY(table_fingerprint(modified) != fingerprint);
    modified = original;
    modified.source_path = "other.csv";
    QVERIFY(table_fingerprint(modified) != fingerprint);
}

void AnalysisSourceBindingTest::fingerprintsRespectBoundariesAndCanonicalPadding()
{
    auto a = worksheet("中文工作表");
    auto b = a;
    a.rows[0] = {"ab", "c"};
    b.rows[0] = {"a", "bc"};
    QVERIFY(table_fingerprint(a) != table_fingerprint(b));
    a.rows[0] = {"1"};
    b = a;
    datalab::domain::refresh_data_table_contract(b);
    QCOMPARE(table_fingerprint(a), table_fingerprint(b));
    QVERIFY(table_fingerprint(DataTable{}).empty());
}

void AnalysisSourceBindingTest::bindingsSurviveSerializationAndLegacyResultsStayUnbound()
{
    const auto page = result(worksheet("A"), "saved");
    auto json = datalab::infrastructure::output_page_to_json(page);
    const auto restored = datalab::infrastructure::output_page_from_json(json);
    QCOMPARE(restored.source_data_fingerprint, page.source_data_fingerprint);
    QCOMPARE(restored.source_worksheet_name, page.source_worksheet_name);
    json.remove(QStringLiteral("source_data_fingerprint"));
    json.remove(QStringLiteral("source_worksheet_name"));
    const auto legacy = datalab::infrastructure::output_page_from_json(json);
    QVERIFY(legacy.source_data_fingerprint.empty());
}

void AnalysisSourceBindingTest::chartRowsOnlyReachTheMatchingWorksheet()
{
    const auto a = worksheet("A");
    auto current = table_fingerprint(a);
    OutputWorkspace workspace;
    workspace.set_source_fingerprint_provider([&current] { return current; });
    workspace.add_page(result(a, "A-result"));
    auto* chart = chart_on(workspace, 0);
    QVERIFY(chart != nullptr);
    std::vector<std::vector<std::size_t>> received;
    int blocked = 0;
    connect(&workspace, &OutputWorkspace::rows_selected, &workspace,
            [&](const auto& rows) { received.push_back(rows); });
    connect(&workspace, &OutputWorkspace::source_link_unavailable, &workspace,
            [&](const QString&) { ++blocked; });
    chart->rows_selected({1});
    QCOMPARE(received, (std::vector<std::vector<std::size_t>>{{1}}));
    current = table_fingerprint(worksheet("B"));
    workspace.invalidate_source_selection();
    chart->rows_selected({0});
    QCOMPARE(received.size(), std::size_t{1});
    QCOMPARE(blocked, 1);
    auto* label = workspace.widget(0)->findChild<QLabel*>(QStringLiteral("source_link_status"));
    QVERIFY(label != nullptr && label->text().contains(QStringLiteral("已暂停")));
    current = table_fingerprint(a);
    chart->rows_selected({0});
    QCOMPARE(received.back(), (std::vector<std::size_t>{0}));
    auto legacy = result(a, "legacy");
    legacy.source_data_fingerprint.clear();
    workspace.add_page(legacy);
    chart_on(workspace, 1)->rows_selected({0});
    QCOMPARE(blocked, 2);
}

void AnalysisSourceBindingTest::worksheetSelectionOnlyHighlightsMatchingResults()
{
    const auto a = worksheet("A");
    const auto b = worksheet("B");
    auto current = table_fingerprint(a);
    OutputWorkspace workspace;
    workspace.set_source_fingerprint_provider([&current] { return current; });
    workspace.add_page(result(a, "A-result"));
    workspace.add_page(result(b, "B-result"));
    auto* chart_a = chart_on(workspace, 0);
    auto* chart_b = chart_on(workspace, 1);
    QVERIFY(chart_a != nullptr && chart_b != nullptr);
    workspace.set_selected_source_rows({1});
    QCOMPARE(chart_a->selected_source_rows(), (std::vector<std::size_t>{1}));
    QVERIFY(chart_b->selected_source_rows().empty());
    current = table_fingerprint(b);
    workspace.invalidate_source_selection();
    QVERIFY(chart_a->selected_source_rows().empty());
    workspace.set_selected_source_rows({0});
    QVERIFY(chart_a->selected_source_rows().empty());
    QCOMPARE(chart_b->selected_source_rows(), (std::vector<std::size_t>{0}));
    auto edited = b;
    edited.rows[0][0] = "999";
    current = table_fingerprint(edited);
    workspace.invalidate_source_selection();
    workspace.set_selected_source_rows({0});
    QVERIFY(chart_a->selected_source_rows().empty());
    QVERIFY(chart_b->selected_source_rows().empty());
}

void AnalysisSourceBindingTest::closedPagesCannotWriteIntoSurvivingDuplicateIds()
{
    const auto table = worksheet("A");
    const auto current = table_fingerprint(table);
    OutputWorkspace workspace;
    workspace.set_source_fingerprint_provider([&current] { return current; });
    workspace.add_page(result(table, "duplicate"));
    workspace.add_page(result(table, "duplicate"));
    auto* first = chart_on(workspace, 0);
    auto* second = chart_on(workspace, 1);
    QVERIFY(first != nullptr && second != nullptr);
    ChartModel edited;
    edited.title = QStringLiteral("second-edited");
    second->display_properties_changed(edited);
    QCOMPARE(workspace.pages()[0].plots[0].title, std::string("duplicate"));
    QCOMPARE(workspace.pages()[1].plots[0].title, std::string("second-edited"));
    QVERIFY(QMetaObject::invokeMethod(workspace.tabBar(), "tabCloseRequested",
                                     Qt::DirectConnection, Q_ARG(int, 0)));
    edited.title = QStringLiteral("closed-edit");
    first->display_properties_changed(edited);
    QCOMPARE(workspace.pages()[0].plots[0].title, std::string("second-edited"));
}

void AnalysisSourceBindingTest::viewRangeKeepsSmallUnitsAndIdentityZoom()
{
    ChartCoordinateMapper mapper(QRectF(0, 0, 400, 200));
    mapper.set_data_range(1.0e-15, 2.0e-15, -2.0e-18, -1.0e-18);
    mapper.zoom(1.0, QPointF(123, 45));
    QCOMPARE(mapper.x_min(), 1.0e-15);
    QCOMPARE(mapper.x_max(), 2.0e-15);
    QCOMPARE(mapper.y_min(), -2.0e-18);
    const QPointF anchor(100, 50);
    const auto data = mapper.to_data(anchor);
    mapper.zoom(2.0, anchor);
    QVERIFY(std::abs((mapper.x_max() - mapper.x_min()) / 5.0e-16 - 1.0) < 1.0e-12);
    QVERIFY(std::abs((mapper.y_max() - mapper.y_min()) / 5.0e-19 - 1.0) < 1.0e-12);
    const auto restored = mapper.to_pixel(data.x(), data.y());
    QVERIFY(std::abs(restored.x() - anchor.x()) < 1.0e-9);
    QVERIFY(std::abs(restored.y() - anchor.y()) < 1.0e-9);
}

void AnalysisSourceBindingTest::smallAggregateRangesKeepTheirScale()
{
    ChartModel model;
    model.box_min = {1.0e-15}; model.box_q1 = {1.5e-15};
    model.box_median = {2.0e-15}; model.box_q3 = {2.5e-15}; model.box_max = {3.0e-15};
    for (auto kind : {ChartKind::BoxPlot, ChartKind::Violin}) {
        model.kind = kind;
        if (kind == ChartKind::Violin) {
            ChartSeries density;
            density.x_values = {0.0, 0.6, 0.0};
            density.values = {1.0e-15, 2.0e-15, 3.0e-15};
            model.series = {density};
        }
        const auto context = build_chart_plot_context(model, QRectF(0, 0, 800, 600));
        QVERIFY(context.valid);
        QVERIFY(context.mapper.y_min() < 1.0e-15);
        QVERIFY(context.mapper.y_max() > 3.0e-15);
        QVERIFY(context.mapper.y_max() - context.mapper.y_min() < 1.0e-14);
        if (kind == ChartKind::Violin) {
            const auto image = ChartRenderer::render_to_pixmap(model, QSize(800, 600)).toImage();
            const auto inside = context.mapper.to_pixel(0.1, 2.0e-15).toPoint();
            const auto color = image.pixelColor(inside);
            QVERIFY(color.blue() > color.red() + 15);
        }
    }
}

void AnalysisSourceBindingTest::invalidNavigationDoesNotPoisonTheMapper()
{
    ChartCoordinateMapper mapper(QRectF(0, 0, 400, 200));
    mapper.set_data_range(0.0, 10.0, 0.0, 20.0);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    mapper.zoom(2.0, QPointF(nan, 5.0));
    mapper.pan(QPointF(1.0, nan));
    mapper.zoom(std::numeric_limits<double>::infinity(), QPointF(2.0, 2.0));
    QCOMPARE(mapper.x_min(), 0.0);
    QCOMPARE(mapper.x_max(), 10.0);
    QCOMPARE(mapper.y_min(), 0.0);
    mapper.set_plot_rect(QRectF());
    mapper.zoom(2.0, QPointF(2.0, 2.0));
    QCOMPARE(mapper.x_max(), 10.0);
}

QTEST_MAIN(AnalysisSourceBindingTest)
#include "analysis_source_binding_test.moc"
