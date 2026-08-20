#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/attribute_capability.h"
#include "domain/statistics/inference_extensions.h"
#include "domain/statistics/normal_distribution.h"
#include "domain/statistics/proportion_test.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

class ProportionTest final : public QObject {
    Q_OBJECT

private slots:
    void exactPValueMatchesBinomialExpansion();
    void exactIntervalMatchesClopperPearson();
    void scoreStatisticMatchesWorkedExample();
    void diagnosesInvalidCounts();
    void wilsonIntervalMatchesWorkedExample();
    void wilsonSharesScoreWithNormalButDifferentCi();
    void wilsonZeroEventsLowerBoundIsZero();
    void agrestiCoullIntervalMatchesWorkedExample();
    void agrestiCoullSharesScoreWithNormalButDifferentCi();
    void agrestiCoullZeroEventsLowerBoundIsZero();
    void serviceSumsCompleteCaseRows();
    void twoProportionsSumsIndependentGroups();
    void twoProportionsNewcombeWilsonKeepsWaldZ();
    void interpretationDoesNotClaimPass();
    void twoProportionsInterpretationIsNotOneSample();
};

void ProportionTest::exactPValueMatchesBinomialExpansion()
{
    // # source: formula_reference — Bin(10, 0.5): P(X<=2)=(1+10+45)/1024
    const auto result = datalab::domain::statistics::one_proportion_test(
        2, 10, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::less,
        datalab::domain::statistics::ProportionMethod::exact);
    QVERIFY(result.p_value.has_value());
    QCOMPARE(*result.p_value, 56.0 / 1024.0);
    const auto two_sided = datalab::domain::statistics::one_proportion_test(
        2, 10, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::exact);
    QVERIFY(two_sided.p_value.has_value());
    QCOMPARE(*two_sided.p_value, 2.0 * 56.0 / 1024.0);
    QCOMPARE(result.method, std::string("exact"));
}

void ProportionTest::exactIntervalMatchesClopperPearson()
{
    const auto result = datalab::domain::statistics::one_proportion_test(
        2, 10, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::exact);
    const auto interval = datalab::domain::statistics::clopper_pearson_interval(2.0, 10.0, 0.05);
    QVERIFY(result.confidence_lower.has_value());
    QVERIFY(result.confidence_upper.has_value());
    QCOMPARE(*result.confidence_lower, *interval.lower);
    QCOMPARE(*result.confidence_upper, *interval.upper);
    QVERIFY(*result.confidence_lower < 0.2);
    QVERIFY(*result.confidence_upper > 0.2);
}

void ProportionTest::scoreStatisticMatchesWorkedExample()
{
    // # source: formula_reference — z = (0.2-0.5)/sqrt(0.5*0.5/10)
    const auto result = datalab::domain::statistics::one_proportion_test(
        2, 10, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::normal);
    QVERIFY(result.z_statistic.has_value());
    const double expected = (0.2 - 0.5) / std::sqrt(0.5 * 0.5 / 10.0);
    QVERIFY(std::abs(*result.z_statistic - expected) < 1.0e-12);
    QCOMPARE(result.method, std::string("normal"));
    QVERIFY(std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "small_count_normal_approximation";
        }));
}

void ProportionTest::diagnosesInvalidCounts()
{
    const auto result = datalab::domain::statistics::one_proportion_test(
        6, 5, 0.5);
    QVERIFY(!result.p_value.has_value());
    QVERIFY(std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "invalid_proportion_counts";
        }));
}

void ProportionTest::wilsonIntervalMatchesWorkedExample()
{
    // # source: formula_reference — Wilson score CI for x=2, n=10, z≈1.959964
    const auto result = datalab::domain::statistics::one_proportion_test(
        2, 10, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::wilson);
    const double z = 1.959963984540054;
    const double n = 10.0;
    const double p = 0.2;
    const double z2 = z * z;
    const double denom = 1.0 + z2 / n;
    const double center = (p + z2 / (2.0 * n)) / denom;
    const double half = z * std::sqrt(p * (1.0 - p) / n + z2 / (4.0 * n * n)) / denom;
    QVERIFY(result.confidence_lower.has_value());
    QVERIFY(result.confidence_upper.has_value());
    QVERIFY(std::abs(*result.confidence_lower - (center - half)) < 1.0e-9);
    QVERIFY(std::abs(*result.confidence_upper - (center + half)) < 1.0e-9);
    QCOMPARE(result.method, std::string("wilson"));
    QCOMPARE(result.ci_method, std::string("wilson_score"));
}

void ProportionTest::wilsonSharesScoreWithNormalButDifferentCi()
{
    // # source: formula_reference
    const auto wilson = datalab::domain::statistics::one_proportion_test(
        2, 10, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::wilson);
    const auto normal = datalab::domain::statistics::one_proportion_test(
        2, 10, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::normal);
    QVERIFY(wilson.z_statistic.has_value());
    QVERIFY(normal.z_statistic.has_value());
    QCOMPARE(*wilson.z_statistic, *normal.z_statistic);
    QCOMPARE(*wilson.p_value, *normal.p_value);
    QVERIFY(std::abs(*wilson.confidence_lower - *normal.confidence_lower) > 1.0e-6);
}

void ProportionTest::wilsonZeroEventsLowerBoundIsZero()
{
    // # source: formula_reference — x=0 forces lower bound to 0
    const auto result = datalab::domain::statistics::one_proportion_test(
        0, 20, 0.1, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::wilson);
    QVERIFY(result.confidence_lower.has_value());
    QCOMPARE(*result.confidence_lower, 0.0);
    QVERIFY(result.confidence_upper.has_value());
    QVERIFY(*result.confidence_upper > 0.0);
}

void ProportionTest::agrestiCoullIntervalMatchesWorkedExample()
{
    // # source: formula_reference — Agresti–Coull CI for x=2, n=10, z≈1.959964
    const auto result = datalab::domain::statistics::one_proportion_test(
        2, 10, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::agresti_coull);
    const double z = 1.959963984540054;
    const double n = 10.0;
    const double z2 = z * z;
    const double n_tilde = n + z2;
    const double p_tilde = (2.0 + z2 / 2.0) / n_tilde;
    const double se_tilde = std::sqrt(p_tilde * (1.0 - p_tilde) / n_tilde);
    QVERIFY(result.confidence_lower.has_value());
    QVERIFY(result.confidence_upper.has_value());
    QVERIFY(std::abs(*result.confidence_lower - (p_tilde - z * se_tilde)) < 1.0e-9);
    QVERIFY(std::abs(*result.confidence_upper - (p_tilde + z * se_tilde)) < 1.0e-9);
    QCOMPARE(result.method, std::string("agresti_coull"));
    QCOMPARE(result.ci_method, std::string("agresti_coull"));
}

void ProportionTest::agrestiCoullSharesScoreWithNormalButDifferentCi()
{
    // # source: formula_reference
    const auto ac = datalab::domain::statistics::one_proportion_test(
        2, 10, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::agresti_coull);
    const auto normal = datalab::domain::statistics::one_proportion_test(
        2, 10, 0.5, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::normal);
    QVERIFY(ac.z_statistic.has_value());
    QVERIFY(normal.z_statistic.has_value());
    QCOMPARE(*ac.z_statistic, *normal.z_statistic);
    QCOMPARE(*ac.p_value, *normal.p_value);
    QVERIFY(std::abs(*ac.confidence_lower - *normal.confidence_lower) > 1.0e-6);
}

void ProportionTest::agrestiCoullZeroEventsLowerBoundIsZero()
{
    // # source: formula_reference — x=0 forces lower bound to 0
    const auto result = datalab::domain::statistics::one_proportion_test(
        0, 20, 0.1, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::ProportionMethod::agresti_coull);
    QVERIFY(result.confidence_lower.has_value());
    QCOMPARE(*result.confidence_lower, 0.0);
    QVERIFY(result.confidence_upper.has_value());
    QVERIFY(*result.confidence_upper > 0.0);
}

void ProportionTest::serviceSumsCompleteCaseRows()
{
    datalab::domain::DataTable table;
    table.columns = {"D", "N"};
    table.rows = {{"1", "50"}, {"*", "50"}, {"2", "50"}, {"1", "50"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.inference.first_events_column = 0;
    configuration.inference.first_trials_column = 1;
    configuration.inference.hypothesis_mean = 0.05;
    configuration.inference.confidence_level = 0.95;
    configuration.inference.proportion_method = "exact";
    const auto page = datalab::application::AnalysisService::one_proportion(
        table, configuration);
    QVERIFY(page.facts.proportion.has_value());
    QCOMPARE(page.facts.proportion->events, std::size_t{4});
    QCOMPARE(page.facts.proportion->trials, std::size_t{150});
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

void ProportionTest::twoProportionsSumsIndependentGroups()
{
    datalab::domain::DataTable table;
    table.columns = {"E1", "N1", "E2", "N2"};
    table.rows = {
        {"1", "50", "10", "100"},
        {"2", "50", "*", "*"},
        {"1", "50", "*", "*"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.inference.first_events_column = 0;
    configuration.inference.first_trials_column = 1;
    configuration.inference.second_events_column = 2;
    configuration.inference.second_trials_column = 3;
    configuration.inference.confidence_level = 0.95;
    const auto page = datalab::application::AnalysisService::two_proportions(
        table, configuration);
    QVERIFY(page.facts.proportion.has_value());
    QCOMPARE(page.facts.proportion->kind, std::string("two_sample"));
    QCOMPARE(page.facts.proportion->events, std::size_t{4});
    QCOMPARE(page.facts.proportion->trials, std::size_t{150});
    QCOMPARE(*page.facts.proportion->second_events, std::size_t{10});
    QCOMPARE(*page.facts.proportion->second_trials, std::size_t{100});
    QVERIFY(page.facts.proportion->difference.has_value());
    QVERIFY(!page.plots.empty());
    QCOMPARE(page.plots[0].kind, datalab::domain::PlotKind::interval);
    QCOMPARE(page.plots[0].categories.front(), std::string("p1 - p2"));
    QVERIFY(std::any_of(
        page.diagnostics.cbegin(), page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
                        return diagnostic.code == "summarized_from_multiple_rows";
        }));
}

void ProportionTest::twoProportionsNewcombeWilsonKeepsWaldZ()
{
    // # source: formula_reference — x1=20,n1=50,x2=10,n2=50; Z same; CI Newcombe
    const auto wald = datalab::domain::statistics::two_proportions_test(
        20, 50, 10, 50, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided, false);
    const auto wilson = datalab::domain::statistics::two_proportions_test(
        20, 50, 10, 50, 0.95,
        datalab::domain::statistics::TestAlternative::two_sided, true);
    QCOMPARE(wald.ci_method, std::string("wald"));
    QCOMPARE(wilson.method, std::string("wilson"));
    QCOMPARE(wilson.ci_method, std::string("newcombe_wilson"));
    QVERIFY(std::abs(wald.z_statistic - wilson.z_statistic) < 1.0e-12);
    QVERIFY(wald.p_value.has_value() && wilson.p_value.has_value());
    QVERIFY(std::abs(*wald.p_value - *wilson.p_value) < 1.0e-12);
    QVERIFY(wald.confidence_lower.has_value() && wilson.confidence_lower.has_value());
    QVERIFY(std::abs(*wald.confidence_lower - *wilson.confidence_lower) > 1.0e-6);

    const double z = datalab::domain::statistics::standard_normal_quantile(0.975);
    const auto wilson_one = [&](std::size_t x, std::size_t n) {
        const double nn = static_cast<double>(n);
        const double p = static_cast<double>(x) / nn;
        const double z2 = z * z;
        const double denom = 1.0 + z2 / nn;
        const double center = (p + z2 / (2.0 * nn)) / denom;
        const double half = z * std::sqrt(p * (1.0 - p) / nn + z2 / (4.0 * nn * nn)) / denom;
        return std::pair<double, double>{center - half, center + half};
    };
    const auto [l1, u1] = wilson_one(20, 50);
    const auto [l2, u2] = wilson_one(10, 50);
    const double p1 = 0.4;
    const double p2 = 0.2;
    const double expected_lower =
        (p1 - p2) - std::sqrt((p1 - l1) * (p1 - l1) + (u2 - p2) * (u2 - p2));
    const double expected_upper =
        (p1 - p2) + std::sqrt((u1 - p1) * (u1 - p1) + (p2 - l2) * (p2 - l2));
    QVERIFY(std::abs(*wilson.confidence_lower - expected_lower) < 1.0e-9);
    QVERIFY(std::abs(*wilson.confidence_upper - expected_upper) < 1.0e-9);

    datalab::domain::DataTable table;
    table.columns = {"E1", "N1", "E2", "N2"};
    table.rows = {{"20", "50", "10", "50"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.inference.first_events_column = 0;
    configuration.inference.first_trials_column = 1;
    configuration.inference.second_events_column = 2;
    configuration.inference.second_trials_column = 3;
    configuration.inference.confidence_level = 0.95;
    configuration.inference.proportion_method = "wilson";
    const auto page = datalab::application::AnalysisService::two_proportions(
        table, configuration);
    QVERIFY(page.facts.proportion.has_value());
    QCOMPARE(page.facts.proportion->method, std::string("wilson"));
    QCOMPARE(page.facts.proportion->ci_method, std::string("newcombe_wilson"));
}

void ProportionTest::interpretationDoesNotClaimPass()
{
    datalab::domain::OutputPage page;
    page.method_name = "1 Proportion";
    datalab::domain::ProportionFacts facts;
    facts.events = 2;
    facts.trials = 10;
    facts.proportion = 0.2;
    facts.hypothesized = 0.5;
    facts.method = "exact";
    facts.p_value = 0.109375;
    facts.assumption_status = "not_verified";
    page.facts.proportion = facts;
    datalab::application::InterpretationService::enrich(page);
    QVERIFY(!page.interpretation.empty());
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
            QVERIFY(bullet.find("过程合格") == std::string::npos);
        }
    }
}

void ProportionTest::twoProportionsInterpretationIsNotOneSample()
{
    datalab::domain::OutputPage page;
    page.method_name = "2 Proportions";
    datalab::domain::ProportionFacts facts;
    facts.kind = "two_sample";
    facts.events = 4;
    facts.trials = 150;
    facts.proportion = 4.0 / 150.0;
    facts.second_events = 10;
    facts.second_trials = 100;
    facts.second_proportion = 0.1;
    facts.difference = 4.0 / 150.0 - 0.1;
    facts.method = "normal";
    facts.p_value = 0.2;
    facts.assumption_status = "not_verified";
    page.facts.proportion = facts;
    datalab::application::InterpretationService::enrich(page);
    QVERIFY(!page.interpretation.empty());
    bool saw_two_sample = false;
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
            QVERIFY(bullet.find("单比例") == std::string::npos);
            if (bullet.find("两比例") != std::string::npos) {
                saw_two_sample = true;
            }
        }
    }
    QVERIFY(saw_two_sample);
}

QTEST_APPLESS_MAIN(ProportionTest)

#include "proportion_test.moc"
