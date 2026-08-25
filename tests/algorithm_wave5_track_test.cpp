#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/distribution_calculator.h"
#include "domain/statistics/random_forest.h"
#include "domain/statistics/taguchi_orthogonal.h"
#include "domain/statistics/weibayes.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class AlgorithmWave5TrackTest final : public QObject {
    Q_OBJECT

private slots:
    void randomForestDomainAndDisclosure();
    void randomForestServiceAndSerialize();
    void randomForestCompleteCaseGate();
    void weibayesDomainMainPath();
    void weibayesZeroFailureHonesty();
    void weibayesServiceAndSerialize();
    void taguchiL8WorksheetExport();
    void taguchiServiceAndSerialize();
    void taguchiFactorCapGate();
    void distCalcNormalCdfHalf();
    void distCalcServiceAndSerialize();
    void distCalcScopeNoGofClaim();
    void wave5InterpretationNoForbiddenPhrases();
};

void AlgorithmWave5TrackTest::randomForestDomainAndDisclosure()
{
    // # source: formula_reference — separable 2-class bagging CART.
    std::vector<std::vector<double>> x = {
        {0, 0}, {0, 1}, {1, 0}, {1, 1}, {0.1, 0.1}, {0.2, 0.8}, {0.9, 0.1}, {0.8, 0.9}};
    std::vector<double> y = {0, 0, 1, 1, 0, 0, 1, 1};
    const auto result = datalab::domain::statistics::fit_random_forest(
        x, y, {"A", "B"}, {"x1", "x2"},
        {datalab::domain::statistics::CartTask::classification, 20, 3, 1, 7, true});
    QVERIFY(result.observation_count >= 8);
    QVERIFY(result.n_trees > 0);
    QVERIFY(!result.variable_importance.empty());
    QVERIFY(result.disclosure.find("NOT TreeNet") != std::string::npos
            || result.disclosure.find("TreeNet") != std::string::npos);
}

void AlgorithmWave5TrackTest::randomForestServiceAndSerialize()
{
    // Marker: RandomForestFacts + disclosure presence.
    DataTable table;
    table.columns = {"y", "x1", "x2"};
    table.rows = {
        {"A", "0", "0"}, {"A", "0", "1"}, {"B", "1", "0"}, {"B", "1", "1"},
        {"A", "0.1", "0.2"}, {"B", "0.9", "0.8"}, {"A", "0.2", "0.1"}, {"B", "0.8", "0.9"}};
    AnalysisConfiguration config;
    config.random_forest.response_column = 0;
    config.random_forest.predictor_columns = {1, 2};
    config.random_forest.n_trees = 15;
    config.random_forest.seed = 3;
    auto page = datalab::application::AnalysisService::random_forest(table, config);
    QVERIFY(page.facts.random_forest.has_value());
    QVERIFY(page.facts.random_forest->disclosure.find("TreeNet") != std::string::npos
            || page.facts.random_forest->disclosure.find("Minitab") != std::string::npos);
    bool has_importance = false;
    for (const auto& t : page.tables) {
        if (t.title == "Variable Importance") {
            has_importance = true;
        }
    }
    QVERIFY(has_importance);

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.random_forest.has_value());
    QCOMPARE(restored.facts.random_forest->algorithm_id,
             std::string("bagged_cart_random_forest"));
}

void AlgorithmWave5TrackTest::randomForestCompleteCaseGate()
{
    // # source: formula_reference — missing ≠ 0; complete-case drops incomplete row.
    DataTable table;
    table.columns = {"y", "x1"};
    table.rows = {{"A", "1"}, {"B", ""}, {"A", "2"}, {"B", "3"}, {"A", "1.5"}, {"B", "2.5"}};
    AnalysisConfiguration config;
    config.random_forest.response_column = 0;
    config.random_forest.predictor_columns = {1};
    config.random_forest.n_trees = 10;
    config.random_forest.min_leaf = 1;
    auto page = datalab::application::AnalysisService::random_forest(table, config);
    QVERIFY(page.facts.random_forest.has_value());
    QCOMPARE(page.facts.random_forest->n, std::size_t{5});
}

void AlgorithmWave5TrackTest::weibayesDomainMainPath()
{
    // # source: formula_reference — fixed β=1 → η = Σt / r for exponential case.
    std::vector<double> times = {10, 20, 30, 40};
    std::vector<bool> events = {true, true, false, true};
    const auto result = datalab::domain::statistics::fit_weibayes(
        times, events, {0, 1, 2, 3}, {1.0});
    QCOMPARE(result.failure_count, std::size_t{3});
    QVERIFY(result.scale.has_value());
    QVERIFY(*result.scale > 0.0);
    QCOMPARE(result.percentiles.size(), std::size_t{3});
}

void AlgorithmWave5TrackTest::weibayesZeroFailureHonesty()
{
    // # source: formula_reference — r=0 honesty path, no η claim.
    std::vector<double> times = {10, 20, 30};
    std::vector<bool> events = {false, false, false};
    const auto result = datalab::domain::statistics::fit_weibayes(times, events, {}, {2.0});
    QVERIFY(result.zero_failure_bound);
    QVERIFY(!result.scale.has_value());
    QVERIFY(result.percentiles.empty());
}

void AlgorithmWave5TrackTest::weibayesServiceAndSerialize()
{
    // Marker: WeibayesFacts serialize.
    DataTable table;
    table.columns = {"Time", "Event"};
    table.rows = {{"10", "1"}, {"20", "0"}, {"30", "1"}, {"40", "1"}};
    AnalysisConfiguration config;
    config.weibayes.time_column = 0;
    config.weibayes.event_column = 1;
    config.weibayes.shape_prior = 2.0;
    auto page = datalab::application::AnalysisService::weibayes(table, config);
    QVERIFY(page.facts.weibayes.has_value());
    QCOMPARE(page.facts.weibayes->failure_count, std::size_t{3});
    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.weibayes.has_value());
    QCOMPARE(restored.facts.weibayes->algorithm_id, std::string("weibayes_fixed_shape"));
}

void AlgorithmWave5TrackTest::taguchiL8WorksheetExport()
{
    // # source: formula_reference — L8 has 8 runs; export clears prior excludes.
    AnalysisConfiguration config;
    config.excluded_rows = {99};
    config.hidden_rows = {88};
    config.taguchi_orthogonal.array = "L8";
    config.taguchi_orthogonal.factor_names = {"A", "B", "C"};
    config.taguchi_orthogonal.low_levels = {"-1", "-1", "-1"};
    config.taguchi_orthogonal.high_levels = {"+1", "+1", "+1"};
    DataTable unused;
    auto page = datalab::application::AnalysisService::taguchi_orthogonal_design(
        unused, config);
    QVERIFY(page.facts.taguchi_orthogonal.has_value());
    QCOMPARE(page.facts.taguchi_orthogonal->array, std::string("L8"));
    QCOMPARE(page.facts.taguchi_orthogonal->run_count, std::size_t{8});
    QVERIFY(page.worksheet_export.has_value());
    QCOMPARE(page.worksheet_export->rows.size(), std::size_t{8});
    QVERIFY(page.configuration.excluded_rows.empty());
    QVERIFY(page.configuration.hidden_rows.empty());
}

void AlgorithmWave5TrackTest::taguchiServiceAndSerialize()
{
    // Marker: TaguchiOrthogonalFacts round-trip.
    AnalysisConfiguration config;
    config.taguchi_orthogonal.array = "L9";
    config.taguchi_orthogonal.factor_names = {"A", "B"};
    config.taguchi_orthogonal.low_levels = {"1", "1"};
    config.taguchi_orthogonal.mid_levels = {"2", "2"};
    config.taguchi_orthogonal.high_levels = {"3", "3"};
    DataTable unused;
    auto page = datalab::application::AnalysisService::taguchi_orthogonal_design(
        unused, config);
    QVERIFY(page.facts.taguchi_orthogonal.has_value());
    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.taguchi_orthogonal.has_value());
    QCOMPARE(restored.facts.taguchi_orthogonal->run_count, std::size_t{9});
}

void AlgorithmWave5TrackTest::taguchiFactorCapGate()
{
    datalab::domain::statistics::TaguchiOrthogonalOptions options;
    options.array = datalab::domain::statistics::TaguchiArray::L8;
    for (int i = 0; i < 10; ++i) {
        datalab::domain::statistics::DoeFactor f;
        f.name = "F" + std::to_string(i);
        f.low_level = "-1";
        f.high_level = "+1";
        options.factors.push_back(f);
    }
    const auto design = datalab::domain::statistics::generate_taguchi_orthogonal(options);
    QCOMPARE(design.factor_count, std::size_t{7});
}

void AlgorithmWave5TrackTest::distCalcNormalCdfHalf()
{
    // # source: formula_reference — standard normal CDF(0) ≈ 0.5.
    datalab::domain::statistics::DistributionCalculatorOptions options;
    options.distribution = datalab::domain::statistics::DistCalcDistribution::normal;
    options.operation = datalab::domain::statistics::DistCalcOperation::cdf;
    options.param1 = 0.0;
    options.param2 = 1.0;
    options.value = 0.0;
    const auto result =
        datalab::domain::statistics::evaluate_distribution_calculator(options);
    QVERIFY(result.result.has_value());
    QVERIFY(std::abs(*result.result - 0.5) < 1.0e-6);
}

void AlgorithmWave5TrackTest::distCalcServiceAndSerialize()
{
    // Marker: DistributionCalculatorFacts + normal CDF(0)≈0.5 round-trip.
    AnalysisConfiguration config;
    config.distribution_calculator.distribution = "normal";
    config.distribution_calculator.operation = "cdf";
    config.distribution_calculator.param1 = 0.0;
    config.distribution_calculator.param2 = 1.0;
    config.distribution_calculator.value = 0.0;
    DataTable unused;
    auto page = datalab::application::AnalysisService::distribution_calculator(
        unused, config);
    QVERIFY(page.facts.distribution_calculator.has_value());
    QVERIFY(page.facts.distribution_calculator->result.has_value());
    QVERIFY(std::abs(*page.facts.distribution_calculator->result - 0.5) < 1.0e-6);
    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.distribution_calculator.has_value());
    QVERIFY(restored.facts.distribution_calculator->result.has_value());
    QVERIFY(std::abs(*restored.facts.distribution_calculator->result - 0.5) < 1.0e-6);
}

void AlgorithmWave5TrackTest::distCalcScopeNoGofClaim()
{
    AnalysisConfiguration config;
    config.distribution_calculator.distribution = "weibull";
    config.distribution_calculator.operation = "pdf";
    config.distribution_calculator.param1 = 2.0;
    config.distribution_calculator.param2 = 100.0;
    config.distribution_calculator.value = 50.0;
    DataTable unused;
    auto page = datalab::application::AnalysisService::distribution_calculator(
        unused, config);
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const std::string& bullet : section.bullets) {
            QVERIFY(bullet.find("分布已正态") == std::string::npos);
        }
    }
}

void AlgorithmWave5TrackTest::wave5InterpretationNoForbiddenPhrases()
{
    datalab::domain::OutputPage page;
    page.method_name = "Random Forest";
    datalab::domain::RandomForestFacts rf;
    rf.n = 10;
    rf.n_trees = 20;
    rf.disclosure = "NOT TreeNet / Minitab Random Forests aligned.";
    page.facts.random_forest = rf;
    datalab::domain::WeibayesFacts wb;
    wb.n = 5;
    wb.failure_count = 0;
    wb.zero_failure_bound = true;
    page.facts.weibayes = wb;
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const std::string& bullet : section.bullets) {
            QVERIFY(bullet.find("过程合格") == std::string::npos);
            QVERIFY(bullet.find("寿命已达标") == std::string::npos);
            QVERIFY(bullet.find("分布已正态") == std::string::npos);
            QVERIFY(bullet.find("Minitab RF对齐") == std::string::npos
                    || bullet.find("禁止") != std::string::npos);
        }
    }
}

QTEST_APPLESS_MAIN(AlgorithmWave5TrackTest)
#include "algorithm_wave5_track_test.moc"
