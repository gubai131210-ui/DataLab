#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/ccf.h"
#include "domain/statistics/discriminant.h"
#include "domain/statistics/ordinal_logistic.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class P3Batch3OrdinalLdaCcfCorrelogramTest final : public QObject {
    Q_OBJECT

private slots:
    void ordinalRecoversOrderedSignal();
    void ldaSeparatesTwoClasses();
    void ccfPeakAtLagZeroForAlignedSeries();
    void correlogramPageAndRoundTrip();
};

void P3Batch3OrdinalLdaCcfCorrelogramTest::ordinalRecoversOrderedSignal()
{
    // # source: formula_reference — higher x → higher ordinal level.
    std::vector<std::vector<double>> predictors;
    std::vector<std::size_t> response;
    for (int i = 0; i < 60; ++i) {
        const double x = 0.05 * i;
        predictors.push_back({x});
        response.push_back(x < 0.8 ? 0 : (x < 1.8 ? 1 : 2));
    }
    const auto result = datalab::domain::statistics::fit_ordinal_logistic(
        response, predictors, {"L", "M", "H"}, {"X"}, 40, 1.0e-5);
    QVERIFY(result.observation_count == 60);
    QVERIFY(!result.slopes.empty());
    QVERIFY(result.slopes.front().estimate > 0.0);
}

void P3Batch3OrdinalLdaCcfCorrelogramTest::ldaSeparatesTwoClasses()
{
    // # source: formula_reference — two well-separated classes → high accuracy.
    std::vector<std::vector<double>> predictors;
    std::vector<std::size_t> classes;
    for (int i = 0; i < 20; ++i) {
        predictors.push_back({0.1 * i, 0.05 * i});
        classes.push_back(0);
        predictors.push_back({8.0 + 0.1 * i, 9.0 + 0.05 * i});
        classes.push_back(1);
    }
    const auto result = datalab::domain::statistics::linear_discriminant(
        classes, predictors, {"A", "B"});
    QVERIFY(result.train_accuracy > 0.9);
    QCOMPARE(result.class_count, std::size_t{2});
}

void P3Batch3OrdinalLdaCcfCorrelogramTest::ccfPeakAtLagZeroForAlignedSeries()
{
    // # source: formula_reference — identical series → ccf(0)=1.
    std::vector<double> series;
    for (int i = 0; i < 40; ++i) {
        series.push_back(std::sin(0.3 * i));
    }
    const auto result = datalab::domain::statistics::compute_ccf(series, series, 5, 0.05);
    QVERIFY(result.ccf.size() > 5);
    double at_zero = 0.0;
    for (std::size_t i = 0; i < result.lags.size(); ++i) {
        if (std::abs(result.lags[i]) < 1.0e-12) {
            at_zero = result.ccf[i];
        }
    }
    QVERIFY(std::abs(at_zero - 1.0) < 1.0e-9);
}

void P3Batch3OrdinalLdaCcfCorrelogramTest::correlogramPageAndRoundTrip()
{
    DataTable table;
    table.columns = {"X1", "X2", "X3", "Ord", "Cls", "A", "B"};
    for (int i = 0; i < 36; ++i) {
        const double x = 0.1 * i;
        table.rows.push_back({
            std::to_string(x),
            std::to_string(2.0 * x + 0.01 * (i % 3)),
            std::to_string(-x + 0.2),
            x < 1.0 ? "1" : (x < 2.0 ? "2" : "3"),
            i < 18 ? "A" : "B",
            std::to_string(std::sin(0.2 * i)),
            std::to_string(std::sin(0.2 * i + 0.1))});
    }

    AnalysisConfiguration ordinal_config;
    ordinal_config.ordinal_logistic.response_column = 3;
    ordinal_config.ordinal_logistic.predictor_columns = {0};
    auto ordinal_page =
        datalab::application::AnalysisService::ordinal_logistic(table, ordinal_config);
    QVERIFY(ordinal_page.facts.ordinal_logistic.has_value());

    AnalysisConfiguration lda_config;
    lda_config.discriminant.response_column = 4;
    lda_config.discriminant.predictor_columns = {0, 1};
    auto lda_page =
        datalab::application::AnalysisService::discriminant(table, lda_config);
    QVERIFY(lda_page.facts.discriminant.has_value());

    AnalysisConfiguration ccf_config;
    ccf_config.ccf.x_column = 5;
    ccf_config.ccf.y_column = 6;
    ccf_config.ccf.max_lag = 4;
    auto ccf_page = datalab::application::AnalysisService::ccf(table, ccf_config);
    QVERIFY(ccf_page.facts.ccf.has_value());

    AnalysisConfiguration corr_config;
    corr_config.correlogram.variable_columns = {0, 1, 2};
    corr_config.correlogram.method = "pearson";
    auto corr_page =
        datalab::application::AnalysisService::correlogram(table, corr_config);
    QVERIFY(corr_page.facts.correlogram.has_value());
    QCOMPARE(corr_page.plots.size(), std::size_t{1});
    // # source: formula_reference — multi-column correlation heatmap page.

    const auto back = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(corr_page));
    QVERIFY(back.facts.correlogram.has_value());

    for (auto* page : {&ordinal_page, &lda_page, &ccf_page, &corr_page}) {
        datalab::application::InterpretationService::enrich(*page);
        for (const auto& section : page->interpretation) {
            for (const auto& bullet : section.bullets) {
                QVERIFY(bullet.find("过程已失控") == std::string::npos);
                QVERIFY(bullet.find("已证明稳定") == std::string::npos);
                QVERIFY(bullet.find("批次合格") == std::string::npos);
                QVERIFY(bullet.find("分布已正态") == std::string::npos);
            }
        }
    }
}

QTEST_MAIN(P3Batch3OrdinalLdaCcfCorrelogramTest)
#include "p3_batch3_ordinal_lda_ccf_correlogram_test.moc"
