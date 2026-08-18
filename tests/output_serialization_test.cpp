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
    QCOMPARE(restored.facts.regression->influential_count, std::size_t{2});
    QCOMPARE(restored.facts.regression->rules.front().id, std::string("influence"));
    QCOMPARE(restored.facts.regression->rules.front().related_rows.front(),
             datalab::domain::RowId{4});
    QVERIFY(restored.facts.anova.has_value());
    QCOMPARE(restored.facts.anova->significant_terms.front(), std::string("Factor A"));
    QVERIFY(restored.facts.msa.has_value());
    QCOMPARE(*restored.facts.msa->ndc, 4.0);
    QCOMPARE(restored.facts.msa->negative_variance_truncated, true);
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
    page.facts.pca = datalab::domain::PcaFacts{"standardized", 2, 1};
    page.facts.forecast = datalab::domain::ForecastFacts{3.2, 0.8};
    const auto restored_more = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QVERIFY(restored_more.facts.descriptive.has_value());
    QCOMPARE(restored_more.facts.descriptive->n, std::size_t{9});
    QVERIFY(restored_more.facts.logistic.has_value());
    QCOMPARE(restored_more.facts.logistic->hosmer_lemeshow_status, std::string("computed"));
    QCOMPARE(*restored_more.facts.logistic->hosmer_lemeshow_statistic, 3.5);
    QCOMPARE(*restored_more.facts.logistic->hosmer_lemeshow_df, std::size_t{7});
    QCOMPARE(restored_more.facts.logistic->high_leverage_count, std::size_t{2});
    QVERIFY(restored_more.facts.pca.has_value());
    QCOMPARE(restored_more.facts.pca->anomaly_count, std::size_t{1});
    QCOMPARE(restored_more.facts.pca->observation_count, std::size_t{0});
    QVERIFY(!restored_more.facts.pca->t2_limit.has_value());
    QVERIFY(restored_more.facts.forecast.has_value());
    QCOMPARE(*restored_more.facts.forecast->mape, 3.2);

    page.facts.pca->observation_count = 12;
    page.facts.pca->t2_limit = 4.2;
    page.facts.pca->q_limit = 1.1;
    page.facts.pca->converged = true;
    page.facts.nonparametric = datalab::domain::NonparametricFacts{};
    page.facts.nonparametric->method = "kruskal_wallis";
    page.facts.nonparametric->p_value = 0.04;
    page.facts.nonparametric->p_value_unadjusted = 0.05;
    page.facts.variance = datalab::domain::VarianceFacts{"Levene", 2.2, 0.176, 2};
    page.configuration.inference.variance_group_column = 3;
    const auto restored_new = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QCOMPARE(restored_new.facts.pca->observation_count, std::size_t{12});
    QCOMPARE(*restored_new.facts.pca->t2_limit, 4.2);
    QVERIFY(restored_new.facts.nonparametric.has_value());
    QCOMPARE(*restored_new.facts.nonparametric->p_value_unadjusted, 0.05);
    QVERIFY(restored_new.facts.variance.has_value());
    QCOMPARE(restored_new.facts.variance->group_count, std::size_t{2});
    QCOMPARE(*restored_new.configuration.inference.variance_group_column, std::size_t{3});
}

QTEST_APPLESS_MAIN(OutputSerializationTest)

#include "output_serialization_test.moc"
