#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/attribute_capability.h"
#include "domain/statistics/poisson_rate.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <string>

class PoissonRateTest final : public QObject {
    Q_OBJECT

private slots:
    void exactPValueMatchesPoissonExpansion();
    void exactIntervalMatchesGarwood();
    void scoreStatisticMatchesWorkedExample();
    void twoSampleExactMatchesConditionalBinomial();
    void twoSampleNormalZMatchesWald();
    void diagnosesInvalidExposure();
    void serviceSumsCompleteCaseRows();
    void twoSampleServiceRejectsMultipleRows();
    void interpretationDoesNotClaimPass();
    void rateRatioLogWaldMatchesWorkedExample();
    void rateRatioZeroEventsDiagnosed();
};

void PoissonRateTest::exactPValueMatchesPoissonExpansion()
{
    // # source: formula_reference — Poi(λ0 t=5): P(X<=2)=e^{-5}(1+5+12.5)
    const auto result = datalab::domain::statistics::one_poisson_rate_test(
        2, 10.0, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::less,
        datalab::domain::statistics::ProportionMethod::exact);
    QVERIFY(result.p_value.has_value());
    QCOMPARE(result.rate, 0.2);
    QVERIFY(std::abs(*result.p_value - 18.5 * std::exp(-5.0)) < 1.0e-12);
    const auto two_sided = datalab::domain::statistics::one_poisson_rate_test(
        2, 10.0, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::exact);
    QVERIFY(two_sided.p_value.has_value());
    QVERIFY(std::abs(*two_sided.p_value - 2.0 * 18.5 * std::exp(-5.0)) < 1.0e-12);
    QCOMPARE(result.method, std::string("exact"));
}

void PoissonRateTest::exactIntervalMatchesGarwood()
{
    const auto result = datalab::domain::statistics::one_poisson_rate_test(
        2, 10.0, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::exact);
    const auto interval = datalab::domain::statistics::garwood_rate(2.0, 10.0, 0.05);
    QVERIFY(result.confidence_lower.has_value());
    QVERIFY(result.confidence_upper.has_value());
    QCOMPARE(*result.confidence_lower, *interval.lower);
    QCOMPARE(*result.confidence_upper, *interval.upper);
}

void PoissonRateTest::scoreStatisticMatchesWorkedExample()
{
    // # source: formula_reference — z = (0.2-0.5)/sqrt(0.5/10)
    const auto result = datalab::domain::statistics::one_poisson_rate_test(
        2, 10.0, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::normal);
    QVERIFY(result.z_statistic.has_value());
    const double expected = (0.2 - 0.5) / std::sqrt(0.5 / 10.0);
    QVERIFY(std::abs(*result.z_statistic - expected) < 1.0e-12);
    QCOMPARE(result.method, std::string("normal"));
    QVERIFY(std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "small_count_normal_approximation";
        }));
}

void PoissonRateTest::twoSampleExactMatchesConditionalBinomial()
{
    // # source: formula_reference — Bin(10, 0.5): P(X<=2)=(1+10+45)/1024
    const auto result = datalab::domain::statistics::two_poisson_rate_test(
        2, 10.0, 8, 10.0, 0.95,
        datalab::domain::statistics::TestAlternative::less,
        datalab::domain::statistics::ProportionMethod::exact);
    QVERIFY(result.p_value.has_value());
    QCOMPARE(*result.p_value, 56.0 / 1024.0);
    QCOMPARE(result.difference, -0.6);
    const auto two_sided = datalab::domain::statistics::two_poisson_rate_test(
        2, 10.0, 8, 10.0, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::exact);
    QVERIFY(two_sided.p_value.has_value());
    QCOMPARE(*two_sided.p_value, 2.0 * 56.0 / 1024.0);
}

void PoissonRateTest::twoSampleNormalZMatchesWald()
{
    // # source: formula_reference — z = (0.2-0.8)/sqrt(0.2/10+0.8/10)
    const auto result = datalab::domain::statistics::two_poisson_rate_test(
        2, 10.0, 8, 10.0, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::normal);
    QVERIFY(result.z_statistic.has_value());
    const double expected = (0.2 - 0.8) / std::sqrt(0.2 / 10.0 + 0.8 / 10.0);
    QVERIFY(std::abs(*result.z_statistic - expected) < 1.0e-12);
}

void PoissonRateTest::diagnosesInvalidExposure()
{
    const auto result = datalab::domain::statistics::one_poisson_rate_test(
        2, 0.0, 0.5);
    QVERIFY(!result.p_value.has_value());
    QVERIFY(std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "invalid_poisson_exposure";
        }));
}

void PoissonRateTest::serviceSumsCompleteCaseRows()
{
    datalab::domain::DataTable table;
    table.columns = {"D", "T"};
    table.rows = {{"1", "50"}, {"*", "50"}, {"2", "50"}, {"1", "50"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.inference.first_events_column = 0;
    configuration.inference.first_trials_column = 1;
    configuration.inference.hypothesis_mean = 0.05;
    configuration.inference.confidence_level = 0.95;
    configuration.inference.proportion_method = "exact";
    const auto page = datalab::application::AnalysisService::one_poisson_rate(
        table, configuration);
    QVERIFY(page.facts.poisson_rate.has_value());
    QCOMPARE(page.facts.poisson_rate->kind, std::string("one_sample"));
    QCOMPARE(page.facts.poisson_rate->events, std::size_t{4});
    QCOMPARE(page.facts.poisson_rate->exposure, 150.0);
    QVERIFY(std::any_of(
        page.diagnostics.cbegin(), page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "summarized_from_multiple_rows";
        }));
    QVERIFY(std::any_of(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "检验结果";
        }));
}

void PoissonRateTest::twoSampleServiceRejectsMultipleRows()
{
    datalab::domain::DataTable table;
    table.columns = {"D1", "T1", "D2", "T2"};
    table.rows = {{"2", "10", "8", "10"}, {"1", "10", "3", "10"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.inference.first_events_column = 0;
    configuration.inference.first_trials_column = 1;
    configuration.inference.second_events_column = 2;
    configuration.inference.second_trials_column = 3;
    const auto page = datalab::application::AnalysisService::two_poisson_rate(
        table, configuration);
    QVERIFY(std::any_of(
        page.diagnostics.cbegin(), page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "analysis";
        }));
}

void PoissonRateTest::interpretationDoesNotClaimPass()
{
    datalab::domain::OutputPage page;
    page.method_name = "1-Sample Poisson Rate";
    datalab::domain::PoissonRateFacts facts;
    facts.kind = "one_sample";
    facts.events = 2;
    facts.exposure = 10.0;
    facts.rate = 0.2;
    facts.hypothesized = 0.5;
    facts.method = "exact";
    facts.p_value = 0.124672;
    facts.assumption_status = "not_verified";
    page.facts.poisson_rate = facts;
    datalab::application::InterpretationService::enrich(page);
    QVERIFY(!page.interpretation.empty());
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
            QVERIFY(bullet.find("过程合格") == std::string::npos);
        }
    }
}

void PoissonRateTest::rateRatioLogWaldMatchesWorkedExample()
{
    // # source: formula_reference — x1=8,t1=10,x2=4,t2=10 → ρ=2
    // SE=sqrt(1/8+1/4)=sqrt(0.375); z=log(2)/SE
    const auto result = datalab::domain::statistics::two_poisson_rate_test(
        8, 10.0, 4, 10.0, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::normal,
        "ratio");
    QVERIFY(result.ratio.has_value());
    QVERIFY(std::abs(*result.ratio - 2.0) < 1.0e-12);
    QCOMPARE(result.comparison, std::string{"ratio"});
    QCOMPARE(result.ci_method, std::string{"log_wald"});
    QVERIFY(result.z_statistic.has_value());
    const double expected_z = std::log(2.0) / std::sqrt(0.375);
    QVERIFY(std::abs(*result.z_statistic - expected_z) < 1.0e-12);
    QVERIFY(result.confidence_lower.has_value());
    QVERIFY(result.confidence_upper.has_value());
    QVERIFY(*result.confidence_lower < 2.0);
    QVERIFY(*result.confidence_upper > 2.0);

    datalab::domain::DataTable table;
    table.columns = {"D1", "T1", "D2", "T2"};
    table.rows = {{"8", "10", "4", "10"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.inference.first_events_column = 0;
    configuration.inference.first_trials_column = 1;
    configuration.inference.second_events_column = 2;
    configuration.inference.second_trials_column = 3;
    configuration.inference.rate_comparison = "ratio";
    configuration.inference.proportion_method = "normal";
    const auto page = datalab::application::AnalysisService::two_poisson_rate(
        table, configuration);
    QVERIFY(page.facts.poisson_rate.has_value());
    QCOMPARE(page.facts.poisson_rate->comparison, std::string{"ratio"});
    QVERIFY(page.facts.poisson_rate->ratio.has_value());
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title == "率比置信区间";
                        }));
}

void PoissonRateTest::rateRatioZeroEventsDiagnosed()
{
    const auto result = datalab::domain::statistics::two_poisson_rate_test(
        0, 10.0, 5, 10.0, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::normal,
        "ratio");
    QVERIFY(std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "zero_events_for_rate_ratio";
        }));
    QVERIFY(!result.confidence_lower.has_value());
}

QTEST_APPLESS_MAIN(PoissonRateTest)

#include "poisson_rate_test.moc"
