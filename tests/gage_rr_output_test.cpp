#include "application/analysis_service.h"
#include "domain/statistics/gage_rr.h"
#include "domain/statistics/nested_gage_rr.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

class GageRrOutputTest final : public QObject {
    Q_OBJECT

private slots:
    void contributionBarAndOperatorXbarR();
    void studyVarParetoCrossed();
    void studyVarParetoNested();
    void toleranceParetoAppearsOnlyWhenToleranceAvailable();
    void skipsMissingLabelsOnByPart();
    void skipsUnbalancedByPartPlots();
    void nestedContributionBarAndOperatorXbarR();
    void nestedByPartPlotWithSourceRows();
    void nestedSkipsUnbalancedByPartPlot();
    void nestedSkipsMissingPartLabels();
    void biasLinearityBiasTableIncludesNAndSe();
};

namespace {

std::string format_cell(double value)
{
    std::ostringstream stream;
    stream.precision(15);
    stream << value;
    return stream.str();
}

}  // namespace

void GageRrOutputTest::contributionBarAndOperatorXbarR()
{
    const std::vector<double> measurements = {
        10.0, 10.2, 10.1, 11.0, 11.1, 10.9, 12.0, 12.1, 11.9,
        10.0, 10.1, 10.2, 11.0, 11.2, 11.1, 12.0, 12.2, 12.1};
    const std::vector<std::string> parts = {
        "P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3",
        "P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"};
    const std::vector<std::string> operators = {
        "A", "A", "A", "A", "A", "A", "A", "A", "A",
        "B", "B", "B", "B", "B", "B", "B", "B", "B"};
    const auto domain = datalab::domain::statistics::crossed_gage_rr(
        measurements, parts, operators, 5.0);

    datalab::domain::DataTable table;
    table.columns = {"Measurement", "Part", "Operator"};
    for (std::size_t index = 0; index < measurements.size(); ++index) {
        table.rows.push_back({format_cell(measurements[index]), parts[index], operators[index]});
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.gage_measurement_column = 0;
    configuration.msa.gage_part_column = 1;
    configuration.msa.gage_operator_column = 2;
    configuration.specifications.lower = 0.0;
    configuration.specifications.upper = 5.0;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::gage_rr(table, configuration);

    QVERIFY(page.facts.msa.has_value());
    QCOMPARE(page.facts.msa->ndc_available, domain.ndc_available);
    QCOMPARE(page.facts.msa->ndc.has_value(), domain.ndc_available);
    if (domain.ndc_available) {
        QCOMPARE(*page.facts.msa->ndc, domain.ndc);
    }
    QCOMPARE(page.facts.msa->negative_variance_truncated,
             domain.negative_variance_truncated);

    const auto find_plot = [&](const std::string& title) {
        return std::find_if(page.plots.cbegin(), page.plots.cend(),
                            [&](const datalab::domain::PlotSpec& plot) {
                                return plot.title == title;
                            });
    };
    const auto contribution = find_plot("方差分量 %Contribution");
    const auto run_chart = find_plot("Gage Run Chart");
    const auto xbar = find_plot("按零件 Xbar");
    const auto range = find_plot("按零件 R");
    QVERIFY(contribution != page.plots.cend());
    QVERIFY(run_chart != page.plots.cend());
    QVERIFY(xbar != page.plots.cend());
    QVERIFY(range != page.plots.cend());
    QCOMPARE(run_chart->source_rows.size(), measurements.size());
    QCOMPARE(run_chart->values.size(), measurements.size());
    QCOMPARE(run_chart->center.size(), measurements.size());
    QCOMPARE(static_cast<int>(contribution->kind),
             static_cast<int>(datalab::domain::PlotKind::pareto));
    QCOMPARE(contribution->categories, (std::vector<std::string>{
        "Repeatability", "Reproducibility", "Part-To-Part"}));
    QCOMPARE(contribution->category_values.size(), std::size_t{3});
    QVERIFY(std::none_of(contribution->categories.cbegin(),
                         contribution->categories.cend(),
                         [](const std::string& name) {
                             return name == "Total Gage R&R";
                         }));
    QVERIFY(!xbar->source_rows.empty());
    QVERIFY(!range->source_rows.empty());
    QCOMPARE(xbar->source_rows.size(), std::size_t{6});
    QCOMPARE(xbar->point_groups, (std::vector<std::string>{
        "P1", "P1", "P2", "P2", "P3", "P3"}));
    QCOMPARE(xbar->point_groups.size(), xbar->source_rows.size());

    const auto by_part = find_plot("按零件");
    const auto interaction = find_plot("操作者×零件交互");
    QVERIFY(by_part != page.plots.cend());
    QVERIFY(interaction != page.plots.cend());
    QCOMPARE(static_cast<int>(by_part->kind),
             static_cast<int>(datalab::domain::PlotKind::scatter));
    QCOMPARE(by_part->source_rows.size(), measurements.size());
    QCOMPARE(by_part->values.size(), measurements.size());
    QCOMPARE(interaction->series.size(), std::size_t{2});
    QCOMPARE(interaction->series.front().values.size(), std::size_t{3});
    QVERIFY(!interaction->source_rows.empty());
    QVERIFY(page.facts.msa.has_value());
    QCOMPARE(page.facts.msa->by_part_plot_available, true);
    QCOMPARE(page.facts.msa->interaction_plot_available, true);
    QCOMPARE(page.facts.msa->plot_point_count, measurements.size());
}

void GageRrOutputTest::studyVarParetoCrossed()
{
    // # source: formula_reference — Pareto uses domain %Study Var, not %Contribution.
    const std::vector<double> measurements = {
        10.0, 10.2, 10.1, 11.0, 11.1, 10.9, 12.0, 12.1, 11.9,
        10.0, 10.1, 10.2, 11.0, 11.2, 11.1, 12.0, 12.2, 12.1};
    const std::vector<std::string> parts = {
        "P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3",
        "P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"};
    const std::vector<std::string> operators = {
        "A", "A", "A", "A", "A", "A", "A", "A", "A",
        "B", "B", "B", "B", "B", "B", "B", "B", "B"};
    const auto domain = datalab::domain::statistics::crossed_gage_rr(
        measurements, parts, operators, 5.0);

    datalab::domain::DataTable table;
    table.columns = {"Measurement", "Part", "Operator"};
    for (std::size_t index = 0; index < measurements.size(); ++index) {
        table.rows.push_back({format_cell(measurements[index]), parts[index], operators[index]});
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.gage_measurement_column = 0;
    configuration.msa.gage_part_column = 1;
    configuration.msa.gage_operator_column = 2;
    configuration.specifications.lower = 0.0;
    configuration.specifications.upper = 5.0;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::gage_rr(table, configuration);

    const auto find_plot = [&](const std::string& title) {
        return std::find_if(page.plots.cbegin(), page.plots.cend(),
                            [&](const datalab::domain::PlotSpec& plot) {
                                return plot.title == title;
                            });
    };
    const auto contribution = find_plot("方差分量 %Contribution");
    const auto study_var = find_plot("方差分量 %Study Var");
    QVERIFY(contribution != page.plots.cend());
    QVERIFY(study_var != page.plots.cend());
    QCOMPARE(study_var->categories, contribution->categories);
    QCOMPARE(study_var->category_values.size(), std::size_t{3});
    for (std::size_t index = 0; index < study_var->categories.size(); ++index) {
        for (const auto& component : domain.variance_components) {
            if (component.source == study_var->categories[index]) {
                QCOMPARE(study_var->category_values[index],
                         component.percent_study_variation);
                QVERIFY(study_var->category_values[index]
                        != component.percent_contribution);
            }
        }
    }
    QVERIFY(std::none_of(study_var->categories.cbegin(),
                         study_var->categories.cend(),
                         [](const std::string& name) {
                             return name == "Total Gage R&R";
                         }));
}

void GageRrOutputTest::studyVarParetoNested()
{
    // # source: formula_reference — Nested Pareto uses domain %Study Var by source name.
    const std::vector<double> measurements = {
        10.1, 10.2, 10.0, 10.1, 11.0, 11.1, 11.2, 11.1, 12.0, 12.1, 12.2, 12.1};
    const std::vector<std::string> parts = {
        "P01", "P01", "P02", "P02", "P03", "P03", "P04", "P04", "P05", "P05", "P06", "P06"};
    const std::vector<std::string> operators = {
        "A", "A", "A", "A", "B", "B", "B", "B", "C", "C", "C", "C"};
    const auto domain = datalab::domain::statistics::nested_gage_rr(
        measurements, parts, operators, 5.0);

    datalab::domain::DataTable table;
    table.columns = {"Measurement", "Part", "Operator"};
    for (std::size_t index = 0; index < measurements.size(); ++index) {
        table.rows.push_back({format_cell(measurements[index]), parts[index], operators[index]});
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.nested_measurement_column = 0;
    configuration.msa.nested_part_column = 1;
    configuration.msa.nested_operator_column = 2;
    configuration.msa.gage_tolerance = 5.0;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::nested_gage_rr(table, configuration);

    const auto find_plot = [&](const std::string& title) {
        return std::find_if(page.plots.cbegin(), page.plots.cend(),
                            [&](const datalab::domain::PlotSpec& plot) {
                                return plot.title == title;
                            });
    };
    const auto contribution = find_plot("方差分量 %Contribution");
    const auto study_var = find_plot("方差分量 %Study Var");
    QVERIFY(contribution != page.plots.cend());
    QVERIFY(study_var != page.plots.cend());
    QCOMPARE(study_var->categories, contribution->categories);
    for (std::size_t index = 0; index < study_var->categories.size(); ++index) {
        for (const auto& component : domain.variance_components) {
            if (component.source == study_var->categories[index]) {
                QCOMPARE(study_var->category_values[index],
                         component.percent_study_variation);
                QVERIFY(study_var->category_values[index]
                        != component.percent_contribution);
            }
        }
    }
}

void GageRrOutputTest::toleranceParetoAppearsOnlyWhenToleranceAvailable()
{
    const std::vector<double> measurements = {
        10.0, 10.2, 10.1, 11.0, 11.1, 10.9, 12.0, 12.1, 11.9,
        10.0, 10.1, 10.2, 11.0, 11.2, 11.1, 12.0, 12.2, 12.1};
    const std::vector<std::string> parts = {
        "P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3",
        "P1", "P1", "P1", "P2", "P2", "P2", "P3", "P3", "P3"};
    const std::vector<std::string> operators = {
        "A", "A", "A", "A", "A", "A", "A", "A", "A",
        "B", "B", "B", "B", "B", "B", "B", "B", "B"};

    datalab::domain::DataTable table;
    table.columns = {"Measurement", "Part", "Operator"};
    for (std::size_t index = 0; index < measurements.size(); ++index) {
        table.rows.push_back({format_cell(measurements[index]), parts[index], operators[index]});
    }

    datalab::domain::AnalysisConfiguration with_tolerance;
    with_tolerance.msa.gage_measurement_column = 0;
    with_tolerance.msa.gage_part_column = 1;
    with_tolerance.msa.gage_operator_column = 2;
    with_tolerance.specifications.lower = 0.0;
    with_tolerance.specifications.upper = 5.0;
    const auto with_page = datalab::application::AnalysisService::gage_rr(table, with_tolerance);
    QVERIFY(std::any_of(with_page.plots.cbegin(), with_page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title == "方差分量 %Tolerance";
                        }));

    datalab::domain::AnalysisConfiguration without_tolerance = with_tolerance;
    without_tolerance.specifications.lower.reset();
    without_tolerance.specifications.upper.reset();
    const auto without_page =
        datalab::application::AnalysisService::gage_rr(table, without_tolerance);
    QVERIFY(std::none_of(without_page.plots.cbegin(), without_page.plots.cend(),
                         [](const datalab::domain::PlotSpec& plot) {
                             return plot.title == "方差分量 %Tolerance";
                         }));

    datalab::domain::AnalysisConfiguration nested_without_tolerance;
    nested_without_tolerance.msa.nested_measurement_column = 0;
    nested_without_tolerance.msa.nested_part_column = 1;
    nested_without_tolerance.msa.nested_operator_column = 2;
    const auto nested_page =
        datalab::application::AnalysisService::nested_gage_rr(table, nested_without_tolerance);
    const auto components = std::find_if(
        nested_page.tables.cbegin(), nested_page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Variance Components";
        });
    QVERIFY(components != nested_page.tables.cend());
    QVERIFY(std::all_of(components->rows.cbegin(), components->rows.cend(),
                        [](const std::vector<std::string>& row) {
                            return row.size() >= 7 && row.back() == "*";
                        }));
}

void GageRrOutputTest::skipsMissingLabelsOnByPart()
{
    datalab::domain::DataTable table;
    table.columns = {"Measurement", "Part", "Operator"};
    table.rows = {
        {"10.0", "P1", "A"}, {"10.2", "P1", "A"}, {"10.1", "P1", "A"},
        {"11.0", "*", "A"},
        {"11.0", "P2", "A"}, {"11.1", "P2", "A"}, {"10.9", "P2", "A"},
        {"12.0", "P3", "A"}, {"12.1", "P3", "A"}, {"11.9", "P3", "A"},
        {"10.0", "P1", "B"}, {"10.1", "P1", "B"}, {"10.2", "P1", "B"},
        {"11.0", "P2", "B"}, {"11.2", "P2", "B"}, {"11.1", "P2", "B"},
        {"12.0", "P3", "B"}, {"12.2", "P3", "B"}, {"12.1", "P3", "B"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.gage_measurement_column = 0;
    configuration.msa.gage_part_column = 1;
    configuration.msa.gage_operator_column = 2;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::gage_rr(table, configuration);
    const auto by_part = std::find_if(
        page.plots.cbegin(), page.plots.cend(),
        [](const datalab::domain::PlotSpec& plot) {
            return plot.title == "按零件";
        });
    QVERIFY(by_part != page.plots.cend());
    QCOMPARE(by_part->source_rows.size(), std::size_t{18});
    QCOMPARE(by_part->values.size(), by_part->source_rows.size());
    QVERIFY(std::none_of(by_part->source_rows.cbegin(), by_part->source_rows.cend(),
                         [](std::size_t row) { return row == 3; }));
    QCOMPARE(by_part->source_rows[0], std::size_t{0});
    QCOMPARE(by_part->source_rows[6], std::size_t{4});
}

void GageRrOutputTest::skipsUnbalancedByPartPlots()
{
    datalab::domain::DataTable table;
    table.columns = {"Measurement", "Part", "Operator"};
    table.rows = {
        {"10.0", "P1", "A"}, {"10.2", "P1", "A"},
        {"11.0", "P2", "A"}, {"11.1", "P2", "A"}, {"10.9", "P2", "A"},
        {"10.0", "P1", "B"}, {"10.1", "P1", "B"}, {"10.2", "P1", "B"},
        {"11.0", "P2", "B"}, {"11.2", "P2", "B"}, {"11.1", "P2", "B"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.gage_measurement_column = 0;
    configuration.msa.gage_part_column = 1;
    configuration.msa.gage_operator_column = 2;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::gage_rr(table, configuration);
    QVERIFY(std::none_of(
        page.plots.cbegin(), page.plots.cend(),
        [](const datalab::domain::PlotSpec& plot) {
            return plot.title == "按零件" || plot.title == "操作者×零件交互";
        }));
}

void GageRrOutputTest::nestedContributionBarAndOperatorXbarR()
{
    // # source: formula_reference — Pareto uses domain %Contribution by source name.
    const std::vector<double> measurements = {
        10.1, 10.2, 10.0, 10.1, 11.0, 11.1, 11.2, 11.1, 12.0, 12.1, 12.2, 12.1};
    const std::vector<std::string> parts = {
        "P01", "P01", "P02", "P02", "P03", "P03", "P04", "P04", "P05", "P05", "P06", "P06"};
    const std::vector<std::string> operators = {
        "A", "A", "A", "A", "B", "B", "B", "B", "C", "C", "C", "C"};
    const auto domain = datalab::domain::statistics::nested_gage_rr(
        measurements, parts, operators, 5.0);

    datalab::domain::DataTable table;
    table.columns = {"Measurement", "Part", "Operator"};
    for (std::size_t index = 0; index < measurements.size(); ++index) {
        table.rows.push_back({format_cell(measurements[index]), parts[index], operators[index]});
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.nested_measurement_column = 0;
    configuration.msa.nested_part_column = 1;
    configuration.msa.nested_operator_column = 2;
    configuration.msa.gage_tolerance = 5.0;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::nested_gage_rr(table, configuration);

    QVERIFY(page.facts.msa.has_value());
    QCOMPARE(page.facts.msa->ndc_available, domain.ndc_available);
    QCOMPARE(page.facts.msa->ndc.has_value(), domain.ndc_available);
    if (domain.ndc_available) {
        QCOMPARE(*page.facts.msa->ndc, domain.ndc);
    }
    QCOMPARE(page.facts.msa->negative_variance_truncated,
             domain.negative_variance_truncated);

    const auto find_plot = [&](const std::string& title) {
        return std::find_if(page.plots.cbegin(), page.plots.cend(),
                            [&](const datalab::domain::PlotSpec& plot) {
                                return plot.title == title;
                            });
    };
    const auto contribution = find_plot("方差分量 %Contribution");
    const auto xbar = find_plot("按操作者 Xbar");
    const auto range = find_plot("按操作者 R");
    QVERIFY(contribution != page.plots.cend());
    QVERIFY(xbar != page.plots.cend());
    QVERIFY(range != page.plots.cend());
    QCOMPARE(static_cast<int>(contribution->kind),
             static_cast<int>(datalab::domain::PlotKind::pareto));
    QCOMPARE(contribution->categories, (std::vector<std::string>{
        "Repeatability", "Reproducibility", "Part-To-Part"}));
    QCOMPARE(contribution->category_values.size(), std::size_t{3});
    for (std::size_t index = 0; index < contribution->categories.size(); ++index) {
        for (const auto& component : domain.variance_components) {
            if (component.source == contribution->categories[index]) {
                QCOMPARE(contribution->category_values[index],
                         component.percent_contribution);
            }
        }
    }
    QVERIFY(std::none_of(contribution->categories.cbegin(),
                         contribution->categories.cend(),
                         [](const std::string& name) {
                             return name == "Total Gage R&R";
                         }));
    QVERIFY(!xbar->source_rows.empty());
    QVERIFY(!range->source_rows.empty());
    QCOMPARE(xbar->source_rows.size(), std::size_t{6});
    QCOMPARE(xbar->point_groups.size(), xbar->source_rows.size());
    const auto by_part = find_plot("按零件");
    QVERIFY(by_part != page.plots.cend());
    QCOMPARE(static_cast<int>(by_part->kind),
             static_cast<int>(datalab::domain::PlotKind::scatter));
    QCOMPARE(by_part->source_rows.size(), measurements.size());
    QCOMPARE(by_part->values.size(), measurements.size());
    QVERIFY(page.facts.msa.has_value());
    QCOMPARE(page.facts.msa->by_part_plot_available, true);
    QCOMPARE(page.facts.msa->interaction_plot_available, false);
    QCOMPARE(page.facts.msa->plot_point_count, measurements.size());
    QVERIFY(std::none_of(page.plots.cbegin(), page.plots.cend(),
                         [](const datalab::domain::PlotSpec& plot) {
                             return plot.title == "操作者×零件交互";
                         }));
}

void GageRrOutputTest::nestedByPartPlotWithSourceRows()
{
    const std::vector<double> measurements = {
        10.1, 10.2, 10.0, 10.1, 11.0, 11.1, 11.2, 11.1, 12.0, 12.1, 12.2, 12.1};
    const std::vector<std::string> parts = {
        "P01", "P01", "P02", "P02", "P03", "P03", "P04", "P04", "P05", "P05", "P06", "P06"};
    const std::vector<std::string> operators = {
        "A", "A", "A", "A", "B", "B", "B", "B", "C", "C", "C", "C"};
    datalab::domain::DataTable table;
    table.columns = {"Measurement", "Part", "Operator"};
    for (std::size_t index = 0; index < measurements.size(); ++index) {
        table.rows.push_back({format_cell(measurements[index]), parts[index], operators[index]});
    }
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.nested_measurement_column = 0;
    configuration.msa.nested_part_column = 1;
    configuration.msa.nested_operator_column = 2;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::nested_gage_rr(table, configuration);
    const auto by_part = std::find_if(
        page.plots.cbegin(), page.plots.cend(),
        [](const datalab::domain::PlotSpec& plot) {
            return plot.title == "按零件";
        });
    QVERIFY(by_part != page.plots.cend());
    QCOMPARE(by_part->source_rows.size(), measurements.size());
    QCOMPARE(by_part->values.size(), by_part->source_rows.size());
    QVERIFY(!by_part->series.empty());
    QCOMPARE(by_part->series.front().label, std::string("零件均值"));
    QVERIFY(page.facts.msa.has_value());
    QCOMPARE(page.facts.msa->by_part_plot_available, true);
    QCOMPARE(page.facts.msa->interaction_plot_available, false);
    QCOMPARE(page.facts.msa->plot_point_count, measurements.size());
    QVERIFY(std::none_of(page.plots.cbegin(), page.plots.cend(),
                         [](const datalab::domain::PlotSpec& plot) {
                             return plot.title == "操作者×零件交互";
                         }));
}

void GageRrOutputTest::nestedSkipsUnbalancedByPartPlot()
{
    datalab::domain::DataTable table;
    table.columns = {"Measurement", "Part", "Operator"};
    table.rows = {
        {"10.1", "P01", "A"}, {"10.2", "P01", "A"},
        {"10.0", "P02", "A"}, {"10.1", "P02", "A"}, {"10.3", "P02", "A"},
        {"11.0", "P03", "B"}, {"11.1", "P03", "B"},
        {"11.2", "P04", "B"}, {"11.1", "P04", "B"},
        {"12.0", "P05", "C"}, {"12.1", "P05", "C"},
        {"12.2", "P06", "C"}, {"12.1", "P06", "C"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.nested_measurement_column = 0;
    configuration.msa.nested_part_column = 1;
    configuration.msa.nested_operator_column = 2;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::nested_gage_rr(table, configuration);
    QVERIFY(page.facts.msa.has_value());
    QCOMPARE(page.facts.msa->by_part_plot_available, false);
    QCOMPARE(page.facts.msa->plot_point_count, std::size_t{0});
    QVERIFY(std::none_of(
        page.plots.cbegin(), page.plots.cend(),
        [](const datalab::domain::PlotSpec& plot) {
            return plot.title == "按零件";
        }));
}

void GageRrOutputTest::nestedSkipsMissingPartLabels()
{
    datalab::domain::DataTable table;
    table.columns = {"Measurement", "Part", "Operator"};
    table.rows = {
        {"10.1", "P01", "A"}, {"10.2", "P01", "A"},
        {"10.0", "P02", "A"}, {"10.1", "P02", "A"},
        {"11.0", "*", "B"},
        {"11.0", "P03", "B"}, {"11.1", "P03", "B"},
        {"11.2", "P04", "B"}, {"11.1", "P04", "B"},
        {"12.0", "P05", "C"}, {"12.1", "P05", "C"},
        {"12.2", "P06", "C"}, {"12.1", "P06", "C"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.nested_measurement_column = 0;
    configuration.msa.nested_part_column = 1;
    configuration.msa.nested_operator_column = 2;
    const datalab::domain::OutputPage page =
        datalab::application::AnalysisService::nested_gage_rr(table, configuration);
    QVERIFY(page.facts.msa.has_value());
    QVERIFY(std::any_of(page.diagnostics.cbegin(), page.diagnostics.cend(),
                        [](const datalab::domain::DiagnosticMessage& message) {
                            return message.code == "missing_values";
                        }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title == "方差分量 %Contribution";
                        }));
    const auto by_part = std::find_if(
        page.plots.cbegin(), page.plots.cend(),
        [](const datalab::domain::PlotSpec& plot) {
            return plot.title == "按零件";
        });
    QVERIFY(by_part != page.plots.cend());
    QCOMPARE(by_part->source_rows.size(), std::size_t{12});
    QCOMPARE(by_part->values.size(), by_part->source_rows.size());
    QVERIFY(std::none_of(by_part->source_rows.cbegin(), by_part->source_rows.cend(),
                         [](std::size_t row) { return row == 4; }));
}

void GageRrOutputTest::biasLinearityBiasTableIncludesNAndSe()
{
    datalab::domain::DataTable table;
    table.columns = {"Reference", "Measurement"};
    table.rows = {
        {"1.0", "1.1"}, {"1.0", "1.0"}, {"2.0", "2.2"},
        {"2.0", "2.1"}, {"3.0", "3.4"}, {"3.0", "3.3"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.msa.gage_measurement_column = 1;
    configuration.msa.reference_column = 0;
    configuration.msa.mode = "bias_linearity";
    configuration.msa.process_variation = 6.0;
    const auto page = datalab::application::AnalysisService::msa_type1(table, configuration);
    const auto bias_table = std::find_if(
        page.tables.cbegin(), page.tables.cend(),
        [](const datalab::domain::StatisticTable& table_out) {
            return table_out.title == "Gage Bias";
        });
    QVERIFY(bias_table != page.tables.cend());
    QCOMPARE(bias_table->headers,
             (std::vector<std::string>{"Reference", "N", "Bias", "SE Bias", "%Bias", "t", "P"}));
    QVERIFY(bias_table->rows.size() >= std::size_t{2});
    QCOMPARE(bias_table->rows.front()[1], std::string{"2"});
    QCOMPARE(bias_table->rows.back()[0], std::string{"Average"});
    QCOMPARE(bias_table->rows.back()[1], std::string{"6"});
    QVERIFY(bias_table->rows.back()[3] != "*");
}

QTEST_APPLESS_MAIN(GageRrOutputTest)

#include "gage_rr_output_test.moc"
