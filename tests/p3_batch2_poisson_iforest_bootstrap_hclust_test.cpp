#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/bootstrap_mean.h"
#include "domain/statistics/hierarchical_cluster.h"
#include "domain/statistics/isolation_forest.h"
#include "domain/statistics/poisson_regression.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class P3Batch2PoissonIforestBootstrapHclustTest final : public QObject {
    Q_OBJECT

private slots:
    void poissonRecoversPositiveSlope();
    void isolationScoresHigherForOutlier();
    void bootstrapMeanCiCoversTruth();
    void hierarchicalCompleteSeparatesBlobs();
    void servicePagesSerializationInterpretation();
};

void P3Batch2PoissonIforestBootstrapHclustTest::poissonRecoversPositiveSlope()
{
    // # source: formula_reference — y ~ Poisson(exp(0.5 + 0.8 x)) → coef x > 0.
    std::vector<std::vector<double>> predictors;
    std::vector<double> response;
    for (int i = 0; i < 40; ++i) {
        const double x = 0.05 * i;
        const double mu = std::exp(0.5 + 0.8 * x);
        predictors.push_back({x});
        response.push_back(std::round(mu));
    }
    const auto result = datalab::domain::statistics::fit_poisson_regression(
        response, predictors, {"X"});
    QVERIFY(result.converged);
    QCOMPARE(result.coefficients.size(), std::size_t{2});
    QVERIFY(result.coefficients[1].coefficient > 0.3);
}

void P3Batch2PoissonIforestBootstrapHclustTest::isolationScoresHigherForOutlier()
{
    // # source: formula_reference — far point gets higher isolation score.
    std::vector<std::vector<double>> rows;
    for (int i = 0; i < 30; ++i) {
        rows.push_back({0.1 * i, 0.1 * i});
    }
    rows.push_back({50.0, -50.0});
    const auto result = datalab::domain::statistics::isolation_forest(
        rows, {50, 30, 7, 0.90});
    QCOMPARE(result.scores.size(), rows.size());
    std::vector<double> ordered = result.scores;
    std::sort(ordered.begin(), ordered.end());
    QVERIFY(result.scores.back() + 1.0e-12 >= ordered[ordered.size() - 3]);
}

void P3Batch2PoissonIforestBootstrapHclustTest::bootstrapMeanCiCoversTruth()
{
    // # source: formula_reference — percentile CI around sample mean of 5s.
    std::vector<double> sample(40, 5.0);
    for (int i = 0; i < 40; ++i) {
        sample[static_cast<std::size_t>(i)] += (i % 2 == 0) ? 0.2 : -0.2;
    }
    const auto result = datalab::domain::statistics::bootstrap_mean_ci(
        sample, {500, 0.95, 3});
    QVERIFY(result.sample_mean.has_value());
    QVERIFY(result.ci_lower.has_value());
    QVERIFY(result.ci_upper.has_value());
    QVERIFY(*result.ci_lower < *result.sample_mean);
    QVERIFY(*result.ci_upper > *result.sample_mean);
    QVERIFY(*result.ci_lower < 5.0 && *result.ci_upper > 5.0);
}

void P3Batch2PoissonIforestBootstrapHclustTest::hierarchicalCompleteSeparatesBlobs()
{
    // # source: formula_reference — two blobs → k=2 clean cut.
    std::vector<std::vector<double>> rows;
    for (int i = 0; i < 8; ++i) {
        rows.push_back({0.0 + 0.01 * i, 0.0});
        rows.push_back({10.0 + 0.01 * i, 10.0});
    }
    const auto result = datalab::domain::statistics::cluster_observations_complete(
        rows, {2, false});
    QCOMPARE(result.cluster_count, std::size_t{2});
    QVERIFY(result.merges.size() + 1 == result.observation_count);
    const std::size_t first = result.assignments.front();
    std::size_t same = 0;
    for (std::size_t index = 0; index < result.assignments.size(); ++index) {
        if (rows[index][0] < 5.0 && result.assignments[index] == first) {
            ++same;
        }
    }
    QVERIFY(same == 0 || same == 8);
}

void P3Batch2PoissonIforestBootstrapHclustTest::servicePagesSerializationInterpretation()
{
    DataTable table;
    table.columns = {"X", "Y", "Count", "A", "B"};
    for (int i = 0; i < 24; ++i) {
        const double x = 0.1 * i;
        table.rows.push_back({
            std::to_string(x),
            std::to_string((i < 12) ? 0.1 * i : 8.0 + 0.1 * i),
            std::to_string(std::max(0.0, std::round(std::exp(0.2 + 0.5 * x)))),
            std::to_string((i < 12) ? 0.05 * i : 9.0 + 0.05 * i),
            std::to_string((i < 12) ? 0.02 * i : 9.5 + 0.02 * i)});
    }

    AnalysisConfiguration poisson_config;
    poisson_config.poisson_regression.response_column = 2;
    poisson_config.poisson_regression.predictor_columns = {0};
    const auto poisson_page =
        datalab::application::AnalysisService::poisson_regression(table, poisson_config);
    QVERIFY(poisson_page.facts.poisson_regression.has_value());

    AnalysisConfiguration forest_config;
    forest_config.isolation_forest.variable_columns = {0, 1};
    forest_config.isolation_forest.tree_count = 40;
    forest_config.isolation_forest.max_samples = 20;
    const auto forest_page =
        datalab::application::AnalysisService::isolation_forest(table, forest_config);
    QVERIFY(forest_page.facts.isolation_forest.has_value());

    AnalysisConfiguration bootstrap_config;
    bootstrap_config.bootstrap_mean.series_column = 0;
    bootstrap_config.bootstrap_mean.replicates = 200;
    auto bootstrap_page =
        datalab::application::AnalysisService::bootstrap_mean(table, bootstrap_config);
    QVERIFY(bootstrap_page.facts.bootstrap_mean.has_value());

    AnalysisConfiguration hclust_config;
    hclust_config.hierarchical_cluster.variable_columns = {3, 4};
    hclust_config.hierarchical_cluster.cluster_count = 2;
    const auto hclust_page =
        datalab::application::AnalysisService::cluster_observations(table, hclust_config);
    QVERIFY(hclust_page.facts.hierarchical_cluster.has_value());

    const auto poisson_back = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(poisson_page));
    QVERIFY(poisson_back.facts.poisson_regression.has_value());
    const auto forest_back = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(forest_page));
    QVERIFY(forest_back.facts.isolation_forest.has_value());
    const auto bootstrap_back = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(bootstrap_page));
    QVERIFY(bootstrap_back.facts.bootstrap_mean.has_value());
    const auto hclust_back = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(hclust_page));
    QVERIFY(hclust_back.facts.hierarchical_cluster.has_value());

    datalab::application::InterpretationService::enrich(bootstrap_page);
    for (const auto& section : bootstrap_page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("过程已失控") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
            QVERIFY(bullet.find("分布已正态") == std::string::npos);
        }
    }
}

QTEST_MAIN(P3Batch2PoissonIforestBootstrapHclustTest)
#include "p3_batch2_poisson_iforest_bootstrap_hclust_test.moc"
