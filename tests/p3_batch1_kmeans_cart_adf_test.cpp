#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/adf_test.h"
#include "domain/statistics/cart_tree.h"
#include "domain/statistics/kmeans.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class P3Batch1KmeansCartAdfTest final : public QObject {
    Q_OBJECT

private slots:
    void kmeansSeparatesTwoBlobs();
    void cartClassificationSplitsOnX();
    void cartRegressionFitsStep();
    void adfRejectsWhiteNoiseDrift();
    void adfKeepsRandomWalk();
    void servicePagesSerializationAndInterpretation();
};

void P3Batch1KmeansCartAdfTest::kmeansSeparatesTwoBlobs()
{
    // # source: formula_reference — two well-separated blobs → 2 clusters.
    std::vector<std::vector<double>> rows;
    for (int i = 0; i < 10; ++i) {
        rows.push_back({0.0 + 0.01 * i, 0.0 + 0.01 * i});
        rows.push_back({10.0 + 0.01 * i, 10.0 + 0.01 * i});
    }
    const auto result = datalab::domain::statistics::cluster_kmeans(
        rows, {2, 50, false});
    QCOMPARE(result.cluster_count, std::size_t{2});
    QCOMPARE(result.observation_count, std::size_t{20});
    QVERIFY(result.converged);
    QVERIFY(result.total_within_ss < 5.0);
    const std::size_t first_label = result.assignments.front();
    std::size_t same_as_first_among_low = 0;
    std::size_t low_count = 0;
    for (std::size_t index = 0; index < result.assignments.size(); ++index) {
        if (rows[index][0] < 5.0) {
            ++low_count;
            if (result.assignments[index] == first_label) {
                ++same_as_first_among_low;
            }
        }
    }
    QVERIFY(low_count == 10);
    QVERIFY(same_as_first_among_low == 0 || same_as_first_among_low == 10);
}

void P3Batch1KmeansCartAdfTest::cartClassificationSplitsOnX()
{
    // # source: formula_reference — label = x>0 → accuracy high, importance on X.
    std::vector<std::vector<double>> predictors;
    std::vector<double> response;
    for (int i = -20; i <= 20; ++i) {
        if (i == 0) {
            continue;
        }
        predictors.push_back({static_cast<double>(i), 0.1 * i});
        response.push_back(i > 0 ? 1.0 : 0.0);
    }
    const auto result = datalab::domain::statistics::fit_cart_tree(
        predictors, response, {"neg", "pos"}, {"X", "Y"}, {datalab::domain::statistics::CartTask::classification, 3, 2});
    QVERIFY(result.observation_count >= 30);
    QVERIFY(result.train_metric > 0.9);
    QVERIFY(result.variable_importance[0] + 1.0e-9 >= result.variable_importance[1]);
    QVERIFY(!result.confusion.empty());
}

void P3Batch1KmeansCartAdfTest::cartRegressionFitsStep()
{
    // # source: formula_reference — y = 0/10 by x threshold → low RMSE.
    std::vector<std::vector<double>> predictors;
    std::vector<double> response;
    for (int i = 0; i < 40; ++i) {
        const double x = static_cast<double>(i);
        predictors.push_back({x});
        response.push_back(x < 20.0 ? 0.0 : 10.0);
    }
    const auto result = datalab::domain::statistics::fit_cart_tree(
        predictors, response, {}, {"X"},
        {datalab::domain::statistics::CartTask::regression, 3, 2});
    QVERIFY(result.train_metric < 1.0);
    QVERIFY(result.leaf_count >= 2);
}

void P3Batch1KmeansCartAdfTest::adfRejectsWhiteNoiseDrift()
{
    // # source: formula_reference — stationary noise → tau below 5% critical (often).
    std::vector<double> series;
    for (int i = 0; i < 80; ++i) {
        series.push_back(((i % 3) - 1) * 0.2 + ((i % 5) - 2) * 0.05);
    }
    const auto result = datalab::domain::statistics::augmented_dickey_fuller(
        series, {datalab::domain::statistics::AdfRegression::drift, 1});
    QVERIFY(result.tau.has_value());
    QVERIFY(result.critical_5.has_value());
    QVERIFY(result.tau < *result.critical_5);
    QVERIFY(result.reject_unit_root_at_5);
}

void P3Batch1KmeansCartAdfTest::adfKeepsRandomWalk()
{
    // # source: formula_reference — integrated path → typically fails to reject.
    std::vector<double> series = {0.0};
    for (int i = 0; i < 100; ++i) {
        series.push_back(series.back() + ((i % 2 == 0) ? 0.3 : -0.25));
    }
    const auto result = datalab::domain::statistics::augmented_dickey_fuller(
        series, {datalab::domain::statistics::AdfRegression::drift, 1});
    QVERIFY(result.tau.has_value());
    QVERIFY(result.critical_5.has_value());
    QVERIFY(!result.reject_unit_root_at_5 || *result.tau > *result.critical_5 - 0.5);
}

void P3Batch1KmeansCartAdfTest::servicePagesSerializationAndInterpretation()
{
    DataTable table;
    table.columns = {"X1", "X2", "Y", "Class", "Series"};
    for (int i = 0; i < 30; ++i) {
        const double x1 = (i < 15) ? 0.1 * i : 8.0 + 0.1 * i;
        const double x2 = (i < 15) ? 0.05 * i : 9.0 + 0.05 * i;
        table.rows.push_back({
            std::to_string(x1),
            std::to_string(x2),
            std::to_string(x1 < 5.0 ? 1.0 : 8.0),
            i < 15 ? "A" : "B",
            std::to_string(0.1 * i + ((i % 2) * 0.05))});
    }

    AnalysisConfiguration kmeans_config;
    kmeans_config.kmeans.variable_columns = {0, 1};
    kmeans_config.kmeans.cluster_count = 2;
    const auto kmeans_page =
        datalab::application::AnalysisService::kmeans(table, kmeans_config);
    QVERIFY(kmeans_page.facts.kmeans.has_value());
    QCOMPARE(kmeans_page.facts.kmeans->k, std::size_t{2});

    AnalysisConfiguration cart_config;
    cart_config.cart_tree.response_column = 3;
    cart_config.cart_tree.predictor_columns = {0, 1};
    cart_config.cart_tree.task = "classification";
    cart_config.cart_tree.max_depth = 3;
    cart_config.cart_tree.min_leaf = 2;
    const auto cart_page =
        datalab::application::AnalysisService::cart_tree(table, cart_config);
    QVERIFY(cart_page.facts.cart_tree.has_value());
    QVERIFY(cart_page.facts.cart_tree->leaf_count >= 1);

    AnalysisConfiguration adf_config;
    adf_config.adf.series_column = 4;
    adf_config.adf.regression = "drift";
    adf_config.adf.lags = 1;
    auto adf_page =
        datalab::application::AnalysisService::adf_test(table, adf_config);
    QVERIFY(adf_page.facts.adf.has_value());
    QVERIFY(adf_page.facts.adf->tau.has_value());

    const QJsonObject kmeans_json =
        datalab::infrastructure::output_page_to_json(kmeans_page);
    const auto kmeans_back =
        datalab::infrastructure::output_page_from_json(kmeans_json);
    QVERIFY(kmeans_back.facts.kmeans.has_value());
    QCOMPARE(kmeans_back.facts.kmeans->k, kmeans_page.facts.kmeans->k);

    const QJsonObject cart_json =
        datalab::infrastructure::output_page_to_json(cart_page);
    const auto cart_back =
        datalab::infrastructure::output_page_from_json(cart_json);
    QVERIFY(cart_back.facts.cart_tree.has_value());

    const QJsonObject adf_json =
        datalab::infrastructure::output_page_to_json(adf_page);
    const auto adf_back =
        datalab::infrastructure::output_page_from_json(adf_json);
    QVERIFY(adf_back.facts.adf.has_value());

    datalab::application::InterpretationService::enrich(adf_page);
    for (const auto& section : adf_page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("过程已失控") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
            QVERIFY(bullet.find("分布已正态") == std::string::npos);
        }
    }
}

QTEST_MAIN(P3Batch1KmeansCartAdfTest)
#include "p3_batch1_kmeans_cart_adf_test.moc"
