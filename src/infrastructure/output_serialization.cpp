#include "infrastructure/output_serialization.h"

#include <QJsonArray>
#include <algorithm>
#include <optional>
#include <utility>

namespace datalab::infrastructure {
namespace {

QJsonArray string_array(const std::vector<std::string>& values)
{
    QJsonArray array;
    for (const std::string& value : values) {
        array.append(QString::fromStdString(value));
    }
    return array;
}

QJsonArray number_array(const std::vector<double>& values)
{
    QJsonArray array;
    for (const double value : values) {
        array.append(value);
    }
    return array;
}

QJsonArray size_array(const std::vector<std::size_t>& values)
{
    QJsonArray array;
    for (const std::size_t value : values) {
        array.append(static_cast<qint64>(value));
    }
    return array;
}

std::vector<std::string> to_strings(const QJsonArray& array)
{
    std::vector<std::string> values;
    for (const QJsonValue& value : array) {
        values.push_back(value.toString().toStdString());
    }
    return values;
}

std::vector<double> to_numbers(const QJsonArray& array)
{
    std::vector<double> values;
    for (const QJsonValue& value : array) {
        values.push_back(value.toDouble());
    }
    return values;
}

std::vector<std::size_t> to_sizes(const QJsonArray& array)
{
    std::vector<std::size_t> values;
    for (const QJsonValue& value : array) {
        values.push_back(static_cast<std::size_t>(value.toInteger()));
    }
    return values;
}

QJsonValue optional_number(const std::optional<double>& value)
{
    return value.has_value() ? QJsonValue(*value) : QJsonValue();
}

QJsonValue optional_size(const std::optional<std::size_t>& value)
{
    return value.has_value() ? QJsonValue(static_cast<qint64>(*value)) : QJsonValue();
}

std::optional<std::size_t> read_optional_size(const QJsonValue& value)
{
    if (!value.isDouble()) {
        return std::nullopt;
    }
    const qint64 number = value.toInteger();
    return number >= 0 ? std::optional<std::size_t>(
        static_cast<std::size_t>(number)) : std::nullopt;
}

std::optional<double> read_optional(const QJsonValue& value)
{
    if (value.isDouble()) {
        return value.toDouble();
    }
    return std::nullopt;
}

}  // namespace

QJsonObject output_page_to_json(const domain::OutputPage& page)
{
    QJsonObject object;
    object.insert(QStringLiteral("schema_version"), 2);
    object.insert(QStringLiteral("id"), QString::fromStdString(page.id));
    object.insert(QStringLiteral("title"), QString::fromStdString(page.title));
    object.insert(QStringLiteral("method_name"), QString::fromStdString(page.method_name));
    object.insert(QStringLiteral("parameter_summary"), QString::fromStdString(page.parameter_summary));
    object.insert(QStringLiteral("chart_type"), QString::fromStdString(page.configuration.chart_type));
    object.insert(QStringLiteral("analysis_name"), QString::fromStdString(page.configuration.analysis_name));
    object.insert(QStringLiteral("variable_columns"), size_array(page.configuration.variable_columns));
    object.insert(QStringLiteral("excluded_rows"), size_array(page.configuration.excluded_rows));
    object.insert(QStringLiteral("included_rows"), size_array(page.configuration.included_rows));
    object.insert(QStringLiteral("stage_column"), optional_size(page.configuration.stage_column));
    object.insert(QStringLiteral("measurement_column"),
                  static_cast<qint64>(page.configuration.selection.measurement_column));
    object.insert(QStringLiteral("subgroup_column"),
                  optional_size(page.configuration.selection.subgroup_column));
    object.insert(QStringLiteral("time_column"),
                  optional_size(page.configuration.selection.time_column));
    object.insert(QStringLiteral("product_column"),
                  optional_size(page.configuration.selection.product_column));
    object.insert(QStringLiteral("defect_count_column"),
                  optional_size(page.configuration.selection.defect_count_column));
    object.insert(QStringLiteral("inspected_count_column"),
                  optional_size(page.configuration.selection.inspected_count_column));
    object.insert(QStringLiteral("inspected_constant"),
                  optional_size(page.configuration.inspected_constant));
    object.insert(QStringLiteral("first_events_column"),
                  optional_size(page.configuration.first_events_column));
    object.insert(QStringLiteral("first_trials_column"),
                  optional_size(page.configuration.first_trials_column));
    object.insert(QStringLiteral("second_events_column"),
                  optional_size(page.configuration.second_events_column));
    object.insert(QStringLiteral("second_trials_column"),
                  optional_size(page.configuration.second_trials_column));
    object.insert(QStringLiteral("row_category_column"),
                  optional_size(page.configuration.row_category_column));
    object.insert(QStringLiteral("column_category_column"),
                  optional_size(page.configuration.column_category_column));
    object.insert(QStringLiteral("lower_spec"), optional_number(page.configuration.specifications.lower));
    object.insert(QStringLiteral("upper_spec"), optional_number(page.configuration.specifications.upper));
    object.insert(QStringLiteral("target_spec"), optional_number(page.configuration.specifications.target));
    object.insert(QStringLiteral("by_column"), optional_size(page.configuration.by_column));
    object.insert(QStringLiteral("moving_range_length"), page.configuration.moving_range_length);
    object.insert(QStringLiteral("sigma_method"), QString::fromStdString(page.configuration.sigma_method));
    object.insert(QStringLiteral("historical_center"),
                  optional_number(page.configuration.historical_center));
    object.insert(QStringLiteral("historical_sigma_z"),
                  optional_number(page.configuration.historical_sigma_z));
    object.insert(QStringLiteral("leave_gaps_for_excluded"),
                  page.configuration.leave_gaps_for_excluded);
    QJsonArray enabled_tests;
    for (const int test : page.configuration.enabled_special_cause_tests) {
        enabled_tests.append(test);
    }
    object.insert(QStringLiteral("enabled_special_cause_tests"), enabled_tests);
    object.insert(QStringLiteral("subgroup_size"),
                  page.configuration.subgroup_size.has_value()
                      ? QJsonValue(*page.configuration.subgroup_size) : QJsonValue());
    object.insert(QStringLiteral("pareto_other_threshold_percent"),
                  optional_number(page.configuration.pareto_other_threshold_percent));
    object.insert(QStringLiteral("hypothesis_mean"),
                  optional_number(page.configuration.hypothesis_mean));
    object.insert(QStringLiteral("confidence_level"), page.configuration.confidence_level);
    object.insert(QStringLiteral("alternative"),
                  QString::fromStdString(page.configuration.alternative));
    object.insert(QStringLiteral("correlation_method"),
                  QString::fromStdString(page.configuration.correlation_method));
    object.insert(QStringLiteral("variance_method"),
                  QString::fromStdString(page.configuration.variance_method));
    object.insert(QStringLiteral("gage_measurement_column"),
                  optional_size(page.configuration.gage_measurement_column));
    object.insert(QStringLiteral("gage_part_column"),
                  optional_size(page.configuration.gage_part_column));
    object.insert(QStringLiteral("gage_operator_column"),
                  optional_size(page.configuration.gage_operator_column));
    object.insert(QStringLiteral("ewma_lambda"), page.configuration.ewma_lambda);
    object.insert(QStringLiteral("ewma_limit_sigma"), page.configuration.ewma_limit_sigma);
    object.insert(QStringLiteral("cusum_target"), page.configuration.cusum_target);
    object.insert(QStringLiteral("cusum_sigma"), page.configuration.cusum_sigma);
    object.insert(QStringLiteral("cusum_k"), page.configuration.cusum_k);
    object.insert(QStringLiteral("cusum_h"), page.configuration.cusum_h);
    object.insert(QStringLiteral("cusum_fast_initial_response"),
                  page.configuration.cusum_fast_initial_response);
    object.insert(QStringLiteral("smoothing_alpha"), page.configuration.smoothing_alpha);
    object.insert(QStringLiteral("smoothing_gamma"), page.configuration.smoothing_gamma);
    object.insert(QStringLiteral("smoothing_method"),
                  QString::fromStdString(page.configuration.smoothing_method));
    object.insert(QStringLiteral("forecast_periods"), page.configuration.forecast_periods);
    object.insert(QStringLiteral("arima_time_column"),
                  optional_size(page.configuration.arima_time_column));
    object.insert(QStringLiteral("arima_value_column"),
                  optional_size(page.configuration.arima_value_column));
    object.insert(QStringLiteral("arima_differencing"), page.configuration.arima_differencing);
    object.insert(QStringLiteral("arima_selection_criterion"),
                  QString::fromStdString(page.configuration.arima_selection_criterion));
    object.insert(QStringLiteral("anova_response_column"),
                  optional_size(page.configuration.anova_response_column));
    object.insert(QStringLiteral("anova_factor_a_column"),
                  optional_size(page.configuration.anova_factor_a_column));
    object.insert(QStringLiteral("anova_factor_b_column"),
                  optional_size(page.configuration.anova_factor_b_column));
    object.insert(QStringLiteral("anova_factor_encoding"),
                  QString::fromStdString(page.configuration.anova_factor_encoding));
    object.insert(QStringLiteral("logistic_response_column"),
                  optional_size(page.configuration.logistic_response_column));
    object.insert(QStringLiteral("logistic_predictor_columns"),
                  size_array(page.configuration.logistic_predictor_columns));
    object.insert(QStringLiteral("logistic_event_level"),
                  QString::fromStdString(page.configuration.logistic_event_level));
    object.insert(QStringLiteral("logistic_link"),
                  QString::fromStdString(page.configuration.logistic_link));
    object.insert(QStringLiteral("logistic_max_iterations"),
                  page.configuration.logistic_max_iterations);
    object.insert(QStringLiteral("logistic_tolerance"),
                  page.configuration.logistic_tolerance);
    object.insert(QStringLiteral("variance_first_column"),
                  optional_size(page.configuration.variance_first_column));
    object.insert(QStringLiteral("variance_second_column"),
                  optional_size(page.configuration.variance_second_column));
    object.insert(QStringLiteral("hypothesized_variance"),
                  optional_number(page.configuration.hypothesized_variance));
    object.insert(QStringLiteral("variance_test_method"),
                  QString::fromStdString(page.configuration.variance_test_method));
    object.insert(QStringLiteral("variance_alternative"),
                  QString::fromStdString(page.configuration.variance_alternative));
    object.insert(QStringLiteral("decomposition_time_column"),
                  optional_size(page.configuration.decomposition_time_column));
    object.insert(QStringLiteral("decomposition_value_column"),
                  optional_size(page.configuration.decomposition_value_column));
    object.insert(QStringLiteral("decomposition_seasonal_period"),
                  page.configuration.decomposition_seasonal_period);
    object.insert(QStringLiteral("decomposition_model"),
                  QString::fromStdString(page.configuration.decomposition_model));
    object.insert(QStringLiteral("doe_factor_names"),
                  string_array(page.configuration.doe_factor_names));
    object.insert(QStringLiteral("doe_factor_columns"),
                  size_array(page.configuration.doe_factor_columns));
    object.insert(QStringLiteral("doe_response_column"),
                  optional_size(page.configuration.doe_response_column));
    object.insert(QStringLiteral("doe_low_levels"),
                  string_array(page.configuration.doe_low_levels));
    object.insert(QStringLiteral("doe_high_levels"),
                  string_array(page.configuration.doe_high_levels));
    object.insert(QStringLiteral("doe_center_point_count"),
                  static_cast<qint64>(page.configuration.doe_center_point_count));
    object.insert(QStringLiteral("doe_block_count"),
                  static_cast<qint64>(page.configuration.doe_block_count));
    object.insert(QStringLiteral("doe_randomize"), page.configuration.doe_randomize);
    object.insert(QStringLiteral("doe_random_seed"),
                  static_cast<qint64>(page.configuration.doe_random_seed));
    object.insert(QStringLiteral("nested_gage_measurement_column"),
                  optional_size(page.configuration.nested_gage_measurement_column));
    object.insert(QStringLiteral("nested_gage_part_column"),
                  optional_size(page.configuration.nested_gage_part_column));
    object.insert(QStringLiteral("nested_gage_operator_column"),
                  optional_size(page.configuration.nested_gage_operator_column));
    object.insert(QStringLiteral("gage_tolerance"), page.configuration.gage_tolerance);
    object.insert(QStringLiteral("msa_reference_column"),
                  optional_size(page.configuration.msa_reference_column));
    object.insert(QStringLiteral("msa_time_column"),
                  optional_size(page.configuration.msa_time_column));
    object.insert(QStringLiteral("msa_reference_value"),
                  optional_number(page.configuration.msa_reference_value));
    object.insert(QStringLiteral("msa_mode"),
                  QString::fromStdString(page.configuration.msa_mode));
    object.insert(QStringLiteral("reliability_time_column"),
                  optional_size(page.configuration.reliability_time_column));
    object.insert(QStringLiteral("reliability_event_column"),
                  optional_size(page.configuration.reliability_event_column));
    object.insert(QStringLiteral("reliability_group_column"),
                  optional_size(page.configuration.reliability_group_column));
    object.insert(QStringLiteral("reliability_model"),
                  QString::fromStdString(page.configuration.reliability_model));
    object.insert(QStringLiteral("power_effect_size"), page.configuration.power_effect_size);
    object.insert(QStringLiteral("power_target"), page.configuration.power_target);
    object.insert(QStringLiteral("power_alpha"), page.configuration.power_alpha);
    object.insert(QStringLiteral("power_sample_size"),
                  static_cast<qint64>(page.configuration.power_sample_size));
    object.insert(QStringLiteral("power_mode"),
                  QString::fromStdString(page.configuration.power_mode));
    object.insert(QStringLiteral("power_group_count"),
                  static_cast<qint64>(page.configuration.power_group_count));
    object.insert(QStringLiteral("power_null_proportion"),
                  page.configuration.power_null_proportion);
    object.insert(QStringLiteral("power_second_proportion"),
                  page.configuration.power_second_proportion);
    object.insert(QStringLiteral("power_variance_method"),
                  QString::fromStdString(page.configuration.power_variance_method));
    object.insert(QStringLiteral("attribute_rating_column"),
                  optional_size(page.configuration.attribute_rating_column));
    object.insert(QStringLiteral("attribute_part_column"),
                  optional_size(page.configuration.attribute_part_column));
    object.insert(QStringLiteral("attribute_appraiser_column"),
                  optional_size(page.configuration.attribute_appraiser_column));
    object.insert(QStringLiteral("attribute_standard_column"),
                  optional_size(page.configuration.attribute_standard_column));
    object.insert(QStringLiteral("attribute_agreement_method"),
                  QString::fromStdString(page.configuration.attribute_agreement_method));
    object.insert(QStringLiteral("seasonal_period"),
                  static_cast<qint64>(page.configuration.seasonal_period));
    object.insert(QStringLiteral("seasonal_error_model"),
                  QString::fromStdString(page.configuration.seasonal_error_model));
    object.insert(QStringLiteral("seasonal_trend_model"),
                  QString::fromStdString(page.configuration.seasonal_trend_model));
    object.insert(QStringLiteral("seasonal_damped_trend"),
                  page.configuration.seasonal_damped_trend);
    object.insert(QStringLiteral("seasonal_beta"), page.configuration.seasonal_beta);
    object.insert(QStringLiteral("seasonal_damping_phi"),
                  page.configuration.seasonal_damping_phi);
    object.insert(QStringLiteral("validation_initial_size"),
                  static_cast<qint64>(page.configuration.validation_initial_size));
    object.insert(QStringLiteral("validation_horizon"),
                  static_cast<qint64>(page.configuration.validation_horizon));
    object.insert(QStringLiteral("validation_step"),
                  static_cast<qint64>(page.configuration.validation_step));
    object.insert(QStringLiteral("pca_variable_columns"),
                  size_array(page.configuration.pca_variable_columns));
    object.insert(QStringLiteral("pca_mode"),
                  QString::fromStdString(page.configuration.pca_mode));
    object.insert(QStringLiteral("pca_component_count"),
                  static_cast<qint64>(page.configuration.pca_component_count));
    object.insert(QStringLiteral("pca_anomaly_quantile"),
                  page.configuration.pca_anomaly_quantile);
    QJsonArray tables;
    for (const auto& table : page.tables) {
        QJsonObject table_object;
        table_object.insert(QStringLiteral("title"), QString::fromStdString(table.title));
        table_object.insert(QStringLiteral("headers"), string_array(table.headers));
        QJsonArray rows;
        for (const auto& row : table.rows) {
            rows.append(string_array(row));
        }
        table_object.insert(QStringLiteral("rows"), rows);
        tables.append(table_object);
    }
    object.insert(QStringLiteral("tables"), tables);

    QJsonArray plots;
    for (const auto& plot : page.plots) {
        QJsonObject plot_object;
        plot_object.insert(QStringLiteral("kind"), static_cast<int>(plot.kind));
        plot_object.insert(QStringLiteral("title"), QString::fromStdString(plot.title));
        plot_object.insert(QStringLiteral("x_axis_title"), QString::fromStdString(plot.x_axis_title));
        plot_object.insert(QStringLiteral("y_axis_title"), QString::fromStdString(plot.y_axis_title));
        plot_object.insert(QStringLiteral("center_label"), QString::fromStdString(plot.center_label));
        plot_object.insert(QStringLiteral("subtitle"), QString::fromStdString(plot.subtitle));
        plot_object.insert(QStringLiteral("show_grid"), plot.show_grid);
        plot_object.insert(QStringLiteral("show_legend"), plot.show_legend);
        plot_object.insert(QStringLiteral("line_width"), plot.line_width);
        plot_object.insert(QStringLiteral("values"), number_array(plot.values));
        plot_object.insert(QStringLiteral("x_values"), number_array(plot.x_values));
        plot_object.insert(QStringLiteral("center"), number_array(plot.center));
        plot_object.insert(QStringLiteral("lower"), number_array(plot.lower));
        plot_object.insert(QStringLiteral("upper"), number_array(plot.upper));
        QJsonArray series;
        for (const domain::PlotSeries& item : plot.series) {
            QJsonObject series_object;
            series_object.insert(QStringLiteral("role"), static_cast<int>(item.role));
            series_object.insert(QStringLiteral("label"), QString::fromStdString(item.label));
            series_object.insert(QStringLiteral("values"), number_array(item.values));
            series_object.insert(QStringLiteral("x_values"), number_array(item.x_values));
            series_object.insert(QStringLiteral("lower"), number_array(item.lower));
            series_object.insert(QStringLiteral("upper"), number_array(item.upper));
            series_object.insert(QStringLiteral("line_width"), item.line_width);
            series_object.insert(QStringLiteral("show_points"), item.show_points);
            series.append(series_object);
        }
        plot_object.insert(QStringLiteral("series"), series);
        plot_object.insert(QStringLiteral("source_rows"), size_array(plot.source_rows));
        plot_object.insert(QStringLiteral("sigma_z"), plot.sigma_z);
        QJsonArray special_points;
        for (const auto& points : plot.special_cause_points) {
            special_points.append(size_array(points));
        }
        plot_object.insert(QStringLiteral("special_cause_points"), special_points);
        plot_object.insert(QStringLiteral("histogram_edges"), number_array(plot.histogram_edges));
        plot_object.insert(QStringLiteral("histogram_counts"), number_array(plot.histogram_counts));
        plot_object.insert(QStringLiteral("lsl"), optional_number(plot.lsl));
        plot_object.insert(QStringLiteral("usl"), optional_number(plot.usl));
        plot_object.insert(QStringLiteral("target"), optional_number(plot.target));
        plot_object.insert(QStringLiteral("process_mean"), optional_number(plot.process_mean));
        plot_object.insert(QStringLiteral("within_sigma"), optional_number(plot.within_sigma));
        plot_object.insert(QStringLiteral("overall_sigma"), optional_number(plot.overall_sigma));
        plot_object.insert(QStringLiteral("categories"), string_array(plot.categories));
        plot_object.insert(QStringLiteral("category_values"), number_array(plot.category_values));
        plot_object.insert(QStringLiteral("cumulative_percent"), number_array(plot.cumulative_percent));
        plot_object.insert(QStringLiteral("box_min"), number_array(plot.box_min));
        plot_object.insert(QStringLiteral("box_q1"), number_array(plot.box_q1));
        plot_object.insert(QStringLiteral("box_median"), number_array(plot.box_median));
        plot_object.insert(QStringLiteral("box_q3"), number_array(plot.box_q3));
        plot_object.insert(QStringLiteral("box_max"), number_array(plot.box_max));
        plot_object.insert(QStringLiteral("box_labels"), string_array(plot.box_labels));
        plots.append(plot_object);
    }
    object.insert(QStringLiteral("plots"), plots);
    QJsonArray diagnostics;
    for (const auto& diagnostic : page.diagnostics) {
        QJsonObject item;
        item.insert(QStringLiteral("severity"), static_cast<int>(diagnostic.severity));
        item.insert(QStringLiteral("code"), QString::fromStdString(diagnostic.code));
        item.insert(QStringLiteral("message"), QString::fromStdString(diagnostic.message));
        diagnostics.append(item);
    }
    object.insert(QStringLiteral("diagnostics"), diagnostics);
    QJsonArray interpretation;
    for (const auto& section : page.interpretation) {
        QJsonObject item;
        item.insert(QStringLiteral("heading"), QString::fromStdString(section.heading));
        item.insert(QStringLiteral("bullets"), string_array(section.bullets));
        item.insert(QStringLiteral("severity"), static_cast<int>(section.severity));
        interpretation.append(item);
    }
    object.insert(QStringLiteral("interpretation"), interpretation);
    return object;
}

domain::OutputPage output_page_from_json(const QJsonObject& object)
{
    domain::OutputPage page;
    page.id = object.value(QStringLiteral("id")).toString().toStdString();
    page.title = object.value(QStringLiteral("title")).toString().toStdString();
    page.method_name = object.value(QStringLiteral("method_name")).toString().toStdString();
    page.parameter_summary = object.value(QStringLiteral("parameter_summary")).toString().toStdString();
    page.configuration.chart_type = object.value(QStringLiteral("chart_type")).toString().toStdString();
    page.configuration.analysis_name = object.value(QStringLiteral("analysis_name")).toString().toStdString();
    const int schema_version = object.value(QStringLiteral("schema_version")).toInt(1);
    page.configuration.variable_columns =
        to_sizes(object.value(QStringLiteral("variable_columns")).toArray());
    page.configuration.excluded_rows =
        to_sizes(object.value(QStringLiteral("excluded_rows")).toArray());
    page.configuration.included_rows =
        to_sizes(object.value(QStringLiteral("included_rows")).toArray());
    page.configuration.stage_column =
        read_optional_size(object.value(QStringLiteral("stage_column")));
    page.configuration.selection.measurement_column = static_cast<std::size_t>(
        std::max<qint64>(0, object.value(QStringLiteral("measurement_column")).toInteger()));
    page.configuration.selection.subgroup_column =
        read_optional_size(object.value(QStringLiteral("subgroup_column")));
    page.configuration.selection.time_column =
        read_optional_size(object.value(QStringLiteral("time_column")));
    page.configuration.selection.product_column =
        read_optional_size(object.value(QStringLiteral("product_column")));
    page.configuration.selection.defect_count_column =
        read_optional_size(object.value(QStringLiteral("defect_count_column")));
    page.configuration.selection.inspected_count_column =
        read_optional_size(object.value(QStringLiteral("inspected_count_column")));
    page.configuration.inspected_constant =
        read_optional_size(object.value(QStringLiteral("inspected_constant")));
    page.configuration.first_events_column =
        read_optional_size(object.value(QStringLiteral("first_events_column")));
    page.configuration.first_trials_column =
        read_optional_size(object.value(QStringLiteral("first_trials_column")));
    page.configuration.second_events_column =
        read_optional_size(object.value(QStringLiteral("second_events_column")));
    page.configuration.second_trials_column =
        read_optional_size(object.value(QStringLiteral("second_trials_column")));
    page.configuration.row_category_column =
        read_optional_size(object.value(QStringLiteral("row_category_column")));
    page.configuration.column_category_column =
        read_optional_size(object.value(QStringLiteral("column_category_column")));
    page.configuration.specifications.lower =
        read_optional(object.value(QStringLiteral("lower_spec")));
    page.configuration.specifications.upper =
        read_optional(object.value(QStringLiteral("upper_spec")));
    page.configuration.specifications.target =
        read_optional(object.value(QStringLiteral("target_spec")));
    page.configuration.by_column =
        read_optional_size(object.value(QStringLiteral("by_column")));
    page.configuration.moving_range_length =
        object.value(QStringLiteral("moving_range_length")).toInt(2);
    page.configuration.sigma_method =
        object.value(QStringLiteral("sigma_method"))
            .toString("average_moving_range").toStdString();
    page.configuration.historical_center =
        read_optional(object.value(QStringLiteral("historical_center")));
    page.configuration.historical_sigma_z =
        read_optional(object.value(QStringLiteral("historical_sigma_z")));
    page.configuration.leave_gaps_for_excluded =
        object.value(QStringLiteral("leave_gaps_for_excluded")).toBool(false);
    page.configuration.enabled_special_cause_tests.clear();
    for (const QJsonValue& value : object.value(
             QStringLiteral("enabled_special_cause_tests")).toArray()) {
        page.configuration.enabled_special_cause_tests.push_back(value.toInt());
    }
    if (object.value(QStringLiteral("subgroup_size")).isDouble()) {
        page.configuration.subgroup_size =
            object.value(QStringLiteral("subgroup_size")).toInt();
    }
    page.configuration.pareto_other_threshold_percent =
        read_optional(object.value(QStringLiteral("pareto_other_threshold_percent")));
    page.configuration.hypothesis_mean =
        read_optional(object.value(QStringLiteral("hypothesis_mean")));
    page.configuration.confidence_level =
        object.value(QStringLiteral("confidence_level")).toDouble(0.95);
    page.configuration.alternative =
        object.value(QStringLiteral("alternative")).toString("two_sided").toStdString();
    page.configuration.correlation_method =
        object.value(QStringLiteral("correlation_method")).toString("pearson").toStdString();
    page.configuration.variance_method =
        object.value(QStringLiteral("variance_method")).toString("welch").toStdString();
    page.configuration.gage_measurement_column =
        read_optional_size(object.value(QStringLiteral("gage_measurement_column")));
    page.configuration.gage_part_column =
        read_optional_size(object.value(QStringLiteral("gage_part_column")));
    page.configuration.gage_operator_column =
        read_optional_size(object.value(QStringLiteral("gage_operator_column")));
    page.configuration.ewma_lambda = object.value(QStringLiteral("ewma_lambda")).toDouble(0.2);
    page.configuration.ewma_limit_sigma =
        object.value(QStringLiteral("ewma_limit_sigma")).toDouble(3.0);
    page.configuration.cusum_target = object.value(QStringLiteral("cusum_target")).toDouble(0.0);
    page.configuration.cusum_sigma = object.value(QStringLiteral("cusum_sigma")).toDouble(1.0);
    page.configuration.cusum_k = object.value(QStringLiteral("cusum_k")).toDouble(0.5);
    page.configuration.cusum_h = object.value(QStringLiteral("cusum_h")).toDouble(4.0);
    page.configuration.cusum_fast_initial_response =
        object.value(QStringLiteral("cusum_fast_initial_response")).toBool(false);
    page.configuration.smoothing_alpha =
        object.value(QStringLiteral("smoothing_alpha")).toDouble(0.2);
    page.configuration.smoothing_gamma =
        object.value(QStringLiteral("smoothing_gamma")).toDouble(0.2);
    page.configuration.smoothing_method =
        object.value(QStringLiteral("smoothing_method")).toString("double").toStdString();
    page.configuration.forecast_periods =
        object.value(QStringLiteral("forecast_periods")).toInt(1);
    page.configuration.arima_time_column =
        read_optional_size(object.value(QStringLiteral("arima_time_column")));
    page.configuration.arima_value_column =
        read_optional_size(object.value(QStringLiteral("arima_value_column")));
    page.configuration.arima_differencing =
        object.value(QStringLiteral("arima_differencing")).toInt(1);
    page.configuration.arima_selection_criterion =
        object.value(QStringLiteral("arima_selection_criterion")).toString("aicc").toStdString();
    page.configuration.anova_response_column =
        read_optional_size(object.value(QStringLiteral("anova_response_column")));
    page.configuration.anova_factor_a_column =
        read_optional_size(object.value(QStringLiteral("anova_factor_a_column")));
    page.configuration.anova_factor_b_column =
        read_optional_size(object.value(QStringLiteral("anova_factor_b_column")));
    page.configuration.anova_factor_encoding =
        object.value(QStringLiteral("anova_factor_encoding")).toString("reference").toStdString();
    page.configuration.logistic_response_column =
        read_optional_size(object.value(QStringLiteral("logistic_response_column")));
    page.configuration.logistic_predictor_columns =
        to_sizes(object.value(QStringLiteral("logistic_predictor_columns")).toArray());
    page.configuration.logistic_event_level =
        object.value(QStringLiteral("logistic_event_level")).toString("1").toStdString();
    page.configuration.logistic_link =
        object.value(QStringLiteral("logistic_link")).toString("logit").toStdString();
    page.configuration.logistic_max_iterations =
        object.value(QStringLiteral("logistic_max_iterations")).toInt(100);
    page.configuration.logistic_tolerance =
        object.value(QStringLiteral("logistic_tolerance")).toDouble(1.0e-8);
    page.configuration.variance_first_column =
        read_optional_size(object.value(QStringLiteral("variance_first_column")));
    page.configuration.variance_second_column =
        read_optional_size(object.value(QStringLiteral("variance_second_column")));
    page.configuration.hypothesized_variance =
        read_optional(object.value(QStringLiteral("hypothesized_variance")));
    page.configuration.variance_test_method =
        object.value(QStringLiteral("variance_test_method")).toString("f").toStdString();
    page.configuration.variance_alternative =
        object.value(QStringLiteral("variance_alternative"))
            .toString("two_sided").toStdString();
    page.configuration.decomposition_time_column =
        read_optional_size(object.value(QStringLiteral("decomposition_time_column")));
    page.configuration.decomposition_value_column =
        read_optional_size(object.value(QStringLiteral("decomposition_value_column")));
    page.configuration.decomposition_seasonal_period =
        object.value(QStringLiteral("decomposition_seasonal_period")).toInt(1);
    page.configuration.decomposition_model =
        object.value(QStringLiteral("decomposition_model"))
            .toString("additive").toStdString();
    page.configuration.doe_factor_names =
        to_strings(object.value(QStringLiteral("doe_factor_names")).toArray());
    page.configuration.doe_factor_columns =
        to_sizes(object.value(QStringLiteral("doe_factor_columns")).toArray());
    page.configuration.doe_response_column =
        read_optional_size(object.value(QStringLiteral("doe_response_column")));
    page.configuration.doe_low_levels =
        to_strings(object.value(QStringLiteral("doe_low_levels")).toArray());
    page.configuration.doe_high_levels =
        to_strings(object.value(QStringLiteral("doe_high_levels")).toArray());
    page.configuration.doe_center_point_count =
        static_cast<std::size_t>(object.value(QStringLiteral("doe_center_point_count")).toInteger());
    page.configuration.doe_block_count =
        static_cast<std::size_t>(object.value(QStringLiteral("doe_block_count")).toInteger(1));
    page.configuration.doe_randomize =
        object.value(QStringLiteral("doe_randomize")).toBool(false);
    page.configuration.doe_random_seed =
        static_cast<std::uint64_t>(object.value(QStringLiteral("doe_random_seed")).toInteger());
    page.configuration.nested_gage_measurement_column =
        read_optional_size(object.value(QStringLiteral("nested_gage_measurement_column")));
    page.configuration.nested_gage_part_column =
        read_optional_size(object.value(QStringLiteral("nested_gage_part_column")));
    page.configuration.nested_gage_operator_column =
        read_optional_size(object.value(QStringLiteral("nested_gage_operator_column")));
    page.configuration.gage_tolerance =
        object.value(QStringLiteral("gage_tolerance")).toDouble(0.0);
    page.configuration.msa_reference_column =
        read_optional_size(object.value(QStringLiteral("msa_reference_column")));
    page.configuration.msa_time_column =
        read_optional_size(object.value(QStringLiteral("msa_time_column")));
    page.configuration.msa_reference_value =
        read_optional(object.value(QStringLiteral("msa_reference_value")));
    page.configuration.msa_mode =
        object.value(QStringLiteral("msa_mode")).toString(QStringLiteral("type1")).toStdString();
    page.configuration.reliability_time_column =
        read_optional_size(object.value(QStringLiteral("reliability_time_column")));
    page.configuration.reliability_event_column =
        read_optional_size(object.value(QStringLiteral("reliability_event_column")));
    page.configuration.reliability_group_column =
        read_optional_size(object.value(QStringLiteral("reliability_group_column")));
    page.configuration.reliability_model =
        object.value(QStringLiteral("reliability_model"))
            .toString(QStringLiteral("kaplan_meier")).toStdString();
    page.configuration.power_effect_size =
        object.value(QStringLiteral("power_effect_size")).toDouble(0.5);
    page.configuration.power_target =
        object.value(QStringLiteral("power_target")).toDouble(0.8);
    page.configuration.power_alpha =
        object.value(QStringLiteral("power_alpha")).toDouble(0.05);
    page.configuration.power_sample_size =
        static_cast<std::size_t>(object.value(QStringLiteral("power_sample_size")).toInteger());
    page.configuration.power_mode =
        object.value(QStringLiteral("power_mode"))
            .toString(QStringLiteral("one_sample_sample_size")).toStdString();
    page.configuration.power_group_count =
        static_cast<std::size_t>(object.value(QStringLiteral("power_group_count")).toInteger(3));
    page.configuration.power_null_proportion =
        object.value(QStringLiteral("power_null_proportion")).toDouble(0.5);
    page.configuration.power_second_proportion =
        object.value(QStringLiteral("power_second_proportion")).toDouble(0.7);
    page.configuration.power_variance_method =
        object.value(QStringLiteral("power_variance_method"))
            .toString(QStringLiteral("pooled")).toStdString();
    page.configuration.attribute_rating_column =
        read_optional_size(object.value(QStringLiteral("attribute_rating_column")));
    page.configuration.attribute_part_column =
        read_optional_size(object.value(QStringLiteral("attribute_part_column")));
    page.configuration.attribute_appraiser_column =
        read_optional_size(object.value(QStringLiteral("attribute_appraiser_column")));
    page.configuration.attribute_standard_column =
        read_optional_size(object.value(QStringLiteral("attribute_standard_column")));
    page.configuration.attribute_agreement_method =
        object.value(QStringLiteral("attribute_agreement_method"))
            .toString("kappa").toStdString();
    page.configuration.seasonal_period =
        static_cast<std::size_t>(object.value(QStringLiteral("seasonal_period")).toInteger(1));
    page.configuration.seasonal_error_model =
        object.value(QStringLiteral("seasonal_error_model"))
            .toString("additive").toStdString();
    page.configuration.seasonal_trend_model =
        object.value(QStringLiteral("seasonal_trend_model"))
            .toString("additive").toStdString();
    page.configuration.seasonal_damped_trend =
        object.value(QStringLiteral("seasonal_damped_trend")).toBool(false);
    page.configuration.seasonal_beta =
        object.value(QStringLiteral("seasonal_beta")).toDouble(0.1);
    page.configuration.seasonal_damping_phi =
        object.value(QStringLiteral("seasonal_damping_phi")).toDouble(0.98);
    page.configuration.validation_initial_size =
        static_cast<std::size_t>(object.value(QStringLiteral("validation_initial_size"))
                                      .toInteger());
    page.configuration.validation_horizon =
        static_cast<std::size_t>(object.value(QStringLiteral("validation_horizon"))
                                      .toInteger(1));
    page.configuration.validation_step =
        static_cast<std::size_t>(object.value(QStringLiteral("validation_step"))
                                      .toInteger(1));
    page.configuration.pca_variable_columns =
        to_sizes(object.value(QStringLiteral("pca_variable_columns")).toArray());
    page.configuration.pca_mode =
        object.value(QStringLiteral("pca_mode")).toString("covariance").toStdString();
    page.configuration.pca_component_count =
        static_cast<std::size_t>(object.value(QStringLiteral("pca_component_count"))
                                      .toInteger());
    page.configuration.pca_anomaly_quantile =
        object.value(QStringLiteral("pca_anomaly_quantile")).toDouble(0.99);
    if (schema_version < 2) {
        page.configuration.selection.measurement_column = 0;
    }
    for (const QJsonValue& table_value : object.value(QStringLiteral("tables")).toArray()) {
        const QJsonObject table_object = table_value.toObject();
        domain::StatisticTable table;
        table.title = table_object.value(QStringLiteral("title")).toString().toStdString();
        table.headers = to_strings(table_object.value(QStringLiteral("headers")).toArray());
        for (const QJsonValue& row : table_object.value(QStringLiteral("rows")).toArray()) {
            table.rows.push_back(to_strings(row.toArray()));
        }
        page.tables.push_back(table);
    }
    for (const QJsonValue& plot_value : object.value(QStringLiteral("plots")).toArray()) {
        const QJsonObject plot_object = plot_value.toObject();
        domain::PlotSpec plot;
        plot.kind = static_cast<domain::PlotKind>(plot_object.value(QStringLiteral("kind")).toInt());
        plot.title = plot_object.value(QStringLiteral("title")).toString().toStdString();
        plot.x_axis_title = plot_object.value(QStringLiteral("x_axis_title")).toString().toStdString();
        plot.y_axis_title = plot_object.value(QStringLiteral("y_axis_title")).toString().toStdString();
        plot.center_label = plot_object.value(QStringLiteral("center_label")).toString().toStdString();
        plot.subtitle = plot_object.value(QStringLiteral("subtitle")).toString().toStdString();
        plot.show_grid = plot_object.value(QStringLiteral("show_grid")).toBool(true);
        plot.show_legend = plot_object.value(QStringLiteral("show_legend")).toBool(true);
        plot.line_width = plot_object.value(QStringLiteral("line_width")).toDouble(1.8);
        plot.values = to_numbers(plot_object.value(QStringLiteral("values")).toArray());
        plot.x_values = to_numbers(plot_object.value(QStringLiteral("x_values")).toArray());
        plot.center = to_numbers(plot_object.value(QStringLiteral("center")).toArray());
        plot.lower = to_numbers(plot_object.value(QStringLiteral("lower")).toArray());
        plot.upper = to_numbers(plot_object.value(QStringLiteral("upper")).toArray());
        for (const QJsonValue& series_value :
             plot_object.value(QStringLiteral("series")).toArray()) {
            const QJsonObject series_object = series_value.toObject();
            domain::PlotSeries series;
            series.role = static_cast<domain::PlotSeriesRole>(
                series_object.value(QStringLiteral("role")).toInt());
            series.label = series_object.value(QStringLiteral("label")).toString().toStdString();
            series.values = to_numbers(series_object.value(QStringLiteral("values")).toArray());
            series.x_values = to_numbers(series_object.value(QStringLiteral("x_values")).toArray());
            series.lower = to_numbers(series_object.value(QStringLiteral("lower")).toArray());
            series.upper = to_numbers(series_object.value(QStringLiteral("upper")).toArray());
            series.line_width = series_object.value(QStringLiteral("line_width")).toDouble(1.8);
            series.show_points = series_object.value(QStringLiteral("show_points")).toBool(false);
            plot.series.push_back(std::move(series));
        }
        plot.source_rows = to_sizes(plot_object.value(QStringLiteral("source_rows")).toArray());
        plot.sigma_z = plot_object.value(QStringLiteral("sigma_z")).toDouble();
        for (const QJsonValue& points : plot_object.value(
                 QStringLiteral("special_cause_points")).toArray()) {
            plot.special_cause_points.push_back(to_sizes(points.toArray()));
        }
        plot.histogram_edges = to_numbers(plot_object.value(QStringLiteral("histogram_edges")).toArray());
        plot.histogram_counts = to_numbers(plot_object.value(QStringLiteral("histogram_counts")).toArray());
        plot.lsl = read_optional(plot_object.value(QStringLiteral("lsl")));
        plot.usl = read_optional(plot_object.value(QStringLiteral("usl")));
        plot.target = read_optional(plot_object.value(QStringLiteral("target")));
        plot.process_mean = read_optional(plot_object.value(QStringLiteral("process_mean")));
        plot.within_sigma = read_optional(plot_object.value(QStringLiteral("within_sigma")));
        plot.overall_sigma = read_optional(plot_object.value(QStringLiteral("overall_sigma")));
        plot.categories = to_strings(plot_object.value(QStringLiteral("categories")).toArray());
        plot.category_values = to_numbers(plot_object.value(QStringLiteral("category_values")).toArray());
        plot.cumulative_percent = to_numbers(plot_object.value(QStringLiteral("cumulative_percent")).toArray());
        plot.box_min = to_numbers(plot_object.value(QStringLiteral("box_min")).toArray());
        plot.box_q1 = to_numbers(plot_object.value(QStringLiteral("box_q1")).toArray());
        plot.box_median = to_numbers(plot_object.value(QStringLiteral("box_median")).toArray());
        plot.box_q3 = to_numbers(plot_object.value(QStringLiteral("box_q3")).toArray());
        plot.box_max = to_numbers(plot_object.value(QStringLiteral("box_max")).toArray());
        plot.box_labels = to_strings(plot_object.value(QStringLiteral("box_labels")).toArray());
        page.plots.push_back(plot);
    }
    for (const QJsonValue& value : object.value(QStringLiteral("diagnostics")).toArray()) {
        const QJsonObject item = value.toObject();
        domain::DiagnosticMessage diagnostic;
        diagnostic.severity = static_cast<domain::DiagnosticMessage::Severity>(
            item.value(QStringLiteral("severity")).toInt());
        diagnostic.code = item.value(QStringLiteral("code")).toString().toStdString();
        diagnostic.message = item.value(QStringLiteral("message")).toString().toStdString();
        page.diagnostics.push_back(diagnostic);
    }
    for (const QJsonValue& value : object.value(QStringLiteral("interpretation")).toArray()) {
        const QJsonObject item = value.toObject();
        domain::InterpretationSection section;
        section.heading = item.value(QStringLiteral("heading")).toString().toStdString();
        section.bullets = to_strings(item.value(QStringLiteral("bullets")).toArray());
        section.severity = static_cast<domain::DiagnosticMessage::Severity>(
            item.value(QStringLiteral("severity")).toInt());
        page.interpretation.push_back(section);
    }
    return page;
}

}  // namespace datalab::infrastructure
