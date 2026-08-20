#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class SigmaEstimateMethod {
    average_moving_range,
    median_moving_range
};

enum class ControlChartKind {
    individuals,
    moving_range,
    xbar,
    range,
    stdev,
    attribute,
    laney,
    ewma,
    cusum,
    g,
    t
};

struct SpecialCauseTestSpec {
    int number = 0;
    int default_k = 0;
    const char* short_name = "";
    const char* description = "";
};

struct SpecialCauseSelection {
    std::vector<int> enabled_tests;
    std::string policy;
};

struct ControlChartResult {
    std::vector<double> plotted_values;
    std::vector<double> center_line;
    std::vector<double> lower_control_limit;
    std::vector<double> upper_control_limit;
    std::vector<double> point_sigma;
    std::vector<std::size_t> test1_points;
    std::vector<std::vector<std::size_t>> special_cause_points;
    std::vector<double> standardized_values;
    std::vector<double> moving_ranges;
    std::vector<std::vector<int>> triggered_tests;
    std::vector<int> primary_test_by_point;
    std::vector<int> signal_direction;
    std::vector<std::string> phase_labels;
    std::vector<RowId> source_rows;
    double sigma_z = 0.0;
    std::vector<DiagnosticMessage> diagnostics;
};

struct IndividualsMovingRangeOptions {
    int moving_range_length = 2;
    SigmaEstimateMethod method = SigmaEstimateMethod::average_moving_range;
    std::vector<std::size_t> omit_from_estimate;
    std::optional<double> historical_mean;
    std::optional<double> historical_sigma;
    std::vector<std::string> phase_labels;
    SpecialCauseSelection special_causes;
};

struct DualControlChartResult {
    ControlChartResult primary;
    ControlChartResult secondary;
    double sigma = 0.0;
    double average_moving_range = 0.0;
    std::vector<DiagnosticMessage> diagnostics;
};

struct ImrRsChartResult {
    ControlChartResult individuals;
    ControlChartResult moving_range;
    ControlChartResult within;
    std::string within_chart;
    double sigma_within = 0.0;
    double sigma_xbar = 0.0;
    double sigma_between = 0.0;
    double sigma_between_within = 0.0;
    bool between_variance_truncated = false;
    std::string method;
    std::vector<DiagnosticMessage> diagnostics;
};

struct LaneyChartOptions {
    std::vector<int> enabled_special_cause_tests;
    std::string special_cause_rule_policy = "default_all_applicable";
    std::optional<double> historical_center;
    std::optional<double> historical_sigma_z;
    std::vector<std::string> phase_labels;
};

struct EwmaOptions {
    double lambda = 0.2;
    double limit_sigma = 3.0;
    std::optional<double> historical_mean;
    std::optional<double> historical_sigma;
    SpecialCauseSelection special_causes;
};

struct CusumOptions {
    double target = 0.0;
    double sigma = 1.0;
    double k = 0.5;
    double h = 4.0;
    bool fast_initial_response = false;
};

struct TimeWeightedControlChartResult {
    ControlChartResult primary;
    ControlChartResult secondary;
    std::vector<std::size_t> upper_signal_points;
    std::vector<std::size_t> lower_signal_points;
    std::vector<DiagnosticMessage> diagnostics;
};

class ControlCharts final {
public:
    static ControlChartResult individuals_moving_range(
        const std::vector<double>& observations);

    static DualControlChartResult individuals_moving_range_dual(
        const std::vector<double>& observations,
        const IndividualsMovingRangeOptions& options = {});

    static ControlChartResult xbar_range(
        const std::vector<std::vector<double>>& subgroups);

    static DualControlChartResult xbar_range_dual(
        const std::vector<std::vector<double>>& subgroups,
        const SpecialCauseSelection& special_causes = {});

    static DualControlChartResult xbar_s_dual(
        const std::vector<std::vector<double>>& subgroups,
        const SpecialCauseSelection& special_causes = {});

    static ImrRsChartResult imr_rs_triple(
        const std::vector<std::vector<double>>& subgroups,
        const SpecialCauseSelection& special_causes = {});

    static ControlChartResult p_chart(
        const std::vector<std::size_t>& defectives,
        const std::vector<std::size_t>& inspected,
        const SpecialCauseSelection& special_causes = {});

    static ControlChartResult np_chart(
        const std::vector<std::size_t>& defectives,
        const std::vector<std::size_t>& inspected,
        const SpecialCauseSelection& special_causes = {});

    static ControlChartResult c_chart(
        const std::vector<std::size_t>& defects,
        std::size_t constant_units,
        const SpecialCauseSelection& special_causes = {});

    static ControlChartResult u_chart(
        const std::vector<std::size_t>& defects,
        const std::vector<std::size_t>& units,
        const SpecialCauseSelection& special_causes = {});

    static ControlChartResult laney_p_chart(
        const std::vector<std::size_t>& defectives,
        const std::vector<std::size_t>& inspected,
        const LaneyChartOptions& options = {});

    static ControlChartResult laney_u_chart(
        const std::vector<std::size_t>& defects,
        const std::vector<std::size_t>& units,
        const LaneyChartOptions& options = {});

    static ControlChartResult ewma_chart(
        const std::vector<double>& observations,
        const EwmaOptions& options = {});

    static TimeWeightedControlChartResult cusum_chart(
        const std::vector<double>& observations,
        const CusumOptions& options = {});

    static ControlChartResult g_chart(
        const std::vector<double>& intervals,
        const std::vector<RowId>& source_rows = {},
        const SpecialCauseSelection& special_causes = {});

    static ControlChartResult t_chart(
        const std::vector<double>& intervals,
        const std::vector<RowId>& source_rows = {},
        const SpecialCauseSelection& special_causes = {});
};

std::vector<std::vector<double>> build_subgroups(
    const std::vector<double>& observations,
    int subgroup_size);

std::vector<std::vector<double>> build_subgroups_by_label(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& labels);

const std::vector<SpecialCauseTestSpec>& all_special_cause_tests();
std::vector<int> applicable_special_cause_tests(ControlChartKind kind);
std::vector<int> default_special_cause_tests(ControlChartKind kind);
ControlChartKind control_chart_kind_from_name(const std::string& name);
std::string control_chart_kind_name(ControlChartKind kind);
SpecialCauseSelection special_cause_selection_from_configuration(
    const std::vector<int>& enabled_tests,
    const std::string& policy);
std::vector<int> parse_special_cause_tests(const std::string& text);
std::string format_special_cause_tests(const std::vector<int>& tests);
std::vector<int> resolve_special_cause_tests(
    const SpecialCauseSelection& selection,
    ControlChartKind kind,
    std::vector<DiagnosticMessage>* diagnostics = nullptr);
void apply_special_cause_tests(
    ControlChartResult& result,
    ControlChartKind kind,
    const SpecialCauseSelection& selection = {});

struct WithinSubgroupSigmaEstimate {
    bool ok = false;
    double sigma = 0.0;
    std::string method;
    std::string chart;
    std::string error_code;
    std::string error_message;
};

WithinSubgroupSigmaEstimate estimate_within_subgroup_sigma(
    const std::vector<std::vector<double>>& subgroups);

}  // namespace datalab::domain::statistics
