#include "domain/statistics/msa_type1.h"
#include "domain/statistics/reliability.h"
#include "domain/statistics/t_power.h"
#include "domain/statistics/attribute_agreement.h"
#include "domain/statistics/hypothesis_tests.h"
#include "application/analysis_service.h"

#include <QtTest/QtTest>

#include <algorithm>
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
    void type1HistogramAndRunChartKeepSourceRows();
    void biasLinearityPlotUsesCompleteCaseAndMeanBand();
    void biasLinearityProcessVariationTables();
    void biasLinearityGageBiasTablesAndInference();
    void fitsThreeParameterWeibullFormulaReference();
    void fleissKappaOverallAgreement();
    void attributeAgreementRateCharts();
    void weightedKappaLinearFormulaReference();
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

    const auto one_variance =
        datalab::domain::statistics::one_variance_sample_size(
            1.4, 0.8, 0.05,
            datalab::domain::statistics::PowerAlternative::greater);
    QVERIFY(one_variance.sample_size >= 2);
    QCOMPARE(one_variance.total_sample_size, one_variance.sample_size);
    QVERIFY(one_variance.power >= 0.8);
    QVERIFY(datalab::domain::statistics::one_variance_power(
                one_variance.sample_size - 1, 1.4, 0.05,
                datalab::domain::statistics::PowerAlternative::greater).power < 0.8);

    const auto two_variance =
        datalab::domain::statistics::two_variance_sample_size(
            1.5, 0.8, 0.05,
            datalab::domain::statistics::PowerAlternative::greater);
    QVERIFY(two_variance.sample_size_per_group >= 2);
    QCOMPARE(two_variance.total_sample_size, 2 * two_variance.sample_size_per_group);
    QVERIFY(two_variance.power >= 0.8);
    QVERIFY(datalab::domain::statistics::two_variance_power(
                two_variance.sample_size_per_group - 1, 1.5, 0.05,
                datalab::domain::statistics::PowerAlternative::greater).power < 0.8);

    // # source: formula_reference — one-sample Poisson normal power
    const auto one_poisson_power = datalab::domain::statistics::one_poisson_rate_power(
        40, 2.0, 2.5, 1.0, 0.05,
        datalab::domain::statistics::PowerAlternative::two_sided);
    QVERIFY(one_poisson_power.power > 0.0);
    QVERIFY(one_poisson_power.power < 1.0);
    QCOMPARE(one_poisson_power.effect_size, 0.5);
    const auto one_poisson_n = datalab::domain::statistics::one_poisson_rate_sample_size(
        2.0, 2.5, 0.8, 1.0, 0.05,
        datalab::domain::statistics::PowerAlternative::two_sided);
    QVERIFY(one_poisson_n.sample_size >= 1);
    QVERIFY(one_poisson_n.power >= 0.8);
    const auto two_poisson_n = datalab::domain::statistics::two_poisson_rate_sample_size(
        2.0, 2.8, 0.8, 1.0, 0.05,
        datalab::domain::statistics::PowerAlternative::two_sided);
    QVERIFY(two_poisson_n.sample_size_per_group >= 1);
    QCOMPARE(two_poisson_n.total_sample_size, 2 * two_poisson_n.sample_size_per_group);
    QVERIFY(two_poisson_n.power >= 0.8);

    datalab::domain::AnalysisConfiguration poisson_power_config;
    poisson_power_config.power.mode = "one_poisson_power";
    poisson_power_config.power.sample_size = 40;
    poisson_power_config.power.null_proportion = 2.0;
    poisson_power_config.power.second_proportion = 2.5;
    poisson_power_config.power.observation_length = 1.0;
    poisson_power_config.power.alpha = 0.05;
    poisson_power_config.power.target = 0.8;
    datalab::domain::DataTable empty;
    const auto poisson_page = datalab::application::AnalysisService::t_power(
        empty, poisson_power_config);
    QVERIFY(poisson_page.facts.power.has_value());
    QCOMPARE(poisson_page.facts.power->mode, std::string("one_poisson_power"));
    QVERIFY(poisson_page.facts.power->actual_power.has_value());
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
    QCOMPARE(msa_page.plots.size(), std::size_t{2});

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
    QVERIFY(std::any_of(
        weibull3_page.plots.cbegin(), weibull3_page.plots.cend(),
        [](const datalab::domain::PlotSpec& plot) {
            return plot.title == "三参数 Weibull 生存曲线"
                || plot.title == "三参数 Weibull 概率图";
        })
        || std::any_of(weibull3_page.diagnostics.cbegin(), weibull3_page.diagnostics.cend(),
                       [](const datalab::domain::DiagnosticMessage& diagnostic) {
                           return diagnostic.severity
                               == datalab::domain::DiagnosticMessage::Severity::error
                               || diagnostic.severity
                               == datalab::domain::DiagnosticMessage::Severity::warning;
                       }));

    reliability.reliability.model = "exponential2";
    const auto exponential2_page =
        datalab::application::AnalysisService::reliability(table, reliability);
    QCOMPARE(exponential2_page.method_name,
             std::string("2-Parameter Exponential Lifetime"));
    QVERIFY(!exponential2_page.tables.empty());
    QCOMPARE(exponential2_page.tables.front().headers[1], std::string("Threshold"));
    QVERIFY(std::any_of(
        exponential2_page.plots.cbegin(), exponential2_page.plots.cend(),
        [](const datalab::domain::PlotSpec& plot) {
            return plot.title.find("两参数指数") != std::string::npos;
        })
        || std::any_of(exponential2_page.diagnostics.cbegin(),
                       exponential2_page.diagnostics.cend(),
                       [](const datalab::domain::DiagnosticMessage& diagnostic) {
                           return diagnostic.severity
                               == datalab::domain::DiagnosticMessage::Severity::error
                               || diagnostic.severity
                               == datalab::domain::DiagnosticMessage::Severity::warning;
                       }));

    reliability.reliability.model = "lognormal3";
    const auto lognormal3_page =
        datalab::application::AnalysisService::reliability(table, reliability);
    QCOMPARE(lognormal3_page.method_name,
             std::string("3-Parameter Lognormal Lifetime"));
    QVERIFY(!lognormal3_page.tables.empty());
    QCOMPARE(lognormal3_page.tables.front().headers[2], std::string("Threshold"));
    QVERIFY(std::any_of(
        lognormal3_page.plots.cbegin(), lognormal3_page.plots.cend(),
        [](const datalab::domain::PlotSpec& plot) {
            return plot.title.find("三参数对数正态") != std::string::npos;
        })
        || std::any_of(lognormal3_page.diagnostics.cbegin(),
                       lognormal3_page.diagnostics.cend(),
                       [](const datalab::domain::DiagnosticMessage& diagnostic) {
                           return diagnostic.severity
                               == datalab::domain::DiagnosticMessage::Severity::error
                               || diagnostic.severity
                               == datalab::domain::DiagnosticMessage::Severity::warning;
                       }));

    datalab::domain::AnalysisConfiguration power;
    power.power.mode = "two_sample_sample_size";
    const auto power_page = datalab::application::AnalysisService::t_power(table, power);
    QCOMPARE(power_page.tables.size(), std::size_t{1});

    power.power.mode = "two_variance_sample_size";
    power.power.effect_size = 1.5;
    power.power.target = 0.8;
    power.power.alpha = 0.05;
    power.inference.alternative = "greater";
    const auto variance_power_page = datalab::application::AnalysisService::t_power(table, power);
    QCOMPARE(variance_power_page.tables.size(), std::size_t{1});
    QVERIFY(variance_power_page.facts.power.has_value());
    QCOMPARE(variance_power_page.facts.power->mode, std::string("two_variance_sample_size"));
}

void ReliabilityMsaPowerTest::type1HistogramAndRunChartKeepSourceRows()
{
    datalab::domain::DataTable table;
    table.columns = {"Measurement"};
    table.rows = {{"10.0"}, {"*"}, {"10.1"}, {"9.9"}, {"10.0"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.gage_measurement_column = 0;
    configuration.msa.reference_value = 10.0;
    configuration.msa.gage_tolerance = 2.0;
    configuration.msa.mode = "type1";
    const auto page = datalab::application::AnalysisService::msa_type1(table, configuration);
    QCOMPARE(page.plots.size(), std::size_t{2});
    QCOMPARE(page.plots[0].kind, datalab::domain::PlotKind::histogram);
    QCOMPARE(page.plots[0].title, std::string("Type 1 Gage 直方图"));
    QVERIFY(page.plots[0].target.has_value());
    QCOMPARE(*page.plots[0].target, 10.0);
    QVERIFY(page.plots[0].lsl.has_value());
    QVERIFY(page.plots[0].usl.has_value());
    QCOMPARE(*page.plots[0].lsl, 9.0);
    QCOMPARE(*page.plots[0].usl, 11.0);
    QCOMPARE(page.plots[0].center_label, std::string("Ref"));
    QCOMPARE(page.plots[0].source_rows, (std::vector<std::size_t>{0, 2, 3, 4}));
    QCOMPARE(page.plots[1].kind, datalab::domain::PlotKind::control);
    QCOMPARE(page.plots[1].title, std::string("Gage Run Chart"));
    QCOMPARE(page.plots[1].source_rows, page.plots[0].source_rows);
    QVERIFY(page.facts.msa.has_value());
    QVERIFY(page.facts.msa->cgk.has_value());
    QVERIFY(std::any_of(
        page.diagnostics.cbegin(), page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "missing_values";
        }));
}

void ReliabilityMsaPowerTest::biasLinearityPlotUsesCompleteCaseAndMeanBand()
{
    // # source: formula_reference — OLS mean CI at x-bar, not a Minitab export.
    const auto domain = datalab::domain::statistics::bias_linearity(
        {1.0, 2.0, 3.0}, {2.0, 3.0, 5.0}, 0.95, {10, 20, 30});
    QCOMPARE(domain.slope, 0.5);
    QVERIFY(std::abs(domain.intercept - (1.0 / 3.0)) < 1.0e-12);
    QCOMPARE(domain.observation_source_rows, (std::vector<std::size_t>{10, 20, 30}));
    QVERIFY(!domain.mean_band.empty());
    const double xbar = 2.0;
    const double sigma = std::sqrt(domain.mean_square_error);
    const double critical = datalab::domain::statistics::student_t_quantile(0.975, 1.0);
    auto nearest = domain.mean_band.front();
    for (const auto& point : domain.mean_band) {
        if (std::abs(point.x - xbar) < std::abs(nearest.x - xbar)) {
            nearest = point;
        }
    }
    const double fitted_at = domain.intercept + domain.slope * nearest.x;
    const double se = sigma * std::sqrt(
        1.0 / 3.0 + (nearest.x - xbar) * (nearest.x - xbar) / domain.sum_of_squares_x);
    QVERIFY(std::abs(nearest.fitted - fitted_at) < 1.0e-9);
    QVERIFY(std::abs(nearest.ci_upper - (fitted_at + critical * se)) < 1.0e-9);
    QVERIFY(std::abs(nearest.ci_lower - (fitted_at - critical * se)) < 1.0e-9);

    datalab::domain::DataTable table;
    table.columns = {"Reference", "Measurement"};
    table.rows = {{"1.0", "1.1"}, {"*", "2.2"}, {"3.0", "3.3"}, {"4.0", "4.4"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.gage_measurement_column = 1;
    configuration.msa.reference_column = 0;
    configuration.msa.mode = "bias_linearity";
    configuration.inference.confidence_level = 0.95;
    const auto page = datalab::application::AnalysisService::msa_type1(table, configuration);
    QCOMPARE(page.plots.size(), std::size_t{1});
    QCOMPARE(page.tables.size(), std::size_t{3});
    QCOMPARE(page.tables[0].title, std::string("Coef"));
    QCOMPARE(page.tables[1].title, std::string("S and R-Sq"));
    QCOMPARE(page.tables[2].title, std::string("Gage Bias"));
    QCOMPARE(page.tables[2].rows.back().front(), std::string("Average"));
    QCOMPARE(page.plots[0].source_rows, (std::vector<std::size_t>{0, 2, 3}));
    QCOMPARE(page.plots[0].values.size(), std::size_t{3});
    QVERIFY(page.plots[0].series.size() >= std::size_t{3});
    QCOMPARE(page.plots[0].series[0].role, datalab::domain::PlotSeriesRole::actual);
    QCOMPARE(page.plots[0].series[1].role, datalab::domain::PlotSeriesRole::fitted);
    QCOMPARE(page.plots[0].series[2].role, datalab::domain::PlotSeriesRole::confidence_band);
    QVERIFY(!page.plots[0].series[1].values.empty());
    QVERIFY(!page.plots[0].series[2].lower.empty());
    QVERIFY(page.facts.msa.has_value());
    QVERIFY(page.facts.msa->slope.has_value());
    QVERIFY(std::any_of(
        page.diagnostics.cbegin(), page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "missing_values";
        }));
    QVERIFY(std::any_of(
        page.diagnostics.cbegin(), page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "process_variation_not_provided";
        }));
}

void ReliabilityMsaPowerTest::biasLinearityProcessVariationTables()
{
    // # source: formula_reference — %Linearity = |slope| × 100 when process_variation supplied.
    const double process_variation = 16.5368;
    const auto domain = datalab::domain::statistics::bias_linearity(
        {1.0, 2.0, 3.0}, {2.0, 3.0, 5.0}, 0.95, {10, 20, 30}, process_variation);
    QCOMPARE(domain.slope, 0.5);
    QVERIFY(domain.process_variation_used.has_value());
    QCOMPARE(*domain.process_variation_used, process_variation);
    QVERIFY(domain.linearity.has_value());
    QCOMPARE(*domain.linearity, std::abs(domain.slope) * process_variation);
    QVERIFY(domain.percent_linearity.has_value());
    QCOMPARE(*domain.percent_linearity, std::abs(domain.slope) * 100.0);
    QVERIFY(domain.slope_p_value.has_value());
    QCOMPARE(domain.levels.size(), std::size_t{3});
    for (const auto& level : domain.levels) {
        QVERIFY(level.percent_bias.has_value());
        QCOMPARE(*level.percent_bias, level.bias / process_variation * 100.0);
    }

    datalab::domain::DataTable table;
    table.columns = {"Reference", "Measurement"};
    table.rows = {{"1.0", "2.0"}, {"2.0", "3.0"}, {"3.0", "5.0"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.gage_measurement_column = 1;
    configuration.msa.reference_column = 0;
    configuration.msa.mode = "bias_linearity";
    configuration.msa.process_variation = process_variation;
    configuration.inference.confidence_level = 0.95;
    const auto page = datalab::application::AnalysisService::msa_type1(table, configuration);
    QCOMPARE(page.tables.size(), std::size_t{4});
    QCOMPARE(page.tables[0].title, std::string("Coef"));
    QCOMPARE(page.tables[1].title, std::string("S and R-Sq"));
    QCOMPARE(page.tables[2].title, std::string("Gage Linearity"));
    QCOMPARE(page.tables[3].title, std::string("Gage Bias"));
    QCOMPARE(page.tables[3].rows.back().front(), std::string("Average"));
    QVERIFY(page.facts.msa.has_value());
    QVERIFY(page.facts.msa->intercept_p_value.has_value());
    QVERIFY(page.facts.msa->residual_s.has_value());
    QVERIFY(page.facts.msa->average_bias.has_value());
    QVERIFY(page.facts.msa->average_bias_p.has_value());
    QVERIFY(page.facts.msa->percent_linearity.has_value());
    QCOMPARE(*page.facts.msa->percent_linearity, *domain.percent_linearity);
    QVERIFY(!std::any_of(
        page.diagnostics.cbegin(), page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "process_variation_not_provided";
        }));
}

void ReliabilityMsaPowerTest::biasLinearityGageBiasTablesAndInference()
{
    // # source: formula_reference — per-level and average bias t-tests, not Minitab export.
    const auto single_replicate = datalab::domain::statistics::bias_linearity(
        {1.0, 2.0, 3.0}, {2.0, 3.0, 5.0}, 0.95, {10, 20, 30});
    for (const auto& level : single_replicate.levels) {
        QVERIFY(!level.t_statistic.has_value());
        QVERIFY(!level.p_value.has_value());
    }
    QVERIFY(single_replicate.intercept_p_value.has_value());
    QVERIFY(single_replicate.residual_s.has_value());
    QCOMPARE(single_replicate.average_bias, 4.0 / 3.0);

    const auto replicated = datalab::domain::statistics::bias_linearity(
        {1.0, 1.0, 2.0, 2.0, 3.0, 3.0},
        {2.0, 2.2, 3.0, 3.2, 5.0, 5.2},
        0.95, {1, 2, 3, 4, 5, 6});
    QCOMPARE(replicated.levels.size(), std::size_t{3});
    for (const auto& level : replicated.levels) {
        QVERIFY(level.t_statistic.has_value());
        QVERIFY(level.p_value.has_value());
        QVERIFY(*level.p_value >= 0.0 && *level.p_value <= 1.0);
    }
    QVERIFY(replicated.average_bias_t.has_value());
    QVERIFY(replicated.average_bias_p.has_value());
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

void ReliabilityMsaPowerTest::attributeAgreementRateCharts()
{
    // # source: formula_reference — cell rate vs standard, not Minitab golden.
    const auto with_standard = datalab::domain::statistics::attribute_agreement(
        {"Pass", "Pass", "Fail", "Pass",
         "Pass", "Fail", "Fail", "Pass",
         "Pass", "Pass", "Fail", "Pass"},
        {"P01", "P02", "P03", "P04",
         "P01", "P02", "P03", "P04",
         "P01", "P02", "P03", "P04"},
        {"A", "A", "A", "A",
         "B", "B", "B", "B",
         "C", "C", "C", "C"},
        {"Pass", "Pass", "Fail", "Pass",
         "Pass", "Pass", "Fail", "Pass",
         "Pass", "Pass", "Fail", "Pass"});
    QCOMPARE(with_standard.agreement_evaluator_labels.size(), std::size_t{3});
    QCOMPARE(with_standard.agreement_item_labels.size(), std::size_t{4});
    QCOMPARE(with_standard.agreement_percent_matrix.size(), std::size_t{3});
    QVERIFY(qAbs(with_standard.agreement_percent_matrix[0][1] - 100.0) < 1.0e-12);
    QVERIFY(qAbs(with_standard.agreement_percent_matrix[1][1] - 0.0) < 1.0e-12);
    QVERIFY(qAbs(with_standard.agreement_percent_matrix[0][2] - 100.0) < 1.0e-12);

    const auto tied = datalab::domain::statistics::attribute_agreement(
        {"Pass", "Fail", "Pass", "Pass"},
        {"P1", "P1", "P2", "P2"},
        {"A", "B", "A", "B"});
    QVERIFY(std::any_of(tied.diagnostics.cbegin(), tied.diagnostics.cend(),
                        [](const datalab::domain::DiagnosticMessage& message) {
                            return message.code == "ambiguous_part_mode";
                        }));
    QVERIFY(!std::isfinite(tied.agreement_percent_matrix[0][0]));
    QVERIFY(qAbs(tied.agreement_percent_matrix[0][1] - 100.0) < 1.0e-12);

    datalab::domain::DataTable table;
    table.columns = {"Rating", "Part", "Appraiser", "Standard"};
    table.rows = {
        {"1", "P01", "A", "1"}, {"1", "P02", "A", "1"},
        {"2", "P03", "A", "2"}, {"1", "P04", "A", "1"},
        {"1", "P01", "B", "1"}, {"2", "P02", "B", "1"},
        {"2", "P03", "B", "2"}, {"1", "P04", "B", "1"},
        {"1", "P01", "C", "1"}, {"1", "P02", "C", "1"},
        {"2", "P03", "C", "2"}, {"1", "P04", "C", "1"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.attribute_rating_column = 0;
    configuration.msa.attribute_part_column = 1;
    configuration.msa.attribute_appraiser_column = 2;
    configuration.msa.attribute_standard_column = 3;
    configuration.msa.kappa_weight_scheme = "linear";
    const auto page = datalab::application::AnalysisService::attribute_agreement(
        table, configuration);
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title == "评估者×零件一致率"
                                && plot.kind == datalab::domain::PlotKind::heatmap
                                && plot.matrix_values.size() == 3
                                && plot.matrix_labels.size() == 4
                                && plot.color_min.has_value()
                                && *plot.color_min == 0.0;
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title == "评估者一致率"
                                && plot.kind == datalab::domain::PlotKind::pareto
                                && plot.cumulative_percent.empty()
                                && plot.category_values.size() == 3;
                        }));
    QVERIFY(std::any_of(page.diagnostics.cbegin(), page.diagnostics.cend(),
                        [](const datalab::domain::DiagnosticMessage& message) {
                            return message.code == "fleiss_remains_unweighted";
                        }));
    QVERIFY(std::none_of(page.diagnostics.cbegin(), page.diagnostics.cend(),
                         [](const datalab::domain::DiagnosticMessage& message) {
                             return message.code == "weighted_kappa_not_implemented";
                         }));
    QVERIFY(page.facts.msa.has_value());
    QVERIFY(page.facts.msa->weighted_kappa_available);
    QCOMPARE(page.facts.msa->kappa_weight_scheme, std::string{"linear"});
}

void ReliabilityMsaPowerTest::weightedKappaLinearFormulaReference()
{
    // # source: formula_reference — 3×3 ordinal table; not Minitab export.
    // Raters A/B on parts with ratings 1,2,3. Perfect agreement → κ_w = 1.
    const auto perfect = datalab::domain::statistics::attribute_agreement(
        {"1", "1", "2", "2", "3", "3"},
        {"P1", "P1", "P2", "P2", "P3", "P3"},
        {"A", "B", "A", "B", "A", "B"},
        {}, 0.95, false, "linear");
    QCOMPARE(perfect.between_evaluator.size(), std::size_t{1});
    QCOMPARE(perfect.between_evaluator.front().estimate.method,
             std::string{"cohen_linear"});
    QVERIFY(perfect.between_evaluator.front().estimate.identifiable);
    QVERIFY(qAbs(perfect.between_evaluator.front().estimate.kappa - 1.0) < 1.0e-9);

    const auto quadratic = datalab::domain::statistics::attribute_agreement(
        {"1", "1", "2", "2", "3", "3"},
        {"P1", "P1", "P2", "P2", "P3", "P3"},
        {"A", "B", "A", "B", "A", "B"},
        {}, 0.95, false, "quadratic");
    QCOMPARE(quadratic.between_evaluator.front().estimate.method,
             std::string{"cohen_quadratic"});
    QVERIFY(qAbs(quadratic.between_evaluator.front().estimate.kappa - 1.0) < 1.0e-9);

    const auto unordered = datalab::domain::statistics::attribute_agreement(
        {"Pass", "Pass", "Fail", "Fail"},
        {"P1", "P1", "P2", "P2"},
        {"A", "B", "A", "B"},
        {}, 0.95, false, "linear");
    QVERIFY(std::any_of(unordered.diagnostics.cbegin(), unordered.diagnostics.cend(),
                        [](const datalab::domain::DiagnosticMessage& message) {
                            return message.code == "ordinal_ratings_unranked";
                        }));
    QCOMPARE(unordered.between_evaluator.front().estimate.method,
             std::string{"cohen_unweighted"});
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
