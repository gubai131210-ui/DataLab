#include "application/graph_service.h"
#include "domain/graph_assembly.h"
#include "domain/quality_types.h"
#include "domain/row_visibility.h"
#include "infrastructure/output_serialization.h"

#include <QtTest/QtTest>

#include <algorithm>

using datalab::application::GraphService;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::summarize_row_visibility;

class GraphVisibilityPhase7Test final : public QObject {
    Q_OBJECT

private slots:
    void hidden_and_excluded_are_distinct()
    {
        const auto summary = summarize_row_visibility(5, {1, 2}, {2, 3});
        QCOMPARE(static_cast<int>(summary.excluded_count), 2);   // 2,3
        QCOMPARE(static_cast<int>(summary.hidden_count), 1);     // 1 only (2 also excluded)
        QCOMPARE(static_cast<int>(summary.analysis_eligible_count), 3);  // 0,1,4
        QCOMPARE(static_cast<int>(summary.display_eligible_count), 2);   // 0,4
        QVERIFY(std::find(summary.analysis_rows.begin(), summary.analysis_rows.end(), 1)
                != summary.analysis_rows.end());
        QVERIFY(std::find(summary.display_rows.begin(), summary.display_rows.end(), 1)
                == summary.display_rows.end());
        QVERIFY(std::find(summary.analysis_rows.begin(), summary.analysis_rows.end(), 3)
                == summary.analysis_rows.end());
    }

    void scatter_and_bar_respect_visibility_contract()
    {
        DataTable table;
        table.columns = {"x", "y", "g"};
        table.rows = {
            {"1", "10", "A"},
            {"2", "20", "A"},
            {"3", "30", "B"},
            {"4", "40", "B"},
            {"5", "50", "B"}};

        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "scatter";
        configuration.graph.x_column = 0;
        configuration.graph.y_column = 1;
        configuration.graph.by_column = 2;
        configuration.hidden_rows = {1};
        configuration.excluded_rows = {3};

        const auto scatter = GraphService::scatter(table, configuration);
        QVERIFY(scatter.facts.eda.has_value());
        QCOMPARE(scatter.facts.eda->hidden_count, std::size_t{1});
        QCOMPARE(scatter.facts.eda->excluded_count, std::size_t{1});
        QCOMPARE(scatter.facts.eda->hidden_excluded_distinct, true);
        QCOMPARE(scatter.facts.eda->analysis_eligible_n, std::size_t{4});
        // Plotted points exclude excluded+hidden → rows 0,2,4
        QCOMPARE(scatter.facts.eda->n, std::size_t{3});
        QVERIFY(!scatter.plots.empty());
        QCOMPARE(scatter.plots.front().source_rows.size(), std::size_t{3});

        configuration.graph.graph_kind = "bar";
        configuration.graph.x_column = 2;
        configuration.graph.y_column.reset();
        const auto bar = GraphService::bar(table, configuration);
        QVERIFY(bar.facts.eda.has_value());
        QCOMPARE(bar.facts.eda->kind, std::string("bar"));
        QCOMPARE(bar.facts.eda->hidden_count, std::size_t{1});
        QCOMPARE(bar.facts.eda->excluded_count, std::size_t{1});
        QCOMPARE(bar.facts.eda->hidden_excluded_distinct, true);
        // Display n omits hidden+excluded; analysis_eligible keeps hidden.
        QCOMPARE(bar.facts.eda->analysis_eligible_n, std::size_t{4});
        QCOMPARE(bar.facts.eda->display_eligible_n, std::size_t{3});
        QCOMPARE(bar.facts.eda->n, std::size_t{3});
        QCOMPARE(bar.facts.eda->analysis_n, std::size_t{4});  // includes hidden
        QCOMPARE(bar.facts.eda->analysis_category_count, std::size_t{2});

        configuration.graph.graph_kind = "density";
        configuration.graph.x_column = 0;
        configuration.graph.y_column.reset();
        configuration.graph.by_column.reset();
        const auto density = GraphService::density(table, configuration);
        QVERIFY(density.facts.eda.has_value());
        QCOMPARE(density.facts.eda->kind, std::string("density"));
        QCOMPARE(density.facts.eda->hidden_excluded_distinct, true);
        QCOMPARE(density.facts.eda->analysis_eligible_n, std::size_t{4});
        QCOMPARE(density.facts.eda->n, std::size_t{3});  // display omits hidden row 1
        QCOMPARE(density.facts.eda->display_eligible_n, std::size_t{3});
        QCOMPARE(density.facts.eda->analysis_n, std::size_t{4});
        QVERIFY(!density.plots.empty());
        QVERIFY(density.plots.front().source_rows.empty());
        QVERIFY(density.plots.front().member_source_rows.empty());
        bool density_mark_note = false;
        for (const auto& diagnostic : density.diagnostics) {
            if (diagnostic.code == "density_curve_not_discrete_marks") {
                density_mark_note = true;
            }
        }
        QVERIFY(density_mark_note);

        configuration.graph.graph_kind = "pie";
        configuration.graph.x_column = 2;
        const auto pie = GraphService::pie(table, configuration);
        QVERIFY(pie.facts.eda.has_value());
        QCOMPARE(pie.facts.eda->hidden_excluded_distinct, true);
        QCOMPARE(pie.facts.eda->analysis_n, std::size_t{4});
        QCOMPARE(pie.facts.eda->n, std::size_t{3});

        configuration.graph.graph_kind = "hexbin";
        configuration.graph.x_column = 0;
        configuration.graph.y_column = 1;
        const auto hexbin = GraphService::hexbin(table, configuration);
        QVERIFY(hexbin.facts.eda.has_value());
        QCOMPARE(hexbin.facts.eda->analysis_n, std::size_t{4});
        QCOMPARE(hexbin.facts.eda->n, std::size_t{3});

        configuration.graph.graph_kind = "matrix";
        configuration.graph.variable_columns = {0, 1};
        configuration.graph.x_column.reset();
        configuration.graph.y_column.reset();
        const auto matrix = GraphService::matrix(table, configuration);
        QVERIFY(matrix.facts.eda.has_value());
        QCOMPARE(matrix.facts.eda->hidden_excluded_distinct, true);
        QCOMPARE(matrix.facts.eda->analysis_n, std::size_t{4});
        QCOMPARE(matrix.facts.eda->n, std::size_t{3});

        configuration.graph.graph_kind = "correlation";
        configuration.graph.variable_columns = {0, 1};
        const auto correlation = GraphService::correlation(table, configuration);
        QVERIFY(correlation.facts.eda.has_value());
        QCOMPARE(correlation.facts.eda->analysis_n, std::size_t{4});
        QCOMPARE(correlation.facts.eda->n, std::size_t{3});

        configuration.graph.graph_kind = "time_series";
        configuration.graph.variable_columns.clear();
        configuration.graph.x_column = 0;
        configuration.graph.y_column = 1;
        const auto series = GraphService::time_series(table, configuration);
        QVERIFY(series.facts.eda.has_value());
        QCOMPARE(series.facts.eda->analysis_n, std::size_t{4});
        QCOMPARE(series.facts.eda->n, std::size_t{3});

        configuration.graph.graph_kind = "area";
        const auto area = GraphService::area(table, configuration);
        QVERIFY(area.facts.eda.has_value());
        QCOMPARE(area.facts.eda->kind, std::string("area"));
        QCOMPARE(area.facts.eda->analysis_n, std::size_t{4});
        QCOMPARE(area.facts.eda->n, std::size_t{3});

        configuration.graph.graph_kind = "heatmap";
        configuration.graph.variable_columns = {0, 1};
        configuration.graph.x_column.reset();
        configuration.graph.y_column.reset();
        configuration.graph.color_scale.clear();
        const auto heatmap_corr = GraphService::heatmap(table, configuration);
        QVERIFY(heatmap_corr.facts.eda.has_value());
        QCOMPARE(heatmap_corr.facts.eda->kind, std::string("heatmap"));
        QCOMPARE(heatmap_corr.facts.eda->analysis_n, std::size_t{4});
        QCOMPARE(heatmap_corr.facts.eda->n, std::size_t{3});

        DataTable cat_table;
        cat_table.columns = {"col", "row", "z"};
        cat_table.rows = {
            {"C1", "R1", "1"},
            {"C2", "R1", "2"},
            {"C1", "R2", "3"},
            {"C2", "R2", "4"},
            {"C1", "R1", "5"}};
        AnalysisConfiguration cat_config;
        cat_config.graph.graph_kind = "heatmap";
        cat_config.graph.x_column = 0;
        cat_config.graph.y_column = 1;
        cat_config.graph.z_column = 2;
        cat_config.graph.color_scale = "category";
        cat_config.hidden_rows = {1};
        cat_config.excluded_rows = {3};
        const auto heatmap_cat = GraphService::heatmap(cat_table, cat_config);
        QVERIFY(heatmap_cat.facts.eda.has_value());
        QCOMPARE(heatmap_cat.facts.eda->analysis_n, std::size_t{4});
        QCOMPARE(heatmap_cat.facts.eda->n, std::size_t{3});

        const auto json = datalab::infrastructure::output_page_to_json(scatter);
        QVERIFY(json.value(QStringLiteral("configuration")).toObject()
                    .value(QStringLiteral("hidden_rows")).isArray());
        QVERIFY(json.value(QStringLiteral("configuration")).toObject()
                    .value(QStringLiteral("excluded_rows")).isArray());
        const auto restored = datalab::infrastructure::output_page_from_json(json);
        QCOMPARE(restored.configuration.hidden_rows, configuration.hidden_rows);
        QCOMPARE(restored.configuration.excluded_rows, configuration.excluded_rows);
        QVERIFY(restored.facts.eda.has_value());
        QCOMPARE(restored.facts.eda->hidden_excluded_distinct, true);
        QCOMPARE(restored.facts.eda->analysis_n, scatter.facts.eda->analysis_n);
    }

    void scatter_facet_creates_controlled_panels_and_truncates()
    {
        DataTable table;
        table.columns = {"x", "y", "facet"};
        table.rows = {
            {"1", "1", "A"},
            {"2", "2", "A"},
            {"3", "3", "B"},
            {"4", "4", "C"},
            {"5", "5", "D"},
            {"6", "6", "E"},
            {"7", "7", "F"},
            {"8", "8", "G"}};
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "scatter";
        configuration.graph.x_column = 0;
        configuration.graph.y_column = 1;
        configuration.graph.facet_column = 2;
        configuration.graph.facet_max_panels = 3;
        configuration.hidden_rows = {1};  // hide one A point
        configuration.excluded_rows = {7};  // exclude G

        const auto page = GraphService::scatter(table, configuration);
        QCOMPARE(page.plots.size(), std::size_t{3});
        QVERIFY(page.facts.eda.has_value());
        QCOMPARE(page.facts.eda->facet_enabled, true);
        QCOMPARE(page.facts.eda->facet_panel_count, std::size_t{3});
        QCOMPARE(page.facts.eda->facet_level_count, std::size_t{6});  // A..F (G excluded)
        QCOMPARE(page.facts.eda->facet_truncated_levels, std::size_t{3});
        QCOMPARE(page.facts.eda->facet_max_panels, 3);
        // Panel A display omits hidden row 1 → 1 point.
        QCOMPARE(page.plots.front().source_rows.size(), std::size_t{1});
        QCOMPARE(page.plots.front().source_rows.front(), std::size_t{0});

        bool saw_truncated = false;
        bool saw_controlled = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "facet_levels_truncated") {
                saw_truncated = true;
            }
            if (diagnostic.code == "facet_controlled_panels") {
                saw_controlled = true;
            }
        }
        QVERIFY(saw_truncated);
        QVERIFY(saw_controlled);

        const auto json = datalab::infrastructure::output_page_to_json(page);
        QCOMPARE(json.value(QStringLiteral("configuration")).toObject()
                     .value(QStringLiteral("graph_facet_column")).toInt(),
                 2);
        QCOMPARE(json.value(QStringLiteral("configuration")).toObject()
                     .value(QStringLiteral("graph_facet_max_panels")).toInt(),
                 3);
        const auto restored = datalab::infrastructure::output_page_from_json(json);
        QCOMPARE(restored.configuration.graph.facet_column, configuration.graph.facet_column);
        QCOMPARE(restored.configuration.graph.facet_max_panels, 3);
        QVERIFY(restored.facts.eda.has_value());
        QCOMPARE(restored.facts.eda->facet_truncated_levels,
                 page.facts.eda->facet_truncated_levels);
        QCOMPARE(restored.plots.size(), page.plots.size());
    }

    void facet_partition_stable_order_domain()
    {
        const auto result = datalab::domain::partition_facet_levels(
            {"B", "A", "B", "C"}, 2);
        QCOMPARE(result.level_count, std::size_t{3});
        QCOMPARE(result.panels.size(), std::size_t{2});
        QCOMPARE(result.panels[0].level, std::string("B"));
        QCOMPARE(result.panels[1].level, std::string("A"));
        QCOMPARE(result.truncated_levels, std::size_t{1});
        QCOMPARE(result.panels[0].member_indices.size(), std::size_t{2});
    }

    void bar_and_density_facet_controlled_panels()
    {
        DataTable table;
        table.columns = {"x", "cat", "facet"};
        table.rows = {
            {"1.0", "A", "P1"},
            {"2.0", "B", "P1"},
            {"3.0", "A", "P2"},
            {"4.0", "B", "P2"},
            {"5.0", "A", "P3"},
            {"6.0", "B", "P3"},
            {"7.0", "A", "P4"},
        };
        AnalysisConfiguration configuration;
        configuration.excluded_rows = {6};  // drop P4 from analysis
        configuration.hidden_rows = {1};    // hide one P1 display row
        configuration.graph.facet_column = 2;
        configuration.graph.facet_max_panels = 2;

        configuration.graph.graph_kind = "bar";
        configuration.graph.x_column = 1;
        const auto bar = GraphService::bar(table, configuration);
        QVERIFY(bar.facts.eda.has_value());
        QCOMPARE(bar.facts.eda->facet_enabled, true);
        QCOMPARE(bar.plots.size(), std::size_t{2});
        QCOMPARE(bar.facts.eda->facet_level_count, std::size_t{3});  // P1..P3
        QCOMPARE(bar.facts.eda->facet_truncated_levels, std::size_t{1});
        QVERIFY(!bar.plots.front().member_source_rows.empty());
        bool bar_trunc = false;
        bool bar_controlled = false;
        for (const auto& diagnostic : bar.diagnostics) {
            if (diagnostic.code == "facet_levels_truncated") {
                bar_trunc = true;
            }
            if (diagnostic.code == "facet_controlled_panels") {
                bar_controlled = true;
            }
        }
        QVERIFY(bar_trunc);
        QVERIFY(bar_controlled);
        // P1 panel: hidden row 1 must not appear in display member linkage.
        bool hidden_leaked = false;
        for (const auto& members : bar.plots.front().member_source_rows) {
            if (std::find(members.begin(), members.end(), std::size_t{1}) != members.end()) {
                hidden_leaked = true;
            }
        }
        QVERIFY(!hidden_leaked);

        configuration.graph.graph_kind = "density";
        configuration.graph.x_column = 0;
        configuration.graph.y_column.reset();
        const auto density = GraphService::density(table, configuration);
        QVERIFY(density.facts.eda.has_value());
        QCOMPARE(density.facts.eda->facet_enabled, true);
        QCOMPARE(density.plots.size(), std::size_t{2});
        for (const auto& plot : density.plots) {
            QVERIFY(plot.source_rows.empty());
            QVERIFY(plot.member_source_rows.empty());
        }
        bool density_mark_note = false;
        for (const auto& diagnostic : density.diagnostics) {
            if (diagnostic.code == "density_curve_not_discrete_marks") {
                density_mark_note = true;
            }
        }
        QVERIFY(density_mark_note);
    }

    void interval_violin_hexbin_facet_controlled_panels()
    {
        DataTable table;
        table.columns = {"y", "cat", "facet", "x"};
        table.rows = {
            {"10", "A", "P1", "1"},
            {"12", "B", "P1", "2"},
            {"14", "A", "P2", "3"},
            {"16", "B", "P2", "4"},
            {"18", "A", "P3", "5"},
            {"20", "B", "P3", "6"},
            {"22", "A", "P4", "7"},
        };
        AnalysisConfiguration configuration;
        configuration.excluded_rows = {6};
        configuration.hidden_rows = {1};
        configuration.graph.facet_column = 2;
        configuration.graph.facet_max_panels = 2;

        configuration.graph.graph_kind = "interval";
        configuration.graph.y_column = 0;
        configuration.graph.by_column = 1;
        const auto interval = GraphService::interval(table, configuration);
        QVERIFY(interval.facts.eda.has_value());
        QCOMPARE(interval.facts.eda->facet_enabled, true);
        QCOMPARE(interval.plots.size(), std::size_t{2});
        QCOMPARE(interval.facts.eda->facet_level_count, std::size_t{3});
        QCOMPARE(interval.facts.eda->facet_truncated_levels, std::size_t{1});
        bool hidden_in_interval = false;
        for (const auto& members : interval.plots.front().member_source_rows) {
            if (std::find(members.begin(), members.end(), std::size_t{1}) != members.end()) {
                hidden_in_interval = true;
            }
        }
        QVERIFY(!hidden_in_interval);

        configuration.graph.graph_kind = "violin";
        configuration.graph.by_column = 1;
        const auto violin = GraphService::violin(table, configuration);
        QVERIFY(violin.facts.eda.has_value());
        QCOMPARE(violin.facts.eda->facet_enabled, true);
        QCOMPARE(violin.plots.size(), std::size_t{2});
        bool hidden_in_violin = false;
        for (const auto& members : violin.plots.front().member_source_rows) {
            if (std::find(members.begin(), members.end(), std::size_t{1}) != members.end()) {
                hidden_in_violin = true;
            }
        }
        QVERIFY(!hidden_in_violin);

        configuration.graph.graph_kind = "hexbin";
        configuration.graph.x_column = 3;
        configuration.graph.y_column = 0;
        configuration.graph.by_column.reset();
        const auto hexbin = GraphService::hexbin(table, configuration);
        QVERIFY(hexbin.facts.eda.has_value());
        QCOMPARE(hexbin.facts.eda->facet_enabled, true);
        QCOMPARE(hexbin.plots.size(), std::size_t{2});
        bool hidden_in_hex = false;
        for (const auto& members : hexbin.plots.front().member_source_rows) {
            if (std::find(members.begin(), members.end(), std::size_t{1}) != members.end()) {
                hidden_in_hex = true;
            }
        }
        QVERIFY(!hidden_in_hex);
    }

    void contour_facet_controlled_panels()
    {
        DataTable table;
        table.columns = {"x", "y", "z", "facet"};
        table.rows = {
            {"0", "0", "1", "P1"},
            {"1", "0", "2", "P1"},
            {"0", "1", "3", "P1"},
            {"1", "1", "4", "P1"},
            {"0", "0", "2", "P2"},
            {"1", "0", "3", "P2"},
            {"0", "1", "4", "P2"},
            {"1", "1", "5", "P2"},
            {"0", "0", "3", "P3"},
            {"1", "0", "4", "P3"},
            {"0", "1", "5", "P3"},
            {"1", "1", "6", "P3"},
            {"0", "0", "9", "P4"},
        };
        AnalysisConfiguration configuration;
        configuration.excluded_rows = {12};
        configuration.hidden_rows = {1};
        configuration.graph.graph_kind = "contour";
        configuration.graph.x_column = 0;
        configuration.graph.y_column = 1;
        configuration.graph.z_column = 2;
        configuration.graph.facet_column = 3;
        configuration.graph.facet_max_panels = 2;
        configuration.graph.contour_levels = 4;
        const auto page = GraphService::contour(table, configuration);
        QVERIFY(page.facts.eda.has_value());
        QCOMPARE(page.facts.eda->facet_enabled, true);
        QCOMPARE(page.plots.size(), std::size_t{2});
        QCOMPARE(page.facts.eda->facet_level_count, std::size_t{3});
        QCOMPARE(page.facts.eda->facet_truncated_levels, std::size_t{1});
        bool trunc = false;
        bool controlled = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "facet_levels_truncated") {
                trunc = true;
            }
            if (diagnostic.code == "facet_controlled_panels") {
                controlled = true;
            }
        }
        QVERIFY(trunc);
        QVERIFY(controlled);
        QCOMPARE(page.facts.eda->hidden_excluded_distinct, true);
        QCOMPARE(page.facts.eda->hidden_count, std::size_t{1});
        QCOMPARE(page.facts.eda->excluded_count, std::size_t{1});
    }

    void matrix_parallel_area_facet_controlled_panels()
    {
        DataTable table;
        table.columns = {"a", "b", "t", "y", "facet"};
        table.rows = {
            {"1", "10", "1", "5", "P1"},
            {"2", "20", "2", "6", "P1"},
            {"3", "30", "3", "7", "P2"},
            {"4", "40", "4", "8", "P2"},
            {"5", "50", "5", "9", "P3"},
            {"6", "60", "6", "10", "P3"},
            {"7", "70", "7", "11", "P4"},
        };
        AnalysisConfiguration configuration;
        configuration.excluded_rows = {6};
        configuration.hidden_rows = {1};
        configuration.graph.facet_column = 4;
        configuration.graph.facet_max_panels = 2;
        configuration.graph.variable_columns = {0, 1};

        configuration.graph.graph_kind = "matrix";
        const auto matrix = GraphService::matrix(table, configuration);
        QVERIFY(matrix.facts.eda.has_value());
        QCOMPARE(matrix.facts.eda->facet_enabled, true);
        QCOMPARE(matrix.plots.size(), std::size_t{2});
        QCOMPARE(matrix.facts.eda->facet_truncated_levels, std::size_t{1});
        bool hidden_in_matrix = false;
        for (const auto row : matrix.plots.front().source_rows) {
            if (row == 1) {
                hidden_in_matrix = true;
            }
        }
        QVERIFY(!hidden_in_matrix);

        configuration.graph.graph_kind = "parallel";
        const auto parallel = GraphService::parallel(table, configuration);
        QVERIFY(parallel.facts.eda.has_value());
        QCOMPARE(parallel.facts.eda->facet_enabled, true);
        QCOMPARE(parallel.plots.size(), std::size_t{2});
        bool hidden_in_parallel = false;
        for (const auto row : parallel.plots.front().source_rows) {
            if (row == 1) {
                hidden_in_parallel = true;
            }
        }
        QVERIFY(!hidden_in_parallel);

        configuration.graph.graph_kind = "area";
        configuration.graph.variable_columns.clear();
        configuration.graph.time_column = 2;
        configuration.graph.x_column = 2;
        configuration.graph.y_column = 3;
        const auto area = GraphService::area(table, configuration);
        QVERIFY(area.facts.eda.has_value());
        QCOMPARE(area.facts.eda->kind, std::string("area"));
        QCOMPARE(area.facts.eda->facet_enabled, true);
        QCOMPARE(area.plots.size(), std::size_t{2});
        QCOMPARE(area.plots.front().kind, datalab::domain::PlotKind::area);
        bool hidden_in_area = false;
        for (const auto row : area.plots.front().source_rows) {
            if (row == 1) {
                hidden_in_area = true;
            }
        }
        QVERIFY(!hidden_in_area);
    }

    void bubble_probability_ecdf_marginal_facet_controlled_panels()
    {
        DataTable table;
        table.columns = {"x", "y", "size", "facet"};
        table.rows = {
            {"1", "10", "1", "P1"},
            {"2", "20", "2", "P1"},
            {"3", "30", "3", "P2"},
            {"4", "40", "4", "P2"},
            {"5", "50", "5", "P3"},
            {"6", "60", "6", "P3"},
            {"7", "70", "7", "P4"},
        };
        AnalysisConfiguration configuration;
        configuration.excluded_rows = {6};
        configuration.hidden_rows = {1};
        configuration.graph.facet_column = 3;
        configuration.graph.facet_max_panels = 2;

        configuration.graph.graph_kind = "bubble";
        configuration.graph.x_column = 0;
        configuration.graph.y_column = 1;
        configuration.graph.size_column = 2;
        const auto bubble = GraphService::bubble(table, configuration);
        QVERIFY(bubble.facts.eda.has_value());
        QCOMPARE(bubble.facts.eda->facet_enabled, true);
        QCOMPARE(bubble.plots.size(), std::size_t{2});
        bool hidden_in_bubble = false;
        for (const auto row : bubble.plots.front().source_rows) {
            if (row == 1) {
                hidden_in_bubble = true;
            }
        }
        QVERIFY(!hidden_in_bubble);

        configuration.graph.graph_kind = "probability";
        configuration.graph.x_column.reset();
        configuration.graph.size_column.reset();
        configuration.graph.y_column = 1;
        const auto probability = GraphService::probability(table, configuration);
        QVERIFY(probability.facts.eda.has_value());
        QCOMPARE(probability.facts.eda->facet_enabled, true);
        QCOMPARE(probability.plots.size(), std::size_t{2});

        configuration.graph.graph_kind = "ecdf";
        const auto ecdf = GraphService::ecdf(table, configuration);
        QVERIFY(ecdf.facts.eda.has_value());
        QCOMPARE(ecdf.facts.eda->facet_enabled, true);
        QCOMPARE(ecdf.plots.size(), std::size_t{2});

        configuration.graph.graph_kind = "marginal";
        configuration.graph.x_column = 0;
        configuration.graph.y_column = 1;
        const auto marginal = GraphService::marginal(table, configuration);
        QVERIFY(marginal.facts.eda.has_value());
        QCOMPARE(marginal.facts.eda->facet_enabled, true);
        QCOMPARE(marginal.plots.size(), std::size_t{2});
        bool hidden_in_marginal = false;
        for (const auto row : marginal.plots.front().source_rows) {
            if (row == 1) {
                hidden_in_marginal = true;
            }
        }
        QVERIFY(!hidden_in_marginal);
    }

    void correlation_and_heatmap_facet_controlled_panels()
    {
        DataTable table;
        table.columns = {"a", "b", "row", "col", "z", "facet"};
        table.rows = {
            {"1", "10", "R1", "C1", "1", "P1"},
            {"2", "20", "R1", "C2", "2", "P1"},
            {"3", "30", "R2", "C1", "3", "P1"},
            {"4", "40", "R2", "C2", "4", "P1"},
            {"5", "50", "R1", "C1", "2", "P2"},
            {"6", "60", "R1", "C2", "3", "P2"},
            {"7", "70", "R2", "C1", "4", "P2"},
            {"8", "80", "R2", "C2", "5", "P2"},
            {"9", "90", "R1", "C1", "3", "P3"},
            {"10", "100", "R1", "C2", "4", "P3"},
            {"11", "110", "R2", "C1", "5", "P3"},
            {"12", "120", "R2", "C2", "6", "P3"},
            {"13", "130", "R1", "C1", "9", "P4"},
        };
        AnalysisConfiguration configuration;
        configuration.excluded_rows = {12};
        configuration.hidden_rows = {1};
        configuration.graph.facet_column = 5;
        configuration.graph.facet_max_panels = 2;
        configuration.graph.variable_columns = {0, 1};
        configuration.graph.correlation_method = "pearson";

        configuration.graph.graph_kind = "correlation";
        const auto correlation = GraphService::correlation(table, configuration);
        QVERIFY(correlation.facts.eda.has_value());
        QCOMPARE(correlation.facts.eda->facet_enabled, true);
        QCOMPARE(correlation.plots.size(), std::size_t{2});
        QCOMPARE(correlation.facts.eda->facet_truncated_levels, std::size_t{1});
        // Correlation cells must not invent member_source_rows.
        QVERIFY(correlation.plots.front().member_source_rows.empty());

        configuration.graph.graph_kind = "heatmap";
        const auto corr_heat = GraphService::heatmap(table, configuration);
        QVERIFY(corr_heat.facts.eda.has_value());
        QCOMPARE(corr_heat.facts.eda->facet_enabled, true);
        QCOMPARE(corr_heat.plots.size(), std::size_t{2});
        QVERIFY(corr_heat.plots.front().member_source_rows.empty());

        configuration.graph.variable_columns.clear();
        configuration.graph.x_column = 3;
        configuration.graph.y_column = 2;
        configuration.graph.z_column = 4;
        const auto cat_heat = GraphService::heatmap(table, configuration);
        QVERIFY(cat_heat.facts.eda.has_value());
        QCOMPARE(cat_heat.facts.eda->facet_enabled, true);
        QCOMPARE(cat_heat.plots.size(), std::size_t{2});
        QVERIFY(!cat_heat.plots.front().member_source_rows.empty());
        bool hidden_leaked = false;
        for (const auto& members : cat_heat.plots.front().member_source_rows) {
            if (std::find(members.begin(), members.end(), std::size_t{1}) != members.end()) {
                hidden_leaked = true;
            }
        }
        QVERIFY(!hidden_leaked);
    }
};

QTEST_MAIN(GraphVisibilityPhase7Test)
#include "graph_visibility_phase7_test.moc"
