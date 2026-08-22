#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/nominal_logistic.h"
#include "domain/statistics/nonparametric_capability.h"
#include "domain/statistics/accelerated_life.h"
#include "domain/statistics/stepwise_regression.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::SpecificationLimits;

class AlgorithmWave2TrackTest final : public QObject {
    Q_OBJECT

private slots:
    void nominalLogisticThreeCategories();
    void nominalLogisticCompleteCaseAndSourceRow();
    void nonparametricCapabilityIndices();
    void nonparametricCapabilityCompleteCaseInvalidatesOnImportChange();
    void stepwiseForwardAiccTableShape();
    void acceleratedLifeArrheniusRegression();
};

void AlgorithmWave2TrackTest::nominalLogisticThreeCategories()
{
    // # source: formula_reference — 3-level nominal; reference = last level.
    std::vector<std::size_t> response = {0, 0, 1, 1, 2, 2, 0, 1, 2, 0,
                                         1, 2, 0, 1, 2, 0, 1, 2, 0, 1};
    std::vector<std::vector<double>> predictors(20, {1.0});
    for (std::size_t i = 0; i < predictors.size(); ++i) {
        predictors[i][0] = static_cast<double>(i) * 0.05;
    }
    const auto result = datalab::domain::statistics::fit_nominal_logistic(
        response, predictors, {"A", "B", "C"}, {"X"});
    QVERIFY(result.observation_count == 20);
    QVERIFY(result.category_count == 3);
    QVERIFY(result.logit_count == 2);
    QVERIFY(!result.coefficients.empty());
}

void AlgorithmWave2TrackTest::nominalLogisticCompleteCaseAndSourceRow()
{
    DataTable table;
    table.columns = {"Y", "X"};
    table.rows.push_back({"A", "0.1"});
    table.rows.push_back({"*", "0.2"});
    table.rows.push_back({"B", "0.3"});
    table.rows.push_back({"C", "0.4"});
    for (int i = 0; i < 12; ++i) {
        table.rows.push_back({i % 3 == 0 ? "A" : (i % 3 == 1 ? "B" : "C"),
                              std::to_string(0.1 * i)});
    }
    AnalysisConfiguration config;
    config.nominal_logistic.response_column = 0;
    config.nominal_logistic.predictor_columns = {1};
    auto page = datalab::application::AnalysisService::nominal_logistic(table, config);
    QVERIFY(page.facts.nominal_logistic.has_value());
    QCOMPARE(page.facts.nominal_logistic->n, std::size_t{14});
}

void AlgorithmWave2TrackTest::nonparametricCapabilityIndices()
{
    // # source: formula_reference — symmetric spread → Cnp ≈ spec/spread.
    std::vector<double> values;
    for (int i = 0; i < 30; ++i) {
        values.push_back(10.0 + 0.01 * i);
    }
    SpecificationLimits specs;
    specs.lower = 9.5;
    specs.upper = 10.5;
    const auto result = datalab::domain::statistics::compute_nonparametric_capability(
        values, {}, specs, 6.0);
    QVERIFY(result.cnp.has_value());
    QVERIFY(result.cnpk.has_value());
    QVERIFY(*result.cnpk > 0.0);
}

void AlgorithmWave2TrackTest::nonparametricCapabilityCompleteCaseInvalidatesOnImportChange()
{
    // A→B: different data file → N changes; excluded row must not carry over.
    DataTable table_a;
    table_a.columns = {"Y"};
    for (int i = 0; i < 12; ++i) {
        table_a.rows.push_back({std::to_string(10.0 + 0.01 * i)});
    }
    AnalysisConfiguration config;
    config.nonparametric_capability.measurement_column = 0;
    config.specifications.lower = 9.0;
    config.specifications.upper = 11.0;
    config.excluded_rows = {0};  // exclude first row on table A
    auto page_a = datalab::application::AnalysisService::nonparametric_capability(
        table_a, config);
    const std::size_t n_a = page_a.facts.nonparametric_capability->n;
    QVERIFY(n_a == 11);

    DataTable table_b;
    table_b.columns = {"Y"};
    for (int i = 0; i < 8; ++i) {
        table_b.rows.push_back({std::to_string(10.5 + 0.02 * i)});
    }
    config.excluded_rows.clear();  // B must not inherit A exclusions
    auto page_b = datalab::application::AnalysisService::nonparametric_capability(
        table_b, config);
    QVERIFY(page_b.facts.nonparametric_capability.has_value());
    QCOMPARE(page_b.facts.nonparametric_capability->n, std::size_t{8});
    QVERIFY(page_b.facts.nonparametric_capability->n != n_a);
    QVERIFY(page_a.id != page_b.id);
}

void AlgorithmWave2TrackTest::stepwiseForwardAiccTableShape()
{
    // # source: formula_reference — forward AICc path records IC columns.
    std::vector<double> y;
    std::vector<std::vector<double>> x;
    for (int i = 0; i < 25; ++i) {
        const double x1 = 0.1 * i;
        const double x2 = ((i * 13) % 7) * 0.02;
        y.push_back(1.0 + 2.0 * x1 + 0.01 * x2);
        x.push_back({x1, x2});
    }
    const auto result = datalab::domain::statistics::fit_stepwise_regression(
        y, x, {"X1", "X2"}, "forward_aicc");
    QVERIFY(result.criterion == "aicc");
    QVERIFY(result.steps.size() >= 2);
    bool has_aicc = false;
    for (const auto& step : result.steps) {
        if (step.aicc.has_value()) {
            has_aicc = true;
        }
    }
    QVERIFY(has_aicc);

    DataTable table;
    table.columns = {"Y", "X1", "X2"};
    for (int i = 0; i < 25; ++i) {
        table.rows.push_back({std::to_string(1.0 + 0.2 * i),
                              std::to_string(0.1 * i),
                              std::to_string(0.01 * i)});
    }
    AnalysisConfiguration config;
    config.stepwise_regression.response_column = 0;
    config.stepwise_regression.predictor_columns = {1, 2};
    config.stepwise_regression.method = "forward_aicc";
    auto page = datalab::application::AnalysisService::stepwise_regression(table, config);
    QVERIFY(page.facts.stepwise_regression.has_value());
    QCOMPARE(page.facts.stepwise_regression->criterion, std::string("aicc"));
    bool table_has_aicc = false;
    for (const auto& tbl : page.tables) {
        if (tbl.title.find("逐步") != std::string::npos) {
            for (const auto& header : tbl.headers) {
                if (header == "AICc") {
                    table_has_aicc = true;
                }
            }
        }
    }
    QVERIFY(table_has_aicc);
}

void AlgorithmWave2TrackTest::acceleratedLifeArrheniusRegression()
{
    // # source: formula_reference — higher stress → shorter life.
    std::vector<double> times = {5000, 5200, 4800, 2000, 2100, 1900, 800, 850, 820, 900};
    std::vector<bool> events = {true, true, true, true, true, true, true, true, true, true};
    std::vector<double> stress = {40, 45, 42, 60, 62, 58, 80, 82, 79, 85};
    const auto result = datalab::domain::statistics::fit_accelerated_life_weibull_arrhenius(
        times, events, stress);
    QVERIFY(result.observation_count == 10);
    QVERIFY(result.stress_level_count >= 2);
    QVERIFY(result.coefficients.size() >= 2);
    QVERIFY(result.shape > 0.0);
}

QTEST_APPLESS_MAIN(AlgorithmWave2TrackTest)
#include "algorithm_wave2_track_test.moc"
