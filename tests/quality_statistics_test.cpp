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
#include "domain/statistics/johnson_transform.h"
#include "domain/statistics/normal_distribution.h"
#include "domain/statistics/spc_constants.h"
#include "domain/statistics/normal_probability.h"
#include "domain/statistics/quality_visuals.h"
#include "domain/statistics/doe_factorial.h"
#include "domain/statistics/nested_gage_rr.h"
#include "domain/statistics/attribute_agreement.h"
#include "domain/statistics/seasonal_forecasting.h"
#include "domain/statistics/pca.h"
#include "domain/statistics/response_optimization.h"
#include "domain/statistics/distribution_identification.h"
#include "domain/statistics/variance_tests.h"
#include "application/analysis_service.h"
#include "application/interpretation_service.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <string>

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
    void attributePhaseLabelsBreakTestTwo();
    void calculatesExpandedDescriptiveStatistics();
    void calculatesAndersonDarlingNormality();
    void enforcesStrictSubgroups();
    void buildsSubgroupChartOutput();
    void calculatesLaneyCharts();
    void detectsLaneySpecialCauseTests();
    void detectsTestEightWithoutAlternation();
    void laneyPhaseLabelsBreakTestTwo();
    void buildsLaneyOutput();
    void buildsCapabilitySixpack();
    void buildsDoeFactorialServiceOutput();
    void calculatesParetoPercentages();
    void combinesParetoOther();
    void buildsParetoOutput();
    void buildsPairedTServiceOutput();
    void buildsOneAndTwoSampleTIntervalPlots();
    void buildsRegressionServiceOutput();
    void buildsResponseOptimizationOutput();
    void buildsMultiResponseOptimizationOutput();
    void buildsLogisticServiceOutput();
    void computesLogisticHosmerLemeshowWhenSampleLarge();
    void identifiesIndividualDistributions();
    void buildsDistributionIdentificationServiceOutput();
    void calculatesBetweenWithinCapability();
    void buildsBetweenWithinCapabilityServiceOutput();
    void buildsCapabilityHistogramContract();
    void calculatesCorrelation();
    void calculatesTTests();
    void calculatesOneWayAnova();
    void buildsInferenceOutput();
    void buildsDescriptiveBoxAndIndividualPlots();
    void buildsNormalityOutputContract();
    void buildsOneWayAnovaIntervalAndResidualPlots();
    void calculatesInferenceExtensions();
    void buildsChiSquareServiceOutputContract();
    void buildsNonparametricServiceOutputContract();
    void calculatesMckeanRyanConfidenceInterval();
    void calculatesRegressionAndBoxCox();
    void buildsBoxCoxServiceOutputContract();
    void regressionAnovaSeqAdjSs();
    void calculatesGageRrAndNonparametric();
    void calculatesTimeSeries();
    void calculatesTwoFactorAnovaAndArima();
    void buildsArimaServiceOutputContract();
    void buildsSeasonalForecastingServiceOutput();
    void buildsTimeSeriesDecompositionServiceOutputContract();
    void buildsTimeSeriesSmoothingServiceOutputContract();
    void buildsSingleExponentialSmoothingServiceOutputContract();
    void calculatesNextBatchAlgorithms();
    void johnsonAndNonnormalCapability();
    void buildsNormalCapabilityTableContract();
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
    QCOMPARE(p_page.tables.back().headers.size(), std::size_t{11});
    QCOMPARE(p_page.tables.back().rows.size(), std::size_t{3});
    QCOMPARE(p_page.plots.size(), std::size_t{1});
    QVERIFY(p_page.facts.spc.has_value());
    QVERIFY(p_page.facts.spc->out_of_control_count.has_value());

    datalab::domain::DataTable staged;
    staged.columns = {"Defectives", "Inspected", "Stage"};
    staged.rows = {
        {"1", "10", "Before"}, {"2", "10", "Before"}, {"1", "10", "After"}};
    datalab::domain::AnalysisConfiguration staged_configuration;
    staged_configuration.chart_type = "p_chart";
    staged_configuration.selection.defect_count_column = 0;
    staged_configuration.selection.inspected_count_column = 1;
    staged_configuration.control.stage_column = 2;
    const auto staged_page = datalab::application::AnalysisService::p_chart(
        staged, staged_configuration);
    QCOMPARE(staged_page.tables.back().rows.front()[2], std::string{"Before"});
    QCOMPARE(staged_page.tables.back().rows.back()[2], std::string{"After"});
    QCOMPARE(staged_page.tables.back().rows.front()[0], std::string{"1"});

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

void QualityStatisticsTest::attributePhaseLabelsBreakTestTwo()
{
    // # source: formula_reference — attribute chart phase labels break Test 2 windows.
    datalab::domain::statistics::ControlChartResult result;
    result.plotted_values = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    result.center_line.assign(9, 0.0);
    result.lower_control_limit.assign(9, -3.0);
    result.upper_control_limit.assign(9, 3.0);
    datalab::domain::statistics::SpecialCauseSelection selection{{2}, "explicit"};
    datalab::domain::statistics::apply_special_cause_tests(
        result, datalab::domain::statistics::ControlChartKind::attribute, selection);
    QVERIFY(!result.special_cause_points[1].empty());

    result.phase_labels = {"A", "A", "A", "A", "B", "B", "B", "B", "B"};
    datalab::domain::statistics::apply_special_cause_tests(
        result, datalab::domain::statistics::ControlChartKind::attribute, selection);
    QVERIFY(result.special_cause_points[1].empty());
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

void QualityStatisticsTest::laneyPhaseLabelsBreakTestTwo()
{
    // # source: formula_reference — stage labels break Test 2 windows on Laney P'.
    const std::vector<std::size_t> defectives(9, 15);
    const std::vector<std::size_t> inspected(9, 100);
    datalab::domain::statistics::LaneyChartOptions options;
    options.enabled_special_cause_tests = {2};
    options.special_cause_rule_policy = "explicit";
    options.historical_center = 0.10;
    options.historical_sigma_z = 1.0;

    const auto without_phases = ControlCharts::laney_p_chart(
        defectives, inspected, options);
    QVERIFY(!without_phases.special_cause_points[1].empty());

    options.phase_labels = {"A", "A", "A", "A", "B", "B", "B", "B", "B"};
    const auto with_phases = ControlCharts::laney_p_chart(
        defectives, inspected, options);
    QVERIFY(with_phases.special_cause_points[1].empty());
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
    QCOMPARE(page.tables.back().headers.size(), std::size_t{21});
    QCOMPARE(page.tables.back().rows.front()[2], std::string{"Before"});
    QCOMPARE(page.plots.size(), std::size_t{1});
    QVERIFY(page.facts.spc.has_value());
    QVERIFY(page.facts.spc->sigma_z.has_value());
    QVERIFY(page.facts.spc->out_of_control_count.has_value());
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
    QVERIFY(page.facts.correlation.has_value());
    QCOMPARE(page.facts.correlation->n, std::size_t{6});
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::scatter
                                && plot.source_rows.size() == 6;
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::matrix
                                && plot.source_rows.size() == 6;
                        }));

    table.rows = {
        {"*", "2", "A"}, {"2", "*", "A"}, {"3", "5", "B"},
        {"4", "7", "B"}, {"5", "9", "C"}, {"6", "11", "C"}};
    const auto aligned = datalab::application::AnalysisService::correlation(
        table, configuration);
    QCOMPARE(aligned.facts.correlation->n, std::size_t{4});
    QCOMPARE(aligned.facts.correlation->missing_skipped, std::size_t{2});
    const datalab::domain::PlotSpec* scatter = nullptr;
    for (const auto& plot : aligned.plots) {
        if (plot.kind == datalab::domain::PlotKind::scatter) {
            scatter = &plot;
        }
    }
    QVERIFY(scatter != nullptr);
    QCOMPARE(scatter->source_rows.size(), std::size_t{4});
    QVERIFY(std::find(scatter->source_rows.cbegin(), scatter->source_rows.cend(),
                      std::size_t{0}) == scatter->source_rows.cend());
    QVERIFY(std::find(scatter->source_rows.cbegin(), scatter->source_rows.cend(),
                      std::size_t{1}) == scatter->source_rows.cend());
    QCOMPARE(scatter->source_rows.front(), std::size_t{2});

    datalab::domain::DataTable three_table;
    three_table.columns = {"X", "Y", "Z"};
    three_table.rows = {{"1", "2", "3"}, {"2", "3", "4"}, {"3", "4", "5"}, {"4", "5", "6"}};
    datalab::domain::AnalysisConfiguration three_config;
    three_config.variable_columns = {0, 1, 2};
    const auto three = datalab::application::AnalysisService::correlation(
        three_table, three_config);
    QVERIFY(std::any_of(three.plots.cbegin(), three.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::matrix
                                && plot.matrix_labels.size() == 3
                                && plot.source_rows.size() == 4;
                        }));

    configuration.variable_columns = {0};
    configuration.by_column = 2;
    table.columns = {"X", "Y", "Group"};
    table.rows = {
        {"1", "2", "A"}, {"2", "3", "A"}, {"3", "5", "B"},
        {"4", "7", "B"}, {"5", "9", "C"}, {"6", "11", "C"}};
    const auto anova = datalab::application::AnalysisService::one_way_anova(
        table, configuration);
    QCOMPARE(anova.tables.size(), std::size_t{3});
}

void QualityStatisticsTest::buildsDescriptiveBoxAndIndividualPlots()
{
    datalab::domain::DataTable table;
    table.columns = {"X", "Group"};
    table.rows = {{"1", "A"}, {"*", "A"}, {"3", "B"}, {"4", "B"}, {"5", "B"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    const auto page = datalab::application::AnalysisService::descriptive(table, configuration);
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::boxplot
                                && plot.box_labels.size() == 1;
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::scatter
                                && plot.values.size() == 4
                                && plot.source_rows.size() == 4
                                && std::find(plot.source_rows.cbegin(), plot.source_rows.cend(),
                                             std::size_t{1}) == plot.source_rows.cend();
                        }));
    QVERIFY(std::any_of(page.diagnostics.cbegin(), page.diagnostics.cend(),
                        [](const datalab::domain::DiagnosticMessage& diagnostic) {
                            return diagnostic.code == "missing_values";
                        }));
    auto interpreted = page;
    datalab::application::InterpretationService::enrich(interpreted);
    const std::string joined = [&]() {
        std::string text;
        for (const auto& section : interpreted.interpretation) {
            for (const auto& bullet : section.bullets) {
                text += bullet;
            }
        }
        return text;
    }();
    QVERIFY(joined.find("不能写成过程合格") != std::string::npos);

    configuration.by_column = 1;
    const auto grouped = datalab::application::AnalysisService::descriptive(
        table, configuration);
    QVERIFY(std::any_of(grouped.plots.cbegin(), grouped.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::boxplot
                                && plot.box_labels.size() == 2;
                        }));
}

void QualityStatisticsTest::buildsNormalityOutputContract()
{
    datalab::domain::DataTable table;
    table.columns = {"X"};
    table.rows = {
        {"9"}, {"1"}, {"2"}, {"3"}, {"4"}, {"5"}, {"6"}, {"7"}, {"8"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    const auto page = datalab::application::AnalysisService::normality_test(
        table, configuration);
    QVERIFY(page.facts.normality.has_value());
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::probability
                                && plot.source_rows.size() == 9
                                && plot.source_rows.front() == 1
                                && plot.values.front() == 1.0;
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::histogram
                                && plot.source_rows.size() == 9
                                && plot.values.size() == 9;
                        }));
    auto interpreted = page;
    datalab::application::InterpretationService::enrich(interpreted);
    const std::string joined = [&]() {
        std::string text;
        for (const auto& section : interpreted.interpretation) {
            for (const auto& bullet : section.bullets) {
                text += bullet;
            }
        }
        return text;
    }();
    QVERIFY(joined.find("已正态") == std::string::npos
        || joined.find("不能写成数据已正态") != std::string::npos);
    QVERIFY(joined.find("数据已正态") == std::string::npos);

    table.rows.push_back({"*"});
    const auto missing = datalab::application::AnalysisService::normality_test(
        table, configuration);
    QCOMPARE(missing.facts.normality->missing_count, std::size_t{1});
}

void QualityStatisticsTest::buildsOneWayAnovaIntervalAndResidualPlots()
{
    // # source: formula_reference — pooled CI uses sqrt(MSE/n), not group s.
    datalab::domain::DataTable table;
    table.columns = {"Y", "Group"};
    table.rows = {
        {"1", "A"}, {"2", "A"}, {"3", "B"}, {"4", "B"}, {"5", "C"}, {"6", "C"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.by_column = 1;
    configuration.inference.confidence_level = 0.95;
    const auto page = datalab::application::AnalysisService::one_way_anova(
        table, configuration);
    QCOMPARE(page.tables.size(), std::size_t{4});
    QCOMPARE(page.tables[2].title, std::string{"Tukey 同时比较"});
    QCOMPARE(page.tables[3].title, std::string{"Grouping Information"});
    QVERIFY(std::find(page.tables[2].headers.cbegin(), page.tables[2].headers.cend(),
                      std::string{"下限"})
            != page.tables[2].headers.cend());
    QVERIFY(std::find(page.tables[2].headers.cbegin(), page.tables[2].headers.cend(),
                      std::string{"上限"})
            != page.tables[2].headers.cend());
    QVERIFY(std::find(page.tables[2].headers.cbegin(), page.tables[2].headers.cend(),
                      std::string{"同时置信区间"})
            == page.tables[2].headers.cend());
    QVERIFY(page.facts.anova.has_value());
    QCOMPARE(page.facts.anova->tukey_interval_columns, std::string{"lower_upper"});
    QVERIFY(page.facts.anova->tukey_grouping_available);
    QVERIFY(page.facts.anova->grouping_letter_count >= 1);
    QVERIFY(std::any_of(page.tables[3].rows.cbegin(), page.tables[3].rows.cend(),
                        [](const std::vector<std::string>& row) {
                            return row.size() >= 4 && !row[3].empty();
                        }));
    // # source: formula_reference — equal-mean groups share a letter; separated means do not
    {
        const auto equal = datalab::domain::statistics::tukey_grouping_letters(
            {"A", "B", "C"}, {5.0, 5.0, 5.0}, {4, 4, 4}, {});
        QCOMPARE(equal.size(), std::size_t{3});
        QCOMPARE(equal[0].grouping, equal[1].grouping);
        QCOMPARE(equal[1].grouping, equal[2].grouping);
        datalab::domain::statistics::TukeyComparison ab;
        ab.first_label = "A";
        ab.second_label = "B";
        ab.significant = true;
        datalab::domain::statistics::TukeyComparison ac;
        ac.first_label = "A";
        ac.second_label = "C";
        ac.significant = true;
        datalab::domain::statistics::TukeyComparison bc;
        bc.first_label = "B";
        bc.second_label = "C";
        bc.significant = false;
        const auto separated = datalab::domain::statistics::tukey_grouping_letters(
            {"A", "B", "C"}, {10.0, 1.0, 1.1}, {5, 5, 5}, {ab, ac, bc});
        QCOMPARE(separated.size(), std::size_t{3});
        QCOMPARE(separated[0].label, std::string{"A"});
        QVERIFY(separated[0].grouping.find('A') != std::string::npos);
        QVERIFY(separated[1].grouping.find(separated[0].grouping.front())
                == std::string::npos);
        QVERIFY(separated[2].grouping.find(separated[0].grouping.front())
                == std::string::npos);
        QVERIFY(separated[1].grouping.find(separated[2].grouping.front())
                != std::string::npos
                || separated[2].grouping.find(separated[1].grouping.front())
                    != std::string::npos);
    }
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title == "Tukey 差值同时区间"
                                && plot.kind == datalab::domain::PlotKind::interval;
                        }));
    QVERIFY(std::any_of(page.tables[0].headers.cbegin(), page.tables[0].headers.cend(),
                        [](const std::string& header) {
                            return header.find("CI") != std::string::npos;
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::interval
                                && plot.title == "区间图"
                                && plot.interval_lower.size() == 3;
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title == "残差与拟合值";
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title == "残差与观测顺序"
                                && plot.source_rows.size() == 6
                                && plot.source_rows.front() == 0;
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::probability;
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title == "残差直方图"
                                && plot.kind == datalab::domain::PlotKind::histogram
                                && !plot.histogram_counts.empty()
                                && plot.histogram_edges.size() == plot.histogram_counts.size() + 1
                                && plot.source_rows.size() == 6;
                        }));
    const datalab::domain::PlotSpec* interval = nullptr;
    for (const auto& plot : page.plots) {
        if (plot.kind == datalab::domain::PlotKind::interval) {
            interval = &plot;
        }
    }
    QVERIFY(interval != nullptr);
    const double mse = 0.5;
    const double critical = datalab::domain::statistics::student_t_quantile(0.975, 3.0);
    const double half = critical * std::sqrt(mse / 2.0);
    QVERIFY(std::abs(interval->values.front() - 1.5) < 1.0e-12);
    QVERIFY(std::abs(interval->interval_lower.front() - (1.5 - half)) < 1.0e-9);
    QVERIFY(std::abs(interval->interval_upper.front() - (1.5 + half)) < 1.0e-9);
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

void QualityStatisticsTest::buildsChiSquareServiceOutputContract()
{
    datalab::domain::DataTable table;
    table.columns = {"Row", "Col"};
    table.rows = {
        {"A", "X"}, {"A", "X"}, {"A", "Y"},
        {"B", "X"}, {"B", "Y"}, {"B", "Y"},
        {"*", "X"}, {"A", "*"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.inference.row_category_column = 0;
    configuration.inference.column_category_column = 1;
    const auto page =
        datalab::application::AnalysisService::chi_square(table, configuration);
    QCOMPARE(page.method_name, std::string{"Chi-Square Association"});
    QCOMPARE(page.tables.size(), std::size_t{3});
    QCOMPARE(page.tables[0].title, std::string{"观察频数"});
    QCOMPARE(page.tables[1].title, std::string{"卡方检验"});
    QCOMPARE(page.tables[2].title, std::string{"单元格统计"});
    QVERIFY(page.parameter_summary.find("N* = 2") != std::string::npos);
    QVERIFY(page.facts.chi_square.has_value());
    QCOMPARE(page.facts.chi_square->total_count, std::size_t{6});
    QCOMPARE(page.facts.chi_square->missing_count, std::size_t{2});
    QCOMPARE(page.facts.chi_square->row_count, std::size_t{2});
    QCOMPARE(page.facts.chi_square->column_count, std::size_t{2});
    QVERIFY(page.facts.chi_square->likelihood_ratio_statistic.has_value());
    QVERIFY(page.tables[0].headers.back() == "合计");
    QVERIFY(page.tables[0].rows.back().front() == "合计");
    QCOMPARE(page.plots.size(), std::size_t{1});
    QCOMPARE(page.plots.front().kind, datalab::domain::PlotKind::heatmap);
    QCOMPARE(page.plots.front().title, std::string{"观察频数热图"});
    QCOMPARE(page.plots.front().categories.size(), std::size_t{2});
    QCOMPARE(page.plots.front().matrix_labels.size(), std::size_t{2});
    QCOMPARE(page.plots.front().matrix_values.size(), std::size_t{2});
    QVERIFY(page.facts.chi_square->plot_available);

    datalab::domain::OutputPage interpreted = page;
    datalab::application::InterpretationService::enrich(interpreted);
    bool mentioned_independence = false;
    for (const auto& section : interpreted.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("因果") == std::string::npos
                    || bullet.find("不能证明因果关系") != std::string::npos);
            QVERIFY(bullet.find("已证明") == std::string::npos);
            QVERIFY(bullet.find("显著相关") == std::string::npos);
            if (bullet.find("不能证明因果关系") != std::string::npos) {
                mentioned_independence = true;
            }
        }
    }
    QVERIFY(mentioned_independence);
}

void QualityStatisticsTest::buildsNonparametricServiceOutputContract()
{
    datalab::domain::DataTable mann_table;
    mann_table.columns = {"A", "B"};
    mann_table.rows = {{"1", "4"}, {"2", "5"}, {"*", "6"}, {"3", "7"}};
    datalab::domain::AnalysisConfiguration mann_config;
    mann_config.variable_columns = {0, 1};
    const auto mann_page =
        datalab::application::AnalysisService::mann_whitney(mann_table, mann_config);
    QCOMPARE(mann_page.plots.size(), std::size_t{2});
    QVERIFY(std::any_of(mann_page.plots.cbegin(), mann_page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::boxplot
                                && plot.box_labels.size() == 2;
                        }));
    const datalab::domain::PlotSpec* individuals = nullptr;
    for (const auto& plot : mann_page.plots) {
        if (plot.kind == datalab::domain::PlotKind::scatter
            && plot.title == "个体值图") {
            individuals = &plot;
            break;
        }
    }
    QVERIFY(individuals != nullptr);
    QCOMPARE(individuals->values.size(), std::size_t{7});
    QCOMPARE(individuals->source_rows.size(), std::size_t{7});
    QVERIFY(std::find(individuals->source_rows.cbegin(), individuals->source_rows.cend(),
                      std::size_t{2}) == individuals->source_rows.cend());
    QVERIFY(mann_page.facts.nonparametric.has_value());
    QCOMPARE(mann_page.facts.nonparametric->group_count, std::size_t{2});
    QCOMPARE(mann_page.facts.nonparametric->plot_point_count, std::size_t{7});
    QCOMPARE(mann_page.facts.nonparametric->missing_count, std::size_t{1});
    QVERIFY(mann_page.facts.nonparametric->location_estimate.has_value());
    QVERIFY(mann_page.facts.nonparametric->ci_lower.has_value());
    QVERIFY(mann_page.facts.nonparametric->ci_upper.has_value());

    datalab::domain::DataTable wilcoxon_table;
    wilcoxon_table.columns = {"Before", "After"};
    wilcoxon_table.rows = {{"1", "2"}, {"3", "4"}, {"*", "5"}, {"5", "6"}};
    datalab::domain::AnalysisConfiguration wilcoxon_config;
    wilcoxon_config.variable_columns = {0, 1};
    const auto wilcoxon_page = datalab::application::AnalysisService::wilcoxon_signed_rank(
        wilcoxon_table, wilcoxon_config);
    QVERIFY(wilcoxon_page.plots.size() >= std::size_t{3});
    QVERIFY(std::any_of(wilcoxon_page.diagnostics.cbegin(), wilcoxon_page.diagnostics.cend(),
                        [](const datalab::domain::DiagnosticMessage& diagnostic) {
                            return diagnostic.code == "missing_values";
                        }));
    const datalab::domain::PlotSpec* paired = nullptr;
    for (const auto& plot : wilcoxon_page.plots) {
        if (plot.title == "配对测量散点图") {
            paired = &plot;
            break;
        }
    }
    QVERIFY(paired != nullptr);
    QCOMPARE(paired->source_rows.size(), paired->values.size());
    QVERIFY(wilcoxon_page.facts.nonparametric.has_value());
    QCOMPARE(wilcoxon_page.facts.nonparametric->plot_point_count, std::size_t{6});

    datalab::domain::DataTable kruskal_table;
    kruskal_table.columns = {"Y", "Group"};
    kruskal_table.rows = {{"1", "A"}, {"2", "A"}, {"4", "B"}, {"5", "B"}, {"7", "C"}, {"8", "C"}};
    datalab::domain::AnalysisConfiguration kruskal_config;
    kruskal_config.variable_columns = {0};
    kruskal_config.by_column = 1;
    const auto kruskal_page = datalab::application::AnalysisService::kruskal_wallis(
        kruskal_table, kruskal_config);
    QCOMPARE(kruskal_page.plots.size(), std::size_t{2});
    QVERIFY(kruskal_page.facts.nonparametric.has_value());
    QCOMPARE(kruskal_page.facts.nonparametric->group_count, std::size_t{3});
    QCOMPARE(kruskal_page.facts.nonparametric->plot_point_count, std::size_t{6});
    QVERIFY(kruskal_page.facts.nonparametric->dunn_available);
    QCOMPARE(kruskal_page.facts.nonparametric->posthoc_pair_count, std::size_t{3});
    QVERIFY(std::any_of(
        kruskal_page.tables.cbegin(), kruskal_page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Dunn 成对比较";
        }));
    QVERIFY(std::any_of(
        kruskal_page.tables.cbegin(), kruskal_page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Grouping Information (Dunn)";
        }));
    for (const auto& plot : kruskal_page.plots) {
        if (plot.kind == datalab::domain::PlotKind::scatter && plot.title == "个体值图") {
            QCOMPARE(plot.source_rows.size(), plot.values.size());
            QVERIFY(!plot.source_rows.empty());
        }
    }

    kruskal_config.inference.nonparametric_posthoc = "steel_dwass";
    const auto steel_page = datalab::application::AnalysisService::kruskal_wallis(
        kruskal_table, kruskal_config);
    QVERIFY(steel_page.facts.nonparametric.has_value());
    QVERIFY(steel_page.facts.nonparametric->steel_dwass_available);
    QVERIFY(!steel_page.facts.nonparametric->dunn_available);
    QCOMPARE(steel_page.facts.nonparametric->posthoc_method, std::string{"steel_dwass"});
    QVERIFY(std::any_of(
        steel_page.tables.cbegin(), steel_page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Steel-Dwass 成对比较";
        }));
    QVERIFY(std::any_of(
        steel_page.tables.cbegin(), steel_page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Grouping Information (Steel-Dwass)";
        }));

    datalab::domain::DataTable friedman_table;
    friedman_table.columns = {"Y", "Treat", "Block"};
    friedman_table.rows = {
        {"1", "A", "1"}, {"2", "B", "1"},
        {"1", "A", "2"}, {"2", "B", "2"},
        {"*", "A", "x"},
        {"1", "A", "3"}, {"2", "B", "3"}};
    datalab::domain::AnalysisConfiguration friedman_config;
    friedman_config.variable_columns = {0};
    friedman_config.by_column = 1;
    friedman_config.inference.anova_factor_b_column = 2;
    auto friedman_page = datalab::application::AnalysisService::friedman(
        friedman_table, friedman_config);
    QVERIFY(friedman_page.facts.nonparametric.has_value());
    QCOMPARE(friedman_page.facts.nonparametric->method, std::string{"friedman"});
    QCOMPARE(friedman_page.facts.nonparametric->group_count, std::size_t{2});
    QVERIFY(friedman_page.facts.nonparametric->p_value.has_value());
    QVERIFY(std::any_of(
        friedman_page.diagnostics.cbegin(), friedman_page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "missing_values";
        }));
    QVERIFY(std::any_of(
        friedman_page.tables.cbegin(), friedman_page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Friedman 检验";
        }));
    QCOMPARE(friedman_page.plots.size(), std::size_t{2});
    for (const auto& plot : friedman_page.plots) {
        if (plot.kind == datalab::domain::PlotKind::scatter && plot.title == "个体值图") {
            QCOMPARE(plot.source_rows.size(), plot.values.size());
            QVERIFY(!plot.source_rows.empty());
        }
    }
    datalab::application::InterpretationService::enrich(friedman_page);
    for (const auto& section : friedman_page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("已证明") == std::string::npos);
        }
    }
    QVERIFY(!friedman_page.facts.nonparametric->nemenyi_available);

    // # source: formula_reference — Friedman + Nemenyi posthoc
    friedman_config.inference.nonparametric_posthoc = "nemenyi";
    auto nemenyi_page = datalab::application::AnalysisService::friedman(
        friedman_table, friedman_config);
    QVERIFY(nemenyi_page.facts.nonparametric.has_value());
    QVERIFY(nemenyi_page.facts.nonparametric->nemenyi_available);
    QCOMPARE(nemenyi_page.facts.nonparametric->posthoc_method, std::string{"nemenyi"});
    QVERIFY(nemenyi_page.facts.nonparametric->posthoc_pair_count >= std::size_t{1});
    QVERIFY(std::any_of(
        nemenyi_page.tables.cbegin(), nemenyi_page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Nemenyi 成对比较";
        }));
    QVERIFY(std::any_of(
        nemenyi_page.tables.cbegin(), nemenyi_page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Grouping Information (Nemenyi)";
        }));

    // # source: formula_reference — McNemar / Sign service wiring
    datalab::domain::DataTable mcnemar_table;
    mcnemar_table.columns = {"Before", "After"};
    mcnemar_table.rows = {
        {"1", "1"}, {"1", "0"}, {"1", "0"}, {"1", "0"}, {"1", "0"}, {"1", "0"},
        {"0", "1"}, {"0", "0"}, {"*", "1"}};
    datalab::domain::AnalysisConfiguration mcnemar_config;
    mcnemar_config.variable_columns = {0, 1};
    auto mcnemar_page = datalab::application::AnalysisService::mcnemar(
        mcnemar_table, mcnemar_config);
    QVERIFY(mcnemar_page.facts.mcnemar.has_value());
    QVERIFY(mcnemar_page.facts.mcnemar->computable);
    QCOMPARE(mcnemar_page.facts.mcnemar->b, std::size_t{5});
    QCOMPARE(mcnemar_page.facts.mcnemar->c, std::size_t{1});
    QVERIFY(std::any_of(
        mcnemar_page.tables.cbegin(), mcnemar_page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "McNemar 检验";
        }));
    datalab::application::InterpretationService::enrich(mcnemar_page);
    for (const auto& section : mcnemar_page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("已证明") == std::string::npos
                    || bullet.find("不能写成已证明") != std::string::npos);
        }
    }

    datalab::domain::DataTable sign_table;
    sign_table.columns = {"X"};
    sign_table.rows = {{"1"}, {"2"}, {"3"}, {"4"}, {"*"}};
    datalab::domain::AnalysisConfiguration sign_config;
    sign_config.variable_columns = {0};
    sign_config.inference.hypothesis_mean = 0.0;
    auto sign_page = datalab::application::AnalysisService::sign_test(
        sign_table, sign_config);
    QVERIFY(sign_page.facts.nonparametric.has_value());
    QCOMPARE(sign_page.facts.nonparametric->method, std::string{"sign_test"});
    QVERIFY(sign_page.facts.nonparametric->p_value.has_value());
    QVERIFY(std::abs(*sign_page.facts.nonparametric->p_value - 0.125) < 1.0e-12);
    QVERIFY(sign_page.facts.nonparametric->ci_lower.has_value());
    QVERIFY(sign_page.facts.nonparametric->ci_upper.has_value());
    QVERIFY(std::any_of(
        sign_page.tables.cbegin(), sign_page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "中位数置信区间";
        }));
    {
        const auto sign_interp =
            datalab::application::InterpretationService::enrich(sign_page);
        for (const auto& section : sign_interp.sections) {
            for (const auto& bullet : section.bullets) {
                QVERIFY(bullet.find("位置差异估计") == std::string::npos);
                QVERIFY(bullet.find("已证明") == std::string::npos
                        || bullet.find("不能写成已证明") != std::string::npos
                        || bullet.find("不能证明") != std::string::npos);
            }
        }
    }

    // # source: formula_reference — Sign CI / Mood group CI / RJ / paired Walsh
    {
        const auto ci = datalab::domain::statistics::sign_median_ci(
            {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, 0.95);
        QVERIFY(ci.estimate.has_value());
        QCOMPARE(*ci.estimate, 5.5);
        QVERIFY(ci.ci_lower.has_value());
        QVERIFY(ci.ci_upper.has_value());
        QVERIFY(*ci.ci_lower <= *ci.estimate);
        QVERIFY(*ci.ci_upper >= *ci.estimate);
        QVERIFY(ci.achieved_confidence.has_value());

        const auto sign_one = datalab::domain::statistics::sign_test(
            {1.0, 2.0, 3.0, 4.0}, 0.0);
        QVERIFY(sign_one.p_value.has_value());
        QVERIFY(std::abs(*sign_one.p_value - 0.125) < 1.0e-12);
        QVERIFY(sign_one.ci_lower.has_value());

        const auto single = datalab::domain::statistics::sign_median_ci({1.0}, 0.95);
        QVERIFY(!single.ci_lower.has_value());
    }
    {
        const auto mood = datalab::domain::statistics::mood_median_test(
            {{1.0, 2.0, 3.0, 4.0}, {20.0, 21.0, 22.0, 23.0}},
            {"A", "B"}, 0.95);
        QVERIFY(mood.p_value.has_value());
        QCOMPARE(mood.groups.size(), std::size_t{2});
        QVERIFY(mood.groups[0].ci_lower.has_value());
        QVERIFY(mood.groups[1].ci_lower.has_value());
        const auto a_ci = datalab::domain::statistics::sign_median_ci(
            {1.0, 2.0, 3.0, 4.0}, 0.95);
        QCOMPARE(*mood.groups[0].ci_lower, *a_ci.ci_lower);
        QCOMPARE(*mood.groups[0].ci_upper, *a_ci.ci_upper);
    }
    {
        // Near-symmetric sample: RJ should not strongly reject.
        const std::vector<double> approx_normal = {
            -1.2, -0.8, -0.3, 0.0, 0.2, 0.5, 0.9, 1.1, 1.4, 1.8};
        std::vector<std::size_t> rows(approx_normal.size());
        std::iota(rows.begin(), rows.end(), 0);
        const auto rj = datalab::domain::statistics::normality_test(
            approx_normal, rows, "ryan_joiner");
        QVERIFY(rj.ryan_joiner_r.has_value());
        QVERIFY(*rj.ryan_joiner_r > 0.9);
        QVERIFY(rj.p_value.has_value());
        QVERIFY(*rj.p_value >= 0.05 || rj.decision == "fail_to_reject");

        const std::vector<double> skewed = {
            1, 1, 1, 1, 2, 2, 3, 10, 20, 50};
        std::vector<std::size_t> skew_rows(skewed.size());
        std::iota(skew_rows.begin(), skew_rows.end(), 0);
        const auto rj_skew = datalab::domain::statistics::normality_test(
            skewed, skew_rows, "ryan_joiner");
        QVERIFY(rj_skew.ryan_joiner_r.has_value());
        QVERIFY(rj_skew.p_value.has_value());
        QVERIFY(*rj_skew.p_value <= 0.10);

        const auto ad_default = datalab::domain::statistics::normality_test(approx_normal);
        QCOMPARE(ad_default.method, std::string{"anderson_darling"});
        QVERIFY(ad_default.anderson_darling.has_value());
    }
    {
        const std::vector<double> first = {10, 12, 14, 16, 18, 20};
        const std::vector<double> second = {9, 11, 13, 15, 17, 19};
        const auto paired = datalab::domain::statistics::wilcoxon_signed_rank(
            first, second, datalab::domain::statistics::TestAlternative::two_sided, 0.95);
        QVERIFY(paired.location_estimate.has_value());
        QVERIFY(paired.ci_lower.has_value());
        QVERIFY(paired.ci_upper.has_value());
        std::vector<double> diffs;
        for (std::size_t i = 0; i < first.size(); ++i) {
            diffs.push_back(first[i] - second[i]);
        }
        const auto one = datalab::domain::statistics::wilcoxon_signed_rank_one_sample(
            diffs, 0.0, datalab::domain::statistics::TestAlternative::two_sided, 0.95);
        QCOMPARE(*paired.location_estimate, *one.location_estimate);
        QCOMPARE(*paired.ci_lower, *one.ci_lower);
        QCOMPARE(*paired.ci_upper, *one.ci_upper);
        QVERIFY(paired.p_value.has_value());
        QCOMPARE(*paired.p_value, *one.p_value);

        datalab::domain::DataTable paired_table;
        paired_table.columns = {"A", "B"};
        for (std::size_t i = 0; i < first.size(); ++i) {
            paired_table.rows.push_back({
                std::to_string(first[i]), std::to_string(second[i])});
        }
        datalab::domain::AnalysisConfiguration paired_config;
        paired_config.variable_columns = {0, 1};
        paired_config.inference.confidence_level = 0.95;
        auto paired_page = datalab::application::AnalysisService::wilcoxon_signed_rank(
            paired_table, paired_config);
        QVERIFY(paired_page.facts.nonparametric.has_value());
        QCOMPARE(paired_page.facts.nonparametric->method,
                 std::string{"wilcoxon_signed_rank"});
        QVERIFY(paired_page.facts.nonparametric->location_estimate.has_value());
        QVERIFY(paired_page.facts.nonparametric->ci_lower.has_value());
        QVERIFY(std::any_of(
            paired_page.tables.cbegin(), paired_page.tables.cend(),
            [](const datalab::domain::StatisticTable& table_out) {
                return table_out.title == "位置估计（Walsh）";
            }));
    }

    // # source: formula_reference — Mood / Cochran / one-sample Wilcoxon
    {
        const auto mood = datalab::domain::statistics::mood_median_test(
            {{1.0, 2.0, 3.0, 4.0}, {20.0, 21.0, 22.0, 23.0}},
            {"A", "B"});
        QVERIFY(mood.p_value.has_value());
        QCOMPARE(mood.groups.size(), std::size_t{2});
        QVERIFY(mood.chi_square > 0.0);
        QVERIFY(*mood.p_value < 0.05);

        datalab::domain::DataTable mood_table;
        mood_table.columns = {"Y", "G"};
        mood_table.rows = {
            {"1", "A"}, {"2", "A"}, {"3", "A"}, {"4", "A"},
            {"20", "B"}, {"21", "B"}, {"22", "B"}, {"23", "B"}};
        datalab::domain::AnalysisConfiguration mood_config;
        mood_config.variable_columns = {0};
        mood_config.by_column = 1;
        auto mood_page = datalab::application::AnalysisService::mood_median(
            mood_table, mood_config);
        QVERIFY(mood_page.facts.nonparametric.has_value());
        QCOMPARE(mood_page.facts.nonparametric->method, std::string{"mood_median"});
        QVERIFY(mood_page.facts.nonparametric->p_value.has_value());
        QVERIFY(std::any_of(
            mood_page.tables.cbegin(), mood_page.tables.cend(),
            [](const datalab::domain::StatisticTable& table_out) {
                return table_out.title == "Mood 中位数检验";
            }));
        QVERIFY(std::any_of(
            mood_page.tables.cbegin(), mood_page.tables.cend(),
            [](const datalab::domain::StatisticTable& table_out) {
                return table_out.title == "各组 Above/Below"
                    && table_out.headers.size() >= 7
                    && table_out.headers[5] == "CI 下限"
                    && table_out.headers[6] == "CI 上限";
            }));
        QCOMPARE(mood_page.plots.size(), std::size_t{2});
        const auto mood_interp =
            datalab::application::InterpretationService::enrich(mood_page);
        for (const auto& section : mood_interp.sections) {
            for (const auto& bullet : section.bullets) {
                QVERIFY(bullet.find("已证明") == std::string::npos
                        || bullet.find("不能写成已证明") != std::string::npos
                        || bullet.find("不能证明") != std::string::npos);
                QVERIFY(bullet.find("位置差异估计") == std::string::npos);
            }
        }
    }
    {
        const auto cochran = datalab::domain::statistics::cochran_q_test(
            {{1, 1, 0}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}, {0, 0, 1}, {1, 0, 0}},
            {"T1", "T2", "T3"});
        QVERIFY(cochran.computable);
        QVERIFY(cochran.p_value.has_value());
        QCOMPARE(cochran.treatment_count, std::size_t{3});
        QCOMPARE(cochran.subject_count, std::size_t{6});

        datalab::domain::DataTable cq_table;
        cq_table.columns = {"T1", "T2", "T3"};
        cq_table.rows = {
            {"1", "1", "0"}, {"1", "0", "1"}, {"0", "1", "1"},
            {"1", "1", "1"}, {"0", "0", "1"}, {"1", "0", "0"}, {"*", "1", "0"}};
        datalab::domain::AnalysisConfiguration cq_config;
        cq_config.variable_columns = {0, 1, 2};
        auto cq_page = datalab::application::AnalysisService::cochran_q(
            cq_table, cq_config);
        QVERIFY(cq_page.facts.cochran_q.has_value());
        QVERIFY(cq_page.facts.cochran_q->computable);
        QCOMPARE(cq_page.facts.cochran_q->missing_count, std::size_t{1});
        QVERIFY(std::any_of(
            cq_page.tables.cbegin(), cq_page.tables.cend(),
            [](const datalab::domain::StatisticTable& table_out) {
                return table_out.title == "Cochran Q 检验";
            }));

        datalab::domain::AnalysisConfiguration cq2;
        cq2.variable_columns = {0, 1};
        auto cq2_page = datalab::application::AnalysisService::cochran_q(
            cq_table, cq2);
        QVERIFY(cq2_page.facts.cochran_q.has_value());
        QVERIFY(!cq2_page.facts.cochran_q->computable);
        QVERIFY(std::any_of(
            cq2_page.diagnostics.cbegin(), cq2_page.diagnostics.cend(),
            [](const datalab::domain::DiagnosticMessage& d) {
                return d.code == "cochran_use_mcnemar";
            }));
    }
    {
        const auto one = datalab::domain::statistics::wilcoxon_signed_rank_one_sample(
            {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, 0.0);
        QVERIFY(one.p_value.has_value());
        QVERIFY(one.location_estimate.has_value());
        QVERIFY(one.ci_lower.has_value());
        QVERIFY(one.ci_upper.has_value());
        QVERIFY(*one.p_value < 0.05);

        datalab::domain::DataTable w_table;
        w_table.columns = {"X"};
        for (int value = 1; value <= 8; ++value) {
            w_table.rows.push_back({std::to_string(value)});
        }
        datalab::domain::AnalysisConfiguration w_config;
        w_config.variable_columns = {0};
        w_config.inference.hypothesis_mean = 0.0;
        auto w_page = datalab::application::AnalysisService::wilcoxon_signed_rank(
            w_table, w_config);
        QVERIFY(w_page.facts.nonparametric.has_value());
        QCOMPARE(w_page.facts.nonparametric->method, std::string{"wilcoxon_one_sample"});
        QVERIFY(w_page.facts.nonparametric->location_estimate.has_value());
        QVERIFY(std::any_of(
            w_page.tables.cbegin(), w_page.tables.cend(),
            [](const datalab::domain::StatisticTable& table_out) {
                return table_out.title == "位置估计（Walsh）";
            }));
        // paired path still requires two columns and keeps scatter
        datalab::domain::DataTable paired = wilcoxon_table;
        datalab::domain::AnalysisConfiguration paired_config = wilcoxon_config;
        const auto paired_page = datalab::application::AnalysisService::wilcoxon_signed_rank(
            paired, paired_config);
        QCOMPARE(paired_page.facts.nonparametric->method, std::string{"wilcoxon_signed_rank"});
        QVERIFY(paired_page.plots.size() >= std::size_t{3});
    }
}

void QualityStatisticsTest::calculatesMckeanRyanConfidenceInterval()
{
    // # source: formula_reference
    const auto separated = datalab::domain::statistics::mann_whitney(
        {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0},
        {11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0});
    QVERIFY(separated.location_estimate.has_value());
    QVERIFY(separated.ci_lower.has_value());
    QVERIFY(separated.ci_upper.has_value());
    QVERIFY(*separated.ci_lower <= *separated.location_estimate);
    QVERIFY(*separated.location_estimate <= *separated.ci_upper);
    QVERIFY(*separated.ci_upper - *separated.ci_lower > 0.0);

    const auto tied = datalab::domain::statistics::mann_whitney(
        {1.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0},
        {2.0, 3.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0});
    QVERIFY(tied.location_estimate.has_value());
    QVERIFY(tied.ci_lower.has_value());
    QVERIFY(tied.ci_upper.has_value());
    QVERIFY(*tied.ci_lower <= *tied.location_estimate);
    QVERIFY(*tied.location_estimate <= *tied.ci_upper);
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

void QualityStatisticsTest::buildsBoxCoxServiceOutputContract()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    table.rows = {{"1.0"}, {"*"}, {"2.0"}, {"4.0"}, {"8.0"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.specifications.lower = 0.5;
    configuration.specifications.upper = 10.0;
    const auto page = datalab::application::AnalysisService::box_cox(table, configuration);
    QVERIFY(page.facts.box_cox.has_value());
    QCOMPARE(page.facts.box_cox->n, std::size_t{4});
    QCOMPARE(page.facts.box_cox->missing_count, std::size_t{1});
    QVERIFY(page.plots.size() >= std::size_t{3});
    QCOMPARE(page.plots[0].title, std::string("Box-Cox λ 选择诊断"));
    QCOMPARE(page.plots[1].title, std::string("变换前正态概率图"));
    QCOMPARE(page.plots[2].title, std::string("变换后正态概率图"));
    QCOMPARE(page.plots[1].kind, datalab::domain::PlotKind::probability);
    QCOMPARE(page.plots[1].source_rows, (std::vector<std::size_t>{0, 2, 3, 4}));
    QCOMPARE(page.plots[2].source_rows, page.plots[1].source_rows);
    QVERIFY(std::any_of(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "变换后过程能力";
        }));
    QCOMPARE(page.method_metadata.estimation_method, std::string("box_cox_grid"));
    QCOMPARE(page.method_metadata.parameter_source, std::string("estimated"));
}

void QualityStatisticsTest::regressionAnovaSeqAdjSs()
{
    const auto regression = datalab::domain::statistics::fit_linear_regression(
        {3.0, 5.0, 7.0, 9.0, 11.0, 13.0},
        {{1.0, 0.0}, {2.0, 1.0}, {3.0, 0.0}, {4.0, 1.0}, {5.0, 0.0}, {6.0, 1.0}},
        {"X1", "X2"});
    QCOMPARE(regression.anova_effects.size(), std::size_t{2});
    for (const auto& effect : regression.anova_effects) {
        QVERIFY(effect.estimable);
        QVERIFY(effect.sequential_sum_of_squares.has_value());
        QVERIFY(*effect.sequential_sum_of_squares >= 0.0);
        QVERIFY(effect.adjusted_sum_of_squares.has_value());
        QVERIFY(*effect.adjusted_sum_of_squares >= 0.0);
    }
    double adjusted_total = 0.0;
    for (const auto& effect : regression.anova_effects) {
        adjusted_total += *effect.adjusted_sum_of_squares;
    }
    QVERIFY(qAbs(adjusted_total + regression.error_sum_of_squares
                 - regression.total_sum_of_squares) < 1.0e-6);
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
    QVERIFY(gage.variance_components.size() == 7);
    QVERIFY(gage.ndc >= 0.0);

    const auto mann_whitney = datalab::domain::statistics::mann_whitney(
        {1.0, 2.0, 3.0}, {5.0, 6.0, 7.0});
    QVERIFY(mann_whitney.p_value.has_value());
    QVERIFY(mann_whitney.rank_sum < 10.0);
    QVERIFY(mann_whitney.effect_size.has_value());
    QVERIFY(mann_whitney.continuity_correction);
    QVERIFY(mann_whitney.small_sample_warning);

    // # source: formula_reference
    const auto tied_mw = datalab::domain::statistics::mann_whitney(
        {1.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0},
        {2.0, 3.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0});
    QVERIFY(tied_mw.tie_correction);
    QVERIFY(tied_mw.p_value.has_value());
    QVERIFY(tied_mw.p_value_without_tie_correction.has_value());
    QVERIFY(*tied_mw.p_value <= *tied_mw.p_value_without_tie_correction + 1.0e-12);

    const auto signed_rank = datalab::domain::statistics::wilcoxon_signed_rank(
        {2.0, 4.0, 6.0, 8.0}, {1.0, 3.0, 5.0, 7.0});
    QVERIFY(signed_rank.p_value.has_value());
    QVERIFY(signed_rank.small_sample_warning);

    const auto tied_signed = datalab::domain::statistics::wilcoxon_signed_rank(
        {2.0, 4.0, 6.0, 9.0, 11.0, 13.0, 15.0, 17.0, 19.0, 21.0},
        {1.0, 3.0, 5.0, 8.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0});
    QVERIFY(tied_signed.tie_correction);
    QVERIFY(tied_signed.p_value.has_value());

    const auto kruskal = datalab::domain::statistics::kruskal_wallis(
        {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}, {7.0, 8.0, 9.0}},
        {"A", "B", "C"});
    QVERIFY(kruskal.p_value.has_value());
    QCOMPARE(kruskal.groups.size(), std::size_t{3});
    QVERIFY(kruskal.groups[0].z_value.has_value());
    QVERIFY(kruskal.p_value_unadjusted.has_value());
    QVERIFY(kruskal.small_sample_warning);
    QCOMPARE(kruskal.dunn_comparisons.size(), std::size_t{3});
    // # source: formula_reference — fully separated ranks: A vs C must be significant
    bool saw_ac = false;
    for (const auto& comparison : kruskal.dunn_comparisons) {
        if ((comparison.first_label == "A" && comparison.second_label == "C")
            || (comparison.first_label == "C" && comparison.second_label == "A")) {
            saw_ac = true;
            QVERIFY(comparison.significant);
            QVERIFY(comparison.adjusted_p_value.has_value());
            QVERIFY(*comparison.adjusted_p_value <= 0.05);
        }
    }
    QVERIFY(saw_ac);

    const auto equal_groups = datalab::domain::statistics::kruskal_wallis(
        {{1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}},
        {"X", "Y", "Z"});
    QCOMPARE(equal_groups.dunn_comparisons.size(), std::size_t{3});
    for (const auto& comparison : equal_groups.dunn_comparisons) {
        QVERIFY(!comparison.significant);
    }

    // # source: formula_reference — Steel-Dwass pairwise Wilcoxon + asymptotic TK
    const auto steel = datalab::domain::statistics::steel_dwass_pairwise(
        {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}, {7.0, 8.0, 9.0}},
        {"A", "B", "C"}, 0.05);
    QCOMPARE(steel.size(), std::size_t{3});
    bool saw_steel_ac = false;
    for (const auto& comparison : steel) {
        if ((comparison.first_label == "A" && comparison.second_label == "C")
            || (comparison.first_label == "C" && comparison.second_label == "A")) {
            saw_steel_ac = true;
            QVERIFY(comparison.significant);
        }
    }
    QVERIFY(saw_steel_ac);
    const auto steel_equal = datalab::domain::statistics::steel_dwass_pairwise(
        {{1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}},
        {"X", "Y", "Z"}, 0.05);
    for (const auto& comparison : steel_equal) {
        QVERIFY(!comparison.significant);
    }

    // # source: formula_reference — Friedman balanced 2 treatments × 3 blocks
    // Block ranks: (1,2), (1,2), (1,2) → R1=3, R2=6; S=12/(3*2*3)*(9+36)-3*3*3=3
    const auto friedman = datalab::domain::statistics::friedman_test(
        {1.0, 2.0, 1.0, 2.0, 1.0, 2.0},
        {"A", "B", "A", "B", "A", "B"},
        {"1", "1", "2", "2", "3", "3"});
    QVERIFY(friedman.diagnostics.empty());
    QCOMPARE(friedman.block_count, std::size_t{3});
    QCOMPARE(friedman.treatment_count, std::size_t{2});
    QVERIFY(std::abs(friedman.s_statistic - 3.0) < 1.0e-12);
    QCOMPARE(friedman.degrees_of_freedom, 1.0);
    QVERIFY(friedman.p_value.has_value());

    // # source: formula_reference — Nemenyi on Friedman mean ranks
    // mean ranks 1 and 2, SE=√(2*3/(6*3))=√(1/3), |Z|=√3 ≈ 1.732
    const auto nemenyi = datalab::domain::statistics::nemenyi_pairwise(friedman, 0.05);
    QCOMPARE(nemenyi.size(), std::size_t{1});
    QVERIFY(std::abs(nemenyi.front().standard_error - std::sqrt(1.0 / 3.0)) < 1.0e-12);
    QVERIFY(std::abs(nemenyi.front().z_statistic - std::sqrt(3.0)) < 1.0e-12);
    QVERIFY(nemenyi.front().p_value.has_value());

    const auto friedman_unbalanced = datalab::domain::statistics::friedman_test(
        {1.0, 2.0, 1.0},
        {"A", "B", "A"},
        {"1", "1", "2"});
    QVERIFY(std::any_of(
        friedman_unbalanced.diagnostics.cbegin(),
        friedman_unbalanced.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "friedman_unbalanced";
        }));

    // # source: formula_reference — McNemar Edwards
    // a=1,b=5,c=1,d=1 → χ²=(|5-1|-1)²/6 = 9/6 = 1.5
    const auto mcnemar = datalab::domain::statistics::mcnemar_test(
        {"1", "1", "1", "1", "1", "1", "0", "0"},
        {"1", "0", "0", "0", "0", "0", "1", "0"});
    QVERIFY(mcnemar.diagnostics.empty());
    QCOMPARE(mcnemar.a, std::size_t{1});
    QCOMPARE(mcnemar.b, std::size_t{5});
    QCOMPARE(mcnemar.c, std::size_t{1});
    QCOMPARE(mcnemar.d, std::size_t{1});
    QVERIFY(std::abs(mcnemar.chi_square - 1.5) < 1.0e-12);
    QVERIFY(mcnemar.p_value.has_value());

    const auto mcnemar_tie = datalab::domain::statistics::mcnemar_test(
        {"pass", "pass", "fail", "fail"},
        {"fail", "fail", "pass", "pass"});
    QVERIFY(mcnemar_tie.diagnostics.empty());
    QCOMPARE(mcnemar_tie.b, std::size_t{2});
    QCOMPARE(mcnemar_tie.c, std::size_t{2});
    QVERIFY(std::abs(mcnemar_tie.chi_square - 0.0) < 1.0e-12);

    const auto mcnemar_none = datalab::domain::statistics::mcnemar_test(
        {"1", "0"}, {"1", "0"});
    QVERIFY(std::any_of(
        mcnemar_none.diagnostics.cbegin(), mcnemar_none.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "mcnemar_no_discordant";
        }));

    // # source: formula_reference — Sign test binomial exact
    // values all > 0 except ties: n+=4,n-=0 → two-sided p = 2*(1/16)=0.125
    const auto sign_one = datalab::domain::statistics::sign_test(
        {1.0, 2.0, 3.0, 4.0}, 0.0);
    QCOMPARE(sign_one.n_positive, std::size_t{4});
    QCOMPARE(sign_one.n_negative, std::size_t{0});
    QVERIFY(sign_one.p_value.has_value());
    QVERIFY(std::abs(*sign_one.p_value - 0.125) < 1.0e-12);

    const auto sign_paired = datalab::domain::statistics::sign_test_paired(
        {5.0, 6.0, 7.0, 8.0}, {1.0, 2.0, 3.0, 4.0});
    QCOMPARE(sign_paired.n_positive, std::size_t{4});
    QVERIFY(sign_paired.p_value.has_value());
    QVERIFY(std::abs(*sign_paired.p_value - 0.125) < 1.0e-12);

    const auto sign_all_ties = datalab::domain::statistics::sign_test(
        {2.0, 2.0, 2.0}, 2.0);
    QVERIFY(std::any_of(
        sign_all_ties.diagnostics.cbegin(), sign_all_ties.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "sign_test_no_nonzero";
        }));
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
    QVERIFY(arima.size() > 3);
    QVERIFY(arima[0].forecasts.size() == 3);
    QVERIFY(std::isfinite(arima[0].aicc));
}

void QualityStatisticsTest::buildsArimaServiceOutputContract()
{
    datalab::domain::DataTable table;
    table.columns = {"Time", "Value"};
    table.rows = {
        {"1", "100.0"}, {"2", "101.2"}, {"3", "100.8"}, {"4", "102.1"},
        {"5", "103.0"}, {"6", "102.7"}, {"7", "104.2"}, {"8", "105.1"},
        {"9", "104.8"}, {"10", "106.0"}, {"11", "bad"}, {"12", "106.8"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.time_series.arima_time_column = 0;
    configuration.time_series.arima_value_column = 1;
    configuration.time_series.arima_differencing = 1;
    configuration.time_series.arima_selection_criterion = "aicc";
    configuration.time_series.forecast_periods = 3;
    const auto page = datalab::application::AnalysisService::arima(table, configuration);
    QCOMPARE(page.method_name, std::string{"ARIMA"});
    QVERIFY(page.tables.size() >= std::size_t{3});
    QCOMPARE(page.tables[0].title, std::string{"候选模型比较"});
    QCOMPARE(page.tables[1].title, std::string{"模型摘要与预测"});
    QCOMPARE(page.tables[1].headers[4], std::string{"Forecast"});
    QCOMPARE(page.tables[2].title, std::string{"拟合与预测明细"});
    QCOMPARE(page.tables[2].headers[1], std::string{"原始行"});
    QCOMPARE(page.tables[2].rows.front()[1], std::string{"1"});
    QVERIFY(page.tables[2].rows.size() >= std::size_t{14});
    QCOMPARE(page.tables[2].rows[10][1], std::string{"12"});
    QVERIFY(page.tables[2].rows[11][2].empty());
    QVERIFY(!page.tables[2].rows[11][5].empty());
    QVERIFY(page.facts.forecast.has_value());
    QVERIFY(page.facts.forecast->mape.has_value());
    QCOMPARE(page.method_metadata.estimation_method, std::string{"arima_candidate_css"});
    QCOMPARE(page.method_metadata.parameter_source, std::string{"estimated"});
    QCOMPARE(page.method_metadata.valid_count, std::size_t{11});
    QCOMPARE(page.method_metadata.missing_count, std::size_t{1});
    QCOMPARE(page.method_metadata.source_rows.size(), std::size_t{11});
}

void QualityStatisticsTest::buildsSeasonalForecastingServiceOutput()
{
    datalab::domain::DataTable table;
    table.columns = {"Demand"};
    table.rows = {
        {"100"}, {"110"}, {"120"}, {"105"},
        {"104"}, {"115"}, {""}, {"109"},
        {"108"}, {"119"}, {"129"}, {"113"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.selection.measurement_column = 0;
    configuration.time_series.seasonal_period = 4;
    configuration.time_series.forecast_periods = 3;
    configuration.time_series.seasonal_error_model = "additive";
    configuration.time_series.seasonal_trend_model = "additive";
    configuration.time_series.validation_horizon = 1;
    configuration.time_series.validation_step = 1;
    const auto page = datalab::application::AnalysisService::seasonal_forecasting(
        table, configuration);
    QCOMPARE(page.method_name, std::string{"Holt-Winters Seasonal Forecasting"});
    QVERIFY(page.tables.size() >= std::size_t{4});
    QCOMPARE(page.tables[0].title, std::string{"预测准确度"});
    QCOMPARE(page.tables[1].title, std::string{"拟合与预测明细"});
    QCOMPARE(page.tables[1].headers[5], std::string{"Forecast"});
    bool has_sarima_table = false;
    bool has_rolling_detail = false;
    for (const auto& output_table : page.tables) {
        if (output_table.title == "SARIMA 候选模型比较") {
            has_sarima_table = true;
        }
        if (output_table.title == "Rolling-origin 明细") {
            has_rolling_detail = true;
        }
    }
    QVERIFY(has_sarima_table);
    QVERIFY(has_rolling_detail);
    QVERIFY(page.facts.forecast.has_value());
    QVERIFY(page.facts.forecast->mape.has_value());
    QCOMPARE(page.method_metadata.estimation_method, std::string{"holt_winters_additive"});
    QCOMPARE(page.method_metadata.parameter_source, std::string{"specified"});
    QCOMPARE(page.method_metadata.valid_count, std::size_t{11});
    QCOMPARE(page.method_metadata.missing_count, std::size_t{1});
    QCOMPARE(page.method_metadata.source_rows.size(), std::size_t{11});
    QCOMPARE(page.plots.size(), std::size_t{1});
    QCOMPARE(page.plots.front().series.size(), std::size_t{4});
    bool has_sarima_diagnostic = false;
    for (const auto& diagnostic : page.diagnostics) {
        if (diagnostic.code == "sarima_css_approximation") {
            has_sarima_diagnostic = true;
            break;
        }
    }
    QVERIFY(has_sarima_diagnostic);
}

void QualityStatisticsTest::buildsTimeSeriesDecompositionServiceOutputContract()
{
    datalab::domain::DataTable table;
    table.columns = {"Time", "Demand"};
    table.rows = {
        {"1", "100"}, {"2", "112"}, {"3", "125"}, {"4", "118"},
        {"5", "108"}, {"6", "121"}, {"7", "133"}, {"8", "126"},
        {"9", "bad"}, {"10", "130"}, {"11", "142"}, {"12", "136"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.time_series.decomposition_time_column = 0;
    configuration.time_series.decomposition_value_column = 1;
    configuration.time_series.decomposition_seasonal_period = 4;
    configuration.time_series.decomposition_model = "additive";
    configuration.time_series.forecast_periods = 3;
    const auto page = datalab::application::AnalysisService::time_series_decomposition(
        table, configuration);
    QCOMPARE(page.method_name, std::string{"Time Series Decomposition"});
    QVERIFY(page.tables.size() >= std::size_t{3});
    QCOMPARE(page.tables[0].title, std::string{"预测准确度"});
    QCOMPARE(page.tables[1].title, std::string{"拟合与预测明细"});
    QCOMPARE(page.tables[1].headers[1], std::string{"原始行"});
    QCOMPARE(page.tables[1].headers[9], std::string{"Forecast"});
    QCOMPARE(page.tables[2].title, std::string{"季节指数"});
    QVERIFY(page.facts.forecast.has_value());
    QVERIFY(page.facts.forecast->mape.has_value());
    QCOMPARE(page.method_metadata.estimation_method,
             std::string{"classical_decomposition_cma_trend"});
    QCOMPARE(page.method_metadata.parameter_source, std::string{"estimated"});
    QCOMPARE(page.method_metadata.valid_count, std::size_t{11});
    QCOMPARE(page.method_metadata.missing_count, std::size_t{1});
    QCOMPARE(page.method_metadata.source_rows.size(), std::size_t{11});
    QVERIFY(page.plots.size() >= std::size_t{1});
    QVERIFY(page.plots.front().series.size() >= std::size_t{4});
    bool has_missing_diagnostic = false;
    for (const auto& diagnostic : page.diagnostics) {
        if (diagnostic.code == "missing_values") {
            has_missing_diagnostic = true;
            break;
        }
    }
    QVERIFY(has_missing_diagnostic);
}

void QualityStatisticsTest::buildsTimeSeriesSmoothingServiceOutputContract()
{
    datalab::domain::DataTable table;
    table.columns = {"Demand"};
    table.rows = {{"100"}, {"108"}, {"116"}, {""}, {"123"}, {"131"}, {"126"}, {"134"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.time_series.smoothing_method = "double";
    configuration.time_series.smoothing_alpha = 0.3;
    configuration.time_series.smoothing_gamma = 0.2;
    configuration.time_series.forecast_periods = 2;
    const auto page = datalab::application::AnalysisService::time_series_smoothing(
        table, configuration);
    QCOMPARE(page.method_name, std::string{"Double Exponential Smoothing"});
    QCOMPARE(page.tables.size(), std::size_t{2});
    QCOMPARE(page.tables[0].title, std::string{"拟合与预测明细"});
    QCOMPARE(page.tables[0].headers[1], std::string{"原始行"});
    QCOMPARE(page.tables[1].title, std::string{"预测准确度"});
    QVERIFY(page.facts.forecast.has_value());
    QVERIFY(page.facts.forecast->mape.has_value());
    QCOMPARE(page.method_metadata.estimation_method, std::string{"holt_linear_des"});
    QCOMPARE(page.method_metadata.parameter_source, std::string{"estimated"});
    QCOMPARE(page.method_metadata.valid_count, std::size_t{7});
    QCOMPARE(page.method_metadata.missing_count, std::size_t{1});
    QCOMPARE(page.method_metadata.source_rows.size(), std::size_t{7});
    QCOMPARE(page.plots.size(), std::size_t{1});
    QCOMPARE(page.plots.front().series.size(), std::size_t{4});
}

void QualityStatisticsTest::buildsSingleExponentialSmoothingServiceOutputContract()
{
    datalab::domain::DataTable table;
    table.columns = {"Demand"};
    table.rows = {{"100"}, {"105"}, {"110"}, {"115"}, {"120"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.time_series.smoothing_method = "single";
    configuration.time_series.smoothing_alpha = 0.4;
    configuration.time_series.forecast_periods = 2;
    const auto page = datalab::application::AnalysisService::time_series_smoothing(
        table, configuration);
    QCOMPARE(page.method_name, std::string{"Single Exponential Smoothing"});
    QCOMPARE(page.method_metadata.estimation_method, std::string{"single_exponential_ses"});
    QCOMPARE(page.tables[0].title, std::string{"拟合与预测明细"});
    QCOMPARE(page.tables[1].title, std::string{"预测准确度"});
    QVERIFY(page.facts.forecast.has_value());
    QCOMPARE(page.plots.front().series.size(), std::size_t{4});
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

    std::vector<double> seasonal_series;
    seasonal_series.reserve(40);
    for (int index = 0; index < 40; ++index) {
        seasonal_series.push_back(
            10.0 + 2.0 * std::sin(2.0 * 3.141592653589793 * index / 4.0)
            + 0.15 * static_cast<double>(index));
    }
    const auto sarima_candidates =
        datalab::domain::statistics::fit_best_sarima_candidates(seasonal_series, 4);
    QVERIFY(!sarima_candidates.empty());
    bool has_mixed = false;
    bool has_css_diagnostic = false;
    for (const auto& candidate : sarima_candidates) {
        if (candidate.order.p > 0 && candidate.order.q > 0) {
            has_mixed = true;
        }
        for (const auto& message : candidate.diagnostics) {
            if (message.code == "sarima_css_approximation") {
                has_css_diagnostic = true;
            }
        }
    }
    QVERIFY(has_mixed);
    QVERIFY(has_css_diagnostic);
    double seasonal_ar_sse = std::numeric_limits<double>::infinity();
    double ar1_only_sse = std::numeric_limits<double>::infinity();
    for (const auto& candidate : sarima_candidates) {
        if (candidate.order.seasonal_p > 0) {
            seasonal_ar_sse = std::min(seasonal_ar_sse, candidate.sse);
        }
        if (candidate.order.p == 1 && candidate.order.q == 0
            && candidate.order.seasonal_p == 0 && candidate.order.seasonal_q == 0
            && candidate.order.seasonal_d == 1) {
            ar1_only_sse = std::min(ar1_only_sse, candidate.sse);
        }
    }
    QVERIFY(seasonal_ar_sse < ar1_only_sse);

    datalab::domain::statistics::PcaOptions pca_options;
    pca_options.mode = datalab::domain::statistics::PcaMode::standardized;
    const auto pca = datalab::domain::statistics::pca(
        {{1.0, 2.0}, {2.0, 4.0}, {3.0, 6.0}, {4.0, 8.0}}, pca_options);
    QCOMPARE(pca.variable_count, std::size_t{2});
    QVERIFY(pca.eigenvalues.size() == 2);
    QVERIFY(pca.converged);
    QVERIFY(pca.coefficients.size() == 2);
    double coefficient_norm = 0.0;
    for (std::size_t variable = 0; variable < pca.coefficients.size(); ++variable) {
        coefficient_norm += pca.coefficients[variable][0] * pca.coefficients[variable][0];
    }
    QVERIFY(qAbs(coefficient_norm - 1.0) < 1.0e-8);
    QVERIFY(qAbs(pca.explained_variance_ratio[0] + pca.explained_variance_ratio[1] - 1.0)
            < 1.0e-8);

    datalab::domain::statistics::PcaOptions truncated;
    truncated.component_count = 1;
    const auto pca_one = datalab::domain::statistics::pca(
        {{1.0, 0.0, 0.2}, {2.0, 1.0, 0.1}, {3.0, 0.5, 1.4}, {4.0, 2.0, 0.8},
         {5.0, 1.5, 2.0}}, truncated);
    QCOMPARE(pca_one.retained_component_count, std::size_t{1});
    QCOMPARE(pca_one.eigenvalues.size(), std::size_t{3});
    QVERIFY(pca_one.cumulative_explained_variance_ratio[0] < 1.0 - 1.0e-8);

    datalab::domain::DataTable table;
    table.columns = {"X", "Y"};
    table.rows = {{"1", "2"}, {"2", "4"}, {"3", "6"}, {"4", "8"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0, 1};
    configuration.pca.variable_columns = {0, 1};
    const auto page = datalab::application::AnalysisService::pca(table, configuration);
    QCOMPARE(page.method_name, std::string("Principal Component Analysis"));
    QVERIFY(page.tables.size() >= std::size_t{5});
    QCOMPARE(page.tables[0].headers[2], std::string{"Proportion"});
    QCOMPARE(page.tables[1].title, std::string{"主成分系数"});
    QCOMPARE(page.tables[2].title, std::string{"相关载荷"});
    QCOMPARE(page.tables[3].title, std::string{"主成分得分"});
    bool has_t2_limits = false;
    bool has_t2_rows = false;
    for (const auto& output_table : page.tables) {
        if (output_table.title == "T² 与 Q 阈值") {
            has_t2_limits = true;
            QCOMPARE(output_table.headers.front(), std::string{"分位数"});
        }
        if (output_table.title == "T² 与 Q 残差") {
            has_t2_rows = true;
        }
    }
    QVERIFY(has_t2_limits);
    QVERIFY(has_t2_rows);
    QVERIFY(page.facts.pca.has_value());
    QVERIFY(page.facts.pca->t2_limit.has_value());
    QVERIFY(page.facts.pca->converged);

    // # source: formula_reference  Minitab Levene = |y - median| ANOVA
    const auto levene = datalab::domain::statistics::levene_two_variances(
        {1.0, 2.0, 3.0, 4.0, 5.0}, {10.0, 12.0, 11.0, 13.0, 14.0});
    const auto levene_mean = datalab::domain::statistics::levene_mean_two_variances(
        {1.0, 2.0, 3.0, 4.0, 5.0}, {10.0, 12.0, 11.0, 13.0, 14.0});
    QVERIFY(levene.p_value.has_value());
    QCOMPARE(static_cast<int>(levene.method),
             static_cast<int>(datalab::domain::statistics::VarianceRobustMethod::brown_forsythe_median));
    QCOMPARE(static_cast<int>(levene_mean.method),
             static_cast<int>(datalab::domain::statistics::VarianceRobustMethod::levene_mean));

    datalab::domain::DataTable variance_table;
    variance_table.columns = {"Y", "Group"};
    variance_table.rows = {
        {"1", "A"}, {"2", "A"}, {"3", "A"}, {"4", "A"},
        {"10", "B"}, {"11", "B"}, {"12", "B"}, {"13", "B"},
        {"20", "C"}, {"21", "C"}, {"22", "C"}, {"23", "C"}};
    datalab::domain::AnalysisConfiguration variance_configuration;
    variance_configuration.inference.variance_first_column = 0;
    variance_configuration.inference.variance_group_column = 1;
    variance_configuration.inference.variance_test_method = "levene";
    const auto variance_page = datalab::application::AnalysisService::variance_test(
        variance_table, variance_configuration);
    QCOMPARE(variance_page.tables.front().rows.front().front(), std::string{"Levene"});
    QVERIFY(variance_page.facts.variance.has_value());
    QCOMPARE(variance_page.facts.variance->group_count, std::size_t{3});

    // # source: formula_reference — Bonett two-sample; scaled group should lower p
    const auto bonett_equal = datalab::domain::statistics::bonett_two_variances(
        {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0},
        {1.1, 2.1, 2.9, 4.2, 4.8, 6.1, 7.0, 8.2});
    QVERIFY(bonett_equal.p_value.has_value());
    QVERIFY(*bonett_equal.p_value > 0.05);
    QVERIFY(bonett_equal.confidence_lower.has_value());
    QVERIFY(bonett_equal.confidence_upper.has_value());
    QVERIFY(*bonett_equal.confidence_lower < 1.0);
    QVERIFY(*bonett_equal.confidence_upper > 1.0);
    const auto bonett_unequal = datalab::domain::statistics::bonett_two_variances(
        {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0},
        {10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0});
    QVERIFY(bonett_unequal.p_value.has_value());
    QVERIFY(*bonett_unequal.p_value < *bonett_equal.p_value);

    datalab::domain::AnalysisConfiguration bonett_configuration;
    bonett_configuration.inference.variance_first_column = 0;
    bonett_configuration.inference.variance_group_column = 1;
    bonett_configuration.inference.variance_test_method = "bonett";
    const auto bonett_k_page = datalab::application::AnalysisService::variance_test(
        variance_table, bonett_configuration);
    QVERIFY(std::any_of(
        bonett_k_page.diagnostics.cbegin(), bonett_k_page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "bonett_requires_two_groups";
        }));

    // # source: formula_reference — Bartlett k groups; scaled group lowers p
    const auto bartlett_equal = datalab::domain::statistics::bartlett_k_groups({
        {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0},
        {1.1, 2.1, 2.9, 4.2, 4.8, 6.1, 7.0, 8.2},
        {0.9, 1.9, 3.1, 3.8, 5.2, 5.9, 7.1, 7.8}});
    QVERIFY(bartlett_equal.p_value.has_value());
    QVERIFY(*bartlett_equal.p_value > 0.05);
    QCOMPARE(bartlett_equal.group_count, std::size_t{3});
    QCOMPARE(bartlett_equal.degrees_of_freedom, 2.0);
    const auto bartlett_unequal = datalab::domain::statistics::bartlett_k_groups({
        {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0},
        {1.1, 2.1, 2.9, 4.2, 4.8, 6.1, 7.0, 8.2},
        {10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0}});
    QVERIFY(bartlett_unequal.p_value.has_value());
    QVERIFY(*bartlett_unequal.p_value < *bartlett_equal.p_value);
    const auto bartlett_zero = datalab::domain::statistics::bartlett_k_groups({
        {1.0, 1.0, 1.0}, {2.0, 3.0, 4.0}});
    QVERIFY(std::any_of(
        bartlett_zero.diagnostics.cbegin(), bartlett_zero.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "zero_group_variance";
        }));

    datalab::domain::AnalysisConfiguration bartlett_configuration;
    bartlett_configuration.inference.variance_first_column = 0;
    bartlett_configuration.inference.variance_group_column = 1;
    bartlett_configuration.inference.variance_test_method = "bartlett";
    const auto bartlett_page = datalab::application::AnalysisService::variance_test(
        variance_table, bartlett_configuration);
    QCOMPARE(bartlett_page.tables.front().rows.front().front(), std::string{"Bartlett"});
    QVERIFY(bartlett_page.facts.variance.has_value());
    QCOMPARE(bartlett_page.facts.variance->method, std::string{"Bartlett"});
    QCOMPARE(bartlett_page.facts.variance->group_count, std::size_t{3});

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

    // 响应分析分支：8 运行（4 角点 ×2 次重复，精确线性响应）→ 5 张表 + 11 张图。
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
    QCOMPARE(page.plots.size(), std::size_t{11});
    QCOMPARE(page.plots[0].kind, datalab::domain::PlotKind::pareto);
    QVERIFY(page.plots[1].title.find("立方") != std::string::npos);
    QCOMPARE(page.plots[7].title, std::string("残差与拟合值"));
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
    QCOMPARE(page.plots.size(), std::size_t{2});
    QCOMPARE(page.plots.front().kind, datalab::domain::PlotKind::scatter);
    QCOMPARE(page.plots.front().source_rows.size(), std::size_t{4});
    QCOMPARE(page.plots.front().source_rows.front(), std::size_t{0});
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::interval
                                && plot.values.size() == 1;
                        }));
    QVERIFY(page.facts.t_test.has_value());
    QCOMPARE(page.facts.t_test->kind, std::string{"paired"});

    table.rows = {{"1", "2"}, {"*", "3"}, {"3", "4"}, {"4", "5"}};
    const auto missing_page = datalab::application::AnalysisService::paired_t(
        table, configuration);
    QCOMPARE(missing_page.plots.front().source_rows.size(), std::size_t{3});
    QVERIFY(std::find(missing_page.plots.front().source_rows.cbegin(),
                      missing_page.plots.front().source_rows.cend(),
                      std::size_t{1})
            == missing_page.plots.front().source_rows.cend());
}

void QualityStatisticsTest::buildsOneAndTwoSampleTIntervalPlots()
{
    // # source: formula_reference — interval center is mean; whiskers use mu0 + difference CI.
    datalab::domain::DataTable table;
    table.columns = {"X", "Y"};
    table.rows = {{"1", "10"}, {"2", "12"}, {"3", "11"}, {"4", "13"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.inference.hypothesis_mean = 0.0;
    configuration.inference.confidence_level = 0.95;
    configuration.inference.alternative = "two-sided";
    const auto one = datalab::application::AnalysisService::one_sample_t(table, configuration);
    QVERIFY(one.facts.t_test.has_value());
    QCOMPARE(one.facts.t_test->kind, std::string{"one_sample"});
    QVERIFY(std::any_of(one.plots.cbegin(), one.plots.cend(),
                        [&](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::interval
                                && plot.values.size() == 1
                                && !plot.source_rows.empty()
                                && plot.interval_lower.size() == 1
                                && one.facts.t_test->ci_lower.has_value()
                                && qAbs(plot.interval_lower.front()
                                        - (*one.facts.t_test->ci_lower
                                           + *configuration.inference.hypothesis_mean))
                                    < 1.0e-9;
                        }));

    configuration.inference.alternative = "greater";
    const auto one_sided = datalab::application::AnalysisService::one_sample_t(
        table, configuration);
    QVERIFY(std::none_of(one_sided.plots.cbegin(), one_sided.plots.cend(),
                         [](const datalab::domain::PlotSpec& plot) {
                             return plot.kind == datalab::domain::PlotKind::interval;
                         }));

    configuration.inference.alternative = "two-sided";
    configuration.variable_columns = {0, 1};
    configuration.inference.variance_method = "welch";
    const auto welch = datalab::application::AnalysisService::two_sample_t(
        table, configuration);
    QVERIFY(welch.facts.t_test.has_value());
    QCOMPARE(welch.facts.t_test->variance_method, std::string{"welch"});
    const datalab::domain::PlotSpec* welch_interval = nullptr;
    for (const auto& plot : welch.plots) {
        if (plot.kind == datalab::domain::PlotKind::interval) {
            welch_interval = &plot;
        }
    }
    QVERIFY(welch_interval != nullptr);
    QCOMPARE(welch_interval->values.size(), std::size_t{2});
    QCOMPARE(welch_interval->source_rows.size(), std::size_t{2});

    configuration.inference.variance_method = "pooled";
    const auto pooled = datalab::application::AnalysisService::two_sample_t(
        table, configuration);
    const datalab::domain::PlotSpec* pooled_interval = nullptr;
    for (const auto& plot : pooled.plots) {
        if (plot.kind == datalab::domain::PlotKind::interval) {
            pooled_interval = &plot;
        }
    }
    QVERIFY(pooled_interval != nullptr);
    QVERIFY(qAbs(pooled_interval->interval_lower[0] - welch_interval->interval_lower[0])
            > 1.0e-12
        || qAbs(pooled_interval->interval_upper[0] - welch_interval->interval_upper[0])
            > 1.0e-12);
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
    QCOMPARE(page.tables.size(), std::size_t{6});
    QCOMPARE(page.tables[0].title, std::string{"模型摘要"});
    QCOMPARE(page.tables[1].title, std::string{"系数"});
    QCOMPARE(page.tables[2].title, std::string{"回归方差分析"});
    QCOMPARE(page.tables[3].title, std::string{"假设检查"});
    QCOMPARE(page.tables[4].title, std::string{"拟合与诊断"});
    QCOMPARE(page.tables[1].rows.size(), std::size_t{2});
    QCOMPARE(page.plots.size(), std::size_t{5});
    QCOMPARE(page.plots.front().title, std::string{"拟合线图"});
    QCOMPARE(page.plots.back().title, std::string{"残差正态概率图"});
    QCOMPARE(page.tables[3].rows.size(), std::size_t{3});
    QCOMPARE(page.tables[4].headers[2], std::string{"响应"});
    QCOMPARE(page.tables[4].headers[7], std::string{"学生化残差"});
    QVERIFY(page.facts.regression.has_value());
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
    QCOMPARE(page.tables.size(), std::size_t{4});
    QCOMPARE(page.tables[0].title, std::string{"模型摘要"});
    QCOMPARE(page.tables[1].title, std::string{"拟合优度"});
    QCOMPARE(page.tables[2].title, std::string{"系数与 Odds Ratio"});
    QCOMPARE(page.tables[3].title, std::string{"拟合与残差"});
    QCOMPARE(page.tables[2].headers.back(), std::string{"VIF"});
    QCOMPARE(page.tables[3].rows.size(), std::size_t{8});
    QCOMPARE(page.tables[3].headers.back(), std::string{"影响点"});
    QCOMPARE(page.tables[1].rows.front()[0], std::string{"Hosmer-Lemeshow"});
    QCOMPARE(page.tables[1].rows.front().back(), std::string{"not_computed"});
    QVERIFY(page.facts.logistic.has_value());
    QVERIFY(page.facts.logistic->leverage_threshold.has_value());
    QCOMPARE(page.plots.size(), std::size_t{1});
    QVERIFY(page.tables[0].rows.front()[2] == std::string{"是"}
            || page.tables[0].rows.front()[2] == std::string{"否"});
}

void QualityStatisticsTest::computesLogisticHosmerLemeshowWhenSampleLarge()
{
    std::vector<int> response;
    std::vector<std::vector<double>> predictors;
    for (int index = 0; index < 30; ++index) {
        response.push_back(index % 3 == 0 ? 0 : 1);
        predictors.push_back({static_cast<double>(index) * 0.1 + 1.0});
    }
    const auto small = datalab::domain::statistics::fit_logistic_regression(
        std::vector<int>(response.begin(), response.begin() + 8),
        std::vector<std::vector<double>>(predictors.begin(), predictors.begin() + 8));
    QCOMPARE(small.hosmer_lemeshow_status, std::string{"not_computed"});
    QVERIFY(!small.hosmer_lemeshow_statistic.has_value());

    const auto large = datalab::domain::statistics::fit_logistic_regression(
        response, predictors);
    QVERIFY(large.converged);
    QCOMPARE(large.hosmer_lemeshow_status, std::string{"computed"});
    QVERIFY(large.hosmer_lemeshow_statistic.has_value());
    QVERIFY(large.hosmer_lemeshow_p.has_value());
    QVERIFY(large.hosmer_lemeshow_df.has_value());
    QVERIFY(large.maximum_vif.has_value());
    QVERIFY(large.leverage_threshold > 0.0);
    QCOMPARE(*large.hosmer_lemeshow_df, large.hosmer_lemeshow_groups - 2);
    QVERIFY(large.hosmer_lemeshow_groups >= 6);
}

void QualityStatisticsTest::identifiesIndividualDistributions()
{
    std::vector<double> normal_sample;
    for (int index = 0; index < 40; ++index) {
        normal_sample.push_back(10.0 + static_cast<double>(index % 7) * 0.05);
    }
    const auto normal_result =
        datalab::domain::statistics::identify_individual_distributions(normal_sample);
    QCOMPARE(normal_result.candidates.size(), std::size_t{4});
    QCOMPARE(normal_result.candidates.front().distribution, std::string{"Normal"});

    std::vector<double> mixed = {-1.0, 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    const auto mixed_result =
        datalab::domain::statistics::identify_individual_distributions(mixed);
    for (const auto& candidate : mixed_result.candidates) {
        if (candidate.distribution == "Weibull"
            || candidate.distribution == "Lognormal"
            || candidate.distribution == "Exponential") {
            QCOMPARE(candidate.status, std::string{"not_computed"});
        }
    }
}

void QualityStatisticsTest::buildsDistributionIdentificationServiceOutput()
{
    datalab::domain::DataTable table;
    table.columns = {"Value"};
    table.rows = {{"9.8"}, {"10.0"}, {"10.1"}, {"9.9"}, {"10.2"}, {"10.0"}, {"9.7"}, {"10.3"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.capability_method = "normal";
    const auto page = datalab::application::AnalysisService::distribution_identification(
        table, configuration);
    QCOMPARE(page.method_name, std::string{"Individual Distribution Identification"});
    QCOMPARE(page.tables.size(), std::size_t{2});
    QCOMPARE(page.tables.front().title, std::string{"拟合优度"});
    QCOMPARE(page.tables.front().rows.size(), std::size_t{4});
    QVERIFY(page.plots.size() >= 1);
    QCOMPARE(page.configuration.capability_method, std::string{"normal"});
    QVERIFY(page.facts.distribution_identification.has_value());
    QCOMPARE(page.facts.distribution_identification->did_not_change_capability_defaults, true);
}

void QualityStatisticsTest::calculatesBetweenWithinCapability()
{
    const std::vector<std::vector<double>> subgroups = {
        {9.8, 10.0, 10.1},
        {9.9, 10.2, 10.0},
        {10.1, 9.7, 10.3},
        {10.0, 10.2, 9.9}};
    std::vector<double> pooled;
    for (const auto& subgroup : subgroups) {
        pooled.insert(pooled.end(), subgroup.begin(), subgroup.end());
    }
    SpecificationLimits specs;
    specs.lower = 9.5;
    specs.upper = 10.5;
    const auto result = ProcessCapability::calculate_between_within(pooled, subgroups, specs);
    QCOMPARE(result.capability_method, std::string{"between_within"});
    QVERIFY(result.subgroup_within_standard_deviation.has_value());
    QVERIFY(result.between_standard_deviation.has_value());
    QVERIFY(result.between_within_standard_deviation.has_value());
    QVERIFY(result.cp.has_value());
    QVERIFY(result.ppk.has_value());
}

void QualityStatisticsTest::buildsBetweenWithinCapabilityServiceOutput()
{
    datalab::domain::DataTable table;
    table.columns = {"Value", "Subgroup"};
    table.rows = {
        {"9.8", "A"}, {"10.0", "A"}, {"10.1", "A"},
        {"9.9", "B"}, {"10.2", "B"}, {"10.0", "B"},
        {"10.1", "C"}, {"9.7", "C"}, {"10.3", "C"},
        {"10.0", "D"}, {"10.2", "D"}, {"9.9", "D"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.selection.measurement_column = 0;
    configuration.selection.subgroup_column = 1;
    configuration.specifications.lower = 9.5;
    configuration.specifications.upper = 10.5;
    configuration.capability_method = "between_within";
    const auto page =
        datalab::application::AnalysisService::between_within_capability(table, configuration);
    QCOMPARE(page.method_name, std::string{"Between/Within Capability Analysis"});
    QVERIFY(!page.tables.empty());
    QCOMPARE(page.tables.front().title, std::string{"Process Data"});
    bool has_between = false;
    for (const auto& row : page.tables.front().rows) {
        if (row.front() == "StDev (Between)") {
            has_between = true;
            QVERIFY(row[1] != "*");
        }
    }
    QVERIFY(has_between);
    QCOMPARE(page.tables[2].title, std::string{"Between/Within Capability"});

    configuration.selection.subgroup_column.reset();
    const auto missing_subgroup =
        datalab::application::AnalysisService::between_within_capability(table, configuration);
    QVERIFY(missing_subgroup.diagnostics.size() > 0
            || !missing_subgroup.tables.empty());
}

void QualityStatisticsTest::buildsCapabilityHistogramContract()
{
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    table.rows = {
        {"4.8"}, {"*"}, {"5.0"}, {"5.2"}, {"4.9"}, {"5.1"}, {"5.0"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.specifications.lower = 4.0;
    configuration.specifications.upper = 6.0;
    configuration.specifications.target = 5.0;
    const auto page =
        datalab::application::AnalysisService::capability(table, configuration);
    QVERIFY(!page.plots.empty());
    const auto hist = std::find_if(
        page.plots.cbegin(), page.plots.cend(),
        [](const datalab::domain::PlotSpec& plot) {
            return plot.kind == datalab::domain::PlotKind::histogram;
        });
    QVERIFY(hist != page.plots.cend());
    QCOMPARE(hist->title, std::string{"过程能力直方图"});
    QVERIFY(hist->lsl.has_value());
    QVERIFY(hist->usl.has_value());
    QVERIFY(hist->target.has_value());
    QCOMPARE(*hist->target, 5.0);
    QVERIFY(hist->process_mean.has_value());
    QVERIFY(hist->within_sigma.has_value());
    QVERIFY(hist->overall_sigma.has_value());
    QCOMPARE(hist->source_rows, (std::vector<std::size_t>{0, 2, 3, 4, 5, 6}));
    QCOMPARE(hist->series.size(), std::size_t{2});
    QCOMPARE(hist->series[0].label, std::string{"Within"});
    QCOMPARE(hist->series[1].label, std::string{"Overall"});

    datalab::domain::OutputPage interpreted = page;
    datalab::application::InterpretationService::enrich(interpreted);
    for (const auto& section : interpreted.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
            QVERIFY(bullet.find("不合格") == std::string::npos);
        }
    }

    const auto sixpack =
        datalab::application::AnalysisService::capability_sixpack(table, configuration);
    QCOMPARE(sixpack.method_name, std::string{"Capability Sixpack"});
    const auto six_hist = std::find_if(
        sixpack.plots.cbegin(), sixpack.plots.cend(),
        [](const datalab::domain::PlotSpec& plot) {
            return plot.kind == datalab::domain::PlotKind::histogram;
        });
    QVERIFY(six_hist != sixpack.plots.cend());
    QCOMPARE(six_hist->series.size(), hist->series.size());
    QCOMPARE(six_hist->series[0].label, hist->series[0].label);
    QCOMPARE(six_hist->lsl, hist->lsl);
    QCOMPARE(sixpack.plots.size(), std::size_t{6});
    QCOMPARE(sixpack.plots[0].title, std::string{"I 图"});
    QCOMPARE(sixpack.plots[1].title, std::string{"过程能力直方图"});
    QCOMPARE(sixpack.plots[2].title, std::string{"MR 图"});
    QCOMPARE(sixpack.plots[3].title, std::string{"正态概率图"});
    QCOMPARE(sixpack.plots[4].title, std::string{"最近 25 个观测"});
    QCOMPARE(sixpack.plots[5].title, std::string{"能力图"});

    datalab::domain::DataTable bw_table;
    bw_table.columns = {"Value", "Subgroup"};
    bw_table.rows = {
        {"9.8", "A"}, {"10.0", "A"}, {"10.1", "A"},
        {"9.9", "B"}, {"10.2", "B"}, {"10.0", "B"},
        {"10.1", "C"}, {"9.7", "C"}, {"10.3", "C"},
        {"10.0", "D"}, {"10.2", "D"}, {"9.9", "D"}};
    datalab::domain::AnalysisConfiguration bw_config;
    bw_config.variable_columns = {0};
    bw_config.selection.measurement_column = 0;
    bw_config.selection.subgroup_column = 1;
    bw_config.specifications.lower = 9.5;
    bw_config.specifications.upper = 10.5;
    const auto bw_page =
        datalab::application::AnalysisService::between_within_capability(bw_table, bw_config);
    const auto bw_hist = std::find_if(
        bw_page.plots.cbegin(), bw_page.plots.cend(),
        [](const datalab::domain::PlotSpec& plot) {
            return plot.kind == datalab::domain::PlotKind::histogram;
        });
    QVERIFY(bw_hist != bw_page.plots.cend());
    QCOMPARE(bw_hist->series[0].label, std::string{"Between/Within"});
    QVERIFY(bw_page.facts.capability.has_value());
    const auto bw_result = ProcessCapability::calculate_between_within(
        {9.8, 10.0, 10.1, 9.9, 10.2, 10.0, 10.1, 9.7, 10.3, 10.0, 10.2, 9.9},
        {{9.8, 10.0, 10.1}, {9.9, 10.2, 10.0}, {10.1, 9.7, 10.3}, {10.0, 10.2, 9.9}},
        {9.5, 10.5, std::nullopt});
    QVERIFY(bw_result.between_within_standard_deviation.has_value());
    QVERIFY(bw_hist->within_sigma.has_value());
    QVERIFY(std::abs(*bw_hist->within_sigma - *bw_result.between_within_standard_deviation)
            < 1.0e-12);
}

void QualityStatisticsTest::buildsResponseOptimizationOutput()
{
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
    configuration.doe.optimization_goal = "maximize";
    const auto page = datalab::application::AnalysisService::response_optimization(
        table, configuration);
    QCOMPARE(page.method_name, std::string{"Response Optimization"});
    QVERIFY(page.tables.size() >= std::size_t{3});
    QCOMPARE(page.tables[0].title, std::string{"最佳组合"});
    QCOMPARE(page.tables[1].title, std::string{"候选组合"});
    QCOMPARE(page.tables[2].title, std::string{"响应预测"});
    QCOMPARE(page.tables[0].headers.size(), std::size_t{6});
    QCOMPARE(page.tables[1].headers.size(), std::size_t{6});
    QCOMPARE(page.tables[2].headers.size(), std::size_t{7});
    QCOMPARE(page.tables[1].rows.size(), std::size_t{4});
    QCOMPARE(page.tables[1].rows.front()[0], std::string{"1"});
    QCOMPARE(page.tables[1].rows.front()[1], std::string{"A=1, B=1"});
    QCOMPARE(page.tables[1].rows.front()[2], std::string{"A=1, B=1"});
    QCOMPARE(page.tables[1].rows.front()[3], std::string{"6.5000"});
    QCOMPARE(page.tables[1].rows.front()[4], std::string{"1.0000"});
    QCOMPARE(page.tables[1].rows.front()[5], std::string{"1.0000"});
    QCOMPARE(page.tables[0].rows.front()[0], std::string{"A"});
    QCOMPARE(page.tables[0].rows.front()[1], std::string{"1"});
    QCOMPARE(page.tables[0].rows.front()[2], std::string{"1"});
    QCOMPARE(page.tables[0].rows.front()[3], std::string{"6.5000"});
    QCOMPARE(page.tables[0].rows.front()[4], std::string{"1.0000"});
    QCOMPARE(page.tables[0].rows.front()[5], std::string{"1.0000"});
    QCOMPARE(page.tables[2].rows.front()[0], std::string{"1 1"});
    QCOMPARE(page.tables[2].rows.front()[1], std::string{"A=1, B=1"});
    QCOMPARE(page.tables[2].rows.front()[2], std::string{"6.5000"});
    QCOMPARE(page.tables[2].rows.front()[3], std::string{"*"});
    QCOMPARE(page.tables[2].rows.front()[4], std::string{"*"});
    QCOMPARE(page.tables[2].rows.front()[5], std::string{"*"});
    QCOMPARE(page.tables[2].rows.front()[6], std::string{"*"});
    QCOMPARE(page.plots.size(), std::size_t{1});
    QCOMPARE(page.plots.front().kind, datalab::domain::PlotKind::scatter);
    QCOMPARE(page.plots.front().title, std::string{"候选组合总体 Desirability"});
    QCOMPARE(page.plots.front().x_values.size(), std::size_t{4});
    QCOMPARE(page.plots.front().values.size(), std::size_t{4});
    QCOMPARE(page.plots.front().point_labels.front(), std::string{"A=1, B=1"});
    QCOMPARE(page.plots.front().values.front(), 1.0);
    QVERIFY(page.facts.doe.has_value());
    QVERIFY(page.facts.doe->has_p_value);
    QCOMPARE(page.method_metadata.valid_count, std::size_t{8});
    QCOMPARE(page.method_metadata.missing_count, std::size_t{0});
    QCOMPARE(page.method_metadata.parameter_source, std::string{"imported_doe_design"});
    QCOMPARE(page.method_metadata.estimation_method,
             std::string{"coded_2_level_desirability"});
    bool has_interval_warning = false;
    for (const auto& diagnostic : page.diagnostics) {
        if (diagnostic.code == "approximate_prediction_standard_error") {
            has_interval_warning = true;
            break;
        }
    }
    QVERIFY(has_interval_warning);

    datalab::domain::DataTable noisy_table;
    noisy_table.columns = {"A", "B", "Y"};
    noisy_table.rows = {
        {"-1", "-1", "-3.5"},
        {"1", "-1", "-0.5"},
        {"bad", "1", "1.5"},
        {"1", "1", ""},
        {"-1", "1", "1.5"},
        {"1", "1", "6.5"},
        {"-1", "-1", "-3.5"},
        {"1", "-1", "-0.5"},
        {"-1", "1", "1.5"},
        {"1", "1", "6.5"}};
    const auto noisy_page = datalab::application::AnalysisService::response_optimization(
        noisy_table, configuration);
    QCOMPARE(noisy_page.method_metadata.valid_count, std::size_t{8});
    QCOMPARE(noisy_page.method_metadata.missing_count, std::size_t{1});
    bool has_invalid_level_warning = false;
    bool has_missing_response_warning = false;
    for (const auto& diagnostic : noisy_page.diagnostics) {
        if (diagnostic.code == "invalid_doe_factor_levels") {
            has_invalid_level_warning = true;
        }
        if (diagnostic.code == "missing_doe_response") {
            has_missing_response_warning = true;
        }
    }
    QVERIFY(has_invalid_level_warning);
    QVERIFY(has_missing_response_warning);
}

void QualityStatisticsTest::buildsMultiResponseOptimizationOutput()
{
    datalab::domain::DataTable table;
    table.columns = {"A", "B", "Y", "Y2"};
    table.rows = {
        {"-1", "-1", "-3.5", "10.0"}, {"1", "-1", "-0.5", "7.0"},
        {"-1", "1", "1.5", "5.0"}, {"1", "1", "6.5", "2.0"},
        {"-1", "-1", "-3.5", "10.0"}, {"1", "-1", "-0.5", "7.0"},
        {"-1", "1", "1.5", "5.0"}, {"1", "1", "6.5", "2.0"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.doe.factor_columns = {0, 1};
    configuration.doe.response_columns = {2, 3};
    configuration.doe.optimization_objectives = {
        {.goal = "maximize"},
        {.goal = "minimize"}};
    const auto page = datalab::application::AnalysisService::response_optimization(
        table, configuration);
    QCOMPARE(page.method_name, std::string{"Response Optimization"});
    QVERIFY(page.tables.size() >= std::size_t{4});
    QCOMPARE(page.tables[0].title, std::string{"响应目标"});
    QCOMPARE(page.tables[1].title, std::string{"最佳组合"});
    QCOMPARE(page.tables[2].title, std::string{"候选组合"});
    QCOMPARE(page.tables[3].title, std::string{"响应预测"});
    QCOMPARE(page.tables[0].rows.size(), std::size_t{2});
    QCOMPARE(page.tables[0].rows[0][0], std::string{"Y"});
    QCOMPARE(page.tables[0].rows[0][1], std::string{"maximize"});
    QCOMPARE(page.tables[0].rows[1][0], std::string{"Y2"});
    QCOMPARE(page.tables[0].rows[1][1], std::string{"minimize"});
    QCOMPARE(page.tables[2].headers.size(), std::size_t{8});
    QCOMPARE(page.tables[3].headers.size(), std::size_t{8});
    QVERIFY(page.facts.doe.has_value());
    QVERIFY(page.facts.doe->multi_response);
    QCOMPARE(page.facts.doe->response_count, std::size_t{2});
    QCOMPARE(page.facts.doe->response_names.size(), std::size_t{2});
    QVERIFY(page.facts.doe->best_overall_desirability.has_value());
    QCOMPARE(page.method_metadata.estimation_method,
             std::string{"coded_2_level_multi_response_desirability"});
    QCOMPARE(page.plots.size(), std::size_t{1});
}

void QualityStatisticsTest::johnsonAndNonnormalCapability()
{
    std::vector<double> lognormal_sample;
    lognormal_sample.reserve(80);
    for (int index = 0; index < 80; ++index) {
        const double probability = (static_cast<double>(index) + 0.5) / 80.0;
        lognormal_sample.push_back(std::exp(
            0.4 * datalab::domain::statistics::standard_normal_quantile(probability)));
    }
    const auto transform =
        datalab::domain::statistics::fit_johnson_transform(lognormal_sample);
    QVERIFY(transform.found || transform.p_value >= 0.0);
    if (transform.found) {
        QVERIFY(transform.parameters.family == datalab::domain::statistics::JohnsonFamily::sl
                || transform.parameters.family == datalab::domain::statistics::JohnsonFamily::su
                || transform.parameters.family == datalab::domain::statistics::JohnsonFamily::sb);
        QVERIFY(transform.p_value > 0.10);
        QVERIFY(!transform.transformed.empty());
    }

    const SpecificationLimits specs{0.5, 3.0, std::nullopt};
    const auto johnson = ProcessCapability::calculate_johnson(lognormal_sample, specs);
    QCOMPARE(johnson.capability_method, std::string("johnson"));
    QVERIFY(!johnson.cp.has_value());
    QVERIFY(!johnson.cpk.has_value());

    const auto normal = ProcessCapability::calculate({4.0, 5.0, 6.0}, 1.0, specs);
    QVERIFY(normal.cp.has_value());
    QCOMPARE(normal.capability_method, std::string("normal"));

    std::vector<double> weibull_sample;
    weibull_sample.reserve(80);
    for (int index = 0; index < 80; ++index) {
        const double probability = (static_cast<double>(index) + 0.5) / 80.0;
        weibull_sample.push_back(10.0 * std::sqrt(-std::log(1.0 - probability)));
    }
    const SpecificationLimits weibull_specs{5.0, 20.0, std::nullopt};
    const auto nonnormal = ProcessCapability::calculate_nonnormal(
        weibull_sample, weibull_specs, "weibull");
    QVERIFY(nonnormal.pp.has_value());
    QVERIFY(nonnormal.ppk.has_value());
    QVERIFY(!nonnormal.cp.has_value());
    const double f_lsl = 1.0 - std::exp(-std::pow(5.0 / 10.0, 2.0));
    const double f_usl = 1.0 - std::exp(-std::pow(20.0 / 10.0, 2.0));
    const double expected_pp =
        (datalab::domain::statistics::standard_normal_quantile(f_usl)
         - datalab::domain::statistics::standard_normal_quantile(f_lsl)) / 6.0;
    QVERIFY(qAbs(*nonnormal.pp - (*nonnormal.z_usl - *nonnormal.z_lsl) / 6.0) < 1.0e-12);
    QVERIFY(qAbs(*nonnormal.pp - expected_pp) / std::abs(expected_pp) < 0.05);

    datalab::domain::DataTable table;
    table.columns = {"Value"};
    for (const double value : weibull_sample) {
        table.rows.push_back({std::to_string(value)});
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.specifications = weibull_specs;
    configuration.capability_method = "non_normal";
    configuration.nonnormal_distribution = "weibull";
    const auto page = datalab::application::AnalysisService::capability(table, configuration);
    QCOMPARE(page.method_name, std::string("Nonnormal Capability Analysis"));
    QVERIFY(!page.tables.empty());
    const auto distribution = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "分布参数";
        });
    QVERIFY(distribution != page.tables.cend());
    const auto ppm = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Performance (PPM)";
        });
    QVERIFY(ppm != page.tables.cend());
    QCOMPARE(ppm->headers.size(), std::size_t{3});
    bool has_within_capability = false;
    for (const auto& table_out : page.tables) {
        if (table_out.title == "Potential (Within) Capability") {
            has_within_capability = true;
        }
    }
    QVERIFY(!has_within_capability);
    QVERIFY(page.facts.capability.has_value());
    QCOMPARE(page.facts.capability->method, std::string("non_normal"));
    QVERIFY(page.facts.capability->fitted_shape.has_value());

    datalab::domain::DataTable johnson_table;
    johnson_table.columns = {"Value"};
    for (const double value : lognormal_sample) {
        johnson_table.rows.push_back({std::to_string(value)});
    }
    datalab::domain::AnalysisConfiguration johnson_config;
    johnson_config.variable_columns = {0};
    johnson_config.specifications = specs;
    johnson_config.capability_method = "johnson";
    const auto johnson_page =
        datalab::application::AnalysisService::capability(johnson_table, johnson_config);
    QCOMPARE(johnson_page.method_name, std::string("Johnson Capability Analysis"));
    if (johnson_page.facts.capability.has_value()
        && johnson_page.facts.capability->transform_p_value.has_value()) {
        const auto transform = std::find_if(
            johnson_page.tables.cbegin(), johnson_page.tables.cend(),
            [](const datalab::domain::StatisticTable& table_out) {
                return table_out.title == "Johnson 变换";
            });
        QVERIFY(transform != johnson_page.tables.cend());
        QVERIFY(!johnson_page.facts.capability->cpk.has_value());
        QVERIFY(johnson_page.facts.capability->ppk.has_value()
                || johnson_page.diagnostics.size() > 0);
        QVERIFY(std::any_of(
            johnson_page.plots.cbegin(), johnson_page.plots.cend(),
            [](const datalab::domain::PlotSpec& plot) {
                return plot.title == "变换后正态概率图";
            }));
    }
}

void QualityStatisticsTest::buildsNormalCapabilityTableContract()
{
    // # source: formula_reference — Process Data AD rows and PPM table shape.
    datalab::domain::DataTable table;
    table.columns = {"Y"};
    table.rows = {
        {"4.8"}, {"5.0"}, {"5.0"}, {"5.2"}, {"4.9"}, {"5.1"}, {"5.0"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0};
    configuration.specifications.lower = 4.0;
    configuration.specifications.upper = std::nullopt;
    const auto page =
        datalab::application::AnalysisService::capability(table, configuration);
    const auto process = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Process Data";
        });
    QVERIFY(process != page.tables.cend());
    bool has_ad = false;
    bool has_assumption = false;
    for (const auto& row : process->rows) {
        if (row.front() == "Anderson-Darling A²*") {
            has_ad = true;
            QVERIFY(row[1] != "*");
        }
        if (row.front() == "假设状态") {
            has_assumption = true;
        }
    }
    QVERIFY(has_ad);
    QVERIFY(has_assumption);
    const auto ppm = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Performance (PPM)";
        });
    QVERIFY(ppm != page.tables.cend());
    QCOMPARE(ppm->headers,
             (std::vector<std::string>{"", "观测", "期望 Within", "期望 Overall"}));
    QCOMPARE(ppm->rows.front()[2], std::string{"*"});
    QVERIFY(page.facts.capability.has_value());
    QVERIFY(page.facts.capability->normality_p_value.has_value());
}

QTEST_APPLESS_MAIN(QualityStatisticsTest)

#include "quality_statistics_test.moc"
