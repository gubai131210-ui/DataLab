#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/general_manova.h"
#include "domain/statistics/mixed_effects_reml.h"
#include "domain/statistics/binary_doe_probit.h"
#include "domain/statistics/life_data_lognormal.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class AlgorithmWave10TrackTest final : public QObject {
    Q_OBJECT

private slots:
    void generalManovaFormulaReference();
    void generalManovaServiceAndSerialize();
    void mixedEffectsFormulaReference();
    void mixedEffectsServiceAndSerialize();
    void binaryDoeProbitFormulaReference();
    void binaryDoeProbitServiceAndSerialize();
    void lifeDataLognormalFormulaReference();
    void lifeDataLognormalServiceAndSerialize();
    void wave10InterpretationNoForbiddenPhrases();
};

void AlgorithmWave10TrackTest::generalManovaFormulaReference()
{
    // # source: formula_reference — Type III SSCP single factor (two responses).
    std::vector<std::vector<double>> responses = {
        {1.0, 2.0}, {1.1, 2.1}, {1.2, 2.0},
        {2.0, 3.0}, {2.1, 3.1}, {2.2, 3.0},
        {3.0, 4.0}, {3.1, 4.1}, {3.2, 4.0}};
    std::vector<std::string> fa = {
        "G1", "G1", "G1", "G2", "G2", "G2", "G3", "G3", "G3"};
    datalab::domain::statistics::GeneralManovaOptions options;
    const auto result = datalab::domain::statistics::general_manova_analyze(
        responses, fa, {}, {}, {}, options);
    QCOMPARE(result.observation_count, std::size_t{9});
    QCOMPARE(result.response_count, std::size_t{2});
    QVERIFY(!result.effect_tests.empty());
}

void AlgorithmWave10TrackTest::generalManovaServiceAndSerialize()
{
    // Marker: GeneralManovaFacts
    DataTable table;
    table.columns = {"Y1", "Y2", "A", "B"};
    table.rows = {
        {"1", "2", "L", "A"}, {"1.1", "2.1", "L", "A"},
        {"2", "3", "L", "B"}, {"2.1", "3.1", "L", "B"},
        {"3", "4", "H", "A"}, {"3.1", "4.1", "H", "A"},
        {"4", "5", "H", "B"}, {"4.1", "5.1", "H", "B"}};
    AnalysisConfiguration config;
    config.general_manova.response_columns = {0, 1};
    config.general_manova.factor_a_column = 2;
    config.general_manova.factor_b_column = 3;
    auto page = datalab::application::AnalysisService::general_manova(table, config);
    QVERIFY(page.facts.general_manova.has_value());
    QCOMPARE(page.facts.general_manova->observation_count, std::size_t{8});

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.general_manova.has_value());
    QCOMPARE(restored.facts.general_manova->algorithm_id,
             std::string("general_manova_type3_sscp"));
}

void AlgorithmWave10TrackTest::mixedEffectsFormulaReference()
{
    // # source: formula_reference — random intercept REML.
    std::vector<double> y = {10.0, 11.0, 12.0, 20.0, 21.0, 22.0};
    std::vector<std::string> random = {"R1", "R1", "R2", "R3", "R3", "R4"};
    std::vector<std::string> fixed = {"L", "L", "L", "H", "H", "H"};
    const auto result = datalab::domain::statistics::mixed_effects_reml_analyze(
        y, random, fixed, {}, {}, {}, {});
    QCOMPARE(result.observation_count, std::size_t{6});
    QVERIFY(result.random_level_count >= std::size_t{2});
    QVERIFY(!result.variance_components.empty());
}

void AlgorithmWave10TrackTest::mixedEffectsServiceAndSerialize()
{
    // Marker: MixedEffectsRemlFacts
    DataTable table;
    table.columns = {"Y", "Random", "Fixed"};
    table.rows = {
        {"10", "R1", "L"}, {"11", "R1", "L"}, {"12", "R2", "L"},
        {"20", "R3", "H"}, {"21", "R3", "H"}, {"22", "R4", "H"}};
    AnalysisConfiguration config;
    config.mixed_effects_reml.response_column = 0;
    config.mixed_effects_reml.random_factor_column = 1;
    config.mixed_effects_reml.fixed_factor_a_column = 2;
    auto page = datalab::application::AnalysisService::mixed_effects_reml(table, config);
    QVERIFY(page.facts.mixed_effects_reml.has_value());
    QCOMPARE(page.facts.mixed_effects_reml->observation_count, std::size_t{6});

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.mixed_effects_reml.has_value());
    QCOMPARE(restored.facts.mixed_effects_reml->algorithm_id,
             std::string("mixed_effects_reml_variance"));
}

void AlgorithmWave10TrackTest::binaryDoeProbitFormulaReference()
{
    // # source: formula_reference — probit IRWLS factorial.
    std::vector<std::vector<std::string>> factors = {
        {"L", "L", "H", "H"}, {"A", "B", "A", "B"}};
    std::vector<int> events = {2, 1, 3, 0};
    std::vector<int> trials = {5, 5, 5, 5};
    datalab::domain::statistics::BinaryDoeProbitOptions options;
    options.link = "probit";
    const auto result = datalab::domain::statistics::analyze_binary_doe_probit(
        factors, events, trials, {}, {}, options);
    QCOMPARE(result.design_row_count, std::size_t{4});
    QVERIFY(result.expanded_observation_count >= std::size_t{4});
    QVERIFY(!result.coefficients.empty());
}

void AlgorithmWave10TrackTest::binaryDoeProbitServiceAndSerialize()
{
    // Marker: BinaryDoeProbitFacts
    DataTable table;
    table.columns = {"F1", "F2", "Events", "Trials"};
    table.rows = {
        {"L", "A", "2", "5"}, {"L", "B", "1", "5"},
        {"H", "A", "3", "5"}, {"H", "B", "0", "5"}};
    AnalysisConfiguration config;
    config.binary_doe_probit.factor_columns = {0, 1};
    config.binary_doe_probit.events_column = 2;
    config.binary_doe_probit.trials_column = 3;
    config.binary_doe_probit.link = "probit";
    auto page = datalab::application::AnalysisService::binary_doe_probit(table, config);
    QVERIFY(page.facts.binary_doe_probit.has_value());
    QCOMPARE(page.facts.binary_doe_probit->link, std::string("probit"));

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.binary_doe_probit.has_value());
    QCOMPARE(restored.facts.binary_doe_probit->algorithm_id,
             std::string("binary_doe_probit_irwls"));
}

void AlgorithmWave10TrackTest::lifeDataLognormalFormulaReference()
{
    // # source: formula_reference — lognormal MLE with censoring.
    std::vector<double> times = {10.0, 12.0, 15.0, 20.0, 25.0, 30.0};
    std::vector<bool> events = {true, true, false, true, true, false};
    std::vector<std::vector<double>> cov = {{0.0}, {1.0}, {0.0}, {1.0}, {0.0}, {1.0}};
    const auto result = datalab::domain::statistics::fit_life_data_lognormal(
        times, events, cov, {}, {}, {});
    QCOMPARE(result.observation_count, std::size_t{6});
    QVERIFY(result.failure_count >= std::size_t{2});
    QVERIFY(!result.coefficients.empty());
}

void AlgorithmWave10TrackTest::lifeDataLognormalServiceAndSerialize()
{
    // Marker: LifeDataLognormalFacts
    DataTable table;
    table.columns = {"Time", "Event", "X1"};
    table.rows = {
        {"10", "1", "0"}, {"12", "1", "1"}, {"15", "0", "0"},
        {"20", "1", "1"}, {"25", "1", "0"}, {"30", "0", "1"}};
    AnalysisConfiguration config;
    config.life_data_lognormal.time_column = 0;
    config.life_data_lognormal.event_column = 1;
    config.life_data_lognormal.covariate_columns = {2};
    auto page = datalab::application::AnalysisService::life_data_lognormal(table, config);
    QVERIFY(page.facts.life_data_lognormal.has_value());
    QCOMPARE(page.facts.life_data_lognormal->observation_count, std::size_t{6});

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.life_data_lognormal.has_value());
    QCOMPARE(restored.facts.life_data_lognormal->algorithm_id,
             std::string("life_data_lognormal_mle"));
}

void AlgorithmWave10TrackTest::wave10InterpretationNoForbiddenPhrases()
{
    DataTable table;
    table.columns = {"Y1", "Y2", "A", "B"};
    table.rows = {
        {"1", "2", "L", "A"}, {"1.1", "2.1", "L", "A"},
        {"2", "3", "L", "B"}, {"2.1", "3.1", "L", "B"}};
    AnalysisConfiguration config;
    config.general_manova.response_columns = {0, 1};
    config.general_manova.factor_a_column = 2;
    config.general_manova.factor_b_column = 3;
    auto page = datalab::application::AnalysisService::general_manova(table, config);
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("见 md") == std::string::npos);
            QVERIFY(bullet.find("见md") == std::string::npos);
            QVERIFY(bullet.find("过程已失控") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
            QVERIFY(bullet.find("分布已正态") == std::string::npos);
            QVERIFY(bullet.find("测量系统合格") == std::string::npos);
        }
    }
}

QTEST_APPLESS_MAIN(AlgorithmWave10TrackTest)
#include "algorithm_wave10_track_test.moc"
