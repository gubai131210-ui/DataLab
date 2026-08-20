#include "application/analysis_service.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <string>

class TwoFactorAnovaOutputTest final : public QObject {
    Q_OBJECT

private slots:
    void residualFourPackAndInteractionPlot();
    void inestimableTermsKeepMissingFP();
};

void TwoFactorAnovaOutputTest::residualFourPackAndInteractionPlot()
{
    datalab::domain::DataTable table;
    table.columns = {"A", "B", "Y"};
    table.rows = {
        {"A1", "B1", "10.0"}, {"A1", "B1", "10.2"},
        {"A1", "B2", "12.0"}, {"A1", "B2", "12.1"},
        {"A2", "B1", "11.0"}, {"A2", "B1", "11.1"},
        {"A2", "B2", "13.0"}, {"A2", "B2", "13.2"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.inference.anova_response_column = 2;
    configuration.inference.anova_factor_a_column = 0;
    configuration.inference.anova_factor_b_column = 1;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::two_factor_anova(table, configuration);
    const auto has_title = [&](const std::string& title) {
        return std::any_of(page.plots.cbegin(), page.plots.cend(),
                           [&](const datalab::domain::PlotSpec& plot) {
                               return plot.title == title;
                           });
    };
    QVERIFY(has_title("残差与拟合值"));
    QVERIFY(has_title("残差与观测顺序"));
    QVERIFY(has_title("残差正态概率图"));
    QVERIFY(has_title("残差直方图"));
    QVERIFY(has_title("交互均值图"));
    bool fitted_has_zero = false;
    bool interaction_has_series = false;
    for (const auto& plot : page.plots) {
        QVERIFY(!plot.source_rows.empty());
        if (plot.title == "残差与拟合值") {
            QVERIFY(!plot.source_rows.empty());
            fitted_has_zero = std::any_of(
                plot.series.cbegin(), plot.series.cend(),
                [](const datalab::domain::PlotSeries& series) {
                    return series.label.find("0") != std::string::npos
                        || (!series.values.empty() && series.values.front() == 0.0
                            && series.values.back() == 0.0);
                });
        }
        if (plot.title == "交互均值图") {
            QVERIFY(plot.series.size() >= 2);
            QVERIFY(plot.series.front().values.size() >= 2);
            QVERIFY(plot.series.front().show_points);
            QVERIFY(!plot.source_rows.empty());
            interaction_has_series = true;
        }
    }
    QVERIFY(fitted_has_zero);
    QVERIFY(interaction_has_series);
}

void TwoFactorAnovaOutputTest::inestimableTermsKeepMissingFP()
{
    datalab::domain::DataTable table;
    table.columns = {"A", "B", "Y"};
    table.rows = {
        {"A1", "B1", "10.0"},
        {"A1", "B2", "12.0"},
        {"A2", "B1", "11.0"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.inference.anova_response_column = 2;
    configuration.inference.anova_factor_a_column = 0;
    configuration.inference.anova_factor_b_column = 1;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::two_factor_anova(table, configuration);
    QVERIFY(!page.tables.empty());
    const auto anova_table = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "ANOVA 表";
        });
    QVERIFY(anova_table != page.tables.cend());
    bool saw_inestimable = false;
    for (const auto& row : anova_table->rows) {
        if (row.size() >= 8 && row[7] == "rank_deficient") {
            saw_inestimable = true;
            QCOMPARE(row[5], std::string("*"));
            QCOMPARE(row[6], std::string("*"));
        }
    }
    QVERIFY(saw_inestimable);
}

QTEST_APPLESS_MAIN(TwoFactorAnovaOutputTest)

#include "two_factor_anova_output_test.moc"
