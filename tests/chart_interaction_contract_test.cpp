#include "reporting/chart_interaction.h"
#include "reporting/chart_plot_context.h"
#include "reporting/chart_adapter.h"
#include "reporting/chart_renderer.h"
#include "infrastructure/output_serialization.h"
#include "ui/analysis_chart_widget.h"
#include "ui/graph_properties_panel.h"
#include "ui/chart_axis_bound_editor.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QLineF>
#include <QListWidget>
#include <QImage>
#include <QPixmap>
#include <QMenu>
#include <QTest>
#include <QTimer>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
const QRectF area(0.0, 0.0, 800.0, 600.0);

ChartModel points(ChartKind kind = ChartKind::Scatter)
{
    ChartModel model;
    model.kind = kind;
    model.x_values = {0.0, 5.0, 10.0};
    model.values = {0.0, 5.0, 10.0};
    model.source_rows = {10, 30, 50};
    return model;
}
}

class ChartInteractionContractTest final : public QObject {
    Q_OBJECT
private slots:
    void implicitControlCoordinatesAndViewport();
    void aggregateBlankSpaceDoesNotSelect();
    void aggregateBrushWorksWithoutRawValues();
    void probabilityUsesEightPercentPadding();
    void bubbleZoomPreservesCursorData();
    void matrixCellGapsAndPartialVisibility();
    void gridNavigationUsesTheDataViewport();
    void parallelSegmentsAreSelectable();
    void raggedSurfaceIsSafeToRenderAndHit();
    void actualSeriesRoundTripAndForecastIsolation();
    void emptyMembershipAndDerivedHistogramRows();
    void legacyControlTooltipKeepsRuleAndLaneyDetails();
    void navigationSurvivesSerializationAndResize();
    void duplicateSeriesLabelsHaveStableEditorIdentity();
    void crossSeriesMarksAreVisibleAndSelectable();
    void unchangedAxisBoundsRetainExactPrecision();
    void widgetDefersSelectionAndTogglesWithControl();
    void widgetWheelUndoAndCancelledMenu();
};

void ChartInteractionContractTest::implicitControlCoordinatesAndViewport()
{
    auto model = points(ChartKind::Control);
    model.x_values.clear();
    const auto context = build_chart_plot_context(model, area);
    QVERIFY(context.valid);
    const auto hit = chart_interaction::hit_test(model, area, context.mapper.to_pixel(1.0, 5.0));
    QVERIFY(hit.has_value());
    QCOMPARE(hit->index, std::size_t{1});
    QVERIFY(!chart_interaction::hit_test(model, area, QPointF(context.plot.left() - 2.0, context.plot.center().y())).has_value());
    ChartModel empty;
    QVERIFY(!chart_interaction::hit_test(empty, area, area.center()).has_value());
}

void ChartInteractionContractTest::aggregateBlankSpaceDoesNotSelect()
{
    ChartModel model;
    model.kind = ChartKind::Histogram;
    model.histogram_edges = {0.0, 1.0, 2.0};
    model.histogram_counts = {2.0, 10.0};
    auto context = build_chart_plot_context(model, area);
    QVERIFY(!chart_interaction::hit_test(model, area, context.mapper.to_pixel(0.5, 8.0)).has_value());
    QVERIFY(chart_interaction::hit_test(model, area, context.mapper.to_pixel(0.5, 1.0)).has_value());

    model = {};
    model.kind = ChartKind::BoxPlot;
    model.box_min = {0.0}; model.box_q1 = {2.0}; model.box_median = {3.0};
    model.box_q3 = {4.0}; model.box_max = {6.0};
    context = build_chart_plot_context(model, area);
    const auto center = context.mapper.to_pixel(0.0, 3.0);
    QVERIFY(chart_interaction::hit_test(model, area, center).has_value());
    QVERIFY(!chart_interaction::hit_test(model, area, center + QPointF(80.0, 0.0)).has_value());

    model = {};
    model.kind = ChartKind::Interval;
    model.values = {4.0}; model.interval_lower = {2.0}; model.interval_upper = {6.0};
    context = build_chart_plot_context(model, area);
    QVERIFY(!chart_interaction::hit_test(model, area, context.mapper.to_pixel(0.0, 4.0) + QPointF(80.0, 0.0)).has_value());
}

void ChartInteractionContractTest::aggregateBrushWorksWithoutRawValues()
{
    ChartModel model;
    model.kind = ChartKind::Bar;
    model.categories = {QStringLiteral("A"), QStringLiteral("B")};
    model.category_values = {4.0, 8.0};
    model.member_source_rows = {{1, 4}, {8}};
    const auto context = build_chart_plot_context(model, area);
    const QPointF inside = context.mapper.to_pixel(0.0, 2.0);
    const auto marks = chart_interaction::select_rectangle(model, area, QRectF(inside - QPointF(4, 4), QSizeF(8, 8)));
    QCOMPARE(marks, (std::vector<std::size_t>{0}));
    QCOMPARE(chart_interaction::resolve_selected_source_rows(model, marks), (std::vector<std::size_t>{1, 4}));
}

void ChartInteractionContractTest::probabilityUsesEightPercentPadding()
{
    const auto model = points(ChartKind::Probability);
    const auto context = build_chart_plot_context(model, area);
    QCOMPARE(context.mapper.x_min(), -0.8);
    QCOMPARE(context.mapper.x_max(), 10.8);
    const QPointF point(context.plot.left() + context.plot.width() * 0.8 / 11.6,
                        context.plot.bottom() - context.plot.height() * 0.8 / 11.6);
    const auto hit = chart_interaction::hit_test(model, area, point);
    QVERIFY(hit.has_value());
    QCOMPARE(hit->index, std::size_t{0});
}

void ChartInteractionContractTest::bubbleZoomPreservesCursorData()
{
    auto model = points(ChartKind::Bubble);
    model.bubble_sizes = {1.0, 4.0, 9.0};
    const auto before = build_chart_plot_context(model, area);
    const QPointF anchor = before.mapper.to_pixel(5.0, 5.0);
    QVERIFY(chart_interaction::zoom_at(model, area, anchor, 2.0));
    const auto after = build_chart_plot_context(model, area);
    QVERIFY(QLineF(anchor, after.mapper.to_pixel(5.0, 5.0)).length() < 1.0e-8);
    const auto hit = chart_interaction::hit_test(model, area, anchor + QPointF(8.0, 0.0));
    QVERIFY(hit.has_value());
    QCOMPARE(hit->index, std::size_t{1});
    const QPointF off_center = after.plot.topLeft() + QPointF(100.0, 80.0);
    const auto data = after.mapper.to_data(off_center);
    QVERIFY(chart_interaction::zoom_at(model, area, off_center, 1.3));
    QVERIFY(QLineF(off_center, build_chart_plot_context(model, area).mapper.to_pixel(data.x(), data.y())).length() < 1.0e-8);
}

void ChartInteractionContractTest::matrixCellGapsAndPartialVisibility()
{
    ChartModel model;
    model.kind = ChartKind::Heatmap;
    model.matrix_labels = {QStringLiteral("A"), QStringLiteral("B")};
    model.categories = model.matrix_labels;
    model.matrix_values = {{1.0, 2.0}, {3.0, 4.0}};
    auto layout = build_heatmap_grid_layout(model, area);
    QVERIFY(layout.valid);
    QVERIFY(!chart_interaction::hit_test(model, area, layout.origin - QPointF(8.0, -10.0)).has_value());
    chart_set_pan_offset(model, layout.viewport, QPointF(layout.viewport.left() - layout.origin.x() - layout.cell_width * 0.5, 0.0));
    layout = build_heatmap_grid_layout(model, area);
    QCOMPARE(layout.first_column, std::size_t{0});
    const auto hit = chart_interaction::hit_test(model, area, QPointF(layout.viewport.left() + 4.0, layout.origin.y() + 10.0));
    QVERIFY(hit.has_value());
    QCOMPARE(hit->matrix_column, std::size_t{0});
    // The gap is not a cell even when the previous cell is within the pointer tolerance.
    const QPointF gap(layout.origin.x() + layout.cell_width - 1.0, layout.origin.y() + 10.0);
    QVERIFY(!chart_interaction::hit_test(model, area, gap).has_value());
}

void ChartInteractionContractTest::gridNavigationUsesTheDataViewport()
{
    ChartModel model;
    model.matrix_labels = {QStringLiteral("A"), QStringLiteral("B")};
    model.matrix_values = {{1.0, 0.5}, {0.5, 1.0}};
    for (auto kind : {ChartKind::Heatmap, ChartKind::Correlation, ChartKind::Matrix}) {
        model.kind = kind;
        model.view = {};
        const auto layout_for = [&]() {
            if (kind == ChartKind::Heatmap) return build_heatmap_grid_layout(model, area);
            if (kind == ChartKind::Correlation) return build_correlation_grid_layout(model, area);
            return build_matrix_grid_layout(model, area);
        };
        const auto before = layout_for();
        QCOMPARE(before.viewport, chart_plot_area(area, kind));
        chart_set_pan_offset(model, chart_plot_area(area, kind), QPointF(80.0, 40.0));
        const auto after = layout_for();
        QCOMPARE(after.origin - before.origin, QPointF(80.0, 40.0));
    }
    model.kind = ChartKind::Matrix;
    model.matrix_labels.assign(100, QStringLiteral("X"));
    model.matrix_values.resize(100);
    model.view.zoom_factor = 0.25;
    QVERIFY(build_matrix_grid_layout(model, area).cell_width >= 18.0);
    QVERIFY(!build_matrix_grid_layout(model, QRectF(0, 0, 100, 100)).valid);
}

void ChartInteractionContractTest::parallelSegmentsAreSelectable()
{
    ChartModel model;
    model.kind = ChartKind::Parallel;
    model.matrix_labels = {QStringLiteral("A"), QStringLiteral("B")};
    model.matrix_values = {{0.0, 10.0}};
    model.lower = {0.0, 0.0}; model.upper = {10.0, 10.0};
    model.source_rows = {42};
    const auto context = build_chart_plot_context(model, area);
    const auto first = chart_parallel_point(model, context, 0, 0);
    const auto second = chart_parallel_point(model, context, 0, 1);
    QVERIFY(first.has_value() && second.has_value());
    const auto hit = chart_interaction::hit_test(model, area, (*first + *second) / 2.0);
    QVERIFY(hit.has_value());
    QCOMPARE(hit->kind, chart_interaction::HitKind::ParallelObservation);
    QCOMPARE(chart_interaction::resolve_selected_source_rows(model, {hit->index}), (std::vector<std::size_t>{42}));
}

void ChartInteractionContractTest::raggedSurfaceIsSafeToRenderAndHit()
{
    ChartModel model;
    model.kind = ChartKind::Surface;
    model.contour_x = {0.0, 1.0, 2.0}; model.contour_y = {0.0, 1.0};
    model.matrix_values = {{0.0, 1.0, 2.0}, {1.0}};
    const auto context = build_chart_plot_context(model, area);
    QVERIFY(context.valid);
    QVERIFY(!chart_surface_point(model, context, 1, 2).has_value());
    QVERIFY(!ChartRenderer::render_to_pixmap(model, area.size().toSize()).isNull());
    const auto point = chart_surface_point(model, context, 0, 1);
    QVERIFY(point.has_value());
    const auto hit = chart_interaction::hit_test(model, area, *point);
    QVERIFY(hit.has_value());
    QCOMPARE(hit->kind, chart_interaction::HitKind::SurfacePoint);
}

void ChartInteractionContractTest::actualSeriesRoundTripAndForecastIsolation()
{
    ChartModel model;
    model.kind = ChartKind::TimeSeries;
    model.source_rows = {100, 200};
    ChartSeries forecast; forecast.role = ChartSeriesRole::Forecast;
    forecast.label = QStringLiteral("预测"); forecast.values = {8.0, 9.0};
    ChartSeries actual; actual.role = ChartSeriesRole::Actual;
    actual.label = QStringLiteral("实际"); actual.values = {1.0, 3.0};
    model.series = {forecast, actual};
    QCOMPARE(chart_interaction::resolve_selected_source_rows(model, {0}), (std::vector<std::size_t>{}));
    QCOMPARE(chart_interaction::resolve_selected_source_rows(model, {3}), (std::vector<std::size_t>{200}));
    QCOMPARE(chart_interaction::resolve_mark_indices_for_source_rows(model, {200}), (std::vector<std::size_t>{3}));
    const auto context = build_chart_plot_context(model, area);
    const auto hit = chart_interaction::hit_test(model, area, context.mapper.to_pixel(1.0, 3.0));
    QVERIFY(hit.has_value());
    QCOMPARE(hit->series_index, (std::optional<std::size_t>{1}));
    QVERIFY(chart_interaction::tooltip_text(model, *hit).contains(QStringLiteral("实际")));
}

void ChartInteractionContractTest::emptyMembershipAndDerivedHistogramRows()
{
    auto model = points();
    model.member_source_rows = {{}, {30}, {50}};
    QVERIFY(chart_interaction::resolve_selected_source_rows(model, {0}).empty());
    QVERIFY(chart_interaction::resolve_mark_indices_for_source_rows(model, {10}).empty());
    model = points(ChartKind::Histogram);
    model.values = {0.0, 0.8, 2.0};
    model.histogram_edges = {0.0, 1.0, 2.0}; model.histogram_counts = {2.0, 1.0};
    QCOMPARE(chart_interaction::resolve_selected_source_rows(model, {0}), (std::vector<std::size_t>{10, 30}));
    QCOMPARE(chart_interaction::resolve_selected_source_rows(model, {1}), (std::vector<std::size_t>{50}));
    QCOMPARE(chart_interaction::resolve_mark_indices_for_source_rows(model, {50}), (std::vector<std::size_t>{1}));
}

void ChartInteractionContractTest::legacyControlTooltipKeepsRuleAndLaneyDetails()
{
    auto model = points(ChartKind::Control);
    model.special_cause_points = {{1}}; model.sigma_z = 1.25; model.signal_direction = {0, -1, 0};
    const auto text = chart_interaction::tooltip_text(model, {chart_interaction::HitKind::Point, 1});
    QVERIFY(text.contains(QStringLiteral("Sigma Z: 1.25")));
    QVERIFY(text.contains(QStringLiteral("信号方向: 下侧")));
    QVERIFY(!text.contains(QStringLiteral("触发规则: 未触发")));
}

void ChartInteractionContractTest::navigationSurvivesSerializationAndResize()
{
    auto model = points();
    const auto initial = build_chart_plot_context(model, area);
    QVERIFY(chart_interaction::zoom_at(model, area, initial.plot.center(), 2.0));
    chart_set_pan_offset(model, initial.plot, QPointF(40.0, 25.0));
    datalab::domain::OutputPage page;
    page.id = "navigation"; page.plots.push_back(plot_from_chart_model(model));
    const auto restored_page = datalab::infrastructure::output_page_from_json(datalab::infrastructure::output_page_to_json(page));
    QCOMPARE(restored_page.plots.size(), std::size_t{1});
    const auto restored = chart_model_from_plot(restored_page.plots.front());
    const auto before = build_chart_plot_context(model, area);
    const auto after = build_chart_plot_context(restored, QRectF(0.0, 0.0, 1200.0, 900.0));
    QVERIFY(std::abs(before.mapper.x_min() - after.mapper.x_min()) < 1.0e-10);
    QVERIFY(std::abs(before.mapper.y_min() - after.mapper.y_min()) < 1.0e-10);
}

void ChartInteractionContractTest::duplicateSeriesLabelsHaveStableEditorIdentity()
{
    auto model = points();
    model.series.resize(2);
    for (auto& series : model.series) series.label = QStringLiteral("相同名称");
    GraphPropertiesPanel panel(model);
    panel.set_selected_series(1);
    auto* list = panel.findChild<QListWidget*>();
    QVERIFY(list != nullptr && list->currentItem() != nullptr);
    QCOMPARE(list->currentItem()->data(Qt::UserRole).toULongLong(), qulonglong{1});
    panel.refresh_model(model);
    QCOMPARE(list->currentItem()->data(Qt::UserRole).toULongLong(), qulonglong{1});
    model.kind = ChartKind::Control;
    model.series[0].label = QStringLiteral("CL");
    panel.refresh_model(model);
    panel.set_selected_path(QStringLiteral("图形 > 控制线 / CL"));
    QCOMPARE(list->currentItem()->text(), QStringLiteral("控制线 / CL"));
}

void ChartInteractionContractTest::crossSeriesMarksAreVisibleAndSelectable()
{
    ChartModel model;
    model.kind = ChartKind::Scatter;
    model.show_grid = false;
    ChartSeries series;
    series.values = {5.0}; series.x_values = {5.0}; series.show_points = true;
    series.style.point_style = ChartPointStyle::Cross;
    series.style.point_size = 5.0;
    series.style.color = QStringLiteral("#ff00ff");
    model.series = {series};
    const auto context = build_chart_plot_context(model, area);
    const auto center = context.mapper.to_pixel(5.0, 5.0).toPoint();
    const auto hit = chart_interaction::hit_test(model, area, center);
    QVERIFY(hit.has_value());
    QCOMPARE(hit->kind, chart_interaction::HitKind::SeriesPoint);
    const auto image = ChartRenderer::render_to_pixmap(model, area.size().toSize()).toImage();
    bool visible = false;
    for (int y = center.y() - 5; y <= center.y() + 5; ++y)
        for (int x = center.x() - 5; x <= center.x() + 5; ++x) {
            const auto color = image.pixelColor(x, y);
            visible = visible || (color.red() > color.green() + 80 && color.blue() > color.green() + 80);
        }
    QVERIFY(visible);
}

void ChartInteractionContractTest::unchangedAxisBoundsRetainExactPrecision()
{
    QDoubleSpinBox spin;
    spin.setRange(-1.0e12, 1.0e12);
    spin.setDecimals(10);
    const double narrow = 1.234567891234e-12;
    chart_axis_bound_editor::load(&spin, narrow, 0.0);
    QCOMPARE(chart_axis_bound_editor::read(&spin), narrow);
    spin.setValue(2.5);
    QCOMPARE(chart_axis_bound_editor::read(&spin), 2.5);
    const double epoch = 1800000000.125;
    chart_axis_bound_editor::load(&spin, epoch, 0.0);
    QCOMPARE(chart_axis_bound_editor::read(&spin), epoch);
}

void ChartInteractionContractTest::widgetDefersSelectionAndTogglesWithControl()
{
    AnalysisChartWidget widget;
    widget.resize(800, 600); widget.set_model(points()); widget.show();
    QApplication::processEvents();
    auto* surface = widget.findChild<QWidget*>(QStringLiteral("chart_surface"));
    QVERIFY(surface != nullptr);
    const auto context = build_chart_plot_context(points(), surface->rect());
    const QPoint position = context.mapper.to_pixel(5.0, 5.0).toPoint();
    std::vector<std::vector<std::size_t>> selections;
    connect(&widget, &AnalysisChartWidget::rows_selected, &widget, [&](const auto& rows) { selections.push_back(rows); });
    QTest::mousePress(surface, Qt::LeftButton, Qt::NoModifier, position);
    QVERIFY(selections.empty());
    QTest::mouseRelease(surface, Qt::LeftButton, Qt::NoModifier, position);
    QCOMPARE(selections.back(), (std::vector<std::size_t>{30}));
    QTest::mouseClick(surface, Qt::LeftButton, Qt::ControlModifier, position);
    QVERIFY(selections.back().empty());
    QTest::mousePress(surface, Qt::LeftButton, Qt::NoModifier, position - QPoint(20, 20));
    QTest::mouseMove(surface, position + QPoint(20, 20));
    const auto before_release = selections.size();
    QTest::mouseRelease(surface, Qt::LeftButton, Qt::NoModifier, position + QPoint(20, 20));
    QCOMPARE(selections.size(), before_release + 1);
    QCOMPARE(selections.back(), (std::vector<std::size_t>{30}));
    QTest::mouseClick(surface, Qt::LeftButton, Qt::NoModifier, context.plot.topLeft().toPoint() + QPoint(30, 30));
    QVERIFY(selections.back().empty());
}

void ChartInteractionContractTest::widgetWheelUndoAndCancelledMenu()
{
    AnalysisChartWidget widget;
    widget.resize(800, 600); widget.set_model(points()); widget.show();
    QApplication::processEvents();
    auto* surface = widget.findChild<QWidget*>(QStringLiteral("chart_surface"));
    QVERIFY(surface != nullptr);
    const QPointF position = surface->rect().center();
    QWheelEvent plain(position, surface->mapToGlobal(position.toPoint()), {}, QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(surface, &plain);
    QVERIFY(!plain.isAccepted());
    ChartModel displayed;
    int changes = 0;
    connect(&widget, &AnalysisChartWidget::display_properties_changed, &widget, [&](const ChartModel& model) { displayed = model; ++changes; });
    QWheelEvent zoom(position, surface->mapToGlobal(position.toPoint()), {}, QPoint(0, 120), Qt::NoButton, Qt::ControlModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(surface, &zoom);
    QVERIFY(zoom.isAccepted());
    QVERIFY(displayed.view.zoom_factor > 1.0);
    QTest::keyClick(surface, Qt::Key_Z, Qt::ControlModifier);
    QCOMPARE(displayed.view.zoom_factor, 1.0);
    QVERIFY(changes >= 2);
    QTimer::singleShot(0, [] { if (auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget())) menu->close(); });
    QContextMenuEvent menu_event(QContextMenuEvent::Mouse, QPoint(4, 4), surface->mapToGlobal(QPoint(4, 4)));
    QApplication::sendEvent(surface, &menu_event);
    QVERIFY(widget.findChild<GraphPropertiesPanel*>() == nullptr);
}

QTEST_MAIN(ChartInteractionContractTest)
#include "chart_interaction_contract_test.moc"
