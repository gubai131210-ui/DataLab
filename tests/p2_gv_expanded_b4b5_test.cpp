#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/expanded_gage_rr.h"
#include "domain/statistics/multivariate_control.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::application::AnalysisService;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

// # source: formula_reference — docs/research/p2_gv_expanded_b4b5.md

class P2GvExpandedB4B5Test final : public QObject {
    Q_OBJECT

private slots:
    void generalizedVarianceSubgroupChart();
    void generalizedVarianceRejectsSmallSubgroup();
    void expandedGageThreeFactorBalanced();
    void imrHistoricalStageTable();
    void forbiddenClaimsAbsent();
};

void P2GvExpandedB4B5Test::generalizedVarianceSubgroupChart()
{
    // 4 subgroups of n=4, p=2
    std::vector<std::vector<std::vector<double>>> subgroups = {
        {{1.0, 2.0}, {1.1, 2.1}, {0.9, 1.9}, {1.05, 2.05}},
        {{1.2, 2.2}, {1.0, 2.0}, {1.1, 1.8}, {0.95, 2.1}},
        {{0.8, 1.7}, {1.0, 2.0}, {1.15, 2.05}, {1.05, 1.95}},
        {{1.0, 2.0}, {1.3, 2.4}, {0.9, 1.8}, {1.1, 2.2}},
    };
    const auto result = datalab::domain::statistics::generalized_variance_chart(subgroups);
    QCOMPARE(result.subgroup_count, std::size_t{4});
    QCOMPARE(result.variable_count, std::size_t{2});
    QCOMPARE(result.subgroup_size, 4);
    QVERIFY(result.b1 > 0.0);
    QVERIFY(result.b2 >= 0.0);
    QVERIFY(result.center_line > 0.0);
    QVERIFY(result.upper_control_limit >= result.center_line);
    QVERIFY(result.lower_control_limit >= 0.0);
    QCOMPARE(result.plotted_determinants.size(), std::size_t{4});
}

void P2GvExpandedB4B5Test::generalizedVarianceRejectsSmallSubgroup()
{
    std::vector<std::vector<std::vector<double>>> subgroups = {
        {{1.0, 2.0}, {1.1, 2.1}},  // n=2, p=2 → invalid
    };
    const auto result = datalab::domain::statistics::generalized_variance_chart(subgroups);
    QVERIFY(!result.diagnostics.empty());
    bool found = false;
    for (const auto& d : result.diagnostics) {
        if (d.code == "subgroup_too_small") {
            found = true;
        }
    }
    QVERIFY(found);
}

void P2GvExpandedB4B5Test::expandedGageThreeFactorBalanced()
{
    // 3 parts × 2 operators × 2 gages × 2 reps = 24
    std::vector<double> y;
    std::vector<std::string> parts;
    std::vector<std::string> opers;
    std::vector<std::string> gages;
    for (int p = 0; p < 3; ++p) {
        for (int o = 0; o < 2; ++o) {
            for (int g = 0; g < 2; ++g) {
                for (int r = 0; r < 2; ++r) {
                    parts.push_back("P" + std::to_string(p));
                    opers.push_back("O" + std::to_string(o));
                    gages.push_back("G" + std::to_string(g));
                    y.push_back(10.0 + 0.5 * p + 0.2 * o + 0.15 * g + 0.05 * r);
                }
            }
        }
    }
    const auto result = datalab::domain::statistics::expanded_gage_rr_three_factor(
        y, parts, opers, gages, 1.0, "Gage");
    QVERIFY(result.three_factor_model);
    QCOMPARE(result.gage.part_count, std::size_t{3});
    QCOMPARE(result.gage.operator_count, std::size_t{2});
    QCOMPARE(result.additional_level_count, std::size_t{2});
    QVERIFY(result.gage.design_balanced);
    QVERIFY(result.gage.variance_components.size() >= 5);
    bool has_total = false;
    for (const auto& c : result.gage.variance_components) {
        if (c.source == "Total Gage R&R") {
            has_total = true;
            QVERIFY(c.variance_component >= 0.0);
        }
    }
    QVERIFY(has_total);
}

void P2GvExpandedB4B5Test::imrHistoricalStageTable()
{
    DataTable table;
    table.columns = {"y", "stage"};
    for (int i = 0; i < 20; ++i) {
        table.rows.push_back({
            std::to_string(10.0 + 0.05 * i),
            i < 10 ? "A" : "B"});
    }
    AnalysisConfiguration config;
    config.analysis_name = "I-MR";
    config.chart_type = "imr";
    config.variable_columns = {0};
    config.control.stage_column = 1;
    config.control.historical_center = 10.0;
    config.control.historical_sigma = 0.2;
    auto page = AnalysisService::individuals_moving_range(table, config);
    QVERIFY(page.facts.spc.has_value());
    QVERIFY(page.facts.spc->historical_parameters_used);
    QCOMPARE(page.facts.spc->stage_count, std::size_t{2});
    bool found_hist_table = false;
    for (const auto& t : page.tables) {
        if (t.title.find("历史参数") != std::string::npos) {
            found_hist_table = true;
        }
    }
    QVERIFY(found_hist_table);

    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("过程已失控") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
        }
    }
}

void P2GvExpandedB4B5Test::forbiddenClaimsAbsent()
{
    DataTable table;
    table.columns = {"x1", "x2", "x3", "x4", "x5"};
    for (int g = 0; g < 6; ++g) {
        for (int i = 0; i < 5; ++i) {
            table.rows.push_back({
                std::to_string(1.0 + 0.01 * g + 0.02 * i),
                std::to_string(2.0 + 0.01 * g - 0.01 * i),
                std::to_string(0.5 * i),
                std::to_string(0.3 * g),
                std::to_string(i % 2)});
        }
    }
    AnalysisConfiguration config;
    config.analysis_name = "广义方差图";
    config.chart_type = "generalized_variance";
    config.variable_columns = {0, 1};
    config.control.subgroup_size = 5;
    auto page = AnalysisService::generalized_variance(table, config);
    QVERIFY(page.facts.multivariate_spc.has_value());
    QCOMPARE(page.facts.multivariate_spc->kind, std::string("generalized_variance"));
    QVERIFY(page.facts.multivariate_spc->subgroup_count > 0);

    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("过程已失控") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
        }
    }
}

QTEST_MAIN(P2GvExpandedB4B5Test)
#include "p2_gv_expanded_b4b5_test.moc"
