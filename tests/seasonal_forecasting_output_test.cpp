#include "application/analysis_service.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <string>

class SeasonalForecastingOutputTest final : public QObject {
    Q_OBJECT

private slots:
    void additiveIncludesSeasonalIndexAndForecastFacts();
    void multiplicativeUsesDistinctMethodMetadata();
};

namespace {

datalab::domain::DataTable seasonal_table()
{
    datalab::domain::DataTable table;
    table.columns = {"Demand"};
    table.rows = {
        {"100"}, {"110"}, {"120"}, {"105"},
        {"104"}, {"115"}, {"125"}, {"109"},
        {"108"}, {"119"}, {"129"}, {"113"}};
    return table;
}

bool has_table_title(const datalab::domain::OutputPage& page, const std::string& title)
{
    return std::any_of(page.tables.cbegin(), page.tables.cend(),
                       [&](const datalab::domain::StatisticTable& table) {
                           return table.title == title;
                       });
}

}  // namespace

void SeasonalForecastingOutputTest::additiveIncludesSeasonalIndexAndForecastFacts()
{
    datalab::domain::AnalysisConfiguration configuration;
    configuration.selection.measurement_column = 0;
    configuration.time_series.seasonal_period = 4;
    configuration.time_series.forecast_periods = 3;
    configuration.time_series.seasonal_error_model = "additive";
    configuration.time_series.seasonal_trend_model = "additive";
    const auto page = datalab::application::AnalysisService::seasonal_forecasting(
        seasonal_table(), configuration);
    QVERIFY(has_table_title(page, "季节指数"));
    QVERIFY(has_table_title(page, "拟合与预测明细"));
    QVERIFY(page.facts.forecast.has_value());
    QVERIFY(page.facts.forecast->mape.has_value());
    QCOMPARE(page.method_metadata.estimation_method, std::string{"holt_winters_additive"});
    QCOMPARE(page.method_metadata.parameter_source, std::string{"specified"});
}

void SeasonalForecastingOutputTest::multiplicativeUsesDistinctMethodMetadata()
{
    datalab::domain::AnalysisConfiguration configuration;
    configuration.selection.measurement_column = 0;
    configuration.time_series.seasonal_period = 4;
    configuration.time_series.forecast_periods = 3;
    configuration.time_series.seasonal_error_model = "multiplicative";
    configuration.time_series.seasonal_trend_model = "additive";
    const auto page = datalab::application::AnalysisService::seasonal_forecasting(
        seasonal_table(), configuration);
    QVERIFY(has_table_title(page, "季节指数"));
    QVERIFY(has_table_title(page, "拟合与预测明细"));
    QVERIFY(page.facts.forecast.has_value());
    QVERIFY(page.facts.forecast->mape.has_value());
    QCOMPARE(page.method_metadata.estimation_method, std::string{"holt_winters_multiplicative"});
}

QTEST_APPLESS_MAIN(SeasonalForecastingOutputTest)

#include "seasonal_forecasting_output_test.moc"
