#include "domain/quality_types.h"
#include "infrastructure/output_serialization.h"

#include <QtTest/QtTest>

class OutputSerializationTest final : public QObject {
    Q_OBJECT

private slots:
    void preservesAnalysisConfiguration();
    void preservesMultiSeriesPlot();
    void preservesLegacySpecialCausePolicy();
    void preservesStructuredAnalysisFacts();
};

void OutputSerializationTest::preservesAnalysisConfiguration()
{
    datalab::domain::OutputPage page;
    page.id = "page-1";
    page.title = "Round trip";
    page.configuration.analysis_name = "ARIMA";
    page.configuration.chart_type = "arima";
    page.configuration.graph.graph_kind = "bubble";
    page.configuration.graph.x_column = 1;
    page.configuration.graph.y_column = 2;
    page.configuration.graph.size_column = 3;
    page.configuration.graph.z_column = 4;
    page.configuration.graph.bin_count = 12;
    page.configuration.graph.confidence_level = 0.9;
    page.configuration.variable_columns = {1, 3};
    page.configuration.selection.measurement_column = 2;
    page.configuration.selection.time_column = 0;
    page.configuration.inspected_constant = 8;
    page.configuration.inference.first_events_column = 5;
    page.configuration.inference.first_trials_column = 6;
    page.configuration.inference.second_events_column = 7;
    page.configuration.inference.second_trials_column = 8;
    page.configuration.inference.row_category_column = 9;
    page.configuration.inference.column_category_column = 10;
    page.configuration.specifications.lower = 1.5;
    page.configuration.specifications.upper = 9.5;
    page.configuration.time_series.arima_time_column = 0;
    page.configuration.time_series.arima_value_column = 2;
    page.configuration.time_series.arima_selection_criterion = "bic";
    page.configuration.time_series.forecast_periods = 6;
    page.configuration.control.ewma_lambda = 0.25;
    page.configuration.control.historical_sigma = 1.25;
    page.configuration.control.enabled_special_cause_tests = {1, 2, 8};
    page.configuration.control.special_cause_rule_policy = "explicit";
    page.configuration.msa.gage_operator_column = 4;
    page.configuration.pca.variable_columns = {1, 2, 3};
    page.configuration.pca.mode = "standardized";
    page.configuration.time_series.seasonal_period = 12;
    page.configuration.doe.factor_names = {"Temperature", "Pressure"};
    page.configuration.doe.factor_columns = {1, 2};
    page.configuration.doe.response_column = 0;
    page.configuration.reliability.group_column = 11;
    page.configuration.power.group_count = 4;
    page.configuration.power.null_proportion = 0.4;
    page.configuration.power.second_proportion = 0.7;
    page.configuration.power.variance_method = "unpooled";
    page.facts.capability = datalab::domain::CapabilityFacts{1.21, 1.08};
    page.facts.spc = datalab::domain::SpcFacts{2, 1.15};
    page.method_metadata.algorithm = "Normal Capability Analysis";
    page.method_metadata.version = "2";
    page.method_metadata.parameters = "LSL=1;USL=9";
    page.method_metadata.source_rows = {0, 4, 9};
    page.method_metadata.diagnostic_codes = {"small_sample", "stability_unverified"};
    page.diagnostics.push_back({
        datalab::domain::DiagnosticMessage::Severity::warning,
        "small_sample",
        "样本量较小",
        {4, 9},
        {2},
        "capability-plot",
        "增加独立观测"});
    page.interpretation.push_back({
        "结论", {"保留配置"}, datalab::domain::DiagnosticMessage::Severity::info});

    const auto restored = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QCOMPARE(QString::fromStdString(restored.configuration.analysis_name),
             QStringLiteral("ARIMA"));
    QCOMPARE(restored.configuration.graph.graph_kind, std::string("bubble"));
    QCOMPARE(*restored.configuration.graph.size_column, std::size_t{3});
    QCOMPARE(*restored.configuration.graph.z_column, std::size_t{4});
    QCOMPARE(restored.configuration.graph.bin_count, 12);
    QCOMPARE(restored.configuration.graph.confidence_level, 0.9);
    QCOMPARE(restored.configuration.variable_columns.size(), std::size_t{2});
    QCOMPARE(restored.configuration.selection.measurement_column, std::size_t{2});
    QCOMPARE(*restored.configuration.selection.time_column, std::size_t{0});
    QCOMPARE(*restored.configuration.inspected_constant, std::size_t{8});
    QCOMPARE(*restored.configuration.inference.first_events_column, std::size_t{5});
    QCOMPARE(*restored.configuration.inference.first_trials_column, std::size_t{6});
    QCOMPARE(*restored.configuration.inference.second_events_column, std::size_t{7});
    QCOMPARE(*restored.configuration.inference.second_trials_column, std::size_t{8});
    QCOMPARE(*restored.configuration.inference.row_category_column, std::size_t{9});
    QCOMPARE(*restored.configuration.inference.column_category_column, std::size_t{10});
    QCOMPARE(*restored.configuration.specifications.lower, 1.5);
    QCOMPARE(*restored.configuration.time_series.arima_value_column, std::size_t{2});
    QCOMPARE(restored.configuration.time_series.arima_selection_criterion, std::string("bic"));
    QCOMPARE(restored.configuration.time_series.forecast_periods, 6);
    QCOMPARE(restored.configuration.control.ewma_lambda, 0.25);
    QCOMPARE(*restored.configuration.control.historical_sigma, 1.25);
    QCOMPARE(restored.configuration.control.enabled_special_cause_tests,
             (std::vector<int>{1, 2, 8}));
    QCOMPARE(restored.configuration.control.special_cause_rule_policy,
             std::string("explicit"));
    QCOMPARE(*restored.configuration.msa.gage_operator_column, std::size_t{4});
    QCOMPARE(restored.configuration.pca.variable_columns.size(), std::size_t{3});
    QCOMPARE(restored.configuration.pca.mode, std::string("standardized"));
    QCOMPARE(restored.configuration.time_series.seasonal_period, std::size_t{12});
    QCOMPARE(restored.configuration.doe.factor_names.size(), std::size_t{2});
    QCOMPARE(restored.configuration.doe.factor_columns.size(), std::size_t{2});
    QCOMPARE(*restored.configuration.doe.response_column, std::size_t{0});
    QCOMPARE(*restored.configuration.reliability.group_column, std::size_t{11});
    QCOMPARE(restored.configuration.power.group_count, std::size_t{4});
    QCOMPARE(restored.configuration.power.null_proportion, 0.4);
    QCOMPARE(restored.configuration.power.second_proportion, 0.7);
    QCOMPARE(restored.configuration.power.variance_method, std::string("unpooled"));
    QCOMPARE(restored.method_metadata.algorithm,
             std::string("Normal Capability Analysis"));
    QCOMPARE(restored.method_metadata.version, std::string("2"));
    QCOMPARE(restored.method_metadata.source_rows,
             std::vector<datalab::domain::RowId>({0, 4, 9}));
    QCOMPARE(restored.diagnostics.front().related_rows,
             std::vector<datalab::domain::RowId>({4, 9}));
    QCOMPARE(restored.diagnostics.front().suggested_action, std::string("增加独立观测"));
    QVERIFY(restored.facts.capability.has_value());
    QCOMPARE(*restored.facts.capability->cpk, 1.21);
    QCOMPARE(*restored.facts.capability->ppk, 1.08);
    QVERIFY(!restored.facts.capability->average_p.has_value());
    QVERIFY(restored.facts.spc.has_value());
    QCOMPARE(*restored.facts.spc->out_of_control_count, std::size_t{2});
    QCOMPARE(restored.interpretation.size(), std::size_t{1});
}

void OutputSerializationTest::preservesMultiSeriesPlot()
{
    datalab::domain::OutputPage page;
    datalab::domain::PlotSpec plot;
    plot.title = "Forecast";
    plot.kind = datalab::domain::PlotKind::bubble;
    plot.values = {10.0, 11.0};
    plot.x_values = {1.0, 2.0};
    plot.bubble_sizes = {4.0, 9.0};
    plot.point_groups = {"A", "B"};
    plot.histogram_edges_y = {0.0, 1.0, 2.0};
    plot.contour_x = {0.0, 1.0};
    plot.contour_y = {0.0, 1.0};
    plot.contour_levels = {0.0, 0.5, 1.0};
    plot.color_min = -1.0;
    plot.color_max = 1.0;
    plot.center = {10.5, 10.5};
    plot.value_style.color = "#008577";
    plot.value_style.point_style = datalab::domain::PlotPointStyle::square;
    plot.legend_font_size = 11;
    plot.triggered_tests = {{1, 5}, {2}};
    plot.series = {
        {datalab::domain::PlotSeriesRole::actual, "Actual", {10.0, 11.0}, {1.0, 2.0},
         {}, {}, 2.0, true},
        {datalab::domain::PlotSeriesRole::forecast, "Forecast", {12.0, 13.0}, {3.0, 4.0},
         {}, {}, 1.6, false},
        {datalab::domain::PlotSeriesRole::confidence_band, "95% CI", {},
         {3.0, 4.0}, {11.0, 11.5}, {13.0, 14.5}, 1.0, false}};
    page.plots.push_back(plot);

    const auto restored = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QCOMPARE(restored.plots.size(), std::size_t{1});
    QCOMPARE(restored.plots.front().values.size(), std::size_t{2});
    QCOMPARE(restored.plots.front().kind, datalab::domain::PlotKind::bubble);
    QCOMPARE(restored.plots.front().bubble_sizes, std::vector<double>({4.0, 9.0}));
    QCOMPARE(restored.plots.front().point_groups,
             std::vector<std::string>({"A", "B"}));
    QCOMPARE(restored.plots.front().histogram_edges_y,
             std::vector<double>({0.0, 1.0, 2.0}));
    QCOMPARE(*restored.plots.front().color_min, -1.0);
    QCOMPARE(restored.plots.front().contour_x, std::vector<double>({0.0, 1.0}));
    QCOMPARE(restored.plots.front().contour_levels.size(), std::size_t{3});
    QCOMPARE(restored.plots.front().series.size(), std::size_t{3});
    QCOMPARE(static_cast<int>(restored.plots.front().series[1].role),
             static_cast<int>(datalab::domain::PlotSeriesRole::forecast));
    QCOMPARE(QString::fromStdString(restored.plots.front().series[2].label),
             QStringLiteral("95% CI"));
    QCOMPARE(restored.plots.front().series[2].lower[0], 11.0);
    QCOMPARE(restored.plots.front().series[0].show_points, true);
    QCOMPARE(restored.plots.front().value_style.color, std::string("#008577"));
    QCOMPARE(restored.plots.front().value_style.point_style,
             datalab::domain::PlotPointStyle::square);
    QCOMPARE(restored.plots.front().legend_font_size, 11);
    QCOMPARE(restored.plots.front().triggered_tests.front(),
             (std::vector<int>{1, 5}));
}

void OutputSerializationTest::preservesLegacySpecialCausePolicy()
{
    datalab::domain::OutputPage page;
    page.configuration.control.enabled_special_cause_tests = {1};
    page.configuration.control.special_cause_rule_policy = "explicit";
    auto object = datalab::infrastructure::output_page_to_json(page);
    object.remove(QStringLiteral("special_cause_rule_policy"));
    const auto legacy_array = datalab::infrastructure::output_page_from_json(object);
    QCOMPARE(legacy_array.configuration.control.enabled_special_cause_tests,
             (std::vector<int>{1}));
    QCOMPARE(legacy_array.configuration.control.special_cause_rule_policy,
             std::string("explicit"));

    object.remove(QStringLiteral("enabled_special_cause_tests"));
    const auto missing = datalab::infrastructure::output_page_from_json(object);
    QCOMPARE(missing.configuration.control.enabled_special_cause_tests,
             (std::vector<int>{1}));
    QCOMPARE(missing.configuration.control.special_cause_rule_policy,
             std::string("explicit"));

    page.configuration.control.enabled_special_cause_tests.clear();
    page.configuration.control.special_cause_rule_policy = "default_all_applicable";
    const auto restored = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored.configuration.control.enabled_special_cause_tests.empty());
    QCOMPARE(restored.configuration.control.special_cause_rule_policy,
             std::string("default_all_applicable"));
}

void OutputSerializationTest::preservesStructuredAnalysisFacts()
{
    datalab::domain::OutputPage page;
    page.id = "facts-1";
    page.title = "Facts";
    page.method_name = "Linear Regression";
    datalab::domain::RegressionFacts regression;
    regression.r_squared = 0.77;
    regression.influential_count = 2;
    regression.outlier_count = 1;
    regression.max_vif = 6.2;
    regression.residual_anderson_darling = 0.41;
    regression.residual_plot_count = 5;
    regression.assumption_status = "not_verified";
    regression.rules.push_back({"influence", "triggered", "存在影响点", {4}, "回查原始行"});
    page.facts.regression = regression;
    datalab::domain::AnovaFacts anova;
    anova.p_value = 0.01;
    anova.error_degrees_of_freedom = 8;
    anova.significant_terms = {"Factor A"};
    anova.family_confidence_level = 0.95;
    page.facts.anova = anova;
    datalab::domain::MsaFacts msa;
    msa.ndc = 4.0;
    msa.ndc_available = true;
    msa.design_balanced = true;
    msa.negative_variance_truncated = true;
    msa.by_part_plot_available = true;
    msa.interaction_plot_available = false;
    msa.plot_point_count = 18;
    msa.linearity = 2.5;
    msa.percent_linearity = 50.0;
    msa.slope_p_value = 0.012;
    msa.intercept_p_value = 0.34;
    msa.residual_s = 0.88;
    msa.average_bias = 0.12;
    msa.average_bias_p = 0.45;
    msa.process_variation_used = 16.5368;
    page.facts.msa = msa;
    datalab::domain::ReliabilityFacts reliability;
    reliability.failure_count = 5;
    reliability.censored_count = 2;
    reliability.identifiable = true;
    reliability.event_encoding = "failure_suspension";
    page.facts.reliability = reliability;

    const auto restored = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored.facts.regression.has_value());
    QCOMPARE(*restored.facts.regression->r_squared, 0.77);
    QCOMPARE(*restored.facts.regression->residual_anderson_darling, 0.41);
    QCOMPARE(restored.facts.regression->residual_plot_count, std::size_t{5});
    QCOMPARE(restored.facts.regression->influential_count, std::size_t{2});
    QCOMPARE(restored.facts.regression->rules.front().id, std::string("influence"));
    QCOMPARE(restored.facts.regression->rules.front().related_rows.front(),
             datalab::domain::RowId{4});
    QVERIFY(restored.facts.anova.has_value());
    QCOMPARE(restored.facts.anova->significant_terms.front(), std::string("Factor A"));
    QVERIFY(restored.facts.msa.has_value());
    QCOMPARE(*restored.facts.msa->ndc, 4.0);
    QCOMPARE(restored.facts.msa->negative_variance_truncated, true);
    QCOMPARE(restored.facts.msa->by_part_plot_available, true);
    QCOMPARE(restored.facts.msa->interaction_plot_available, false);
    QCOMPARE(restored.facts.msa->plot_point_count, std::size_t{18});
    QCOMPARE(*restored.facts.msa->linearity, 2.5);
    QCOMPARE(*restored.facts.msa->percent_linearity, 50.0);
    QCOMPARE(*restored.facts.msa->slope_p_value, 0.012);
    QCOMPARE(*restored.facts.msa->intercept_p_value, 0.34);
    QCOMPARE(*restored.facts.msa->residual_s, 0.88);
    QCOMPARE(*restored.facts.msa->average_bias, 0.12);
    QCOMPARE(*restored.facts.msa->average_bias_p, 0.45);
    QCOMPARE(*restored.facts.msa->process_variation_used, 16.5368);
    QVERIFY(restored.facts.reliability.has_value());
    QCOMPARE(*restored.facts.reliability->failure_count, std::size_t{5});
    QCOMPARE(restored.facts.reliability->event_encoding, std::string("failure_suspension"));
    page.facts.descriptive = datalab::domain::DescriptiveFacts{9, 1, 4.5, 0.8};
    page.facts.logistic = datalab::domain::LogisticFacts{};
    page.facts.logistic->converged = true;
    page.facts.logistic->complete_separation = false;
    page.facts.logistic->hosmer_lemeshow_statistic = 3.5;
    page.facts.logistic->hosmer_lemeshow_p = 0.42;
    page.facts.logistic->hosmer_lemeshow_df = 7;
    page.facts.logistic->hosmer_lemeshow_groups = 9;
    page.facts.logistic->hosmer_lemeshow_status = "computed";
    page.facts.logistic->high_leverage_count = 2;
    page.facts.logistic->leverage_threshold = 0.75;
    page.facts.logistic->maximum_leverage = 0.62;
    page.facts.logistic->maximum_vif = 4.8;
    page.facts.pca = datalab::domain::PcaFacts{"standardized", 2, 1};
    page.facts.pca->residual_ad_p = 0.22;
    page.facts.pca->diagnostic_plot_count = 1;
    page.facts.forecast = datalab::domain::ForecastFacts{3.2, 0.8};
    datalab::domain::ChiSquareFacts chi_square_facts;
    chi_square_facts.statistic = 5.1;
    chi_square_facts.p_value = 0.024;
    chi_square_facts.degrees_of_freedom = 1.0;
    chi_square_facts.row_count = 2;
    chi_square_facts.column_count = 2;
    chi_square_facts.total_count = 100;
    chi_square_facts.missing_count = 3;
    chi_square_facts.likelihood_ratio_statistic = 5.0;
    chi_square_facts.likelihood_ratio_p_value = 0.025;
    chi_square_facts.plot_available = true;
    page.facts.chi_square = chi_square_facts;
    const auto restored_more = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored_more.facts.descriptive.has_value());
    QCOMPARE(restored_more.facts.descriptive->n, std::size_t{9});
    QVERIFY(restored_more.facts.logistic.has_value());
    QCOMPARE(restored_more.facts.logistic->hosmer_lemeshow_status, std::string("computed"));
    QCOMPARE(*restored_more.facts.logistic->hosmer_lemeshow_statistic, 3.5);
    QCOMPARE(*restored_more.facts.logistic->hosmer_lemeshow_df, std::size_t{7});
    QCOMPARE(restored_more.facts.logistic->high_leverage_count, std::size_t{2});
    QCOMPARE(*restored_more.facts.logistic->leverage_threshold, 0.75);
    QCOMPARE(*restored_more.facts.logistic->maximum_vif, 4.8);
    QVERIFY(restored_more.facts.pca.has_value());
    QCOMPARE(restored_more.facts.pca->anomaly_count, std::size_t{1});
    QCOMPARE(*restored_more.facts.pca->residual_ad_p, 0.22);
    QCOMPARE(restored_more.facts.pca->diagnostic_plot_count, std::size_t{1});
    QCOMPARE(restored_more.facts.pca->observation_count, std::size_t{0});
    QVERIFY(!restored_more.facts.pca->t2_limit.has_value());
    QVERIFY(restored_more.facts.forecast.has_value());
    QCOMPARE(*restored_more.facts.forecast->mape, 3.2);
    QVERIFY(restored_more.facts.chi_square.has_value());
    QCOMPARE(restored_more.facts.chi_square->total_count, std::size_t{100});
    QCOMPARE(restored_more.facts.chi_square->missing_count, std::size_t{3});
    QCOMPARE(*restored_more.facts.chi_square->likelihood_ratio_statistic, 5.0);
    QVERIFY(restored_more.facts.chi_square->plot_available);
    datalab::domain::ChiSquareGofFacts gof_facts;
    gof_facts.statistic = 8.9583;
    gof_facts.p_value = 0.03;
    gof_facts.degrees_of_freedom = 3.0;
    gof_facts.category_count = 4;
    gof_facts.total_count = 40;
    gof_facts.missing_count = 2;
    gof_facts.expected_below_five_count = 2;
    gof_facts.minimum_expected_count = 2.4;
    gof_facts.validity_status = "caution";
    gof_facts.recommendation = "建议合并相邻类别后复算；当前 P 值可作为探索性证据。";
    gof_facts.plot_available = true;
    gof_facts.proportion_source = "specified";
    page.facts.chi_square_gof = gof_facts;
    datalab::domain::PowerFacts power_facts;
    power_facts.power = 0.81;
    power_facts.effect_size = 0.5;
    power_facts.mode = "one_sample_sample_size";
    power_facts.sample_size = 34;
    power_facts.target = 0.8;
    power_facts.actual_power = 0.812;
    page.facts.power = power_facts;
    page.configuration.inference.expected_proportions = "0.1,0.2,0.3,0.4";
    page.configuration.inference.gof_category_column = 2;
    page.configuration.power.sample_size_list = "20,30";
    page.configuration.power.effect_size_list = "0.4,0.5";
    const auto restored_gof = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored_gof.facts.chi_square_gof.has_value());
    QCOMPARE(*restored_gof.facts.chi_square_gof->statistic, 8.9583);
    QCOMPARE(restored_gof.facts.chi_square_gof->missing_count, std::size_t{2});
    QCOMPARE(restored_gof.facts.chi_square_gof->expected_below_five_count, std::size_t{2});
    QCOMPARE(*restored_gof.facts.chi_square_gof->minimum_expected_count, 2.4);
    QCOMPARE(restored_gof.facts.chi_square_gof->validity_status, std::string("caution"));
    QVERIFY(restored_gof.facts.power.has_value());
    QCOMPARE(*restored_gof.facts.power->actual_power, 0.812);
    QCOMPARE(restored_gof.configuration.inference.expected_proportions,
             std::string("0.1,0.2,0.3,0.4"));
    QCOMPARE(restored_gof.configuration.power.sample_size_list, std::string("20,30"));
    datalab::domain::OutlierTestFacts outlier_facts;
    outlier_facts.n = 5;
    outlier_facts.missing_count = 1;
    outlier_facts.g_statistic = 1.7;
    outlier_facts.source_row = 5;
    outlier_facts.direction = "largest";
    outlier_facts.assumption_status = "not_verified";
    page.facts.outlier_test = outlier_facts;
    const auto restored_outlier = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored_outlier.facts.outlier_test.has_value());
    QCOMPARE(*restored_outlier.facts.outlier_test->g_statistic, 1.7);
    QCOMPARE(*restored_outlier.facts.outlier_test->source_row, std::size_t{5});
    outlier_facts.method = "dixon_r10";
    outlier_facts.dixon_r = 0.55;
    outlier_facts.critical_value = 0.642;
    page.facts.outlier_test = outlier_facts;
    page.configuration.inference.outlier_method = "dixon_r10";
    page.configuration.inference.tolerance_method = "nonparametric";
    page.configuration.inference.known_sigma = 1.25;
    const auto restored_outlier2 = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QCOMPARE(restored_outlier2.facts.outlier_test->method, std::string("dixon_r10"));
    QCOMPARE(*restored_outlier2.facts.outlier_test->dixon_r, 0.55);
    QCOMPARE(*restored_outlier2.facts.outlier_test->critical_value, 0.642);
    QCOMPARE(restored_outlier2.configuration.inference.outlier_method,
             std::string("dixon_r10"));
    QCOMPARE(restored_outlier2.configuration.inference.tolerance_method,
             std::string("nonparametric"));
    QCOMPARE(*restored_outlier2.configuration.inference.known_sigma, 1.25);

    page.facts.variability = datalab::domain::VariabilityFacts{};
    page.facts.variability->factor_count = 2;
    page.facts.variability->valid_count = 8;
    page.facts.variability->missing_count = 1;
    page.facts.variability->cell_count = 4;
    page.facts.variability->overall_mean = 12.5;
    page.facts.variability->mean_of_cell_sds = 1.1;
    page.facts.variability->factor_names = {"零件", "操作者"};
    const auto restored_var = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored_var.facts.variability.has_value());
    QCOMPARE(restored_var.facts.variability->factor_count, std::size_t{2});
    QCOMPARE(restored_var.facts.variability->cell_count, std::size_t{4});
    QCOMPARE(*restored_var.facts.variability->overall_mean, 12.5);
    QCOMPARE(restored_var.facts.variability->factor_names.size(), std::size_t{2});

    page.facts.t_test = datalab::domain::TTestFacts{};
    page.facts.t_test->kind = "one_sample_z";
    page.facts.t_test->n = 5;
    page.facts.t_test->z_statistic = 1.2;
    page.facts.t_test->known_sigma = 2.0;
    page.facts.t_test->sample_standard_deviation = 1.8;
    const auto restored_z = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored_z.facts.t_test.has_value());
    QCOMPARE(restored_z.facts.t_test->kind, std::string("one_sample_z"));
    QCOMPARE(*restored_z.facts.t_test->z_statistic, 1.2);
    QCOMPARE(*restored_z.facts.t_test->known_sigma, 2.0);

    page.facts.multi_vari = datalab::domain::MultiVariFacts{2, 6, 1, 1.0, {"零件", "操作者"}};
    const auto restored_multi = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored_multi.facts.multi_vari.has_value());
    QCOMPARE(restored_multi.facts.multi_vari->factor_count, std::size_t{2});
    QCOMPARE(restored_multi.facts.multi_vari->valid_count, std::size_t{6});
    QCOMPARE(restored_multi.facts.multi_vari->missing_count, std::size_t{1});
    QCOMPARE(restored_multi.facts.multi_vari->factor_names.size(), std::size_t{2});

    page.facts.tolerance = datalab::domain::ToleranceFacts{};
    page.facts.tolerance->valid_count = 43;
    page.facts.tolerance->missing_count = 1;
    page.facts.tolerance->mean = 10.2;
    page.facts.tolerance->standard_deviation = 1.1;
    page.facts.tolerance->coverage = 0.90;
    page.facts.tolerance->confidence_level = 0.99;
    page.facts.tolerance->lower = 7.8;
    page.facts.tolerance->upper = 12.6;
    page.facts.tolerance->k_factor = 2.217;
    page.facts.tolerance->achieved_confidence = 0.957;
    page.facts.tolerance->method = "howe_two_sided";
    page.facts.tolerance->method_family = "normal";
    page.facts.tolerance->interval_type = "two_sided";
    page.facts.tolerance->assumption_status = "not_verified";
    page.configuration.inference.coverage_proportion = 0.90;
    const auto restored_tolerance = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored_tolerance.facts.tolerance.has_value());
    QCOMPARE(restored_tolerance.facts.tolerance->valid_count, std::size_t{43});
    QCOMPARE(restored_tolerance.facts.tolerance->missing_count, std::size_t{1});
    QCOMPARE(*restored_tolerance.facts.tolerance->k_factor, 2.217);
    QCOMPARE(*restored_tolerance.facts.tolerance->achieved_confidence, 0.957);
    QCOMPARE(restored_tolerance.facts.tolerance->method_family, std::string("normal"));
    QCOMPARE(restored_tolerance.facts.tolerance->method, std::string("howe_two_sided"));
    QCOMPARE(*restored_tolerance.configuration.inference.coverage_proportion, 0.90);

    page.facts.pca->observation_count = 12;
    page.facts.pca->t2_limit = 4.2;
    page.facts.pca->q_limit = 1.1;
    page.facts.pca->converged = true;
    page.facts.nonparametric = datalab::domain::NonparametricFacts{};
    page.facts.nonparametric->method = "kruskal_wallis";
    page.facts.nonparametric->p_value = 0.04;
    page.facts.nonparametric->p_value_unadjusted = 0.05;
    page.facts.nonparametric->group_count = 3;
    page.facts.nonparametric->plot_point_count = 12;
    page.facts.nonparametric->missing_count = 1;
    page.facts.nonparametric->location_estimate = -2.5;
    page.facts.nonparametric->ci_lower = -4.0;
    page.facts.nonparametric->ci_upper = -1.0;
    page.facts.nonparametric->dunn_available = true;
    page.facts.nonparametric->steel_dwass_available = false;
    page.facts.nonparametric->posthoc_method = "dunn";
    page.facts.nonparametric->posthoc_pair_count = 3;
    page.facts.nonparametric->grouping_letter_count = 3;
    page.facts.variance = datalab::domain::VarianceFacts{"Levene", 2.2, 0.176, 2};
    page.configuration.inference.variance_group_column = 3;
    const auto restored_new = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QCOMPARE(restored_new.facts.pca->observation_count, std::size_t{12});
    QCOMPARE(*restored_new.facts.pca->t2_limit, 4.2);
    QVERIFY(restored_new.facts.nonparametric.has_value());
    QCOMPARE(*restored_new.facts.nonparametric->p_value_unadjusted, 0.05);
    QCOMPARE(restored_new.facts.nonparametric->group_count, std::size_t{3});
    QCOMPARE(restored_new.facts.nonparametric->plot_point_count, std::size_t{12});
    QCOMPARE(*restored_new.facts.nonparametric->location_estimate, -2.5);
    QCOMPARE(*restored_new.facts.nonparametric->ci_lower, -4.0);
    QCOMPARE(*restored_new.facts.nonparametric->ci_upper, -1.0);
    QVERIFY(restored_new.facts.nonparametric->dunn_available);
    QVERIFY(!restored_new.facts.nonparametric->steel_dwass_available);
    QCOMPARE(restored_new.facts.nonparametric->posthoc_method, std::string{"dunn"});
    QCOMPARE(restored_new.facts.nonparametric->posthoc_pair_count, std::size_t{3});
    QCOMPARE(restored_new.facts.nonparametric->grouping_letter_count, std::size_t{3});
    QVERIFY(restored_new.facts.variance.has_value());
    QCOMPARE(restored_new.facts.variance->group_count, std::size_t{2});
    QCOMPARE(*restored_new.configuration.inference.variance_group_column, std::size_t{3});

    page.facts.proportion = datalab::domain::ProportionFacts{};
    page.facts.proportion->events = 2;
    page.facts.proportion->trials = 10;
    page.facts.proportion->proportion = 0.2;
    page.facts.proportion->hypothesized = 0.5;
    page.facts.proportion->method = "exact";
    page.facts.proportion->p_value = 0.109375;
    page.configuration.inference.proportion_method = "exact";
    page.facts.equivalence = datalab::domain::EquivalenceFacts{};
    page.facts.equivalence->kind = "one_sample";
    page.facts.equivalence->difference = 0.1;
    page.facts.equivalence->lower = -1.0;
    page.facts.equivalence->upper = 1.0;
    page.facts.equivalence->ci_method = "tost_1_minus_alpha";
    page.facts.equivalence->both_pvalues_below_alpha = true;
    page.facts.equivalence->within_limits = true;
    page.facts.equivalence->assumption_status = "not_verified";
    page.configuration.inference.equivalence_lower = -1.0;
    page.configuration.inference.equivalence_upper = 1.0;
    page.facts.doe = page.facts.doe.value_or(datalab::domain::DoeFacts{});
    page.facts.doe->residual_count = 8;
    const auto restored_round = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored_round.facts.proportion.has_value());
    QCOMPARE(restored_round.facts.proportion->events, std::size_t{2});
    QCOMPARE(restored_round.facts.proportion->method, std::string("exact"));
    page.facts.proportion->kind = "two_sample";
    page.facts.proportion->second_events = 8;
    page.facts.proportion->second_trials = 20;
    page.facts.proportion->second_proportion = 0.4;
    page.facts.proportion->difference = -0.2;
    page.facts.proportion->method = "normal";
    const auto restored_two = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QCOMPARE(restored_two.facts.proportion->kind, std::string("two_sample"));
    QCOMPARE(*restored_two.facts.proportion->second_events, std::size_t{8});
    QCOMPARE(*restored_two.facts.proportion->difference, -0.2);
    page.facts.poisson_rate = datalab::domain::PoissonRateFacts{};
    page.facts.poisson_rate->kind = "one_sample";
    page.facts.poisson_rate->events = 4;
    page.facts.poisson_rate->exposure = 150.0;
    page.facts.poisson_rate->rate = 4.0 / 150.0;
    page.facts.poisson_rate->method = "exact";
    const auto restored_poisson = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored_poisson.facts.poisson_rate.has_value());
    QCOMPARE(restored_poisson.facts.poisson_rate->events, std::size_t{4});
    QCOMPARE(restored_poisson.facts.poisson_rate->kind, std::string("one_sample"));
    page.facts.box_cox = datalab::domain::BoxCoxFacts{};
    page.facts.box_cox->lambda = 0.5;
    page.facts.box_cox->n = 4;
    page.facts.box_cox->missing_count = 1;
    page.facts.box_cox->transformed_standard_deviation = 0.2;
    page.facts.box_cox->rounded_lambda = true;
    const auto restored_box = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored_box.facts.box_cox.has_value());
    QCOMPARE(restored_box.facts.box_cox->n, std::size_t{4});
    QCOMPARE(restored_box.facts.box_cox->lambda, 0.5);
    QVERIFY(restored_round.facts.equivalence.has_value());
    QCOMPARE(restored_round.facts.equivalence->within_limits, true);
    QCOMPARE(restored_round.facts.equivalence->ci_method, std::string("tost_1_minus_alpha"));
    QCOMPARE(restored_round.facts.equivalence->both_pvalues_below_alpha, true);
    QCOMPARE(*restored_round.configuration.inference.equivalence_lower, -1.0);
    page.facts.t_test = datalab::domain::TTestFacts{};
    page.facts.t_test->kind = "one_sample";
    page.facts.t_test->n = 8;
    page.facts.t_test->missing_count = 1;
    page.facts.t_test->mean = 2.5;
    page.facts.t_test->p_value = 0.04;
    page.facts.t_test->assumption_status = "not_verified";
    page.facts.normality = datalab::domain::NormalityFacts{};
    page.facts.normality->n = 8;
    page.facts.normality->decision = "fail_to_reject";
    page.facts.normality->p_value = 0.4;
    page.facts.correlation = datalab::domain::CorrelationFacts{};
    page.facts.correlation->method = "pearson";
    page.facts.correlation->variable_count = 2;
    page.facts.correlation->n = 6;
    const auto restored_infer = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored_infer.facts.t_test.has_value());
    QCOMPARE(restored_infer.facts.t_test->kind, std::string("one_sample"));
    QCOMPARE(restored_infer.facts.t_test->n, std::size_t{8});
    QVERIFY(restored_infer.facts.normality.has_value());
    QCOMPARE(restored_infer.facts.normality->decision, std::string("fail_to_reject"));
    QVERIFY(restored_infer.facts.correlation.has_value());
    QCOMPARE(restored_infer.facts.correlation->n, std::size_t{6});
    QVERIFY(restored_round.facts.doe.has_value());
    QCOMPARE(restored_round.facts.doe->residual_count, std::size_t{8});
}

QTEST_APPLESS_MAIN(OutputSerializationTest)

#include "output_serialization_test.moc"
