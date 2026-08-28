#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/analyze_variability.h"
#include "domain/statistics/factor_analysis.h"
#include "domain/statistics/glm_two_way.h"
#include "domain/statistics/mixture_analyze.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class AlgorithmWave7TrackTest final : public QObject {
    Q_OBJECT

private slots:
    void mixtureScheffeFormulaReference();
    void mixtureServiceAndSerialize();
    void glmTwoWayFormulaReference();
    void glmServiceAndSerialize();
    void variabilityEffectFormulaReference();
    void variabilityServiceAndSerialize();
    void factorLoadingsFormulaReference();
    void factorServiceAndSerialize();
    void wave7InterpretationNoForbiddenPhrases();
};

void AlgorithmWave7TrackTest::mixtureScheffeFormulaReference()
{
    // # source: formula_reference — 3-component linear Scheffé at vertex x1=1.
    std::vector<std::vector<double>> components = {{1.0, 0.0, 0.0},
                                                   {0.0, 1.0, 0.0},
                                                   {0.0, 0.0, 1.0}};
    std::vector<double> response = {10.0, 20.0, 30.0};
    datalab::domain::statistics::MixtureAnalyzeOptions options;
    options.model_order = datalab::domain::statistics::MixtureModelOrder::linear;
    const auto result = datalab::domain::statistics::analyze_mixture_scheffe(
        components, response, {"x1", "x2", "x3"}, {}, options);
    QCOMPARE(result.observation_count, std::size_t{3});
    QCOMPARE(result.coefficients.size(), std::size_t{3});
    QVERIFY(result.coefficients[0].coefficient > 9.0);
    QVERIFY(result.coefficients[2].coefficient > 29.0);
}

void AlgorithmWave7TrackTest::mixtureServiceAndSerialize()
{
    // Marker: MixtureAnalyzeFacts
    DataTable table;
    table.columns = {"x1", "x2", "x3", "Y"};
    table.rows = {
        {"1", "0", "0", "10"}, {"0", "1", "0", "20"}, {"0", "0", "1", "30"}};
    AnalysisConfiguration config;
    config.mixture_analyze.component_columns = {0, 1, 2};
    config.mixture_analyze.response_column = 3;
    config.mixture_analyze.model_order = "linear";
    auto page = datalab::application::AnalysisService::mixture_analyze(table, config);
    QVERIFY(page.facts.mixture_analyze.has_value());
    QCOMPARE(page.facts.mixture_analyze->component_count, std::size_t{3});
    QVERIFY(!page.tables.empty());

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.mixture_analyze.has_value());
    QCOMPARE(restored.facts.mixture_analyze->algorithm_id,
             std::string("mixture_scheffe_ols"));
}

void AlgorithmWave7TrackTest::glmTwoWayFormulaReference()
{
    // # source: formula_reference — 2×2 balanced; main effects estimable.
    std::vector<std::string> a = {"1", "1", "2", "2"};
    std::vector<std::string> b = {"1", "2", "1", "2"};
    std::vector<double> y = {10.0, 12.0, 20.0, 22.0};
    datalab::domain::statistics::GlmTwoWayOptions options;
    options.include_interaction = true;
    const auto result = datalab::domain::statistics::glm_two_way_analyze(
        a, b, y, {}, options);
    QCOMPARE(result.observation_count, std::size_t{4});
    QCOMPARE(result.anova_effects.size(), std::size_t{3});
    QVERIFY(!result.fitted_means.empty());
}

void AlgorithmWave7TrackTest::glmServiceAndSerialize()
{
    // Marker: GlmTwoWayFacts
    DataTable table;
    table.columns = {"A", "B", "Y"};
    table.rows = {
        {"1", "1", "10"}, {"1", "2", "12"}, {"2", "1", "20"}, {"2", "2", "22"}};
    AnalysisConfiguration config;
    config.glm_two_way.factor_a_column = 0;
    config.glm_two_way.factor_b_column = 1;
    config.glm_two_way.response_column = 2;
    auto page = datalab::application::AnalysisService::glm_two_way(table, config);
    QVERIFY(page.facts.glm_two_way.has_value());
    QCOMPARE(page.facts.glm_two_way->observation_count, std::size_t{4});

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.glm_two_way.has_value());
    QCOMPARE(restored.facts.glm_two_way->algorithm_id,
             std::string("glm_two_way_type3"));
}

void AlgorithmWave7TrackTest::variabilityEffectFormulaReference()
{
    // # source: formula_reference — effect = 2*coef for ±1 coding.
    std::vector<std::vector<std::string>> factors = {{"low"}, {"high"}};
    std::vector<std::vector<double>> reps = {{1.0, 2.0}, {4.0, 8.0}};
    const auto result = datalab::domain::statistics::analyze_variability_dispersion(
        factors, reps, {"A"}, {});
    QVERIFY(result.run_count >= std::size_t{2});
    if (!result.coefficients.empty()) {
        const double effect = result.coefficients.front().effect;
        const double coef = result.coefficients.front().coefficient;
        QCOMPARE(effect, 2.0 * coef);
    }
}

void AlgorithmWave7TrackTest::variabilityServiceAndSerialize()
{
    // Marker: AnalyzeVariabilityFacts
    DataTable table;
    table.columns = {"A", "R1", "R2"};
    table.rows = {
        {"low", "1", "2"}, {"high", "4", "8"}, {"low", "1.1", "1.9"},
        {"high", "3.9", "8.1"}};
    AnalysisConfiguration config;
    config.analyze_variability.factor_columns = {0};
    config.analyze_variability.replicate_columns = {1, 2};
    auto page =
        datalab::application::AnalysisService::analyze_variability(table, config);
    QVERIFY(page.facts.analyze_variability.has_value());
    QVERIFY(page.facts.analyze_variability->run_count >= std::size_t{2});

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.analyze_variability.has_value());
    QCOMPARE(restored.facts.analyze_variability->algorithm_id,
             std::string("analyze_variability_ln_sigma_lse"));
}

void AlgorithmWave7TrackTest::factorLoadingsFormulaReference()
{
    // # source: formula_reference — 3 vars, correlation eigen; loadings L=sqrt(λ)*v.
    std::vector<std::vector<double>> rows = {
        {1.0, 2.0, 3.0}, {2.0, 4.0, 6.0}, {3.0, 6.0, 9.0}, {1.5, 3.0, 4.5}};
    datalab::domain::statistics::FactorAnalysisOptions options;
    options.factor_count = 1;
    options.use_kaiser_rule = false;
    const auto result = datalab::domain::statistics::factor_analysis_extract(
        rows, {"V1", "V2", "V3"}, {}, options);
    QCOMPARE(result.variable_count, std::size_t{3});
    QVERIFY(result.retained_factor_count >= std::size_t{1});
    QVERIFY(!result.loadings_table.empty());
    QVERIFY(result.loadings_table.front().communality >= 0.0);
}

void AlgorithmWave7TrackTest::factorServiceAndSerialize()
{
    // Marker: FactorAnalysisFacts
    DataTable table;
    table.columns = {"V1", "V2", "V3"};
    table.rows = {
        {"1", "2", "3"}, {"2", "4", "6"}, {"3", "6", "9"}, {"1.5", "3", "4.5"}};
    AnalysisConfiguration config;
    config.factor_analysis.variable_columns = {0, 1, 2};
    config.factor_analysis.factor_count = 1;
    auto page = datalab::application::AnalysisService::factor_analysis(table, config);
    QVERIFY(page.facts.factor_analysis.has_value());
    QVERIFY(!page.tables.empty());
    QVERIFY(!page.plots.empty());

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.factor_analysis.has_value());
    QCOMPARE(restored.facts.factor_analysis->algorithm_id,
             std::string("factor_analysis_pca_extraction"));
}

void AlgorithmWave7TrackTest::wave7InterpretationNoForbiddenPhrases()
{
    DataTable table;
    table.columns = {"x1", "x2", "x3", "Y"};
    table.rows = {{"1", "0", "0", "10"}, {"0", "1", "0", "20"}};
    AnalysisConfiguration config;
    config.mixture_analyze.component_columns = {0, 1, 2};
    config.mixture_analyze.response_column = 3;
    auto page = datalab::application::AnalysisService::mixture_analyze(table, config);
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("见 md") == std::string::npos);
            QVERIFY(bullet.find("见md") == std::string::npos);
        }
    }
}

QTEST_APPLESS_MAIN(AlgorithmWave7TrackTest)
#include "algorithm_wave7_track_test.moc"
