#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/equivalence_test.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

class EquivalenceTest final : public QObject {
    Q_OBJECT

private slots:
    void oneSampleTostMatchesWorkedExample();
    void diagnosesReversedLimits();
    void welchAndPooledDifferInSeAndDf();
    void pairedTostMatchesWorkedExample();
    void servicePageHasIntervalPlot();
    void pairedServiceUsesCompleteCasePairs();
    void interpretationStatesLimitsOnly();
    void oneProportionZTostMatchesWorkedExample();
    void twoProportionZTostServiceCompleteCase();
    void twoSampleRatioTostMatchesWorkedExample();
    void twoSampleRatioDiagnosesNonpositiveReference();
    void twoSampleRatioServicePageUsesRatioAxis();
};

void EquivalenceTest::oneSampleTostMatchesWorkedExample()
{
    // # source: formula_reference — n=5, mean=12, s^2=2.5, target=12, δ=±2, α=0.05
    const std::vector<double> observations{10.0, 11.0, 12.0, 13.0, 14.0};
    const auto result = datalab::domain::statistics::one_sample_equivalence_test(
        observations, 12.0, -2.0, 2.0, 0.95);
    QCOMPARE(result.first_count, std::size_t{5});
    QVERIFY(std::abs(result.difference) < 1.0e-12);
    const double expected_t = 2.0 / std::sqrt(0.5);
    QVERIFY(std::abs(result.t_lower - expected_t) < 1.0e-12);
    QVERIFY(std::abs(result.t_upper + expected_t) < 1.0e-12);
    QVERIFY(result.confidence_lower.has_value());
    QVERIFY(result.confidence_upper.has_value());
    const double expected_half_width = 2.132 * std::sqrt(0.5);
    QVERIFY(std::abs(*result.confidence_upper - expected_half_width) < 5.0e-4);
    QVERIFY(std::abs(*result.confidence_lower + expected_half_width) < 5.0e-4);
    QVERIFY(result.within_limits);
    QVERIFY(result.p_lower.has_value());
    QVERIFY(result.p_upper.has_value());
    QVERIFY(*result.p_lower <= result.alpha);
    QVERIFY(*result.p_upper <= result.alpha);
    QVERIFY(result.both_pvalues_below_alpha);
    QCOMPARE(result.ci_method, std::string("tost_1_minus_alpha"));
}

void EquivalenceTest::diagnosesReversedLimits()
{
    const std::vector<double> observations{10.0, 11.0, 12.0, 13.0, 14.0};
    const auto result = datalab::domain::statistics::one_sample_equivalence_test(
        observations, 12.0, 2.0, -2.0, 0.95);
    QVERIFY(!result.within_limits);
    QVERIFY(!result.confidence_lower.has_value());
    QVERIFY(std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "invalid_equivalence_limits";
        }));
}

void EquivalenceTest::welchAndPooledDifferInSeAndDf()
{
    const std::vector<double> first{1.0, 2.0, 3.0};
    const std::vector<double> second{10.0, 20.0, 40.0};
    const auto welch = datalab::domain::statistics::two_sample_equivalence_test(
        first, second, -5.0, 5.0, 0.95,
        datalab::domain::statistics::VarianceMethod::welch);
    const auto pooled = datalab::domain::statistics::two_sample_equivalence_test(
        first, second, -5.0, 5.0, 0.95,
        datalab::domain::statistics::VarianceMethod::pooled);
    QVERIFY(std::abs(welch.standard_error - pooled.standard_error) > 1.0e-12);
    QVERIFY(std::abs(welch.degrees_of_freedom - pooled.degrees_of_freedom) > 1.0e-12);
}

void EquivalenceTest::pairedTostMatchesWorkedExample()
{
    // # source: formula_reference — differences {1,1,2,1}, dbar=1.25, s^2=0.25, SE=0.25
    const std::vector<double> first{11.0, 13.0, 15.0, 16.0};
    const std::vector<double> second{10.0, 12.0, 13.0, 15.0};
    const auto result = datalab::domain::statistics::paired_equivalence_test(
        first, second, 0.5, 2.0, 0.95);
    QCOMPARE(result.kind, std::string{"paired"});
    QCOMPARE(result.first_count, std::size_t{4});
    QVERIFY(std::abs(result.difference - 1.25) < 1.0e-12);
    QVERIFY(std::abs(result.standard_error - 0.25) < 1.0e-12);
    QVERIFY(std::abs(result.t_lower - 3.0) < 1.0e-12);
    QVERIFY(std::abs(result.t_upper + 3.0) < 1.0e-12);
    QVERIFY(result.both_pvalues_below_alpha);
    QVERIFY(result.within_limits);
    QCOMPARE(result.ci_method, std::string("tost_1_minus_alpha"));
}

void EquivalenceTest::servicePageHasIntervalPlot()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    table.rows = {{"10"}, {"11"}, {"12"}, {"13"}, {"14"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.selection.measurement_column = 0;
    configuration.inference.hypothesis_mean = 12.0;
    configuration.inference.equivalence_lower = -2.0;
    configuration.inference.equivalence_upper = 2.0;
    configuration.inference.confidence_level = 0.95;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::one_sample_equivalence(
            table, configuration);
    QVERIFY(std::any_of(page.tables.cbegin(), page.tables.cend(),
                        [](const datalab::domain::StatisticTable& table_out) {
                            return table_out.title == "等价性检验";
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::interval
                                && !plot.interval_lower.empty()
                                && !plot.interval_upper.empty();
                        }));
    QVERIFY(page.facts.equivalence.has_value());
    QVERIFY(page.facts.equivalence->within_limits);
}

void EquivalenceTest::pairedServiceUsesCompleteCasePairs()
{
    datalab::domain::DataTable table;
    table.columns = {"X", "Y"};
    table.rows = {{"11", "10"}, {"13", "12"}, {"15", "*"}, {"16", "15"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0, 1};
    configuration.inference.equivalence_lower = 0.0;
    configuration.inference.equivalence_upper = 2.5;
    configuration.inference.confidence_level = 0.95;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::paired_equivalence(table, configuration);
    QVERIFY(page.facts.equivalence.has_value());
    QCOMPARE(page.facts.equivalence->kind, std::string{"paired"});
    QVERIFY(std::any_of(page.diagnostics.cbegin(), page.diagnostics.cend(),
                        [](const datalab::domain::DiagnosticMessage& diagnostic) {
                            return diagnostic.code == "missing_values";
                        }));
    QVERIFY(std::any_of(page.tables.cbegin(), page.tables.cend(),
                        [](const datalab::domain::StatisticTable& table_out) {
                            return table_out.title == "描述统计"
                                && !table_out.rows.empty()
                                && table_out.rows.front().front() == "配对差值";
                        }));
}

void EquivalenceTest::interpretationStatesLimitsOnly()
{
    datalab::domain::OutputPage page;
    page.method_name = "1-Sample Equivalence Test";
    datalab::domain::EquivalenceFacts facts;
    facts.kind = "one_sample";
    facts.difference = 0.0;
    facts.lower = -2.0;
    facts.upper = 2.0;
    facts.ci_lower = -1.5;
    facts.ci_upper = 1.5;
    facts.within_limits = true;
    facts.assumption_status = "not_verified";
    page.facts.equivalence = facts;
    datalab::application::InterpretationService::enrich(page);
    QVERIFY(!page.interpretation.empty());
    bool mentions_limits = false;
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
            if (bullet.find("界限内") != std::string::npos) {
                mentions_limits = true;
            }
        }
    }
    QVERIFY(mentions_limits);
}

void EquivalenceTest::oneProportionZTostMatchesWorkedExample()
{
    // # source: formula_reference — x=50,n=100,p0=0.5,δ=±0.1,α=0.05
    // SE=sqrt(0.25/100)=0.05; z_L=2; z_U=-2; CI half-width = z_0.95*0.05
    const auto result = datalab::domain::statistics::one_proportion_equivalence_test(
        50, 100, 0.5, -0.1, 0.1, 0.95);
    QCOMPARE(result.kind, std::string{"one_proportion"});
    QCOMPARE(result.ci_method, std::string{"wald_z_tost"});
    QVERIFY(std::abs(result.difference) < 1.0e-12);
    QVERIFY(std::abs(result.standard_error - 0.05) < 1.0e-12);
    QVERIFY(std::abs(result.t_lower - 2.0) < 1.0e-12);
    QVERIFY(std::abs(result.t_upper + 2.0) < 1.0e-12);
    QVERIFY(result.p_lower.has_value());
    QVERIFY(result.p_upper.has_value());
    QVERIFY(*result.p_lower < 0.025);
    QVERIFY(*result.p_upper < 0.025);
    QVERIFY(result.within_limits);
    QVERIFY(result.confidence_lower.has_value());
    QVERIFY(result.confidence_upper.has_value());
    const double half = 1.6448536269514722 * 0.05;
    QVERIFY(std::abs(*result.confidence_upper - half) < 1.0e-6);
    QVERIFY(std::abs(*result.confidence_lower + half) < 1.0e-6);
}

void EquivalenceTest::twoProportionZTostServiceCompleteCase()
{
    datalab::domain::DataTable table;
    table.columns = {"E1", "N1", "E2", "N2"};
    table.rows = {
        {"20", "40", "18", "40"},
        {"10", "20", "*", "20"},
        {"20", "40", "22", "40"},
    };
    datalab::domain::AnalysisConfiguration configuration;
    configuration.inference.first_events_column = 0;
    configuration.inference.first_trials_column = 1;
    configuration.inference.second_events_column = 2;
    configuration.inference.second_trials_column = 3;
    configuration.inference.equivalence_lower = -0.2;
    configuration.inference.equivalence_upper = 0.2;
    configuration.inference.confidence_level = 0.95;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::two_proportion_equivalence(
            table, configuration);
    QVERIFY(page.facts.equivalence.has_value());
    QCOMPARE(page.facts.equivalence->kind, std::string{"two_proportion"});
    QCOMPARE(page.facts.equivalence->ci_method, std::string{"wald_z_tost"});
    QVERIFY(std::any_of(page.tables.cbegin(), page.tables.cend(),
                        [](const datalab::domain::StatisticTable& table_out) {
                            return table_out.title == "描述统计"
                                && !table_out.headers.empty()
                                && table_out.headers[1] == "事件数";
                        }));
    QVERIFY(std::any_of(page.diagnostics.cbegin(), page.diagnostics.cend(),
                        [](const datalab::domain::DiagnosticMessage& diagnostic) {
                            return diagnostic.code == "missing_values";
                        }));
}

void EquivalenceTest::twoSampleRatioTostMatchesWorkedExample()
{
    // # source: formula_reference — test {8,10,12}, ref {9,10,11}, ρ̂=1
    // s1=2, s2=1, n1=n2=3; δ1=0.8, δ2=1.25, α=0.05 Welch
    const std::vector<double> test_sample{8.0, 10.0, 12.0};
    const std::vector<double> reference{9.0, 10.0, 11.0};
    const auto result = datalab::domain::statistics::two_sample_equivalence_ratio_test(
        test_sample, reference, 0.8, 1.25, 0.95,
        datalab::domain::statistics::VarianceMethod::welch);
    QCOMPARE(result.kind, std::string{"two_sample_ratio"});
    QCOMPARE(result.ci_method, std::string{"tost_ratio_1_minus_alpha"});
    QVERIFY(std::abs(result.difference - 1.0) < 1.0e-12);
    const double se_lower = std::sqrt(4.0 / 3.0 + 0.64 / 3.0);
    const double se_upper = std::sqrt(4.0 / 3.0 + 1.5625 / 3.0);
    QVERIFY(std::abs(result.t_lower - (10.0 - 0.8 * 10.0) / se_lower) < 1.0e-9);
    QVERIFY(std::abs(result.t_upper - (10.0 - 1.25 * 10.0) / se_upper) < 1.0e-9);
    QVERIFY(result.p_lower.has_value());
    QVERIFY(result.p_upper.has_value());
    QVERIFY(result.confidence_lower.has_value());
    QVERIFY(result.confidence_upper.has_value());
    QVERIFY(*result.confidence_lower < 1.0);
    QVERIFY(*result.confidence_upper > 1.0);
}

void EquivalenceTest::twoSampleRatioDiagnosesNonpositiveReference()
{
    const std::vector<double> test_sample{8.0, 10.0, 12.0};
    const std::vector<double> reference{-1.0, 0.0, 1.0};
    const auto result = datalab::domain::statistics::two_sample_equivalence_ratio_test(
        test_sample, reference, 0.8, 1.25, 0.95);
    QVERIFY(std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "nonpositive_reference_mean";
        }));
    QVERIFY(!result.confidence_lower.has_value());
}

void EquivalenceTest::twoSampleRatioServicePageUsesRatioAxis()
{
    datalab::domain::DataTable table;
    table.columns = {"Test", "Ref"};
    table.rows = {{"8", "9"}, {"10", "10"}, {"12", "11"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0, 1};
    configuration.inference.equivalence_lower = 0.8;
    configuration.inference.equivalence_upper = 1.25;
    configuration.inference.confidence_level = 0.95;
    configuration.inference.variance_method = "welch";
    datalab::domain::OutputPage page =
        datalab::application::AnalysisService::two_sample_equivalence_ratio(
            table, configuration);
    QVERIFY(page.facts.equivalence.has_value());
    QCOMPARE(page.facts.equivalence->kind, std::string{"two_sample_ratio"});
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::interval
                                && plot.x_axis_title == "比值";
                        }));
    datalab::application::InterpretationService::enrich(page);
    bool mentions_ratio = false;
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("已证明等价") == std::string::npos);
            if (bullet.find("均值比") != std::string::npos) {
                mentions_ratio = true;
            }
        }
    }
    QVERIFY(mentions_ratio);
}

QTEST_APPLESS_MAIN(EquivalenceTest)

#include "equivalence_test.moc"
