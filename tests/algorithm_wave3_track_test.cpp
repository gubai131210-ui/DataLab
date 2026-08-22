#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/bootstrap_two_sample.h"
#include "domain/statistics/probit_reliability.h"
#include "domain/statistics/reliability.h"
#include "domain/statistics/accelerated_life.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class AlgorithmWave3TrackTest final : public QObject {
    Q_OBJECT

private slots:
    void bootstrapTwoSampleMeanDifference();
    void bootstrapTwoSampleServiceAndSerialize();
    void probitReliabilityLd50();
    void probitReliabilityServiceAndInterpret();
    void logRankKGroupsThreeGroups();
    void acceleratedLifeUseStressPercentiles();
    void wave3InterpretationNoForbiddenPhrases();
};

void AlgorithmWave3TrackTest::bootstrapTwoSampleMeanDifference()
{
    // # source: formula_reference — known mean shift.
    std::vector<double> first = {1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<double> second = {3.0, 4.0, 5.0, 6.0, 7.0};
    const auto result = datalab::domain::statistics::bootstrap_two_sample_mean_difference_ci(
        first, second, {200, 0.95, 42});
    QCOMPARE(result.n_first, std::size_t{5});
    QCOMPARE(result.n_second, std::size_t{5});
    QVERIFY(result.mean_difference.has_value());
    QVERIFY(*result.mean_difference > 1.5 && *result.mean_difference < 2.5);
    QVERIFY(result.ci_lower.has_value() && result.ci_upper.has_value());
    QVERIFY(*result.ci_lower < *result.mean_difference);
    QVERIFY(*result.ci_upper > *result.mean_difference);
}

void AlgorithmWave3TrackTest::bootstrapTwoSampleServiceAndSerialize()
{
    DataTable table;
    table.columns = {"A", "B"};
    for (int i = 0; i < 10; ++i) {
        table.rows.push_back({std::to_string(1.0 + i), std::to_string(3.0 + i)});
    }
    AnalysisConfiguration config;
    config.bootstrap_two_sample.first_column = 0;
    config.bootstrap_two_sample.second_column = 1;
    config.bootstrap_two_sample.replicates = 150;
    config.bootstrap_two_sample.seed = 7;
    auto page = datalab::application::AnalysisService::bootstrap_two_sample(table, config);
    QVERIFY(page.facts.bootstrap_two_sample.has_value());
    QCOMPARE(page.facts.bootstrap_two_sample->n_first, std::size_t{10});

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.bootstrap_two_sample.has_value());
    QCOMPARE(restored.facts.bootstrap_two_sample->method, std::string("percentile"));
}

void AlgorithmWave3TrackTest::probitReliabilityLd50()
{
    std::vector<std::size_t> events = {0, 1, 2, 4, 6, 8};
    std::vector<std::size_t> trials = {10, 10, 10, 10, 10, 10};
    std::vector<double> stress = {10, 20, 30, 40, 50, 60};
    const auto result = datalab::domain::statistics::fit_probit_reliability(
        events, trials, stress);
    QVERIFY(result.converged);
    QVERIFY(result.coefficients.size() >= 2);
    QVERIFY(result.ld50.has_value());
}

void AlgorithmWave3TrackTest::probitReliabilityServiceAndInterpret()
{
    DataTable table;
    table.columns = {"Events", "Trials", "Stress"};
    table.rows.push_back({"0", "10", "10"});
    table.rows.push_back({"1", "10", "20"});
    table.rows.push_back({"3", "10", "30"});
    table.rows.push_back({"5", "10", "40"});
    table.rows.push_back({"7", "10", "50"});
    table.rows.push_back({"9", "10", "60"});
    AnalysisConfiguration config;
    config.probit_reliability.events_column = 0;
    config.probit_reliability.trials_column = 1;
    config.probit_reliability.stress_column = 2;
    auto page = datalab::application::AnalysisService::probit_reliability(table, config);
    QVERIFY(page.facts.probit_reliability.has_value());
    QVERIFY(page.facts.probit_reliability->converged);
    datalab::application::InterpretationService::enrich(page);
    bool has_probit = false;
    for (const auto& section : page.interpretation) {
        for (const std::string& bullet : section.bullets) {
            if (bullet.find("Probit") != std::string::npos
                || bullet.find("probit") != std::string::npos
                || bullet.find("logit") != std::string::npos) {
                has_probit = true;
            }
        }
    }
    QVERIFY(has_probit);
}

void AlgorithmWave3TrackTest::logRankKGroupsThreeGroups()
{
    std::vector<double> times = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    std::vector<bool> events = {true, true, true, true, true, true,
                                true, true, true, true, true, true};
    std::vector<int> groups = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2};
    const auto result = datalab::domain::statistics::log_rank_k_groups(
        times, events, groups);
    QVERIFY(result.degrees_of_freedom >= 2.0);
    QVERIFY(result.p_value >= 0.0 && result.p_value <= 1.0);
    QCOMPARE(result.group_summaries.size(), std::size_t{3});
}

void AlgorithmWave3TrackTest::acceleratedLifeUseStressPercentiles()
{
    std::vector<double> times = {5000, 5200, 4800, 2000, 2100, 1900, 800, 850, 820, 900};
    std::vector<bool> events = {true, true, true, true, true, true, true, true, true, true};
    std::vector<double> stress = {40, 45, 42, 60, 62, 58, 80, 82, 79, 85};
    const auto result = datalab::domain::statistics::fit_accelerated_life_weibull_arrhenius(
        times, events, stress, {}, 0.95, 25.0);
    QVERIFY(result.b10_at_use_stress.has_value());
    QVERIFY(result.b50_at_use_stress.has_value());
    QVERIFY(result.b90_at_use_stress.has_value());
    QVERIFY(!result.life_stress_curve.empty());
    QVERIFY(!result.percentiles_at_use_stress.empty());
}

void AlgorithmWave3TrackTest::wave3InterpretationNoForbiddenPhrases()
{
    datalab::domain::OutputPage page;
    page.method_name = "Bootstrap Two-Sample";
    datalab::domain::BootstrapTwoSampleFacts facts;
    facts.n_first = 10;
    facts.n_second = 10;
    facts.mean_difference = 2.0;
    page.facts.bootstrap_two_sample = facts;
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const std::string& bullet : section.bullets) {
            QVERIFY(bullet.find("过程合格") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
        }
    }
}

QTEST_APPLESS_MAIN(AlgorithmWave3TrackTest)
#include "algorithm_wave3_track_test.moc"
