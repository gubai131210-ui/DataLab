#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain {

using RowId = std::uint64_t;

enum class ColumnType {
    unknown,
    numeric,
    categorical,
    time
};

enum class CellState {
    valid,
    missing,
    invalid
};

struct ImportMetadata {
    int schema_version = 1;
    std::string sheet_name;
    std::size_t sheet_index = 0;
    std::size_t original_row_count = 0;
    std::vector<std::string> warnings;
};

struct DataTable {
    std::string name;
    std::string source_path;
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> import_warnings;
    ImportMetadata import_metadata;
    std::vector<RowId> row_ids;
    std::vector<ColumnType> column_types;
    std::vector<std::vector<CellState>> cell_states;
};

struct DataSelection {
    std::size_t measurement_column = 0;
    std::optional<std::size_t> subgroup_column;
    std::optional<std::size_t> time_column;
    std::optional<std::size_t> product_column;
    std::optional<std::size_t> defect_count_column;
    std::optional<std::size_t> inspected_count_column;
};

struct SpecificationLimits {
    std::optional<double> lower;
    std::optional<double> upper;
    std::optional<double> target;
};

struct CleaningOperation {
    std::string operation;
    std::string reason;
    std::vector<std::size_t> affected_rows;
};

struct InferenceConfiguration {
    double confidence_level = 0.95;
    std::string alternative = "two_sided";
    std::string correlation_method = "pearson";
    std::string variance_method = "welch";
    std::optional<double> hypothesis_mean;
    std::optional<std::size_t> first_events_column;
    std::optional<std::size_t> first_trials_column;
    std::optional<std::size_t> second_events_column;
    std::optional<std::size_t> second_trials_column;
    std::optional<std::size_t> row_category_column;
    std::optional<std::size_t> column_category_column;
    std::optional<std::size_t> anova_response_column;
    std::optional<std::size_t> anova_factor_a_column;
    std::optional<std::size_t> anova_factor_b_column;
    std::string anova_factor_encoding = "reference";
    std::optional<std::size_t> logistic_response_column;
    std::vector<std::size_t> logistic_predictor_columns;
    std::string logistic_event_level = "1";
    std::string logistic_link = "logit";
    int logistic_max_iterations = 100;
    double logistic_tolerance = 1.0e-8;
    std::optional<std::size_t> variance_first_column;
    std::optional<std::size_t> variance_second_column;
    std::optional<double> hypothesized_variance;
    std::string variance_test_method = "f";
    std::string variance_alternative = "two_sided";
};

struct ControlConfiguration {
    std::optional<int> subgroup_size;
    int moving_range_length = 2;
    std::string sigma_method = "average_moving_range";
    std::vector<int> enabled_special_cause_tests;
    std::string special_cause_rule_policy = "default_all_applicable";
    std::optional<std::size_t> stage_column;
    std::optional<double> historical_center;
    std::optional<double> historical_sigma_z;
    double ewma_lambda = 0.2;
    double ewma_limit_sigma = 3.0;
    double cusum_target = 0.0;
    double cusum_sigma = 1.0;
    double cusum_k = 0.5;
    double cusum_h = 4.0;
    bool cusum_fast_initial_response = false;
};

struct TimeSeriesConfiguration {
    double smoothing_alpha = 0.2;
    double smoothing_gamma = 0.2;
    std::string smoothing_method = "double";
    int forecast_periods = 1;
    std::optional<std::size_t> arima_time_column;
    std::optional<std::size_t> arima_value_column;
    int arima_differencing = 1;
    std::string arima_selection_criterion = "aicc";
    std::optional<std::size_t> decomposition_time_column;
    std::optional<std::size_t> decomposition_value_column;
    int decomposition_seasonal_period = 1;
    std::string decomposition_model = "additive";
    std::size_t seasonal_period = 1;
    std::string seasonal_error_model = "additive";
    std::string seasonal_trend_model = "additive";
    bool seasonal_damped_trend = false;
    double seasonal_beta = 0.1;
    double seasonal_damping_phi = 0.98;
    std::size_t validation_initial_size = 0;
    std::size_t validation_horizon = 1;
    std::size_t validation_step = 1;
};

struct DoeConfiguration {
    std::vector<std::string> factor_names;
    std::vector<std::size_t> factor_columns;
    std::optional<std::size_t> response_column;
    std::vector<std::string> low_levels;
    std::vector<std::string> high_levels;
    std::size_t center_point_count = 0;
    std::size_t block_count = 1;
    bool randomize = false;
    std::uint64_t random_seed = 0;
};

struct MsaConfiguration {
    std::optional<std::size_t> gage_measurement_column;
    std::optional<std::size_t> gage_part_column;
    std::optional<std::size_t> gage_operator_column;
    double gage_tolerance = 0.0;
    std::optional<std::size_t> nested_measurement_column;
    std::optional<std::size_t> nested_part_column;
    std::optional<std::size_t> nested_operator_column;
    std::optional<std::size_t> attribute_rating_column;
    std::optional<std::size_t> attribute_part_column;
    std::optional<std::size_t> attribute_appraiser_column;
    std::optional<std::size_t> attribute_standard_column;
    std::string attribute_agreement_method = "kappa";
    std::optional<std::size_t> reference_column;
    std::optional<std::size_t> time_column;
    std::optional<double> reference_value;
    std::string mode = "type1";
};

struct ReliabilityConfiguration {
    std::optional<std::size_t> time_column;
    std::optional<std::size_t> event_column;
    std::optional<std::size_t> group_column;
    std::string model = "kaplan_meier";
};

struct PowerConfiguration {
    double effect_size = 0.5;
    double target = 0.8;
    double alpha = 0.05;
    std::size_t sample_size = 0;
    std::string mode = "one_sample_sample_size";
    std::size_t group_count = 3;
    double null_proportion = 0.5;
    double second_proportion = 0.7;
    std::string variance_method = "pooled";
};

struct PcaConfiguration {
    std::vector<std::size_t> variable_columns;
    std::string mode = "covariance";
    std::size_t component_count = 0;
    double anomaly_quantile = 0.99;
};

struct GraphConfiguration {
    std::string graph_kind;
    std::optional<std::size_t> x_column;
    std::optional<std::size_t> y_column;
    std::optional<std::size_t> size_column;
    std::optional<std::size_t> by_column;
    std::optional<std::size_t> label_column;
    std::vector<std::size_t> variable_columns;
    std::string correlation_method = "pearson";
    double confidence_level = 0.95;
    std::string interval_type = "mean_t";
    bool show_p_values = false;
    std::optional<std::size_t> z_column;
    std::optional<std::size_t> time_column;
    std::optional<std::size_t> weight_column;
    int bin_count = 0;
    double other_threshold_percent = 5.0;
    bool show_normal_reference = false;
    bool connect_missing = true;
    std::string stack_mode = "none";
    std::string color_scale = "auto";
    int contour_levels = 8;
    int payload_version = 1;
};

struct AnalysisConfiguration {
    std::string analysis_name;
    std::string chart_type;
    DataSelection selection;
    SpecificationLimits specifications;
    std::vector<std::size_t> excluded_rows;
    std::vector<std::size_t> variable_columns;
    std::optional<std::size_t> by_column;
    std::optional<double> pareto_other_threshold_percent;
    std::optional<std::size_t> inspected_constant;
    InferenceConfiguration inference;
    ControlConfiguration control;
    TimeSeriesConfiguration time_series;
    DoeConfiguration doe;
    MsaConfiguration msa;
    ReliabilityConfiguration reliability;
    PowerConfiguration power;
    PcaConfiguration pca;
    GraphConfiguration graph;
    std::vector<std::size_t> included_rows;
    bool leave_gaps_for_excluded = false;
};

struct DiagnosticMessage {
    enum class Severity {
        info,
        warning,
        error
    };

    Severity severity = Severity::info;
    std::string code;
    std::string message;
    std::vector<RowId> related_rows;
    std::vector<std::size_t> related_columns;
    std::string related_plot_id;
    std::string suggested_action;
};

enum class PlotKind {
    control,
    histogram,
    boxplot,
    pareto,
    probability,
    scatter,
    interval,
    correlation,
    bubble,
    ecdf,
    matrix,
    marginal,
    parallel,
    heatmap,
    time_series,
    area,
    contour,
    pie
};

enum class PlotSeriesRole {
    generic,
    actual,
    fitted,
    forecast,
    interaction_first,
    interaction_second,
    confidence_band,
    trend,
    seasonal,
    remainder,
    component
};

enum class PlotLineStyle {
    solid,
    dash,
    dot,
    dash_dot
};

enum class PlotPointStyle {
    none,
    circle,
    square,
    triangle,
    cross
};

struct PlotSeriesStyle {
    bool visible = true;
    std::string color = "#455a64";
    std::string fill_color;
    PlotLineStyle line_style = PlotLineStyle::solid;
    PlotPointStyle point_style = PlotPointStyle::none;
    double line_width = 1.8;
    double point_size = 3.5;
    double opacity = 1.0;
};

struct PlotReferenceStyle {
    bool visible = true;
    std::string label;
    std::string color = "#455a64";
    PlotLineStyle line_style = PlotLineStyle::dash;
    double line_width = 1.2;
};

struct PlotSeries {
    PlotSeriesRole role = PlotSeriesRole::generic;
    std::string label;
    std::vector<double> values;
    std::vector<double> x_values;
    std::vector<double> lower;
    std::vector<double> upper;
    double line_width = 1.8;
    bool show_points = false;
    PlotSeriesStyle style;
};

struct StatisticTable {
    std::string title;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
};

struct PlotSpec {
    PlotKind kind = PlotKind::control;
    std::string title;
    std::string x_axis_title;
    std::string y_axis_title;
    std::string center_label = "CL";
    std::string subtitle;
    bool show_grid = true;
    bool show_legend = true;
    double line_width = 1.8;
    int legend_font_size = 8;
    std::string grid_color = "#e3e7eb";
    PlotSeriesStyle value_style{
        true, "#1565c0", {}, PlotLineStyle::solid,
        PlotPointStyle::circle, 1.8, 3.5, 1.0};
    PlotReferenceStyle center_style{
        true, "CL", "#2e7d32", PlotLineStyle::dash, 1.2};
    PlotReferenceStyle lower_style{
        true, "LCL", "#d32f2f", PlotLineStyle::dash, 1.0};
    PlotReferenceStyle upper_style{
        true, "UCL", "#d32f2f", PlotLineStyle::dash, 1.0};
    std::vector<double> values;
    std::vector<double> x_values;
    std::vector<double> center;
    std::vector<double> lower;
    std::vector<double> upper;
    std::vector<PlotSeries> series;
    std::vector<std::size_t> source_rows;
    std::vector<std::vector<std::size_t>> special_cause_points;
    std::vector<std::vector<int>> triggered_tests;
    std::vector<int> primary_test_by_point;
    std::vector<int> signal_direction;
    double sigma_z = 0.0;
    std::vector<double> histogram_edges;
    std::vector<double> histogram_counts;
    std::optional<double> lsl;
    std::optional<double> usl;
    std::optional<double> target;
    std::optional<double> process_mean;
    std::optional<double> within_sigma;
    std::optional<double> overall_sigma;
    std::vector<std::string> categories;
    std::vector<double> category_values;
    std::vector<double> cumulative_percent;
    std::vector<double> box_min;
    std::vector<double> box_q1;
    std::vector<double> box_median;
    std::vector<double> box_q3;
    std::vector<double> box_max;
    std::vector<std::string> box_labels;
    std::vector<double> interval_lower;
    std::vector<double> interval_upper;
    std::vector<std::size_t> interval_counts;
    std::vector<std::string> point_labels;
    std::vector<std::string> point_groups;
    std::vector<double> bubble_sizes;
    std::vector<std::string> matrix_labels;
    std::vector<std::vector<double>> matrix_values;
    std::vector<std::vector<std::size_t>> matrix_counts;
    std::vector<std::vector<double>> matrix_p_values;
    std::vector<double> histogram_edges_y;
    std::vector<double> histogram_counts_y;
    std::vector<double> contour_x;
    std::vector<double> contour_y;
    std::vector<double> contour_levels;
    std::optional<double> color_min;
    std::optional<double> color_max;
};

struct InterpretationSection {
    std::string heading;
    std::vector<std::string> bullets;
    DiagnosticMessage::Severity severity = DiagnosticMessage::Severity::info;
};

struct CapabilityFacts {
    std::optional<double> cpk;
    std::optional<double> ppk;
    std::optional<double> cpm;
    std::optional<double> z_bench;
    std::string assumption_status = "not_verified";
    std::string specification_mode;
};

struct RegressionFacts {
    std::optional<double> r_squared;
    std::optional<double> residual_normality_p;
    std::size_t influential_count = 0;
    std::string assumption_status = "not_verified";
};

struct SpcFacts {
    std::optional<std::size_t> out_of_control_count;
    std::optional<double> sigma_z;
};

struct DoeFacts {
    std::vector<std::string> significant_terms;
    bool has_p_value = false;
};

struct MsaFacts {
    std::optional<double> slope;
    std::optional<double> bias_low;
    std::optional<double> bias_high;
    std::optional<double> p_value;
    std::optional<double> cgk;
    std::optional<double> tolerance_percent;
};

struct ReliabilityFacts {
    std::optional<double> shape;
    std::optional<std::size_t> censored_count;
};

struct ForecastFacts {
    std::optional<double> mape;
    std::optional<double> mase;
};

struct PowerFacts {
    std::optional<double> power;
    std::optional<double> effect_size;
};

struct ParetoFacts {
    std::string largest_category;
    std::optional<double> largest_count;
    std::optional<double> largest_percent;
    std::optional<double> top_categories_percent;
};

struct QualityEvidence {
    std::string method_version = "1";
    std::size_t valid_count = 0;
    std::size_t missing_count = 0;
    std::size_t omitted_count = 0;
    std::vector<std::size_t> source_rows;
    std::string parameter_source = "estimated";
    std::string assumption_status = "not_verified";
    std::string not_computed_reason;
    std::optional<double> alpha;
    std::optional<double> confidence_level;
    std::optional<double> degrees_of_freedom;
};

struct MethodMetadata {
    std::string algorithm;
    std::string version = "1";
    std::string parameters;
    std::string missing_policy = "skip_missing";
    std::string estimation_method;
    std::vector<RowId> source_rows;
    std::vector<std::string> diagnostic_codes;
    std::string assumption_status = "not_verified";
    std::string parameter_source;
    std::size_t valid_count = 0;
    std::size_t missing_count = 0;
    std::string not_computed_reason;
};

struct InterpretationFacts {
    std::optional<CapabilityFacts> capability;
    std::optional<SpcFacts> spc;
    std::optional<DoeFacts> doe;
    std::optional<MsaFacts> msa;
    std::optional<ReliabilityFacts> reliability;
    std::optional<ForecastFacts> forecast;
    std::optional<PowerFacts> power;
    std::optional<ParetoFacts> pareto;
    std::optional<RegressionFacts> regression;
};

struct OutputPage {
    std::string id;
    std::string title;
    std::string method_name;
    std::string parameter_summary;
    AnalysisConfiguration configuration;
    std::vector<StatisticTable> tables;
    std::vector<PlotSpec> plots;
    std::vector<DiagnosticMessage> diagnostics;
    std::vector<InterpretationSection> interpretation;
    InterpretationFacts facts;
    MethodMetadata method_metadata;
};

}  // namespace datalab::domain
