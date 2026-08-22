#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/best_subsets_regression.h"
#include "domain/statistics/logistic_regression.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <algorithm>
#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class AlgorithmBatch2026AugTest final : public QObject {
    Q_OBJECT

private slots:
    void bestSubsetsPrefersStrongPredictor();
    void bestSubsetsServiceCompleteCaseAndSourceRow();
    void bestSubsetsSerializationRoundTrip();
    void bestSubsetsInterpretationNoForbiddenPhrases();
    void logisticConcordanceAndClassification();
    void batchCapabilityGroupsByBatch();
};

void AlgorithmBatch2026AugTest::logisticConcordanceAndClassification()
{
    // # source: formula_reference — perfect rank agreement → high concordance.
    std::vector<int> response = {0, 0, 1, 1};
    std::vector<std::vector<double>> predictors = {{0.0}, {0.1}, {0.9}, {1.0}};
    const auto result = datalab::domain::statistics::fit_logistic_regression(
        response, predictors, {"X"});
    QVERIFY(result.converged);
    QVERIFY(result.concordant_pairs > 0);
    QVERIFY(result.true_positive + result.true_negative > 0);

    DataTable table;
    table.columns = {"Y", "X"};
    for (int i = 0; i < 40; ++i) {
        const int y = i >= 20 ? 1 : 0;
        table.rows.push_back({std::to_string(y), std::to_string(0.05 * i)});
    }
    AnalysisConfiguration config;
    config.inference.logistic_response_column = 0;
    config.inference.logistic_predictor_columns = {1};
    config.inference.logistic_event_level = "1";
    auto page = datalab::application::AnalysisService::logistic_regression(table, config);
    QVERIFY(page.facts.logistic.has_value());
    QVERIFY(page.facts.logistic->concordant_pairs > 0);
    bool has_association_table = false;
    for (const auto& table_out : page.tables) {
        if (table_out.title.find("关联统计") != std::string::npos) {
            has_association_table = true;
        }
    }
    QVERIFY(has_association_table);
}

void AlgorithmBatch2026AugTest::batchCapabilityGroupsByBatch()
{
    // # source: formula_reference — two batches with distinct means.
    DataTable table;
    table.columns = {"Y", "Batch"};
    for (int i = 0; i < 5; ++i) {
        table.rows.push_back({std::to_string(10.0 + 0.01 * i), "A"});
    }
    for (int i = 0; i < 5; ++i) {
        table.rows.push_back({std::to_string(12.0 + 0.01 * i), "B"});
    }
    AnalysisConfiguration config;
    config.batch_capability.measurement_column = 0;
    config.batch_capability.batch_column = 1;
    config.specifications.lower = 9.0;
    config.specifications.upper = 13.0;
    auto page = datalab::application::AnalysisService::batch_capability(table, config);
    QVERIFY(page.facts.batch_capability.has_value());
    QCOMPARE(page.facts.batch_capability->batch_count, std::size_t{2});
    QCOMPARE(page.facts.batch_capability->total_observations, std::size_t{10});
    QVERIFY(page.tables.front().rows.size() >= 2);
}

void AlgorithmBatch2026AugTest::bestSubsetsPrefersStrongPredictor()
{
    // # source: formula_reference — y≈x1; x2 noise → 1-var model with x1 wins.
    std::vector<double> y;
    std::vector<std::vector<double>> x;
    for (int i = 0; i < 40; ++i) {
        const double x1 = 0.1 * i;
        const double x2 = ((i * 17) % 10) * 0.01;
        y.push_back(2.0 + 3.0 * x1 + 0.01 * x2);
        x.push_back({x1, x2});
    }
    const auto result = datalab::domain::statistics::fit_best_subsets_regression(
        y, x, {"X1", "X2"}, 1, 2, 1);
    QVERIFY(result.best_overall.has_value());
    QVERIFY(result.best_overall->r_squared > 0.9);
    bool found_x1_single = false;
    for (const auto& model : result.model_summaries) {
        if (model.predictor_count == 1 && model.predictors_in_model[0]) {
            found_x1_single = true;
            QVERIFY(model.r_squared > 0.85);
        }
    }
    QVERIFY(found_x1_single);
}

void AlgorithmBatch2026AugTest::bestSubsetsServiceCompleteCaseAndSourceRow()
{
    DataTable table;
    table.columns = {"Y", "X1", "X2"};
    table.rows.push_back({"1", "0.1", "0.01"});
    table.rows.push_back({"*", "0.2", "0.02"});  // missing Y → excluded
    table.rows.push_back({"2.2", "0.2", "0.02"});
    table.rows.push_back({"3.1", "0.3", "0.03"});
    table.rows.push_back({"4.0", "0.4", "0.04"});
    table.rows.push_back({"5.2", "0.5", "0.05"});

    AnalysisConfiguration config;
    config.best_subsets_regression.response_column = 0;
    config.best_subsets_regression.predictor_columns = {1, 2};
    auto page = datalab::application::AnalysisService::best_subsets_regression(table, config);
    QVERIFY(page.facts.best_subsets_regression.has_value());
    QCOMPARE(page.facts.best_subsets_regression->n, std::size_t{5});
    QVERIFY(!page.tables.empty());
    bool missing_warning = false;
    for (const auto& diagnostic : page.diagnostics) {
        if (diagnostic.code == "missing_values") {
            missing_warning = true;
        }
    }
    QVERIFY(missing_warning);

    DataTable table_b;
    table_b.columns = table.columns;
    table_b.rows = {{"9", "1", "1"}, {"10", "2", "2"}, {"11", "3", "3"}, {"12", "4", "4"}};
    config.best_subsets_regression.response_column = 0;
    auto page_b =
        datalab::application::AnalysisService::best_subsets_regression(table_b, config);
    QVERIFY(page_b.facts.best_subsets_regression.has_value());
    QVERIFY(page_b.id != page.id);
}

void AlgorithmBatch2026AugTest::bestSubsetsSerializationRoundTrip()
{
    datalab::domain::OutputPage page;
    page.id = "best_subsets_test";
    page.title = "Best Subsets 回归";
    page.method_name = "Best Subsets Regression";
    datalab::domain::BestSubsetsRegressionFacts facts;
    facts.n = 30;
    facts.candidate_count = 3;
    facts.model_count = 3;
    facts.models_per_size = 1;
    facts.best_r_squared = 0.88;
    facts.best_adjusted_r_squared = 0.86;
    facts.best_predictor_count = 2;
    page.facts.best_subsets_regression = facts;

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.best_subsets_regression.has_value());
    QCOMPARE(restored.facts.best_subsets_regression->n, std::size_t{30});
    QCOMPARE(*restored.facts.best_subsets_regression->best_r_squared, 0.88);
    QCOMPARE(*restored.facts.best_subsets_regression->best_predictor_count, std::size_t{2});
}

void AlgorithmBatch2026AugTest::bestSubsetsInterpretationNoForbiddenPhrases()
{
    datalab::domain::OutputPage page;
    page.method_name = "Best Subsets Regression";
    datalab::domain::BestSubsetsRegressionFacts facts;
    facts.n = 20;
    facts.candidate_count = 4;
    facts.model_count = 4;
    facts.best_r_squared = 0.75;
    facts.best_predictor_count = 2;
    page.facts.best_subsets_regression = facts;
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const std::string& bullet : section.bullets) {
            QVERIFY(bullet.find("过程已失控") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
        }
    }
}

QTEST_APPLESS_MAIN(AlgorithmBatch2026AugTest)
#include "algorithm_batch_2026_08_test.moc"
