#include "domain/quality_types.h"
#include "infrastructure/output_serialization.h"

#include <QtTest/QtTest>

class OutputSerializationTest final : public QObject {
    Q_OBJECT

private slots:
    void preservesAnalysisConfiguration();
    void preservesMultiSeriesPlot();
};

void OutputSerializationTest::preservesAnalysisConfiguration()
{
    datalab::domain::OutputPage page;
    page.id = "page-1";
    page.title = "Round trip";
    page.configuration.analysis_name = "ARIMA";
    page.configuration.chart_type = "arima";
    page.configuration.variable_columns = {1, 3};
    page.configuration.selection.measurement_column = 2;
    page.configuration.selection.time_column = 0;
    page.configuration.inspected_constant = 8;
    page.configuration.first_events_column = 5;
    page.configuration.first_trials_column = 6;
    page.configuration.second_events_column = 7;
    page.configuration.second_trials_column = 8;
    page.configuration.row_category_column = 9;
    page.configuration.column_category_column = 10;
    page.configuration.specifications.lower = 1.5;
    page.configuration.specifications.upper = 9.5;
    page.configuration.arima_time_column = 0;
    page.configuration.arima_value_column = 2;
    page.configuration.arima_selection_criterion = "bic";
    page.configuration.forecast_periods = 6;
    page.configuration.ewma_lambda = 0.25;
    page.configuration.gage_operator_column = 4;
    page.configuration.pca_variable_columns = {1, 2, 3};
    page.configuration.pca_mode = "standardized";
    page.configuration.seasonal_period = 12;
    page.configuration.doe_factor_names = {"Temperature", "Pressure"};
    page.configuration.doe_factor_columns = {1, 2};
    page.configuration.doe_response_column = 0;
    page.configuration.reliability_group_column = 11;
    page.configuration.power_group_count = 4;
    page.configuration.power_null_proportion = 0.4;
    page.configuration.power_second_proportion = 0.7;
    page.configuration.power_variance_method = "unpooled";
    page.interpretation.push_back({
        "结论", {"保留配置"}, datalab::domain::DiagnosticMessage::Severity::info});

    const auto restored = datalab::infrastructure::output_page_from_json(
        datalab::infrastructure::output_page_to_json(page));
    QCOMPARE(QString::fromStdString(restored.configuration.analysis_name),
             QStringLiteral("ARIMA"));
    QCOMPARE(restored.configuration.variable_columns.size(), std::size_t{2});
    QCOMPARE(restored.configuration.selection.measurement_column, std::size_t{2});
    QCOMPARE(*restored.configuration.selection.time_column, std::size_t{0});
    QCOMPARE(*restored.configuration.inspected_constant, std::size_t{8});
    QCOMPARE(*restored.configuration.first_events_column, std::size_t{5});
    QCOMPARE(*restored.configuration.first_trials_column, std::size_t{6});
    QCOMPARE(*restored.configuration.second_events_column, std::size_t{7});
    QCOMPARE(*restored.configuration.second_trials_column, std::size_t{8});
    QCOMPARE(*restored.configuration.row_category_column, std::size_t{9});
    QCOMPARE(*restored.configuration.column_category_column, std::size_t{10});
    QCOMPARE(*restored.configuration.specifications.lower, 1.5);
    QCOMPARE(*restored.configuration.arima_value_column, std::size_t{2});
    QCOMPARE(restored.configuration.arima_selection_criterion, std::string("bic"));
    QCOMPARE(restored.configuration.forecast_periods, 6);
    QCOMPARE(restored.configuration.ewma_lambda, 0.25);
    QCOMPARE(*restored.configuration.gage_operator_column, std::size_t{4});
    QCOMPARE(restored.configuration.pca_variable_columns.size(), std::size_t{3});
    QCOMPARE(restored.configuration.pca_mode, std::string("standardized"));
    QCOMPARE(restored.configuration.seasonal_period, std::size_t{12});
    QCOMPARE(restored.configuration.doe_factor_names.size(), std::size_t{2});
    QCOMPARE(restored.configuration.doe_factor_columns.size(), std::size_t{2});
    QCOMPARE(*restored.configuration.doe_response_column, std::size_t{0});
    QCOMPARE(*restored.configuration.reliability_group_column, std::size_t{11});
    QCOMPARE(restored.configuration.power_group_count, std::size_t{4});
    QCOMPARE(restored.configuration.power_null_proportion, 0.4);
    QCOMPARE(restored.configuration.power_second_proportion, 0.7);
    QCOMPARE(restored.configuration.power_variance_method, std::string("unpooled"));
    QCOMPARE(restored.interpretation.size(), std::size_t{1});
}

void OutputSerializationTest::preservesMultiSeriesPlot()
{
    datalab::domain::OutputPage page;
    datalab::domain::PlotSpec plot;
    plot.title = "Forecast";
    plot.values = {10.0, 11.0};
    plot.center = {10.5, 10.5};
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
    QCOMPARE(restored.plots.front().series.size(), std::size_t{3});
    QCOMPARE(static_cast<int>(restored.plots.front().series[1].role),
             static_cast<int>(datalab::domain::PlotSeriesRole::forecast));
    QCOMPARE(QString::fromStdString(restored.plots.front().series[2].label),
             QStringLiteral("95% CI"));
    QCOMPARE(restored.plots.front().series[2].lower[0], 11.0);
    QCOMPARE(restored.plots.front().series[0].show_points, true);
}

QTEST_APPLESS_MAIN(OutputSerializationTest)

#include "output_serialization_test.moc"
