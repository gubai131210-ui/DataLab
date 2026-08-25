#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/mixture_design.h"
#include "domain/statistics/nhpp_repairable.h"
#include "domain/statistics/reliability_test_plan.h"
#include "domain/statistics/taguchi_analyze.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class AlgorithmWave6TrackTest final : public QObject {
    Q_OBJECT

private slots:
    void taguchiSnFormulaReference();
    void taguchiServiceAndSerialize();
    void taguchiCompleteCaseGate();
    void mixtureQ3N6FormulaReference();
    void mixtureExportClearsExcludes();
    void mixtureBadQGate();
    void nhppBetaFormulaReference();
    void nhppServiceAndSerialize();
    void nhppBadTimesGate();
    void rtpMonotonicityFormulaReference();
    void rtpServiceAndSerialize();
    void rtpInvalidRGate();
    void wave6InterpretationNoForbiddenPhrases();
};

void AlgorithmWave6TrackTest::taguchiSnFormulaReference()
{
    // # source: formula_reference — smaller-is-better S/N for Y={1,2}.
    // mean Y^2 = (1+4)/2 = 2.5 → S/N = -10 log10(2.5)
    const auto sn = datalab::domain::statistics::sn_ratio_smaller({1.0, 2.0});
    QVERIFY(sn.has_value());
    const double expected = -10.0 * std::log10(2.5);
    QCOMPARE(*sn, expected);

    std::vector<std::vector<std::string>> levels = {{"1"}, {"2"}, {"1"}, {"2"}};
    std::vector<std::vector<double>> responses = {{10, 12}, {8, 9}, {11, 13}, {7, 8}};
    const auto result = datalab::domain::statistics::analyze_taguchi_static(
        levels, responses, {"A"}, {},
        {datalab::domain::statistics::TaguchiSnType::larger_is_better});
    QCOMPARE(result.run_count, std::size_t{4});
    QVERIFY(!result.means_table.empty());
    QVERIFY(!result.sn_table.empty());
}

void AlgorithmWave6TrackTest::taguchiServiceAndSerialize()
{
    // Marker: TaguchiAnalyzeFacts
    DataTable table;
    table.columns = {"A", "Y1", "Y2"};
    table.rows = {
        {"1", "10", "12"}, {"2", "8", "9"}, {"1", "11", "13"}, {"2", "7", "8"}};
    AnalysisConfiguration config;
    config.taguchi_analyze.factor_columns = {0};
    config.taguchi_analyze.response_columns = {1, 2};
    config.taguchi_analyze.sn_type = "larger";
    auto page = datalab::application::AnalysisService::taguchi_analyze(table, config);
    QVERIFY(page.facts.taguchi_analyze.has_value());
    QCOMPARE(page.facts.taguchi_analyze->run_count, std::size_t{4});
    bool has_means = false;
    bool has_sn = false;
    for (const auto& t : page.tables) {
        if (t.title.find("Means") != std::string::npos) {
            has_means = true;
        }
        if (t.title.find("Signal to Noise") != std::string::npos) {
            has_sn = true;
        }
    }
    QVERIFY(has_means);
    QVERIFY(has_sn);
    QVERIFY(!page.plots.empty());

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.taguchi_analyze.has_value());
    QCOMPARE(restored.facts.taguchi_analyze->algorithm_id,
             std::string("taguchi_analyze_static_sn"));
}

void AlgorithmWave6TrackTest::taguchiCompleteCaseGate()
{
    // # source: formula_reference — missing response drops row (complete-case).
    DataTable table;
    table.columns = {"A", "Y1", "Y2"};
    table.rows = {
        {"1", "10", "12"}, {"2", "", "9"}, {"1", "11", "13"}, {"2", "7", "8"}};
    AnalysisConfiguration config;
    config.taguchi_analyze.factor_columns = {0};
    config.taguchi_analyze.response_columns = {1, 2};
    auto page = datalab::application::AnalysisService::taguchi_analyze(table, config);
    QVERIFY(page.facts.taguchi_analyze.has_value());
    QCOMPARE(page.facts.taguchi_analyze->run_count, std::size_t{3});
}

void AlgorithmWave6TrackTest::mixtureQ3N6FormulaReference()
{
    // # source: formula_reference — N = q(q+1)/2 for q=3 → 6.
    datalab::domain::statistics::MixtureDesignOptions options;
    options.component_count = 3;
    options.component_names = {"x1", "x2", "x3"};
    const auto design =
        datalab::domain::statistics::generate_mixture_simplex_lattice(options);
    QCOMPARE(design.run_count, std::size_t{6});
    QCOMPARE(design.component_count, std::size_t{3});
    QVERIFY(design.runs.size() == 6);
}

void AlgorithmWave6TrackTest::mixtureExportClearsExcludes()
{
    // Marker: MixtureDesignFacts + worksheet_export clears excludes.
    AnalysisConfiguration config;
    config.excluded_rows = {99};
    config.hidden_rows = {88};
    config.mixture_design.component_count = 3;
    config.mixture_design.component_names = {"x1", "x2", "x3"};
    DataTable unused;
    auto page = datalab::application::AnalysisService::mixture_design(unused, config);
    QVERIFY(page.facts.mixture_design.has_value());
    QCOMPARE(page.facts.mixture_design->run_count, std::size_t{6});
    QVERIFY(page.worksheet_export.has_value());
    QCOMPARE(page.worksheet_export->rows.size(), std::size_t{6});
    QVERIFY(page.configuration.excluded_rows.empty());
    QVERIFY(page.configuration.hidden_rows.empty());
}

void AlgorithmWave6TrackTest::mixtureBadQGate()
{
    datalab::domain::statistics::MixtureDesignOptions options;
    options.component_count = 5;
    const auto design =
        datalab::domain::statistics::generate_mixture_simplex_lattice(options);
    QVERIFY(!design.diagnostics.empty());
    QCOMPARE(design.run_count, std::size_t{0});
}

void AlgorithmWave6TrackTest::nhppBetaFormulaReference()
{
    // # source: formula_reference — synthetic times; β = n / Σ ln(T/ti).
    std::vector<double> times = {10.0, 20.0, 40.0};
    const double T = 40.0;
    const double n = 3.0;
    const double sum_ln =
        std::log(T / 10.0) + std::log(T / 20.0) + std::log(T / 40.0);
    const double expected_beta = n / sum_ln;
    const auto result = datalab::domain::statistics::fit_nhpp_crow_amsaa(
        times, {}, {T, 5});
    QVERIFY(result.beta.has_value());
    QCOMPARE(*result.beta, expected_beta);
    QVERIFY(result.lambda.has_value());
}

void AlgorithmWave6TrackTest::nhppServiceAndSerialize()
{
    // Marker: NhppRepairableFacts
    DataTable table;
    table.columns = {"Time"};
    table.rows = {{"10"}, {"20"}, {"40"}, {"80"}};
    AnalysisConfiguration config;
    config.nhpp_repairable.time_column = 0;
    config.nhpp_repairable.truncation_time = 80.0;
    auto page = datalab::application::AnalysisService::nhpp_repairable(table, config);
    QVERIFY(page.facts.nhpp_repairable.has_value());
    QCOMPARE(page.facts.nhpp_repairable->failure_count, std::size_t{4});
    QVERIFY(page.facts.nhpp_repairable->beta.has_value());
    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.nhpp_repairable.has_value());
    QCOMPARE(restored.facts.nhpp_repairable->algorithm_id,
             std::string("nhpp_crow_amsaa_mle"));
}

void AlgorithmWave6TrackTest::nhppBadTimesGate()
{
    const auto result = datalab::domain::statistics::fit_nhpp_crow_amsaa(
        {-1.0, 2.0}, {}, {});
    QVERIFY(!result.beta.has_value());
    QVERIFY(!result.diagnostics.empty());
}

void AlgorithmWave6TrackTest::rtpMonotonicityFormulaReference()
{
    // # source: formula_reference — higher CL → larger n (zero-failure, δ=1).
    datalab::domain::statistics::ReliabilityTestPlanOptions lo;
    lo.shape_beta = 1.0;
    lo.target_reliability = 0.9;
    lo.confidence_level = 0.8;
    lo.test_time = 1.0;
    lo.mission_time = 1.0;
    lo.allowed_failures = 0;
    datalab::domain::statistics::ReliabilityTestPlanOptions hi = lo;
    hi.confidence_level = 0.95;
    const auto plan_lo =
        datalab::domain::statistics::plan_reliability_demonstration(lo);
    const auto plan_hi =
        datalab::domain::statistics::plan_reliability_demonstration(hi);
    QVERIFY(plan_lo.sample_size.has_value());
    QVERIFY(plan_hi.sample_size.has_value());
    QVERIFY(*plan_hi.sample_size >= *plan_lo.sample_size);
}

void AlgorithmWave6TrackTest::rtpServiceAndSerialize()
{
    // Marker: ReliabilityTestPlanFacts
    AnalysisConfiguration config;
    config.reliability_test_plan.shape_beta = 1.0;
    config.reliability_test_plan.target_reliability = 0.9;
    config.reliability_test_plan.confidence_level = 0.9;
    config.reliability_test_plan.test_time = 1.0;
    config.reliability_test_plan.mission_time = 1.0;
    DataTable unused;
    auto page =
        datalab::application::AnalysisService::reliability_test_plan(unused, config);
    QVERIFY(page.facts.reliability_test_plan.has_value());
    QVERIFY(page.facts.reliability_test_plan->sample_size.has_value());
    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.reliability_test_plan.has_value());
    QCOMPARE(restored.facts.reliability_test_plan->algorithm_id,
             std::string("reliability_demo_test_plan_weibull"));
}

void AlgorithmWave6TrackTest::rtpInvalidRGate()
{
    datalab::domain::statistics::ReliabilityTestPlanOptions options;
    options.target_reliability = 1.5;
    const auto plan =
        datalab::domain::statistics::plan_reliability_demonstration(options);
    QVERIFY(!plan.sample_size.has_value());
    QVERIFY(!plan.diagnostics.empty());
}

void AlgorithmWave6TrackTest::wave6InterpretationNoForbiddenPhrases()
{
    AnalysisConfiguration config;
    config.reliability_test_plan.target_reliability = 0.9;
    config.reliability_test_plan.confidence_level = 0.9;
    DataTable unused;
    auto page =
        datalab::application::AnalysisService::reliability_test_plan(unused, config);
    datalab::application::InterpretationService::enrich(page);

    DataTable nhpp_table;
    nhpp_table.columns = {"Time"};
    nhpp_table.rows = {{"10"}, {"20"}, {"40"}};
    AnalysisConfiguration nhpp_config;
    nhpp_config.nhpp_repairable.time_column = 0;
    auto nhpp_page =
        datalab::application::AnalysisService::nhpp_repairable(nhpp_table, nhpp_config);
    datalab::application::InterpretationService::enrich(nhpp_page);

    auto assert_no_affirmative = [](const datalab::domain::OutputPage& p) {
        for (const auto& section : p.interpretation) {
            for (const std::string& bullet : section.bullets) {
                // Allow meta forbid sentences that contain the tokens after 禁止.
                if (bullet.find("禁止") != std::string::npos) {
                    continue;
                }
                QVERIFY(bullet.find("已证明稳定") == std::string::npos);
                QVERIFY(bullet.find("已合格") == std::string::npos);
                QVERIFY(bullet.find("过程已优化") == std::string::npos);
                QVERIFY(bullet.find("ROCOF合格") == std::string::npos);
            }
        }
    };
    assert_no_affirmative(page);
    assert_no_affirmative(nhpp_page);
}

QTEST_MAIN(AlgorithmWave6TrackTest)
#include "algorithm_wave6_track_test.moc"
