#include "application/analysis_service.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <string>
#include <vector>

class RegressionOutputTest final : public QObject {
    Q_OBJECT

private slots:
    void buildsStructuredRegressionContract();
    void buildsResidualPlotPerPredictor();
    void omitsUnusualObservationsWhenNoneFlagged();
    void buildsUnusualObservationsTableForLargeResidual();
    void buildsFittedLineBandsForSinglePredictor();
    void omitsFittedLineWhenMultiplePredictors();
};

void RegressionOutputTest::buildsStructuredRegressionContract()
{
    datalab::domain::DataTable table;
    table.columns = {"Temperature", "Pressure", "Response"};
    table.rows = {
        {"20", "2.1", "12.1"},
        {"22", "2.4", "13.4"},
        {"24", "", "14.2"},
        {"26", "3.0", "15.8"},
        {"28", "3.4", "17.1"},
        {"30", "3.7", "18.5"}};

    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {2, 0, 1};
    configuration.inference.confidence_level = 0.95;

    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::regression(table, configuration);

    QCOMPARE(page.method_name, std::string{"Linear Regression"});
    QCOMPARE(page.tables.size(), std::size_t{6});
    QCOMPARE(page.tables[0].title, std::string{"模型摘要"});
    QCOMPARE(page.tables[2].title, std::string{"回归方差分析"});
    QCOMPARE(page.tables[3].title, std::string{"假设检查"});
    QCOMPARE(page.tables[4].title, std::string{"拟合与诊断"});
    QCOMPARE(page.tables[5].title, std::string{"规则证据"});
    QVERIFY(page.parameter_summary.find("响应 = Response") != std::string::npos);
    QVERIFY(page.parameter_summary.find("预测变量 = Temperature, Pressure") != std::string::npos);
    QVERIFY(page.parameter_summary.find("有效观测 = 5") != std::string::npos);
    QVERIFY(page.facts.regression.has_value());
    QVERIFY(page.facts.regression->durbin_watson.has_value());
    QCOMPARE(page.facts.regression->assumptions.size(), std::size_t{3});
    QVERIFY(page.method_metadata.parameters.find("Response") != std::string::npos);
    QCOMPARE(page.method_metadata.source_rows,
             (std::vector<datalab::domain::RowId>{0, 1, 3, 4, 5}));

    QVERIFY(std::any_of(
        page.diagnostics.cbegin(), page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "missing_values";
        }));
    QCOMPARE(page.tables[3].headers,
             (std::vector<std::string>{"检查项", "状态", "统计量", "P-Value", "说明"}));
    QCOMPARE(page.tables[3].rows[0][0], std::string{"residual_normality"});
    QCOMPARE(page.tables[4].headers[2], std::string{"响应"});
    QCOMPARE(page.tables[4].headers[7], std::string{"学生化残差"});
    QCOMPARE(page.tables[4].rows.size(), std::size_t{5});
    QCOMPARE(page.tables[4].rows.front()[1], std::string{"1"});
    QCOMPARE(page.tables[4].rows.front()[2], std::string{"12.1"});
    QVERIFY(std::none_of(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table) {
            return table.title == "异常观测";
        }));
}

void RegressionOutputTest::buildsResidualPlotPerPredictor()
{
    datalab::domain::DataTable table;
    table.columns = {"Temperature", "Pressure", "Response"};
    table.rows = {
        {"20", "2.1", "12.1"},
        {"22", "2.4", "13.4"},
        {"24", "2.8", "14.2"},
        {"26", "3.0", "15.8"},
        {"28", "3.4", "17.1"},
        {"30", "3.7", "18.5"}};

    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {2, 0, 1};

    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::regression(table, configuration);

    QCOMPARE(page.plots.size(), std::size_t{5});
    QCOMPARE(page.plots[0].title, std::string{"残差与拟合值"});
    QCOMPARE(page.plots[1].title, std::string{"残差与观测顺序"});
    QCOMPARE(page.plots[2].title, std::string{"残差与预测变量 - Temperature"});
    QCOMPARE(page.plots[3].title, std::string{"残差与预测变量 - Pressure"});
    QCOMPARE(page.plots[4].title, std::string{"残差正态概率图"});
    QCOMPARE(page.plots[2].source_rows, page.plots[3].source_rows);
    QCOMPARE(page.plots[2].x_values.size(), std::size_t{6});
    QCOMPARE(page.plots[3].x_values.size(), std::size_t{6});
    QCOMPARE(page.plots[2].x_axis_title, std::string{"Temperature"});
    QCOMPARE(page.plots[3].x_axis_title, std::string{"Pressure"});
}

void RegressionOutputTest::omitsUnusualObservationsWhenNoneFlagged()
{
    datalab::domain::DataTable table;
    table.columns = {"X", "Y"};
    table.rows = {
        {"1", "1.0"}, {"2", "2.0"}, {"3", "3.0"}, {"4", "4.0"}, {"5", "5.0"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {1, 0};
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::regression(table, configuration);
    QVERIFY(page.facts.regression.has_value());
    QVERIFY(std::none_of(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table) {
            return table.title == "异常观测";
        }));
}

void RegressionOutputTest::buildsUnusualObservationsTableForLargeResidual()
{
    // # source: formula_reference — |standardized residual| > 2 flags R.
    datalab::domain::DataTable table;
    table.columns = {"X", "Y"};
    table.rows = {
        {"1", "1.0"}, {"2", "2.0"}, {"3", "3.0"}, {"4", "4.0"},
        {"5", "5.0"}, {"6", "6.0"}, {"7", "100.0"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {1, 0};
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::regression(table, configuration);
    const auto unusual = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table) {
            return table.title == "异常观测";
        });
    QVERIFY(unusual != page.tables.cend());
    QCOMPARE(unusual->headers,
             (std::vector<std::string>{
                 "观测", "原始行", "响应", "拟合值", "残差", "标准化残差",
                 "杠杆", "Cook", "DFITS", "标记"}));
    QVERIFY(!unusual->rows.empty());
    for (const auto& row : unusual->rows) {
        QCOMPARE(row.size(), std::size_t{10});
        QVERIFY(row.back().find('R') != std::string::npos
                || row.back().find('X') != std::string::npos
                || row.back().find('I') != std::string::npos);
    }
    const auto last = unusual->rows.back();
    QCOMPARE(last[1], std::string{"7"});
    QCOMPARE(last[2], std::string{"100"});
    QVERIFY(last.back().find('R') != std::string::npos);
    QVERIFY(page.facts.regression.has_value());
}

void RegressionOutputTest::buildsFittedLineBandsForSinglePredictor()
{
    // # source: formula_reference — PI wider than CI; points keep source_row.
    datalab::domain::DataTable table;
    table.columns = {"X", "Y"};
    table.rows = {
        {"1", "1.0"}, {"2", "2.1"}, {"3", "2.9"}, {"4", "4.2"}, {"5", "4.8"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {1, 0};
    configuration.inference.confidence_level = 0.95;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::regression(table, configuration);
    QVERIFY(!page.plots.empty());
    QCOMPARE(page.plots.front().title, std::string{"拟合线图"});
    QCOMPARE(page.plots.front().source_rows,
             (std::vector<std::size_t>{0, 1, 2, 3, 4}));
    const auto& series = page.plots.front().series;
    QVERIFY(series.size() >= 4);
    QCOMPARE(series[0].role, datalab::domain::PlotSeriesRole::actual);
    QCOMPARE(series[1].role, datalab::domain::PlotSeriesRole::fitted);
    QCOMPARE(series[2].role, datalab::domain::PlotSeriesRole::confidence_band);
    QCOMPARE(series[3].role, datalab::domain::PlotSeriesRole::confidence_band);
    QVERIFY(series[2].label.find("CI") != std::string::npos);
    QVERIFY(series[3].label.find("PI") != std::string::npos);
    QVERIFY(!series[2].lower.empty());
    QCOMPARE(series[2].lower.size(), series[2].upper.size());
    QCOMPARE(series[3].lower.size(), series[3].upper.size());
    const std::size_t mid = series[2].lower.size() / 2;
    const double ci_width = series[2].upper[mid] - series[2].lower[mid];
    const double pi_width = series[3].upper[mid] - series[3].lower[mid];
    QVERIFY(ci_width > 0.0);
    QVERIFY(pi_width > ci_width);
    bool residual_has_zero_line = false;
    for (const auto& plot : page.plots) {
        if (plot.title == "残差与拟合值") {
            QVERIFY(!plot.series.empty());
            QCOMPARE(plot.series.front().values.front(), 0.0);
            residual_has_zero_line = true;
        }
    }
    QVERIFY(residual_has_zero_line);
    QVERIFY(page.facts.regression.has_value());
    QCOMPARE(page.facts.regression->outlier_count, std::size_t{0});
}

void RegressionOutputTest::omitsFittedLineWhenMultiplePredictors()
{
    datalab::domain::DataTable table;
    table.columns = {"Temperature", "Pressure", "Response"};
    table.rows = {
        {"20", "2.1", "12.1"},
        {"22", "2.4", "13.4"},
        {"24", "2.8", "14.2"},
        {"26", "3.0", "15.8"},
        {"28", "3.4", "17.1"},
        {"30", "3.7", "18.5"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {2, 0, 1};
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::regression(table, configuration);
    QVERIFY(std::none_of(
        page.plots.cbegin(), page.plots.cend(),
        [](const datalab::domain::PlotSpec& plot) {
            return plot.title == "拟合线图";
        }));
    QCOMPARE(page.plots.front().title, std::string{"残差与拟合值"});
}

QTEST_APPLESS_MAIN(RegressionOutputTest)

#include "regression_output_test.moc"
