#include "domain/quality_diagnostics.h"
#include "domain/statistics/analysis_rules.h"
#include "domain/statistics/gage_rr.h"
#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/inference_extensions.h"
#include "domain/statistics/regression.h"
#include "domain/statistics/reliability.h"
#include "domain/statistics/two_factor_anova.h"

#include <QtTest/QtTest>

#include <optional>
#include <string>
#include <vector>

class AnalysisRuleEvidenceTest final : public QObject {
    Q_OBJECT

private slots:
    void catalogsAreNonEmpty();
    void regressionExposesAssumptionsAndInfluenceRules();
    void anovaDoesNotFabricateUnestimableTerms();
    void tukeyReportsFamilyConfidence();
    void gageKeepsInteractionAndNdcEvidence();
    void reliabilityRejectsUnknownEventsAndKeepsRiskSet();
};

void AnalysisRuleEvidenceTest::catalogsAreNonEmpty()
{
    QVERIFY(!datalab::domain::statistics::regression_rule_catalog().empty());
    QVERIFY(!datalab::domain::statistics::anova_rule_catalog().empty());
    QVERIFY(!datalab::domain::statistics::msa_rule_catalog().empty());
    QVERIFY(!datalab::domain::statistics::reliability_rule_catalog().empty());
}

void AnalysisRuleEvidenceTest::regressionExposesAssumptionsAndInfluenceRules()
{
    const auto result = datalab::domain::statistics::fit_linear_regression(
        {3.0, 5.0, 7.0, 9.0, 11.0, 40.0},
        {{1.0}, {2.0}, {3.0}, {4.0}, {5.0}, {6.0}},
        {"X"}, 0.95, {10, 11, 12, 13, 14, 15});
    QVERIFY(!result.diagnostics_summary.assumptions.empty());
    QVERIFY(!result.diagnostics_summary.rules.empty());
    QCOMPARE(result.observations.front().source_row, std::size_t{10});
    const auto facts = datalab::domain::statistics::regression_facts_from(result);
    QVERIFY(facts.error_degrees_of_freedom.has_value());
    QVERIFY(*facts.error_degrees_of_freedom > 0.0);
    QVERIFY(!facts.rules.empty());

    const auto collinear = datalab::domain::statistics::fit_linear_regression(
        {1.0, 2.0, 3.0, 4.0},
        {{1.0, 2.0}, {2.0, 4.0}, {3.0, 6.0}, {4.0, 8.0}},
        {"X", "TwoX"});
    QVERIFY(datalab::domain::has_diagnostic_code(
        collinear.diagnostics, "rank_deficient_design"));
    QVERIFY(!collinear.diagnostics_summary.rules.empty());
}

void AnalysisRuleEvidenceTest::anovaDoesNotFabricateUnestimableTerms()
{
    datalab::domain::statistics::TwoFactorAnovaInput missing_cell{
        {"A1", "A1", "A2"},
        {"B1", "B2", "B1"},
        {10.0, 12.0, 11.0},
        datalab::domain::statistics::AnovaFactorEncoding::reference,
        {3, 4, 5}};
    const auto anova = datalab::domain::statistics::two_factor_anova(missing_cell);
    QVERIFY(!anova.effects.empty());
    QVERIFY(!anova.rules.empty());
    for (const auto& effect : anova.effects) {
        if (!effect.estimable) {
            QVERIFY(!effect.f_statistic.has_value());
            QVERIFY(!effect.p_value.has_value());
        }
    }
    const auto facts = datalab::domain::statistics::two_factor_anova_facts_from(anova);
    QVERIFY(facts.not_estimable_term_count > 0 || !facts.estimable
            || anova.error_degrees_of_freedom == 0);

    const auto one_way = datalab::domain::statistics::one_way_anova(
        {{1.0, 2.0, 3.0}, {2.0, 3.0, 4.0}, {8.0, 9.0, 10.0}},
        {"A", "B", "C"});
    QVERIFY(one_way.p_value.has_value());
    QVERIFY(one_way.levene_p_value.has_value() || !one_way.assumptions.empty());
    QVERIFY(!one_way.residuals.empty());
}

void AnalysisRuleEvidenceTest::tukeyReportsFamilyConfidence()
{
    const auto tukey = datalab::domain::statistics::tukey_multiple_comparisons(
        {{1.0, 1.1, 0.9}, {1.05, 0.95, 1.0}, {5.0, 5.1, 4.9}},
        {"A", "B", "C"}, 0.95);
    QCOMPARE(tukey.family_confidence_level, 0.95);
    QVERIFY(!tukey.rules.empty());
    bool has_significant = false;
    for (const auto& comparison : tukey.comparisons) {
        if (comparison.significant) {
            has_significant = true;
            QVERIFY(comparison.confidence_lower > 0.0
                    || comparison.confidence_upper < 0.0);
        }
    }
    QVERIFY(has_significant);
}

void AnalysisRuleEvidenceTest::gageKeepsInteractionAndNdcEvidence()
{
    const auto gage = datalab::domain::statistics::crossed_gage_rr(
        {10.0, 10.2, 10.1, 11.0, 11.1, 10.9, 12.0, 12.1, 11.9,
         10.0, 10.1, 10.2, 11.0, 11.2, 11.1, 12.0, 12.2, 12.1},
        {"P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3",
         "P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"},
        {"A", "A", "A", "A", "A", "A", "A", "A", "A",
         "B", "B", "B", "B", "B", "B", "B", "B", "B"},
        5.0);
    QCOMPARE(gage.design_balanced, true);
    QCOMPARE(gage.interaction_retained, true);
    QVERIFY(gage.interaction_p_value.has_value());
    QVERIFY(gage.variance_components.size() >= 7);
    bool has_operator_part = false;
    for (const auto& component : gage.variance_components) {
        if (component.source == "Operator * Part") {
            has_operator_part = true;
        }
    }
    QVERIFY(has_operator_part);
    const auto facts = datalab::domain::statistics::gage_rr_facts_from(gage);
    QVERIFY(!facts.rules.empty());
    QVERIFY(facts.ndc_available);
}

void AnalysisRuleEvidenceTest::reliabilityRejectsUnknownEventsAndKeepsRiskSet()
{
    QVERIFY(!datalab::domain::statistics::parse_reliability_event("maybe").has_value());
    const auto km = datalab::domain::statistics::kaplan_meier(
        {1.0, 2.0, 2.0, 4.0, 5.0},
        {true, true, false, true, false},
        0.95, {7, 8, 9, 10, 11});
    QVERIFY(!km.points.empty());
    QCOMPARE(km.points[1].failures, std::size_t{1});
    QCOMPARE(km.points[1].censored, std::size_t{1});
    QVERIFY(!km.points.front().source_rows.empty());
    QVERIFY(!km.rules.empty());
    const auto facts = datalab::domain::statistics::kaplan_meier_facts_from(km);
    QCOMPARE(facts.failure_count, std::optional<std::size_t>(3));
    QCOMPARE(facts.censored_count, std::optional<std::size_t>(2));

    const auto all_censored = datalab::domain::statistics::kaplan_meier(
        {1.0, 2.0}, {false, false});
    QCOMPARE(all_censored.not_computed_reason, std::string{"all_censored"});
    QVERIFY(!all_censored.rules.empty());
}

QTEST_MAIN(AnalysisRuleEvidenceTest)
#include "analysis_rule_evidence_test.moc"
