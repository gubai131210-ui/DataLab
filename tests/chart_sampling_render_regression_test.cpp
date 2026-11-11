#include "reporting/chart_downsampling.h"
#include "reporting/chart_interaction.h"
#include "reporting/chart_plot_context.h"
#include "reporting/chart_renderer.h"
#include "reporting/chart_adapter.h"
#include "reporting/chart_series_contract.h"
#include "application/graph_service.h"

#include <QColor>
#include <QImage>
#include <QPixmap>
#include <QTest>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace {
const QRectF area(0.0, 0.0, 800.0, 600.0);

bool has_magenta(const QImage& image, const QPoint& point, int radius = 2)
{
    for (int y = point.y() - radius; y <= point.y() + radius; ++y)
        for (int x = point.x() - radius; x <= point.x() + radius; ++x) {
            if (!image.rect().contains(x, y)) continue;
            const auto color = image.pixelColor(x, y);
            if (color.red() > color.green() + 30 && color.blue() > color.green() + 30) return true;
        }
    return false;
}

bool blue_fill(const QColor& color)
{
    return color.blue() > color.red() + 25 && color.blue() > color.green();
}

ChartModel plain_model(ChartKind kind)
{
    ChartModel model;
    model.kind = kind;
    model.title.clear();
    model.x_axis_title.clear();
    model.y_axis_title.clear();
    model.show_grid = false;
    model.show_legend = false;
    model.value_style.color = QStringLiteral("#ff00ff");
    model.value_style.point_style = ChartPointStyle::None;
    model.value_style.line_width = 2.0;
    return model;
}
}

class ChartSamplingRenderRegressionTest final : public QObject {
    Q_OBJECT
private slots:
    void invalidSamplesCannotPoisonBuckets();
    void viewportSamplingRetainsLocalPoints();
    void pointSamplingTracksBothAxes();
    void pointSamplingPreservesVerticalSpread();
    void sampleBudgetAndOverflowRangesStayValid();
    void missingSamplesBreakLineAndArea();
    void confidenceBandsBreakAtMissingValues();
    void auxiliarySeriesDoNotSuppressPrimaryLine();
    void graphServiceMultiSeriesKeepsSeriesBoundariesAndRows();
    void exactFittedSeriesDoesNotHideActual();
    void transparentSeriesCannotBeHitOrBrushed();
    void partialOpacityChangesRenderedSeries();
    void transparentAggregateSeriesDoNotExpandRanges();
    void marginalHistogramsShareZoomedAxes();
    void marginalHistogramsSupportTinyUnitsAndShortEdges();
};

void ChartSamplingRenderRegressionTest::invalidSamplesCannotPoisonBuckets()
{
    std::vector<double> x(400), y(400, 0.0);
    std::iota(x.begin(), x.end(), 0.0);
    x[0] = std::numeric_limits<double>::quiet_NaN();
    x[50] = std::numeric_limits<double>::infinity();
    y[100] = std::numeric_limits<double>::quiet_NaN();
    y[213] = 900.0;
    const auto sampled = datalab::reporting::chart_downsample_indices(x, y, area, 32);
    QVERIFY(sampled.size() > 8 && sampled.size() <= 32);
    QVERIFY(std::is_sorted(sampled.begin(), sampled.end()));
    QVERIFY(std::find(sampled.begin(), sampled.end(), 213) != sampled.end());
    for (std::size_t index : sampled) QVERIFY(std::isfinite(x[index]) && std::isfinite(y[index]));
    const auto small = datalab::reporting::chart_downsample_indices(
        {0.0, 1.0, 2.0}, {1.0, std::numeric_limits<double>::quiet_NaN(), 3.0}, area);
    QCOMPARE(small, (std::vector<std::size_t>{0, 2}));
}

void ChartSamplingRenderRegressionTest::viewportSamplingRetainsLocalPoints()
{
    std::vector<double> x(10000), y(10000, 0.0);
    std::iota(x.begin(), x.end(), 0.0);
    y[5006] = 10.0; y[5008] = -10.0;
    ChartCoordinateMapper mapper(area);
    mapper.set_data_range(5000.0, 5010.0, -20.0, 20.0);
    const auto sampled = datalab::reporting::chart_downsample_indices(x, y, area, mapper, 16);
    QCOMPARE(sampled.size(), std::size_t{13});
    QVERIFY(std::find(sampled.begin(), sampled.end(), 5005) != sampled.end());
    QVERIFY(std::find(sampled.begin(), sampled.end(), 5006) != sampled.end());
    QVERIFY(std::find(sampled.begin(), sampled.end(), 5008) != sampled.end());
    QCOMPARE(sampled.front(), std::size_t{4999});
    QCOMPARE(sampled.back(), std::size_t{5011});
    const auto crossing = datalab::reporting::chart_downsample_indices(
        {-100.0, 100.0}, {0.0, 1.0}, area, mapper, 16);
    QVERIFY(crossing.empty());
    mapper.set_data_range(-1.0, 1.0, 0.0, 1.0);
    QCOMPARE(datalab::reporting::chart_downsample_indices(
        {-100.0, 100.0}, {0.0, 1.0}, area, mapper, 16), (std::vector<std::size_t>{0, 1}));
}

void ChartSamplingRenderRegressionTest::pointSamplingTracksBothAxes()
{
    std::vector<double> x(10000), y(10000);
    std::iota(x.begin(), x.end(), 0.0);
    for (std::size_t index = 0; index < y.size(); ++index) y[index] = index % 2 ? 100.0 : 0.0;
    y[5005] = 50.0;
    ChartCoordinateMapper mapper(area);
    mapper.set_data_range(0.0, 10000.0, 40.0, 60.0);
    QCOMPARE(datalab::reporting::chart_visible_point_indices(x, y, area, mapper, 16),
             (std::vector<std::size_t>{5005}));
    const auto edge = datalab::reporting::chart_visible_point_indices(
        {-1.0}, {50.0}, area, mapper, 16);
    QCOMPARE(edge, (std::vector<std::size_t>{0}));
    x.assign(10000, 5.0);
    for (std::size_t index = 0; index < y.size(); ++index) y[index] = index % 2 ? 60.7 : 39.3;
    y[5005] = 50.0;
    mapper.set_data_range(0.0, 10.0, 40.0, 60.0);
    const auto crowded_edge = datalab::reporting::chart_visible_point_indices(x, y, area, mapper, 16);
    QVERIFY(std::find(crowded_edge.begin(), crowded_edge.end(), 5005) != crowded_edge.end());
    QVERIFY(crowded_edge.size() <= 16);
}

void ChartSamplingRenderRegressionTest::pointSamplingPreservesVerticalSpread()
{
    std::vector<double> x(10000, 5.0), y(10000);
    for (std::size_t index = 0; index < y.size(); ++index) y[index] = index % 101;
    ChartCoordinateMapper mapper(area);
    mapper.set_data_range(0.0, 10.0, 0.0, 100.0);
    const auto sampled = datalab::reporting::chart_visible_point_indices(x, y, area, mapper, 64);
    QVERIFY(sampled.size() > 4 && sampled.size() <= 64);
    QVERIFY(std::any_of(sampled.begin(), sampled.end(), [&](std::size_t index) {
        return y[index] > 25.0 && y[index] < 75.0;
    }));
    QVERIFY(std::any_of(sampled.begin(), sampled.end(), [&](std::size_t index) { return y[index] == 0.0; }));
    QVERIFY(std::any_of(sampled.begin(), sampled.end(), [&](std::size_t index) { return y[index] == 100.0; }));
}

void ChartSamplingRenderRegressionTest::sampleBudgetAndOverflowRangesStayValid()
{
    const double maximum = std::numeric_limits<double>::max();
    const std::vector<double> x{-maximum, -maximum / 2.0, 0.0, maximum / 2.0, maximum};
    const std::vector<double> y{0.0, 3.0, -2.0, 1.0, 0.0};
    for (std::size_t budget : {1, 2, 3, 4}) {
        const auto sampled = datalab::reporting::chart_downsample_indices(x, y, area, budget);
        QVERIFY(!sampled.empty() && sampled.size() <= budget);
        for (std::size_t index : sampled) QVERIFY(index < x.size());
    }
}

void ChartSamplingRenderRegressionTest::missingSamplesBreakLineAndArea()
{
    auto model = plain_model(ChartKind::TimeSeries);
    model.x_values.resize(2000); model.values.assign(2000, 1.0);
    std::iota(model.x_values.begin(), model.x_values.end(), 0.0);
    for (std::size_t index = 900; index <= 1100; ++index)
        model.values[index] = std::numeric_limits<double>::quiet_NaN();
    QVERIFY(datalab::reporting::chart_samples_are_connected(model.x_values, model.values, 10, 20));
    QVERIFY(!datalab::reporting::chart_samples_are_connected(model.x_values, model.values, 899, 1101));
    for (auto kind : {ChartKind::TimeSeries, ChartKind::Area}) {
        model.kind = kind;
        const auto context = build_chart_plot_context(model, area);
        const auto image = ChartRenderer::render_to_pixmap(model, area.size().toSize()).toImage();
        const double y = kind == ChartKind::Area ? 0.5 : 1.0;
        QVERIFY(has_magenta(image, context.mapper.to_pixel(500.0, y).toPoint()));
        QVERIFY(!has_magenta(image, context.mapper.to_pixel(1000.0, y).toPoint()));
    }
}

void ChartSamplingRenderRegressionTest::confidenceBandsBreakAtMissingValues()
{
    auto model = plain_model(ChartKind::Scatter);
    model.value_style.visible = false;
    ChartSeries band;
    band.role = ChartSeriesRole::ConfidenceBand;
    band.x_values = {0.0, 1.0, 2.0, 3.0, 4.0};
    const double nan = std::numeric_limits<double>::quiet_NaN();
    band.lower = {0.0, 0.0, nan, 0.0, 0.0};
    band.upper = {2.0, 2.0, nan, 2.0, 2.0};
    band.style.fill_color = QStringLiteral("#ff00ff");
    model.series = {band};
    for (auto kind : {ChartKind::Scatter, ChartKind::Control}) {
        model.kind = kind;
        const auto context = build_chart_plot_context(model, area);
        const auto image = ChartRenderer::render_to_pixmap(model, area.size().toSize()).toImage();
        QVERIFY(has_magenta(image, context.mapper.to_pixel(0.5, 1.0).toPoint()));
        QVERIFY(!has_magenta(image, context.mapper.to_pixel(2.0, 1.0).toPoint()));
    }
}

void ChartSamplingRenderRegressionTest::auxiliarySeriesDoNotSuppressPrimaryLine()
{
    auto model = plain_model(ChartKind::TimeSeries);
    model.x_values = {0.0, 4.0}; model.values = {1.0, 1.0};
    ChartSeries extra;
    extra.x_values = {0.0, 4.0}; extra.values = {1.8, 1.8};
    extra.style.color = QStringLiteral("#00ffff");
    model.series = {extra};
    const auto context = build_chart_plot_context(model, area);
    const auto image = ChartRenderer::render_to_pixmap(model, area.size().toSize()).toImage();
    QVERIFY(has_magenta(image, context.mapper.to_pixel(2.0, 1.0).toPoint()));
}

void ChartSamplingRenderRegressionTest::graphServiceMultiSeriesKeepsSeriesBoundariesAndRows()
{
    datalab::domain::DataTable table;
    table.columns = {"Time", "A", "B"};
    table.rows = {{"0", "1", "8"}, {"1", "2", "7"}, {"2", "1", "8"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.graph.variable_columns = {1, 2};
    configuration.graph.time_column = 0;
    const auto page = datalab::application::GraphService::time_series(table, configuration);
    QCOMPARE(page.plots.size(), std::size_t{1});
    auto model = chart_model_from_plot(page.plots.front());
    model.title.clear(); model.show_grid = false; model.show_legend = false;
    model.value_style.color = QStringLiteral("#ff00ff");
    QVERIFY(datalab::reporting::chart_primary_mirrors_series(model));
    auto context = build_chart_plot_context(model, area);
    QVERIFY(context.primary_mirrors_series);
    const auto image = ChartRenderer::render_to_pixmap(model, area.size().toSize()).toImage();
    QVERIFY(!has_magenta(image, context.mapper.to_pixel(1.0, 4.5).toPoint()));
    const auto hit = chart_interaction::hit_test(model, area, context.mapper.to_pixel(1.0, 2.0));
    QVERIFY(hit.has_value() && hit->series_index == 0);
    QCOMPARE(chart_interaction::resolve_selected_source_rows(model, {hit->index}),
             (std::vector<std::size_t>{1}));
    const auto selected = chart_interaction::resolve_mark_indices_for_source_rows(model, {1});
    QVERIFY(std::find(selected.begin(), selected.end(), 7) != selected.end());
    QVERIFY(std::find(selected.begin(), selected.end(), 10) != selected.end());
    model.series[0].style.visible = false;
    context = build_chart_plot_context(model, area);
    QVERIFY(context.mapper.y_min() > 6.0);
    QVERIFY(!chart_interaction::hit_test(model, area, context.mapper.to_pixel(1.0, 2.0)).has_value());
}

void ChartSamplingRenderRegressionTest::exactFittedSeriesDoesNotHideActual()
{
    auto model = plain_model(ChartKind::TimeSeries);
    model.x_values = {0.0, 1.0}; model.values = {1.0, 2.0}; model.source_rows = {10, 20};
    ChartSeries series;
    series.x_values = model.x_values; series.values = model.values;
    for (auto role : {ChartSeriesRole::Fitted, ChartSeriesRole::Forecast}) {
        series.role = role;
        model.series = {series};
        QVERIFY(!datalab::reporting::chart_primary_mirrors_series(model));
        const auto context = build_chart_plot_context(model, area);
        const auto hit = chart_interaction::hit_test(model, area, context.mapper.to_pixel(0.0, 1.0));
        QVERIFY(hit.has_value());
        QCOMPARE(hit->kind, chart_interaction::HitKind::Point);
        QCOMPARE(chart_interaction::resolve_selected_source_rows(model, {hit->index}),
                 (std::vector<std::size_t>{10}));
        const auto image = ChartRenderer::render_to_pixmap(model, area.size().toSize()).toImage();
        QVERIFY(has_magenta(image, context.mapper.to_pixel(0.5, 1.5).toPoint()));
    }
}

void ChartSamplingRenderRegressionTest::transparentSeriesCannotBeHitOrBrushed()
{
    auto model = plain_model(ChartKind::Scatter);
    model.value_style.visible = false;
    ChartSeries hidden;
    hidden.x_values = {5.0}; hidden.values = {5.0}; hidden.show_points = true;
    hidden.style.point_style = ChartPointStyle::Circle;
    hidden.style.opacity = 0.0;
    ChartSeries visible = hidden;
    visible.x_values = {0.0, 10.0}; visible.values = {0.0, 10.0};
    visible.style.opacity = 1.0; visible.style.line_width = 0.0;
    model.series = {hidden, visible};
    const auto context = build_chart_plot_context(model, area);
    const auto point = context.mapper.to_pixel(5.0, 5.0);
    QVERIFY(!chart_interaction::hit_test(model, area, point).has_value());
    QVERIFY(chart_interaction::select_rectangle(model, area,
        QRectF(point - QPointF(10, 10), QSizeF(20, 20))).empty());
}

void ChartSamplingRenderRegressionTest::partialOpacityChangesRenderedSeries()
{
    auto model = plain_model(ChartKind::Scatter);
    model.value_style.visible = false;
    ChartSeries series;
    series.x_values = {5.0}; series.values = {5.0}; series.show_points = true;
    series.style.color = QStringLiteral("#ff00ff");
    series.style.point_style = ChartPointStyle::Circle;
    series.style.line_width = 0.0; series.style.opacity = 0.5;
    model.series = {series};
    const auto context = build_chart_plot_context(model, area);
    const auto image = ChartRenderer::render_to_pixmap(model, area.size().toSize()).toImage();
    const auto color = image.pixelColor(context.mapper.to_pixel(5.0, 5.0).toPoint());
    QVERIFY(color.red() > 240 && color.blue() > 240);
    QVERIFY(color.green() > 80 && color.green() < 190);
}

void ChartSamplingRenderRegressionTest::transparentAggregateSeriesDoNotExpandRanges()
{
    ChartModel model;
    model.box_min = {0.0}; model.box_q1 = {2.0}; model.box_median = {5.0};
    model.box_q3 = {8.0}; model.box_max = {10.0};
    ChartSeries hidden;
    hidden.x_values = {0.0}; hidden.values = {1.0e6}; hidden.style.opacity = 0.0;
    model.series = {hidden};
    for (auto kind : {ChartKind::BoxPlot, ChartKind::Violin}) {
        model.kind = kind;
        const auto context = build_chart_plot_context(model, area);
        QVERIFY(context.valid);
        QVERIFY(context.mapper.y_max() < 20.0);
    }
}

void ChartSamplingRenderRegressionTest::marginalHistogramsShareZoomedAxes()
{
    auto model = plain_model(ChartKind::Marginal);
    model.x_values = {0.0, 5.0, 10.0}; model.values = {0.0, 5.0, 10.0};
    model.histogram_edges = {0.0, 2.0, 10.0}; model.histogram_counts = {1.0, 2.0};
    model.histogram_edges_y = {0.0, 8.0, 10.0}; model.histogram_counts_y = {2.0, 1.0};
    model.x_min = 1.0; model.x_max = 7.0; model.y_min = 2.0; model.y_max = 9.0;
    model.view.zoom_factor = 1.2; model.view.pan_offset = QPointF(5, -7);
    const auto layout = chart_marginal_plot_layout(area);
    const auto context = build_chart_plot_context(model, area);
    QCOMPARE(context.plot, layout.scatter);
    const auto image = ChartRenderer::render_to_pixmap(model, area.size().toSize()).toImage();
    const QPoint x_bin(qRound(context.mapper.to_pixel(2.2, 0.0).x()),
                       qRound(layout.x_histogram.top() + 5.0));
    const QPoint y_bin(qRound(layout.y_histogram.right() - 5.0),
                       qRound(context.mapper.to_pixel(0.0, 7.5).y()));
    QVERIFY(blue_fill(image.pixelColor(x_bin)));
    QVERIFY(blue_fill(image.pixelColor(y_bin)));
}

void ChartSamplingRenderRegressionTest::marginalHistogramsSupportTinyUnitsAndShortEdges()
{
    auto model = plain_model(ChartKind::Marginal);
    model.x_values = {1.0e-15, 3.0e-15, 5.0e-15};
    model.values = {1.0e-18, 3.0e-18, 5.0e-18};
    model.histogram_edges = {1.0e-15, 3.0e-15, 5.0e-15};
    model.histogram_counts = {1.0, 2.0, 1000.0};
    model.histogram_edges_y = {1.0e-18, 3.0e-18}; model.histogram_counts_y = {1.0};
    const auto layout = chart_marginal_plot_layout(area);
    const auto context = build_chart_plot_context(model, area);
    const auto image = ChartRenderer::render_to_pixmap(model, area.size().toSize()).toImage();
    const QPoint inside(qRound(context.mapper.to_pixel(4.0e-15, 0.0).x()),
                        qRound(layout.x_histogram.top() + 5.0));
    QVERIFY(blue_fill(image.pixelColor(inside)));
}

QTEST_MAIN(ChartSamplingRenderRegressionTest)
#include "chart_sampling_render_regression_test.moc"
