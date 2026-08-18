#include "domain/quality_diagnostics.h"
#include "domain/quality_types.h"
#include "domain/statistics/attribute_agreement.h"
#include "domain/statistics/control_charts.h"
#include "domain/statistics/gage_rr.h"
#include "domain/statistics/inference_extensions.h"
#include "domain/statistics/msa_type1.h"
#include "domain/statistics/nested_gage_rr.h"
#include "domain/statistics/normality_test.h"
#include "domain/statistics/process_capability.h"
#include "domain/statistics/regression.h"
#include "domain/statistics/reliability.h"
#include "domain/statistics/two_factor_anova.h"
#include "infrastructure/output_serialization.h"

#include <QtTest/QtTest>
#include <QJsonObject>

#include <cmath>
#include <limits>
#include <string>
#include <vector>

using datalab::domain::SpecificationLimits;
using datalab::domain::has_diagnostic_code;
using datalab::domain::statistics::ProcessCapability;

class QualityDiagnosticsTest final : public QObject {
    Q_OBJECT

private slots:
    void capabilityBilateralAndOneSided();
    void capabilityRejectsInvalidLimits();
    void capabilityIgnoresEqualToSpec();
    void normalityDecisionsAndSampleSize();
    void anovaDoesNotFabricateF();
    void regressionSourceRowsAndErrorDf();
    void tukeyIntervalContainsZeroIsNotSignificant();
    void gageToleranceAndNdc();
    void gageReportsInteractionAndRawVariance();
    void type1ZeroRepeatability();
    void kappaUnidentifiableAndAlpha();
    void reliabilityEventEncodingAndKm();
    void xbarRejectsUnequalSubgroups();
    void oldJsonKeepsSafeDefaults();
};

void QualityDiagnosticsTest::capabilityBilateralAndOneSided()
{
    const auto bilateral = ProcessCapability::calculate(
        5.0, 1.0, 2.0, SpecificationLimits{0.0, 10.0, 5.0});
    QVERIFY(bilateral.cp.has_value());
    QVERIFY(bilateral.cpk.has_value());
    QCOMPARE(bilateral.specification_mode, std::string{"bilateral"});
    QVERIFY(has_diagnostic_code(bilateral.diagnostics, "assumption_not_verified"));

    const auto lower_only = ProcessCapability::calculate(
        {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, 1.0,
        SpecificationLimits{0.0, std::nullopt, std::nullopt});
    QVERIFY(lower_only.cpl.has_value());
    QVERIFY(!lower_only.cpu.has_value());
    QVERIFY(!lower_only.cp.has_value());
    QCOMPARE(lower_only.specification_mode, std::string{"lower_only"});
    QCOMPARE(lower_only.evidence.valid_count, std::size_t{8});

    const auto upper_only = ProcessCapability::calculate(
        {1.0, 2.0, 3.0}, 1.0, SpecificationLimits{std::nullopt, 10.0, std::nullopt});
    QVERIFY(upper_only.cpu.has_value());
    QVERIFY(!upper_only.cpl.has_value());
    QVERIFY(!upper_only.cpm.has_value());
}

void QualityDiagnosticsTest::capabilityRejectsInvalidLimits()
{
    const auto equal = ProcessCapability::calculate(
        5.0, 1.0, 1.0, SpecificationLimits{10.0, 10.0, std::nullopt});
    QVERIFY(has_diagnostic_code(equal.diagnostics, "invalid_specification"));
    QVERIFY(!equal.cp.has_value());

    const auto nan_spec = ProcessCapability::calculate(
        5.0, 1.0, 1.0,
        SpecificationLimits{std::numeric_limits<double>::quiet_NaN(), 10.0, std::nullopt});
    QVERIFY(has_diagnostic_code(nan_spec.diagnostics, "invalid_specification"));

    const auto inf_target = ProcessCapability::calculate(
        5.0, 1.0, 1.0,
        SpecificationLimits{0.0, 10.0, std::numeric_limits<double>::infinity()});
    QVERIFY(has_diagnostic_code(inf_target.diagnostics, "invalid_target"));

    const auto zero_sigma = ProcessCapability::calculate(
        5.0, 0.0, 0.0, SpecificationLimits{0.0, 10.0, 5.0});
    QVERIFY(has_diagnostic_code(zero_sigma.diagnostics, "invalid_within_sigma"));
    QVERIFY(!zero_sigma.cp.has_value());
}

void QualityDiagnosticsTest::capabilityIgnoresEqualToSpec()
{
    const auto result = ProcessCapability::calculate(
        {0.0, 5.0, 10.0}, 1.0, SpecificationLimits{0.0, 10.0, std::nullopt});
    QCOMPARE(result.sample_size, std::size_t{3});
    QVERIFY(result.observed_ppm_total.has_value());
    QCOMPARE(*result.observed_ppm_total, 0.0);

    const auto with_missing = ProcessCapability::calculate(
        {1.0, std::numeric_limits<double>::quiet_NaN(), 2.0, 3.0},
        1.0, SpecificationLimits{0.0, 10.0, std::nullopt});
    QCOMPARE(with_missing.evidence.valid_count, std::size_t{3});
    QCOMPARE(with_missing.evidence.missing_count, std::size_t{1});
}

void QualityDiagnosticsTest::normalityDecisionsAndSampleSize()
{
    const auto n2 = datalab::domain::statistics::normality_test({1.0, 2.0});
    QCOMPARE(n2.decision, std::string{"not_computed"});
    QVERIFY(has_diagnostic_code(n2.messages, "insufficient_data"));

    const auto n3 = datalab::domain::statistics::normality_test({1.0, 2.0, 3.0});
    QVERIFY(n3.p_value.has_value());
    QVERIFY(n3.adjusted_anderson_darling.has_value());
    QVERIFY(n3.sample_size_warning);
    QVERIFY(*n3.p_value >= 0.0 && *n3.p_value <= 1.0);

    const auto n8 = datalab::domain::statistics::normality_test(
        {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0});
    QVERIFY(!n8.sample_size_warning);
    QVERIFY(n8.decision == "reject" || n8.decision == "fail_to_reject");

    const auto constant = datalab::domain::statistics::normality_test({2.0, 2.0, 2.0, 2.0});
    QCOMPARE(constant.decision, std::string{"not_computed"});
    QVERIFY(has_diagnostic_code(constant.messages, "zero_variance"));
}

void QualityDiagnosticsTest::anovaDoesNotFabricateF()
{
    datalab::domain::statistics::TwoFactorAnovaInput missing_cell{
        {"A1", "A1", "A2"},
        {"B1", "B2", "B1"},
        {10.0, 12.0, 11.0}};
    const auto anova = datalab::domain::statistics::two_factor_anova(missing_cell);
    QVERIFY(!anova.effects.empty());
    for (const auto& effect : anova.effects) {
        if (!effect.estimable) {
            QVERIFY(!effect.f_statistic.has_value());
            QVERIFY(!effect.p_value.has_value());
        }
    }
}

void QualityDiagnosticsTest::regressionSourceRowsAndErrorDf()
{
    const auto ok = datalab::domain::statistics::fit_linear_regression(
        {3.0, 5.0, 7.0, 9.0, 11.0},
        {{1.0}, {2.0}, {3.0}, {4.0}, {5.0}},
        {"X"}, 0.95, {10, 11, 12, 13, 14});
    QCOMPARE(ok.observations.size(), std::size_t{5});
    QCOMPARE(ok.observations.front().source_row, std::size_t{10});
    QVERIFY(std::abs(ok.observations.front().internally_standardized_residual
                   - ok.observations.front().studentized_residual) > 1.0e-12);
    QVERIFY(std::abs(ok.observations.front().studentized_residual
                   - ok.observations.front().deleted_studentized_residual) > 1.0e-12);
    QVERIFY(!ok.diagnostics_summary.residual_vs_fitted_x.empty());

    const auto no_df = datalab::domain::statistics::fit_linear_regression(
        {1.0, 3.0}, {{1.0}, {2.0}}, {"X"});
    QVERIFY(has_diagnostic_code(no_df.diagnostics, "insufficient_data")
            || has_diagnostic_code(no_df.diagnostics, "no_error_degrees_of_freedom"));
}

void QualityDiagnosticsTest::tukeyIntervalContainsZeroIsNotSignificant()
{
    const auto tukey = datalab::domain::statistics::tukey_multiple_comparisons(
        {{1.0, 1.1, 0.9}, {1.05, 0.95, 1.0}}, {"A", "B"}, 0.95);
    QCOMPARE(tukey.method.find("approximation") != std::string::npos, true);
    QVERIFY(!tukey.comparisons.empty());
    QCOMPARE(tukey.comparisons.front().significant, false);
}

void QualityDiagnosticsTest::gageToleranceAndNdc()
{
    const auto invalid = datalab::domain::statistics::crossed_gage_rr(
        {10.0, 10.1, 10.2, 11.0}, {"P1", "P1", "P2", "P2"}, {"A", "B", "A", "B"},
        std::numeric_limits<double>::quiet_NaN());
    QVERIFY(has_diagnostic_code(invalid.diagnostics, "invalid_tolerance"));

    const auto zero_tol = datalab::domain::statistics::crossed_gage_rr(
        {10.0, 10.2, 10.1, 11.0, 11.1, 10.9, 12.0, 12.1, 11.9,
         10.0, 10.1, 10.2, 11.0, 11.2, 11.1, 12.0, 12.2, 12.1},
        {"P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3",
         "P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"},
        {"A", "A", "A", "A", "A", "A", "A", "A", "A",
         "B", "B", "B", "B", "B", "B", "B", "B", "B"},
        0.0);
    QVERIFY(!zero_tol.variance_components.empty());
    QCOMPARE(zero_tol.variance_components.front().percent_tolerance_available, false);
    if (zero_tol.ndc_available) {
        QVERIFY(zero_tol.ndc >= 1.0);
    }
}

void QualityDiagnosticsTest::gageReportsInteractionAndRawVariance()
{
    const auto gage = datalab::domain::statistics::crossed_gage_rr(
        {10.0, 10.2, 10.1, 11.0, 11.1, 10.9, 12.0, 12.1, 11.9,
         10.0, 10.1, 10.2, 11.0, 11.2, 11.1, 12.0, 12.2, 12.1},
        {"P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3",
         "P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"},
        {"A", "A", "A", "A", "A", "A", "A", "A", "A",
         "B", "B", "B", "B", "B", "B", "B", "B", "B"},
        6.0);
    QCOMPARE(gage.interaction_retained, true);
    QVERIFY(gage.interaction_p_value.has_value());
    bool found_raw = false;
    for (const auto& component : gage.variance_components) {
        if (component.truncated) {
            QVERIFY(component.raw_variance_component < 0.0);
            QCOMPARE(component.variance_component, 0.0);
        }
        if (component.source == "Total Gage R&R") {
            found_raw = true;
        }
    }
    QVERIFY(found_raw);
    QVERIFY(!gage.rules.empty());
}

void QualityDiagnosticsTest::type1ZeroRepeatability()
{
    const auto zero = datalab::domain::statistics::msa_type1({10.0, 10.0, 10.0}, 9.0, 1.0);
    QVERIFY(has_diagnostic_code(zero.diagnostics, "zero_repeatability"));
    QCOMPARE(zero.inference_available, false);
    QCOMPARE(zero.p_value, 1.0);
}

void QualityDiagnosticsTest::kappaUnidentifiableAndAlpha()
{
    const auto same = datalab::domain::statistics::attribute_agreement(
        {"Pass", "Pass", "Pass", "Pass", "Pass", "Pass"},
        {"P1", "P2", "P3", "P1", "P2", "P3"},
        {"A", "A", "A", "B", "B", "B"}, {}, 0.90);
    QVERIFY(!same.between_evaluator.empty());
    QCOMPARE(same.between_evaluator.front().estimate.identifiable, false);

    const auto unbalanced = datalab::domain::statistics::attribute_agreement(
        {"Pass", "Fail", "Pass", "Pass", "Fail"},
        {"P1", "P1", "P2", "P1", "P2"},
        {"A", "A", "A", "B", "B"});
    QVERIFY(has_diagnostic_code(unbalanced.diagnostics, "unbalanced_replicates"));
}

void QualityDiagnosticsTest::reliabilityEventEncodingAndKm()
{
    QVERIFY(datalab::domain::statistics::parse_reliability_event("fail").value());
    QVERIFY(!datalab::domain::statistics::parse_reliability_event("censored").value());
    QVERIFY(!datalab::domain::statistics::parse_reliability_event("yes").has_value());

    const auto km = datalab::domain::statistics::kaplan_meier(
        {1.0, 2.0, 2.0, 4.0}, {true, true, false, true}, 0.95, {7, 8, 9, 10});
    QVERIFY(!km.points.empty());
    QCOMPARE(km.failure_count, std::size_t{3});
    QVERIFY(!km.points.front().source_rows.empty());

    const auto all_censored = datalab::domain::statistics::kaplan_meier(
        {1.0, 2.0}, {false, false});
    QCOMPARE(all_censored.not_computed_reason, std::string{"all_censored"});
}

void QualityDiagnosticsTest::xbarRejectsUnequalSubgroups()
{
    const auto unequal = datalab::domain::statistics::ControlCharts::xbar_range_dual(
        {{1.0, 2.0}, {3.0, 4.0, 5.0}});
    QVERIFY(has_diagnostic_code(unequal.diagnostics, "unbalanced_design"));
}

void QualityDiagnosticsTest::oldJsonKeepsSafeDefaults()
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), QStringLiteral("legacy"));
    object.insert(QStringLiteral("title"), QStringLiteral("旧页"));
    object.insert(QStringLiteral("method_name"), QStringLiteral("Normal Capability Analysis"));
    const auto page = datalab::infrastructure::output_page_from_json(object);
    QCOMPARE(page.method_metadata.assumption_status, std::string{"not_verified"});
    QCOMPARE(page.method_metadata.valid_count, std::size_t{0});
    QVERIFY(page.method_metadata.not_computed_reason.empty());
}

QTEST_MAIN(QualityDiagnosticsTest)
#include "quality_diagnostics_test.moc"
