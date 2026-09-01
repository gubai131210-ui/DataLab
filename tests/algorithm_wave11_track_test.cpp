#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/simple_correspondence.h"
#include "domain/statistics/multiple_correspondence.h"
#include "domain/statistics/nonlinear_regression.h"
#include "domain/statistics/split_plot_design.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class AlgorithmWave11TrackTest final : public QObject {
    Q_OBJECT

private slots:
    void simpleCorrespondenceFormulaReference();
    void simpleCorrespondenceServiceAndSerialize();
    void simpleCorrespondenceEmptyTableError();
    void multipleCorrespondenceFormulaReference();
    void multipleCorrespondenceServiceAndSerialize();
    void multipleCorrespondenceVariableCountGate();
    void nonlinearRegressionFormulaReference();
    void nonlinearRegressionServiceAndSerialize();
    void nonlinearRegressionInvalidStartDiagnostic();
    void splitPlotDesignFormulaReference();
    void splitPlotDesignServiceAndSerialize();
    void splitPlotDesignWholePlotMonotone();
    void wave11InterpretationNoForbiddenPhrases();
};

void AlgorithmWave11TrackTest::simpleCorrespondenceFormulaReference()
{
    // # source: formula_reference — 2×3 contingency inertia I = chi2/n.
    std::vector<std::string> row = {"R1", "R1", "R1", "R2", "R2", "R2"};
    std::vector<std::string> col = {"C1", "C2", "C3", "C1", "C2", "C3"};
    const auto result = datalab::domain::statistics::simple_correspondence_analyze(
        row, col, {}, {});
    QCOMPARE(result.observation_count, std::size_t{6});
    QCOMPARE(result.row_level_count, std::size_t{2});
    QCOMPARE(result.column_level_count, std::size_t{3});
    QVERIFY(result.total_inertia >= 0.0);
    QVERIFY(std::abs(result.total_inertia - result.chi_square / 6.0) < 1.0e-9);
}

void AlgorithmWave11TrackTest::simpleCorrespondenceServiceAndSerialize()
{
    // Marker: SimpleCorrespondenceFacts
    DataTable table;
    table.columns = {"Row", "Col"};
    table.rows = {
        {"R1", "C1"}, {"R1", "C2"}, {"R2", "C1"},
        {"R2", "C2"}, {"R1", "C1"}, {"R2", "C2"}};
    AnalysisConfiguration config;
    config.simple_correspondence.row_variable_column = 0;
    config.simple_correspondence.column_variable_column = 1;
    auto page = datalab::application::AnalysisService::simple_correspondence(table, config);
    QVERIFY(page.facts.simple_correspondence.has_value());
    QCOMPARE(page.facts.simple_correspondence->observation_count, std::size_t{6});

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.simple_correspondence.has_value());
    QCOMPARE(restored.facts.simple_correspondence->algorithm_id,
             std::string("simple_correspondence_svd"));
}

void AlgorithmWave11TrackTest::simpleCorrespondenceEmptyTableError()
{
    std::vector<std::string> row;
    std::vector<std::string> col;
    const auto result = datalab::domain::statistics::simple_correspondence_analyze(
        row, col, {}, {});
    QVERIFY(!result.diagnostics.empty());
}

void AlgorithmWave11TrackTest::multipleCorrespondenceFormulaReference()
{
    // # source: formula_reference — indicator matrix MCA 4 variables.
    std::vector<std::vector<std::string>> columns = {
        {"A", "A", "B", "B"},
        {"X", "Y", "X", "Y"},
        {"L", "H", "L", "H"}};
    const auto result = datalab::domain::statistics::multiple_correspondence_analyze(
        columns, {}, {});
    QCOMPARE(result.observation_count, std::size_t{4});
    QCOMPARE(result.variable_count, std::size_t{3});
    QVERIFY(result.category_count >= std::size_t{6});
    QVERIFY(!result.column_contributions.empty());
}

void AlgorithmWave11TrackTest::multipleCorrespondenceServiceAndSerialize()
{
    // Marker: MultipleCorrespondenceFacts
    DataTable table;
    table.columns = {"V1", "V2", "V3"};
    table.rows = {
        {"A", "X", "L"}, {"A", "Y", "H"},
        {"B", "X", "L"}, {"B", "Y", "H"}};
    AnalysisConfiguration config;
    config.multiple_correspondence.categorical_columns = {0, 1, 2};
    auto page = datalab::application::AnalysisService::multiple_correspondence(table, config);
    QVERIFY(page.facts.multiple_correspondence.has_value());
    QCOMPARE(page.facts.multiple_correspondence->variable_count, std::size_t{3});

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.multiple_correspondence.has_value());
    QCOMPARE(restored.facts.multiple_correspondence->algorithm_id,
             std::string("multiple_correspondence_burt"));
}

void AlgorithmWave11TrackTest::multipleCorrespondenceVariableCountGate()
{
    DataTable table;
    table.columns = {"V1", "V2"};
    table.rows = {{"A", "X"}, {"B", "Y"}};
    AnalysisConfiguration config;
    config.multiple_correspondence.categorical_columns = {0, 1};
    auto page = datalab::application::AnalysisService::multiple_correspondence(table, config);
    QVERIFY(!page.diagnostics.empty() || page.tables.empty());
}

void AlgorithmWave11TrackTest::nonlinearRegressionFormulaReference()
{
    // # source: formula_reference — growth model GN convergence.
    std::vector<double> x = {1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<double> y = {2.0, 4.5, 6.8, 8.2, 9.0};
    datalab::domain::statistics::NonlinearRegressionOptions options;
    options.model_id = "growth";
    options.algorithm = "gn";
    options.starting_values = {10.0, 8.0, 0.5};
    const auto result = datalab::domain::statistics::nonlinear_regression_fit(
        y, x, {}, options);
    QCOMPARE(result.observation_count, std::size_t{5});
    QVERIFY(result.iteration_count > 0);
    QVERIFY(!result.parameters.empty());
}

void AlgorithmWave11TrackTest::nonlinearRegressionServiceAndSerialize()
{
    // Marker: NonlinearRegressionFacts
    DataTable table;
    table.columns = {"Y", "X"};
    table.rows = {
        {"2", "1"}, {"4.5", "2"}, {"6.8", "3"}, {"8.2", "4"}, {"9", "5"}};
    AnalysisConfiguration config;
    config.nonlinear_regression.response_column = 0;
    config.nonlinear_regression.predictor_column = 1;
    config.nonlinear_regression.model_id = "growth";
    auto page = datalab::application::AnalysisService::nonlinear_regression(table, config);
    QVERIFY(page.facts.nonlinear_regression.has_value());
    QCOMPARE(page.facts.nonlinear_regression->model_id, std::string("growth"));

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.nonlinear_regression.has_value());
    QCOMPARE(restored.facts.nonlinear_regression->algorithm_id,
             std::string("nonlinear_regression_gn_lm"));
}

void AlgorithmWave11TrackTest::nonlinearRegressionInvalidStartDiagnostic()
{
    std::vector<double> x = {1.0, 2.0, 3.0};
    std::vector<double> y = {1.0, 2.0, 3.0};
    datalab::domain::statistics::NonlinearRegressionOptions options;
    options.model_id = "growth";
    options.starting_values = {1.0};
    const auto result = datalab::domain::statistics::nonlinear_regression_fit(
        y, x, {}, options);
    QVERIFY(!result.diagnostics.empty());
}

void AlgorithmWave11TrackTest::splitPlotDesignFormulaReference()
{
    // # source: formula_reference — 4 factors 1 HTC 32 runs toy.
    datalab::domain::statistics::SplitPlotDesignOptions options;
    for (const char* name : {"A", "B", "C", "D"}) {
        datalab::domain::statistics::DoeFactor factor;
        factor.name = name;
        factor.low_level = "-";
        factor.high_level = "+";
        options.factors.push_back(factor);
    }
    options.htc_factor_index = 0;
    options.whole_plot_replicates = 2;
    options.randomize = false;
    const auto result = datalab::domain::statistics::generate_split_plot_design(options);
    QCOMPARE(result.factor_count, std::size_t{4});
    QCOMPARE(result.run_count, std::size_t{32});
    QCOMPARE(result.whole_plot_count, std::size_t{4});
}

void AlgorithmWave11TrackTest::splitPlotDesignServiceAndSerialize()
{
    // Marker: SplitPlotDesignFacts
    DataTable table;
    AnalysisConfiguration config;
    config.split_plot_design.factor_names = {"A", "B", "C"};
    config.split_plot_design.low_levels = {"-", "-", "-"};
    config.split_plot_design.high_levels = {"+", "+", "+"};
    config.split_plot_design.htc_factor_index = 0;
    auto page = datalab::application::AnalysisService::split_plot_design(table, config);
    QVERIFY(page.facts.split_plot_design.has_value());
    QVERIFY(page.facts.split_plot_design->run_count >= std::size_t{8});

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.split_plot_design.has_value());
    QCOMPARE(restored.facts.split_plot_design->algorithm_id,
             std::string("split_plot_design_2level"));
}

void AlgorithmWave11TrackTest::splitPlotDesignWholePlotMonotone()
{
    datalab::domain::statistics::SplitPlotDesignOptions options;
    for (const char* name : {"A", "B"}) {
        datalab::domain::statistics::DoeFactor factor;
        factor.name = name;
        factor.low_level = "-";
        factor.high_level = "+";
        options.factors.push_back(factor);
    }
    options.randomize = false;
    const auto result = datalab::domain::statistics::generate_split_plot_design(options);
    std::size_t prev_wp = 0;
    for (const auto& run : result.runs) {
        QVERIFY(run.whole_plot >= prev_wp || run.whole_plot == 1);
        prev_wp = run.whole_plot;
    }
}

void AlgorithmWave11TrackTest::wave11InterpretationNoForbiddenPhrases()
{
    DataTable table;
    table.columns = {"Row", "Col"};
    table.rows = {{"R1", "C1"}, {"R2", "C2"}, {"R1", "C2"}, {"R2", "C1"}};
    AnalysisConfiguration config;
    config.simple_correspondence.row_variable_column = 0;
    config.simple_correspondence.column_variable_column = 1;
    auto page = datalab::application::AnalysisService::simple_correspondence(table, config);
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("见 md") == std::string::npos);
            QVERIFY(bullet.find("见md") == std::string::npos);
        }
    }
}

QTEST_APPLESS_MAIN(AlgorithmWave11TrackTest)
#include "algorithm_wave11_track_test.moc"
