#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
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
    std::size_t column_count = 0;
    std::string dataset_id;
    std::string imported_at;
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
    std::optional<std::size_t> variance_group_column;
    std::optional<double> hypothesized_variance;
    std::string variance_test_method = "f";
    std::string variance_alternative = "two_sided";
    std::optional<double> coverage_proportion;
    std::string proportion_method = "exact";
    std::string expected_proportions;
    std::optional<std::size_t> gof_category_column;
    std::optional<double> equivalence_lower;
    std::optional<double> equivalence_upper;
    std::string equivalence_ratio_transform = "none";
    std::string nonparametric_posthoc = "dunn";
    std::string rate_comparison = "difference";
};

struct ControlConfiguration {
    std::optional<int> subgroup_size;
    int moving_range_length = 2;
    std::string sigma_method = "average_moving_range";
    std::vector<int> enabled_special_cause_tests;
    std::string special_cause_rule_policy = "default_all_applicable";
    std::optional<std::size_t> stage_column;
    std::optional<double> historical_center;
    std::optional<double> historical_sigma;
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

struct DoeResponseObjectiveConfig {
    std::string goal = "maximize";
    std::optional<double> lower;
    std::optional<double> upper;
    std::optional<double> target;
    double weight = 1.0;
};

struct DoeConfiguration {
    std::vector<std::string> factor_names;
    std::vector<std::size_t> factor_columns;
    std::optional<std::size_t> response_column;
    std::vector<std::size_t> response_columns;
    std::vector<std::string> low_levels;
    std::vector<std::string> high_levels;
    std::size_t center_point_count = 0;
    std::size_t block_count = 1;
    bool randomize = false;
    std::uint64_t random_seed = 0;
    std::string optimization_goal = "maximize";
    std::optional<double> optimization_lower;
    std::optional<double> optimization_upper;
    std::optional<double> optimization_target;
    double optimization_weight = 1.0;
    double optimization_confidence = 0.95;
    std::vector<DoeResponseObjectiveConfig> optimization_objectives;
    std::string contour_x_factor;
    std::string contour_y_factor;
    // Actual-unit hold for non-axis factors: name -> actual text (empty map = coded 0).
    std::map<std::string, std::string> contour_hold_actual;
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
    std::string kappa_weight_scheme = "none";
    bool ratings_are_ordinal = false;
    std::optional<std::size_t> reference_column;
    std::optional<std::size_t> time_column;
    std::optional<double> reference_value;
    std::optional<double> process_variation;
    std::string mode = "type1";
};

struct ReliabilityConfiguration {
    std::optional<std::size_t> time_column;
    std::optional<std::size_t> event_column;
    std::optional<std::size_t> group_column;
    std::string model = "kaplan_meier";
    std::vector<double> percentile_levels = {10.0, 50.0, 90.0};
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
    double observation_length = 1.0;
    std::string variance_method = "pooled";
    std::string sample_size_list;
    std::string effect_size_list;
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
    std::string capability_method = "normal";
    std::string nonnormal_distribution = "weibull";
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
    pie,
    surface
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
    int schema_version = 1;
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
    int title_font_size = 11;
    int axis_font_size = 9;
    std::string theme_preset = "default";
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
    std::optional<double> y_min;
    std::optional<double> y_max;
    std::optional<double> x_min;
    std::optional<double> x_max;
    std::string data_region_fill;
};

struct InterpretationSection {
    std::string heading;
    std::vector<std::string> bullets;
    DiagnosticMessage::Severity severity = DiagnosticMessage::Severity::info;
};

struct AssumptionCheck {
    std::string name;
    std::string status = "not_verified";
    std::optional<double> statistic;
    std::optional<double> p_value;
    std::string evidence_summary;
};

struct RuleEvidence {
    std::string id;
    std::string status = "not_applicable";
    std::string message;
    std::vector<RowId> related_rows;
    std::string suggested_action;
};

struct CapabilityFacts {
    std::optional<double> cpk;
    std::optional<double> ppk;
    std::optional<double> cpm;
    std::optional<double> z_bench;
    std::string assumption_status = "not_verified";
    std::string specification_mode;
    std::string method;
    std::string johnson_family;
    std::optional<double> normality_p_value;
    std::optional<double> transform_p_value;
    std::optional<double> transform_anderson_darling;
    std::string nonnormal_distribution;
    std::optional<double> fitted_shape;
    std::optional<double> fitted_scale;
    std::optional<double> average_p;
    std::optional<double> percent_defective;
    std::optional<double> ppm_defective;
    std::optional<double> process_z;
    std::optional<double> mean_dpu;
    std::optional<double> cp;
    std::optional<double> pp;
    std::optional<double> cpk_lower;
    std::optional<double> cpk_upper;
    std::optional<double> ppk_lower;
    std::optional<double> ppk_upper;
    std::string capability_ci_method;
};

struct RegressionFacts {
    std::optional<double> r_squared;
    std::optional<double> residual_normality_p;
    std::optional<double> residual_anderson_darling;
    std::size_t residual_plot_count = 0;
    std::size_t influential_count = 0;
    std::string assumption_status = "not_verified";
    std::size_t outlier_count = 0;
    std::size_t high_leverage_count = 0;
    std::optional<double> max_vif;
    std::optional<double> durbin_watson;
    std::optional<double> error_degrees_of_freedom;
    bool rank_deficient = false;
    std::vector<AssumptionCheck> assumptions;
    std::vector<RuleEvidence> rules;
};

struct AnovaFacts {
    std::optional<double> p_value;
    std::size_t error_degrees_of_freedom = 0;
    bool estimable = true;
    std::size_t not_estimable_term_count = 0;
    std::vector<std::string> significant_terms;
    std::optional<double> family_confidence_level;
    std::size_t tukey_significant_pairs = 0;
    std::string tukey_method;
    std::string tukey_interval_columns;  // "lower_upper" when split columns are used
    bool tukey_grouping_available = false;
    std::size_t grouping_letter_count = 0;
    std::string assumption_status = "not_verified";
    std::vector<AssumptionCheck> assumptions;
    std::vector<RuleEvidence> rules;
};

struct SpcFacts {
    std::optional<std::size_t> out_of_control_count;
    std::optional<double> sigma_z;
    std::optional<double> sigma_within;
    std::optional<double> sigma_between;
    std::optional<double> sigma_between_within;
    std::string between_within_method;
};

struct DoeFacts {
    std::vector<std::string> significant_terms;
    bool has_p_value = false;
    std::size_t response_count = 0;
    bool multi_response = false;
    std::optional<double> best_overall_desirability;
    std::vector<std::string> response_names;
    bool prediction_interval_available = false;
    std::string largest_standardized_effect_term;
    std::optional<double> pareto_reference;
    std::string pareto_method;
    std::size_t residual_count = 0;
    std::size_t factor_count = 0;
    bool cube_plot_available = false;
    bool contour_plot_available = false;
    std::string contour_x_factor;
    std::string contour_y_factor;
    std::vector<std::string> held_factor_names;
    std::vector<std::string> held_actual_values;
    std::vector<double> held_coded_values;
};

struct MultiVariFacts {
    std::size_t factor_count = 0;
    std::size_t valid_count = 0;
    std::size_t missing_count = 0;
    double combination_coverage = 0.0;
    std::vector<std::string> factor_names;
};

struct ToleranceFacts {
    std::size_t valid_count = 0;
    std::size_t missing_count = 0;
    std::optional<double> mean;
    std::optional<double> standard_deviation;
    std::optional<double> coverage;
    std::optional<double> confidence_level;
    std::optional<double> lower;
    std::optional<double> upper;
    std::optional<double> k_factor;
    std::optional<double> achieved_confidence;
    std::string method;
    std::string method_family = "normal";
    std::string interval_type = "two_sided";
    std::string assumption_status = "not_verified";
};

struct MsaFacts {
    std::optional<double> slope;
    std::optional<double> bias_low;
    std::optional<double> bias_high;
    std::optional<double> p_value;
    std::optional<double> cgk;
    std::optional<double> tolerance_percent;
    std::optional<double> ndc;
    bool ndc_available = false;
    bool design_balanced = true;
    bool interaction_retained = true;
    std::optional<double> interaction_p_value;
    bool interaction_reduction_recommended = false;
    bool negative_variance_truncated = false;
    std::optional<double> gage_percent_study_variation;
    std::optional<double> gage_percent_contribution;
    std::optional<double> linearity;
    std::optional<double> percent_linearity;
    std::optional<double> slope_p_value;
    std::optional<double> intercept_p_value;
    std::optional<double> residual_s;
    std::optional<double> average_bias;
    std::optional<double> average_bias_p;
    std::optional<double> process_variation_used;
    bool ratings_are_ordinal = false;
    bool kendall_available = false;
    bool weighted_kappa_available = false;
    bool by_part_plot_available = false;
    bool interaction_plot_available = false;
    std::size_t plot_point_count = 0;
    std::optional<double> kendall_w;
    std::optional<double> kendall_w_p;
    std::optional<double> kendall_tau;
    std::optional<double> kendall_tau_p;
    std::string kappa_weight_scheme = "none";
    std::string assumption_status = "not_verified";
    std::vector<RuleEvidence> rules;
};

struct ReliabilityFacts {
    std::optional<double> shape;
    std::optional<std::size_t> censored_count;
    std::optional<std::size_t> failure_count;
    std::optional<std::size_t> valid_count;
    std::optional<double> median_life;
    bool identifiable = false;
    bool converged = false;
    std::string not_computed_reason;
    std::string event_encoding = "failure_suspension";
    std::string distribution;
    std::optional<double> location;
    std::optional<double> scale;
    std::optional<double> threshold;
    std::vector<RuleEvidence> rules;
};

struct ForecastFacts {
    std::optional<double> mape;
    std::optional<double> mase;
    std::optional<double> rolling_origin_mape;
    std::optional<double> rolling_origin_mase;
};

struct DescriptiveFacts {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::optional<double> mean;
    std::optional<double> standard_deviation;
};

struct ChiSquareFacts {
    std::optional<double> statistic;
    std::optional<double> p_value;
    std::optional<double> degrees_of_freedom;
    bool expected_count_warning = false;
    std::size_t row_count = 0;
    std::size_t column_count = 0;
    std::size_t total_count = 0;
    std::size_t missing_count = 0;
    std::optional<double> likelihood_ratio_statistic;
    std::optional<double> likelihood_ratio_p_value;
    bool plot_available = false;
};

struct ChiSquareGofFacts {
    std::optional<double> statistic;
    std::optional<double> p_value;
    std::optional<double> degrees_of_freedom;
    std::size_t category_count = 0;
    std::size_t total_count = 0;
    std::size_t missing_count = 0;
    bool expected_count_warning = false;
    std::size_t expected_below_five_count = 0;
    std::optional<double> minimum_expected_count;
    std::string validity_status = "ok";
    std::string recommendation;
    bool plot_available = false;
    std::string proportion_source = "equal";
};

struct NonparametricFacts {
    std::string method;
    std::optional<double> statistic;
    std::optional<double> p_value;
    bool tie_correction = false;
    bool continuity_correction = true;
    std::string approximation = "normal";
    bool small_sample_warning = false;
    std::optional<double> effect_size;
    std::optional<double> p_value_unadjusted;
    std::size_t group_count = 0;
    std::size_t plot_point_count = 0;
    std::size_t missing_count = 0;
    std::optional<double> location_estimate;
    std::optional<double> ci_lower;
    std::optional<double> ci_upper;
    bool dunn_available = false;
    bool steel_dwass_available = false;
    std::string posthoc_method = "dunn";
    std::size_t posthoc_pair_count = 0;
    std::size_t grouping_letter_count = 0;
};

struct LogisticFacts {
    bool converged = false;
    bool complete_separation = false;
    std::optional<double> hosmer_lemeshow_statistic;
    std::optional<double> hosmer_lemeshow_p;
    std::optional<std::size_t> hosmer_lemeshow_df;
    std::size_t hosmer_lemeshow_groups = 0;
    std::string hosmer_lemeshow_status = "not_computed";
    std::size_t high_leverage_count = 0;
    std::optional<double> leverage_threshold;
    std::optional<double> maximum_leverage;
    std::optional<double> maximum_vif;
};

struct PcaFacts {
    std::string mode = "covariance";
    std::size_t retained_component_count = 0;
    std::size_t anomaly_count = 0;
    std::size_t observation_count = 0;
    std::optional<double> t2_limit;
    std::optional<double> q_limit;
    std::optional<double> residual_ad_p;
    std::size_t diagnostic_plot_count = 0;
    bool converged = false;
};

struct VarianceFacts {
    std::string method;
    std::optional<double> statistic;
    std::optional<double> p_value;
    std::optional<double> ci_lower;
    std::optional<double> ci_upper;
    std::size_t group_count = 0;
};

struct ProportionFacts {
    std::string kind = "one_sample";
    std::size_t events = 0;
    std::size_t trials = 0;
    std::optional<double> proportion;
    std::optional<std::size_t> second_events;
    std::optional<std::size_t> second_trials;
    std::optional<double> second_proportion;
    std::optional<double> difference;
    std::optional<double> hypothesized;
    std::string method;
    std::string ci_method;
    std::optional<double> p_value;
    std::optional<double> fisher_p_value;
    std::optional<double> ci_lower;
    std::optional<double> ci_upper;
    std::string assumption_status = "not_verified";
};

struct BoxCoxFacts {
    double lambda = 0.0;
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::optional<double> transformed_standard_deviation;
    bool rounded_lambda = true;
    std::string assumption_status = "not_verified";
};

struct PoissonRateFacts {
    std::string kind;
    std::size_t events = 0;
    double exposure = 0.0;
    std::optional<double> rate;
    std::optional<std::size_t> second_events;
    std::optional<double> second_exposure;
    std::optional<double> second_rate;
    std::optional<double> hypothesized;
    std::string method;
    std::string comparison = "difference";
    std::optional<double> ratio;
    std::optional<double> ratio_ci_lower;
    std::optional<double> ratio_ci_upper;
    std::optional<double> z_statistic;
    std::optional<double> p_value;
    std::optional<double> ci_lower;
    std::optional<double> ci_upper;
    std::string assumption_status = "not_verified";
};

struct EquivalenceFacts {
    std::string kind;
    std::optional<double> difference;
    std::optional<double> lower;
    std::optional<double> upper;
    std::optional<double> ci_lower;
    std::optional<double> ci_upper;
    std::optional<double> p_lower;
    std::optional<double> p_upper;
    std::optional<double> alpha;
    std::string ci_method = "tost_1_minus_alpha";
    bool within_limits = false;
    bool both_pvalues_below_alpha = false;
    std::string assumption_status = "not_verified";
};

struct TTestFacts {
    std::string kind;
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::optional<double> mean;
    std::optional<double> difference;
    std::optional<double> p_value;
    std::optional<double> ci_lower;
    std::optional<double> ci_upper;
    std::string variance_method;
    std::string assumption_status = "not_verified";
};

struct NormalityFacts {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::string decision = "not_computed";
    std::optional<double> p_value;
    std::optional<double> anderson_darling;
    double alpha = 0.05;
    std::string assumption_status = "not_verified";
};

struct CorrelationFacts {
    std::string method = "pearson";
    std::size_t variable_count = 0;
    std::size_t n = 0;
    std::size_t missing_skipped = 0;
    std::string assumption_status = "not_verified";
};

struct OutlierTestFacts {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::optional<double> mean;
    std::optional<double> standard_deviation;
    std::optional<double> g_statistic;
    std::optional<double> p_value;
    std::optional<double> outlier_value;
    std::optional<std::size_t> source_row;
    std::string direction;
    std::string alternative = "two_sided";
    double alpha = 0.05;
    std::string assumption_status = "not_verified";
};

struct DistributionIdentificationFacts {
    std::string best_distribution;
    std::optional<double> best_anderson_darling;
    std::optional<double> best_p_value;
    bool did_not_change_capability_defaults = true;
};

struct PowerFacts {
    std::optional<double> power;
    std::optional<double> effect_size;
    std::string mode;
    std::optional<std::size_t> sample_size;
    std::optional<double> target;
    std::optional<double> actual_power;
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
    std::optional<AnovaFacts> anova;
    std::optional<DescriptiveFacts> descriptive;
    std::optional<ChiSquareFacts> chi_square;
    std::optional<ChiSquareGofFacts> chi_square_gof;
    std::optional<NonparametricFacts> nonparametric;
    std::optional<LogisticFacts> logistic;
    std::optional<DistributionIdentificationFacts> distribution_identification;
    std::optional<PcaFacts> pca;
    std::optional<VarianceFacts> variance;
    std::optional<MultiVariFacts> multi_vari;
    std::optional<ToleranceFacts> tolerance;
    std::optional<ProportionFacts> proportion;
    std::optional<BoxCoxFacts> box_cox;
    std::optional<PoissonRateFacts> poisson_rate;
    std::optional<EquivalenceFacts> equivalence;
    std::optional<TTestFacts> t_test;
    std::optional<NormalityFacts> normality;
    std::optional<CorrelationFacts> correlation;
    std::optional<OutlierTestFacts> outlier_test;
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
