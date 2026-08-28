#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/binary_response_doe.h"
#include "domain/statistics/cluster_variables.h"
#include "domain/statistics/glm_three_factor.h"
#include "domain/statistics/life_data_regression.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class AlgorithmWave8TrackTest final : public QObject {
    Q_OBJECT

private slots:
    void binaryDoeFormulaReference();
    void binaryDoeServiceAndSerialize();
    void clusterVariablesFormulaReference();
    void clusterVariablesServiceAndSerialize();
    void glmThreeFactorFormulaReference();
    void glmThreeFactorServiceAndSerialize();
    void lifeRegressionFormulaReference();
    void lifeRegressionServiceAndSerialize();
    void wave8InterpretationNoForbiddenPhrases();
};

void AlgorithmWave8TrackTest::binaryDoeFormulaReference()
{
    // # source: formula_reference — 2×2 logit; OR = exp(beta).
    std::vector<std::vector<std::string>> factors = {
        {"low", "low", "high", "high"},
        {"low", "high", "low", "high"}};
    std::vector<int> events = {8, 2, 3, 9};
    std::vector<int> trials = {10, 10, 10, 10};
    datalab::domain::statistics::BinaryResponseDoeOptions options;
    options.use_events_trials = true;
    const auto result = datalab::domain::statistics::analyze_binary_response_doe(
        factors, events, trials, {"A", "B"}, {}, options);
    QVERIFY(result.expanded_observation_count >= std::size_t{20});
    QVERIFY(!result.coefficients.empty());
    const auto intercept = std::find_if(
        result.coefficients.cbegin(), result.coefficients.cend(),
        [](const auto& row) { return row.term == "Intercept"; });
    if (intercept != result.coefficients.cend()) {
        QVERIFY(intercept->odds_ratio > 0.0);
    }
}

void AlgorithmWave8TrackTest::binaryDoeServiceAndSerialize()
{
    // Marker: BinaryResponseDoeFacts
    DataTable table;
    table.columns = {"A", "B", "Events", "Trials"};
    table.rows = {
        {"low", "low", "8", "10"}, {"low", "high", "2", "10"},
        {"high", "low", "3", "10"}, {"high", "high", "9", "10"}};
    AnalysisConfiguration config;
    config.binary_response_doe.factor_columns = {0, 1};
    config.binary_response_doe.events_column = 2;
    config.binary_response_doe.trials_column = 3;
    config.binary_response_doe.use_events_trials = true;
    auto page = datalab::application::AnalysisService::binary_response_doe(table, config);
    QVERIFY(page.facts.binary_response_doe.has_value());
    QVERIFY(!page.tables.empty());

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.binary_response_doe.has_value());
    QCOMPARE(restored.facts.binary_response_doe->algorithm_id,
             std::string("binary_response_doe_logit_irwls"));
}

void AlgorithmWave8TrackTest::clusterVariablesFormulaReference()
{
    // # source: formula_reference — rho=1 => d=0; merges = p-1.
    std::vector<std::vector<double>> data = {
        {1.0, 2.0, 3.0}, {2.0, 4.0, 6.0}, {3.0, 6.0, 9.0}, {4.0, 8.0, 12.0}};
    datalab::domain::statistics::ClusterVariablesOptions options;
    options.linkage = "complete";
    const auto result = datalab::domain::statistics::cluster_variables_analyze(
        data, {"V1", "V2", "V3"}, options);
    QCOMPARE(result.variable_count, std::size_t{3});
    QCOMPARE(result.merges.size(), std::size_t{2});
    QCOMPARE(result.distance_matrix[0][1], 0.0);
}

void AlgorithmWave8TrackTest::clusterVariablesServiceAndSerialize()
{
    // Marker: ClusterVariablesFacts
    DataTable table;
    table.columns = {"V1", "V2", "V3"};
    table.rows = {
        {"1", "2", "3"}, {"2", "4", "6"}, {"3", "6", "9"}, {"4", "8", "12"}};
    AnalysisConfiguration config;
    config.cluster_variables.variable_columns = {0, 1, 2};
    auto page = datalab::application::AnalysisService::cluster_variables(table, config);
    QVERIFY(page.facts.cluster_variables.has_value());
    QCOMPARE(page.facts.cluster_variables->merge_count, std::size_t{2});
    QVERIFY(!page.plots.empty());

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.cluster_variables.has_value());
    QCOMPARE(restored.facts.cluster_variables->algorithm_id,
             std::string("cluster_variables_corr_hclust"));
}

void AlgorithmWave8TrackTest::glmThreeFactorFormulaReference()
{
    // # source: formula_reference — unbalanced 3-factor Type III estimable.
    std::vector<std::string> a = {"1", "1", "1", "2", "2", "2"};
    std::vector<std::string> b = {"1", "1", "2", "1", "2", "2"};
    std::vector<std::string> c = {"1", "2", "1", "2", "1", "2"};
    std::vector<double> y = {10.0, 11.0, 12.0, 20.0, 21.0, 22.0};
    datalab::domain::statistics::GlmThreeFactorOptions options;
    const auto result = datalab::domain::statistics::glm_three_factor_analyze(
        a, b, c, y, {}, options);
    QCOMPARE(result.observation_count, std::size_t{6});
    QVERIFY(result.anova_effects.size() >= std::size_t{3});
    QVERIFY(!result.fitted_means.empty());
}

void AlgorithmWave8TrackTest::glmThreeFactorServiceAndSerialize()
{
    // Marker: GlmThreeFactorFacts
    DataTable table;
    table.columns = {"A", "B", "C", "Y"};
    table.rows = {
        {"1", "1", "1", "10"}, {"1", "1", "2", "11"}, {"1", "2", "1", "12"},
        {"2", "1", "2", "20"}, {"2", "2", "1", "21"}, {"2", "2", "2", "22"}};
    AnalysisConfiguration config;
    config.glm_three_factor.response_column = 3;
    config.glm_three_factor.factor_a_column = 0;
    config.glm_three_factor.factor_b_column = 1;
    config.glm_three_factor.factor_c_column = 2;
    auto page = datalab::application::AnalysisService::glm_three_factor(table, config);
    QVERIFY(page.facts.glm_three_factor.has_value());
    QCOMPARE(page.facts.glm_three_factor->observation_count, std::size_t{6});

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.glm_three_factor.has_value());
    QCOMPARE(restored.facts.glm_three_factor->algorithm_id,
             std::string("glm_three_factor_type3"));
}

void AlgorithmWave8TrackTest::lifeRegressionFormulaReference()
{
    // # source: formula_reference — Weibull MLE with censoring converges.
    std::vector<double> times = {10.0, 12.0, 15.0, 18.0, 20.0, 25.0, 30.0, 35.0};
    std::vector<bool> events = {true, true, false, true, true, false, true, true};
    std::vector<std::vector<double>> covariates = {
        {0.0}, {1.0}, {0.0}, {1.0}, {0.0}, {1.0}, {0.0}, {1.0}};
    datalab::domain::statistics::LifeDataRegressionOptions options;
    const auto result = datalab::domain::statistics::fit_life_data_regression_weibull(
        times, events, covariates, {"X1"}, {}, options);
    QCOMPARE(result.observation_count, std::size_t{8});
    QVERIFY(result.failure_count >= std::size_t{2});
    QVERIFY(result.shape > 0.0);
    QVERIFY(!result.coefficients.empty());
}

void AlgorithmWave8TrackTest::lifeRegressionServiceAndSerialize()
{
    // Marker: LifeDataRegressionFacts
    DataTable table;
    table.columns = {"Time", "Event", "X1"};
    table.rows = {
        {"10", "1", "0"}, {"12", "1", "1"}, {"15", "0", "0"}, {"18", "1", "1"},
        {"20", "1", "0"}, {"25", "0", "1"}, {"30", "1", "0"}, {"35", "1", "1"}};
    AnalysisConfiguration config;
    config.life_data_regression.time_column = 0;
    config.life_data_regression.censor_column = 1;
    config.life_data_regression.covariate_columns = {2};
    auto page =
        datalab::application::AnalysisService::life_data_regression(table, config);
    QVERIFY(page.facts.life_data_regression.has_value());
    QVERIFY(!page.tables.empty());

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.life_data_regression.has_value());
    QCOMPARE(restored.facts.life_data_regression->algorithm_id,
             std::string("life_data_regression_weibull_mle"));
}

void AlgorithmWave8TrackTest::wave8InterpretationNoForbiddenPhrases()
{
    DataTable table;
    table.columns = {"V1", "V2", "V3"};
    table.rows = {
        {"1", "2", "3"}, {"2", "4", "6"}, {"3", "6", "9"}, {"4", "8", "12"}};
    AnalysisConfiguration config;
    config.cluster_variables.variable_columns = {0, 1, 2};
    auto page = datalab::application::AnalysisService::cluster_variables(table, config);
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("见 md") == std::string::npos);
            QVERIFY(bullet.find("见md") == std::string::npos);
            QVERIFY(bullet.find("过程已失控") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
            QVERIFY(bullet.find("分布已正态") == std::string::npos);
            QVERIFY(bullet.find("寿命已达标") == std::string::npos);
        }
    }
}

QTEST_APPLESS_MAIN(AlgorithmWave8TrackTest)
#include "algorithm_wave8_track_test.moc"
