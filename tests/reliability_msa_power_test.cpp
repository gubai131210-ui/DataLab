#include "domain/statistics/msa_type1.h"
#include "domain/statistics/reliability.h"
#include "domain/statistics/t_power.h"
#include "domain/statistics/attribute_agreement.h"
#include "application/analysis_service.h"

#include <QtTest/QtTest>

#include <cmath>
#include <string>

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
    void fitsThreeParameterWeibullFormulaReference();
    void fleissKappaOverallAgreement();
    void kendallConcordanceAndTauFormulaReference();
    void fitsExponential2AndLognormal3FormulaReference();
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
    QVERIFY(weibull.b10.has_value() && weibull.b50.has_value()
            && weibull.b90.has_value());
    const double b10 = datalab::domain::statistics::percentile_life_weibull(
        weibull.shape, weibull.scale, 10.0);
    QVERIFY(qAbs(*weibull.b10 - b10) < 1.0e-6);

    const auto distribution_compare =
        datalab::domain::statistics::compare_parametric_distributions(
            {10.0, 12.0, 14.0, 16.0, 18.0, 20.0},
            {true, true, true, false, false, false});
    QCOMPARE(distribution_compare.size(), std::size_t{3});
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

    const std::vector<double> lognormal_times = {
        std::exp(0.0), std::exp(0.2), std::exp(-0.1), std::exp(0.4),
        std::exp(0.1), std::exp(-0.2), std::exp(0.3), std::exp(0.05)};
    const std::vector<bool> all_failed(lognormal_times.size(), true);
    const auto lognormal = datalab::domain::statistics::fit_lognormal(
        lognormal_times, all_failed);
    QVERIFY(lognormal.identifiable);
    QVERIFY(lognormal.converged);
    double log_mean = 0.0;
    for (const double time : lognormal_times) {
        log_mean += std::log(time);
    }
    log_mean /= static_cast<double>(lognormal_times.size());
    QVERIFY(qAbs(lognormal.location - log_mean) < 1.0e-8);
    const double p50 = datalab::domain::statistics::percentile_life_lognormal(
        lognormal.location, lognormal.scale, 50.0);
    QVERIFY(qAbs(*lognormal.b50 - p50) < 1.0e-9);
    QVERIFY(qAbs(p50 - std::exp(lognormal.location)) < 1.0e-9);
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

    const auto all_censored_lognormal = datalab::domain::statistics::fit_lognormal(
        {1.0, 2.0, 3.0}, {false, false, false});
    QVERIFY(!all_censored_lognormal.identifiable);
    QVERIFY(!all_censored_lognormal.diagnostics.empty());
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
    QCOMPARE(reliability_page.plots.size(), std::size_t{1});

    reliability.reliability.model = "lognormal";
    const auto lognormal_page =
        datalab::application::AnalysisService::reliability(table, reliability);
    QVERIFY(lognormal_page.tables.size() >= std::size_t{2});
    QCOMPARE(lognormal_page.method_name, std::string("Lognormal Lifetime"));

    reliability.reliability.model = "weibull3";
    const auto weibull3_page =
        datalab::application::AnalysisService::reliability(table, reliability);
    QCOMPARE(weibull3_page.method_name, std::string("3-Parameter Weibull Lifetime"));
    QVERIFY(!weibull3_page.tables.empty());
    QCOMPARE(weibull3_page.tables.front().headers[2], std::string("Threshold"));

    reliability.reliability.model = "exponential2";
    const auto exponential2_page =
        datalab::application::AnalysisService::reliability(table, reliability);
    QCOMPARE(exponential2_page.method_name,
             std::string("2-Parameter Exponential Lifetime"));
    QVERIFY(!exponential2_page.tables.empty());
    QCOMPARE(exponential2_page.tables.front().headers[1], std::string("Threshold"));

    reliability.reliability.model = "lognormal3";
    const auto lognormal3_page =
        datalab::application::AnalysisService::reliability(table, reliability);
    QCOMPARE(lognormal3_page.method_name,
             std::string("3-Parameter Lognormal Lifetime"));
    QVERIFY(!lognormal3_page.tables.empty());
    QCOMPARE(lognormal3_page.tables.front().headers[2], std::string("Threshold"));

    datalab::domain::AnalysisConfiguration power;
    power.power.mode = "two_sample_sample_size";
    const auto power_page = datalab::application::AnalysisService::t_power(table, power);
    QCOMPARE(power_page.tables.size(), std::size_t{1});
}

void ReliabilityMsaPowerTest::fitsThreeParameterWeibullFormulaReference()
{
    // source: formula_reference — not a Minitab export.
    const double shape = 2.0;
    const double scale = 10.0;
    const double threshold = 5.0;
    const std::vector<double> probabilities = {
        0.08, 0.16, 0.24, 0.32, 0.40, 0.48, 0.56, 0.64, 0.72, 0.80, 0.88};
    std::vector<double> times;
    std::vector<bool> events;
    times.reserve(probabilities.size());
    for (const double probability : probabilities) {
        times.push_back(
            threshold + scale * std::pow(-std::log(1.0 - probability), 1.0 / shape));
        events.push_back(true);
    }
    const auto fitted = datalab::domain::statistics::fit_weibull3(times, events);
    QVERIFY(fitted.identifiable);
    QVERIFY(fitted.converged);
    QVERIFY(fitted.threshold.has_value());
    QVERIFY(fitted.shape > 1.0);
    QVERIFY(*fitted.threshold < times.front());
    const double b50 = datalab::domain::statistics::percentile_life_weibull3(
        fitted.shape, fitted.scale, *fitted.threshold, 50.0);
    QVERIFY(qAbs(*fitted.b50 - b50) < 1.0e-9);

    const auto few = datalab::domain::statistics::fit_weibull3(
        {6.0, 7.0, 20.0}, {true, true, false});
    QVERIFY(!few.identifiable);
    QVERIFY(!few.diagnostics.empty());
}

void ReliabilityMsaPowerTest::fleissKappaOverallAgreement()
{
    // source: formula_reference — not a Minitab export.
    const auto perfect = datalab::domain::statistics::attribute_agreement(
        {"A", "A", "A", "B", "B", "B", "A", "A", "A"},
        {"P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"},
        {"R1", "R2", "R3", "R1", "R2", "R3", "R1", "R2", "R3"});
    QVERIFY(perfect.overall_available);
    QCOMPARE(perfect.overall.method, std::string("fleiss"));
    QVERIFY(perfect.overall.identifiable);
    QVERIFY(qAbs(perfect.overall.kappa - 1.0) < 1.0e-9);
    QCOMPARE(perfect.between_evaluator.front().estimate.method,
             std::string("cohen_unweighted"));

    const auto chance = datalab::domain::statistics::attribute_agreement(
        {"A", "B", "C", "B", "C", "A", "C", "A", "B"},
        {"P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"},
        {"R1", "R2", "R3", "R1", "R2", "R3", "R1", "R2", "R3"});
    QVERIFY(chance.overall_available);
    QVERIFY(chance.overall.identifiable);
    QVERIFY(chance.overall.kappa < 0.5);
}

void ReliabilityMsaPowerTest::kendallConcordanceAndTauFormulaReference()
{
    // source: formula_reference — not a Minitab export.
    const auto nominal = datalab::domain::statistics::attribute_agreement(
        {"1", "1", "1", "2", "2", "2", "3", "3", "3"},
        {"P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"},
        {"R1", "R2", "R3", "R1", "R2", "R3", "R1", "R2", "R3"});
    QVERIFY(!nominal.between_kendall.has_value());

    const auto perfect = datalab::domain::statistics::attribute_agreement(
        {"1", "1", "1", "2", "2", "2", "3", "3", "3"},
        {"P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"},
        {"R1", "R2", "R3", "R1", "R2", "R3", "R1", "R2", "R3"},
        {}, 0.95, true);
    QVERIFY(perfect.between_kendall.has_value());
    QVERIFY(perfect.between_kendall->identifiable);
    QVERIFY(qAbs(perfect.between_kendall->coefficient - 1.0) < 1.0e-9);

    const auto two_levels = datalab::domain::statistics::attribute_agreement(
        {"1", "1", "1", "2", "2", "2"},
        {"P1", "P1", "P1", "P2", "P2", "P2"},
        {"R1", "R2", "R3", "R1", "R2", "R3"},
        {}, 0.95, true);
    QVERIFY(!two_levels.between_kendall.has_value());
    bool saw_levels = false;
    for (const auto& diagnostic : two_levels.diagnostics) {
        if (diagnostic.code == "kendall_requires_three_ordinal_levels") {
            saw_levels = true;
        }
    }
    QVERIFY(saw_levels);

    const auto unranked = datalab::domain::statistics::attribute_agreement(
        {"low", "low", "low", "mid", "mid", "mid", "high", "high", "high"},
        {"P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"},
        {"R1", "R2", "R3", "R1", "R2", "R3", "R1", "R2", "R3"},
        {}, 0.95, true);
    QVERIFY(!unranked.between_kendall.has_value());
    bool saw_unranked = false;
    for (const auto& diagnostic : unranked.diagnostics) {
        if (diagnostic.code == "ordinal_ratings_unranked") {
            saw_unranked = true;
        }
    }
    QVERIFY(saw_unranked);

    const auto vs_standard = datalab::domain::statistics::attribute_agreement(
        {"1", "1", "3", "2", "2", "2", "3", "3", "1"},
        {"P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"},
        {"R1", "R2", "R3", "R1", "R2", "R3", "R1", "R2", "R3"},
        {"1", "1", "1", "2", "2", "2", "3", "3", "3"},
        0.95, true);
    QVERIFY(!vs_standard.against_standard_kendall.empty());
    QVERIFY(vs_standard.against_standard_kendall.front().estimate.identifiable);
    QVERIFY(qAbs(vs_standard.against_standard_kendall.front().estimate.tau - 1.0)
            < 1.0e-9);
    QVERIFY(vs_standard.overall_kendall.has_value());

    datalab::domain::DataTable table;
    table.columns = {"Rating", "Part", "Appraiser"};
    table.rows = {
        {"1", "P1", "R1"}, {"1", "P1", "R2"}, {"1", "P1", "R3"},
        {"2", "P2", "R1"}, {"2", "P2", "R2"}, {"2", "P2", "R3"},
        {"3", "P3", "R1"}, {"3", "P3", "R2"}, {"3", "P3", "R3"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.attribute_rating_column = 0;
    configuration.msa.attribute_part_column = 1;
    configuration.msa.attribute_appraiser_column = 2;
    configuration.msa.ratings_are_ordinal = true;
    const auto page = datalab::application::AnalysisService::attribute_agreement(
        table, configuration);
    bool saw_kendall = false;
    for (const auto& table_spec : page.tables) {
        if (table_spec.title.find("Kendall") != std::string::npos) {
            saw_kendall = true;
        }
    }
    QVERIFY(saw_kendall);
    QVERIFY(page.facts.msa.has_value());
    QVERIFY(page.facts.msa->kendall_available);
}

void ReliabilityMsaPowerTest::fitsExponential2AndLognormal3FormulaReference()
{
    // source: formula_reference — not a Minitab export.
    const double theta = 8.0;
    const double threshold = 4.0;
    const std::vector<double> probabilities = {
        0.08, 0.16, 0.24, 0.32, 0.40, 0.48, 0.56, 0.64, 0.72, 0.80, 0.88};
    std::vector<double> exp_times;
    std::vector<bool> exp_events;
    for (const double probability : probabilities) {
        exp_times.push_back(threshold - theta * std::log(1.0 - probability));
        exp_events.push_back(true);
    }
    const auto exponential2 =
        datalab::domain::statistics::fit_exponential2(exp_times, exp_events);
    QVERIFY(exponential2.identifiable);
    QVERIFY(exponential2.threshold.has_value());
    QVERIFY(*exponential2.threshold < exp_times.front());
    const double b50 = datalab::domain::statistics::percentile_life_exponential2(
        exponential2.rate, *exponential2.threshold, 50.0);
    QVERIFY(qAbs(*exponential2.b50 - b50) < 1.0e-9);

    const auto all_equal = datalab::domain::statistics::fit_exponential2(
        {5.0, 5.0, 5.0}, {true, true, true});
    QVERIFY(!all_equal.identifiable);
    QVERIFY(!all_equal.diagnostics.empty());

    const double location = 0.2;
    const double scale = 0.4;
    std::vector<double> ln_times;
    std::vector<bool> ln_events;
    for (const double probability : probabilities) {
        const double z = (probability - 0.5) * 2.5;
        ln_times.push_back(threshold + std::exp(location + scale * z));
        ln_events.push_back(true);
    }
    const auto lognormal3 =
        datalab::domain::statistics::fit_lognormal3(ln_times, ln_events);
    QVERIFY(lognormal3.identifiable);
    QVERIFY(lognormal3.threshold.has_value());
    QVERIFY(*lognormal3.threshold < ln_times.front());
    const double ln_b50 = datalab::domain::statistics::percentile_life_lognormal3(
        lognormal3.location, lognormal3.scale, *lognormal3.threshold, 50.0);
    QVERIFY(qAbs(*lognormal3.b50 - ln_b50) < 1.0e-9);

    const auto few = datalab::domain::statistics::fit_lognormal3(
        {6.0, 20.0}, {true, false});
    QVERIFY(!few.identifiable);

    const auto compare = datalab::domain::statistics::compare_parametric_distributions(
        exp_times, exp_events);
    QCOMPARE(compare.size(), std::size_t{3});
}

QTEST_APPLESS_MAIN(ReliabilityMsaPowerTest)
#include "reliability_msa_power_test.moc"
