#include "application/graph_service.h"
#include "application/interpretation_service.h"
#include "domain/quality_types.h"
#include "domain/statistics/eda_plots.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::application::GraphService;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

// # source: formula_reference — docs/research/p2_eda_density_hexbin_violin_bar.md

class P2EdaPlotsTest final : public QObject {
    Q_OBJECT

private slots:
    void kdeIntegratesRoughlyToOne();
    void hexbinCountsSumToN();
    void barKeepsOrderWithoutCumulative();
    void violinHasDensityAndBox();
    void servicePagesAndRoundTrip();
};

void P2EdaPlotsTest::kdeIntegratesRoughlyToOne()
{
    std::vector<double> values;
    for (int i = 0; i < 40; ++i) {
        values.push_back(static_cast<double>(i) / 10.0);
    }
    const auto kde = datalab::domain::statistics::gaussian_kde(values, 128);
    QVERIFY(kde.bandwidth > 0.0);
    QCOMPARE(kde.x.size(), kde.density.size());
    double area = 0.0;
    for (std::size_t i = 1; i < kde.x.size(); ++i) {
        area += 0.5 * (kde.density[i - 1] + kde.density[i]) * (kde.x[i] - kde.x[i - 1]);
    }
    QVERIFY(std::abs(area - 1.0) < 0.08);
}

void P2EdaPlotsTest::hexbinCountsSumToN()
{
    std::vector<double> x = {0, 0, 1, 1, 0.5};
    std::vector<double> y = {0, 1, 0, 1, 0.5};
    std::vector<std::size_t> rows = {0, 1, 2, 3, 4};
    const auto bins = datalab::domain::statistics::hexbin_rectangular(x, y, rows, 4);
    double sum = 0.0;
    for (const auto& row : bins.counts) {
        for (const double count : row) {
            sum += count;
        }
    }
    QCOMPARE(sum, 5.0);
}

void P2EdaPlotsTest::barKeepsOrderWithoutCumulative()
{
    const auto bars = datalab::domain::statistics::bar_chart_counts({"B", "A", "B", "C"});
    QCOMPARE(bars.categories, (std::vector<std::string>{"B", "A", "C"}));
    QCOMPARE(bars.values[0], 2.0);
    QVERIFY(!bars.categories.empty());
}

void P2EdaPlotsTest::violinHasDensityAndBox()
{
    std::vector<double> values = {1, 2, 2, 3, 4, 5, 5, 6, 8, 9};
    std::vector<std::string> groups(values.size(), "G");
    std::vector<std::size_t> rows(values.size());
    for (std::size_t i = 0; i < rows.size(); ++i) {
        rows[i] = i;
    }
    const auto violin = datalab::domain::statistics::violin_plot(values, groups, rows);
    QCOMPARE(violin.groups.size(), std::size_t{1});
    QVERIFY(violin.groups.front().density_values.size() >= 2);
    QVERIFY(violin.groups.front().q3 >= violin.groups.front().q1);
}

void P2EdaPlotsTest::servicePagesAndRoundTrip()
{
    DataTable table;
    table.columns = {"X", "Y", "G", "Cat"};
    for (int i = 0; i < 20; ++i) {
        table.rows.push_back({
            std::to_string(i * 0.1),
            std::to_string(std::sin(0.3 * i)),
            i < 10 ? "A" : "B",
            i % 2 == 0 ? "Red" : "Blue"});
    }

    AnalysisConfiguration density_config;
    density_config.graph.graph_kind = "density";
    density_config.graph.x_column = 0;
    auto density_page = GraphService::density(table, density_config);
    datalab::application::InterpretationService::enrich(density_page);
    QVERIFY(density_page.facts.eda.has_value());
    QCOMPARE(density_page.facts.eda->kind, std::string("density"));
    QCOMPARE(density_page.plots.front().kind, datalab::domain::PlotKind::density);

    AnalysisConfiguration hex_config;
    hex_config.graph.graph_kind = "hexbin";
    hex_config.graph.x_column = 0;
    hex_config.graph.y_column = 1;
    const auto hex_page = GraphService::hexbin(table, hex_config);
    QVERIFY(hex_page.facts.eda.has_value());
    QCOMPARE(hex_page.plots.front().kind, datalab::domain::PlotKind::hexbin);

    AnalysisConfiguration violin_config;
    violin_config.graph.graph_kind = "violin";
    violin_config.graph.y_column = 0;
    violin_config.graph.by_column = 2;
    const auto violin_page = GraphService::violin(table, violin_config);
    QVERIFY(violin_page.facts.eda.has_value());
    QCOMPARE(violin_page.plots.front().kind, datalab::domain::PlotKind::violin);

    AnalysisConfiguration bar_config;
    bar_config.graph.graph_kind = "bar";
    bar_config.graph.x_column = 3;
    const auto bar_page = GraphService::bar(table, bar_config);
    QVERIFY(bar_page.facts.eda.has_value());
    QVERIFY(!bar_page.facts.eda->sorted_by_count);
    QVERIFY(!bar_page.facts.eda->has_cumulative_percent);
    QVERIFY(bar_page.plots.front().cumulative_percent.empty());

    const QJsonObject json = datalab::infrastructure::output_page_to_json(density_page);
    const auto restored = datalab::infrastructure::output_page_from_json(json);
    QVERIFY(restored.facts.eda.has_value());
    QCOMPARE(restored.facts.eda->kind, std::string("density"));

    for (const auto& section : density_page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("过程已失控") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("分布已正态") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
        }
    }
}

QTEST_MAIN(P2EdaPlotsTest)
#include "p2_eda_plots_test.moc"
