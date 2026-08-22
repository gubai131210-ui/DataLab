#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/quality_types.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::application::AnalysisService;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::PlotKind;

// # source: formula_reference — docs/research/p2_eda4_crosstab_chi_resid.md

class P2Eda4CrosstabChiResidTest final : public QObject {
    Q_OBJECT

private slots:
    void eda4plotHasFourPlots();
    void crossTabHasPercentsNoChiSquareFacts();
    void chiSquareDeepensResidualsAndPercents();
    void interpretationAvoidsForbiddenClaims();
};

DataTable numeric_series_table()
{
    DataTable table;
    table.columns = {"Y"};
    for (int i = 0; i < 20; ++i) {
        table.rows.push_back({std::to_string(static_cast<double>(i) + 0.1 * (i % 3))});
    }
    return table;
}

DataTable category_pair_table()
{
    DataTable table;
    table.columns = {"Row", "Col"};
    const std::vector<std::pair<std::string, std::string>> pairs = {
        {"A", "X"}, {"A", "X"}, {"A", "Y"},
        {"B", "X"}, {"B", "Y"}, {"B", "Y"}, {"B", "Y"},
        {"C", "X"}, {"C", "X"}, {"C", "Y"},
    };
    for (const auto& [r, c] : pairs) {
        table.rows.push_back({r, c});
    }
    return table;
}

void P2Eda4CrosstabChiResidTest::eda4plotHasFourPlots()
{
    AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    auto page = AnalysisService::eda_4plot(numeric_series_table(), configuration);
    QCOMPARE(page.plots.size(), std::size_t{4});
    QCOMPARE(page.plots[0].title, std::string("Run Sequence"));
    QCOMPARE(page.plots[1].title, std::string("Lag-1 Plot"));
    QCOMPARE(page.plots[2].kind, PlotKind::histogram);
    QCOMPARE(page.plots[3].kind, PlotKind::probability);
    QVERIFY(page.facts.eda.has_value());
    QCOMPARE(page.facts.eda->kind, std::string("eda_4plot"));
    QCOMPARE(page.facts.eda->n, std::size_t{20});

    const QJsonObject json = datalab::infrastructure::output_page_to_json(page);
    const auto round_trip = datalab::infrastructure::output_page_from_json(json);
    QVERIFY(round_trip.facts.eda.has_value());
    QCOMPARE(round_trip.facts.eda->kind, std::string("eda_4plot"));
}

void P2Eda4CrosstabChiResidTest::crossTabHasPercentsNoChiSquareFacts()
{
    AnalysisConfiguration configuration;
    configuration.inference.row_category_column = 0;
    configuration.inference.column_category_column = 1;
    auto page = AnalysisService::cross_tabulation(category_pair_table(), configuration);
    QVERIFY(page.facts.cross_tab.has_value());
    QVERIFY(!page.facts.chi_square.has_value());
    QCOMPARE(page.facts.cross_tab->total_count, std::size_t{10});
    QVERIFY(page.facts.cross_tab->percent_tables_available);
    QVERIFY(page.tables.size() >= 4);
    bool found_info = false;
    for (const auto& diagnostic : page.diagnostics) {
        if (diagnostic.code == "cross_tab_no_chi_square") {
            found_info = true;
        }
    }
    QVERIFY(found_info);

    const QJsonObject json = datalab::infrastructure::output_page_to_json(page);
    const auto round_trip = datalab::infrastructure::output_page_from_json(json);
    QVERIFY(round_trip.facts.cross_tab.has_value());
    QCOMPARE(round_trip.facts.cross_tab->total_count, std::size_t{10});
}

void P2Eda4CrosstabChiResidTest::chiSquareDeepensResidualsAndPercents()
{
    AnalysisConfiguration configuration;
    configuration.inference.row_category_column = 0;
    configuration.inference.column_category_column = 1;
    auto page = AnalysisService::chi_square(category_pair_table(), configuration);
    QVERIFY(page.facts.chi_square.has_value());
    QVERIFY(page.facts.chi_square->percent_tables_available);
    QVERIFY(page.facts.chi_square->residual_heatmap_available);
    QVERIFY(page.facts.chi_square->max_abs_adjusted_residual.has_value());
    QVERIFY(!page.facts.chi_square->largest_contribution_cell.empty());
    QVERIFY(page.plots.size() >= 2);
    QCOMPARE(page.plots.back().title, std::string("调整残差热图"));

    bool has_row_pct = false;
    for (const auto& table : page.tables) {
        if (table.title == "行百分比") {
            has_row_pct = true;
        }
    }
    QVERIFY(has_row_pct);

    const QJsonObject json = datalab::infrastructure::output_page_to_json(page);
    const auto round_trip = datalab::infrastructure::output_page_from_json(json);
    QVERIFY(round_trip.facts.chi_square.has_value());
    QVERIFY(round_trip.facts.chi_square->residual_heatmap_available);
    QCOMPARE(round_trip.facts.chi_square->largest_contribution_cell,
             page.facts.chi_square->largest_contribution_cell);
}

void P2Eda4CrosstabChiResidTest::interpretationAvoidsForbiddenClaims()
{
    AnalysisConfiguration eda_config;
    eda_config.variable_columns = {0};
    auto eda_page = AnalysisService::eda_4plot(numeric_series_table(), eda_config);
    datalab::application::InterpretationService::enrich(eda_page);

    AnalysisConfiguration tab_config;
    tab_config.inference.row_category_column = 0;
    tab_config.inference.column_category_column = 1;
    auto tab_page = AnalysisService::cross_tabulation(category_pair_table(), tab_config);
    datalab::application::InterpretationService::enrich(tab_page);
    auto chi_page = AnalysisService::chi_square(category_pair_table(), tab_config);
    datalab::application::InterpretationService::enrich(chi_page);

    const auto check = [](const datalab::domain::OutputPage& page) {
        for (const auto& section : page.interpretation) {
            for (const auto& bullet : section.bullets) {
                QVERIFY(bullet.find("过程已失控") == std::string::npos);
                QVERIFY(bullet.find("已证明稳定") == std::string::npos);
                QVERIFY(bullet.find("批次合格") == std::string::npos);
                QVERIFY(bullet.find("分布已正态") == std::string::npos);
            }
        }
    };
    check(eda_page);
    check(tab_page);
    check(chi_page);
}

QTEST_MAIN(P2Eda4CrosstabChiResidTest)
#include "p2_eda4_crosstab_chi_resid_test.moc"
