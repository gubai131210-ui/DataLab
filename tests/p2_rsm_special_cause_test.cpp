#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/quality_types.h"
#include "domain/statistics/control_charts.h"
#include "domain/statistics/rsm_analysis.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::application::AnalysisService;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::RsmFacts;
using datalab::domain::statistics::ControlChartKind;
using datalab::domain::statistics::SpecialCauseSelection;
using datalab::domain::statistics::fit_rsm_analysis;
using datalab::domain::statistics::resolve_special_cause_tests;

// # source: formula_reference — docs/research/p2_rsm_special_cause_rules.md

class P2RsmSpecialCauseTest final : public QObject {
    Q_OBJECT

private slots:
    void rsmQuadraticFitHasResidualsAndContour();
    void rsmConstantResponseQuadraticNearZero();
    void minitabLikeDefaultsToTestOne();
    void allApplicableDefaultsToEightOnIndividuals();
    void rsmFactsRoundTrip();
};

void P2RsmSpecialCauseTest::rsmQuadraticFitHasResidualsAndContour()
{
    std::vector<std::vector<double>> coded;
    std::vector<double> response;
    for (double x1 : {-1.0, 0.0, 1.0}) {
        for (double x2 : {-1.0, 0.0, 1.0}) {
            coded.push_back({x1, x2});
            response.push_back(1.0 + 2.0 * x1 - x2 + 0.5 * x1 * x2 + x1 * x1);
        }
    }
    const auto fit = fit_rsm_analysis(response, coded, {"A", "B"}, "Y");
    QVERIFY(fit.regression.coefficients.size() >= 6);
    QCOMPARE(fit.regression.observations.size(), std::size_t{9});
    QVERIFY(std::isfinite(fit.regression.r_squared));
    QVERIFY(fit.regression.r_squared > 0.99);

    DataTable table;
    table.columns = {"Y", "A", "B"};
    for (std::size_t i = 0; i < response.size(); ++i) {
        table.rows.push_back({
            std::to_string(response[i]),
            std::to_string(coded[i][0]),
            std::to_string(coded[i][1])});
    }
    AnalysisConfiguration configuration;
    configuration.variable_columns = {0, 1, 2};
    auto page = AnalysisService::rsm_response(table, configuration);
    datalab::application::InterpretationService::enrich(page);
    QVERIFY(page.facts.rsm.has_value());
    QCOMPARE(page.facts.rsm->factor_count, std::size_t{2});
    QVERIFY(page.facts.rsm->residual_count > 0);
    QVERIFY(page.facts.rsm->contour_plot_available);
    int residual_plots = 0;
    int surface_or_contour = 0;
    for (const auto& plot : page.plots) {
        if (plot.title.find("残差") != std::string::npos) {
            ++residual_plots;
        }
        if (plot.kind == datalab::domain::PlotKind::contour
            || plot.kind == datalab::domain::PlotKind::surface) {
            ++surface_or_contour;
        }
    }
    QVERIFY(residual_plots >= 3);
    QVERIFY(surface_or_contour >= 1);
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("过程已失控") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
        }
    }
}

void P2RsmSpecialCauseTest::rsmConstantResponseQuadraticNearZero()
{
    std::vector<std::vector<double>> coded = {
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}, {0, 0}, {0, 0}};
    std::vector<double> response(coded.size(), 10.0);
    const auto fit = fit_rsm_analysis(response, coded, {"A", "B"}, "Y");
    QVERIFY(fit.regression.coefficients.size() >= 6);
    for (std::size_t i = 1; i < fit.regression.coefficients.size(); ++i) {
        QVERIFY(std::abs(fit.regression.coefficients[i].coefficient) < 1.0e-8);
    }
}

void P2RsmSpecialCauseTest::minitabLikeDefaultsToTestOne()
{
    SpecialCauseSelection selection;
    selection.policy = "minitab_like";
    QCOMPARE(resolve_special_cause_tests(selection, ControlChartKind::individuals),
             (std::vector<int>{1}));
}

void P2RsmSpecialCauseTest::allApplicableDefaultsToEightOnIndividuals()
{
    SpecialCauseSelection selection;
    selection.policy = "all_applicable";
    QCOMPARE(resolve_special_cause_tests(selection, ControlChartKind::individuals).size(),
             std::size_t{8});
}

void P2RsmSpecialCauseTest::rsmFactsRoundTrip()
{
    datalab::domain::OutputPage page;
    page.id = "rsm_roundtrip";
    page.title = "响应曲面分析";
    page.method_name = "Response Surface Analysis";
    RsmFacts facts;
    facts.factor_count = 2;
    facts.term_count = 6;
    facts.residual_count = 9;
    facts.r_squared = 0.99;
    facts.adjusted_r_squared = 0.98;
    facts.contour_plot_available = true;
    facts.largest_abs_t_term = "A";
    facts.response_name = "Y";
    page.facts.rsm = facts;
    page.facts.spc = datalab::domain::SpcFacts{};
    page.facts.spc->rule_policy = "minitab_like";
    page.facts.spc->enabled_special_cause_tests = {1};
    page.configuration.control.special_cause_rule_policy = "minitab_like";

    const QJsonObject json = datalab::infrastructure::output_page_to_json(page);
    const auto restored = datalab::infrastructure::output_page_from_json(json);
    QVERIFY(restored.facts.rsm.has_value());
    QCOMPARE(restored.facts.rsm->factor_count, std::size_t{2});
    QCOMPARE(restored.facts.rsm->largest_abs_t_term, std::string("A"));
    QVERIFY(restored.facts.spc.has_value());
    QCOMPARE(restored.facts.spc->rule_policy, std::string("minitab_like"));
    QCOMPARE(restored.facts.spc->enabled_special_cause_tests, (std::vector<int>{1}));
}

QTEST_MAIN(P2RsmSpecialCauseTest)
#include "p2_rsm_special_cause_test.moc"
