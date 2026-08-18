#include "domain/statistics/msa_type1.h"
#include "domain/statistics/reliability.h"
#include "domain/statistics/t_power.h"
#include "application/analysis_service.h"

#include <QtTest/QtTest>

class ReliabilityMsaPowerTest final : public QObject {
    Q_OBJECT
private slots:
    void type1AndStability();
    void kaplanMeierAndLifetimeModels();
    void handlesAllCensoringAndFewFailures();
    void rejectsInvalidReliabilityInput();
    void tPowerAndSampleSize();
    void extendedPowerAndSampleSize();
    void buildsServiceOutputPages();
};

void ReliabilityMsaPowerTest::type1AndStability()
{
    const auto type1 = datalab::domain::statistics::msa_type1(
        {10.0, 10.1, 9.9, 10.0, 10.2}, 10.0, 1.0);
    QCOMPARE(type1.count, std::size_t{5});
    QVERIFY(std::abs(type1.bias) < 0.1);
    QVERIFY(type1.p_value >= 0.0 && type1.p_value <= 1.0);
    QVERIFY(type1.bias_ci_lower < type1.bias);
    QVERIFY(type1.bias_ci_upper > type1.bias);
    QCOMPARE(type1.degrees_of_freedom, 4.0);
    const auto linearity = datalab::domain::statistics::bias_linearity(
        {1.0, 2.0, 3.0, 4.0}, {1.1, 2.2, 3.3, 4.4});
    QVERIFY(linearity.slope > 0.0);
    QVERIFY(linearity.slope_ci_lower < linearity.slope);
    QVERIFY(linearity.slope_ci_upper > linearity.slope);
    const auto stability = datalab::domain::statistics::gage_stability(
        {1.0, 1.1, 1.0, 1.2, 1.1});
    QCOMPARE(stability.values.size(), std::size_t{5});
    QVERIFY(stability.upper_control_limit > stability.center);
}

void ReliabilityMsaPowerTest::kaplanMeierAndLifetimeModels()
{
    const auto km = datalab::domain::statistics::kaplan_meier(
        {1.0, 2.0, 3.0, 4.0}, {true, false, true, true});
    QCOMPARE(km.points.size(), std::size_t{4});
    QVERIFY(km.median_life.has_value());
    const auto weibull = datalab::domain::statistics::fit_weibull(
        {1.0, 2.0, 3.0, 4.0, 5.0}, {true, true, true, true, false});
    QVERIFY(weibull.shape > 0.0);
    const auto exponential = datalab::domain::statistics::fit_exponential(
        {1.0, 2.0, 3.0}, {true, false, true});
    QVERIFY(exponential.rate > 0.0);
    QVERIFY(weibull.log_likelihood < 0.0);
    QVERIFY(weibull.aic > 0.0 && weibull.bic > 0.0);
    QVERIFY(weibull.b10.has_value() && weibull.b50.has_value() &&
            weibull.b90.has_value());
    QVERIFY(km.points[2].standard_error > 0.0);
    QVERIFY(km.points[2].confidence_lower < km.points[2].survival);
    QVERIFY(km.points[2].confidence_upper > km.points[2].survival);
    QVERIFY(km.censoring_fraction == 0.25);
    const auto log_rank = datalab::domain::statistics::log_rank_test(
        {1.0, 2.0, 3.0, 4.0, 5.0, 6.0},
        {true, false, true, false, true, true},
        {0, 0, 1, 1, 0, 1});
    QVERIFY(log_rank.chi_square >= 0.0);
    QVERIFY(log_rank.p_value >= 0.0 && log_rank.p_value <= 1.0);
}

void ReliabilityMsaPowerTest::handlesAllCensoringAndFewFailures()
{
    const auto all_censored = datalab::domain::statistics::kaplan_meier(
        {1.0, 2.0, 3.0}, {false, false, false});
    QVERIFY(!all_censored.survival_identifiable);
    QVERIFY(!all_censored.median_life.has_value());
    QVERIFY(all_censored.censoring_fraction == 1.0);
    QVERIFY(!all_censored.diagnostics.empty());

    const auto censored_exponential = datalab::domain::statistics::fit_exponential(
        {1.0, 2.0, 10.0, 10.0}, {true, false, false, false});
    QVERIFY(censored_exponential.identifiable);
    QVERIFY(censored_exponential.rate > 0.0);
    QCOMPARE(censored_exponential.failures, std::size_t{1});

    const auto one_failure_weibull = datalab::domain::statistics::fit_weibull(
        {1.0, 2.0, 10.0}, {true, false, false});
    QVERIFY(!one_failure_weibull.identifiable);
    QVERIFY(!one_failure_weibull.diagnostics.empty());
}

void ReliabilityMsaPowerTest::rejectsInvalidReliabilityInput()
{
    const auto invalid_time = datalab::domain::statistics::kaplan_meier(
        {1.0, 0.0, 2.0}, {true, false, true});
    QVERIFY(!invalid_time.diagnostics.empty());

    const auto mismatched = datalab::domain::statistics::fit_exponential(
        {1.0, 2.0}, {true});
    QVERIFY(!mismatched.diagnostics.empty());

    const auto invalid_confidence = datalab::domain::statistics::kaplan_meier(
        {1.0, 2.0}, {true, false}, 1.0);
    QVERIFY(!invalid_confidence.diagnostics.empty());
}

void ReliabilityMsaPowerTest::tPowerAndSampleSize()
{
    const auto n = datalab::domain::statistics::one_sample_t_sample_size(0.5);
    QVERIFY(n.sample_size > 0);
    QCOMPARE(n.sample_size_per_group, std::size_t{0});
    QCOMPARE(n.total_sample_size, n.sample_size);
    QVERIFY(n.degrees_of_freedom == static_cast<double>(n.sample_size - 1));
    QVERIFY(n.critical_value > 1.9);
    QVERIFY(n.noncentrality_parameter > 0.0);
    QVERIFY(n.power >= 0.8);
    const auto power = datalab::domain::statistics::one_sample_t_power(
        n.sample_size, 0.5);
    QVERIFY2(power.power > 0.7, qPrintable(QString::number(power.power)));
    QVERIFY(power.power <= 1.0);
    QCOMPARE(power.total_sample_size, power.sample_size);
    QCOMPARE(power.sample_size_per_group, std::size_t{0});
    QCOMPARE(power.degrees_of_freedom,
             static_cast<double>(power.sample_size - 1));
    QVERIFY(power.noncentrality_parameter > 0.0);
    QVERIFY(power.critical_value > 1.9);

    const auto two_group =
        datalab::domain::statistics::two_sample_t_sample_size(0.5);
    QVERIFY(two_group.sample_size > 0);
    QCOMPARE(two_group.sample_size_per_group, two_group.sample_size);
    QCOMPARE(two_group.total_sample_size, 2 * two_group.sample_size);
    QCOMPARE(two_group.degrees_of_freedom,
             static_cast<double>(two_group.total_sample_size - 2));
    QVERIFY(two_group.power >= 0.8);
}

void ReliabilityMsaPowerTest::extendedPowerAndSampleSize()
{
    const auto anova = datalab::domain::statistics::one_way_anova_sample_size(3, 0.35);
    QVERIFY(anova.sample_size_per_group >= 2);
    QCOMPARE(anova.total_sample_size, anova.sample_size_per_group * 3);
    QVERIFY(anova.degrees_of_freedom == 2.0);
    QVERIFY(anova.noncentrality_parameter > 0.0);
    QVERIFY(anova.power >= 0.8);
    QVERIFY(datalab::domain::statistics::one_way_anova_power(
                anova.sample_size_per_group - 1, 3, 0.35).power < 0.8);

    const auto one_proportion =
        datalab::domain::statistics::one_sample_proportion_sample_size(
            0.5, 0.65, 0.8, 0.05,
            datalab::domain::statistics::PowerAlternative::greater);
    QVERIFY(one_proportion.sample_size > 0);
    QVERIFY(one_proportion.power >= 0.8);
    QVERIFY(std::isinf(one_proportion.degrees_of_freedom));

    const auto two_proportion =
        datalab::domain::statistics::two_proportion_sample_size(
            0.5, 0.7, 0.8, 0.05,
            datalab::domain::statistics::PowerAlternative::two_sided,
            datalab::domain::statistics::ProportionVarianceMethod::unpooled);
    QVERIFY(two_proportion.sample_size_per_group > 0);
    QCOMPARE(two_proportion.total_sample_size, 2 * two_proportion.sample_size_per_group);
    QVERIFY(two_proportion.power >= 0.8);
    QVERIFY(two_proportion.critical_value > 1.9);
}

void ReliabilityMsaPowerTest::buildsServiceOutputPages()
{
    datalab::domain::DataTable table;
    table.columns = {"Measurement", "Reference", "Time", "Event"};
    table.rows = {{"10.0", "10.0", "1", "1"}, {"10.1", "10.0", "2", "0"},
                  {"9.9", "10.0", "3", "1"}, {"10.0", "10.0", "4", "1"}};
    datalab::domain::AnalysisConfiguration msa;
    msa.msa.gage_measurement_column = 0;
    msa.msa.reference_value = 10.0;
    msa.msa.gage_tolerance = 1.0;
    const auto msa_page = datalab::application::AnalysisService::msa_type1(table, msa);
    QCOMPARE(msa_page.tables.size(), std::size_t{1});
    QCOMPARE(msa_page.plots.size(), std::size_t{1});

    datalab::domain::AnalysisConfiguration reliability;
    reliability.reliability.time_column = 2;
    reliability.reliability.event_column = 3;
    const auto reliability_page =
        datalab::application::AnalysisService::reliability(table, reliability);
    QCOMPARE(reliability_page.tables.size(), std::size_t{1});
    QCOMPARE(reliability_page.plots.size(), std::size_t{1});

    datalab::domain::AnalysisConfiguration power;
    power.power.mode = "two_sample_sample_size";
    const auto power_page = datalab::application::AnalysisService::t_power(table, power);
    QCOMPARE(power_page.tables.size(), std::size_t{1});
}

QTEST_APPLESS_MAIN(ReliabilityMsaPowerTest)
#include "reliability_msa_power_test.moc"
