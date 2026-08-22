#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/control_charts.h"
#include "domain/statistics/multivariate_control.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::application::AnalysisService;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

// # source: formula_reference — docs/research/p2_t2_mewma_nelson_emp.md

class P2T2MewmaNelsonEmpTest final : public QObject {
    Q_OBJECT

private slots:
    void nelsonEstimateExcludesLargeRanges();
    void mssdProducesFiniteSigma();
    void hotellingT2Phase1HasUcl();
    void mewmaHasAsymptoticUclWarning();
    void empClassificationBoundaries();
    void forbiddenClaimsAbsent();
};

void P2T2MewmaNelsonEmpTest::nelsonEstimateExcludesLargeRanges()
{
    std::vector<double> values = {10, 10.1, 9.9, 10.0, 10.2, 10.05, 30.0, 10.1, 9.95, 10.0};
    datalab::domain::statistics::IndividualsMovingRangeOptions base;
    base.moving_range_length = 2;
    const auto without = datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(
        values, base);
    base.use_nelson_estimate = true;
    const auto with = datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(
        values, base);
    QVERIFY(with.nelson_excluded_ranges >= 1);
    QVERIFY(with.sigma < without.sigma);
    QVERIFY(with.sigma_method_label.find("nelson") != std::string::npos);
}

void P2T2MewmaNelsonEmpTest::mssdProducesFiniteSigma()
{
    std::vector<double> values;
    for (int i = 0; i < 20; ++i) {
        values.push_back(10.0 + 0.1 * (i % 3));
    }
    datalab::domain::statistics::IndividualsMovingRangeOptions options;
    options.method = datalab::domain::statistics::SigmaEstimateMethod::mssd;
    const auto dual = datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(
        values, options);
    QVERIFY(std::isfinite(dual.sigma));
    QVERIFY(dual.sigma > 0.0);
    QCOMPARE(dual.sigma_method_label, std::string("mssd"));
}

void P2T2MewmaNelsonEmpTest::hotellingT2Phase1HasUcl()
{
    std::vector<std::vector<double>> rows;
    for (int i = 0; i < 30; ++i) {
        rows.push_back({
            0.1 * i,
            0.05 * i + ((i % 2 == 0) ? 0.2 : -0.1),
            std::sin(0.2 * i)});
    }
    std::vector<std::size_t> source(rows.size());
    for (std::size_t i = 0; i < source.size(); ++i) {
        source[i] = i;
    }
    const auto result = datalab::domain::statistics::hotelling_t2_individuals(rows, source);
    QCOMPARE(result.variable_count, std::size_t{3});
    QVERIFY(result.upper_control_limit > 0.0);
    QCOMPARE(result.limit_method, std::string("phase1_tracy_young_mason_beta"));
    QCOMPARE(result.t2.size(), rows.size());
}

void P2T2MewmaNelsonEmpTest::mewmaHasAsymptoticUclWarning()
{
    std::vector<std::vector<double>> rows;
    for (int i = 0; i < 25; ++i) {
        rows.push_back({static_cast<double>(i), static_cast<double>(i) * 0.5 + 1.0});
    }
    std::vector<std::size_t> source(rows.size());
    for (std::size_t i = 0; i < source.size(); ++i) {
        source[i] = i;
    }
    const auto result = datalab::domain::statistics::mewma_chart(rows, source);
    QVERIFY(result.upper_control_limit > 0.0);
    QCOMPARE(result.ucl_method, std::string("asymptotic_chi_square_approx"));
    bool warned = false;
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == "mewma_ucl_not_arl_calibrated") {
            warned = true;
        }
    }
    QVERIFY(warned);
}

void P2T2MewmaNelsonEmpTest::empClassificationBoundaries()
{
    const auto first = datalab::domain::statistics::emp_classification_from_components(
        0.9, 0.05, 0.03, 0.02);
    QCOMPARE(first.classification, std::string("First"));
    const auto fourth = datalab::domain::statistics::emp_classification_from_components(
        0.05, 0.4, 0.3, 0.25);
    QCOMPARE(fourth.classification, std::string("Fourth"));
    QVERIFY(first.probable_error > 0.0);
}

void P2T2MewmaNelsonEmpTest::forbiddenClaimsAbsent()
{
    DataTable table;
    table.columns = {"Y"};
    for (int i = 0; i < 15; ++i) {
        table.rows.push_back({std::to_string(10.0 + 0.05 * i)});
    }
    AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.control.use_nelson_estimate = true;
    auto page = AnalysisService::individuals_moving_range(table, configuration);
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("过程已失控") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
            QVERIFY(bullet.find("分布已正态") == std::string::npos);
        }
    }
    QVERIFY(page.facts.spc.has_value());
    QVERIFY(page.facts.spc->use_nelson_estimate);
}

QTEST_MAIN(P2T2MewmaNelsonEmpTest)
#include "p2_t2_mewma_nelson_emp_test.moc"
