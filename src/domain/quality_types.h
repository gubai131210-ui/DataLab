#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain {

struct DataTable {
    std::string name;
    std::string source_path;
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> import_warnings;
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

struct AnalysisConfiguration {
    std::string analysis_name;
    std::string chart_type;
    DataSelection selection;
    SpecificationLimits specifications;
    std::vector<std::size_t> excluded_rows;
    std::vector<std::size_t> variable_columns;
    std::optional<std::size_t> by_column;
    std::optional<int> subgroup_size;
    std::optional<double> pareto_other_threshold_percent;
    std::optional<std::size_t> inspected_constant;
    std::optional<std::size_t> first_events_column;
    std::optional<std::size_t> first_trials_column;
    std::optional<std::size_t> second_events_column;
    std::optional<std::size_t> second_trials_column;
    std::optional<std::size_t> row_category_column;
    std::optional<std::size_t> column_category_column;
    std::optional<std::size_t> gage_measurement_column;
    std::optional<std::size_t> gage_part_column;
    std::optional<std::size_t> gage_operator_column;
    std::optional<double> hypothesis_mean;
    double confidence_level = 0.95;
    std::string alternative = "two_sided";
    std::string correlation_method = "pearson";
    std::string variance_method = "welch";
    int moving_range_length = 2;
    std::string sigma_method = "average_moving_range";
    std::vector<int> enabled_special_cause_tests = {1};
    std::optional<std::size_t> stage_column;
    std::optional<double> historical_center;
    std::optional<double> historical_sigma_z;
    double smoothing_alpha = 0.2;
    double smoothing_gamma = 0.2;
    std::string smoothing_method = "double";
    int forecast_periods = 1;
    double ewma_lambda = 0.2;
    double ewma_limit_sigma = 3.0;
    double cusum_target = 0.0;
    double cusum_sigma = 1.0;
    double cusum_k = 0.5;
    double cusum_h = 4.0;
    bool cusum_fast_initial_response = false;
    std::optional<std::size_t> arima_time_column;
    std::optional<std::size_t> arima_value_column;
    int arima_differencing = 1;
    std::string arima_selection_criterion = "aicc";
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
    std::optional<std::size_t> decomposition_time_column;
    std::optional<std::size_t> decomposition_value_column;
    int decomposition_seasonal_period = 1;
    std::string decomposition_model = "additive";
    std::vector<std::string> doe_factor_names;
    std::vector<std::size_t> doe_factor_columns;
    std::optional<std::size_t> doe_response_column;
    std::vector<std::string> doe_low_levels;
    std::vector<std::string> doe_high_levels;
    std::size_t doe_center_point_count = 0;
    std::size_t doe_block_count = 1;
    bool doe_randomize = false;
    std::uint64_t doe_random_seed = 0;
    std::optional<std::size_t> nested_gage_measurement_column;
    std::optional<std::size_t> nested_gage_part_column;
    std::optional<std::size_t> nested_gage_operator_column;
    double gage_tolerance = 0.0;
    std::optional<std::size_t> attribute_rating_column;
    std::optional<std::size_t> attribute_part_column;
    std::optional<std::size_t> attribute_appraiser_column;
    std::optional<std::size_t> attribute_standard_column;
    std::string attribute_agreement_method = "kappa";
    std::optional<std::size_t> msa_reference_column;
    std::optional<std::size_t> msa_time_column;
    std::optional<double> msa_reference_value;
    std::string msa_mode = "type1";
    std::optional<std::size_t> reliability_time_column;
    std::optional<std::size_t> reliability_event_column;
    std::optional<std::size_t> reliability_group_column;
    std::string reliability_model = "kaplan_meier";
    double power_effect_size = 0.5;
    double power_target = 0.8;
    double power_alpha = 0.05;
    std::size_t power_sample_size = 0;
    std::string power_mode = "one_sample_sample_size";
    std::size_t power_group_count = 3;
    double power_null_proportion = 0.5;
    double power_second_proportion = 0.7;
    std::string power_variance_method = "pooled";
    std::size_t seasonal_period = 1;
    std::string seasonal_error_model = "additive";
    std::string seasonal_trend_model = "additive";
    bool seasonal_damped_trend = false;
    double seasonal_beta = 0.1;
    double seasonal_damping_phi = 0.98;
    std::size_t validation_initial_size = 0;
    std::size_t validation_horizon = 1;
    std::size_t validation_step = 1;
    std::vector<std::size_t> pca_variable_columns;
    std::string pca_mode = "covariance";
    std::size_t pca_component_count = 0;
    double pca_anomaly_quantile = 0.99;
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
};

enum class PlotKind {
    control,
    histogram,
    boxplot,
    pareto,
    probability,
    scatter
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

struct PlotSeries {
    PlotSeriesRole role = PlotSeriesRole::generic;
    std::string label;
    std::vector<double> values;
    std::vector<double> x_values;
    std::vector<double> lower;
    std::vector<double> upper;
    double line_width = 1.8;
    bool show_points = false;
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
    std::vector<double> values;
    std::vector<double> x_values;
    std::vector<double> center;
    std::vector<double> lower;
    std::vector<double> upper;
    std::vector<PlotSeries> series;
    std::vector<std::size_t> source_rows;
    std::vector<std::vector<std::size_t>> special_cause_points;
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
};

struct InterpretationSection {
    std::string heading;
    std::vector<std::string> bullets;
    DiagnosticMessage::Severity severity = DiagnosticMessage::Severity::info;
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
};

}  // namespace datalab::domain
