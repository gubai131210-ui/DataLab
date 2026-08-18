#include "domain/quality_types.h"
#include "domain/statistics/control_charts.h"
#include "domain/statistics/correlation.h"
#include "domain/statistics/descriptive_statistics.h"
#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/inference_extensions.h"
#include "domain/statistics/box_cox.h"
#include "domain/statistics/regression.h"
#include "domain/statistics/gage_rr.h"
#include "domain/statistics/nonparametric_tests.h"
#include "domain/statistics/time_series.h"
#include "domain/statistics/arima.h"
#include "domain/statistics/two_factor_anova.h"
#include "domain/statistics/normality_test.h"
#include "domain/statistics/process_capability.h"
#include "domain/statistics/spc_constants.h"
#include "domain/statistics/normal_probability.h"
#include "domain/statistics/quality_visuals.h"
#include "domain/statistics/doe_factorial.h"
#include "domain/statistics/nested_gage_rr.h"
#include "domain/statistics/attribute_agreement.h"
#include "domain/statistics/seasonal_forecasting.h"
#include "domain/statistics/pca.h"
#include "domain/statistics/response_optimization.h"
#include "application/analysis_service.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>

using datalab::domain::SpecificationLimits;
using datalab::domain::statistics::ControlCharts;
using datalab::domain::statistics::ProcessCapability;
using datalab::domain::statistics::SpcConstants;

class QualityStatisticsTest final : public QObject {
    Q_OBJECT

private slots:
    void calculatesCapabilityIndices();
    void rejectsInvalidSpecifications();
    void calculatesIndividualsMovingRange();
    void usesMovingRangeSigma();
    void calculatesPChart();
    void usesFullSpcConstants();
    void capabilityUsesWithinAndOverall();
    void calculatesXbarS();
    void calculatesNpCAndUCharts();
    void validatesAttributeChartBoundaries();
    void rejectsInvalidAttributeCounts();
    void buildsAttributeChartOutput();
    void calculatesExpandedDescriptiveStatistics();
    void calculatesAndersonDarlingNormality();
    void enforcesStrictSubgroups();
    void buildsSubgroupChartOutput();
    void calculatesLaneyCharts();
    void detectsLaneySpecialCauseTests();
    void detectsTestEightWithoutAlternation();
    void buildsLaneyOutput();
    void buildsCapabilitySixpack();
    void buildsDoeFactorialServiceOutput();
    void calculatesParetoPercentages();
    void combinesParetoOther();
    void buildsParetoOutput();
    void buildsPairedTServiceOutput();
    void buildsRegressionServiceOutput();
    void buildsLogisticServiceOutput();
    void calculatesCorrelation();
    void calculatesTTests();
    void calculatesOneWayAnova();
    void buildsInferenceOutput();
    void calculatesInferenceExtensions();
    void calculatesRegressionAndBoxCox();
    void calculatesGageRrAndNonparametric();
    void calculatesTimeSeries();
    void calculatesTwoFactorAnovaAndArima();
    void calculatesNextBatchAlgorithms();
};

void QualityStatisticsTest::calculatesCapabilityIndices()
{
    const SpecificationLimits specifications{0.0, 10.0, 5.0};
    const auto result = ProcessCapability::calculate(5.0, 1.0, 2.0, specifications);

    QVERIFY(result.cp.has_value());
    QVERIFY(result.cpk.has_value());
    QVERIFY(result.cpl.has_value());
    QVERIFY(result.cpu.has_value());
    QVERIFY(result.pp.has_value());
    QVERIFY(result.ppk.has_value());
    QCOMPARE(*result.cp, 10.0 / 6.0);
    QCOMPARE(*result.cpk, 5.0 / 3.0);
    QCOMPARE(*result.cpl, 5.0 / 3.0);
    QCOMPARE(*result.cpu, 5.0 / 3.0);
    QCOMPARE(*result.pp, 10.0 / 12.0);
    QCOMPARE(*result.ppk, 5.0 / 6.0);
}

void QualityStatisticsTest::rejectsInvalidSpecifications()
{
    const SpecificationLimits specifications{10.0, 0.0, std::nullopt};
    const auto result = ProcessCapability::calculate(5.0, 1.0, 1.0, specifications);

    QVERIFY(!result.diagnostics.empty());
    QVERIFY(!result.cp.has_value());
}

void QualityStatisticsTest::calculatesIndividualsMovingRange()
{
    const auto result = ControlCharts::individuals_moving_range({1.0, 2.0, 3.0, 2.0});

    QCOMPARE(result.plotted_values.size(), std::size_t{4});
    QCOMPARE(result.center_line.size(), std::size_t{4});
    QCOMPARE(result.lower_control_limit.size(), std::size_t{4});
    QCOMPARE(result.upper_control_limit.size(), std::size_t{4});
    QVERIFY(result.upper_control_limit.front() > result.center_line.front());
}

void QualityStatisticsTest::usesMovingRangeSigma()
{
    const auto dual = ControlCharts::individuals_moving_range_dual({1.0, 2.0, 3.0, 2.0});
    QCOMPARE(dual.average_moving_range, 1.0);
    QVERIFY(qAbs(dual.sigma - 1.0 / 1.128) < 1.0e-12);
    QCOMPARE(dual.primary.center_line.front(), 2.0);
    QVERIFY(qAbs(dual.primary.upper_control_limit.front() - (2.0 + 3.0 / 1.128)) < 1.0e-12);
    QVERIFY(dual.secondary.plotted_values.size() == 4);
}

void QualityStatisticsTest::calculatesPChart()
{
    const auto result = ControlCharts::p_chart({1, 2, 1}, {10, 10, 10});

    QCOMPARE(result.plotted_values.size(), std::size_t{3});
    QCOMPARE(result.center_line.front(), 0.13333333333333333);
    QVERIFY(result.lower_control_limit.front() >= 0.0);
    QVERIFY(result.upper_control_limit.front() <= 1.0);
}

void QualityStatisticsTest::usesFullSpcConstants()
{
    QVERIFY(qAbs(*SpcConstants::d2(2) - 1.128) < 1.0e-9);
    QVERIFY(qAbs(*SpcConstants::d2(5) - 2.326) < 1.0e-9);
    const auto dual = ControlCharts::xbar_range_dual({{1.0, 2.0, 3.0, 4.0, 5.0}, {2.0, 3.0, 4.0, 5.0, 6.0}});
    QVERIFY(qAbs(dual.sigma - 4.0 / 2.326) < 1.0e-9);
}

void QualityStatisticsTest::capabilityUsesWithinAndOverall()
{
    const SpecificationLimits specifications{0.0, 10.0, std::nullopt};
    const auto result = ProcessCapability::calculate({4.0, 5.0, 6.0}, 1.0, specifications);
    QVERIFY(result.cp.has_value());
    QVERIFY(result.observed_ppm_total.has_value());
    QCOMPARE(result.sample_size, std::size_t{3});
}

void QualityStatisticsTest::calculatesXbarS()
{
    const auto result = ControlCharts::xbar_s_dual(
        {{1.0, 2.0, 3.0, 4.0}, {2.0, 3.0, 4.0, 5.0}});
    QCOMPARE(result.primary.plotted_values.size(), std::size_t{2});
    QCOMPARE(result.secondary.plotted_values.size(), std::size_t{2});
    QVERIFY(result.sigma > 0.0);
    QVERIFY(result.primary.upper_control_limit.front()
            > result.primary.center_line.front());
}

void QualityStatisticsTest::calculatesNpCAndUCharts()
{
    const auto np = ControlCharts::np_chart({1, 2, 1}, {10, 10, 10});
    const auto c = ControlCharts::c_chart({1, 2, 3}, 10);
    const auto u = ControlCharts::u_chart({1, 2, 3}, {10, 20, 10});
    QCOMPARE(np.plotted_values.size(), std::size_t{3});
    QCOMPARE(c.plotted_values.size(), std::size_t{3});
    QCOMPARE(u.plotted_values.size(), std::size_t{3});
    QVERIFY(c.lower_control_limit.front() >= 0.0);
    QVERIFY(u.upper_control_limit.front() > u.center_line.front());
}

void QualityStatisticsTest::validatesAttributeChartBoundaries()
{
    const auto p = ControlCharts::p_chart({2, 4}, {10, 20});
    const double pbar = 6.0 / 30.0;
    const double first_sigma = std::sqrt(pbar * (1.0 - pbar) / 10.0);
    QCOMPARE(p.center_line.front(), pbar);
    QCOMPARE(p.plotted_values.front(), 0.2);
    QVERIFY(qAbs(p.upper_control_limit.front() - std::min(1.0, pbar + 3.0 * first_sigma))
            < 1.0e-12);
    QVERIFY(p.lower_control_limit.front() >= 0.0);

    const auto np = ControlCharts::np_chart({2, 4}, {10, 20});
    const double np_center = 10.0 * pbar;
    QCOMPARE(np.center_line.front(), np_center);
    QVERIFY(np.lower_control_limit.front() >= 0.0);

    const auto c = ControlCharts::c_chart({0, 0, 0}, 10);
    QCOMPARE(c.center_line.front(), 0.0);
    QCOMPARE(c.lower_control_limit.front(), 0.0);
    QCOMPARE(c.upper_control_limit.front(), 0.0);

    const auto u = ControlCharts::u_chart({1, 4}, {10, 20});
    const double ubar = 5.0 / 30.0;
    QCOMPARE(u.center_line.front(), ubar);
    QCOMPARE(u.plotted_values[1], 0.2);
    QVERIFY(u.lower_control_limit.front() >= 0.0);
}

void QualityStatisticsTest::rejectsInvalidAttributeCounts()
{
    QVERIFY(!ControlCharts::p_chart({1}, {0}).diagnostics.empty());
    QVERIFY(!ControlCharts::p_chart({3}, {2}).diagnostics.empty());
    QVERIFY(!ControlCharts::np_chart({1, 2}, {10}).diagnostics.empty());
    QVERIFY(!ControlCharts::c_chart({}, 10).diagnostics.empty());
    QVERIFY(!ControlCharts::c_chart({1}, 0).diagnostics.empty());
    QVERIFY(!ControlCharts::u_chart({1}, {0}).diagnostics.empty());
    QVERIFY(!ControlCharts::u_chart({1, 2}, {10}).diagnostics.empty());
}

void QualityStatisticsTest::buildsAttributeChartOutput()
{
    datalab::domain::DataTable table;
    table.columns = {"Defectives", "Inspected", "Units"};
    table.rows = {{"1", "10", "10"}, {"2", "10", "20"}, {"1", "10", "10"}};

    datalab::domain::AnalysisConfiguration p_configuration;
    p_configuration.chart_type = "p_chart";
    p_configuration.selection.defect_count_column = 0;
    p_configuration.selection.inspected_count_column = 1;
    const auto p_page = datalab::application::AnalysisService::p_chart(
        table, p_configuration);
    QCOMPARE(p_page.tables.size(), std::size_t{2});
    QCOMPARE(p_page.tables.back().headers.size(), std::size_t{10});
    QCOMPARE(p_page.tables.back().rows.size(), std::size_t{3});
    QCOMPARE(p_page.plots.size(), std::size_t{1});

    datalab::domain::AnalysisConfiguration u_configuration;
    u_configuration.chart_type = "u_chart";
    u_configuration.selection.defect_count_column = 0;
    u_configuration.selection.inspected_count_column = 2;
    const auto u_page = datalab::application::AnalysisService::u_chart(
        table, u_configuration);
    QCOMPARE(u_page.tables.size(), std::size_t{2});
    QCOMPARE(u_page.tables.back().rows.front()[3], std::string{"10"});
    QCOMPARE(u_page.plots.front().y_axis_title, std::string{"单位缺陷数"});
}

void QualityStatisticsTest::calculatesExpandedDescriptiveStatistics()
{
    const auto result = datalab::domain::statistics::DescriptiveStatistics::calculate(
        {1.0, 2.0, 4.0, 8.0});
    QVERIFY(result.has_value());
    QCOMPARE(result->sum, 15.0);
    QCOMPARE(result->range, 7.0);
    QCOMPARE(result->first_quartile, 1.75);
    QCOMPARE(result->third_quartile, 5.0);
    QCOMPARE(result->interquartile_range, 3.25);
    QVERIFY(qAbs(result->variance - 28.75 / 3.0) < 1.0e-12);
}

void QualityStatisticsTest::calculatesAndersonDarlingNormality()
{
    const auto result = datalab::domain::statistics::normality_test(
        {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
    QCOMPARE(result.count, std::size_t{6});
    QVERIFY(result.anderson_darling.has_value());
    QVERIFY(result.p_value.has_value());
    QVERIFY(*result.p_value >= 0.0 && *result.p_value <= 1.0);
    QCOMPARE(result.probability_plot.ordered_values.size(), std::size_t{6});
    QCOMPARE(result.probability_plot.theoretical_quantiles.size(), std::size_t{6});
}

void QualityStatisticsTest::enforcesStrictSubgroups()
{
    datalab::domain::DataTable table;
    table.columns = {"Value", "Group"};
    table.rows = {{"1", "A"}, {"2", "A"}, {"3", "B"}, {"4", "B"}};

    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.selection.measurement_column = 0;
    configuration.selection.subgroup_column = 1;
    const auto page = datalab::application::AnalysisService::xbar_range(
        table, configuration);
    QVERIFY(page.tables.size() >= 2);

    table.rows.push_back({"5", "B"});
    const auto invalid = datalab::application::AnalysisService::xbar_range(
        table, configuration);
    QVERIFY(!invalid.diagnostics.empty());
    QVERIFY(invalid.tables.empty());
}

void QualityStatisticsTest::buildsSubgroupChartOutput()
{
    datalab::domain::DataTable table;
    table.columns = {"Value"};
    table.rows = {{"1"}, {"2"}, {"3"}, {"4"}, {"2"}, {"3"}, {"4"}, {"5"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.selection.measurement_column = 0;
    configuration.control.subgroup_size = 4;
    const auto page = datalab::application::AnalysisService::xbar_s(
        table, configuration);
    QCOMPARE(page.plots.size(), std::size_t{2});
    QCOMPARE(page.tables.size(), std::size_t{2});
    QCOMPARE(page.tables.back().rows.size(), std::size_t{2});
}

void QualityStatisticsTest::calculatesLaneyCharts()
{
    const auto p = ControlCharts::laney_p_chart(
        {10, 12, 8, 11, 9, 13}, {100, 100, 100, 100, 100, 100});
    QCOMPARE(p.plotted_values.size(), std::size_t{6});
    QVERIFY(p.sigma_z >= 0.0);
    QVERIFY(p.upper_control_limit.front() <= 1.0);
    QVERIFY(p.lower_control_limit.front() >= 0.0);

    const auto u = ControlCharts::laney_u_chart(
        {2, 5, 1, 4, 3, 6}, {10, 20, 5, 10, 15, 20});
    QCOMPARE(u.plotted_values.size(), std::size_t{6});
    QVERIFY(u.sigma_z >= 0.0);
    QVERIFY(u.upper_control_limit.front() > u.center_line.front()
            || qFuzzyCompare(u.upper_control_limit.front(), u.center_line.front()));
}

void QualityStatisticsTest::detectsLaneySpecialCauseTests()
{
    datalab::domain::statistics::LaneyChartOptions options;
    options.enabled_special_cause_tests = {2, 3, 4};
    const auto result = ControlCharts::laney_p_chart(
        {10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20},
        {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
        options);
    QVERIFY(result.special_cause_points.size() == 8);
    QVERIFY(!result.special_cause_points[1].empty());
}

void QualityStatisticsTest::detectsTestEightWithoutAlternation()
{
    datalab::domain::statistics::LaneyChartOptions options;
    options.enabled_special_cause_tests = {8};
    options.historical_center = 0.1;
    options.historical_sigma_z = 1.0;
    const auto result = ControlCharts::laney_p_chart(
        {20, 20, 20, 20, 20, 20, 20, 20},
        {100, 100, 100, 100, 100, 100, 100, 100},
        options);
    QVERIFY(!result.special_cause_points[7].empty());
}

void QualityStatisticsTest::buildsLaneyOutput()
{
    datalab::domain::DataTable table;
    table.columns = {"Defectives", "Inspected", "Stage"};
    table.rows = {
        {"10", "100", "Before"}, {"12", "100", "Before"},
        {"8", "100", "After"}, {"11", "100", "After"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.selection.defect_count_column = 0;
    configuration.selection.inspected_count_column = 1;
    configuration.control.stage_column = 2;
    configuration.control.enabled_special_cause_tests = {1, 2};
    const auto page = datalab::application::AnalysisService::laney_p_chart(
        table, configuration);
    QCOMPARE(page.method_name, std::string{"Laney P' Chart"});
    QCOMPARE(page.tables.size(), std::size_t{2});
    QCOMPARE(page.tables.back().headers.size(), std::size_t{17});
    QCOMPARE(page.tables.back().rows.front()[2], std::string{"Before"});
    QCOMPARE(page.plots.size(), std::size_t{1});
}

void QualityStatisticsTest::buildsCapabilitySixpack()
{
    datalab::domain::DataTable table;
    table.name = "fixture";
    table.columns = {"Diameter"};
    for (int index = 0; index < 30; ++index) {
        table.rows.push_back({std::to_string(74.0 + 0.01 * (index % 5))});
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.chart_type = "capability_sixpack";
    configuration.variable_columns = {0};
    configuration.specifications.lower = 73.95;
    configuration.specifications.upper = 74.10;
    const auto page = datalab::application::AnalysisService::capability_sixpack(
        table, configuration);
    QCOMPARE(page.method_name, std::string{"Capability Sixpack"});
    QVERIFY(page.plots.size() >= 5);
    QVERIFY(!page.tables.empty());
}

void QualityStatisticsTest::calculatesParetoPercentages()
{
    const auto result = datalab::domain::statistics::pareto(
        {{"A", 5}, {"B", 3}, {"C", 2}});
    QCOMPARE(result.size(), std::size_t{3});
    QCOMPARE(result[0].category, std::string{"A"});
    QCOMPARE(result[0].percent, 50.0);
    QCOMPARE(result[1].percent, 30.0);
    QCOMPARE(result[1].cumulative_percent, 80.0);
    QCOMPARE(result[2].cumulative_percent, 100.0);
}

void QualityStatisticsTest::combinesParetoOther()
{
    datalab::domain::statistics::ParetoOptions options;
    options.other_threshold_percent = 80.0;
    const auto result = datalab::domain::statistics::pareto(
        {{"A", 5}, {"B", 3}, {"C", 1}, {"D", 1}}, options);
    // Keep categories until Cum% first surpasses 80 (A+B+C = 90), then merge rest.
    QCOMPARE(result.size(), std::size_t{4});
    QCOMPARE(result[2].category, std::string{"C"});
    QCOMPARE(result[3].category, std::string{"Other"});
    QCOMPARE(result[3].count, std::size_t{1});
    QCOMPARE(result[3].cumulative_percent, 100.0);
}

void QualityStatisticsTest::buildsParetoOutput()
{
    datalab::domain::DataTable table;
    table.columns = {"Defect"};
    table.rows = {{"Scratch"}, {"Dent"}, {"Scratch"}, {"Stain"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.chart_type = "pareto";
    configuration.variable_columns = {0};
    const auto page = datalab::application::AnalysisService::pareto(table, configuration);
    QCOMPARE(page.tables.size(), std::size_t{1});
    QCOMPARE(page.tables.front().headers.size(), std::size_t{4});
    QCOMPARE(page.tables.front().headers[2], std::string{"Percent"});
    QCOMPARE(page.tables.front().rows.front()[0], std::string{"Scratch"});
    QCOMPARE(page.tables.front().rows.front()[2], std::string{"50"});
    QCOMPARE(page.plots.size(), std::size_t{1});
    QCOMPARE(page.plots.front().categories.size(), std::size_t{3});
    QCOMPARE(page.plots.front().category_values.size(), std::size_t{3});
    QCOMPARE(page.plots.front().cumulative_percent.size(), std::size_t{3});
    QCOMPARE(page.plots.front().title, std::string{"Defect 的 Pareto 图"});
}

void QualityStatisticsTest::calculatesCorrelation()
{
    const auto pearson = datalab::domain::statistics::correlation_matrix(
        {{1, 2, 3, 4, 5}, {2, 4, 6, 8, 10}},
        datalab::domain::statistics::CorrelationMethod::pearson);
    QVERIFY(qAbs(pearson.coefficients[0][1] - 1.0) < 1.0e-12);
    QCOMPARE(pearson.counts[0][1], std::size_t{5});
    const auto spearman = datalab::domain::statistics::correlation_matrix(
        {{1, 2, 2, 4}, {4, 3, 3, 1}},
        datalab::domain::statistics::CorrelationMethod::spearman);
    QVERIFY(qAbs(spearman.coefficients[0][1] + 1.0) < 1.0e-12);
}

void QualityStatisticsTest::calculatesTTests()
{
    const auto one_sample = datalab::domain::statistics::one_sample_t_test(
        {8, 9, 10, 11, 12}, 10.0);
    QVERIFY(one_sample.p_value.has_value());
    QVERIFY(qAbs(one_sample.t_statistic) < 1.0e-12);
    QVERIFY(one_sample.confidence_lower.has_value());
    QVERIFY(one_sample.confidence_upper.has_value());

    const auto two_sample = datalab::domain::statistics::two_sample_t_test(
        {1, 2, 3, 4}, {5, 6, 7, 8});
    QVERIFY(two_sample.p_value.has_value());
    QVERIFY(two_sample.mean_difference < 0.0);
    QVERIFY(two_sample.degrees_of_freedom > 0.0);
}

void QualityStatisticsTest::calculatesOneWayAnova()
{
    const auto result = datalab::domain::statistics::one_way_anova(
        {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}},
        {"A", "B", "C"});
    QVERIFY(result.p_value.has_value());
    QCOMPARE(result.total_count, std::size_t{9});
    QCOMPARE(result.total_degrees_of_freedom, std::size_t{8});
    QVERIFY(result.f_statistic > 0.0);
}

void QualityStatisticsTest::buildsInferenceOutput()
{
    datalab::domain::DataTable table;
    table.columns = {"X", "Y", "Group"};
    table.rows = {
        {"1", "2", "A"}, {"2", "3", "A"}, {"3", "5", "B"},
        {"4", "7", "B"}, {"5", "9", "C"}, {"6", "11", "C"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0, 1};
    const auto page = datalab::application::AnalysisService::correlation(table, configuration);
    QCOMPARE(page.tables.size(), std::size_t{2});
    QCOMPARE(page.method_name, std::string{"Correlation"});

    configuration.variable_columns = {0};
    configuration.by_column = 2;
    const auto anova = datalab::application::AnalysisService::one_way_anova(
        table, configuration);
    QCOMPARE(anova.tables.size(), std::size_t{3});
}

void QualityStatisticsTest::calculatesInferenceExtensions()
{
    const auto paired = datalab::domain::statistics::paired_t_test(
        {10.0, 11.0, 12.0, 14.0}, {9.0, 10.0, 11.5, 12.0});
    QCOMPARE(paired.count, std::size_t{4});
    QVERIFY(paired.p_value.has_value());
    QVERIFY(qAbs(paired.mean_difference - 1.125) < 1.0e-12);

    const auto tukey = datalab::domain::statistics::tukey_multiple_comparisons(
        {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}, {7.0, 8.0, 9.0}},
        {"A", "B", "C"});
    QCOMPARE(tukey.comparisons.size(), std::size_t{3});
    QVERIFY(tukey.comparisons.front().confidence_lower
        < tukey.comparisons.front().confidence_upper);

    const auto proportions = datalab::domain::statistics::two_proportions_test(
        20, 100, 10, 100);
    QVERIFY(proportions.p_value.has_value());
    QVERIFY(proportions.fisher_p_value.has_value());
    QVERIFY(proportions.difference > 0.0);

    const auto chi_square = datalab::domain::statistics::chi_square_association(
        {{20.0, 30.0}, {10.0, 40.0}});
    QVERIFY(chi_square.p_value.has_value());
    QCOMPARE(chi_square.cells.size(), std::size_t{4});
    QVERIFY(chi_square.pearson_statistic > 0.0);
}

void QualityStatisticsTest::calculatesRegressionAndBoxCox()
{
    const auto regression = datalab::domain::statistics::fit_linear_regression(
        {3.0, 5.0, 7.0, 9.0, 11.0},
        {{1.0}, {2.0}, {3.0}, {4.0}, {5.0}},
        {"X"});
    QCOMPARE(regression.coefficients.size(), std::size_t{2});
    QVERIFY(qAbs(regression.coefficients[1].coefficient - 2.0) < 1.0e-10);
    QVERIFY(regression.r_squared > 0.999999);
    QCOMPARE(regression.observations.size(), std::size_t{5});

    const auto box_cox = datalab::domain::statistics::box_cox_transform(
        {1.0, 2.0, 4.0, 8.0});
    QVERIFY(box_cox.transformed_values.size() == 4);
    QVERIFY(box_cox.lambdas.size() > 100);
    QVERIFY(box_cox.transformed_standard_deviation > 0.0);
}

void QualityStatisticsTest::calculatesGageRrAndNonparametric()
{
    const auto gage = datalab::domain::statistics::crossed_gage_rr(
        {10.0, 10.2, 10.1, 11.0, 11.1, 10.9, 12.0, 12.1, 11.9,
         10.0, 10.1, 10.2, 11.0, 11.2, 11.1, 12.0, 12.2, 12.1},
        {"P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3",
         "P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"},
        {"A", "A", "A", "A", "A", "A", "A", "A", "A",
         "B", "B", "B", "B", "B", "B", "B", "B", "B"},
        5.0);
    QCOMPARE(gage.part_count, std::size_t{3});
    QCOMPARE(gage.operator_count, std::size_t{2});
    QCOMPARE(gage.replicate_count, std::size_t{3});
    QVERIFY(gage.variance_components.size() == 5);
    QVERIFY(gage.ndc >= 0.0);

    const auto mann_whitney = datalab::domain::statistics::mann_whitney(
        {1.0, 2.0, 3.0}, {5.0, 6.0, 7.0});
    QVERIFY(mann_whitney.p_value.has_value());
    QVERIFY(mann_whitney.rank_sum < 10.0);

    const auto signed_rank = datalab::domain::statistics::wilcoxon_signed_rank(
        {2.0, 4.0, 6.0, 8.0}, {1.0, 3.0, 5.0, 7.0});
    QVERIFY(signed_rank.p_value.has_value());

    const auto kruskal = datalab::domain::statistics::kruskal_wallis(
        {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}, {7.0, 8.0, 9.0}},
        {"A", "B", "C"});
    QVERIFY(kruskal.p_value.has_value());
    QCOMPARE(kruskal.groups.size(), std::size_t{3});
}

void QualityStatisticsTest::calculatesTimeSeries()
{
    const auto single = datalab::domain::statistics::single_exponential_smoothing(
        {10.0, 11.0, 12.0, 13.0}, 0.2, 2);
    QCOMPARE(single.forecasts.size(), std::size_t{2});
    QVERIFY(single.mad >= 0.0);
    const auto dual = datalab::domain::statistics::double_exponential_smoothing(
        {10.0, 11.0, 12.0, 13.0}, 0.3, 0.2, 2);
    QCOMPARE(dual.forecasts.size(), std::size_t{2});
    QVERIFY(dual.upper.front() >= dual.lower.front());
}

void QualityStatisticsTest::calculatesTwoFactorAnovaAndArima()
{
    const datalab::domain::statistics::TwoFactorAnovaInput input{
        {"A1", "A1", "A1", "A1", "A2", "A2", "A2", "A2"},
        {"B1", "B1", "B2", "B2", "B1", "B1", "B2", "B2"},
        {10.0, 10.2, 12.0, 12.1, 11.0, 11.1, 13.0, 13.2},
        datalab::domain::statistics::AnovaFactorEncoding::reference};
    const auto anova = datalab::domain::statistics::two_factor_anova(input);
    QCOMPARE(anova.effects.size(), std::size_t{3});
    QCOMPARE(anova.error_degrees_of_freedom, std::size_t{4});
    QVERIFY(anova.effects[0].sequential_sum_of_squares.has_value());
    QVERIFY(*anova.effects[0].sequential_sum_of_squares > 0.0);

    const auto arima = datalab::domain::statistics::fit_arima_candidates(
        {10.0, 10.4, 10.1, 10.8, 10.6, 11.0, 10.9, 11.3}, 3);
    QCOMPARE(arima.size(), std::size_t{3});
    QVERIFY(arima[0].forecasts.size() == 3);
    QVERIFY(std::isfinite(arima[0].aicc));
}

void QualityStatisticsTest::calculatesNextBatchAlgorithms()
{
    const auto design = datalab::domain::statistics::generate_2_level_factorial({
        {{"Temperature", "180", "220"}, {"Pressure", "20", "40"}},
        1, 2, true, 42});
    QCOMPARE(design.runs.size(), std::size_t{5});
    QVERIFY(datalab::domain::statistics::validate_design(design).valid);

    const auto nested = datalab::domain::statistics::nested_gage_rr(
        {10.1, 10.2, 11.0, 11.1, 12.0, 12.1},
        {"P1", "P1", "P2", "P2", "P3", "P3"},
        {"A", "A", "B", "B", "C", "C"}, 2.0);
    QCOMPARE(nested.part_count, std::size_t{3});
    QVERIFY(!nested.variance_components.empty());

    const auto agreement = datalab::domain::statistics::attribute_agreement(
        {"Pass", "Pass", "Fail", "Pass", "Fail", "Fail"},
        {"P1", "P2", "P3", "P1", "P2", "P3"},
        {"A", "A", "A", "B", "B", "B"},
        {"Pass", "Pass", "Fail", "Pass", "Pass", "Fail"});
    QCOMPARE(agreement.evaluator_count, std::size_t{2});
    QVERIFY(!agreement.within_evaluator.empty());

    datalab::domain::statistics::SeasonalForecastingOptions forecast_options;
    forecast_options.seasonal_period = 4;
    forecast_options.forecast_periods = 2;
    const auto forecast = datalab::domain::statistics::fit_seasonal_forecasting(
        {100.0, 110.0, 120.0, 105.0, 104.0, 115.0, 125.0, 109.0}, forecast_options);
    QCOMPARE(forecast.forecasts.size(), std::size_t{2});
    QVERIFY(forecast.metrics.count > 0);

    datalab::domain::statistics::FixedSarimaParameters sarima_parameters;
    sarima_parameters.order.p = 1;
    sarima_parameters.ar = {0.2};
    const auto sarima = datalab::domain::statistics::forecast_fixed_sarima(
        {10.0, 10.4, 10.1, 10.8, 10.6, 11.0, 10.9, 11.3}, sarima_parameters, 2);
    QCOMPARE(sarima.forecasts.size(), std::size_t{2});
    QCOMPARE(sarima.lower.size(), std::size_t{2});

    datalab::domain::statistics::PcaOptions pca_options;
    pca_options.mode = datalab::domain::statistics::PcaMode::standardized;
    const auto pca = datalab::domain::statistics::pca(
        {{1.0, 2.0}, {2.0, 4.0}, {3.0, 6.0}, {4.0, 8.0}}, pca_options);
    QCOMPARE(pca.variable_count, std::size_t{2});
    QVERIFY(pca.eigenvalues.size() == 2);
    QVERIFY(pca.converged);

    datalab::domain::DataTable table;
    table.columns = {"X", "Y"};
    table.rows = {{"1", "2"}, {"2", "4"}, {"3", "6"}, {"4", "8"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0, 1};
    configuration.pca.variable_columns = {0, 1};
    const auto page = datalab::application::AnalysisService::pca(table, configuration);
    QCOMPARE(page.method_name, std::string("Principal Component Analysis"));
    QVERIFY(!page.tables.empty());

    datalab::domain::statistics::ResponseModel response_model;
    response_model.response_name = "Yield";
    response_model.factor_names = {"Temperature", "Pressure"};
    response_model.intercept = 10.0;
    response_model.main_effect_coefficients = {2.0, 1.0};
    response_model.observation_count = 8;
    response_model.residual_standard_error = 1.0;
    response_model.residual_degrees_of_freedom = 6.0;
    const auto optimized =
        datalab::domain::statistics::optimize_response_desirability(
            {response_model},
            {{"Yield", datalab::domain::statistics::ResponseGoal::maximize,
              0.0, 20.0, 0.0, 1.0}});
    QCOMPARE(optimized.candidates.size(), std::size_t{4});
    QVERIFY(optimized.best_candidate.has_value());
}

void QualityStatisticsTest::buildsDoeFactorialServiceOutput()
{
    // 设计矩阵分支：2 因子无响应列 → 4 次运行。
    datalab::domain::DataTable design_table;
    design_table.columns = {"X"};
    design_table.rows = {{"1"}, {"2"}};
    datalab::domain::AnalysisConfiguration design_configuration;
    design_configuration.doe.factor_names = {"Temperature", "Pressure"};
    design_configuration.doe.low_levels = {"-1", "-1"};
    design_configuration.doe.high_levels = {"+1", "+1"};
    design_configuration.doe.center_point_count = 0;
    design_configuration.doe.block_count = 1;
    design_configuration.doe.randomize = false;
    const auto design_page = datalab::application::AnalysisService::doe_factorial(
        design_table, design_configuration);
    QCOMPARE(design_page.method_name, std::string{"2-Level Factorial Design"});
    QCOMPARE(design_page.tables.size(), std::size_t{1});
    QCOMPARE(design_page.tables.front().title, std::string{"设计矩阵"});
    QCOMPARE(design_page.tables.front().headers.size(), std::size_t{5});
    QCOMPARE(design_page.tables.front().headers[3], std::string{"Temperature"});
    QCOMPARE(design_page.tables.front().rows.size(), std::size_t{4});

    // 响应分析分支：8 运行（4 角点 ×2 次重复，精确线性响应）→ 4 张表 + 3 张图。
    datalab::domain::DataTable table;
    table.columns = {"A", "B", "Y"};
    table.rows = {
        {"-1", "-1", "-3.5"}, {"1", "-1", "-0.5"},
        {"-1", "1", "1.5"}, {"1", "1", "6.5"},
        {"-1", "-1", "-3.5"}, {"1", "-1", "-0.5"},
        {"-1", "1", "1.5"}, {"1", "1", "6.5"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.doe.factor_columns = {0, 1};
    configuration.doe.response_column = 2;
    const auto page = datalab::application::AnalysisService::doe_factorial(table, configuration);
    QCOMPARE(page.method_name, std::string{"2-Level Factorial Response Analysis"});
    QVERIFY(page.id.rfind("doe_response", 0) == 0);
    QCOMPARE(page.tables.size(), std::size_t{5});
    QCOMPARE(page.tables[0].title, std::string{"系数与效应"});
    QCOMPARE(page.tables[0].rows.size(), std::size_t{4});
    QCOMPARE(page.tables[1].title, std::string{"DOE ANOVA"});
    QCOMPARE(page.tables[2].title, std::string{"模型项与区组"});
    QCOMPARE(page.tables[2].rows.size(), std::size_t{3});
    QCOMPARE(page.tables[3].title, std::string{"纯误差与失拟"});
    QCOMPARE(page.tables[4].title, std::string{"残差诊断"});
    QCOMPARE(page.tables[4].rows.size(), std::size_t{8});
    QCOMPARE(page.plots.size(), std::size_t{3});
    QVERIFY(page.parameter_summary.find("有效运行数 = 8") != std::string::npos);
    // 系数表项名（doe_pages 组装内容）：Constant + 因子名（column_label 带列号前缀）。
    QCOMPARE(page.tables[0].rows[0][0], std::string{"Constant"});
    QCOMPARE(page.tables[0].rows[1][0], std::string{"C1  A"});
    QCOMPARE(page.tables[0].rows[2][0], std::string{"C2  B"});
    QCOMPARE(page.tables[0].rows[3][0], std::string{"C1  A*C2  B"});
}

void QualityStatisticsTest::buildsPairedTServiceOutput()
{
    datalab::domain::DataTable table;
    table.columns = {"Before", "After"};
    table.rows = {{"1", "2"}, {"2", "3"}, {"3", "4"}, {"4", "5"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0, 1};
    const auto page = datalab::application::AnalysisService::paired_t(table, configuration);
    QCOMPARE(page.method_name, std::string{"Paired t"});
    QCOMPARE(page.tables.size(), std::size_t{1});
    QCOMPARE(page.tables.front().title, std::string{"配对差值统计"});
    QCOMPARE(page.tables.front().rows.size(), std::size_t{1});
    QCOMPARE(page.tables.front().rows.front()[0], std::string{"4"});
    QCOMPARE(page.plots.size(), std::size_t{1});
}

void QualityStatisticsTest::buildsRegressionServiceOutput()
{
    datalab::domain::DataTable table;
    table.columns = {"X", "Y"};
    table.rows = {{"1", "2"}, {"2", "4"}, {"3", "6"}, {"4", "8"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {1, 0};
    const auto page = datalab::application::AnalysisService::regression(table, configuration);
    QCOMPARE(page.method_name, std::string{"Linear Regression"});
    QCOMPARE(page.tables.size(), std::size_t{4});
    QCOMPARE(page.tables[0].title, std::string{"模型摘要"});
    QCOMPARE(page.tables[1].title, std::string{"系数"});
    QCOMPARE(page.tables[1].rows.size(), std::size_t{2});
    QCOMPARE(page.plots.size(), std::size_t{3});
    // 完全线性拟合：R-sq = 1。
    QCOMPARE(page.tables[0].rows.front()[1], std::string{"1"});
}

void QualityStatisticsTest::buildsLogisticServiceOutput()
{
    // 非完全可分数据（X=4 的反转打破可分性），避免 IRLS 秩亏/不收敛。
    datalab::domain::DataTable table;
    table.columns = {"Y", "X"};
    table.rows = {{"0", "1"}, {"0", "2"}, {"1", "3"}, {"0", "4"},
                  {"1", "5"}, {"1", "6"}, {"1", "7"}, {"1", "8"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.inference.logistic_response_column = 0;
    configuration.inference.logistic_predictor_columns = {1};
    const auto page =
        datalab::application::AnalysisService::logistic_regression(table, configuration);
    QCOMPARE(page.method_name, std::string{"Binary Logistic Regression"});
    QCOMPARE(page.tables.size(), std::size_t{3});
    QCOMPARE(page.tables[0].title, std::string{"模型摘要"});
    QCOMPARE(page.tables[1].title, std::string{"系数与 Odds Ratio"});
    QCOMPARE(page.tables[2].title, std::string{"拟合与残差"});
    QCOMPARE(page.tables[2].rows.size(), std::size_t{8});
    QCOMPARE(page.plots.size(), std::size_t{1});
    QVERIFY(page.tables[0].rows.front()[2] == std::string{"是"}
            || page.tables[0].rows.front()[2] == std::string{"否"});
}

QTEST_APPLESS_MAIN(QualityStatisticsTest)

#include "quality_statistics_test.moc"
