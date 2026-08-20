#include "application/analysis_service.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <string>

class DoeFactorialOutputTest final : public QObject {
    Q_OBJECT

private slots:
    void buildsEffectsParetoAndCubeForTwoFactors();
    void twoFactorResponsePagePlotOrder();
    void responsePageIncludesResidualFourPack();
    void fourFactorDesignSkipsCubePlotWithDiagnostic();
    void threeFactorContourUsesSelectedAxes();
    void threeFactorContourActualHoldChangesGrid();
};

void DoeFactorialOutputTest::buildsEffectsParetoAndCubeForTwoFactors()
{
    // # source: formula_reference — 2-factor response page has Pareto, cube, series.
    datalab::domain::DataTable table;
    table.columns = {"A", "B", "Y"};
    table.rows = {
        {"-1", "-1", "4.3"},
        {"1", "-1", "12.1"},
        {"-1", "1", "5.8"},
        {"1", "1", "15.4"},
        {"-1", "-1", "4.7"},
        {"1", "-1", "11.6"},
        {"-1", "1", "6.2"},
        {"1", "1", "14.9"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.doe.response_column = 2;
    configuration.doe.factor_columns = {0, 1};
    configuration.doe.low_levels = {"-1", "-1"};
    configuration.doe.high_levels = {"1", "1"};
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::doe_factorial(table, configuration);
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title.find("Pareto") != std::string::npos
                                || plot.title.find("效应") != std::string::npos;
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title.find("立方") != std::string::npos;
                        }));
    bool main_effect_has_series = false;
    for (const auto& plot : page.plots) {
        if (plot.title.find("主效应") != std::string::npos) {
            QVERIFY(!plot.series.empty());
            QVERIFY(plot.series.front().show_points);
            main_effect_has_series = true;
        }
        if (plot.kind == datalab::domain::PlotKind::pareto) {
            QVERIFY(!plot.categories.empty());
            QVERIFY(plot.cumulative_percent.empty());
            QVERIFY(!plot.center.empty());
        }
    }
    QVERIFY(main_effect_has_series);
    QVERIFY(page.facts.doe.has_value());
    QVERIFY(!page.facts.doe->largest_standardized_effect_term.empty());
    QVERIFY(page.facts.doe->cube_plot_available);
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::contour
                                && !plot.contour_x.empty()
                                && !plot.matrix_values.empty();
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::surface;
                        }));
}

void DoeFactorialOutputTest::twoFactorResponsePagePlotOrder()
{
    // # source: formula_reference — plot index contract, not Minitab export.
    datalab::domain::DataTable table;
    table.columns = {"A", "B", "Y"};
    table.rows = {
        {"-1", "-1", "4.3"}, {"1", "-1", "12.1"},
        {"-1", "1", "5.8"}, {"1", "1", "15.4"},
        {"-1", "-1", "4.7"}, {"1", "-1", "11.6"},
        {"-1", "1", "6.2"}, {"1", "1", "14.9"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.doe.response_column = 2;
    configuration.doe.factor_columns = {0, 1};
    configuration.doe.low_levels = {"-1", "-1"};
    configuration.doe.high_levels = {"1", "1"};
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::doe_factorial(table, configuration);
    QCOMPARE(page.plots.size(), std::size_t{11});
    QCOMPARE(page.plots[0].kind, datalab::domain::PlotKind::pareto);
    QVERIFY(page.plots[1].title.find("立方") != std::string::npos);
    QVERIFY(page.plots[2].title.find("主效应") != std::string::npos);
    QVERIFY(page.plots[3].title.find("主效应") != std::string::npos);
    QVERIFY(page.plots[4].title.find("交互") != std::string::npos);
    QCOMPARE(page.plots[5].kind, datalab::domain::PlotKind::contour);
    QCOMPARE(page.plots[6].kind, datalab::domain::PlotKind::surface);
    QCOMPARE(page.plots[7].title, std::string("残差与拟合值"));
    QCOMPARE(page.plots[8].title, std::string("残差与观测顺序"));
    QCOMPARE(page.plots[9].title, std::string("残差正态概率图"));
    QCOMPARE(page.plots[10].title, std::string("残差直方图"));
}

void DoeFactorialOutputTest::responsePageIncludesResidualFourPack()
{
    datalab::domain::DataTable table;
    table.columns = {"A", "B", "Y"};
    table.rows = {
        {"-1", "-1", "4.3"},
        {"1", "-1", "12.1"},
        {"-1", "1", "5.8"},
        {"1", "1", "15.4"},
        {"-1", "-1", "4.7"},
        {"1", "-1", "11.6"},
        {"-1", "1", "6.2"},
        {"1", "1", "14.9"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.doe.response_column = 2;
    configuration.doe.factor_columns = {0, 1};
    configuration.doe.low_levels = {"-1", "-1"};
    configuration.doe.high_levels = {"1", "1"};
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::doe_factorial(table, configuration);
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
    bool fitted_has_zero = false;
    bool probability_has_rows = false;
    for (const auto& plot : page.plots) {
        if (plot.title == "残差与拟合值") {
            QVERIFY(std::any_of(plot.series.cbegin(), plot.series.cend(),
                                [](const datalab::domain::PlotSeries& series) {
                                    return series.label == "残差 = 0";
                                }));
            fitted_has_zero = true;
        }
        if (plot.title == "残差正态概率图") {
            QVERIFY(!plot.source_rows.empty());
            probability_has_rows = true;
        }
    }
    QVERIFY(fitted_has_zero);
    QVERIFY(probability_has_rows);
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title.find("Pareto") != std::string::npos
                                || plot.title.find("效应") != std::string::npos;
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title.find("立方") != std::string::npos;
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::contour;
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::surface;
                        }));
    QVERIFY(page.facts.doe.has_value());
    QCOMPARE(page.facts.doe->residual_count, std::size_t{8});
    QVERIFY(!page.facts.doe->largest_standardized_effect_term.empty());
}

void DoeFactorialOutputTest::fourFactorDesignSkipsCubePlotWithDiagnostic()
{
    // # source: formula_reference — ≥4 factors skip cube plot with explicit diagnostic.
    datalab::domain::DataTable table;
    table.columns = {"A", "B", "C", "D", "Y"};
    table.rows = {
        {"-1", "-1", "-1", "-1", "4.3"},
        {"1", "-1", "-1", "-1", "12.1"},
        {"-1", "1", "-1", "-1", "5.8"},
        {"1", "1", "-1", "-1", "15.4"},
        {"-1", "-1", "1", "-1", "6.1"},
        {"1", "-1", "1", "-1", "11.2"},
        {"-1", "1", "1", "-1", "7.0"},
        {"1", "1", "1", "-1", "14.0"},
        {"-1", "-1", "-1", "1", "5.5"},
        {"1", "-1", "-1", "1", "13.0"},
        {"-1", "1", "-1", "1", "6.4"},
        {"1", "1", "-1", "1", "14.5"},
        {"-1", "-1", "1", "1", "6.8"},
        {"1", "-1", "1", "1", "12.8"},
        {"-1", "1", "1", "1", "7.2"},
        {"1", "1", "1", "1", "15.0"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.doe.response_column = 4;
    configuration.doe.factor_columns = {0, 1, 2, 3};
    configuration.doe.low_levels = {"-1", "-1", "-1", "-1"};
    configuration.doe.high_levels = {"1", "1", "1", "1"};
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::doe_factorial(table, configuration);
    QVERIFY(std::none_of(page.plots.cbegin(), page.plots.cend(),
                         [](const datalab::domain::PlotSpec& plot) {
                             return plot.title.find("立方") != std::string::npos;
                         }));
    QVERIFY(std::any_of(page.diagnostics.cbegin(), page.diagnostics.cend(),
                        [](const datalab::domain::DiagnosticMessage& diagnostic) {
                            return diagnostic.code == "cube_plot_requires_2_or_3_factors";
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title.find("Pareto") != std::string::npos
                                || plot.title.find("效应") != std::string::npos;
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title.find("主效应") != std::string::npos;
                        }));
    QVERIFY(page.facts.doe.has_value());
    QCOMPARE(page.facts.doe->factor_count, std::size_t{4});
    QVERIFY(!page.facts.doe->cube_plot_available);
}

void DoeFactorialOutputTest::threeFactorContourUsesSelectedAxes()
{
    // # source: formula_reference — contour axes follow configured factor names.
    datalab::domain::DataTable table;
    table.columns = {"A", "B", "C", "Y"};
    table.rows = {
        {"-1", "-1", "-1", "4.0"},
        {"1", "-1", "-1", "10.0"},
        {"-1", "1", "-1", "5.0"},
        {"1", "1", "-1", "12.0"},
        {"-1", "-1", "1", "6.0"},
        {"1", "-1", "1", "11.0"},
        {"-1", "1", "1", "7.0"},
        {"1", "1", "1", "13.0"},
    };
    datalab::domain::AnalysisConfiguration configuration;
    configuration.doe.response_column = 3;
    configuration.doe.factor_columns = {0, 1, 2};
    configuration.doe.low_levels = {"-1", "-1", "-1"};
    configuration.doe.high_levels = {"1", "1", "1"};
    configuration.doe.contour_x_factor = "A";
    configuration.doe.contour_y_factor = "C";
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::doe_factorial(table, configuration);
    QVERIFY(page.facts.doe.has_value());
    QVERIFY(page.facts.doe->contour_plot_available);
    QCOMPARE(page.facts.doe->contour_x_factor, std::string{"A"});
    QCOMPARE(page.facts.doe->contour_y_factor, std::string{"C"});
    QVERIFY(std::find(page.facts.doe->held_factor_names.cbegin(),
                      page.facts.doe->held_factor_names.cend(),
                      "B") != page.facts.doe->held_factor_names.cend());
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.kind == datalab::domain::PlotKind::contour
                                && plot.title.find("A vs C") != std::string::npos
                                && plot.x_axis_title.find("A") != std::string::npos
                                && plot.y_axis_title.find("C") != std::string::npos;
                        }));
}

void DoeFactorialOutputTest::threeFactorContourActualHoldChangesGrid()
{
    // # source: formula_reference — B=1 (high) changes contour vs hold-at-0.
    datalab::domain::DataTable table;
    table.columns = {"A", "B", "C", "Y"};
    table.rows = {
        {"-1", "-1", "-1", "4.0"},
        {"1", "-1", "-1", "10.0"},
        {"-1", "1", "-1", "5.0"},
        {"1", "1", "-1", "12.0"},
        {"-1", "-1", "1", "6.0"},
        {"1", "-1", "1", "11.0"},
        {"-1", "1", "1", "7.0"},
        {"1", "1", "1", "13.0"},
    };
    datalab::domain::AnalysisConfiguration base;
    base.doe.response_column = 3;
    base.doe.factor_columns = {0, 1, 2};
    base.doe.low_levels = {"-1", "-1", "-1"};
    base.doe.high_levels = {"1", "1", "1"};
    base.doe.contour_x_factor = "A";
    base.doe.contour_y_factor = "C";
    const auto zero_page =
        datalab::application::AnalysisService::doe_factorial(table, base);
    datalab::domain::AnalysisConfiguration held = base;
    held.doe.contour_hold_actual["B"] = "1";
    const auto held_page =
        datalab::application::AnalysisService::doe_factorial(table, held);
    QVERIFY(held_page.facts.doe.has_value());
    QCOMPARE(held_page.facts.doe->held_factor_names.front(), std::string{"B"});
    QVERIFY(!held_page.facts.doe->held_actual_values.empty());
    QCOMPARE(held_page.facts.doe->held_actual_values.front(), std::string{"1"});
    QVERIFY(!held_page.facts.doe->held_coded_values.empty());
    QVERIFY(std::abs(held_page.facts.doe->held_coded_values.front() - 1.0) < 1.0e-9);

    const auto* zero_contour = static_cast<const datalab::domain::PlotSpec*>(nullptr);
    const auto* held_contour = static_cast<const datalab::domain::PlotSpec*>(nullptr);
    for (const auto& plot : zero_page.plots) {
        if (plot.kind == datalab::domain::PlotKind::contour) {
            zero_contour = &plot;
            break;
        }
    }
    for (const auto& plot : held_page.plots) {
        if (plot.kind == datalab::domain::PlotKind::contour) {
            held_contour = &plot;
            break;
        }
    }
    QVERIFY(zero_contour != nullptr);
    QVERIFY(held_contour != nullptr);
    QVERIFY(!zero_contour->matrix_values.empty());
    QVERIFY(!held_contour->matrix_values.empty());
    QVERIFY(std::abs(zero_contour->matrix_values[0][0]
                     - held_contour->matrix_values[0][0]) > 1.0e-6);

    held.doe.contour_hold_actual["B"] = "not_a_level";
    const auto bad_page =
        datalab::application::AnalysisService::doe_factorial(table, held);
    QVERIFY(std::any_of(
        bad_page.diagnostics.cbegin(), bad_page.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "invalid_hold_value";
        }));
}

QTEST_APPLESS_MAIN(DoeFactorialOutputTest)

#include "doe_factorial_output_test.moc"
