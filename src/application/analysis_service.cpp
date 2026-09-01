#include "application/analysis_service.h"
#include "application/chart_pages.h"
#include "application/column_assembly.h"
#include "application/computation_trace_attach.h"
#include "application/doe_pages.h"
#include "application/output_builder.h"

#include "domain/column_extract.h"
#include "domain/graph_assembly.h"
#include "domain/quality_diagnostics.h"
#include "domain/statistics/control_charts.h"
#include "domain/statistics/special_cause_rule_catalog.h"
#include "domain/statistics/descriptive_statistics.h"
#include "domain/statistics/process_capability.h"
#include "domain/statistics/attribute_capability.h"
#include "domain/statistics/quality_visuals.h"
#include "domain/statistics/normal_probability.h"
#include "domain/statistics/normality_test.h"
#include "domain/statistics/correlation.h"
#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/inference_extensions.h"
#include "domain/statistics/regression.h"
#include "domain/statistics/box_cox.h"
#include "domain/statistics/gage_rr.h"
#include "domain/statistics/expanded_gage_rr.h"
#include "domain/statistics/msa_type1.h"
#include "domain/statistics/reliability.h"
#include "domain/statistics/censoring_contract.h"
#include "domain/statistics/aalen_johansen_cif.h"
#include "domain/statistics/gray_test.h"
#include "domain/statistics/fine_gray.h"
#include "domain/statistics/cox_regression.h"
#include "domain/statistics/t_power.h"
#include "domain/statistics/autocorrelation.h"
#include "domain/statistics/nonparametric_tests.h"
#include "domain/statistics/time_series.h"
#include "domain/statistics/arima.h"
#include "domain/statistics/two_factor_anova.h"
#include "domain/statistics/logistic_regression.h"
#include "domain/statistics/variance_tests.h"
#include "domain/statistics/time_series_decomposition.h"
#include "domain/statistics/doe_factorial.h"
#include "domain/statistics/rsm_analysis.h"
#include "domain/statistics/nested_gage_rr.h"
#include "domain/statistics/attribute_agreement.h"
#include "domain/statistics/seasonal_forecasting.h"
#include "domain/statistics/pca.h"
#include "domain/statistics/kmeans.h"
#include "domain/statistics/cart_tree.h"
#include "domain/statistics/random_forest.h"
#include "domain/statistics/weibayes.h"
#include "domain/statistics/taguchi_orthogonal.h"
#include "domain/statistics/distribution_calculator.h"
#include "domain/statistics/taguchi_analyze.h"
#include "domain/statistics/mixture_design.h"
#include "domain/statistics/nhpp_repairable.h"
#include "domain/statistics/reliability_test_plan.h"
#include "domain/statistics/mixture_analyze.h"
#include "domain/statistics/glm_two_way.h"
#include "domain/statistics/glm_three_factor.h"
#include "domain/statistics/analyze_variability.h"
#include "domain/statistics/factor_analysis.h"
#include "domain/statistics/binary_response_doe.h"
#include "domain/statistics/cluster_variables.h"
#include "domain/statistics/life_data_regression.h"
#include "domain/statistics/expanded_gage_unbalanced.h"
#include "domain/statistics/split_plot_analyze.h"
#include "domain/statistics/mixture_process_variable.h"
#include "domain/statistics/manova_one_way.h"
#include "domain/statistics/general_manova.h"
#include "domain/statistics/mixed_effects_reml.h"
#include "domain/statistics/binary_doe_probit.h"
#include "domain/statistics/life_data_lognormal.h"
#include "domain/statistics/simple_correspondence.h"
#include "domain/statistics/multiple_correspondence.h"
#include "domain/statistics/nonlinear_regression.h"
#include "domain/statistics/split_plot_design.h"
#include "domain/statistics/adf_test.h"
#include "domain/statistics/poisson_regression.h"
#include "domain/statistics/isolation_forest.h"
#include "domain/statistics/bootstrap_mean.h"
#include "domain/statistics/bootstrap_two_sample.h"
#include "domain/statistics/probit_reliability.h"
#include "domain/statistics/hierarchical_cluster.h"
#include "domain/statistics/ordinal_logistic.h"
#include "domain/statistics/nominal_logistic.h"
#include "domain/statistics/nonparametric_capability.h"
#include "domain/statistics/accelerated_life.h"
#include "domain/statistics/discriminant.h"
#include "domain/statistics/ccf.h"
#include "domain/statistics/correlation.h"
#include "domain/statistics/stepwise_regression.h"
#include "domain/statistics/best_subsets_regression.h"
#include "domain/statistics/batch_capability.h"
#include "domain/statistics/km_interval.h"
#include "domain/statistics/plackett_burman.h"
#include "domain/statistics/response_surface_design.h"
#include "domain/statistics/analysis_rules.h"
#include "domain/statistics/distribution_identification.h"
#include "domain/statistics/response_optimization.h"
#include "domain/statistics/multi_vari.h"
#include "domain/statistics/tolerance_intervals.h"
#include "domain/statistics/proportion_test.h"
#include "domain/statistics/poisson_rate.h"
#include "domain/statistics/equivalence_test.h"
#include "domain/statistics/grubbs_test.h"
#include "domain/statistics/quality_extensions.h"
#include "domain/statistics/multivariate_control.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <map>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <set>
#include <utility>

namespace datalab::application {
namespace {

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::DiagnosticMessage;
using datalab::domain::ExtractedNumericColumn;
using datalab::domain::OutputPage;
using datalab::domain::PlotKind;
using datalab::domain::PlotLineStyle;
using datalab::domain::PlotPointStyle;
using datalab::domain::PlotSeries;
using datalab::domain::PlotSeriesRole;
using datalab::domain::PlotSpec;
using datalab::domain::RowId;
using datalab::domain::StatisticTable;
using datalab::domain::SpecificationLimits;
using datalab::domain::column_label;
using datalab::domain::extract_numeric_column;
using datalab::domain::extract_text_column;
using datalab::domain::is_missing_cell;

struct SummarizedCountPair {
    bool ok = false;
    std::string error;
    std::size_t events = 0;
    std::size_t trials = 0;
    std::size_t row_count = 0;
    std::size_t missing = 0;
};

SummarizedCountPair sum_event_trial_columns(
    const DataTable& table,
    const ExtractedNumericColumn& events_column,
    const ExtractedNumericColumn& trials_column,
    const AnalysisConfiguration& configuration)
{
    SummarizedCountPair out;
    const auto aligned = align_complete_rows_with_source({events_column, trials_column});
    const std::size_t eligible = table.rows.size() >= configuration.excluded_rows.size()
        ? table.rows.size() - configuration.excluded_rows.size() : table.rows.size();
    out.missing = eligible > aligned.values.size()
        ? eligible - aligned.values.size() : 0;
    if (aligned.values.empty()) {
        out.error = "没有可用于比例检验的 complete-case 行。";
        return out;
    }
    for (const auto& row : aligned.values) {
        std::vector<std::size_t> counts;
        if (!append_nonnegative_counts(row, counts) || counts.size() != 2
            || counts[0] > counts[1] || counts[1] == 0) {
            out.error = "事件数和试验数必须为非负整数，且事件数不能超过试验数。";
            return out;
        }
        out.events += counts[0];
        out.trials += counts[1];
    }
    out.row_count = aligned.values.size();
    out.ok = true;
    return out;
}

OutputPage finalize_page(OutputPage page);

void add_zero_residual_reference(PlotSpec& plot)
{
    if (plot.x_values.empty()) {
        return;
    }
    const auto [x_min, x_max] = std::minmax_element(
        plot.x_values.cbegin(), plot.x_values.cend());
    PlotSeries zero;
    zero.label = "残差 = 0";
    zero.x_values = {*x_min, *x_max};
    zero.values = {0.0, 0.0};
    zero.line_width = 1.0;
    plot.series.push_back(std::move(zero));
}

template<typename Component>
void append_gage_contribution_pareto(
    OutputPage& page, const std::vector<Component>& components)
{
    PlotSpec contribution;
    contribution.kind = PlotKind::pareto;
    contribution.title = "方差分量 %Contribution";
    contribution.x_axis_title = "来源";
    contribution.y_axis_title = "%Contribution";
    const std::array<const char*, 3> bar_sources = {
        "Repeatability", "Reproducibility", "Part-To-Part"};
    double contribution_total = 0.0;
    for (const char* source : bar_sources) {
        for (const auto& component : components) {
            if (component.source == source) {
                contribution.categories.push_back(component.source);
                contribution.category_values.push_back(component.percent_contribution);
                contribution_total += component.percent_contribution;
                contribution.cumulative_percent.push_back(contribution_total);
                break;
            }
        }
    }
    if (!contribution.category_values.empty()) {
        page.plots.push_back(std::move(contribution));
    }
}

template<typename Component>
void append_gage_study_var_pareto(
    OutputPage& page, const std::vector<Component>& components)
{
    PlotSpec study_var;
    study_var.kind = PlotKind::pareto;
    study_var.title = "方差分量 %Study Var";
    study_var.x_axis_title = "来源";
    study_var.y_axis_title = "%Study Var";
    const std::array<const char*, 3> bar_sources = {
        "Repeatability", "Reproducibility", "Part-To-Part"};
    double study_var_total = 0.0;
    for (const char* source : bar_sources) {
        for (const auto& component : components) {
            if (component.source == source) {
                study_var.categories.push_back(component.source);
                study_var.category_values.push_back(component.percent_study_variation);
                study_var_total += component.percent_study_variation;
                study_var.cumulative_percent.push_back(study_var_total);
                break;
            }
        }
    }
    if (!study_var.category_values.empty()) {
        page.plots.push_back(std::move(study_var));
    }
}

template<typename Component>
void append_gage_tolerance_pareto(
    OutputPage& page, const std::vector<Component>& components)
{
    PlotSpec tolerance_plot;
    tolerance_plot.kind = PlotKind::pareto;
    tolerance_plot.title = "方差分量 %Tolerance";
    tolerance_plot.x_axis_title = "来源";
    tolerance_plot.y_axis_title = "%Tolerance";
    const std::array<const char*, 3> bar_sources = {
        "Repeatability", "Reproducibility", "Part-To-Part"};
    double cumulative = 0.0;
    for (const char* source : bar_sources) {
        for (const auto& component : components) {
            if (component.source == source && component.percent_tolerance_available) {
                tolerance_plot.categories.push_back(component.source);
                tolerance_plot.category_values.push_back(component.percent_tolerance);
                cumulative += component.percent_tolerance;
                tolerance_plot.cumulative_percent.push_back(cumulative);
                break;
            }
        }
    }
    if (!tolerance_plot.category_values.empty()) {
        page.plots.push_back(std::move(tolerance_plot));
    }
}

void append_part_xbar_range_plots(
    OutputPage& page,
    const std::vector<double>& values,
    const std::vector<std::string>& part_values,
    const std::vector<std::string>& operator_values,
    const std::vector<std::size_t>& source_rows,
    std::size_t replicate_count,
    bool design_balanced)
{
    if (replicate_count < 2) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "insufficient_replicates",
            "重复次数小于 2，无法绘制按零件 R 图。"});
        return;
    }
    if (!design_balanced || values.empty()) {
        return;
    }
    std::map<std::string, std::map<std::string, std::vector<double>>> cells;
    std::map<std::string, std::map<std::string, std::size_t>> cell_rows;
    for (std::size_t index = 0; index < values.size(); ++index) {
        const std::string& oper = operator_values[index];
        const std::string& part = part_values[index];
        if (cells[part][oper].empty()) {
            cell_rows[part][oper] = source_rows[index];
        }
        cells[part][oper].push_back(values[index]);
    }
    std::vector<std::vector<double>> subgroups;
    std::vector<std::size_t> subgroup_rows;
    std::vector<std::string> phase_labels;
    bool equal_size = true;
    for (const auto& part : cells) {
        for (const auto& oper : part.second) {
            if (oper.second.size() != replicate_count) {
                equal_size = false;
                break;
            }
            subgroups.push_back(oper.second);
            subgroup_rows.push_back(cell_rows[part.first][oper.first]);
            phase_labels.push_back(part.first);
        }
        if (!equal_size) {
            break;
        }
    }
    if (!equal_size || subgroups.empty()) {
        return;
    }
    auto chart = datalab::domain::statistics::ControlCharts::xbar_range_dual(subgroups);
    chart.primary.phase_labels = phase_labels;
    chart.secondary.phase_labels = phase_labels;
    PlotSpec xbar_plot = control_plot(
        "按零件 Xbar", "子组均值", chart.primary, subgroup_rows);
    xbar_plot.point_groups = phase_labels;
    xbar_plot.point_labels = phase_labels;
    PlotSpec range_plot = control_plot(
        "按零件 R", "子组极差", chart.secondary, subgroup_rows);
    range_plot.point_groups = phase_labels;
    range_plot.point_labels = phase_labels;
    page.plots.push_back(std::move(xbar_plot));
    page.plots.push_back(std::move(range_plot));
    page.diagnostics.insert(page.diagnostics.end(),
                            chart.diagnostics.cbegin(), chart.diagnostics.cend());
}

void append_gage_run_chart(
    OutputPage& page,
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows)
{
    if (values.empty()) {
        return;
    }
    const double center = std::accumulate(values.begin(), values.end(), 0.0)
        / static_cast<double>(values.size());
    PlotSpec plot;
    plot.kind = PlotKind::control;
    plot.title = "Gage Run Chart";
    plot.x_axis_title = "观测序号";
    plot.y_axis_title = "测量值";
    plot.center_label = "Mean";
    plot.values = values;
    plot.center.assign(values.size(), center);
    plot.source_rows = source_rows;
    page.plots.push_back(std::move(plot));
}

void append_operator_xbar_range_plots(
    OutputPage& page,
    const std::vector<double>& values,
    const std::vector<std::string>& part_values,
    const std::vector<std::string>& operator_values,
    const std::vector<std::size_t>& source_rows,
    std::size_t replicate_count,
    bool design_balanced)
{
    if (replicate_count < 2) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "insufficient_replicates",
            "重复次数小于 2，无法绘制按操作者 R 图。"});
        return;
    }
    if (!design_balanced || values.empty()) {
        return;
    }
    std::map<std::string, std::map<std::string, std::vector<double>>> cells;
    std::map<std::string, std::map<std::string, std::size_t>> cell_rows;
    for (std::size_t index = 0; index < values.size(); ++index) {
        const std::string& oper = operator_values[index];
        const std::string& part = part_values[index];
        if (cells[oper][part].empty()) {
            cell_rows[oper][part] = source_rows[index];
        }
        cells[oper][part].push_back(values[index]);
    }
    std::vector<std::vector<double>> subgroups;
    std::vector<std::size_t> subgroup_rows;
    std::vector<std::string> phase_labels;
    bool equal_size = true;
    for (const auto& oper : cells) {
        for (const auto& part : oper.second) {
            if (part.second.size() != replicate_count) {
                equal_size = false;
                break;
            }
            subgroups.push_back(part.second);
            subgroup_rows.push_back(cell_rows[oper.first][part.first]);
            phase_labels.push_back(oper.first);
        }
        if (!equal_size) {
            break;
        }
    }
    if (!equal_size || subgroups.empty()) {
        return;
    }
    auto chart = datalab::domain::statistics::ControlCharts::xbar_range_dual(subgroups);
    chart.primary.phase_labels = phase_labels;
    chart.secondary.phase_labels = phase_labels;
    PlotSpec xbar_plot = control_plot(
        "按操作者 Xbar", "子组均值", chart.primary, subgroup_rows);
    xbar_plot.point_groups = phase_labels;
    xbar_plot.point_labels = phase_labels;
    PlotSpec range_plot = control_plot(
        "按操作者 R", "子组极差", chart.secondary, subgroup_rows);
    range_plot.point_groups = phase_labels;
    range_plot.point_labels = phase_labels;
    page.plots.push_back(std::move(xbar_plot));
    page.plots.push_back(std::move(range_plot));
    page.diagnostics.insert(page.diagnostics.end(),
                            chart.diagnostics.cbegin(), chart.diagnostics.cend());
}

void append_gage_by_part_plot(
    OutputPage& page,
    const std::vector<double>& values,
    const std::vector<std::string>& part_values,
    const std::vector<std::size_t>& source_rows)
{
    if (values.empty()) {
        return;
    }
    std::vector<std::string> part_order;
    std::map<std::string, std::vector<double>> part_measurements;
    std::map<std::string, std::vector<std::size_t>> part_rows;
    std::map<std::string, std::vector<std::string>> part_labels;
    for (std::size_t index = 0; index < values.size(); ++index) {
        const std::string& part = part_values[index];
        if (part_measurements[part].empty()) {
            part_order.push_back(part);
        }
        part_measurements[part].push_back(values[index]);
        part_rows[part].push_back(source_rows[index]);
        part_labels[part].push_back(part);
    }
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "按零件";
    plot.x_axis_title = "零件";
    plot.y_axis_title = "测量值";
    plot.show_legend = true;
    PlotSeries means;
    means.label = "零件均值";
    means.show_points = true;
    means.style.point_style = PlotPointStyle::cross;
    means.style.line_style = PlotLineStyle::solid;
    for (std::size_t part_index = 0; part_index < part_order.size(); ++part_index) {
        const std::string& part = part_order[part_index];
        const std::vector<double>& measurements = part_measurements[part];
        double sum = 0.0;
        for (std::size_t inner = 0; inner < measurements.size(); ++inner) {
            plot.x_values.push_back(static_cast<double>(part_index + 1));
            plot.values.push_back(measurements[inner]);
            plot.source_rows.push_back(part_rows[part][inner]);
            plot.point_labels.push_back(part_labels[part][inner]);
            sum += measurements[inner];
        }
        means.x_values.push_back(static_cast<double>(part_index + 1));
        means.values.push_back(sum / static_cast<double>(measurements.size()));
    }
    plot.series.push_back(std::move(means));
    page.plots.push_back(std::move(plot));
}

void append_gage_interaction_plot(
    OutputPage& page,
    const std::vector<double>& values,
    const std::vector<std::string>& part_values,
    const std::vector<std::string>& operator_values,
    const std::vector<std::size_t>& source_rows)
{
    if (values.empty()) {
        return;
    }
    std::vector<std::string> part_order;
    std::vector<std::string> operator_order;
    std::map<std::string, std::map<std::string, std::vector<double>>> cells;
    std::map<std::string, std::map<std::string, std::size_t>> cell_rows;
    for (std::size_t index = 0; index < values.size(); ++index) {
        const std::string& part = part_values[index];
        const std::string& oper = operator_values[index];
        if (cells[oper][part].empty()) {
            cell_rows[oper][part] = source_rows[index];
        }
        if (std::find(part_order.cbegin(), part_order.cend(), part) == part_order.cend()) {
            part_order.push_back(part);
        }
        if (std::find(operator_order.cbegin(), operator_order.cend(), oper)
            == operator_order.cend()) {
            operator_order.push_back(oper);
        }
        cells[oper][part].push_back(values[index]);
    }
    if (part_order.size() < 2 || operator_order.size() < 2) {
        return;
    }
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "操作者×零件交互";
    plot.x_axis_title = "零件";
    plot.y_axis_title = "单元格均值";
    plot.show_legend = true;
    for (const std::string& oper : operator_order) {
        PlotSeries series;
        series.label = oper;
        series.show_points = true;
        series.style.point_style = PlotPointStyle::circle;
        series.style.line_style = PlotLineStyle::solid;
        series.role = PlotSeriesRole::interaction_first;
        for (std::size_t part_index = 0; part_index < part_order.size(); ++part_index) {
            const std::string& part = part_order[part_index];
            const auto found = cells[oper].find(part);
            if (found == cells[oper].end() || found->second.empty()) {
                continue;
            }
            double sum = 0.0;
            for (const double value : found->second) {
                sum += value;
            }
            series.x_values.push_back(static_cast<double>(part_index + 1));
            series.values.push_back(sum / static_cast<double>(found->second.size()));
            plot.source_rows.push_back(cell_rows[oper][part]);
        }
        if (!series.values.empty()) {
            plot.series.push_back(std::move(series));
        }
    }
    if (!plot.series.empty()) {
        page.plots.push_back(std::move(plot));
    }
}

void append_parametric_reliability_plots(
    OutputPage& page,
    const std::vector<double>& times,
    const std::vector<bool>& events,
    const std::vector<std::size_t>& source_rows,
    const std::function<double(double)>& cdf,
    bool identifiable,
    bool converged,
    const std::string& model_label)
{
    if (!identifiable || !converged || times.empty()) {
        return;
    }
    double t_min = times.front();
    double t_max = times.front();
    for (const double time : times) {
        t_min = std::min(t_min, time);
        t_max = std::max(t_max, time);
    }
    if (!(t_max > t_min)) {
        t_max = t_min + 1.0;
    }
    PlotSpec survival;
    survival.kind = PlotKind::scatter;
    survival.title = model_label + " 生存曲线";
    survival.x_axis_title = "时间";
    survival.y_axis_title = "S(t)";
    PlotSeries fitted;
    fitted.label = "拟合 S(t)";
    fitted.style.point_style = PlotPointStyle::none;
    fitted.style.line_style = PlotLineStyle::solid;
    const std::size_t grid = 40;
    for (std::size_t index = 0; index <= grid; ++index) {
        const double time = t_min + (t_max - t_min)
            * static_cast<double>(index) / static_cast<double>(grid);
        fitted.x_values.push_back(time);
        fitted.values.push_back(1.0 - cdf(time));
    }
    survival.series.push_back(std::move(fitted));
    page.plots.push_back(std::move(survival));

    PlotSpec probability;
    probability.kind = PlotKind::scatter;
    probability.title = model_label + " 概率图";
    probability.x_axis_title = "时间";
    probability.y_axis_title = "F(t)";
    for (std::size_t index = 0; index < times.size(); ++index) {
        if (!events[index]) {
            continue;
        }
        probability.x_values.push_back(times[index]);
        probability.values.push_back(cdf(times[index]));
        probability.source_rows.push_back(source_rows[index]);
    }
    if (!probability.values.empty()) {
        page.plots.push_back(std::move(probability));
    }
}

struct AttributeImport {
    std::vector<datalab::domain::statistics::AttributeSample> samples;
    std::size_t missing_count = 0;
    std::string error;
};

AttributeImport import_attribute_samples(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    AttributeImport imported;
    if (!configuration.selection.defect_count_column.has_value()
        && configuration.variable_columns.empty()) {
        imported.error = "请选择不合格品/缺陷数列。";
        return imported;
    }
    const std::size_t defect_column = configuration.selection.defect_count_column.value_or(
        first_variable(configuration));
    if (!configuration.selection.inspected_count_column.has_value()
        && !configuration.inspected_constant.has_value()) {
        imported.error = "请指定检验数/单位数列或常数。";
        return imported;
    }
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row_index)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        const auto& row = table.rows[row_index];
        if (defect_column >= row.size()) {
            imported.missing_count += 1;
            continue;
        }
        const std::string& defect_cell = row[defect_column];
        std::string inspected_cell;
        if (configuration.selection.inspected_count_column.has_value()) {
            const std::size_t inspected_column = *configuration.selection.inspected_count_column;
            if (inspected_column >= row.size()) {
                imported.missing_count += 1;
                continue;
            }
            inspected_cell = row[inspected_column];
        }
        if (is_missing_cell(defect_cell)
            || (configuration.selection.inspected_count_column.has_value()
                && is_missing_cell(inspected_cell))) {
            imported.missing_count += 1;
            continue;
        }
        const auto defectives = parse_numeric_cell(defect_cell);
        std::optional<double> inspected = configuration.inspected_constant.has_value()
            ? std::optional<double>(static_cast<double>(*configuration.inspected_constant))
            : parse_numeric_cell(inspected_cell);
        if (!defectives.has_value() || !inspected.has_value()) {
            imported.missing_count += 1;
            continue;
        }
        imported.samples.push_back({*defectives, *inspected, row_index});
    }
    return imported;
}

std::string format_confidence_interval(
    const datalab::domain::statistics::ConfidenceInterval& interval)
{
    if (!interval.lower.has_value() && !interval.upper.has_value()) {
        return "*";
    }
    return "[" + format_optional(interval.lower) + ", " + format_optional(interval.upper) + "]";
}

PlotSpec cumulative_attribute_plot(
    const std::string& title,
    const std::string& y_axis,
    const datalab::domain::statistics::AttributeCapabilityResult& result,
    double center,
    const datalab::domain::statistics::ConfidenceInterval& interval)
{
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = title;
    plot.x_axis_title = "样本";
    plot.y_axis_title = y_axis;
    plot.show_legend = true;
    for (std::size_t index = 0; index < result.cumulative_values.size(); ++index) {
        plot.x_values.push_back(static_cast<double>(index + 1));
        plot.values.push_back(result.cumulative_values[index]);
    }
    plot.source_rows = result.source_rows;
    if (!plot.x_values.empty()) {
        const double x0 = plot.x_values.front();
        const double x1 = plot.x_values.back();
        PlotSeries middle;
        middle.label = "中心";
        middle.x_values = {x0, x1};
        middle.values = {center, center};
        plot.series.push_back(std::move(middle));
        if (interval.lower.has_value()) {
            PlotSeries lower;
            lower.label = "下限";
            lower.style.line_style = PlotLineStyle::dash;
            lower.x_values = {x0, x1};
            lower.values = {*interval.lower, *interval.lower};
            plot.series.push_back(std::move(lower));
        }
        if (interval.upper.has_value()) {
            PlotSeries upper;
            upper.label = "上限";
            upper.style.line_style = PlotLineStyle::dash;
            upper.x_values = {x0, x1};
            upper.values = {*interval.upper, *interval.upper};
            plot.series.push_back(std::move(upper));
        }
    }
    return plot;
}

OutputPage build_attribute_capability_page(
    const AnalysisConfiguration& configuration,
    const AttributeImport& imported,
    datalab::domain::statistics::AttributeCapabilityResult result,
    bool binomial)
{
    OutputPage page;
    page.id = new_id(binomial ? "binomial_cap" : "poisson_cap");
    page.title = binomial ? "二项过程能力" : "泊松过程能力";
    page.method_name = binomial ? "Binomial Capability" : "Poisson Capability";
    page.configuration = configuration;
    page.configuration.capability_method = binomial ? "binomial" : "poisson";
    page.parameter_summary = binomial
        ? "分布 = 二项    子组数 = " + std::to_string(result.sample_count)
        : "分布 = 泊松    子组数 = " + std::to_string(result.sample_count);
    page.diagnostics = result.diagnostics;
    if (imported.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "跳过 " + std::to_string(imported.missing_count)
                + " 个缺失或非法单元格（计入 N*）。"});
    }
    StatisticTable process;
    process.title = "过程数据";
    process.headers = {"指标", "数值"};
    process.rows = {
        {"子组数", std::to_string(result.sample_count)},
        {"N*", std::to_string(imported.missing_count)},
        {binomial ? "不合格品合计" : "缺陷合计", format_number(result.defectives_total)},
        {binomial ? "检验数合计" : "单位数合计", format_number(result.inspected_total)}};
    if (result.average_p.has_value()) {
        process.rows.push_back({"Average P", format_number(*result.average_p)});
    }
    if (result.mean_dpu.has_value()) {
        process.rows.push_back({"Mean DPU", format_number(*result.mean_dpu)});
    }
    page.tables.push_back(std::move(process));
    StatisticTable capability;
    capability.title = binomial ? "二项能力" : "泊松能力";
    capability.headers = {"指标", "数值", "95% CI"};
    if (binomial) {
        capability.rows = {
            {"%Defective", format_optional(result.percent_defective),
             format_confidence_interval(result.percent_defective_interval)},
            {"PPM Defective", format_optional(result.ppm_defective),
             format_confidence_interval(result.ppm_interval)},
            {"Process Z", format_optional(result.process_z),
             format_confidence_interval(result.process_z_interval)}};
    } else {
        capability.rows = {
            {"Mean DPU", format_optional(result.mean_dpu),
             format_confidence_interval(result.mean_dpu_interval)},
            {"Mean Defective", format_optional(result.mean_defective),
             format_confidence_interval(result.mean_defective_interval)},
            {"Min DPU", format_optional(result.minimum_dpu), "*"},
            {"Max DPU", format_optional(result.maximum_dpu), "*"}};
    }
    if (result.target.has_value()) {
        capability.rows.push_back({
            binomial ? "目标不合格品率" : "目标 DPU",
            format_number(*result.target), "*"});
    }
    page.tables.push_back(std::move(capability));
    if (binomial && result.percent_defective.has_value()) {
        page.plots.push_back(cumulative_attribute_plot(
            "累计 %Defective", "%Defective", result, *result.percent_defective,
            result.percent_defective_interval));
    } else if (!binomial && result.mean_dpu.has_value()) {
        page.plots.push_back(cumulative_attribute_plot(
            "累计 DPU", "DPU", result, *result.mean_dpu, result.mean_dpu_interval));
    }
    const auto tests =
        datalab::domain::statistics::special_cause_selection_from_configuration(
            configuration.control.enabled_special_cause_tests,
            configuration.control.special_cause_rule_policy);
    if (binomial && !result.defectives.empty()) {
        const auto chart = datalab::domain::statistics::ControlCharts::p_chart(
            result.defectives, result.inspected, tests);
        page.plots.push_back(control_plot("P 图", "不合格品率", chart, result.source_rows));
        page.diagnostics.insert(page.diagnostics.end(),
                                chart.diagnostics.cbegin(), chart.diagnostics.cend());
    } else if (!binomial && !result.defectives.empty()) {
        const auto chart = datalab::domain::statistics::ControlCharts::u_chart(
            result.defectives, result.inspected, tests);
        page.plots.push_back(control_plot("U 图", "单位缺陷数", chart, result.source_rows));
        page.diagnostics.insert(page.diagnostics.end(),
                                chart.diagnostics.cbegin(), chart.diagnostics.cend());
    }
    domain::CapabilityFacts facts;
    facts.method = result.method;
    facts.assumption_status = result.assumption_status;
    facts.average_p = result.average_p;
    facts.percent_defective = result.percent_defective;
    facts.ppm_defective = result.ppm_defective;
    facts.process_z = result.process_z;
    facts.z_bench = result.process_z;
    facts.mean_dpu = result.mean_dpu;
    page.facts.capability = std::move(facts);
    return finalize_page(std::move(page));
}

PlotSpec make_fitted_line_plot(
    const datalab::domain::statistics::RegressionResult& result,
    const std::string& predictor_label,
    const std::string& response_label,
    const std::vector<std::size_t>& source_rows)
{
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "拟合线图";
    plot.x_axis_title = predictor_label;
    plot.y_axis_title = response_label;
    plot.show_legend = true;
    PlotSeries actual;
    actual.role = PlotSeriesRole::actual;
    actual.label = "观测";
    actual.show_points = true;
    actual.x_values = result.simple_predictor_values;
    for (const auto& observation : result.observations) {
        actual.values.push_back(observation.response);
    }
    plot.x_values = actual.x_values;
    plot.values = actual.values;
    plot.source_rows = source_rows;
    const auto bands = datalab::domain::statistics::fitted_line_bands(result);
    PlotSeries fitted;
    fitted.role = PlotSeriesRole::fitted;
    fitted.label = "拟合线";
    PlotSeries ci;
    ci.role = PlotSeriesRole::confidence_band;
    const int percent = static_cast<int>(std::lround(result.confidence_level * 100.0));
    ci.label = std::to_string(percent) + "% CI";
    PlotSeries pi;
    pi.role = PlotSeriesRole::confidence_band;
    pi.label = std::to_string(percent) + "% PI";
    for (const auto& point : bands) {
        fitted.x_values.push_back(point.x);
        fitted.values.push_back(point.fitted);
        ci.x_values.push_back(point.x);
        ci.lower.push_back(point.ci_lower);
        ci.upper.push_back(point.ci_upper);
        pi.x_values.push_back(point.x);
        pi.lower.push_back(point.pi_lower);
        pi.upper.push_back(point.pi_upper);
    }
    plot.series = {std::move(actual), std::move(fitted), std::move(ci), std::move(pi)};
    return plot;
}

// ---- capability 家族 / logistic 的页面装配辅助（阶段 2.3 薄壳化）----

// 组装配过程能力页正文（Process Data / Performance PPM / Potential Within /
// Overall 四表 + 直方图），capability() 只负责校验、Within σ 与附加图/指标。
datalab::domain::OutputPage build_capability_content(
    const datalab::domain::AnalysisConfiguration& configuration,
    const datalab::domain::ExtractedNumericColumn& extracted,
    const datalab::domain::statistics::ProcessCapabilityResult& capability_result,
    int subgroup_size,
    const std::string& within_method)
{
    const std::string method = capability_result.capability_method.empty()
        ? "normal" : capability_result.capability_method;
    const bool is_johnson = method == "johnson";
    const bool is_nonnormal = method == "non_normal";
    const bool is_transformed = is_johnson || is_nonnormal;
    const bool is_between_within = method == "between_within";

    datalab::domain::OutputPage page;
    page.id = new_id("cap");
    page.title = "正态过程能力分析";
    page.method_name = "Normal Capability Analysis";
    page.configuration = configuration;
    page.diagnostics = capability_result.diagnostics;
    page.parameter_summary =
        "变量: " + extracted.name
        + "    子组大小 = " + std::to_string(subgroup_size)
        + "    Within σ: " + within_method
        + "    方法: " + method;
    if (is_johnson) {
        page.title = "Johnson 变换过程能力";
        page.method_name = "Johnson Capability Analysis";
    } else if (is_nonnormal) {
        page.title = "非正态过程能力";
        page.method_name = "Nonnormal Capability Analysis";
    } else if (is_between_within) {
        page.title = "组间/组内过程能力";
        page.method_name = "Between/Within Capability Analysis";
    }

    std::optional<double> normality_p;
    StatisticTable process;
    process.title = "Process Data";
    process.headers = {"项目", "数值"};
    process.rows = {
        {"LSL", format_optional(configuration.specifications.lower)},
        {"Target", format_optional(configuration.specifications.target)},
        {"USL", format_optional(configuration.specifications.upper)},
        {"Sample Mean", format_number(capability_result.mean)},
        {"Sample N", std::to_string(capability_result.sample_size)},
        {"Missing N*", std::to_string(capability_result.evidence.missing_count)},
        {"规格模式", capability_result.specification_mode.empty()
            ? "*" : capability_result.specification_mode}};
    if (is_nonnormal) {
        process.rows.push_back({"Distribution", capability_result.nonnormal_distribution});
    }
    if (is_between_within) {
        process.rows.push_back(
            {"StDev (Within)",
             format_optional(capability_result.subgroup_within_standard_deviation)});
        process.rows.push_back(
            {"Within σ 来源", capability_result.within_sigma_method.empty()
                ? within_method : capability_result.within_sigma_method});
        process.rows.push_back(
            {"StDev (Between)",
             format_optional(capability_result.between_standard_deviation)});
        process.rows.push_back(
            {"Between σ 来源", capability_result.between_sigma_method.empty()
                ? "*" : capability_result.between_sigma_method});
        process.rows.push_back(
            {"StDev (Between/Within)",
             format_optional(capability_result.between_within_standard_deviation)});
        process.rows.push_back(
            {"Between/Within σ 来源",
             capability_result.between_within_sigma_method.empty()
                 ? "*" : capability_result.between_within_sigma_method});
    } else if (!is_transformed) {
        process.rows.push_back(
            {"StDev (Within)", format_number(capability_result.within_standard_deviation)});
        process.rows.push_back(
            {"Within σ 来源", capability_result.within_sigma_method.empty()
                ? within_method : capability_result.within_sigma_method});
    }
    process.rows.push_back(
        {"StDev (Overall)", format_number(capability_result.overall_standard_deviation)});
    process.rows.push_back(
        {"Overall σ 方法", capability_result.overall_sigma_method});
    if (!is_transformed) {
        const auto normality = datalab::domain::statistics::normality_test(extracted.values);
        normality_p = normality.p_value;
        process.rows.push_back(
            {"Anderson-Darling A²",
             normality.anderson_darling.has_value()
                 ? format_number(*normality.anderson_darling) : "*"});
        process.rows.push_back(
            {"Anderson-Darling A²*",
             normality.adjusted_anderson_darling.has_value()
                 ? format_number(*normality.adjusted_anderson_darling) : "*"});
        process.rows.push_back(
            {"AD P-Value",
             normality.p_value.has_value() ? format_number(*normality.p_value) : "*"});
        std::string ad_decision = "无法计算";
        if (normality.decision == "reject") {
            ad_decision = "在 alpha 下拒绝正态假设";
        } else if (normality.decision == "fail_to_reject") {
            ad_decision = "在 alpha 下未拒绝正态假设";
        }
        process.rows.push_back({"AD 判定", ad_decision});
    }
    process.rows.push_back(
        {"假设状态", capability_result.evidence.assumption_status});
    page.tables.push_back(process);

    if (is_johnson && capability_result.transform_p_value.has_value()) {
        StatisticTable transform;
        transform.title = "Johnson 变换";
        transform.headers = {"项目", "数值"};
        transform.rows = {
            {"Selected Family", capability_result.johnson_family},
            {"P-Value",
             format_optional(capability_result.transform_p_value)},
            {"Anderson-Darling",
             format_optional(capability_result.transform_anderson_darling)}};
        page.tables.push_back(transform);
    }

    if (is_nonnormal && capability_result.fitted_shape.has_value()) {
        StatisticTable distribution;
        distribution.title = "分布参数";
        distribution.headers = {"参数", "数值"};
        if (capability_result.nonnormal_distribution == "lognormal") {
            distribution.rows = {
                {"Location", format_optional(capability_result.fitted_scale)},
                {"Scale", format_optional(capability_result.fitted_shape)}};
        } else {
            distribution.rows = {
                {"Shape", format_optional(capability_result.fitted_shape)},
                {"Scale", format_optional(capability_result.fitted_scale)}};
        }
        page.tables.push_back(distribution);
    }

    StatisticTable ppm;
    ppm.title = is_transformed ? "Performance (PPM)" : "Performance (PPM)";
    if (is_transformed) {
        ppm.headers = {"", "观测", "期望 Overall"};
        ppm.rows = {
            {"低于 LSL",
             format_optional(capability_result.observed_ppm_below, 4),
             format_optional(capability_result.expected_ppm_overall_below, 4)},
            {"高于 USL",
             format_optional(capability_result.observed_ppm_above, 4),
             format_optional(capability_result.expected_ppm_overall_above, 4)},
            {"合计",
             format_optional(capability_result.observed_ppm_total, 4),
             format_optional(capability_result.expected_ppm_overall_total, 4)}};
    } else {
        ppm.headers = {"", "观测", "期望 Within", "期望 Overall"};
        ppm.rows = {
            {"低于 LSL",
             format_optional(capability_result.observed_ppm_below, 4),
             format_optional(capability_result.expected_ppm_within_below, 4),
             format_optional(capability_result.expected_ppm_overall_below, 4)},
            {"高于 USL",
             format_optional(capability_result.observed_ppm_above, 4),
             format_optional(capability_result.expected_ppm_within_above, 4),
             format_optional(capability_result.expected_ppm_overall_above, 4)},
            {"合计",
             format_optional(capability_result.observed_ppm_total, 4),
             format_optional(capability_result.expected_ppm_within_total, 4),
             format_optional(capability_result.expected_ppm_overall_total, 4)}};
    }
    page.tables.push_back(ppm);

    const auto index_row = [](const std::string& name,
                              const std::optional<double>& estimate,
                              const std::optional<double>& lower,
                              const std::optional<double>& upper) {
        return std::vector<std::string>{
            name,
            format_optional(estimate),
            format_optional(lower),
            format_optional(upper)};
    };

    if (!is_transformed) {
        StatisticTable within;
        within.title = is_between_within ? "Between/Within Capability"
                                         : "Potential (Within) Capability";
        within.headers = {"指标", "估计", "下限", "上限"};
        within.rows = {
            index_row("Cp", capability_result.cp, capability_result.cp_lower,
                      capability_result.cp_upper),
            index_row("CPL", capability_result.cpl, capability_result.cpl_lower,
                      capability_result.cpl_upper),
            index_row("CPU", capability_result.cpu, capability_result.cpu_lower,
                      capability_result.cpu_upper),
            index_row("Cpk", capability_result.cpk, capability_result.cpk_lower,
                      capability_result.cpk_upper),
            index_row("Cpm", capability_result.cpm, std::nullopt, std::nullopt),
            index_row("Z.Bench", capability_result.z_bench, std::nullopt, std::nullopt)};
        page.tables.push_back(within);
    }

    const bool johnson_spec_limit_blocked =
        is_johnson
        && capability_result.evidence.not_computed_reason == "johnson_spec_outside_support";
    if (!johnson_spec_limit_blocked) {
        StatisticTable overall;
        overall.title = "Overall Capability";
        overall.headers = {"指标", "估计", "下限", "上限"};
        overall.rows = {
            index_row("Pp", capability_result.pp, capability_result.pp_lower,
                      capability_result.pp_upper),
            index_row("PPL", capability_result.ppl, capability_result.ppl_lower,
                      capability_result.ppl_upper),
            index_row("PPU", capability_result.ppu, capability_result.ppu_lower,
                      capability_result.ppu_upper),
            index_row("Ppk", capability_result.ppk, capability_result.ppk_lower,
                      capability_result.ppk_upper),
            index_row("Z.LSL", capability_result.z_lsl, std::nullopt, std::nullopt),
            index_row("Z.USL", capability_result.z_usl, std::nullopt, std::nullopt)};
        page.tables.push_back(overall);
    }

    const auto bins = datalab::domain::statistics::histogram(extracted.values, 0);
    PlotSpec hist;
    hist.kind = PlotKind::histogram;
    hist.title = "过程能力直方图";
    hist.x_axis_title = extracted.name;
    hist.y_axis_title = "频数";
    hist.histogram_edges = bins.edges;
    hist.histogram_counts = bins.counts;
    hist.values = extracted.values;
    hist.source_rows = extracted.source_rows;
    hist.lsl = configuration.specifications.lower;
    hist.usl = configuration.specifications.upper;
    hist.target = configuration.specifications.target;
    hist.process_mean = capability_result.mean;
    const bool between_within = is_between_within
        && capability_result.between_within_standard_deviation.has_value();
    hist.overall_sigma = capability_result.overall_standard_deviation;
    if (!is_transformed) {
        hist.within_sigma = between_within
            ? *capability_result.between_within_standard_deviation
            : capability_result.within_standard_deviation;
        PlotSeries within_curve;
        within_curve.label = between_within ? "Between/Within" : "Within";
        within_curve.style.color = "#455a64";
        within_curve.style.line_style = PlotLineStyle::dash;
        within_curve.style.point_style = PlotPointStyle::none;
        within_curve.style.line_width = 1.2;
        hist.series.push_back(within_curve);
    }
    PlotSeries overall_curve;
    overall_curve.label = "Overall";
    overall_curve.style.color = "#c62828";
    overall_curve.style.line_style = PlotLineStyle::solid;
    overall_curve.style.point_style = PlotPointStyle::none;
    overall_curve.style.line_width = 1.4;
    hist.series.push_back(overall_curve);
    hist.show_legend = true;
    page.plots.push_back(hist);

    if (is_johnson && !capability_result.transformed_values.empty()) {
        const auto probability = datalab::domain::statistics::normal_probability_plot(
            capability_result.transformed_values, extracted.source_rows);
        PlotSpec transformed_probability;
        transformed_probability.kind = PlotKind::probability;
        transformed_probability.title = "变换后正态概率图";
        transformed_probability.x_axis_title = "理论标准正态分位数";
        transformed_probability.y_axis_title = "变换后测量";
        transformed_probability.values = probability.ordered_values;
        transformed_probability.x_values = probability.theoretical_quantiles;
        transformed_probability.source_rows = probability.source_rows;
        page.plots.push_back(std::move(transformed_probability));
    }

    page.facts.capability = domain::CapabilityFacts{
        capability_result.cpk,
        capability_result.ppk,
        capability_result.cpm,
        capability_result.z_bench,
        capability_result.evidence.assumption_status,
        capability_result.specification_mode,
        method,
        capability_result.johnson_family};
    if (page.facts.capability.has_value()) {
        page.facts.capability->normality_p_value = normality_p;
        page.facts.capability->transform_p_value = capability_result.transform_p_value;
        page.facts.capability->transform_anderson_darling =
            capability_result.transform_anderson_darling;
        page.facts.capability->nonnormal_distribution =
            capability_result.nonnormal_distribution;
        page.facts.capability->fitted_shape = capability_result.fitted_shape;
        page.facts.capability->fitted_scale = capability_result.fitted_scale;
        page.facts.capability->cp = capability_result.cp;
        page.facts.capability->pp = capability_result.pp;
        page.facts.capability->cpk_lower = capability_result.cpk_lower;
        page.facts.capability->cpk_upper = capability_result.cpk_upper;
        page.facts.capability->ppk_lower = capability_result.ppk_lower;
        page.facts.capability->ppk_upper = capability_result.ppk_upper;
        page.facts.capability->capability_ci_method = capability_result.capability_ci_method;
        page.facts.capability->evidence_type = "formula_reference";
        page.facts.capability->stability_screen_status =
            capability_result.stability_screen_status;
        page.facts.capability->stability_out_of_control_count =
            capability_result.stability_out_of_control_count;
        page.facts.capability->bimodality_screen_status =
            capability_result.bimodality_screen_status;
        page.facts.capability->bimodality_peak_count =
            capability_result.bimodality_peak_count;
        page.facts.capability->hartigan_dip_status =
            capability_result.hartigan_dip_status;
        page.facts.capability->hartigan_dip_statistic =
            capability_result.hartigan_dip_statistic;
        page.facts.capability->hartigan_dip_p_value =
            capability_result.hartigan_dip_p_value;
        page.facts.capability->mixture_status = capability_result.mixture_status;
        page.facts.capability->mixture_k_selected = capability_result.mixture_k_selected;
        page.facts.capability->mixture_k_max = capability_result.mixture_k_max;
        page.facts.capability->mixture_weight1 = capability_result.mixture_weight1;
        page.facts.capability->mixture_mean1 = capability_result.mixture_mean1;
        page.facts.capability->mixture_mean2 = capability_result.mixture_mean2;
        page.facts.capability->mixture_sd1 = capability_result.mixture_sd1;
        page.facts.capability->mixture_sd2 = capability_result.mixture_sd2;
        page.facts.capability->mixture_delta_bic = capability_result.mixture_delta_bic;
        page.facts.capability->mixture_algorithm_id =
            capability_result.mixture_algorithm_id;
        page.facts.capability->mixture_evidence_type =
            capability_result.mixture_evidence_type;
        page.facts.capability->mixture_components.clear();
        for (const auto& c : capability_result.mixture_components) {
            domain::CapabilityFacts::MixtureComponentFacts item;
            item.weight = c.weight;
            item.mean = c.mean;
            item.sd = c.sd;
            page.facts.capability->mixture_components.push_back(item);
        }
        // No productized stability+normality verification workflow yet: never
        // open pass/fail from a bare assumption_status string.
        page.facts.capability->pass_fail_judgment_allowed = false;
        if (is_johnson) {
            // Phase 6 CAP-NN-3: gated until golden/tail/review — research preview only.
            page.facts.capability->research_preview = true;
            page.facts.capability->gate_status = "gated_research";
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::warning,
                "johnson_capability_gated",
                "Johnson 能力为研究/预览结果：可显示拟合与变换尺度指数，"
                "但不得输出过程合格判定（门禁：缺 golden/尾部/人工解释验收）。"});
        } else if (is_nonnormal) {
            page.facts.capability->research_preview = false;
            page.facts.capability->gate_status = "open_with_limits";
        } else {
            page.facts.capability->research_preview = false;
            if (capability_result.stability_screen_status == "signals") {
                page.facts.capability->gate_status = "stability_screen_signals";
            } else if (capability_result.mixture_status == "preferred_2comp"
                       || capability_result.mixture_status == "preferred_kcomp") {
                page.facts.capability->gate_status =
                    capability_result.mixture_status == "preferred_kcomp"
                    ? "mixture_preferred_kcomp"
                    : "mixture_preferred_2comp";
            } else if (capability_result.hartigan_dip_status == "evidence_against") {
                page.facts.capability->gate_status = "hartigan_dip_evidence_against";
            } else if (capability_result.bimodality_screen_status == "suspected") {
                page.facts.capability->gate_status = "bimodality_suspected";
            } else {
                page.facts.capability->gate_status = "stability_unverified";
            }
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::warning,
                "capability_pass_fail_blocked_by_stability_prerequisite",
                "正态能力未满足稳定性/正态性验收前置：禁止过程合格判定"
                "（pass_fail_judgment_allowed=false）。"});
        }
    }
    domain::apply_evidence(page.method_metadata, capability_result.evidence);
    page.method_metadata.estimation_method = within_method;
    return page;
}

// 正态概率图（含参考线），capability_sixpack 使用。
PlotSpec probability_plot_spec(
    const datalab::domain::statistics::NormalProbabilityResult& probability,
    const std::string& variable_name)
{
    PlotSpec plot;
    plot.kind = PlotKind::probability;
    plot.title = "正态概率图";
    plot.x_axis_title = "理论标准正态分位数";
    plot.y_axis_title = variable_name;
    plot.values = probability.ordered_values;
    plot.x_values = probability.theoretical_quantiles;
    plot.source_rows = probability.source_rows;
    plot.center.resize(probability.ordered_values.size());
    if (probability.theoretical_quantiles.size() >= 2) {
        const double x0 = probability.theoretical_quantiles.front();
        const double x1 = probability.theoretical_quantiles.back();
        const double y0 = probability.ordered_values.front();
        const double y1 = probability.ordered_values.back();
        const double slope = (x1 == x0) ? 0.0 : (y1 - y0) / (x1 - x0);
        for (std::size_t index = 0; index < plot.center.size(); ++index) {
            plot.center[index] =
                y0 + slope * (probability.theoretical_quantiles[index] - x0);
        }
    }
    return plot;
}

// "最后 25 个子组 / 最近 25 个观测"图，capability_sixpack 使用。
PlotSpec last_points_plot(
    const datalab::domain::ExtractedNumericColumn& extracted,
    const datalab::domain::statistics::ControlChartResult& primary,
    int subgroup_size)
{
    const bool by_subgroups = subgroup_size > 1;
    PlotSpec plot;
    plot.kind = PlotKind::control;
    plot.title = by_subgroups ? "最后 25 个子组" : "最近 25 个观测";
    plot.x_axis_title = "样本";
    plot.y_axis_title = "测量值";
    if (by_subgroups) {
        const std::size_t group_count = extracted.values.size() / subgroup_size;
        const std::size_t first_group = group_count > 25 ? group_count - 25 : 0;
        for (std::size_t group = first_group; group < group_count; ++group) {
            for (std::size_t offset = 0; offset < subgroup_size; ++offset) {
                const std::size_t source = group * subgroup_size + offset;
                plot.values.push_back(extracted.values[source]);
                plot.x_values.push_back(static_cast<double>(group - first_group + 1));
                plot.source_rows.push_back(extracted.source_rows[source]);
            }
        }
    } else {
        plot = control_plot("最近 25 个观测", "测量值", primary, extracted.source_rows);
        const std::size_t first = plot.values.size() > 25 ? plot.values.size() - 25 : 0;
        plot.values.erase(plot.values.begin(),
                          plot.values.begin() + static_cast<std::ptrdiff_t>(first));
        plot.source_rows.erase(plot.source_rows.begin(),
                               plot.source_rows.begin() + static_cast<std::ptrdiff_t>(first));
    }
    return plot;
}

// Logistic 回归 complete-case 导入（事件解析 + 预测变量行），logistic_regression 使用。
struct LogisticImport {
    std::vector<int> response;
    std::vector<std::vector<double>> predictors;
    std::vector<std::size_t> source_rows;
};

LogisticImport logistic_import_rows(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    LogisticImport result;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row_index)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        const auto& row = table.rows[row_index];
        const std::size_t response_column = *configuration.inference.logistic_response_column;
        if (response_column >= row.size()) {
            continue;
        }
        int event = -1;
        if (const auto numeric = parse_numeric_cell(row[response_column]);
            numeric.has_value() && (*numeric == 0.0 || *numeric == 1.0)) {
            event = static_cast<int>(*numeric);
        } else if (!configuration.inference.logistic_event_level.empty()) {
            event = row[response_column] == configuration.inference.logistic_event_level ? 1 : 0;
        }
        std::vector<double> predictor_row;
        bool complete = event >= 0;
        for (const std::size_t column : configuration.inference.logistic_predictor_columns) {
            if (column >= row.size()) {
                complete = false;
                break;
            }
            const auto value = parse_numeric_cell(row[column]);
            if (!value.has_value()) {
                complete = false;
                break;
            }
            predictor_row.push_back(*value);
        }
        if (complete) {
            result.response.push_back(event);
            result.predictors.push_back(std::move(predictor_row));
            result.source_rows.push_back(row_index);
        }
    }
    return result;
}

PlotSpec interval_plot_spec(
    const std::string& title,
    const std::string& x_title,
    const std::string& y_title,
    const std::vector<std::string>& categories,
    const std::vector<double>& means,
    const std::vector<double>& lower,
    const std::vector<double>& upper,
    const std::vector<std::size_t>& counts,
    const std::vector<std::size_t>& source_rows)
{
    PlotSpec plot;
    plot.kind = PlotKind::interval;
    plot.title = title;
    plot.x_axis_title = x_title;
    plot.y_axis_title = y_title;
    plot.categories = categories;
    plot.values = means;
    plot.interval_lower = lower;
    plot.interval_upper = upper;
    plot.interval_counts = counts;
    plot.source_rows = source_rows;
    return plot;
}

struct GroupedDistributionPlots {
    PlotSpec box;
    PlotSpec individuals;
    PlotSeries outliers;
    std::size_t point_count = 0;
};

GroupedDistributionPlots make_grouped_distribution_plots(
    const std::string& box_title,
    const std::string& individuals_title)
{
    GroupedDistributionPlots plots;
    plots.box.kind = PlotKind::boxplot;
    plots.box.title = box_title;
    plots.box.x_axis_title = "分组";
    plots.box.y_axis_title = "观测";
    plots.individuals.kind = PlotKind::scatter;
    plots.individuals.title = individuals_title;
    plots.individuals.x_axis_title = "分组";
    plots.individuals.y_axis_title = "观测";
    plots.outliers.role = PlotSeriesRole::generic;
    plots.outliers.label = "异常点";
    plots.outliers.style.color = "#c62828";
    plots.outliers.style.point_style = PlotPointStyle::circle;
    plots.outliers.show_points = true;
    return plots;
}

void append_group_to_distribution_plots(
    GroupedDistributionPlots& plots,
    const std::string& label,
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows)
{
    if (values.empty()) {
        return;
    }
    const auto summary = datalab::domain::statistics::box_plot_summary(values);
    const std::size_t group_index = plots.box.box_labels.size();
    plots.box.box_labels.push_back(label);
    plots.box.box_min.push_back(summary.whisker_low);
    plots.box.box_q1.push_back(summary.first_quartile);
    plots.box.box_median.push_back(summary.median);
    plots.box.box_q3.push_back(summary.third_quartile);
    plots.box.box_max.push_back(summary.whisker_high);
    for (const double outlier : summary.outliers) {
        plots.outliers.x_values.push_back(static_cast<double>(group_index));
        plots.outliers.values.push_back(outlier);
    }
    for (std::size_t index = 0; index < values.size(); ++index) {
        plots.individuals.x_values.push_back(static_cast<double>(group_index));
        plots.individuals.values.push_back(values[index]);
        if (index < source_rows.size()) {
            plots.individuals.source_rows.push_back(source_rows[index]);
        }
        plots.individuals.point_groups.push_back(label);
        ++plots.point_count;
    }
}

void push_distribution_plots(OutputPage& page, GroupedDistributionPlots& plots)
{
    if (!plots.outliers.values.empty()) {
        plots.box.series.push_back(std::move(plots.outliers));
    }
    if (!plots.box.box_labels.empty()) {
        page.plots.push_back(std::move(plots.box));
    }
    if (!plots.individuals.values.empty()) {
        page.plots.push_back(std::move(plots.individuals));
    }
}

std::pair<double, double> mean_t_interval(
    const double mean,
    const double standard_error,
    const double degrees_of_freedom,
    const double confidence_level)
{
    const double critical = datalab::domain::statistics::student_t_quantile(
        0.5 + confidence_level / 2.0, degrees_of_freedom);
    return {mean - critical * standard_error, mean + critical * standard_error};
}

OutputPage finalize_page(OutputPage page)
{
    page.method_metadata.algorithm = page.method_name;
    page.method_metadata.parameters = page.parameter_summary;
    page.method_metadata.source_rows.clear();
    for (const auto& plot : page.plots) {
        page.method_metadata.source_rows.insert(
            page.method_metadata.source_rows.end(),
            plot.source_rows.begin(), plot.source_rows.end());
    }
    std::sort(page.method_metadata.source_rows.begin(),
              page.method_metadata.source_rows.end());
    page.method_metadata.source_rows.erase(
        std::unique(page.method_metadata.source_rows.begin(),
                    page.method_metadata.source_rows.end()),
        page.method_metadata.source_rows.end());
    page.method_metadata.diagnostic_codes.clear();
    for (const auto& diagnostic : page.diagnostics) {
        if (std::find(page.method_metadata.diagnostic_codes.begin(),
                      page.method_metadata.diagnostic_codes.end(),
                      diagnostic.code) == page.method_metadata.diagnostic_codes.end()) {
            page.method_metadata.diagnostic_codes.push_back(diagnostic.code);
        }
    }
    std::optional<double> cpk;
    std::optional<double> ppk;
    std::optional<std::size_t> out_of_control;
    domain::MsaFacts msa;
    domain::ReliabilityFacts reliability;
    domain::ForecastFacts forecast;
    domain::PowerFacts power;
    domain::ParetoFacts pareto;
    domain::DoeFacts doe;
    bool has_msa_fact = false;
    bool has_reliability_fact = false;
    bool has_forecast_fact = false;
    bool has_power_fact = false;
    bool has_pareto_fact = false;
    bool has_doe_fact = false;

    for (const PlotSpec& plot : page.plots) {
        if (plot.sigma_z > 0.0) {
            if (!page.facts.spc.has_value()) {
                page.facts.spc = domain::SpcFacts{};
            }
            page.facts.spc->sigma_z = plot.sigma_z;
        }
    }

    for (const StatisticTable& table : page.tables) {
        for (const std::vector<std::string>& row : table.rows) {
            for (std::size_t index = 0; index + 1 < row.size(); ++index) {
                const std::optional<double> value = parse_numeric_cell(row[index + 1]);
                if (row[index] == "Cpk" && value.has_value()) {
                    cpk = value;
                } else if (row[index] == "Ppk" && value.has_value()) {
                    ppk = value;
                } else if ((row[index] == "超限点数"
                            || row[index] == "Out of Control")
                           && value.has_value() && *value >= 0.0) {
                    out_of_control = static_cast<std::size_t>(*value);
                }
                if (row[index] == "Slope" && value.has_value()) {
                    msa.slope = value;
                    has_msa_fact = true;
                } else if (row[index] == "Bias Low" && value.has_value()) {
                    msa.bias_low = value;
                    has_msa_fact = true;
                } else if (row[index] == "Bias High" && value.has_value()) {
                    msa.bias_high = value;
                    has_msa_fact = true;
                } else if (row[index] == "P" && value.has_value()) {
                    msa.p_value = value;
                    has_msa_fact = true;
                } else if (row[index] == "Cgk" && value.has_value()) {
                    msa.cgk = value;
                    has_msa_fact = true;
                } else if (row[index] == "%Tolerance" && value.has_value()) {
                    msa.tolerance_percent = value;
                    has_msa_fact = true;
                } else if (row[index] == "Shape" && value.has_value()) {
                    reliability.shape = value;
                    has_reliability_fact = true;
                } else if (row[index] == "Censored" && value.has_value()
                           && *value >= 0.0) {
                    reliability.censored_count = static_cast<std::size_t>(*value);
                    has_reliability_fact = true;
                } else if (row[index] == "MAPE (%)" && value.has_value()) {
                    forecast.mape = value;
                    has_forecast_fact = true;
                } else if (row[index] == "MASE" && value.has_value()) {
                    forecast.mase = value;
                    has_forecast_fact = true;
                } else if (row[index] == "Power" && value.has_value()) {
                    power.power = value;
                    has_power_fact = true;
                } else if (row[index] == "Effect Size" && value.has_value()) {
                    power.effect_size = value;
                    has_power_fact = true;
                }
            }
            if (table.title.find("ANOVA") != std::string::npos && row.size() >= 2) {
                const std::optional<double> p_value = parse_numeric_cell(row.back());
                if (p_value.has_value()) {
                    doe.has_p_value = true;
                    has_doe_fact = true;
                    if (*p_value < 1.0 - page.configuration.inference.confidence_level
                        && !row.front().empty()) {
                        doe.significant_terms.push_back(row.front());
                    }
                }
            }
        }
        if (table.title.find("Pareto") != std::string::npos
            && !table.rows.empty() && table.rows.front().size() >= 4) {
            const std::vector<std::string>& row = table.rows.front();
            pareto.largest_category = row[0];
            pareto.largest_count = parse_numeric_cell(row[1]);
            pareto.largest_percent = parse_numeric_cell(row[2]);
            pareto.top_categories_percent = parse_numeric_cell(
                table.rows[std::min<std::size_t>(1, table.rows.size() - 1)][3]);
            has_pareto_fact = true;
        }
    }

    if (cpk.has_value() || ppk.has_value()) {
        if (!page.facts.capability.has_value()) {
            page.facts.capability = domain::CapabilityFacts{};
        }
        page.facts.capability->cpk = cpk;
        page.facts.capability->ppk = ppk;
    }
    if (out_of_control.has_value() || page.facts.spc.has_value()) {
        if (!page.facts.spc.has_value()) {
            page.facts.spc = domain::SpcFacts{};
        }
        page.facts.spc->out_of_control_count = out_of_control;
    }
    if (has_msa_fact && !page.facts.msa.has_value()) page.facts.msa = msa;
    if (has_reliability_fact && !page.facts.reliability.has_value()) {
        page.facts.reliability = reliability;
    }
    if (has_forecast_fact && !page.facts.forecast.has_value()) {
        page.facts.forecast = forecast;
    }
    if (has_power_fact && !page.facts.power.has_value()) page.facts.power = power;
    if (has_pareto_fact && !page.facts.pareto.has_value()) page.facts.pareto = pareto;
    if (has_doe_fact && !page.facts.doe.has_value()) page.facts.doe = doe;
    if (page.facts.spc.has_value()) {
        page.facts.spc->rule_policy =
            page.configuration.control.special_cause_rule_policy;
        page.facts.spc->enabled_special_cause_tests =
            page.configuration.control.enabled_special_cause_tests;
    }
    if (page.computation_traces.empty()) {
        std::string command_id = page.analysis_command_id;
        if (command_id.empty()) {
            command_id = resolve_command_id_from_page(page);
        }
        if (!command_id.empty()) {
            if (page.analysis_command_id.empty()) {
                page.analysis_command_id = command_id;
            }
            attach_computation_traces(page, command_id);
        }
    }
    return page;
}

}  // namespace

OutputPage AnalysisService::descriptive(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    OutputPage page;
    page.id = new_id("desc");
    page.title = "显示描述性统计";
    page.method_name = "Display Descriptive Statistics";
    page.configuration = configuration;

    std::vector<datalab::domain::statistics::DescriptiveStatisticsResult> rows;
    const std::vector<std::size_t> columns = configuration.variable_columns.empty()
        ? std::vector<std::size_t>{configuration.selection.measurement_column}
        : configuration.variable_columns;

    for (const std::size_t column : columns) {
        const ExtractedNumericColumn extracted =
            extract_numeric_column(table, column, configuration.excluded_rows);
        if (configuration.by_column.has_value()) {
            const std::vector<std::string> groups = extract_text_column(table, *configuration.by_column);
            std::map<std::string, std::vector<double>> grouped;
            std::map<std::string, std::size_t> missing;
            for (std::size_t index = 0; index < extracted.values.size(); ++index) {
                const std::size_t row = extracted.source_rows[index];
                const std::string label = row < groups.size() ? groups[row] : "*";
                grouped[label].push_back(extracted.values[index]);
            }
            for (std::size_t row = 0; row < table.rows.size(); ++row) {
                if (std::find(configuration.excluded_rows.begin(), configuration.excluded_rows.end(), row)
                    != configuration.excluded_rows.end()) {
                    continue;
                }
                if (column >= table.rows[row].size() || is_missing_cell(table.rows[row][column])) {
                    const std::string label = row < groups.size() ? groups[row] : "*";
                    ++missing[label];
                }
            }
            for (auto& [label, values] : grouped) {
                auto stats = datalab::domain::statistics::DescriptiveStatistics::calculate(
                    values, missing[label], values.size() + missing[label]);
                if (stats.has_value()) {
                    stats->group_label = extracted.name + " / " + label;
                    rows.push_back(*stats);
                }
            }
        } else {
            auto stats = datalab::domain::statistics::DescriptiveStatistics::calculate(
                extracted.values, extracted.missing_count, extracted.total_count);
            if (stats.has_value()) {
                stats->group_label = extracted.name;
                rows.push_back(*stats);
            }
        }
    }
    if (rows.empty()) {
        return error_page(page.title, page.method_name, "所选列没有足够的数值观测。");
    }
    page.tables.push_back(descriptive_table(rows));
    page.parameter_summary = "变量数 = " + std::to_string(columns.size());
    domain::DescriptiveFacts facts;
    facts.n = rows.front().count;
    facts.missing_count = rows.front().missing_count;
    facts.mean = rows.front().mean;
    facts.standard_deviation = rows.front().sample_standard_deviation;
    page.facts.descriptive = facts;

    PlotSpec box;
    box.kind = PlotKind::boxplot;
    box.title = "箱线图";
    box.x_axis_title = "分组";
    box.y_axis_title = "观测";
    PlotSeries outliers;
    outliers.role = PlotSeriesRole::generic;
    outliers.label = "异常点";
    outliers.style.color = "#c62828";
    outliers.style.point_style = PlotPointStyle::circle;
    outliers.show_points = true;
    PlotSpec individuals;
    individuals.kind = PlotKind::scatter;
    individuals.title = "个体值图";
    individuals.x_axis_title = "分组";
    individuals.y_axis_title = "观测";
    std::size_t total_missing = 0;

    const auto append_visuals = [&](const std::string& label,
                                    const std::vector<double>& values,
                                    const std::vector<std::size_t>& source_rows) {
        if (values.empty()) {
            return;
        }
        const auto summary = datalab::domain::statistics::box_plot_summary(values);
        const std::size_t group_index = box.box_labels.size();
        box.box_labels.push_back(label);
        box.box_min.push_back(summary.whisker_low);
        box.box_q1.push_back(summary.first_quartile);
        box.box_median.push_back(summary.median);
        box.box_q3.push_back(summary.third_quartile);
        box.box_max.push_back(summary.whisker_high);
        for (const double outlier : summary.outliers) {
            outliers.x_values.push_back(static_cast<double>(group_index));
            outliers.values.push_back(outlier);
        }
        for (std::size_t index = 0; index < values.size(); ++index) {
            individuals.x_values.push_back(static_cast<double>(group_index));
            individuals.values.push_back(values[index]);
            if (index < source_rows.size()) {
                individuals.source_rows.push_back(source_rows[index]);
            }
            individuals.point_groups.push_back(label);
        }
    };

    for (const std::size_t column : columns) {
        const ExtractedNumericColumn extracted =
            extract_numeric_column(table, column, configuration.excluded_rows);
        total_missing += extracted.missing_count;
        if (configuration.by_column.has_value()) {
            const std::vector<std::string> groups = extract_text_column(table, *configuration.by_column);
            std::vector<std::string> order;
            std::vector<std::vector<double>> grouped;
            std::vector<std::vector<std::size_t>> grouped_rows;
            for (std::size_t index = 0; index < extracted.values.size(); ++index) {
                const std::size_t row = extracted.source_rows[index];
                const std::string group = row < groups.size() ? groups[row] : "*";
                const std::size_t group_index = datalab::domain::stable_group_index(order, group);
                if (group_index >= grouped.size()) {
                    grouped.emplace_back();
                    grouped_rows.emplace_back();
                }
                grouped[group_index].push_back(extracted.values[index]);
                grouped_rows[group_index].push_back(row);
            }
            for (std::size_t group = 0; group < order.size(); ++group) {
                append_visuals(extracted.name + " / " + order[group],
                               grouped[group], grouped_rows[group]);
            }
        } else {
            append_visuals(extracted.name, extracted.values, extracted.source_rows);
        }
    }
    if (total_missing > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "描述统计跳过 " + std::to_string(total_missing)
                + " 个缺失或非法单元格（含 *）。"});
    }
    if (!outliers.values.empty()) {
        box.series.push_back(std::move(outliers));
    }
    if (!box.box_labels.empty()) {
        page.plots.push_back(std::move(box));
    }
    if (!individuals.values.empty()) {
        page.plots.push_back(std::move(individuals));
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::normality_test(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::size_t column = first_variable(configuration);
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, column, configuration.excluded_rows);
    const std::string method =
        configuration.inference.normality_method == "ryan_joiner"
            ? "ryan_joiner" : "anderson_darling";
    const auto result = datalab::domain::statistics::normality_test(
        extracted.values, extracted.source_rows, method);

    OutputPage page;
    page.id = new_id("normality");
    page.title = "正态性检验";
    page.method_name = "Normality Test";
    page.configuration = configuration;
    const std::string method_label =
        method == "ryan_joiner" ? "Ryan-Joiner" : "Anderson-Darling";
    page.parameter_summary = "变量: " + extracted.name
        + "    方法: " + method_label + "    缺失值 N* = "
        + std::to_string(extracted.missing_count);
    if (!result.messages.empty()) {
        page.diagnostics = result.messages;
    } else {
        for (const std::string& diagnostic : result.diagnostics) {
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::warning, "normality", diagnostic});
        }
    }
    StatisticTable table_out;
    table_out.title = "正态性检验";
    std::string conclusion = "无法计算";
    if (result.decision == "reject") {
        conclusion = "在 alpha 下拒绝正态假设";
    } else if (result.decision == "fail_to_reject") {
        conclusion = "在 alpha 下未拒绝正态假设";
    }
    if (method == "ryan_joiner") {
        table_out.headers = {"变量", "N", "N*", "Mean", "StDev", "RJ(R)", "Alpha",
                             "P-Value", "判定"};
        table_out.rows.push_back({
            extracted.name,
            std::to_string(result.count),
            std::to_string(extracted.missing_count),
            format_number(result.mean),
            result.sample_standard_deviation > 0.0
                ? format_number(result.sample_standard_deviation) : "*",
            result.ryan_joiner_r.has_value() ? format_number(*result.ryan_joiner_r) : "*",
            format_number(result.alpha),
            result.p_value.has_value() ? format_number(*result.p_value) : "*",
            conclusion});
    } else {
        table_out.headers = {"变量", "N", "N*", "Mean", "StDev", "AD", "A²*", "Alpha",
                             "P-Value", "判定"};
        table_out.rows.push_back({
            extracted.name,
            std::to_string(result.count),
            std::to_string(extracted.missing_count),
            format_number(result.mean),
            result.sample_standard_deviation > 0.0
                ? format_number(result.sample_standard_deviation) : "*",
            result.anderson_darling.has_value() ? format_number(*result.anderson_darling) : "*",
            result.adjusted_anderson_darling.has_value()
                ? format_number(*result.adjusted_anderson_darling) : "*",
            format_number(result.alpha),
            result.p_value.has_value() ? format_number(*result.p_value) : "*",
            conclusion});
    }
    page.tables.push_back(table_out);

    PlotSpec plot;
    plot.kind = PlotKind::probability;
    plot.title = extracted.name + " 的正态概率图";
    plot.x_axis_title = "标准正态分位数";
    plot.y_axis_title = extracted.name;
    plot.values = result.probability_plot.ordered_values;
    plot.x_values = result.probability_plot.theoretical_quantiles;
    plot.source_rows = result.probability_plot.source_rows;
    plot.line_width = 1.4;
    page.plots.push_back(plot);
    const datalab::domain::statistics::HistogramResult bins =
        datalab::domain::statistics::histogram(extracted.values, 0);
    PlotSpec hist;
    hist.kind = PlotKind::histogram;
    hist.title = extracted.name + " 的直方图";
    hist.x_axis_title = extracted.name;
    hist.y_axis_title = "频数";
    hist.histogram_edges = bins.edges;
    hist.histogram_counts = bins.counts;
    hist.values = extracted.values;
    hist.source_rows = extracted.source_rows;
    page.plots.push_back(std::move(hist));
    domain::NormalityFacts facts;
    facts.n = result.count;
    facts.missing_count = extracted.missing_count;
    facts.method = result.method;
    facts.decision = result.decision;
    facts.p_value = result.p_value;
    facts.anderson_darling = result.anderson_darling;
    facts.ryan_joiner_r = result.ryan_joiner_r;
    facts.alpha = result.alpha;
    facts.assumption_status = result.evidence.assumption_status.empty()
        ? "not_verified" : result.evidence.assumption_status;
    page.facts.normality = std::move(facts);
    domain::apply_evidence(page.method_metadata, result.evidence);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::outlier_test(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::size_t column = first_variable(configuration);
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, column, configuration.excluded_rows);
    double alpha = 1.0 - configuration.inference.confidence_level;
    if (!(alpha > 0.0 && alpha < 1.0)) {
        alpha = 0.05;
    }
    const std::string method =
        configuration.inference.outlier_method == "dixon_r10" ? "dixon_r10" : "grubbs";
    const auto alternative = parse_alternative(configuration.inference.alternative);

    std::size_t n = 0;
    std::size_t missing_count = extracted.missing_count;
    double mean = 0.0;
    double sample_sd = 0.0;
    std::optional<double> g_statistic;
    std::optional<double> dixon_r;
    std::optional<double> critical_value;
    std::optional<double> p_value;
    std::optional<double> outlier_value;
    std::optional<std::size_t> outlier_index;
    std::optional<std::size_t> source_row;
    std::string direction;
    std::string alternative_text = configuration.inference.alternative.empty()
        ? "two_sided" : configuration.inference.alternative;
    std::string assumption_status = "not_verified";
    std::vector<DiagnosticMessage> diagnostics;

    if (method == "dixon_r10") {
        const auto result = datalab::domain::statistics::dixon_r10_outlier_test(
            extracted.values, extracted.source_rows, alternative, alpha,
            extracted.missing_count);
        n = result.n;
        missing_count = result.missing_count;
        mean = result.mean;
        sample_sd = result.sample_standard_deviation;
        dixon_r = result.r_statistic;
        critical_value = result.critical_value;
        p_value = result.p_value;
        outlier_value = result.outlier_value;
        outlier_index = result.outlier_index;
        source_row = result.source_row;
        direction = result.direction;
        alternative_text = result.alternative;
        assumption_status = result.assumption_status;
        diagnostics = result.diagnostics;
    } else {
        const auto result = datalab::domain::statistics::grubbs_outlier_test(
            extracted.values, extracted.source_rows, alternative, alpha,
            extracted.missing_count);
        n = result.n;
        missing_count = result.missing_count;
        mean = result.mean;
        sample_sd = result.sample_standard_deviation;
        g_statistic = result.g_statistic;
        p_value = result.p_value;
        outlier_value = result.outlier_value;
        outlier_index = result.outlier_index;
        source_row = result.source_row;
        direction = result.direction;
        alternative_text = result.alternative;
        assumption_status = result.assumption_status;
        diagnostics = result.diagnostics;
    }

    const std::string method_label =
        method == "dixon_r10" ? "Dixon r10" : "Grubbs";
    OutputPage page;
    page.id = new_id("outlier_test");
    page.title = "异常值检验";
    page.method_name = "Outlier Test";
    page.configuration = configuration;
    page.diagnostics = std::move(diagnostics);
    page.parameter_summary = "变量: " + extracted.name
        + "    方法: " + method_label
        + "    备择: " + alternative_text
        + "    α = " + format_number(alpha)
        + "    缺失值 N* = " + std::to_string(extracted.missing_count);

    StatisticTable method_table;
    method_table.title = "方法";
    method_table.headers = {"方法"};
    method_table.rows.push_back({method_label});
    page.tables.push_back(std::move(method_table));

    StatisticTable table_out;
    table_out.title = "异常值检验";
    if (method == "dixon_r10") {
        table_out.headers = {
            "变量", "N", "N*", "Mean", "StDev", "r", "临界值", "P-Value",
            "嫌疑值", "方向", "source_row"};
        table_out.rows.push_back({
            extracted.name,
            std::to_string(n),
            std::to_string(missing_count),
            n > 0 ? format_number(mean) : "*",
            sample_sd > 0.0 ? format_number(sample_sd) : "*",
            dixon_r.has_value() ? format_number(*dixon_r) : "*",
            critical_value.has_value() ? format_number(*critical_value) : "*",
            p_value.has_value() ? format_number(*p_value) : "*",
            outlier_value.has_value() ? format_number(*outlier_value) : "*",
            direction.empty() ? "*" : direction,
            source_row.has_value() ? std::to_string(*source_row) : "*"});
    } else {
        table_out.headers = {
            "变量", "N", "N*", "Mean", "StDev", "G", "P-Value",
            "嫌疑值", "方向", "source_row"};
        table_out.rows.push_back({
            extracted.name,
            std::to_string(n),
            std::to_string(missing_count),
            n > 0 ? format_number(mean) : "*",
            sample_sd > 0.0 ? format_number(sample_sd) : "*",
            g_statistic.has_value() ? format_number(*g_statistic) : "*",
            p_value.has_value() ? format_number(*p_value) : "*",
            outlier_value.has_value() ? format_number(*outlier_value) : "*",
            direction.empty() ? "*" : direction,
            source_row.has_value() ? std::to_string(*source_row) : "*"});
    }
    page.tables.push_back(std::move(table_out));

    if (!extracted.values.empty()) {
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = extracted.name + " 的个体值图";
        plot.x_axis_title = "观测序";
        plot.y_axis_title = extracted.name;
        plot.values = extracted.values;
        plot.source_rows = extracted.source_rows;
        for (std::size_t index = 0; index < extracted.values.size(); ++index) {
            plot.x_values.push_back(static_cast<double>(index + 1));
            if (outlier_index.has_value() && index == *outlier_index) {
                plot.point_labels.push_back("嫌疑点");
            } else {
                plot.point_labels.push_back("");
            }
        }
        if (outlier_index.has_value() && *outlier_index < extracted.values.size()) {
            PlotSeries highlight;
            highlight.label = "嫌疑观测";
            highlight.style.color = "#c62828";
            highlight.style.point_style = PlotPointStyle::circle;
            highlight.show_points = true;
            highlight.x_values.push_back(
                static_cast<double>(*outlier_index + 1));
            highlight.values.push_back(extracted.values[*outlier_index]);
            plot.series.push_back(std::move(highlight));
        }
        page.plots.push_back(std::move(plot));
    }

    domain::OutlierTestFacts facts;
    facts.n = n;
    facts.missing_count = missing_count;
    if (n > 0) {
        facts.mean = mean;
        facts.standard_deviation = sample_sd;
    }
    facts.method = method;
    facts.g_statistic = g_statistic;
    facts.dixon_r = dixon_r;
    facts.critical_value = critical_value;
    facts.p_value = p_value;
    facts.outlier_value = outlier_value;
    facts.source_row = source_row;
    facts.direction = direction;
    facts.alternative = alternative_text;
    facts.alpha = alpha;
    facts.assumption_status = assumption_status;
    page.facts.outlier_test = std::move(facts);
    page.method_metadata.estimation_method = method;
    page.method_metadata.assumption_status = assumption_status;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::correlation(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() < 2) {
        return error_page("相关分析", "Correlation", "请选择至少两列数值变量。");
    }
    std::vector<ExtractedNumericColumn> extracted_columns;
    extracted_columns.reserve(configuration.variable_columns.size());
    std::vector<std::string> names;
    for (const std::size_t column : configuration.variable_columns) {
        extracted_columns.push_back(
            extract_numeric_column(table, column, configuration.excluded_rows));
        names.push_back(extracted_columns.back().name);
    }
    const auto aligned = align_complete_rows_with_source(extracted_columns);
    std::vector<std::vector<double>> columns(extracted_columns.size());
    for (const auto& row : aligned.values) {
        for (std::size_t column = 0; column < row.size(); ++column) {
            columns[column].push_back(row[column]);
        }
    }
    const bool spearman = configuration.inference.correlation_method == "spearman";
    const bool compute_partial = configuration.inference.compute_partial_correlation;
    const auto result = datalab::domain::statistics::correlation_matrix(
        columns,
        spearman ? datalab::domain::statistics::CorrelationMethod::spearman
                 : datalab::domain::statistics::CorrelationMethod::pearson,
        configuration.inference.confidence_level,
        compute_partial);
    OutputPage page;
    page.id = new_id("correlation");
    page.title = spearman ? "Spearman 秩相关" : "Pearson 相关";
    page.method_name = "Correlation";
    page.configuration = configuration;
    page.parameter_summary = "方法 = " + std::string(spearman ? "Spearman" : "Pearson")
        + "    置信水平 = " + format_number(configuration.inference.confidence_level)
        + "    有效变量数 = " + std::to_string(columns.size())
        + "    N = " + std::to_string(aligned.source_rows.size())
        + (compute_partial ? "    偏相关 = 是" : "");
    page.diagnostics = result.diagnostics;
    const std::size_t eligible = table.rows.size() >= configuration.excluded_rows.size()
        ? table.rows.size() - configuration.excluded_rows.size() : table.rows.size();
    const std::size_t skipped = eligible > aligned.source_rows.size()
        ? eligible - aligned.source_rows.size() : 0;
    if (skipped > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "相关分析 complete-case 跳过 " + std::to_string(skipped)
                + " 个含缺失或非法单元格的行。"});
    }
    StatisticTable coefficients;
    coefficients.title = "相关系数矩阵";
    coefficients.headers.push_back("变量");
    coefficients.headers.insert(coefficients.headers.end(), names.cbegin(), names.cend());
    for (std::size_t row = 0; row < names.size(); ++row) {
        std::vector<std::string> values;
        values.push_back(names[row]);
        for (std::size_t column = 0; column < names.size(); ++column) {
            values.push_back(format_number(result.coefficients[row][column]));
        }
        coefficients.rows.push_back(values);
    }
    page.tables.push_back(coefficients);
    if (result.covariance_available) {
        StatisticTable covariance;
        covariance.title = "协方差矩阵";
        covariance.headers.push_back("变量");
        covariance.headers.insert(covariance.headers.end(), names.cbegin(), names.cend());
        for (std::size_t row = 0; row < names.size(); ++row) {
            std::vector<std::string> values;
            values.push_back(names[row]);
            for (std::size_t column = 0; column < names.size(); ++column) {
                values.push_back(format_number(result.covariances[row][column]));
            }
            covariance.rows.push_back(values);
        }
        page.tables.push_back(covariance);
    }
    if (result.partial_available) {
        StatisticTable partial;
        partial.title = "偏相关系数矩阵";
        partial.headers.push_back("变量");
        partial.headers.insert(partial.headers.end(), names.cbegin(), names.cend());
        for (std::size_t row = 0; row < names.size(); ++row) {
            std::vector<std::string> values;
            values.push_back(names[row]);
            for (std::size_t column = 0; column < names.size(); ++column) {
                values.push_back(format_number(result.partial_coefficients[row][column]));
            }
            partial.rows.push_back(values);
        }
        page.tables.push_back(partial);
    }
    StatisticTable pair_table;
    pair_table.title = "相关分析详细结果";
    pair_table.headers = {"变量 1", "变量 2", "N", "相关系数", "P-Value", "置信区间"};
    for (const auto& pair : result.pairs) {
        const std::string interval = pair.confidence_lower.has_value()
            && pair.confidence_upper.has_value()
            ? "[" + format_number(*pair.confidence_lower) + ", "
                + format_number(*pair.confidence_upper) + "]" : "*";
        pair_table.rows.push_back({
            pair.first_column < names.size() ? names[pair.first_column] : "*",
            pair.second_column < names.size() ? names[pair.second_column] : "*",
            std::to_string(pair.count),
            format_number(pair.coefficient),
            pair.p_value.has_value() ? format_number(*pair.p_value) : "*",
            interval});
        append_diagnostics(page.diagnostics, pair.diagnostics, "相关分析：");
    }
    page.tables.push_back(pair_table);
    if (columns.size() >= 2 && !aligned.source_rows.empty()) {
        PlotSpec matrix;
        matrix.kind = PlotKind::matrix;
        matrix.title = "矩阵散点图";
        matrix.x_axis_title = "变量";
        matrix.y_axis_title = "变量";
        matrix.matrix_labels = names;
        matrix.matrix_values = columns;
        matrix.source_rows = aligned.source_rows;
        page.plots.push_back(std::move(matrix));
    }
    if (columns.size() == 2 && columns[0].size() == columns[1].size()) {
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = names[0] + " 与 " + names[1] + " 的散点图";
        plot.x_axis_title = names[0];
        plot.y_axis_title = names[1];
        plot.x_values = columns[0];
        plot.values = columns[1];
        plot.source_rows = aligned.source_rows;
        page.plots.push_back(plot);
    }
    domain::CorrelationFacts facts;
    facts.method = spearman ? "spearman" : "pearson";
    facts.variable_count = columns.size();
    facts.n = aligned.source_rows.size();
    facts.missing_skipped = skipped;
    facts.assumption_status = "not_verified";
    facts.covariance_available = result.covariance_available;
    facts.partial_available = result.partial_available;
    page.facts.correlation = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::one_sample_t(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (!configuration.inference.hypothesis_mean.has_value()) {
        return error_page("单样本 t 检验", "One-Sample T", "请指定假设均值。");
    }
    const auto result = datalab::domain::statistics::one_sample_t_test(
        extracted.values,
        *configuration.inference.hypothesis_mean,
        configuration.inference.confidence_level,
        parse_alternative(configuration.inference.alternative));
    OutputPage page;
    page.id = new_id("one_sample_t");
    page.title = "单样本 t 检验";
    page.method_name = "One-Sample T";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    假设均值 = " + format_number(*configuration.inference.hypothesis_mean)
        + "    备择：总体均值 " + alternative_label(configuration.inference.alternative)
        + " 假设均值";
    append_diagnostics(page.diagnostics, result.diagnostics, "单样本 t：");
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "跳过 " + std::to_string(extracted.missing_count)
                + " 个缺失或非法单元格。"});
    }
    page.tables.push_back(t_test_table("单样本 t 检验", result, extracted.name));
    if (result.confidence_lower.has_value() && result.confidence_upper.has_value()) {
        page.plots.push_back(interval_plot_spec(
            "区间图", extracted.name, extracted.name,
            {extracted.name},
            {result.mean},
            {*configuration.inference.hypothesis_mean + *result.confidence_lower},
            {*configuration.inference.hypothesis_mean + *result.confidence_upper},
            {result.count},
            extracted.source_rows.empty()
                ? std::vector<std::size_t>{}
                : std::vector<std::size_t>{extracted.source_rows.front()}));
    } else {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "one_sided_interval_not_plotted",
            "单侧置信区间缺少一端，未输出均值区间图。"});
    }
    domain::TTestFacts facts;
    facts.kind = "one_sample";
    facts.n = result.count;
    facts.missing_count = extracted.missing_count;
    facts.mean = result.mean;
    facts.difference = result.difference;
    facts.p_value = result.p_value;
    facts.ci_lower = result.confidence_lower;
    facts.ci_upper = result.confidence_upper;
    facts.assumption_status = "not_verified";
    page.facts.t_test = std::move(facts);
    page.analysis_command_id = "one_sample_t";
    attach_computation_traces(page, "one_sample_t");
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::one_sample_z(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (!configuration.inference.hypothesis_mean.has_value()) {
        return error_page("单样本 Z 检验", "One-Sample Z", "请指定假设均值。");
    }
    if (!configuration.inference.known_sigma.has_value()
        || !(*configuration.inference.known_sigma > 0.0)) {
        return error_page("单样本 Z 检验", "One-Sample Z",
                          "请指定已知总体标准差 σ（必须为正）。");
    }
    const double known_sigma = *configuration.inference.known_sigma;
    const auto result = datalab::domain::statistics::one_sample_z_test(
        extracted.values,
        *configuration.inference.hypothesis_mean,
        known_sigma,
        configuration.inference.confidence_level,
        parse_alternative(configuration.inference.alternative));

    double sample_sd = 0.0;
    if (extracted.values.size() >= 2) {
        const double mean = result.mean;
        double ss = 0.0;
        for (const double value : extracted.values) {
            const double residual = value - mean;
            ss += residual * residual;
        }
        sample_sd = std::sqrt(ss / static_cast<double>(extracted.values.size() - 1));
    }

    OutputPage page;
    page.id = new_id("one_sample_z");
    page.title = "单样本 Z 检验";
    page.method_name = "One-Sample Z";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    假设均值 = " + format_number(*configuration.inference.hypothesis_mean)
        + "    Known σ = " + format_number(known_sigma)
        + "    备择：总体均值 " + alternative_label(configuration.inference.alternative)
        + " 假设均值";
    append_diagnostics(page.diagnostics, result.diagnostics, "单样本 Z：");
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "跳过 " + std::to_string(extracted.missing_count)
                + " 个缺失或非法单元格。"});
    }

    StatisticTable desc;
    desc.title = "描述统计";
    desc.headers = {"变量", "N", "N*", "Mean", "StDev", "SE Mean", "Known σ"};
    desc.rows.push_back({
        extracted.name,
        std::to_string(result.count),
        std::to_string(extracted.missing_count),
        result.count > 0 ? format_number(result.mean) : "*",
        extracted.values.size() >= 2 ? format_number(sample_sd) : "*",
        result.count > 0 ? format_number(result.standard_error) : "*",
        format_number(known_sigma)});
    page.tables.push_back(std::move(desc));

    StatisticTable test;
    test.title = "单样本 Z 检验";
    test.headers = {"假设均值", "Z", "P-Value"};
    test.rows.push_back({
        format_number(result.hypothesized_mean),
        result.count > 0 ? format_number(result.z_statistic) : "*",
        result.p_value.has_value() ? format_number(*result.p_value) : "*"});
    page.tables.push_back(std::move(test));

    StatisticTable ci;
    ci.title = "置信区间";
    ci.headers = {"置信水平", "下限", "上限"};
    const std::string lower_text = result.confidence_lower.has_value()
        ? format_number(*configuration.inference.hypothesis_mean + *result.confidence_lower)
        : "-∞";
    const std::string upper_text = result.confidence_upper.has_value()
        ? format_number(*configuration.inference.hypothesis_mean + *result.confidence_upper)
        : "+∞";
    ci.rows.push_back({
        format_number(result.confidence_level),
        lower_text,
        upper_text});
    page.tables.push_back(std::move(ci));

    if (result.confidence_lower.has_value() && result.confidence_upper.has_value()) {
        page.plots.push_back(interval_plot_spec(
            "区间图", extracted.name, extracted.name,
            {extracted.name},
            {result.mean},
            {*configuration.inference.hypothesis_mean + *result.confidence_lower},
            {*configuration.inference.hypothesis_mean + *result.confidence_upper},
            {result.count},
            extracted.source_rows.empty()
                ? std::vector<std::size_t>{}
                : std::vector<std::size_t>{extracted.source_rows.front()}));
    } else {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "one_sided_interval_not_plotted",
            "单侧置信区间缺少一端，未输出均值区间图。"});
    }

    domain::TTestFacts facts;
    facts.kind = "one_sample_z";
    facts.n = result.count;
    facts.missing_count = extracted.missing_count;
    facts.mean = result.mean;
    facts.difference = result.difference;
    facts.p_value = result.p_value;
    facts.ci_lower = result.confidence_lower;
    facts.ci_upper = result.confidence_upper;
    facts.z_statistic = result.z_statistic;
    facts.known_sigma = known_sigma;
    if (extracted.values.size() >= 2) {
        facts.sample_standard_deviation = sample_sd;
    }
    facts.assumption_status = "not_verified";
    page.facts.t_test = std::move(facts);
    page.method_metadata.estimation_method = "one_sample_z";
    page.method_metadata.assumption_status = "not_verified";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::two_sample_t(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() < 2) {
        return error_page("双样本 t 检验", "Two-Sample T", "请选择两列独立样本变量。");
    }
    const ExtractedNumericColumn first = extract_numeric_column(
        table, configuration.variable_columns[0], configuration.excluded_rows);
    const ExtractedNumericColumn second = extract_numeric_column(
        table, configuration.variable_columns[1], configuration.excluded_rows);
    const auto result = datalab::domain::statistics::two_sample_t_test(
        first.values,
        second.values,
        configuration.inference.confidence_level,
        parse_alternative(configuration.inference.alternative),
        parse_variance_method(configuration.inference.variance_method));
    OutputPage page;
    page.id = new_id("two_sample_t");
    page.title = "双样本 t 检验";
    page.method_name = "Two-Sample T";
    page.configuration = configuration;
    page.parameter_summary = "方法 = "
        + std::string(configuration.inference.variance_method == "pooled" ? "合并方差" : "Welch")
        + "    置信水平 = " + format_number(configuration.inference.confidence_level);
    append_diagnostics(page.diagnostics, result.diagnostics, "双样本 t：");
    if (first.missing_count > 0 || second.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "双样本 t 检验跳过缺失或非法单元格；组 1 = "
                + std::to_string(first.missing_count) + "，组 2 = "
                + std::to_string(second.missing_count) + "。"});
    }
    StatisticTable groups;
    groups.title = "双样本描述统计";
    groups.headers = {"组", "N", "Mean", "StDev"};
    groups.rows = {
        {first.name, std::to_string(result.first.count), format_number(result.first.mean),
         format_number(result.first.sample_standard_deviation)},
        {second.name, std::to_string(result.second.count), format_number(result.second.mean),
         format_number(result.second.sample_standard_deviation)}};
    page.tables.push_back(groups);
    StatisticTable test;
    test.title = "双样本 t 检验";
    test.headers = {"均值差", "SE 差值", "T", "DF", "P-Value", "置信区间"};
    const std::string interval = result.confidence_lower.has_value()
        && result.confidence_upper.has_value()
        ? "[" + format_number(*result.confidence_lower) + ", "
            + format_number(*result.confidence_upper) + "]" : "*";
    test.rows.push_back({
        format_number(result.mean_difference),
        format_number(result.standard_error_difference),
        format_number(result.t_statistic),
        format_number(result.degrees_of_freedom),
        result.p_value.has_value() ? format_number(*result.p_value) : "*",
        interval});
    page.tables.push_back(test);
    if (parse_alternative(configuration.inference.alternative)
            == datalab::domain::statistics::TestAlternative::two_sided
        && result.first.count >= 2 && result.second.count >= 2) {
        const double n1 = static_cast<double>(result.first.count);
        const double n2 = static_cast<double>(result.second.count);
        const double s1 = result.first.sample_standard_deviation;
        const double s2 = result.second.sample_standard_deviation;
        std::vector<double> lowers;
        std::vector<double> uppers;
        if (result.variance_method == datalab::domain::statistics::VarianceMethod::pooled) {
            const double pooled_variance =
                ((n1 - 1.0) * s1 * s1 + (n2 - 1.0) * s2 * s2) / (n1 + n2 - 2.0);
            const double df = n1 + n2 - 2.0;
            const auto first_ci = mean_t_interval(
                result.first.mean, std::sqrt(pooled_variance / n1), df,
                configuration.inference.confidence_level);
            const auto second_ci = mean_t_interval(
                result.second.mean, std::sqrt(pooled_variance / n2), df,
                configuration.inference.confidence_level);
            lowers = {first_ci.first, second_ci.first};
            uppers = {first_ci.second, second_ci.second};
        } else {
            const auto first_ci = mean_t_interval(
                result.first.mean, s1 / std::sqrt(n1), n1 - 1.0,
                configuration.inference.confidence_level);
            const auto second_ci = mean_t_interval(
                result.second.mean, s2 / std::sqrt(n2), n2 - 1.0,
                configuration.inference.confidence_level);
            lowers = {first_ci.first, second_ci.first};
            uppers = {first_ci.second, second_ci.second};
        }
        std::vector<std::size_t> interval_rows;
        if (!first.source_rows.empty()) {
            interval_rows.push_back(first.source_rows.front());
        }
        if (!second.source_rows.empty()) {
            interval_rows.push_back(second.source_rows.front());
        }
        page.plots.push_back(interval_plot_spec(
            "区间图", "组", "均值",
            {first.name, second.name},
            {result.first.mean, result.second.mean},
            lowers, uppers,
            {result.first.count, result.second.count},
            interval_rows));
    } else {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "one_sided_interval_not_plotted",
            "单侧置信区间缺少一端，未输出组均值区间图。"});
    }
    domain::TTestFacts facts;
    facts.kind = "two_sample";
    facts.n = result.first.count + result.second.count;
    facts.missing_count = first.missing_count + second.missing_count;
    facts.difference = result.mean_difference;
    facts.p_value = result.p_value;
    facts.ci_lower = result.confidence_lower;
    facts.ci_upper = result.confidence_upper;
    facts.variance_method =
        result.variance_method == datalab::domain::statistics::VarianceMethod::pooled
            ? "pooled" : "welch";
    facts.assumption_status = "not_verified";
    page.facts.t_test = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage make_equivalence_page(
    const std::string& title,
    const std::string& method_name,
    const std::string& id_prefix,
    const AnalysisConfiguration& configuration,
    const datalab::domain::statistics::EquivalenceTestResult& result)
{
    OutputPage page;
    page.id = new_id(id_prefix);
    page.title = title;
    page.method_name = method_name;
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    page.parameter_summary = "α = " + format_number(result.alpha)
        + "    界限 = [" + format_number(result.lower) + ", "
        + format_number(result.upper) + "]";
    StatisticTable groups;
    groups.title = "描述统计";
    const bool proportion_kind = result.kind == "one_proportion"
        || result.kind == "two_proportion";
    const bool ratio_kind = result.kind == "two_sample_ratio";
    if (proportion_kind) {
        groups.headers = {"组", "事件数", "试验数", "比例"};
        if (result.kind == "one_proportion") {
            groups.rows.push_back({
                "样本",
                format_number(result.first_standard_deviation),
                std::to_string(result.first_count),
                format_number(result.first_mean)});
            groups.rows.push_back({
                "目标比例", "*", "*", format_number(result.second_mean)});
        } else {
            groups.rows.push_back({
                "样本 1",
                format_number(result.first_standard_deviation),
                std::to_string(result.first_count),
                format_number(result.first_mean)});
            groups.rows.push_back({
                "样本 2",
                format_number(result.second_standard_deviation),
                std::to_string(result.second_count),
                format_number(result.second_mean)});
        }
    } else {
        groups.headers = {"组", "N", "Mean", "StDev"};
        if (result.kind == "paired") {
            groups.rows.push_back({
                "配对差值", std::to_string(result.first_count),
                format_number(result.first_mean),
                format_number(result.first_standard_deviation)});
        } else {
            groups.rows.push_back({
                "样本 1", std::to_string(result.first_count),
                format_number(result.first_mean),
                format_number(result.first_standard_deviation)});
        }
        if (result.kind == "two_sample" || result.kind == "two_sample_ratio") {
            groups.rows.push_back({
                result.kind == "two_sample_ratio" ? "参考样本" : "样本 2",
                std::to_string(result.second_count),
                format_number(result.second_mean),
                format_number(result.second_standard_deviation)});
        }
    }
    if (result.kind == "two_sample_ratio" && !groups.rows.empty()) {
        groups.rows[0][0] = "检验样本";
    }
    page.tables.push_back(std::move(groups));
    StatisticTable test;
    test.title = "等价性检验";
    const std::string estimate_header = ratio_kind ? "比值" : "差值";
    test.headers = proportion_kind
        ? std::vector<std::string>{"差值", "下限 z", "下限 P", "上限 z", "上限 P",
                                   "α", "CI", "CI 方法", "界限", "结论"}
        : std::vector<std::string>{estimate_header, "下限 t", "下限 P", "上限 t", "上限 P",
                                   "α", "CI", "CI 方法", "界限", "结论"};
    const std::string interval = result.confidence_lower.has_value()
        && result.confidence_upper.has_value()
        ? "[" + format_number(*result.confidence_lower) + ", "
            + format_number(*result.confidence_upper) + "]"
        : "*";
    test.rows.push_back({
        format_number(result.difference),
        format_number(result.t_lower),
        format_optional(result.p_lower),
        format_number(result.t_upper),
        format_optional(result.p_upper),
        format_number(result.alpha),
        interval,
        result.ci_method,
        "[" + format_number(result.lower) + ", " + format_number(result.upper) + "]",
        result.within_limits ? "within_limits" : "not_within_limits"});
    page.tables.push_back(std::move(test));
    if (result.confidence_lower.has_value() && result.confidence_upper.has_value()) {
        PlotSpec interval_plot;
        interval_plot.kind = PlotKind::interval;
        interval_plot.title = "等价性区间";
        const char* axis = proportion_kind ? "比例差" : (ratio_kind ? "比值" : "差值");
        interval_plot.x_axis_title = axis;
        interval_plot.y_axis_title = axis;
        interval_plot.categories = {axis};
        interval_plot.values = {result.difference};
        interval_plot.interval_lower = {*result.confidence_lower};
        interval_plot.interval_upper = {*result.confidence_upper};
        PlotSeries lower_line;
        lower_line.label = "等价下限";
        lower_line.x_values = {0.0, 1.0};
        lower_line.values = {result.lower, result.lower};
        PlotSeries upper_line;
        upper_line.label = "等价上限";
        upper_line.x_values = {0.0, 1.0};
        upper_line.values = {result.upper, result.upper};
        interval_plot.series = {std::move(lower_line), std::move(upper_line)};
        page.plots.push_back(std::move(interval_plot));
    }
    domain::EquivalenceFacts facts;
    facts.kind = result.kind;
    facts.difference = result.difference;
    facts.lower = result.lower;
    facts.upper = result.upper;
    facts.ci_lower = result.confidence_lower;
    facts.ci_upper = result.confidence_upper;
    facts.p_lower = result.p_lower;
    facts.p_upper = result.p_upper;
    facts.alpha = result.alpha;
    facts.ci_method = result.ci_method;
    facts.within_limits = result.within_limits;
    facts.both_pvalues_below_alpha = result.both_pvalues_below_alpha;
    facts.assumption_status = "not_verified";
    page.facts.equivalence = std::move(facts);
    return page;
}

OutputPage AnalysisService::one_sample_equivalence(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.inference.equivalence_lower.has_value()
        || !configuration.inference.equivalence_upper.has_value()) {
        return error_page("单样本等价性检验", "1-Sample Equivalence Test",
                          "请指定等价下限和上限。");
    }
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    const double target = configuration.inference.hypothesis_mean.value_or(0.0);
    const auto result = datalab::domain::statistics::one_sample_equivalence_test(
        extracted.values, target,
        *configuration.inference.equivalence_lower,
        *configuration.inference.equivalence_upper,
        configuration.inference.confidence_level);
    OutputPage page = make_equivalence_page(
        "单样本等价性检验", "1-Sample Equivalence Test", "one_sample_eq",
        configuration, result);
    page.parameter_summary += "    目标 = " + format_number(target)
        + "    变量 = " + extracted.name;
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "跳过 " + std::to_string(extracted.missing_count)
                + " 个缺失或非法单元格。"});
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::two_sample_equivalence(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() < 2) {
        return error_page("双样本等价性检验", "2-Sample Equivalence Test",
                          "请选择两列独立样本。");
    }
    if (!configuration.inference.equivalence_lower.has_value()
        || !configuration.inference.equivalence_upper.has_value()) {
        return error_page("双样本等价性检验", "2-Sample Equivalence Test",
                          "请指定等价下限和上限。");
    }
    const ExtractedNumericColumn first = extract_numeric_column(
        table, configuration.variable_columns[0], configuration.excluded_rows);
    const ExtractedNumericColumn second = extract_numeric_column(
        table, configuration.variable_columns[1], configuration.excluded_rows);
    const auto result = datalab::domain::statistics::two_sample_equivalence_test(
        first.values, second.values,
        *configuration.inference.equivalence_lower,
        *configuration.inference.equivalence_upper,
        configuration.inference.confidence_level,
        parse_variance_method(configuration.inference.variance_method));
    OutputPage page = make_equivalence_page(
        "双样本等价性检验", "2-Sample Equivalence Test", "two_sample_eq",
        configuration, result);
    page.parameter_summary += "    方法 = "
        + std::string(configuration.inference.variance_method == "pooled" ? "合并方差" : "Welch");
    if (first.missing_count > 0 || second.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "双样本等价性检验跳过缺失或非法单元格。"});
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::two_sample_equivalence_ratio(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() < 2) {
        return error_page("双样本均值比等价性检验", "2-Sample Equivalence Ratio Test",
                          "请选择检验列与参考列。");
    }
    if (!configuration.inference.equivalence_lower.has_value()
        || !configuration.inference.equivalence_upper.has_value()) {
        return error_page("双样本均值比等价性检验", "2-Sample Equivalence Ratio Test",
                          "请指定比值等价下限和上限。");
    }
    const ExtractedNumericColumn first = extract_numeric_column(
        table, configuration.variable_columns[0], configuration.excluded_rows);
    const ExtractedNumericColumn second = extract_numeric_column(
        table, configuration.variable_columns[1], configuration.excluded_rows);
    const bool use_log =
        configuration.inference.equivalence_ratio_transform == "log";
    const auto result = datalab::domain::statistics::two_sample_equivalence_ratio_test(
        first.values, second.values,
        *configuration.inference.equivalence_lower,
        *configuration.inference.equivalence_upper,
        configuration.inference.confidence_level,
        parse_variance_method(configuration.inference.variance_method),
        use_log);
    OutputPage page = make_equivalence_page(
        "双样本均值比等价性检验", "2-Sample Equivalence Ratio Test",
        "two_sample_eq_ratio", configuration, result);
    page.parameter_summary += "    方法 = "
        + std::string(configuration.inference.variance_method == "pooled" ? "合并方差" : "Welch")
        + "    变换 = " + std::string(use_log ? "log" : "none")
        + "    检验 = " + first.name + "    参考 = " + second.name;
    if (first.missing_count > 0 || second.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "均值比等价性检验跳过缺失或非法单元格。"});
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::paired_equivalence(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() < 2) {
        return error_page("配对等价性检验", "Paired Equivalence Test",
                          "请选择两列配对测量值。");
    }
    if (!configuration.inference.equivalence_lower.has_value()
        || !configuration.inference.equivalence_upper.has_value()) {
        return error_page("配对等价性检验", "Paired Equivalence Test",
                          "请指定等价下限和上限。");
    }
    const auto first = extract_numeric_column(
        table, configuration.variable_columns[0], configuration.excluded_rows);
    const auto second = extract_numeric_column(
        table, configuration.variable_columns[1], configuration.excluded_rows);
    const auto aligned = align_complete_rows_with_source({first, second});
    std::vector<double> first_values;
    std::vector<double> second_values;
    first_values.reserve(aligned.values.size());
    second_values.reserve(aligned.values.size());
    for (const auto& row : aligned.values) {
        first_values.push_back(row[0]);
        second_values.push_back(row[1]);
    }
    const auto result = datalab::domain::statistics::paired_equivalence_test(
        first_values, second_values,
        *configuration.inference.equivalence_lower,
        *configuration.inference.equivalence_upper,
        configuration.inference.confidence_level);
    OutputPage page = make_equivalence_page(
        "配对等价性检验", "Paired Equivalence Test", "paired_equivalence",
        configuration, result);
    page.parameter_summary += "    第一列 = " + first.name + "    第二列 = " + second.name;
    if (first.missing_count > 0 || second.missing_count > 0
        || first.total_count > aligned.source_rows.size()
        || second.total_count > aligned.source_rows.size()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "配对等价性检验按 complete-case 对齐，已跳过不成对的缺失或非法单元格。"});
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::one_proportion_equivalence(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.inference.first_events_column.has_value()
        || !configuration.inference.first_trials_column.has_value()) {
        return error_page("单比例等价性检验", "1-Proportion Equivalence Test",
                          "请选择事件数列和试验数列。");
    }
    if (!configuration.inference.hypothesis_mean.has_value()) {
        return error_page("单比例等价性检验", "1-Proportion Equivalence Test",
                          "请指定目标比例。");
    }
    if (!configuration.inference.equivalence_lower.has_value()
        || !configuration.inference.equivalence_upper.has_value()) {
        return error_page("单比例等价性检验", "1-Proportion Equivalence Test",
                          "请指定等价下限和上限。");
    }
    const auto events_column = extract_numeric_column(
        table, *configuration.inference.first_events_column, configuration.excluded_rows);
    const auto trials_column = extract_numeric_column(
        table, *configuration.inference.first_trials_column, configuration.excluded_rows);
    const auto summarized = sum_event_trial_columns(
        table, events_column, trials_column, configuration);
    if (!summarized.ok) {
        return error_page("单比例等价性检验", "1-Proportion Equivalence Test",
                          summarized.error);
    }
    const auto result = datalab::domain::statistics::one_proportion_equivalence_test(
        summarized.events, summarized.trials,
        *configuration.inference.hypothesis_mean,
        *configuration.inference.equivalence_lower,
        *configuration.inference.equivalence_upper,
        configuration.inference.confidence_level,
        summarized.row_count, summarized.missing);
    OutputPage page = make_equivalence_page(
        "单比例等价性检验", "1-Proportion Equivalence Test",
        "one_proportion_equivalence", configuration, result);
    page.parameter_summary += "    事件 = " + events_column.name
        + "    试验 = " + trials_column.name
        + "    目标比例 = "
        + format_number(*configuration.inference.hypothesis_mean);
    if (summarized.row_count > 1) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "summarized_from_multiple_rows",
            "已将多行事件数/试验数求和后再做单比例等价性检验。"});
    }
    if (summarized.missing > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "跳过 " + std::to_string(summarized.missing)
                + " 个缺失或非法单元格。"});
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::two_proportion_equivalence(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.inference.first_events_column.has_value()
        || !configuration.inference.first_trials_column.has_value()
        || !configuration.inference.second_events_column.has_value()
        || !configuration.inference.second_trials_column.has_value()) {
        return error_page("两比例等价性检验", "2-Proportion Equivalence Test",
                          "请选择两组事件数和试验数列。");
    }
    if (!configuration.inference.equivalence_lower.has_value()
        || !configuration.inference.equivalence_upper.has_value()) {
        return error_page("两比例等价性检验", "2-Proportion Equivalence Test",
                          "请指定等价下限和上限。");
    }
    const auto first_events = extract_numeric_column(
        table, *configuration.inference.first_events_column, configuration.excluded_rows);
    const auto first_trials = extract_numeric_column(
        table, *configuration.inference.first_trials_column, configuration.excluded_rows);
    const auto second_events = extract_numeric_column(
        table, *configuration.inference.second_events_column, configuration.excluded_rows);
    const auto second_trials = extract_numeric_column(
        table, *configuration.inference.second_trials_column, configuration.excluded_rows);
    const auto first = sum_event_trial_columns(
        table, first_events, first_trials, configuration);
    const auto second = sum_event_trial_columns(
        table, second_events, second_trials, configuration);
    if (!first.ok) {
        return error_page("两比例等价性检验", "2-Proportion Equivalence Test",
                          "第一组：" + first.error);
    }
    if (!second.ok) {
        return error_page("两比例等价性检验", "2-Proportion Equivalence Test",
                          "第二组：" + second.error);
    }
    const auto result = datalab::domain::statistics::two_proportion_equivalence_test(
        first.events, first.trials, second.events, second.trials,
        *configuration.inference.equivalence_lower,
        *configuration.inference.equivalence_upper,
        configuration.inference.confidence_level,
        first.row_count, second.row_count, first.missing, second.missing);
    OutputPage page = make_equivalence_page(
        "两比例等价性检验", "2-Proportion Equivalence Test",
        "two_proportion_equivalence", configuration, result);
    page.parameter_summary += "    第一组 = " + first_events.name + "/"
        + first_trials.name + "    第二组 = " + second_events.name + "/"
        + second_trials.name;
    if (first.row_count > 1 || second.row_count > 1) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "summarized_from_multiple_rows",
            "已将一组或多组的多行事件数/试验数求和后再做两比例等价性检验。"});
    }
    if (first.missing > 0 || second.missing > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "两比例等价性检验跳过了缺失或非法单元格。"});
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::one_way_anova(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.by_column.has_value()) {
        return error_page("单因素 ANOVA", "One-Way ANOVA", "请选择因子/分组列。");
    }
    const ExtractedNumericColumn response = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    const std::vector<std::string> labels = extract_text_column(
        table, *configuration.by_column);
    std::map<std::string, std::vector<double>> grouped;
    for (std::size_t index = 0; index < response.values.size(); ++index) {
        const std::size_t row = response.source_rows[index];
        if (row >= labels.size() || is_missing_cell(labels[row])) {
            return error_page("单因素 ANOVA", "One-Way ANOVA",
                              "因子列存在缺失标签，无法进行分组。原始行 "
                                  + std::to_string(row + 1));
        }
        grouped[labels[row]].push_back(response.values[index]);
    }
    std::vector<std::vector<double>> groups;
    std::vector<std::string> group_labels;
    for (const auto& [label, values] : grouped) {
        group_labels.push_back(label);
        groups.push_back(values);
    }
    const auto result = datalab::domain::statistics::one_way_anova(groups, group_labels);
    OutputPage page;
    page.id = new_id("anova");
    page.title = "单因素 ANOVA";
    page.method_name = "One-Way ANOVA";
    page.configuration = configuration;
    page.parameter_summary = "响应 = " + response.name
        + "    因子 = " + column_label(table, *configuration.by_column);
    page.diagnostics = result.diagnostics;
    if (response.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "ANOVA 跳过 " + std::to_string(response.missing_count)
                + " 个缺失或非法响应值。"});
    }
    StatisticTable means;
    means.title = "组均值";
    const double interval_confidence =
        configuration.inference.confidence_level > 0.0
            && configuration.inference.confidence_level < 1.0
            ? configuration.inference.confidence_level : 0.95;
    const bool interval_available = result.error_degrees_of_freedom > 0
        && result.error_mean_square > 0.0;
    const double interval_critical = interval_available
        ? datalab::domain::statistics::student_t_quantile(
            0.5 + interval_confidence / 2.0,
            static_cast<double>(result.error_degrees_of_freedom))
        : std::numeric_limits<double>::quiet_NaN();
    means.headers = interval_available
        ? std::vector<std::string>{"组", "N", "Mean", "StDev",
            std::to_string(static_cast<int>(interval_confidence * 100.0 + 0.5)) + "% CI"}
        : std::vector<std::string>{"组", "N", "Mean", "StDev"};
    std::vector<double> interval_lower;
    std::vector<double> interval_upper;
    std::vector<std::string> interval_labels;
    std::vector<double> interval_means;
    std::vector<std::size_t> interval_counts;
    for (const auto& group : result.groups) {
        std::vector<std::string> row = {
            group.label, std::to_string(group.count), format_number(group.mean),
            format_number(group.sample_standard_deviation)};
        if (interval_available && group.count > 0 && std::isfinite(interval_critical)) {
            const double half_width = interval_critical
                * std::sqrt(result.error_mean_square / static_cast<double>(group.count));
            const double lower = group.mean - half_width;
            const double upper = group.mean + half_width;
            row.push_back("[" + format_number(lower) + ", " + format_number(upper) + "]");
            interval_labels.push_back(group.label);
            interval_means.push_back(group.mean);
            interval_lower.push_back(lower);
            interval_upper.push_back(upper);
            interval_counts.push_back(group.count);
        }
        means.rows.push_back(std::move(row));
    }
    page.tables.push_back(means);
    StatisticTable anova;
    anova.title = "方差分析";
    anova.headers = {"来源", "DF", "SS", "MS", "F", "P-Value"};
    anova.rows = {
        {"组间", std::to_string(result.between_degrees_of_freedom),
         format_number(result.between_sum_of_squares), format_number(result.between_mean_square),
         format_number(result.f_statistic), result.p_value.has_value()
             ? format_number(*result.p_value) : "*"},
        {"误差", std::to_string(result.error_degrees_of_freedom),
         format_number(result.error_sum_of_squares), format_number(result.error_mean_square),
         "", ""},
        {"合计", std::to_string(result.total_degrees_of_freedom),
         format_number(result.total_sum_of_squares), "", "", ""}};
    page.tables.push_back(anova);
    const auto tukey = datalab::domain::statistics::tukey_multiple_comparisons(
        groups, group_labels, configuration.inference.confidence_level);
    append_diagnostics(page.diagnostics, tukey.diagnostics, "Tukey: ");
    bool tukey_grouping_available = false;
    std::size_t grouping_letter_count = 0;
    if (!tukey.comparisons.empty()) {
        StatisticTable comparisons;
        comparisons.title = "Tukey 同时比较";
        comparisons.headers = {"差值", "Difference", "SE Difference", "q",
                               "下限", "上限", "Adjusted P-Value", "显著",
                               "族置信水平", "误差 DF", "MSE", "方法"};
        std::vector<std::string> tukey_labels;
        std::vector<double> tukey_centers;
        std::vector<double> tukey_lower;
        std::vector<double> tukey_upper;
        for (const auto& comparison : tukey.comparisons) {
            comparisons.rows.push_back({
                comparison.first_label + " - " + comparison.second_label,
                format_number(comparison.mean_difference),
                format_number(comparison.standard_error),
                format_number(comparison.q_statistic),
                format_number(comparison.confidence_lower),
                format_number(comparison.confidence_upper),
                format_number(comparison.adjusted_p_value),
                comparison.significant ? "是（区间不含 0）" : "否（区间含 0）",
                format_number(tukey.family_confidence_level),
                format_number(tukey.error_degrees_of_freedom),
                format_number(tukey.error_mean_square),
                tukey.method});
            tukey_labels.push_back(comparison.first_label + " - " + comparison.second_label);
            tukey_centers.push_back(comparison.mean_difference);
            tukey_lower.push_back(comparison.confidence_lower);
            tukey_upper.push_back(comparison.confidence_upper);
        }
        page.tables.push_back(comparisons);
        PlotSpec tukey_interval;
        tukey_interval.kind = PlotKind::interval;
        tukey_interval.title = "Tukey 差值同时区间";
        tukey_interval.x_axis_title = "成对比较";
        tukey_interval.y_axis_title = "均值差";
        tukey_interval.categories = tukey_labels;
        tukey_interval.values = tukey_centers;
        tukey_interval.interval_lower = tukey_lower;
        tukey_interval.interval_upper = tukey_upper;
        PlotSeries zero;
        zero.label = "差 = 0";
        zero.style.point_style = PlotPointStyle::none;
        for (std::size_t index = 0; index < tukey_labels.size(); ++index) {
            zero.x_values.push_back(static_cast<double>(index + 1));
            zero.values.push_back(0.0);
        }
        zero.line_width = 1.0;
        tukey_interval.series.push_back(std::move(zero));
        page.plots.push_back(std::move(tukey_interval));

        std::vector<double> grouping_means;
        std::vector<std::size_t> grouping_counts;
        grouping_means.reserve(result.groups.size());
        grouping_counts.reserve(result.groups.size());
        std::vector<std::string> grouping_labels;
        grouping_labels.reserve(result.groups.size());
        for (const auto& group : result.groups) {
            grouping_labels.push_back(group.label);
            grouping_means.push_back(group.mean);
            grouping_counts.push_back(group.count);
        }
        const auto grouping = datalab::domain::statistics::tukey_grouping_letters(
            grouping_labels, grouping_means, grouping_counts, tukey.comparisons);
        if (!grouping.empty()) {
            StatisticTable grouping_table;
            grouping_table.title = "Grouping Information";
            grouping_table.headers = {"水平", "N", "均值", "Grouping"};
            for (const auto& row : grouping) {
                grouping_table.rows.push_back({
                    row.label,
                    std::to_string(row.count),
                    format_number(row.mean),
                    row.grouping});
                for (const char letter : row.grouping) {
                    grouping_letter_count = std::max(
                        grouping_letter_count,
                        static_cast<std::size_t>(letter - 'A' + 1));
                }
            }
            page.tables.push_back(std::move(grouping_table));
            tukey_grouping_available = true;
        }
    }
    if (!interval_means.empty()) {
        PlotSpec interval;
        interval.kind = PlotKind::interval;
        interval.title = "区间图";
        interval.x_axis_title = column_label(table, *configuration.by_column);
        interval.y_axis_title = response.name;
        interval.categories = interval_labels;
        interval.values = interval_means;
        interval.interval_lower = interval_lower;
        interval.interval_upper = interval_upper;
        interval.interval_counts = interval_counts;
        page.plots.push_back(std::move(interval));
    } else if (!interval_available) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "anova_interval_unavailable",
            "误差自由度或 MSE 不可用，未输出组均值区间。"});
    }
    std::map<std::string, double> group_means;
    for (const auto& group : result.groups) {
        group_means[group.label] = group.mean;
    }
    std::vector<double> ordered_fitted;
    std::vector<double> ordered_residuals;
    std::vector<std::size_t> ordered_rows;
    for (std::size_t index = 0; index < response.values.size(); ++index) {
        const std::size_t row = response.source_rows[index];
        if (row >= labels.size()) {
            continue;
        }
        const auto mean = group_means.find(labels[row]);
        if (mean == group_means.end()) {
            continue;
        }
        ordered_fitted.push_back(mean->second);
        ordered_residuals.push_back(response.values[index] - mean->second);
        ordered_rows.push_back(row);
    }
    if (!ordered_residuals.empty()) {
        PlotSpec residual_plot;
        residual_plot.kind = PlotKind::scatter;
        residual_plot.title = "残差与拟合值";
        residual_plot.x_axis_title = "拟合值";
        residual_plot.y_axis_title = "残差";
        residual_plot.x_values = ordered_fitted;
        residual_plot.values = ordered_residuals;
        residual_plot.source_rows = ordered_rows;
        add_zero_residual_reference(residual_plot);
        page.plots.push_back(std::move(residual_plot));
        PlotSpec order_plot;
        order_plot.kind = PlotKind::scatter;
        order_plot.title = "残差与观测顺序";
        order_plot.x_axis_title = "观测顺序";
        order_plot.y_axis_title = "残差";
        for (std::size_t index = 0; index < ordered_residuals.size(); ++index) {
            order_plot.x_values.push_back(static_cast<double>(index + 1));
            order_plot.values.push_back(ordered_residuals[index]);
        }
        order_plot.source_rows = ordered_rows;
        add_zero_residual_reference(order_plot);
        page.plots.push_back(std::move(order_plot));
        PlotSpec residual_probability;
        residual_probability.kind = PlotKind::probability;
        residual_probability.title = "残差正态概率图";
        residual_probability.x_axis_title = "理论分位数";
        residual_probability.y_axis_title = "残差";
        const auto probability = datalab::domain::statistics::normal_probability_plot(
            ordered_residuals, ordered_rows);
        residual_probability.x_values = probability.theoretical_quantiles;
        residual_probability.values = probability.ordered_values;
        residual_probability.source_rows = probability.source_rows;
        page.plots.push_back(std::move(residual_probability));
        const datalab::domain::statistics::HistogramResult bins =
            datalab::domain::statistics::histogram(ordered_residuals, 0);
        PlotSpec hist;
        hist.kind = PlotKind::histogram;
        hist.title = "残差直方图";
        hist.x_axis_title = "残差";
        hist.y_axis_title = "频数";
        hist.histogram_edges = bins.edges;
        hist.histogram_counts = bins.counts;
        hist.values = ordered_residuals;
        hist.source_rows = ordered_rows;
        page.plots.push_back(std::move(hist));
    }
    page.facts.anova = datalab::domain::statistics::one_way_anova_facts_from(
        result, tukey, tukey_grouping_available, grouping_letter_count);
    append_rule_table(page, page.facts.anova->rules);
    domain::apply_evidence(page.method_metadata, result.evidence);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::paired_t(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() < 2) {
        return error_page("配对 t 检验", "Paired t", "请选择两列配对测量值。");
    }
    const auto first = extract_numeric_column(
        table, configuration.variable_columns[0], configuration.excluded_rows);
    const auto second = extract_numeric_column(
        table, configuration.variable_columns[1], configuration.excluded_rows);
    const auto aligned = align_complete_rows_with_source({first, second});
    std::vector<double> first_values;
    std::vector<double> second_values;
    first_values.reserve(aligned.values.size());
    second_values.reserve(aligned.values.size());
    for (const auto& row : aligned.values) {
        first_values.push_back(row[0]);
        second_values.push_back(row[1]);
    }
    const auto result = datalab::domain::statistics::paired_t_test(
        first_values, second_values, configuration.inference.confidence_level,
        parse_alternative(configuration.inference.alternative));
    OutputPage page;
    page.id = new_id("paired_t");
    page.title = "配对 t 检验";
    page.method_name = "Paired t";
    page.configuration = configuration;
    page.parameter_summary = "第一列 = " + first.name + "    第二列 = " + second.name;
    page.diagnostics = result.diagnostics;
    StatisticTable output;
    output.title = "配对差值统计";
    output.headers = {"N", "Mean Difference", "StDev", "SE", "T", "DF",
                      "P-Value", "置信区间"};
    std::string interval = "*";
    if (result.confidence_lower.has_value() && result.confidence_upper.has_value()) {
        interval = "[" + format_number(*result.confidence_lower) + ", "
            + format_number(*result.confidence_upper) + "]";
    } else if (result.confidence_lower.has_value()) {
        interval = "[" + format_number(*result.confidence_lower) + ", +∞)";
    } else if (result.confidence_upper.has_value()) {
        interval = "(-∞, " + format_number(*result.confidence_upper) + "]";
    }
    output.rows.push_back({
        std::to_string(result.count), format_number(result.mean_difference),
        format_number(result.sample_standard_deviation), format_number(result.standard_error),
        format_number(result.t_statistic), format_number(result.degrees_of_freedom),
        result.p_value.has_value() ? format_number(*result.p_value) : "*", interval});
    page.tables.push_back(output);
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "配对测量散点图";
    plot.x_axis_title = first.name;
    plot.y_axis_title = second.name;
    plot.x_values = first_values;
    plot.values = second_values;
    plot.source_rows = aligned.source_rows;
    page.plots.push_back(plot);
    if (result.confidence_lower.has_value() && result.confidence_upper.has_value()) {
        page.plots.push_back(interval_plot_spec(
            "差值区间图", "差值", "Mean Difference",
            {"差值"},
            {result.mean_difference},
            {*result.confidence_lower},
            {*result.confidence_upper},
            {result.count},
            aligned.source_rows.empty()
                ? std::vector<std::size_t>{}
                : std::vector<std::size_t>{aligned.source_rows.front()}));
    } else {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "one_sided_interval_not_plotted",
            "单侧置信区间缺少一端，未输出差值区间图。"});
    }
    domain::TTestFacts facts;
    facts.kind = "paired";
    facts.n = result.count;
    facts.missing_count = first.total_count > aligned.source_rows.size()
        ? first.total_count - aligned.source_rows.size() : 0;
    facts.difference = result.mean_difference;
    facts.p_value = result.p_value;
    facts.ci_lower = result.confidence_lower;
    facts.ci_upper = result.confidence_upper;
    facts.assumption_status = "not_verified";
    page.facts.t_test = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::regression(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() < 2) {
        return error_page("线性回归", "Linear Regression",
                          "请选择一个响应变量和至少一个预测变量。");
    }
    const auto response = extract_numeric_column(
        table, configuration.variable_columns.front(), configuration.excluded_rows);
    std::vector<ExtractedNumericColumn> predictors;
    for (std::size_t index = 1; index < configuration.variable_columns.size(); ++index) {
        predictors.push_back(extract_numeric_column(
            table, configuration.variable_columns[index], configuration.excluded_rows));
    }
    std::vector<ExtractedNumericColumn> columns;
    columns.reserve(predictors.size() + 1);
    columns.push_back(response);
    columns.insert(columns.end(), predictors.begin(), predictors.end());
    const auto aligned = align_complete_rows_with_source(columns);
    std::vector<double> response_values;
    std::vector<std::vector<double>> predictor_values;
    response_values.reserve(aligned.values.size());
    predictor_values.reserve(aligned.values.size());
    for (const auto& row : aligned.values) {
        response_values.push_back(row[0]);
        predictor_values.emplace_back(row.begin() + 1, row.end());
    }
    std::vector<std::string> labels;
    for (std::size_t index = 1; index < configuration.variable_columns.size(); ++index) {
        const std::size_t column = configuration.variable_columns[index];
        labels.push_back(column < table.columns.size()
            ? table.columns[column]
            : column_label(table, column));
    }
    std::string predictor_summary;
    for (std::size_t index = 0; index < labels.size(); ++index) {
        if (index != 0) {
            predictor_summary += ", ";
        }
        predictor_summary += labels[index];
    }
    const auto result = datalab::domain::statistics::fit_linear_regression(
        response_values, predictor_values, labels,
        configuration.inference.confidence_level, aligned.source_rows);
    OutputPage page;
    page.id = new_id("regression");
    page.title = "线性回归";
    page.method_name = "Linear Regression";
    page.configuration = configuration;
    page.parameter_summary = "响应 = " + response.name
        + "    预测变量 = " + predictor_summary
        + "    有效观测 = " + std::to_string(aligned.values.size())
        + "    置信水平 = " + format_number(configuration.inference.confidence_level);
    page.diagnostics = result.diagnostics;
    std::size_t complete_case_drop_count = response.values.size() > aligned.values.size()
        ? response.values.size() - aligned.values.size()
        : 0;
    for (const auto& predictor : predictors) {
        if (predictor.values.size() > aligned.values.size()) {
            complete_case_drop_count = std::max(
                complete_case_drop_count, predictor.values.size() - aligned.values.size());
        }
    }
    if (complete_case_drop_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "回归使用 complete-case，跳过响应缺失、预测变量缺失或无效的行。"});
    }
    StatisticTable summary_table;
    summary_table.title = "模型摘要";
    summary_table.headers = {"S", "R-sq", "R-sq(adj)", "R-sq(pred)", "PRESS", "F",
                             "P-Value", "Durbin-Watson", "异常", "高杠杆", "影响"};
    summary_table.rows.push_back({
        format_number(result.residual_standard_deviation),
        format_number(result.r_squared), format_number(result.adjusted_r_squared),
        format_number(result.predicted_r_squared), format_number(result.press),
        format_number(result.f_statistic),
        result.model_p_value.has_value() ? format_number(*result.model_p_value) : "*",
        format_number(result.diagnostics_summary.durbin_watson),
        std::to_string(result.diagnostics_summary.outlier_count),
        std::to_string(result.diagnostics_summary.high_leverage_count),
        std::to_string(result.diagnostics_summary.influential_count)});
    page.tables.push_back(summary_table);
    StatisticTable dw_table;
    dw_table.title = "Durbin-Watson";
    dw_table.headers = {"DW", "dL (α=0.05)", "dU (α=0.05)", "判定区", "顺序"};
    dw_table.rows.push_back({
        format_number(result.diagnostics_summary.durbin_watson),
        format_optional(result.diagnostics_summary.durbin_watson_dl),
        format_optional(result.diagnostics_summary.durbin_watson_du),
        result.diagnostics_summary.durbin_watson_decision,
        result.diagnostics_summary.durbin_watson_order});
    page.tables.push_back(std::move(dw_table));
    StatisticTable coefficient_table;
    coefficient_table.title = "系数";
    coefficient_table.headers = {"项", "Coef", "SE Coef", "T", "P-Value", "置信区间", "VIF"};
    for (const auto& coefficient : result.coefficients) {
        coefficient_table.rows.push_back({
            coefficient.term, format_number(coefficient.coefficient),
            format_number(coefficient.standard_error), format_number(coefficient.t_statistic),
            coefficient.p_value.has_value() ? format_number(*coefficient.p_value) : "*",
            coefficient.confidence_lower.has_value() && coefficient.confidence_upper.has_value()
                ? "[" + format_number(*coefficient.confidence_lower) + ", "
                    + format_number(*coefficient.confidence_upper) + "]" : "*",
            coefficient.vif.has_value() ? format_number(*coefficient.vif) : ""});
    }
    page.tables.push_back(coefficient_table);
    StatisticTable anova_table;
    anova_table.title = "回归方差分析";
    anova_table.headers = {"来源", "Seq SS", "Adj SS", "DF", "MS", "F", "P-Value"};
    if (!result.anova_effects.empty()) {
        for (const auto& effect : result.anova_effects) {
            anova_table.rows.push_back({
                effect.term,
                effect.sequential_sum_of_squares.has_value()
                    ? format_number(*effect.sequential_sum_of_squares) : "*",
                effect.adjusted_sum_of_squares.has_value()
                    ? format_number(*effect.adjusted_sum_of_squares) : "*",
                std::to_string(effect.degrees_of_freedom),
                effect.mean_square.has_value() ? format_number(*effect.mean_square) : "*",
                effect.f_statistic.has_value() ? format_number(*effect.f_statistic) : "*",
                effect.p_value.has_value() ? format_number(*effect.p_value) : "*"});
        }
    } else {
        anova_table.rows.push_back({
            "回归", "*", format_number(result.regression_sum_of_squares),
            std::to_string(result.predictor_count),
            format_number(result.regression_mean_square), format_number(result.f_statistic),
            result.model_p_value.has_value() ? format_number(*result.model_p_value) : "*"});
    }
    anova_table.rows.push_back({
        "误差", "", format_number(result.error_sum_of_squares),
        std::to_string(result.observation_count - result.predictor_count - 1),
        format_number(result.error_mean_square), "", ""});
    anova_table.rows.push_back({
        "合计", "", format_number(result.total_sum_of_squares),
        std::to_string(result.observation_count - 1), "", "", ""});
    page.tables.push_back(anova_table);
    StatisticTable assumptions_table;
    assumptions_table.title = "假设检查";
    assumptions_table.headers = {"检查项", "状态", "统计量", "P-Value", "说明"};
    for (const auto& assumption : result.diagnostics_summary.assumptions) {
        assumptions_table.rows.push_back({
            assumption.name,
            assumption.status,
            assumption.statistic.has_value() ? format_number(*assumption.statistic) : "*",
            assumption.p_value.has_value() ? format_number(*assumption.p_value) : "*",
            assumption.evidence_summary});
    }
    page.tables.push_back(assumptions_table);
    StatisticTable diagnostics;
    diagnostics.title = "拟合与诊断";
    diagnostics.headers = {"观测", "原始行", "响应", "拟合值", "残差", "标准化残差",
                           "内部标准化残差", "学生化残差", "删除学生化残差",
                           "杠杆值", "Cook 距离", "DFITS", "诊断标记"};
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        const auto& observation = result.observations[index];
        diagnostics.rows.push_back({
            std::to_string(index + 1),
            std::to_string(observation.source_row + 1),
            format_number(observation.response),
            format_number(observation.fitted),
            format_number(observation.residual),
            format_number(observation.standardized_residual),
            format_number(observation.internally_standardized_residual),
            format_number(observation.studentized_residual),
            format_number(observation.deleted_studentized_residual),
            format_number(observation.leverage), format_number(observation.cooks_distance),
            format_number(observation.dfits),
            observation.diagnostic_flags.empty() ? std::string()
                : [&] {
                    std::string joined;
                    for (std::size_t flag = 0; flag < observation.diagnostic_flags.size(); ++flag) {
                        if (flag > 0) {
                            joined += ";";
                        }
                        joined += observation.diagnostic_flags[flag];
                    }
                    return joined;
                }()});
    }
    page.tables.push_back(diagnostics);
    StatisticTable unusual;
    unusual.title = "异常观测";
    unusual.headers = {"观测", "原始行", "响应", "拟合值", "残差", "标准化残差",
                       "杠杆", "Cook", "DFITS", "标记"};
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        const auto& observation = result.observations[index];
        if (!observation.unusual_r && !observation.unusual_x
            && !observation.is_influential) {
            continue;
        }
        std::string marks;
        if (observation.unusual_r) {
            marks += "R";
        }
        if (observation.unusual_x) {
            marks += "X";
        }
        if (observation.is_influential) {
            marks += "I";
        }
        unusual.rows.push_back({
            std::to_string(index + 1),
            std::to_string(observation.source_row + 1),
            format_number(observation.response),
            format_number(observation.fitted),
            format_number(observation.residual),
            format_number(observation.internally_standardized_residual),
            format_number(observation.leverage),
            format_number(observation.cooks_distance),
            format_number(observation.dfits),
            marks});
    }
    if (!unusual.rows.empty()) {
        page.tables.push_back(std::move(unusual));
    }
    if (result.predictor_count == 1 && !labels.empty()) {
        page.plots.push_back(make_fitted_line_plot(
            result, labels.front(), response.name, aligned.source_rows));
    }
    PlotSpec residual_plot;
    residual_plot.kind = PlotKind::scatter;
    residual_plot.title = "残差与拟合值";
    residual_plot.x_axis_title = "拟合值";
    residual_plot.y_axis_title = "残差";
    for (const auto& observation : result.observations) {
        residual_plot.x_values.push_back(observation.fitted);
        residual_plot.values.push_back(observation.residual);
    }
    residual_plot.source_rows = aligned.source_rows;
    add_zero_residual_reference(residual_plot);
    page.plots.push_back(residual_plot);
    PlotSpec order_plot;
    order_plot.kind = PlotKind::scatter;
    order_plot.title = "残差与观测顺序";
    order_plot.x_axis_title = "观测顺序";
    order_plot.y_axis_title = "残差";
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        order_plot.x_values.push_back(static_cast<double>(index + 1));
        order_plot.values.push_back(result.observations[index].residual);
    }
    order_plot.source_rows = aligned.source_rows;
    add_zero_residual_reference(order_plot);
    page.plots.push_back(order_plot);
    for (std::size_t predictor_index = 0; predictor_index < labels.size(); ++predictor_index) {
        PlotSpec predictor_plot;
        predictor_plot.kind = PlotKind::scatter;
        predictor_plot.title = "残差与预测变量 - " + labels[predictor_index];
        predictor_plot.x_axis_title = labels[predictor_index];
        predictor_plot.y_axis_title = "残差";
        for (std::size_t index = 0; index < predictor_values.size()
             && index < result.observations.size(); ++index) {
            if (predictor_index < predictor_values[index].size()) {
                predictor_plot.x_values.push_back(predictor_values[index][predictor_index]);
                predictor_plot.values.push_back(result.observations[index].residual);
            }
        }
        predictor_plot.source_rows = aligned.source_rows;
        add_zero_residual_reference(predictor_plot);
        page.plots.push_back(std::move(predictor_plot));
    }
    PlotSpec residual_probability;
    residual_probability.kind = PlotKind::probability;
    residual_probability.title = "残差正态概率图";
    residual_probability.x_axis_title = "理论分位数";
    residual_probability.y_axis_title = "残差";
    {
        std::vector<double> residuals;
        residuals.reserve(result.observations.size());
        for (const auto& observation : result.observations) {
            residuals.push_back(observation.residual);
        }
        const auto probability = datalab::domain::statistics::normal_probability_plot(
            residuals, aligned.source_rows);
        residual_probability.x_values = probability.theoretical_quantiles;
        residual_probability.values = probability.ordered_values;
        residual_probability.source_rows = probability.source_rows;
    }
    page.plots.push_back(residual_probability);
    if (result.diagnostics_summary.residual_normality.has_value()) {
        const auto& normality = *result.diagnostics_summary.residual_normality;
        page.interpretation.push_back({
            "残差正态性",
            {"Anderson-Darling = " + format_optional(normality.anderson_darling)
                 + (normality.decision == "fail_to_reject"
                        ? "，在 alpha 下未拒绝残差正态假设，不能据此宣称模型合格。"
                        : "，请结合正态概率图和 P 值判断残差正态性，不能只看 R²。")},
            DiagnosticMessage::Severity::info});
    }
    page.facts.regression = datalab::domain::statistics::regression_facts_from(result);
    if (page.facts.regression.has_value()) {
        page.facts.regression->residual_plot_count = page.plots.size();
    }
    append_rule_table(page, page.facts.regression->rules);
    domain::apply_evidence(page.method_metadata, result.evidence);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::two_proportions(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.inference.first_events_column.has_value()
        || !configuration.inference.first_trials_column.has_value()
        || !configuration.inference.second_events_column.has_value()
        || !configuration.inference.second_trials_column.has_value()) {
        return error_page("两比例检验", "2 Proportions",
                          "请选择两组事件数和试验数列。");
    }
    const auto first_events = extract_numeric_column(
        table, *configuration.inference.first_events_column, configuration.excluded_rows);
    const auto first_trials = extract_numeric_column(
        table, *configuration.inference.first_trials_column, configuration.excluded_rows);
    const auto second_events = extract_numeric_column(
        table, *configuration.inference.second_events_column, configuration.excluded_rows);
    const auto second_trials = extract_numeric_column(
        table, *configuration.inference.second_trials_column, configuration.excluded_rows);
    const auto first = sum_event_trial_columns(
        table, first_events, first_trials, configuration);
    const auto second = sum_event_trial_columns(
        table, second_events, second_trials, configuration);
    if (!first.ok) {
        return error_page("两比例检验", "2 Proportions",
                          "第一组：" + first.error);
    }
    if (!second.ok) {
        return error_page("两比例检验", "2 Proportions",
                          "第二组：" + second.error);
    }
    datalab::domain::statistics::TwoProportionCiMethod ci_method =
        datalab::domain::statistics::TwoProportionCiMethod::wald;
    if (configuration.inference.proportion_method == "wilson") {
        ci_method = datalab::domain::statistics::TwoProportionCiMethod::newcombe_wilson;
    } else if (configuration.inference.proportion_method == "agresti_coull") {
        ci_method = datalab::domain::statistics::TwoProportionCiMethod::agresti_coull;
    }
    const auto result = datalab::domain::statistics::two_proportions_test(
        first.events, first.trials, second.events, second.trials,
        configuration.inference.confidence_level,
        parse_alternative(configuration.inference.alternative),
        ci_method);
    OutputPage page;
    page.id = new_id("two_proportions");
    page.title = "两比例检验";
    page.method_name = "2 Proportions";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    if (first.row_count > 1 || second.row_count > 1) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "summarized_from_multiple_rows",
            "已将一组或多组的多行事件数/试验数求和后再做两比例检验。"});
    }
    if (first.missing > 0 || second.missing > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "两比例检验跳过了缺失或非法单元格。"});
    }
    page.parameter_summary = "第一组 = " + first_events.name + "/" + first_trials.name
        + "    第二组 = " + second_events.name + "/" + second_trials.name;
    StatisticTable output;
    output.title = "两比例检验";
    output.headers = {"组", "事件数", "试验数", "比例", "N*", "行数"};
    output.rows.push_back({"第一组", std::to_string(result.first_events),
                           std::to_string(result.first_trials),
                           format_number(result.first_proportion),
                           std::to_string(first.missing),
                           std::to_string(first.row_count)});
    output.rows.push_back({"第二组", std::to_string(result.second_events),
                           std::to_string(result.second_trials),
                           format_number(result.second_proportion),
                           std::to_string(second.missing),
                           std::to_string(second.row_count)});
    output.rows.push_back({"差值", "", "", format_number(result.difference), "", ""});
    page.tables.push_back(output);
    StatisticTable test;
    test.title = "检验结果";
    test.headers = {"方法", "Z", "P-Value", "置信区间", "Fisher P-Value"};
    const std::string interval = result.confidence_lower.has_value()
        && result.confidence_upper.has_value()
        ? "[" + format_number(*result.confidence_lower) + ", "
            + format_number(*result.confidence_upper) + "]" : "*";
    test.rows.push_back(std::vector<std::string>{
        result.method,
        format_number(result.z_statistic),
        result.p_value.has_value() ? format_number(*result.p_value) : "*",
        interval,
        result.fisher_p_value.has_value()
            ? format_number(*result.fisher_p_value) : "*"});
    page.tables.push_back(test);
    if (result.confidence_lower.has_value() && result.confidence_upper.has_value()) {
        PlotSpec interval_plot;
        interval_plot.kind = PlotKind::interval;
        interval_plot.title = "差值置信区间";
        interval_plot.x_axis_title = "对比";
        interval_plot.y_axis_title = "p1 - p2";
        interval_plot.categories = {"p1 - p2"};
        interval_plot.values = {result.difference};
        interval_plot.interval_lower = {*result.confidence_lower};
        interval_plot.interval_upper = {*result.confidence_upper};
        page.plots.push_back(std::move(interval_plot));
    }
    domain::ProportionFacts facts;
    facts.kind = "two_sample";
    facts.events = result.first_events;
    facts.trials = result.first_trials;
    facts.proportion = result.first_proportion;
    facts.second_events = result.second_events;
    facts.second_trials = result.second_trials;
    facts.second_proportion = result.second_proportion;
    facts.difference = result.difference;
    facts.method = result.method;
    facts.ci_method = result.ci_method;
    facts.p_value = result.p_value;
    facts.fisher_p_value = result.fisher_p_value;
    facts.ci_lower = result.confidence_lower;
    facts.ci_upper = result.confidence_upper;
    facts.assumption_status = "not_verified";
    page.facts.proportion = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::one_proportion(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.inference.first_events_column.has_value()
        || !configuration.inference.first_trials_column.has_value()) {
        return error_page("单比例检验", "1 Proportion",
                          "请选择事件数列和试验数列。");
    }
    if (!configuration.inference.hypothesis_mean.has_value()) {
        return error_page("单比例检验", "1 Proportion", "请指定假设比例。");
    }
    const auto events_column = extract_numeric_column(
        table, *configuration.inference.first_events_column, configuration.excluded_rows);
    const auto trials_column = extract_numeric_column(
        table, *configuration.inference.first_trials_column, configuration.excluded_rows);
    const auto aligned = align_complete_rows_with_source({events_column, trials_column});
    if (aligned.values.empty()) {
        return error_page("单比例检验", "1 Proportion",
                          "没有可用于单比例检验的 complete-case 行。");
    }
    std::size_t events = 0;
    std::size_t trials = 0;
    for (const auto& row : aligned.values) {
        std::vector<std::size_t> counts;
        if (!append_nonnegative_counts(row, counts) || counts.size() != 2
            || counts[0] > counts[1] || counts[1] == 0) {
            return error_page("单比例检验", "1 Proportion",
                              "事件数和试验数必须为非负整数，且事件数不能超过试验数。");
        }
        events += counts[0];
        trials += counts[1];
    }
    const std::size_t eligible = table.rows.size() >= configuration.excluded_rows.size()
        ? table.rows.size() - configuration.excluded_rows.size() : table.rows.size();
    const std::size_t missing = eligible > aligned.values.size()
        ? eligible - aligned.values.size() : 0;
    const auto method = configuration.inference.proportion_method == "normal"
        ? datalab::domain::statistics::ProportionMethod::normal
        : configuration.inference.proportion_method == "wilson"
            ? datalab::domain::statistics::ProportionMethod::wilson
            : configuration.inference.proportion_method == "agresti_coull"
                ? datalab::domain::statistics::ProportionMethod::agresti_coull
                : datalab::domain::statistics::ProportionMethod::exact;
    const auto result = datalab::domain::statistics::one_proportion_test(
        events, trials, *configuration.inference.hypothesis_mean,
        configuration.inference.confidence_level,
        parse_alternative(configuration.inference.alternative), method,
        aligned.values.size(), missing);
    OutputPage page;
    page.id = new_id("one_proportion");
    page.title = "单比例检验";
    page.method_name = "1 Proportion";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    page.parameter_summary = "事件 = " + events_column.name
        + "    试验 = " + trials_column.name
        + "    假设比例 = " + format_number(*configuration.inference.hypothesis_mean)
        + "    方法 = " + result.method
        + "    CI = " + result.ci_method;
    StatisticTable summary;
    summary.title = "单比例描述";
    summary.headers = {"事件数", "试验数", "比例", "N*", "行数"};
    summary.rows.push_back({
        std::to_string(result.events), std::to_string(result.trials),
        format_number(result.proportion), std::to_string(result.missing_count),
        std::to_string(result.row_count)});
    page.tables.push_back(std::move(summary));
    StatisticTable test;
    test.title = "检验结果";
    test.headers = {"方法", "CI 方法", "Z", "P-Value", "置信区间"};
    const std::string interval = result.confidence_lower.has_value()
        || result.confidence_upper.has_value()
        ? "[" + format_optional(result.confidence_lower) + ", "
            + format_optional(result.confidence_upper) + "]"
        : "*";
    test.rows.push_back({
        result.method,
        result.ci_method,
        format_optional(result.z_statistic),
        format_optional(result.p_value),
        interval});
    page.tables.push_back(std::move(test));
    domain::ProportionFacts facts;
    facts.kind = "one_sample";
    facts.events = result.events;
    facts.trials = result.trials;
    facts.proportion = result.proportion;
    facts.hypothesized = result.hypothesized;
    facts.method = result.method;
    facts.ci_method = result.ci_method;
    facts.p_value = result.p_value;
    facts.ci_lower = result.confidence_lower;
    facts.ci_upper = result.confidence_upper;
    facts.assumption_status = "not_verified";
    page.facts.proportion = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::one_poisson_rate(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.inference.first_events_column.has_value()) {
        return error_page("单样本泊松率", "1-Sample Poisson Rate",
                          "请选择缺陷计数列。");
    }
    if (!configuration.inference.first_trials_column.has_value()
        && !configuration.inspected_constant.has_value()) {
        return error_page("单样本泊松率", "1-Sample Poisson Rate",
                          "请选择观测长度列或输入观测长度常数。");
    }
    if (!configuration.inference.hypothesis_mean.has_value()) {
        return error_page("单样本泊松率", "1-Sample Poisson Rate",
                          "请指定假设发生率。");
    }
    const auto events_column = extract_numeric_column(
        table, *configuration.inference.first_events_column, configuration.excluded_rows);
    std::size_t events = 0;
    double exposure = 0.0;
    std::size_t row_count = 0;
    std::size_t missing = 0;
    if (configuration.inference.first_trials_column.has_value()) {
        const auto exposure_column = extract_numeric_column(
            table, *configuration.inference.first_trials_column, configuration.excluded_rows);
        const auto aligned = align_complete_rows_with_source({events_column, exposure_column});
        if (aligned.values.empty()) {
            return error_page("单样本泊松率", "1-Sample Poisson Rate",
                              "没有可用于单样本泊松率检验的 complete-case 行。");
        }
        for (const auto& row : aligned.values) {
            std::vector<std::size_t> counts;
            if (!append_nonnegative_counts({row[0]}, counts) || counts.size() != 1) {
                return error_page("单样本泊松率", "1-Sample Poisson Rate",
                                  "缺陷数必须为非负整数。");
            }
            if (!(row[1] > 0.0) || !std::isfinite(row[1])) {
                return error_page("单样本泊松率", "1-Sample Poisson Rate",
                                  "观测长度必须为正数。");
            }
            events += counts[0];
            exposure += row[1];
        }
        row_count = aligned.values.size();
        const std::size_t eligible = table.rows.size() >= configuration.excluded_rows.size()
            ? table.rows.size() - configuration.excluded_rows.size() : table.rows.size();
        missing = eligible > row_count ? eligible - row_count : 0;
    } else {
        if (events_column.values.empty()) {
            return error_page("单样本泊松率", "1-Sample Poisson Rate",
                              "没有可用于单样本泊松率检验的 complete-case 行。");
        }
        std::vector<std::size_t> counts;
        if (!append_nonnegative_counts(events_column.values, counts)
            || counts.size() != events_column.values.size()) {
            return error_page("单样本泊松率", "1-Sample Poisson Rate",
                              "缺陷数必须为非负整数。");
        }
        for (const std::size_t count : counts) {
            events += count;
        }
        const double length = static_cast<double>(*configuration.inspected_constant);
        if (!(length > 0.0)) {
            return error_page("单样本泊松率", "1-Sample Poisson Rate",
                              "观测长度必须为正数。");
        }
        exposure = length * static_cast<double>(counts.size());
        row_count = counts.size();
        missing = events_column.missing_count;
    }
    const auto method = configuration.inference.proportion_method == "normal"
        ? datalab::domain::statistics::ProportionMethod::normal
        : datalab::domain::statistics::ProportionMethod::exact;
    const auto result = datalab::domain::statistics::one_poisson_rate_test(
        events, exposure, *configuration.inference.hypothesis_mean,
        configuration.inference.confidence_level,
        parse_alternative(configuration.inference.alternative), method,
        row_count, missing);
    OutputPage page;
    page.id = new_id("one_poisson_rate");
    page.title = "单样本泊松率";
    page.method_name = "1-Sample Poisson Rate";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    page.parameter_summary = "缺陷 = " + events_column.name
        + "    假设发生率 = " + format_number(*configuration.inference.hypothesis_mean)
        + "    方法 = " + result.method;
    StatisticTable summary;
    summary.title = "泊松率描述";
    summary.headers = {"事件数", "观测长度", "率", "N*", "行数"};
    summary.rows.push_back({
        std::to_string(result.events), format_number(result.exposure),
        format_number(result.rate), std::to_string(result.missing_count),
        std::to_string(result.row_count)});
    page.tables.push_back(std::move(summary));
    StatisticTable test;
    test.title = "检验结果";
    test.headers = {"方法", "Z", "P-Value", "置信区间"};
    const std::string interval = result.confidence_lower.has_value()
        || result.confidence_upper.has_value()
        ? "[" + format_optional(result.confidence_lower) + ", "
            + format_optional(result.confidence_upper) + "]"
        : "*";
    test.rows.push_back({
        result.method,
        format_optional(result.z_statistic),
        format_optional(result.p_value),
        interval});
    page.tables.push_back(std::move(test));
    domain::PoissonRateFacts facts;
    facts.kind = "one_sample";
    facts.events = result.events;
    facts.exposure = result.exposure;
    facts.rate = result.rate;
    facts.hypothesized = result.hypothesized;
    facts.method = result.method;
    facts.z_statistic = result.z_statistic;
    facts.p_value = result.p_value;
    facts.ci_lower = result.confidence_lower;
    facts.ci_upper = result.confidence_upper;
    facts.assumption_status = "not_verified";
    page.facts.poisson_rate = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::two_poisson_rate(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.inference.first_events_column.has_value()
        || !configuration.inference.first_trials_column.has_value()
        || !configuration.inference.second_events_column.has_value()
        || !configuration.inference.second_trials_column.has_value()) {
        return error_page("双样本泊松率", "2-Sample Poisson Rate",
                          "请选择两组缺陷计数和观测长度列。");
    }
    const auto first_events = extract_numeric_column(
        table, *configuration.inference.first_events_column, configuration.excluded_rows);
    const auto first_exposure = extract_numeric_column(
        table, *configuration.inference.first_trials_column, configuration.excluded_rows);
    const auto second_events = extract_numeric_column(
        table, *configuration.inference.second_events_column, configuration.excluded_rows);
    const auto second_exposure = extract_numeric_column(
        table, *configuration.inference.second_trials_column, configuration.excluded_rows);
    auto count_at = [](const ExtractedNumericColumn& column) {
        std::vector<std::size_t> counts;
        append_nonnegative_counts(column.values, counts);
        return counts;
    };
    const auto first_event_values = count_at(first_events);
    const auto second_event_values = count_at(second_events);
    if (first_event_values.size() != 1 || first_exposure.values.size() != 1
        || second_event_values.size() != 1 || second_exposure.values.size() != 1) {
        return error_page("双样本泊松率", "2-Sample Poisson Rate",
                          "双样本泊松率当前要求每组使用一行汇总计数。");
    }
    const auto method = configuration.inference.proportion_method == "normal"
        ? datalab::domain::statistics::ProportionMethod::normal
        : datalab::domain::statistics::ProportionMethod::exact;
    const auto result = datalab::domain::statistics::two_poisson_rate_test(
        first_event_values.front(), first_exposure.values.front(),
        second_event_values.front(), second_exposure.values.front(),
        configuration.inference.confidence_level,
        parse_alternative(configuration.inference.alternative), method,
        configuration.inference.rate_comparison);
    OutputPage page;
    page.id = new_id("two_poisson_rate");
    page.title = "双样本泊松率";
    page.method_name = "2-Sample Poisson Rate";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    page.parameter_summary = "第一组 = " + first_events.name + "/" + first_exposure.name
        + "    第二组 = " + second_events.name + "/" + second_exposure.name
        + "    方法 = " + result.method
        + "    比较 = " + result.comparison;
    StatisticTable output;
    output.title = "双样本泊松率描述";
    output.headers = {"组", "事件数", "观测长度", "率"};
    output.rows.push_back({
        "第一组", std::to_string(result.first_events),
        format_number(result.first_exposure), format_number(result.first_rate)});
    output.rows.push_back({
        "第二组", std::to_string(result.second_events),
        format_number(result.second_exposure), format_number(result.second_rate)});
    if (result.comparison == "ratio") {
        output.rows.push_back({
            "率比", "", "",
            result.ratio.has_value() ? format_number(*result.ratio) : "*"});
    } else {
        output.rows.push_back({"差值", "", "", format_number(result.difference)});
    }
    page.tables.push_back(std::move(output));
    StatisticTable test;
    test.title = "检验结果";
    test.headers = {"方法", "Z", "P-Value", "置信区间", "CI 方法"};
    const std::string interval = result.confidence_lower.has_value()
        || result.confidence_upper.has_value()
        ? "[" + format_optional(result.confidence_lower) + ", "
            + format_optional(result.confidence_upper) + "]"
        : "*";
    test.rows.push_back({
        result.method,
        format_optional(result.z_statistic),
        format_optional(result.p_value),
        interval,
        result.ci_method});
    page.tables.push_back(std::move(test));
    if (result.comparison == "ratio" && result.ratio.has_value()
        && result.confidence_lower.has_value() && result.confidence_upper.has_value()) {
        PlotSpec interval_plot;
        interval_plot.kind = PlotKind::interval;
        interval_plot.title = "率比置信区间";
        interval_plot.x_axis_title = "率比";
        interval_plot.y_axis_title = "λ1/λ2";
        interval_plot.categories = {"率比"};
        interval_plot.values = {*result.ratio};
        interval_plot.interval_lower = {*result.confidence_lower};
        interval_plot.interval_upper = {*result.confidence_upper};
        PlotSeries one_line;
        one_line.label = "ρ=1";
        one_line.x_values = {0.0, 1.0};
        one_line.values = {1.0, 1.0};
        interval_plot.series = {std::move(one_line)};
        page.plots.push_back(std::move(interval_plot));
    }
    domain::PoissonRateFacts facts;
    facts.kind = "two_sample";
    facts.events = result.first_events;
    facts.exposure = result.first_exposure;
    facts.rate = result.first_rate;
    facts.second_events = result.second_events;
    facts.second_exposure = result.second_exposure;
    facts.second_rate = result.second_rate;
    facts.method = result.method;
    facts.comparison = result.comparison;
    facts.ratio = result.ratio;
    facts.z_statistic = result.z_statistic;
    facts.p_value = result.p_value;
    facts.ci_lower = result.confidence_lower;
    facts.ci_upper = result.confidence_upper;
    if (result.comparison == "ratio") {
        facts.ratio_ci_lower = result.confidence_lower;
        facts.ratio_ci_upper = result.confidence_upper;
    }
    facts.assumption_status = "not_verified";
    page.facts.poisson_rate = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::chi_square(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.inference.row_category_column.has_value()
        || !configuration.inference.column_category_column.has_value()) {
        return error_page("列联表卡方", "Chi-Square Association",
                          "请选择行分类列和列分类列。");
    }
    const auto rows = extract_text_column(table, *configuration.inference.row_category_column);
    const auto columns = extract_text_column(table, *configuration.inference.column_category_column);
    std::map<std::string, std::size_t> row_indices;
    std::map<std::string, std::size_t> column_indices;
    std::vector<std::vector<double>> observed;
    std::size_t missing_count = 0;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (row >= rows.size() || row >= columns.size()
            || is_missing_cell(rows[row]) || is_missing_cell(columns[row])) {
            ++missing_count;
            continue;
        }
        const auto row_result = row_indices.emplace(rows[row], row_indices.size());
        const auto column_result = column_indices.emplace(columns[row], column_indices.size());
        if (row_result.second) {
            observed.emplace_back(column_indices.size(), 0.0);
        }
        for (auto& values : observed) {
            values.resize(column_indices.size(), 0.0);
        }
        observed[row_result.first->second][column_result.first->second] += 1.0;
    }
    std::vector<std::string> row_labels(row_indices.size());
    for (const auto& [label, index] : row_indices) {
        row_labels[index] = label;
    }
    std::vector<std::string> column_labels(column_indices.size());
    for (const auto& [label, index] : column_indices) {
        column_labels[index] = label;
    }
    const auto result = datalab::domain::statistics::chi_square_association(
        observed, row_labels, column_labels);
    OutputPage page;
    page.id = new_id("chi_square");
    page.title = "列联表卡方";
    page.method_name = "Chi-Square Association";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    double total_count = 0.0;
    std::vector<double> row_totals(observed.size(), 0.0);
    std::vector<double> column_totals(
        observed.empty() ? 0 : observed.front().size(), 0.0);
    for (std::size_t row = 0; row < observed.size(); ++row) {
        for (std::size_t column = 0; column < observed[row].size(); ++column) {
            row_totals[row] += observed[row][column];
            column_totals[column] += observed[row][column];
            total_count += observed[row][column];
        }
    }
    page.parameter_summary =
        "行: " + column_label(table, *configuration.inference.row_category_column)
        + "    列: " + column_label(table, *configuration.inference.column_category_column)
        + "    N = " + format_number(total_count)
        + "    N* = " + std::to_string(missing_count);
    StatisticTable counts;
    counts.title = "观察频数";
    counts.headers.push_back("");
    counts.headers.insert(counts.headers.end(), column_labels.cbegin(), column_labels.cend());
    counts.headers.push_back("合计");
    for (std::size_t row = 0; row < observed.size(); ++row) {
        std::vector<std::string> out_row;
        out_row.push_back(row_labels[row]);
        for (std::size_t column = 0; column < observed[row].size(); ++column) {
            out_row.push_back(format_number(observed[row][column]));
        }
        out_row.push_back(format_number(row_totals[row]));
        counts.rows.push_back(std::move(out_row));
    }
    std::vector<std::string> total_row{"合计"};
    for (const double column_total : column_totals) {
        total_row.push_back(format_number(column_total));
    }
    total_row.push_back(format_number(total_count));
    counts.rows.push_back(std::move(total_row));
    page.tables.push_back(counts);
    StatisticTable output;
    output.title = "卡方检验";
    output.headers = {"Pearson χ²", "DF", "P-Value", "Likelihood Ratio χ²", "P-Value"};
    output.rows.push_back({
        format_number(result.pearson_statistic), format_number(result.degrees_of_freedom),
        result.p_value.has_value() ? format_number(*result.p_value) : "*",
        format_number(result.likelihood_ratio_statistic),
        result.likelihood_ratio_p_value.has_value()
            ? format_number(*result.likelihood_ratio_p_value) : "*"});
    page.tables.push_back(output);
    StatisticTable cells;
    cells.title = "单元格统计";
    cells.headers = {"行", "列", "Observed", "Expected", "Raw Residual",
                     "Standardized Residual", "Adjusted Residual", "Contribution"};
    for (const auto& cell : result.cells) {
        cells.rows.push_back({
            cell.row_label, cell.column_label, format_number(cell.observed),
            format_number(cell.expected), format_number(cell.raw_residual),
            format_number(cell.standardized_residual), format_number(cell.adjusted_residual),
            format_number(cell.contribution)});
    }
    page.tables.push_back(cells);
    domain::ChiSquareFacts facts;
    facts.statistic = result.pearson_statistic;
    facts.p_value = result.p_value;
    facts.degrees_of_freedom = result.degrees_of_freedom;
    facts.expected_count_warning = std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const DiagnosticMessage& diagnostic) {
            return diagnostic.code.find("expected_count") != std::string::npos;
        });
    facts.row_count = result.rows;
    facts.column_count = result.columns;
    facts.total_count = static_cast<std::size_t>(total_count);
    facts.missing_count = missing_count;
    facts.likelihood_ratio_statistic = result.likelihood_ratio_statistic;
    facts.likelihood_ratio_p_value = result.likelihood_ratio_p_value;
    if (total_count > 0.0) {
        auto append_percent = [&](const char* title,
                                  const std::function<double(double, std::size_t, std::size_t)>& pct) {
            StatisticTable percent;
            percent.title = title;
            percent.headers.push_back("");
            percent.headers.insert(percent.headers.end(),
                                   column_labels.cbegin(), column_labels.cend());
            for (std::size_t row = 0; row < observed.size(); ++row) {
                std::vector<std::string> out_row;
                out_row.push_back(row_labels[row]);
                for (std::size_t column = 0; column < observed[row].size(); ++column) {
                    out_row.push_back(format_number(
                        pct(observed[row][column], row, column), 4));
                }
                percent.rows.push_back(std::move(out_row));
            }
            page.tables.push_back(std::move(percent));
        };
        append_percent("行百分比", [&](double value, std::size_t row, std::size_t) {
            return row_totals[row] > 0.0 ? 100.0 * value / row_totals[row] : 0.0;
        });
        append_percent("列百分比", [&](double value, std::size_t, std::size_t column) {
            return column_totals[column] > 0.0 ? 100.0 * value / column_totals[column] : 0.0;
        });
        append_percent("合计百分比", [&](double value, std::size_t, std::size_t) {
            return 100.0 * value / total_count;
        });
        facts.percent_tables_available = true;
    }
    double max_abs_adj = -1.0;
    double max_contribution = -1.0;
    for (const auto& cell : result.cells) {
        const double magnitude = std::abs(cell.adjusted_residual);
        if (magnitude > max_abs_adj) {
            max_abs_adj = magnitude;
            facts.max_abs_adjusted_residual = magnitude;
        }
        if (cell.contribution > max_contribution) {
            max_contribution = cell.contribution;
            facts.largest_contribution_cell =
                cell.row_label + " × " + cell.column_label;
        }
    }
    if (!observed.empty() && !observed.front().empty()) {
        PlotSpec heatmap;
        heatmap.kind = PlotKind::heatmap;
        heatmap.title = "观察频数热图";
        heatmap.x_axis_title = column_label(table, *configuration.inference.column_category_column);
        heatmap.y_axis_title = column_label(table, *configuration.inference.row_category_column);
        heatmap.categories = row_labels;
        heatmap.matrix_labels = column_labels;
        heatmap.matrix_values = observed;
        double max_count = 0.0;
        for (const auto& row : observed) {
            for (const double value : row) {
                max_count = std::max(max_count, value);
            }
        }
        heatmap.color_min = 0.0;
        heatmap.color_max = max_count > 0.0 ? max_count : 1.0;
        page.plots.push_back(std::move(heatmap));
        facts.plot_available = true;

        std::vector<std::vector<double>> adj(
            result.rows, std::vector<double>(result.columns, 0.0));
        double adj_min = 0.0;
        double adj_max = 0.0;
        bool first_adj = true;
        std::size_t cell_index = 0;
        for (std::size_t row = 0; row < result.rows; ++row) {
            for (std::size_t column = 0; column < result.columns; ++column) {
                if (cell_index >= result.cells.size()) {
                    break;
                }
                const double value = result.cells[cell_index].adjusted_residual;
                adj[row][column] = value;
                if (first_adj) {
                    adj_min = value;
                    adj_max = value;
                    first_adj = false;
                } else {
                    adj_min = std::min(adj_min, value);
                    adj_max = std::max(adj_max, value);
                }
                ++cell_index;
            }
        }
        PlotSpec residual_heat;
        residual_heat.kind = PlotKind::heatmap;
        residual_heat.title = "调整残差热图";
        residual_heat.x_axis_title =
            column_label(table, *configuration.inference.column_category_column);
        residual_heat.y_axis_title =
            column_label(table, *configuration.inference.row_category_column);
        residual_heat.categories = row_labels;
        residual_heat.matrix_labels = column_labels;
        residual_heat.matrix_values = adj;
        residual_heat.color_min = adj_min;
        residual_heat.color_max = adj_max;
        page.plots.push_back(std::move(residual_heat));
        facts.residual_heatmap_available = true;
    }
    page.facts.chi_square = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::cross_tabulation(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.inference.row_category_column.has_value()
        || !configuration.inference.column_category_column.has_value()) {
        return error_page("交叉表", "Cross Tabulation",
                          "请选择行分类列和列分类列。");
    }
    const auto rows = extract_text_column(table, *configuration.inference.row_category_column);
    const auto columns = extract_text_column(table, *configuration.inference.column_category_column);
    std::map<std::string, std::size_t> row_indices;
    std::map<std::string, std::size_t> column_indices;
    std::vector<std::vector<double>> observed;
    std::size_t missing_count = 0;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (row >= rows.size() || row >= columns.size()
            || is_missing_cell(rows[row]) || is_missing_cell(columns[row])) {
            ++missing_count;
            continue;
        }
        const auto row_result = row_indices.emplace(rows[row], row_indices.size());
        const auto column_result = column_indices.emplace(columns[row], column_indices.size());
        if (row_result.second) {
            observed.emplace_back(column_indices.size(), 0.0);
        }
        for (auto& values : observed) {
            values.resize(column_indices.size(), 0.0);
        }
        observed[row_result.first->second][column_result.first->second] += 1.0;
    }
    if (observed.empty() || observed.front().empty()) {
        return error_page("交叉表", "Cross Tabulation",
                          "有效交叉分类观测不足。");
    }
    std::vector<std::string> row_labels(row_indices.size());
    for (const auto& [label, index] : row_indices) {
        row_labels[index] = label;
    }
    std::vector<std::string> column_labels(column_indices.size());
    for (const auto& [label, index] : column_indices) {
        column_labels[index] = label;
    }
    double total_count = 0.0;
    std::vector<double> row_totals(observed.size(), 0.0);
    std::vector<double> column_totals(observed.front().size(), 0.0);
    for (std::size_t row = 0; row < observed.size(); ++row) {
        for (std::size_t column = 0; column < observed[row].size(); ++column) {
            row_totals[row] += observed[row][column];
            column_totals[column] += observed[row][column];
            total_count += observed[row][column];
        }
    }

    OutputPage page;
    page.id = new_id("cross_tab");
    page.title = "交叉表";
    page.method_name = "Cross Tabulation";
    page.configuration = configuration;
    page.parameter_summary =
        "行: " + column_label(table, *configuration.inference.row_category_column)
        + "    列: " + column_label(table, *configuration.inference.column_category_column)
        + "    N = " + format_number(total_count)
        + "    N* = " + std::to_string(missing_count)
        + "    （本命令不做卡方检验；关联检验请用列联表卡方）";

    StatisticTable counts;
    counts.title = "观察频数";
    counts.headers.push_back("");
    counts.headers.insert(counts.headers.end(), column_labels.cbegin(), column_labels.cend());
    counts.headers.push_back("合计");
    for (std::size_t row = 0; row < observed.size(); ++row) {
        std::vector<std::string> out_row;
        out_row.push_back(row_labels[row]);
        for (std::size_t column = 0; column < observed[row].size(); ++column) {
            out_row.push_back(format_number(observed[row][column]));
        }
        out_row.push_back(format_number(row_totals[row]));
        counts.rows.push_back(std::move(out_row));
    }
    std::vector<std::string> total_row{"合计"};
    for (const double column_total : column_totals) {
        total_row.push_back(format_number(column_total));
    }
    total_row.push_back(format_number(total_count));
    counts.rows.push_back(std::move(total_row));
    page.tables.push_back(std::move(counts));

    auto append_percent = [&](const char* title,
                              const std::function<double(double, std::size_t, std::size_t)>& pct) {
        StatisticTable percent;
        percent.title = title;
        percent.headers.push_back("");
        percent.headers.insert(percent.headers.end(),
                               column_labels.cbegin(), column_labels.cend());
        for (std::size_t row = 0; row < observed.size(); ++row) {
            std::vector<std::string> out_row;
            out_row.push_back(row_labels[row]);
            for (std::size_t column = 0; column < observed[row].size(); ++column) {
                out_row.push_back(format_number(
                    pct(observed[row][column], row, column), 4));
            }
            percent.rows.push_back(std::move(out_row));
        }
        page.tables.push_back(std::move(percent));
    };
    if (total_count > 0.0) {
        append_percent("行百分比", [&](double value, std::size_t row, std::size_t) {
            return row_totals[row] > 0.0 ? 100.0 * value / row_totals[row] : 0.0;
        });
        append_percent("列百分比", [&](double value, std::size_t, std::size_t column) {
            return column_totals[column] > 0.0 ? 100.0 * value / column_totals[column] : 0.0;
        });
        append_percent("合计百分比", [&](double value, std::size_t, std::size_t) {
            return 100.0 * value / total_count;
        });
    }

    PlotSpec heatmap;
    heatmap.kind = PlotKind::heatmap;
    heatmap.title = "观察频数热图";
    heatmap.x_axis_title = column_label(table, *configuration.inference.column_category_column);
    heatmap.y_axis_title = column_label(table, *configuration.inference.row_category_column);
    heatmap.categories = row_labels;
    heatmap.matrix_labels = column_labels;
    heatmap.matrix_values = observed;
    double max_count = 0.0;
    for (const auto& row : observed) {
        for (const double value : row) {
            max_count = std::max(max_count, value);
        }
    }
    heatmap.color_min = 0.0;
    heatmap.color_max = max_count > 0.0 ? max_count : 1.0;
    page.plots.push_back(std::move(heatmap));

    domain::CrossTabFacts facts;
    facts.row_count = observed.size();
    facts.column_count = observed.front().size();
    facts.total_count = static_cast<std::size_t>(total_count);
    facts.missing_count = missing_count;
    facts.percent_tables_available = total_count > 0.0;
    page.facts.cross_tab = facts;
    page.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "cross_tab_no_chi_square",
        "交叉表仅汇总频数与百分比；独立性检验请使用命令 chi_square。"});
    return finalize_page(std::move(page));
}

namespace {

std::vector<double> parse_comma_numbers(const std::string& text)
{
    std::vector<double> values;
    std::string token;
    for (char character : text + ",") {
        if (character == ',' || character == ';' || character == ' ') {
            if (!token.empty()) {
                try {
                    values.push_back(std::stod(token));
                } catch (...) {
                }
                token.clear();
            }
        } else {
            token.push_back(character);
        }
    }
    return values;
}

std::vector<std::size_t> parse_comma_sizes(const std::string& text)
{
    std::vector<std::size_t> values;
    for (const double value : parse_comma_numbers(text)) {
        if (value >= 1.0 && std::isfinite(value)) {
            values.push_back(static_cast<std::size_t>(value));
        }
    }
    return values;
}

}  // namespace

OutputPage AnalysisService::chi_square_gof(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::optional<std::size_t> column = configuration.inference.gof_category_column.has_value()
        ? configuration.inference.gof_category_column
        : (configuration.variable_columns.empty()
               ? std::optional<std::size_t>{}
               : std::optional<std::size_t>{configuration.variable_columns.front()});
    if (!column.has_value()) {
        return error_page("卡方拟合优度", "Chi-Square Goodness-of-Fit",
                          "请选择分类列。");
    }
    const auto labels = extract_text_column(table, *column);
    std::vector<std::string> categories;
    std::vector<double> counts;
    std::size_t missing_count = 0;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (row >= labels.size() || is_missing_cell(labels[row])) {
            ++missing_count;
            continue;
        }
        const auto found = std::find(categories.cbegin(), categories.cend(), labels[row]);
        if (found == categories.cend()) {
            categories.push_back(labels[row]);
            counts.push_back(1.0);
        } else {
            counts[static_cast<std::size_t>(found - categories.cbegin())] += 1.0;
        }
    }
    const std::vector<double> proportions =
        parse_comma_numbers(configuration.inference.expected_proportions);
    const auto result = datalab::domain::statistics::chi_square_goodness_of_fit(
        categories, counts, proportions);
    OutputPage page;
    page.id = new_id("chi_square_gof");
    page.title = "卡方拟合优度";
    page.method_name = "Chi-Square Goodness-of-Fit";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    double total = 0.0;
    for (const double count : counts) {
        total += count;
    }
    page.parameter_summary = "分类列 = " + column_label(table, *column)
        + "    N = " + format_number(total)
        + "    N* = " + std::to_string(missing_count)
        + "    比例 = " + result.proportion_source;
    StatisticTable observed;
    observed.title = "观察与期望";
    observed.headers = {
        "Category", "Observed", "TestProportion", "Expected", "Residual",
        "Contribution to Chi-Square"};
    for (const auto& row : result.categories) {
        observed.rows.push_back({
            row.category, format_number(row.observed), format_number(row.test_proportion),
            format_number(row.expected), format_number(row.residual),
            format_number(row.contribution)});
    }
    page.tables.push_back(observed);
    StatisticTable test;
    test.title = "卡方检验";
    test.headers = {"N", "N*", "DF", "Chi-Sq", "P-Value", "最小期望频数", "<5 类别数", "有效性"};
    test.rows.push_back({
        format_number(total), std::to_string(missing_count),
        format_number(result.degrees_of_freedom), format_number(result.pearson_statistic),
        result.p_value.has_value() ? format_number(*result.p_value) : "*",
        format_optional(result.minimum_expected_count),
        std::to_string(result.expected_below_five_count),
        result.validity_status});
    page.tables.push_back(test);
    if (!result.categories.empty()
        && std::none_of(result.diagnostics.cbegin(), result.diagnostics.cend(),
                        [](const DiagnosticMessage& diagnostic) {
                            return diagnostic.severity == DiagnosticMessage::Severity::error;
                        })) {
        PlotSpec bars;
        bars.kind = PlotKind::scatter;
        bars.title = "观察与期望";
        bars.x_axis_title = "类别序号";
        bars.y_axis_title = "计数";
        PlotSeries observed_series;
        observed_series.label = "Observed";
        PlotSeries expected_series;
        expected_series.label = "Expected";
        expected_series.style.line_style = PlotLineStyle::dash;
        for (std::size_t index = 0; index < result.categories.size(); ++index) {
            const double x = static_cast<double>(index + 1);
            observed_series.x_values.push_back(x);
            observed_series.values.push_back(result.categories[index].observed);
            expected_series.x_values.push_back(x);
            expected_series.values.push_back(result.categories[index].expected);
            bars.point_groups.push_back(result.categories[index].category);
        }
        bars.series.push_back(observed_series);
        bars.series.push_back(expected_series);
        bars.show_legend = true;
        page.plots.push_back(std::move(bars));
        page.facts.chi_square_gof = domain::ChiSquareGofFacts{};
        page.facts.chi_square_gof->plot_available = true;
    }
    if (!page.facts.chi_square_gof.has_value()) {
        page.facts.chi_square_gof = domain::ChiSquareGofFacts{};
    }
    page.facts.chi_square_gof->statistic = result.pearson_statistic;
    page.facts.chi_square_gof->p_value = result.p_value;
    page.facts.chi_square_gof->degrees_of_freedom = result.degrees_of_freedom;
    page.facts.chi_square_gof->category_count = result.categories.size();
    page.facts.chi_square_gof->total_count = static_cast<std::size_t>(total);
    page.facts.chi_square_gof->missing_count = missing_count;
    page.facts.chi_square_gof->expected_count_warning = std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const DiagnosticMessage& diagnostic) {
            return diagnostic.code == "expected_count_below_five";
        });
    page.facts.chi_square_gof->expected_below_five_count = result.expected_below_five_count;
    page.facts.chi_square_gof->minimum_expected_count = result.minimum_expected_count;
    page.facts.chi_square_gof->validity_status = result.validity_status;
    page.facts.chi_square_gof->recommendation = result.recommendation;
    page.facts.chi_square_gof->proportion_source = result.proportion_source;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::box_cox(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto extracted = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    std::optional<double> requested_lambda;
    if (configuration.inference.hypothesis_mean.has_value()) {
        requested_lambda = configuration.inference.hypothesis_mean;
    }
    const auto result = datalab::domain::statistics::box_cox_transform(
        extracted.values, requested_lambda);
    OutputPage page;
    page.id = new_id("box_cox");
    page.title = "Box-Cox 变换";
    page.method_name = "Box-Cox Transformation";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    lambda = " + format_number(result.lambda);
    page.diagnostics = result.diagnostics;
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "Box-Cox 跳过 " + std::to_string(extracted.missing_count)
                + " 个缺失或非法观测。"});
    }
    StatisticTable summary;
    summary.title = "变换参数";
    summary.headers = {"N", "Lambda", "Transformed StDev"};
    summary.rows.push_back({
        std::to_string(result.transformed_values.size()),
        format_number(result.lambda),
        format_number(result.transformed_standard_deviation)});
    page.tables.push_back(summary);
    if (configuration.specifications.lower.has_value()
        || configuration.specifications.upper.has_value()) {
        using datalab::domain::statistics::box_cox_limits_order_ok;
        using datalab::domain::statistics::box_cox_transform_limit;
        const double lambda = result.lambda;
        SpecificationLimits transformed_specifications;
        bool capability_inputs_ok = true;
        if (configuration.specifications.lower.has_value()) {
            const auto transformed = box_cox_transform_limit(
                *configuration.specifications.lower, lambda);
            if (!transformed.has_value()) {
                capability_inputs_ok = false;
                page.diagnostics.push_back({
                    DiagnosticMessage::Severity::warning,
                    "box_cox_invalid_spec_limit",
                    "规格下限无法变换（须为正有限数）。"});
            } else {
                transformed_specifications.lower = *transformed;
            }
        }
        if (configuration.specifications.upper.has_value()) {
            const auto transformed = box_cox_transform_limit(
                *configuration.specifications.upper, lambda);
            if (!transformed.has_value()) {
                capability_inputs_ok = false;
                page.diagnostics.push_back({
                    DiagnosticMessage::Severity::warning,
                    "box_cox_invalid_spec_limit",
                    "规格上限无法变换（须为正有限数）。"});
            } else {
                transformed_specifications.upper = *transformed;
            }
        }
        if (configuration.specifications.target.has_value()) {
            const auto transformed = box_cox_transform_limit(
                *configuration.specifications.target, lambda);
            if (!transformed.has_value()) {
                capability_inputs_ok = false;
                page.diagnostics.push_back({
                    DiagnosticMessage::Severity::warning,
                    "box_cox_invalid_spec_limit",
                    "规格目标无法变换（须为正有限数）。"});
            } else {
                transformed_specifications.target = *transformed;
            }
        }
        if (configuration.specifications.lower.has_value()
            && configuration.specifications.upper.has_value()
            && !box_cox_limits_order_ok(
                *configuration.specifications.lower,
                *configuration.specifications.upper,
                lambda)) {
            capability_inputs_ok = false;
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::warning,
                "box_cox_spec_limits_order",
                "变换后规格限顺序无效；请检查 LSL/USL 与 λ。"});
        }
        if (capability_inputs_ok
            && (transformed_specifications.lower.has_value()
                || transformed_specifications.upper.has_value())) {
            const auto capability = datalab::domain::statistics::ProcessCapability::calculate(
                result.transformed_values, result.transformed_standard_deviation,
                transformed_specifications);
            StatisticTable capability_table;
            capability_table.title = "变换后过程能力";
            capability_table.headers = {"指标", "数值"};
            capability_table.rows = {
                {"Cp", format_optional(capability.cp)},
                {"Cpk", format_optional(capability.cpk)},
                {"Pp", format_optional(capability.pp)},
                {"Ppk", format_optional(capability.ppk)}};
            page.tables.push_back(capability_table);
            append_diagnostics(page.diagnostics, capability.diagnostics, "能力分析: ");
        }
    }
    if (!result.lambdas.empty() && !result.standard_deviations.empty()) {
        PlotSpec lambda_plot;
        lambda_plot.kind = PlotKind::scatter;
        lambda_plot.title = "Box-Cox λ 选择诊断";
        lambda_plot.x_axis_title = "Lambda";
        lambda_plot.y_axis_title = "标准化变换 SD";
        lambda_plot.show_legend = true;
        PlotSeries curve;
        curve.role = PlotSeriesRole::generic;
        curve.label = "SD(W)";
        curve.x_values = result.lambdas;
        curve.values = result.standard_deviations;
        lambda_plot.x_values = result.lambdas;
        lambda_plot.values = result.standard_deviations;
        PlotSeries selected;
        selected.role = PlotSeriesRole::fitted;
        selected.label = "选定 λ";
        const double y_min = *std::min_element(
            result.standard_deviations.cbegin(), result.standard_deviations.cend());
        const double y_max = *std::max_element(
            result.standard_deviations.cbegin(), result.standard_deviations.cend());
        selected.x_values = {result.lambda, result.lambda};
        selected.values = {y_min, y_max};
        lambda_plot.series = {std::move(curve), std::move(selected)};
        page.plots.push_back(std::move(lambda_plot));
    }
    if (result.transformed_values.size() == extracted.values.size()
        && !extracted.values.empty()) {
        const auto before = datalab::domain::statistics::normal_probability_plot(
            extracted.values, extracted.source_rows);
        PlotSpec before_plot;
        before_plot.kind = PlotKind::probability;
        before_plot.title = "变换前正态概率图";
        before_plot.x_axis_title = "理论分位数";
        before_plot.y_axis_title = extracted.name;
        before_plot.x_values = before.theoretical_quantiles;
        before_plot.values = before.ordered_values;
        before_plot.source_rows = before.source_rows;
        page.plots.push_back(std::move(before_plot));
        const auto after = datalab::domain::statistics::normal_probability_plot(
            result.transformed_values, extracted.source_rows);
        PlotSpec after_plot;
        after_plot.kind = PlotKind::probability;
        after_plot.title = "变换后正态概率图";
        after_plot.x_axis_title = "理论分位数";
        after_plot.y_axis_title = "变换值";
        after_plot.x_values = after.theoretical_quantiles;
        after_plot.values = after.ordered_values;
        after_plot.source_rows = after.source_rows;
        page.plots.push_back(std::move(after_plot));
    }
    domain::BoxCoxFacts facts;
    facts.lambda = result.lambda;
    facts.n = result.transformed_values.size();
    facts.missing_count = extracted.missing_count;
    facts.transformed_standard_deviation = result.transformed_standard_deviation;
    facts.rounded_lambda = !requested_lambda.has_value();
    facts.assumption_status = "not_verified";
    page.facts.box_cox = std::move(facts);
    page.method_metadata.estimation_method = "box_cox_grid";
    page.method_metadata.parameter_source =
        requested_lambda.has_value() ? "specified" : "estimated";
    page.method_metadata.valid_count = result.transformed_values.size();
    page.method_metadata.missing_count = extracted.missing_count;
    page.method_metadata.source_rows.clear();
    for (const std::size_t row : extracted.source_rows) {
        page.method_metadata.source_rows.push_back(static_cast<domain::RowId>(row));
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::gage_rr(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.msa.gage_measurement_column.has_value()
        || !configuration.msa.gage_part_column.has_value()
        || !configuration.msa.gage_operator_column.has_value()) {
        return error_page("Crossed Gage R&R", "Crossed Gage R&R",
                          "请选择测量值、零件和操作员列。");
    }
    const auto measurements = extract_numeric_column(
        table, *configuration.msa.gage_measurement_column, configuration.excluded_rows);
    const auto parts = extract_text_column(table, *configuration.msa.gage_part_column);
    const auto operators = extract_text_column(table, *configuration.msa.gage_operator_column);
    std::map<std::size_t, double> measurement_by_row;
    for (std::size_t index = 0; index < measurements.values.size(); ++index) {
        measurement_by_row[measurements.source_rows[index]] = measurements.values[index];
    }
    std::vector<double> values;
    std::vector<std::string> part_values;
    std::vector<std::string> operator_values;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        const auto measurement = measurement_by_row.find(row);
        if (measurement == measurement_by_row.end()
            || row >= parts.size() || row >= operators.size()
            || is_missing_cell(parts[row]) || is_missing_cell(operators[row])) {
            continue;
        }
        values.push_back(measurement->second);
        part_values.push_back(parts[row]);
        operator_values.push_back(operators[row]);
        source_rows.push_back(row);
    }
    const double tolerance = configuration.specifications.lower.has_value()
        && configuration.specifications.upper.has_value()
        ? *configuration.specifications.upper - *configuration.specifications.lower : 0.0;
    const auto result = datalab::domain::statistics::crossed_gage_rr(
        values, part_values, operator_values, tolerance);
    OutputPage page;
    page.id = new_id("gage_rr");
    page.title = "Crossed Gage R&R";
    page.method_name = "Crossed Gage R&R (ANOVA)";
    page.configuration = configuration;
    page.parameter_summary = "测量值 = " + measurements.name
        + "    零件 = " + column_label(table, *configuration.msa.gage_part_column)
        + "    操作员 = " + column_label(table, *configuration.msa.gage_operator_column);
    page.diagnostics = result.diagnostics;
    StatisticTable anova;
    anova.title = "Gage R&R 方差分析";
    anova.headers = {"来源", "DF", "SS", "MS", "F", "P-Value"};
    for (const auto& row : result.anova_rows) {
        anova.rows.push_back({
            row.source, std::to_string(row.degrees_of_freedom),
            format_number(row.sum_of_squares), format_number(row.mean_square),
            format_number(row.f_statistic),
            row.p_value.has_value() ? format_number(*row.p_value) : "*"});
    }
    page.tables.push_back(anova);
    StatisticTable components;
    components.title = "方差分量";
    components.headers = {"来源", "Raw VarComp", "VarComp", "截断", "StdDev",
                          "%Contribution", "Study Var", "%Study Var", "%Tolerance"};
    for (const auto& component : result.variance_components) {
        components.rows.push_back({
            component.source, format_number(component.raw_variance_component),
            format_number(component.variance_component),
            component.truncated ? "是" : "否",
            format_number(component.standard_deviation),
            format_number(component.percent_contribution),
            format_number(component.study_variation),
            format_number(component.percent_study_variation),
            component.percent_tolerance_available
                ? format_number(component.percent_tolerance) : "*"});
    }
    page.tables.push_back(components);
    StatisticTable summary;
    summary.title = "Gage R&R 摘要";
    summary.headers = {"零件数", "操作员数", "重复次数", "ndc"};
    summary.rows.push_back({
        std::to_string(result.part_count), std::to_string(result.operator_count),
        std::to_string(result.replicate_count), format_number(result.ndc)});
    page.tables.push_back(summary);
    append_gage_contribution_pareto(page, result.variance_components);
    append_gage_study_var_pareto(page, result.variance_components);
    append_gage_tolerance_pareto(page, result.variance_components);
    append_gage_run_chart(page, values, source_rows);
    append_part_xbar_range_plots(
        page, values, part_values, operator_values, source_rows,
        result.replicate_count, result.design_balanced);
    if (result.replicate_count >= 2 && result.design_balanced) {
        append_gage_by_part_plot(page, values, part_values, source_rows);
        append_gage_interaction_plot(
            page, values, part_values, operator_values, source_rows);
    }
    page.facts.msa = datalab::domain::statistics::gage_rr_facts_from(result);
    if (page.facts.msa.has_value()) {
        const bool plots_available =
            result.replicate_count >= 2 && result.design_balanced;
        page.facts.msa->by_part_plot_available = plots_available;
        page.facts.msa->interaction_plot_available = plots_available;
        if (plots_available) {
            page.facts.msa->plot_point_count = values.size();
        }
    }
    append_rule_table(page, page.facts.msa->rules);
    domain::apply_evidence(page.method_metadata, result.evidence);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::emp_crossed(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.msa.gage_measurement_column.has_value()
        || !configuration.msa.gage_part_column.has_value()
        || !configuration.msa.gage_operator_column.has_value()) {
        return error_page("EMP Crossed", "EMP Crossed",
                          "请选择测量值、零件和操作员列。");
    }
    const auto measurements = extract_numeric_column(
        table, *configuration.msa.gage_measurement_column, configuration.excluded_rows);
    const auto parts = extract_text_column(table, *configuration.msa.gage_part_column);
    const auto operators = extract_text_column(table, *configuration.msa.gage_operator_column);
    std::map<std::size_t, double> measurement_by_row;
    for (std::size_t index = 0; index < measurements.values.size(); ++index) {
        measurement_by_row[measurements.source_rows[index]] = measurements.values[index];
    }
    std::vector<double> values;
    std::vector<std::string> part_values;
    std::vector<std::string> operator_values;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        const auto measurement = measurement_by_row.find(row);
        if (measurement == measurement_by_row.end()
            || row >= parts.size() || row >= operators.size()
            || is_missing_cell(parts[row]) || is_missing_cell(operators[row])) {
            continue;
        }
        values.push_back(measurement->second);
        part_values.push_back(parts[row]);
        operator_values.push_back(operators[row]);
    }
    const auto gage = datalab::domain::statistics::crossed_gage_rr(
        values, part_values, operator_values, 0.0);
    double part_var = 0.0;
    double repeat = 0.0;
    double oper = 0.0;
    double interaction = 0.0;
    for (const auto& component : gage.variance_components) {
        if (component.source == "Part-To-Part") {
            part_var = component.variance_component;
        } else if (component.source == "Repeatability") {
            repeat = component.variance_component;
        } else if (component.source == "Operator") {
            oper = component.variance_component;
        } else if (component.source == "Operator * Part") {
            interaction = component.variance_component;
        }
    }
    const auto emp = datalab::domain::statistics::emp_classification_from_components(
        part_var, repeat, oper, interaction);

    OutputPage page;
    page.id = new_id("emp");
    page.title = "EMP Crossed";
    page.method_name = "Evaluate Measurement Process (Crossed)";
    page.configuration = configuration;
    page.parameter_summary = "测量值 = " + measurements.name
        + "    零件 = " + column_label(table, *configuration.msa.gage_part_column)
        + "    操作员 = " + column_label(table, *configuration.msa.gage_operator_column)
        + "    （Wheeler EMP；非全量 Expanded Gage）";
    page.diagnostics = gage.diagnostics;
    page.diagnostics.insert(page.diagnostics.end(), emp.diagnostics.begin(), emp.diagnostics.end());

    StatisticTable components;
    components.title = "方差分量（交叉 ANOVA）";
    components.headers = {"来源", "VarComp", "%Contribution"};
    for (const auto& component : gage.variance_components) {
        components.rows.push_back({
            component.source, format_number(component.variance_component),
            format_number(component.percent_contribution)});
    }
    page.tables.push_back(std::move(components));

    StatisticTable emp_table;
    emp_table.title = "EMP 统计";
    emp_table.headers = {"指标", "数值"};
    emp_table.rows = {
        {"ICC (no bias)", format_number(emp.icc_no_bias)},
        {"ICC (with bias)", format_number(emp.icc_with_bias)},
        {"ICC (with bias and interaction)", format_number(emp.icc_with_interaction)},
        {"Probable Error", format_number(emp.probable_error)},
        {"Classification", emp.classification},
        {"Classification basis", emp.classification_basis},
        {"Attenuation % (approx)", format_number(emp.attenuation_percent)},
    };
    page.tables.push_back(std::move(emp_table));

    page.facts.msa = datalab::domain::statistics::gage_rr_facts_from(gage);
    if (page.facts.msa.has_value()) {
        page.facts.msa->emp_available = true;
        page.facts.msa->emp_icc_no_bias = emp.icc_no_bias;
        page.facts.msa->emp_icc_with_bias = emp.icc_with_bias;
        page.facts.msa->emp_icc_with_interaction = emp.icc_with_interaction;
        page.facts.msa->emp_probable_error = emp.probable_error;
        page.facts.msa->emp_classification = emp.classification;
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::expanded_gage_rr(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.msa.gage_measurement_column.has_value()
        || !configuration.msa.gage_part_column.has_value()
        || !configuration.msa.gage_operator_column.has_value()
        || !configuration.msa.gage_additional_column.has_value()) {
        return error_page("Expanded Gage R&R", "Expanded Gage R&R",
                          "请选择测量值、零件、操作员与附加因子列；无附加因子时请用交叉 Gage R&R。");
    }
    const auto measurements = extract_numeric_column(
        table, *configuration.msa.gage_measurement_column, configuration.excluded_rows);
    const auto parts = extract_text_column(table, *configuration.msa.gage_part_column);
    const auto operators = extract_text_column(table, *configuration.msa.gage_operator_column);
    const auto additional = extract_text_column(
        table, *configuration.msa.gage_additional_column);
    std::map<std::size_t, double> measurement_by_row;
    for (std::size_t index = 0; index < measurements.values.size(); ++index) {
        measurement_by_row[measurements.source_rows[index]] = measurements.values[index];
    }
    std::vector<double> values;
    std::vector<std::string> part_values;
    std::vector<std::string> operator_values;
    std::vector<std::string> additional_values;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        const auto measurement = measurement_by_row.find(row);
        if (measurement == measurement_by_row.end()
            || row >= parts.size() || row >= operators.size() || row >= additional.size()
            || is_missing_cell(parts[row]) || is_missing_cell(operators[row])
            || is_missing_cell(additional[row])) {
            continue;
        }
        values.push_back(measurement->second);
        part_values.push_back(parts[row]);
        operator_values.push_back(operators[row]);
        additional_values.push_back(additional[row]);
    }
    const std::string additional_name =
        column_label(table, *configuration.msa.gage_additional_column);
    const double tolerance = configuration.msa.gage_tolerance > 0.0
        ? configuration.msa.gage_tolerance
        : 0.0;
    const auto expanded = datalab::domain::statistics::expanded_gage_rr_three_factor(
        values, part_values, operator_values, additional_values, tolerance, additional_name);

    OutputPage page;
    page.id = new_id("expanded_gage");
    page.title = "Expanded Gage R&R";
    page.method_name = "Expanded Gage R&R (3-factor balanced)";
    page.configuration = configuration;
    page.parameter_summary = "测量值 = " + measurements.name
        + "    零件 / 操作员 / " + additional_name
        + "    （平衡三因子随机；非全量 GLM）";
    page.diagnostics = expanded.gage.diagnostics;

    StatisticTable anova;
    anova.title = "ANOVA";
    anova.headers = {"来源", "DF", "SS", "MS"};
    for (const auto& row : expanded.gage.anova_rows) {
        anova.rows.push_back({
            row.source,
            std::to_string(row.degrees_of_freedom),
            format_number(row.sum_of_squares),
            format_number(row.mean_square)});
    }
    page.tables.push_back(std::move(anova));

    StatisticTable components;
    components.title = "方差分量";
    components.headers = {"来源", "VarComp", "%Contribution", "%StudyVar"};
    for (const auto& component : expanded.gage.variance_components) {
        components.rows.push_back({
            component.source,
            format_number(component.variance_component),
            format_number(component.percent_contribution),
            format_number(component.percent_study_variation)});
    }
    page.tables.push_back(std::move(components));

    page.facts.msa = datalab::domain::statistics::gage_rr_facts_from(expanded.gage);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::mann_whitney(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() != 2) {
        return error_page("Mann-Whitney 检验", "Mann-Whitney", "请选择正好两列独立样本。");
    }
    const auto first = extract_numeric_column(
        table, configuration.variable_columns[0], configuration.excluded_rows);
    const auto second = extract_numeric_column(
        table, configuration.variable_columns[1], configuration.excluded_rows);
    const double confidence_level =
        configuration.inference.confidence_level > 0.0
            && configuration.inference.confidence_level < 1.0
            ? configuration.inference.confidence_level : 0.95;
    const auto result = datalab::domain::statistics::mann_whitney(
        first.values, second.values, parse_alternative(configuration.inference.alternative),
        confidence_level);
    OutputPage page;
    page.id = new_id("mann_whitney");
    page.title = "Mann-Whitney 检验";
    page.method_name = "Mann-Whitney U";
    page.configuration = configuration;
    page.parameter_summary = first.name + " vs " + second.name;
    page.diagnostics = result.diagnostics;
    const int confidence_percent =
        static_cast<int>(confidence_level * 100.0 + 0.5);
    StatisticTable output;
    output.title = "秩和检验";
    output.headers = {"第一组 N", "第二组 N", "秩和", "Z", "P-Value", "未调整 P",
                      "位置差异",
                      std::to_string(confidence_percent) + "% CI",
                      "Ties 修正", "连续性修正", "近似方法", "小样本警告",
                      "效应量"};
    std::string interval = "*";
    if (result.ci_lower.has_value() && result.ci_upper.has_value()) {
        interval = "[" + format_number(*result.ci_lower) + ", "
            + format_number(*result.ci_upper) + "]";
    } else if (result.ci_lower.has_value()) {
        interval = "[" + format_number(*result.ci_lower) + ", +∞)";
    } else if (result.ci_upper.has_value()) {
        interval = "(-∞, " + format_number(*result.ci_upper) + "]";
    }
    output.rows.push_back({std::to_string(result.first_count), std::to_string(result.second_count),
        format_number(result.rank_sum), format_number(result.z_statistic),
        format_optional(result.p_value), format_optional(result.p_value_without_tie_correction),
        format_optional(result.location_estimate),
        interval,
        result.tie_correction ? "是" : "否",
        result.continuity_correction ? "是" : "否",
        result.approximation,
        result.small_sample_warning ? "是" : "否",
        format_optional(result.effect_size)});
    page.tables.push_back(output);
    const std::size_t missing_count = first.missing_count + second.missing_count;
    if (missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "Mann–Whitney 跳过 " + std::to_string(missing_count)
                + " 个缺失或非法单元格（含 *）。"});
    }
    auto plots = make_grouped_distribution_plots("箱线图", "个体值图");
    append_group_to_distribution_plots(plots, first.name, first.values, first.source_rows);
    append_group_to_distribution_plots(plots, second.name, second.values, second.source_rows);
    push_distribution_plots(page, plots);
    domain::NonparametricFacts facts;
    facts.method = "mann_whitney";
    facts.statistic = result.z_statistic;
    facts.p_value = result.p_value;
    facts.tie_correction = result.tie_correction;
    facts.continuity_correction = result.continuity_correction;
    facts.approximation = result.approximation;
    facts.small_sample_warning = result.small_sample_warning;
    facts.effect_size = result.effect_size;
    facts.p_value_unadjusted = result.p_value_without_tie_correction;
    facts.group_count = 2;
    facts.plot_point_count = plots.point_count;
    facts.missing_count = missing_count;
    facts.location_estimate = result.location_estimate;
    facts.ci_lower = result.ci_lower;
    facts.ci_upper = result.ci_upper;
    page.facts.nonparametric = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::wilcoxon_signed_rank(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.empty()
        || configuration.variable_columns.size() > 2) {
        return error_page("Wilcoxon 符号秩检验", "Wilcoxon signed-rank",
                          "请选择一列（相对 η0）或两列配对样本。");
    }
    const double hypothesized =
        configuration.inference.hypothesis_mean.value_or(0.0);
    double confidence = 0.95;
    if (configuration.inference.confidence_level > 0.0) {
        confidence = configuration.inference.confidence_level;
        if (confidence > 1.0) {
            confidence /= 100.0;
        }
    }
    OutputPage page;
    page.id = new_id("wilcoxon");
    page.title = "Wilcoxon 符号秩检验";
    page.method_name = "Wilcoxon signed-rank";
    page.configuration = configuration;

    datalab::domain::statistics::SignedRankResult result;
    std::size_t missing_count = 0;
    std::size_t plot_point_count = 0;
    const bool one_sample = configuration.variable_columns.size() == 1;

    if (one_sample) {
        const auto extracted = extract_numeric_column(
            table, configuration.variable_columns.front(), configuration.excluded_rows);
        missing_count = extracted.missing_count;
        page.parameter_summary = extracted.name + "  vs η0 = "
            + format_number(hypothesized);
        result = datalab::domain::statistics::wilcoxon_signed_rank_one_sample(
            extracted.values, hypothesized,
            parse_alternative(configuration.inference.alternative),
            confidence);
        page.diagnostics = result.diagnostics;
        auto plots = make_grouped_distribution_plots("箱线图", "个体值图");
        append_group_to_distribution_plots(
            plots, extracted.name, extracted.values, extracted.source_rows);
        push_distribution_plots(page, plots);
        plot_point_count = plots.point_count;
    } else {
        const auto first = extract_numeric_column(
            table, configuration.variable_columns[0], configuration.excluded_rows);
        const auto second = extract_numeric_column(
            table, configuration.variable_columns[1], configuration.excluded_rows);
        const auto aligned = align_complete_rows_with_source({first, second});
        std::vector<double> first_values;
        std::vector<double> second_values;
        first_values.reserve(aligned.values.size());
        second_values.reserve(aligned.values.size());
        for (const auto& row : aligned.values) {
            first_values.push_back(row[0]);
            second_values.push_back(row[1]);
        }
        result = datalab::domain::statistics::wilcoxon_signed_rank(
            first_values, second_values,
            parse_alternative(configuration.inference.alternative),
            confidence);
        page.parameter_summary = first.name + " vs " + second.name;
        page.diagnostics = result.diagnostics;
        missing_count = first.total_count > aligned.source_rows.size()
            ? first.total_count - aligned.source_rows.size() : 0;
        if (missing_count > 0) {
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::warning,
                "missing_values",
                "Wilcoxon 跳过 " + std::to_string(missing_count)
                    + " 个缺失或非法单元格（含 *）。"});
        }
        auto plots = make_grouped_distribution_plots("箱线图", "个体值图");
        append_group_to_distribution_plots(plots, first.name, first_values, aligned.source_rows);
        append_group_to_distribution_plots(plots, second.name, second_values, aligned.source_rows);
        push_distribution_plots(page, plots);
        plot_point_count = plots.point_count;
        PlotSpec paired;
        paired.kind = PlotKind::scatter;
        paired.title = "配对测量散点图";
        paired.x_axis_title = first.name;
        paired.y_axis_title = second.name;
        paired.x_values = first_values;
        paired.values = second_values;
        paired.source_rows = aligned.source_rows;
        page.plots.push_back(std::move(paired));
    }

    StatisticTable output;
    output.title = "符号秩检验";
    if (one_sample) {
        output.headers = {"非零差值 N", "η0", "正秩和", "负秩和", "Z", "P-Value",
                          "Ties 修正", "连续性修正", "近似方法", "小样本警告"};
        output.rows.push_back({
            std::to_string(result.count), format_number(result.hypothesized_median),
            format_number(result.positive_rank_sum),
            format_number(result.negative_rank_sum), format_number(result.z_statistic),
            format_optional(result.p_value),
            result.tie_correction ? "是" : "否",
            result.continuity_correction ? "是" : "否", result.approximation,
            result.small_sample_warning ? "是" : "否"});
    } else {
        output.headers = {"非零差值 N", "正秩和", "负秩和", "Z", "P-Value", "Ties 修正",
                          "连续性修正", "近似方法", "小样本警告"};
        output.rows.push_back({
            std::to_string(result.count), format_number(result.positive_rank_sum),
            format_number(result.negative_rank_sum), format_number(result.z_statistic),
            format_optional(result.p_value),
            result.tie_correction ? "是" : "否",
            result.continuity_correction ? "是" : "否", result.approximation,
            result.small_sample_warning ? "是" : "否"});
    }
    page.tables.push_back(output);
    if (result.location_estimate.has_value()) {
        StatisticTable location;
        location.title = "位置估计（Walsh）";
        location.headers = {"估计中位数", "CI 下限", "CI 上限"};
        location.rows.push_back({
            format_optional(result.location_estimate),
            format_optional(result.ci_lower),
            format_optional(result.ci_upper)});
        page.tables.push_back(std::move(location));
    }

    domain::NonparametricFacts facts;
    facts.method = one_sample ? "wilcoxon_one_sample" : "wilcoxon_signed_rank";
    facts.statistic = result.z_statistic;
    facts.p_value = result.p_value;
    facts.tie_correction = result.tie_correction;
    facts.continuity_correction = result.continuity_correction;
    facts.approximation = result.approximation;
    facts.small_sample_warning = result.small_sample_warning;
    facts.group_count = one_sample ? 1 : 2;
    facts.plot_point_count = plot_point_count;
    facts.missing_count = missing_count;
    facts.location_estimate = result.location_estimate;
    facts.ci_lower = result.ci_lower;
    facts.ci_upper = result.ci_upper;
    page.facts.nonparametric = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::sign_test(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.empty()
        || configuration.variable_columns.size() > 2) {
        return error_page("符号检验", "Sign test",
                          "请选择一列（单样本）或两列（配对）。");
    }
    const double hypothesized =
        configuration.inference.hypothesis_mean.value_or(0.0);
    double confidence = 0.95;
    if (configuration.inference.confidence_level > 0.0) {
        confidence = configuration.inference.confidence_level;
        if (confidence > 1.0) {
            confidence /= 100.0;
        }
    }
    OutputPage page;
    page.id = new_id("sign_test");
    page.title = "符号检验";
    page.method_name = "Sign test";
    page.configuration = configuration;

    datalab::domain::statistics::SignTestResult result;
    std::size_t missing_count = 0;
    std::size_t plot_point_count = 0;
    if (configuration.variable_columns.size() == 1) {
        const auto extracted = extract_numeric_column(
            table, configuration.variable_columns.front(), configuration.excluded_rows);
        missing_count = extracted.missing_count;
        page.parameter_summary = extracted.name + "  vs η0 = "
            + format_number(hypothesized);
        result = datalab::domain::statistics::sign_test(
            extracted.values, hypothesized,
            parse_alternative(configuration.inference.alternative),
            confidence);
        auto plots = make_grouped_distribution_plots("箱线图", "个体值图");
        append_group_to_distribution_plots(
            plots, extracted.name, extracted.values, extracted.source_rows);
        push_distribution_plots(page, plots);
        plot_point_count = plots.point_count;
    } else {
        const auto first = extract_numeric_column(
            table, configuration.variable_columns[0], configuration.excluded_rows);
        const auto second = extract_numeric_column(
            table, configuration.variable_columns[1], configuration.excluded_rows);
        const auto aligned = align_complete_rows_with_source({first, second});
        missing_count = first.total_count > aligned.source_rows.size()
            ? first.total_count - aligned.source_rows.size() : 0;
        page.parameter_summary = first.name + " - " + second.name + "  vs 0";
        std::vector<double> first_values;
        std::vector<double> second_values;
        for (const auto& row : aligned.values) {
            first_values.push_back(row[0]);
            second_values.push_back(row[1]);
        }
        result = datalab::domain::statistics::sign_test_paired(
            first_values, second_values,
            parse_alternative(configuration.inference.alternative),
            confidence);
        auto plots = make_grouped_distribution_plots("箱线图", "个体值图");
        append_group_to_distribution_plots(
            plots, first.name, first_values, aligned.source_rows);
        append_group_to_distribution_plots(
            plots, second.name, second_values, aligned.source_rows);
        push_distribution_plots(page, plots);
        plot_point_count = plots.point_count;
        PlotSpec paired;
        paired.kind = PlotKind::scatter;
        paired.title = "配对测量散点图";
        paired.x_axis_title = first.name;
        paired.y_axis_title = second.name;
        paired.x_values = first_values;
        paired.values = second_values;
        paired.source_rows = aligned.source_rows;
        page.plots.push_back(std::move(paired));
    }
    page.diagnostics = result.diagnostics;
    if (missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "符号检验跳过 " + std::to_string(missing_count)
                + " 个缺失或非法单元格（含 *）。"});
    }
    StatisticTable summary;
    summary.title = "符号摘要";
    summary.headers = {
        "有效符号 N", "结数", "n+", "n-", "样本中位数", "假设中位数 η0"};
    summary.rows.push_back({
        std::to_string(result.n_nonzero),
        std::to_string(result.n_ties),
        std::to_string(result.n_positive),
        std::to_string(result.n_negative),
        format_optional(result.sample_median),
        format_number(result.hypothesized_median)});
    page.tables.push_back(std::move(summary));
    StatisticTable test;
    test.title = "符号检验";
    test.headers = {"P-Value", "近似方法", "小样本警告"};
    test.rows.push_back({
        format_optional(result.p_value),
        result.approximation,
        result.small_sample_warning ? "是" : "否"});
    page.tables.push_back(std::move(test));
    if (result.ci_lower.has_value() || result.ci_upper.has_value()) {
        StatisticTable ci_table;
        ci_table.title = "中位数置信区间";
        ci_table.headers = {
            "估计中位数", "CI 下限", "CI 上限", "名义置信水平", "达到水平", "方法"};
        ci_table.rows.push_back({
            format_optional(result.sample_median),
            format_optional(result.ci_lower),
            format_optional(result.ci_upper),
            format_number(result.confidence_level),
            format_optional(result.achieved_confidence),
            "sign_order_statistic"});
        page.tables.push_back(std::move(ci_table));
    }

    domain::NonparametricFacts facts;
    facts.method = configuration.variable_columns.size() == 2
        ? "sign_test_paired" : "sign_test";
    facts.statistic = static_cast<double>(result.n_positive);
    facts.p_value = result.p_value;
    facts.tie_correction = false;
    facts.continuity_correction = false;
    facts.approximation = result.approximation;
    facts.small_sample_warning = result.small_sample_warning;
    facts.group_count = configuration.variable_columns.size();
    facts.plot_point_count = plot_point_count;
    facts.missing_count = missing_count;
    facts.location_estimate = result.sample_median;
    facts.ci_lower = result.ci_lower;
    facts.ci_upper = result.ci_upper;
    page.facts.nonparametric = std::move(facts);
    return finalize_page(std::move(page));
}

namespace {

bool has_interior_numeric_gap(
    const DataTable& table,
    std::size_t column,
    const std::vector<std::size_t>& excluded_rows,
    const ExtractedNumericColumn& extracted)
{
    if (extracted.source_rows.size() < 2) {
        return false;
    }
    std::set<std::size_t> excluded(
        excluded_rows.cbegin(), excluded_rows.cend());
    const std::size_t first = extracted.source_rows.front();
    const std::size_t last = extracted.source_rows.back();
    for (std::size_t row = first; row <= last; ++row) {
        if (excluded.count(row) != 0) {
            continue;
        }
        if (column >= table.rows[row].size()
            || is_missing_cell(table.rows[row][column])) {
            return true;
        }
        double value = 0.0;
        if (!datalab::domain::parse_finite_number(table.rows[row][column], value)) {
            return true;
        }
    }
    return false;
}

datalab::domain::statistics::RunsCriterionKind parse_runs_criterion(
    const std::string& raw)
{
    if (raw == "median") {
        return datalab::domain::statistics::RunsCriterionKind::median;
    }
    if (raw == "value") {
        return datalab::domain::statistics::RunsCriterionKind::value;
    }
    return datalab::domain::statistics::RunsCriterionKind::mean;
}

std::string runs_criterion_label(
    datalab::domain::statistics::RunsCriterionKind kind)
{
    switch (kind) {
    case datalab::domain::statistics::RunsCriterionKind::median:
        return "median";
    case datalab::domain::statistics::RunsCriterionKind::value:
        return "value";
    case datalab::domain::statistics::RunsCriterionKind::mean:
    default:
        return "mean";
    }
}

}  // namespace

OutputPage AnalysisService::runs_test(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() != 1) {
        return error_page("游程检验", "Runs Test",
                          "请选择一列数值序列（按行序）。");
    }
    const std::size_t column = configuration.variable_columns.front();
    const auto extracted = extract_numeric_column(
        table, column, configuration.excluded_rows);
    OutputPage page;
    page.id = new_id("runs_test");
    page.title = "游程检验";
    page.method_name = "Runs Test";
    page.configuration = configuration;
    page.parameter_summary = extracted.name;
    if (has_interior_numeric_gap(table, column, configuration.excluded_rows, extracted)) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::error,
            "runs_interior_missing",
            "序列中间存在缺失或非法值，游程检验要求完整连续观测，未计算检验。"});
        domain::NonparametricFacts facts;
        facts.method = "runs_test";
        facts.missing_count = extracted.missing_count + extracted.invalid_count;
        page.facts.nonparametric = std::move(facts);
        return finalize_page(std::move(page));
    }
    const auto criterion_kind = parse_runs_criterion(
        configuration.inference.runs_criterion);
    const std::optional<double> criterion_value =
        criterion_kind == datalab::domain::statistics::RunsCriterionKind::value
            ? configuration.inference.hypothesis_mean
            : std::nullopt;
    const auto result = datalab::domain::statistics::runs_test(
        extracted.values, criterion_kind, criterion_value);
    page.diagnostics = result.diagnostics;
    if (extracted.missing_count + extracted.invalid_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "游程检验跳过两端缺失或非法单元格 N* = "
                + std::to_string(extracted.missing_count + extracted.invalid_count)
                + "。"});
    }
    page.parameter_summary = extracted.name + "    K = " + format_number(result.criterion)
        + " (" + runs_criterion_label(result.criterion_kind) + ")"
        + "    N = " + std::to_string(result.n);
    StatisticTable criterion;
    criterion.title = "比较准则";
    criterion.headers = {"准则", "K", "N", "≤K (B)", ">K (A)"};
    criterion.rows.push_back({
        runs_criterion_label(result.criterion_kind),
        format_number(result.criterion),
        std::to_string(result.n),
        std::to_string(result.below_or_equal),
        std::to_string(result.above)});
    page.tables.push_back(std::move(criterion));
    StatisticTable test;
    test.title = "Runs检验";
    test.headers = {"Observed", "Expected", "Z", "P-Value", "近似", "小样本警告"};
    test.rows.push_back({
        std::to_string(result.observed_runs),
        format_optional(result.expected_runs),
        format_optional(result.z_statistic),
        format_optional(result.p_value),
        result.approximation,
        result.small_sample_warning ? "是" : "否"});
    page.tables.push_back(std::move(test));
    if (!extracted.values.empty()) {
        PlotSpec plot;
        plot.kind = PlotKind::control;
        plot.title = extracted.name + " 游程序列图";
        plot.x_axis_title = "观测序号";
        plot.y_axis_title = extracted.name;
        plot.center_label = "K";
        plot.lower_style.visible = false;
        plot.upper_style.visible = false;
        for (std::size_t index = 0; index < extracted.values.size(); ++index) {
            plot.x_values.push_back(static_cast<double>(index + 1));
            plot.values.push_back(extracted.values[index]);
            plot.center.push_back(result.criterion);
        }
        plot.source_rows = extracted.source_rows;
        page.plots.push_back(std::move(plot));
    }
    domain::NonparametricFacts facts;
    facts.method = "runs_test";
    facts.statistic = static_cast<double>(result.observed_runs);
    facts.p_value = result.p_value;
    facts.tie_correction = false;
    facts.continuity_correction = false;
    facts.approximation = result.approximation;
    facts.small_sample_warning = result.small_sample_warning;
    facts.group_count = 1;
    facts.plot_point_count = extracted.values.size();
    facts.missing_count = extracted.missing_count + extracted.invalid_count;
    facts.location_estimate = result.criterion;
    page.facts.nonparametric = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::fisher_exact(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() != 2) {
        return error_page("Fisher 精确检验", "Fisher Exact",
                          "请选择正好两列分类变量以构建 2×2 表。");
    }
    const std::size_t row_col = configuration.variable_columns[0];
    const std::size_t col_col = configuration.variable_columns[1];
    std::set<std::size_t> excluded(
        configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend());
    std::map<std::string, std::size_t> row_indices;
    std::map<std::string, std::size_t> column_indices;
    std::vector<std::vector<std::size_t>> counts;
    std::size_t missing_count = 0;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (excluded.count(row_index) != 0) {
            continue;
        }
        const auto& row = table.rows[row_index];
        const std::string row_label =
            row_col < row.size() ? row[row_col] : "";
        const std::string column_label_text =
            col_col < row.size() ? row[col_col] : "";
        if (is_missing_cell(row_label) || is_missing_cell(column_label_text)) {
            ++missing_count;
            continue;
        }
        const auto row_result = row_indices.emplace(row_label, row_indices.size());
        const auto column_result =
            column_indices.emplace(column_label_text, column_indices.size());
        if (row_result.second) {
            counts.emplace_back(column_indices.size(), 0);
        }
        for (auto& values : counts) {
            values.resize(column_indices.size(), 0);
        }
        counts[row_result.first->second][column_result.first->second] += 1;
    }
    OutputPage page;
    page.id = new_id("fisher_exact");
    page.title = "Fisher 精确检验";
    page.method_name = "Fisher Exact";
    page.configuration = configuration;
    page.parameter_summary =
        column_label(table, row_col) + " × " + column_label(table, col_col)
        + "    N* = " + std::to_string(missing_count);
    if (row_indices.size() != 2 || column_indices.size() != 2
        || counts.size() != 2 || counts.front().size() != 2) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::error,
            "fisher_not_2x2",
            "Fisher 精确检验要求恰好 2×2 水平；当前行水平数 = "
                + std::to_string(row_indices.size())
                + "，列水平数 = " + std::to_string(column_indices.size()) + "。"});
        domain::ChiSquareFacts facts;
        facts.method = "fisher_exact";
        facts.row_count = row_indices.size();
        facts.column_count = column_indices.size();
        facts.missing_count = missing_count;
        page.facts.chi_square = std::move(facts);
        return finalize_page(std::move(page));
    }
    std::vector<std::string> row_labels(2);
    std::vector<std::string> column_labels(2);
    for (const auto& [label, index] : row_indices) {
        row_labels[index] = label;
    }
    for (const auto& [label, index] : column_indices) {
        column_labels[index] = label;
    }
    const std::size_t a = counts[0][0];
    const std::size_t b = counts[0][1];
    const std::size_t c = counts[1][0];
    const std::size_t d = counts[1][1];
    const auto result = datalab::domain::statistics::fisher_exact_2x2(
        a, b, c, d, row_labels[0], row_labels[1],
        column_labels[0], column_labels[1]);
    page.diagnostics = result.diagnostics;
    if (missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "Fisher 精确检验按 complete-case 跳过 "
                + std::to_string(missing_count) + " 个缺失单元格。"});
    }
    const std::size_t total = a + b + c + d;
    const std::size_t row0 = a + b;
    const std::size_t row1 = c + d;
    const std::size_t col0 = a + c;
    const std::size_t col1 = b + d;
    StatisticTable cross;
    cross.title = "交叉表";
    cross.headers = {"", column_labels[0], column_labels[1], "合计"};
    cross.rows.push_back({
        row_labels[0], std::to_string(a), std::to_string(b), std::to_string(row0)});
    cross.rows.push_back({
        row_labels[1], std::to_string(c), std::to_string(d), std::to_string(row1)});
    cross.rows.push_back({
        "合计", std::to_string(col0), std::to_string(col1), std::to_string(total)});
    page.tables.push_back(std::move(cross));
    StatisticTable test;
    test.title = "Fisher精确检验";
    test.headers = {"方法", "P-Value", "优势比 OR", "a", "b", "c", "d"};
    test.rows.push_back({
        "Fisher exact",
        format_optional(result.p_value),
        format_optional(result.odds_ratio),
        std::to_string(a), std::to_string(b),
        std::to_string(c), std::to_string(d)});
    page.tables.push_back(std::move(test));
    domain::ChiSquareFacts facts;
    facts.method = "fisher_exact";
    facts.fisher_p_value = result.p_value;
    facts.p_value = result.p_value;
    facts.odds_ratio = result.odds_ratio;
    facts.row_count = 2;
    facts.column_count = 2;
    facts.total_count = total;
    facts.missing_count = missing_count;
    {
        PlotSpec heatmap;
        heatmap.kind = PlotKind::heatmap;
        heatmap.title = "观察频数热图";
        heatmap.x_axis_title = column_label(table, col_col);
        heatmap.y_axis_title = column_label(table, row_col);
        heatmap.categories = row_labels;
        heatmap.matrix_labels = column_labels;
        heatmap.matrix_values = {
            {static_cast<double>(a), static_cast<double>(b)},
            {static_cast<double>(c), static_cast<double>(d)}};
        const double max_count = static_cast<double>(
            std::max(std::max(a, b), std::max(c, d)));
        heatmap.color_min = 0.0;
        heatmap.color_max = max_count > 0.0 ? max_count : 1.0;
        page.plots.push_back(std::move(heatmap));
        facts.plot_available = true;
    }
    page.facts.chi_square = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::mcnemar(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() != 2) {
        return error_page("McNemar 检验", "McNemar",
                          "请选择正好两列配对二元结果。");
    }
    const std::size_t first_col = configuration.variable_columns[0];
    const std::size_t second_col = configuration.variable_columns[1];
    std::set<std::size_t> excluded(
        configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend());
    std::vector<std::string> first_labels;
    std::vector<std::string> second_labels;
    std::size_t missing_count = 0;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (excluded.count(row_index) != 0) {
            continue;
        }
        const auto& row = table.rows[row_index];
        const std::string left =
            first_col < row.size() ? row[first_col] : "";
        const std::string right =
            second_col < row.size() ? row[second_col] : "";
        if (is_missing_cell(left) || is_missing_cell(right)) {
            ++missing_count;
            continue;
        }
        first_labels.push_back(left);
        second_labels.push_back(right);
    }
    const auto result = datalab::domain::statistics::mcnemar_test(
        first_labels, second_labels);
    OutputPage page;
    page.id = new_id("mcnemar");
    page.title = "McNemar 检验";
    page.method_name = "McNemar";
    page.configuration = configuration;
    page.parameter_summary =
        (first_col < table.columns.size() ? table.columns[first_col]
                                          : std::to_string(first_col))
        + " vs "
        + (second_col < table.columns.size() ? table.columns[second_col]
                                             : std::to_string(second_col));
    page.diagnostics = result.diagnostics;
    if (missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "McNemar 按 complete-case 跳过 " + std::to_string(missing_count)
                + " 个缺失或非法单元格（含 *）。"});
    }
    domain::McNemarFacts facts;
    facts.a = result.a;
    facts.b = result.b;
    facts.c = result.c;
    facts.d = result.d;
    facts.discordant = result.discordant;
    facts.pair_count = result.pair_count;
    facts.missing_count = missing_count;
    facts.degrees_of_freedom = result.degrees_of_freedom;
    facts.continuity_correction = result.continuity_correction;
    facts.method = result.method;
    facts.computable = result.diagnostics.empty() && result.p_value.has_value();
    if (facts.computable) {
        facts.chi_square = result.chi_square;
        facts.p_value = result.p_value;
        StatisticTable cross;
        cross.title = "2×2 交叉表";
        cross.headers = {"单元格", "计数"};
        cross.rows.push_back({"+ / + (a)", std::to_string(result.a)});
        cross.rows.push_back({"+ / − (b)", std::to_string(result.b)});
        cross.rows.push_back({"− / + (c)", std::to_string(result.c)});
        cross.rows.push_back({"− / − (d)", std::to_string(result.d)});
        cross.rows.push_back({"不一致对数 b+c", std::to_string(result.discordant)});
        cross.rows.push_back({"有效配对数", std::to_string(result.pair_count)});
        page.tables.push_back(std::move(cross));
        StatisticTable test;
        test.title = "McNemar 检验";
        test.headers = {"χ² (Edwards)", "DF", "P-Value", "连续性校正", "方法"};
        test.rows.push_back({
            format_number(result.chi_square),
            format_number(result.degrees_of_freedom),
            format_optional(result.p_value),
            result.continuity_correction ? "是" : "否",
            result.method});
        page.tables.push_back(std::move(test));
    }
    page.facts.mcnemar = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::cochran_q(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() < 2) {
        return error_page("Cochran Q 检验", "Cochran Q",
                          "请选择至少两列配对二元结果（k≥3 才计算；k=2 请用 McNemar）。");
    }
    if (configuration.variable_columns.size() == 2) {
        OutputPage page;
        page.id = new_id("cochran_q");
        page.title = "Cochran Q 检验";
        page.method_name = "Cochran Q";
        page.configuration = configuration;
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::error,
            "cochran_use_mcnemar",
            "Cochran Q 要求至少 3 个处理列；两列配对请用 McNemar。"});
        domain::CochranQFacts facts;
        facts.treatment_count = 2;
        facts.computable = false;
        page.facts.cochran_q = std::move(facts);
        return finalize_page(std::move(page));
    }

    std::set<std::size_t> excluded(
        configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend());
    std::vector<std::vector<std::string>> columns;
    std::vector<std::string> labels;
    columns.reserve(configuration.variable_columns.size());
    for (const std::size_t column : configuration.variable_columns) {
        columns.emplace_back();
        labels.push_back(column < table.columns.size()
                             ? table.columns[column]
                             : std::to_string(column));
    }
    std::size_t missing_count = 0;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (excluded.count(row_index) != 0) {
            continue;
        }
        const auto& row = table.rows[row_index];
        bool any_missing = false;
        std::vector<std::string> cells;
        cells.reserve(configuration.variable_columns.size());
        for (const std::size_t column : configuration.variable_columns) {
            const std::string cell = column < row.size() ? row[column] : "";
            if (is_missing_cell(cell)) {
                any_missing = true;
                break;
            }
            cells.push_back(cell);
        }
        if (any_missing) {
            ++missing_count;
            continue;
        }
        for (std::size_t index = 0; index < cells.size(); ++index) {
            columns[index].push_back(cells[index]);
        }
    }

    OutputPage page;
    page.id = new_id("cochran_q");
    page.title = "Cochran Q 检验";
    page.method_name = "Cochran Q";
    page.configuration = configuration;
    page.parameter_summary = "处理列数 = "
        + std::to_string(configuration.variable_columns.size());

    std::map<std::string, bool> level_map;
    std::vector<datalab::domain::DiagnosticMessage> encode_diagnostics;
    if (!datalab::domain::statistics::encode_paired_binary_levels(
            columns, level_map, encode_diagnostics)) {
        page.diagnostics = std::move(encode_diagnostics);
        domain::CochranQFacts facts;
        facts.treatment_count = configuration.variable_columns.size();
        facts.missing_count = missing_count;
        facts.computable = false;
        page.facts.cochran_q = std::move(facts);
        return finalize_page(std::move(page));
    }

    const std::size_t n = columns.empty() ? 0 : columns.front().size();
    std::vector<std::vector<int>> binary_rows(n);
    for (std::size_t row = 0; row < n; ++row) {
        binary_rows[row].resize(columns.size());
        for (std::size_t col = 0; col < columns.size(); ++col) {
            bool positive = false;
            if (!datalab::domain::statistics::resolve_binary_label(
                    columns[col][row], level_map, positive)) {
                page.diagnostics.push_back({
                    DiagnosticMessage::Severity::error,
                    "cochran_not_binary",
                    "存在无法识别为二元水平的标签。"});
                domain::CochranQFacts facts;
                facts.treatment_count = columns.size();
                facts.missing_count = missing_count;
                facts.computable = false;
                page.facts.cochran_q = std::move(facts);
                return finalize_page(std::move(page));
            }
            binary_rows[row][col] = positive ? 1 : 0;
        }
    }

    const auto result = datalab::domain::statistics::cochran_q_test(binary_rows, labels);
    page.diagnostics = result.diagnostics;
    if (missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "Cochran Q 按 complete-case 跳过 " + std::to_string(missing_count)
                + " 行缺失（含 *）。"});
    }

    domain::CochranQFacts facts;
    facts.treatment_count = result.treatment_count;
    facts.subject_count = result.subject_count;
    facts.missing_count = missing_count;
    facts.degrees_of_freedom = result.degrees_of_freedom;
    facts.approximation = result.approximation;
    facts.computable = result.computable && result.p_value.has_value();
    if (facts.computable) {
        facts.q_statistic = result.q_statistic;
        facts.p_value = result.p_value;
        StatisticTable counts;
        counts.title = "处理成功计数";
        counts.headers = {"处理", "成功数", "成功率", "N"};
        for (const auto& treatment : result.treatments) {
            counts.rows.push_back({
                treatment.label,
                std::to_string(treatment.success_count),
                format_number(treatment.success_rate),
                std::to_string(result.subject_count)});
        }
        page.tables.push_back(std::move(counts));
        StatisticTable test;
        test.title = "Cochran Q 检验";
        test.headers = {"Q", "DF", "P-Value", "近似方法"};
        test.rows.push_back({
            format_number(result.q_statistic),
            format_number(result.degrees_of_freedom),
            format_optional(result.p_value),
            result.approximation});
        page.tables.push_back(std::move(test));

        PlotSpec bar;
        bar.kind = PlotKind::pareto;
        bar.title = "各处理阳性率";
        bar.x_axis_title = "处理";
        bar.y_axis_title = "阳性率";
        for (const auto& treatment : result.treatments) {
            bar.categories.push_back(treatment.label);
            bar.values.push_back(treatment.success_rate);
        }
        page.plots.push_back(std::move(bar));
    }
    page.facts.cochran_q = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::mood_median(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.empty() || !configuration.by_column.has_value()) {
        return error_page("Mood 中位数检验", "Mood median",
                          "请选择测量列和分组列。");
    }
    const auto extracted = extract_numeric_column(
        table, configuration.variable_columns.front(), configuration.excluded_rows);
    const auto labels = extract_text_column(table, *configuration.by_column);
    std::vector<std::string> order;
    std::vector<std::vector<double>> grouped;
    std::vector<std::vector<std::size_t>> grouped_rows;
    for (std::size_t index = 0; index < extracted.values.size(); ++index) {
        const std::size_t row = extracted.source_rows[index];
        if (row >= labels.size() || is_missing_cell(labels[row])) {
            continue;
        }
        const std::string label = labels[row];
        const std::size_t group_index = datalab::domain::stable_group_index(order, label);
        if (group_index >= grouped.size()) {
            grouped.emplace_back();
            grouped_rows.emplace_back();
        }
        grouped[group_index].push_back(extracted.values[index]);
        grouped_rows[group_index].push_back(row);
    }
    std::vector<std::vector<double>> groups;
    std::vector<std::string> group_labels;
    for (std::size_t group = 0; group < order.size(); ++group) {
        if (!grouped[group].empty()) {
            group_labels.push_back(order[group]);
            groups.push_back(grouped[group]);
        }
    }
    double confidence = 0.95;
    if (configuration.inference.confidence_level > 0.0) {
        confidence = configuration.inference.confidence_level;
        if (confidence > 1.0) {
            confidence /= 100.0;
        }
    }
    const auto result = datalab::domain::statistics::mood_median_test(
        groups, group_labels, confidence);
    OutputPage page;
    page.id = new_id("mood_median");
    page.title = "Mood 中位数检验";
    page.method_name = "Mood median";
    page.configuration = configuration;
    page.parameter_summary = "测量值 = " + extracted.name
        + "；总体中位数 M = " + format_number(result.overall_median);
    page.diagnostics = result.diagnostics;
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "Mood 跳过 " + std::to_string(extracted.missing_count)
                + " 个缺失或非法单元格（含 *）。"});
    }

    StatisticTable summary;
    summary.title = "各组 Above/Below";
    summary.headers = {"组别", "N", "组中位数", "N≤", "N>", "CI 下限", "CI 上限"};
    for (const auto& group : result.groups) {
        summary.rows.push_back({
            group.label,
            std::to_string(group.count),
            format_number(group.median),
            std::to_string(group.n_le),
            std::to_string(group.n_gt),
            format_optional(group.ci_lower),
            format_optional(group.ci_upper)});
    }
    page.tables.push_back(std::move(summary));

    if (result.p_value.has_value()) {
        StatisticTable test;
        test.title = "Mood 中位数检验";
        test.headers = {"总体中位数 M", "χ²", "DF", "P-Value", "近似方法"};
        test.rows.push_back({
            format_number(result.overall_median),
            format_number(result.chi_square),
            format_number(result.degrees_of_freedom),
            format_optional(result.p_value),
            result.approximation});
        page.tables.push_back(std::move(test));
    }

    auto plots = make_grouped_distribution_plots("箱线图", "个体值图");
    for (std::size_t group = 0; group < order.size(); ++group) {
        if (grouped[group].empty()) {
            continue;
        }
        append_group_to_distribution_plots(
            plots, order[group], grouped[group], grouped_rows[group]);
    }
    push_distribution_plots(page, plots);

    domain::NonparametricFacts facts;
    facts.method = "mood_median";
    facts.statistic = result.chi_square;
    facts.p_value = result.p_value;
    facts.approximation = result.approximation;
    facts.small_sample_warning = result.small_sample_warning;
    facts.group_count = result.groups.size();
    facts.plot_point_count = plots.point_count;
    facts.missing_count = extracted.missing_count;
    page.facts.nonparametric = std::move(facts);
    return finalize_page(std::move(page));
}


OutputPage AnalysisService::kruskal_wallis(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.empty() || !configuration.by_column.has_value()) {
        return error_page("Kruskal-Wallis 检验", "Kruskal-Wallis", "请选择测量列和分组列。");
    }
    const auto extracted = extract_numeric_column(
        table, configuration.variable_columns.front(), configuration.excluded_rows);
    const auto labels = extract_text_column(table, *configuration.by_column);
    std::vector<std::string> order;
    std::vector<std::vector<double>> grouped;
    std::vector<std::vector<std::size_t>> grouped_rows;
    for (std::size_t index = 0; index < extracted.values.size(); ++index) {
        const std::size_t row = extracted.source_rows[index];
        if (row >= labels.size() || is_missing_cell(labels[row])) {
            continue;
        }
        const std::string label = labels[row];
        const std::size_t group_index = datalab::domain::stable_group_index(order, label);
        if (group_index >= grouped.size()) {
            grouped.emplace_back();
            grouped_rows.emplace_back();
        }
        grouped[group_index].push_back(extracted.values[index]);
        grouped_rows[group_index].push_back(row);
    }
    std::vector<std::vector<double>> groups;
    std::vector<std::string> group_labels;
    for (std::size_t group = 0; group < order.size(); ++group) {
        if (!grouped[group].empty()) {
            group_labels.push_back(order[group]);
            groups.push_back(grouped[group]);
        }
    }
    const auto result = datalab::domain::statistics::kruskal_wallis(groups, group_labels);
    OutputPage page;
    page.id = new_id("kruskal_wallis");
    page.title = "Kruskal-Wallis 检验";
    page.method_name = "Kruskal-Wallis";
    page.configuration = configuration;
    page.parameter_summary = "测量值 = " + extracted.name;
    page.diagnostics = result.diagnostics;
    StatisticTable output;
    output.title = "Kruskal-Wallis 结果";
    output.headers = {"组别", "N", "中位数", "平均秩", "Z"};
    for (const auto& group : result.groups) {
        output.rows.push_back({group.label, std::to_string(group.count),
            format_number(group.median), format_number(group.mean_rank),
            format_optional(group.z_value)});
    }
    page.tables.push_back(output);
    StatisticTable summary;
    summary.title = "检验统计量";
    summary.headers = {"H", "调整后 H", "DF", "P-Value", "未调整 P", "Ties 修正",
                       "小样本警告", "近似方法", "效应量"};
    summary.rows.push_back({format_number(result.h_statistic),
        format_number(result.adjusted_h_statistic), format_number(result.degrees_of_freedom),
        format_optional(result.p_value), format_optional(result.p_value_unadjusted),
        result.tie_correction ? "是" : "否",
        result.small_sample_warning ? "是" : "否",
        result.approximation,
        format_optional(result.effect_size)});
    page.tables.push_back(summary);
    const bool use_steel_dwass =
        configuration.inference.nonparametric_posthoc == "steel_dwass";
    const auto posthoc = use_steel_dwass
        ? datalab::domain::statistics::steel_dwass_pairwise(
              groups, group_labels, result.family_alpha)
        : result.dunn_comparisons;
    if (!posthoc.empty()) {
        StatisticTable pairs;
        pairs.title = use_steel_dwass ? "Steel-Dwass 成对比较" : "Dunn 成对比较";
        pairs.headers = use_steel_dwass
            ? std::vector<std::string>{"对比", "Z", "未调整 P", "Bonferroni P", "显著"}
            : std::vector<std::string>{
                  "对比", "平均秩差", "SE", "Z", "未调整 P", "Bonferroni P", "显著"};
        for (const auto& comparison : posthoc) {
            if (use_steel_dwass) {
                pairs.rows.push_back({
                    comparison.first_label + " - " + comparison.second_label,
                    format_number(comparison.z_statistic),
                    format_optional(comparison.p_value),
                    format_optional(comparison.adjusted_p_value),
                    comparison.significant ? "是" : "否"});
            } else {
                pairs.rows.push_back({
                    comparison.first_label + " - " + comparison.second_label,
                    format_number(comparison.mean_rank_difference),
                    format_number(comparison.standard_error),
                    format_number(comparison.z_statistic),
                    format_optional(comparison.p_value),
                    format_optional(comparison.adjusted_p_value),
                    comparison.significant ? "是" : "否"});
            }
        }
        page.tables.push_back(std::move(pairs));

        std::vector<datalab::domain::statistics::TukeyComparison> tukey_like;
        for (const auto& comparison : posthoc) {
            datalab::domain::statistics::TukeyComparison row;
            row.first_label = comparison.first_label;
            row.second_label = comparison.second_label;
            row.mean_difference = comparison.mean_rank_difference;
            row.standard_error = comparison.standard_error;
            row.significant = comparison.significant;
            tukey_like.push_back(std::move(row));
        }
        std::vector<std::string> grouping_labels;
        std::vector<double> grouping_means;
        std::vector<std::size_t> grouping_counts;
        for (const auto& group : result.groups) {
            grouping_labels.push_back(group.label);
            grouping_means.push_back(group.median);
            grouping_counts.push_back(group.count);
        }
        const auto letters = datalab::domain::statistics::tukey_grouping_letters(
            grouping_labels, grouping_means, grouping_counts, tukey_like);
        if (!letters.empty()) {
            StatisticTable grouping;
            grouping.title = use_steel_dwass
                ? "Grouping Information (Steel-Dwass)"
                : "Grouping Information (Dunn)";
            grouping.headers = {"水平", "N", "中位数", "Grouping"};
            for (const auto& row : letters) {
                grouping.rows.push_back({
                    row.label, std::to_string(row.count),
                    format_number(row.mean), row.grouping});
            }
            page.tables.push_back(std::move(grouping));
        }
    }
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "Kruskal–Wallis 跳过 " + std::to_string(extracted.missing_count)
                + " 个缺失或非法单元格（含 *）。"});
    }
    auto plots = make_grouped_distribution_plots("箱线图", "个体值图");
    for (std::size_t group = 0; group < order.size(); ++group) {
        if (group < grouped.size() && !grouped[group].empty()) {
            append_group_to_distribution_plots(
                plots, order[group], grouped[group], grouped_rows[group]);
        }
    }
    push_distribution_plots(page, plots);
    domain::NonparametricFacts facts;
    facts.method = "kruskal_wallis";
    facts.statistic = result.adjusted_h_statistic;
    facts.p_value = result.p_value;
    facts.tie_correction = result.tie_correction;
    facts.continuity_correction = false;
    facts.approximation = result.approximation;
    facts.small_sample_warning = result.small_sample_warning;
    facts.effect_size = result.effect_size;
    facts.p_value_unadjusted = result.p_value_unadjusted;
    facts.group_count = group_labels.size();
    facts.plot_point_count = plots.point_count;
    facts.missing_count = extracted.missing_count;
    facts.posthoc_method = use_steel_dwass ? "steel_dwass" : "dunn";
    facts.dunn_available = !use_steel_dwass && !posthoc.empty();
    facts.steel_dwass_available = use_steel_dwass && !posthoc.empty();
    facts.posthoc_pair_count = posthoc.size();
    facts.grouping_letter_count = 0;
    for (const auto& table_out : page.tables) {
        if (table_out.title.find("Grouping Information") != std::string::npos) {
            facts.grouping_letter_count = table_out.rows.size();
            break;
        }
    }
    page.facts.nonparametric = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::friedman(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.empty() || !configuration.by_column.has_value()
        || !configuration.inference.anova_factor_b_column.has_value()) {
        return error_page("Friedman 检验", "Friedman",
                          "请选择响应、处理与区组列。");
    }
    const std::size_t response_column = configuration.variable_columns.front();
    const std::size_t treatment_column = *configuration.by_column;
    const std::size_t block_column = *configuration.inference.anova_factor_b_column;
    std::set<std::size_t> excluded(
        configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend());
    std::vector<double> responses;
    std::vector<std::string> treatments;
    std::vector<std::string> blocks;
    std::vector<std::size_t> source_rows;
    std::size_t missing_count = 0;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (excluded.count(row_index) != 0) {
            continue;
        }
        const auto& row = table.rows[row_index];
        const std::string response_text =
            response_column < row.size() ? row[response_column] : "";
        const std::string treatment_text =
            treatment_column < row.size() ? row[treatment_column] : "";
        const std::string block_text =
            block_column < row.size() ? row[block_column] : "";
        const auto numeric = parse_numeric_cell(response_text);
        if (!numeric.has_value() || is_missing_cell(treatment_text)
            || is_missing_cell(block_text)) {
            ++missing_count;
            continue;
        }
        responses.push_back(*numeric);
        treatments.push_back(treatment_text);
        blocks.push_back(block_text);
        source_rows.push_back(row_index);
    }
    const auto result = datalab::domain::statistics::friedman_test(
        responses, treatments, blocks);
    OutputPage page;
    page.id = new_id("friedman");
    page.title = "Friedman 检验";
    page.method_name = "Friedman";
    page.configuration = configuration;
    page.parameter_summary =
        "响应 = " + (response_column < table.columns.size()
                         ? table.columns[response_column] : std::to_string(response_column))
        + "    处理 = "
        + (treatment_column < table.columns.size()
               ? table.columns[treatment_column] : std::to_string(treatment_column))
        + "    区组 = "
        + (block_column < table.columns.size()
               ? table.columns[block_column] : std::to_string(block_column));
    page.diagnostics = result.diagnostics;
    if (missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "Friedman 按 complete-case 跳过 " + std::to_string(missing_count)
                + " 个缺失或非法单元格（含 *）。"});
    }
    if (!result.diagnostics.empty()) {
        domain::NonparametricFacts facts;
        facts.method = "friedman";
        facts.missing_count = missing_count;
        facts.group_count = result.treatment_count;
        page.facts.nonparametric = std::move(facts);
        return finalize_page(std::move(page));
    }
    StatisticTable summary;
    summary.title = "处理摘要";
    summary.headers = {"处理", "N", "中位数", "平均秩"};
    for (const auto& treatment : result.treatments) {
        summary.rows.push_back({
            treatment.label, std::to_string(treatment.count),
            format_number(treatment.median), format_number(treatment.mean_rank)});
    }
    page.tables.push_back(std::move(summary));
    StatisticTable test;
    test.title = "Friedman 检验";
    test.headers = {"S", "调整后 S", "DF", "P-Value", "Ties 修正", "区组数", "处理数"};
    test.rows.push_back({
        format_number(result.s_statistic),
        format_number(result.adjusted_s_statistic),
        format_number(result.degrees_of_freedom),
        format_optional(result.p_value),
        result.tie_correction ? "是" : "否",
        std::to_string(result.block_count),
        std::to_string(result.treatment_count)});
    page.tables.push_back(std::move(test));

    const bool use_nemenyi =
        configuration.inference.nonparametric_posthoc == "nemenyi";
    std::vector<datalab::domain::statistics::DunnComparison> nemenyi;
    if (use_nemenyi) {
        nemenyi = datalab::domain::statistics::nemenyi_pairwise(result, 0.05);
        if (!nemenyi.empty()) {
            StatisticTable pairs;
            pairs.title = "Nemenyi 成对比较";
            pairs.headers = {
                "对比", "平均秩差", "SE", "Z", "未调整 P", "Bonferroni P", "显著"};
            for (const auto& comparison : nemenyi) {
                pairs.rows.push_back({
                    comparison.first_label + " - " + comparison.second_label,
                    format_number(comparison.mean_rank_difference),
                    format_number(comparison.standard_error),
                    format_number(comparison.z_statistic),
                    format_optional(comparison.p_value),
                    format_optional(comparison.adjusted_p_value),
                    comparison.significant ? "是" : "否"});
            }
            page.tables.push_back(std::move(pairs));

            std::vector<datalab::domain::statistics::TukeyComparison> tukey_like;
            for (const auto& comparison : nemenyi) {
                datalab::domain::statistics::TukeyComparison row;
                row.first_label = comparison.first_label;
                row.second_label = comparison.second_label;
                row.mean_difference = comparison.mean_rank_difference;
                row.standard_error = comparison.standard_error;
                row.significant = comparison.significant;
                tukey_like.push_back(std::move(row));
            }
            std::vector<std::string> grouping_labels;
            std::vector<double> grouping_means;
            std::vector<std::size_t> grouping_counts;
            for (const auto& treatment : result.treatments) {
                grouping_labels.push_back(treatment.label);
                grouping_means.push_back(treatment.median);
                grouping_counts.push_back(treatment.count);
            }
            const auto letters = datalab::domain::statistics::tukey_grouping_letters(
                grouping_labels, grouping_means, grouping_counts, tukey_like);
            if (!letters.empty()) {
                StatisticTable grouping;
                grouping.title = "Grouping Information (Nemenyi)";
                grouping.headers = {"水平", "N", "中位数", "Grouping"};
                for (const auto& row : letters) {
                    grouping.rows.push_back({
                        row.label, std::to_string(row.count),
                        format_number(row.mean), row.grouping});
                }
                page.tables.push_back(std::move(grouping));
            }
        }
    }

    std::vector<std::string> order;
    std::vector<std::vector<double>> grouped;
    std::vector<std::vector<std::size_t>> grouped_rows;
    for (std::size_t index = 0; index < responses.size(); ++index) {
        const std::size_t group_index =
            datalab::domain::stable_group_index(order, treatments[index]);
        if (group_index >= grouped.size()) {
            grouped.emplace_back();
            grouped_rows.emplace_back();
        }
        grouped[group_index].push_back(responses[index]);
        grouped_rows[group_index].push_back(source_rows[index]);
    }
    auto plots = make_grouped_distribution_plots("箱线图", "个体值图");
    for (std::size_t group = 0; group < order.size(); ++group) {
        append_group_to_distribution_plots(
            plots, order[group], grouped[group], grouped_rows[group]);
    }
    push_distribution_plots(page, plots);

    domain::NonparametricFacts facts;
    facts.method = "friedman";
    facts.statistic = result.adjusted_s_statistic;
    facts.p_value = result.p_value;
    facts.tie_correction = result.tie_correction;
    facts.continuity_correction = false;
    facts.approximation = result.approximation;
    facts.group_count = result.treatment_count;
    facts.plot_point_count = plots.point_count;
    facts.missing_count = missing_count;
    facts.posthoc_method = use_nemenyi ? "nemenyi" : "";
    facts.nemenyi_available = use_nemenyi && !nemenyi.empty();
    facts.posthoc_pair_count = nemenyi.size();
    facts.grouping_letter_count = 0;
    for (const auto& table_out : page.tables) {
        if (table_out.title.find("Grouping Information") != std::string::npos) {
            facts.grouping_letter_count = table_out.rows.size();
            break;
        }
    }
    page.facts.nonparametric = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::ewma(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto extracted = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    datalab::domain::statistics::EwmaOptions options;
    options.lambda = configuration.control.ewma_lambda;
    options.limit_sigma = configuration.control.ewma_limit_sigma;
    options.historical_mean = configuration.control.historical_center;
    options.historical_sigma = configuration.control.historical_sigma;
    options.special_causes =
        datalab::domain::statistics::special_cause_selection_from_configuration(
            configuration.control.enabled_special_cause_tests,
            configuration.control.special_cause_rule_policy);
    const auto chart = datalab::domain::statistics::ControlCharts::ewma_chart(
        extracted.values, options);
    OutputPage page;
    page.id = new_id("ewma");
    page.title = "EWMA 控制图";
    page.method_name = "EWMA Chart";
    page.configuration = configuration;
    page.diagnostics = chart.diagnostics;
    const bool historical = configuration.control.historical_center.has_value()
        || configuration.control.historical_sigma.has_value();
    const double mean = options.historical_mean.value_or(
        extracted.values.empty() ? 0.0
            : std::accumulate(extracted.values.cbegin(), extracted.values.cend(), 0.0)
                / static_cast<double>(extracted.values.size()));
    double sigma = options.historical_sigma.value_or(0.0);
    if (!options.historical_sigma.has_value() && extracted.values.size() > 1) {
        double sum_squared = 0.0;
        for (const double value : extracted.values) {
            sum_squared += (value - mean) * (value - mean);
        }
        sigma = std::sqrt(sum_squared / static_cast<double>(extracted.values.size() - 1));
    }
    page.parameter_summary = "变量 = " + extracted.name
        + "    λ = " + format_number(options.lambda)
        + "    控制限倍数 = " + format_number(options.limit_sigma)
        + (historical ? "（历史参数）" : "（估计）");
    StatisticTable parameters;
    parameters.title = "EWMA 参数";
    parameters.headers = {"指标", "数值"};
    parameters.rows = {
        {"N", std::to_string(extracted.values.size())},
        {"N*", std::to_string(extracted.missing_count)},
        {"λ", format_number(options.lambda)},
        {"控制限倍数", format_number(options.limit_sigma)},
        {"μ", format_number(mean)},
        {"σ (within)", format_number(sigma)},
        {"参数来源", historical ? "历史参数" : "估计"},
        {"「单点超出 3σ 控制限」触发点数", std::to_string(chart.test1_points.size())},
        {"规则策略", configuration.control.special_cause_rule_policy == "explicit"
            ? "用户指定"
            : (configuration.control.special_cause_rule_policy == "minitab_like"
                   || configuration.control.special_cause_rule_policy == "default_minitab_like"
                   ? "minitab_like（仅「单点超出 3σ 控制限」）"
                   : "all_applicable（全部适用）")},
        {"启用测试", datalab::domain::statistics::format_special_cause_tests(
            datalab::domain::statistics::resolve_special_cause_tests(
                options.special_causes,
                datalab::domain::statistics::ControlChartKind::ewma))},
        {"适用性", "EWMA 只开放「单点超出 3σ 控制限」；其余特殊原因规则不附加到 EWMA。"}};
    page.tables.push_back(parameters);
    page.tables.push_back(ewma_point_table(chart, extracted.values, extracted.source_rows));
    page.plots.push_back(control_plot("EWMA 控制图", "EWMA", chart, extracted.source_rows));
    page.facts.spc = domain::SpcFacts{};
    page.facts.spc->sigma_within =
        sigma > 0.0 ? std::optional<double>{sigma} : std::nullopt;
    page.facts.spc->out_of_control_count = chart.test1_points.size();
    return finalize_page(std::move(page));
}

namespace {

struct MultivariateMatrix {
    bool ok = false;
    std::string error;
    std::vector<std::vector<double>> rows;
    std::vector<std::size_t> source_rows;
    std::vector<std::string> variable_names;
};

MultivariateMatrix extract_multivariate_matrix(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    MultivariateMatrix matrix;
    if (configuration.variable_columns.size() < 2) {
        matrix.error = "请至少选择两个数值变量列。";
        return matrix;
    }
    std::vector<ExtractedNumericColumn> columns;
    for (const std::size_t column : configuration.variable_columns) {
        columns.push_back(extract_numeric_column(table, column, configuration.excluded_rows));
        matrix.variable_names.push_back(columns.back().name);
    }
    std::map<std::size_t, std::vector<double>> by_row;
    for (std::size_t column = 0; column < columns.size(); ++column) {
        for (std::size_t index = 0; index < columns[column].values.size(); ++index) {
            by_row[columns[column].source_rows[index]].resize(columns.size(),
                std::numeric_limits<double>::quiet_NaN());
            by_row[columns[column].source_rows[index]][column] = columns[column].values[index];
        }
    }
    for (const auto& [row, values] : by_row) {
        bool complete = true;
        for (const double value : values) {
            if (!std::isfinite(value)) {
                complete = false;
                break;
            }
        }
        if (!complete) {
            continue;
        }
        matrix.rows.push_back(values);
        matrix.source_rows.push_back(row);
    }
    if (matrix.rows.size() < 3) {
        matrix.error = "complete-case 多元观测不足。";
        return matrix;
    }
    matrix.ok = true;
    return matrix;
}

}  // namespace

OutputPage AnalysisService::hotelling_t2(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto matrix = extract_multivariate_matrix(table, configuration);
    if (!matrix.ok) {
        return error_page("Hotelling T²", "Hotelling T2", matrix.error);
    }
    datalab::domain::statistics::HotellingT2Options options;
    options.phase = configuration.control.sigma_method == "phase2" ? "phase2" : "phase1";
    options.alpha = configuration.inference.confidence_level > 0.0
            && configuration.inference.confidence_level < 1.0
        ? 1.0 - configuration.inference.confidence_level
        : 0.05;
    // Prefer explicit alpha via power field if set oddly — keep 0.05 default.
    if (configuration.inference.confidence_level <= 0.0
        || configuration.inference.confidence_level >= 1.0) {
        options.alpha = 0.05;
    }
    const auto result = datalab::domain::statistics::hotelling_t2_individuals(
        matrix.rows, matrix.source_rows, options);

    OutputPage page;
    page.id = new_id("t2");
    page.title = "Hotelling T²";
    page.method_name = "Hotelling T2 Individuals";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    std::string names;
    for (std::size_t i = 0; i < matrix.variable_names.size(); ++i) {
        if (i > 0) {
            names += ", ";
        }
        names += matrix.variable_names[i];
    }
    page.parameter_summary = "变量: " + names
        + "    m = " + std::to_string(result.observation_count)
        + "    p = " + std::to_string(result.variable_count)
        + "    phase = " + result.limit_method
        + "    UCL = " + format_number(result.upper_control_limit);

    StatisticTable summary;
    summary.title = "T² 摘要";
    summary.headers = {"指标", "数值"};
    summary.rows = {
        {"m", std::to_string(result.observation_count)},
        {"p", std::to_string(result.variable_count)},
        {"UCL", format_number(result.upper_control_limit)},
        {"限方法", result.limit_method},
        {"超 UCL 点数", std::to_string(result.out_of_control_count)},
    };
    page.tables.push_back(std::move(summary));

    StatisticTable points;
    points.title = "逐点 T²";
    points.headers = {"观测顺序", "source_row", "T²", "信号"};
    for (std::size_t i = 0; i < result.t2.size(); ++i) {
        points.rows.push_back({
            std::to_string(i + 1),
            std::to_string(result.source_rows[i]),
            format_number(result.t2[i]),
            result.t2[i] > result.upper_control_limit ? "超限" : ""});
    }
    page.tables.push_back(std::move(points));

    PlotSpec plot;
    plot.kind = PlotKind::control;
    plot.title = "Hotelling T² 图";
    plot.x_axis_title = "观测顺序";
    plot.y_axis_title = "T²";
    for (std::size_t i = 0; i < result.t2.size(); ++i) {
        plot.x_values.push_back(static_cast<double>(i + 1));
        plot.values.push_back(result.t2[i]);
        plot.center.push_back(0.0);
        plot.lower.push_back(0.0);
        plot.upper.push_back(result.upper_control_limit);
    }
    plot.source_rows = result.source_rows;
    page.plots.push_back(std::move(plot));

    domain::MultivariateSpcFacts facts;
    facts.kind = "hotelling_t2";
    facts.observation_count = result.observation_count;
    facts.variable_count = result.variable_count;
    facts.out_of_control_count = result.out_of_control_count;
    facts.upper_control_limit = result.upper_control_limit;
    facts.limit_method = result.limit_method;
    facts.phase = options.phase;
    page.facts.multivariate_spc = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::mewma(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto matrix = extract_multivariate_matrix(table, configuration);
    if (!matrix.ok) {
        return error_page("MEWMA", "MEWMA", matrix.error);
    }
    datalab::domain::statistics::MewmaOptions options;
    options.lambda = configuration.control.ewma_lambda > 0.0
            && configuration.control.ewma_lambda <= 1.0
        ? configuration.control.ewma_lambda
        : 0.1;
    options.alpha = 0.05;
    if (configuration.control.historical_sigma.has_value()
        && *configuration.control.historical_sigma > 0.0) {
        // Reuse historical_sigma field as optional user UCL for MEWMA.
        options.upper_control_limit = configuration.control.historical_sigma;
    }
    const auto result = datalab::domain::statistics::mewma_chart(
        matrix.rows, matrix.source_rows, options);

    OutputPage page;
    page.id = new_id("mewma");
    page.title = "MEWMA";
    page.method_name = "Multivariate EWMA";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    std::string names;
    for (std::size_t i = 0; i < matrix.variable_names.size(); ++i) {
        if (i > 0) {
            names += ", ";
        }
        names += matrix.variable_names[i];
    }
    page.parameter_summary = "变量: " + names
        + "    λ = " + format_number(result.lambda)
        + "    UCL = " + format_number(result.upper_control_limit)
        + "    (" + result.ucl_method + ")";

    StatisticTable summary;
    summary.title = "MEWMA 摘要";
    summary.headers = {"指标", "数值"};
    summary.rows = {
        {"m", std::to_string(result.observation_count)},
        {"p", std::to_string(result.variable_count)},
        {"λ", format_number(result.lambda)},
        {"UCL", format_number(result.upper_control_limit)},
        {"UCL 方法", result.ucl_method},
        {"超 UCL 点数", std::to_string(result.out_of_control_count)},
    };
    page.tables.push_back(std::move(summary));

    StatisticTable points;
    points.title = "逐点 MEWMA T²";
    points.headers = {"观测顺序", "source_row", "T²", "信号"};
    for (std::size_t i = 0; i < result.t2.size(); ++i) {
        points.rows.push_back({
            std::to_string(i + 1),
            std::to_string(result.source_rows[i]),
            format_number(result.t2[i]),
            result.t2[i] > result.upper_control_limit ? "超限" : ""});
    }
    page.tables.push_back(std::move(points));

    PlotSpec plot;
    plot.kind = PlotKind::control;
    plot.title = "MEWMA T² 图";
    plot.x_axis_title = "观测顺序";
    plot.y_axis_title = "T²";
    for (std::size_t i = 0; i < result.t2.size(); ++i) {
        plot.x_values.push_back(static_cast<double>(i + 1));
        plot.values.push_back(result.t2[i]);
        plot.center.push_back(0.0);
        plot.lower.push_back(0.0);
        plot.upper.push_back(result.upper_control_limit);
    }
    plot.source_rows = result.source_rows;
    page.plots.push_back(std::move(plot));

    domain::MultivariateSpcFacts facts;
    facts.kind = "mewma";
    facts.observation_count = result.observation_count;
    facts.variable_count = result.variable_count;
    facts.out_of_control_count = result.out_of_control_count;
    facts.upper_control_limit = result.upper_control_limit;
    facts.lambda = result.lambda;
    facts.limit_method = result.ucl_method;
    page.facts.multivariate_spc = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::generalized_variance(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto matrix = extract_multivariate_matrix(table, configuration);
    if (!matrix.ok) {
        return error_page("广义方差图", "Generalized Variance", matrix.error);
    }
    const int subgroup_size = configuration.control.subgroup_size.value_or(0);
    if (subgroup_size <= static_cast<int>(matrix.variable_names.size())) {
        return error_page("广义方差图", "Generalized Variance",
                          "子组大小 n 必须大于变量数 p。");
    }
    if (matrix.rows.size() % static_cast<std::size_t>(subgroup_size) != 0) {
        return error_page("广义方差图", "Generalized Variance",
                          "观测数必须能被子组大小整除（等量子组）。");
    }
    std::vector<std::vector<std::vector<double>>> subgroups;
    std::vector<std::size_t> subgroup_source_rows;
    for (std::size_t start = 0; start < matrix.rows.size();
         start += static_cast<std::size_t>(subgroup_size)) {
        std::vector<std::vector<double>> subgroup;
        subgroup.reserve(static_cast<std::size_t>(subgroup_size));
        for (int offset = 0; offset < subgroup_size; ++offset) {
            subgroup.push_back(matrix.rows[start + static_cast<std::size_t>(offset)]);
        }
        subgroups.push_back(std::move(subgroup));
        subgroup_source_rows.push_back(matrix.source_rows[start]);
    }
    const auto result = datalab::domain::statistics::generalized_variance_chart(
        subgroups, subgroup_source_rows);

    OutputPage page;
    page.id = new_id("gv");
    page.title = "广义方差图";
    page.method_name = "Generalized Variance Chart";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    std::string names;
    for (std::size_t i = 0; i < matrix.variable_names.size(); ++i) {
        if (i > 0) {
            names += ", ";
        }
        names += matrix.variable_names[i];
    }
    page.parameter_summary = "变量: " + names
        + "    n = " + std::to_string(result.subgroup_size)
        + "    p = " + std::to_string(result.variable_count)
        + "    子组数 = " + std::to_string(result.subgroup_count)
        + "    CL = " + format_number(result.center_line);

    StatisticTable summary;
    summary.title = "广义方差摘要";
    summary.headers = {"指标", "数值"};
    summary.rows = {
        {"子组数", std::to_string(result.subgroup_count)},
        {"n", std::to_string(result.subgroup_size)},
        {"p", std::to_string(result.variable_count)},
        {"b1", format_number(result.b1)},
        {"b2", format_number(result.b2)},
        {"|Σ̂|", format_number(result.sigma_determinant)},
        {"CL", format_number(result.center_line)},
        {"UCL", format_number(result.upper_control_limit)},
        {"LCL", format_number(result.lower_control_limit)},
        {"超限子组数", std::to_string(result.out_of_control_count)},
    };
    page.tables.push_back(std::move(summary));

    StatisticTable points;
    points.title = "逐子组 |S|";
    points.headers = {"子组", "source_row", "|S|", "信号"};
    for (std::size_t i = 0; i < result.plotted_determinants.size(); ++i) {
        const double value = result.plotted_determinants[i];
        const bool ooc = value > result.upper_control_limit
            || value < result.lower_control_limit;
        points.rows.push_back({
            std::to_string(i + 1),
            std::to_string(result.source_rows[i]),
            format_number(value),
            ooc ? "超限" : ""});
    }
    page.tables.push_back(std::move(points));

    PlotSpec plot;
    plot.kind = PlotKind::control;
    plot.title = "广义方差 |S| 图";
    plot.x_axis_title = "子组";
    plot.y_axis_title = "|S|";
    for (std::size_t i = 0; i < result.plotted_determinants.size(); ++i) {
        plot.x_values.push_back(static_cast<double>(i + 1));
        plot.values.push_back(result.plotted_determinants[i]);
        plot.center.push_back(result.center_line);
        plot.lower.push_back(result.lower_control_limit);
        plot.upper.push_back(result.upper_control_limit);
    }
    plot.source_rows = result.source_rows;
    page.plots.push_back(std::move(plot));

    domain::MultivariateSpcFacts facts;
    facts.kind = "generalized_variance";
    facts.observation_count = matrix.rows.size();
    facts.variable_count = result.variable_count;
    facts.subgroup_count = result.subgroup_count;
    facts.out_of_control_count = result.out_of_control_count;
    facts.upper_control_limit = result.upper_control_limit;
    facts.lower_control_limit = result.lower_control_limit;
    facts.center_line = result.center_line;
    facts.limit_method = "montgomery_b1_b2";
    page.facts.multivariate_spc = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::cusum(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto extracted = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    datalab::domain::statistics::CusumOptions options;
    options.target = configuration.control.cusum_target;
    options.sigma = configuration.control.cusum_sigma;
    options.k = configuration.control.cusum_k;
    options.h = configuration.control.cusum_h;
    options.fast_initial_response = configuration.control.cusum_fast_initial_response;
    const auto chart = datalab::domain::statistics::ControlCharts::cusum_chart(
        extracted.values, options);
    OutputPage page;
    page.id = new_id("cusum");
    page.title = "CUSUM 控制图";
    page.method_name = "CUSUM Chart";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    目标 = " + format_number(options.target)
        + "    k = " + format_number(options.k)
        + "    h = " + format_number(options.h);
    page.diagnostics = chart.diagnostics;
    const std::size_t signal_count = chart.upper_signal_points.size()
        + chart.lower_signal_points.size();
    StatisticTable parameters;
    parameters.title = "CUSUM 参数";
    parameters.headers = {"指标", "数值"};
    parameters.rows = {
        {"N", std::to_string(extracted.values.size())},
        {"N*", std::to_string(extracted.missing_count)},
        {"目标 T", format_number(options.target)},
        {"σ", format_number(options.sigma)},
        {"k", format_number(options.k)},
        {"h", format_number(options.h)},
        {"FIR", options.fast_initial_response ? "是" : "否"},
        {"上侧首次信号点", chart.upper_signal_points.empty()
            ? "无" : std::to_string(chart.upper_signal_points.front() + 1)},
        {"下侧首次信号点", chart.lower_signal_points.empty()
            ? "无" : std::to_string(chart.lower_signal_points.front() + 1)},
        {"信号总数", std::to_string(signal_count)},
        {"规则策略", "CUSUM 专用信号"},
        {"启用测试", "无（不套用 Tests 1–8）"},
        {"判定口径", "上侧/下侧累计和超过决策间隔 hσ 记为信号，原因待调查。"}};
    page.tables.push_back(parameters);
    page.tables.push_back(cusum_point_table(chart, extracted.values, extracted.source_rows));
    page.tables.push_back(cusum_signal_table(chart, extracted.source_rows));
    page.plots.push_back(control_plot("上侧 CUSUM", "CUSUM+", chart.primary, extracted.source_rows));
    page.plots.push_back(control_plot("下侧 CUSUM", "CUSUM-", chart.secondary, extracted.source_rows));
    std::set<std::size_t> unique_signals;
    unique_signals.insert(chart.upper_signal_points.cbegin(), chart.upper_signal_points.cend());
    unique_signals.insert(chart.lower_signal_points.cbegin(), chart.lower_signal_points.cend());
    page.facts.spc = domain::SpcFacts{};
    page.facts.spc->out_of_control_count = unique_signals.size();
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::zone_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto extracted = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    datalab::domain::statistics::ZoneChartOptions options;
    options.moving_range_length = configuration.control.moving_range_length;
    options.historical_mean = configuration.control.historical_center;
    options.historical_sigma = configuration.control.historical_sigma;
    const auto result = datalab::domain::statistics::ControlCharts::zone_chart(
        extracted.values, options);
    OutputPage page;
    page.id = new_id("zone_chart");
    page.title = "区域图";
    page.method_name = "Zone Chart";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    const bool historical = configuration.control.historical_center.has_value()
        || configuration.control.historical_sigma.has_value();
    const double threshold = options.signal_threshold > 0.0 ? options.signal_threshold : 8.0;
    page.parameter_summary = "变量 = " + extracted.name
        + "    CL = " + format_number(result.center)
        + "    σ = " + format_number(result.sigma)
        + "    得分阈值 = " + format_number(threshold)
        + (historical ? "（历史参数）" : "（估计）");
    StatisticTable parameters;
    parameters.title = "区域图参数";
    parameters.headers = {"指标", "数值"};
    parameters.rows = {
        {"N", std::to_string(extracted.values.size())},
        {"N*", std::to_string(extracted.missing_count)},
        {"CL", format_number(result.center)},
        {"σ (within)", format_number(result.sigma)},
        {"移动极差长度", std::to_string(options.moving_range_length)},
        {"Jaehn 累计阈值", format_number(threshold)},
        {"Jaehn 信号点数", std::to_string(result.signal_points.size())},
        {"参数来源", historical ? "历史参数" : "估计"},
        {"计分规则", "Jaehn 1/2/4 权重；不是完整 Western Electric Tests 1–8。"}};
    page.tables.push_back(parameters);
    page.tables.push_back(zone_point_table(result, extracted.source_rows));
    page.plots.push_back(control_plot(
        "区域图（个体值）", "测量值", result.individuals, extracted.source_rows));
    PlotSpec score_plot;
    score_plot.kind = PlotKind::control;
    score_plot.title = "区域累计得分";
    score_plot.x_axis_title = "观测序号";
    score_plot.y_axis_title = "累计得分";
    score_plot.center_label = "阈值";
    score_plot.lower_style.visible = false;
    score_plot.upper_style.visible = false;
    for (std::size_t index = 0; index < result.zone_scores.size(); ++index) {
        score_plot.x_values.push_back(static_cast<double>(index + 1));
        score_plot.values.push_back(result.zone_scores[index]);
        score_plot.center.push_back(threshold);
    }
    score_plot.source_rows = extracted.source_rows;
    page.plots.push_back(std::move(score_plot));
    domain::ZoneChartFacts facts;
    facts.n = extracted.values.size();
    facts.missing_count = extracted.missing_count;
    facts.center = result.center;
    facts.sigma = result.sigma;
    facts.signal_threshold = threshold;
    facts.signal_count = result.signal_points.size();
    page.facts.zone_chart = std::move(facts);
    page.facts.spc = domain::SpcFacts{};
    page.facts.spc->out_of_control_count = result.signal_points.size();
    page.facts.spc->sigma_within = result.sigma > 0.0
        ? std::optional<double>{result.sigma} : std::nullopt;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::z_mr(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto extracted = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    datalab::domain::statistics::ZmrOptions options;
    options.moving_range_length = configuration.control.moving_range_length;
    if (configuration.by_column.has_value()) {
        const std::vector<std::string> labels = extract_text_column(
            table, *configuration.by_column);
        options.group_labels.reserve(extracted.source_rows.size());
        for (const std::size_t row : extracted.source_rows) {
            if (row >= labels.size() || is_missing_cell(labels[row])) {
                return error_page("Z-MR 控制图", "Z-MR Chart",
                                  "分组列存在缺失标签，原始行 "
                                      + std::to_string(row + 1));
            }
            options.group_labels.push_back(labels[row]);
        }
    }
    const auto result = datalab::domain::statistics::ControlCharts::z_mr_chart(
        extracted.values, options);
    OutputPage page;
    page.id = new_id("z_mr");
    page.title = "Z-MR 控制图";
    page.method_name = "Z-MR Chart";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    const std::size_t z_ooc = result.z_chart.test1_points.size();
    const std::string group_summary = configuration.by_column.has_value()
        ? "    分组 = " + column_label(table, *configuration.by_column)
        : "    分组 = （单过程）";
    page.parameter_summary = "变量 = " + extracted.name + group_summary
        + "    MR̄(Z) = " + format_number(result.average_mr)
        + "    Z 超限 = " + std::to_string(z_ooc);
    StatisticTable parameters;
    parameters.title = "Z-MR 参数";
    parameters.headers = {"指标", "数值"};
    parameters.rows = {
        {"N", std::to_string(extracted.values.size())},
        {"N*", std::to_string(extracted.missing_count)},
        {"移动极差长度", std::to_string(options.moving_range_length)},
        {"MR̄(Z)", format_number(result.average_mr)},
        {"Z 图「单点超出 3σ 控制限」触发", std::to_string(z_ooc)},
        {"参数来源",
         std::any_of(result.diagnostics.cbegin(), result.diagnostics.cend(),
                     [](const DiagnosticMessage& message) {
                         return message.code == "zmr_sample_parameters";
                     })
             ? "样本估计（未提供完整历史 μ/σ）"
             : "历史 μ/σ"},
        {"适用测试", "Z 图 Tests 1–4；MR 图无 Shewhart 测试。"}};
    page.tables.push_back(parameters);
    page.tables.push_back(zmr_point_table(
        result, extracted.source_rows, options.group_labels));
    page.plots.push_back(control_plot("Z 图", "Z", result.z_chart, extracted.source_rows));
    page.plots.push_back(control_plot("MR(Z) 图", "MR(Z)", result.mr_chart, extracted.source_rows));
    domain::ZmrFacts facts;
    facts.n = extracted.values.size();
    facts.missing_count = extracted.missing_count;
    facts.group_count = options.group_labels.empty()
        ? 1
        : std::set<std::string>(
              options.group_labels.cbegin(), options.group_labels.cend()).size();
    facts.used_sample_parameters = std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const DiagnosticMessage& message) {
            return message.code == "zmr_sample_parameters";
        });
    facts.average_mr = result.average_mr;
    facts.z_out_of_control_count = z_ooc;
    page.facts.z_mr = std::move(facts);
    page.facts.spc = domain::SpcFacts{};
    page.facts.spc->out_of_control_count = z_ooc;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::moving_average(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto extracted = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    datalab::domain::statistics::MovingAverageOptions options;
    options.window = std::max(2, configuration.control.ma_window);
    options.limit_sigma = configuration.control.ewma_limit_sigma;
    options.moving_range_length = configuration.control.moving_range_length;
    options.historical_mean = configuration.control.historical_center;
    options.historical_sigma = configuration.control.historical_sigma;
    const auto chart = datalab::domain::statistics::ControlCharts::moving_average_chart(
        extracted.values, options);
    OutputPage page;
    page.id = new_id("moving_average");
    page.title = "移动平均控制图";
    page.method_name = "Moving Average Chart";
    page.configuration = configuration;
    page.diagnostics = chart.diagnostics;
    const bool historical = configuration.control.historical_center.has_value()
        || configuration.control.historical_sigma.has_value();
    const double center = options.historical_mean.value_or(
        extracted.values.empty() ? 0.0
            : std::accumulate(extracted.values.cbegin(), extracted.values.cend(), 0.0)
                / static_cast<double>(extracted.values.size()));
    page.parameter_summary = "变量 = " + extracted.name
        + "    窗宽 w = " + std::to_string(options.window)
        + "    控制限倍数 = " + format_number(options.limit_sigma)
        + (historical ? "（历史参数）" : "（估计）");
    StatisticTable parameters;
    parameters.title = "移动平均参数";
    parameters.headers = {"指标", "数值"};
    parameters.rows = {
        {"N", std::to_string(extracted.values.size())},
        {"N*", std::to_string(extracted.missing_count)},
        {"窗宽 w", std::to_string(options.window)},
        {"控制限倍数", format_number(options.limit_sigma)},
        {"CL", format_number(center)},
        {"移动极差长度", std::to_string(options.moving_range_length)},
        {"「单点超出 3σ 控制限」触发点数", std::to_string(chart.test1_points.size())},
        {"参数来源", historical ? "历史参数" : "估计"},
        {"适用性", "MA 图默认「单点超出 3σ 控制限」；与 EWMA 独立，仅完整窗点参与。"}};
    page.tables.push_back(parameters);
    page.tables.push_back(moving_average_point_table(
        chart, extracted.values, extracted.source_rows));
    page.plots.push_back(control_plot("移动平均控制图", "MA", chart, extracted.source_rows));
    page.facts.moving_average = domain::MovingAverageChartFacts{};
    page.facts.moving_average->n = extracted.values.size();
    page.facts.moving_average->missing_count = extracted.missing_count;
    page.facts.moving_average->window = options.window;
    page.facts.moving_average->limit_sigma = options.limit_sigma;
    page.facts.moving_average->center = center;
    page.facts.moving_average->out_of_control_count = chart.test1_points.size();
    if (!chart.point_sigma.empty() && options.window > 0) {
        page.facts.moving_average->sigma_within =
            chart.point_sigma.front() * std::sqrt(static_cast<double>(options.window));
    }
    page.facts.spc = domain::SpcFacts{};
    page.facts.spc->out_of_control_count = chart.test1_points.size();
    return finalize_page(std::move(page));
}

namespace {

OutputPage rare_event_chart_page(
    const DataTable& table,
    const AnalysisConfiguration& configuration,
    const std::string& id_prefix,
    const std::string& title,
    const std::string& method_name,
    const std::string& plot_title,
    bool geometric)
{
    const auto extracted = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    const auto tests =
        datalab::domain::statistics::special_cause_selection_from_configuration(
            configuration.control.enabled_special_cause_tests,
            configuration.control.special_cause_rule_policy);
    std::vector<RowId> source_ids;
    source_ids.reserve(extracted.source_rows.size());
    for (const std::size_t row : extracted.source_rows) {
        source_ids.push_back(static_cast<RowId>(row));
    }
    const auto chart = geometric
        ? datalab::domain::statistics::ControlCharts::g_chart(
            extracted.values, source_ids, tests)
        : datalab::domain::statistics::ControlCharts::t_chart(
            extracted.values, source_ids, tests);
    OutputPage page;
    page.id = new_id(id_prefix);
    page.title = title;
    page.method_name = method_name;
    page.configuration = configuration;
    page.diagnostics = chart.diagnostics;
    page.parameter_summary = "变量 = " + extracted.name
        + "    N = " + std::to_string(extracted.values.size())
        + "    N* = " + std::to_string(extracted.missing_count);
    StatisticTable parameters;
    parameters.title = title + " 参数";
    parameters.headers = {"指标", "数值"};
    const std::string enabled_tests = datalab::domain::statistics::format_special_cause_tests(
        datalab::domain::statistics::resolve_special_cause_tests(
            tests, geometric
                ? datalab::domain::statistics::ControlChartKind::g
                : datalab::domain::statistics::ControlChartKind::t));
    parameters.rows = {
        {"N", std::to_string(extracted.values.size())},
        {"N*", std::to_string(extracted.missing_count)},
        {"「单点超出 3σ 控制限」触发点数", std::to_string(chart.test1_points.size())},
        {"启用测试", enabled_tests}};
    page.tables.push_back(parameters);
    page.tables.push_back(rare_event_point_table(title + " 逐点统计", chart, extracted.source_rows));
    page.plots.push_back(control_plot(plot_title, "间隔", chart, extracted.source_rows));
    page.facts.spc = domain::SpcFacts{};
    page.facts.spc->out_of_control_count = chart.test1_points.size();
    return finalize_page(std::move(page));
}

}  // namespace

OutputPage AnalysisService::g_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    return rare_event_chart_page(
        table, configuration, "gchart", "G 图", "G Chart", "G 图（几何间隔）", true);
}

OutputPage AnalysisService::t_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    return rare_event_chart_page(
        table, configuration, "tchart", "T 图", "T Chart", "T 图（时间间隔）", false);
}

OutputPage AnalysisService::time_series_smoothing(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto extracted = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    if (extracted.values.empty()) {
        return error_page("时间序列平滑", "Time Series Smoothing", "时间序列没有有效数值观测。");
    }
    const bool double_method = configuration.time_series.smoothing_method != "single";
    const auto result = double_method
        ? datalab::domain::statistics::double_exponential_smoothing(
            extracted.values, configuration.time_series.smoothing_alpha, configuration.time_series.smoothing_gamma,
            static_cast<std::size_t>(std::max(1, configuration.time_series.forecast_periods)))
        : datalab::domain::statistics::single_exponential_smoothing(
            extracted.values, configuration.time_series.smoothing_alpha,
            static_cast<std::size_t>(std::max(1, configuration.time_series.forecast_periods)));
    OutputPage page;
    page.id = new_id("time_series");
    page.title = "时间序列平滑";
    page.method_name = double_method ? "Double Exponential Smoothing" : "Single Exponential Smoothing";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    方法 = " + (double_method ? "double" : "single")
        + "    alpha = " + format_number(configuration.time_series.smoothing_alpha)
        + "    gamma = " + format_number(configuration.time_series.smoothing_gamma)
        + "    预测期数 = " + std::to_string(std::max(1, configuration.time_series.forecast_periods))
        + "    有效观测 = " + std::to_string(extracted.values.size());
    page.diagnostics = result.diagnostics;
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "missing_values",
            "平滑跳过缺失或无效的数值单元格。"});
    }
    StatisticTable output;
    output.title = "拟合与预测明细";
    output.headers = {"序号", "原始行", "实际值", "拟合值", "残差", "Forecast", "Lower", "Upper"};
    for (std::size_t index = 0; index < extracted.values.size(); ++index) {
        const double fitted =
            index < result.fitted.size() ? result.fitted[index] : std::numeric_limits<double>::quiet_NaN();
        output.rows.push_back({
            std::to_string(index + 1),
            index < extracted.source_rows.size() ? std::to_string(extracted.source_rows[index] + 1) : "*",
            format_number(extracted.values[index]),
            index < result.fitted.size() ? format_number(result.fitted[index]) : "*",
            std::isfinite(fitted) ? format_number(extracted.values[index] - fitted) : "*",
            "",
            "",
            ""});
    }
    for (std::size_t index = 0; index < result.forecasts.size(); ++index) {
        output.rows.push_back({
            std::to_string(extracted.values.size() + index + 1),
            "",
            "",
            "",
            "",
            format_number(result.forecasts[index]),
            index < result.lower.size() ? format_number(result.lower[index]) : "*",
            index < result.upper.size() ? format_number(result.upper[index]) : "*"});
    }
    page.tables.push_back(output);
    StatisticTable metrics;
    metrics.title = "预测准确度";
    metrics.headers = {"N", "MAD", "MSD", "MAPE (%)"};
    metrics.rows.push_back({
        std::to_string(extracted.values.size()),
        format_number(result.mad),
        format_number(result.msd),
        format_number(result.mape)});
    page.tables.push_back(metrics);
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "实际值、拟合值与预测区间";
    plot.x_axis_title = "观测序号";
    plot.y_axis_title = extracted.name;
    datalab::domain::PlotSeries actual;
    actual.role = datalab::domain::PlotSeriesRole::actual;
    actual.label = "实际值";
    actual.values = extracted.values;
    actual.show_points = true;
    actual.x_values.resize(extracted.values.size());
    for (std::size_t index = 0; index < actual.x_values.size(); ++index) {
        actual.x_values[index] = static_cast<double>(index + 1);
    }
    datalab::domain::PlotSeries fitted;
    fitted.role = datalab::domain::PlotSeriesRole::fitted;
    fitted.label = "拟合值";
    fitted.values = result.fitted;
    fitted.x_values.resize(result.fitted.size());
    for (std::size_t index = 0; index < fitted.x_values.size(); ++index) {
        fitted.x_values[index] = static_cast<double>(index + 1);
    }
    datalab::domain::PlotSeries forecast;
    forecast.role = datalab::domain::PlotSeriesRole::forecast;
    forecast.label = "预测值";
    forecast.values = result.forecasts;
    forecast.x_values.resize(result.forecasts.size());
    for (std::size_t index = 0; index < forecast.x_values.size(); ++index) {
        forecast.x_values[index] = static_cast<double>(extracted.values.size() + index + 1);
    }
    datalab::domain::PlotSeries confidence;
    confidence.role = datalab::domain::PlotSeriesRole::confidence_band;
    confidence.label = "预测区间";
    confidence.lower = result.lower;
    confidence.upper = result.upper;
    confidence.x_values = forecast.x_values;
    plot.series = {std::move(actual), std::move(fitted), std::move(forecast),
                   std::move(confidence)};
    plot.source_rows = extracted.source_rows;
    page.plots.push_back(std::move(plot));
    page.facts.forecast = domain::ForecastFacts{result.mape, std::nullopt, std::nullopt, std::nullopt};
    page.method_metadata.estimation_method =
        double_method ? "holt_linear_des" : "single_exponential_ses";
    page.method_metadata.parameter_source = "estimated";
    page.method_metadata.valid_count = extracted.values.size();
    page.method_metadata.missing_count = extracted.missing_count;
    page.method_metadata.source_rows.clear();
    page.method_metadata.source_rows.reserve(extracted.source_rows.size());
    for (const std::size_t row : extracted.source_rows) {
        page.method_metadata.source_rows.push_back(static_cast<domain::RowId>(row));
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::arima(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.time_series.arima_value_column.has_value()) {
        return error_page("ARIMA", "ARIMA", "请选择时间序列数值列。");
    }
    const auto values = extract_numeric_column(
        table, *configuration.time_series.arima_value_column, configuration.excluded_rows);
    if (values.values.empty()) {
        return error_page("ARIMA", "ARIMA", "时间序列没有有效数值观测。");
    }
    if (configuration.time_series.arima_time_column.has_value()) {
        const auto time = extract_numeric_column(
            table, *configuration.time_series.arima_time_column, configuration.excluded_rows);
        if (time.values.size() != values.values.size()) {
            return error_page("ARIMA", "ARIMA", "时间列与数值列的有效行不一致。");
        }
        for (std::size_t index = 1; index < time.values.size(); ++index) {
            if (!(time.values[index] > time.values[index - 1])) {
                return error_page("ARIMA", "ARIMA",
                                  "时间列必须严格递增，且不能包含重复时间点。");
            }
        }
    }
    const std::size_t forecast_periods = configuration.time_series.forecast_periods > 0
        ? static_cast<std::size_t>(configuration.time_series.forecast_periods) : 1;
    const auto candidates = datalab::domain::statistics::fit_arima_candidates(
        values.values, forecast_periods, 3, 3,
        std::clamp(configuration.time_series.arima_differencing, 0, 2));
    if (candidates.empty()) {
        return error_page("ARIMA", "ARIMA", "没有可拟合的候选模型。");
    }
    auto criterion_value = [&](const datalab::domain::statistics::ArimaResult& result) {
        if (configuration.time_series.arima_selection_criterion == "aic") {
            return result.aic;
        }
        if (configuration.time_series.arima_selection_criterion == "bic") {
            return result.bic;
        }
        return result.aicc;
    };
    const auto best = std::min_element(
        candidates.cbegin(), candidates.cend(),
        [&](const auto& first, const auto& second) {
            return criterion_value(first) < criterion_value(second);
        });
    auto model_name = [](const datalab::domain::statistics::ArimaResult& result) {
        return datalab::domain::statistics::arima_order_label(result.order);
    };
    OutputPage page;
    page.id = new_id("arima");
    page.title = "ARIMA 基础预测";
    page.method_name = "ARIMA";
    page.configuration = configuration;
    page.parameter_summary = "响应列 = "
        + column_label(table, *configuration.time_series.arima_value_column)
        + "    选择准则 = " + configuration.time_series.arima_selection_criterion;
    for (const auto& candidate : candidates) {
        page.diagnostics.insert(page.diagnostics.end(),
                                candidate.diagnostics.cbegin(),
                                candidate.diagnostics.cend());
    }
    StatisticTable comparison;
    comparison.title = "候选模型比较";
    comparison.headers = {"模型", "SSE", "AIC", "AICc", "BIC"};
    for (const auto& candidate : candidates) {
        comparison.rows.push_back({
            model_name(candidate), format_number(candidate.sse),
            format_number(candidate.aic), format_number(candidate.aicc),
            format_number(candidate.bic)});
    }
    page.tables.push_back(comparison);
    StatisticTable forecast;
    forecast.title = "模型摘要与预测";
    forecast.headers = {"最优模型", "截距", "系数/漂移", "预测期", "Forecast", "Lower", "Upper"};
    for (std::size_t index = 0; index < best->forecasts.size(); ++index) {
        forecast.rows.push_back({
            index == 0 ? model_name(*best) : "",
            index == 0 ? format_number(best->intercept) : "",
            index == 0 ? format_number(best->coefficient + best->drift) : "",
            std::to_string(index + 1),
            format_number(best->forecasts[index]),
            index < best->lower.size() ? format_number(best->lower[index]) : "*",
            index < best->upper.size() ? format_number(best->upper[index]) : "*"});
    }
    page.tables.push_back(forecast);
    StatisticTable detail;
    detail.title = "拟合与预测明细";
    detail.headers = {"序号", "原始行", "Observed", "Fitted", "Residual",
                      "Forecast", "Lower", "Upper"};
    for (std::size_t index = 0; index < values.values.size(); ++index) {
        const double fitted = index < best->fitted.size()
            ? best->fitted[index] : std::numeric_limits<double>::quiet_NaN();
        const double residual = index < best->residuals.size()
            ? best->residuals[index]
            : (std::isfinite(fitted) ? values.values[index] - fitted
               : std::numeric_limits<double>::quiet_NaN());
        detail.rows.push_back({
            std::to_string(index + 1),
            index < values.source_rows.size()
                ? std::to_string(values.source_rows[index] + 1) : "*",
            format_number(values.values[index]),
            index < best->fitted.size() ? format_number(best->fitted[index]) : "*",
            std::isfinite(residual) ? format_number(residual) : "*",
            "",
            "",
            ""});
    }
    for (std::size_t index = 0; index < best->forecasts.size(); ++index) {
        detail.rows.push_back({
            std::to_string(values.values.size() + index + 1),
            "",
            "",
            "",
            "",
            format_number(best->forecasts[index]),
            index < best->lower.size() ? format_number(best->lower[index]) : "*",
            index < best->upper.size() ? format_number(best->upper[index]) : "*"});
    }
    page.tables.push_back(detail);
    double absolute_percent = 0.0;
    double absolute_error = 0.0;
    double naive_error = 0.0;
    std::size_t mape_count = 0;
    for (std::size_t index = 0; index < values.values.size()
         && index < best->fitted.size(); ++index) {
        const double residual = values.values[index] - best->fitted[index];
        if (std::abs(values.values[index]) > 1.0e-12) {
            absolute_percent += std::abs(residual / values.values[index]);
            ++mape_count;
        }
        absolute_error += std::abs(residual);
        if (index > 0) {
            naive_error += std::abs(values.values[index] - values.values[index - 1]);
        }
    }
    domain::ForecastFacts forecast_facts;
    if (mape_count > 0) {
        forecast_facts.mape = 100.0 * absolute_percent / static_cast<double>(mape_count);
    }
    if (values.values.size() > 1 && naive_error > 0.0) {
        forecast_facts.mase = (absolute_error / static_cast<double>(values.values.size()))
            / (naive_error / static_cast<double>(values.values.size() - 1));
    }
    page.facts.forecast = forecast_facts;
    page.method_metadata.estimation_method = "arima_candidate_css";
    page.method_metadata.parameter_source = "estimated";
    page.method_metadata.valid_count = values.values.size();
    page.method_metadata.missing_count = values.missing_count;
    page.method_metadata.source_rows.clear();
    page.method_metadata.source_rows.reserve(values.source_rows.size());
    for (const std::size_t row : values.source_rows) {
        page.method_metadata.source_rows.push_back(static_cast<domain::RowId>(row));
    }
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "ARIMA 拟合与预测";
    plot.x_axis_title = "Period";
    plot.y_axis_title = column_label(table, *configuration.time_series.arima_value_column);
    plot.x_values.reserve(best->fitted.size() + best->forecasts.size());
    plot.values.reserve(best->fitted.size() + best->forecasts.size());
    for (std::size_t index = 0; index < best->fitted.size(); ++index) {
        plot.x_values.push_back(static_cast<double>(index + 1));
        plot.values.push_back(best->fitted[index]);
    }
    for (std::size_t index = 0; index < best->forecasts.size(); ++index) {
        plot.x_values.push_back(static_cast<double>(best->fitted.size() + index + 1));
        plot.values.push_back(best->forecasts[index]);
    }
    datalab::domain::PlotSeries actual;
    actual.role = datalab::domain::PlotSeriesRole::actual;
    actual.label = "实际值";
    actual.values = values.values;
    actual.show_points = true;
    actual.x_values.resize(values.values.size());
    for (std::size_t index = 0; index < actual.x_values.size(); ++index) {
        actual.x_values[index] = static_cast<double>(index + 1);
    }
    datalab::domain::PlotSeries fitted_series;
    fitted_series.role = datalab::domain::PlotSeriesRole::fitted;
    fitted_series.label = "拟合值";
    fitted_series.values = best->fitted;
    fitted_series.x_values.resize(best->fitted.size());
    for (std::size_t index = 0; index < fitted_series.x_values.size(); ++index) {
        fitted_series.x_values[index] = static_cast<double>(index + 1);
    }
    datalab::domain::PlotSeries forecast_series;
    forecast_series.role = datalab::domain::PlotSeriesRole::forecast;
    forecast_series.label = "预测值";
    forecast_series.values = best->forecasts;
    forecast_series.lower = best->lower;
    forecast_series.upper = best->upper;
    forecast_series.x_values.resize(best->forecasts.size());
    for (std::size_t index = 0; index < forecast_series.x_values.size(); ++index) {
        forecast_series.x_values[index] =
            static_cast<double>(best->fitted.size() + index + 1);
    }
    datalab::domain::PlotSeries confidence;
    confidence.role = datalab::domain::PlotSeriesRole::confidence_band;
    confidence.label = "预测区间";
    confidence.lower = best->lower;
    confidence.upper = best->upper;
    confidence.x_values = forecast_series.x_values;
    plot.series = {std::move(actual), std::move(fitted_series),
                   std::move(forecast_series), std::move(confidence)};
    plot.source_rows = values.source_rows;
    page.plots.push_back(plot);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::two_factor_anova(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.inference.anova_response_column.has_value()
        || !configuration.inference.anova_factor_a_column.has_value()
        || !configuration.inference.anova_factor_b_column.has_value()) {
        return error_page("双因素 ANOVA", "Two-Factor ANOVA",
                          "请选择响应变量、因子 A 和因子 B。");
    }
    datalab::domain::statistics::TwoFactorAnovaInput input;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row_index)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        const auto& row = table.rows[row_index];
        const std::size_t response_column = *configuration.inference.anova_response_column;
        const std::size_t factor_a_column = *configuration.inference.anova_factor_a_column;
        const std::size_t factor_b_column = *configuration.inference.anova_factor_b_column;
        if (response_column >= row.size() || factor_a_column >= row.size()
            || factor_b_column >= row.size()) {
            continue;
        }
        const auto response = parse_numeric_cell(row[response_column]);
        if (!response.has_value() || is_missing_cell(row[factor_a_column])
            || is_missing_cell(row[factor_b_column])) {
            continue;
        }
        input.response.push_back(*response);
        input.factor_a.push_back(row[factor_a_column]);
        input.factor_b.push_back(row[factor_b_column]);
        input.source_rows.push_back(row_index);
    }
    input.encoding = configuration.inference.anova_factor_encoding == "effect"
        ? datalab::domain::statistics::AnovaFactorEncoding::effect
        : datalab::domain::statistics::AnovaFactorEncoding::reference;
    const auto result = datalab::domain::statistics::two_factor_anova(input);
    OutputPage page;
    page.id = new_id("two_factor_anova");
    page.title = "双因素 ANOVA";
    page.method_name = "Two-Factor ANOVA";
    page.configuration = configuration;
    page.parameter_summary = "响应 = "
        + column_label(table, *configuration.inference.anova_response_column)
        + "    因子 A = " + column_label(table, *configuration.inference.anova_factor_a_column)
        + "    因子 B = " + column_label(table, *configuration.inference.anova_factor_b_column);
    page.diagnostics = result.diagnostics;
    StatisticTable effects;
    effects.title = "ANOVA 表";
    effects.headers = {"来源", "Seq SS", "Adj SS", "DF", "MS", "F", "P-Value", "可估计性"};
    for (const auto& effect : result.effects) {
        effects.rows.push_back({
            effect.term, format_optional(effect.sequential_sum_of_squares),
            format_optional(effect.adjusted_sum_of_squares),
            std::to_string(effect.degrees_of_freedom), format_optional(effect.mean_square),
            format_optional(effect.f_statistic),
            effect.p_value.has_value() ? format_number(*effect.p_value) : "*",
            effect.estimability});
    }
    effects.rows.push_back({"Error", "", "", std::to_string(result.error_degrees_of_freedom),
                            format_number(result.error_mean_square),
                            "", "", ""});
    page.tables.push_back(effects);
    StatisticTable means_a;
    means_a.title = "因子 A 均值";
    means_a.headers = {"水平", "N", "均值"};
    for (const auto& mean : result.factor_a_means) {
        means_a.rows.push_back({mean.level, std::to_string(mean.count), format_number(mean.mean)});
    }
    page.tables.push_back(means_a);
    StatisticTable means_b;
    means_b.title = "因子 B 均值";
    means_b.headers = {"水平", "N", "均值"};
    for (const auto& mean : result.factor_b_means) {
        means_b.rows.push_back({mean.level, std::to_string(mean.count), format_number(mean.mean)});
    }
    page.tables.push_back(means_b);
    StatisticTable interaction;
    interaction.title = "交互均值";
    interaction.headers = {"因子 A", "因子 B", "N", "均值"};
    for (const auto& mean : result.interaction_means) {
        interaction.rows.push_back({mean.factor_a_level, mean.factor_b_level,
                                    std::to_string(mean.count), format_number(mean.mean)});
    }
    page.tables.push_back(interaction);
    if (!result.interaction_means.empty() && !result.factor_a_means.empty()) {
        PlotSpec interaction_plot;
        interaction_plot.kind = PlotKind::scatter;
        interaction_plot.title = "交互均值图";
        interaction_plot.x_axis_title = "因子 A";
        interaction_plot.y_axis_title = "均值";
        interaction_plot.show_legend = true;
        for (const auto& mean_b : result.factor_b_means) {
            PlotSeries series;
            series.label = mean_b.level;
            series.show_points = true;
            series.style.point_style = PlotPointStyle::circle;
            series.style.line_style = PlotLineStyle::solid;
            for (std::size_t a_index = 0; a_index < result.factor_a_means.size(); ++a_index) {
                const std::string& level_a = result.factor_a_means[a_index].level;
                for (const auto& cell : result.interaction_means) {
                    if (cell.factor_a_level != level_a || cell.factor_b_level != mean_b.level) {
                        continue;
                    }
                    series.x_values.push_back(static_cast<double>(a_index + 1));
                    series.values.push_back(cell.mean);
                    for (std::size_t row = 0; row < input.factor_a.size(); ++row) {
                        if (input.factor_a[row] == level_a && input.factor_b[row] == mean_b.level) {
                            interaction_plot.source_rows.push_back(input.source_rows[row]);
                            break;
                        }
                    }
                    break;
                }
            }
            if (!series.values.empty()) {
                interaction_plot.series.push_back(std::move(series));
            }
        }
        page.plots.push_back(std::move(interaction_plot));
    }
    if (!result.residuals.empty()) {
        PlotSpec residual_plot;
        residual_plot.kind = PlotKind::scatter;
        residual_plot.title = "残差与拟合值";
        residual_plot.x_axis_title = "拟合值";
        residual_plot.y_axis_title = "残差";
        residual_plot.x_values = result.fitted;
        residual_plot.values = result.residuals;
        residual_plot.source_rows = result.observation_source_rows;
        add_zero_residual_reference(residual_plot);
        page.plots.push_back(std::move(residual_plot));

        PlotSpec order_plot;
        order_plot.kind = PlotKind::scatter;
        order_plot.title = "残差与观测顺序";
        order_plot.x_axis_title = "观测顺序";
        order_plot.y_axis_title = "残差";
        for (std::size_t index = 0; index < result.residuals.size(); ++index) {
            order_plot.x_values.push_back(static_cast<double>(index + 1));
            order_plot.values.push_back(result.residuals[index]);
        }
        order_plot.source_rows = result.observation_source_rows;
        add_zero_residual_reference(order_plot);
        page.plots.push_back(std::move(order_plot));

        PlotSpec residual_probability;
        residual_probability.kind = PlotKind::probability;
        residual_probability.title = "残差正态概率图";
        residual_probability.x_axis_title = "理论分位数";
        residual_probability.y_axis_title = "残差";
        const auto probability = datalab::domain::statistics::normal_probability_plot(
            result.residuals, result.observation_source_rows);
        residual_probability.x_values = probability.theoretical_quantiles;
        residual_probability.values = probability.ordered_values;
        residual_probability.source_rows = probability.source_rows;
        page.plots.push_back(std::move(residual_probability));

        const datalab::domain::statistics::HistogramResult bins =
            datalab::domain::statistics::histogram(result.residuals, 0);
        PlotSpec hist;
        hist.kind = PlotKind::histogram;
        hist.title = "残差直方图";
        hist.x_axis_title = "残差";
        hist.y_axis_title = "频数";
        hist.histogram_edges = bins.edges;
        hist.histogram_counts = bins.counts;
        hist.values = result.residuals;
        hist.source_rows = result.observation_source_rows;
        page.plots.push_back(std::move(hist));
    }
    page.facts.anova = datalab::domain::statistics::two_factor_anova_facts_from(result);
    append_rule_table(page, page.facts.anova->rules);
    domain::apply_evidence(page.method_metadata, result.evidence);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::logistic_regression(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.inference.logistic_response_column.has_value()
        || configuration.inference.logistic_predictor_columns.empty()) {
        return error_page("二元 Logistic 回归", "Binary Logistic Regression",
                          "请选择二元响应列和至少一个预测变量。");
    }
    const LogisticImport imported = logistic_import_rows(table, configuration);
    const std::vector<int>& response = imported.response;
    const std::vector<std::vector<double>>& predictors = imported.predictors;
    const std::vector<std::size_t>& source_rows = imported.source_rows;
    std::vector<std::string> labels;
    for (const std::size_t column : configuration.inference.logistic_predictor_columns) {
        labels.push_back(column_label(table, column));
    }
    const auto result = datalab::domain::statistics::fit_logistic_regression(
        response, predictors, labels, configuration.inference.confidence_level,
        static_cast<std::size_t>(std::max(1, configuration.inference.logistic_max_iterations)),
        configuration.inference.logistic_tolerance);
    OutputPage page;
    page.id = new_id("logistic");
    page.title = "二元 Logistic 回归";
    page.method_name = "Binary Logistic Regression";
    page.configuration = configuration;
    page.parameter_summary = "响应 = "
        + column_label(table, *configuration.inference.logistic_response_column)
        + "    事件水平 = " + configuration.inference.logistic_event_level
        + "    预测变量数 = " + std::to_string(labels.size());
    page.diagnostics = result.diagnostics;
    if (source_rows.size() < table.rows.size()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "Logistic 回归使用 complete-case，缺失或非法行已排除。"});
    }
    StatisticTable summary;
    summary.title = "模型摘要";
    summary.headers = {"N", "迭代次数", "收敛", "Log-Likelihood", "Deviance", "AIC", "BIC"};
    summary.rows.push_back({
        std::to_string(result.observation_count),
        std::to_string(result.iteration_count),
        result.converged ? "是" : "否",
        format_number(result.log_likelihood), format_number(result.deviance),
        format_number(result.aic), format_number(result.bic)});
    page.tables.push_back(summary);
    StatisticTable goodness;
    goodness.title = "拟合优度";
    goodness.headers = {"检验", "卡方", "DF", "组数", "P-Value", "状态"};
    goodness.rows.push_back({
        "Hosmer-Lemeshow",
        format_optional(result.hosmer_lemeshow_statistic),
        result.hosmer_lemeshow_df.has_value()
            ? std::to_string(*result.hosmer_lemeshow_df) : "*",
        result.hosmer_lemeshow_groups > 0
            ? std::to_string(result.hosmer_lemeshow_groups) : "*",
        format_optional(result.hosmer_lemeshow_p),
        result.hosmer_lemeshow_status});
    page.tables.push_back(goodness);
    StatisticTable association;
    association.title = "关联统计（配对）";
    association.headers = {"Concordant", "Discordant", "Tied", "Pairs Concordance (%)"};
    association.rows.push_back({
        std::to_string(result.concordant_pairs),
        std::to_string(result.discordant_pairs),
        std::to_string(result.tied_pairs),
        format_optional(result.pairs_concordance_percent)});
    page.tables.push_back(association);
    StatisticTable classification;
    classification.title = "分类表（阈值 0.5）";
    classification.headers = {"", "预测 0", "预测 1"};
    classification.rows.push_back({
        "实际 0",
        std::to_string(result.true_negative),
        std::to_string(result.false_positive)});
    classification.rows.push_back({
        "实际 1",
        std::to_string(result.false_negative),
        std::to_string(result.true_positive)});
    page.tables.push_back(classification);
    StatisticTable coefficients;
    coefficients.title = "系数与 Odds Ratio";
    coefficients.headers = {"项", "Coef", "SE Coef", "Z", "P-Value",
                            "Odds Ratio", "95% CI", "VIF"};
    for (const auto& coefficient : result.coefficients) {
        coefficients.rows.push_back({
            coefficient.term, format_number(coefficient.coefficient),
            format_number(coefficient.standard_error),
            format_number(coefficient.z_statistic),
            format_number(coefficient.p_value),
            format_number(coefficient.odds_ratio),
            "[" + format_number(coefficient.confidence_lower) + ", "
                + format_number(coefficient.confidence_upper) + "]",
            format_optional(coefficient.vif)});
    }
    page.tables.push_back(coefficients);
    StatisticTable fitted;
    fitted.title = "拟合与残差";
    fitted.headers = {"原始行", "响应", "预测概率", "Pearson 残差", "Deviance 残差", "杠杆值",
                      "影响点"};
    std::size_t high_leverage_count = 0;
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        const auto& observation = result.observations[index];
        if (observation.high_leverage) {
            ++high_leverage_count;
        }
        fitted.rows.push_back({
            index < source_rows.size() ? std::to_string(source_rows[index] + 1) : "*",
            std::to_string(observation.response), format_number(observation.probability),
            format_number(observation.pearson_residual),
            format_number(observation.deviance_residual),
            format_number(observation.leverage),
            observation.high_leverage ? "是" : "否"});
    }
    page.tables.push_back(fitted);
    page.facts.logistic = domain::LogisticFacts{
        result.converged,
        result.complete_separation,
        result.hosmer_lemeshow_statistic,
        result.hosmer_lemeshow_p,
        result.hosmer_lemeshow_df,
        result.hosmer_lemeshow_groups,
        result.hosmer_lemeshow_status,
        high_leverage_count,
        result.leverage_threshold,
        result.maximum_leverage,
        result.maximum_vif,
        result.concordant_pairs,
        result.discordant_pairs,
        result.tied_pairs,
        result.pairs_concordance_percent,
        result.true_positive,
        result.true_negative,
        result.false_positive,
        result.false_negative};
    if (configuration.inference.logistic_stepwise_enabled
        && configuration.inference.logistic_predictor_columns.size() >= 2) {
        const auto stepwise = datalab::domain::statistics::fit_logistic_stepwise(
            response, predictors, labels,
            configuration.inference.logistic_stepwise_method.empty()
                ? "forward_aicc" : configuration.inference.logistic_stepwise_method,
            configuration.inference.logistic_stepwise_alpha_enter,
            configuration.inference.logistic_stepwise_alpha_remove,
            configuration.inference.confidence_level,
            static_cast<std::size_t>(std::max(1, configuration.inference.logistic_max_iterations)),
            configuration.inference.logistic_tolerance);
        page.diagnostics.insert(
            page.diagnostics.end(),
            stepwise.diagnostics.begin(),
            stepwise.diagnostics.end());
        if (!stepwise.steps.empty()) {
            StatisticTable stepwise_table;
            stepwise_table.title = "Stepwise Details";
            stepwise_table.headers = {
                "Step", "Action", "Term", "Deviance", "AIC", "AICc", "BIC",
                "Enter P", "Remove P"};
            for (const auto& step : stepwise.steps) {
                stepwise_table.rows.push_back({
                    std::to_string(step.step),
                    step.action,
                    step.term,
                    step.deviance.has_value() ? format_number(*step.deviance) : "—",
                    step.aic.has_value() ? format_number(*step.aic) : "—",
                    step.aicc.has_value() ? format_number(*step.aicc) : "—",
                    step.bic.has_value() ? format_number(*step.bic) : "—",
                    step.enter_p_value.has_value()
                        ? format_number(*step.enter_p_value) : "—",
                    step.remove_p_value.has_value()
                        ? format_number(*step.remove_p_value) : "—"});
            }
            page.tables.push_back(std::move(stepwise_table));
            page.facts.logistic->stepwise_method = stepwise.method;
            page.facts.logistic->stepwise_criterion = stepwise.criterion;
            page.facts.logistic->stepwise_step_count = stepwise.steps.size();
            page.facts.logistic->stepwise_selected_count = stepwise.selected_terms.size();
            page.facts.logistic->stepwise_best_step_index = stepwise.best_step_index;
            page.facts.logistic->stepwise_log_likelihood =
                stepwise.final_model.log_likelihood;
            page.facts.logistic->stepwise_aic = stepwise.final_model.aic;
            page.facts.logistic->stepwise_bic = stepwise.final_model.bic;
            for (const auto& step : stepwise.steps) {
                domain::LogisticStepwiseStepFacts facts;
                facts.step = step.step;
                facts.action = step.action;
                facts.term = step.term;
                facts.deviance = step.deviance;
                facts.aic = step.aic;
                facts.aicc = step.aicc;
                facts.bic = step.bic;
                facts.enter_p = step.enter_p_value;
                facts.remove_p = step.remove_p_value;
                page.facts.logistic->stepwise_steps.push_back(std::move(facts));
            }
        }
    }
    PlotSpec probability;
    probability.kind = PlotKind::scatter;
    probability.title = "预测概率";
    probability.x_axis_title = "观测顺序";
    probability.y_axis_title = "事件概率";
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        probability.x_values.push_back(static_cast<double>(index + 1));
        probability.values.push_back(result.observations[index].probability);
        probability.source_rows.push_back(
            index < source_rows.size() ? source_rows[index] : index);
    }
    page.plots.push_back(probability);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::distribution_identification(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.empty()) {
        return error_page("个体分布识别", "Individual Distribution Identification",
                          "请选择测量值列。");
    }
    const ExtractedNumericColumn extracted = extract_numeric_column(
        table, configuration.variable_columns.front(), configuration.excluded_rows);
    const auto result = datalab::domain::statistics::identify_individual_distributions(
        extracted.values, extracted.source_rows);
    OutputPage page;
    page.id = new_id("distribution_id");
    page.title = "个体分布识别";
    page.method_name = "Individual Distribution Identification";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    N = " + std::to_string(extracted.values.size())
        + "    N* = " + std::to_string(extracted.missing_count);
    page.diagnostics = result.diagnostics;
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            extracted.name + " 跳过 " + std::to_string(extracted.missing_count)
                + " 个缺失或非法单元格。"});
    }

    StatisticTable goodness;
    goodness.title = "拟合优度";
    goodness.headers = {"分布", "AD", "AD*", "P-Value", "判定", "状态"};
    for (const auto& candidate : result.candidates) {
        std::string decision = "*";
        if (candidate.decision == "reject") {
            decision = "在 alpha 下拒绝";
        } else if (candidate.decision == "fail_to_reject") {
            decision = "在 alpha 下未拒绝";
        }
        goodness.rows.push_back({
            candidate.distribution,
            format_optional(candidate.anderson_darling),
            format_optional(candidate.adjusted_anderson_darling),
            format_optional(candidate.p_value),
            decision,
            candidate.status});
    }
    page.tables.push_back(goodness);

    StatisticTable parameters;
    parameters.title = "参数估计";
    parameters.headers = {"分布", "位置/形状", "尺度", "状态"};
    for (const auto& candidate : result.candidates) {
        std::string location_shape = "*";
        if (candidate.distribution == "Weibull") {
            location_shape = candidate.shape.has_value()
                ? format_number(*candidate.shape) : "*";
        } else if (candidate.location.has_value()) {
            location_shape = format_number(*candidate.location);
        }
        parameters.rows.push_back({
            candidate.distribution,
            location_shape,
            candidate.scale.has_value() ? format_number(*candidate.scale) : "*",
            candidate.status});
    }
    page.tables.push_back(parameters);

    for (const auto& candidate : result.candidates) {
        if (candidate.status != "computed"
            || candidate.probability_plot.ordered_values.empty()) {
            continue;
        }
        PlotSpec plot;
        plot.kind = PlotKind::probability;
        plot.title = candidate.distribution + " 概率图";
        plot.x_axis_title = "理论分位数";
        plot.y_axis_title = extracted.name;
        plot.values = candidate.probability_plot.ordered_values;
        plot.x_values = candidate.probability_plot.theoretical_quantiles;
        plot.source_rows = candidate.probability_plot.source_rows;
        plot.line_width = 1.4;
        page.plots.push_back(plot);
    }

    if (!result.candidates.empty()) {
        const auto& best = result.candidates.front();
        page.facts.distribution_identification = domain::DistributionIdentificationFacts{
            best.distribution,
            best.anderson_darling,
            best.p_value,
            true};
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::multi_vari(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.graph.variable_columns.size() < 2
        || configuration.graph.variable_columns.size() > 4) {
        return error_page("Multi-Vari 图", "Multi-Vari Chart",
                          "Multi-Vari 图需要选择 2～4 个因子列。");
    }
    const std::size_t measurement_column = configuration.selection.measurement_column;
    std::set<std::size_t> excluded(
        configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend());
    std::vector<double> measurements;
    std::vector<std::vector<std::string>> factor_levels;
    std::vector<std::size_t> source_rows;
    std::size_t missing_count = 0;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (excluded.count(row_index) != 0) {
            continue;
        }
        const auto& row = table.rows[row_index];
        const std::string measurement_text =
            measurement_column < row.size() ? row[measurement_column] : "";
        const auto numeric = parse_numeric_cell(measurement_text);
        bool complete = numeric.has_value();
        std::vector<std::string> levels;
        levels.reserve(configuration.graph.variable_columns.size());
        for (const std::size_t factor_column : configuration.graph.variable_columns) {
            const std::string text =
                factor_column < row.size() ? row[factor_column] : "";
            if (is_missing_cell(text)) {
                complete = false;
            }
            levels.push_back(text);
        }
        if (!complete) {
            ++missing_count;
            continue;
        }
        measurements.push_back(*numeric);
        factor_levels.push_back(std::move(levels));
        source_rows.push_back(row_index);
    }
    std::vector<std::string> factor_names;
    for (const std::size_t column : configuration.graph.variable_columns) {
        factor_names.push_back(column < table.columns.size()
                                   ? table.columns[column]
                                   : column_label(table, column));
    }
    const auto result = datalab::domain::statistics::multi_vari_chart(
        measurements, factor_levels, source_rows, factor_names);
    OutputPage page;
    page.id = new_id("multi_vari");
    page.title = "Multi-Vari 图";
    page.method_name = "Multi-Vari Chart";
    page.configuration = configuration;
    const std::string measurement_name =
        measurement_column < table.columns.size()
            ? table.columns[measurement_column]
            : column_label(table, measurement_column);
    page.parameter_summary = "测量 = " + measurement_name
        + "    因子 = "
        + [&] {
            std::string text;
            for (std::size_t index = 0; index < factor_names.size(); ++index) {
                if (index > 0) {
                    text += ", ";
                }
                text += factor_names[index];
            }
            return text;
        }()
        + "    有效观测 = " + std::to_string(result.valid_count)
        + "    缺失 = " + std::to_string(missing_count);
    page.diagnostics = result.diagnostics;
    if (missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "测量或因子缺失、* 或非法单元格已跳过 "
                + std::to_string(missing_count) + " 行。"});
    }
    StatisticTable factor_table;
    factor_table.title = "因子均值";
    factor_table.headers = {"因子", "水平", "N", "均值"};
    for (const auto& mean : result.factor_means) {
        factor_table.rows.push_back({
            mean.factor_name, mean.level, std::to_string(mean.count),
            format_number(mean.mean)});
    }
    if (!factor_table.rows.empty()) {
        page.tables.push_back(std::move(factor_table));
    }
    StatisticTable cell_table;
    cell_table.title = "单元均值";
    cell_table.headers = {"组合", "N", "均值"};
    for (const auto& cell : result.cell_means) {
        std::string combination;
        for (std::size_t index = 0; index < cell.levels.size(); ++index) {
            if (index > 0) {
                combination += " / ";
            }
            combination += cell.levels[index];
        }
        cell_table.rows.push_back({
            combination, std::to_string(cell.count), format_number(cell.mean)});
    }
    if (!cell_table.rows.empty()) {
        page.tables.push_back(std::move(cell_table));
    }
    if (result.plot_available && !result.points.empty()) {
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "Multi-Vari 图";
        plot.x_axis_title = factor_names.empty() ? "因子 1" : factor_names.front();
        plot.y_axis_title = measurement_name;
        plot.value_style.visible = false;
        plot.value_style.point_style = PlotPointStyle::circle;
        for (const auto& point : result.points) {
            plot.x_values.push_back(point.x_position);
            plot.values.push_back(point.measurement);
            plot.source_rows.push_back(point.source_row);
        }
        for (const auto& series : result.mean_series) {
            PlotSeries plotted;
            plotted.role = PlotSeriesRole::interaction_first;
            plotted.label = series.label;
            plotted.x_values = series.x_values;
            plotted.values = series.y_values;
            plotted.show_points = true;
            plotted.style.point_style = PlotPointStyle::square;
            plotted.style.line_style = PlotLineStyle::solid;
            plot.series.push_back(std::move(plotted));
        }
        page.plots.push_back(std::move(plot));
    }
    domain::MultiVariFacts facts;
    facts.factor_count = result.factor_count;
    facts.valid_count = result.valid_count;
    facts.missing_count = missing_count;
    facts.combination_coverage = result.combination_coverage;
    facts.factor_names = factor_names;
    page.facts.multi_vari = std::move(facts);
    page.method_metadata.estimation_method = "multi_vari_cell_means";
    page.method_metadata.valid_count = result.valid_count;
    page.method_metadata.missing_count = missing_count;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::variability_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.graph.variable_columns.empty()
        || configuration.graph.variable_columns.size() > 2) {
        return error_page("变异性图", "Variability Chart",
                          "变异性图需要选择 1～2 个因子列。");
    }
    const std::size_t measurement_column = configuration.selection.measurement_column;
    std::set<std::size_t> excluded(
        configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend());
    std::vector<double> measurements;
    std::vector<std::string> factor_a;
    std::vector<std::string> factor_b;
    std::vector<std::size_t> source_rows;
    std::size_t missing_count = 0;
    const bool two_factor = configuration.graph.variable_columns.size() == 2;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (excluded.count(row_index) != 0) {
            continue;
        }
        const auto& row = table.rows[row_index];
        const std::string measurement_text =
            measurement_column < row.size() ? row[measurement_column] : "";
        const auto numeric = parse_numeric_cell(measurement_text);
        const std::size_t factor_a_column = configuration.graph.variable_columns[0];
        const std::string level_a =
            factor_a_column < row.size() ? row[factor_a_column] : "";
        bool complete = numeric.has_value() && !is_missing_cell(level_a);
        std::string level_b;
        if (two_factor) {
            const std::size_t factor_b_column = configuration.graph.variable_columns[1];
            level_b = factor_b_column < row.size() ? row[factor_b_column] : "";
            if (is_missing_cell(level_b)) {
                complete = false;
            }
        }
        if (!complete) {
            ++missing_count;
            continue;
        }
        measurements.push_back(*numeric);
        factor_a.push_back(level_a);
        if (two_factor) {
            factor_b.push_back(level_b);
        }
        source_rows.push_back(row_index);
    }

    std::vector<std::string> factor_names;
    for (const std::size_t column : configuration.graph.variable_columns) {
        factor_names.push_back(column < table.columns.size()
                                   ? table.columns[column]
                                   : column_label(table, column));
    }
    const auto result = datalab::domain::statistics::variability_chart_summarize(
        measurements, factor_a, two_factor ? factor_b : std::vector<std::string>{},
        source_rows);

    // Cell min/max for range bars (presentation; domain already computed mean/SD).
    std::map<std::string, std::vector<double>> cell_values;
    for (std::size_t index = 0; index < measurements.size(); ++index) {
        std::string key = factor_a[index];
        if (two_factor) {
            key += " | ";
            key += factor_b[index];
        }
        cell_values[key].push_back(measurements[index]);
    }

    double overall_sum = 0.0;
    for (const double value : measurements) {
        overall_sum += value;
    }
    const std::optional<double> overall_mean = measurements.empty()
        ? std::optional<double>{}
        : std::optional<double>{overall_sum / static_cast<double>(measurements.size())};

    double sd_sum = 0.0;
    std::size_t sd_count = 0;
    for (const auto& cell : result.cells) {
        if (cell.n >= 2) {
            sd_sum += cell.sample_sd;
            ++sd_count;
        }
    }
    const std::optional<double> mean_of_cell_sds = sd_count > 0
        ? std::optional<double>{sd_sum / static_cast<double>(sd_count)}
        : std::optional<double>{};

    OutputPage page;
    page.id = new_id("variability_chart");
    page.title = "变异性图";
    page.method_name = "Variability Chart";
    page.configuration = configuration;
    const std::string measurement_name =
        measurement_column < table.columns.size()
            ? table.columns[measurement_column]
            : column_label(table, measurement_column);
    page.parameter_summary = "测量 = " + measurement_name + "    因子 = ";
    for (std::size_t index = 0; index < factor_names.size(); ++index) {
        if (index > 0) {
            page.parameter_summary += ", ";
        }
        page.parameter_summary += factor_names[index];
    }
    page.parameter_summary += "    有效观测 = " + std::to_string(result.valid_count)
        + "    缺失 = " + std::to_string(missing_count);
    page.diagnostics = result.diagnostics;
    if (missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "测量或因子缺失、* 或非法单元格已跳过 "
                + std::to_string(missing_count) + " 行。"});
    }
    if (sd_count == 0 && !result.cells.empty()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "variability_sd_panel_empty",
            "所有单元仅 1 个观测，标准差图无可绘点。"});
    }

    StatisticTable cell_table;
    cell_table.title = "单元统计";
    cell_table.headers = {"组合", "N", "Mean", "StDev", "Min", "Max"};
    for (const auto& cell : result.cells) {
        double cell_min = 0.0;
        double cell_max = 0.0;
        const auto found = cell_values.find(cell.label);
        if (found != cell_values.end() && !found->second.empty()) {
            cell_min = *std::min_element(found->second.cbegin(), found->second.cend());
            cell_max = *std::max_element(found->second.cbegin(), found->second.cend());
        }
        cell_table.rows.push_back({
            cell.label,
            std::to_string(cell.n),
            format_number(cell.mean),
            cell.n >= 2 ? format_number(cell.sample_sd) : "*",
            cell.n > 0 ? format_number(cell_min) : "*",
            cell.n > 0 ? format_number(cell_max) : "*"});
    }
    if (!cell_table.rows.empty()) {
        page.tables.push_back(std::move(cell_table));
    }

    if (!result.cells.empty()) {
        std::vector<std::string> categories;
        std::vector<double> means;
        std::vector<double> lowers;
        std::vector<double> uppers;
        std::vector<std::size_t> counts;
        std::vector<std::size_t> plot_source_rows;
        std::vector<double> sds;
        std::vector<std::string> sd_categories;
        std::vector<std::size_t> sd_counts;
        std::vector<std::size_t> sd_source_rows;
        for (const auto& cell : result.cells) {
            categories.push_back(cell.label);
            means.push_back(cell.mean);
            counts.push_back(cell.n);
            plot_source_rows.push_back(
                cell.source_rows.empty() ? 0 : cell.source_rows.front());
            double cell_min = cell.mean;
            double cell_max = cell.mean;
            const auto found = cell_values.find(cell.label);
            if (found != cell_values.end() && !found->second.empty()) {
                cell_min = *std::min_element(found->second.cbegin(), found->second.cend());
                cell_max = *std::max_element(found->second.cbegin(), found->second.cend());
            }
            if (cell.n >= 2) {
                lowers.push_back(cell_min);
                uppers.push_back(cell_max);
                sd_categories.push_back(cell.label);
                sds.push_back(cell.sample_sd);
                sd_counts.push_back(cell.n);
                sd_source_rows.push_back(
                    cell.source_rows.empty() ? 0 : cell.source_rows.front());
            } else {
                lowers.push_back(cell.mean);
                uppers.push_back(cell.mean);
            }
        }

        PlotSpec mean_plot = interval_plot_spec(
            "均值与极差",
            factor_names.empty() ? "因子" : factor_names.front(),
            measurement_name,
            categories, means, lowers, uppers, counts, plot_source_rows);
        if (overall_mean.has_value()) {
            mean_plot.center.assign(categories.size(), *overall_mean);
            mean_plot.center_label = "总均值";
        }
        page.plots.push_back(std::move(mean_plot));

        PlotSpec sd_plot;
        sd_plot.kind = PlotKind::interval;
        sd_plot.title = "标准差图";
        sd_plot.x_axis_title =
            factor_names.empty() ? "因子" : factor_names.front();
        sd_plot.y_axis_title = "StDev";
        sd_plot.categories = sd_categories;
        sd_plot.values = sds;
        sd_plot.interval_lower = sds;
        sd_plot.interval_upper = sds;
        sd_plot.interval_counts = sd_counts;
        sd_plot.source_rows = sd_source_rows;
        if (mean_of_cell_sds.has_value() && !sd_categories.empty()) {
            sd_plot.center.assign(sd_categories.size(), *mean_of_cell_sds);
            sd_plot.center_label = "平均 StDev";
        }
        page.plots.push_back(std::move(sd_plot));
    }

    domain::VariabilityFacts facts;
    facts.factor_count = result.factor_count;
    facts.valid_count = result.valid_count;
    facts.missing_count = missing_count;
    facts.cell_count = result.cells.size();
    facts.overall_mean = overall_mean;
    facts.mean_of_cell_sds = mean_of_cell_sds;
    facts.factor_names = factor_names;
    page.facts.variability = std::move(facts);
    page.method_metadata.estimation_method = "variability_cell_mean_sd";
    page.method_metadata.valid_count = result.valid_count;
    page.method_metadata.missing_count = missing_count;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::tolerance_intervals(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::size_t column = first_variable(configuration);
    if (column >= table.columns.size()) {
        return error_page("容差区间", "Tolerance Intervals", "请选择测量值列。");
    }
    const auto extracted = extract_numeric_column(
        table, column, configuration.excluded_rows);
    const double coverage = configuration.inference.coverage_proportion.value_or(0.95);
    const std::string interval_type = configuration.inference.alternative.empty()
        ? "two_sided" : configuration.inference.alternative;
    const bool use_nonparametric =
        configuration.inference.tolerance_method == "nonparametric"
        || configuration.inference.variance_method == "nonparametric";
    const auto result = use_nonparametric
        ? datalab::domain::statistics::nonparametric_tolerance_interval(
            extracted.values, extracted.source_rows, coverage,
            configuration.inference.confidence_level, interval_type)
        : datalab::domain::statistics::normal_tolerance_interval(
            extracted.values, extracted.source_rows, coverage,
            configuration.inference.confidence_level, interval_type);
    OutputPage page;
    page.id = new_id("tolerance");
    page.title = use_nonparametric ? "非参数容差区间" : "正态容差区间";
    page.method_name = "Tolerance Intervals";
    page.configuration = configuration;
    page.parameter_summary = "测量 = " + extracted.name
        + "    方法 = " + std::string(use_nonparametric ? "nonparametric" : "normal")
        + "    覆盖率 = " + format_number(coverage)
        + "    置信水平 = " + format_number(configuration.inference.confidence_level)
        + "    方向 = " + interval_type;
    page.diagnostics = result.diagnostics;
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info, "missing_values",
            "缺失、* 或非法数值未进入容差区间计算。"});
    }
    StatisticTable process;
    process.title = "过程数据";
    process.headers = {"N", "N*", "Mean", "StDev"};
    process.rows.push_back({
        std::to_string(result.valid_count),
        std::to_string(result.missing_count + extracted.missing_count),
        format_number(result.mean),
        format_number(result.sample_standard_deviation)});
    page.tables.push_back(std::move(process));
    StatisticTable interval;
    interval.title = use_nonparametric ? "非参数容差区间" : "正态容差区间";
    interval.headers = {"方法", "方法族", "覆盖率", "目标置信水平", "Achieved", "k", "下限", "上限"};
    interval.rows.push_back({
        result.method.empty() ? std::string("*") : result.method,
        result.method_family,
        format_number(result.coverage),
        format_number(result.confidence_level),
        format_optional(result.achieved_confidence),
        format_optional(result.k_factor),
        format_optional(result.lower),
        format_optional(result.upper)});
    page.tables.push_back(std::move(interval));
    if (!extracted.values.empty()) {
        const auto bins = datalab::domain::statistics::histogram(extracted.values, 0);
        PlotSpec hist;
        hist.kind = PlotKind::histogram;
        hist.title = "容差区间直方图";
        hist.x_axis_title = extracted.name;
        hist.y_axis_title = "频数";
        hist.histogram_edges = bins.edges;
        hist.histogram_counts = bins.counts;
        hist.values = extracted.values;
        hist.source_rows = extracted.source_rows;
        hist.process_mean = result.mean;
        hist.lsl = result.lower;
        hist.usl = result.upper;
        page.plots.push_back(std::move(hist));
    }
    domain::ToleranceFacts facts;
    facts.valid_count = result.valid_count;
    facts.missing_count = result.missing_count + extracted.missing_count;
    if (result.valid_count >= 2 && result.sample_standard_deviation > 0.0) {
        facts.mean = result.mean;
        facts.standard_deviation = result.sample_standard_deviation;
    }
    facts.coverage = result.coverage;
    facts.confidence_level = result.confidence_level;
    facts.lower = result.lower;
    facts.upper = result.upper;
    facts.k_factor = result.k_factor;
    facts.achieved_confidence = result.achieved_confidence;
    facts.method = result.method;
    facts.method_family = result.method_family;
    facts.interval_type = result.interval_type;
    facts.assumption_status = result.assumption_status;
    page.method_metadata.estimation_method = result.method;
    page.method_metadata.assumption_status = result.assumption_status;
    page.method_metadata.valid_count = result.valid_count;
    page.method_metadata.missing_count = facts.missing_count;
    page.facts.tolerance = std::move(facts);
    page.method_metadata.source_rows.clear();
    for (const std::size_t row : result.source_rows) {
        page.method_metadata.source_rows.push_back(static_cast<domain::RowId>(row));
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::variance_test(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.inference.variance_first_column.has_value()) {
        return error_page("方差检验", "Variance Test", "请选择第一样本列或测量列。");
    }
    const auto first = extract_numeric_column(
        table, *configuration.inference.variance_first_column, configuration.excluded_rows);
    const auto alternative = parse_alternative(configuration.inference.variance_alternative);
    OutputPage page;
    page.id = new_id("variance");
    page.title = "方差检验";
    page.method_name = "Variance Test";
    page.configuration = configuration;
    page.parameter_summary = "第一样本 = "
        + column_label(table, *configuration.inference.variance_first_column)
        + "    方法 = " + configuration.inference.variance_test_method;
    StatisticTable summary;
    summary.title = "方差检验结果";
    summary.headers = {"方法", "N", "统计量", "DF", "P-Value", "置信区间"};

    const auto fill_variance_facts = [&](const std::string& method,
                                         std::optional<double> statistic,
                                         std::optional<double> p_value,
                                         std::size_t group_count,
                                         std::optional<double> ci_lower = std::nullopt,
                                         std::optional<double> ci_upper = std::nullopt) {
        domain::VarianceFacts facts;
        facts.method = method;
        facts.statistic = statistic;
        facts.p_value = p_value;
        facts.ci_lower = ci_lower;
        facts.ci_upper = ci_upper;
        facts.group_count = group_count;
        page.facts.variance = std::move(facts);
    };

    if (configuration.inference.variance_group_column.has_value()) {
        const auto labels = extract_text_column(
            table, *configuration.inference.variance_group_column);
        std::map<std::string, std::vector<double>> grouped;
        for (std::size_t index = 0; index < first.values.size(); ++index) {
            const std::size_t row = first.source_rows[index];
            if (row < labels.size() && !is_missing_cell(labels[row])) {
                grouped[labels[row]].push_back(first.values[index]);
            }
        }
        std::vector<std::vector<double>> groups;
        for (auto& [label, values] : grouped) {
            if (!values.empty()) {
                groups.push_back(std::move(values));
            }
        }
        page.parameter_summary += "    分组列 = "
            + column_label(table, *configuration.inference.variance_group_column);
        if (configuration.inference.variance_test_method == "bonett") {
            if (groups.size() != 2) {
                page.diagnostics.push_back({
                    DiagnosticMessage::Severity::error,
                    "bonett_requires_two_groups",
                    "Bonett 仅支持两样本；k>2 组请改用 Levene（中位数）。"});
                page.tables.push_back(summary);
                fill_variance_facts("Bonett", std::nullopt, std::nullopt, groups.size());
                return finalize_page(std::move(page));
            }
            const auto result = datalab::domain::statistics::bonett_two_variances(
                groups[0], groups[1], configuration.inference.confidence_level, alternative);
            page.diagnostics = result.diagnostics;
            summary.rows.push_back({
                "Bonett",
                std::to_string(result.first_count) + " / "
                    + std::to_string(result.second_count),
                format_number(result.z_statistic),
                "*",
                result.p_value.has_value() ? format_number(*result.p_value) : "*",
                result.confidence_lower.has_value() && result.confidence_upper.has_value()
                    ? "[" + format_number(*result.confidence_lower) + ", "
                        + format_number(*result.confidence_upper) + "]"
                    : "*"});
            page.tables.push_back(summary);
            fill_variance_facts(
                "Bonett", result.z_statistic, result.p_value, 2,
                result.confidence_lower, result.confidence_upper);
            return finalize_page(std::move(page));
        }
        if (configuration.inference.variance_test_method == "bartlett") {
            const auto result = datalab::domain::statistics::bartlett_k_groups(
                groups, configuration.inference.confidence_level);
            page.diagnostics = result.diagnostics;
            summary.rows.push_back({
                "Bartlett", std::to_string(result.total_count),
                format_number(result.chi_square_statistic),
                format_number(result.degrees_of_freedom),
                result.p_value.has_value() ? format_number(*result.p_value) : "*", "*"});
            page.tables.push_back(summary);
            fill_variance_facts(
                "Bartlett", result.chi_square_statistic, result.p_value, result.group_count);
            return finalize_page(std::move(page));
        }
        const auto method = configuration.inference.variance_test_method == "levene_mean"
            ? datalab::domain::statistics::VarianceRobustMethod::levene_mean
            : datalab::domain::statistics::VarianceRobustMethod::brown_forsythe_median;
        const auto result = datalab::domain::statistics::levene_k_groups(
            groups, configuration.inference.confidence_level, method);
        page.diagnostics = result.diagnostics;
        const char* method_name = method == datalab::domain::statistics::VarianceRobustMethod::levene_mean
            ? "Levene (mean)" : "Levene";
        summary.rows.push_back({
            method_name, std::to_string(result.total_count),
            format_number(result.f_statistic),
            format_number(result.numerator_degrees_of_freedom) + " / "
                + format_number(result.denominator_degrees_of_freedom),
            result.p_value.has_value() ? format_number(*result.p_value) : "*", "*"});
        page.tables.push_back(summary);
        fill_variance_facts(method_name, result.f_statistic, result.p_value, result.group_count);
        return finalize_page(std::move(page));
    }

    if (!configuration.inference.variance_second_column.has_value()) {
        if (!configuration.inference.hypothesized_variance.has_value()) {
            return error_page("一方差检验", "1 Variance",
                              "一方差检验需要输入假设方差。");
        }
        const auto result = datalab::domain::statistics::chi_square_one_variance_test(
            first.values, *configuration.inference.hypothesized_variance,
            configuration.inference.confidence_level, alternative);
        page.diagnostics = result.diagnostics;
        summary.rows.push_back({
            "Chi-Square", std::to_string(result.count),
            format_number(result.chi_square_statistic),
            format_number(result.degrees_of_freedom),
            result.p_value.has_value() ? format_number(*result.p_value) : "*",
            result.confidence_lower.has_value() && result.confidence_upper.has_value()
                ? "[" + format_number(*result.confidence_lower) + ", "
                    + format_number(*result.confidence_upper) + "]" : "*"});
        page.tables.push_back(summary);
        fill_variance_facts("Chi-Square", result.chi_square_statistic, result.p_value, 1,
                            result.confidence_lower, result.confidence_upper);
        return finalize_page(std::move(page));
    }

    const auto second = extract_numeric_column(
        table, *configuration.inference.variance_second_column, configuration.excluded_rows);

    if (configuration.inference.variance_test_method == "bonett") {
        const auto result = datalab::domain::statistics::bonett_two_variances(
            first.values, second.values, configuration.inference.confidence_level, alternative);
        page.diagnostics = result.diagnostics;
        summary.rows.push_back({
            "Bonett",
            std::to_string(result.first_count) + " / "
                + std::to_string(result.second_count),
            format_number(result.z_statistic),
            "*",
            result.p_value.has_value() ? format_number(*result.p_value) : "*",
            result.confidence_lower.has_value() && result.confidence_upper.has_value()
                ? "[" + format_number(*result.confidence_lower) + ", "
                    + format_number(*result.confidence_upper) + "]"
                : "*"});
        page.tables.push_back(summary);
        fill_variance_facts(
            "Bonett", result.z_statistic, result.p_value, 2,
            result.confidence_lower, result.confidence_upper);
        return finalize_page(std::move(page));
    }

    if (configuration.inference.variance_test_method == "bartlett") {
        const auto result = datalab::domain::statistics::bartlett_k_groups(
            {first.values, second.values}, configuration.inference.confidence_level);
        page.diagnostics = result.diagnostics;
        summary.rows.push_back({
            "Bartlett", std::to_string(result.total_count),
            format_number(result.chi_square_statistic),
            format_number(result.degrees_of_freedom),
            result.p_value.has_value() ? format_number(*result.p_value) : "*", "*"});
        page.tables.push_back(summary);
        fill_variance_facts(
            "Bartlett", result.chi_square_statistic, result.p_value, result.group_count);
        return finalize_page(std::move(page));
    }

    const auto f_result = datalab::domain::statistics::f_test_two_variances(
        first.values, second.values, configuration.inference.confidence_level, alternative);
    page.diagnostics = f_result.diagnostics;
    summary.rows.push_back({
        "F-test", std::to_string(f_result.first_count) + " / "
            + std::to_string(f_result.second_count),
        format_number(f_result.f_statistic),
        format_number(f_result.numerator_degrees_of_freedom) + " / "
            + format_number(f_result.denominator_degrees_of_freedom),
        f_result.p_value.has_value() ? format_number(*f_result.p_value) : "*",
        f_result.confidence_lower.has_value() && f_result.confidence_upper.has_value()
            ? "[" + format_number(*f_result.confidence_lower) + ", "
                + format_number(*f_result.confidence_upper) + "]" : "*"});
    const auto robust = configuration.inference.variance_test_method == "levene_mean"
        ? datalab::domain::statistics::levene_mean_two_variances(
            first.values, second.values, configuration.inference.confidence_level, alternative)
        : datalab::domain::statistics::levene_two_variances(
            first.values, second.values, configuration.inference.confidence_level, alternative);
    page.diagnostics.insert(page.diagnostics.end(), robust.diagnostics.cbegin(),
                            robust.diagnostics.cend());
    const char* robust_name = configuration.inference.variance_test_method == "levene_mean"
        ? "Levene (mean)" : "Levene";
    summary.rows.push_back({
        robust_name,
        std::to_string(robust.total_count),
        format_number(robust.f_statistic),
        format_number(robust.numerator_degrees_of_freedom) + " / "
            + format_number(robust.denominator_degrees_of_freedom),
        robust.p_value.has_value() ? format_number(*robust.p_value) : "*", "*"});
    page.tables.push_back(summary);
    const bool use_f = configuration.inference.variance_test_method == "f";
    fill_variance_facts(
        use_f ? "F-test" : robust_name,
        use_f ? std::optional<double>(f_result.f_statistic) : robust.f_statistic,
        use_f ? f_result.p_value : robust.p_value,
        2,
        use_f ? f_result.confidence_lower : std::nullopt,
        use_f ? f_result.confidence_upper : std::nullopt);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::time_series_decomposition(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.time_series.decomposition_value_column.has_value()
        || configuration.time_series.decomposition_seasonal_period < 1) {
        return error_page("时间序列分解", "Time Series Decomposition",
                          "请选择时间序列值列并输入正整数季节周期。");
    }
    const auto values = extract_numeric_column(
        table, *configuration.time_series.decomposition_value_column, configuration.excluded_rows);
    if (values.values.empty()) {
        return error_page("时间序列分解", "Time Series Decomposition", "时间序列没有有效数值观测。");
    }
    std::vector<double> time;
    std::vector<double> aligned_values;
    std::vector<std::size_t> aligned_source_rows;
    std::size_t aligned_missing = values.missing_count;
    if (configuration.time_series.decomposition_time_column.has_value()) {
        const auto extracted = extract_numeric_column(
            table, *configuration.time_series.decomposition_time_column, configuration.excluded_rows);
        aligned_missing += extracted.missing_count;
        std::map<std::size_t, double> time_by_row;
        for (std::size_t index = 0; index < extracted.source_rows.size(); ++index) {
            time_by_row[extracted.source_rows[index]] = extracted.values[index];
        }
        for (std::size_t index = 0; index < values.source_rows.size(); ++index) {
            const std::size_t row = values.source_rows[index];
            const auto time_it = time_by_row.find(row);
            if (time_it == time_by_row.end()) {
                ++aligned_missing;
                continue;
            }
            aligned_source_rows.push_back(row);
            aligned_values.push_back(values.values[index]);
            time.push_back(time_it->second);
        }
        if (aligned_values.empty()) {
            return error_page("时间序列分解", "Time Series Decomposition",
                              "时间列与值列没有可对齐的完整观测。");
        }
    } else {
        aligned_values = values.values;
        aligned_source_rows = values.source_rows;
        time.reserve(aligned_values.size());
        for (std::size_t index = 0; index < aligned_values.size(); ++index) {
            time.push_back(static_cast<double>(index + 1));
        }
    }
    const auto result = datalab::domain::statistics::decompose_time_series(
        {time, aligned_values},
        {configuration.time_series.decomposition_model == "multiplicative"
             ? datalab::domain::statistics::DecompositionModel::multiplicative
             : datalab::domain::statistics::DecompositionModel::additive,
         static_cast<std::size_t>(configuration.time_series.decomposition_seasonal_period),
         static_cast<std::size_t>(std::max(1, configuration.time_series.forecast_periods))});
    OutputPage page;
    page.id = new_id("decomposition");
    page.title = "时间序列分解";
    page.method_name = "Time Series Decomposition";
    page.configuration = configuration;
    page.parameter_summary = "值列 = "
        + column_label(table, *configuration.time_series.decomposition_value_column)
        + "    周期 = " + std::to_string(configuration.time_series.decomposition_seasonal_period)
        + "    模型 = " + configuration.time_series.decomposition_model
        + "    有效观测 = " + std::to_string(aligned_values.size());
    page.diagnostics = result.diagnostics;
    if (configuration.time_series.decomposition_time_column.has_value()
        && aligned_values.size() < values.values.size()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "incomplete_time_value_pairs",
            "时间列与值列未配对的行已跳过，分解仅使用可对齐的完整观测。"});
    }
    if (aligned_missing > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "missing_values",
            "分解跳过缺失或无效的时间/数值单元格。"});
    }
    StatisticTable summary;
    summary.title = "预测准确度";
    summary.headers = {"N", "Trend Intercept", "Trend Slope", "MAD", "MSD", "MAPE"};
    summary.rows.push_back({
        std::to_string(result.observations.size()),
        format_number(result.trend_intercept), format_number(result.trend_slope),
        format_number(result.mad), format_number(result.msd), format_number(result.mape)});
    page.tables.push_back(summary);
    StatisticTable detail;
    detail.title = "拟合与预测明细";
    detail.headers = {"序号", "原始行", "Time", "Observed", "Moving Average", "Trend",
                      "Seasonal Index", "Fitted", "Residual", "Forecast"};
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        const std::size_t phase = result.seasonal_period > 0
            ? (index % result.seasonal_period) : 0;
        detail.rows.push_back({
            std::to_string(index + 1),
            index < aligned_source_rows.size() ? std::to_string(aligned_source_rows[index] + 1) : "*",
            index < result.time.size() ? format_number(result.time[index]) : "*",
            format_number(result.observations[index]),
            index < result.centered_moving_average.size()
                ? format_number(result.centered_moving_average[index]) : "*",
            index < result.trend.size() ? format_number(result.trend[index]) : "*",
            phase < result.seasonal_indices.size()
                ? format_number(result.seasonal_indices[phase]) : "*",
            index < result.fitted.size() ? format_number(result.fitted[index]) : "*",
            index < result.residuals.size() ? format_number(result.residuals[index]) : "*",
            ""});
    }
    for (std::size_t index = 0; index < result.forecasts.size(); ++index) {
        detail.rows.push_back({
            std::to_string(result.observations.size() + index + 1),
            "",
            "*",
            "",
            "",
            "",
            "",
            "",
            "",
            format_number(result.forecasts[index])});
    }
    page.tables.push_back(detail);
    StatisticTable seasonal;
    seasonal.title = "季节指数";
    seasonal.headers = {"Phase", "Seasonal Index"};
    for (std::size_t phase = 0; phase < result.seasonal_indices.size(); ++phase) {
        seasonal.rows.push_back({
            std::to_string(phase + 1),
            format_number(result.seasonal_indices[phase])});
    }
    page.tables.push_back(seasonal);
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "时间序列分解拟合";
    plot.x_axis_title = "Time";
    plot.y_axis_title = column_label(table, *configuration.time_series.decomposition_value_column);
    plot.x_values = result.time;
    plot.values = result.observations;
    datalab::domain::PlotSeries observed;
    observed.role = datalab::domain::PlotSeriesRole::actual;
    observed.label = "实际值";
    observed.x_values = result.time;
    observed.values = result.observations;
    observed.show_points = true;
    datalab::domain::PlotSeries trend;
    trend.role = datalab::domain::PlotSeriesRole::trend;
    trend.label = "趋势";
    trend.x_values = result.time;
    trend.values = result.trend;
    datalab::domain::PlotSeries fitted;
    fitted.role = datalab::domain::PlotSeriesRole::fitted;
    fitted.label = "拟合值";
    fitted.x_values = result.time;
    fitted.values = result.fitted;
    datalab::domain::PlotSeries remainder;
    remainder.role = datalab::domain::PlotSeriesRole::remainder;
    remainder.label = "残差";
    remainder.x_values = result.time;
    remainder.values = result.residuals;
    datalab::domain::PlotSeries forecast;
    forecast.role = datalab::domain::PlotSeriesRole::forecast;
    forecast.label = "预测值";
    forecast.values = result.forecasts;
    if (!result.time.empty() && result.time.size() > 1 && !result.forecasts.empty()) {
        const double step = result.time.back() - result.time[result.time.size() - 2];
        forecast.x_values.reserve(result.forecasts.size());
        for (std::size_t index = 0; index < result.forecasts.size(); ++index) {
            forecast.x_values.push_back(result.time.back() + step * static_cast<double>(index + 1));
        }
    }
    plot.series = {std::move(observed), std::move(trend), std::move(fitted),
                   std::move(remainder), std::move(forecast)};
    plot.source_rows = aligned_source_rows;
    page.plots.push_back(plot);
    page.facts.forecast = domain::ForecastFacts{result.mape, std::nullopt, std::nullopt, std::nullopt};
    page.method_metadata.estimation_method = "classical_decomposition_cma_trend";
    page.method_metadata.parameter_source = "estimated";
    page.method_metadata.valid_count = aligned_values.size();
    page.method_metadata.missing_count = aligned_missing;
    page.method_metadata.source_rows.clear();
    page.method_metadata.source_rows.reserve(aligned_source_rows.size());
    for (const std::size_t row : aligned_source_rows) {
        page.method_metadata.source_rows.push_back(static_cast<domain::RowId>(row));
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::rsm_response(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() < 3) {
        return error_page("响应曲面分析", "RSM Response Analysis",
                          "请选择响应列与至少两个连续因子列。");
    }
    const auto response_column = extract_numeric_column(
        table, configuration.variable_columns.front(), configuration.excluded_rows);
    std::vector<ExtractedNumericColumn> factor_extracts;
    for (std::size_t index = 1; index < configuration.variable_columns.size(); ++index) {
        factor_extracts.push_back(extract_numeric_column(
            table, configuration.variable_columns[index], configuration.excluded_rows));
    }
    std::vector<ExtractedNumericColumn> columns;
    columns.reserve(factor_extracts.size() + 1);
    columns.push_back(response_column);
    columns.insert(columns.end(), factor_extracts.begin(), factor_extracts.end());
    const auto aligned = align_complete_rows_with_source(columns);
    if (aligned.values.size() < 3) {
        return error_page("响应曲面分析", "RSM Response Analysis",
                          "complete-case 有效行不足，无法拟合二次模型。");
    }
    std::vector<double> response;
    std::vector<std::vector<double>> raw_factors;
    response.reserve(aligned.values.size());
    raw_factors.reserve(aligned.values.size());
    for (const auto& row : aligned.values) {
        response.push_back(row[0]);
        raw_factors.emplace_back(row.begin() + 1, row.end());
    }
    OutputPage page;
    page.id = new_id("rsm_response");
    page.title = "响应曲面分析";
    page.method_name = "Response Surface Analysis";
    page.configuration = configuration;
    std::vector<std::string> factor_names;
    for (std::size_t index = 1; index < configuration.variable_columns.size(); ++index) {
        const std::size_t column = configuration.variable_columns[index];
        factor_names.push_back(column < table.columns.size()
            ? table.columns[column]
            : column_label(table, column));
    }
    const std::size_t response_index = configuration.variable_columns.front();
    const std::string response_name = response_index < table.columns.size()
        ? table.columns[response_index]
        : column_label(table, response_index);
    page.parameter_summary = "响应 = " + response_name
        + "    因子数 = " + std::to_string(factor_names.size())
        + "    有效观测 = " + std::to_string(response.size())
        + "    模型 = 线性+交互+纯二次（编码单位）";
    const auto& design_cfg = configuration.response_surface_design;
    const bool use_design_bounds =
        !design_cfg.low_levels.empty()
        && design_cfg.low_levels.size() == factor_names.size()
        && design_cfg.high_levels.size() == factor_names.size();
    std::vector<DiagnosticMessage> coding_diagnostics;
    std::vector<std::vector<double>> coded;
    std::string coding_mode;
    if (use_design_bounds) {
        coded = datalab::domain::statistics::code_rsm_factors_from_design_bounds(
            raw_factors,
            design_cfg.low_levels,
            design_cfg.high_levels,
            design_cfg.centers,
            coding_diagnostics);
        coding_mode = "design_bounds";
    } else {
        coded = datalab::domain::statistics::code_rsm_factors(
            raw_factors, coding_diagnostics);
        coding_mode = "minmax";
        for (const auto& message : coding_diagnostics) {
            if (message.code == "rsm_factors_already_coded") {
                coding_mode = "already_coded";
                break;
            }
        }
    }
    page.diagnostics.insert(page.diagnostics.end(),
                            coding_diagnostics.cbegin(), coding_diagnostics.cend());
    if (coded.empty()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "rsm_coding_failed",
            "因子编码失败。"});
        return finalize_page(std::move(page));
    }

    std::size_t center_point_count = 0;
    for (const auto& row : coded) {
        bool is_center = true;
        for (const double value : row) {
            if (std::abs(value) > 1.0e-9) {
                is_center = false;
                break;
            }
        }
        if (is_center) {
            ++center_point_count;
        }
    }
    if (center_point_count == 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "rsm_no_center_points",
            "未检测到中心点（编码全 0）；纯误差/失拟诊断受限。"});
    }
    if (!design_cfg.design_source_id.empty()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info, "rsm_design_source",
            "设计来源 ID = " + design_cfg.design_source_id
                + (design_cfg.design_kind.empty()
                       ? ""
                       : ("；设计族 = " + design_cfg.design_kind))
                + "。"});
    }

    const auto fit = datalab::domain::statistics::fit_rsm_analysis(
        response, coded, factor_names, response_name, aligned.source_rows);
    page.diagnostics.insert(page.diagnostics.end(),
                            fit.diagnostics.cbegin(), fit.diagnostics.cend());
    const auto& result = fit.regression;
    StatisticTable summary_table;
    summary_table.title = "模型摘要";
    summary_table.headers = {"S", "R-sq", "R-sq(adj)", "R-sq(pred)", "F", "P-Value"};
    summary_table.rows.push_back({
        format_number(result.residual_standard_deviation),
        format_number(result.r_squared),
        format_number(result.adjusted_r_squared),
        format_number(result.predicted_r_squared),
        format_number(result.f_statistic),
        result.model_p_value.has_value() ? format_number(*result.model_p_value) : "*"});
    page.tables.push_back(std::move(summary_table));
    StatisticTable coefficient_table;
    coefficient_table.title = "系数（编码单位）";
    coefficient_table.headers = {"项", "Coef", "SE Coef", "T", "P-Value"};
    for (const auto& coefficient : result.coefficients) {
        coefficient_table.rows.push_back({
            coefficient.term,
            format_number(coefficient.coefficient),
            format_number(coefficient.standard_error),
            format_number(coefficient.t_statistic),
            coefficient.p_value.has_value() ? format_number(*coefficient.p_value) : "*"});
    }
    page.tables.push_back(std::move(coefficient_table));
    StatisticTable anova_table;
    anova_table.title = "方差分析";
    anova_table.headers = {"来源", "DF", "Adj SS", "MS", "F", "P-Value"};
    anova_table.rows.push_back({
        "回归",
        std::to_string(result.predictor_count),
        format_number(result.regression_sum_of_squares),
        format_number(result.regression_mean_square),
        format_number(result.f_statistic),
        result.model_p_value.has_value() ? format_number(*result.model_p_value) : "*"});
    const std::size_t error_df = result.observation_count > result.predictor_count + 1
        ? result.observation_count - result.predictor_count - 1
        : 0;
    anova_table.rows.push_back({
        "误差",
        std::to_string(error_df),
        format_number(result.error_sum_of_squares),
        format_number(result.error_mean_square), "", ""});
    if (fit.lack_of_fit_anova_row.has_value()) {
        const auto& lof = *fit.lack_of_fit_anova_row;
        anova_table.rows.push_back({
            "失拟",
            std::to_string(lof.degrees_of_freedom),
            format_number(lof.sum_of_squares),
            format_number(lof.mean_square),
            format_number(lof.f_statistic),
            lof.p_value.has_value() ? format_number(*lof.p_value) : "*"});
    }
    if (fit.pure_error_anova_row.has_value()) {
        const auto& pe = *fit.pure_error_anova_row;
        anova_table.rows.push_back({
            "纯误差",
            std::to_string(pe.degrees_of_freedom),
            format_number(pe.sum_of_squares),
            format_number(pe.mean_square), "", ""});
    }
    anova_table.rows.push_back({
        "合计",
        std::to_string(result.observation_count > 0 ? result.observation_count - 1 : 0),
        format_number(result.total_sum_of_squares), "", "", ""});
    page.tables.push_back(std::move(anova_table));

    PlotSpec residual_plot;
    residual_plot.kind = PlotKind::scatter;
    residual_plot.title = "残差与拟合值";
    residual_plot.x_axis_title = "拟合值";
    residual_plot.y_axis_title = "残差";
    for (const auto& observation : result.observations) {
        residual_plot.x_values.push_back(observation.fitted);
        residual_plot.values.push_back(observation.residual);
    }
    add_zero_residual_reference(residual_plot);
    page.plots.push_back(std::move(residual_plot));
    PlotSpec order_plot;
    order_plot.kind = PlotKind::scatter;
    order_plot.title = "残差与观测顺序";
    order_plot.x_axis_title = "观测顺序";
    order_plot.y_axis_title = "残差";
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        order_plot.x_values.push_back(static_cast<double>(index + 1));
        order_plot.values.push_back(result.observations[index].residual);
    }
    add_zero_residual_reference(order_plot);
    page.plots.push_back(std::move(order_plot));
    {
        PlotSpec residual_probability;
        residual_probability.kind = PlotKind::probability;
        residual_probability.title = "残差正态概率图";
        residual_probability.x_axis_title = "理论分位数";
        residual_probability.y_axis_title = "残差";
        std::vector<double> residuals;
        residuals.reserve(result.observations.size());
        for (const auto& observation : result.observations) {
            residuals.push_back(observation.residual);
        }
        const auto probability = datalab::domain::statistics::normal_probability_plot(
            residuals, aligned.source_rows);
        residual_probability.x_values = probability.theoretical_quantiles;
        residual_probability.values = probability.ordered_values;
        residual_probability.source_rows = probability.source_rows;
        page.plots.push_back(std::move(residual_probability));
    }
    PlotSpec hist;
    hist.kind = PlotKind::histogram;
    hist.title = "残差直方图";
    hist.x_axis_title = "残差";
    for (const auto& observation : result.observations) {
        hist.values.push_back(observation.residual);
    }
    page.plots.push_back(std::move(hist));

    domain::RsmFacts facts;
    facts.factor_count = factor_names.size();
    facts.term_count = result.coefficients.size();
    facts.residual_count = result.observations.size();
    facts.r_squared = result.r_squared;
    facts.adjusted_r_squared = result.adjusted_r_squared;
    facts.response_name = response_name;
    facts.design_source_id = design_cfg.design_source_id;
    facts.design_kind = design_cfg.design_kind;
    facts.coding_mode = coding_mode;
    facts.center_point_count = center_point_count;
    facts.surface_is_static = true;
    facts.evidence_type = "formula_reference";
    facts.pure_error_available = fit.pure_error_anova_row.has_value();
    facts.lack_of_fit_available = fit.lack_of_fit_anova_row.has_value();
    if (fit.pure_error_anova_row.has_value()) {
        facts.pure_error_df = fit.pure_error_anova_row->degrees_of_freedom;
    }
    if (fit.lack_of_fit_anova_row.has_value()) {
        facts.lack_of_fit_df = fit.lack_of_fit_anova_row->degrees_of_freedom;
        facts.lack_of_fit_f = fit.lack_of_fit_anova_row->f_statistic;
        facts.lack_of_fit_p = fit.lack_of_fit_anova_row->p_value;
    }
    double largest_t = -1.0;
    for (std::size_t index = 1; index < result.coefficients.size(); ++index) {
        const double magnitude = std::abs(result.coefficients[index].t_statistic);
        if (magnitude > largest_t) {
            largest_t = magnitude;
            facts.largest_abs_t_term = result.coefficients[index].term;
        }
    }
    if (factor_names.size() >= 2 && !result.coefficients.empty()) {
        const auto grid = datalab::domain::statistics::evaluate_rsm_grid(fit, 0, 1, 25);
        page.diagnostics.insert(page.diagnostics.end(),
                                grid.diagnostics.cbegin(), grid.diagnostics.cend());
        if (!grid.x.empty() && !grid.y.empty() && !grid.z.empty()) {
            facts.contour_plot_available = true;
            PlotSpec contour;
            contour.kind = PlotKind::contour;
            contour.title = "等值线图 - " + factor_names[0] + " vs " + factor_names[1];
            contour.x_axis_title = factor_names[0] + "（编码）";
            contour.y_axis_title = factor_names[1] + "（编码）";
            contour.contour_x = grid.x;
            contour.contour_y = grid.y;
            contour.matrix_values = grid.z;
            double minimum = grid.z.front().front();
            double maximum = minimum;
            for (const auto& row : grid.z) {
                for (const double value : row) {
                    minimum = std::min(minimum, value);
                    maximum = std::max(maximum, value);
                }
            }
            contour.color_min = minimum;
            contour.color_max = maximum;
            page.plots.push_back(contour);
            PlotSpec surface = contour;
            surface.kind = PlotKind::surface;
            surface.title = "静态响应曲面图（非可旋转 3D）- "
                + factor_names[0] + " vs " + factor_names[1];
            page.plots.push_back(std::move(surface));
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::info, "rsm_static_surface",
                "曲面图为静态栅格可视化，不得解读为可旋转交互 3D。"});
        }
    }
    page.facts.rsm = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::doe_factorial(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.doe.response_column.has_value()
        || !configuration.doe.factor_columns.empty()) {
        if (!configuration.doe.response_column.has_value()
            || configuration.doe.factor_columns.empty()) {
            return error_page("DOE 响应分析", "DOE Response Analysis",
                              "请选择响应列和至少一个设计因子列。");
        }
        if (configuration.doe.factor_columns.size()
            >= std::numeric_limits<std::size_t>::digits) {
            return error_page("DOE 响应分析", "DOE Response Analysis",
                              "因子数量过大，无法建立 2 水平模型。");
        }
        if (*configuration.doe.response_column >= table.columns.size()) {
            return error_page("DOE 响应分析", "DOE Response Analysis",
                              "响应列索引超出当前数据表范围。");
        }
        for (const std::size_t column : configuration.doe.factor_columns) {
            if (column >= table.columns.size()) {
                return error_page("DOE 响应分析", "DOE Response Analysis",
                                  "设计因子列索引超出当前数据表范围。");
            }
        }
        const ImportedFactorialRuns imported =
            import_factorial_runs_from_worksheet(table, configuration);
        if (imported.design.runs.empty()) {
            return error_page("DOE 响应分析", "DOE Response Analysis",
                              "没有可用于响应分析的有效 DOE 运行。");
        }
        datalab::domain::statistics::DoeFactorialDesign imported_design = imported.design;
        if (imported.skipped_level_rows > 0) {
            imported_design.diagnostics.push_back({
                datalab::domain::DiagnosticMessage::Severity::warning,
                "missing_doe_run", "存在缺少有效因子水平的运行，已跳过。"});
        }
        if (imported.center_run_count > 0) {
            imported_design.diagnostics.push_back({
                datalab::domain::DiagnosticMessage::Severity::info,
                "doe_center_runs_imported",
                "已按 PointType/中心水平导入中心点运行，可用于曲率与纯误差。"});
        }
        std::vector<double> responses;
        responses.reserve(imported.source_rows.size());
        for (const std::size_t row_index : imported.source_rows) {
            if (row_index >= table.rows.size()
                || *configuration.doe.response_column >= table.rows[row_index].size()) {
                responses.push_back(std::numeric_limits<double>::quiet_NaN());
                continue;
            }
            responses.push_back(
                parse_numeric_cell(
                    table.rows[row_index][*configuration.doe.response_column])
                    .value_or(std::numeric_limits<double>::quiet_NaN()));
        }
        const auto fit = datalab::domain::statistics::fit_response_analysis(
            imported_design, responses,
            column_label(table, *configuration.doe.response_column));
        return doe_response_page(table, configuration, imported_design, responses, fit);
    }
    std::vector<datalab::domain::statistics::DoeFactor> factors;
    for (std::size_t index = 0; index < configuration.doe.factor_names.size(); ++index) {
        datalab::domain::statistics::DoeFactor factor;
        factor.name = configuration.doe.factor_names[index];
        factor.low_level = index < configuration.doe.low_levels.size()
            ? configuration.doe.low_levels[index] : "-1";
        factor.high_level = index < configuration.doe.high_levels.size()
            ? configuration.doe.high_levels[index] : "+1";
        factors.push_back(std::move(factor));
    }
    const auto design = datalab::domain::statistics::generate_2_level_factorial({
        factors,
        configuration.doe.center_point_count,
        configuration.doe.block_count,
        configuration.doe.randomize,
        configuration.doe.random_seed,
        configuration.doe.fraction_p,
        configuration.doe.generators_text});
    return doe_design_page(configuration, factors, design);
}

namespace {

struct ImportedDoeRuns {
    datalab::domain::statistics::DoeFactorialDesign design;
    std::vector<std::size_t> source_rows;
    std::size_t skipped_level_rows = 0;
};

std::vector<std::size_t> resolve_doe_response_columns(
    const AnalysisConfiguration& configuration)
{
    if (!configuration.doe.response_columns.empty()) {
        return configuration.doe.response_columns;
    }
    if (configuration.doe.response_column.has_value()) {
        return {*configuration.doe.response_column};
    }
    return {};
}

datalab::domain::statistics::ResponseGoal parse_response_goal(
    const std::string& goal_text)
{
    if (goal_text == "minimize") {
        return datalab::domain::statistics::ResponseGoal::minimize;
    }
    if (goal_text == "target") {
        return datalab::domain::statistics::ResponseGoal::target;
    }
    return datalab::domain::statistics::ResponseGoal::maximize;
}

domain::DoeResponseObjectiveConfig objective_config_for(
    const domain::DoeConfiguration& doe,
    std::size_t index)
{
    if (index < doe.optimization_objectives.size()) {
        return doe.optimization_objectives[index];
    }
    domain::DoeResponseObjectiveConfig config;
    config.goal = doe.optimization_goal;
    config.lower = doe.optimization_lower;
    config.upper = doe.optimization_upper;
    config.target = doe.optimization_target;
    config.weight = doe.optimization_weight;
    return config;
}

ImportedDoeRuns import_doe_factorial_runs(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const ImportedFactorialRuns shared =
        import_factorial_runs_from_worksheet(table, configuration);
    ImportedDoeRuns imported;
    imported.design = shared.design;
    imported.source_rows = shared.source_rows;
    imported.skipped_level_rows = shared.skipped_level_rows;
    return imported;
}

std::vector<double> extract_doe_responses(
    const DataTable& table,
    std::size_t response_column,
    const ImportedDoeRuns& imported,
    std::size_t& skipped_response_rows)
{
    std::vector<double> responses;
    responses.reserve(imported.source_rows.size());
    for (const std::size_t row_index : imported.source_rows) {
        if (row_index >= table.rows.size()
            || response_column >= table.rows[row_index].size()) {
            ++skipped_response_rows;
            responses.push_back(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        const auto response = parse_numeric_cell(table.rows[row_index][response_column]);
        if (response.has_value()) {
            responses.push_back(*response);
        } else {
            ++skipped_response_rows;
            responses.push_back(std::numeric_limits<double>::quiet_NaN());
        }
    }
    return responses;
}

datalab::domain::statistics::ResponseModel build_response_model(
    const datalab::domain::statistics::DoeResponseAnalysisResult& fit,
    const datalab::domain::statistics::DoeFactorialDesign& design,
    double confidence_level)
{
    datalab::domain::statistics::ResponseModel model;
    model.response_name = fit.response_name;
    model.observation_count = fit.residuals.size();
    model.residual_degrees_of_freedom =
        static_cast<double>(fit.residual_degrees_of_freedom);
    model.residual_standard_error = std::sqrt(std::max(0.0, fit.residual_mean_square));
    model.confidence_level = confidence_level;
    for (const auto& factor : design.factors) {
        model.factor_names.push_back(factor.name);
    }
    if (!fit.coefficients.empty()) {
        model.intercept = fit.coefficients.front();
    }
    const std::size_t factor_count = design.factors.size();
    for (std::size_t index = 0; index < factor_count
         && index + 1 < fit.coefficients.size(); ++index) {
        model.main_effect_coefficients.push_back(fit.coefficients[index + 1]);
    }
    std::size_t coefficient_index = 1 + factor_count;
    for (std::size_t first = 0; first < factor_count; ++first) {
        for (std::size_t second = first + 1; second < factor_count; ++second) {
            if (coefficient_index >= fit.coefficients.size()) {
                break;
            }
            model.interaction_coefficients.push_back({
                design.factors[first].name,
                design.factors[second].name,
                fit.coefficients[coefficient_index]});
            ++coefficient_index;
        }
    }
    const std::size_t expected_terms =
        1 + model.main_effect_coefficients.size() + model.interaction_coefficients.size();
    if (fit.xtx_inverse.size() == expected_terms
        && std::isfinite(fit.residual_mean_square) && fit.residual_mean_square > 0.0) {
        model.coefficient_covariance = fit.xtx_inverse;
        for (auto& row : model.coefficient_covariance) {
            for (double& value : row) {
                value *= fit.residual_mean_square;
            }
        }
    } else {
        model.observation_count = 0;
    }
    return model;
}

std::vector<std::string> significant_doe_terms(
    const datalab::domain::statistics::DoeResponseAnalysisResult& fit,
    double alpha)
{
    std::vector<std::string> terms;
    for (const auto& row : fit.model_anova_rows) {
        if (row.p_value.has_value() && *row.p_value < alpha && !row.source.empty()) {
            terms.push_back(row.source);
        }
    }
    return terms;
}

datalab::domain::statistics::ResponseObjective build_response_objective(
    const datalab::domain::statistics::ResponseModel& model,
    const AnalysisConfiguration& configuration,
    std::size_t objective_index,
    const std::vector<double>& responses)
{
    const domain::DoeResponseObjectiveConfig config =
        objective_config_for(configuration.doe, objective_index);
    double observed_min = 0.0;
    double observed_max = 1.0;
    bool have_range = false;
    for (const double value : responses) {
        if (!std::isfinite(value)) {
            continue;
        }
        if (!have_range) {
            observed_min = value;
            observed_max = value;
            have_range = true;
        } else {
            observed_min = std::min(observed_min, value);
            observed_max = std::max(observed_max, value);
        }
    }
    datalab::domain::statistics::ResponseObjective objective;
    objective.response_name = model.response_name;
    objective.goal = parse_response_goal(config.goal);
    objective.lower = config.lower.value_or(observed_min);
    objective.upper = config.upper.value_or(observed_max);
    objective.target = config.target.value_or((objective.lower + objective.upper) / 2.0);
    objective.weight = config.weight;
    return objective;
}

}  // namespace

OutputPage AnalysisService::response_optimization(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::vector<std::size_t> response_columns =
        resolve_doe_response_columns(configuration);
    if (response_columns.empty() || configuration.doe.factor_columns.empty()) {
        return error_page("DOE 响应优化", "Response Optimization",
                          "请选择至少一个响应列和已导入因子列。");
    }
    for (const std::size_t column : response_columns) {
        if (column >= table.columns.size()) {
            return error_page("DOE 响应优化", "Response Optimization",
                              "响应列索引超出当前数据表范围。");
        }
    }
    for (const std::size_t column : configuration.doe.factor_columns) {
        if (column >= table.columns.size()) {
            return error_page("DOE 响应优化", "Response Optimization",
                              "设计因子列索引超出当前数据表范围。");
        }
    }
    const ImportedDoeRuns imported = import_doe_factorial_runs(table, configuration);
    if (imported.design.runs.empty()) {
        return error_page("DOE 响应优化", "Response Optimization",
                          "没有可用于优化的有效 DOE 运行。");
    }
    std::vector<datalab::domain::statistics::ResponseModel> models;
    std::vector<datalab::domain::statistics::ResponseObjective> objectives;
    std::vector<datalab::domain::statistics::DoeResponseAnalysisResult> fits;
    std::vector<std::string> response_names;
    std::size_t skipped_response_rows = 0;
    bool exact_prediction_intervals_available = true;
    const double alpha = 1.0 - configuration.inference.confidence_level;
    domain::DoeFacts doe_facts;
    doe_facts.response_count = response_columns.size();
    doe_facts.multi_response = response_columns.size() > 1;
    for (std::size_t index = 0; index < response_columns.size(); ++index) {
        const std::size_t response_column = response_columns[index];
        std::size_t response_missing = 0;
        const std::vector<double> responses = extract_doe_responses(
            table, response_column, imported, response_missing);
        skipped_response_rows += response_missing;
        const std::string response_name = column_label(table, response_column);
        const auto fit = datalab::domain::statistics::fit_response_analysis(
            imported.design, responses, response_name);
        fits.push_back(fit);
        models.push_back(build_response_model(
            fit, imported.design, configuration.doe.optimization_confidence));
        if (models.back().coefficient_covariance.empty()) {
            exact_prediction_intervals_available = false;
        }
        objectives.push_back(build_response_objective(
            models.back(), configuration, index, responses));
        response_names.push_back(models.back().response_name);
        if (!fit.model_anova_rows.empty()) {
            doe_facts.has_p_value = true;
        }
        const std::vector<std::string> terms = significant_doe_terms(fit, alpha);
        doe_facts.significant_terms.insert(
            doe_facts.significant_terms.end(), terms.cbegin(), terms.cend());
    }
    std::sort(doe_facts.significant_terms.begin(), doe_facts.significant_terms.end());
    doe_facts.significant_terms.erase(
        std::unique(doe_facts.significant_terms.begin(),
                    doe_facts.significant_terms.end()),
        doe_facts.significant_terms.end());
    const auto optimized = datalab::domain::statistics::optimize_response_desirability(
        models, objectives);
    OutputPage page;
    page.id = new_id("response_optimization");
    page.title = "DOE 响应优化";
    page.method_name = "Response Optimization";
    page.configuration = configuration;
    std::string response_summary;
    for (std::size_t index = 0; index < response_names.size(); ++index) {
        if (index != 0) {
            response_summary += ", ";
        }
        response_summary += response_names[index];
    }
    page.parameter_summary = "响应 = " + response_summary
        + "    有效运行数 = " + std::to_string(imported.design.runs.size());
    for (const auto& fit : fits) {
        page.diagnostics.insert(page.diagnostics.end(),
                                fit.diagnostics.cbegin(), fit.diagnostics.cend());
    }
    page.diagnostics.insert(page.diagnostics.end(),
                            optimized.diagnostics.cbegin(), optimized.diagnostics.cend());
    if (!exact_prediction_intervals_available) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "response_optimizer_covariance_unavailable",
            "缺少回归系数协方差矩阵，响应优化表中的置信区间与预测区间显示为 *。"});
    }
    if (imported.skipped_level_rows > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "invalid_doe_factor_levels",
            "跳过 " + std::to_string(imported.skipped_level_rows)
                + " 行：因子水平既不匹配 -1/+1，也不匹配给定的低/高水平。"});
    }
    if (skipped_response_rows > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_doe_response",
            "跳过 " + std::to_string(skipped_response_rows)
                + " 个缺失或非法响应值。"});
    }
    auto actual_level = [&](std::size_t factor, int coded) {
        if (factor < configuration.doe.low_levels.size() && coded < 0) {
            return configuration.doe.low_levels[factor];
        }
        if (factor < configuration.doe.high_levels.size() && coded > 0) {
            return configuration.doe.high_levels[factor];
        }
        return std::to_string(coded);
    };
    const bool multi_response = models.size() > 1;
    if (multi_response) {
        StatisticTable objectives_table;
        objectives_table.title = "响应目标";
        objectives_table.headers = {"响应", "目标", "下限", "上限", "目标值", "权重"};
        for (std::size_t index = 0; index < objectives.size(); ++index) {
            const auto& objective = objectives[index];
            objectives_table.rows.push_back({
                objective.response_name,
                configuration.doe.optimization_objectives.size() > index
                    ? configuration.doe.optimization_objectives[index].goal
                    : configuration.doe.optimization_goal,
                format_number(objective.lower),
                format_number(objective.upper),
                objective.goal == datalab::domain::statistics::ResponseGoal::target
                    ? format_number(objective.target) : "*",
                format_number(objective.weight)});
        }
        page.tables.push_back(std::move(objectives_table));
    }
    StatisticTable best_table;
    best_table.title = "最佳组合";
    if (multi_response) {
        best_table.headers = {"因子", "编码水平", "实际水平"};
        for (const std::string& name : response_names) {
            best_table.headers.push_back(name + " 预测");
            best_table.headers.push_back(name + " D");
        }
        best_table.headers.push_back("总体 D");
    } else {
        best_table.headers = {"因子", "编码水平", "实际水平", "最佳预测", "单响应 D", "总体 D"};
    }
    if (optimized.best_candidate.has_value()) {
        const auto& best = *optimized.best_candidate;
        doe_facts.best_overall_desirability = best.overall_desirability;
        for (std::size_t factor = 0; factor < best.coded_levels.size(); ++factor) {
            std::vector<std::string> row = {
                factor < models.front().factor_names.size()
                    ? models.front().factor_names[factor] : "*",
                std::to_string(best.coded_levels[factor]),
                actual_level(factor, best.coded_levels[factor])};
            if (multi_response) {
                if (factor == 0) {
                    for (std::size_t response = 0; response < best.predictions.size(); ++response) {
                        row.push_back(format_number(best.predictions[response].predicted_value));
                        row.push_back(response < best.desirabilities.size()
                                          ? format_number(best.desirabilities[response]) : "*");
                    }
                    row.push_back(format_number(best.overall_desirability));
                }
            } else {
                row.push_back(best.predictions.empty()
                                  ? "*" : format_number(best.predictions.front().predicted_value));
                row.push_back(best.desirabilities.empty()
                                  ? "*" : format_number(best.desirabilities.front()));
                row.push_back(format_number(best.overall_desirability));
                if (factor != 0) {
                    row[3] = "";
                    row[4] = "";
                    row[5] = "";
                }
            }
            best_table.rows.push_back(std::move(row));
        }
    }
    page.tables.push_back(best_table);
    std::vector<datalab::domain::statistics::OptimizationCandidate> ranked_candidates =
        optimized.candidates;
    std::sort(
        ranked_candidates.begin(),
        ranked_candidates.end(),
        [](const datalab::domain::statistics::OptimizationCandidate& left,
           const datalab::domain::statistics::OptimizationCandidate& right) {
            if (left.overall_desirability != right.overall_desirability) {
                return left.overall_desirability > right.overall_desirability;
            }
            const double left_prediction = left.predictions.empty()
                ? -std::numeric_limits<double>::infinity()
                : left.predictions.front().predicted_value;
            const double right_prediction = right.predictions.empty()
                ? -std::numeric_limits<double>::infinity()
                : right.predictions.front().predicted_value;
            return left_prediction > right_prediction;
        });
    StatisticTable candidates;
    candidates.title = "候选组合";
    if (multi_response) {
        candidates.headers = {"排序", "组合", "实际水平"};
        for (const std::string& name : response_names) {
            candidates.headers.push_back(name + " 预测");
            candidates.headers.push_back(name + " D");
        }
        candidates.headers.push_back("总体 D");
    } else {
        candidates.headers = {"排序", "组合", "实际水平", "预测值", "单响应 D", "总体 D"};
    }
    for (std::size_t index = 0; index < ranked_candidates.size(); ++index) {
        const auto& candidate = ranked_candidates[index];
        std::string combination;
        std::string actual_combination;
        for (std::size_t factor = 0; factor < candidate.coded_levels.size(); ++factor) {
            if (factor > 0) {
                combination += ", ";
                actual_combination += ", ";
            }
            combination += (factor < models.front().factor_names.size()
                                ? models.front().factor_names[factor] : "X")
                + "=" + std::to_string(candidate.coded_levels[factor]);
            actual_combination += (factor < models.front().factor_names.size()
                                       ? models.front().factor_names[factor] : "X")
                + "=" + actual_level(factor, candidate.coded_levels[factor]);
        }
        for (const auto& prediction : candidate.predictions) {
            append_diagnostics(page.diagnostics, prediction.diagnostics, "响应优化预测：");
        }
        std::vector<std::string> row = {
            std::to_string(index + 1), combination, actual_combination};
        if (multi_response) {
            for (std::size_t response = 0; response < candidate.predictions.size(); ++response) {
                row.push_back(format_number(candidate.predictions[response].predicted_value));
                row.push_back(response < candidate.desirabilities.size()
                                  ? format_number(candidate.desirabilities[response]) : "*");
            }
            row.push_back(format_number(candidate.overall_desirability));
        } else {
            row.push_back(candidate.predictions.empty()
                              ? "*" : format_number(candidate.predictions.front().predicted_value));
            row.push_back(candidate.desirabilities.empty()
                              ? "*" : format_number(candidate.desirabilities.front()));
            row.push_back(format_number(candidate.overall_desirability));
        }
        candidates.rows.push_back(std::move(row));
    }
    page.tables.push_back(candidates);
    StatisticTable prediction;
    prediction.title = "响应预测";
    if (multi_response) {
        prediction.headers = {"组合", "实际水平", "响应", "预测", "置信下限", "置信上限",
                              "预测下限", "预测上限"};
        for (const auto& candidate : ranked_candidates) {
            std::string combination;
            std::string actual_combination;
            for (std::size_t factor = 0; factor < candidate.coded_levels.size(); ++factor) {
                if (factor > 0) {
                    combination += " ";
                    actual_combination += ", ";
                }
                combination += std::to_string(candidate.coded_levels[factor]);
                actual_combination += (factor < models.front().factor_names.size()
                                           ? models.front().factor_names[factor] : "X")
                    + "=" + actual_level(factor, candidate.coded_levels[factor]);
            }
            for (std::size_t response = 0; response < candidate.predictions.size(); ++response) {
                const auto& item = candidate.predictions[response];
                std::string ci_low = "*";
                std::string ci_high = "*";
                std::string pi_low = "*";
                std::string pi_high = "*";
                if (item.interval.has_value()) {
                    ci_low = format_number(item.interval->confidence_lower);
                    ci_high = format_number(item.interval->confidence_upper);
                    pi_low = format_number(item.interval->prediction_lower);
                    pi_high = format_number(item.interval->prediction_upper);
                }
                prediction.rows.push_back({
                    combination, actual_combination, item.response_name,
                    format_number(item.predicted_value), ci_low, ci_high, pi_low, pi_high});
            }
        }
    } else {
        prediction.headers = {
            "组合", "实际水平", "预测", "置信下限", "置信上限", "预测下限", "预测上限"};
        for (const auto& candidate : ranked_candidates) {
            std::string combination;
            std::string actual_combination;
            for (std::size_t factor = 0; factor < candidate.coded_levels.size(); ++factor) {
                if (factor > 0) {
                    combination += " ";
                    actual_combination += ", ";
                }
                combination += std::to_string(candidate.coded_levels[factor]);
                actual_combination += (factor < models.front().factor_names.size()
                                           ? models.front().factor_names[factor] : "X")
                    + "=" + actual_level(factor, candidate.coded_levels[factor]);
            }
            std::string ci_low = "*";
            std::string ci_high = "*";
            std::string pi_low = "*";
            std::string pi_high = "*";
            std::string predicted = "*";
            if (!candidate.predictions.empty()) {
                predicted = format_number(candidate.predictions.front().predicted_value);
                if (candidate.predictions.front().interval.has_value()) {
                    const auto& interval = *candidate.predictions.front().interval;
                    ci_low = format_number(interval.confidence_lower);
                    ci_high = format_number(interval.confidence_upper);
                    pi_low = format_number(interval.prediction_lower);
                    pi_high = format_number(interval.prediction_upper);
                }
            }
            prediction.rows.push_back({
                combination, actual_combination, predicted, ci_low, ci_high, pi_low, pi_high});
        }
    }
    page.tables.push_back(prediction);
    if (!ranked_candidates.empty()) {
        PlotSpec desirability_plot;
        desirability_plot.kind = PlotKind::scatter;
        desirability_plot.title = "候选组合总体 Desirability";
        desirability_plot.x_axis_title = "候选排序";
        desirability_plot.y_axis_title = "Overall D";
        desirability_plot.value_style.point_style = PlotPointStyle::circle;
        desirability_plot.value_style.line_width = 0.0;
        for (std::size_t index = 0; index < ranked_candidates.size(); ++index) {
            const auto& candidate = ranked_candidates[index];
            desirability_plot.x_values.push_back(static_cast<double>(index + 1));
            desirability_plot.values.push_back(candidate.overall_desirability);
            std::string label;
            for (std::size_t factor = 0; factor < candidate.coded_levels.size(); ++factor) {
                if (factor > 0) {
                    label += ", ";
                }
                label += (factor < models.front().factor_names.size()
                              ? models.front().factor_names[factor] : "X")
                    + "=" + actual_level(factor, candidate.coded_levels[factor]);
            }
            desirability_plot.point_labels.push_back(label);
        }
        page.plots.push_back(std::move(desirability_plot));
    }
    doe_facts.response_names = response_names;
    doe_facts.prediction_interval_available = exact_prediction_intervals_available;
    page.facts.doe = doe_facts;
    page.method_metadata.valid_count = fits.empty() ? 0 : fits.front().residuals.size();
    page.method_metadata.missing_count = skipped_response_rows;
    page.method_metadata.parameter_source = "imported_doe_design";
    page.method_metadata.estimation_method = multi_response
        ? "coded_2_level_multi_response_desirability"
        : "coded_2_level_desirability";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::msa_type1(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.msa.gage_measurement_column.has_value()) {
        return error_page("MSA Type 1", "MSA Type 1",
                          "请选择测量值列，并在配置中提供参考值。");
    }
    const auto measurements = extract_numeric_column(
        table, *configuration.msa.gage_measurement_column, configuration.excluded_rows);
    if (configuration.msa.mode == "stability") {
        const auto result = datalab::domain::statistics::gage_stability(measurements.values);
        OutputPage page;
        page.id = new_id("msa_stability");
        page.title = "Gage Stability";
        page.method_name = "Stability / Gage Run Chart";
        page.configuration = configuration;
        page.parameter_summary = "测量值 = " + measurements.name;
        page.diagnostics = result.diagnostics;
        StatisticTable summary;
        summary.title = "Stability 统计";
        summary.headers = {"Center", "Sigma", "LCL", "UCL", "Out of Control"};
        summary.rows.push_back({format_number(result.center), format_number(result.sigma),
                                format_number(result.lower_control_limit),
                                format_number(result.upper_control_limit),
                                std::to_string(result.out_of_control.size())});
        page.tables.push_back(std::move(summary));
        PlotSpec plot;
        plot.kind = PlotKind::control;
        plot.title = "Gage Stability Run Chart";
        plot.x_axis_title = "观测序号";
        plot.y_axis_title = "测量值";
        plot.values = result.values;
        plot.center.assign(result.values.size(), result.center);
        plot.lower.assign(result.values.size(), result.lower_control_limit);
        plot.upper.assign(result.values.size(), result.upper_control_limit);
        plot.source_rows = measurements.source_rows;
        plot.triggered_tests = result.triggered_tests;
        plot.primary_test_by_point = result.primary_test_by_point;
        page.plots.push_back(std::move(plot));
        page.facts.msa = datalab::domain::statistics::stability_facts_from(result);
        append_rule_table(page, page.facts.msa->rules);
        return finalize_page(std::move(page));
    }
    if (configuration.msa.mode == "bias_linearity") {
        if (!configuration.msa.reference_column.has_value()) {
            return error_page("Bias/Linearity", "Bias/Linearity", "请选择参考值列。");
        }
        const auto references = extract_numeric_column(
            table, *configuration.msa.reference_column, configuration.excluded_rows);
        const auto aligned = align_complete_rows_with_source({references, measurements});
        if (aligned.values.empty()) {
            return error_page("Bias/Linearity", "Bias/Linearity",
                              "没有可用于 Bias/Linearity 的 complete-case 行。");
        }
        std::vector<double> aligned_references;
        std::vector<double> aligned_measurements;
        aligned_references.reserve(aligned.values.size());
        aligned_measurements.reserve(aligned.values.size());
        for (const auto& row : aligned.values) {
            aligned_references.push_back(row[0]);
            aligned_measurements.push_back(row[1]);
        }
        const auto result = datalab::domain::statistics::bias_linearity(
            aligned_references, aligned_measurements,
            configuration.inference.confidence_level, aligned.source_rows,
            configuration.msa.process_variation);
        OutputPage page;
        page.id = new_id("msa_bias_linearity");
        page.title = "Bias/Linearity";
        page.method_name = "MSA Bias and Linearity";
        page.configuration = configuration;
        page.parameter_summary = "测量值 = " + measurements.name
            + "    参考值 = " + references.name;
        if (configuration.msa.process_variation.has_value()) {
            page.parameter_summary += "    过程变差(6σ) = "
                + format_number(*configuration.msa.process_variation);
        }
        page.diagnostics = result.diagnostics;
        const std::size_t eligible = table.rows.size() >= configuration.excluded_rows.size()
            ? table.rows.size() - configuration.excluded_rows.size() : table.rows.size();
        if (eligible > aligned.values.size()) {
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::warning, "missing_values",
                "Bias/Linearity 跳过 "
                    + std::to_string(eligible - aligned.values.size())
                    + " 个缺失或不完整观测。"});
        }
        if (!configuration.msa.process_variation.has_value()) {
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::info, "process_variation_not_provided",
                "未提供过程变差（6×过程标准差），Linearity / %Linearity / %Bias 未计算。"});
        }
        StatisticTable coef;
        coef.title = "Coef";
        coef.headers = {"Term", "Coef", "SE Coef", "T", "P"};
        const auto format_t = [&](std::optional<double> t) {
            return t.has_value() ? format_number(*t) : "*";
        };
        const auto format_p = [&](std::optional<double> p) {
            return p.has_value() ? format_number(*p) : "*";
        };
        const std::optional<double> intercept_t =
            result.intercept_standard_error.has_value()
            && *result.intercept_standard_error > 0.0
                ? std::optional<double>(
                    result.intercept / *result.intercept_standard_error)
                : std::nullopt;
        const std::optional<double> slope_t =
            result.slope_standard_error > 0.0
                ? std::optional<double>(result.slope / result.slope_standard_error)
                : std::nullopt;
        coef.rows.push_back({
            "Constant", format_number(result.intercept),
            format_optional(result.intercept_standard_error),
            format_t(intercept_t), format_p(result.intercept_p_value)});
        coef.rows.push_back({
            "Slope", format_number(result.slope),
            format_number(result.slope_standard_error),
            format_t(slope_t), format_p(result.slope_p_value)});
        page.tables.push_back(std::move(coef));
        StatisticTable fit_summary;
        fit_summary.title = "S and R-Sq";
        fit_summary.headers = {"S", "R-Sq"};
        fit_summary.rows.push_back({
            format_optional(result.residual_s), format_number(result.r_squared)});
        page.tables.push_back(std::move(fit_summary));
        if (result.process_variation_used.has_value()) {
            StatisticTable linearity_table;
            linearity_table.title = "Gage Linearity";
            linearity_table.headers = {"Linearity", "%Linearity", "P Constant", "P Slope"};
            linearity_table.rows.push_back({
                format_number(*result.linearity),
                format_number(*result.percent_linearity),
                format_p(result.intercept_p_value),
                format_p(result.slope_p_value)});
            page.tables.push_back(std::move(linearity_table));
        }
        StatisticTable gage_bias;
        gage_bias.title = "Gage Bias";
        gage_bias.headers = {"Reference", "N", "Bias", "SE Bias", "%Bias", "t", "P"};
        for (const auto& level : result.levels) {
            gage_bias.rows.push_back({
                format_number(level.reference),
                std::to_string(level.valid_count),
                format_number(level.bias),
                format_optional(level.standard_error),
                level.percent_bias.has_value()
                    ? format_number(*level.percent_bias) : "*",
                format_t(level.t_statistic),
                format_p(level.p_value)});
        }
        const std::optional<double> average_bias_se =
            result.average_bias_t.has_value() && *result.average_bias_t != 0.0
                ? std::optional<double>(std::abs(result.average_bias / *result.average_bias_t))
                : std::nullopt;
        std::size_t average_count = 0;
        for (const auto& level : result.levels) {
            average_count += level.valid_count;
        }
        gage_bias.rows.push_back({
            "Average",
            std::to_string(average_count),
            format_number(result.average_bias),
            format_optional(average_bias_se),
            result.process_variation_used.has_value()
                ? format_number(result.average_bias / *result.process_variation_used * 100.0)
                : "*",
            format_t(result.average_bias_t),
            format_p(result.average_bias_p)});
        page.tables.push_back(std::move(gage_bias));
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "Bias versus Reference";
        plot.x_axis_title = "参考值";
        plot.y_axis_title = "Bias";
        plot.show_legend = true;
        PlotSeries actual;
        actual.role = PlotSeriesRole::actual;
        actual.label = "观测偏倚";
        actual.show_points = true;
        actual.x_values = aligned_references;
        actual.values.reserve(aligned_references.size());
        for (std::size_t i = 0; i < aligned_references.size(); ++i) {
            actual.values.push_back(aligned_measurements[i] - aligned_references[i]);
        }
        plot.x_values = actual.x_values;
        plot.values = actual.values;
        plot.source_rows = aligned.source_rows;
        PlotSeries fitted;
        fitted.role = PlotSeriesRole::fitted;
        fitted.label = "拟合线";
        PlotSeries ci;
        ci.role = PlotSeriesRole::confidence_band;
        const int percent = static_cast<int>(
            std::lround(configuration.inference.confidence_level * 100.0));
        ci.label = std::to_string(percent) + "% CI";
        for (const auto& point : result.mean_band) {
            fitted.x_values.push_back(point.x);
            fitted.values.push_back(point.fitted);
            ci.x_values.push_back(point.x);
            ci.lower.push_back(point.ci_lower);
            ci.upper.push_back(point.ci_upper);
        }
        plot.series = {std::move(actual), std::move(fitted), std::move(ci)};
        page.plots.push_back(std::move(plot));
        page.facts.msa = datalab::domain::statistics::bias_linearity_facts_from(result);
        append_rule_table(page, page.facts.msa->rules);
        return finalize_page(std::move(page));
    }
    if (!configuration.msa.reference_value.has_value()) {
        return error_page("MSA Type 1", "MSA Type 1", "请输入参考值。");
    }
    const auto result = datalab::domain::statistics::msa_type1(
        measurements.values, *configuration.msa.reference_value,
        configuration.msa.gage_tolerance, configuration.inference.confidence_level);
    OutputPage page;
    page.id = new_id("msa_type1");
    page.title = "MSA Type 1 Gage";
    page.method_name = "MSA Type 1 Gage + Bias";
    page.configuration = configuration;
    page.parameter_summary = "测量值 = " + measurements.name
        + "    参考值 = " + format_number(*configuration.msa.reference_value);
    page.diagnostics = result.diagnostics;
    if (measurements.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "Type 1 Gage 跳过 " + std::to_string(measurements.missing_count)
                + " 个缺失或非法观测。"});
    }
    StatisticTable table_result;
    table_result.title = "Type 1 Gage 结果";
    table_result.headers = {
        "N", "Mean", "StdDev", "Bias", "SE Bias", "T", "DF", "P",
        "Bias CI Low", "Bias CI High", "Cg", "Cgk", "%Tolerance"};
    table_result.rows.push_back({
        std::to_string(result.count), format_number(result.mean),
        format_number(result.standard_deviation), format_number(result.bias),
        format_number(result.bias_standard_error), format_number(result.t_statistic),
        format_number(result.degrees_of_freedom), format_number(result.p_value),
        format_number(result.bias_ci_lower), format_number(result.bias_ci_upper),
        format_number(result.cg), format_number(result.cgk),
        format_number(result.percent_tolerance)});
    page.tables.push_back(std::move(table_result));
    const double reference = *configuration.msa.reference_value;
    const auto bins = datalab::domain::statistics::histogram(measurements.values, 0);
    PlotSpec histogram;
    histogram.kind = PlotKind::histogram;
    histogram.title = "Type 1 Gage 直方图";
    histogram.x_axis_title = measurements.name;
    histogram.y_axis_title = "频数";
    histogram.center_label = "Ref";
    histogram.histogram_edges = bins.edges;
    histogram.histogram_counts = bins.counts;
    histogram.values = measurements.values;
    histogram.source_rows = measurements.source_rows;
    histogram.target = reference;
    if (configuration.specifications.lower.has_value()
        || configuration.specifications.upper.has_value()) {
        histogram.lsl = configuration.specifications.lower;
        histogram.usl = configuration.specifications.upper;
    } else if (configuration.msa.gage_tolerance > 0.0) {
        const double half = configuration.msa.gage_tolerance / 2.0;
        histogram.lsl = reference - half;
        histogram.usl = reference + half;
    }
    page.plots.push_back(std::move(histogram));
    PlotSpec plot;
    plot.kind = PlotKind::control;
    plot.title = "Gage Run Chart";
    plot.x_axis_title = "测量序号";
    plot.y_axis_title = "测量值";
    plot.center_label = "Reference";
    plot.values = measurements.values;
    plot.center.assign(plot.values.size(), reference);
    plot.source_rows = measurements.source_rows;
    page.plots.push_back(std::move(plot));
    page.facts.msa = datalab::domain::statistics::type1_facts_from(result);
    append_rule_table(page, page.facts.msa->rules);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::reliability(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.reliability.time_column.has_value()) {
        return error_page("Reliability", "Reliability", "请选择寿命列。");
    }
    if (!configuration.reliability.event_column.has_value()
        && !configuration.reliability.censoring_type_column.has_value()) {
        return error_page(
            "Reliability", "Reliability",
            "请选择失效/删失指示列，或逐行删失类型列（exact/right/left/interval）。");
    }
    const auto times = extract_numeric_column(
        table, *configuration.reliability.time_column, configuration.excluded_rows);
    const auto event_text = configuration.reliability.event_column.has_value()
        ? extract_text_column(table, *configuration.reliability.event_column)
        : std::vector<std::string>{};
    const auto censor_type_text =
        configuration.reliability.censoring_type_column.has_value()
            ? extract_text_column(table, *configuration.reliability.censoring_type_column)
            : std::vector<std::string>{};
    const auto failure_mode_text =
        configuration.reliability.failure_mode_column.has_value()
            ? extract_text_column(table, *configuration.reliability.failure_mode_column)
            : std::vector<std::string>{};
    std::vector<double> aligned_times;
    std::vector<bool> events;
    std::vector<int> aligned_groups;
    std::vector<std::string> group_levels;
    std::vector<std::size_t> aligned_source_rows;
    std::vector<std::size_t> invalid_event_rows;
    std::vector<std::size_t> invalid_censor_rows;
    std::vector<std::size_t> invalid_interval_bound_rows;
    std::vector<std::size_t> invalid_exposure_rows;
    std::vector<datalab::domain::statistics::CensoringObservation> typed_observations;
    typed_observations.reserve(times.source_rows.size());
    const auto group_text = configuration.reliability.group_column.has_value()
        ? extract_text_column(table, *configuration.reliability.group_column)
        : std::vector<std::string>{};
    const bool has_interval_bounds =
        configuration.reliability.interval_left_column.has_value()
        && configuration.reliability.interval_right_column.has_value();
    for (std::size_t index = 0; index < times.source_rows.size(); ++index) {
        const std::size_t row = times.source_rows[index];

        std::optional<datalab::domain::statistics::CensoringType> typed;
        if (!censor_type_text.empty()) {
            if (row >= censor_type_text.size() || is_missing_cell(censor_type_text[row])) {
                invalid_censor_rows.push_back(row);
                continue;
            }
            typed = datalab::domain::statistics::parse_censoring_type(censor_type_text[row]);
            if (!typed.has_value()) {
                invalid_censor_rows.push_back(row);
                continue;
            }
        }

        std::optional<bool> parsed_event;
        if (!event_text.empty()) {
            if (row >= event_text.size() || is_missing_cell(event_text[row])) {
                continue;
            }
            parsed_event = datalab::domain::statistics::parse_reliability_event(event_text[row]);
            if (!parsed_event.has_value()) {
                invalid_event_rows.push_back(row);
                continue;
            }
        } else if (typed.has_value()) {
            // Interval/left are not exact failures; right is censored.
            parsed_event = (*typed == datalab::domain::statistics::CensoringType::exact);
        }

        if (typed.has_value() && parsed_event.has_value()) {
            const bool typed_is_failure =
                (*typed == datalab::domain::statistics::CensoringType::exact);
            if (typed_is_failure != *parsed_event) {
                invalid_censor_rows.push_back(row);
                continue;
            }
        }

        datalab::domain::statistics::CensoringObservation observation;
        observation.type = typed.has_value()
            ? *typed
            : (*parsed_event ? datalab::domain::statistics::CensoringType::exact
                             : datalab::domain::statistics::CensoringType::right);
        observation.time = times.values[index];
        observation.time_unit = configuration.reliability.time_unit;
        observation.source_row = row;
        if (!failure_mode_text.empty() && row < failure_mode_text.size()
            && !is_missing_cell(failure_mode_text[row])) {
            observation.failure_mode = failure_mode_text[row];
        }
        if (observation.type == datalab::domain::statistics::CensoringType::interval) {
            if (!has_interval_bounds) {
                invalid_interval_bound_rows.push_back(row);
                continue;
            }
            const std::size_t left_col = *configuration.reliability.interval_left_column;
            const std::size_t right_col = *configuration.reliability.interval_right_column;
            if (row >= table.rows.size()
                || left_col >= table.rows[row].size()
                || right_col >= table.rows[row].size()
                || is_missing_cell(table.rows[row][left_col])
                || is_missing_cell(table.rows[row][right_col])) {
                invalid_interval_bound_rows.push_back(row);
                continue;
            }
            double left = 0.0;
            double right = 0.0;
            if (!datalab::domain::parse_finite_number(table.rows[row][left_col], left)
                || !datalab::domain::parse_finite_number(table.rows[row][right_col], right)) {
                invalid_interval_bound_rows.push_back(row);
                continue;
            }
            observation.interval_left = left;
            observation.interval_right = right;
            // Keep time as midpoint for inventory only; classic KM still blocked.
            observation.time = 0.5 * (left + right);
        }
        if (configuration.reliability.exposure_column.has_value()) {
            const std::size_t exposure_col = *configuration.reliability.exposure_column;
            if (row >= table.rows.size()
                || exposure_col >= table.rows[row].size()
                || is_missing_cell(table.rows[row][exposure_col])) {
                invalid_exposure_rows.push_back(row);
                continue;
            }
            double exposure_value = 0.0;
            if (!datalab::domain::parse_finite_number(
                    table.rows[row][exposure_col], exposure_value)
                || !std::isfinite(exposure_value) || exposure_value < 0.0) {
                invalid_exposure_rows.push_back(row);
                continue;
            }
            observation.exposure = exposure_value;
        }

        if (!group_text.empty()) {
            if (row >= group_text.size()) {
                continue;
            }
            const std::string& label = group_text[row];
            if (is_missing_cell(label)) {
                continue;
            }
            auto level = std::find(group_levels.begin(), group_levels.end(), label);
            if (level == group_levels.end()) {
                group_levels.push_back(label);
                level = group_levels.end() - 1;
            }
            observation.group = label;
            aligned_groups.push_back(
                static_cast<int>(std::distance(group_levels.begin(), level)));
        }

        typed_observations.push_back(observation);

        aligned_times.push_back(observation.time);
        events.push_back(*parsed_event);
        aligned_source_rows.push_back(row);
    }
    if (!invalid_event_rows.empty()) {
        OutputPage page = error_page(
            "Reliability", "Reliability",
            "事件列只接受明确的失效/删失编码；未知值不会被静默当作删失。");
        page.diagnostics.front().code = "invalid_event_value";
        page.diagnostics.front().related_rows.clear();
        for (const std::size_t row : invalid_event_rows) {
            page.diagnostics.front().related_rows.push_back(
                static_cast<datalab::domain::RowId>(row));
        }
        return page;
    }
    if (!invalid_censor_rows.empty()) {
        OutputPage page = error_page(
            "Reliability", "Reliability",
            "删失类型列无法解析，或与事件列冲突；未知值不会被静默改写。"
            "left/interval 若出现在数据中，将由删失契约拒绝经典 KM 路径。");
        page.diagnostics.front().code = "invalid_censoring_type_value";
        page.diagnostics.front().related_rows.clear();
        for (const std::size_t row : invalid_censor_rows) {
            page.diagnostics.front().related_rows.push_back(
                static_cast<datalab::domain::RowId>(row));
        }
        return page;
    }
    if (!invalid_interval_bound_rows.empty()) {
        OutputPage page = error_page(
            "Reliability", "Reliability",
            "区间删失行需要有效的左右界列（interval_left < interval_right）；"
            "缺失或非有限值不会被静默补齐。");
        page.configuration = configuration;
        page.diagnostics.front().code = "missing_interval_bounds";
        page.diagnostics.front().related_rows.clear();
        for (const std::size_t row : invalid_interval_bound_rows) {
            page.diagnostics.front().related_rows.push_back(
                static_cast<datalab::domain::RowId>(row));
        }
        return page;
    }
    if (!invalid_exposure_rows.empty()) {
        OutputPage page = error_page(
            "Reliability", "Reliability",
            "暴露量列必须为有限非负数；缺失或非法值不会被静默当作 0 或 1。");
        page.configuration = configuration;
        page.diagnostics.front().code = "invalid_exposure_value";
        page.diagnostics.front().related_rows.clear();
        for (const std::size_t row : invalid_exposure_rows) {
            page.diagnostics.front().related_rows.push_back(
                static_cast<datalab::domain::RowId>(row));
        }
        return page;
    }

    // Phase 5: run censoring contract (typed column or event-derived exact/right).
    {
        const auto contract =
            datalab::domain::statistics::validate_censoring_contract(typed_observations);
        if (!contract.ok) {
            auto page = error_page(
                "Reliability", "Reliability",
                contract.diagnostics.empty() ? "删失契约校验失败。"
                                             : contract.diagnostics.front().message);
            page.configuration = configuration;
            page.diagnostics = contract.diagnostics;
            return finalize_page(std::move(page));
        }
        // Prefer contract-normalized vectors (right-censored never treated as failures).
        aligned_times = contract.times_for_right_censored_km;
        events = contract.events_for_right_censored_km;
        aligned_source_rows = contract.source_rows_for_km;
    }

    const std::string model = configuration.reliability.model;
    OutputPage page;
    page.id = new_id("reliability");
    page.title = "可靠性分析";
    page.method_name = model == "weibull" ? "Weibull Lifetime"
        : model == "weibull3" ? "3-Parameter Weibull Lifetime"
        : model == "exponential" ? "Exponential Lifetime"
        : model == "exponential2" ? "2-Parameter Exponential Lifetime"
        : model == "lognormal" ? "Lognormal Lifetime"
        : model == "lognormal3" ? "3-Parameter Lognormal Lifetime" : "Kaplan-Meier";
    page.configuration = configuration;
    page.parameter_summary = "寿命列 = " + times.name
        + "    模型 = " + model;
    if (model == "weibull" || model == "weibull3") {
        const auto result = model == "weibull3"
            ? datalab::domain::statistics::fit_weibull3(aligned_times, events)
            : datalab::domain::statistics::fit_weibull(aligned_times, events);
        page.diagnostics = result.diagnostics;
        if (model == "weibull") {
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::info,
                "two_param_weibull",
                "当前为二参数 Weibull。含阈值的三参数模型请选择 model=weibull3。"});
        }
        StatisticTable summary;
        summary.title = model == "weibull3" ? "三参数 Weibull 参数" : "Weibull 参数";
        if (model == "weibull3") {
            summary.headers = {
                "Shape", "Scale", "Threshold", "B10", "B50", "B90", "LogLik",
                "AIC", "BIC", "Failures", "Observations", "Censoring", "Converged"};
            summary.rows.push_back({
                result.identifiable ? format_number(result.shape) : "*",
                result.identifiable ? format_number(result.scale) : "*",
                result.identifiable && result.threshold.has_value()
                    ? format_number(*result.threshold) : "*",
                result.b10.has_value() ? format_number(*result.b10) : "*",
                result.b50.has_value() ? format_number(*result.b50) : "*",
                result.b90.has_value() ? format_number(*result.b90) : "*",
                result.identifiable ? format_number(result.log_likelihood) : "*",
                result.identifiable ? format_number(result.aic) : "*",
                result.identifiable ? format_number(result.bic) : "*",
                std::to_string(result.failures),
                std::to_string(result.observations),
                format_number(result.censoring_fraction),
                result.converged ? "是" : "否"});
        } else {
            summary.headers = {
                "Shape", "Scale", "B10", "B50", "B90", "LogLik", "AIC", "BIC",
                "Failures", "Observations", "Censoring", "Converged"};
            summary.rows.push_back({format_number(result.shape), format_number(result.scale),
                                    result.b10.has_value() ? format_number(*result.b10) : "*",
                                    result.b50.has_value() ? format_number(*result.b50) : "*",
                                    result.b90.has_value() ? format_number(*result.b90) : "*",
                                    format_number(result.log_likelihood),
                                    format_number(result.aic), format_number(result.bic),
                                    std::to_string(result.failures),
                                    std::to_string(result.observations),
                                    format_number(result.censoring_fraction),
                                    result.converged ? "是" : "否"});
        }
        page.tables.push_back(std::move(summary));
        StatisticTable percentiles;
        percentiles.title = "百分位寿命";
        percentiles.headers = {"Percentile", "Life"};
        const std::vector<double> levels =
            configuration.reliability.percentile_levels.empty()
                ? std::vector<double>{10.0, 50.0, 90.0}
                : configuration.reliability.percentile_levels;
        if (result.identifiable && result.converged) {
            for (const double level : levels) {
                const double life = model == "weibull3" && result.threshold.has_value()
                    ? datalab::domain::statistics::percentile_life_weibull3(
                          result.shape, result.scale, *result.threshold, level)
                    : datalab::domain::statistics::percentile_life_weibull(
                          result.shape, result.scale, level);
                percentiles.rows.push_back({format_number(level) + "%", format_number(life)});
            }
        }
        page.tables.push_back(std::move(percentiles));
        page.facts.reliability = datalab::domain::statistics::weibull_facts_from(result);
        append_rule_table(page, page.facts.reliability->rules);
        domain::apply_evidence(page.method_metadata, result.evidence);
        if (model == "weibull3") {
            const double shape = result.shape;
            const double scale = result.scale;
            const double threshold = result.threshold.value_or(0.0);
            append_parametric_reliability_plots(
                page, aligned_times, events, aligned_source_rows,
                [shape, scale, threshold](double time) {
                    return datalab::domain::statistics::cdf_weibull3(
                        time, shape, scale, threshold);
                },
                result.identifiable, result.converged, "三参数 Weibull");
        }
    } else if (model == "exponential" || model == "exponential2") {
        const auto result = model == "exponential2"
            ? datalab::domain::statistics::fit_exponential2(aligned_times, events)
            : datalab::domain::statistics::fit_exponential(aligned_times, events);
        page.diagnostics = result.diagnostics;
        if (model == "exponential") {
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::info,
                "one_param_exponential",
                "当前为一参数指数。含阈值的两参数模型请选择 model=exponential2。"});
        }
        StatisticTable summary;
        summary.title = model == "exponential2" ? "两参数指数参数" : "Exponential 参数";
        if (model == "exponential2") {
            summary.headers = {
                "Scale", "Threshold", "Mean Life", "B10", "B50", "B90", "LogLik",
                "AIC", "BIC", "Failures", "Observations"};
            summary.rows.push_back({
                result.identifiable ? format_number(1.0 / result.rate) : "*",
                result.identifiable && result.threshold.has_value()
                    ? format_number(*result.threshold) : "*",
                result.identifiable ? format_number(result.mean_life) : "*",
                result.b10.has_value() ? format_number(*result.b10) : "*",
                result.b50.has_value() ? format_number(*result.b50) : "*",
                result.b90.has_value() ? format_number(*result.b90) : "*",
                result.identifiable ? format_number(result.log_likelihood) : "*",
                result.identifiable ? format_number(result.aic) : "*",
                result.identifiable ? format_number(result.bic) : "*",
                std::to_string(result.failures),
                std::to_string(result.observations)});
        } else {
            summary.headers = {
                "Rate", "Mean Life", "B10", "B50", "B90", "LogLik", "AIC", "BIC",
                "Failures", "Observations"};
            summary.rows.push_back({
                format_number(result.rate), format_number(result.mean_life),
                result.b10.has_value() ? format_number(*result.b10) : "*",
                result.b50.has_value() ? format_number(*result.b50) : "*",
                result.b90.has_value() ? format_number(*result.b90) : "*",
                format_number(result.log_likelihood), format_number(result.aic),
                format_number(result.bic), std::to_string(result.failures),
                std::to_string(result.observations)});
        }
        page.tables.push_back(std::move(summary));
        StatisticTable percentiles;
        percentiles.title = "百分位寿命";
        percentiles.headers = {"Percentile", "Life"};
        const std::vector<double> levels =
            configuration.reliability.percentile_levels.empty()
                ? std::vector<double>{10.0, 50.0, 90.0}
                : configuration.reliability.percentile_levels;
        if (result.identifiable) {
            for (const double level : levels) {
                const double life = model == "exponential2" && result.threshold.has_value()
                    ? datalab::domain::statistics::percentile_life_exponential2(
                          result.rate, *result.threshold, level)
                    : datalab::domain::statistics::percentile_life_exponential(
                          result.rate, level);
                percentiles.rows.push_back({format_number(level) + "%", format_number(life)});
            }
        }
        page.tables.push_back(std::move(percentiles));
        page.facts.reliability = datalab::domain::statistics::exponential_facts_from(result);
        append_rule_table(page, page.facts.reliability->rules);
        domain::apply_evidence(page.method_metadata, result.evidence);
        if (model == "exponential2") {
            const double rate = result.rate;
            const double threshold = result.threshold.value_or(0.0);
            append_parametric_reliability_plots(
                page, aligned_times, events, aligned_source_rows,
                [rate, threshold](double time) {
                    return datalab::domain::statistics::cdf_exponential2(
                        time, rate, threshold);
                },
                result.identifiable, result.converged, "两参数指数");
        }
    } else if (model == "lognormal" || model == "lognormal3") {
        const auto result = model == "lognormal3"
            ? datalab::domain::statistics::fit_lognormal3(aligned_times, events)
            : datalab::domain::statistics::fit_lognormal(aligned_times, events);
        page.diagnostics = result.diagnostics;
        if (model == "lognormal") {
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::info,
                "two_param_lognormal",
                "当前为二参数对数正态。含阈值的三参数模型请选择 model=lognormal3。"});
        }
        StatisticTable summary;
        summary.title = model == "lognormal3" ? "三参数对数正态参数" : "Lognormal 参数";
        if (model == "lognormal3") {
            summary.headers = {
                "Location", "Scale", "Threshold", "B10", "B50", "B90", "LogLik",
                "AIC", "BIC", "Failures", "Observations", "Censoring", "Converged"};
            summary.rows.push_back({
                result.identifiable ? format_number(result.location) : "*",
                result.identifiable ? format_number(result.scale) : "*",
                result.identifiable && result.threshold.has_value()
                    ? format_number(*result.threshold) : "*",
                result.b10.has_value() ? format_number(*result.b10) : "*",
                result.b50.has_value() ? format_number(*result.b50) : "*",
                result.b90.has_value() ? format_number(*result.b90) : "*",
                result.identifiable ? format_number(result.log_likelihood) : "*",
                result.identifiable ? format_number(result.aic) : "*",
                result.identifiable ? format_number(result.bic) : "*",
                std::to_string(result.failures),
                std::to_string(result.observations),
                format_number(result.censoring_fraction),
                result.converged ? "是" : "否"});
        } else {
            summary.headers = {
                "Location", "Scale", "B10", "B50", "B90", "LogLik", "AIC", "BIC",
                "Failures", "Observations", "Censoring", "Converged"};
            summary.rows.push_back({
                format_number(result.location), format_number(result.scale),
                result.b10.has_value() ? format_number(*result.b10) : "*",
                result.b50.has_value() ? format_number(*result.b50) : "*",
                result.b90.has_value() ? format_number(*result.b90) : "*",
                format_number(result.log_likelihood),
                format_number(result.aic), format_number(result.bic),
                std::to_string(result.failures),
                std::to_string(result.observations),
                format_number(result.censoring_fraction),
                result.converged ? "是" : "否"});
        }
        page.tables.push_back(std::move(summary));
        StatisticTable percentiles;
        percentiles.title = "百分位寿命";
        percentiles.headers = {"Percentile", "Life"};
        const std::vector<double> levels =
            configuration.reliability.percentile_levels.empty()
                ? std::vector<double>{10.0, 50.0, 90.0}
                : configuration.reliability.percentile_levels;
        if (result.identifiable && result.converged) {
            for (const double level : levels) {
                const double life = model == "lognormal3" && result.threshold.has_value()
                    ? datalab::domain::statistics::percentile_life_lognormal3(
                          result.location, result.scale, *result.threshold, level)
                    : datalab::domain::statistics::percentile_life_lognormal(
                          result.location, result.scale, level);
                percentiles.rows.push_back({format_number(level) + "%", format_number(life)});
            }
        }
        page.tables.push_back(std::move(percentiles));
        page.facts.reliability = datalab::domain::statistics::lognormal_facts_from(result);
        append_rule_table(page, page.facts.reliability->rules);
        domain::apply_evidence(page.method_metadata, result.evidence);
        if (model == "lognormal3") {
            const double location = result.location;
            const double scale = result.scale;
            const double threshold = result.threshold.value_or(0.0);
            append_parametric_reliability_plots(
                page, aligned_times, events, aligned_source_rows,
                [location, scale, threshold](double time) {
                    return datalab::domain::statistics::cdf_lognormal3(
                        time, location, scale, threshold);
                },
                result.identifiable, result.converged, "三参数对数正态");
        }
    } else {
        const auto result = datalab::domain::statistics::kaplan_meier(
            aligned_times, events, 0.95, aligned_source_rows);
        page.diagnostics = result.diagnostics;
        StatisticTable summary;
        summary.title = "Kaplan-Meier 生存表";
        summary.headers = {
            "Time", "At Risk", "Failures", "Censored", "Survival", "SE",
            "CI Lower", "CI Upper"};
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "Kaplan-Meier 生存曲线";
        plot.x_axis_title = "Time";
        plot.y_axis_title = "Survival";
        for (const auto& point : result.points) {
            summary.rows.push_back({format_number(point.time), std::to_string(point.at_risk),
                                    std::to_string(point.failures), std::to_string(point.censored),
                                    format_number(point.survival),
                                    format_number(point.standard_error),
                                    format_number(point.confidence_lower),
                                    format_number(point.confidence_upper)});
            plot.x_values.push_back(point.time);
            plot.values.push_back(point.survival);
            plot.lower.push_back(point.confidence_lower);
            plot.upper.push_back(point.confidence_upper);
            plot.source_rows.insert(plot.source_rows.end(),
                                    point.source_rows.begin(), point.source_rows.end());
        }
        page.tables.push_back(std::move(summary));
        StatisticTable percentiles;
        percentiles.title = "百分位寿命";
        percentiles.headers = {"Percentile", "Life"};
        const std::vector<double> levels =
            configuration.reliability.percentile_levels.empty()
                ? std::vector<double>{10.0, 50.0, 90.0}
                : configuration.reliability.percentile_levels;
        if (result.survival_identifiable) {
            for (const double level : levels) {
                const auto life = datalab::domain::statistics::percentile_life_km(
                    result.points, level);
                percentiles.rows.push_back({
                    format_number(level) + "%",
                    life.has_value() ? format_number(*life) : "*"});
            }
        }
        page.tables.push_back(std::move(percentiles));
        const bool has_log_rank_groups =
            aligned_groups.size() == aligned_times.size() && group_levels.size() >= 2;
        if (has_log_rank_groups) {
            PlotSpec group_plot;
            group_plot.kind = PlotKind::scatter;
            group_plot.title = "Kaplan-Meier 生存曲线（分组）";
            group_plot.x_axis_title = "Time";
            group_plot.y_axis_title = "Survival";
            group_plot.show_legend = true;
            for (std::size_t group_index = 0; group_index < group_levels.size(); ++group_index) {
                std::vector<double> group_times;
                std::vector<bool> group_events;
                std::vector<std::size_t> group_source_rows;
                group_times.reserve(aligned_times.size());
                group_events.reserve(aligned_times.size());
                group_source_rows.reserve(aligned_times.size());
                for (std::size_t index = 0; index < aligned_times.size(); ++index) {
                    if (aligned_groups[index] != static_cast<int>(group_index)) {
                        continue;
                    }
                    group_times.push_back(aligned_times[index]);
                    group_events.push_back(events[index]);
                    group_source_rows.push_back(aligned_source_rows[index]);
                }
                const auto group_km = datalab::domain::statistics::kaplan_meier(
                    group_times, group_events, 0.95, group_source_rows);
                PlotSeries series;
                series.label = group_levels[group_index];
                series.role = PlotSeriesRole::generic;
                for (const auto& point : group_km.points) {
                    series.x_values.push_back(point.time);
                    series.values.push_back(point.survival);
                    series.lower.push_back(point.confidence_lower);
                    series.upper.push_back(point.confidence_upper);
                }
                group_plot.series.push_back(std::move(series));
            }
            page.plots.push_back(std::move(group_plot));
        } else {
            page.plots.push_back(std::move(plot));
        }
        page.facts.reliability = datalab::domain::statistics::kaplan_meier_facts_from(result);
        append_rule_table(page, page.facts.reliability->rules);
        domain::apply_evidence(page.method_metadata, result.evidence);
        if (has_log_rank_groups) {
            if (group_levels.size() == 2) {
                const auto log_rank = datalab::domain::statistics::log_rank_test(
                    aligned_times, events, aligned_groups);
                page.diagnostics.insert(page.diagnostics.end(),
                                        log_rank.diagnostics.cbegin(),
                                        log_rank.diagnostics.cend());
                if (log_rank.diagnostics.empty()) {
                    StatisticTable equality;
                    equality.title = "Test for Equality of Survival Dist";
                    equality.headers = {"Chi-Square", "DF", "P-Value"};
                    equality.rows.push_back({
                        format_number(log_rank.chi_square),
                        format_number(log_rank.degrees_of_freedom),
                        format_number(log_rank.p_value)});
                    page.tables.push_back(std::move(equality));
                    StatisticTable group_counts;
                    group_counts.title = "Log-rank 分组样本";
                    group_counts.headers = {"Group", "At Risk", "Events", "Censored"};
                    group_counts.rows.push_back({
                        group_levels[0],
                        std::to_string(log_rank.group_one_n),
                        std::to_string(log_rank.group_one_failures),
                        std::to_string(log_rank.group_one_censored)});
                    group_counts.rows.push_back({
                        group_levels[1],
                        std::to_string(log_rank.group_two_n),
                        std::to_string(log_rank.group_two_failures),
                        std::to_string(log_rank.group_two_censored)});
                    page.tables.push_back(std::move(group_counts));
                    page.facts.reliability->log_rank_group_count = 2;
                    page.facts.reliability->log_rank_chi_square = log_rank.chi_square;
                    page.facts.reliability->log_rank_df = log_rank.degrees_of_freedom;
                    page.facts.reliability->log_rank_p_value = log_rank.p_value;
                    page.facts.reliability->log_rank_groups = {
                        {group_levels[0], 0, log_rank.group_one_n,
                         log_rank.group_one_failures, log_rank.group_one_censored},
                        {group_levels[1], 1, log_rank.group_two_n,
                         log_rank.group_two_failures, log_rank.group_two_censored}};
                }
            } else {
                const auto log_rank = datalab::domain::statistics::log_rank_k_groups(
                    aligned_times, events, aligned_groups);
                page.diagnostics.insert(page.diagnostics.end(),
                                        log_rank.diagnostics.cbegin(),
                                        log_rank.diagnostics.cend());
                if (log_rank.diagnostics.empty()) {
                    StatisticTable equality;
                    equality.title = "Test for Equality of Survival Dist";
                    equality.headers = {"Chi-Square", "DF", "P-Value"};
                    equality.rows.push_back({
                        format_number(log_rank.chi_square),
                        format_number(log_rank.df),
                        format_number(log_rank.p_value)});
                    page.tables.push_back(std::move(equality));
                    StatisticTable group_counts;
                    group_counts.title = "Log-rank 分组样本";
                    group_counts.headers = {"Group", "At Risk", "Events", "Censored"};
                    page.facts.reliability->log_rank_group_count = log_rank.group_summaries.size();
                    page.facts.reliability->log_rank_chi_square = log_rank.chi_square;
                    page.facts.reliability->log_rank_df = log_rank.df;
                    page.facts.reliability->log_rank_p_value = log_rank.p_value;
                    page.facts.reliability->log_rank_groups.clear();
                    page.facts.reliability->log_rank_groups.reserve(
                        log_rank.group_summaries.size());
                    for (const auto& summary : log_rank.group_summaries) {
                        const std::string& label =
                            summary.group_id >= 0
                            && static_cast<std::size_t>(summary.group_id) < group_levels.size()
                            ? group_levels[static_cast<std::size_t>(summary.group_id)]
                            : std::to_string(summary.group_id);
                        group_counts.rows.push_back({
                            label,
                            std::to_string(summary.n),
                            std::to_string(summary.failures),
                            std::to_string(summary.censored)});
                        page.facts.reliability->log_rank_groups.push_back({
                            label,
                            summary.group_id,
                            summary.n,
                            summary.failures,
                            summary.censored});
                    }
                    page.tables.push_back(std::move(group_counts));
                }
            }
        }
    }
    if (model == "weibull" || model == "exponential" || model == "lognormal") {
        const auto distribution_candidates =
            datalab::domain::statistics::compare_parametric_distributions(
                aligned_times, events);
        StatisticTable distribution_table;
        distribution_table.title = "参数分布比较";
        distribution_table.headers = {"分布", "AIC", "BIC", "Converged"};
        for (const auto& candidate : distribution_candidates) {
            distribution_table.rows.push_back({
                candidate.name,
                format_number(candidate.aic),
                format_number(candidate.bic),
                candidate.converged ? "是" : "否"});
            page.diagnostics.insert(page.diagnostics.end(),
                                    candidate.diagnostics.cbegin(),
                                    candidate.diagnostics.cend());
        }
        page.tables.push_back(std::move(distribution_table));
    }
    if (page.facts.reliability.has_value()) {
        std::set<std::string> modes;
        double total_exposure = 0.0;
        std::size_t exposure_rows = 0;
        for (const auto& observation : typed_observations) {
            if (observation.type == datalab::domain::statistics::CensoringType::exact
                && !observation.failure_mode.empty()) {
                modes.insert(observation.failure_mode);
            }
            if (observation.exposure.has_value()) {
                total_exposure += *observation.exposure;
                ++exposure_rows;
            }
        }
        page.facts.reliability->failure_modes.assign(modes.begin(), modes.end());
        page.facts.reliability->failure_mode_distinct_count =
            page.facts.reliability->failure_modes.size();
        if (configuration.reliability.exposure_column.has_value()) {
            page.facts.reliability->total_exposure = total_exposure;
            page.facts.reliability->exposure_row_count = exposure_rows;
            page.facts.reliability->exposure_source = "column_sum";
            page.parameter_summary +=
                "    暴露量合计 = " + format_number(total_exposure)
                + "（列求和，" + std::to_string(exposure_rows) + " 行）";
        }
        if (!page.facts.reliability->failure_modes.empty()) {
            StatisticTable mode_table;
            mode_table.title = "失效模式（观测到的 exact 失效）";
            mode_table.headers = {"Failure Mode"};
            for (const auto& mode : page.facts.reliability->failure_modes) {
                mode_table.rows.push_back({mode});
            }
            page.tables.push_back(std::move(mode_table));
        }

        const std::string mode_model = configuration.reliability.model.empty()
            ? "kaplan_meier"
            : configuration.reliability.model;
        const auto mode_fits = datalab::domain::statistics::fit_reliability_by_failure_mode(
            typed_observations,
            mode_model,
            configuration.reliability.warranty_time);
        page.diagnostics.insert(
            page.diagnostics.end(),
            mode_fits.diagnostics.begin(),
            mode_fits.diagnostics.end());
        if (mode_fits.ran) {
            page.facts.reliability->mode_fit_scheme = mode_fits.fitting_scheme;
            StatisticTable fit_table;
            fit_table.title = "分模式可靠度（cause-specific）";
            fit_table.headers = {
                "Failure Mode", "Failures", "Competing", "Right Censored",
                "Identifiable", "R(Tw)", "Median", "Note"};
            for (const auto& fit : mode_fits.modes) {
                page.diagnostics.insert(
                    page.diagnostics.end(),
                    fit.diagnostics.begin(),
                    fit.diagnostics.end());
                domain::ReliabilityModeFitFacts facts;
                facts.failure_mode = fit.failure_mode;
                facts.failure_count = fit.failure_count;
                facts.competing_failure_count = fit.competing_failure_count;
                facts.right_censored_count = fit.right_censored_count;
                facts.valid_count = fit.valid_count;
                facts.identifiable = fit.identifiable;
                facts.converged = fit.converged;
                facts.shape = fit.shape;
                facts.scale = fit.scale;
                facts.location = fit.location;
                facts.rate = fit.rate;
                facts.median_life = fit.median_life;
                facts.reliability_at_warranty = fit.reliability_at_warranty;
                facts.not_computed_reason = fit.not_computed_reason;
                facts.evidence_type = fit.evidence_type;
                facts.algorithm_id = fit.algorithm_id;
                facts.source_rows = fit.source_rows;
                fit_table.rows.push_back({
                    fit.failure_mode,
                    std::to_string(fit.failure_count),
                    std::to_string(fit.competing_failure_count),
                    std::to_string(fit.right_censored_count),
                    fit.identifiable ? "yes" : "no",
                    fit.reliability_at_warranty.has_value()
                        ? format_number(*fit.reliability_at_warranty)
                        : "—",
                    fit.median_life.has_value() ? format_number(*fit.median_life) : "—",
                    fit.not_computed_reason.empty() ? fit.algorithm_id
                                                    : fit.not_computed_reason});
                page.facts.reliability->mode_fits.push_back(std::move(facts));
            }
            page.tables.push_back(std::move(fit_table));
        }

        const auto cif = datalab::domain::statistics::aalen_johansen_cif(
            typed_observations, configuration.reliability.warranty_time);
        page.diagnostics.insert(
            page.diagnostics.end(),
            cif.diagnostics.begin(),
            cif.diagnostics.end());
        if (cif.ran) {
            page.facts.reliability->cif_algorithm_id = cif.algorithm_id;
            page.facts.reliability->cif_evidence_type = cif.evidence_type;
            StatisticTable cif_table;
            cif_table.title = "累计发生函数 CIF（Aalen-Johansen）";
            cif_table.headers = {
                "Failure Mode", "Failures", "CIF(last)", "CIF(Tw)", "Points", "Algorithm"};
            for (const auto& mode : cif.modes) {
                domain::ReliabilityCifModeFacts facts;
                facts.failure_mode = mode.failure_mode;
                facts.failure_count = mode.failure_count;
                facts.cif_at_last_event = mode.cif_at_last_event;
                facts.cif_at_warranty = mode.cif_at_warranty;
                facts.point_count = mode.points.size();
                cif_table.rows.push_back({
                    mode.failure_mode,
                    std::to_string(mode.failure_count),
                    mode.cif_at_last_event.has_value()
                        ? format_number(*mode.cif_at_last_event)
                        : "—",
                    mode.cif_at_warranty.has_value()
                        ? format_number(*mode.cif_at_warranty)
                        : "—",
                    std::to_string(mode.points.size()),
                    cif.algorithm_id});
                page.facts.reliability->cif_modes.push_back(std::move(facts));
            }
            page.tables.push_back(std::move(cif_table));

            PlotSpec cif_plot;
            cif_plot.kind = PlotKind::scatter;
            cif_plot.title = "累计发生函数 CIF 曲线";
            cif_plot.x_axis_title = "Time";
            cif_plot.y_axis_title = "CIF";
            cif_plot.show_legend = true;
            for (const auto& mode : cif.modes) {
                PlotSeries series;
                series.label = mode.failure_mode;
                series.role = PlotSeriesRole::generic;
                for (const auto& point : mode.points) {
                    series.x_values.push_back(point.time);
                    series.values.push_back(point.cif);
                }
                cif_plot.series.push_back(std::move(series));
            }
            if (!cif_plot.series.empty()) {
                page.plots.push_back(std::move(cif_plot));
            }

            if (configuration.reliability.group_column.has_value()
                && page.facts.reliability->failure_mode_distinct_count >= 2) {
                const auto gray = datalab::domain::statistics::gray_test_cif(
                    typed_observations);
                page.diagnostics.insert(
                    page.diagnostics.end(),
                    gray.diagnostics.begin(),
                    gray.diagnostics.end());
                if (gray.ran) {
                    page.facts.reliability->gray_chi_square = gray.chi_square;
                    page.facts.reliability->gray_df = gray.df;
                    page.facts.reliability->gray_p_value = gray.p_value;
                    page.facts.reliability->gray_group_count = gray.group_count;
                    page.facts.reliability->gray_algorithm_id = "gray_cif_group_test";
                    StatisticTable gray_table;
                    gray_table.title = "Gray 检验（CIF 组间比较）";
                    gray_table.headers = {"Chi-Square", "DF", "P-Value", "Groups", "Algorithm"};
                    gray_table.rows.push_back({
                        gray.chi_square.has_value()
                            ? format_number(*gray.chi_square) : "—",
                        gray.df.has_value()
                            ? format_number(*gray.df) : "—",
                        gray.p_value.has_value()
                            ? format_number(*gray.p_value) : "—",
                        std::to_string(gray.group_count),
                        "gray_cif_group_test"});
                    page.tables.push_back(std::move(gray_table));
                } else {
                    page.facts.reliability->gray_not_computed_reason =
                        gray.not_computed_reason;
                }
            }
        }

        std::vector<std::size_t> fg_covariate_cols = configuration.reliability.covariate_columns;
        if (fg_covariate_cols.empty()
            && configuration.reliability.covariate_column.has_value()) {
            fg_covariate_cols.push_back(*configuration.reliability.covariate_column);
        }
        if (!fg_covariate_cols.empty()
            || configuration.reliability.group_column.has_value()) {
            datalab::domain::statistics::FineGrayResult fg;
            if (fg_covariate_cols.size() >= 2) {
                const std::size_t p = fg_covariate_cols.size();
                std::vector<std::vector<double>> x_matrix(
                    typed_observations.size(),
                    std::vector<double>(
                        p, std::numeric_limits<double>::quiet_NaN()));
                std::vector<std::string> cov_names;
                cov_names.reserve(p);
                for (std::size_t k = 0; k < p; ++k) {
                    const auto cov = extract_numeric_column(
                        table, fg_covariate_cols[k], configuration.excluded_rows);
                    std::map<std::size_t, double> by_row;
                    for (std::size_t i = 0; i < cov.source_rows.size(); ++i) {
                        by_row[cov.source_rows[i]] = cov.values[i];
                    }
                    for (std::size_t i = 0; i < typed_observations.size(); ++i) {
                        const auto it = by_row.find(typed_observations[i].source_row);
                        if (it != by_row.end()) {
                            x_matrix[i][k] = it->second;
                        }
                    }
                    std::string name = "x" + std::to_string(k);
                    if (fg_covariate_cols[k] < table.columns.size()) {
                        name = table.columns[fg_covariate_cols[k]];
                    }
                    cov_names.push_back(std::move(name));
                }
                fg = datalab::domain::statistics::fine_gray_multi(
                    typed_observations, x_matrix, cov_names, {});
                if (configuration.reliability.group_column.has_value()) {
                    page.diagnostics.push_back({
                        DiagnosticMessage::Severity::info,
                        "fine_gray_covariate_priority",
                        "已指定多协变量列：Fine-Gray 使用 multi IPCW，"
                        "不与二分类 group Fine-Gray 同时运行。"});
                }
            } else if (!fg_covariate_cols.empty()) {
                const auto cov = extract_numeric_column(
                    table, fg_covariate_cols.front(),
                    configuration.excluded_rows);
                std::vector<double> x_values(typed_observations.size(),
                                             std::numeric_limits<double>::quiet_NaN());
                std::map<std::size_t, double> by_row;
                for (std::size_t i = 0; i < cov.source_rows.size(); ++i) {
                    by_row[cov.source_rows[i]] = cov.values[i];
                }
                for (std::size_t i = 0; i < typed_observations.size(); ++i) {
                    const auto it = by_row.find(typed_observations[i].source_row);
                    if (it != by_row.end()) {
                        x_values[i] = it->second;
                    }
                }
                std::string cov_name = "x";
                if (fg_covariate_cols.front() < table.columns.size()) {
                    cov_name = table.columns[fg_covariate_cols.front()];
                }
                fg = datalab::domain::statistics::fine_gray_continuous(
                    typed_observations, x_values, {}, cov_name);
                if (configuration.reliability.group_column.has_value()) {
                    page.diagnostics.push_back({
                        DiagnosticMessage::Severity::info,
                        "fine_gray_covariate_priority",
                        "已指定连续协变量列：Fine-Gray 使用 continuous IPCW，"
                        "不与二分类 group Fine-Gray 同时运行。"});
                }
            } else {
                fg = datalab::domain::statistics::fine_gray_binary(typed_observations);
            }
            page.diagnostics.insert(
                page.diagnostics.end(), fg.diagnostics.begin(), fg.diagnostics.end());
            page.facts.reliability->fine_gray_algorithm_id = fg.algorithm_id;
            page.facts.reliability->fine_gray_evidence_type = fg.evidence_type;
            page.facts.reliability->fine_gray_kind = fg.kind;
            page.facts.reliability->fine_gray_target_mode = fg.target_failure_mode;
            page.facts.reliability->fine_gray_covariate_name = fg.covariate_name;
            page.facts.reliability->fine_gray_group0 = fg.group_level_0;
            page.facts.reliability->fine_gray_group1 = fg.group_level_1;
            page.facts.reliability->fine_gray_converged = fg.converged;
            page.facts.reliability->fine_gray_covariate_mean = fg.covariate_mean;
            page.facts.reliability->fine_gray_beta = fg.beta;
            page.facts.reliability->fine_gray_se = fg.se_beta;
            page.facts.reliability->fine_gray_hazard_ratio = fg.hazard_ratio;
            page.facts.reliability->fine_gray_p_value = fg.p_value;
            page.facts.reliability->fine_gray_not_computed_reason =
                fg.not_computed_reason;
            page.facts.reliability->fine_gray_target_failures = fg.target_failures;
            page.facts.reliability->fine_gray_competing_failures =
                fg.competing_failures;
            for (const auto& term : fg.terms) {
                domain::ReliabilityFineGrayTermFacts facts;
                facts.name = term.name;
                facts.mean = term.mean;
                facts.beta = term.beta;
                facts.se = term.se_beta;
                facts.hazard_ratio = term.hazard_ratio;
                facts.p_value = term.p_value;
                page.facts.reliability->fine_gray_terms.push_back(std::move(facts));
            }
            if (fg.ran && fg.converged && !fg.terms.empty()) {
                StatisticTable fg_table;
                if (fg.kind == "multi") {
                    fg_table.title = "Fine-Gray 子分布风险（多协变量）";
                    fg_table.headers = {
                        "Target Mode", "Covariate", "Mean", "Beta", "SE", "HR(+1)", "P",
                        "Algorithm"};
                    for (const auto& term : fg.terms) {
                        fg_table.rows.push_back({
                            fg.target_failure_mode,
                            term.name,
                            term.mean.has_value() ? format_number(*term.mean) : "—",
                            term.beta.has_value() ? format_number(*term.beta) : "—",
                            term.se_beta.has_value() ? format_number(*term.se_beta)
                                                     : "—",
                            term.hazard_ratio.has_value()
                                ? format_number(*term.hazard_ratio)
                                : "—",
                            term.p_value.has_value() ? format_number(*term.p_value)
                                                     : "—",
                            fg.algorithm_id});
                    }
                } else if (fg.kind == "continuous") {
                    fg_table.title = "Fine-Gray 子分布风险（连续协变量）";
                    fg_table.headers = {
                        "Target Mode", "Covariate", "Mean", "Beta", "SE", "HR(+1)", "P",
                        "Algorithm"};
                    fg_table.rows.push_back({
                        fg.target_failure_mode,
                        fg.covariate_name,
                        fg.covariate_mean.has_value() ? format_number(*fg.covariate_mean)
                                                      : "—",
                        format_number(*fg.beta),
                        fg.se_beta.has_value() ? format_number(*fg.se_beta) : "—",
                        fg.hazard_ratio.has_value() ? format_number(*fg.hazard_ratio)
                                                    : "—",
                        fg.p_value.has_value() ? format_number(*fg.p_value) : "—",
                        fg.algorithm_id});
                } else {
                    fg_table.title = "Fine-Gray 子分布风险（二分类 group）";
                    fg_table.headers = {
                        "Target Mode", "Group0", "Group1", "Beta", "SE", "HR", "P",
                        "Algorithm"};
                    fg_table.rows.push_back({
                        fg.target_failure_mode,
                        fg.group_level_0,
                        fg.group_level_1,
                        format_number(*fg.beta),
                        fg.se_beta.has_value() ? format_number(*fg.se_beta) : "—",
                        fg.hazard_ratio.has_value() ? format_number(*fg.hazard_ratio)
                                                    : "—",
                        fg.p_value.has_value() ? format_number(*fg.p_value) : "—",
                        fg.algorithm_id});
                }
                page.tables.push_back(std::move(fg_table));
            }
        }
    }
    if (!typed_observations.empty()) {
        page.worksheet_export =
            datalab::domain::statistics::censoring_observations_to_worksheet(
                typed_observations);
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "censoring_worksheet_export_ready",
            "已生成逐观测删失状态工作表（"
                + std::to_string(typed_observations.size())
                + " 行；censoring_type=exact|right|left|interval）。"
                  "可写回活动表以便审计或再导入；不是 vendor_oracle。"});
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::reliability_warranty(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    datalab::domain::statistics::WarrantySummaryOptions options;
    options.warranty_time = configuration.reliability.warranty_time;
    options.time_unit = configuration.reliability.time_unit;
    options.exposure = configuration.reliability.exposure;
    options.reliability_at_warranty = configuration.reliability.reliability_at_warranty;
    options.observed_failures = configuration.reliability.warranty_observed_failures;
    options.censored_count = configuration.reliability.warranty_censored_count;
    options.valid_count = configuration.reliability.warranty_valid_count;
    options.model_name = configuration.reliability.model.empty()
        ? "external_reliability"
        : configuration.reliability.model;
    options.design_or_dataset_id = configuration.reliability.dataset_id;
    options.reliability_is_prediction = configuration.reliability.reliability_is_prediction;

    std::string exposure_source = "scalar";
    std::size_t exposure_row_count = 0;
    std::vector<DiagnosticMessage> exposure_diagnostics;
    if (configuration.reliability.exposure_column.has_value()) {
        const std::size_t exposure_col = *configuration.reliability.exposure_column;
        const auto excluded = [&](std::size_t row) {
            return std::find(configuration.excluded_rows.cbegin(),
                             configuration.excluded_rows.cend(),
                             row)
                != configuration.excluded_rows.cend();
        };
        double sum = 0.0;
        std::vector<std::size_t> invalid_rows;
        for (std::size_t row = 0; row < table.rows.size(); ++row) {
            if (excluded(row)) {
                continue;
            }
            if (exposure_col >= table.rows[row].size()
                || is_missing_cell(table.rows[row][exposure_col])) {
                invalid_rows.push_back(row);
                continue;
            }
            double value = 0.0;
            if (!datalab::domain::parse_finite_number(table.rows[row][exposure_col], value)
                || !std::isfinite(value) || value < 0.0) {
                invalid_rows.push_back(row);
                continue;
            }
            sum += value;
            ++exposure_row_count;
        }
        if (!invalid_rows.empty()) {
            OutputPage page;
            page.id = new_id("reliability_warranty");
            page.title = "保修摘要";
            page.method_name = "Warranty Summary";
            page.configuration = configuration;
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::error,
                "invalid_exposure_value",
                "暴露量列必须为有限非负数；缺失或非法值不会被静默补齐。"});
            for (const std::size_t row : invalid_rows) {
                page.diagnostics.front().related_rows.push_back(
                    static_cast<datalab::domain::RowId>(row));
            }
            return finalize_page(std::move(page));
        }
        if (exposure_row_count == 0 || !(sum > 0.0)) {
            OutputPage page;
            page.id = new_id("reliability_warranty");
            page.title = "保修摘要";
            page.method_name = "Warranty Summary";
            page.configuration = configuration;
            page.diagnostics.push_back({
                DiagnosticMessage::Severity::error,
                "warranty_zero_exposure",
                "暴露量列求和后必须为正有限数；不得用标量默认值静默补齐。"});
            return finalize_page(std::move(page));
        }
        if (configuration.reliability.exposure > 0.0) {
            exposure_diagnostics.push_back({
                DiagnosticMessage::Severity::info,
                "warranty_exposure_column_overrides_scalar",
                "同时提供了暴露量列与标量暴露量；摘要使用列求和，标量被忽略。"});
        }
        options.exposure = sum;
        exposure_source = "column_sum";
    }

    // Build strata before overall summary so counts backfill into the summary table.
    std::vector<datalab::domain::statistics::WarrantyStratumInput> stratum_inputs;
    bool want_failure_mode = false;
    bool want_group = false;
    {
        want_failure_mode = configuration.reliability.failure_mode_column.has_value();
        want_group = configuration.reliability.group_column.has_value()
            && !want_failure_mode;
        if ((want_failure_mode || want_group)
            && !table.rows.empty()
            && (configuration.reliability.event_column.has_value()
                || configuration.reliability.censoring_type_column.has_value())) {
            const std::size_t label_col = want_failure_mode
                ? *configuration.reliability.failure_mode_column
                : *configuration.reliability.group_column;
            const std::string stratum_kind =
                want_failure_mode ? "failure_mode" : "group";
            const auto labels = extract_text_column(table, label_col);
            const auto event_text = configuration.reliability.event_column.has_value()
                ? extract_text_column(table, *configuration.reliability.event_column)
                : std::vector<std::string>{};
            const auto censor_type_text =
                configuration.reliability.censoring_type_column.has_value()
                    ? extract_text_column(
                          table, *configuration.reliability.censoring_type_column)
                    : std::vector<std::string>{};

            struct Acc {
                double exposure = 0.0;
                std::size_t failures = 0;
                std::size_t censored = 0;
                std::size_t valid = 0;
                std::vector<std::size_t> source_rows;
            };
            std::map<std::string, Acc> by_label;
            const auto excluded = [&](std::size_t row) {
                return std::find(configuration.excluded_rows.cbegin(),
                                 configuration.excluded_rows.cend(),
                                 row)
                    != configuration.excluded_rows.cend();
            };

            for (std::size_t row = 0; row < table.rows.size(); ++row) {
                if (excluded(row)) {
                    continue;
                }
                bool is_failure = false;
                bool is_censored = false;
                bool typed = false;
                if (!censor_type_text.empty() && row < censor_type_text.size()
                    && !is_missing_cell(censor_type_text[row])) {
                    const auto parsed =
                        datalab::domain::statistics::parse_censoring_type(
                            censor_type_text[row]);
                    if (!parsed.has_value()) {
                        continue;
                    }
                    typed = true;
                    is_failure =
                        *parsed == datalab::domain::statistics::CensoringType::exact;
                    is_censored = !is_failure;
                } else if (!event_text.empty() && row < event_text.size()
                           && !is_missing_cell(event_text[row])) {
                    const auto parsed_event =
                        datalab::domain::statistics::parse_reliability_event(
                            event_text[row]);
                    if (!parsed_event.has_value()) {
                        continue;
                    }
                    typed = true;
                    is_failure = *parsed_event;
                    is_censored = !is_failure;
                }
                if (!typed) {
                    continue;
                }

                std::string label = "(unlabeled)";
                if (row < labels.size() && !is_missing_cell(labels[row])
                    && !labels[row].empty()) {
                    label = labels[row];
                }
                Acc& acc = by_label[label];
                ++acc.valid;
                if (is_failure) {
                    ++acc.failures;
                } else if (is_censored) {
                    ++acc.censored;
                }
                acc.source_rows.push_back(row);

                if (configuration.reliability.exposure_column.has_value()) {
                    const std::size_t exposure_col =
                        *configuration.reliability.exposure_column;
                    if (exposure_col < table.rows[row].size()
                        && !is_missing_cell(table.rows[row][exposure_col])) {
                        double value = 0.0;
                        if (datalab::domain::parse_finite_number(
                                table.rows[row][exposure_col], value)
                            && std::isfinite(value) && value >= 0.0) {
                            acc.exposure += value;
                        }
                    }
                }
            }

            stratum_inputs.reserve(by_label.size());
            std::size_t sum_fail = 0;
            std::size_t sum_cens = 0;
            std::size_t sum_valid = 0;
            for (const auto& [label, acc] : by_label) {
                datalab::domain::statistics::WarrantyStratumInput input;
                input.label = label;
                input.kind = stratum_kind;
                input.exposure = acc.exposure;
                input.observed_failures = acc.failures;
                input.censored_count = acc.censored;
                input.valid_count = acc.valid;
                input.source_rows = acc.source_rows;
                sum_fail += acc.failures;
                sum_cens += acc.censored;
                sum_valid += acc.valid;
                stratum_inputs.push_back(std::move(input));
            }
            if (options.observed_failures == 0 && sum_fail > 0) {
                options.observed_failures = sum_fail;
            }
            if (options.censored_count == 0 && sum_cens > 0) {
                options.censored_count = sum_cens;
            }
            if (options.valid_count == 0 && sum_valid > 0) {
                options.valid_count = sum_valid;
            }

            // Optional cause-specific R(Tw) per failure_mode when time column exists.
            if (want_failure_mode
                && configuration.reliability.time_column.has_value()) {
                const auto times = extract_numeric_column(
                    table,
                    *configuration.reliability.time_column,
                    configuration.excluded_rows);
                std::vector<datalab::domain::statistics::CensoringObservation>
                    mode_observations;
                mode_observations.reserve(times.source_rows.size());
                for (std::size_t index = 0; index < times.source_rows.size(); ++index) {
                    const std::size_t row = times.source_rows[index];
                    datalab::domain::statistics::CensoringObservation observation;
                    observation.time = times.values[index];
                    observation.source_row = row;
                    observation.time_unit = configuration.reliability.time_unit;
                    if (row < labels.size() && !is_missing_cell(labels[row])) {
                        observation.failure_mode = labels[row];
                    }
                    bool typed = false;
                    if (!censor_type_text.empty() && row < censor_type_text.size()
                        && !is_missing_cell(censor_type_text[row])) {
                        const auto parsed =
                            datalab::domain::statistics::parse_censoring_type(
                                censor_type_text[row]);
                        if (!parsed.has_value()) {
                            continue;
                        }
                        observation.type = *parsed;
                        typed = true;
                    } else if (!event_text.empty() && row < event_text.size()
                               && !is_missing_cell(event_text[row])) {
                        const auto parsed_event =
                            datalab::domain::statistics::parse_reliability_event(
                                event_text[row]);
                        if (!parsed_event.has_value()) {
                            continue;
                        }
                        observation.type = *parsed_event
                            ? datalab::domain::statistics::CensoringType::exact
                            : datalab::domain::statistics::CensoringType::right;
                        typed = true;
                    }
                    if (!typed) {
                        continue;
                    }
                    mode_observations.push_back(std::move(observation));
                }
                const std::string mode_model = configuration.reliability.model.empty()
                    ? "weibull"
                    : configuration.reliability.model;
                const auto mode_fits =
                    datalab::domain::statistics::fit_reliability_by_failure_mode(
                        mode_observations,
                        mode_model,
                        options.warranty_time);
                exposure_diagnostics.insert(
                    exposure_diagnostics.end(),
                    mode_fits.diagnostics.begin(),
                    mode_fits.diagnostics.end());
                for (const auto& fit : mode_fits.modes) {
                    exposure_diagnostics.insert(
                        exposure_diagnostics.end(),
                        fit.diagnostics.begin(),
                        fit.diagnostics.end());
                }
                for (auto& input : stratum_inputs) {
                    for (const auto& fit : mode_fits.modes) {
                        if (fit.failure_mode == input.label
                            && fit.identifiable
                            && fit.reliability_at_warranty.has_value()) {
                            input.reliability_at_warranty = fit.reliability_at_warranty;
                            break;
                        }
                    }
                }
            }
        }
    }

    const auto summary = datalab::domain::statistics::summarize_warranty(options);
    OutputPage page;
    page.id = new_id("reliability_warranty");
    page.title = "保修摘要";
    page.method_name = "Warranty Summary";
    page.configuration = configuration;
    page.parameter_summary = "T_w = " + format_number(options.warranty_time)
        + " " + options.time_unit
        + "    暴露量 = " + format_number(options.exposure)
        + "（" + exposure_source + "）"
        + "    R(T_w) = " + format_number(options.reliability_at_warranty);
    page.diagnostics = exposure_diagnostics;
    page.diagnostics.insert(page.diagnostics.end(),
                            summary.diagnostics.begin(), summary.diagnostics.end());
    if (!summary.ok) {
        return finalize_page(std::move(page));
    }

    StatisticTable summary_table;
    summary_table.title = "保修摘要";
    summary_table.headers = {"Property", "Value"};
    summary_table.rows.push_back({"Warranty time T_w", format_number(summary.warranty_time)});
    summary_table.rows.push_back({"Time unit", summary.time_unit});
    summary_table.rows.push_back({"Exposure", format_number(summary.exposure)});
    summary_table.rows.push_back({"Exposure source", exposure_source});
    summary_table.rows.push_back({"Exposure rows", std::to_string(exposure_row_count)});
    summary_table.rows.push_back({"R(T_w)", format_number(summary.reliability_at_warranty)});
    summary_table.rows.push_back({"F(T_w)=1-R(T_w)", format_number(summary.failure_probability)});
    summary_table.rows.push_back({"Expected failures", format_number(summary.expected_failures)});
    summary_table.rows.push_back({"Claims per 1000", format_number(summary.claims_per_1000)});
    summary_table.rows.push_back({"Observed failures", std::to_string(summary.observed_failures)});
    summary_table.rows.push_back({"Censored count", std::to_string(summary.censored_count)});
    summary_table.rows.push_back({"Valid count", std::to_string(summary.valid_count)});
    summary_table.rows.push_back({"Model", summary.model_name});
    summary_table.rows.push_back({"Quantity label", summary.quantity_label});
    summary_table.rows.push_back({"Evidence", summary.evidence_type});
    page.tables.push_back(std::move(summary_table));

    domain::WarrantyFacts facts;
    facts.warranty_time = summary.warranty_time;
    facts.time_unit = summary.time_unit;
    facts.exposure = summary.exposure;
    facts.reliability_at_warranty = summary.reliability_at_warranty;
    facts.failure_probability = summary.failure_probability;
    facts.expected_failures = summary.expected_failures;
    facts.claims_per_1000 = summary.claims_per_1000;
    facts.observed_failures = summary.observed_failures;
    facts.censored_count = summary.censored_count;
    facts.valid_count = summary.valid_count;
    facts.model_name = summary.model_name;
    facts.quantity_label = summary.quantity_label;
    facts.evidence_type = summary.evidence_type;
    facts.exposure_source = exposure_source;
    facts.exposure_row_count = exposure_row_count;

    if (!stratum_inputs.empty()) {
        const auto strata_summary =
            datalab::domain::statistics::summarize_warranty_strata(
                options, stratum_inputs);
        page.diagnostics.insert(page.diagnostics.end(),
                                strata_summary.diagnostics.begin(),
                                strata_summary.diagnostics.end());
        if (strata_summary.ok && !strata_summary.strata.empty()) {
            StatisticTable stratum_table;
            stratum_table.title = want_failure_mode
                ? "失效模式分母追溯"
                : "分组分母追溯";
            stratum_table.headers = {
                "Stratum", "Kind", "Exposure", "Attribution",
                "Share", "Observed failures", "Censored", "Valid",
                "R(Tw)", "R source", "Expected failures", "Source rows"};
            for (const auto& stratum : strata_summary.strata) {
                std::string rows_text;
                for (std::size_t i = 0; i < stratum.source_rows.size(); ++i) {
                    if (i > 0) {
                        rows_text += ",";
                    }
                    rows_text += std::to_string(stratum.source_rows[i] + 1);
                    if (i >= 11) {
                        rows_text += ",...";
                        break;
                    }
                }
                stratum_table.rows.push_back({
                    stratum.label,
                    stratum.kind,
                    format_number(stratum.exposure),
                    stratum.exposure_attribution,
                    format_number(stratum.share_of_total_exposure),
                    std::to_string(stratum.observed_failures),
                    std::to_string(stratum.censored_count),
                    std::to_string(stratum.valid_count),
                    stratum.reliability_at_warranty.has_value()
                        ? format_number(*stratum.reliability_at_warranty)
                        : "—",
                    stratum.uses_mode_specific_reliability ? "cause_specific"
                                                           : "pooled",
                    format_number(stratum.expected_failures),
                    rows_text});
                domain::WarrantyStratumFacts sf;
                sf.label = stratum.label;
                sf.kind = stratum.kind;
                sf.exposure = stratum.exposure;
                sf.observed_failures = stratum.observed_failures;
                sf.censored_count = stratum.censored_count;
                sf.valid_count = stratum.valid_count;
                sf.expected_failures = stratum.expected_failures;
                sf.share_of_total_exposure = stratum.share_of_total_exposure;
                sf.exposure_attribution = stratum.exposure_attribution;
                sf.reliability_at_warranty = stratum.reliability_at_warranty;
                sf.uses_mode_specific_reliability =
                    stratum.uses_mode_specific_reliability;
                sf.source_rows = stratum.source_rows;
                facts.strata.push_back(std::move(sf));
            }
            page.tables.push_back(std::move(stratum_table));
            facts.stratum_kind = strata_summary.stratum_kind;
            facts.uses_pooled_reliability = strata_summary.uses_pooled_reliability;
            facts.uses_mode_specific_reliability =
                strata_summary.uses_mode_specific_reliability;
        }
    }

    page.facts.warranty = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::t_power(
    const DataTable&,
    const AnalysisConfiguration& configuration)
{
    const bool two_sample = configuration.power.mode.find("two_sample") == 0;
    const bool calculate_power = configuration.power.mode.find("_power") != std::string::npos;
    const auto alternative = configuration.inference.alternative == "greater"
        ? datalab::domain::statistics::PowerAlternative::greater
        : configuration.inference.alternative == "less"
            ? datalab::domain::statistics::PowerAlternative::less
            : datalab::domain::statistics::PowerAlternative::two_sided;
    const auto variance_method = configuration.power.variance_method == "unpooled"
        ? datalab::domain::statistics::ProportionVarianceMethod::unpooled
        : datalab::domain::statistics::ProportionVarianceMethod::pooled;
    const auto compute = [&](std::size_t sample_size, double effect, bool want_power)
        -> datalab::domain::statistics::TPowerResult {
        const double lower = configuration.power.equivalence_lower.value_or(-std::abs(effect));
        const double upper = configuration.power.equivalence_upper.value_or(std::abs(effect));
        const double true_diff = configuration.power.equivalence_difference;
        if (configuration.power.mode.find("equivalence_one_sample") == 0) {
            return want_power
                ? datalab::domain::statistics::equivalence_one_sample_power(
                    sample_size, lower, upper, true_diff, configuration.power.alpha)
                : datalab::domain::statistics::equivalence_one_sample_sample_size(
                    lower, upper, configuration.power.target, true_diff,
                    configuration.power.alpha);
        }
        if (configuration.power.mode.find("equivalence_two_sample") == 0) {
            return want_power
                ? datalab::domain::statistics::equivalence_two_sample_power(
                    sample_size, lower, upper, true_diff, configuration.power.alpha)
                : datalab::domain::statistics::equivalence_two_sample_sample_size(
                    lower, upper, configuration.power.target, true_diff,
                    configuration.power.alpha);
        }
        if (configuration.power.mode.find("doe_factorial") == 0) {
            const std::size_t k = std::max<std::size_t>(2, configuration.power.group_count);
            const std::size_t p = configuration.power.doe_fraction_p;
            const std::size_t replicates = sample_size > 0
                ? sample_size
                : std::max<std::size_t>(1, configuration.power.doe_replicates);
            return want_power
                ? datalab::domain::statistics::doe_factorial_power(
                    k, p, replicates, effect, configuration.power.alpha)
                : datalab::domain::statistics::doe_factorial_sample_size(
                    k, p, effect, configuration.power.target, configuration.power.alpha);
        }
        if (configuration.power.mode.find("tolerance") == 0) {
            const double coverage = effect > 0.0 && effect < 1.0 ? effect : 0.95;
            const double max_k = configuration.power.null_proportion > 1.0
                ? configuration.power.null_proportion
                : 4.0;
            return datalab::domain::statistics::tolerance_normal_sample_size(
                coverage, configuration.power.target > 0.0 && configuration.power.target < 1.0
                    ? configuration.power.target
                    : 0.95,
                max_k);
        }
        if (configuration.power.mode.find("one_variance") == 0) {
            return want_power
                ? datalab::domain::statistics::one_variance_power(
                    sample_size, effect, configuration.power.alpha, alternative)
                : datalab::domain::statistics::one_variance_sample_size(
                    effect, configuration.power.target, configuration.power.alpha, alternative);
        }
        if (configuration.power.mode.find("two_variance") == 0) {
            return want_power
                ? datalab::domain::statistics::two_variance_power(
                    sample_size, effect, configuration.power.alpha, alternative)
                : datalab::domain::statistics::two_variance_sample_size(
                    effect, configuration.power.target, configuration.power.alpha, alternative);
        }
        if (configuration.power.mode.find("anova") == 0) {
            return want_power
                ? datalab::domain::statistics::one_way_anova_power(
                    sample_size, configuration.power.group_count,
                    effect, configuration.power.alpha)
                : datalab::domain::statistics::one_way_anova_sample_size(
                    configuration.power.group_count, effect,
                    configuration.power.target, configuration.power.alpha);
        }
        if (configuration.power.mode.find("one_poisson") == 0) {
            return want_power
                ? datalab::domain::statistics::one_poisson_rate_power(
                    sample_size, configuration.power.null_proportion,
                    configuration.power.second_proportion,
                    configuration.power.observation_length, configuration.power.alpha,
                    alternative)
                : datalab::domain::statistics::one_poisson_rate_sample_size(
                    configuration.power.null_proportion, configuration.power.second_proportion,
                    configuration.power.target, configuration.power.observation_length,
                    configuration.power.alpha, alternative);
        }
        if (configuration.power.mode.find("two_poisson") == 0) {
            return want_power
                ? datalab::domain::statistics::two_poisson_rate_power(
                    sample_size, configuration.power.null_proportion,
                    configuration.power.second_proportion,
                    configuration.power.observation_length, configuration.power.alpha,
                    alternative)
                : datalab::domain::statistics::two_poisson_rate_sample_size(
                    configuration.power.null_proportion, configuration.power.second_proportion,
                    configuration.power.target, configuration.power.observation_length,
                    configuration.power.alpha, alternative);
        }
        if (configuration.power.mode.find("one_proportion") == 0) {
            return want_power
                ? datalab::domain::statistics::one_sample_proportion_power(
                    sample_size, configuration.power.null_proportion,
                    configuration.power.second_proportion, configuration.power.alpha,
                    alternative)
                : datalab::domain::statistics::one_sample_proportion_sample_size(
                    configuration.power.null_proportion, configuration.power.second_proportion,
                    configuration.power.target, configuration.power.alpha, alternative);
        }
        if (configuration.power.mode.find("two_proportion") == 0) {
            return want_power
                ? datalab::domain::statistics::two_proportion_power(
                    sample_size, configuration.power.null_proportion,
                    configuration.power.second_proportion, configuration.power.alpha,
                    alternative, variance_method)
                : datalab::domain::statistics::two_proportion_sample_size(
                    configuration.power.null_proportion, configuration.power.second_proportion,
                    configuration.power.target, configuration.power.alpha,
                    alternative, variance_method);
        }
        if (want_power) {
            return two_sample
                ? datalab::domain::statistics::two_sample_t_power(
                    sample_size, effect, configuration.power.alpha)
                : datalab::domain::statistics::one_sample_t_power(
                    sample_size, effect, configuration.power.alpha);
        }
        return two_sample
            ? datalab::domain::statistics::two_sample_t_sample_size(
                effect, configuration.power.target, configuration.power.alpha)
            : datalab::domain::statistics::one_sample_t_sample_size(
                effect, configuration.power.target, configuration.power.alpha);
    };

    std::vector<std::size_t> sample_sizes = parse_comma_sizes(configuration.power.sample_size_list);
    std::vector<double> effects = parse_comma_numbers(configuration.power.effect_size_list);
    if (sample_sizes.empty() && configuration.power.sample_size > 0) {
        sample_sizes.push_back(configuration.power.sample_size);
    }
    if (effects.empty()) {
        effects.push_back(configuration.power.effect_size);
    }
    if (calculate_power && sample_sizes.empty()) {
        sample_sizes.push_back(20);
    }

    std::vector<datalab::domain::statistics::TPowerResult> rows;
    if (calculate_power) {
        for (const std::size_t sample_size : sample_sizes) {
            for (const double effect : effects) {
                rows.push_back(compute(sample_size, effect, true));
            }
        }
    } else {
        for (const double effect : effects) {
            rows.push_back(compute(0, effect, false));
        }
    }

    OutputPage page;
    page.id = new_id("t_power");
    page.title = "T 功效与样本量";
    page.method_name = calculate_power ? "T Test Power" : "T Test Sample Size";
    page.configuration = configuration;
    page.parameter_summary = "模式 = " + configuration.power.mode;
    StatisticTable summary;
    summary.title = "功效与样本量";
    summary.headers = {
        "Sample Size", "Per Group", "Total N", "Effect Size", "Target Power",
        "Actual Power", "DF", "Alpha"};
    for (const auto& result : rows) {
        page.diagnostics.insert(
            page.diagnostics.end(), result.diagnostics.cbegin(), result.diagnostics.cend());
        summary.rows.push_back({
            std::to_string(result.sample_size),
            std::to_string(result.sample_size_per_group),
            std::to_string(result.total_sample_size),
            format_number(result.effect_size),
            format_number(configuration.power.target),
            format_number(result.power),
            format_number(result.degrees_of_freedom),
            format_number(configuration.power.alpha)});
    }
    page.tables.push_back(std::move(summary));
    if (!rows.empty()) {
        PlotSpec curve;
        curve.kind = PlotKind::scatter;
        curve.title = "功效曲线";
        curve.show_legend = true;
        PlotSeries series;
        series.label = "功效";
        series.style.point_style = PlotPointStyle::none;
        if (calculate_power) {
            curve.x_axis_title = "Effect Size";
            curve.y_axis_title = "Power";
            const std::size_t n = sample_sizes.front();
            const double center = effects.front();
            for (int step = 1; step <= 20; ++step) {
                const double effect = std::max(0.05, center * static_cast<double>(step) / 10.0);
                const auto point = compute(n, effect, true);
                series.x_values.push_back(effect);
                series.values.push_back(point.power);
            }
        } else {
            curve.x_axis_title = "Sample Size";
            curve.y_axis_title = "Actual Power";
            const double effect = effects.front();
            const std::size_t needed = std::max<std::size_t>(2, rows.front().sample_size);
                const std::size_t start = std::max<std::size_t>(2, needed / 2);
            const std::size_t stop = std::max(start + 1, needed * 2);
            for (std::size_t n = start; n <= stop; ++n) {
                datalab::domain::statistics::TPowerResult powered = compute(n, effect, true);
                series.x_values.push_back(static_cast<double>(n));
                series.values.push_back(powered.power);
            }
        }
        curve.series.push_back(series);
        page.plots.push_back(std::move(curve));
        page.facts.power = domain::PowerFacts{};
        page.facts.power->power = rows.front().power;
        page.facts.power->effect_size = rows.front().effect_size;
        page.facts.power->mode = configuration.power.mode;
        page.facts.power->sample_size = rows.front().sample_size;
        page.facts.power->target = configuration.power.target;
        page.facts.power->actual_power = rows.front().power;
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::acf_pacf(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.empty()) {
        return error_page("ACF/PACF", "ACF/PACF", "请选择一个数值序列列。");
    }
    const ExtractedNumericColumn extracted = extract_numeric_column(
        table, configuration.variable_columns.front(), configuration.excluded_rows);
    const std::size_t max_lag = configuration.graph.bin_count > 0
        ? static_cast<std::size_t>(configuration.graph.bin_count)
        : 0;
    const double alpha =
        configuration.inference.confidence_level > 0.0
            && configuration.inference.confidence_level < 1.0
        ? 1.0 - configuration.inference.confidence_level
        : 0.05;
    const auto result = datalab::domain::statistics::compute_acf_pacf(
        extracted.values, max_lag, alpha);

    OutputPage page;
    page.id = new_id("acf_pacf");
    page.title = "自相关 / 偏自相关";
    page.method_name = "ACF/PACF";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    N = " + std::to_string(result.n)
        + "    max lag = " + std::to_string(result.max_lag)
        + "    带宽 = ±" + format_number(result.band_half_width);
    page.diagnostics = result.diagnostics;
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            extracted.name + " 跳过 " + std::to_string(extracted.missing_count)
                + " 个缺失或非法单元格。"});
    }

    StatisticTable table_out;
    table_out.title = "ACF / PACF";
    table_out.headers = {"Lag", "ACF", "PACF", "Lower Band", "Upper Band"};
    for (std::size_t index = 0; index < result.lags.size(); ++index) {
        table_out.rows.push_back({
            format_number(result.lags[index]),
            format_number(result.acf[index]),
            index < result.pacf.size() ? format_number(result.pacf[index]) : "*",
            format_number(-result.band_half_width),
            format_number(result.band_half_width)});
    }
    page.tables.push_back(std::move(table_out));

    if (result.ljung_box_statistic.has_value()) {
        StatisticTable lb;
        lb.title = "Ljung–Box";
        lb.headers = {"Statistic", "DF", "P-Value"};
        lb.rows.push_back({
            format_number(*result.ljung_box_statistic),
            std::to_string(result.max_lag),
            format_optional(result.ljung_box_p_value)});
        page.tables.push_back(std::move(lb));
    }

    auto make_corr_plot = [&](const std::string& title, const std::vector<double>& values) {
        PlotSpec plot;
        plot.kind = PlotKind::time_series;
        plot.title = title;
        plot.x_axis_title = "Lag";
        plot.y_axis_title = title;
        plot.x_values = result.lags;
        plot.values = values;
        PlotSeries upper;
        upper.label = "上置信限";
        upper.role = PlotSeriesRole::confidence_band;
        upper.x_values = result.lags;
        upper.values.assign(result.lags.size(), result.band_half_width);
        upper.style.point_style = PlotPointStyle::none;
        PlotSeries lower = upper;
        lower.label = "下置信限";
        lower.values.assign(result.lags.size(), -result.band_half_width);
        plot.series.push_back(std::move(upper));
        plot.series.push_back(std::move(lower));
        return plot;
    };
    if (result.acf.size() == result.lags.size()) {
        page.plots.push_back(make_corr_plot("ACF", result.acf));
    }
    if (result.pacf.size() == result.lags.size()) {
        page.plots.push_back(make_corr_plot("PACF", result.pacf));
    }

    page.facts.acf_pacf = domain::AcfPacfFacts{};
    page.facts.acf_pacf->n = result.n;
    page.facts.acf_pacf->missing_count = result.missing_count + extracted.missing_count;
    page.facts.acf_pacf->max_lag = result.max_lag;
    page.facts.acf_pacf->confidence_band_method = result.confidence_band_method;
    page.facts.acf_pacf->band_half_width = result.band_half_width;
    page.facts.acf_pacf->alpha = result.alpha;
    page.facts.acf_pacf->ljung_box_available = result.ljung_box_statistic.has_value();
    page.facts.acf_pacf->ljung_box_statistic = result.ljung_box_statistic;
    page.facts.acf_pacf->ljung_box_p_value = result.ljung_box_p_value;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::nested_gage_rr(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.msa.nested_measurement_column.has_value()
        || !configuration.msa.nested_part_column.has_value()
        || !configuration.msa.nested_operator_column.has_value()) {
        return error_page("Nested Gage R&R", "Nested Gage R&R",
                          "请选择测量值、部件和操作者列。");
    }
    const auto measurements = extract_numeric_column(
        table, *configuration.msa.nested_measurement_column, configuration.excluded_rows);
    const auto parts = extract_text_column(table, *configuration.msa.nested_part_column);
    const auto operators = extract_text_column(table, *configuration.msa.nested_operator_column);
    std::map<std::size_t, double> measurement_by_row;
    for (std::size_t index = 0; index < measurements.values.size(); ++index) {
        measurement_by_row[measurements.source_rows[index]] = measurements.values[index];
    }
    std::vector<double> values;
    std::vector<std::string> part_values;
    std::vector<std::string> operator_values;
    std::vector<std::size_t> source_rows;
    std::size_t skipped_missing = measurements.missing_count;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        const auto measurement = measurement_by_row.find(row);
        if (measurement == measurement_by_row.end()
            || row >= parts.size() || row >= operators.size()
            || is_missing_cell(parts[row]) || is_missing_cell(operators[row])) {
            if (measurement != measurement_by_row.end()
                && row < parts.size() && row < operators.size()
                && (is_missing_cell(parts[row]) || is_missing_cell(operators[row]))) {
                ++skipped_missing;
            }
            continue;
        }
        values.push_back(measurement->second);
        part_values.push_back(parts[row]);
        operator_values.push_back(operators[row]);
        source_rows.push_back(row);
    }
    const auto result = datalab::domain::statistics::nested_gage_rr(
        values, part_values, operator_values, configuration.msa.gage_tolerance);
    OutputPage page;
    page.id = new_id("nested_gage");
    page.title = "Nested Gage R&R";
    page.method_name = "Nested Gage R&R";
    page.configuration = configuration;
    page.parameter_summary = "部件数 = " + std::to_string(result.part_count)
        + "    操作者数 = " + std::to_string(result.operator_count);
    page.diagnostics = result.diagnostics;
    if (skipped_missing > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "Nested Gage R&R 跳过缺失或非法的测量/零件/操作者单元格。"});
    }
    StatisticTable anova;
    anova.title = "Nested Gage R&R ANOVA";
    anova.headers = {"Source", "DF", "SS", "MS", "F"};
    for (const auto& row : result.anova_rows) {
        anova.rows.push_back({row.source, std::to_string(row.degrees_of_freedom),
            format_number(row.sum_of_squares), format_number(row.mean_square),
            format_number(row.f_statistic)});
    }
    page.tables.push_back(std::move(anova));
    StatisticTable components;
    components.title = "Variance Components";
    components.headers = {"Source", "Variance", "Std Dev", "%Contribution",
                          "Study Var", "%Study Var", "%Tolerance"};
    for (const auto& row : result.variance_components) {
        components.rows.push_back({row.source, format_number(row.variance_component),
            format_number(row.standard_deviation), format_number(row.percent_contribution),
            format_number(row.study_variation), format_number(row.percent_study_variation),
            row.percent_tolerance_available ? format_number(row.percent_tolerance) : "*"});
    }
    page.tables.push_back(std::move(components));
    append_gage_contribution_pareto(page, result.variance_components);
    append_gage_study_var_pareto(page, result.variance_components);
    append_gage_tolerance_pareto(page, result.variance_components);
    append_operator_xbar_range_plots(
        page, values, part_values, operator_values, source_rows,
        result.replicate_count, result.design_balanced);
    if (result.replicate_count >= 2 && result.design_balanced) {
        append_gage_by_part_plot(page, values, part_values, source_rows);
    }
    page.facts.msa = datalab::domain::statistics::nested_gage_facts_from(result);
    if (page.facts.msa.has_value()) {
        const bool by_part_available =
            result.replicate_count >= 2 && result.design_balanced;
        page.facts.msa->by_part_plot_available = by_part_available;
        if (by_part_available) {
            page.facts.msa->plot_point_count = values.size();
        }
    }
    append_rule_table(page, page.facts.msa->rules);
    domain::apply_evidence(page.method_metadata, result.evidence);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::attribute_agreement(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.msa.attribute_rating_column.has_value()
        || !configuration.msa.attribute_part_column.has_value()
        || !configuration.msa.attribute_appraiser_column.has_value()) {
        return error_page("属性一致性分析", "Attribute Agreement Analysis",
                          "请选择评级、部件和评估者列。");
    }
    const auto ratings = extract_text_column(table, *configuration.msa.attribute_rating_column);
    const auto parts = extract_text_column(table, *configuration.msa.attribute_part_column);
    const auto appraisers = extract_text_column(table, *configuration.msa.attribute_appraiser_column);
    std::vector<std::string> standards;
    if (configuration.msa.attribute_standard_column.has_value()) {
        standards = extract_text_column(table, *configuration.msa.attribute_standard_column);
    }
    const auto result = datalab::domain::statistics::attribute_agreement(
        ratings, parts, appraisers, standards, configuration.inference.confidence_level,
        configuration.msa.ratings_are_ordinal, configuration.msa.kappa_weight_scheme);
    OutputPage page;
    page.id = new_id("attribute_agreement");
    page.title = "属性一致性分析";
    page.method_name = "Attribute Agreement Analysis";
    page.configuration = configuration;
    page.parameter_summary = "部件数 = " + std::to_string(result.item_count)
        + "    评估者数 = " + std::to_string(result.evaluator_count)
        + "    Kappa权重 = " + configuration.msa.kappa_weight_scheme;
    page.diagnostics = result.diagnostics;
    StatisticTable within;
    within.title = "评估者内一致性";
    within.headers = {"Evaluator", "N", "Agreement %", "Kappa", "95% CI", "Method"};
    const auto method_label = [](const std::string& method) {
        if (method == "fleiss") {
            return "Fleiss";
        }
        if (method == "cohen_linear") {
            return "Cohen linear";
        }
        if (method == "cohen_quadratic") {
            return "Cohen quadratic";
        }
        return "Cohen";
    };
    for (const auto& row : result.within_evaluator) {
        within.rows.push_back({row.evaluator, std::to_string(row.estimate.valid_count),
            format_number(row.estimate.agreement_percent), format_number(row.estimate.kappa),
            "[" + format_number(row.estimate.kappa_ci_low) + ", "
                + format_number(row.estimate.kappa_ci_high) + "]",
            method_label(row.estimate.method)});
    }
    page.tables.push_back(std::move(within));
    StatisticTable between;
    between.title = "评估者间一致性";
    between.headers = {"Evaluator 1", "Evaluator 2", "N", "Agreement %", "Kappa", "Method"};
    for (const auto& row : result.between_evaluator) {
        between.rows.push_back({row.first_evaluator, row.second_evaluator,
            std::to_string(row.estimate.valid_count),
            format_number(row.estimate.agreement_percent),
            format_number(row.estimate.kappa),
            method_label(row.estimate.method)});
    }
    page.tables.push_back(std::move(between));
    if (result.overall_available) {
        StatisticTable overall;
        overall.title = "评估者间总体 Kappa（Fleiss）";
        overall.headers = {"N", "Agreement %", "Kappa", "SE", "95% CI", "Method"};
        overall.rows.push_back({
            std::to_string(result.overall.valid_count),
            format_number(result.overall.agreement_percent),
            result.overall.identifiable ? format_number(result.overall.kappa) : "*",
            result.overall.identifiable
                ? format_number(result.overall.kappa_standard_error) : "*",
            result.overall.identifiable
                ? ("[" + format_number(result.overall.kappa_ci_low) + ", "
                   + format_number(result.overall.kappa_ci_high) + "]")
                : "*",
            method_label(result.overall.method)});
        page.tables.push_back(std::move(overall));
    }
    if (result.between_kendall.has_value()) {
        StatisticTable kendall;
        kendall.title = "评估者间 Kendall W";
        kendall.headers = {"Coef", "Chi-Sq", "DF", "P"};
        const auto& estimate = *result.between_kendall;
        kendall.rows.push_back({
            estimate.identifiable ? format_number(estimate.coefficient) : "*",
            estimate.identifiable ? format_number(estimate.chi_square) : "*",
            estimate.identifiable ? format_number(estimate.degrees_of_freedom) : "*",
            estimate.identifiable ? format_number(estimate.p_value) : "*"});
        page.tables.push_back(std::move(kendall));
    }
    if (!result.within_kendall.empty()) {
        StatisticTable kendall;
        kendall.title = "评估者内 Kendall W";
        kendall.headers = {"Evaluator", "Coef", "Chi-Sq", "DF", "P"};
        for (const auto& row : result.within_kendall) {
            kendall.rows.push_back({
                row.evaluator,
                row.estimate.identifiable ? format_number(row.estimate.coefficient) : "*",
                row.estimate.identifiable ? format_number(row.estimate.chi_square) : "*",
                row.estimate.identifiable ? format_number(row.estimate.degrees_of_freedom) : "*",
                row.estimate.identifiable ? format_number(row.estimate.p_value) : "*"});
        }
        page.tables.push_back(std::move(kendall));
    }
    if (!result.against_standard_kendall.empty()) {
        StatisticTable kendall;
        kendall.title = "评估者 vs 标准 Kendall τ";
        kendall.headers = {"Evaluator", "Coef", "SE Coef", "Z", "P"};
        for (const auto& row : result.against_standard_kendall) {
            kendall.rows.push_back({
                row.evaluator,
                row.estimate.identifiable ? format_number(row.estimate.tau) : "*",
                row.estimate.identifiable ? format_number(row.estimate.standard_error) : "*",
                row.estimate.identifiable ? format_number(row.estimate.z) : "*",
                row.estimate.identifiable ? format_number(row.estimate.p_value) : "*"});
        }
        page.tables.push_back(std::move(kendall));
    }
    if (result.overall_kendall.has_value()) {
        StatisticTable kendall;
        kendall.title = "全体 vs 标准 Kendall τ";
        kendall.headers = {"Coef", "SE Coef", "Z", "P"};
        const auto& estimate = *result.overall_kendall;
        kendall.rows.push_back({
            estimate.identifiable ? format_number(estimate.tau) : "*",
            estimate.identifiable ? format_number(estimate.standard_error) : "*",
            estimate.identifiable ? format_number(estimate.z) : "*",
            estimate.identifiable ? format_number(estimate.p_value) : "*"});
        page.tables.push_back(std::move(kendall));
    }
    if (!result.agreement_percent_matrix.empty()) {
        PlotSpec heatmap;
        heatmap.kind = PlotKind::heatmap;
        heatmap.title = "评估者×零件一致率";
        heatmap.x_axis_title = "零件";
        heatmap.y_axis_title = "评估者";
        heatmap.categories = result.agreement_evaluator_labels;
        heatmap.matrix_labels = result.agreement_item_labels;
        heatmap.matrix_values = result.agreement_percent_matrix;
        heatmap.color_min = 0.0;
        heatmap.color_max = 100.0;
        page.plots.push_back(std::move(heatmap));
    }
    if (!result.within_evaluator.empty()) {
        PlotSpec bars;
        bars.kind = PlotKind::pareto;
        bars.title = "评估者一致率";
        bars.x_axis_title = "评估者";
        bars.y_axis_title = "Agreement %";
        for (const auto& row : result.within_evaluator) {
            bars.categories.push_back(row.evaluator);
            bars.category_values.push_back(row.estimate.agreement_percent);
        }
        page.plots.push_back(std::move(bars));
    }
    page.facts.msa = datalab::domain::statistics::attribute_agreement_facts_from(result);
    append_rule_table(page, page.facts.msa->rules);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::seasonal_forecasting(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::size_t value_column = configuration.time_series.decomposition_value_column.value_or(
        configuration.selection.measurement_column);
    const auto extracted = extract_numeric_column(table, value_column, configuration.excluded_rows);
    const auto result = datalab::domain::statistics::fit_seasonal_forecasting(
        extracted.values,
        {configuration.time_series.seasonal_error_model == "multiplicative"
             ? datalab::domain::statistics::SeasonalErrorModel::multiplicative
             : datalab::domain::statistics::SeasonalErrorModel::additive,
         configuration.time_series.seasonal_trend_model == "none"
             ? datalab::domain::statistics::TrendModel::none
             : configuration.time_series.seasonal_trend_model == "multiplicative"
                 ? datalab::domain::statistics::TrendModel::multiplicative
                 : datalab::domain::statistics::TrendModel::additive,
         configuration.time_series.seasonal_period, static_cast<std::size_t>(
             std::max(1, configuration.time_series.forecast_periods)), configuration.time_series.seasonal_damped_trend,
         configuration.time_series.smoothing_alpha, configuration.time_series.seasonal_beta, configuration.time_series.smoothing_gamma,
         configuration.time_series.seasonal_damping_phi, configuration.inference.confidence_level});
    OutputPage page;
    page.id = new_id("seasonal_forecast");
    page.title = "季节性预测";
    page.method_name = "Holt-Winters Seasonal Forecasting";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + column_label(table, value_column)
        + "    周期 = " + std::to_string(configuration.time_series.seasonal_period)
        + "    误差模型 = " + configuration.time_series.seasonal_error_model
        + "    趋势模型 = " + configuration.time_series.seasonal_trend_model
        + "    有效观测 = " + std::to_string(extracted.values.size());
    page.diagnostics = result.diagnostics;
    StatisticTable metrics;
    metrics.title = "预测准确度";
    metrics.headers = {"N", "MAD", "MSD", "MAPE", "RMSE", "MASE"};
    metrics.rows.push_back({std::to_string(result.metrics.count),
        format_number(result.metrics.mad), format_number(result.metrics.msd),
        format_number(result.metrics.mape), format_number(result.metrics.rmse),
        format_number(result.metrics.mase)});
    page.tables.push_back(std::move(metrics));
    StatisticTable detail;
    detail.title = "拟合与预测明细";
    detail.headers = {"序号", "原始行", "实际值", "拟合值", "残差", "Forecast", "Lower", "Upper"};
    for (std::size_t index = 0; index < extracted.values.size(); ++index) {
        detail.rows.push_back({
            std::to_string(index + 1),
            index < extracted.source_rows.size()
                ? std::to_string(extracted.source_rows[index] + 1) : "*",
            format_number(extracted.values[index]),
            index < result.fitted.size() ? format_number(result.fitted[index]) : "*",
            index < result.residuals.size() ? format_number(result.residuals[index]) : "*",
            "", "", ""});
    }
    for (std::size_t index = 0; index < result.forecasts.size(); ++index) {
        detail.rows.push_back({
            std::to_string(extracted.values.size() + index + 1),
            "",
            "",
            "",
            "",
            format_number(result.forecasts[index]),
            index < result.lower.size() ? format_number(result.lower[index]) : "*",
            index < result.upper.size() ? format_number(result.upper[index]) : "*"});
    }
    page.tables.push_back(std::move(detail));
    if (!result.seasonal.empty()) {
        StatisticTable seasonal;
        seasonal.title = "季节指数";
        seasonal.headers = {"Phase", "Seasonal Index"};
        for (std::size_t phase = 0; phase < result.seasonal.size(); ++phase) {
            seasonal.rows.push_back({
                std::to_string(phase + 1),
                format_number(result.seasonal[phase])});
        }
        page.tables.push_back(std::move(seasonal));
    }
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "跳过 " + std::to_string(extracted.missing_count)
                + " 个缺失或非法单元格。"});
    }
    if (configuration.time_series.seasonal_period > 1) {
        const auto sarima_candidates =
            datalab::domain::statistics::fit_best_sarima_candidates(
                extracted.values, configuration.time_series.seasonal_period);
        if (!sarima_candidates.empty()) {
            StatisticTable sarima_table;
            sarima_table.title = "SARIMA 候选模型比较";
            sarima_table.headers = {"模型", "SSE", "AIC", "AICc", "BIC"};
            for (const auto& candidate : sarima_candidates) {
                sarima_table.rows.push_back({
                    datalab::domain::statistics::sarima_order_label(candidate.order),
                    format_number(candidate.sse),
                    format_number(candidate.aic),
                    format_number(candidate.aicc),
                    format_number(candidate.bic)});
            }
            page.tables.push_back(std::move(sarima_table));
            for (const auto& candidate : sarima_candidates) {
                page.diagnostics.insert(page.diagnostics.end(),
                                        candidate.diagnostics.cbegin(),
                                        candidate.diagnostics.cend());
            }
        }
    }
    datalab::domain::statistics::RollingOriginOptions rolling_options;
    rolling_options.initial_training_size =
        configuration.time_series.validation_initial_size;
    rolling_options.horizon = std::max<std::size_t>(
        1, configuration.time_series.validation_horizon);
    rolling_options.step = std::max<std::size_t>(1, configuration.time_series.validation_step);
    rolling_options.model_options = {
        configuration.time_series.seasonal_error_model == "multiplicative"
            ? datalab::domain::statistics::SeasonalErrorModel::multiplicative
            : datalab::domain::statistics::SeasonalErrorModel::additive,
        configuration.time_series.seasonal_trend_model == "none"
            ? datalab::domain::statistics::TrendModel::none
            : configuration.time_series.seasonal_trend_model == "multiplicative"
                ? datalab::domain::statistics::TrendModel::multiplicative
                : datalab::domain::statistics::TrendModel::additive,
        configuration.time_series.seasonal_period,
        rolling_options.horizon,
        configuration.time_series.seasonal_damped_trend,
        configuration.time_series.smoothing_alpha,
        configuration.time_series.seasonal_beta,
        configuration.time_series.smoothing_gamma,
        configuration.time_series.seasonal_damping_phi,
        configuration.inference.confidence_level};
    const auto rolling_validation =
        datalab::domain::statistics::rolling_origin_validate(
            extracted.values, rolling_options);
    page.diagnostics.insert(page.diagnostics.end(),
                            rolling_validation.diagnostics.cbegin(),
                            rolling_validation.diagnostics.cend());
    if (rolling_validation.metrics.count > 0) {
        StatisticTable rolling_table;
        rolling_table.title = "Rolling-origin 评估";
        rolling_table.headers = {"N", "MAD", "MSD", "MAPE", "RMSE", "MASE"};
        rolling_table.rows.push_back({
            std::to_string(rolling_validation.metrics.count),
            format_number(rolling_validation.metrics.mad),
            format_number(rolling_validation.metrics.msd),
            format_number(rolling_validation.metrics.mape),
            format_number(rolling_validation.metrics.rmse),
            format_number(rolling_validation.metrics.mase)});
        page.tables.push_back(std::move(rolling_table));
        StatisticTable rolling_detail;
        rolling_detail.title = "Rolling-origin 明细";
        rolling_detail.headers = {"Origin", "Horizon", "Actual", "Forecast", "Lower", "Upper"};
        const std::size_t horizon = std::max<std::size_t>(
            1, configuration.time_series.validation_horizon);
        for (std::size_t index = 0; index < rolling_validation.actuals.size(); ++index) {
            const std::size_t origin_index = horizon > 0 ? index / horizon : 0;
            const std::size_t horizon_step = horizon > 0 ? (index % horizon) + 1 : 1;
            rolling_detail.rows.push_back({
                origin_index < rolling_validation.origins.size()
                    ? std::to_string(rolling_validation.origins[origin_index]) : "*",
                std::to_string(horizon_step),
                format_number(rolling_validation.actuals[index]),
                index < rolling_validation.forecasts.size()
                    ? format_number(rolling_validation.forecasts[index]) : "*",
                index < rolling_validation.lower.size()
                    ? format_number(rolling_validation.lower[index]) : "*",
                index < rolling_validation.upper.size()
                    ? format_number(rolling_validation.upper[index]) : "*"});
        }
        page.tables.push_back(std::move(rolling_detail));
    }
    page.facts.forecast = domain::ForecastFacts{
        result.metrics.mape, result.metrics.mase,
        rolling_validation.metrics.count > 0
            ? std::optional<double>(rolling_validation.metrics.mape)
            : std::nullopt,
        rolling_validation.metrics.count > 0
            ? std::optional<double>(rolling_validation.metrics.mase)
            : std::nullopt};
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "季节性拟合与预测";
    plot.x_axis_title = "Order";
    plot.y_axis_title = column_label(table, value_column);
    datalab::domain::PlotSeries actual;
    actual.role = datalab::domain::PlotSeriesRole::actual;
    actual.label = "实际值";
    actual.values = extracted.values;
    actual.show_points = true;
    actual.x_values.resize(extracted.values.size());
    for (std::size_t index = 0; index < actual.x_values.size(); ++index) {
        actual.x_values[index] = static_cast<double>(index + 1);
    }
    datalab::domain::PlotSeries fitted;
    fitted.role = datalab::domain::PlotSeriesRole::fitted;
    fitted.label = "拟合值";
    fitted.values = result.fitted;
    fitted.x_values.resize(result.fitted.size());
    for (std::size_t index = 0; index < fitted.x_values.size(); ++index) {
        fitted.x_values[index] = static_cast<double>(index + 1);
    }
    datalab::domain::PlotSeries forecast;
    forecast.role = datalab::domain::PlotSeriesRole::forecast;
    forecast.label = "预测值";
    forecast.values = result.forecasts;
    forecast.lower = result.lower;
    forecast.upper = result.upper;
    forecast.x_values.resize(result.forecasts.size());
    for (std::size_t index = 0; index < forecast.x_values.size(); ++index) {
        forecast.x_values[index] = static_cast<double>(extracted.values.size() + index + 1);
    }
    datalab::domain::PlotSeries confidence;
    confidence.role = datalab::domain::PlotSeriesRole::confidence_band;
    confidence.label = "预测区间";
    confidence.lower = result.lower;
    confidence.upper = result.upper;
    confidence.x_values = forecast.x_values;
    plot.source_rows = extracted.source_rows;
    plot.series = {std::move(actual), std::move(fitted), std::move(forecast), std::move(confidence)};
    page.plots.push_back(std::move(plot));
    page.method_metadata.estimation_method =
        configuration.time_series.seasonal_error_model == "multiplicative"
            ? "holt_winters_multiplicative" : "holt_winters_additive";
    page.method_metadata.parameter_source = "specified";
    page.method_metadata.valid_count = extracted.values.size();
    page.method_metadata.missing_count = extracted.missing_count;
    page.method_metadata.source_rows.clear();
    page.method_metadata.source_rows.reserve(extracted.source_rows.size());
    for (const std::size_t row : extracted.source_rows) {
        page.method_metadata.source_rows.push_back(static_cast<domain::RowId>(row));
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::pca(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::vector<std::size_t>& columns = configuration.pca.variable_columns.empty()
        ? configuration.variable_columns : configuration.pca.variable_columns;
    if (columns.size() < 2) {
        return error_page("PCA 主成分分析", "Principal Component Analysis",
                          "至少需要选择两个数值变量。");
    }
    std::vector<std::vector<double>> rows;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column : columns) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (valid) {
            rows.push_back(std::move(values));
            source_rows.push_back(row);
        }
    }
    const auto result = datalab::domain::statistics::principal_component_analysis(
        rows, {configuration.pca.mode == "standardized"
                   ? datalab::domain::statistics::PcaMode::standardized
                   : datalab::domain::statistics::PcaMode::covariance,
               configuration.pca.component_count, 100, 1.0e-10,
               configuration.pca.anomaly_quantile});
    OutputPage page;
    page.id = new_id("pca");
    page.title = "主成分分析";
    page.method_name = "Principal Component Analysis";
    page.configuration = configuration;
    page.parameter_summary = "观测数 = " + std::to_string(result.observation_count)
        + "    变量数 = " + std::to_string(result.variable_count);
    page.diagnostics = result.diagnostics;
    StatisticTable eigen;
    eigen.title = "特征值与解释率";
    eigen.headers = {"Component", "Eigenvalue", "Proportion", "Cumulative"};
    for (std::size_t index = 0; index < result.eigenvalues.size(); ++index) {
        eigen.rows.push_back({std::to_string(index + 1), format_number(result.eigenvalues[index]),
            format_number(result.explained_variance_ratio[index]),
            format_number(result.cumulative_explained_variance_ratio[index])});
    }
    page.tables.push_back(std::move(eigen));
    StatisticTable coefficients;
    coefficients.title = "主成分系数";
    coefficients.headers = {"Variable"};
    for (std::size_t index = 0; index < result.retained_component_count; ++index) {
        coefficients.headers.push_back("PC" + std::to_string(index + 1));
    }
    for (std::size_t variable = 0; variable < result.coefficients.size(); ++variable) {
        std::vector<std::string> row = {column_label(table, columns[variable])};
        for (std::size_t component = 0;
             component < result.coefficients[variable].size(); ++component) {
            row.push_back(format_number(result.coefficients[variable][component]));
        }
        coefficients.rows.push_back(std::move(row));
    }
    page.tables.push_back(std::move(coefficients));
    StatisticTable loadings;
    loadings.title = "相关载荷";
    loadings.headers = {"Variable"};
    for (std::size_t index = 0; index < result.retained_component_count; ++index) {
        loadings.headers.push_back("PC" + std::to_string(index + 1));
    }
    for (std::size_t variable = 0; variable < result.loadings.size(); ++variable) {
        std::vector<std::string> row = {column_label(table, columns[variable])};
        for (std::size_t component = 0;
             component < result.loadings[variable].size(); ++component) {
            row.push_back(format_number(result.loadings[variable][component]));
        }
        loadings.rows.push_back(std::move(row));
    }
    page.tables.push_back(std::move(loadings));
    StatisticTable scores;
    scores.title = "主成分得分";
    scores.headers = {"原始行"};
    for (std::size_t index = 0; index < result.retained_component_count; ++index) {
        scores.headers.push_back("PC" + std::to_string(index + 1));
    }
    for (std::size_t observation = 0; observation < result.scores.size(); ++observation) {
        const std::size_t source_index = observation < result.valid_rows.size()
            ? result.valid_rows[observation] : observation;
        std::vector<std::string> row = {
            source_index < source_rows.size()
                ? std::to_string(source_rows[source_index] + 1) : "*"};
        for (double score : result.scores[observation]) {
            row.push_back(format_number(score));
        }
        scores.rows.push_back(std::move(row));
    }
    page.tables.push_back(std::move(scores));
    if (result.converged && !result.hotelling_t2.empty()) {
        StatisticTable limits;
        limits.title = "T² 与 Q 阈值";
        limits.headers = {"分位数", "T² 限", "Q 限", "口径"};
        limits.rows.push_back({
            format_number(configuration.pca.anomaly_quantile),
            format_number(result.hotelling_t2_limit),
            format_number(result.q_residual_limit),
            "经验分位（非 Minitab T² 控制图 UCL）"});
        page.tables.push_back(std::move(limits));
        StatisticTable diagnostics;
        diagnostics.title = "T² 与 Q 残差";
        diagnostics.headers = {"原始行", "Hotelling T²", "T² 异常", "Q 残差", "Q 异常", "综合异常"};
        for (std::size_t index = 0; index < result.hotelling_t2.size(); ++index) {
            const std::size_t source_index = index < result.valid_rows.size()
                ? result.valid_rows[index] : index;
            diagnostics.rows.push_back({
                source_index < source_rows.size()
                    ? std::to_string(source_rows[source_index] + 1) : "*",
                format_number(result.hotelling_t2[index]),
                index < result.hotelling_t2_anomaly.size() && result.hotelling_t2_anomaly[index]
                    ? "是" : "否",
                index < result.q_residuals.size() ? format_number(result.q_residuals[index]) : "*",
                index < result.q_residual_anomaly.size() && result.q_residual_anomaly[index]
                    ? "是" : "否",
                index < result.anomaly.size() && result.anomaly[index] ? "是" : "否"});
        }
        page.tables.push_back(std::move(diagnostics));
    }
    domain::PcaFacts pca_facts;
    pca_facts.mode = configuration.pca.mode;
    pca_facts.retained_component_count = result.retained_component_count;
    pca_facts.anomaly_count = static_cast<std::size_t>(
        std::count(result.anomaly.cbegin(), result.anomaly.cend(), true));
    pca_facts.observation_count = result.observation_count;
    pca_facts.converged = result.converged;
    if (result.converged && !result.hotelling_t2.empty()) {
        pca_facts.t2_limit = result.hotelling_t2_limit;
        pca_facts.q_limit = result.q_residual_limit;
        if (!result.q_residuals.empty()) {
            const auto residual_normality =
                datalab::domain::statistics::normality_test(result.q_residuals);
            pca_facts.residual_ad_p = residual_normality.p_value;
        }
    }
    page.facts.pca = pca_facts;
    if (result.scores.size() >= 2 && result.retained_component_count >= 2) {
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "PCA 得分图";
        plot.x_axis_title = "PC1";
        plot.y_axis_title = "PC2";
        for (const auto& score : result.scores) {
            plot.x_values.push_back(score[0]);
            plot.values.push_back(score[1]);
        }
        plot.source_rows.clear();
        for (std::size_t observation = 0; observation < result.scores.size(); ++observation) {
            const std::size_t source_index = observation < result.valid_rows.size()
                ? result.valid_rows[observation] : observation;
            plot.source_rows.push_back(
                source_index < source_rows.size() ? source_rows[source_index] : source_index);
        }
        page.plots.push_back(std::move(plot));
    }
    if (page.facts.pca.has_value()) {
        page.facts.pca->diagnostic_plot_count = page.plots.size();
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::kmeans(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::vector<std::size_t>& columns = configuration.kmeans.variable_columns.empty()
        ? configuration.variable_columns : configuration.kmeans.variable_columns;
    if (columns.size() < 2) {
        return error_page("K-Means 聚类", "Cluster K-Means",
                          "至少需要选择两个数值变量。");
    }
    if (configuration.kmeans.cluster_count < 2) {
        return error_page("K-Means 聚类", "Cluster K-Means", "k 必须 ≥ 2。");
    }
    std::vector<std::vector<double>> rows;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column : columns) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (valid) {
            rows.push_back(std::move(values));
            source_rows.push_back(row);
        }
    }
    const auto result = datalab::domain::statistics::cluster_kmeans(
        rows,
        {configuration.kmeans.cluster_count, configuration.kmeans.max_iterations,
         configuration.kmeans.standardize});
    OutputPage page;
    page.id = new_id("kmeans");
    page.title = "K-Means 聚类";
    page.method_name = "Cluster K-Means";
    page.configuration = configuration;
    page.parameter_summary = "k = " + std::to_string(result.cluster_count)
        + "    N = " + std::to_string(result.observation_count)
        + "    迭代 = " + std::to_string(result.iterations)
        + (result.standardized ? "    标准化" : "");
    page.diagnostics = result.diagnostics;
    if (result.observation_count == 0 || result.centroids.empty()) {
        return finalize_page(std::move(page));
    }

    StatisticTable summary;
    summary.title = "簇摘要";
    summary.headers = {"Cluster", "Size", "Within SS"};
    for (std::size_t cluster = 0; cluster < result.cluster_count; ++cluster) {
        summary.rows.push_back({
            std::to_string(cluster + 1),
            cluster < result.cluster_sizes.size()
                ? std::to_string(result.cluster_sizes[cluster]) : "*",
            cluster < result.within_ss.size()
                ? format_number(result.within_ss[cluster]) : "*"});
    }
    summary.rows.push_back({"Total", std::to_string(result.observation_count),
                            format_number(result.total_within_ss)});
    page.tables.push_back(std::move(summary));

    StatisticTable centroids;
    centroids.title = "质心（分析尺度）";
    centroids.headers = {"Cluster"};
    for (const std::size_t column : columns) {
        centroids.headers.push_back(column_label(table, column));
    }
    for (std::size_t cluster = 0; cluster < result.centroids.size(); ++cluster) {
        std::vector<std::string> row = {std::to_string(cluster + 1)};
        for (double value : result.centroids[cluster]) {
            row.push_back(format_number(value));
        }
        centroids.rows.push_back(std::move(row));
    }
    page.tables.push_back(std::move(centroids));

    StatisticTable assignments;
    assignments.title = "簇分配";
    assignments.headers = {"原始行", "Cluster", "Distance"};
    for (std::size_t index = 0; index < result.assignments.size(); ++index) {
        const std::size_t source_index = index < result.valid_rows.size()
            ? result.valid_rows[index] : index;
        assignments.rows.push_back({
            source_index < source_rows.size()
                ? std::to_string(source_rows[source_index] + 1) : "*",
            std::to_string(result.assignments[index] + 1),
            index < result.distances_to_centroid.size()
                ? format_number(result.distances_to_centroid[index]) : "*"});
    }
    page.tables.push_back(std::move(assignments));

    if (columns.size() >= 2 && !rows.empty()) {
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "K-Means 散点（前两列）";
        plot.x_axis_title = column_label(table, columns[0]);
        plot.y_axis_title = column_label(table, columns[1]);
        for (std::size_t index = 0; index < rows.size(); ++index) {
            plot.x_values.push_back(rows[index][0]);
            plot.values.push_back(rows[index][1]);
            const std::size_t source_index = index < result.valid_rows.size()
                ? result.valid_rows[index] : index;
            plot.source_rows.push_back(
                source_index < source_rows.size() ? source_rows[source_index] : source_index);
            PlotSeries point;
            point.label = "C" + std::to_string(
                index < result.assignments.size() ? result.assignments[index] + 1 : 0);
            point.role = PlotSeriesRole::generic;
            point.x_values = {rows[index][0]};
            point.values = {rows[index][1]};
            plot.series.push_back(std::move(point));
        }
        page.plots.push_back(std::move(plot));
    }

    domain::KMeansFacts facts;
    facts.k = result.cluster_count;
    facts.n = result.observation_count;
    facts.variable_count = result.variable_count;
    facts.iterations = result.iterations;
    facts.converged = result.converged;
    facts.standardized = result.standardized;
    facts.total_within_ss = result.total_within_ss;
    page.facts.kmeans = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::cart_tree(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.cart_tree.response_column.has_value()
        || configuration.cart_tree.predictor_columns.empty()) {
        return error_page("CART 单树", "CART Tree",
                          "请选择响应列与至少一个数值预测列。");
    }
    const bool is_regression = configuration.cart_tree.task == "regression";
    const std::size_t response_column = *configuration.cart_tree.response_column;
    const auto& predictors_cols = configuration.cart_tree.predictor_columns;

    std::vector<std::string> class_labels;
    std::map<std::string, std::size_t> class_index;
    if (!is_regression) {
        for (std::size_t row = 0; row < table.rows.size(); ++row) {
            if (std::find(configuration.excluded_rows.cbegin(),
                          configuration.excluded_rows.cend(), row)
                != configuration.excluded_rows.cend()) {
                continue;
            }
            if (response_column >= table.rows[row].size()) {
                continue;
            }
            const std::string& cell = table.rows[row][response_column];
            if (is_missing_cell(cell)) {
                continue;
            }
            if (class_index.find(cell) == class_index.end()) {
                class_index[cell] = class_labels.size();
                class_labels.push_back(cell);
            }
        }
        if (class_labels.size() < 2) {
            return error_page("CART 单树", "CART Tree",
                              "分类任务需要至少两个响应类别。");
        }
    }

    std::vector<std::vector<double>> predictors;
    std::vector<double> response;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (response_column >= table.rows[row].size()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column : predictors_cols) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (!valid) {
            continue;
        }
        if (is_regression) {
            const auto parsed = parse_numeric_cell(table.rows[row][response_column]);
            if (!parsed.has_value()) {
                continue;
            }
            response.push_back(*parsed);
        } else {
            const std::string& cell = table.rows[row][response_column];
            if (is_missing_cell(cell) || class_index.find(cell) == class_index.end()) {
                continue;
            }
            response.push_back(static_cast<double>(class_index[cell]));
        }
        predictors.push_back(std::move(values));
        source_rows.push_back(row);
    }

    std::vector<std::string> predictor_names;
    for (const std::size_t column : predictors_cols) {
        predictor_names.push_back(column_label(table, column));
    }
    datalab::domain::statistics::CartTreeOptions options;
    options.task = is_regression
        ? datalab::domain::statistics::CartTask::regression
        : datalab::domain::statistics::CartTask::classification;
    options.max_depth = configuration.cart_tree.max_depth;
    options.min_leaf = configuration.cart_tree.min_leaf;
    const auto result = datalab::domain::statistics::fit_cart_tree(
        predictors, response, class_labels, predictor_names, options);

    OutputPage page;
    page.id = new_id("cart_tree");
    page.title = "CART 单树";
    page.method_name = "CART Tree";
    page.configuration = configuration;
    page.parameter_summary = std::string("任务 = ") + configuration.cart_tree.task
        + "    N = " + std::to_string(result.observation_count)
        + "    深度上限 = " + std::to_string(result.max_depth)
        + "    叶数 = " + std::to_string(result.leaf_count);
    page.diagnostics = result.diagnostics;
    if (result.nodes.empty()) {
        return finalize_page(std::move(page));
    }

    StatisticTable tree;
    tree.title = "树结点";
    tree.headers = {"Node", "Parent", "Depth", "Leaf", "N", "Impurity", "Prediction",
                    "SplitVar", "Threshold"};
    for (const auto& node : result.nodes) {
        tree.rows.push_back({
            std::to_string(node.id),
            node.parent_id < 0 ? "*" : std::to_string(node.parent_id),
            std::to_string(node.depth),
            node.is_leaf ? "是" : "否",
            std::to_string(node.n),
            format_number(node.impurity),
            node.prediction_label.empty() ? format_number(node.prediction)
                                          : node.prediction_label,
            node.split_variable.has_value()
                && *node.split_variable < predictor_names.size()
                ? predictor_names[*node.split_variable] : "*",
            node.split_threshold.has_value() ? format_number(*node.split_threshold)
                                            : "*"});
    }
    page.tables.push_back(std::move(tree));

    StatisticTable importance;
    importance.title = "变量重要性";
    importance.headers = {"Variable", "Importance"};
    for (std::size_t index = 0; index < result.variable_importance.size(); ++index) {
        importance.rows.push_back({
            index < predictor_names.size() ? predictor_names[index]
                                           : ("X" + std::to_string(index + 1)),
            format_number(result.variable_importance[index])});
    }
    page.tables.push_back(std::move(importance));

    if (!is_regression && !result.confusion.empty()) {
        StatisticTable confusion;
        confusion.title = "训练集混淆矩阵";
        confusion.headers = {"Actual \\ Predicted"};
        for (const std::string& label : class_labels) {
            confusion.headers.push_back(label);
        }
        for (std::size_t actual = 0; actual < result.confusion.size(); ++actual) {
            std::vector<std::string> row = {
                actual < class_labels.size() ? class_labels[actual] : "?"};
            for (std::size_t predicted = 0;
                 predicted < result.confusion[actual].size(); ++predicted) {
                row.push_back(std::to_string(result.confusion[actual][predicted]));
            }
            confusion.rows.push_back(std::move(row));
        }
        page.tables.push_back(std::move(confusion));
    }

    if (is_regression) {
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "观测 vs 拟合";
        plot.x_axis_title = "观测";
        plot.y_axis_title = "拟合";
        for (std::size_t index = 0; index < result.valid_rows.size(); ++index) {
            const std::size_t local = result.valid_rows[index];
            if (local >= response.size() || local >= result.fitted.size()) {
                continue;
            }
            plot.x_values.push_back(response[local]);
            plot.values.push_back(result.fitted[local]);
            plot.source_rows.push_back(
                local < source_rows.size() ? source_rows[local] : local);
        }
        page.plots.push_back(std::move(plot));
    }

    domain::CartTreeFacts facts;
    facts.task = configuration.cart_tree.task;
    facts.n = result.observation_count;
    facts.predictor_count = result.predictor_count;
    facts.max_depth = result.max_depth;
    facts.node_count = result.node_count;
    facts.leaf_count = result.leaf_count;
    facts.train_metric = result.train_metric;
    facts.top_variable = result.top_variable;
    page.facts.cart_tree = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::adf_test(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::size_t column = configuration.adf.series_column.has_value()
        ? *configuration.adf.series_column
        : (configuration.variable_columns.empty()
               ? static_cast<std::size_t>(-1)
               : configuration.variable_columns.front());
    if (column == static_cast<std::size_t>(-1)) {
        return error_page("ADF 单位根", "Augmented Dickey-Fuller",
                          "请选择一个数值序列列。");
    }
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, column, configuration.excluded_rows);
    datalab::domain::statistics::AdfOptions options;
    options.lags = configuration.adf.lags;
    if (configuration.adf.regression == "none") {
        options.regression = datalab::domain::statistics::AdfRegression::none;
    } else if (configuration.adf.regression == "trend") {
        options.regression = datalab::domain::statistics::AdfRegression::trend;
    } else {
        options.regression = datalab::domain::statistics::AdfRegression::drift;
    }
    const auto result =
        datalab::domain::statistics::augmented_dickey_fuller(extracted.values, options);

    OutputPage page;
    page.id = new_id("adf_test");
    page.title = "ADF 单位根检验";
    page.method_name = "Augmented Dickey-Fuller";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    N = " + std::to_string(result.n)
        + "    lags = " + std::to_string(result.lags)
        + "    回归 = " + configuration.adf.regression;
    page.diagnostics = result.diagnostics;
    if (extracted.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            extracted.name + " 跳过 " + std::to_string(extracted.missing_count)
                + " 个缺失或非法单元格。"});
    }

    StatisticTable test;
    test.title = "ADF 检验";
    test.headers = {"Statistic", "Value"};
    test.rows.push_back({"tau", format_optional(result.tau)});
    test.rows.push_back({"Critical 1%", format_optional(result.critical_1)});
    test.rows.push_back({"Critical 5%", format_optional(result.critical_5)});
    test.rows.push_back({"Critical 10%", format_optional(result.critical_10)});
    test.rows.push_back({
        "Reject unit root at 5%", result.reject_unit_root_at_5 ? "是" : "否"});
    test.rows.push_back({"Used observations", std::to_string(result.used_observations)});
    page.tables.push_back(std::move(test));

    if (!result.coefficients.empty()) {
        StatisticTable coefficients;
        coefficients.title = "ADF 回归系数";
        coefficients.headers = {"Term", "Estimate", "SE", "t"};
        for (const auto& coefficient : result.coefficients) {
            coefficients.rows.push_back({
                coefficient.name,
                format_number(coefficient.estimate),
                format_number(coefficient.standard_error),
                format_number(coefficient.t_statistic)});
        }
        page.tables.push_back(std::move(coefficients));
    }

    PlotSpec plot;
    plot.kind = PlotKind::time_series;
    plot.title = "序列";
    plot.x_axis_title = "Index";
    plot.y_axis_title = extracted.name;
    for (std::size_t index = 0; index < extracted.values.size(); ++index) {
        plot.x_values.push_back(static_cast<double>(index + 1));
        plot.values.push_back(extracted.values[index]);
        if (index < extracted.source_rows.size()) {
            plot.source_rows.push_back(extracted.source_rows[index]);
        }
    }
    page.plots.push_back(std::move(plot));

    domain::AdfFacts facts;
    facts.n = result.n;
    facts.missing_count = result.missing_count + extracted.missing_count;
    facts.lags = result.lags;
    facts.used_observations = result.used_observations;
    facts.regression = configuration.adf.regression;
    facts.tau = result.tau;
    facts.critical_5 = result.critical_5;
    facts.reject_unit_root_at_5 = result.reject_unit_root_at_5;
    page.facts.adf = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::poisson_regression(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.poisson_regression.response_column.has_value()
        || configuration.poisson_regression.predictor_columns.empty()) {
        return error_page("Poisson 回归", "Poisson Regression",
                          "请选择计数响应列与至少一个数值预测列。");
    }
    const std::size_t response_column = *configuration.poisson_regression.response_column;
    const auto& predictors_cols = configuration.poisson_regression.predictor_columns;
    std::vector<std::vector<double>> predictors;
    std::vector<double> response;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (response_column >= table.rows[row].size()) {
            continue;
        }
        const auto y = parse_numeric_cell(table.rows[row][response_column]);
        if (!y.has_value() || *y < 0.0) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column : predictors_cols) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (!valid) {
            continue;
        }
        response.push_back(*y);
        predictors.push_back(std::move(values));
        source_rows.push_back(row);
    }
    std::vector<std::string> labels;
    for (const std::size_t column : predictors_cols) {
        labels.push_back(column_label(table, column));
    }
    const auto result = datalab::domain::statistics::fit_poisson_regression(
        response, predictors, labels, configuration.inference.confidence_level,
        configuration.poisson_regression.max_iterations,
        configuration.poisson_regression.tolerance);
    OutputPage page;
    page.id = new_id("poisson_regression");
    page.title = "Poisson 回归";
    page.method_name = "Poisson Regression";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    预测变量 = " + std::to_string(result.predictor_count)
        + "    链 = log";
    page.diagnostics = result.diagnostics;
    StatisticTable summary;
    summary.title = "模型摘要";
    summary.headers = {"N", "迭代", "收敛", "LogLik", "Deviance", "AIC"};
    summary.rows.push_back({
        std::to_string(result.observation_count),
        std::to_string(result.iteration_count),
        result.converged ? "是" : "否",
        format_number(result.log_likelihood),
        format_number(result.deviance),
        format_number(result.aic)});
    page.tables.push_back(std::move(summary));
    StatisticTable coefficients;
    coefficients.title = "系数";
    coefficients.headers = {"Term", "Coef", "SE", "Z", "P", "CI Lower", "CI Upper"};
    for (const auto& coefficient : result.coefficients) {
        coefficients.rows.push_back({
            coefficient.term,
            format_number(coefficient.coefficient),
            format_number(coefficient.standard_error),
            format_number(coefficient.z_statistic),
            format_number(coefficient.p_value),
            format_number(coefficient.confidence_lower),
            format_number(coefficient.confidence_upper)});
    }
    page.tables.push_back(std::move(coefficients));
    StatisticTable fitted;
    fitted.title = "拟合与 Pearson 残差";
    fitted.headers = {"原始行", "Y", "Fitted", "Pearson"};
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        fitted.rows.push_back({
            index < source_rows.size() ? std::to_string(source_rows[index] + 1) : "*",
            format_number(result.observations[index].response),
            format_number(result.observations[index].fitted),
            format_number(result.observations[index].pearson_residual)});
    }
    page.tables.push_back(std::move(fitted));
    if (!result.observations.empty()) {
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "拟合 vs Pearson 残差";
        plot.x_axis_title = "Fitted";
        plot.y_axis_title = "Pearson";
        for (std::size_t index = 0; index < result.observations.size(); ++index) {
            plot.x_values.push_back(result.observations[index].fitted);
            plot.values.push_back(result.observations[index].pearson_residual);
            if (index < source_rows.size()) {
                plot.source_rows.push_back(source_rows[index]);
            }
        }
        page.plots.push_back(std::move(plot));
    }
    domain::PoissonRegressionFacts facts;
    facts.n = result.observation_count;
    facts.predictor_count = result.predictor_count;
    facts.iteration_count = result.iteration_count;
    facts.converged = result.converged;
    facts.deviance = result.deviance;
    facts.aic = result.aic;
    page.facts.poisson_regression = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::isolation_forest(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::vector<std::size_t>& columns =
        configuration.isolation_forest.variable_columns.empty()
        ? configuration.variable_columns
        : configuration.isolation_forest.variable_columns;
    if (columns.size() < 2) {
        return error_page("Isolation Forest", "Isolation Forest",
                          "至少需要两个数值变量（多元异常）。");
    }
    std::vector<std::vector<double>> rows;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column : columns) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (valid) {
            rows.push_back(std::move(values));
            source_rows.push_back(row);
        }
    }
    const auto result = datalab::domain::statistics::isolation_forest(
        rows,
        {configuration.isolation_forest.tree_count,
         configuration.isolation_forest.max_samples,
         configuration.isolation_forest.seed,
         configuration.isolation_forest.score_quantile});
    OutputPage page;
    page.id = new_id("isolation_forest");
    page.title = "Isolation Forest";
    page.method_name = "Isolation Forest";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    trees = " + std::to_string(result.tree_count)
        + "    异常数 = " + std::to_string(result.anomaly_count);
    page.diagnostics = result.diagnostics;
    StatisticTable scores;
    scores.title = "异常分数";
    scores.headers = {"原始行", "Score", "Anomaly"};
    for (std::size_t index = 0; index < result.scores.size(); ++index) {
        const std::size_t source_index = index < result.valid_rows.size()
            ? result.valid_rows[index] : index;
        scores.rows.push_back({
            source_index < source_rows.size()
                ? std::to_string(source_rows[source_index] + 1) : "*",
            format_number(result.scores[index]),
            index < result.anomaly.size() && result.anomaly[index] ? "是" : "否"});
    }
    page.tables.push_back(std::move(scores));
    PlotSpec plot;
    plot.kind = PlotKind::time_series;
    plot.title = "Isolation 分数";
    plot.x_axis_title = "Index";
    plot.y_axis_title = "Score";
    for (std::size_t index = 0; index < result.scores.size(); ++index) {
        plot.x_values.push_back(static_cast<double>(index + 1));
        plot.values.push_back(result.scores[index]);
        const std::size_t source_index = index < result.valid_rows.size()
            ? result.valid_rows[index] : index;
        plot.source_rows.push_back(
            source_index < source_rows.size() ? source_rows[source_index] : source_index);
    }
    PlotSeries threshold;
    threshold.label = "阈值";
    threshold.role = PlotSeriesRole::confidence_band;
    threshold.x_values = plot.x_values;
    threshold.values.assign(plot.x_values.size(), result.score_threshold);
    threshold.style.point_style = PlotPointStyle::none;
    plot.series.push_back(std::move(threshold));
    page.plots.push_back(std::move(plot));
    domain::IsolationForestFacts facts;
    facts.n = result.observation_count;
    facts.variable_count = result.variable_count;
    facts.tree_count = result.tree_count;
    facts.anomaly_count = result.anomaly_count;
    facts.score_threshold = result.score_threshold;
    page.facts.isolation_forest = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::bootstrap_mean(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::size_t column = configuration.bootstrap_mean.series_column.has_value()
        ? *configuration.bootstrap_mean.series_column
        : (configuration.variable_columns.empty()
               ? static_cast<std::size_t>(-1)
               : configuration.variable_columns.front());
    if (column == static_cast<std::size_t>(-1)) {
        return error_page("Bootstrap 均值 CI", "Bootstrap Mean CI",
                          "请选择一个数值列。");
    }
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, column, configuration.excluded_rows);
    const auto result = datalab::domain::statistics::bootstrap_mean_ci(
        extracted.values,
        {configuration.bootstrap_mean.replicates,
         configuration.bootstrap_mean.confidence_level > 0.0
             ? configuration.bootstrap_mean.confidence_level
             : configuration.inference.confidence_level,
         configuration.bootstrap_mean.seed});
    OutputPage page;
    page.id = new_id("bootstrap_mean");
    page.title = "Bootstrap 均值置信区间";
    page.method_name = "Bootstrap Mean CI";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    N = " + std::to_string(result.n)
        + "    B = " + std::to_string(result.replicates)
        + "    方法 = percentile";
    page.diagnostics = result.diagnostics;
    StatisticTable summary;
    summary.title = "Bootstrap 摘要";
    summary.headers = {"Statistic", "Value"};
    summary.rows.push_back({"Sample Mean", format_optional(result.sample_mean)});
    summary.rows.push_back({"CI Lower", format_optional(result.ci_lower)});
    summary.rows.push_back({"CI Upper", format_optional(result.ci_upper)});
    summary.rows.push_back({"Confidence", format_number(result.confidence_level)});
    summary.rows.push_back({"Replicates", std::to_string(result.replicates)});
    page.tables.push_back(std::move(summary));
    if (!result.bootstrap_means.empty()) {
        PlotSpec plot;
        plot.kind = PlotKind::histogram;
        plot.title = "Bootstrap 均值分布";
        plot.x_axis_title = "Mean*";
        plot.y_axis_title = "Count";
        plot.values = result.bootstrap_means;
        page.plots.push_back(std::move(plot));
    }
    domain::BootstrapMeanFacts facts;
    facts.n = result.n;
    facts.replicates = result.replicates;
    facts.method = result.method;
    facts.sample_mean = result.sample_mean;
    facts.ci_lower = result.ci_lower;
    facts.ci_upper = result.ci_upper;
    facts.confidence_level = result.confidence_level;
    page.facts.bootstrap_mean = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::bootstrap_two_sample(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::size_t first_column =
        configuration.bootstrap_two_sample.first_column.has_value()
        ? *configuration.bootstrap_two_sample.first_column
        : (configuration.variable_columns.size() >= 1
               ? configuration.variable_columns[0]
               : static_cast<std::size_t>(-1));
    const std::size_t second_column =
        configuration.bootstrap_two_sample.second_column.has_value()
        ? *configuration.bootstrap_two_sample.second_column
        : (configuration.variable_columns.size() >= 2
               ? configuration.variable_columns[1]
               : static_cast<std::size_t>(-1));
    if (first_column == static_cast<std::size_t>(-1)
        || second_column == static_cast<std::size_t>(-1)) {
        return error_page("Bootstrap 双样本均值差 CI", "Bootstrap Two-Sample Mean Difference CI",
                          "请选择两个数值列。");
    }
    const ExtractedNumericColumn first = extract_numeric_column(
        table, first_column, configuration.excluded_rows);
    const ExtractedNumericColumn second = extract_numeric_column(
        table, second_column, configuration.excluded_rows);
    const auto result = datalab::domain::statistics::bootstrap_two_sample_mean_difference_ci(
        first.values,
        second.values,
        {configuration.bootstrap_two_sample.replicates,
         configuration.bootstrap_two_sample.confidence_level > 0.0
             ? configuration.bootstrap_two_sample.confidence_level
             : configuration.inference.confidence_level,
         configuration.bootstrap_two_sample.seed});
    OutputPage page;
    page.id = new_id("bootstrap_two_sample");
    page.title = "Bootstrap 双样本均值差置信区间";
    page.method_name = "Bootstrap Two-Sample Mean Difference CI";
    page.configuration = configuration;
    page.parameter_summary = "样本 1 = " + first.name
        + "    样本 2 = " + second.name
        + "    N1 = " + std::to_string(result.n_first)
        + "    N2 = " + std::to_string(result.n_second)
        + "    B = " + std::to_string(result.replicates)
        + "    方法 = percentile";
    page.diagnostics = result.diagnostics;
    if (result.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "Bootstrap 双样本分析跳过缺失或非法单元格；缺失 = "
                + std::to_string(result.missing_count) + "。"});
    }
    StatisticTable summary;
    summary.title = "Bootstrap 摘要";
    summary.headers = {"Statistic", "Value"};
    summary.rows.push_back({"Mean 1", format_optional(result.mean_first)});
    summary.rows.push_back({"Mean 2", format_optional(result.mean_second)});
    summary.rows.push_back({"Mean Difference", format_optional(result.mean_difference)});
    summary.rows.push_back({"CI Lower", format_optional(result.ci_lower)});
    summary.rows.push_back({"CI Upper", format_optional(result.ci_upper)});
    summary.rows.push_back({"Confidence", format_number(result.confidence_level)});
    summary.rows.push_back({"Replicates", std::to_string(result.replicates)});
    page.tables.push_back(std::move(summary));
    if (!result.bootstrap_differences.empty()) {
        PlotSpec plot;
        plot.kind = PlotKind::histogram;
        plot.title = "Bootstrap 均值差分布";
        plot.x_axis_title = "Mean Difference*";
        plot.y_axis_title = "Count";
        plot.values = result.bootstrap_differences;
        page.plots.push_back(std::move(plot));
    }
    domain::BootstrapTwoSampleFacts facts;
    facts.n_first = result.n_first;
    facts.n_second = result.n_second;
    facts.replicates = result.replicates;
    facts.method = result.method;
    facts.mean_first = result.mean_first;
    facts.mean_second = result.mean_second;
    facts.mean_difference = result.mean_difference;
    facts.ci_lower = result.ci_lower;
    facts.ci_upper = result.ci_upper;
    facts.confidence_level = result.confidence_level;
    page.facts.bootstrap_two_sample = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::probit_reliability(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.probit_reliability.events_column.has_value()
        || !configuration.probit_reliability.trials_column.has_value()
        || !configuration.probit_reliability.stress_column.has_value()) {
        return error_page("Probit 可靠性", "Probit Reliability",
                          "请选择事件数列、试验数列和应力/剂量列。");
    }
    const ExtractedNumericColumn events_column = extract_numeric_column(
        table, *configuration.probit_reliability.events_column, configuration.excluded_rows);
    const ExtractedNumericColumn trials_column = extract_numeric_column(
        table, *configuration.probit_reliability.trials_column, configuration.excluded_rows);
    const ExtractedNumericColumn stress_column = extract_numeric_column(
        table, *configuration.probit_reliability.stress_column, configuration.excluded_rows);
    const auto aligned = align_complete_rows_with_source(
        {events_column, trials_column, stress_column});
    if (aligned.values.size() < 3) {
        return error_page("Probit 可靠性", "Probit Reliability",
                          "complete-case 有效行不足，无法拟合 logit 模型。");
    }
    std::vector<std::size_t> events;
    std::vector<std::size_t> trials;
    std::vector<double> stress;
    events.reserve(aligned.values.size());
    trials.reserve(aligned.values.size());
    stress.reserve(aligned.values.size());
    for (const auto& row : aligned.values) {
        std::vector<std::size_t> counts;
        if (!append_nonnegative_counts({row[0], row[1]}, counts) || counts.size() != 2
            || counts[0] > counts[1] || counts[1] == 0) {
            return error_page("Probit 可靠性", "Probit Reliability",
                              "事件数必须为非负整数且不超过试验数。");
        }
        events.push_back(counts[0]);
        trials.push_back(counts[1]);
        stress.push_back(row[2]);
    }
    const auto result = datalab::domain::statistics::fit_probit_reliability(
        events, trials, stress, configuration.inference.confidence_level,
        configuration.probit_reliability.max_iterations,
        configuration.probit_reliability.tolerance);
    OutputPage page;
    page.id = new_id("probit_reliability");
    page.title = "Probit 可靠性";
    page.method_name = "Probit Reliability";
    page.configuration = configuration;
    page.parameter_summary = "事件 = " + events_column.name
        + "    试验 = " + trials_column.name
        + "    应力 = " + stress_column.name
        + "    链接 = logit"
        + "    N = " + std::to_string(result.observation_count);
    page.diagnostics = result.diagnostics;
    const std::size_t eligible = table.rows.size() >= configuration.excluded_rows.size()
        ? table.rows.size() - configuration.excluded_rows.size() : table.rows.size();
    const std::size_t skipped = eligible > aligned.source_rows.size()
        ? eligible - aligned.source_rows.size() : 0;
    if (skipped > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "Probit 可靠性使用 complete-case，已跳过 "
                + std::to_string(skipped) + " 个含缺失或非法单元格的行。"});
    }
    StatisticTable summary;
    summary.title = "模型摘要";
    summary.headers = {"N", "迭代次数", "收敛", "Log-Likelihood", "Deviance", "AIC", "LD50"};
    summary.rows.push_back({
        std::to_string(result.observation_count),
        std::to_string(result.iteration_count),
        result.converged ? "是" : "否",
        format_number(result.log_likelihood),
        format_number(result.deviance),
        format_number(result.aic),
        format_optional(result.ld50)});
    page.tables.push_back(summary);
    StatisticTable coefficients;
    coefficients.title = "系数";
    coefficients.headers = {"项", "系数", "标准误", "Z", "P-Value", "CI Lower", "CI Upper"};
    for (const auto& coefficient : result.coefficients) {
        coefficients.rows.push_back({
            coefficient.term,
            format_number(coefficient.coefficient),
            format_number(coefficient.standard_error),
            format_number(coefficient.z_statistic),
            format_number(coefficient.p_value),
            format_number(coefficient.confidence_lower),
            format_number(coefficient.confidence_upper)});
    }
    page.tables.push_back(coefficients);
    if (result.ld50.has_value()) {
        StatisticTable ld50_table;
        ld50_table.title = "LD50";
        ld50_table.headers = {"Estimate", "SE", "CI Lower", "CI Upper"};
        ld50_table.rows.push_back({
            format_optional(result.ld50),
            format_optional(result.ld50_standard_error),
            format_optional(result.ld50_confidence_lower),
            format_optional(result.ld50_confidence_upper)});
        page.tables.push_back(std::move(ld50_table));
    }
    StatisticTable fitted;
    fitted.title = "拟合表";
    fitted.headers = {"原始行", "应力", "事件", "试验", "比例", "拟合概率", "Pearson 残差"};
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        const auto& observation = result.observations[index];
        fitted.rows.push_back({
            index < aligned.source_rows.size()
                ? std::to_string(aligned.source_rows[index] + 1) : "*",
            format_number(observation.stress),
            std::to_string(observation.events),
            std::to_string(observation.trials),
            format_number(observation.proportion),
            format_number(observation.fitted_probability),
            format_number(observation.pearson_residual)});
    }
    page.tables.push_back(std::move(fitted));
    if (!result.observations.empty()) {
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "应力-失效概率";
        plot.x_axis_title = stress_column.name;
        plot.y_axis_title = "Probability";
        for (const auto& observation : result.observations) {
            plot.x_values.push_back(observation.stress);
            plot.values.push_back(observation.proportion);
        }
        PlotSeries fitted_series;
        fitted_series.label = "拟合";
        fitted_series.role = PlotSeriesRole::fitted;
        fitted_series.style.point_style = PlotPointStyle::none;
        for (const auto& observation : result.observations) {
            fitted_series.x_values.push_back(observation.stress);
            fitted_series.values.push_back(observation.fitted_probability);
        }
        plot.series.push_back(std::move(fitted_series));
        page.plots.push_back(std::move(plot));
    }
    domain::ProbitReliabilityFacts facts;
    facts.n = result.observation_count;
    facts.iteration_count = result.iteration_count;
    facts.converged = result.converged;
    facts.link = result.link;
    if (result.coefficients.size() >= 2) {
        facts.intercept = result.coefficients[0].coefficient;
        facts.stress_coefficient = result.coefficients[1].coefficient;
    }
    facts.ld50 = result.ld50;
    facts.ld50_standard_error = result.ld50_standard_error;
    facts.ld50_confidence_lower = result.ld50_confidence_lower;
    facts.ld50_confidence_upper = result.ld50_confidence_upper;
    facts.log_likelihood = result.log_likelihood;
    facts.deviance = result.deviance;
    facts.aic = result.aic;
    page.facts.probit_reliability = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::cluster_observations(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::vector<std::size_t>& columns =
        configuration.hierarchical_cluster.variable_columns.empty()
        ? configuration.variable_columns
        : configuration.hierarchical_cluster.variable_columns;
    if (columns.size() < 2) {
        return error_page("层次聚类", "Cluster Observations",
                          "至少需要两个数值变量。");
    }
    std::vector<std::vector<double>> rows;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column : columns) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (valid) {
            rows.push_back(std::move(values));
            source_rows.push_back(row);
        }
    }
    const auto result = datalab::domain::statistics::cluster_observations_complete(
        rows,
        {configuration.hierarchical_cluster.cluster_count,
         configuration.hierarchical_cluster.standardize});
    OutputPage page;
    page.id = new_id("cluster_observations");
    page.title = "层次聚类（观测）";
    page.method_name = "Cluster Observations";
    page.configuration = configuration;
    page.parameter_summary = "linkage = complete    k = "
        + std::to_string(result.cluster_count)
        + "    N = " + std::to_string(result.observation_count);
    page.diagnostics = result.diagnostics;
    StatisticTable merges;
    merges.title = "合并历程";
    merges.headers = {"Step", "Left", "Right", "New", "Height"};
    for (const auto& merge : result.merges) {
        merges.rows.push_back({
            std::to_string(merge.step),
            std::to_string(merge.left_id),
            std::to_string(merge.right_id),
            std::to_string(merge.new_id),
            format_number(merge.height)});
    }
    page.tables.push_back(std::move(merges));
    StatisticTable assignments;
    assignments.title = "簇分配（切 k）";
    assignments.headers = {"原始行", "Cluster"};
    for (std::size_t index = 0; index < result.assignments.size(); ++index) {
        const std::size_t source_index = index < result.valid_rows.size()
            ? result.valid_rows[index] : index;
        assignments.rows.push_back({
            source_index < source_rows.size()
                ? std::to_string(source_rows[source_index] + 1) : "*",
            std::to_string(result.assignments[index] + 1)});
    }
    page.tables.push_back(std::move(assignments));
    if (columns.size() >= 2 && !rows.empty()) {
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "层次聚类散点（前两列）";
        plot.x_axis_title = column_label(table, columns[0]);
        plot.y_axis_title = column_label(table, columns[1]);
        for (std::size_t index = 0; index < rows.size(); ++index) {
            plot.x_values.push_back(rows[index][0]);
            plot.values.push_back(rows[index][1]);
            const std::size_t source_index = index < result.valid_rows.size()
                ? result.valid_rows[index] : index;
            plot.source_rows.push_back(
                source_index < source_rows.size() ? source_rows[source_index] : source_index);
        }
        page.plots.push_back(std::move(plot));
    }
    domain::HierarchicalClusterFacts facts;
    facts.n = result.observation_count;
    facts.variable_count = result.variable_count;
    facts.cluster_count = result.cluster_count;
    facts.merge_count = result.merges.size();
    facts.linkage = result.linkage;
    facts.standardized = result.standardized;
    page.facts.hierarchical_cluster = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::ordinal_logistic(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.ordinal_logistic.response_column.has_value()
        || configuration.ordinal_logistic.predictor_columns.empty()) {
        return error_page("有序 Logistic", "Ordinal Logistic",
                          "请选择有序响应列与至少一个数值预测列。");
    }
    const std::size_t response_column = *configuration.ordinal_logistic.response_column;
    std::vector<std::string> category_labels;
    std::map<std::string, std::size_t> category_index;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (response_column >= table.rows[row].size()) {
            continue;
        }
        const std::string& cell = table.rows[row][response_column];
        if (is_missing_cell(cell)) {
            continue;
        }
        if (category_index.find(cell) == category_index.end()) {
            category_index[cell] = category_labels.size();
            category_labels.push_back(cell);
        }
    }
    // Prefer numeric order when all labels parse as numbers.
    bool all_numeric = !category_labels.empty();
    for (const std::string& label : category_labels) {
        if (!parse_numeric_cell(label).has_value()) {
            all_numeric = false;
            break;
        }
    }
    if (all_numeric) {
        std::sort(category_labels.begin(), category_labels.end(),
                  [](const std::string& left, const std::string& right) {
                      return *parse_numeric_cell(left) < *parse_numeric_cell(right);
                  });
        category_index.clear();
        for (std::size_t index = 0; index < category_labels.size(); ++index) {
            category_index[category_labels[index]] = index;
        }
    }
    if (category_labels.size() < 3) {
        return error_page("有序 Logistic", "Ordinal Logistic",
                          "有序响应至少需要 3 个水平。");
    }
    std::vector<std::size_t> response;
    std::vector<std::vector<double>> predictors;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (response_column >= table.rows[row].size()) {
            continue;
        }
        const std::string& cell = table.rows[row][response_column];
        if (is_missing_cell(cell) || category_index.find(cell) == category_index.end()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column : configuration.ordinal_logistic.predictor_columns) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (!valid) {
            continue;
        }
        response.push_back(category_index[cell]);
        predictors.push_back(std::move(values));
    }
    std::vector<std::string> labels;
    for (const std::size_t column : configuration.ordinal_logistic.predictor_columns) {
        labels.push_back(column_label(table, column));
    }
    const auto result = datalab::domain::statistics::fit_ordinal_logistic(
        response, predictors, category_labels, labels,
        configuration.ordinal_logistic.max_iterations);
    OutputPage page;
    page.id = new_id("ordinal_logistic");
    page.title = "有序 Logistic 回归";
    page.method_name = "Ordinal Logistic Regression";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    水平 = " + std::to_string(result.category_count)
        + "    链 = logit";
    page.diagnostics = result.diagnostics;
    StatisticTable summary;
    summary.title = "模型摘要";
    summary.headers = {"N", "水平", "迭代", "收敛", "LogLik", "AIC"};
    summary.rows.push_back({
        std::to_string(result.observation_count),
        std::to_string(result.category_count),
        std::to_string(result.iteration_count),
        result.converged ? "是" : "否",
        format_number(result.log_likelihood),
        format_number(result.aic)});
    page.tables.push_back(std::move(summary));
    StatisticTable coefficients;
    coefficients.title = "系数（阈值与斜率）";
    coefficients.headers = {"Term", "Coef", "SE", "Z", "P"};
    for (const auto& coefficient : result.thresholds) {
        coefficients.rows.push_back({
            coefficient.term, format_number(coefficient.estimate),
            format_number(coefficient.standard_error),
            format_number(coefficient.z_statistic),
            format_number(coefficient.p_value)});
    }
    for (const auto& coefficient : result.slopes) {
        coefficients.rows.push_back({
            coefficient.term, format_number(coefficient.estimate),
            format_number(coefficient.standard_error),
            format_number(coefficient.z_statistic),
            format_number(coefficient.p_value)});
    }
    page.tables.push_back(std::move(coefficients));
    domain::OrdinalLogisticFacts facts;
    facts.n = result.observation_count;
    facts.category_count = result.category_count;
    facts.predictor_count = result.predictor_count;
    facts.iteration_count = result.iteration_count;
    facts.converged = result.converged;
    facts.log_likelihood = result.log_likelihood;
    facts.aic = result.aic;
    page.facts.ordinal_logistic = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::nominal_logistic(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.nominal_logistic.response_column.has_value()
        || configuration.nominal_logistic.predictor_columns.empty()) {
        return error_page("名义 Logistic", "Nominal Logistic",
                          "请选择名义响应列与至少一个数值预测列。");
    }
    const std::size_t response_column = *configuration.nominal_logistic.response_column;
    std::vector<std::string> category_labels;
    std::map<std::string, std::size_t> category_index;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (response_column >= table.rows[row].size()) {
            continue;
        }
        const std::string& cell = table.rows[row][response_column];
        if (is_missing_cell(cell)) {
            continue;
        }
        if (category_index.find(cell) == category_index.end()) {
            category_index[cell] = category_labels.size();
            category_labels.push_back(cell);
        }
    }
    if (category_labels.size() < 3) {
        return error_page("名义 Logistic", "Nominal Logistic",
                          "名义响应至少需要 3 个水平。");
    }
    std::vector<std::size_t> response;
    std::vector<std::vector<double>> predictors;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (response_column >= table.rows[row].size()) {
            continue;
        }
        const std::string& cell = table.rows[row][response_column];
        if (is_missing_cell(cell) || category_index.find(cell) == category_index.end()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column : configuration.nominal_logistic.predictor_columns) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (!valid) {
            continue;
        }
        response.push_back(category_index[cell]);
        predictors.push_back(std::move(values));
    }
    std::vector<std::string> labels;
    for (const std::size_t column : configuration.nominal_logistic.predictor_columns) {
        labels.push_back(column_label(table, column));
    }
    const auto result = datalab::domain::statistics::fit_nominal_logistic(
        response, predictors, category_labels, labels,
        configuration.nominal_logistic.max_iterations,
        1.0e-6,
        configuration.inference.confidence_level > 0.0
            ? configuration.inference.confidence_level : 0.95);
    OutputPage page;
    page.id = new_id("nominal_logistic");
    page.title = "名义 Logistic 回归";
    page.method_name = "Nominal Logistic Regression";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    水平 = " + std::to_string(result.category_count)
        + "    参考 = " + result.reference_category;
    page.diagnostics = result.diagnostics;
    StatisticTable summary;
    summary.title = "模型摘要";
    summary.headers = {"N", "水平", "Logit", "迭代", "收敛", "LogLik", "AIC", "G", "G P"};
    summary.rows.push_back({
        std::to_string(result.observation_count),
        std::to_string(result.category_count),
        std::to_string(result.logit_count),
        std::to_string(result.iteration_count),
        result.converged ? "是" : "否",
        format_number(result.log_likelihood),
        format_number(result.aic),
        format_number(result.g_statistic),
        format_number(result.g_p_value)});
    page.tables.push_back(std::move(summary));
    StatisticTable coefficients;
    coefficients.title = "Logistic 回归表";
    coefficients.headers = {"Logit", "Predictor", "Coef", "SE", "Z", "P", "OR", "Lower", "Upper"};
    for (const auto& coefficient : result.coefficients) {
        coefficients.rows.push_back({
            coefficient.logit_label, coefficient.term,
            format_number(coefficient.estimate),
            format_number(coefficient.standard_error),
            format_number(coefficient.z_statistic),
            format_number(coefficient.p_value),
            format_number(coefficient.odds_ratio),
            format_number(coefficient.confidence_lower),
            format_number(coefficient.confidence_upper)});
    }
    page.tables.push_back(std::move(coefficients));
    domain::NominalLogisticFacts facts;
    facts.n = result.observation_count;
    facts.category_count = result.category_count;
    facts.logit_count = result.logit_count;
    facts.predictor_count = result.predictor_count;
    facts.iteration_count = result.iteration_count;
    facts.converged = result.converged;
    facts.log_likelihood = result.log_likelihood;
    facts.aic = result.aic;
    facts.g_p_value = result.g_p_value;
    facts.reference_category = result.reference_category;
    page.facts.nominal_logistic = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::discriminant(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.discriminant.response_column.has_value()
        || configuration.discriminant.predictor_columns.empty()) {
        return error_page("线性判别", "Discriminant Analysis",
                          "请选择类别响应与至少一个数值预测列。");
    }
    const std::size_t response_column = *configuration.discriminant.response_column;
    std::vector<std::string> class_labels;
    std::map<std::string, std::size_t> class_index_map;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (response_column >= table.rows[row].size()) {
            continue;
        }
        const std::string& cell = table.rows[row][response_column];
        if (is_missing_cell(cell)) {
            continue;
        }
        if (class_index_map.find(cell) == class_index_map.end()) {
            class_index_map[cell] = class_labels.size();
            class_labels.push_back(cell);
        }
    }
    if (class_labels.size() < 2) {
        return error_page("线性判别", "Discriminant Analysis",
                          "至少需要两个类别。");
    }
    std::vector<std::size_t> class_index;
    std::vector<std::vector<double>> predictors;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (response_column >= table.rows[row].size()) {
            continue;
        }
        const std::string& cell = table.rows[row][response_column];
        if (is_missing_cell(cell) || class_index_map.find(cell) == class_index_map.end()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column : configuration.discriminant.predictor_columns) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (!valid) {
            continue;
        }
        class_index.push_back(class_index_map[cell]);
        predictors.push_back(std::move(values));
        source_rows.push_back(row);
    }
    const auto result = datalab::domain::statistics::linear_discriminant(
        class_index, predictors, class_labels);
    OutputPage page;
    page.id = new_id("discriminant");
    page.title = "线性判别分析";
    page.method_name = "Linear Discriminant Analysis";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    类数 = " + std::to_string(result.class_count)
        + "    准确率 ≈ " + format_number(result.train_accuracy);
    page.diagnostics = result.diagnostics;
    StatisticTable means;
    means.title = "类均值";
    means.headers = {"Class", "N"};
    for (const std::size_t column : configuration.discriminant.predictor_columns) {
        means.headers.push_back(column_label(table, column));
    }
    for (std::size_t cls = 0; cls < result.class_means.size(); ++cls) {
        std::vector<std::string> row = {
            cls < class_labels.size() ? class_labels[cls] : "?",
            cls < result.class_sizes.size() ? std::to_string(result.class_sizes[cls]) : "*"};
        for (double value : result.class_means[cls]) {
            row.push_back(format_number(value));
        }
        means.rows.push_back(std::move(row));
    }
    page.tables.push_back(std::move(means));
    StatisticTable confusion;
    confusion.title = "训练集混淆矩阵";
    confusion.headers = {"Actual \\ Predicted"};
    for (const std::string& label : class_labels) {
        confusion.headers.push_back(label);
    }
    for (std::size_t actual = 0; actual < result.confusion.size(); ++actual) {
        std::vector<std::string> row = {
            actual < class_labels.size() ? class_labels[actual] : "?"};
        for (std::size_t predicted = 0; predicted < result.confusion[actual].size();
             ++predicted) {
            row.push_back(std::to_string(result.confusion[actual][predicted]));
        }
        confusion.rows.push_back(std::move(row));
    }
    page.tables.push_back(std::move(confusion));
    if (result.ld1.size() == result.ld2.size() && !result.ld1.empty()) {
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "LD 投影（示意）";
        plot.x_axis_title = "LD1";
        plot.y_axis_title = "LD2";
        plot.x_values = result.ld1;
        plot.values = result.ld2;
        plot.source_rows = source_rows;
        page.plots.push_back(std::move(plot));
    }
    domain::DiscriminantFacts facts;
    facts.n = result.observation_count;
    facts.class_count = result.class_count;
    facts.predictor_count = result.predictor_count;
    facts.train_accuracy = result.train_accuracy;
    page.facts.discriminant = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::ccf(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.ccf.x_column.has_value()
        || !configuration.ccf.y_column.has_value()) {
        return error_page("CCF 互相关", "Cross Correlation",
                          "请选择两个数值序列列。");
    }
    const auto x = extract_numeric_column(
        table, *configuration.ccf.x_column, configuration.excluded_rows);
    const auto y = extract_numeric_column(
        table, *configuration.ccf.y_column, configuration.excluded_rows);
    const std::size_t n = std::min(x.values.size(), y.values.size());
    std::vector<double> xs(x.values.begin(), x.values.begin() + static_cast<std::ptrdiff_t>(n));
    std::vector<double> ys(y.values.begin(), y.values.begin() + static_cast<std::ptrdiff_t>(n));
    // Align by source row intersection when lengths differ due to missing.
    if (x.source_rows != y.source_rows) {
        std::map<std::size_t, double> y_by_row;
        for (std::size_t i = 0; i < y.values.size() && i < y.source_rows.size(); ++i) {
            y_by_row[y.source_rows[i]] = y.values[i];
        }
        xs.clear();
        ys.clear();
        for (std::size_t i = 0; i < x.values.size() && i < x.source_rows.size(); ++i) {
            const auto found = y_by_row.find(x.source_rows[i]);
            if (found == y_by_row.end()) {
                continue;
            }
            xs.push_back(x.values[i]);
            ys.push_back(found->second);
        }
    }
    const double alpha =
        configuration.inference.confidence_level > 0.0
            && configuration.inference.confidence_level < 1.0
        ? 1.0 - configuration.inference.confidence_level
        : 0.05;
    const auto result = datalab::domain::statistics::compute_ccf(
        xs, ys, configuration.ccf.max_lag, alpha);
    OutputPage page;
    page.id = new_id("ccf");
    page.title = "互相关（CCF）";
    page.method_name = "Cross Correlation";
    page.configuration = configuration;
    page.parameter_summary = "X = " + x.name + "    Y = " + y.name
        + "    N = " + std::to_string(result.n)
        + "    max|lag| = " + std::to_string(result.max_lag);
    page.diagnostics = result.diagnostics;
    StatisticTable table_out;
    table_out.title = "CCF";
    table_out.headers = {"Lag", "CCF", "Lower", "Upper"};
    for (std::size_t index = 0; index < result.lags.size(); ++index) {
        table_out.rows.push_back({
            format_number(result.lags[index]),
            format_number(result.ccf[index]),
            format_number(-result.band_half_width),
            format_number(result.band_half_width)});
    }
    page.tables.push_back(std::move(table_out));
    PlotSpec plot;
    plot.kind = PlotKind::time_series;
    plot.title = "CCF";
    plot.x_axis_title = "Lag";
    plot.y_axis_title = "CCF";
    plot.x_values = result.lags;
    plot.values = result.ccf;
    PlotSeries upper;
    upper.label = "上置信限";
    upper.role = PlotSeriesRole::confidence_band;
    upper.x_values = result.lags;
    upper.values.assign(result.lags.size(), result.band_half_width);
    upper.style.point_style = PlotPointStyle::none;
    PlotSeries lower = upper;
    lower.label = "下置信限";
    lower.values.assign(result.lags.size(), -result.band_half_width);
    plot.series.push_back(std::move(upper));
    plot.series.push_back(std::move(lower));
    page.plots.push_back(std::move(plot));
    domain::CcfFacts facts;
    facts.n = result.n;
    facts.missing_count = result.missing_count;
    facts.max_lag = result.max_lag;
    facts.band_half_width = result.band_half_width;
    for (std::size_t index = 0; index < result.lags.size(); ++index) {
        if (std::abs(result.lags[index]) < 1.0e-12) {
            facts.ccf_at_zero = result.ccf[index];
            break;
        }
    }
    page.facts.ccf = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::correlogram(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::vector<std::size_t>& columns =
        configuration.correlogram.variable_columns.empty()
        ? configuration.variable_columns
        : configuration.correlogram.variable_columns;
    if (columns.size() < 2) {
        return error_page("Correlogram", "Correlogram",
                          "至少需要两个数值变量。");
    }
    std::vector<std::vector<double>> column_data(columns.size());
    std::vector<std::string> labels;
    for (std::size_t index = 0; index < columns.size(); ++index) {
        labels.push_back(column_label(table, columns[index]));
        const auto extracted = extract_numeric_column(
            table, columns[index], configuration.excluded_rows);
        column_data[index] = extracted.values;
    }
    // Align complete-case across columns by source rows of first column path:
    // rebuild using row-wise complete-case.
    column_data.assign(columns.size(), {});
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column : columns) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (!valid) {
            continue;
        }
        for (std::size_t index = 0; index < values.size(); ++index) {
            column_data[index].push_back(values[index]);
        }
    }
    const auto method = configuration.correlogram.method == "spearman"
        ? datalab::domain::statistics::CorrelationMethod::spearman
        : datalab::domain::statistics::CorrelationMethod::pearson;
    const auto result = datalab::domain::statistics::correlation_matrix(
        column_data, method, configuration.inference.confidence_level);
    OutputPage page;
    page.id = new_id("correlogram");
    page.title = "Correlogram（相关热图）";
    page.method_name = "Correlogram";
    page.configuration = configuration;
    page.parameter_summary = "变量数 = " + std::to_string(columns.size())
        + "    方法 = " + configuration.correlogram.method;
    page.diagnostics = result.diagnostics;
    StatisticTable matrix;
    matrix.title = "相关矩阵";
    matrix.headers = {"Variable"};
    for (const std::string& label : labels) {
        matrix.headers.push_back(label);
    }
    for (std::size_t row = 0; row < result.coefficients.size(); ++row) {
        std::vector<std::string> out = {row < labels.size() ? labels[row] : "?"};
        for (double value : result.coefficients[row]) {
            out.push_back(format_number(value));
        }
        matrix.rows.push_back(std::move(out));
    }
    page.tables.push_back(std::move(matrix));
    PlotSpec plot;
    plot.kind = PlotKind::heatmap;
    plot.title = "相关热图";
    plot.x_axis_title = "变量";
    plot.y_axis_title = "变量";
    plot.categories = labels;
    plot.matrix_labels = labels;
    plot.matrix_values = result.coefficients;
    plot.color_min = -1.0;
    plot.color_max = 1.0;
    page.plots.push_back(std::move(plot));
    domain::CorrelogramFacts facts;
    facts.variable_count = columns.size();
    facts.method = configuration.correlogram.method;
    facts.pair_count = result.pairs.size();
    page.facts.correlogram = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::stepwise_regression(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.stepwise_regression.response_column.has_value()
        || configuration.stepwise_regression.predictor_columns.size() < 2) {
        return error_page("逐步回归", "Stepwise Regression",
                          "请选择响应列与至少两个候选预测列。");
    }
    const std::size_t response_column = *configuration.stepwise_regression.response_column;
    std::vector<double> response;
    std::vector<std::vector<double>> predictors;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (response_column >= table.rows[row].size()) {
            continue;
        }
        const auto y = parse_numeric_cell(table.rows[row][response_column]);
        if (!y.has_value()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column : configuration.stepwise_regression.predictor_columns) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (!valid) {
            continue;
        }
        response.push_back(*y);
        predictors.push_back(std::move(values));
        source_rows.push_back(row);
    }
    std::vector<std::string> labels;
    for (const std::size_t column : configuration.stepwise_regression.predictor_columns) {
        labels.push_back(column_label(table, column));
    }
    const auto result = datalab::domain::statistics::fit_stepwise_regression(
        response, predictors, labels,
        configuration.stepwise_regression.method,
        configuration.stepwise_regression.alpha_enter,
        configuration.stepwise_regression.alpha_remove,
        configuration.inference.confidence_level > 0.0
            ? configuration.inference.confidence_level : 0.95,
        source_rows);
    OutputPage page;
    page.id = new_id("stepwise_regression");
    page.title = "逐步回归";
    page.method_name = "Stepwise Regression";
    page.configuration = configuration;
    page.parameter_summary = "方法 = " + result.method
        + (result.criterion != "alpha" ? (" / 准则 = " + result.criterion) : "")
        + "    N = " + std::to_string(result.observation_count)
        + "    选入 = " + std::to_string(result.selected_terms.size())
        + (result.criterion == "aicc" || result.criterion == "bic"
               ? ("    最优步 = " + std::to_string(result.best_step_index)) : "");
    page.diagnostics = result.diagnostics;
    StatisticTable steps;
    steps.title = "逐步步骤";
    const bool show_ic = result.criterion != "alpha";
    steps.headers = show_ic
        ? std::vector<std::string>{"Step", "Action", "Term", "R-sq", "R-sq(adj)", "SSE", "AICc", "BIC", "P"}
        : std::vector<std::string>{"Step", "Action", "Term", "R-sq", "R-sq(adj)", "SSE", "P"};
    for (const auto& step : result.steps) {
        std::string p_text = "*";
        if (step.entered_p_value.has_value()) {
            p_text = format_number(*step.entered_p_value);
        } else if (step.removed_p_value.has_value()) {
            p_text = format_number(*step.removed_p_value);
        }
        if (show_ic) {
            steps.rows.push_back({
                std::to_string(step.step), step.action, step.term,
                format_number(step.r_squared), format_number(step.adjusted_r_squared),
                format_number(step.error_sum_of_squares),
                step.aicc.has_value() ? format_number(*step.aicc) : "*",
                step.bic.has_value() ? format_number(*step.bic) : "*",
                p_text});
        } else {
            steps.rows.push_back({
                std::to_string(step.step), step.action, step.term,
                format_number(step.r_squared), format_number(step.adjusted_r_squared),
                format_number(step.error_sum_of_squares), p_text});
        }
    }
    page.tables.push_back(std::move(steps));
    StatisticTable selected;
    selected.title = "选入项";
    selected.headers = {"Term"};
    for (const std::string& term : result.selected_terms) {
        selected.rows.push_back({term});
    }
    if (selected.rows.empty()) {
        selected.rows.push_back({"(仅截距)"});
    }
    page.tables.push_back(std::move(selected));
    if (!result.final_model.coefficients.empty()) {
        StatisticTable coefficients;
        coefficients.title = "终模型系数";
        coefficients.headers = {"Term", "Coef", "SE", "T", "P"};
        for (const auto& coefficient : result.final_model.coefficients) {
            coefficients.rows.push_back({
                coefficient.term,
                format_number(coefficient.coefficient),
                format_number(coefficient.standard_error),
                format_number(coefficient.t_statistic),
                coefficient.p_value.has_value() ? format_number(*coefficient.p_value) : "*"});
        }
        page.tables.push_back(std::move(coefficients));
    }
    domain::StepwiseRegressionFacts facts;
    facts.n = result.observation_count;
    facts.candidate_count = result.candidate_count;
    facts.selected_count = result.selected_terms.size();
    facts.step_count = result.steps.size();
    facts.method = result.method;
    facts.criterion = result.criterion;
    facts.best_step_index = result.best_step_index;
    facts.r_squared = result.final_model.r_squared;
    facts.adjusted_r_squared = result.final_model.adjusted_r_squared;
    if (result.best_step_index < result.steps.size()) {
        facts.best_aicc = result.steps[result.best_step_index].aicc;
        facts.best_bic = result.steps[result.best_step_index].bic;
    }
    page.facts.stepwise_regression = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::best_subsets_regression(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.best_subsets_regression.response_column.has_value()
        || configuration.best_subsets_regression.predictor_columns.empty()) {
        return error_page("Best Subsets 回归", "Best Subsets Regression",
                          "请选择响应列与至少一个候选预测列。");
    }
    const std::size_t response_column =
        *configuration.best_subsets_regression.response_column;
    std::vector<double> response;
    std::vector<std::vector<double>> predictors;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (response_column >= table.rows[row].size()) {
            continue;
        }
        const auto y = parse_numeric_cell(table.rows[row][response_column]);
        if (!y.has_value()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column :
             configuration.best_subsets_regression.predictor_columns) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (!valid) {
            continue;
        }
        response.push_back(*y);
        predictors.push_back(std::move(values));
        source_rows.push_back(row);
    }
    std::vector<std::string> labels;
    for (const std::size_t column :
         configuration.best_subsets_regression.predictor_columns) {
        labels.push_back(column_label(table, column));
    }
    const auto result = datalab::domain::statistics::fit_best_subsets_regression(
        response, predictors, labels,
        configuration.best_subsets_regression.min_predictors,
        configuration.best_subsets_regression.max_predictors,
        configuration.best_subsets_regression.models_per_size,
        configuration.inference.confidence_level > 0.0
            ? configuration.inference.confidence_level : 0.95,
        source_rows);
    OutputPage page;
    page.id = new_id("best_subsets_regression");
    page.title = "Best Subsets 回归";
    page.method_name = "Best Subsets Regression";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    候选 = " + std::to_string(result.candidate_count)
        + "    每规模模型数 = " + std::to_string(result.models_per_size);
    page.diagnostics = result.diagnostics;
    if (source_rows.size() < table.rows.size()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "Best Subsets 使用 complete-case，缺失或非法行已排除。"});
    }
    StatisticTable summary;
    summary.title = "模型摘要";
    std::vector<std::string> headers = {"Vars"};
    for (const std::string& label : labels) {
        headers.push_back(label);
    }
    headers.insert(headers.end(),
                   {"R-sq", "R-sq(adj)", "Mallows Cp", "S"});
    summary.headers = headers;
    for (const auto& model : result.model_summaries) {
        std::vector<std::string> row;
        row.push_back(std::to_string(model.predictor_count));
        for (bool included : model.predictors_in_model) {
            row.push_back(included ? "X" : " ");
        }
        row.push_back(format_number(model.r_squared));
        row.push_back(format_number(model.adjusted_r_squared));
        row.push_back(format_number(model.mallows_cp));
        row.push_back(format_number(model.s));
        summary.rows.push_back(std::move(row));
    }
    page.tables.push_back(std::move(summary));
    domain::BestSubsetsRegressionFacts facts;
    facts.n = result.observation_count;
    facts.candidate_count = result.candidate_count;
    facts.model_count = result.model_summaries.size();
    facts.models_per_size = result.models_per_size;
    if (result.best_overall.has_value()) {
        facts.best_r_squared = result.best_overall->r_squared;
        facts.best_adjusted_r_squared = result.best_overall->adjusted_r_squared;
        facts.best_predictor_count = result.best_overall->predictor_count;
    }
    page.facts.best_subsets_regression = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::km_interval(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.km_interval.left_column.has_value()
        || !configuration.km_interval.right_column.has_value()) {
        return error_page("区间删失 KM", "Interval-Censored KM",
                          "请选择区间左端与右端列（右删失可用空/Inf）。");
    }
    const std::size_t left_column = *configuration.km_interval.left_column;
    const std::size_t right_column = *configuration.km_interval.right_column;
    std::vector<datalab::domain::statistics::IntervalObservation> observations;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (left_column >= table.rows[row].size()
            || right_column >= table.rows[row].size()) {
            continue;
        }
        const auto left = parse_numeric_cell(table.rows[row][left_column]);
        if (!left.has_value()) {
            continue;
        }
        datalab::domain::statistics::IntervalObservation obs;
        obs.left = *left;
        obs.source_row = row;
        const std::string& right_text = table.rows[row][right_column];
        if (is_missing_cell(right_text) || right_text == "Inf" || right_text == "+Inf"
            || right_text == "inf" || right_text == "INF") {
            obs.right = std::numeric_limits<double>::infinity();
        } else {
            const auto right = parse_numeric_cell(right_text);
            if (!right.has_value()) {
                continue;
            }
            obs.right = *right;
        }
        observations.push_back(obs);
    }
    const auto result = datalab::domain::statistics::kaplan_meier_interval(observations);
    OutputPage page;
    page.id = new_id("km_interval");
    page.title = "区间删失 Kaplan–Meier（Turnbull）";
    page.method_name = "Interval-Censored KM";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    精确 = " + std::to_string(result.exact_count)
        + "    左 = " + std::to_string(result.left_censored_count)
        + "    区间 = " + std::to_string(result.interval_censored_count)
        + "    右 = " + std::to_string(result.right_censored_count);
    page.diagnostics = result.diagnostics;
    StatisticTable summary;
    summary.title = "摘要";
    summary.headers = {"N", "Exact", "Left", "Interval", "Right", "Iter", "Median"};
    summary.rows.push_back({
        std::to_string(result.observation_count),
        std::to_string(result.exact_count),
        std::to_string(result.left_censored_count),
        std::to_string(result.interval_censored_count),
        std::to_string(result.right_censored_count),
        std::to_string(result.iteration_count),
        result.median_life.has_value() ? format_number(*result.median_life) : "*"});
    page.tables.push_back(std::move(summary));
    StatisticTable curve;
    curve.title = "生存估计";
    curve.headers = {"Time", "Mass", "S(t)"};
    for (const auto& point : result.points) {
        curve.rows.push_back({
            format_number(point.time), format_number(point.mass),
            format_number(point.survival)});
    }
    page.tables.push_back(std::move(curve));
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "Turnbull 生存曲线";
    plot.x_axis_title = "时间";
    plot.y_axis_title = "S(t)";
    for (const auto& point : result.points) {
        plot.x_values.push_back(point.time);
        plot.values.push_back(point.survival);
    }
    page.plots.push_back(std::move(plot));
    domain::KmIntervalFacts facts;
    facts.n = result.observation_count;
    facts.exact_count = result.exact_count;
    facts.left_censored_count = result.left_censored_count;
    facts.right_censored_count = result.right_censored_count;
    facts.interval_censored_count = result.interval_censored_count;
    facts.iteration_count = result.iteration_count;
    facts.converged = result.converged;
    facts.identifiable = result.identifiable;
    facts.median_life = result.median_life;
    facts.evidence_type = "formula_reference";
    facts.algorithm_id = "turnbull_npmle_simplified_grid";
    facts.gate_status = "open_with_limits";
    facts.research_preview = false;
    facts.classic_km_equivalent =
        result.left_censored_count == 0 && result.interval_censored_count == 0
        && result.right_censored_count == 0 && result.exact_count > 0;
    page.facts.km_interval = facts;
    page.method_metadata.algorithm = "km_interval_turnbull";
    page.method_metadata.version = "1";
    page.method_metadata.valid_count = result.observation_count;
    page.diagnostics.push_back({
        DiagnosticMessage::Severity::warning,
        "km_interval_not_vendor_oracle",
        "区间删失 Turnbull 为简化网格 NPMLE（formula_reference）；"
        "分模式可靠度拟合与 pinned R/商业对齐仍未冻结，不得宣称 vendor_oracle。"});
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::doe_plackett_burman(
    const DataTable&,
    const AnalysisConfiguration& configuration)
{
    if (configuration.plackett_burman.factor_names.empty()) {
        return error_page("Plackett–Burman 设计", "Plackett-Burman Design",
                          "请提供至少一个因子名称。");
    }
    std::vector<datalab::domain::statistics::DoeFactor> factors;
    for (std::size_t index = 0; index < configuration.plackett_burman.factor_names.size();
         ++index) {
        datalab::domain::statistics::DoeFactor factor;
        factor.name = configuration.plackett_burman.factor_names[index];
        factor.low_level = index < configuration.plackett_burman.low_levels.size()
            ? configuration.plackett_burman.low_levels[index] : "-1";
        factor.high_level = index < configuration.plackett_burman.high_levels.size()
            ? configuration.plackett_burman.high_levels[index] : "+1";
        factors.push_back(std::move(factor));
    }
    const auto pb = datalab::domain::statistics::generate_plackett_burman({
        factors,
        configuration.plackett_burman.center_point_count,
        configuration.plackett_burman.randomize,
        configuration.plackett_burman.random_seed});
    datalab::domain::statistics::DoeFactorialDesign design;
    design.factors = pb.factors;
    design.runs = pb.runs;
    design.diagnostics = pb.diagnostics;
    design.design_kind = "plackett_burman";
    design.resolution = 3;
    auto page = doe_design_page(configuration, factors, design);
    page.title = "Plackett–Burman 设计";
    page.method_name = "Plackett-Burman Design";
    domain::PlackettBurmanFacts facts;
    facts.factor_count = pb.factor_count;
    facts.run_count = pb.runs.size();
    facts.center_point_count = configuration.plackett_burman.center_point_count;
    page.facts.plackett_burman = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::random_forest(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.random_forest.response_column.has_value()
        || configuration.random_forest.predictor_columns.empty()) {
        return error_page("随机森林", "Random Forest",
                          "请选择响应列与至少一个数值预测列。");
    }
    const bool is_regression = configuration.random_forest.task == "regression";
    const std::size_t response_column = *configuration.random_forest.response_column;
    const auto& predictors_cols = configuration.random_forest.predictor_columns;

    std::vector<std::string> class_labels;
    std::map<std::string, std::size_t> class_index;
    if (!is_regression) {
        for (std::size_t row = 0; row < table.rows.size(); ++row) {
            if (std::find(configuration.excluded_rows.cbegin(),
                          configuration.excluded_rows.cend(), row)
                != configuration.excluded_rows.cend()) {
                continue;
            }
            if (response_column >= table.rows[row].size()) {
                continue;
            }
            const std::string& cell = table.rows[row][response_column];
            if (is_missing_cell(cell)) {
                continue;
            }
            if (class_index.find(cell) == class_index.end()) {
                class_index[cell] = class_labels.size();
                class_labels.push_back(cell);
            }
        }
        if (class_labels.size() < 2) {
            return error_page("随机森林", "Random Forest",
                              "分类任务需要至少两个响应类别。");
        }
    }

    std::vector<std::vector<double>> predictors;
    std::vector<double> response;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (response_column >= table.rows[row].size()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (const std::size_t column : predictors_cols) {
            if (column >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][column]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (!valid) {
            continue;
        }
        if (is_regression) {
            const auto parsed = parse_numeric_cell(table.rows[row][response_column]);
            if (!parsed.has_value()) {
                continue;
            }
            response.push_back(*parsed);
        } else {
            const std::string& cell = table.rows[row][response_column];
            if (is_missing_cell(cell) || class_index.find(cell) == class_index.end()) {
                continue;
            }
            response.push_back(static_cast<double>(class_index[cell]));
        }
        predictors.push_back(std::move(values));
        source_rows.push_back(row);
    }

    std::vector<std::string> predictor_names;
    for (const std::size_t column : predictors_cols) {
        predictor_names.push_back(column_label(table, column));
    }
    datalab::domain::statistics::RandomForestOptions options;
    options.task = is_regression
        ? datalab::domain::statistics::CartTask::regression
        : datalab::domain::statistics::CartTask::classification;
    options.n_trees = std::max<std::size_t>(1, configuration.random_forest.n_trees);
    options.max_depth = configuration.random_forest.max_depth;
    options.min_leaf = configuration.random_forest.min_leaf;
    options.seed = configuration.random_forest.seed;
    options.compute_oob = configuration.random_forest.compute_oob;
    const auto result = datalab::domain::statistics::fit_random_forest(
        predictors, response, class_labels, predictor_names, options);

    OutputPage page;
    page.id = new_id("random_forest");
    page.title = "随机森林";
    page.method_name = "Random Forest";
    page.configuration = configuration;
    page.parameter_summary = std::string("任务 = ") + configuration.random_forest.task
        + "    N = " + std::to_string(result.observation_count)
        + "    树数 = " + std::to_string(result.n_trees)
        + "    深度上限 = " + std::to_string(result.max_depth);
    page.diagnostics = result.diagnostics;
    if (source_rows.size() < table.rows.size()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "随机森林使用 complete-case（缺失≠0）；仅排除 excluded_rows，hidden≠excluded。"});
    }
    if (result.observation_count == 0) {
        return finalize_page(std::move(page));
    }

    StatisticTable summary;
    summary.title = "Model Summary";
    summary.headers = {"Metric", "Value"};
    summary.rows.push_back({"N", std::to_string(result.observation_count)});
    summary.rows.push_back({"Predictors", std::to_string(result.predictor_count)});
    summary.rows.push_back({"Trees", std::to_string(result.n_trees)});
    summary.rows.push_back({
        is_regression ? "Train RMSE" : "Train Accuracy",
        format_number(result.train_metric)});
    if (result.oob_metric.has_value()) {
        summary.rows.push_back({
            is_regression ? "OOB RMSE" : "OOB Accuracy",
            format_number(*result.oob_metric)});
    }
    summary.rows.push_back({"Top Variable", result.top_variable});
    summary.rows.push_back({"Disclosure", result.disclosure});
    page.tables.push_back(std::move(summary));

    StatisticTable importance;
    importance.title = "Variable Importance";
    importance.headers = {"Variable", "Mean Impurity Decrease"};
    for (std::size_t index = 0; index < result.variable_importance.size(); ++index) {
        importance.rows.push_back({
            index < predictor_names.size() ? predictor_names[index]
                                           : ("X" + std::to_string(index + 1)),
            format_number(result.variable_importance[index])});
    }
    page.tables.push_back(std::move(importance));

    if (!is_regression && !result.confusion.empty()) {
        StatisticTable confusion;
        confusion.title = "Confusion Matrix";
        confusion.headers = {"Actual \\ Predicted"};
        for (const std::string& label : class_labels) {
            confusion.headers.push_back(label);
        }
        for (std::size_t actual = 0; actual < result.confusion.size(); ++actual) {
            std::vector<std::string> row = {
                actual < class_labels.size() ? class_labels[actual] : "?"};
            for (std::size_t predicted = 0;
                 predicted < result.confusion[actual].size(); ++predicted) {
                row.push_back(std::to_string(result.confusion[actual][predicted]));
            }
            confusion.rows.push_back(std::move(row));
        }
        page.tables.push_back(std::move(confusion));
    }

    domain::RandomForestFacts facts;
    facts.task = configuration.random_forest.task;
    facts.n = result.observation_count;
    facts.predictor_count = result.predictor_count;
    facts.n_trees = result.n_trees;
    facts.max_depth = result.max_depth;
    facts.train_metric = result.train_metric;
    facts.oob_metric = result.oob_metric;
    facts.top_variable = result.top_variable;
    facts.disclosure = result.disclosure;
    page.facts.random_forest = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::weibayes(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.weibayes.time_column.has_value()
        || !configuration.weibayes.event_column.has_value()) {
        return error_page("Weibayes", "Weibayes",
                          "请选择时间列与事件/删失指示列。");
    }
    const auto times = extract_numeric_column(
        table, *configuration.weibayes.time_column, configuration.excluded_rows);
    const auto event_text = extract_text_column(
        table, *configuration.weibayes.event_column);
    std::vector<double> aligned_times;
    std::vector<bool> events;
    std::vector<std::size_t> source_rows;
    for (std::size_t i = 0; i < times.source_rows.size(); ++i) {
        const std::size_t row = times.source_rows[i];
        if (row >= event_text.size()) {
            continue;
        }
        const auto parsed_event =
            datalab::domain::statistics::parse_reliability_event(event_text[row]);
        if (!parsed_event.has_value()) {
            continue;
        }
        aligned_times.push_back(times.values[i]);
        events.push_back(*parsed_event);
        source_rows.push_back(row);
    }
    const auto result = datalab::domain::statistics::fit_weibayes(
        aligned_times, events, source_rows,
        {configuration.weibayes.shape_prior});

    OutputPage page;
    page.id = new_id("weibayes");
    page.title = "Weibayes";
    page.method_name = "Weibayes";
    page.configuration = configuration;
    page.parameter_summary = "β = " + format_number(result.shape_prior)
        + "    N = " + std::to_string(result.n)
        + "    Failures r = " + std::to_string(result.failure_count)
        + "    Censored = " + std::to_string(result.censored_count);
    page.diagnostics = result.diagnostics;
    if (source_rows.size() < table.rows.size()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "Weibayes 使用 complete-case；仅 exact+right 删失主路径。"});
    }

    StatisticTable censor;
    censor.title = "Censoring Summary";
    censor.headers = {"Item", "Value"};
    censor.rows.push_back({"N", std::to_string(result.n)});
    censor.rows.push_back({"Failures (r)", std::to_string(result.failure_count)});
    censor.rows.push_back({"Right censored", std::to_string(result.censored_count)});
    censor.rows.push_back({"Zero-failure bound", result.zero_failure_bound ? "是" : "否"});
    page.tables.push_back(std::move(censor));

    StatisticTable params;
    params.title = "Parameter Estimates";
    params.headers = {"Parameter", "Estimate"};
    params.rows.push_back({"Shape β (prior)", format_number(result.shape_prior)});
    params.rows.push_back({
        "Scale η",
        result.scale.has_value() ? format_number(*result.scale) : "*"});
    page.tables.push_back(std::move(params));

    if (!result.percentiles.empty()) {
        StatisticTable percentiles;
        percentiles.title = "Percentiles";
        percentiles.headers = {"Percentile", "Life"};
        for (const auto& row : result.percentiles) {
            percentiles.rows.push_back({
                "B" + std::to_string(static_cast<int>(row.percentile)),
                format_number(row.life)});
        }
        page.tables.push_back(std::move(percentiles));
    }

    domain::WeibayesFacts facts;
    facts.n = result.n;
    facts.failure_count = result.failure_count;
    facts.censored_count = result.censored_count;
    facts.shape_prior = result.shape_prior;
    facts.scale = result.scale;
    facts.zero_failure_bound = result.zero_failure_bound;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    for (const auto& row : result.percentiles) {
        if (row.percentile == 10.0) {
            facts.b10 = row.life;
        } else if (row.percentile == 50.0) {
            facts.b50 = row.life;
        } else if (row.percentile == 90.0) {
            facts.b90 = row.life;
        }
    }
    page.facts.weibayes = facts;
    page.analysis_command_id = "weibayes";
    attach_computation_traces(page, "weibayes");
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::taguchi_orthogonal_design(
    const DataTable&,
    const AnalysisConfiguration& configuration)
{
    if (configuration.taguchi_orthogonal.factor_names.empty()) {
        return error_page("Taguchi 正交设计", "Taguchi Orthogonal Design",
                          "请提供至少一个因子名称。");
    }
    std::vector<datalab::domain::statistics::DoeFactor> factors;
    for (std::size_t index = 0;
         index < configuration.taguchi_orthogonal.factor_names.size(); ++index) {
        datalab::domain::statistics::DoeFactor factor;
        factor.name = configuration.taguchi_orthogonal.factor_names[index];
        factor.low_level = index < configuration.taguchi_orthogonal.low_levels.size()
            ? configuration.taguchi_orthogonal.low_levels[index] : "-1";
        factor.high_level = index < configuration.taguchi_orthogonal.high_levels.size()
            ? configuration.taguchi_orthogonal.high_levels[index] : "+1";
        if (index < configuration.taguchi_orthogonal.mid_levels.size()) {
            factor.mid_level = configuration.taguchi_orthogonal.mid_levels[index];
        }
        factors.push_back(std::move(factor));
    }
    datalab::domain::statistics::TaguchiOrthogonalOptions options;
    options.array = datalab::domain::statistics::parse_taguchi_array(
        configuration.taguchi_orthogonal.array);
    options.factors = factors;
    options.randomize = configuration.taguchi_orthogonal.randomize;
    options.random_seed = configuration.taguchi_orthogonal.random_seed;
    const auto taguchi =
        datalab::domain::statistics::generate_taguchi_orthogonal(options);
    auto design = datalab::domain::statistics::taguchi_to_factorial_design(taguchi);
    auto page = doe_design_page(configuration, taguchi.factors, design);
    // New worksheet must not inherit prior excluded/hidden rows (A→B honesty).
    page.configuration.excluded_rows.clear();
    page.configuration.hidden_rows.clear();
    page.title = "Taguchi 正交设计";
    page.method_name = "Taguchi Orthogonal Design";
    page.parameter_summary = "阵列 = " + taguchi.array_name
        + "    因子 = " + std::to_string(taguchi.factor_count)
        + "    运行 = " + std::to_string(taguchi.run_count)
        + "    水平数 = " + std::to_string(taguchi.levels_per_factor);
    domain::TaguchiOrthogonalFacts facts;
    facts.array = taguchi.array_name;
    facts.factor_count = taguchi.factor_count;
    facts.run_count = taguchi.run_count;
    facts.levels_per_factor = taguchi.levels_per_factor;
    page.facts.taguchi_orthogonal = facts;
    if (page.worksheet_export.has_value()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info, "taguchi_worksheet_export",
            "已生成设计矩阵工作表（不含旧 excluded_rows/hidden_rows）。"
            "本命令仅设计生成，不含完整 Taguchi ANOVA。"});
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::distribution_calculator(
    const DataTable&,
    const AnalysisConfiguration& configuration)
{
    datalab::domain::statistics::DistributionCalculatorOptions options;
    options.distribution = datalab::domain::statistics::parse_distcalc_distribution(
        configuration.distribution_calculator.distribution);
    options.operation = datalab::domain::statistics::parse_distcalc_operation(
        configuration.distribution_calculator.operation);
    options.param1 = configuration.distribution_calculator.param1;
    options.param2 = configuration.distribution_calculator.param2;
    options.param3 = configuration.distribution_calculator.param3;
    options.value = configuration.distribution_calculator.value;
    const auto result =
        datalab::domain::statistics::evaluate_distribution_calculator(options);

    OutputPage page;
    page.id = new_id("distribution_calculator");
    page.title = "分布计算器";
    page.method_name = "Distribution Calculator";
    page.configuration = configuration;
    page.parameter_summary = result.distribution + " / " + result.operation
        + "    value = " + format_number(result.value);
    page.diagnostics = result.diagnostics;

    StatisticTable params;
    params.title = "Parameters";
    params.headers = {"Name", "Value"};
    params.rows.push_back({"Distribution", result.distribution});
    params.rows.push_back({"Operation", result.operation});
    params.rows.push_back({"Param1", format_number(result.param1)});
    params.rows.push_back({"Param2", format_number(result.param2)});
    params.rows.push_back({"Param3", format_number(result.param3)});
    params.rows.push_back({"Input value", format_number(result.value)});
    page.tables.push_back(std::move(params));

    StatisticTable outcome;
    outcome.title = "Result";
    outcome.headers = {"Quantity", "Value"};
    outcome.rows.push_back({
        result.operation,
        result.result.has_value() ? format_number(*result.result) : "*"});
    page.tables.push_back(std::move(outcome));

    domain::DistributionCalculatorFacts facts;
    facts.distribution = result.distribution;
    facts.operation = result.operation;
    facts.param1 = result.param1;
    facts.param2 = result.param2;
    facts.param3 = result.param3;
    facts.value = result.value;
    facts.result = result.result;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.distribution_calculator = facts;
    return finalize_page(std::move(page));
}

namespace {

StatisticTable taguchi_response_table(
    const std::string& title,
    const std::vector<datalab::domain::statistics::TaguchiFactorResponse>& factors)
{
    StatisticTable table;
    table.title = title;
    table.headers = {"Level"};
    std::size_t max_levels = 0;
    for (const auto& factor : factors) {
        table.headers.push_back(factor.factor_name);
        max_levels = std::max(max_levels, factor.level_averages.size());
    }
    for (std::size_t level_index = 0; level_index < max_levels; ++level_index) {
        std::vector<std::string> row;
        row.push_back(std::to_string(level_index + 1));
        for (const auto& factor : factors) {
            if (level_index < factor.level_averages.size()) {
                row.push_back(format_number(factor.level_averages[level_index].average));
            } else {
                row.push_back("*");
            }
        }
        table.rows.push_back(std::move(row));
    }
    std::vector<std::string> delta_row = {"Delta"};
    std::vector<std::string> rank_row = {"Rank"};
    for (const auto& factor : factors) {
        delta_row.push_back(format_number(factor.delta));
        rank_row.push_back(std::to_string(factor.rank));
    }
    table.rows.push_back(std::move(delta_row));
    table.rows.push_back(std::move(rank_row));
    return table;
}

PlotSpec taguchi_main_effects_plot(
    const datalab::domain::statistics::TaguchiFactorResponse& factor,
    const std::string& y_title)
{
    PlotSpec plot;
    plot.kind = PlotKind::scatter;
    plot.title = "主效应图 - " + factor.factor_name + " (" + y_title + ")";
    plot.x_axis_title = factor.factor_name;
    plot.y_axis_title = y_title;
    PlotSeries series;
    series.label = factor.factor_name;
    series.show_points = true;
    for (std::size_t i = 0; i < factor.level_averages.size(); ++i) {
        const double x = static_cast<double>(i + 1);
        plot.x_values.push_back(x);
        plot.values.push_back(factor.level_averages[i].average);
        series.x_values.push_back(x);
        series.values.push_back(factor.level_averages[i].average);
        plot.categories.push_back(factor.level_averages[i].level);
        plot.point_labels.push_back(factor.level_averages[i].level);
    }
    plot.series = {std::move(series)};
    return plot;
}

}  // namespace

OutputPage AnalysisService::taguchi_analyze(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.taguchi_analyze.factor_columns.empty()
        || configuration.taguchi_analyze.response_columns.empty()) {
        return error_page("Taguchi 分析", "Analyze Taguchi Design",
                          "请选择至少一个因子列与一个响应列。");
    }

    std::vector<domain::ExtractedNumericColumn> response_cols;
    for (std::size_t col : configuration.taguchi_analyze.response_columns) {
        response_cols.push_back(
            extract_numeric_column(table, col, configuration.excluded_rows));
    }
    const auto aligned = align_complete_rows_with_source(response_cols);
    if (aligned.values.empty()) {
        return error_page("Taguchi 分析", "Analyze Taguchi Design",
                          "无完整响应行（complete-case）。");
    }

    std::vector<std::string> factor_names;
    for (std::size_t col : configuration.taguchi_analyze.factor_columns) {
        if (col < table.columns.size()) {
            factor_names.push_back(table.columns[col]);
        } else {
            factor_names.push_back("F" + std::to_string(col + 1));
        }
    }

    std::vector<std::vector<std::string>> factor_levels;
    std::vector<std::vector<double>> responses;
    std::vector<std::size_t> source_rows;
    factor_levels.reserve(aligned.source_rows.size());
    responses.reserve(aligned.source_rows.size());

    for (std::size_t i = 0; i < aligned.source_rows.size(); ++i) {
        const std::size_t row = aligned.source_rows[i];
        std::vector<std::string> levels;
        bool skip = false;
        for (std::size_t col : configuration.taguchi_analyze.factor_columns) {
            if (row >= table.rows.size() || col >= table.rows[row].size()
                || table.rows[row][col].empty()) {
                skip = true;
                break;
            }
            levels.push_back(table.rows[row][col]);
        }
        if (skip) {
            continue;
        }
        factor_levels.push_back(std::move(levels));
        responses.push_back(aligned.values[i]);
        source_rows.push_back(row);
    }

    datalab::domain::statistics::TaguchiAnalyzeOptions options;
    options.sn_type = datalab::domain::statistics::parse_taguchi_sn_type(
        configuration.taguchi_analyze.sn_type);
    const auto result = datalab::domain::statistics::analyze_taguchi_static(
        factor_levels, responses, factor_names, source_rows, options);

    OutputPage page;
    page.id = new_id("taguchi_analyze");
    page.title = "Taguchi 分析";
    page.method_name = "Analyze Taguchi Design";
    page.configuration = configuration;
    page.parameter_summary = "S/N = " + result.sn_type_name
        + "    因子 = " + std::to_string(result.factor_count)
        + "    响应列 = " + std::to_string(result.response_count)
        + "    运行 = " + std::to_string(result.run_count);
    page.diagnostics = result.diagnostics;
    if (source_rows.size() < table.rows.size()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "Taguchi 分析使用 complete-case；因子与响应均非空的行才纳入。"});
    }

    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.severity == DiagnosticMessage::Severity::error) {
            page.facts.taguchi_analyze = domain::TaguchiAnalyzeFacts{};
            page.facts.taguchi_analyze->sn_type = result.sn_type_name;
            page.analysis_command_id = "taguchi_analyze";
            return finalize_page(std::move(page));
        }
    }

    page.tables.push_back(
        taguchi_response_table("Response Table for Means", result.means_table));
    page.tables.push_back(taguchi_response_table(
        "Response Table for Signal to Noise Ratios", result.sn_table));

    StatisticTable diagnostics_table;
    diagnostics_table.title = "Diagnostics";
    diagnostics_table.headers = {"Item", "Value"};
    diagnostics_table.rows.push_back({"S/N type", result.sn_type_name});
    diagnostics_table.rows.push_back({"Runs", std::to_string(result.run_count)});
    diagnostics_table.rows.push_back(
        {"Response columns", std::to_string(result.response_count)});
    page.tables.push_back(std::move(diagnostics_table));

    for (const auto& factor : result.means_table) {
        if (!factor.level_averages.empty()) {
            page.plots.push_back(taguchi_main_effects_plot(factor, "Mean"));
        }
    }
    for (const auto& factor : result.sn_table) {
        if (!factor.level_averages.empty()) {
            page.plots.push_back(taguchi_main_effects_plot(factor, "S/N"));
        }
    }
    if (page.plots.empty()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info, "taguchi_analyze_no_plot",
            "无足够水平均值可绘制主效应图。"});
    }

    domain::TaguchiAnalyzeFacts facts;
    facts.sn_type = result.sn_type_name;
    facts.factor_count = result.factor_count;
    facts.response_count = result.response_count;
    facts.run_count = result.run_count;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    if (!result.sn_table.empty()) {
        std::size_t top = 0;
        for (std::size_t i = 1; i < result.sn_table.size(); ++i) {
            if (result.sn_table[i].delta > result.sn_table[top].delta) {
                top = i;
            }
        }
        facts.top_delta = result.sn_table[top].delta;
        facts.top_factor = result.sn_table[top].factor_name;
    }
    page.facts.taguchi_analyze = facts;
    page.analysis_command_id = "taguchi_analyze";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::mixture_design(
    const DataTable&,
    const AnalysisConfiguration& configuration)
{
    datalab::domain::statistics::MixtureDesignOptions options;
    options.component_count = configuration.mixture_design.component_count;
    options.component_names = configuration.mixture_design.component_names;
    options.randomize = configuration.mixture_design.randomize;
    options.random_seed = configuration.mixture_design.random_seed;
    const auto design =
        datalab::domain::statistics::generate_mixture_simplex_lattice(options);

    OutputPage page;
    page.id = new_id("mixture_design");
    page.title = "Mixture 设计";
    page.method_name = "Mixture Design";
    page.configuration = configuration;
    // New worksheet must not inherit prior excluded/hidden rows (A→B honesty).
    page.configuration.excluded_rows.clear();
    page.configuration.hidden_rows.clear();
    page.parameter_summary = "simplex-lattice m=2    q = "
        + std::to_string(design.component_count)
        + "    N = " + std::to_string(design.run_count);
    page.diagnostics = design.diagnostics;

    for (const auto& diagnostic : design.diagnostics) {
        if (diagnostic.severity == DiagnosticMessage::Severity::error) {
            domain::MixtureDesignFacts facts;
            facts.component_count = design.component_count;
            facts.degree = design.degree;
            facts.run_count = design.run_count;
            facts.design_kind = design.design_kind;
            page.facts.mixture_design = facts;
            page.analysis_command_id = "mixture_design";
            return finalize_page(std::move(page));
        }
    }

    StatisticTable info;
    info.title = "Design Info";
    info.headers = {"Property", "Value"};
    info.rows.push_back({"Type", design.design_kind});
    info.rows.push_back({"Components (q)", std::to_string(design.component_count)});
    info.rows.push_back({"Degree (m)", std::to_string(design.degree)});
    info.rows.push_back({"Points (N)", std::to_string(design.run_count)});
    info.rows.push_back({"Formula", "N = q(q+1)/2"});
    page.tables.push_back(std::move(info));

    StatisticTable matrix;
    matrix.title = "Design Matrix";
    matrix.headers = {"StdOrder", "RunOrder"};
    for (const auto& name : design.component_names) {
        matrix.headers.push_back(name);
    }
    for (const auto& run : design.runs) {
        std::vector<std::string> row = {
            std::to_string(run.standard_order), std::to_string(run.run_order)};
        for (double x : run.proportions) {
            row.push_back(format_number(x));
        }
        matrix.rows.push_back(std::move(row));
    }
    page.tables.push_back(std::move(matrix));

    DataTable export_table;
    export_table.name = "mixture_design";
    export_table.source_path = page.id;
    for (const auto& name : design.component_names) {
        export_table.columns.push_back(name);
    }
    export_table.columns.push_back("RunOrder");
    export_table.columns.push_back("Response");
    for (const auto& run : design.runs) {
        std::vector<std::string> row;
        for (double x : run.proportions) {
            row.push_back(format_number(x));
        }
        row.push_back(std::to_string(run.run_order));
        row.push_back("");
        export_table.rows.push_back(std::move(row));
    }
    page.worksheet_export = std::move(export_table);
    page.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "mixture_worksheet_export",
        "已生成设计矩阵工作表（分量比例 + RunOrder + 空 Response）；"
        "新表不携带旧 excluded_rows/hidden_rows。本命令仅设计生成。"});

    domain::MixtureDesignFacts facts;
    facts.component_count = design.component_count;
    facts.degree = design.degree;
    facts.run_count = design.run_count;
    facts.design_kind = design.design_kind;
    facts.evidence_type = design.evidence_type;
    facts.algorithm_id = design.algorithm_id;
    page.facts.mixture_design = facts;
    page.analysis_command_id = "mixture_design";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::mixture_analyze(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.mixture_analyze.component_columns.empty()
        || !configuration.mixture_analyze.response_column.has_value()) {
        return error_page("Mixture 分析", "Analyze Mixture Design",
                          "请选择分量列与响应列。");
    }
    std::vector<std::string> component_names;
    for (std::size_t col : configuration.mixture_analyze.component_columns) {
        component_names.push_back(column_label(table, col));
    }
    std::vector<std::vector<double>> components;
    std::vector<double> response;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        std::vector<double> comp_row;
        bool valid = true;
        for (std::size_t col : configuration.mixture_analyze.component_columns) {
            if (col >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][col]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            comp_row.push_back(*parsed);
        }
        const std::size_t resp_col = *configuration.mixture_analyze.response_column;
        if (!valid || resp_col >= table.rows[row].size()) {
            continue;
        }
        const auto y = parse_numeric_cell(table.rows[row][resp_col]);
        if (!y.has_value()) {
            continue;
        }
        components.push_back(std::move(comp_row));
        response.push_back(*y);
        source_rows.push_back(row);
    }
    datalab::domain::statistics::MixtureAnalyzeOptions options;
    options.model_order = datalab::domain::statistics::parse_mixture_model_order(
        configuration.mixture_analyze.model_order);
    const auto result = datalab::domain::statistics::analyze_mixture_scheffe(
        components, response, component_names, source_rows, options);

    OutputPage page;
    page.id = new_id("mixture_analyze");
    page.title = "Mixture 分析";
    page.method_name = "Analyze Mixture Design";
    page.configuration = configuration;
    page.parameter_summary = "q = " + std::to_string(result.component_count)
        + "    模型 = " + result.model_order
        + "    N = " + std::to_string(result.observation_count);
    page.diagnostics = result.diagnostics;

    StatisticTable coef_table;
    coef_table.title = "Coefficients";
    coef_table.headers = {"Term", "Coef", "SE", "T", "P"};
    for (const auto& coef : result.coefficients) {
        coef_table.rows.push_back({
            coef.term,
            format_number(coef.coefficient),
            format_number(coef.standard_error),
            coef.t_statistic.has_value() ? format_number(*coef.t_statistic) : "*",
            coef.p_value.has_value() ? format_number(*coef.p_value) : "*"});
    }
    page.tables.push_back(std::move(coef_table));

    StatisticTable anova;
    anova.title = "ANOVA";
    anova.headers = {"Source", "Seq SS", "Adj SS", "DF", "MS", "F", "P"};
    for (const auto& effect : result.anova_effects) {
        anova.rows.push_back({
            effect.term,
            effect.sequential_sum_of_squares.has_value()
                ? format_number(*effect.sequential_sum_of_squares) : "*",
            effect.adjusted_sum_of_squares.has_value()
                ? format_number(*effect.adjusted_sum_of_squares) : "*",
            std::to_string(effect.degrees_of_freedom),
            effect.mean_square.has_value() ? format_number(*effect.mean_square) : "*",
            effect.f_statistic.has_value() ? format_number(*effect.f_statistic) : "*",
            effect.p_value.has_value() ? format_number(*effect.p_value) : "*"});
    }
    page.tables.push_back(std::move(anova));

    StatisticTable fits;
    fits.title = "Fits and Residuals";
    fits.headers = {"Source Row", "Observed", "Fitted", "Residual"};
    for (const auto& fit : result.fits) {
        fits.rows.push_back({
            std::to_string(fit.source_row + 1),
            format_number(fit.observed),
            format_number(fit.fitted),
            format_number(fit.residual)});
    }
    page.tables.push_back(std::move(fits));

    domain::MixtureAnalyzeFacts facts;
    facts.component_count = result.component_count;
    facts.observation_count = result.observation_count;
    facts.model_order = result.model_order;
    facts.r_squared = result.r_squared;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.mixture_analyze = facts;
    page.analysis_command_id = "mixture_analyze";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::glm_two_way(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.glm_two_way.response_column.has_value()
        || !configuration.glm_two_way.factor_a_column.has_value()
        || !configuration.glm_two_way.factor_b_column.has_value()) {
        return error_page("双因子 GLM", "GLM Two-Way",
                          "请选择响应列与两个因子列。");
    }
    std::vector<std::string> factor_a;
    std::vector<std::string> factor_b;
    std::vector<double> response;
    std::vector<std::size_t> source_rows;
    const std::size_t col_a = *configuration.glm_two_way.factor_a_column;
    const std::size_t col_b = *configuration.glm_two_way.factor_b_column;
    const std::size_t col_y = *configuration.glm_two_way.response_column;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (col_a >= table.rows[row].size() || col_b >= table.rows[row].size()
            || col_y >= table.rows[row].size()) {
            continue;
        }
        if (table.rows[row][col_a].empty() || table.rows[row][col_b].empty()) {
            continue;
        }
        const auto y = parse_numeric_cell(table.rows[row][col_y]);
        if (!y.has_value()) {
            continue;
        }
        factor_a.push_back(table.rows[row][col_a]);
        factor_b.push_back(table.rows[row][col_b]);
        response.push_back(*y);
        source_rows.push_back(row);
    }
    datalab::domain::statistics::GlmTwoWayOptions options;
    options.include_interaction = configuration.glm_two_way.include_interaction;
    const auto result = datalab::domain::statistics::glm_two_way_analyze(
        factor_a, factor_b, response, source_rows, options);

    OutputPage page;
    page.id = new_id("glm_two_way");
    page.title = "双因子 GLM";
    page.method_name = "GLM Two-Way";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + (result.include_interaction ? "    含交互" : "    主效应");
    page.diagnostics = result.diagnostics;

    StatisticTable anova;
    anova.title = "ANOVA (Type III Adj SS)";
    anova.headers = {"Source", "Adj SS", "DF", "MS", "F", "P"};
    for (const auto& effect : result.anova_effects) {
        anova.rows.push_back({
            effect.term,
            effect.adjusted_sum_of_squares.has_value()
                ? format_number(*effect.adjusted_sum_of_squares) : "*",
            std::to_string(effect.degrees_of_freedom),
            effect.mean_square.has_value() ? format_number(*effect.mean_square) : "*",
            effect.f_statistic.has_value() ? format_number(*effect.f_statistic) : "*",
            effect.p_value.has_value() ? format_number(*effect.p_value) : "*"});
    }
    page.tables.push_back(std::move(anova));

    StatisticTable fitted_means;
    fitted_means.title = "Fitted Means";
    fitted_means.headers = {"Factor", "Level", "N", "Fitted Mean"};
    for (const auto& mean : result.fitted_means) {
        fitted_means.rows.push_back({
            mean.factor, mean.level, std::to_string(mean.count),
            format_number(mean.fitted_mean)});
    }
    page.tables.push_back(std::move(fitted_means));

    StatisticTable residuals;
    residuals.title = "Residual Diagnostics";
    residuals.headers = {"Source Row", "Fitted", "Residual"};
    for (std::size_t i = 0; i < result.residuals.size(); ++i) {
        residuals.rows.push_back({
            i < result.observation_source_rows.size()
                ? std::to_string(result.observation_source_rows[i] + 1) : "*",
            i < result.fitted.size() ? format_number(result.fitted[i]) : "*",
            format_number(result.residuals[i])});
    }
    page.tables.push_back(std::move(residuals));

    domain::GlmTwoWayFacts facts;
    facts.observation_count = result.observation_count;
    facts.include_interaction = result.include_interaction;
    facts.design_balanced = result.design_balanced;
    facts.residual_normality_p = result.residual_normality_p;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.glm_two_way = facts;
    page.analysis_command_id = "glm_two_way";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::analyze_variability(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.analyze_variability.factor_columns.empty()
        || configuration.analyze_variability.replicate_columns.empty()) {
        return error_page("Analyze Variability", "Analyze Variability",
                          "请选择因子列与重复响应列。");
    }
    std::vector<std::string> factor_names;
    for (std::size_t col : configuration.analyze_variability.factor_columns) {
        factor_names.push_back(column_label(table, col));
    }
    std::vector<std::vector<std::string>> factor_levels;
    std::vector<std::vector<double>> replicates;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        std::vector<std::string> levels;
        bool valid = true;
        for (std::size_t col : configuration.analyze_variability.factor_columns) {
            if (col >= table.rows[row].size() || table.rows[row][col].empty()) {
                valid = false;
                break;
            }
            levels.push_back(table.rows[row][col]);
        }
        std::vector<double> reps;
        for (std::size_t col : configuration.analyze_variability.replicate_columns) {
            if (col >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][col]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            reps.push_back(*parsed);
        }
        if (!valid || reps.empty()) {
            continue;
        }
        factor_levels.push_back(std::move(levels));
        replicates.push_back(std::move(reps));
        source_rows.push_back(row);
    }
    datalab::domain::statistics::AnalyzeVariabilityOptions options;
    options.estimation_method = configuration.analyze_variability.estimation_method;
    const auto result = datalab::domain::statistics::analyze_variability_dispersion(
        factor_levels, replicates, factor_names, source_rows, options);

    OutputPage page;
    page.id = new_id("analyze_variability");
    page.title = "Analyze Variability";
    page.method_name = "Analyze Variability";
    page.configuration = configuration;
    page.parameter_summary = "运行 = " + std::to_string(result.run_count)
        + "    因子 = " + std::to_string(result.factor_count)
        + "    重复 = " + std::to_string(result.replicate_count);
    page.diagnostics = result.diagnostics;

    StatisticTable std_table;
    std_table.title = "Std Dev by Run";
    std_table.headers = {"Source Row", "Std Dev", "ln(Std Dev)", "Replicates"};
    for (const auto& run : result.runs) {
        std_table.rows.push_back({
            std::to_string(run.source_row + 1),
            format_number(run.std_dev),
            format_number(run.log_std_dev),
            std::to_string(run.replicate_count)});
    }
    page.tables.push_back(std::move(std_table));

    StatisticTable coef;
    coef.title = "Dispersion Effects";
    coef.headers = {"Term", "Coef", "Effect (2×Coef)", "SE"};
    for (const auto& c : result.coefficients) {
        coef.rows.push_back({
            c.term, format_number(c.coefficient), format_number(c.effect),
            c.standard_error.has_value() ? format_number(*c.standard_error) : "*"});
    }
    page.tables.push_back(std::move(coef));

    domain::AnalyzeVariabilityFacts facts;
    facts.run_count = result.run_count;
    facts.factor_count = result.factor_count;
    facts.replicate_count = result.replicate_count;
    facts.estimation_method = result.estimation_method;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.analyze_variability = facts;
    page.analysis_command_id = "analyze_variability";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::factor_analysis(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::vector<std::size_t>& columns =
        configuration.factor_analysis.variable_columns.empty()
            ? configuration.variable_columns
            : configuration.factor_analysis.variable_columns;
    if (columns.size() < 3) {
        return error_page("因子分析", "Factor Analysis", "至少需要选择三个数值变量。");
    }
    std::vector<std::vector<double>> rows;
    std::vector<std::string> names;
    for (std::size_t col : columns) {
        names.push_back(column_label(table, col));
    }
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (std::size_t col : columns) {
            if (col >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][col]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (valid) {
            rows.push_back(std::move(values));
        }
    }
    datalab::domain::statistics::FactorAnalysisOptions options;
    options.factor_count = configuration.factor_analysis.factor_count;
    options.use_kaiser_rule = configuration.factor_analysis.use_kaiser_rule;
    options.varimax_rotation = configuration.factor_analysis.varimax_rotation;
    const auto result = datalab::domain::statistics::factor_analysis_extract(
        rows, names, {}, options);

    OutputPage page;
    page.id = new_id("factor_analysis");
    page.title = "因子分析";
    page.method_name = "Factor Analysis";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    变量 = " + std::to_string(result.variable_count)
        + "    因子 = " + std::to_string(result.retained_factor_count);
    page.diagnostics = result.diagnostics;

    StatisticTable loadings;
    loadings.title = "Factor Loadings";
    loadings.headers = {"Variable"};
    for (std::size_t f = 0; f < result.retained_factor_count; ++f) {
        loadings.headers.push_back("F" + std::to_string(f + 1));
    }
    loadings.headers.push_back("Communality");
    for (const auto& row : result.loadings_table) {
        std::vector<std::string> table_row = {row.variable};
        for (double value : row.loadings) {
            table_row.push_back(format_number(value));
        }
        table_row.push_back(format_number(row.communality));
        loadings.rows.push_back(std::move(table_row));
    }
    page.tables.push_back(std::move(loadings));

    StatisticTable variance;
    variance.title = "Variance Explained";
    variance.headers = {"Factor", "Eigenvalue", "% Var", "Cumulative %"};
    for (const auto& row : result.variance_explained) {
        variance.rows.push_back({
            std::to_string(row.factor_index),
            format_number(row.eigenvalue),
            format_number(row.percent_variance),
            format_number(row.cumulative_percent)});
    }
    page.tables.push_back(std::move(variance));

    if (!result.eigenvalues.empty()) {
        PlotSpec scree;
        scree.kind = PlotKind::scatter;
        scree.title = "Scree Plot";
        scree.x_axis_title = "Component";
        scree.y_axis_title = "Eigenvalue";
        for (std::size_t i = 0; i < result.eigenvalues.size(); ++i) {
            scree.x_values.push_back(static_cast<double>(i + 1));
            scree.values.push_back(result.eigenvalues[i]);
        }
        page.plots.push_back(std::move(scree));
    }

    domain::FactorAnalysisFacts facts;
    facts.observation_count = result.observation_count;
    facts.variable_count = result.variable_count;
    facts.retained_factor_count = result.retained_factor_count;
    facts.varimax_applied = result.varimax_applied;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.factor_analysis = facts;
    page.analysis_command_id = "factor_analysis";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::binary_response_doe(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.binary_response_doe.factor_columns.empty()) {
        return error_page("二值响应 DOE", "Binary Response DOE",
                          "请至少选择一个因子列。");
    }
    const bool use_events_trials = configuration.binary_response_doe.use_events_trials;
    if (use_events_trials) {
        if (!configuration.binary_response_doe.events_column.has_value()
            || !configuration.binary_response_doe.trials_column.has_value()) {
            return error_page("二值响应 DOE", "Binary Response DOE",
                              "events/trials 模式需要事件列与试验列。");
        }
    } else if (!configuration.binary_response_doe.binary_column.has_value()) {
        return error_page("二值响应 DOE", "Binary Response DOE",
                          "0/1 模式需要二值响应列。");
    }

    std::vector<std::vector<std::string>> factor_columns;
    std::vector<std::string> factor_labels;
    for (std::size_t col : configuration.binary_response_doe.factor_columns) {
        factor_labels.push_back(column_label(table, col));
        factor_columns.emplace_back();
    }
    std::vector<int> events;
    std::vector<int> trials;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        bool valid = true;
        for (std::size_t index = 0; index < configuration.binary_response_doe.factor_columns.size();
             ++index) {
            const std::size_t col = configuration.binary_response_doe.factor_columns[index];
            if (col >= table.rows[row].size() || table.rows[row][col].empty()) {
                valid = false;
                break;
            }
            factor_columns[index].push_back(table.rows[row][col]);
        }
        if (!valid) {
            continue;
        }
        if (use_events_trials) {
            const auto event = parse_numeric_cell(
                table.rows[row][*configuration.binary_response_doe.events_column]);
            const auto trial = parse_numeric_cell(
                table.rows[row][*configuration.binary_response_doe.trials_column]);
            if (!event.has_value() || !trial.has_value()) {
                continue;
            }
            events.push_back(static_cast<int>(*event));
            trials.push_back(static_cast<int>(*trial));
        } else {
            const auto binary = parse_numeric_cell(
                table.rows[row][*configuration.binary_response_doe.binary_column]);
            if (!binary.has_value()) {
                continue;
            }
            const int value = static_cast<int>(*binary);
            events.push_back(value == 0 ? 0 : 1);
            trials.push_back(1);
        }
        source_rows.push_back(row);
    }

    datalab::domain::statistics::BinaryResponseDoeOptions options;
    options.include_ab_interaction =
        configuration.binary_response_doe.include_ab_interaction;
    options.use_events_trials = use_events_trials;
    const auto result = datalab::domain::statistics::analyze_binary_response_doe(
        factor_columns, events, trials, factor_labels, source_rows, options);

    OutputPage page;
    page.id = new_id("binary_response_doe");
    page.title = "二值响应 DOE";
    page.method_name = "Binary Response DOE";
    page.configuration = configuration;
    page.parameter_summary = "设计行 = " + std::to_string(result.design_row_count)
        + "    展开 N = " + std::to_string(result.expanded_observation_count)
        + "    Link = logit";
    page.diagnostics = result.diagnostics;

    StatisticTable coef;
    coef.title = "Coefficients (Logit IRWLS)";
    coef.headers = {"Term", "Coef", "SE", "Z", "P", "Odds Ratio"};
    for (const auto& row : result.coefficients) {
        coef.rows.push_back({
            row.term,
            format_number(row.coefficient),
            format_number(row.standard_error),
            format_number(row.z_statistic),
            format_number(row.p_value),
            format_number(row.odds_ratio)});
    }
    page.tables.push_back(std::move(coef));

    StatisticTable fit;
    fit.title = "Goodness-of-Fit";
    fit.headers = {"Metric", "Value"};
    fit.rows.push_back({"Deviance", format_number(result.deviance)});
    fit.rows.push_back({"AIC", format_number(result.aic)});
    fit.rows.push_back({"Iterations", std::to_string(result.iteration_count)});
    fit.rows.push_back({"Converged", result.converged ? "Yes" : "No"});
    page.tables.push_back(std::move(fit));

    domain::BinaryResponseDoeFacts facts;
    facts.design_row_count = result.design_row_count;
    facts.expanded_observation_count = result.expanded_observation_count;
    facts.factor_count = result.factor_count;
    facts.event_count = result.event_count;
    facts.trial_count = result.trial_count;
    facts.include_ab_interaction = result.include_ab_interaction;
    facts.converged = result.converged;
    facts.deviance = result.deviance;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.binary_response_doe = facts;
    page.analysis_command_id = "binary_response_doe";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::cluster_variables(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::vector<std::size_t>& columns =
        configuration.cluster_variables.variable_columns.empty()
            ? configuration.variable_columns
            : configuration.cluster_variables.variable_columns;
    if (columns.size() < 3) {
        return error_page("变量聚类", "Cluster Variables",
                          "至少需要三个数值变量。");
    }
    std::vector<std::vector<double>> rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        std::vector<double> values;
        bool valid = true;
        for (std::size_t col : columns) {
            if (col >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][col]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            values.push_back(*parsed);
        }
        if (valid) {
            rows.push_back(std::move(values));
        }
    }
    std::vector<std::string> labels;
    for (std::size_t col : columns) {
        labels.push_back(column_label(table, col));
    }
    datalab::domain::statistics::ClusterVariablesOptions options;
    options.linkage = configuration.cluster_variables.linkage;
    options.use_absolute_correlation =
        configuration.cluster_variables.use_absolute_correlation;
    const auto result = datalab::domain::statistics::cluster_variables_analyze(
        rows, labels, options);

    OutputPage page;
    page.id = new_id("cluster_variables");
    page.title = "变量聚类";
    page.method_name = "Cluster Variables";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + std::to_string(result.variable_count)
        + "    N = " + std::to_string(result.observation_count)
        + "    连结 = " + result.linkage;
    page.diagnostics = result.diagnostics;

    StatisticTable merges;
    merges.title = "Amalgamation Steps";
    merges.headers = {"Step", "Left", "Right", "Height", "Similarity"};
    for (const auto& merge : result.merges) {
        merges.rows.push_back({
            std::to_string(merge.step),
            merge.left_label,
            merge.right_label,
            format_number(merge.height),
            format_number(merge.similarity)});
    }
    page.tables.push_back(std::move(merges));

    if (!result.merges.empty()) {
        PlotSpec dendrogram;
        dendrogram.kind = PlotKind::scatter;
        dendrogram.title = "Dendrogram (Linkage Heights)";
        dendrogram.x_axis_title = "Step";
        dendrogram.y_axis_title = "Height";
        for (const auto& merge : result.merges) {
            dendrogram.x_values.push_back(static_cast<double>(merge.step));
            dendrogram.values.push_back(merge.height);
        }
        page.plots.push_back(std::move(dendrogram));
    }

    domain::ClusterVariablesFacts facts;
    facts.observation_count = result.observation_count;
    facts.variable_count = result.variable_count;
    facts.merge_count = result.merges.size();
    facts.linkage = result.linkage;
    facts.max_distance = result.max_distance;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.cluster_variables = facts;
    page.analysis_command_id = "cluster_variables";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::glm_three_factor(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.glm_three_factor.response_column.has_value()
        || !configuration.glm_three_factor.factor_a_column.has_value()
        || !configuration.glm_three_factor.factor_b_column.has_value()
        || !configuration.glm_three_factor.factor_c_column.has_value()) {
        return error_page("三因子 GLM", "GLM Three-Factor",
                          "请选择响应列与三个因子列。");
    }
    std::vector<std::string> factor_a;
    std::vector<std::string> factor_b;
    std::vector<std::string> factor_c;
    std::vector<double> response;
    std::vector<std::size_t> source_rows;
    const std::size_t col_a = *configuration.glm_three_factor.factor_a_column;
    const std::size_t col_b = *configuration.glm_three_factor.factor_b_column;
    const std::size_t col_c = *configuration.glm_three_factor.factor_c_column;
    const std::size_t col_y = *configuration.glm_three_factor.response_column;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (col_a >= table.rows[row].size() || col_b >= table.rows[row].size()
            || col_c >= table.rows[row].size() || col_y >= table.rows[row].size()) {
            continue;
        }
        if (table.rows[row][col_a].empty() || table.rows[row][col_b].empty()
            || table.rows[row][col_c].empty()) {
            continue;
        }
        const auto y = parse_numeric_cell(table.rows[row][col_y]);
        if (!y.has_value()) {
            continue;
        }
        factor_a.push_back(table.rows[row][col_a]);
        factor_b.push_back(table.rows[row][col_b]);
        factor_c.push_back(table.rows[row][col_c]);
        response.push_back(*y);
        source_rows.push_back(row);
    }
    datalab::domain::statistics::GlmThreeFactorOptions options;
    options.include_ab_interaction =
        configuration.glm_three_factor.include_ab_interaction;
    options.include_ac_interaction =
        configuration.glm_three_factor.include_ac_interaction;
    options.include_bc_interaction =
        configuration.glm_three_factor.include_bc_interaction;
    const auto result = datalab::domain::statistics::glm_three_factor_analyze(
        factor_a, factor_b, factor_c, response, source_rows, options);

    OutputPage page;
    page.id = new_id("glm_three_factor");
    page.title = "三因子 GLM";
    page.method_name = "GLM Three-Factor";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + (result.design_balanced ? "    平衡" : "    不平衡");
    page.diagnostics = result.diagnostics;

    StatisticTable anova;
    anova.title = "ANOVA (Type III Adj SS)";
    anova.headers = {"Source", "Adj SS", "DF", "MS", "F", "P"};
    for (const auto& effect : result.anova_effects) {
        anova.rows.push_back({
            effect.term,
            effect.adjusted_sum_of_squares.has_value()
                ? format_number(*effect.adjusted_sum_of_squares) : "*",
            std::to_string(effect.degrees_of_freedom),
            effect.mean_square.has_value() ? format_number(*effect.mean_square) : "*",
            effect.f_statistic.has_value() ? format_number(*effect.f_statistic) : "*",
            effect.p_value.has_value() ? format_number(*effect.p_value) : "*"});
    }
    page.tables.push_back(std::move(anova));

    StatisticTable fitted_means;
    fitted_means.title = "Fitted Means";
    fitted_means.headers = {"Factor", "Level", "N", "Fitted Mean"};
    for (const auto& mean : result.fitted_means) {
        fitted_means.rows.push_back({
            mean.factor, mean.level, std::to_string(mean.count),
            format_number(mean.fitted_mean)});
    }
    page.tables.push_back(std::move(fitted_means));

    domain::GlmThreeFactorFacts facts;
    facts.observation_count = result.observation_count;
    facts.include_ab_interaction = result.include_ab_interaction;
    facts.include_ac_interaction = result.include_ac_interaction;
    facts.include_bc_interaction = result.include_bc_interaction;
    facts.design_balanced = result.design_balanced;
    facts.residual_normality_p = result.residual_normality_p;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.glm_three_factor = facts;
    page.analysis_command_id = "glm_three_factor";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::life_data_regression(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.life_data_regression.time_column.has_value()
        || !configuration.life_data_regression.censor_column.has_value()
        || configuration.life_data_regression.covariate_columns.empty()) {
        return error_page("寿命数据回归", "Life Data Regression",
                          "请选择时间列、删失列与至少一个协变量。");
    }
    if (configuration.life_data_regression.covariate_columns.size() > 2) {
        return error_page("寿命数据回归", "Life Data Regression",
                          "窄化实现最多支持 2 个协变量。");
    }
    const std::size_t time_col = *configuration.life_data_regression.time_column;
    const std::size_t censor_col = *configuration.life_data_regression.censor_column;
    std::vector<double> times;
    std::vector<bool> events;
    std::vector<std::vector<double>> covariates;
    std::vector<std::size_t> source_rows;
    std::vector<std::string> covariate_labels;
    for (std::size_t col : configuration.life_data_regression.covariate_columns) {
        covariate_labels.push_back(column_label(table, col));
    }
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (time_col >= table.rows[row].size()
            || censor_col >= table.rows[row].size()) {
            continue;
        }
        const auto time = parse_numeric_cell(table.rows[row][time_col]);
        if (!time.has_value() || *time <= 0.0) {
            continue;
        }
        const auto censor = parse_numeric_cell(table.rows[row][censor_col]);
        if (!censor.has_value()) {
            continue;
        }
        std::vector<double> cov;
        bool valid = true;
        for (std::size_t col : configuration.life_data_regression.covariate_columns) {
            if (col >= table.rows[row].size()) {
                valid = false;
                break;
            }
            const auto parsed = parse_numeric_cell(table.rows[row][col]);
            if (!parsed.has_value()) {
                valid = false;
                break;
            }
            cov.push_back(*parsed);
        }
        if (!valid) {
            continue;
        }
        times.push_back(*time);
        events.push_back(*censor > 0.5);
        covariates.push_back(std::move(cov));
        source_rows.push_back(row);
    }
    datalab::domain::statistics::LifeDataRegressionOptions options;
    options.percentile_levels = configuration.life_data_regression.percentile_levels;
    const auto result = datalab::domain::statistics::fit_life_data_regression_weibull(
        times, events, covariates, covariate_labels, source_rows, options);

    OutputPage page;
    page.id = new_id("life_data_regression");
    page.title = "寿命数据回归";
    page.method_name = "Life Data Regression";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    失败 = " + std::to_string(result.failure_count)
        + "    删失 = " + std::to_string(result.censored_count)
        + "    Shape = " + format_number(result.shape);
    page.diagnostics = result.diagnostics;

    StatisticTable regression;
    regression.title = "Regression Table";
    regression.headers = {"Term", "Coef", "SE", "Z", "P", "Lower", "Upper"};
    for (const auto& row : result.coefficients) {
        regression.rows.push_back({
            row.term,
            format_number(row.estimate),
            format_number(row.standard_error),
            format_number(row.z_statistic),
            format_number(row.p_value),
            format_number(row.confidence_lower),
            format_number(row.confidence_upper)});
    }
    page.tables.push_back(std::move(regression));

    if (!result.percentiles.empty()) {
        StatisticTable percentiles;
        percentiles.title = "Percentile Table";
        percentiles.headers = {"Profile", "Percentile", "Life"};
        for (const auto& row : result.percentiles) {
            percentiles.rows.push_back({
                row.covariate_profile,
                format_number(row.percentile),
                format_number(row.life)});
        }
        page.tables.push_back(std::move(percentiles));
    }

    domain::LifeDataRegressionFacts facts;
    facts.observation_count = result.observation_count;
    facts.failure_count = result.failure_count;
    facts.censored_count = result.censored_count;
    facts.covariate_count = result.covariate_count;
    facts.converged = result.converged;
    facts.shape = result.shape;
    facts.distribution = result.distribution;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.life_data_regression = facts;
    page.analysis_command_id = "life_data_regression";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::expanded_gage_unbalanced(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto& cfg = configuration.expanded_gage_unbalanced;
    if (!cfg.measurement_column.has_value() || !cfg.part_column.has_value()
        || !cfg.operator_column.has_value()) {
        return error_page("不平衡 Expanded Gage R&R", "Expanded Gage Unbalanced",
                          "请选择测量、Part 与 Operator 列。");
    }
    std::vector<double> measurements;
    std::vector<std::string> parts;
    std::vector<std::string> operators;
    std::vector<std::string> additional;
    std::vector<std::size_t> source_rows;
    const std::size_t col_m = *cfg.measurement_column;
    const std::size_t col_p = *cfg.part_column;
    const std::size_t col_o = *cfg.operator_column;
    const std::size_t col_a = cfg.additional_column.value_or(0);
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (col_m >= table.rows[row].size() || col_p >= table.rows[row].size()
            || col_o >= table.rows[row].size()) {
            continue;
        }
        if (table.rows[row][col_p].empty() || table.rows[row][col_o].empty()) {
            continue;
        }
        if (cfg.include_additional_factor) {
            if (col_a >= table.rows[row].size() || table.rows[row][col_a].empty()) {
                continue;
            }
        }
        const auto y = parse_numeric_cell(table.rows[row][col_m]);
        if (!y.has_value()) {
            continue;
        }
        measurements.push_back(*y);
        parts.push_back(table.rows[row][col_p]);
        operators.push_back(table.rows[row][col_o]);
        additional.push_back(cfg.include_additional_factor ? table.rows[row][col_a] : std::string{});
        source_rows.push_back(row);
    }
    datalab::domain::statistics::ExpandedGageUnbalancedOptions options;
    options.include_additional_factor = cfg.include_additional_factor;
    options.part_random = cfg.part_random;
    options.operator_random = cfg.operator_random;
    options.additional_random = cfg.additional_random;
    options.study_var_multiplier = cfg.study_var_multiplier;
    const double tolerance = cfg.tolerance.value_or(0.0);
    const auto result = datalab::domain::statistics::expanded_gage_unbalanced_analyze(
        measurements, parts, operators, additional, tolerance, source_rows, options);

    OutputPage page;
    page.id = new_id("expanded_gage_unbalanced");
    page.title = "不平衡 Expanded Gage R&R";
    page.method_name = "Expanded Gage Unbalanced";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    Part = " + std::to_string(result.part_count)
        + "    Operator = " + std::to_string(result.operator_count)
        + (result.design_balanced ? "    平衡" : "    不平衡");
    page.diagnostics = result.diagnostics;

    StatisticTable anova;
    anova.title = "ANOVA";
    anova.headers = {"Source", "DF", "SS", "MS", "F", "P"};
    for (const auto& row : result.anova_rows) {
        anova.rows.push_back({
            row.source,
            std::to_string(row.degrees_of_freedom),
            format_number(row.sum_of_squares),
            format_number(row.mean_square),
            format_number(row.f_statistic),
            row.p_value.has_value() ? format_number(*row.p_value) : "*"});
    }
    page.tables.push_back(std::move(anova));

    StatisticTable varcomp;
    varcomp.title = "Gage R&R VarComp";
    varcomp.headers = {"Source", "VarComp", "%Contribution", "Study Var", "%Study Var", "NDC"};
    for (const auto& comp : result.variance_components) {
        varcomp.rows.push_back({
            comp.source,
            format_number(comp.variance_component),
            format_number(comp.percent_contribution),
            format_number(comp.study_variation),
            format_number(comp.percent_study_variation),
            comp.source == "Total Gage R&R" && result.ndc_available
                ? format_number(result.ndc) : ""});
    }
    page.tables.push_back(std::move(varcomp));

    domain::ExpandedGageUnbalancedFacts facts;
    facts.observation_count = result.observation_count;
    facts.part_count = result.part_count;
    facts.operator_count = result.operator_count;
    facts.design_balanced = result.design_balanced;
    facts.has_additional_factor = result.has_additional_factor;
    facts.ndc = result.ndc;
    facts.ndc_available = result.ndc_available;
    facts.gage_rr_percent_study_var = result.gage_rr_percent_study_var;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.expanded_gage_unbalanced = facts;
    page.analysis_command_id = "expanded_gage_unbalanced";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::split_plot_analyze(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto& cfg = configuration.split_plot_analyze;
    if (!cfg.response_column.has_value() || !cfg.htc_factor_column.has_value()
        || !cfg.etc_factor_a_column.has_value() || !cfg.whole_plot_column.has_value()) {
        return error_page("裂区析因分析", "Split-Plot Analyze",
                          "请选择响应、难改/易改因子与 WP 列。");
    }
    std::vector<double> response;
    std::vector<std::string> htc;
    std::vector<std::string> etc_a;
    std::vector<std::string> etc_b;
    std::vector<std::string> wp;
    std::vector<std::size_t> source_rows;
    const bool has_etc_b = cfg.etc_factor_b_column.has_value();
    const std::size_t col_y = *cfg.response_column;
    const std::size_t col_htc = *cfg.htc_factor_column;
    const std::size_t col_etc_a = *cfg.etc_factor_a_column;
    const std::size_t col_etc_b = cfg.etc_factor_b_column.value_or(0);
    const std::size_t col_wp = *cfg.whole_plot_column;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (col_y >= table.rows[row].size() || col_htc >= table.rows[row].size()
            || col_etc_a >= table.rows[row].size() || col_wp >= table.rows[row].size()) {
            continue;
        }
        if (table.rows[row][col_htc].empty() || table.rows[row][col_etc_a].empty()
            || table.rows[row][col_wp].empty()) {
            continue;
        }
        const auto y = parse_numeric_cell(table.rows[row][col_y]);
        if (!y.has_value()) {
            continue;
        }
        if (has_etc_b) {
            if (col_etc_b >= table.rows[row].size() || table.rows[row][col_etc_b].empty()) {
                continue;
            }
            etc_b.push_back(table.rows[row][col_etc_b]);
        }
        response.push_back(*y);
        htc.push_back(table.rows[row][col_htc]);
        etc_a.push_back(table.rows[row][col_etc_a]);
        wp.push_back(table.rows[row][col_wp]);
        source_rows.push_back(row);
    }
    datalab::domain::statistics::SplitPlotAnalyzeOptions options;
    options.include_htc_etc_interaction = cfg.include_htc_etc_interaction;
    options.include_etc_interaction = cfg.include_etc_interaction;
    const auto result = datalab::domain::statistics::split_plot_analyze(
        response, htc, etc_a, wp, etc_b, source_rows, options);

    OutputPage page;
    page.id = new_id("split_plot_analyze");
    page.title = "裂区析因分析";
    page.method_name = "Split-Plot Analyze";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    WP = " + std::to_string(result.whole_plot_count);
    page.diagnostics = result.diagnostics;

    StatisticTable anova;
    anova.title = "Split-Plot ANOVA";
    anova.headers = {"Source", "Layer", "SS", "DF", "MS", "F", "P"};
    for (const auto& effect : result.anova_effects) {
        anova.rows.push_back({
            effect.term,
            effect.error_layer,
            effect.sum_of_squares.has_value() ? format_number(*effect.sum_of_squares) : "*",
            std::to_string(effect.degrees_of_freedom),
            effect.mean_square.has_value() ? format_number(*effect.mean_square) : "*",
            effect.f_statistic.has_value() ? format_number(*effect.f_statistic) : "*",
            effect.p_value.has_value() ? format_number(*effect.p_value) : "*"});
    }
    page.tables.push_back(std::move(anova));

    StatisticTable fits;
    fits.title = "Fits and Residuals";
    fits.headers = {"Source Row", "WP", "Observed", "Fitted", "Residual", "WP Residual"};
    for (const auto& row : result.fits) {
        fits.rows.push_back({
            std::to_string(row.source_row),
            row.whole_plot_id,
            format_number(row.observed),
            format_number(row.fitted),
            format_number(row.residual),
            format_number(row.whole_plot_residual)});
    }
    page.tables.push_back(std::move(fits));

    domain::SplitPlotAnalyzeFacts facts;
    facts.observation_count = result.observation_count;
    facts.whole_plot_count = result.whole_plot_count;
    facts.include_htc_etc_interaction = result.include_htc_etc_interaction;
    facts.include_etc_interaction = result.include_etc_interaction;
    facts.wp_r_squared = result.wp_r_squared;
    facts.sp_r_squared = result.sp_r_squared;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.split_plot_analyze = facts;
    page.analysis_command_id = "split_plot_analyze";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::mixture_process_variable(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto& cfg = configuration.mixture_process_variable;
    if (cfg.component_columns.size() < 2 || !cfg.response_column.has_value()
        || !cfg.process_column.has_value()) {
        return error_page("Mixture + 过程变量", "Mixture Process Variable",
                          "请选择 2～4 个组分列、响应列与过程变量列。");
    }
    std::vector<std::vector<double>> components;
    std::vector<double> process;
    std::vector<double> response;
    std::vector<std::size_t> source_rows;
    std::vector<std::string> component_names;
    for (std::size_t col : cfg.component_columns) {
        component_names.push_back(
            col < table.columns.size() ? table.columns[col] : ("x" + std::to_string(col + 1)));
    }
    const std::size_t col_y = *cfg.response_column;
    const std::size_t col_x = *cfg.process_column;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (col_y >= table.rows[row].size() || col_x >= table.rows[row].size()) {
            continue;
        }
        std::vector<double> comp_row;
        bool ok = true;
        for (std::size_t col : cfg.component_columns) {
            if (col >= table.rows[row].size()) {
                ok = false;
                break;
            }
            const auto value = parse_numeric_cell(table.rows[row][col]);
            if (!value.has_value()) {
                ok = false;
                break;
            }
            comp_row.push_back(*value);
        }
        const auto y = parse_numeric_cell(table.rows[row][col_y]);
        const auto x = parse_numeric_cell(table.rows[row][col_x]);
        if (!ok || !y.has_value() || !x.has_value()) {
            continue;
        }
        components.push_back(comp_row);
        process.push_back(*x);
        response.push_back(*y);
        source_rows.push_back(row);
    }
    datalab::domain::statistics::MixtureProcessVariableOptions options;
    options.component_order =
        datalab::domain::statistics::parse_mixture_model_order(cfg.component_order);
    options.include_component_process_interaction =
        cfg.include_component_process_interaction;
    options.sum_tolerance = cfg.sum_tolerance;
    const auto result = datalab::domain::statistics::analyze_mixture_process_variable(
        components, process, response, component_names, source_rows, options);

    OutputPage page;
    page.id = new_id("mixture_process_variable");
    page.title = "Mixture + 过程变量";
    page.method_name = "Mixture Process Variable";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    q = " + std::to_string(result.component_count)
        + "    R² = " + format_number(result.r_squared);
    page.diagnostics = result.diagnostics;

    StatisticTable coef;
    coef.title = "Coefficients";
    coef.headers = {"Term", "Coef", "SE", "T", "P"};
    for (const auto& row : result.coefficients) {
        coef.rows.push_back({
            row.term,
            format_number(row.coefficient),
            row.standard_error > 0.0 ? format_number(row.standard_error) : "*",
            row.t_statistic.has_value() ? format_number(*row.t_statistic) : "*",
            row.p_value.has_value() ? format_number(*row.p_value) : "*"});
    }
    page.tables.push_back(std::move(coef));

    StatisticTable anova;
    anova.title = "ANOVA";
    anova.headers = {"Source", "Adj SS", "DF", "MS", "F", "P"};
    for (const auto& effect : result.anova_effects) {
        anova.rows.push_back({
            effect.term,
            effect.adjusted_sum_of_squares.has_value()
                ? format_number(*effect.adjusted_sum_of_squares) : "*",
            std::to_string(effect.degrees_of_freedom),
            effect.mean_square.has_value() ? format_number(*effect.mean_square) : "*",
            effect.f_statistic.has_value() ? format_number(*effect.f_statistic) : "*",
            effect.p_value.has_value() ? format_number(*effect.p_value) : "*"});
    }
    page.tables.push_back(std::move(anova));

    domain::MixtureProcessVariableFacts facts;
    facts.component_count = result.component_count;
    facts.observation_count = result.observation_count;
    facts.component_order = result.component_order;
    facts.include_component_process_interaction =
        result.include_component_process_interaction;
    facts.r_squared = result.r_squared;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.mixture_process_variable = facts;
    page.analysis_command_id = "mixture_process_variable";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::manova_one_way(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto& cfg = configuration.manova_one_way;
    if (cfg.response_columns.size() < 2 || !cfg.factor_column.has_value()) {
        return error_page("单因子 MANOVA", "MANOVA One-Way",
                          "请选择 2～4 个响应列与 1 个因子列。");
    }
    std::vector<std::vector<double>> responses;
    std::vector<std::string> factor;
    std::vector<std::size_t> source_rows;
    const std::size_t col_f = *cfg.factor_column;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (col_f >= table.rows[row].size() || table.rows[row][col_f].empty()) {
            continue;
        }
        std::vector<double> y_row;
        bool ok = true;
        for (std::size_t col : cfg.response_columns) {
            if (col >= table.rows[row].size()) {
                ok = false;
                break;
            }
            const auto value = parse_numeric_cell(table.rows[row][col]);
            if (!value.has_value()) {
                ok = false;
                break;
            }
            y_row.push_back(*value);
        }
        if (!ok) {
            continue;
        }
        responses.push_back(y_row);
        factor.push_back(table.rows[row][col_f]);
        source_rows.push_back(row);
    }
    datalab::domain::statistics::ManovaOneWayOptions options;
    options.wilks = cfg.wilks;
    options.pillai = cfg.pillai;
    options.lawley_hotelling = cfg.lawley_hotelling;
    options.roy = cfg.roy;
    const auto result = datalab::domain::statistics::manova_one_way_analyze(
        responses, factor, source_rows, options);

    OutputPage page;
    page.id = new_id("manova_one_way");
    page.title = "单因子 MANOVA";
    page.method_name = "MANOVA One-Way";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    响应 = " + std::to_string(result.response_count)
        + "    组 = " + std::to_string(result.group_count);
    page.diagnostics = result.diagnostics;

    StatisticTable tests;
    tests.title = "MANOVA Test Table";
    tests.headers = {"Test", "Value", "F", "Num DF", "Den DF", "P", "Approx"};
    for (const auto& row : result.test_rows) {
        tests.rows.push_back({
            row.test_name,
            format_number(row.value),
            row.f_statistic.has_value() ? format_number(*row.f_statistic) : "*",
            row.numerator_df.has_value() ? format_number(*row.numerator_df) : "*",
            row.denominator_df.has_value() ? format_number(*row.denominator_df) : "*",
            row.p_value.has_value() ? format_number(*row.p_value) : "*",
            row.approximate ? "yes" : "no"});
    }
    page.tables.push_back(std::move(tests));

    StatisticTable means;
    means.title = "Group Mean Vectors";
    means.headers = {"Group", "N"};
    for (std::size_t j = 0; j < result.response_count; ++j) {
        means.headers.push_back("Y" + std::to_string(j + 1));
    }
    for (const auto& group : result.group_means) {
        std::vector<std::string> row = {group.group, std::to_string(group.count)};
        for (double value : group.means) {
            row.push_back(format_number(value));
        }
        means.rows.push_back(row);
    }
    page.tables.push_back(std::move(means));

    if (!result.eigenvalues.empty()) {
        StatisticTable eigen;
        eigen.title = "Eigen Analysis";
        eigen.headers = {"Index", "Eigenvalue", "Proportion"};
        for (const auto& row : result.eigenvalues) {
            eigen.rows.push_back({
                std::to_string(row.index),
                format_number(row.eigenvalue),
                format_number(row.proportion)});
        }
        page.tables.push_back(std::move(eigen));
    }

    domain::ManovaOneWayFacts facts;
    facts.observation_count = result.observation_count;
    facts.response_count = result.response_count;
    facts.group_count = result.group_count;
    facts.wilks = cfg.wilks;
    facts.pillai = cfg.pillai;
    facts.lawley_hotelling = cfg.lawley_hotelling;
    facts.roy = cfg.roy;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.manova_one_way = facts;
    page.analysis_command_id = "manova_one_way";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::general_manova(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto& cfg = configuration.general_manova;
    if (cfg.response_columns.size() < 2 || !cfg.factor_a_column.has_value()) {
        return error_page("General MANOVA", "General MANOVA",
                          "请选择 2～4 个响应列与至少 1 个因子列。");
    }
    std::vector<std::vector<double>> responses;
    std::vector<std::string> factor_a;
    std::vector<std::string> factor_b;
    std::vector<double> covariate;
    std::vector<std::size_t> source_rows;
    const std::size_t col_a = *cfg.factor_a_column;
    const bool has_b = cfg.factor_b_column.has_value();
    const std::size_t col_b = cfg.factor_b_column.value_or(0);
    const bool has_cov = cfg.covariate_column.has_value();
    const std::size_t col_cov = cfg.covariate_column.value_or(0);
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (col_a >= table.rows[row].size() || table.rows[row][col_a].empty()) {
            continue;
        }
        if (has_b && (col_b >= table.rows[row].size() || table.rows[row][col_b].empty())) {
            continue;
        }
        std::vector<double> y_row;
        bool ok = true;
        for (std::size_t col : cfg.response_columns) {
            if (col >= table.rows[row].size()) {
                ok = false;
                break;
            }
            const auto value = parse_numeric_cell(table.rows[row][col]);
            if (!value.has_value()) {
                ok = false;
                break;
            }
            y_row.push_back(*value);
        }
        if (!ok) {
            continue;
        }
        if (has_cov) {
            const auto cov = parse_numeric_cell(table.rows[row][col_cov]);
            if (!cov.has_value()) {
                continue;
            }
            covariate.push_back(*cov);
        }
        responses.push_back(y_row);
        factor_a.push_back(table.rows[row][col_a]);
        if (has_b) {
            factor_b.push_back(table.rows[row][col_b]);
        }
        source_rows.push_back(row);
    }
    datalab::domain::statistics::GeneralManovaOptions options;
    options.include_interaction = cfg.include_interaction;
    options.wilks = cfg.wilks;
    options.pillai = cfg.pillai;
    options.lawley_hotelling = cfg.lawley_hotelling;
    options.roy = cfg.roy;
    const auto result = datalab::domain::statistics::general_manova_analyze(
        responses, factor_a, factor_b, covariate, source_rows, options);

    OutputPage page;
    page.id = new_id("general_manova");
    page.title = "General MANOVA";
    page.method_name = "General MANOVA";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    响应 = " + std::to_string(result.response_count)
        + "    效应 = " + std::to_string(result.effect_tests.size());
    page.diagnostics = result.diagnostics;

    for (const auto& effect : result.effect_tests) {
        StatisticTable tests;
        tests.title = "MANOVA: " + effect.effect_name;
        tests.headers = {"Test", "Value", "F", "Num DF", "Den DF", "P", "Approx"};
        for (const auto& row : effect.test_rows) {
            tests.rows.push_back({
                row.test_name,
                format_number(row.value),
                row.f_statistic.has_value() ? format_number(*row.f_statistic) : "*",
                row.numerator_df.has_value() ? format_number(*row.numerator_df) : "*",
                row.denominator_df.has_value() ? format_number(*row.denominator_df) : "*",
                row.p_value.has_value() ? format_number(*row.p_value) : "*",
                row.approximate ? "yes" : "no"});
        }
        page.tables.push_back(std::move(tests));
    }

    if (!result.cell_means.empty()) {
        StatisticTable means;
        means.title = "Cell Mean Vectors";
        means.headers = {"Cell", "N"};
        for (std::size_t j = 0; j < result.response_count; ++j) {
            means.headers.push_back("Y" + std::to_string(j + 1));
        }
        for (const auto& cell : result.cell_means) {
            std::vector<std::string> row = {cell.cell_label, std::to_string(cell.count)};
            for (double value : cell.means) {
                row.push_back(format_number(value));
            }
            means.rows.push_back(row);
        }
        page.tables.push_back(std::move(means));
    }

    domain::GeneralManovaFacts facts;
    facts.observation_count = result.observation_count;
    facts.response_count = result.response_count;
    facts.effect_count = result.effect_tests.size();
    facts.has_covariate = has_cov;
    facts.has_interaction = cfg.include_interaction && has_b;
    facts.wilks = cfg.wilks;
    facts.pillai = cfg.pillai;
    facts.lawley_hotelling = cfg.lawley_hotelling;
    facts.roy = cfg.roy;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.general_manova = facts;
    page.analysis_command_id = "general_manova";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::mixed_effects_reml(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto& cfg = configuration.mixed_effects_reml;
    if (!cfg.response_column.has_value() || !cfg.random_factor_column.has_value()) {
        return error_page("混合效应 REML", "Mixed Effects REML",
                          "请选择响应列与随机因子列。");
    }
    std::vector<double> response;
    std::vector<std::string> random_factor;
    std::vector<std::string> fixed_a;
    std::vector<std::string> fixed_b;
    std::vector<double> covariate;
    std::vector<std::size_t> source_rows;
    const std::size_t col_y = *cfg.response_column;
    const std::size_t col_r = *cfg.random_factor_column;
    const bool has_fa = cfg.fixed_factor_a_column.has_value();
    const std::size_t col_fa = cfg.fixed_factor_a_column.value_or(0);
    const bool has_fb = cfg.fixed_factor_b_column.has_value();
    const std::size_t col_fb = cfg.fixed_factor_b_column.value_or(0);
    const bool has_cov = cfg.covariate_column.has_value();
    const std::size_t col_cov = cfg.covariate_column.value_or(0);
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (col_y >= table.rows[row].size() || col_r >= table.rows[row].size()) {
            continue;
        }
        if (table.rows[row][col_r].empty()) {
            continue;
        }
        const auto y = parse_numeric_cell(table.rows[row][col_y]);
        if (!y.has_value()) {
            continue;
        }
        if (has_fa && (col_fa >= table.rows[row].size() || table.rows[row][col_fa].empty())) {
            continue;
        }
        if (has_fb && (col_fb >= table.rows[row].size() || table.rows[row][col_fb].empty())) {
            continue;
        }
        if (has_cov) {
            const auto cov = parse_numeric_cell(table.rows[row][col_cov]);
            if (!cov.has_value()) {
                continue;
            }
            covariate.push_back(*cov);
        }
        response.push_back(*y);
        random_factor.push_back(table.rows[row][col_r]);
        if (has_fa) {
            fixed_a.push_back(table.rows[row][col_fa]);
        }
        if (has_fb) {
            fixed_b.push_back(table.rows[row][col_fb]);
        }
        source_rows.push_back(row);
    }
    datalab::domain::statistics::MixedEffectsRemlOptions options;
    options.reml_method = cfg.reml_method;
    const auto result = datalab::domain::statistics::mixed_effects_reml_analyze(
        response, random_factor, fixed_a, fixed_b, covariate, source_rows, options);

    OutputPage page;
    page.id = new_id("mixed_effects_reml");
    page.title = "混合效应 REML";
    page.method_name = "Mixed Effects REML";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    随机水平 = " + std::to_string(result.random_level_count);
    page.diagnostics = result.diagnostics;

    StatisticTable varcomp;
    varcomp.title = "Variance Components";
    varcomp.headers = {"Source", "VarComp", "%Contribution"};
    for (const auto& row : result.variance_components) {
        varcomp.rows.push_back({
            row.source,
            format_number(row.variance_component),
            format_number(row.percent_contribution)});
    }
    page.tables.push_back(std::move(varcomp));

    StatisticTable fixed;
    fixed.title = "Fixed Effects (BLUE)";
    fixed.headers = {"Term", "Coef", "SE", "t", "P"};
    for (const auto& row : result.fixed_effects) {
        fixed.rows.push_back({
            row.term,
            format_number(row.coefficient),
            format_number(row.standard_error),
            format_number(row.t_statistic),
            format_number(row.p_value)});
    }
    page.tables.push_back(std::move(fixed));

    domain::MixedEffectsRemlFacts facts;
    facts.observation_count = result.observation_count;
    facts.random_level_count = result.random_level_count;
    facts.fixed_term_count = result.fixed_effects.size();
    facts.converged = result.converged;
    facts.residual_variance = result.residual_variance;
    facts.random_variance = result.random_variance;
    facts.reml_method = cfg.reml_method;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.mixed_effects_reml = facts;
    page.analysis_command_id = "mixed_effects_reml";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::binary_doe_probit(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto& cfg = configuration.binary_doe_probit;
    if (cfg.factor_columns.empty()) {
        return error_page("二值 DOE Probit", "Binary DOE Probit",
                          "请至少选择一个因子列。");
    }
    const bool use_events_trials = cfg.events_column.has_value() && cfg.trials_column.has_value();
    if (!use_events_trials && !cfg.binary_response_column.has_value()) {
        return error_page("二值 DOE Probit", "Binary DOE Probit",
                          "需要 Events/Trials 或 0/1 响应列。");
    }

    std::vector<std::vector<std::string>> factor_columns;
    std::vector<std::string> factor_labels;
    for (std::size_t col : cfg.factor_columns) {
        factor_labels.push_back(column_label(table, col));
        factor_columns.emplace_back();
    }
    std::vector<int> events;
    std::vector<int> trials;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        bool valid = true;
        for (std::size_t index = 0; index < cfg.factor_columns.size(); ++index) {
            const std::size_t col = cfg.factor_columns[index];
            if (col >= table.rows[row].size() || table.rows[row][col].empty()) {
                valid = false;
                break;
            }
            factor_columns[index].push_back(table.rows[row][col]);
        }
        if (!valid) {
            continue;
        }
        if (use_events_trials) {
            const auto event = parse_numeric_cell(table.rows[row][*cfg.events_column]);
            const auto trial = parse_numeric_cell(table.rows[row][*cfg.trials_column]);
            if (!event.has_value() || !trial.has_value()) {
                continue;
            }
            events.push_back(static_cast<int>(*event));
            trials.push_back(static_cast<int>(*trial));
        } else {
            const auto binary = parse_numeric_cell(
                table.rows[row][*cfg.binary_response_column]);
            if (!binary.has_value()) {
                continue;
            }
            const int value = static_cast<int>(*binary);
            events.push_back(value == 0 ? 0 : 1);
            trials.push_back(1);
        }
        source_rows.push_back(row);
    }

    datalab::domain::statistics::BinaryDoeProbitOptions options;
    options.link = cfg.link;
    options.include_ab_interaction = cfg.include_ab_interaction;
    const auto result = datalab::domain::statistics::analyze_binary_doe_probit(
        factor_columns, events, trials, factor_labels, source_rows, options);

    OutputPage page;
    page.id = new_id("binary_doe_probit");
    page.title = "二值 DOE Probit/Gompit";
    page.method_name = "Binary DOE Probit";
    page.configuration = configuration;
    page.parameter_summary = "设计行 = " + std::to_string(result.design_row_count)
        + "    N = " + std::to_string(result.expanded_observation_count)
        + "    Link = " + result.link;
    page.diagnostics = result.diagnostics;

    StatisticTable coef;
    coef.title = "Coefficients (" + result.link + " IRWLS)";
    coef.headers = {"Term", "Coef", "SE", "Z", "P"};
    for (const auto& row : result.coefficients) {
        coef.rows.push_back({
            row.term,
            format_number(row.coefficient),
            format_number(row.standard_error),
            format_number(row.z_statistic),
            format_number(row.p_value)});
    }
    page.tables.push_back(std::move(coef));

    StatisticTable fit;
    fit.title = "Goodness-of-Fit";
    fit.headers = {"Metric", "Value"};
    fit.rows.push_back({"Deviance", format_number(result.deviance)});
    fit.rows.push_back({"AIC", format_number(result.aic)});
    fit.rows.push_back({"Iterations", std::to_string(result.iteration_count)});
    fit.rows.push_back({"Converged", result.converged ? "Yes" : "No"});
    page.tables.push_back(std::move(fit));

    domain::BinaryDoeProbitFacts facts;
    facts.design_row_count = result.design_row_count;
    facts.expanded_observation_count = result.expanded_observation_count;
    facts.factor_count = result.factor_count;
    facts.link = result.link;
    facts.converged = result.converged;
    facts.iteration_count = result.iteration_count;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.binary_doe_probit = facts;
    page.analysis_command_id = "binary_doe_probit";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::life_data_lognormal(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto& cfg = configuration.life_data_lognormal;
    if (!cfg.time_column.has_value() || !cfg.event_column.has_value()) {
        return error_page("寿命 Lognormal", "Life Data Lognormal",
                          "请选择时间与删失列。");
    }
    std::vector<double> times;
    std::vector<bool> events;
    std::vector<std::vector<double>> covariates;
    std::vector<std::string> cov_labels;
    std::vector<std::size_t> source_rows;
    for (std::size_t col : cfg.covariate_columns) {
        cov_labels.push_back(column_label(table, col));
    }
    const std::size_t col_t = *cfg.time_column;
    const std::size_t col_e = *cfg.event_column;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (col_t >= table.rows[row].size() || col_e >= table.rows[row].size()) {
            continue;
        }
        const auto t = parse_numeric_cell(table.rows[row][col_t]);
        if (!t.has_value() || *t <= 0.0) {
            continue;
        }
        const auto event_cell = parse_numeric_cell(table.rows[row][col_e]);
        if (!event_cell.has_value()) {
            continue;
        }
        std::vector<double> cov_row;
        bool cov_ok = true;
        for (std::size_t col : cfg.covariate_columns) {
            if (col >= table.rows[row].size()) {
                cov_ok = false;
                break;
            }
            const auto v = parse_numeric_cell(table.rows[row][col]);
            if (!v.has_value()) {
                cov_ok = false;
                break;
            }
            cov_row.push_back(*v);
        }
        if (!cov_ok) {
            continue;
        }
        times.push_back(*t);
        events.push_back(*event_cell != 0.0);
        covariates.push_back(cov_row);
        source_rows.push_back(row);
    }

    datalab::domain::statistics::LifeDataLognormalOptions options;
    options.confidence_level = cfg.confidence_level;
    options.percentile_levels = cfg.percentile_levels;
    const auto result = datalab::domain::statistics::fit_life_data_lognormal(
        times, events, covariates, cov_labels, source_rows, options);

    OutputPage page;
    page.id = new_id("life_data_lognormal");
    page.title = "寿命数据 Lognormal";
    page.method_name = "Life Data Lognormal";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    失败 = " + std::to_string(result.failure_count)
        + "    删失 = " + std::to_string(result.censored_count);
    page.diagnostics = result.diagnostics;

    StatisticTable regression;
    regression.title = "Regression Table (log scale)";
    regression.headers = {"Term", "Coef", "SE", "Z", "P", "CI Lower", "CI Upper"};
    for (const auto& row : result.coefficients) {
        regression.rows.push_back({
            row.term,
            format_number(row.estimate),
            format_number(row.standard_error),
            format_number(row.z_statistic),
            format_number(row.p_value),
            format_number(row.confidence_lower),
            format_number(row.confidence_upper)});
    }
    page.tables.push_back(std::move(regression));

    if (!result.percentiles.empty()) {
        StatisticTable pct;
        pct.title = "Percentiles";
        pct.headers = {"Percentile", "Life", "Profile"};
        for (const auto& row : result.percentiles) {
            pct.rows.push_back({
                format_number(row.percentile),
                format_number(row.life),
                row.covariate_profile});
        }
        page.tables.push_back(std::move(pct));
    }

    StatisticTable dist;
    dist.title = "Distribution Summary";
    dist.headers = {"Parameter", "Estimate"};
    dist.rows.push_back({"log(σ)", format_number(result.log_sigma)});
    dist.rows.push_back({"Log-Likelihood", format_number(result.log_likelihood)});
    dist.rows.push_back({"Converged", result.converged ? "Yes" : "No"});
    page.tables.push_back(std::move(dist));

    domain::LifeDataLognormalFacts facts;
    facts.observation_count = result.observation_count;
    facts.failure_count = result.failure_count;
    facts.censored_count = result.censored_count;
    facts.covariate_count = result.covariate_count;
    facts.converged = result.converged;
    facts.log_sigma = result.log_sigma;
    facts.log_likelihood = result.log_likelihood;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.life_data_lognormal = facts;
    page.analysis_command_id = "life_data_lognormal";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::simple_correspondence(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto& cfg = configuration.simple_correspondence;
    if (!cfg.row_variable_column.has_value() || !cfg.column_variable_column.has_value()) {
        return error_page("简单对应分析", "Simple Correspondence",
                          "请选择行变量与列变量。");
    }
    std::vector<std::string> row_var;
    std::vector<std::string> col_var;
    std::vector<std::size_t> source_rows;
    const std::size_t col_r = *cfg.row_variable_column;
    const std::size_t col_c = *cfg.column_variable_column;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (col_r >= table.rows[row].size() || col_c >= table.rows[row].size()) {
            continue;
        }
        if (table.rows[row][col_r].empty() || table.rows[row][col_c].empty()) {
            continue;
        }
        row_var.push_back(table.rows[row][col_r]);
        col_var.push_back(table.rows[row][col_c]);
        source_rows.push_back(row);
    }
    datalab::domain::statistics::SimpleCorrespondenceOptions options;
    options.component_count = cfg.component_count;
    options.include_row_contributions = cfg.include_row_contributions;
    options.include_column_contributions = cfg.include_column_contributions;
    const auto result = datalab::domain::statistics::simple_correspondence_analyze(
        row_var, col_var, source_rows, options);

    OutputPage page;
    page.id = new_id("simple_correspondence");
    page.title = "简单对应分析";
    page.method_name = "Simple Correspondence Analysis";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    惯性 = " + format_number(result.total_inertia)
        + "    χ² = " + format_number(result.chi_square);
    page.diagnostics = result.diagnostics;

    StatisticTable summary;
    summary.title = "Summary of Analysis";
    summary.headers = {"Item", "Value"};
    summary.rows.push_back({"Observations", std::to_string(result.observation_count)});
    summary.rows.push_back({"Total Inertia", format_number(result.total_inertia)});
    summary.rows.push_back({"Chi-Square", format_number(result.chi_square)});
    summary.rows.push_back({"DF", std::to_string(result.chi_square_df)});
    if (result.chi_square_p_value.has_value()) {
        summary.rows.push_back({"P-Value", format_number(*result.chi_square_p_value)});
    }
    page.tables.push_back(std::move(summary));

    if (!result.row_contributions.empty()) {
        StatisticTable rows_table;
        rows_table.title = "Row Contributions";
        rows_table.headers = {"Row", "Quality", "Mass", "Inertia"};
        for (const auto& row : result.row_contributions) {
            rows_table.rows.push_back({
                row.label, format_number(row.quality),
                format_number(row.mass), format_number(row.inertia)});
        }
        page.tables.push_back(std::move(rows_table));
    }
    if (!result.column_contributions.empty()) {
        StatisticTable cols_table;
        cols_table.title = "Column Contributions";
        cols_table.headers = {"Column", "Quality", "Mass", "Inertia"};
        for (const auto& col : result.column_contributions) {
            cols_table.rows.push_back({
                col.label, format_number(col.quality),
                format_number(col.mass), format_number(col.inertia)});
        }
        page.tables.push_back(std::move(cols_table));
    }

    domain::SimpleCorrespondenceFacts facts;
    facts.observation_count = result.observation_count;
    facts.row_level_count = result.row_level_count;
    facts.column_level_count = result.column_level_count;
    facts.component_count = result.component_count;
    facts.total_inertia = result.total_inertia;
    facts.chi_square = result.chi_square;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.simple_correspondence = facts;
    page.analysis_command_id = "simple_correspondence";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::multiple_correspondence(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto& cfg = configuration.multiple_correspondence;
    if (cfg.categorical_columns.size() < 3 || cfg.categorical_columns.size() > 6) {
        return error_page("多重对应分析", "Multiple Correspondence",
                          "请选择 3～6 个分类变量列。");
    }
    std::vector<std::vector<std::string>> columns(cfg.categorical_columns.size());
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        bool valid = true;
        for (std::size_t v = 0; v < cfg.categorical_columns.size(); ++v) {
            const std::size_t col = cfg.categorical_columns[v];
            if (col >= table.rows[row].size() || table.rows[row][col].empty()) {
                valid = false;
                break;
            }
            columns[v].push_back(table.rows[row][col]);
        }
        if (!valid) {
            continue;
        }
        source_rows.push_back(row);
    }
    datalab::domain::statistics::MultipleCorrespondenceOptions options;
    options.component_count = cfg.component_count;
    options.include_column_contributions = cfg.include_column_contributions;
    const auto result = datalab::domain::statistics::multiple_correspondence_analyze(
        columns, source_rows, options);

    OutputPage page;
    page.id = new_id("multiple_correspondence");
    page.title = "多重对应分析";
    page.method_name = "Multiple Correspondence Analysis";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    变量 = " + std::to_string(result.variable_count)
        + "    惯性 = " + format_number(result.total_inertia);
    page.diagnostics = result.diagnostics;

    StatisticTable summary;
    summary.title = "Summary of Analysis";
    summary.headers = {"Item", "Value"};
    summary.rows.push_back({"Observations", std::to_string(result.observation_count)});
    summary.rows.push_back({"Variables", std::to_string(result.variable_count)});
    summary.rows.push_back({"Categories", std::to_string(result.category_count)});
    summary.rows.push_back({"Total Inertia", format_number(result.total_inertia)});
    page.tables.push_back(std::move(summary));

    if (!result.column_contributions.empty()) {
        StatisticTable cols_table;
        cols_table.title = "Column Contributions";
        cols_table.headers = {"Column", "Quality", "Mass", "Inertia"};
        for (const auto& col : result.column_contributions) {
            cols_table.rows.push_back({
                col.label, format_number(col.quality),
                format_number(col.mass), format_number(col.inertia)});
        }
        page.tables.push_back(std::move(cols_table));
    }

    domain::MultipleCorrespondenceFacts facts;
    facts.observation_count = result.observation_count;
    facts.variable_count = result.variable_count;
    facts.category_count = result.category_count;
    facts.column_count = result.category_count;
    facts.component_count = result.component_count;
    facts.total_inertia = result.total_inertia;
    facts.chi_square = result.chi_square;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.multiple_correspondence = facts;
    page.analysis_command_id = "multiple_correspondence";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::nonlinear_regression(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const auto& cfg = configuration.nonlinear_regression;
    if (!cfg.response_column.has_value() || !cfg.predictor_column.has_value()) {
        return error_page("非线性回归", "Nonlinear Regression",
                          "请选择响应列与预测列。");
    }
    std::vector<double> response;
    std::vector<double> predictor;
    std::vector<std::size_t> source_rows;
    const std::size_t col_y = *cfg.response_column;
    const std::size_t col_x = *cfg.predictor_column;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        const auto y = parse_numeric_cell(table.rows[row][col_y]);
        const auto x = parse_numeric_cell(table.rows[row][col_x]);
        if (!y.has_value() || !x.has_value()) {
            continue;
        }
        response.push_back(*y);
        predictor.push_back(*x);
        source_rows.push_back(row);
    }
    datalab::domain::statistics::NonlinearRegressionOptions options;
    options.model_id = cfg.model_id;
    options.algorithm = cfg.algorithm;
    options.starting_values = cfg.starting_values;
    options.max_iterations = cfg.max_iterations;
    options.tolerance = cfg.tolerance;
    options.lm_lambda = cfg.lm_lambda;
    const auto result = datalab::domain::statistics::fit_nonlinear_regression(
        response, predictor, source_rows, options);

    OutputPage page;
    page.id = new_id("nonlinear_regression");
    page.title = "非线性回归";
    page.method_name = "Nonlinear Regression";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    模型 = " + result.model_id
        + "    收敛 = " + (result.converged ? "是" : "否");
    page.diagnostics = result.diagnostics;

    StatisticTable method;
    method.title = "Method";
    method.headers = {"Item", "Value"};
    method.rows.push_back({"Algorithm", result.algorithm});
    method.rows.push_back({"Iterations", std::to_string(result.iteration_count)});
    method.rows.push_back({"Converged", result.converged ? "Yes" : "No"});
    page.tables.push_back(std::move(method));

    StatisticTable params;
    params.title = "Parameter Estimates";
    params.headers = {"Parameter", "Estimate", "SE", "Lower CI", "Upper CI"};
    for (const auto& param : result.parameters) {
        params.rows.push_back({
            param.name, format_number(param.estimate),
            format_number(param.standard_error),
            param.lower_ci.has_value() ? format_number(*param.lower_ci) : "*",
            param.upper_ci.has_value() ? format_number(*param.upper_ci) : "*"});
    }
    page.tables.push_back(std::move(params));

    StatisticTable fit;
    fit.title = "Summary of Fit";
    fit.headers = {"Item", "Value"};
    fit.rows.push_back({"SSE", format_number(result.sse)});
    fit.rows.push_back({"DF", std::to_string(result.error_df)});
    fit.rows.push_back({"MSE", format_number(result.mse)});
    fit.rows.push_back({"S", format_number(result.s)});
    fit.rows.push_back({"R-Sq", format_number(result.r_squared)});
    page.tables.push_back(std::move(fit));

    domain::NonlinearRegressionFacts facts;
    facts.observation_count = result.observation_count;
    facts.model_id = result.model_id;
    facts.algorithm = result.algorithm;
    facts.converged = result.converged;
    facts.iteration_count = result.iteration_count;
    facts.sse = result.sse;
    facts.mse = result.mse;
    facts.s = result.s;
    facts.r_squared = result.r_squared;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.nonlinear_regression = facts;
    page.analysis_command_id = "nonlinear_regression";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::split_plot_design(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    (void)table;
    const auto& cfg = configuration.split_plot_design;
    if (cfg.factor_names.size() < 2 || cfg.factor_names.size() > 4) {
        return error_page("裂区设计", "Split-Plot Design",
                          "需要 2～4 个因子。");
    }
    datalab::domain::statistics::SplitPlotDesignOptions options;
    options.htc_factor_index = cfg.htc_factor_index;
    options.whole_plot_replicates = cfg.whole_plot_replicates;
    options.randomize = cfg.randomize;
    options.random_seed = cfg.random_seed;
    options.etc_fraction_p = cfg.etc_fraction_p;
    for (std::size_t i = 0; i < cfg.factor_names.size(); ++i) {
        datalab::domain::statistics::DoeFactor factor;
        factor.name = cfg.factor_names[i];
        factor.low_level = i < cfg.low_levels.size() ? cfg.low_levels[i] : "-";
        factor.high_level = i < cfg.high_levels.size() ? cfg.high_levels[i] : "+";
        options.factors.push_back(factor);
    }
    const auto result = datalab::domain::statistics::generate_split_plot_design(options);

    OutputPage page;
    page.id = new_id("split_plot_design");
    page.title = "2 水平裂区设计";
    page.method_name = "Split-Plot Design";
    page.configuration = configuration;
    page.parameter_summary = "因子 = " + std::to_string(result.factor_count)
        + "    Whole plots = " + std::to_string(result.whole_plot_count)
        + "    Runs = " + std::to_string(result.run_count);
    page.diagnostics = result.diagnostics;

    StatisticTable summary;
    summary.title = "Design Summary";
    summary.headers = {"Item", "Value"};
    summary.rows.push_back({"Factors", std::to_string(result.factor_count)});
    summary.rows.push_back({"HTC Factor", result.htc_factor_name});
    summary.rows.push_back({"Whole Plots", std::to_string(result.whole_plot_count)});
    summary.rows.push_back({"Runs", std::to_string(result.run_count)});
    page.tables.push_back(std::move(summary));

    StatisticTable design;
    design.title = "Design Table";
    design.headers = {"StdOrder", "RunOrder", "WholePlot", "PointType"};
    for (const auto& name : result.factor_names) {
        design.headers.push_back(name);
    }
    for (const auto& run : result.runs) {
        std::vector<std::string> row = {
            std::to_string(run.standard_order),
            std::to_string(run.run_order),
            std::to_string(run.whole_plot),
            run.point_type};
        for (const auto& level : run.factor_levels) {
            row.push_back(level);
        }
        design.rows.push_back(row);
    }
    page.tables.push_back(std::move(design));

    domain::SplitPlotDesignFacts facts;
    facts.factor_count = result.factor_count;
    facts.whole_plot_count = result.whole_plot_count;
    facts.run_count = result.run_count;
    facts.htc_factor_index = result.htc_factor_index;
    facts.htc_factor_name = result.htc_factor_name;
    facts.randomized = result.randomized;
    facts.random_seed = result.random_seed;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.split_plot_design = facts;
    page.analysis_command_id = "split_plot_design";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::nhpp_repairable(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.nhpp_repairable.time_column.has_value()) {
        return error_page("可修复 NHPP", "NHPP Repairable System",
                          "请选择累积失效时间列。");
    }
    const auto times = extract_numeric_column(
        table, *configuration.nhpp_repairable.time_column, configuration.excluded_rows);
    datalab::domain::statistics::NhppRepairableOptions options;
    options.truncation_time = configuration.nhpp_repairable.truncation_time;
    const auto result = datalab::domain::statistics::fit_nhpp_crow_amsaa(
        times.values, times.source_rows, options);

    OutputPage page;
    page.id = new_id("nhpp_repairable");
    page.title = "可修复系统 NHPP";
    page.method_name = "NHPP Crow-AMSAA";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.failure_count)
        + "    T = " + format_number(result.truncation_time)
        + (result.beta.has_value() ? ("    β ≈ " + format_number(*result.beta)) : "")
        + (result.lambda.has_value() ? ("    λ ≈ " + format_number(*result.lambda)) : "");
    page.diagnostics = result.diagnostics;
    if (times.source_rows.size() < table.rows.size()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "NHPP 使用 complete-case；仅正有限时间行纳入。"});
    }

    StatisticTable summary;
    summary.title = "Observation Summary";
    summary.headers = {"Item", "Value"};
    summary.rows.push_back({"Failures (n)", std::to_string(result.failure_count)});
    summary.rows.push_back({"Truncation T", format_number(result.truncation_time)});
    page.tables.push_back(std::move(summary));

    StatisticTable params;
    params.title = "Parameter Estimates";
    params.headers = {"Parameter", "Estimate"};
    params.rows.push_back({
        "Beta β",
        result.beta.has_value() ? format_number(*result.beta) : "*"});
    params.rows.push_back({
        "Lambda λ",
        result.lambda.has_value() ? format_number(*result.lambda) : "*"});
    page.tables.push_back(std::move(params));

    if (!result.intensity_curve.empty()) {
        StatisticTable intensity;
        intensity.title = "Intensity / Mean function";
        intensity.headers = {"t", "lambda(t)", "M(t)"};
        for (const auto& point : result.intensity_curve) {
            intensity.rows.push_back({
                format_number(point.t),
                format_number(point.intensity),
                format_number(point.mean_function)});
        }
        page.tables.push_back(std::move(intensity));
    }

    if (configuration.nhpp_repairable.include_duane_plot
        && result.failure_times.size() >= 2) {
        PlotSpec duane;
        duane.kind = PlotKind::scatter;
        duane.title = "Duane 图（累积 MTBF 趋势参考）";
        duane.x_axis_title = "t (log)";
        duane.y_axis_title = "t/i (log)";
        PlotSeries series;
        series.label = "Duane";
        series.show_points = true;
        for (std::size_t i = 0; i < result.failure_times.size(); ++i) {
            const double t = result.failure_times[i];
            const double mtbf = t / static_cast<double>(i + 1);
            if (t > 0.0 && mtbf > 0.0) {
                const double x = std::log10(t);
                const double y = std::log10(mtbf);
                duane.x_values.push_back(x);
                duane.values.push_back(y);
                series.x_values.push_back(x);
                series.values.push_back(y);
            }
        }
        duane.series = {std::move(series)};
        if (!duane.x_values.empty()) {
            page.plots.push_back(std::move(duane));
        }
    }

    domain::NhppRepairableFacts facts;
    facts.failure_count = result.failure_count;
    facts.truncation_time = result.truncation_time;
    facts.beta = result.beta;
    facts.lambda = result.lambda;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.nhpp_repairable = facts;
    page.analysis_command_id = "nhpp_repairable";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::reliability_test_plan(
    const DataTable&,
    const AnalysisConfiguration& configuration)
{
    datalab::domain::statistics::ReliabilityTestPlanOptions options;
    options.shape_beta = configuration.reliability_test_plan.shape_beta;
    options.target_reliability = configuration.reliability_test_plan.target_reliability;
    options.confidence_level = configuration.reliability_test_plan.confidence_level;
    options.test_time = configuration.reliability_test_plan.test_time;
    options.mission_time = configuration.reliability_test_plan.mission_time;
    options.allowed_failures = configuration.reliability_test_plan.allowed_failures;
    const auto result =
        datalab::domain::statistics::plan_reliability_demonstration(options);

    OutputPage page;
    page.id = new_id("reliability_test_plan");
    page.title = "可靠性试验计划";
    page.method_name = "Reliability Demonstration Test Plan";
    page.configuration = configuration;
    page.parameter_summary = "β = " + format_number(result.shape_beta)
        + "    R = " + format_number(result.target_reliability)
        + "    CL = " + format_number(result.confidence_level)
        + (result.sample_size.has_value()
               ? ("    n = " + std::to_string(*result.sample_size))
               : "    n = *");
    page.diagnostics = result.diagnostics;

    StatisticTable plan;
    plan.title = "Test Plan";
    plan.headers = {"Item", "Value"};
    plan.rows.push_back({
        "Sample size n",
        result.sample_size.has_value() ? std::to_string(*result.sample_size) : "*"});
    plan.rows.push_back(
        {"Allowed failures r", std::to_string(result.allowed_failures)});
    plan.rows.push_back({"Test time T0", format_number(result.test_time)});
    plan.rows.push_back({"Mission time tm", format_number(result.mission_time)});
    plan.rows.push_back({"Shape β (assumed)", format_number(result.shape_beta)});
    plan.rows.push_back({"Target R", format_number(result.target_reliability)});
    plan.rows.push_back({"Confidence CL", format_number(result.confidence_level)});
    plan.rows.push_back({"Delta δ", format_number(result.time_ratio_delta)});
    plan.rows.push_back({"R_test = R^δ", format_number(result.test_reliability)});
    page.tables.push_back(std::move(plan));

    StatisticTable assumptions;
    assumptions.title = "Assumptions Summary";
    assumptions.headers = {"Assumption", "Note"};
    assumptions.rows.push_back(
        {"Weibull shape β", "工程假设，非本命令从数据估计"});
    assumptions.rows.push_back(
        {"Demonstration", "以 CL 演示任务时间处可靠度 ≥ R"});
    assumptions.rows.push_back(
        {"Not estimation", "不输出寿命点估计；禁止「寿命已达标」"});
    page.tables.push_back(std::move(assumptions));

    domain::ReliabilityTestPlanFacts facts;
    facts.shape_beta = result.shape_beta;
    facts.target_reliability = result.target_reliability;
    facts.confidence_level = result.confidence_level;
    facts.test_time = result.test_time;
    facts.mission_time = result.mission_time;
    facts.time_ratio_delta = result.time_ratio_delta;
    facts.allowed_failures = result.allowed_failures;
    facts.sample_size = result.sample_size;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    page.facts.reliability_test_plan = facts;
    page.analysis_command_id = "reliability_test_plan";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::doe_response_surface_design(
    const DataTable&,
    const AnalysisConfiguration& configuration)
{
    AnalysisConfiguration effective = configuration;
    auto& cfg = effective.response_surface_design;
    if (cfg.design_source_id.empty()) {
        cfg.design_source_id = cfg.design_kind + "-" + cfg.ccd_variant + "-seed-"
            + std::to_string(cfg.random_seed);
    }
    if (cfg.factor_ids.empty() && cfg.factor_names.empty()) {
        return error_page("响应曲面设计", "Response Surface Design",
                          "请提供至少一个连续因素。");
    }
    const std::size_t n = std::max(cfg.factor_ids.size(), cfg.factor_names.size());
    datalab::domain::statistics::ResponseSurfaceDesignOptions options;
    options.design_kind = cfg.design_kind == "bbd"
        ? datalab::domain::statistics::ResponseSurfaceDesignKind::bbd
        : datalab::domain::statistics::ResponseSurfaceDesignKind::ccd;
    if (cfg.ccd_variant == "ccc") {
        options.ccd_variant = datalab::domain::statistics::CcdVariant::ccc;
    } else if (cfg.ccd_variant == "cci") {
        options.ccd_variant = datalab::domain::statistics::CcdVariant::cci;
    } else {
        options.ccd_variant = datalab::domain::statistics::CcdVariant::ccf;
    }
    options.center_point_count = cfg.center_point_count;
    options.block_count = std::max<std::size_t>(1, cfg.block_count);
    options.randomize = cfg.randomize;
    options.random_seed = cfg.random_seed;
    options.allow_beyond_range = cfg.allow_beyond_range;
    options.alpha_override = cfg.alpha_override;

    for (std::size_t index = 0; index < n; ++index) {
        datalab::domain::statistics::ResponseSurfaceFactor factor;
        factor.id = index < cfg.factor_ids.size() && !cfg.factor_ids[index].empty()
            ? cfg.factor_ids[index]
            : (index < cfg.factor_names.size() ? cfg.factor_names[index]
                                              : ("F" + std::to_string(index + 1)));
        factor.name = index < cfg.factor_names.size() ? cfg.factor_names[index] : factor.id;
        factor.unit = index < cfg.factor_units.size() ? cfg.factor_units[index] : "";
        factor.type = "continuous";
        factor.low = index < cfg.low_levels.size() ? cfg.low_levels[index] : -1.0;
        factor.high = index < cfg.high_levels.size() ? cfg.high_levels[index] : 1.0;
        if (index < cfg.centers.size()) {
            factor.center = cfg.centers[index];
        }
        options.factors.push_back(std::move(factor));
    }

    const auto design =
        datalab::domain::statistics::generate_response_surface_design(options);
    if (!design.ok) {
        auto page = error_page(
            cfg.design_kind == "bbd" ? "Box–Behnken 设计" : "中心复合设计 (CCD)",
            cfg.design_kind == "bbd" ? "Box-Behnken Design" : "Central Composite Design",
            design.diagnostics.empty() ? "设计生成失败。"
                                       : design.diagnostics.front().message);
        page.diagnostics = design.diagnostics;
        return finalize_page(std::move(page));
    }
    return finalize_page(response_surface_design_page(effective, design));
}

OutputPage AnalysisService::individuals_moving_range(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (extracted.values.size() < 2) {
        return error_page("I-MR 控制图", "I-MR Chart", "I-MR 至少需要两个数值观测。");
    }

    datalab::domain::statistics::IndividualsMovingRangeOptions options;
    options.moving_range_length = std::max(2, configuration.control.moving_range_length);
    if (configuration.control.sigma_method == "median_moving_range") {
        options.method = datalab::domain::statistics::SigmaEstimateMethod::median_moving_range;
    } else if (configuration.control.sigma_method == "mssd") {
        options.method = datalab::domain::statistics::SigmaEstimateMethod::mssd;
    } else {
        options.method = datalab::domain::statistics::SigmaEstimateMethod::average_moving_range;
    }
    options.use_nelson_estimate = configuration.control.use_nelson_estimate;
    options.special_causes =
        datalab::domain::statistics::special_cause_selection_from_configuration(
            configuration.control.enabled_special_cause_tests,
            configuration.control.special_cause_rule_policy);
    options.historical_mean = configuration.control.historical_center;
    options.historical_sigma = configuration.control.historical_sigma;
    std::vector<std::string> stages;
    if (configuration.control.stage_column.has_value()) {
        const std::vector<std::string> stage_values = extract_text_column(
            table, *configuration.control.stage_column);
        for (const std::size_t row : extracted.source_rows) {
            const std::string stage = row < stage_values.size() ? stage_values[row] : "";
            if (is_missing_cell(stage)) {
                return error_page("I-MR 控制图", "I-MR Chart",
                                  "阶段列存在缺失标签，请补齐原始数据。");
            }
            stages.push_back(stage);
        }
        options.phase_labels = stages;
    }
    const auto dual = datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(
        extracted.values, options);

    OutputPage page;
    page.id = new_id("imr");
    page.title = "I-MR 控制图";
    page.method_name = "I-MR Chart";
    page.configuration = configuration;
    page.diagnostics = dual.diagnostics;
    page.diagnostics.insert(
        page.diagnostics.end(), dual.primary.diagnostics.begin(), dual.primary.diagnostics.end());
    page.diagnostics.insert(
        page.diagnostics.end(), dual.secondary.diagnostics.begin(), dual.secondary.diagnostics.end());
    const bool historical = configuration.control.historical_center.has_value()
        || configuration.control.historical_sigma.has_value();
    page.parameter_summary =
        "变量: " + extracted.name
        + "    移动极差长度 = " + std::to_string(options.moving_range_length)
        + "    σ 方法 = " + dual.sigma_method_label
        + "    σ = " + format_number(dual.sigma)
        + (historical ? "（历史参数）" : "（估计）");
    StatisticTable table_out;
    table_out.title = "I-MR 参数";
    table_out.headers = {"指标", "数值"};
    table_out.rows = {
        {"N", std::to_string(extracted.values.size())},
        {"N*", std::to_string(extracted.missing_count)},
        {"均值", format_number(dual.primary.center_line.empty() ? 0.0 : dual.primary.center_line.front())},
        {"MR̄", format_number(dual.average_moving_range)},
        {"σ 估计方法", dual.sigma_method_label},
        {"Nelson estimate", options.use_nelson_estimate ? "是" : "否"},
        {"Nelson 剔除 MR 数", std::to_string(dual.nelson_excluded_ranges)},
        {"σ (within)", format_number(dual.sigma)},
        {"参数来源", historical ? "历史参数" : "估计"},
        {"规则策略", configuration.control.special_cause_rule_policy == "explicit"
            ? "用户指定"
            : (configuration.control.special_cause_rule_policy == "minitab_like"
                   || configuration.control.special_cause_rule_policy == "default_minitab_like"
                   ? "minitab_like（仅「单点超出 3σ 控制限」）"
                   : "all_applicable（全部适用）")},
        {"I 图启用测试", datalab::domain::statistics::format_special_cause_tests(
            datalab::domain::statistics::resolve_special_cause_tests(
                options.special_causes,
                datalab::domain::statistics::ControlChartKind::individuals))},
        {"MR 图适用规则", "单点超出 3σ 控制限 / 连续 9 点同侧 / 连续 6 点趋势 / 连续 14 点交替（Minitab 不在 MR 图上启用后四条规则）"},
        {"「单点超出 3σ 控制限」触发点数", std::to_string(dual.primary.test1_points.size())}};
    page.tables.push_back(table_out);

    // B4: Historical / stage parameter transparency
    {
        StatisticTable hist;
        hist.title = "历史参数与分阶段估计";
        hist.headers = {"项", "值"};
        hist.rows.push_back({
            "历史 μ",
            configuration.control.historical_center.has_value()
                ? format_number(*configuration.control.historical_center) : "（未指定）"});
        hist.rows.push_back({
            "历史 σ",
            configuration.control.historical_sigma.has_value()
                ? format_number(*configuration.control.historical_sigma) : "（未指定）"});
        hist.rows.push_back({
            "控制限参数来源",
            historical ? "历史参数优先" : "当前数据估计"});
        if (!stages.empty()) {
            std::map<std::string, std::vector<double>> by_stage;
            for (std::size_t i = 0; i < stages.size() && i < extracted.values.size(); ++i) {
                by_stage[stages[i]].push_back(extracted.values[i]);
            }
            hist.rows.push_back({"阶段数", std::to_string(by_stage.size())});
            for (const auto& [label, values] : by_stage) {
                double sum = 0.0;
                for (const double value : values) {
                    sum += value;
                }
                const double mean = values.empty() ? 0.0 : sum / static_cast<double>(values.size());
                double mr_sum = 0.0;
                std::size_t mr_count = 0;
                for (std::size_t i = 1; i < values.size(); ++i) {
                    mr_sum += std::abs(values[i] - values[i - 1]);
                    ++mr_count;
                }
                const double mr_bar = mr_count > 0 ? mr_sum / static_cast<double>(mr_count) : 0.0;
                const double sigma_est = mr_bar / 1.128;  // d2 for MR=2
                hist.rows.push_back({
                    "阶段 " + label + "（N / 均值 / σ̂_MR）",
                    std::to_string(values.size()) + " / " + format_number(mean)
                        + " / " + format_number(sigma_est)});
            }
            hist.rows.push_back({
                "说明",
                "分阶段估计仅作对照；全局控制限仍由历史参数或全样本估计决定，不自动按阶段切换限。"});
        } else {
            hist.rows.push_back({"阶段列", "未指定"});
        }
        page.tables.push_back(std::move(hist));
    }

    page.tables.push_back(individuals_point_table(
        dual.primary, dual.secondary, extracted.source_rows, stages));
    page.plots.push_back(control_plot("单值图 (I)", "测量值", dual.primary, extracted.source_rows));
    page.plots.push_back(control_plot("移动极差图 (MR)", "移动极差", dual.secondary, extracted.source_rows));
    page.facts.spc = domain::SpcFacts{};
    page.facts.spc->sigma_within = dual.sigma;
    page.facts.spc->estimated_sigma = dual.sigma;
    page.facts.spc->out_of_control_count = dual.primary.test1_points.size();
    page.facts.spc->sigma_method = dual.sigma_method_label;
    page.facts.spc->use_nelson_estimate = options.use_nelson_estimate;
    page.facts.spc->nelson_excluded_ranges = dual.nelson_excluded_ranges;
    page.facts.spc->historical_parameters_used = historical;
    page.facts.spc->stage_count = 0;
    if (!stages.empty()) {
        std::set<std::string> unique_stages(stages.begin(), stages.end());
        page.facts.spc->stage_count = unique_stages.size();
    }
    attach_special_cause_rules(
        *page.facts.spc,
        dual.primary,
        datalab::domain::statistics::ControlChartKind::individuals,
        options.special_causes);
    append_special_cause_rule_table(page, *page.facts.spc);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::xbar_range(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    DualSubgroupChartSpec spec;
    spec.title = "Xbar-R 控制图";
    spec.method_name = "Xbar-R Chart";
    spec.id_prefix = "xbarr";
    spec.sigma_label = "R̄";
    spec.sigma_expression = "R̄ / d2";
    spec.secondary_short = "R";
    spec.secondary_plot_title = "R 图";
    spec.secondary_axis = "子组极差";
    spec.parameter_table_title = "Xbar-R 参数";
    spec.subgroup_table_title = "Xbar-R 逐子组统计";
    spec.compute = [](const std::vector<std::vector<double>>& subgroups,
                      const datalab::domain::statistics::SpecialCauseSelection& tests) {
        return datalab::domain::statistics::ControlCharts::xbar_range_dual(subgroups, tests);
    };
    spec.validate = [](const std::vector<std::vector<double>>& subgroups) -> std::string {
        return subgroups.front().size() > 8
            ? "Xbar-R 适用于子组大小不超过 8；较大子组请使用 Xbar-S。" : "";
    };
    return finalize_page(subgroup_dual_chart_page(table, configuration, spec));
}

OutputPage AnalysisService::xbar_s(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    DualSubgroupChartSpec spec;
    spec.title = "Xbar-S 控制图";
    spec.method_name = "Xbar-S Chart";
    spec.id_prefix = "xbars";
    spec.sigma_label = "S̄";
    spec.sigma_expression = "S̄ / c4";
    spec.secondary_short = "S";
    spec.secondary_plot_title = "S 图";
    spec.secondary_axis = "子组标准差";
    spec.parameter_table_title = "Xbar-S 参数";
    spec.subgroup_table_title = "Xbar-S 逐子组统计";
    spec.use_config_subgroup_size_in_summary = true;
    spec.compute = [](const std::vector<std::vector<double>>& subgroups,
                      const datalab::domain::statistics::SpecialCauseSelection& tests) {
        return datalab::domain::statistics::ControlCharts::xbar_s_dual(subgroups, tests);
    };
    return finalize_page(subgroup_dual_chart_page(table, configuration, spec));
}

OutputPage AnalysisService::imr_rs(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    std::string subgroup_error;
    const std::optional<SubgroupInput> input =
        build_strict_subgroups(table, extracted, configuration, subgroup_error);
    if (!input.has_value()) {
        return error_page("I-MR-R/S 控制图", "I-MR-R/S Chart", subgroup_error);
    }
    const datalab::domain::statistics::SpecialCauseSelection special_causes =
        datalab::domain::statistics::special_cause_selection_from_configuration(
            configuration.control.enabled_special_cause_tests,
            configuration.control.special_cause_rule_policy);
    datalab::domain::statistics::ImrRsChartResult chart =
        datalab::domain::statistics::ControlCharts::imr_rs_triple(
            input->values, special_causes);
    if (chart.individuals.plotted_values.empty()) {
        std::string message = "无法计算 I-MR-R/S 控制图。";
        for (const auto& diagnostic : chart.diagnostics) {
            if (diagnostic.severity == DiagnosticMessage::Severity::error) {
                message = diagnostic.message;
                break;
            }
        }
        OutputPage page = error_page("I-MR-R/S 控制图", "I-MR-R/S Chart", message);
        page.diagnostics.insert(
            page.diagnostics.end(), chart.diagnostics.cbegin(), chart.diagnostics.cend());
        return page;
    }

    std::vector<std::size_t> subgroup_rows;
    subgroup_rows.reserve(input->source_rows.size());
    for (const auto& rows : input->source_rows) {
        subgroup_rows.push_back(rows.front());
    }

    OutputPage page;
    page.id = new_id("imrrs");
    page.title = "I-MR-R/S 控制图";
    page.method_name = "I-MR-R/S Chart";
    page.configuration = configuration;
    page.diagnostics = chart.diagnostics;
    page.parameter_summary = "变量: " + extracted.name
        + "    子组大小 = " + std::to_string(input->values.front().size())
        + "    子组数 = " + std::to_string(input->values.size())
        + "    σ_within = " + format_number(chart.sigma_within)
        + "    σ_BW = " + format_number(chart.sigma_between_within);
    StatisticTable table_out;
    table_out.title = "I-MR-R/S 参数";
    table_out.headers = {"指标", "数值"};
    table_out.rows = {
        {"N", std::to_string(extracted.values.size())},
        {"N*", std::to_string(extracted.missing_count)},
        {"子组大小", std::to_string(input->values.front().size())},
        {"子组数", std::to_string(input->values.size())},
        {"σ_within", format_number(chart.sigma_within)},
        {"σ_between", format_number(chart.sigma_between)},
        {"σ_BW", format_number(chart.sigma_between_within)},
        {"方法", chart.method},
        {"组内图", chart.within_chart == "stdev" ? "S" : "R"},
        {"规则策略", configuration.control.special_cause_rule_policy == "explicit"
            ? "用户指定"
            : (configuration.control.special_cause_rule_policy == "minitab_like"
                   || configuration.control.special_cause_rule_policy == "default_minitab_like"
                   ? "minitab_like（仅「单点超出 3σ 控制限」）"
                   : "all_applicable（全部适用）")},
        {"I 图启用测试", datalab::domain::statistics::format_special_cause_tests(
            datalab::domain::statistics::resolve_special_cause_tests(
                special_causes,
                datalab::domain::statistics::ControlChartKind::individuals))},
        {"MR/R/S 适用规则", "单点超出 3σ 控制限、连续 9 点同侧、连续 6 点趋势、连续 14 点交替"}};
    page.tables.push_back(std::move(table_out));
    std::vector<std::string> stages;
    if (configuration.control.stage_column.has_value()) {
        const std::vector<std::string> stage_values = extract_text_column(
            table, *configuration.control.stage_column);
        for (const auto& rows : input->source_rows) {
            const std::size_t row = rows.front();
            const std::string stage = row < stage_values.size() ? stage_values[row] : "";
            if (is_missing_cell(stage)) {
                return error_page("I-MR-R/S 控制图", "I-MR-R/S Chart",
                                  "阶段列存在缺失标签，请补齐原始数据。");
            }
            stages.push_back(stage);
        }
        chart.individuals.phase_labels = stages;
        chart.moving_range.phase_labels = stages;
        chart.within.phase_labels = stages;
        datalab::domain::statistics::apply_special_cause_tests(
            chart.individuals,
            datalab::domain::statistics::ControlChartKind::individuals,
            special_causes);
        datalab::domain::statistics::apply_special_cause_tests(
            chart.moving_range,
            datalab::domain::statistics::ControlChartKind::moving_range,
            special_causes);
        datalab::domain::statistics::apply_special_cause_tests(
            chart.within,
            chart.within_chart == "stdev"
                ? datalab::domain::statistics::ControlChartKind::stdev
                : datalab::domain::statistics::ControlChartKind::range,
            special_causes);
    }
    page.tables.push_back(individuals_point_table(
        chart.individuals, chart.moving_range, subgroup_rows, stages));
    StatisticTable subgroup_table;
    subgroup_table.title = "I-MR-R/S 逐子组统计";
    subgroup_table.headers = {"原始行", "子组", "N", "Xbar",
                              chart.within_chart == "stdev" ? "S" : "R",
                              "I CL", "I LCL", "I UCL", "触发规则", "主要规则"};
    for (std::size_t index = 0; index < input->values.size(); ++index) {
        const std::string triggered = index < chart.individuals.triggered_tests.size()
            ? datalab::domain::statistics::format_triggered_special_cause_rules(
                  chart.individuals.triggered_tests[index])
            : datalab::domain::statistics::format_triggered_special_cause_rules({});
        subgroup_table.rows.push_back({
            std::to_string(input->source_rows[index].front() + 1),
            input->labels[index],
            std::to_string(input->values[index].size()),
            format_number(chart.individuals.plotted_values[index]),
            format_number(chart.within.plotted_values[index]),
            format_number(chart.individuals.center_line[index]),
            format_number(chart.individuals.lower_control_limit[index]),
            format_number(chart.individuals.upper_control_limit[index]),
            triggered,
            datalab::domain::statistics::format_primary_special_cause_rule(
                index < chart.individuals.primary_test_by_point.size()
                    ? chart.individuals.primary_test_by_point[index]
                    : 0)});
    }
    page.tables.push_back(std::move(subgroup_table));
    page.plots.push_back(control_plot(
        "I 图（子组均值）", "子组均值", chart.individuals, subgroup_rows));
    page.plots.push_back(control_plot(
        "MR 图", "移动极差", chart.moving_range, subgroup_rows));
    page.plots.push_back(control_plot(
        chart.within_chart == "stdev" ? "S 图" : "R 图",
        chart.within_chart == "stdev" ? "子组标准差" : "子组极差",
        chart.within, subgroup_rows));
    page.facts.spc = domain::SpcFacts{};
    page.facts.spc->sigma_within = chart.sigma_within;
    page.facts.spc->sigma_between = chart.sigma_between;
    page.facts.spc->sigma_between_within = chart.sigma_between_within;
    page.facts.spc->between_within_method = chart.method;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::p_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    AttributeChartSpec spec;
    spec.title = "P 图";
    spec.method_name = "P Chart";
    spec.id_prefix = "pchart";
    spec.count_header = "不合格品数";
    spec.denominator_header = "检验数";
    spec.rate_header = "不合格品率";
    spec.plot_title = "P 图";
    spec.y_axis = "不合格品率";
    spec.parameter_summary = "分布 = 二项分布    p̄ = Σ不合格品数 / Σ检验数    "
        "「单点超出 3σ 控制限」= 超出 3σ 控制限的点（rule_id=beyond_control_limit）";
    spec.assemble = [](const DataTable& table,
                       const AnalysisConfiguration& configuration,
                       std::string& error) -> std::optional<AttributeChartData> {
        const std::size_t defect_column = configuration.selection.defect_count_column.value_or(
            first_variable(configuration));
        const ExtractedNumericColumn defectives =
            extract_numeric_column(table, defect_column, configuration.excluded_rows);
        AttributeChartData data;
        data.source_rows = defectives.source_rows;
        if (configuration.inspected_constant.has_value()) {
            if (!append_nonnegative_counts(defectives.values, data.counts)) {
                error = "不合格品数必须是非负整数。";
                return std::nullopt;
            }
            for (std::size_t index = 0; index < data.counts.size(); ++index) {
                data.denominators.push_back(*configuration.inspected_constant);
            }
        } else if (configuration.selection.inspected_count_column.has_value()) {
            const ExtractedNumericColumn inspected = extract_numeric_column(
                table, *configuration.selection.inspected_count_column, configuration.excluded_rows);
            const std::size_t count = std::min(defectives.values.size(), inspected.values.size());
            if (!append_nonnegative_counts(
                    std::vector<double>(defectives.values.begin(), defectives.values.begin() + count),
                    data.counts)
                || !append_nonnegative_counts(
                    std::vector<double>(inspected.values.begin(), inspected.values.begin() + count),
                    data.denominators)) {
                error = "不合格品数和检验数必须是非负整数。";
                return std::nullopt;
            }
        }
        if (data.counts.empty()) {
            error = "请指定不合格品数列和检验数（常数或列）。";
            return std::nullopt;
        }
        return data;
    };
    spec.compute = [](const std::vector<std::size_t>& defectives,
                      const std::vector<std::size_t>& inspected,
                      const datalab::domain::statistics::SpecialCauseSelection& tests) {
        return datalab::domain::statistics::ControlCharts::p_chart(defectives, inspected, tests);
    };
    return attribute_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::np_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    AttributeChartSpec spec;
    spec.title = "NP 图";
    spec.method_name = "NP Chart";
    spec.id_prefix = "np";
    spec.count_header = "不合格品数";
    spec.denominator_header = "检验数";
    spec.rate_header = "不合格品数";
    spec.plot_title = "NP 图";
    spec.y_axis = "不合格品数";
    spec.parameter_summary = "分布 = 二项分布    np̄_i = n_i p̄    "
        "「单点超出 3σ 控制限」= 超出 3σ 控制限的点（rule_id=beyond_control_limit）";
    spec.assemble = [](const DataTable& table,
                       const AnalysisConfiguration& configuration,
                       std::string& error) -> std::optional<AttributeChartData> {
        const std::size_t defect_column = configuration.selection.defect_count_column.value_or(
            first_variable(configuration));
        const ExtractedNumericColumn defects =
            extract_numeric_column(table, defect_column, configuration.excluded_rows);
        if (!configuration.inspected_constant.has_value()
            && !configuration.selection.inspected_count_column.has_value()) {
            error = "NP 图需要固定检验数或检验数列。";
            return std::nullopt;
        }
        AttributeChartData data;
        data.source_rows = defects.source_rows;
        if (!append_nonnegative_counts(defects.values, data.counts)) {
            error = "不合格品数必须是非负整数。";
            return std::nullopt;
        }
        for (std::size_t index = 0; index < data.counts.size(); ++index) {
            data.denominators.push_back(configuration.inspected_constant.value_or(1));
        }
        if (!configuration.inspected_constant.has_value()
            && configuration.selection.inspected_count_column.has_value()) {
            const ExtractedNumericColumn inspected = extract_numeric_column(
                table, *configuration.selection.inspected_count_column, configuration.excluded_rows);
            data.denominators.clear();
            if (!append_nonnegative_counts(inspected.values, data.denominators)) {
                error = "检验数必须是非负整数。";
                return std::nullopt;
            }
        }
        return data;
    };
    spec.compute = [](const std::vector<std::size_t>& defectives,
                      const std::vector<std::size_t>& inspected,
                      const datalab::domain::statistics::SpecialCauseSelection& tests) {
        return datalab::domain::statistics::ControlCharts::np_chart(defectives, inspected, tests);
    };
    return attribute_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::c_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    AttributeChartSpec spec;
    spec.title = "C 图";
    spec.method_name = "C Chart";
    spec.id_prefix = "cchart";
    spec.count_header = "缺陷数";
    spec.denominator_header = "单位数";
    spec.rate_header = "缺陷数";
    spec.plot_title = "C 图";
    spec.y_axis = "缺陷数";
    spec.parameter_summary = "分布 = 泊松分布    c̄ = 缺陷数均值    "
        "C 图要求每个子组单位数相同    「单点超出 3σ 控制限」= 超出 3σ 控制限的点（rule_id=beyond_control_limit）";
    spec.assemble = [](const DataTable& table,
                       const AnalysisConfiguration& configuration,
                       std::string& error) -> std::optional<AttributeChartData> {
        const ExtractedNumericColumn defects = extract_numeric_column(
            table, configuration.selection.defect_count_column.value_or(first_variable(configuration)),
            configuration.excluded_rows);
        AttributeChartData data;
        data.source_rows = defects.source_rows;
        if (!append_nonnegative_counts(defects.values, data.counts)) {
            error = "缺陷数必须是非负整数。";
            return std::nullopt;
        }
        data.denominators.assign(
            data.counts.size(), configuration.inspected_constant.value_or(1));
        return data;
    };
    spec.compute = [](const std::vector<std::size_t>& counts,
                      const std::vector<std::size_t>& denominators,
                      const datalab::domain::statistics::SpecialCauseSelection& tests) {
        return datalab::domain::statistics::ControlCharts::c_chart(
            counts, denominators.empty() ? 1 : denominators.front(), tests);
    };
    return attribute_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::u_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    AttributeChartSpec spec;
    spec.title = "U 图";
    spec.method_name = "U Chart";
    spec.id_prefix = "uchart";
    spec.count_header = "缺陷数";
    spec.denominator_header = "单位数";
    spec.rate_header = "单位缺陷数";
    spec.plot_title = "U 图";
    spec.y_axis = "单位缺陷数";
    spec.parameter_summary = "分布 = 泊松分布    ū = Σ缺陷数 / Σ单位数    "
        "「单点超出 3σ 控制限」= 超出 3σ 控制限的点（rule_id=beyond_control_limit）";
    spec.assemble = [](const DataTable& table,
                       const AnalysisConfiguration& configuration,
                       std::string& error) -> std::optional<AttributeChartData> {
        const ExtractedNumericColumn defects = extract_numeric_column(
            table, configuration.selection.defect_count_column.value_or(first_variable(configuration)),
            configuration.excluded_rows);
        if (!configuration.selection.inspected_count_column.has_value()) {
            error = "U 图需要单位数列。";
            return std::nullopt;
        }
        const ExtractedNumericColumn units = extract_numeric_column(
            table, *configuration.selection.inspected_count_column, configuration.excluded_rows);
        AttributeChartData data;
        data.source_rows = defects.source_rows;
        const std::size_t count = std::min(defects.values.size(), units.values.size());
        if (!append_nonnegative_counts(
                std::vector<double>(defects.values.begin(), defects.values.begin() + count),
                data.counts)
            || !append_nonnegative_counts(
                std::vector<double>(units.values.begin(), units.values.begin() + count),
                data.denominators)) {
            error = "缺陷数和单位数必须是非负整数。";
            return std::nullopt;
        }
        return data;
    };
    spec.compute = [](const std::vector<std::size_t>& defects,
                      const std::vector<std::size_t>& units,
                      const datalab::domain::statistics::SpecialCauseSelection& tests) {
        return datalab::domain::statistics::ControlCharts::u_chart(defects, units, tests);
    };
    return attribute_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::laney_p_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    LaneyChartSpec spec;
    spec.title = "Laney P' 图";
    spec.method_name = "Laney P' Chart";
    spec.id_prefix = "laneyp";
    spec.distribution_text = "二项分布";
    spec.center_label = "p̄";
    spec.count_header = "不合格品数";
    spec.denominator_header = "检验数";
    spec.y_axis = "不合格品率";
    spec.include_enabled_tests_row = true;
    spec.assemble = [](const DataTable& table,
                       const AnalysisConfiguration& effective,
                       std::string& error) -> std::optional<AttributeChartData> {
        const std::size_t defect_column = effective.selection.defect_count_column.value_or(
            first_variable(effective));
        const ExtractedNumericColumn defectives =
            extract_numeric_column(table, defect_column, effective.excluded_rows);
        AttributeChartData data;
        data.source_rows = defectives.source_rows;
        if (!append_nonnegative_counts(defectives.values, data.counts)) {
            error = "不合格品数必须是非负整数。";
            return std::nullopt;
        }
        if (effective.inspected_constant.has_value()) {
            data.denominators.assign(data.counts.size(), *effective.inspected_constant);
        } else if (effective.selection.inspected_count_column.has_value()) {
            const ExtractedNumericColumn inspected = extract_numeric_column(
                table, *effective.selection.inspected_count_column, effective.excluded_rows);
            if (!append_nonnegative_counts(inspected.values, data.denominators)
                || data.denominators.size() != data.counts.size()) {
                error = "检验数必须是有效的非负整数列。";
                return std::nullopt;
            }
        } else {
            error = "请指定检验数列或检验数常数。";
            return std::nullopt;
        }
        return data;
    };
    spec.compute = [](const std::vector<std::size_t>& defectives,
                      const std::vector<std::size_t>& inspected,
                      const datalab::domain::statistics::LaneyChartOptions& options) {
        return datalab::domain::statistics::ControlCharts::laney_p_chart(
            defectives, inspected, options);
    };
    return laney_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::laney_u_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    LaneyChartSpec spec;
    spec.title = "Laney U' 图";
    spec.method_name = "Laney U' Chart";
    spec.id_prefix = "laneyu";
    spec.distribution_text = "泊松分布";
    spec.center_label = "ū";
    spec.count_header = "缺陷数";
    spec.denominator_header = "单位数";
    spec.y_axis = "单位缺陷数";
    spec.include_enabled_tests_row = true;
    spec.assemble = [spec](const DataTable& table,
                           const AnalysisConfiguration& effective,
                           std::string& error) -> std::optional<AttributeChartData> {
        const std::size_t defect_column = effective.selection.defect_count_column.value_or(
            first_variable(effective));
        const ExtractedNumericColumn defects =
            extract_numeric_column(table, defect_column, effective.excluded_rows);
        if (!effective.selection.inspected_count_column.has_value()) {
            error = spec.title + "需要单位数列。";
            return std::nullopt;
        }
        const ExtractedNumericColumn units = extract_numeric_column(
            table, *effective.selection.inspected_count_column, effective.excluded_rows);
        AttributeChartData data;
        data.source_rows = defects.source_rows;
        if (!append_nonnegative_counts(defects.values, data.counts)
            || !append_nonnegative_counts(units.values, data.denominators)
            || data.counts.size() != data.denominators.size()) {
            error = "缺陷数和单位数必须是有效的非负整数列。";
            return std::nullopt;
        }
        return data;
    };
    spec.compute = [](const std::vector<std::size_t>& defects,
                      const std::vector<std::size_t>& units,
                      const datalab::domain::statistics::LaneyChartOptions& options) {
        return datalab::domain::statistics::ControlCharts::laney_u_chart(
            defects, units, options);
    };
    return laney_chart_page(table, configuration, spec);
}

OutputPage AnalysisService::between_within_capability(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.selection.subgroup_column.has_value()) {
        return error_page("组间/组内过程能力", "Between/Within Capability Analysis",
                          "组间/组内能力需要子组标识列。");
    }
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (extracted.values.size() < 4) {
        return error_page("组间/组内过程能力", "Between/Within Capability Analysis",
                          "组间/组内能力至少需要四个数值观测。");
    }
    if (!configuration.specifications.lower.has_value()
        && !configuration.specifications.upper.has_value()) {
        return error_page("组间/组内过程能力", "Between/Within Capability Analysis",
                          "请输入 LSL 或 USL。");
    }
    std::string subgroup_error;
    const std::optional<SubgroupInput> subgroups = build_strict_subgroups(
        table, extracted, configuration, subgroup_error);
    if (!subgroups.has_value()) {
        return error_page("组间/组内过程能力", "Between/Within Capability Analysis",
                          subgroup_error);
    }
    auto capability_result =
        datalab::domain::statistics::ProcessCapability::calculate_between_within(
            extracted.values, subgroups->values, configuration.specifications);
    if (capability_result.evidence.not_computed_reason == "insufficient_subgroups"
        || capability_result.evidence.not_computed_reason == "unequal_subgroups"
        || capability_result.evidence.not_computed_reason == "empty_data") {
        OutputPage page = error_page("组间/组内过程能力", "Between/Within Capability Analysis",
                                     "无法计算组间/组内能力指标。");
        page.diagnostics = capability_result.diagnostics;
        page.configuration = configuration;
        return finalize_page(std::move(page));
    }
    datalab::domain::statistics::apply_capability_stability_screen(
        capability_result, extracted.values);
    datalab::domain::statistics::apply_capability_bimodality_screen(
        capability_result, extracted.values);
    datalab::domain::statistics::apply_capability_hartigan_dip_screen(
        capability_result, extracted.values);
    datalab::domain::statistics::apply_capability_mixture_screen(
        capability_result, extracted.values);
    AnalysisConfiguration page_configuration = configuration;
    page_configuration.capability_method = "between_within";
    const int subgroup_size = static_cast<int>(subgroups->values.front().size());
    OutputPage page = build_capability_content(
        page_configuration, extracted, capability_result, subgroup_size, "R̄ / d2(n)");
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::batch_capability(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.batch_capability.measurement_column.has_value()
        || !configuration.batch_capability.batch_column.has_value()) {
        return error_page("批次过程能力", "Batch Capability Analysis",
                          "请选择测量列与批次列。");
    }
    if (!configuration.specifications.lower.has_value()
        && !configuration.specifications.upper.has_value()) {
        return error_page("批次过程能力", "Batch Capability Analysis",
                          "请输入 LSL 或 USL。");
    }
    const std::size_t measurement_column =
        *configuration.batch_capability.measurement_column;
    const std::size_t batch_column = *configuration.batch_capability.batch_column;
    std::vector<double> values;
    std::vector<std::string> batch_labels;
    std::vector<std::size_t> source_rows;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (std::find(configuration.excluded_rows.cbegin(),
                      configuration.excluded_rows.cend(), row)
            != configuration.excluded_rows.cend()) {
            continue;
        }
        if (measurement_column >= table.rows[row].size()
            || batch_column >= table.rows[row].size()) {
            continue;
        }
        const auto parsed = parse_numeric_cell(table.rows[row][measurement_column]);
        const std::string& batch_cell = table.rows[row][batch_column];
        if (is_missing_cell(batch_cell)) {
            continue;
        }
        std::string batch = batch_cell;
        while (!batch.empty()
               && std::isspace(static_cast<unsigned char>(batch.front())) != 0) {
            batch.erase(batch.begin());
        }
        while (!batch.empty()
               && std::isspace(static_cast<unsigned char>(batch.back())) != 0) {
            batch.pop_back();
        }
        if (!parsed.has_value() || batch.empty()) {
            continue;
        }
        values.push_back(*parsed);
        batch_labels.push_back(batch);
        source_rows.push_back(row);
    }
    const auto result = datalab::domain::statistics::compute_batch_capability(
        values, batch_labels, source_rows, configuration.specifications,
        configuration.batch_capability.min_batch_size);
    OutputPage page;
    page.id = new_id("batch_capability");
    page.title = "批次过程能力";
    page.method_name = "Batch Capability Analysis";
    page.configuration = configuration;
    page.parameter_summary = "批次列 = " + column_label(table, batch_column)
        + "    测量 = " + column_label(table, measurement_column)
        + "    批次数 = " + std::to_string(result.batch_count);
    page.diagnostics = result.diagnostics;
    if (source_rows.size() < table.rows.size()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "批次能力使用 complete-case，缺失行已排除。"});
    }
    StatisticTable summary;
    summary.title = "批次能力摘要";
    summary.headers = {"批次", "N", "均值", "σ", "Cp", "Cpk", "Pp", "Ppk"};
    for (const auto& batch : result.batches) {
        summary.rows.push_back({
            batch.batch_id,
            std::to_string(batch.sample_size),
            format_number(batch.mean),
            format_number(batch.within_standard_deviation),
            format_optional(batch.cp),
            format_optional(batch.cpk),
            format_optional(batch.pp),
            format_optional(batch.ppk)});
    }
    page.tables.push_back(std::move(summary));
    domain::BatchCapabilityFacts facts;
    facts.batch_count = result.batch_count;
    facts.skipped_batch_count = result.skipped_batch_count;
    facts.total_observations = result.total_observations;
    page.facts.batch_capability = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::nonparametric_capability(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.nonparametric_capability.measurement_column.has_value()) {
        return error_page("非参数过程能力", "Nonparametric Capability Analysis",
                          "请选择测量列。");
    }
    const ExtractedNumericColumn extracted = extract_numeric_column(
        table, *configuration.nonparametric_capability.measurement_column,
        configuration.excluded_rows);
    if (extracted.values.size() < 10) {
        return error_page("非参数过程能力", "Nonparametric Capability Analysis",
                          "非参数能力至少需要 10 个有效观测。");
    }
    const auto result = datalab::domain::statistics::compute_nonparametric_capability(
        extracted.values, extracted.source_rows, configuration.specifications,
        configuration.nonparametric_capability.tolerance_k);
    OutputPage page;
    page.id = new_id("nonparametric_capability");
    page.title = "非参数过程能力";
    page.method_name = "Nonparametric Capability Analysis";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.sample_size)
        + "    K = " + format_number(result.tolerance_k);
    page.diagnostics = result.diagnostics;
    StatisticTable capability;
    capability.title = "Overall Capability";
    capability.headers = {"N", "Median", "Xpl", "Xpu", "Cnp", "Cnpl", "Cnpu", "Cnpk"};
    capability.rows.push_back({
        std::to_string(result.sample_size),
        format_number(result.median),
        format_number(result.lower_percentile),
        format_number(result.upper_percentile),
        result.cnp.has_value() ? format_number(*result.cnp) : "*",
        result.cnpl.has_value() ? format_number(*result.cnpl) : "*",
        result.cnpu.has_value() ? format_number(*result.cnpu) : "*",
        result.cnpk.has_value() ? format_number(*result.cnpk) : "*"});
    page.tables.push_back(std::move(capability));
    if (result.observed_ppm_total.has_value()) {
        StatisticTable ppm;
        ppm.title = "Observed Performance";
        ppm.headers = {"PPM < LSL", "PPM > USL", "PPM Total"};
        ppm.rows.push_back({
            result.observed_ppm_below.has_value()
                ? format_number(*result.observed_ppm_below) : "*",
            result.observed_ppm_above.has_value()
                ? format_number(*result.observed_ppm_above) : "*",
            format_number(*result.observed_ppm_total)});
        page.tables.push_back(std::move(ppm));
    }
    const auto bins = datalab::domain::statistics::histogram(extracted.values, 0);
    PlotSpec hist;
    hist.kind = PlotKind::histogram;
    hist.title = "Capability Histogram";
    hist.x_axis_title = extracted.name;
    hist.y_axis_title = "频数";
    hist.histogram_edges = bins.edges;
    hist.histogram_counts = bins.counts;
    hist.values = extracted.values;
    hist.source_rows = extracted.source_rows;
    hist.lsl = configuration.specifications.lower;
    hist.usl = configuration.specifications.upper;
    page.plots.push_back(std::move(hist));
    domain::NonparametricCapabilityFacts facts;
    facts.n = result.sample_size;
    facts.tolerance_k = result.tolerance_k;
    facts.cnp = result.cnp;
    facts.cnpl = result.cnpl;
    facts.cnpu = result.cnpu;
    facts.cnpk = result.cnpk;
    facts.median = result.median;
    facts.lower_percentile = result.lower_percentile;
    facts.upper_percentile = result.upper_percentile;
    facts.observed_ppm_below = result.observed_ppm_below;
    facts.observed_ppm_above = result.observed_ppm_above;
    facts.observed_ppm_total = result.observed_ppm_total;
    page.facts.nonparametric_capability = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::cox_regression(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.cox_regression.time_column.has_value()
        || !configuration.cox_regression.event_column.has_value()
        || configuration.cox_regression.covariate_columns.empty()) {
        return error_page("Cox 回归", "Cox Regression",
                          "请选择时间列、事件列与至少一个协变量。");
    }
    const auto times = extract_numeric_column(
        table, *configuration.cox_regression.time_column, configuration.excluded_rows);
    const auto event_text = extract_text_column(
        table, *configuration.cox_regression.event_column);
    const std::size_t p = configuration.cox_regression.covariate_columns.size();
    std::vector<ExtractedNumericColumn> covariate_columns;
    covariate_columns.reserve(p);
    for (std::size_t column : configuration.cox_regression.covariate_columns) {
        covariate_columns.push_back(
            extract_numeric_column(table, column, configuration.excluded_rows));
    }
    std::vector<double> aligned_times;
    std::vector<bool> events;
    std::vector<std::vector<double>> covariates;
    std::vector<std::size_t> source_rows;
    std::vector<std::string> labels;
    for (std::size_t column : configuration.cox_regression.covariate_columns) {
        labels.push_back(column_label(table, column));
    }
    for (std::size_t i = 0; i < times.source_rows.size(); ++i) {
        const std::size_t row = times.source_rows[i];
        if (row >= event_text.size()) {
            continue;
        }
        const auto parsed_event =
            datalab::domain::statistics::parse_reliability_event(event_text[row]);
        if (!parsed_event.has_value()) {
            continue;
        }
        std::vector<double> row_covariates;
        row_covariates.reserve(p);
        bool missing_covariate = false;
        for (const auto& cov : covariate_columns) {
            const auto it = std::find(cov.source_rows.begin(), cov.source_rows.end(), row);
            if (it == cov.source_rows.end()) {
                missing_covariate = true;
                break;
            }
            row_covariates.push_back(cov.values[static_cast<std::size_t>(
                std::distance(cov.source_rows.begin(), it))]);
        }
        if (missing_covariate) {
            continue;
        }
        aligned_times.push_back(times.values[i]);
        events.push_back(*parsed_event);
        covariates.push_back(std::move(row_covariates));
        source_rows.push_back(row);
    }
    const double confidence = configuration.cox_regression.confidence_level > 0.0
        ? configuration.cox_regression.confidence_level
        : configuration.inference.confidence_level;
    const auto result = datalab::domain::statistics::fit_cox_regression(
        aligned_times, events, covariates, labels, source_rows, confidence,
        configuration.cox_regression.ties_method,
        static_cast<std::size_t>(std::max(1, configuration.cox_regression.max_iterations)),
        configuration.cox_regression.tolerance);
    OutputPage page;
    page.id = new_id("cox_regression");
    page.title = "Cox 回归";
    page.method_name = "Cox Regression";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.n)
        + "    Events = " + std::to_string(result.events)
        + "    Censored = " + std::to_string(result.censored)
        + "    Ties = " + result.ties_method;
    page.diagnostics = result.diagnostics;
    if (source_rows.size() < table.rows.size()) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "missing_values",
            "Cox 回归使用 complete-case，缺失或非法行已排除。"});
    }
    StatisticTable summary;
    summary.title = "模型摘要";
    summary.headers = {"N", "Events", "Censored", "Converged", "Log-Likelihood", "Algorithm"};
    summary.rows.push_back({
        std::to_string(result.n),
        std::to_string(result.events),
        std::to_string(result.censored),
        result.converged ? "是" : "否",
        format_number(result.log_likelihood),
        result.algorithm_id});
    page.tables.push_back(std::move(summary));
    StatisticTable coefficients;
    coefficients.title = "系数与相对风险";
    coefficients.headers = {
        "Term", "Coef", "SE Coef", "Z", "P-Value", "HR", "CI Lower", "CI Upper"};
    for (const auto& coefficient : result.coefficients) {
        coefficients.rows.push_back({
            coefficient.term,
            format_number(coefficient.beta),
            format_number(coefficient.standard_error),
            format_number(coefficient.z_statistic),
            format_number(coefficient.p_value),
            format_number(coefficient.hazard_ratio),
            format_number(coefficient.confidence_lower),
            format_number(coefficient.confidence_upper)});
    }
    page.tables.push_back(std::move(coefficients));
    domain::CoxRegressionFacts facts;
    facts.n = result.n;
    facts.events = result.events;
    facts.censored = result.censored;
    facts.converged = result.converged;
    facts.log_likelihood = result.log_likelihood;
    facts.evidence_type = result.evidence_type;
    facts.algorithm_id = result.algorithm_id;
    facts.ties_method = result.ties_method;
    for (const auto& coefficient : result.coefficients) {
        domain::CoxRegressionCoefficientFacts term;
        term.term = coefficient.term;
        term.beta = coefficient.beta;
        term.se = coefficient.standard_error;
        term.z = coefficient.z_statistic;
        term.p_value = coefficient.p_value;
        term.hazard_ratio = coefficient.hazard_ratio;
        term.ci_lower = coefficient.confidence_lower;
        term.ci_upper = coefficient.confidence_upper;
        facts.coefficients.push_back(std::move(term));
    }
    page.facts.cox_regression = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::accelerated_life(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.accelerated_life.time_column.has_value()
        || !configuration.accelerated_life.event_column.has_value()
        || !configuration.accelerated_life.stress_column.has_value()) {
        return error_page("加速寿命", "Accelerated Life Testing",
                          "请选择寿命列、失效指示列与应力列。");
    }
    const auto times = extract_numeric_column(
        table, *configuration.accelerated_life.time_column, configuration.excluded_rows);
    const auto event_text = extract_text_column(
        table, *configuration.accelerated_life.event_column);
    const auto stress = extract_numeric_column(
        table, *configuration.accelerated_life.stress_column, configuration.excluded_rows);
    std::vector<double> aligned_times;
    std::vector<bool> events;
    std::vector<double> aligned_stress;
    std::vector<std::size_t> source_rows;
    for (std::size_t i = 0; i < times.source_rows.size(); ++i) {
        const std::size_t row = times.source_rows[i];
        if (row >= event_text.size()) {
            continue;
        }
        const std::size_t stress_index = std::find(stress.source_rows.begin(),
            stress.source_rows.end(), row) - stress.source_rows.begin();
        if (stress_index >= stress.source_rows.size()) {
            continue;
        }
        const auto parsed_event =
            datalab::domain::statistics::parse_reliability_event(event_text[row]);
        if (!parsed_event.has_value()) {
            continue;
        }
        aligned_times.push_back(times.values[i]);
        events.push_back(*parsed_event);
        aligned_stress.push_back(stress.values[stress_index]);
        source_rows.push_back(row);
    }
    const auto result = datalab::domain::statistics::fit_accelerated_life_weibull_arrhenius(
        aligned_times, events, aligned_stress, source_rows,
        configuration.inference.confidence_level > 0.0
            ? configuration.inference.confidence_level : 0.95,
        configuration.accelerated_life.use_stress_celsius);
    OutputPage page;
    page.id = new_id("accelerated_life");
    page.title = "加速寿命分析";
    page.method_name = "Accelerated Life Testing";
    page.configuration = configuration;
    page.parameter_summary = "N = " + std::to_string(result.observation_count)
        + "    失效 = " + std::to_string(result.failure_count)
        + "    应力水平 = " + std::to_string(result.stress_level_count)
        + "    使用应力 = " + format_number(result.use_stress_celsius) + " °C"
        + "    变换 = Arrhenius";
    page.diagnostics = result.diagnostics;
    StatisticTable regression;
    regression.title = "Regression Table";
    regression.headers = {"Predictor", "Coef", "SE Coef", "Z", "P", "Lower", "Upper"};
    for (const auto& coefficient : result.coefficients) {
        regression.rows.push_back({
            coefficient.term,
            format_number(coefficient.estimate),
            format_number(coefficient.standard_error),
            format_number(coefficient.z_statistic),
            format_number(coefficient.p_value),
            format_number(coefficient.confidence_lower),
            format_number(coefficient.confidence_upper)});
    }
    page.tables.push_back(std::move(regression));
    StatisticTable shape;
    shape.title = "Weibull Shape";
    shape.headers = {"Shape", "SE Shape"};
    shape.rows.push_back({format_number(result.shape), format_number(result.shape_se)});
    page.tables.push_back(std::move(shape));
    StatisticTable accelerated_percentiles;
    accelerated_percentiles.title = "Percentiles for Accelerated Levels";
    accelerated_percentiles.headers = {"Stress (°C)", "Percentile", "Life"};
    for (const auto& row : result.percentiles_at_stress_levels) {
        accelerated_percentiles.rows.push_back({
            format_number(row.stress_celsius),
            format_number(row.percentile) + "%",
            format_number(row.life)});
    }
    page.tables.push_back(std::move(accelerated_percentiles));
    StatisticTable design_percentiles;
    design_percentiles.title = "Percentiles at Design Level";
    design_percentiles.headers = {"Use Stress (°C)", "Percentile", "Life"};
    for (const auto& row : result.percentiles_at_use_stress) {
        design_percentiles.rows.push_back({
            format_number(row.stress_celsius),
            format_number(row.percentile) + "%",
            format_number(row.life)});
    }
    page.tables.push_back(std::move(design_percentiles));
    if (!result.life_stress_curve.empty()) {
        PlotSpec life_stress_plot;
        life_stress_plot.kind = PlotKind::scatter;
        life_stress_plot.title = "Life-Stress Plot";
        life_stress_plot.x_axis_title = "Stress (°C)";
        life_stress_plot.y_axis_title = "Percentile Life";
        life_stress_plot.show_legend = true;
        for (const double percentile : {10.0, 50.0, 90.0}) {
            PlotSeries series;
            series.role = PlotSeriesRole::fitted;
            series.show_points = false;
            series.label = "B" + std::to_string(static_cast<int>(percentile));
            for (const auto& point : result.life_stress_curve) {
                if (point.percentile != percentile) {
                    continue;
                }
                series.x_values.push_back(point.stress_celsius);
                series.values.push_back(point.life);
            }
            if (!series.x_values.empty()) {
                life_stress_plot.series.push_back(std::move(series));
            }
        }
        page.plots.push_back(std::move(life_stress_plot));
    }
    domain::AcceleratedLifeFacts facts;
    facts.n = result.observation_count;
    facts.failure_count = result.failure_count;
    facts.censored_count = result.censored_count;
    facts.stress_level_count = result.stress_level_count;
    facts.converged = result.converged;
    facts.transform = result.transform;
    facts.shape = result.shape;
    facts.log_likelihood = result.log_likelihood;
    facts.use_stress_celsius = result.use_stress_celsius;
    facts.b10_at_use_stress = result.b10_at_use_stress;
    facts.b50_at_use_stress = result.b50_at_use_stress;
    facts.b90_at_use_stress = result.b90_at_use_stress;
    page.facts.accelerated_life = facts;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::binomial_capability(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const AttributeImport imported = import_attribute_samples(table, configuration);
    if (!imported.error.empty()) {
        return error_page("二项过程能力", "Binomial Capability", imported.error);
    }
    auto result = datalab::domain::statistics::binomial_capability(
        imported.samples, imported.missing_count, configuration.specifications.target,
        configuration.inference.confidence_level > 0.0
            ? configuration.inference.confidence_level : 0.95);
    if (result.sample_count == 0) {
        OutputPage page = error_page("二项过程能力", "Binomial Capability",
                                     "没有可用于二项过程能力的有效子组。");
        page.diagnostics = result.diagnostics;
        page.configuration = configuration;
        return finalize_page(std::move(page));
    }
    return build_attribute_capability_page(configuration, imported, std::move(result), true);
}

OutputPage AnalysisService::poisson_capability(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const AttributeImport imported = import_attribute_samples(table, configuration);
    if (!imported.error.empty()) {
        return error_page("泊松过程能力", "Poisson Capability", imported.error);
    }
    auto result = datalab::domain::statistics::poisson_capability(
        imported.samples, imported.missing_count, configuration.specifications.target,
        configuration.inference.confidence_level > 0.0
            ? configuration.inference.confidence_level : 0.95);
    if (result.sample_count == 0) {
        OutputPage page = error_page("泊松过程能力", "Poisson Capability",
                                     "没有可用于泊松过程能力的有效子组。");
        page.diagnostics = result.diagnostics;
        page.configuration = configuration;
        return finalize_page(std::move(page));
    }
    return build_attribute_capability_page(configuration, imported, std::move(result), false);
}

OutputPage AnalysisService::capability(
    const DataTable& table,
    const AnalysisConfiguration& configuration,
    std::vector<double>* capability_indices)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (extracted.values.size() < 2) {
        return error_page("正态过程能力", "Normal Capability Analysis", "过程能力至少需要两个数值观测。");
    }
    if (!configuration.specifications.lower.has_value()
        && !configuration.specifications.upper.has_value()) {
        return error_page("正态过程能力", "Normal Capability Analysis", "请输入 LSL 或 USL。");
    }

    const int subgroup_size = configuration.control.subgroup_size.value_or(1);
    datalab::domain::statistics::ProcessCapabilityResult capability_result;
    std::string within_method = "样本标准差";
    if (configuration.capability_method == "johnson") {
        capability_result = datalab::domain::statistics::ProcessCapability::calculate_johnson(
            extracted.values, configuration.specifications);
        within_method = "not_applicable";
    } else if (configuration.capability_method == "non_normal") {
        const std::string distribution =
            configuration.nonnormal_distribution.empty()
                ? "weibull" : configuration.nonnormal_distribution;
        capability_result = datalab::domain::statistics::ProcessCapability::calculate_nonnormal(
            extracted.values, configuration.specifications, distribution);
        within_method = "not_applicable";
    } else {
        double within_sigma = 0.0;
        if (subgroup_size <= 1) {
            datalab::domain::statistics::IndividualsMovingRangeOptions options;
            options.moving_range_length = std::max(2, configuration.control.moving_range_length);
        options.method = configuration.control.sigma_method == "median_moving_range"
                ? datalab::domain::statistics::SigmaEstimateMethod::median_moving_range
                : (configuration.control.sigma_method == "mssd"
                       ? datalab::domain::statistics::SigmaEstimateMethod::mssd
                       : datalab::domain::statistics::SigmaEstimateMethod::average_moving_range);
            options.use_nelson_estimate = configuration.control.use_nelson_estimate;
            const auto dual =
                datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(
                    extracted.values, options);
            within_sigma = dual.sigma;
            within_method = "平均移动极差 / d2";
        } else {
            const auto subgroups = datalab::domain::statistics::build_subgroups(
                extracted.values, subgroup_size);
            const auto dual = datalab::domain::statistics::ControlCharts::xbar_range_dual(subgroups);
            within_sigma = dual.sigma;
            within_method = "R̄ / d2";
        }

        capability_result = datalab::domain::statistics::ProcessCapability::calculate(
            extracted.values, within_sigma, configuration.specifications);
    }
    datalab::domain::statistics::apply_capability_stability_screen(
        capability_result, extracted.values);
    datalab::domain::statistics::apply_capability_bimodality_screen(
        capability_result, extracted.values);
    datalab::domain::statistics::apply_capability_hartigan_dip_screen(
        capability_result, extracted.values);
    datalab::domain::statistics::apply_capability_mixture_screen(
        capability_result, extracted.values);
    const double confidence = configuration.inference.confidence_level > 0.0
            && configuration.inference.confidence_level < 1.0
        ? configuration.inference.confidence_level : 0.95;
    datalab::domain::statistics::fill_capability_index_intervals(
        capability_result, confidence);
    OutputPage page = build_capability_content(
        configuration, extracted, capability_result, subgroup_size, within_method);
    if (capability_indices != nullptr) {
        if (capability_result.cp.has_value()) {
            capability_indices->push_back(*capability_result.cp);
        }
        if (capability_result.cpk.has_value()) {
            capability_indices->push_back(*capability_result.cpk);
        }
        if (capability_result.pp.has_value()) {
            capability_indices->push_back(*capability_result.pp);
        }
        if (capability_result.ppk.has_value()) {
            capability_indices->push_back(*capability_result.ppk);
        }
    }

    if (subgroup_size <= 1) {
        datalab::domain::statistics::IndividualsMovingRangeOptions options;
        options.moving_range_length = std::max(2, configuration.control.moving_range_length);
        const auto dual = datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(
            extracted.values, options);
        page.plots.push_back(control_plot("I 图", "测量值", dual.primary, extracted.source_rows));
    }
    page.analysis_command_id = "capability";
    attach_computation_traces(page, "capability");
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::capability_sixpack(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    AnalysisConfiguration normal_configuration = configuration;
    normal_configuration.capability_method = "normal";
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(normal_configuration),
                               normal_configuration.excluded_rows);
    if (extracted.values.size() < 2) {
        return error_page("过程能力 Sixpack", "Capability Sixpack", "Sixpack 至少需要两个数值观测。");
    }
    std::vector<double> capability_indices;
    OutputPage capability_page = capability(table, normal_configuration, &capability_indices);
    if (capability_page.tables.empty() && capability_page.plots.empty()) {
        capability_page.title = "过程能力 Sixpack";
        capability_page.method_name = "Capability Sixpack";
        return capability_page;
    }

    const int subgroup_size = configuration.control.subgroup_size.value_or(1);
    std::vector<PlotSpec> primary_plots;
    datalab::domain::statistics::ControlChartResult primary;
    datalab::domain::statistics::ControlChartResult secondary;
    std::vector<std::size_t> subgroup_rows;
    if (subgroup_size > 1) {
        const auto subgroups = datalab::domain::statistics::build_subgroups(
            extracted.values, subgroup_size);
        const auto xbar_r =
            datalab::domain::statistics::ControlCharts::xbar_range_dual(subgroups);
        for (std::size_t index = 0; index < subgroups.size(); ++index) {
            const std::size_t source = index * subgroup_size;
            subgroup_rows.push_back(source < extracted.source_rows.size()
                                        ? extracted.source_rows[source] : source);
        }
        primary = xbar_r.primary;
        secondary = xbar_r.secondary;
        primary_plots.push_back(control_plot("Xbar 图", "子组均值", primary, subgroup_rows));
    } else {
        datalab::domain::statistics::IndividualsMovingRangeOptions options;
        options.moving_range_length = std::max(2, configuration.control.moving_range_length);
        const auto imr =
            datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(
                extracted.values, options);
        primary = imr.primary;
        secondary = imr.secondary;
        primary_plots.push_back(control_plot("I 图", "测量值", primary, extracted.source_rows));
    }
    primary_plots.push_back(control_plot(
        subgroup_size > 1 ? "R 图" : "MR 图",
        subgroup_size > 1 ? "子组极差" : "移动极差",
        secondary,
        subgroup_size > 1 ? subgroup_rows : extracted.source_rows));
    const auto probability =
        datalab::domain::statistics::normal_probability_plot(
            extracted.values, extracted.source_rows);

    capability_page.id = new_id("sixpack");
    capability_page.title = "过程能力 Sixpack";
    capability_page.method_name = "Capability Sixpack";
    capability_page.configuration = configuration;
    PlotSpec capability_histogram;
    for (const auto& plot : capability_page.plots) {
        if (plot.kind == PlotKind::histogram) {
            capability_histogram = plot;
            break;
        }
    }
    capability_page.plots.clear();
    if (!primary_plots.empty()) {
        capability_page.plots.push_back(std::move(primary_plots[0]));
    }
    capability_page.plots.push_back(capability_histogram);
    if (primary_plots.size() > 1) {
        capability_page.plots.push_back(std::move(primary_plots[1]));
    }
    capability_page.plots.push_back(probability_plot_spec(probability, extracted.name));
    capability_page.plots.push_back(last_points_plot(extracted, primary, subgroup_size));
    PlotSpec capability_plot;
    capability_plot.kind = PlotKind::control;
    capability_plot.title = "能力图";
    capability_plot.x_axis_title = "指标";
    capability_plot.y_axis_title = "能力指数";
    for (std::size_t index = 0; index < capability_indices.size(); ++index) {
        capability_plot.values.push_back(capability_indices[index]);
        capability_plot.x_values.push_back(static_cast<double>(index));
    }
    capability_page.plots.push_back(capability_plot);
    capability_page.parameter_summary +=
        "    正态概率图相关系数 = " + format_number(probability.correlation);
    return capability_page;
}

OutputPage AnalysisService::histogram(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (extracted.values.empty()) {
        return error_page("直方图", "Histogram", "所选列没有数值观测。");
    }
    const auto bins = datalab::domain::statistics::histogram(
        extracted.values, configuration.graph.bin_count);
    OutputPage page;
    page.id = new_id("hist");
    page.title = "直方图";
    page.method_name = "Histogram";
    page.configuration = configuration;
    const std::string bin_rule = configuration.graph.bin_count > 0
        ? "手工 " + std::to_string(configuration.graph.bin_count)
        : "Sturges";
    page.parameter_summary = "变量: " + extracted.name
        + "    N = " + std::to_string(extracted.values.size())
        + "    组数 = " + std::to_string(bins.counts.size())
        + "    分箱规则 = " + bin_rule;

    const auto append_histogram = [&](const std::string& title,
                                      const datalab::domain::statistics::HistogramResult& result,
                                      const std::vector<double>& values,
                                      const std::vector<std::size_t>& source_rows) {
        PlotSpec hist;
        hist.kind = PlotKind::histogram;
        hist.title = title;
        hist.x_axis_title = extracted.name;
        hist.y_axis_title = "频数";
        hist.histogram_edges = result.edges;
        hist.histogram_counts = result.counts;
        hist.values = values;
        hist.source_rows = source_rows;
        page.plots.push_back(std::move(hist));
    };

    const std::optional<std::size_t> group_column =
        configuration.graph.by_column.has_value()
            ? configuration.graph.by_column : configuration.by_column;
    if (group_column.has_value()) {
        const std::vector<std::string> labels = extract_text_column(table, *group_column);
        std::vector<std::string> order;
        std::vector<std::vector<double>> grouped_values;
        std::vector<std::vector<std::size_t>> grouped_rows;
        for (std::size_t index = 0; index < extracted.values.size(); ++index) {
            const std::size_t row = extracted.source_rows[index];
            const std::string group = row < labels.size() ? labels[row] : "*";
            const std::size_t group_index = datalab::domain::stable_group_index(order, group);
            if (group_index >= grouped_values.size()) {
                grouped_values.emplace_back();
                grouped_rows.emplace_back();
            }
            grouped_values[group_index].push_back(extracted.values[index]);
            grouped_rows[group_index].push_back(row);
        }
        for (std::size_t group = 0; group < order.size(); ++group) {
            const auto grouped_bins = datalab::domain::statistics::histogram_with_edges(
                grouped_values[group], bins.edges);
            append_histogram(extracted.name + " | " + order[group], grouped_bins,
                             grouped_values[group], grouped_rows[group]);
        }
    } else {
        append_histogram(extracted.name, bins, extracted.values, extracted.source_rows);
    }
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::eda_4plot(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (extracted.values.size() < 2) {
        return error_page("EDA 四图", "EDA 4-Plot", "至少需要两个数值观测。");
    }
    OutputPage page;
    page.id = new_id("eda4");
    page.title = "EDA 四图";
    page.method_name = "EDA 4-Plot";
    page.configuration = configuration;
    page.parameter_summary =
        "变量: " + extracted.name
        + "    N = " + std::to_string(extracted.values.size())
        + "    （NIST：run sequence / lag-1 / histogram / normal probability）";

    PlotSpec run_sequence;
    run_sequence.kind = PlotKind::scatter;
    run_sequence.title = "Run Sequence";
    run_sequence.x_axis_title = "观测顺序";
    run_sequence.y_axis_title = extracted.name;
    for (std::size_t index = 0; index < extracted.values.size(); ++index) {
        run_sequence.x_values.push_back(static_cast<double>(index + 1));
        run_sequence.values.push_back(extracted.values[index]);
    }
    run_sequence.source_rows = extracted.source_rows;
    page.plots.push_back(std::move(run_sequence));

    PlotSpec lag_plot;
    lag_plot.kind = PlotKind::scatter;
    lag_plot.title = "Lag-1 Plot";
    lag_plot.x_axis_title = "Y(i-1)";
    lag_plot.y_axis_title = "Y(i)";
    for (std::size_t index = 1; index < extracted.values.size(); ++index) {
        lag_plot.x_values.push_back(extracted.values[index - 1]);
        lag_plot.values.push_back(extracted.values[index]);
        lag_plot.source_rows.push_back(extracted.source_rows[index]);
    }
    page.plots.push_back(std::move(lag_plot));

    const auto bins = datalab::domain::statistics::histogram(
        extracted.values, configuration.graph.bin_count);
    PlotSpec hist;
    hist.kind = PlotKind::histogram;
    hist.title = "Histogram";
    hist.x_axis_title = extracted.name;
    hist.y_axis_title = "频数";
    hist.histogram_edges = bins.edges;
    hist.histogram_counts = bins.counts;
    hist.values = extracted.values;
    hist.source_rows = extracted.source_rows;
    page.plots.push_back(std::move(hist));

    PlotSpec probability;
    probability.kind = PlotKind::probability;
    probability.title = "Normal Probability Plot";
    probability.x_axis_title = "理论分位数";
    probability.y_axis_title = extracted.name;
    const auto npp = datalab::domain::statistics::normal_probability_plot(
        extracted.values, extracted.source_rows);
    probability.x_values = npp.theoretical_quantiles;
    probability.values = npp.ordered_values;
    probability.source_rows = npp.source_rows;
    page.plots.push_back(std::move(probability));

    StatisticTable summary;
    summary.title = "四图说明";
    summary.headers = {"图", "检查假设"};
    summary.rows = {
        {"Run Sequence", "位置是否漂移；散布是否大致恒定"},
        {"Lag-1", "相邻观测是否呈结构（随机性）"},
        {"Histogram", "分布形态（是否近似钟形）"},
        {"Normal Probability", "正态分位是否近似直线"},
    };
    page.tables.push_back(std::move(summary));

    domain::EdaPlotFacts facts;
    facts.kind = "eda_4plot";
    facts.n = extracted.values.size();
    page.facts.eda = facts;
    page.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "eda_4plot_exploratory",
        "EDA 四图用于探索位置/散布/随机性/分布形态假设，不能写成过程受控或分布已正态。"});
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::boxplot(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const ExtractedNumericColumn extracted =
        extract_numeric_column(table, first_variable(configuration), configuration.excluded_rows);
    if (extracted.values.empty()) {
        return error_page("箱线图", "Boxplot", "所选列没有数值观测。");
    }
    OutputPage page;
    page.id = new_id("box");
    page.title = "箱线图";
    page.method_name = "Boxplot";
    page.configuration = configuration;
    PlotSpec plot;
    plot.kind = PlotKind::boxplot;
    plot.title = extracted.name;
    plot.y_axis_title = extracted.name;
    plot.x_axis_title = "分组";
    plot.source_rows = extracted.source_rows;
    PlotSeries outliers;
    outliers.role = PlotSeriesRole::generic;
    outliers.label = "异常点";
    outliers.style.color = "#c62828";
    outliers.style.point_style = PlotPointStyle::circle;
    outliers.show_points = true;
    StatisticTable summary_table;
    summary_table.title = "箱线统计";
    summary_table.headers = {
        "分组", "N", "最小值", "Q1", "中位数", "Q3", "最大值", "IQR", "下须", "上须", "异常点数"};

    const auto append_group = [&](const std::string& label, const std::vector<double>& values) {
        const auto summary = datalab::domain::statistics::box_plot_summary(values);
        const std::size_t group_index = plot.box_labels.size();
        plot.box_labels.push_back(label);
        plot.box_min.push_back(summary.whisker_low);
        plot.box_q1.push_back(summary.first_quartile);
        plot.box_median.push_back(summary.median);
        plot.box_q3.push_back(summary.third_quartile);
        plot.box_max.push_back(summary.whisker_high);
        for (const double outlier : summary.outliers) {
            outliers.x_values.push_back(static_cast<double>(group_index));
            outliers.values.push_back(outlier);
        }
        summary_table.rows.push_back({
            label,
            std::to_string(summary.count),
            format_number(summary.minimum),
            format_number(summary.first_quartile),
            format_number(summary.median),
            format_number(summary.third_quartile),
            format_number(summary.maximum),
            format_number(summary.iqr),
            format_number(summary.whisker_low),
            format_number(summary.whisker_high),
            std::to_string(summary.outliers.size())});
    };

    if (configuration.by_column.has_value()) {
        const std::vector<std::string> groups = extract_text_column(table, *configuration.by_column);
        std::vector<std::string> order;
        std::vector<std::vector<double>> grouped;
        for (std::size_t index = 0; index < extracted.values.size(); ++index) {
            const std::size_t row = extracted.source_rows[index];
            const std::string group = row < groups.size() ? groups[row] : "*";
            const std::size_t group_index = datalab::domain::stable_group_index(order, group);
            if (group_index >= grouped.size()) {
                grouped.emplace_back();
            }
            grouped[group_index].push_back(extracted.values[index]);
        }
        for (std::size_t group = 0; group < order.size(); ++group) {
            append_group(order[group], grouped[group]);
        }
    } else {
        append_group(extracted.name, extracted.values);
    }
    if (!outliers.values.empty()) {
        plot.series.push_back(std::move(outliers));
    }
    page.plots.push_back(plot);
    page.tables.push_back(std::move(summary_table));
    page.parameter_summary = "变量: " + extracted.name
        + "    须线 = Tukey 1.5×IQR    组数 = "
        + std::to_string(plot.box_labels.size());
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::pareto(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    const std::size_t column = first_variable(configuration);
    std::map<std::string, std::size_t> counts;
    std::vector<DiagnosticMessage> diagnostics;
    if (configuration.selection.defect_count_column.has_value()) {
        const ExtractedNumericColumn counts_col = extract_numeric_column(
            table, *configuration.selection.defect_count_column, configuration.excluded_rows);
        const std::vector<std::string> names = extract_text_column(table, column);
        for (std::size_t index = 0; index < counts_col.values.size(); ++index) {
            const std::size_t row = counts_col.source_rows[index];
            const std::string name = row < names.size() ? names[row] : "?";
            if (is_missing_cell(name)) {
                diagnostics.push_back({
                    DiagnosticMessage::Severity::warning,
                    "missing_pareto_category",
                    "已忽略类别为空或 * 的汇总行。"});
                continue;
            }
            const double count = counts_col.values[index];
            if (count < 0.0 || std::floor(count) != count) {
                diagnostics.push_back({
                    DiagnosticMessage::Severity::warning,
                    "invalid_pareto_count",
                    "已忽略负数或非整数缺陷计数。"});
                continue;
            }
            counts[name] += static_cast<std::size_t>(count);
        }
    } else {
        for (std::size_t row = 0; row < table.rows.size(); ++row) {
            if (std::find(configuration.excluded_rows.begin(), configuration.excluded_rows.end(), row)
                != configuration.excluded_rows.end()) {
                continue;
            }
            if (column >= table.rows[row].size() || is_missing_cell(table.rows[row][column])) {
                continue;
            }
            ++counts[table.rows[row][column]];
        }
    }
    std::vector<std::pair<std::string, std::size_t>> pairs(counts.begin(), counts.end());
    datalab::domain::statistics::ParetoOptions options;
    options.other_threshold_percent = configuration.pareto_other_threshold_percent;
    const auto items = datalab::domain::statistics::pareto(pairs, options);
    if (items.empty()) {
        return error_page("柏拉图", "Pareto Chart", "没有可用于柏拉图的类别数据。");
    }
    OutputPage page;
    page.id = new_id("pareto");
    page.title = "柏拉图";
    page.method_name = "Pareto Chart";
    page.configuration = configuration;
    page.diagnostics = diagnostics;
    page.parameter_summary = "类别列: " + column_label(table, column)
        + "    总计数 = " + std::to_string(
            std::accumulate(pairs.begin(), pairs.end(), std::size_t{0},
                            [](std::size_t total, const auto& item) {
                                return total + item.second;
                            }));
    if (configuration.pareto_other_threshold_percent.has_value()) {
        page.parameter_summary += "    Other 阈值 = "
            + format_number(*configuration.pareto_other_threshold_percent, 4) + "%";
    }
    StatisticTable table_out;
    table_out.title = "缺陷计数";
    table_out.headers = {"类别", "计数", "Percent", "Cum %"};
    PlotSpec plot;
    plot.kind = PlotKind::pareto;
    plot.title = column < table.columns.size()
        ? table.columns[column] + " 的 Pareto 图"
        : "C" + std::to_string(column + 1) + " 的 Pareto 图";
    plot.x_axis_title = "类别";
    plot.y_axis_title = "计数";
    for (const auto& item : items) {
        table_out.rows.push_back({
            item.category,
            std::to_string(item.count),
            format_number(item.percent, 4),
            format_number(item.cumulative_percent, 4)});
        plot.categories.push_back(item.category);
        plot.category_values.push_back(static_cast<double>(item.count));
        plot.cumulative_percent.push_back(item.cumulative_percent);
    }
    page.tables.push_back(table_out);
    page.plots.push_back(plot);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::run_chart(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() != 1) {
        return error_page("运行图", "Run Chart",
                          "请选择一列数值观测（子组大小=1）。");
    }
    const std::size_t column = configuration.variable_columns.front();
    const auto extracted = extract_numeric_column(
        table, column, configuration.excluded_rows);
    OutputPage page;
    page.id = new_id("run_chart");
    page.title = "运行图";
    page.method_name = "Run Chart";
    page.configuration = configuration;
    page.parameter_summary = extracted.name;
    if (has_interior_numeric_gap(table, column, configuration.excluded_rows, extracted)) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::error,
            "run_chart_interior_missing",
            "序列中间存在缺失或非法值，运行图随机性检验要求完整连续观测，未计算四模式 P。"});
        domain::RunChartFacts facts;
        facts.missing_count = extracted.missing_count + extracted.invalid_count;
        page.facts.run_chart = std::move(facts);
        return finalize_page(std::move(page));
    }
    const auto result = datalab::domain::statistics::run_chart_analysis(
        extracted.values);
    page.diagnostics = result.diagnostics;
    if (extracted.missing_count + extracted.invalid_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "运行图跳过两端缺失或非法单元格 N* = "
                + std::to_string(extracted.missing_count + extracted.invalid_count)
                + "。"});
    }
    page.parameter_summary = extracted.name
        + "    中位数 = " + format_number(result.median)
        + "    N = " + std::to_string(result.n);
    StatisticTable about;
    about.title = "关于中位数的游程";
    about.headers = {
        "Number of runs", "Expected", "Longest run",
        "P clustering", "P mixtures"};
    about.rows.push_back({
        std::to_string(result.runs_about_median),
        format_optional(result.expected_runs_about_median),
        std::to_string(result.longest_run_about_median),
        format_optional(result.p_clustering),
        format_optional(result.p_mixtures)});
    page.tables.push_back(std::move(about));
    StatisticTable updown;
    updown.title = "上升/下降游程";
    updown.headers = {
        "Number of runs", "Expected", "Longest run",
        "P trends", "P oscillation"};
    updown.rows.push_back({
        std::to_string(result.runs_up_down),
        format_optional(result.expected_runs_up_down),
        std::to_string(result.longest_run_up_down),
        format_optional(result.p_trends),
        format_optional(result.p_oscillation)});
    page.tables.push_back(std::move(updown));
    if (!extracted.values.empty()) {
        PlotSpec plot;
        plot.kind = PlotKind::control;
        plot.title = extracted.name + " 运行图";
        plot.x_axis_title = "观测序号";
        plot.y_axis_title = extracted.name;
        plot.center_label = "中位数";
        plot.lower_style.visible = false;
        plot.upper_style.visible = false;
        for (std::size_t index = 0; index < extracted.values.size(); ++index) {
            plot.x_values.push_back(static_cast<double>(index + 1));
            plot.values.push_back(extracted.values[index]);
            plot.center.push_back(result.median);
        }
        plot.source_rows = extracted.source_rows;
        page.plots.push_back(std::move(plot));
    }
    domain::RunChartFacts facts;
    facts.n = result.n;
    facts.median = result.median;
    facts.runs_about_median = result.runs_about_median;
    facts.runs_up_down = result.runs_up_down;
    facts.p_clustering = result.p_clustering;
    facts.p_mixtures = result.p_mixtures;
    facts.p_trends = result.p_trends;
    facts.p_oscillation = result.p_oscillation;
    facts.missing_count = extracted.missing_count + extracted.invalid_count;
    page.facts.run_chart = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::cause_and_effect(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() != 2) {
        return error_page("因果图", "Cause-and-Effect",
                          "请选择类别列与原因列。");
    }
    const std::size_t category_col = configuration.variable_columns[0];
    const std::size_t cause_col = configuration.variable_columns[1];
    std::set<std::size_t> excluded(
        configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend());
    std::vector<std::pair<std::string, std::string>> pairs;
    std::size_t missing_count = 0;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (excluded.count(row_index) != 0) {
            continue;
        }
        const auto& row = table.rows[row_index];
        const std::string category =
            category_col < row.size() ? row[category_col] : "";
        const std::string cause =
            cause_col < row.size() ? row[cause_col] : "";
        if (is_missing_cell(category) || is_missing_cell(cause)
            || category.empty() || cause.empty()) {
            ++missing_count;
            continue;
        }
        pairs.emplace_back(category, cause);
    }
    const std::string effect = configuration.effect_title.empty()
        ? "效应" : configuration.effect_title;
    const auto result = datalab::domain::statistics::cause_and_effect_summarize(
        effect, pairs);
    OutputPage page;
    page.id = new_id("cause_and_effect");
    page.title = "因果图";
    page.method_name = "Cause-and-Effect";
    page.configuration = configuration;
    page.parameter_summary = "效应: " + result.effect
        + "    类别数 = " + std::to_string(result.categories.size())
        + "    原因数 = " + std::to_string(result.cause_count)
        + "    N* = " + std::to_string(missing_count);
    page.diagnostics = result.diagnostics;
    if (missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "因果图跳过 " + std::to_string(missing_count)
                + " 个空类别或空原因单元格。"});
    }
    StatisticTable summary;
    summary.title = "结构摘要";
    summary.headers = {"类别", "原因数", "原因"};
    PlotSpec plot;
    plot.kind = PlotKind::pareto;
    plot.title = result.effect + " — 类别原因计数";
    plot.x_axis_title = "类别";
    plot.y_axis_title = "原因数";
    for (const auto& category : result.categories) {
        std::string joined;
        for (std::size_t index = 0; index < category.causes.size(); ++index) {
            if (index > 0) {
                joined += "; ";
            }
            joined += category.causes[index];
        }
        summary.rows.push_back({
            category.category,
            std::to_string(category.causes.size()),
            joined});
        plot.categories.push_back(category.category);
        plot.category_values.push_back(
            static_cast<double>(category.causes.size()));
    }
    page.tables.push_back(std::move(summary));
    if (!plot.categories.empty()) {
        page.plots.push_back(std::move(plot));
    }
    domain::CauseEffectFacts facts;
    facts.effect = result.effect;
    facts.category_count = result.categories.size();
    facts.cause_count = result.cause_count;
    facts.missing_count = missing_count;
    page.facts.cause_effect = std::move(facts);
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::acceptance_sampling(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    (void)table;
    const std::size_t sample_size = configuration.inference.acceptance_sample_size;
    const std::size_t acceptance_number = configuration.inference.acceptance_number;
    if (sample_size == 0) {
        return error_page("属性抽样", "Acceptance Sampling", "请指定样本量 n ≥ 1。");
    }
    const auto result = datalab::domain::statistics::acceptance_sampling_binomial(
        sample_size,
        acceptance_number,
        configuration.inference.acceptance_aql,
        configuration.inference.acceptance_rql,
        configuration.inference.acceptance_lot_size);
    OutputPage page;
    page.id = new_id("acceptance_sampling");
    page.title = "属性一次抽样";
    page.method_name = "Acceptance Sampling";
    page.configuration = configuration;
    page.parameter_summary = "模型 = 二项 OC    n = " + std::to_string(sample_size)
        + "    c = " + std::to_string(acceptance_number)
        + (result.lot_size.has_value()
               ? "    N = " + std::to_string(*result.lot_size) + "（信息）" : "")
        + (result.aql.has_value()
               ? "    AQL = " + format_number(*result.aql) : "")
        + (result.rql.has_value()
               ? "    RQL = " + format_number(*result.rql) : "");
    page.diagnostics = result.diagnostics;
    StatisticTable plan;
    plan.title = "抽样计划";
    plan.headers = {"n", "c", "模型", "N（可选）", "AQL", "RQL", "Pa(AQL)", "Pa(RQL)"};
    plan.rows.push_back({
        std::to_string(result.sample_size),
        std::to_string(result.acceptance_number),
        result.model,
        result.lot_size.has_value() ? std::to_string(*result.lot_size) : "*",
        format_optional(result.aql),
        format_optional(result.rql),
        format_optional(result.pa_at_aql),
        format_optional(result.pa_at_rql)});
    page.tables.push_back(std::move(plan));
    if (!result.oc_curve.empty()) {
        StatisticTable oc;
        oc.title = "OC 曲线";
        oc.headers = {"p", "Pa(p)"};
        for (const auto& point : result.oc_curve) {
            oc.rows.push_back({
                format_number(point.fraction_defective),
                format_number(point.probability_accept)});
        }
        page.tables.push_back(std::move(oc));
        PlotSpec plot;
        plot.kind = PlotKind::scatter;
        plot.title = "OC 曲线（二项）";
        plot.x_axis_title = "不合格品率 p";
        plot.y_axis_title = "接收概率 Pa(p)";
        plot.show_legend = false;
        PlotSeries curve;
        curve.label = "Pa(p)";
        curve.style.point_style = PlotPointStyle::circle;
        curve.style.line_style = PlotLineStyle::solid;
        for (const auto& point : result.oc_curve) {
            curve.x_values.push_back(point.fraction_defective);
            curve.values.push_back(point.probability_accept);
            plot.x_values.push_back(point.fraction_defective);
            plot.values.push_back(point.probability_accept);
        }
        plot.series.push_back(std::move(curve));
        page.plots.push_back(std::move(plot));
    }
    domain::AcceptanceSamplingFacts facts;
    facts.sample_size = result.sample_size;
    facts.acceptance_number = result.acceptance_number;
    facts.lot_size = result.lot_size;
    facts.model = result.model;
    facts.aql = result.aql;
    facts.rql = result.rql;
    facts.pa_at_aql = result.pa_at_aql;
    facts.pa_at_rql = result.pa_at_rql;
    facts.oc_point_count = result.oc_curve.size();
    page.facts.acceptance_sampling = std::move(facts);
    page.method_metadata.estimation_method = "binomial_oc";
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::anom(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (!configuration.by_column.has_value()) {
        return error_page("ANOM", "Analysis of Means",
                          "请选择因子/分组列。");
    }
    const ExtractedNumericColumn response = extract_numeric_column(
        table, first_variable(configuration), configuration.excluded_rows);
    const std::vector<std::string> labels = extract_text_column(
        table, *configuration.by_column);
    std::map<std::string, std::vector<double>> grouped;
    for (std::size_t index = 0; index < response.values.size(); ++index) {
        const std::size_t row = response.source_rows[index];
        if (row >= labels.size() || is_missing_cell(labels[row])) {
            return error_page("ANOM", "Analysis of Means",
                              "因子列存在缺失标签，无法进行分组。原始行 "
                                  + std::to_string(row + 1));
        }
        grouped[labels[row]].push_back(response.values[index]);
    }
    std::vector<std::vector<double>> groups;
    std::vector<std::string> group_labels;
    for (const auto& [label, values] : grouped) {
        group_labels.push_back(label);
        groups.push_back(values);
    }
    const double alpha = configuration.inference.anom_alpha > 0.0
            && configuration.inference.anom_alpha < 1.0
        ? configuration.inference.anom_alpha
        : 0.05;
    const auto result = datalab::domain::statistics::analysis_of_means(
        groups, group_labels, alpha);
    OutputPage page;
    page.id = new_id("anom");
    page.title = "均值分析 (ANOM)";
    page.method_name = "Analysis of Means";
    page.configuration = configuration;
    page.parameter_summary = "响应 = " + response.name
        + "    因子 = " + column_label(table, *configuration.by_column)
        + "    α = " + format_number(result.alpha)
        + "    方法 = " + result.decision_limit_method;
    page.diagnostics = result.diagnostics;
    page.diagnostics.push_back({
        DiagnosticMessage::Severity::warning,
        "anom_normal_only",
        "本轮 ANOM 仅支持正态均值；二项/泊松计数请用其他工具。"});
    if (response.missing_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "ANOM 跳过 " + std::to_string(response.missing_count)
                + " 个缺失或非法响应值。"});
    }
    StatisticTable limits;
    limits.title = "决策限";
    limits.headers = {"总体均值", "组内 SD", "UDL", "LDL", "α", "方法"};
    limits.rows.push_back({
        format_number(result.overall_mean),
        format_number(result.pooled_sd),
        format_number(result.udl),
        format_number(result.ldl),
        format_number(result.alpha),
        result.decision_limit_method});
    page.tables.push_back(std::move(limits));
    StatisticTable groups_table;
    groups_table.title = "组均值";
    groups_table.headers = {"组", "N", "Mean", "超出决策限"};
    std::size_t outside_count = 0;
    std::vector<std::string> plot_labels;
    std::vector<double> plot_means;
    for (const auto& group : result.groups) {
        if (group.outside_limits) {
            ++outside_count;
        }
        groups_table.rows.push_back({
            group.label,
            std::to_string(group.n),
            format_number(group.mean),
            group.outside_limits ? "是" : "否"});
        plot_labels.push_back(group.label);
        plot_means.push_back(group.mean);
    }
    page.tables.push_back(std::move(groups_table));
    if (!plot_means.empty()) {
        PlotSpec plot;
        plot.kind = PlotKind::control;
        plot.title = response.name + " ANOM 图";
        plot.x_axis_title = column_label(table, *configuration.by_column);
        plot.y_axis_title = response.name;
        plot.center_label = "总体均值";
        plot.center_style.label = "总体均值";
        plot.upper_style.label = "UDL";
        plot.lower_style.label = "LDL";
        plot.categories = plot_labels;
        for (std::size_t index = 0; index < plot_means.size(); ++index) {
            plot.x_values.push_back(static_cast<double>(index + 1));
            plot.values.push_back(plot_means[index]);
            plot.center.push_back(result.overall_mean);
            plot.upper.push_back(result.udl);
            plot.lower.push_back(result.ldl);
        }
        page.plots.push_back(std::move(plot));
    }
    domain::AnomFacts facts;
    facts.overall_mean = result.overall_mean;
    facts.pooled_sd = result.pooled_sd;
    facts.udl = result.udl;
    facts.ldl = result.ldl;
    facts.alpha = result.alpha;
    facts.group_count = result.groups.size();
    facts.total_n = result.total_n;
    facts.outside_count = outside_count;
    facts.decision_limit_method = result.decision_limit_method;
    page.facts.anom = std::move(facts);
    page.method_metadata.estimation_method = result.decision_limit_method;
    return finalize_page(std::move(page));
}

OutputPage AnalysisService::poisson_gof(
    const DataTable& table,
    const AnalysisConfiguration& configuration)
{
    if (configuration.variable_columns.size() != 1) {
        return error_page("泊松拟合优度", "Poisson Goodness-of-Fit",
                          "请选择一列非负整数计数。");
    }
    const ExtractedNumericColumn extracted = extract_numeric_column(
        table, configuration.variable_columns.front(), configuration.excluded_rows);
    const auto result = datalab::domain::statistics::poisson_goodness_of_fit(
        extracted.values);
    OutputPage page;
    page.id = new_id("poisson_gof");
    page.title = "泊松拟合优度";
    page.method_name = "Poisson Goodness-of-Fit";
    page.configuration = configuration;
    page.parameter_summary = "变量 = " + extracted.name
        + "    N = " + std::to_string(result.n)
        + "    λ̂ = " + format_number(result.lambda_hat)
        + "    N* = " + std::to_string(extracted.missing_count + extracted.invalid_count);
    page.diagnostics = result.diagnostics;
    if (extracted.missing_count + extracted.invalid_count > 0) {
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "missing_values",
            "泊松拟合优度跳过 "
                + std::to_string(extracted.missing_count + extracted.invalid_count)
                + " 个缺失或非法单元格。"});
    }
    StatisticTable estimate;
    estimate.title = "泊松参数";
    estimate.headers = {"N", "N*", "λ̂", "类别数", "期望<5 数", "有效性"};
    estimate.rows.push_back({
        std::to_string(result.n),
        std::to_string(extracted.missing_count + extracted.invalid_count),
        format_number(result.lambda_hat),
        std::to_string(result.category_count),
        std::to_string(result.expected_below_five_count),
        result.validity_status});
    page.tables.push_back(std::move(estimate));
    StatisticTable test;
    test.title = "卡方检验";
    test.headers = {"DF", "Chi-Sq", "P-Value"};
    test.rows.push_back({
        format_number(result.degrees_of_freedom),
        format_number(result.chi_square),
        result.p_value.has_value() ? format_number(*result.p_value) : "*"});
    page.tables.push_back(std::move(test));
    domain::ChiSquareGofFacts facts;
    facts.method = "poisson";
    facts.statistic = result.chi_square;
    facts.p_value = result.p_value;
    facts.degrees_of_freedom = result.degrees_of_freedom;
    facts.category_count = result.category_count;
    facts.total_count = result.n;
    facts.missing_count = extracted.missing_count + extracted.invalid_count;
    facts.expected_below_five_count = result.expected_below_five_count;
    facts.validity_status = result.validity_status;
    facts.proportion_source = "poisson_lambda_hat";
    facts.lambda_hat = result.lambda_hat;
    facts.expected_count_warning = result.expected_below_five_count > 0
        || result.validity_status != "ok";
    page.facts.chi_square_gof = std::move(facts);
    page.method_metadata.estimation_method = "poisson_pearson_chi_square";
    return finalize_page(std::move(page));
}

}  // namespace datalab::application
