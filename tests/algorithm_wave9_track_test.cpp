#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/expanded_gage_unbalanced.h"
#include "domain/statistics/split_plot_analyze.h"
#include "domain/statistics/mixture_process_variable.h"
#include "domain/statistics/manova_one_way.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class AlgorithmWave9TrackTest final : public QObject {
    Q_OBJECT

private slots:
    void expandedGageFormulaReference();
    void expandedGageServiceAndSerialize();
    void splitPlotFormulaReference();
    void splitPlotServiceAndSerialize();
    void mixtureProcessFormulaReference();
    void mixtureProcessServiceAndSerialize();
    void manovaFormulaReference();
    void manovaServiceAndSerialize();
    void wave9InterpretationNoForbiddenPhrases();
};

void AlgorithmWave9TrackTest::expandedGageFormulaReference()
{
    // # source: formula_reference — unbalanced Part×Operator VarComp.
    std::vector<double> m = {10.1, 10.2, 10.0, 10.3, 11.0, 11.1, 10.9, 11.2, 12.0};
    std::vector<std::string> parts = {"P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"};
    std::vector<std::string> ops = {"O1", "O1", "O2", "O1", "O2", "O2", "O1", "O2", "O2"};
    const auto result = datalab::domain::statistics::expanded_gage_unbalanced_analyze(
        m, parts, ops, {}, 0.0, {}, {});
    QVERIFY(result.observation_count >= std::size_t{6});
    QVERIFY(!result.variance_components.empty());
    QVERIFY(result.part_count >= std::size_t{2});
}

void AlgorithmWave9TrackTest::expandedGageServiceAndSerialize()
{
    // Marker: ExpandedGageUnbalancedFacts
    DataTable table;
    table.columns = {"Part", "Operator", "Measure"};
    table.rows = {
        {"P1", "O1", "10.1"}, {"P1", "O1", "10.2"}, {"P1", "O2", "10.0"},
        {"P2", "O1", "11.0"}, {"P2", "O2", "11.1"}, {"P2", "O2", "10.9"}};
    AnalysisConfiguration config;
    config.expanded_gage_unbalanced.measurement_column = 2;
    config.expanded_gage_unbalanced.part_column = 0;
    config.expanded_gage_unbalanced.operator_column = 1;
    auto page = datalab::application::AnalysisService::expanded_gage_unbalanced(table, config);
    QVERIFY(page.facts.expanded_gage_unbalanced.has_value());
    QVERIFY(!page.tables.empty());

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.expanded_gage_unbalanced.has_value());
    QCOMPARE(restored.facts.expanded_gage_unbalanced->algorithm_id,
             std::string("expanded_gage_unbalanced_glm_varcomp"));
}

void AlgorithmWave9TrackTest::splitPlotFormulaReference()
{
    // # source: formula_reference — HTC F uses WP error denominator.
    std::vector<double> y = {10.0, 11.0, 12.0, 13.0, 20.0, 21.0, 22.0, 23.0};
    std::vector<std::string> htc = {"L", "L", "L", "L", "H", "H", "H", "H"};
    std::vector<std::string> etc = {"A", "A", "B", "B", "A", "A", "B", "B"};
    std::vector<std::string> wp = {"WP1", "WP1", "WP1", "WP1", "WP2", "WP2", "WP2", "WP2"};
    const auto result = datalab::domain::statistics::split_plot_analyze(
        y, htc, etc, wp, {}, {}, {});
    QCOMPARE(result.observation_count, std::size_t{8});
    QVERIFY(result.whole_plot_count >= std::size_t{2});
    bool has_wp_error = false;
    for (const auto& effect : result.anova_effects) {
        if (effect.term == "WP Error") {
            has_wp_error = true;
        }
    }
    QVERIFY(has_wp_error);
}

void AlgorithmWave9TrackTest::splitPlotServiceAndSerialize()
{
    // Marker: SplitPlotAnalyzeFacts
    DataTable table;
    table.columns = {"Y", "HTC", "ETC", "WP"};
    table.rows = {
        {"10", "L", "A", "WP1"}, {"11", "L", "A", "WP1"},
        {"12", "L", "B", "WP1"}, {"13", "L", "B", "WP1"},
        {"20", "H", "A", "WP2"}, {"21", "H", "A", "WP2"},
        {"22", "H", "B", "WP2"}, {"23", "H", "B", "WP2"}};
    AnalysisConfiguration config;
    config.split_plot_analyze.response_column = 0;
    config.split_plot_analyze.htc_factor_column = 1;
    config.split_plot_analyze.etc_factor_a_column = 2;
    config.split_plot_analyze.whole_plot_column = 3;
    auto page = datalab::application::AnalysisService::split_plot_analyze(table, config);
    QVERIFY(page.facts.split_plot_analyze.has_value());
    QCOMPARE(page.facts.split_plot_analyze->observation_count, std::size_t{8});

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.split_plot_analyze.has_value());
    QCOMPARE(restored.facts.split_plot_analyze->algorithm_id,
             std::string("split_plot_analyze_wp_sp"));
}

void AlgorithmWave9TrackTest::mixtureProcessFormulaReference()
{
    // # source: formula_reference — Scheffé + process OLS no intercept.
    std::vector<std::vector<double>> components = {
        {0.5, 0.5}, {0.6, 0.4}, {0.4, 0.6}, {0.55, 0.45}};
    std::vector<double> process = {1.0, 2.0, 1.5, 2.5};
    std::vector<double> response = {10.0, 12.0, 11.0, 13.0};
    datalab::domain::statistics::MixtureProcessVariableOptions options;
    options.include_component_process_interaction = false;
    const auto result = datalab::domain::statistics::analyze_mixture_process_variable(
        components, process, response, {"A", "B"}, {}, options);
    QCOMPARE(result.observation_count, std::size_t{4});
    QVERIFY(!result.coefficients.empty());
}

void AlgorithmWave9TrackTest::mixtureProcessServiceAndSerialize()
{
    // Marker: MixtureProcessVariableFacts
    DataTable table;
    table.columns = {"A", "B", "X1", "Y"};
    table.rows = {
        {"0.5", "0.5", "1", "10"}, {"0.6", "0.4", "2", "12"},
        {"0.4", "0.6", "1.5", "11"}, {"0.55", "0.45", "2.5", "13"}};
    AnalysisConfiguration config;
    config.mixture_process_variable.component_columns = {0, 1};
    config.mixture_process_variable.response_column = 3;
    config.mixture_process_variable.process_column = 2;
    auto page =
        datalab::application::AnalysisService::mixture_process_variable(table, config);
    QVERIFY(page.facts.mixture_process_variable.has_value());
    QVERIFY(!page.tables.empty());

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.mixture_process_variable.has_value());
    QCOMPARE(restored.facts.mixture_process_variable->algorithm_id,
             std::string("mixture_process_variable_scheffe_ols"));
}

void AlgorithmWave9TrackTest::manovaFormulaReference()
{
    // # source: formula_reference — 2 responses 3 groups Wilks computable.
    std::vector<std::vector<double>> responses = {
        {1.0, 2.0}, {1.1, 2.1}, {1.2, 2.0},
        {2.0, 3.0}, {2.1, 3.1}, {2.2, 3.0},
        {3.0, 4.0}, {3.1, 4.1}, {3.2, 4.0}};
    std::vector<std::string> factor = {
        "G1", "G1", "G1", "G2", "G2", "G2", "G3", "G3", "G3"};
    const auto result = datalab::domain::statistics::manova_one_way_analyze(
        responses, factor, {}, {});
    QCOMPARE(result.group_count, std::size_t{3});
    QCOMPARE(result.response_count, std::size_t{2});
    QVERIFY(!result.test_rows.empty());
}

void AlgorithmWave9TrackTest::manovaServiceAndSerialize()
{
    // Marker: ManovaOneWayFacts
    DataTable table;
    table.columns = {"Y1", "Y2", "Group"};
    table.rows = {
        {"1", "2", "G1"}, {"1.1", "2.1", "G1"}, {"1.2", "2", "G1"},
        {"2", "3", "G2"}, {"2.1", "3.1", "G2"}, {"2.2", "3", "G2"},
        {"3", "4", "G3"}, {"3.1", "4.1", "G3"}, {"3.2", "4", "G3"}};
    AnalysisConfiguration config;
    config.manova_one_way.response_columns = {0, 1};
    config.manova_one_way.factor_column = 2;
    auto page = datalab::application::AnalysisService::manova_one_way(table, config);
    QVERIFY(page.facts.manova_one_way.has_value());
    QCOMPARE(page.facts.manova_one_way->group_count, std::size_t{3});

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.manova_one_way.has_value());
    QCOMPARE(restored.facts.manova_one_way->algorithm_id,
             std::string("manova_one_way_multivariate"));
}

void AlgorithmWave9TrackTest::wave9InterpretationNoForbiddenPhrases()
{
    DataTable table;
    table.columns = {"Y1", "Y2", "Group"};
    table.rows = {
        {"1", "2", "G1"}, {"1.1", "2.1", "G1"}, {"1.2", "2", "G1"},
        {"2", "3", "G2"}, {"2.1", "3.1", "G2"}, {"2.2", "3", "G2"}};
    AnalysisConfiguration config;
    config.manova_one_way.response_columns = {0, 1};
    config.manova_one_way.factor_column = 2;
    auto page = datalab::application::AnalysisService::manova_one_way(table, config);
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

QTEST_APPLESS_MAIN(AlgorithmWave9TrackTest)
#include "algorithm_wave9_track_test.moc"
