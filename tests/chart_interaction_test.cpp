#include "reporting/chart_interaction.h"
#include "application/graph_service.h"
#include "domain/quality_types.h"
#include "infrastructure/output_serialization.h"
#include "reporting/chart_adapter.h"

#include <QTest>

#include <algorithm>
#include <limits>

using datalab::application::GraphService;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class ChartInteractionTest final : public QObject {
    Q_OBJECT

private slots:
    void parallelTooltipIncludesObservationValues();
    void contourTooltipIncludesCellValue();
    void resolve_selected_expands_member_source_rows();
    void interval_and_bar_populate_member_source_rows();
    void pie_and_category_heatmap_member_source_rows();
    void hexbin_member_source_rows_match_cell_index();
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

void ChartInteractionTest::resolve_selected_expands_member_source_rows()
{
    ChartModel model;
    model.source_rows = {10, 20};
    model.member_source_rows = {{1, 2, 5}, {7, 8}};
    const auto rows = chart_interaction::resolve_selected_source_rows(model, {0});
    QCOMPARE(rows.size(), static_cast<std::size_t>(3));
    QVERIFY(std::find(rows.begin(), rows.end(), 1) != rows.end());
    QVERIFY(std::find(rows.begin(), rows.end(), 2) != rows.end());
    QVERIFY(std::find(rows.begin(), rows.end(), 5) != rows.end());

    // Fallback when member_source_rows is absent for the index.
    ChartModel fallback_model;
    fallback_model.source_rows = {10, 20};
    const auto fallback =
        chart_interaction::resolve_selected_source_rows(fallback_model, {1});
    QCOMPARE(fallback, (std::vector<std::size_t>{20}));

    // Empty member list must NOT fall back to source_rows[0].
    ChartModel empty_cell;
    empty_cell.source_rows = {0, 0};
    empty_cell.member_source_rows = {{}, {7, 8}};
    const auto empty_resolve =
        chart_interaction::resolve_selected_source_rows(empty_cell, {0});
    QVERIFY(empty_resolve.empty());
}

void ChartInteractionTest::interval_and_bar_populate_member_source_rows()
{
    DataTable table;
    table.columns = {"g", "y"};
    table.rows = {
        {"A", "1"}, {"A", "2"}, {"A", "3"},
        {"B", "4"}, {"B", "5"},
        {"A", "6"}};
    AnalysisConfiguration configuration;
    configuration.graph.graph_kind = "interval";
    configuration.graph.y_column = 1;
    configuration.graph.by_column = 0;
    configuration.hidden_rows = {5};  // hide last A
    configuration.excluded_rows = {4};  // exclude one B

    const auto interval = GraphService::interval(table, configuration);
    QVERIFY(!interval.plots.empty());
    QCOMPARE(interval.plots.front().member_source_rows.size(),
             interval.plots.front().categories.size());
    bool found_a = false;
    for (std::size_t i = 0; i < interval.plots.front().categories.size(); ++i) {
        if (interval.plots.front().categories[i] == "A") {
            found_a = true;
            // A rows 0,1,2 (5 hidden from display)
            QCOMPARE(interval.plots.front().member_source_rows[i].size(),
                     static_cast<std::size_t>(3));
        }
    }
    QVERIFY(found_a);

    const ChartModel model = chart_model_from_plot(interval.plots.front());
    QVERIFY(!model.member_source_rows.empty());
    const auto selected = chart_interaction::resolve_selected_source_rows(model, {0});
    QVERIFY(selected.size() >= 2);

    configuration.graph.graph_kind = "bar";
    configuration.graph.x_column = 0;
    configuration.graph.y_column.reset();
    configuration.graph.by_column.reset();
    configuration.hidden_rows = {5};
    configuration.excluded_rows = {4};
    const auto bar = GraphService::bar(table, configuration);
    QVERIFY(!bar.plots.empty());
    QVERIFY(!bar.plots.front().member_source_rows.empty());
    for (std::size_t i = 0; i < bar.plots.front().categories.size(); ++i) {
        if (bar.plots.front().categories[i] == "A") {
            QCOMPARE(bar.plots.front().member_source_rows[i].size(),
                     static_cast<std::size_t>(3));
        }
        if (bar.plots.front().categories[i] == "B") {
            // row 3 only (4 excluded, display)
            QCOMPARE(bar.plots.front().member_source_rows[i].size(),
                     static_cast<std::size_t>(1));
        }
    }

    const auto json = datalab::infrastructure::output_page_to_json(bar);
    const auto restored = datalab::infrastructure::output_page_from_json(json);
    QVERIFY(!restored.plots.empty());
    QCOMPARE(restored.plots.front().member_source_rows.size(),
             bar.plots.front().member_source_rows.size());
}

void ChartInteractionTest::pie_and_category_heatmap_member_source_rows()
{
    DataTable table;
    table.columns = {"cat", "row", "col", "z"};
    table.rows = {
        {"A", "R1", "C1", "1"},
        {"A", "R1", "C1", "3"},
        {"B", "R1", "C2", "5"},
        {"B", "R2", "C1", "7"},
        {"C", "R2", "C2", "9"}};

    AnalysisConfiguration pie_cfg;
    pie_cfg.graph.graph_kind = "pie";
    pie_cfg.graph.x_column = 0;
    pie_cfg.graph.other_threshold_percent = 0.0;
    const auto pie = GraphService::pie(table, pie_cfg);
    QVERIFY(!pie.plots.empty());
    QCOMPARE(pie.plots.front().member_source_rows.size(),
             pie.plots.front().categories.size());
    bool found_a = false;
    for (std::size_t i = 0; i < pie.plots.front().categories.size(); ++i) {
        if (pie.plots.front().categories[i] == "A") {
            found_a = true;
            QCOMPARE(pie.plots.front().member_source_rows[i].size(),
                     static_cast<std::size_t>(2));
        }
    }
    QVERIFY(found_a);

    // Force Other merge: tiny C slice merges when threshold is high.
    pie_cfg.graph.other_threshold_percent = 25.0;
    const auto pie_other = GraphService::pie(table, pie_cfg);
    QVERIFY(!pie_other.plots.empty());
    bool found_other = false;
    for (std::size_t i = 0; i < pie_other.plots.front().categories.size(); ++i) {
        if (pie_other.plots.front().categories[i] == "Other") {
            found_other = true;
            QVERIFY(!pie_other.plots.front().member_source_rows[i].empty());
            // Row 4 is category C (20% of 5 equal weights) → should be in Other.
            const auto& members = pie_other.plots.front().member_source_rows[i];
            QVERIFY(std::find(members.begin(), members.end(), 4) != members.end());
        }
    }
    QVERIFY(found_other);
    const ChartModel pie_model = chart_model_from_plot(pie.plots.front());
    const auto pie_rows =
        chart_interaction::resolve_selected_source_rows(pie_model, {0});
    QVERIFY(pie_rows.size() >= 1);

    AnalysisConfiguration heat_cfg;
    heat_cfg.graph.graph_kind = "heatmap";
    heat_cfg.graph.color_scale = "category";
    heat_cfg.graph.x_column = 2;
    heat_cfg.graph.y_column = 1;
    heat_cfg.graph.z_column = 3;
    const auto heat = GraphService::heatmap(table, heat_cfg);
    QVERIFY(!heat.plots.empty());
    QVERIFY(!heat.plots.front().member_source_rows.empty());
    // Cell R1/C1 should include rows 0 and 1.
    const std::size_t ncols = heat.plots.front().matrix_labels.size();
    QVERIFY(ncols >= 1);
    std::size_t r1 = 0;
    for (std::size_t i = 0; i < heat.plots.front().categories.size(); ++i) {
        if (heat.plots.front().categories[i] == "R1") {
            r1 = i;
        }
    }
    std::size_t c1 = 0;
    for (std::size_t i = 0; i < heat.plots.front().matrix_labels.size(); ++i) {
        if (heat.plots.front().matrix_labels[i] == "C1") {
            c1 = i;
        }
    }
    const std::size_t cell = r1 * ncols + c1;
    QVERIFY(cell < heat.plots.front().member_source_rows.size());
    QCOMPARE(heat.plots.front().member_source_rows[cell].size(),
             static_cast<std::size_t>(2));
    const ChartModel heat_model = chart_model_from_plot(heat.plots.front());
    const auto cell_rows =
        chart_interaction::resolve_selected_source_rows(heat_model, {cell});
    QCOMPARE(cell_rows.size(), static_cast<std::size_t>(2));

    // Leave R2×C2 empty so GraphService flatten + resolve stay honest.
    DataTable sparse;
    sparse.columns = {"cat", "row", "col", "z"};
    sparse.rows = {
        {"A", "R1", "C1", "1"},
        {"B", "R1", "C2", "5"},
        {"B", "R2", "C1", "7"}};
    const auto sparse_heat = GraphService::heatmap(sparse, heat_cfg);
    QVERIFY(!sparse_heat.plots.empty());
    const std::size_t sparse_ncols = sparse_heat.plots.front().matrix_labels.size();
    std::size_t sparse_r2 = 0;
    std::size_t sparse_c2 = 0;
    for (std::size_t i = 0; i < sparse_heat.plots.front().categories.size(); ++i) {
        if (sparse_heat.plots.front().categories[i] == "R2") {
            sparse_r2 = i;
        }
    }
    for (std::size_t i = 0; i < sparse_heat.plots.front().matrix_labels.size(); ++i) {
        if (sparse_heat.plots.front().matrix_labels[i] == "C2") {
            sparse_c2 = i;
        }
    }
    const std::size_t empty_cell = sparse_r2 * sparse_ncols + sparse_c2;
    QVERIFY(empty_cell < sparse_heat.plots.front().member_source_rows.size());
    QVERIFY(sparse_heat.plots.front().member_source_rows[empty_cell].empty());
    QCOMPARE(sparse_heat.plots.front().source_rows[empty_cell],
             std::numeric_limits<std::size_t>::max());
    const ChartModel sparse_model = chart_model_from_plot(sparse_heat.plots.front());
    const auto empty_rows =
        chart_interaction::resolve_selected_source_rows(sparse_model, {empty_cell});
    QVERIFY(empty_rows.empty());
}

void ChartInteractionTest::hexbin_member_source_rows_match_cell_index()
{
    DataTable table;
    table.columns = {"x", "y"};
    // Force a 2×2 grid: bin_count=2; points in three corners leave one empty.
    table.rows = {
        {"0", "0"},
        {"0", "0"},
        {"1", "0"},
        {"0", "1"}};
    AnalysisConfiguration cfg;
    cfg.graph.graph_kind = "hexbin";
    cfg.graph.x_column = 0;
    cfg.graph.y_column = 1;
    cfg.graph.bin_count = 2;
    const auto page = GraphService::hexbin(table, cfg);
    QVERIFY(!page.plots.empty());
    QCOMPARE(page.plots.front().contour_x.size(), static_cast<std::size_t>(3));  // edges
    QCOMPARE(page.plots.front().contour_y.size(), static_cast<std::size_t>(3));
    const std::size_t ncols = page.plots.front().contour_x.size() - 1;
    QCOMPARE(page.plots.front().member_source_rows.size(), ncols * ncols);
    // Bin (0,0) should contain the two origin points (rows 0 and 1).
    QCOMPARE(page.plots.front().member_source_rows[0].size(), static_cast<std::size_t>(2));
    // With points at (0,0)×2, (1,0), (0,1) on a 2×2 grid, cell (1,1) is empty.
    const std::size_t empty_cell = 1 * ncols + 1;
    QVERIFY(empty_cell < page.plots.front().member_source_rows.size());
    QVERIFY(page.plots.front().member_source_rows[empty_cell].empty());
    QCOMPARE(page.plots.front().source_rows[empty_cell],
             std::numeric_limits<std::size_t>::max());
    const ChartModel model = chart_model_from_plot(page.plots.front());
    const auto filled =
        chart_interaction::resolve_selected_source_rows(model, {0});
    QCOMPARE(filled.size(), static_cast<std::size_t>(2));
    const auto empty_resolve =
        chart_interaction::resolve_selected_source_rows(model, {empty_cell});
    QVERIFY(empty_resolve.empty());
}

QTEST_MAIN(ChartInteractionTest)
#include "chart_interaction_test.moc"
