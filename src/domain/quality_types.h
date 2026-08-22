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
    std::string provider_id;
    std::string source_object;
    std::string object_kind;
    std::string filter_summary;
    std::vector<std::string> selected_columns;
    bool row_id_is_synthetic = false;
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
    std::string normality_method = "anderson_darling";
    std::string expected_proportions;
    std::optional<std::size_t> gof_category_column;
    std::optional<double> equivalence_lower;
    std::optional<double> equivalence_upper;
    std::string equivalence_ratio_transform = "none";
    std::string nonparametric_posthoc = "dunn";
    std::string rate_comparison = "difference";
    // runs_test: mean | median | value（value 时用 hypothesis_mean 作为 K）
    std::string runs_criterion = "mean";
    // outlier_test: grubbs | dixon_r10
    std::string outlier_method = "grubbs";
    // tolerance_intervals: normal | nonparametric（兼容旧 variance_method=nonparametric）
    std::string tolerance_method = "normal";
    // one_sample_z: 已知总体标准差
    std::optional<double> known_sigma;
    // acceptance_sampling: 二项 OC（不读工作表）
    std::size_t acceptance_sample_size = 0;
    std::size_t acceptance_number = 0;
    std::optional<double> acceptance_aql;
    std::optional<double> acceptance_rql;
    std::optional<std::size_t> acceptance_lot_size;
    // correlation: 可选偏相关（Pearson only）
    bool compute_partial_correlation = false;
    // anom: 族误差率 α
    double anom_alpha = 0.05;
    // logistic_regression stepwise (Wave-4)
    bool logistic_stepwise_enabled = false;
    std::string logistic_stepwise_method = "stepwise";
    double logistic_stepwise_alpha_enter = 0.15;
    double logistic_stepwise_alpha_remove = 0.15;
};

struct ControlConfiguration {
    std::optional<int> subgroup_size;
    int moving_range_length = 2;
    std::string sigma_method = "average_moving_range";
    bool use_nelson_estimate = false;
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
    int ma_window = 3;
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
    std::size_t fraction_p = 0;
    std::string generators_text;
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
    std::optional<std::size_t> gage_additional_column;  // Expanded Gage 第 3 因子
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
    std::optional<std::size_t> censoring_type_column;
    std::optional<std::size_t> time_unit_column;
    std::optional<std::size_t> failure_mode_column;
    std::optional<std::size_t> exposure_column;
    // Fine-Gray covariates: one continuous column, or multiple (multi IPCW).
    // If covariate_columns.size() >= 2 → multi; else first/single → continuous; else group → binary.
    std::optional<std::size_t> covariate_column;
    std::vector<std::size_t> covariate_columns;
    // Interval censoring bounds (required when any row is typed interval).
    std::optional<std::size_t> interval_left_column;
    std::optional<std::size_t> interval_right_column;
    std::string model = "kaplan_meier";
    std::vector<double> percentile_levels = {10.0, 50.0, 90.0};
    // Warranty summary (model=warranty / reliability_warranty command)
    double warranty_time = 0.0;
    std::string time_unit;
    double exposure = 0.0;
    double reliability_at_warranty = 0.0;
    bool reliability_is_prediction = true;
    std::string dataset_id;
    std::size_t warranty_observed_failures = 0;
    std::size_t warranty_censored_count = 0;
    std::size_t warranty_valid_count = 0;
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
    // Equivalence: lower/upper limits in σ units (default ±effect_size when unset).
    std::optional<double> equivalence_lower;
    std::optional<double> equivalence_upper;
    // Equivalence true difference in σ units (default 0).
    double equivalence_difference = 0.0;
    // DOE factorial power: fraction p (also readable from observation_length as int).
    std::size_t doe_fraction_p = 0;
    std::size_t doe_replicates = 1;
};

struct PcaConfiguration {
    std::vector<std::size_t> variable_columns;
    std::string mode = "covariance";
    std::size_t component_count = 0;
    double anomaly_quantile = 0.99;
};

struct KMeansConfiguration {
    std::vector<std::size_t> variable_columns;
    std::size_t cluster_count = 2;
    std::size_t max_iterations = 100;
    bool standardize = false;
};

struct CartTreeConfiguration {
    std::optional<std::size_t> response_column;
    std::vector<std::size_t> predictor_columns;
    std::string task = "classification";  // classification | regression
    std::size_t max_depth = 5;
    std::size_t min_leaf = 5;
};

struct AdfConfiguration {
    std::optional<std::size_t> series_column;
    std::string regression = "drift";  // none | drift | trend
    std::size_t lags = 0;  // 0 = default
};

struct PoissonRegressionConfiguration {
    std::optional<std::size_t> response_column;
    std::vector<std::size_t> predictor_columns;
    std::size_t max_iterations = 100;
    double tolerance = 1.0e-8;
};

struct IsolationForestConfiguration {
    std::vector<std::size_t> variable_columns;
    std::size_t tree_count = 100;
    std::size_t max_samples = 256;
    std::uint32_t seed = 1;
    double score_quantile = 0.90;
};

struct BootstrapMeanConfiguration {
    std::optional<std::size_t> series_column;
    std::size_t replicates = 2000;
    double confidence_level = 0.95;
    std::uint32_t seed = 1;
};

struct BootstrapTwoSampleConfiguration {
    std::optional<std::size_t> first_column;
    std::optional<std::size_t> second_column;
    std::size_t replicates = 2000;
    double confidence_level = 0.95;
    std::uint32_t seed = 1;
};

struct ProbitReliabilityConfiguration {
    std::optional<std::size_t> events_column;
    std::optional<std::size_t> trials_column;
    std::optional<std::size_t> stress_column;
    std::size_t max_iterations = 100;
    double tolerance = 1.0e-8;
};

struct HierarchicalClusterConfiguration {
    std::vector<std::size_t> variable_columns;
    std::size_t cluster_count = 2;
    bool standardize = false;
};

struct OrdinalLogisticConfiguration {
    std::optional<std::size_t> response_column;
    std::vector<std::size_t> predictor_columns;
    std::size_t max_iterations = 50;
};

struct NominalLogisticConfiguration {
    std::optional<std::size_t> response_column;
    std::vector<std::size_t> predictor_columns;
    std::size_t max_iterations = 50;
};

struct NonparametricCapabilityConfiguration {
    std::optional<std::size_t> measurement_column;
    double tolerance_k = 6.0;
};

struct CoxRegressionConfiguration {
    std::optional<std::size_t> time_column;
    std::optional<std::size_t> event_column;
    std::vector<std::size_t> covariate_columns;
    double confidence_level = 0.95;
    std::string ties_method = "breslow";
    int max_iterations = 100;
    double tolerance = 1.0e-8;
};

struct AcceleratedLifeConfiguration {
    std::optional<std::size_t> time_column;
    std::optional<std::size_t> event_column;
    std::optional<std::size_t> stress_column;
    double use_stress_celsius = 25.0;
};

struct DiscriminantConfiguration {
    std::optional<std::size_t> response_column;
    std::vector<std::size_t> predictor_columns;
};

struct CcfConfiguration {
    std::optional<std::size_t> x_column;
    std::optional<std::size_t> y_column;
    std::size_t max_lag = 0;
};

struct CorrelogramConfiguration {
    std::vector<std::size_t> variable_columns;
    std::string method = "pearson";
};

struct StepwiseRegressionConfiguration {
    std::optional<std::size_t> response_column;
    std::vector<std::size_t> predictor_columns;
    std::string method = "stepwise";  // stepwise / forward / backward
    double alpha_enter = 0.15;
    double alpha_remove = 0.15;
};

struct BestSubsetsRegressionConfiguration {
    std::optional<std::size_t> response_column;
    std::vector<std::size_t> predictor_columns;
    std::size_t min_predictors = 1;
    std::size_t max_predictors = 0;  // 0 = all candidates
    std::size_t models_per_size = 1;
};

struct BatchCapabilityConfiguration {
    std::optional<std::size_t> measurement_column;
    std::optional<std::size_t> batch_column;
    std::size_t min_batch_size = 2;
};

struct KmIntervalConfiguration {
    std::optional<std::size_t> left_column;
    std::optional<std::size_t> right_column;
};

struct PlackettBurmanConfiguration {
    std::vector<std::string> factor_names;
    std::vector<std::string> low_levels;
    std::vector<std::string> high_levels;
    std::size_t center_point_count = 0;
    bool randomize = false;
    std::uint64_t random_seed = 0;
};

// Phase 4: CCD / BBD design generation (continuous factors only).
struct ResponseSurfaceDesignConfiguration {
    std::string design_kind = "ccd";  // ccd | bbd
    std::string ccd_variant = "ccf";  // ccc | cci | ccf
    std::string design_source_id;     // stable id linking design → RSM
    std::vector<std::string> factor_ids;
    std::vector<std::string> factor_names;
    std::vector<std::string> factor_units;
    std::vector<double> low_levels;
    std::vector<double> high_levels;
    std::vector<double> centers;  // empty → midpoints
    std::size_t center_point_count = 1;
    std::size_t block_count = 1;
    bool randomize = true;
    std::uint64_t random_seed = 1;
    bool allow_beyond_range = false;
    std::optional<double> alpha_override;
};

struct GraphConfiguration {
    std::string graph_kind;
    std::optional<std::size_t> x_column;
    std::optional<std::size_t> y_column;
    std::optional<std::size_t> size_column;
    std::optional<std::size_t> by_column;
    std::optional<std::size_t> label_column;
    // Controlled facet: one panel per level (not free-form canvas). Distinct from by_column
    // (within-plot grouping/color). Excess levels are truncated with a diagnostic.
    std::optional<std::size_t> facet_column;
    int facet_max_panels = 6;  // hard product cap; clamp 1..12 in services
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
    std::vector<std::size_t> hidden_rows;  // Phase 7: display-only; ≠ excluded
    std::vector<std::size_t> variable_columns;
    std::optional<std::size_t> by_column;
    std::optional<double> pareto_other_threshold_percent;
    std::optional<std::size_t> inspected_constant;
    std::string effect_title;  // cause_and_effect 效果标题
    InferenceConfiguration inference;
    ControlConfiguration control;
    TimeSeriesConfiguration time_series;
    DoeConfiguration doe;
    MsaConfiguration msa;
    ReliabilityConfiguration reliability;
    PowerConfiguration power;
    PcaConfiguration pca;
    KMeansConfiguration kmeans;
    CartTreeConfiguration cart_tree;
    AdfConfiguration adf;
    PoissonRegressionConfiguration poisson_regression;
    IsolationForestConfiguration isolation_forest;
    BootstrapMeanConfiguration bootstrap_mean;
    BootstrapTwoSampleConfiguration bootstrap_two_sample;
    ProbitReliabilityConfiguration probit_reliability;
    HierarchicalClusterConfiguration hierarchical_cluster;
    OrdinalLogisticConfiguration ordinal_logistic;
    NominalLogisticConfiguration nominal_logistic;
    NonparametricCapabilityConfiguration nonparametric_capability;
    CoxRegressionConfiguration cox_regression;
    AcceleratedLifeConfiguration accelerated_life;
    DiscriminantConfiguration discriminant;
    CcfConfiguration ccf;
    CorrelogramConfiguration correlogram;
    StepwiseRegressionConfiguration stepwise_regression;
    BestSubsetsRegressionConfiguration best_subsets_regression;
    BatchCapabilityConfiguration batch_capability;
    KmIntervalConfiguration km_interval;
    PlackettBurmanConfiguration plackett_burman;
    ResponseSurfaceDesignConfiguration response_surface_design;
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
    surface,
    density,
    hexbin,
    violin,
    bar
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
    // Optional typed metadata for report interaction (backward compatible when empty).
    std::vector<std::string> column_kinds;  // text/number/percent/p_value/row_id/status/rule_id/timestamp
    std::vector<RowId> row_ids;
    std::vector<std::string> rule_ids;
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
    // Aggregated geoms (bar/interval/violin/…): selecting index i expands to all
    // worksheet rows in member_source_rows[i]. Present index (even empty) is
    // authoritative; fall back to source_rows[i] only when the index is absent
    // from member_source_rows.
    std::vector<std::vector<std::size_t>> member_source_rows;
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
    std::string name;
    std::string window;
    std::string threshold;
    std::string comparison_direction;
    std::vector<std::size_t> plotted_points;
    std::string not_applicable_reason;
    std::string not_verified_reason;
    std::string calculation_failed_reason;
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
    // Phase 6 gate: Johnson stays research-preview until golden/tail review.
    // Stability prerequisite: default deny pass/fail until assumptions verified.
    bool pass_fail_judgment_allowed = false;
    bool research_preview = false;
    std::string gate_status = "stability_unverified";  // open | gated_research | blocked | stability_unverified | open_with_limits
    std::string evidence_type = "formula_reference";
    std::string stability_screen_status = "not_run";
    std::size_t stability_out_of_control_count = 0;
    std::string bimodality_screen_status = "not_run";
    std::size_t bimodality_peak_count = 0;
    std::string hartigan_dip_status = "not_run";
    double hartigan_dip_statistic = 0.0;
    std::optional<double> hartigan_dip_p_value;
    std::string mixture_status = "not_run";
    int mixture_k_selected = 1;
    int mixture_k_max = 4;
    double mixture_weight1 = 0.0;
    double mixture_mean1 = 0.0;
    double mixture_mean2 = 0.0;
    double mixture_sd1 = 0.0;
    double mixture_sd2 = 0.0;
    double mixture_delta_bic = 0.0;
    std::string mixture_algorithm_id = "gaussian_mixture_k_bic";
    std::string mixture_evidence_type = "formula_reference";
    struct MixtureComponentFacts {
        double weight = 0.0;
        double mean = 0.0;
        double sd = 0.0;
    };
    std::vector<MixtureComponentFacts> mixture_components;
};

struct BatchCapabilityFacts {
    std::size_t batch_count = 0;
    std::size_t skipped_batch_count = 0;
    std::size_t total_observations = 0;
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
    std::optional<double> durbin_watson_dl;
    std::optional<double> durbin_watson_du;
    std::string durbin_watson_decision = "not_computed";
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
    std::string rule_policy;
    std::vector<int> enabled_special_cause_tests;
    std::vector<std::string> enabled_special_cause_rule_ids;
    std::vector<RuleEvidence> rules;
    std::string sigma_method;
    bool use_nelson_estimate = false;
    std::size_t nelson_excluded_ranges = 0;
    std::optional<double> estimated_sigma;
    bool historical_parameters_used = false;
    std::size_t stage_count = 0;
};

struct MultivariateSpcFacts {
    std::string kind;  // hotelling_t2 | mewma | generalized_variance
    std::size_t observation_count = 0;
    std::size_t variable_count = 0;
    std::size_t subgroup_count = 0;
    std::size_t out_of_control_count = 0;
    std::optional<double> upper_control_limit;
    std::optional<double> lower_control_limit;
    std::optional<double> center_line;
    std::optional<double> lambda;
    std::string limit_method;
    std::string phase;
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
    std::string design_kind;
    std::size_t fraction_p = 0;
    int resolution = 0;
    std::size_t run_count = 0;
    std::string generator_text;
};

struct RsmFacts {
    std::size_t factor_count = 0;
    std::size_t term_count = 0;
    std::size_t residual_count = 0;
    std::optional<double> r_squared;
    std::optional<double> adjusted_r_squared;
    bool contour_plot_available = false;
    std::string largest_abs_t_term;
    std::string response_name;
    // Phase 4.4 design provenance / honesty fields
    std::string design_source_id;
    std::string design_kind;
    std::string coding_mode;  // minmax | design_bounds | already_coded
    std::size_t center_point_count = 0;
    bool surface_is_static = true;
    std::string evidence_type = "formula_reference";
    // Phase 4.4 LOF honesty fields (formula_reference; not vendor_oracle)
    bool pure_error_available = false;
    bool lack_of_fit_available = false;
    std::optional<double> lack_of_fit_f;
    std::optional<double> lack_of_fit_p;
    std::size_t pure_error_df = 0;
    std::size_t lack_of_fit_df = 0;
};

struct MultiVariFacts {
    std::size_t factor_count = 0;
    std::size_t valid_count = 0;
    std::size_t missing_count = 0;
    double combination_coverage = 0.0;
    std::vector<std::string> factor_names;
};

struct VariabilityFacts {
    std::size_t factor_count = 0;
    std::size_t valid_count = 0;
    std::size_t missing_count = 0;
    std::size_t cell_count = 0;
    std::optional<double> overall_mean;
    std::optional<double> mean_of_cell_sds;
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
    // EMP Crossed (Wheeler)
    bool emp_available = false;
    std::optional<double> emp_icc_no_bias;
    std::optional<double> emp_icc_with_bias;
    std::optional<double> emp_icc_with_interaction;
    std::optional<double> emp_probable_error;
    std::string emp_classification;
};

struct ReliabilityModeFitFacts {
    std::string failure_mode;
    std::size_t failure_count = 0;
    std::size_t competing_failure_count = 0;
    std::size_t right_censored_count = 0;
    std::size_t valid_count = 0;
    bool identifiable = false;
    bool converged = false;
    std::optional<double> shape;
    std::optional<double> scale;
    std::optional<double> location;
    std::optional<double> rate;
    std::optional<double> median_life;
    std::optional<double> reliability_at_warranty;
    std::string not_computed_reason;
    std::string evidence_type = "formula_reference";
    std::string algorithm_id = "cause_specific_right_censored_competing";
    std::vector<std::size_t> source_rows;
};

struct ReliabilityCifModeFacts {
    std::string failure_mode;
    std::size_t failure_count = 0;
    std::optional<double> cif_at_last_event;
    std::optional<double> cif_at_warranty;
    std::size_t point_count = 0;
};

struct ReliabilityFineGrayTermFacts {
    std::string name;
    std::optional<double> mean;
    std::optional<double> beta;
    std::optional<double> se;
    std::optional<double> hazard_ratio;
    std::optional<double> p_value;
};

struct ReliabilityLogRankGroupFacts {
    std::string label;
    int group_id = 0;
    std::size_t n = 0;
    std::size_t failures = 0;
    std::size_t censored = 0;
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
    std::string evidence_type = "formula_reference";
    std::string time_unit;
    std::size_t exact_count = 0;
    std::size_t right_censored_count = 0;
    std::size_t left_censored_count = 0;
    std::size_t interval_censored_count = 0;
    std::vector<std::string> failure_modes;
    std::size_t failure_mode_distinct_count = 0;
    std::optional<double> total_exposure;
    std::size_t exposure_row_count = 0;
    std::string exposure_source;  // empty | scalar | column_sum
    // Cause-specific per-mode fits (competing modes censored). Empty = not run.
    std::string mode_fit_scheme;  // empty | cause_specific
    std::vector<ReliabilityModeFitFacts> mode_fits;
    // Aalen–Johansen CIF (competing risks). Empty = not run.
    std::string cif_algorithm_id;  // empty | aalen_johansen_cif
    std::string cif_evidence_type = "formula_reference";
    std::vector<ReliabilityCifModeFacts> cif_modes;
    // Fine–Gray subdistribution hazard. Empty algorithm = not run.
    std::string fine_gray_algorithm_id;  // empty | fine_gray_binary_ipcw | fine_gray_continuous_ipcw | fine_gray_multi_ipcw
    std::string fine_gray_evidence_type = "formula_reference";
    std::string fine_gray_kind;  // empty | binary | continuous | multi
    std::string fine_gray_target_mode;
    std::string fine_gray_covariate_name;
    std::string fine_gray_group0;
    std::string fine_gray_group1;
    bool fine_gray_converged = false;
    std::optional<double> fine_gray_covariate_mean;
    std::optional<double> fine_gray_beta;
    std::optional<double> fine_gray_se;
    std::optional<double> fine_gray_hazard_ratio;
    std::optional<double> fine_gray_p_value;
    std::string fine_gray_not_computed_reason;
    std::size_t fine_gray_target_failures = 0;
    std::size_t fine_gray_competing_failures = 0;
    std::vector<ReliabilityFineGrayTermFacts> fine_gray_terms;
    // K-group Log-rank (Kaplan-Meier stratified comparison). Empty groups = not run.
    std::optional<std::size_t> log_rank_group_count;
    std::optional<double> log_rank_chi_square;
    std::optional<double> log_rank_df;
    std::optional<double> log_rank_p_value;
    std::vector<ReliabilityLogRankGroupFacts> log_rank_groups;
    // Gray test for CIF group comparison (formula_reference; optional narrow gate).
    std::optional<double> gray_chi_square;
    std::optional<double> gray_df;
    std::optional<double> gray_p_value;
    std::optional<std::size_t> gray_group_count;
    std::string gray_not_computed_reason;
    std::string gray_algorithm_id;  // empty | gray_cif_group_test
};

struct WarrantyStratumFacts {
    std::string label;
    std::string kind;  // failure_mode | group
    double exposure = 0.0;
    std::size_t observed_failures = 0;
    std::size_t censored_count = 0;
    std::size_t valid_count = 0;
    double expected_failures = 0.0;
    double share_of_total_exposure = 0.0;
    std::string exposure_attribution;
    std::optional<double> reliability_at_warranty;  // mode-specific when fitted
    bool uses_mode_specific_reliability = false;
    std::vector<std::size_t> source_rows;
};

struct WarrantyFacts {
    double warranty_time = 0.0;
    std::string time_unit;
    double exposure = 0.0;
    double reliability_at_warranty = 0.0;
    double failure_probability = 0.0;
    double expected_failures = 0.0;
    double claims_per_1000 = 0.0;
    std::size_t observed_failures = 0;
    std::size_t censored_count = 0;
    std::size_t valid_count = 0;
    std::string model_name;
    std::string quantity_label = "prediction";
    std::string evidence_type = "formula_reference";
    std::string exposure_source;  // scalar | column_sum
    std::size_t exposure_row_count = 0;
    std::string stratum_kind;  // empty | failure_mode | group
    bool uses_pooled_reliability = true;
    bool uses_mode_specific_reliability = false;
    std::vector<WarrantyStratumFacts> strata;
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
    std::optional<double> fisher_p_value;
    std::optional<double> odds_ratio;
    std::string method;  // empty=pearson association; fisher_exact when dedicated
    std::optional<double> max_abs_adjusted_residual;
    std::string largest_contribution_cell;
    bool residual_heatmap_available = false;
    bool percent_tables_available = false;
};

struct CrossTabFacts {
    std::size_t row_count = 0;
    std::size_t column_count = 0;
    std::size_t total_count = 0;
    std::size_t missing_count = 0;
    bool percent_tables_available = false;
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
    std::string method;  // empty=chi_square_gof; poisson=poisson_gof
    std::optional<double> lambda_hat;
};

struct McNemarFacts {
    std::size_t a = 0;
    std::size_t b = 0;
    std::size_t c = 0;
    std::size_t d = 0;
    std::size_t discordant = 0;
    std::size_t pair_count = 0;
    std::size_t missing_count = 0;
    std::optional<double> chi_square;
    std::optional<double> p_value;
    double degrees_of_freedom = 1.0;
    bool continuity_correction = true;
    std::string method = "edwards";
    bool computable = false;
};

struct CochranQFacts {
    std::size_t treatment_count = 0;
    std::size_t subject_count = 0;
    std::size_t missing_count = 0;
    std::optional<double> q_statistic;
    std::optional<double> p_value;
    double degrees_of_freedom = 0.0;
    bool computable = false;
    std::string approximation = "chi_square";
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
    bool nemenyi_available = false;
    std::string posthoc_method = "dunn";
    std::size_t posthoc_pair_count = 0;
    std::size_t grouping_letter_count = 0;
};

struct LogisticStepwiseStepFacts {
    std::size_t step = 0;
    std::string action;
    std::string term;
    std::optional<double> deviance;
    std::optional<double> aic;
    std::optional<double> aicc;
    std::optional<double> bic;
    std::optional<double> enter_p;
    std::optional<double> remove_p;
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
    std::size_t concordant_pairs = 0;
    std::size_t discordant_pairs = 0;
    std::size_t tied_pairs = 0;
    std::optional<double> pairs_concordance_percent;
    std::size_t true_positive = 0;
    std::size_t true_negative = 0;
    std::size_t false_positive = 0;
    std::size_t false_negative = 0;
    // Stepwise selection (Wave-4; empty method = not run)
    std::string stepwise_method;
    std::string stepwise_criterion;
    std::size_t stepwise_step_count = 0;
    std::size_t stepwise_selected_count = 0;
    std::size_t stepwise_best_step_index = 0;
    std::vector<LogisticStepwiseStepFacts> stepwise_steps;
    std::optional<double> stepwise_log_likelihood;
    std::optional<double> stepwise_aic;
    std::optional<double> stepwise_bic;
};

struct CoxRegressionCoefficientFacts {
    std::string term;
    std::optional<double> beta;
    std::optional<double> se;
    std::optional<double> z;
    std::optional<double> p_value;
    std::optional<double> hazard_ratio;
    std::optional<double> ci_lower;
    std::optional<double> ci_upper;
};

struct CoxRegressionFacts {
    std::size_t n = 0;
    std::size_t events = 0;
    std::size_t censored = 0;
    bool converged = false;
    std::optional<double> log_likelihood;
    std::string evidence_type = "formula_reference";
    std::string algorithm_id = "cox_ph_fixed_covariates";
    std::string ties_method = "breslow";
    std::vector<CoxRegressionCoefficientFacts> coefficients;
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

struct KMeansFacts {
    std::size_t k = 0;
    std::size_t n = 0;
    std::size_t variable_count = 0;
    std::size_t iterations = 0;
    bool converged = false;
    bool standardized = false;
    std::optional<double> total_within_ss;
};

struct CartTreeFacts {
    std::string task = "classification";
    std::size_t n = 0;
    std::size_t predictor_count = 0;
    std::size_t max_depth = 0;
    std::size_t node_count = 0;
    std::size_t leaf_count = 0;
    std::optional<double> train_metric;
    std::string top_variable;
};

struct AdfFacts {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::size_t lags = 0;
    std::size_t used_observations = 0;
    std::string regression = "drift";
    std::optional<double> tau;
    std::optional<double> critical_5;
    bool reject_unit_root_at_5 = false;
};

struct PoissonRegressionFacts {
    std::size_t n = 0;
    std::size_t predictor_count = 0;
    std::size_t iteration_count = 0;
    bool converged = false;
    std::optional<double> deviance;
    std::optional<double> aic;
};

struct IsolationForestFacts {
    std::size_t n = 0;
    std::size_t variable_count = 0;
    std::size_t tree_count = 0;
    std::size_t anomaly_count = 0;
    std::optional<double> score_threshold;
};

struct BootstrapMeanFacts {
    std::size_t n = 0;
    std::size_t replicates = 0;
    std::string method = "percentile";
    std::optional<double> sample_mean;
    std::optional<double> ci_lower;
    std::optional<double> ci_upper;
    double confidence_level = 0.95;
};

struct BootstrapTwoSampleFacts {
    std::size_t n_first = 0;
    std::size_t n_second = 0;
    std::size_t replicates = 0;
    std::string method = "percentile";
    std::optional<double> mean_first;
    std::optional<double> mean_second;
    std::optional<double> mean_difference;
    std::optional<double> ci_lower;
    std::optional<double> ci_upper;
    double confidence_level = 0.95;
};

struct ProbitReliabilityFacts {
    std::size_t n = 0;
    std::size_t iteration_count = 0;
    bool converged = false;
    std::string link = "logit";
    std::optional<double> intercept;
    std::optional<double> stress_coefficient;
    std::optional<double> ld50;
    std::optional<double> ld50_standard_error;
    std::optional<double> ld50_confidence_lower;
    std::optional<double> ld50_confidence_upper;
    std::optional<double> log_likelihood;
    std::optional<double> deviance;
    std::optional<double> aic;
};

struct HierarchicalClusterFacts {
    std::size_t n = 0;
    std::size_t variable_count = 0;
    std::size_t cluster_count = 0;
    std::size_t merge_count = 0;
    std::string linkage = "complete";
    bool standardized = false;
};

struct OrdinalLogisticFacts {
    std::size_t n = 0;
    std::size_t category_count = 0;
    std::size_t predictor_count = 0;
    std::size_t iteration_count = 0;
    bool converged = false;
    std::optional<double> log_likelihood;
    std::optional<double> aic;
};

struct NominalLogisticFacts {
    std::size_t n = 0;
    std::size_t category_count = 0;
    std::size_t logit_count = 0;
    std::size_t predictor_count = 0;
    std::size_t iteration_count = 0;
    bool converged = false;
    std::optional<double> log_likelihood;
    std::optional<double> aic;
    std::optional<double> g_p_value;
    std::string reference_category;
};

struct NonparametricCapabilityFacts {
    std::size_t n = 0;
    double tolerance_k = 6.0;
    std::optional<double> cnp;
    std::optional<double> cnpl;
    std::optional<double> cnpu;
    std::optional<double> cnpk;
    std::optional<double> median;
    std::optional<double> lower_percentile;
    std::optional<double> upper_percentile;
    std::optional<double> observed_ppm_below;
    std::optional<double> observed_ppm_above;
    std::optional<double> observed_ppm_total;
    std::vector<double> histogram_bin_edges;
    std::vector<double> histogram_bin_counts;
};

struct AcceleratedLifeFacts {
    std::size_t n = 0;
    std::size_t failure_count = 0;
    std::size_t censored_count = 0;
    std::size_t stress_level_count = 0;
    bool converged = false;
    std::string transform = "arrhenius";
    std::optional<double> shape;
    std::optional<double> log_likelihood;
    std::optional<double> use_stress_celsius;
    std::optional<double> b10_at_use_stress;
    std::optional<double> b50_at_use_stress;
    std::optional<double> b90_at_use_stress;
};

struct DiscriminantFacts {
    std::size_t n = 0;
    std::size_t class_count = 0;
    std::size_t predictor_count = 0;
    std::optional<double> train_accuracy;
};

struct CcfFacts {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::size_t max_lag = 0;
    std::optional<double> band_half_width;
    std::optional<double> ccf_at_zero;
};

struct CorrelogramFacts {
    std::size_t variable_count = 0;
    std::string method = "pearson";
    std::size_t pair_count = 0;
};

struct StepwiseRegressionFacts {
    std::size_t n = 0;
    std::size_t candidate_count = 0;
    std::size_t selected_count = 0;
    std::size_t step_count = 0;
    std::size_t best_step_index = 0;
    std::string method = "stepwise";
    std::string criterion = "alpha";
    std::optional<double> r_squared;
    std::optional<double> adjusted_r_squared;
    std::optional<double> best_aicc;
    std::optional<double> best_bic;
};

struct BestSubsetsRegressionFacts {
    std::size_t n = 0;
    std::size_t candidate_count = 0;
    std::size_t model_count = 0;
    std::size_t models_per_size = 1;
    std::optional<double> best_r_squared;
    std::optional<double> best_adjusted_r_squared;
    std::optional<std::size_t> best_predictor_count;
};

struct KmIntervalFacts {
    std::size_t n = 0;
    std::size_t exact_count = 0;
    std::size_t left_censored_count = 0;
    std::size_t right_censored_count = 0;
    std::size_t interval_censored_count = 0;
    std::size_t iteration_count = 0;
    bool converged = false;
    bool identifiable = false;
    std::optional<double> median_life;
    // Simplified Turnbull NPMLE — formula_reference only; never vendor_oracle/golden.
    std::string evidence_type = "formula_reference";
    std::string algorithm_id = "turnbull_npmle_simplified_grid";
    std::string gate_status = "open_with_limits";  // not vendor-aligned
    bool research_preview = false;
    bool classic_km_equivalent = false;  // true only when all observations are exact
};

struct PlackettBurmanFacts {
    std::size_t factor_count = 0;
    std::size_t run_count = 0;
    std::size_t center_point_count = 0;
};

struct DesignGenerationFacts {
    std::string design_kind;   // ccd | bbd
    std::string ccd_variant;   // ccc | cci | ccf | empty for BBD
    std::string design_source_id;
    std::size_t factor_count = 0;
    std::size_t run_count = 0;
    std::size_t cube_count = 0;
    std::size_t star_count = 0;
    std::size_t edge_count = 0;
    std::size_t center_count = 0;
    double alpha = 1.0;
    bool allow_beyond_range = false;
    bool beyond_range_detected = false;
    bool randomized = false;
    std::uint64_t random_seed = 0;
    std::string evidence_type = "formula_reference";
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
    // one_sample_z
    std::optional<double> z_statistic;
    std::optional<double> known_sigma;
    std::optional<double> sample_standard_deviation;
};

struct NormalityFacts {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::string method = "anderson_darling";
    std::string decision = "not_computed";
    std::optional<double> p_value;
    std::optional<double> anderson_darling;
    std::optional<double> ryan_joiner_r;
    double alpha = 0.05;
    std::string assumption_status = "not_verified";
};

struct CorrelationFacts {
    std::string method = "pearson";
    std::size_t variable_count = 0;
    std::size_t n = 0;
    std::size_t missing_skipped = 0;
    std::string assumption_status = "not_verified";
    bool covariance_available = false;
    bool partial_available = false;
};

struct AcceptanceSamplingFacts {
    std::size_t sample_size = 0;
    std::size_t acceptance_number = 0;
    std::optional<std::size_t> lot_size;
    std::string model = "binomial";
    std::optional<double> aql;
    std::optional<double> rql;
    std::optional<double> pa_at_aql;
    std::optional<double> pa_at_rql;
    std::size_t oc_point_count = 0;
};

struct AnomFacts {
    double overall_mean = 0.0;
    double pooled_sd = 0.0;
    double udl = 0.0;
    double ldl = 0.0;
    double alpha = 0.05;
    std::size_t group_count = 0;
    std::size_t total_n = 0;
    std::size_t outside_count = 0;
    std::string decision_limit_method;
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
    std::string method = "grubbs";
    std::optional<double> dixon_r;
    std::optional<double> critical_value;
};

struct RunChartFacts {
    std::size_t n = 0;
    std::optional<double> median;
    std::size_t runs_about_median = 0;
    std::size_t runs_up_down = 0;
    std::optional<double> p_clustering;
    std::optional<double> p_mixtures;
    std::optional<double> p_trends;
    std::optional<double> p_oscillation;
    std::size_t missing_count = 0;
};

struct ZoneChartFacts {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::optional<double> center;
    std::optional<double> sigma;
    double signal_threshold = 8.0;
    std::size_t signal_count = 0;
};

struct ZmrFacts {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::size_t group_count = 0;
    bool used_sample_parameters = true;
    std::optional<double> average_mr;
    std::size_t z_out_of_control_count = 0;
};

struct MovingAverageChartFacts {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    int window = 3;
    double limit_sigma = 3.0;
    std::optional<double> center;
    std::optional<double> sigma_within;
    std::size_t out_of_control_count = 0;
};

struct CauseEffectFacts {
    std::string effect;
    std::size_t category_count = 0;
    std::size_t cause_count = 0;
    std::size_t missing_count = 0;
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

struct AcfPacfFacts {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::size_t max_lag = 0;
    std::string confidence_band_method = "white_noise_fixed";
    std::optional<double> band_half_width;
    std::optional<double> alpha;
    bool ljung_box_available = false;
    std::optional<double> ljung_box_statistic;
    std::optional<double> ljung_box_p_value;
};

struct EdaPlotFacts {
    std::string kind;
    std::size_t n = 0;  // display-side plotted / aggregated N
    std::optional<double> bandwidth;
    std::size_t category_count = 0;  // display-side
    std::size_t x_bins = 0;
    std::size_t y_bins = 0;
    bool sorted_by_count = false;
    bool has_cumulative_percent = false;
    // Phase 7 visibility contract
    std::size_t hidden_count = 0;
    std::size_t excluded_count = 0;
    std::size_t analysis_eligible_n = 0;
    std::size_t display_eligible_n = 0;
    bool hidden_excluded_distinct = true;
    // Dual-line persistence: analysis-side aggregates (include hidden, omit excluded)
    std::size_t analysis_n = 0;
    std::size_t analysis_category_count = 0;
    // Controlled facet orchestration (scatter first)
    bool facet_enabled = false;
    std::size_t facet_panel_count = 0;
    std::size_t facet_level_count = 0;
    std::size_t facet_truncated_levels = 0;
    int facet_max_panels = 0;
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
    std::optional<BatchCapabilityFacts> batch_capability;
    std::optional<SpcFacts> spc;
    std::optional<MultivariateSpcFacts> multivariate_spc;
    std::optional<DoeFacts> doe;
    std::optional<RsmFacts> rsm;
    std::optional<MsaFacts> msa;
    std::optional<ReliabilityFacts> reliability;
    std::optional<WarrantyFacts> warranty;
    std::optional<ForecastFacts> forecast;
    std::optional<PowerFacts> power;
    std::optional<AcfPacfFacts> acf_pacf;
    std::optional<EdaPlotFacts> eda;
    std::optional<ParetoFacts> pareto;
    std::optional<RegressionFacts> regression;
    std::optional<AnovaFacts> anova;
    std::optional<DescriptiveFacts> descriptive;
    std::optional<ChiSquareFacts> chi_square;
    std::optional<CrossTabFacts> cross_tab;
    std::optional<ChiSquareGofFacts> chi_square_gof;
    std::optional<McNemarFacts> mcnemar;
    std::optional<CochranQFacts> cochran_q;
    std::optional<NonparametricFacts> nonparametric;
    std::optional<LogisticFacts> logistic;
    std::optional<DistributionIdentificationFacts> distribution_identification;
    std::optional<PcaFacts> pca;
    std::optional<KMeansFacts> kmeans;
    std::optional<CartTreeFacts> cart_tree;
    std::optional<AdfFacts> adf;
    std::optional<PoissonRegressionFacts> poisson_regression;
    std::optional<IsolationForestFacts> isolation_forest;
    std::optional<BootstrapMeanFacts> bootstrap_mean;
    std::optional<BootstrapTwoSampleFacts> bootstrap_two_sample;
    std::optional<ProbitReliabilityFacts> probit_reliability;
    std::optional<HierarchicalClusterFacts> hierarchical_cluster;
    std::optional<OrdinalLogisticFacts> ordinal_logistic;
    std::optional<NominalLogisticFacts> nominal_logistic;
    std::optional<NonparametricCapabilityFacts> nonparametric_capability;
    std::optional<CoxRegressionFacts> cox_regression;
    std::optional<AcceleratedLifeFacts> accelerated_life;
    std::optional<DiscriminantFacts> discriminant;
    std::optional<CcfFacts> ccf;
    std::optional<CorrelogramFacts> correlogram;
    std::optional<StepwiseRegressionFacts> stepwise_regression;
    std::optional<BestSubsetsRegressionFacts> best_subsets_regression;
    std::optional<KmIntervalFacts> km_interval;
    std::optional<PlackettBurmanFacts> plackett_burman;
    std::optional<DesignGenerationFacts> design_generation;
    std::optional<VarianceFacts> variance;
    std::optional<MultiVariFacts> multi_vari;
    std::optional<VariabilityFacts> variability;
    std::optional<ToleranceFacts> tolerance;
    std::optional<ProportionFacts> proportion;
    std::optional<BoxCoxFacts> box_cox;
    std::optional<PoissonRateFacts> poisson_rate;
    std::optional<EquivalenceFacts> equivalence;
    std::optional<TTestFacts> t_test;
    std::optional<NormalityFacts> normality;
    std::optional<CorrelationFacts> correlation;
    std::optional<AcceptanceSamplingFacts> acceptance_sampling;
    std::optional<AnomFacts> anom;
    std::optional<OutlierTestFacts> outlier_test;
    std::optional<RunChartFacts> run_chart;
    std::optional<ZoneChartFacts> zone_chart;
    std::optional<ZmrFacts> z_mr;
    std::optional<MovingAverageChartFacts> moving_average;
    std::optional<CauseEffectFacts> cause_effect;
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
    // Optional worksheet payload for design generators (CCD/BBD) — UI may replace active sheet.
    std::optional<DataTable> worksheet_export;
};

}  // namespace datalab::domain
