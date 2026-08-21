#include "application/analysis_service.h"
#include "application/chart_pages.h"
#include "application/column_assembly.h"
#include "application/doe_pages.h"
#include "application/output_builder.h"

#include "domain/column_extract.h"
#include "domain/graph_assembly.h"
#include "domain/quality_diagnostics.h"
#include "domain/statistics/control_charts.h"
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
#include "domain/statistics/msa_type1.h"
#include "domain/statistics/reliability.h"
#include "domain/statistics/t_power.h"
#include "domain/statistics/nonparametric_tests.h"
#include "domain/statistics/time_series.h"
#include "domain/statistics/arima.h"
#include "domain/statistics/two_factor_anova.h"
#include "domain/statistics/logistic_regression.h"
#include "domain/statistics/variance_tests.h"
#include "domain/statistics/time_series_decomposition.h"
#include "domain/statistics/doe_factorial.h"
#include "domain/statistics/nested_gage_rr.h"
#include "domain/statistics/attribute_agreement.h"
#include "domain/statistics/seasonal_forecasting.h"
#include "domain/statistics/pca.h"
#include "domain/statistics/analysis_rules.h"
#include "domain/statistics/distribution_identification.h"
#include "domain/statistics/response_optimization.h"
#include "domain/statistics/multi_vari.h"
#include "domain/statistics/tolerance_intervals.h"
#include "domain/statistics/proportion_test.h"
#include "domain/statistics/poisson_rate.h"
#include "domain/statistics/equivalence_test.h"
#include "domain/statistics/grubbs_test.h"

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
    const auto result = datalab::domain::statistics::normality_test(
        extracted.values, extracted.source_rows);

    OutputPage page;
    page.id = new_id("normality");
    page.title = "正态性检验";
    page.method_name = "Normality Test";
    page.configuration = configuration;
    page.parameter_summary = "变量: " + extracted.name
        + "    方法: Anderson-Darling    缺失值 N* = "
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
    table_out.headers = {"变量", "N", "N*", "Mean", "StDev", "AD", "A²*", "Alpha",
                         "P-Value", "判定"};
    std::string conclusion = "无法计算";
    if (result.decision == "reject") {
        conclusion = "在 alpha 下拒绝正态假设";
    } else if (result.decision == "fail_to_reject") {
        conclusion = "在 alpha 下未拒绝正态假设";
    }
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
    facts.decision = result.decision;
    facts.p_value = result.p_value;
    facts.anderson_darling = result.anderson_darling;
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
    const auto result = datalab::domain::statistics::grubbs_outlier_test(
        extracted.values, extracted.source_rows,
        parse_alternative(configuration.inference.alternative),
        alpha, extracted.missing_count);

    OutputPage page;
    page.id = new_id("outlier_test");
    page.title = "异常值检验（Grubbs）";
    page.method_name = "Outlier Test";
    page.configuration = configuration;
    page.diagnostics = result.diagnostics;
    page.parameter_summary = "变量: " + extracted.name
        + "    方法: Grubbs    备择: " + result.alternative
        + "    α = " + format_number(result.alpha)
        + "    缺失值 N* = " + std::to_string(extracted.missing_count);
    StatisticTable table_out;
    table_out.title = "异常值检验";
    table_out.headers = {"变量", "N", "N*", "Mean", "StDev", "G", "P-Value",
                         "嫌疑值", "方向", "source_row"};
    table_out.rows.push_back({
        extracted.name,
        std::to_string(result.n),
        std::to_string(result.missing_count),
        result.n > 0 ? format_number(result.mean) : "*",
        result.sample_standard_deviation > 0.0
            ? format_number(result.sample_standard_deviation) : "*",
        result.g_statistic.has_value() ? format_number(*result.g_statistic) : "*",
        result.p_value.has_value() ? format_number(*result.p_value) : "*",
        result.outlier_value.has_value() ? format_number(*result.outlier_value) : "*",
        result.direction.empty() ? "*" : result.direction,
        result.source_row.has_value() ? std::to_string(*result.source_row) : "*"});
    page.tables.push_back(table_out);

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
            if (result.outlier_index.has_value() && index == *result.outlier_index) {
                plot.point_labels.push_back("嫌疑点");
            } else {
                plot.point_labels.push_back("");
            }
        }
        if (result.outlier_index.has_value()) {
            PlotSeries highlight;
            highlight.label = "嫌疑观测";
            highlight.style.color = "#c62828";
            highlight.style.point_style = PlotPointStyle::circle;
            highlight.show_points = true;
            highlight.x_values.push_back(
                static_cast<double>(*result.outlier_index + 1));
            highlight.values.push_back(extracted.values[*result.outlier_index]);
            plot.series.push_back(std::move(highlight));
        }
        page.plots.push_back(std::move(plot));
    }
    domain::OutlierTestFacts facts;
    facts.n = result.n;
    facts.missing_count = result.missing_count;
    if (result.n > 0) {
        facts.mean = result.mean;
        facts.standard_deviation = result.sample_standard_deviation;
    }
    facts.g_statistic = result.g_statistic;
    facts.p_value = result.p_value;
    facts.outlier_value = result.outlier_value;
    facts.source_row = result.source_row;
    facts.direction = result.direction;
    facts.alternative = result.alternative;
    facts.alpha = result.alpha;
    facts.assumption_status = result.assumption_status;
    page.facts.outlier_test = std::move(facts);
    page.method_metadata.estimation_method = "grubbs";
    page.method_metadata.assumption_status = result.assumption_status;
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
    const auto result = datalab::domain::statistics::correlation_matrix(
        columns,
        spearman ? datalab::domain::statistics::CorrelationMethod::spearman
                 : datalab::domain::statistics::CorrelationMethod::pearson,
        configuration.inference.confidence_level);
    OutputPage page;
    page.id = new_id("correlation");
    page.title = spearman ? "Spearman 秩相关" : "Pearson 相关";
    page.method_name = "Correlation";
    page.configuration = configuration;
    page.parameter_summary = "方法 = " + std::string(spearman ? "Spearman" : "Pearson")
        + "    置信水平 = " + format_number(configuration.inference.confidence_level)
        + "    有效变量数 = " + std::to_string(columns.size())
        + "    N = " + std::to_string(aligned.source_rows.size());
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
    }
    page.facts.chi_square = facts;
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
        const auto transform_limit = [&result](double value) {
            return result.lambda == 0.0
                ? std::log(value)
                : (std::pow(value, result.lambda) - 1.0) / result.lambda;
        };
        SpecificationLimits transformed_specifications;
        if (configuration.specifications.lower.has_value()) {
            transformed_specifications.lower = transform_limit(
                *configuration.specifications.lower);
        }
        if (configuration.specifications.upper.has_value()) {
            transformed_specifications.upper = transform_limit(
                *configuration.specifications.upper);
        }
        if (configuration.specifications.target.has_value()) {
            transformed_specifications.target = transform_limit(
                *configuration.specifications.target);
        }
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
            parse_alternative(configuration.inference.alternative));
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
    if (one_sample && result.location_estimate.has_value()) {
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
            parse_alternative(configuration.inference.alternative));
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
            parse_alternative(configuration.inference.alternative));
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
    page.facts.nonparametric = std::move(facts);
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
    const auto result = datalab::domain::statistics::mood_median_test(groups, group_labels);
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
    summary.headers = {"组别", "N", "组中位数", "N≤", "N>"};
    for (const auto& group : result.groups) {
        summary.rows.push_back({
            group.label,
            std::to_string(group.count),
            format_number(group.median),
            std::to_string(group.n_le),
            std::to_string(group.n_gt)});
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
        {"Test 1 超限点数", std::to_string(chart.test1_points.size())},
        {"规则策略", configuration.control.special_cause_rule_policy == "explicit"
            ? "用户指定" : "默认全选适用规则"},
        {"启用测试", datalab::domain::statistics::format_special_cause_tests(
            datalab::domain::statistics::resolve_special_cause_tests(
                options.special_causes,
                datalab::domain::statistics::ControlChartKind::ewma))},
        {"适用性", "EWMA 只开放 Test 1；Tests 2–8 不附加到 EWMA。"}};
    page.tables.push_back(parameters);
    page.tables.push_back(ewma_point_table(chart, extracted.values, extracted.source_rows));
    page.plots.push_back(control_plot("EWMA 控制图", "EWMA", chart, extracted.source_rows));
    page.facts.spc = domain::SpcFacts{};
    page.facts.spc->sigma_within =
        sigma > 0.0 ? std::optional<double>{sigma} : std::nullopt;
    page.facts.spc->out_of_control_count = chart.test1_points.size();
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
        {"Test 1 超限点数", std::to_string(chart.test1_points.size())},
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
        result.maximum_vif};
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
    const bool use_nonparametric = configuration.inference.variance_method == "nonparametric";
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
        datalab::domain::statistics::DoeFactorialDesign imported_design;
        for (const std::size_t column : configuration.doe.factor_columns) {
            if (column >= table.columns.size()) {
                return error_page("DOE 响应分析", "DOE Response Analysis",
                                  "设计因子列索引超出当前数据表范围。");
            }
            imported_design.factors.push_back({
                column_label(table, column), "-1", "+1"});
        }
        std::set<std::size_t> excluded(
            configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend());
        std::vector<double> responses;
        for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
            if (excluded.count(row_index) != 0) {
                continue;
            }
            const auto& row = table.rows[row_index];
            if (*configuration.doe.response_column >= row.size()) {
                continue;
            }
            std::vector<int> levels;
            bool valid_levels = true;
            for (std::size_t factor = 0;
                 factor < configuration.doe.factor_columns.size(); ++factor) {
                const std::size_t column = configuration.doe.factor_columns[factor];
                const std::string value = column < row.size() ? row[column] : "";
                const auto numeric = parse_numeric_cell(value);
                if (numeric.has_value() && (*numeric == -1.0 || *numeric == 1.0)) {
                    levels.push_back(static_cast<int>(*numeric));
                } else {
                    const std::string low = factor < configuration.doe.low_levels.size()
                        ? configuration.doe.low_levels[factor] : "-1";
                    const std::string high = factor < configuration.doe.high_levels.size()
                        ? configuration.doe.high_levels[factor] : "+1";
                    levels.push_back(value == low ? -1 : value == high ? 1 : 0);
                    if (levels.back() == 0) {
                        valid_levels = false;
                    }
                }
            }
            if (!valid_levels) {
                imported_design.diagnostics.push_back({
                    datalab::domain::DiagnosticMessage::Severity::warning,
                    "missing_doe_run", "存在缺少有效因子水平的运行，已跳过。"});
                continue;
            }
            datalab::domain::statistics::DoeRun run;
            run.standard_order = row_index;
            run.run_order = imported_design.runs.size();
            run.coded_levels = std::move(levels);
            imported_design.runs.push_back(std::move(run));
            responses.push_back(parse_numeric_cell(row[*configuration.doe.response_column])
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
        configuration.doe.random_seed});
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
    ImportedDoeRuns imported;
    for (const std::size_t column : configuration.doe.factor_columns) {
        imported.design.factors.push_back({column_label(table, column), "-1", "+1"});
    }
    std::set<std::size_t> excluded(
        configuration.excluded_rows.cbegin(), configuration.excluded_rows.cend());
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (excluded.count(row_index) != 0) {
            continue;
        }
        const auto& row = table.rows[row_index];
        std::vector<int> levels;
        bool valid_levels = true;
        for (std::size_t factor = 0;
             factor < configuration.doe.factor_columns.size(); ++factor) {
            const std::size_t column = configuration.doe.factor_columns[factor];
            const std::string value = column < row.size() ? row[column] : "";
            const auto numeric = parse_numeric_cell(value);
            if (numeric.has_value() && (*numeric == -1.0 || *numeric == 1.0)) {
                levels.push_back(static_cast<int>(*numeric));
            } else {
                const std::string low = factor < configuration.doe.low_levels.size()
                    ? configuration.doe.low_levels[factor] : "-1";
                const std::string high = factor < configuration.doe.high_levels.size()
                    ? configuration.doe.high_levels[factor] : "+1";
                levels.push_back(value == low ? -1 : value == high ? 1 : 0);
                if (levels.back() == 0) {
                    valid_levels = false;
                }
            }
        }
        if (!valid_levels) {
            ++imported.skipped_level_rows;
            continue;
        }
        datalab::domain::statistics::DoeRun run;
        run.standard_order = row_index;
        run.run_order = imported.design.runs.size();
        run.coded_levels = std::move(levels);
        imported.design.runs.push_back(std::move(run));
        imported.source_rows.push_back(row_index);
    }
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
    if (!configuration.reliability.time_column.has_value()
        || !configuration.reliability.event_column.has_value()) {
        return error_page("Reliability", "Reliability", "请选择寿命列和失效/删失指示列。");
    }
    const auto times = extract_numeric_column(
        table, *configuration.reliability.time_column, configuration.excluded_rows);
    const auto event_text = extract_text_column(
        table, *configuration.reliability.event_column);
    std::vector<double> aligned_times;
    std::vector<bool> events;
    std::vector<int> aligned_groups;
    std::vector<std::string> group_levels;
    std::vector<std::size_t> aligned_source_rows;
    std::vector<std::size_t> invalid_event_rows;
    const auto group_text = configuration.reliability.group_column.has_value()
        ? extract_text_column(table, *configuration.reliability.group_column)
        : std::vector<std::string>{};
    for (std::size_t index = 0; index < times.source_rows.size(); ++index) {
        const std::size_t row = times.source_rows[index];
        if (row >= event_text.size() || is_missing_cell(event_text[row])) continue;
        const auto parsed_event = datalab::domain::statistics::parse_reliability_event(
            event_text[row]);
        if (!parsed_event.has_value()) {
            invalid_event_rows.push_back(row);
            continue;
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
            if (group_levels.size() > 2) {
                return error_page("Reliability", "Reliability",
                                  "Log-rank 分组目前只支持两个非空水平。");
            }
            aligned_groups.push_back(
                static_cast<int>(std::distance(group_levels.begin(), level)));
        }
        aligned_times.push_back(times.values[index]);
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
    const std::string model = configuration.reliability.model;
    OutputPage page;
    page.id = new_id("reliability");
    page.title = "Reliability Analysis";
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
        plot.title = "Kaplan-Meier Survival Curve";
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
        page.plots.push_back(std::move(plot));
        page.facts.reliability = datalab::domain::statistics::kaplan_meier_facts_from(result);
        append_rule_table(page, page.facts.reliability->rules);
        domain::apply_evidence(page.method_metadata, result.evidence);
        if (aligned_groups.size() == aligned_times.size() && group_levels.size() == 2) {
            const auto log_rank = datalab::domain::statistics::log_rank_test(
                aligned_times, events, aligned_groups);
            page.diagnostics.insert(page.diagnostics.end(),
                                    log_rank.diagnostics.cbegin(),
                                    log_rank.diagnostics.cend());
            StatisticTable comparison;
            comparison.title = "Log-rank 分组比较";
            comparison.headers = {"Chi-Square", "DF", "P-Value",
                                  "Group 1 N", "Group 1 Failures", "Group 1 Censored",
                                  "Group 2 N", "Group 2 Failures", "Group 2 Censored"};
            comparison.rows.push_back({
                format_number(log_rank.chi_square),
                format_number(log_rank.degrees_of_freedom),
                format_number(log_rank.p_value),
                std::to_string(log_rank.group_one_n),
                std::to_string(log_rank.group_one_failures),
                std::to_string(log_rank.group_one_censored),
                std::to_string(log_rank.group_two_n),
                std::to_string(log_rank.group_two_failures),
                std::to_string(log_rank.group_two_censored)});
            page.tables.push_back(std::move(comparison));
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
    const auto compute = [&](std::size_t sample_size, double effect)
        -> datalab::domain::statistics::TPowerResult {
        if (configuration.power.mode.find("one_variance") == 0) {
            return calculate_power
                ? datalab::domain::statistics::one_variance_power(
                    sample_size, effect, configuration.power.alpha, alternative)
                : datalab::domain::statistics::one_variance_sample_size(
                    effect, configuration.power.target, configuration.power.alpha, alternative);
        }
        if (configuration.power.mode.find("two_variance") == 0) {
            return calculate_power
                ? datalab::domain::statistics::two_variance_power(
                    sample_size, effect, configuration.power.alpha, alternative)
                : datalab::domain::statistics::two_variance_sample_size(
                    effect, configuration.power.target, configuration.power.alpha, alternative);
        }
        if (configuration.power.mode.find("anova") == 0) {
            return calculate_power
                ? datalab::domain::statistics::one_way_anova_power(
                    sample_size, configuration.power.group_count,
                    effect, configuration.power.alpha)
                : datalab::domain::statistics::one_way_anova_sample_size(
                    configuration.power.group_count, effect,
                    configuration.power.target, configuration.power.alpha);
        }
        if (configuration.power.mode.find("one_poisson") == 0) {
            return calculate_power
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
            return calculate_power
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
            return calculate_power
                ? datalab::domain::statistics::one_sample_proportion_power(
                    sample_size, configuration.power.null_proportion,
                    configuration.power.second_proportion, configuration.power.alpha,
                    alternative)
                : datalab::domain::statistics::one_sample_proportion_sample_size(
                    configuration.power.null_proportion, configuration.power.second_proportion,
                    configuration.power.target, configuration.power.alpha, alternative);
        }
        if (configuration.power.mode.find("two_proportion") == 0) {
            return calculate_power
                ? datalab::domain::statistics::two_proportion_power(
                    sample_size, configuration.power.null_proportion,
                    configuration.power.second_proportion, configuration.power.alpha,
                    alternative, variance_method)
                : datalab::domain::statistics::two_proportion_sample_size(
                    configuration.power.null_proportion, configuration.power.second_proportion,
                    configuration.power.target, configuration.power.alpha,
                    alternative, variance_method);
        }
        if (calculate_power) {
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
                rows.push_back(compute(sample_size, effect));
            }
        }
    } else {
        for (const double effect : effects) {
            rows.push_back(compute(0, effect));
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
                const auto point = compute(n, effect);
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
                datalab::domain::statistics::TPowerResult powered;
                if (configuration.power.mode.find("anova") == 0) {
                    powered = datalab::domain::statistics::one_way_anova_power(
                        n, configuration.power.group_count, effect, configuration.power.alpha);
                } else if (configuration.power.mode.find("one_poisson") == 0) {
                    powered = datalab::domain::statistics::one_poisson_rate_power(
                        n, configuration.power.null_proportion,
                        configuration.power.second_proportion,
                        configuration.power.observation_length, configuration.power.alpha,
                        alternative);
                } else if (configuration.power.mode.find("two_poisson") == 0) {
                    powered = datalab::domain::statistics::two_poisson_rate_power(
                        n, configuration.power.null_proportion,
                        configuration.power.second_proportion,
                        configuration.power.observation_length, configuration.power.alpha,
                        alternative);
                } else if (configuration.power.mode.find("one_proportion") == 0) {
                    powered = datalab::domain::statistics::one_sample_proportion_power(
                        n, configuration.power.null_proportion,
                        configuration.power.second_proportion, configuration.power.alpha,
                        alternative);
                } else if (configuration.power.mode.find("two_proportion") == 0) {
                    powered = datalab::domain::statistics::two_proportion_power(
                        n, configuration.power.null_proportion,
                        configuration.power.second_proportion, configuration.power.alpha,
                        alternative, variance_method);
                } else if (configuration.power.mode.find("one_variance") == 0) {
                    powered = datalab::domain::statistics::one_variance_power(
                        n, effect, configuration.power.alpha, alternative);
                } else if (configuration.power.mode.find("two_variance") == 0) {
                    powered = datalab::domain::statistics::two_variance_power(
                        n, effect, configuration.power.alpha, alternative);
                } else if (two_sample) {
                    powered = datalab::domain::statistics::two_sample_t_power(
                        n, effect, configuration.power.alpha);
                } else {
                    powered = datalab::domain::statistics::one_sample_t_power(
                        n, effect, configuration.power.alpha);
                }
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
    options.method = configuration.control.sigma_method == "median_moving_range"
        ? datalab::domain::statistics::SigmaEstimateMethod::median_moving_range
        : datalab::domain::statistics::SigmaEstimateMethod::average_moving_range;
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
        + "    σ = MR / d2 = " + format_number(dual.sigma)
        + (historical ? "（历史参数）" : "（估计）");
    StatisticTable table_out;
    table_out.title = "I-MR 参数";
    table_out.headers = {"指标", "数值"};
    table_out.rows = {
        {"N", std::to_string(extracted.values.size())},
        {"N*", std::to_string(extracted.missing_count)},
        {"均值", format_number(dual.primary.center_line.empty() ? 0.0 : dual.primary.center_line.front())},
        {"MR̄", format_number(dual.average_moving_range)},
        {"σ (within)", format_number(dual.sigma)},
        {"参数来源", historical ? "历史参数" : "估计"},
        {"规则策略", configuration.control.special_cause_rule_policy == "explicit"
            ? "用户指定" : "默认全选适用规则"},
        {"I 图启用测试", datalab::domain::statistics::format_special_cause_tests(
            datalab::domain::statistics::resolve_special_cause_tests(
                options.special_causes,
                datalab::domain::statistics::ControlChartKind::individuals))},
        {"MR 图适用规则", "Test 1–4（Minitab 不在 MR 图上启用 Test 5–8）"},
        {"Test 1 超限点数", std::to_string(dual.primary.test1_points.size())}};
    page.tables.push_back(table_out);
    page.tables.push_back(individuals_point_table(
        dual.primary, dual.secondary, extracted.source_rows, stages));
    page.plots.push_back(control_plot("单值图 (I)", "测量值", dual.primary, extracted.source_rows));
    page.plots.push_back(control_plot("移动极差图 (MR)", "移动极差", dual.secondary, extracted.source_rows));
    page.facts.spc = domain::SpcFacts{};
    page.facts.spc->sigma_within = dual.sigma;
    page.facts.spc->out_of_control_count = dual.primary.test1_points.size();
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
            ? "用户指定" : "默认全选适用规则"},
        {"I 图启用测试", datalab::domain::statistics::format_special_cause_tests(
            datalab::domain::statistics::resolve_special_cause_tests(
                special_causes,
                datalab::domain::statistics::ControlChartKind::individuals))},
        {"MR/R/S 适用规则", "Test 1–4"}};
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
                              "I CL", "I LCL", "I UCL", "触发测试", "最小测试"};
    for (std::size_t index = 0; index < input->values.size(); ++index) {
        std::string triggered;
        if (index < chart.individuals.triggered_tests.size()) {
            for (const int test : chart.individuals.triggered_tests[index]) {
                if (!triggered.empty()) {
                    triggered += ",";
                }
                triggered += "Test " + std::to_string(test);
            }
        }
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
            index < chart.individuals.primary_test_by_point.size()
                && chart.individuals.primary_test_by_point[index] > 0
                ? "Test " + std::to_string(chart.individuals.primary_test_by_point[index])
                : ""});
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
        "Test 1 = 超出 3σ 控制限的点";
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
        "Test 1 = 超出 3σ 控制限的点";
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
        "C 图要求每个子组单位数相同    Test 1 = 超出 3σ 控制限的点";
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
        "Test 1 = 超出 3σ 控制限的点";
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
    const auto capability_result =
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
    AnalysisConfiguration page_configuration = configuration;
    page_configuration.capability_method = "between_within";
    const int subgroup_size = static_cast<int>(subgroups->values.front().size());
    OutputPage page = build_capability_content(
        page_configuration, extracted, capability_result, subgroup_size, "R̄ / d2(n)");
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
                : datalab::domain::statistics::SigmaEstimateMethod::average_moving_range;
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

}  // namespace datalab::application
