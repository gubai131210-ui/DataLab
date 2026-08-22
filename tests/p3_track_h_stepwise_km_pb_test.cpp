#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/km_interval.h"
#include "domain/statistics/plackett_burman.h"
#include "domain/statistics/stepwise_regression.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <cmath>
#include <limits>
#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class P3TrackHStepwiseKmPbTest final : public QObject {
    Q_OBJECT

private slots:
    void stepwiseSelectsStrongPredictor();
    void kmIntervalExactMedian();
    void pbTwelveRunSevenFactors();
    void servicePagesAndForbiddenPhrases();
};

void P3TrackHStepwiseKmPbTest::stepwiseSelectsStrongPredictor()
{
    // # source: formula_reference — y≈x1; x2 noise → enter x1.
    std::vector<double> y;
    std::vector<std::vector<double>> x;
    for (int i = 0; i < 40; ++i) {
        const double x1 = 0.1 * i;
        const double x2 = ((i * 17) % 10) * 0.01;
        y.push_back(2.0 + 3.0 * x1 + 0.01 * x2);
        x.push_back({x1, x2, 0.001 * i});
    }
    const auto result = datalab::domain::statistics::fit_stepwise_regression(
        y, x, {"X1", "X2", "X3"}, "stepwise", 0.15, 0.15);
    QVERIFY(!result.selected_terms.empty());
    QVERIFY(std::find(result.selected_terms.begin(), result.selected_terms.end(), "X1")
            != result.selected_terms.end());
}

void P3TrackHStepwiseKmPbTest::kmIntervalExactMedian()
{
    // # source: formula_reference — exact failures at 1..9 → median near 5.
    std::vector<datalab::domain::statistics::IntervalObservation> obs;
    for (int t = 1; t <= 9; ++t) {
        obs.push_back({static_cast<double>(t), static_cast<double>(t),
                       static_cast<std::size_t>(t)});
    }
    const auto result = datalab::domain::statistics::kaplan_meier_interval(obs);
    QVERIFY(result.identifiable);
    QVERIFY(result.median_life.has_value());
    QVERIFY(std::abs(*result.median_life - 5.0) < 1.5);
}

void P3TrackHStepwiseKmPbTest::pbTwelveRunSevenFactors()
{
    // # source: formula_reference — 7 factors → N=12 PB runs.
    std::vector<datalab::domain::statistics::DoeFactor> factors;
    for (int i = 0; i < 7; ++i) {
        factors.push_back({std::string(1, static_cast<char>('A' + i)), "-1", "+1"});
    }
    const auto design = datalab::domain::statistics::generate_plackett_burman(
        {factors, 0, false, 1});
    QCOMPARE(design.runs.size(), std::size_t{12});
    QCOMPARE(design.factor_count, std::size_t{7});
}

void P3TrackHStepwiseKmPbTest::servicePagesAndForbiddenPhrases()
{
    DataTable table;
    table.columns = {"Y", "X1", "X2", "X3", "L", "R"};
    for (int i = 0; i < 30; ++i) {
        const double x1 = 0.1 * i;
        table.rows.push_back({
            std::to_string(1.0 + 2.0 * x1),
            std::to_string(x1),
            std::to_string(0.01 * (i % 5)),
            std::to_string(0.001 * i),
            std::to_string(1.0 + 0.2 * i),
            i % 4 == 0 ? "Inf" : std::to_string(1.5 + 0.2 * i)});
    }

    AnalysisConfiguration stepwise;
    stepwise.stepwise_regression.response_column = 0;
    stepwise.stepwise_regression.predictor_columns = {1, 2, 3};
    auto step_page =
        datalab::application::AnalysisService::stepwise_regression(table, stepwise);
    QVERIFY(step_page.facts.stepwise_regression.has_value());

    AnalysisConfiguration km;
    km.km_interval.left_column = 4;
    km.km_interval.right_column = 5;
    auto km_page = datalab::application::AnalysisService::km_interval(table, km);
    QVERIFY(km_page.facts.km_interval.has_value());

    AnalysisConfiguration pb;
    pb.plackett_burman.factor_names = {"A", "B", "C", "D", "E"};
    pb.plackett_burman.low_levels = {"-1", "-1", "-1", "-1", "-1"};
    pb.plackett_burman.high_levels = {"+1", "+1", "+1", "+1", "+1"};
    auto pb_page =
        datalab::application::AnalysisService::doe_plackett_burman(table, pb);
    QVERIFY(pb_page.facts.plackett_burman.has_value());
    QCOMPARE(pb_page.facts.plackett_burman->run_count, std::size_t{8});

    const auto back = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(step_page));
    QVERIFY(back.facts.stepwise_regression.has_value());

    for (auto* page : {&step_page, &km_page, &pb_page}) {
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

QTEST_MAIN(P3TrackHStepwiseKmPbTest)
#include "p3_track_h_stepwise_km_pb_test.moc"
