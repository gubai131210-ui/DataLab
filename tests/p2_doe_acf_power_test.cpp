#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/autocorrelation.h"
#include "domain/statistics/doe_factorial.h"
#include "domain/statistics/t_power.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class P2DoeAcfPowerTest final : public QObject {
    Q_OBJECT

private slots:
    void fractionalFourMinusOneGeneratorMatchesProduct();
    void acfWhiteNoiseBandAndConstantSeries();
    void acfAr1PacfLag1Dominant();
    void equivalencePowerIncreasesWithN();
    void doeFactorialPowerIncreasesWithReplicates();
    void toleranceSampleSizeFinite();
    void servicePagesAndSerialization();
    void interpretationAvoidsForbiddenClaims();
};

void P2DoeAcfPowerTest::fractionalFourMinusOneGeneratorMatchesProduct()
{
    // # source: formula_reference — 2^(4-1), D=ABC → 8 runs, Res IV, D=A*B*C.
    datalab::domain::statistics::DoeDesignOptions options;
    options.factors = {
        {"A", "-1", "1"}, {"B", "-1", "1"}, {"C", "-1", "1"}, {"D", "-1", "1"}};
    options.fraction_p = 1;
    options.randomize = false;
    const auto design =
        datalab::domain::statistics::generate_2_level_factorial(options);
    QCOMPARE(design.runs.size(), std::size_t{8});
    QCOMPARE(design.resolution, 4);
    QVERIFY(!design.generators.empty());
    for (const auto& run : design.runs) {
        QCOMPARE(run.coded_levels.size(), std::size_t{4});
        const int expected = run.coded_levels[0] * run.coded_levels[1] * run.coded_levels[2];
        QCOMPARE(run.coded_levels[3], expected);
    }
}

void P2DoeAcfPowerTest::acfWhiteNoiseBandAndConstantSeries()
{
    // # source: formula_reference — band ≈ 1.96/√n; constant → zero variance diagnostic.
    const std::vector<double> series = {
        0.1, -0.2, 0.05, 0.3, -0.15, 0.0, 0.12, -0.08, 0.2, -0.1,
        0.05, -0.25, 0.15, 0.0, -0.05, 0.18, -0.12, 0.08, -0.03, 0.1};
    const auto result = datalab::domain::statistics::compute_acf_pacf(series, 5, 0.05);
    QCOMPARE(result.n, series.size());
    QVERIFY(result.band_half_width > 0.0);
    const double expected = 1.95996398454 / std::sqrt(static_cast<double>(series.size()));
    QVERIFY(std::abs(result.band_half_width - expected) < 1.0e-6);
    QCOMPARE(result.acf.front(), 1.0);

    const auto constant = datalab::domain::statistics::compute_acf_pacf(
        {3.0, 3.0, 3.0, 3.0}, 2, 0.05);
    QVERIFY(std::any_of(constant.diagnostics.cbegin(), constant.diagnostics.cend(),
                        [](const datalab::domain::DiagnosticMessage& d) {
                            return d.code == "acf_zero_variance";
                        }));
}

void P2DoeAcfPowerTest::acfAr1PacfLag1Dominant()
{
    // # source: formula_reference — AR(1)-like path → |PACF(1)| large vs later lags.
    std::vector<double> series = {0.0};
    for (int i = 0; i < 80; ++i) {
        series.push_back(0.8 * series.back() + ((i % 2 == 0) ? 0.05 : -0.04));
    }
    const auto result = datalab::domain::statistics::compute_acf_pacf(series, 6, 0.05);
    QVERIFY(result.pacf.size() > 2);
    QVERIFY(std::abs(result.pacf[1]) > 0.4);
    QVERIFY(std::abs(result.pacf[1]) > std::abs(result.pacf[2]));
}

void P2DoeAcfPowerTest::equivalencePowerIncreasesWithN()
{
    // # source: formula_reference — θ=0, ±δ, larger n → higher power.
    const auto small = datalab::domain::statistics::equivalence_one_sample_power(
        10, -0.5, 0.5, 0.0, 0.05);
    const auto large = datalab::domain::statistics::equivalence_one_sample_power(
        40, -0.5, 0.5, 0.0, 0.05);
    QVERIFY(large.power + 1.0e-9 >= small.power);
    QVERIFY(large.power > 0.5);
}

void P2DoeAcfPowerTest::doeFactorialPowerIncreasesWithReplicates()
{
    // # source: formula_reference — more replicates → higher power.
    const auto one = datalab::domain::statistics::doe_factorial_power(4, 1, 1, 1.0, 0.05);
    const auto two = datalab::domain::statistics::doe_factorial_power(4, 1, 2, 1.0, 0.05);
    QVERIFY(two.power + 1.0e-9 >= one.power);
}

void P2DoeAcfPowerTest::toleranceSampleSizeFinite()
{
    // # source: formula_reference — Howe k(n) ≤ 4 for P=γ=0.95 yields finite n.
    const auto result = datalab::domain::statistics::tolerance_normal_sample_size(
        0.95, 0.95, 4.0);
    QVERIFY(result.sample_size >= 2);
    QVERIFY(std::isfinite(result.critical_value));
}

void P2DoeAcfPowerTest::servicePagesAndSerialization()
{
    AnalysisConfiguration design_config;
    design_config.doe.factor_names = {"A", "B", "C", "D"};
    design_config.doe.low_levels = {"-1", "-1", "-1", "-1"};
    design_config.doe.high_levels = {"1", "1", "1", "1"};
    design_config.doe.fraction_p = 1;
    design_config.doe.randomize = false;
    DataTable empty;
    const auto design_page =
        datalab::application::AnalysisService::doe_factorial(empty, design_config);
    QVERIFY(design_page.facts.doe.has_value());
    QCOMPARE(design_page.facts.doe->fraction_p, std::size_t{1});
    QCOMPARE(design_page.facts.doe->run_count, std::size_t{8});
    QVERIFY(std::any_of(design_page.tables.cbegin(), design_page.tables.cend(),
                        [](const datalab::domain::StatisticTable& t) {
                            return t.title == "别名结构";
                        }));

    DataTable series;
    series.columns = {"Y"};
    for (int i = 0; i < 30; ++i) {
        series.rows.push_back({std::to_string(std::sin(0.3 * i) + 0.01 * i)});
    }
    AnalysisConfiguration acf_config;
    acf_config.variable_columns = {0};
    const auto acf_page =
        datalab::application::AnalysisService::acf_pacf(series, acf_config);
    QVERIFY(acf_page.facts.acf_pacf.has_value());
    QCOMPARE(acf_page.plots.size(), std::size_t{2});

    AnalysisConfiguration power_config;
    power_config.power.mode = "equivalence_one_sample_power";
    power_config.power.effect_size = 0.5;
    power_config.power.sample_size = 20;
    power_config.power.alpha = 0.05;
    const auto power_page =
        datalab::application::AnalysisService::t_power(empty, power_config);
    QVERIFY(power_page.facts.power.has_value());

    const QJsonObject json =
        datalab::infrastructure::output_page_to_json(design_page);
    const auto restored = datalab::infrastructure::output_page_from_json(json);
    QVERIFY(restored.facts.doe.has_value());
    QCOMPARE(restored.facts.doe->run_count, design_page.facts.doe->run_count);
}

void P2DoeAcfPowerTest::interpretationAvoidsForbiddenClaims()
{
    DataTable series;
    series.columns = {"Y"};
    series.rows = {{"1"}, {"2"}, {"1.5"}, {"2.2"}, {"1.1"}, {"2.0"}, {"1.8"}, {"2.1"}};
    AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    auto page = datalab::application::AnalysisService::acf_pacf(series, configuration);
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("过程已失控") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
        }
    }
}

QTEST_MAIN(P2DoeAcfPowerTest)
#include "p2_doe_acf_power_test.moc"
