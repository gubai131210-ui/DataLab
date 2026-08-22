#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/cox_regression.h"
#include "domain/statistics/gray_test.h"
#include "domain/statistics/logistic_regression.h"
#include "domain/statistics/nonparametric_capability.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::SpecificationLimits;
using datalab::domain::statistics::CensoringObservation;
using datalab::domain::statistics::CensoringType;

class AlgorithmWave4TrackTest final : public QObject {
    Q_OBJECT

private slots:
    void nonparametricHistogramAndPpmDomain();
    void nonparametricServiceAndSerialize();
    void coxRegressionDomain();
    void coxRegressionServiceAndSerialize();
    void grayTestCifDomain();
    void logisticStepwiseDomain();
    void wave4InterpretationNoForbiddenPhrases();
};

void AlgorithmWave4TrackTest::nonparametricHistogramAndPpmDomain()
{
    // # source: formula_reference — symmetric spread inside specs.
    std::vector<double> values;
    for (int i = 0; i < 20; ++i) {
        values.push_back(9.5 + static_cast<double>(i) * 0.1);
    }
    std::vector<std::size_t> source_rows(values.size());
    for (std::size_t i = 0; i < source_rows.size(); ++i) {
        source_rows[i] = i;
    }
    SpecificationLimits specs;
    specs.lower = 9.0;
    specs.upper = 11.0;
    const auto result = datalab::domain::statistics::compute_nonparametric_capability(
        values, source_rows, specs, 6.0);
    QCOMPARE(result.sample_size, std::size_t{20});
    QVERIFY(result.cnpl.has_value());
    QVERIFY(result.cnpu.has_value());
    QVERIFY(result.observed_ppm_total.has_value());
    QVERIFY(*result.observed_ppm_total >= 0.0);
}

void AlgorithmWave4TrackTest::nonparametricServiceAndSerialize()
{
    DataTable table;
    table.columns = {"Measure"};
    for (int i = 0; i < 12; ++i) {
        table.rows.push_back({std::to_string(10.0 + i * 0.05)});
    }
    AnalysisConfiguration config;
    config.nonparametric_capability.measurement_column = 0;
    config.specifications.lower = 9.5;
    config.specifications.upper = 11.5;
    auto page_a = datalab::application::AnalysisService::nonparametric_capability(table, config);
    QVERIFY(page_a.facts.nonparametric_capability.has_value());
    const std::size_t n_a = page_a.facts.nonparametric_capability->n;
    bool has_histogram = false;
    for (const auto& plot : page_a.plots) {
        if (plot.title == "Capability Histogram") {
            has_histogram = true;
            QVERIFY(plot.lsl.has_value());
            QVERIFY(plot.usl.has_value());
        }
    }
    QVERIFY(has_histogram);

    table.rows.push_back({"20.0"});
    auto page_b = datalab::application::AnalysisService::nonparametric_capability(table, config);
    QVERIFY(page_b.facts.nonparametric_capability.has_value());
    QVERIFY(page_b.facts.nonparametric_capability->n != n_a);

    const auto serialized = datalab::infrastructure::serialize_output_page(page_b);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.nonparametric_capability.has_value());
    QVERIFY(restored.facts.nonparametric_capability->observed_ppm_total.has_value());
}

void AlgorithmWave4TrackTest::coxRegressionDomain()
{
    // # source: formula_reference — single covariate monotone hazard.
    std::vector<double> times = {1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<bool> events = {true, false, true, false, true, false, true, false};
    std::vector<std::vector<double>> covariates = {
        {0}, {0}, {1}, {1}, {1}, {0}, {1}, {0}};
    const auto result = datalab::domain::statistics::fit_cox_regression(
        times, events, covariates, {"x"}, {}, 0.95, "breslow", 50, 1.0e-6);
    QCOMPARE(result.n, std::size_t{8});
    QCOMPARE(result.events, std::size_t{4});
    QCOMPARE(result.censored, std::size_t{4});
    QVERIFY(!result.coefficients.empty());
}

void AlgorithmWave4TrackTest::coxRegressionServiceAndSerialize()
{
    DataTable table;
    table.columns = {"Time", "Event", "X"};
    table.rows.push_back({"1", "1", "0"});
    table.rows.push_back({"2", "0", "0"});
    table.rows.push_back({"3", "1", "1"});
    table.rows.push_back({"4", "0", "1"});
    table.rows.push_back({"5", "1", "1"});
    table.rows.push_back({"6", "0", "0"});
    AnalysisConfiguration config;
    config.cox_regression.time_column = 0;
    config.cox_regression.event_column = 1;
    config.cox_regression.covariate_columns = {2};
    auto page = datalab::application::AnalysisService::cox_regression(table, config);
    QVERIFY(page.facts.cox_regression.has_value());
    QCOMPARE(page.facts.cox_regression->n, std::size_t{6});

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.cox_regression.has_value());
    QCOMPARE(restored.facts.cox_regression->algorithm_id,
             std::string("cox_ph_fixed_covariates"));
}

void AlgorithmWave4TrackTest::grayTestCifDomain()
{
    std::vector<CensoringObservation> observations;
    observations.push_back({CensoringType::exact, 1.0, 0, 0, {}, "A", "Mode1", {}, 0});
    observations.push_back({CensoringType::exact, 2.0, 0, 0, {}, "A", "Mode2", {}, 1});
    observations.push_back({CensoringType::exact, 1.5, 0, 0, {}, "B", "Mode1", {}, 2});
    observations.push_back({CensoringType::exact, 2.5, 0, 0, {}, "B", "Mode2", {}, 3});
    const auto gray = datalab::domain::statistics::gray_test_cif(observations);
    QVERIFY(gray.ran);
    QVERIFY(gray.chi_square.has_value());
    QVERIFY(gray.p_value.has_value());
    QCOMPARE(gray.group_count, std::size_t{2});
}

void AlgorithmWave4TrackTest::logisticStepwiseDomain()
{
    std::vector<int> response = {0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1};
    std::vector<std::vector<double>> predictors = {
        {1, 0}, {1, 1}, {0, 1}, {1, 1}, {2, 0}, {2, 1},
        {0, 0}, {2, 1}, {1, 0}, {2, 1}, {0, 1}, {1, 1}};
    const auto stepwise = datalab::domain::statistics::fit_logistic_stepwise(
        response, predictors, {"x1", "x2"}, "forward_aicc");
    QVERIFY(stepwise.steps.size() >= 2);
    QVERIFY(!stepwise.selected_terms.empty());
}

void AlgorithmWave4TrackTest::wave4InterpretationNoForbiddenPhrases()
{
    datalab::domain::OutputPage page;
    page.method_name = "Cox Regression";
    datalab::domain::CoxRegressionFacts facts;
    facts.n = 10;
    facts.events = 5;
    facts.converged = true;
    facts.algorithm_id = "cox_ph_fixed_covariates";
    page.facts.cox_regression = facts;
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const std::string& bullet : section.bullets) {
            QVERIFY(bullet.find("过程合格") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("寿命达标") == std::string::npos);
        }
    }
}

QTEST_APPLESS_MAIN(AlgorithmWave4TrackTest)
#include "algorithm_wave4_track_test.moc"
