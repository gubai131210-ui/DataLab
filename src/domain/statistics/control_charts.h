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

struct ControlChartResult {
    std::vector<double> plotted_values;
    std::vector<double> center_line;
    std::vector<double> lower_control_limit;
    std::vector<double> upper_control_limit;
    std::vector<std::size_t> test1_points;
    std::vector<std::vector<std::size_t>> special_cause_points;
    std::vector<double> standardized_values;
    std::vector<double> moving_ranges;
    double sigma_z = 0.0;
    std::vector<DiagnosticMessage> diagnostics;
};

struct IndividualsMovingRangeOptions {
    int moving_range_length = 2;
    SigmaEstimateMethod method = SigmaEstimateMethod::average_moving_range;
    std::vector<std::size_t> omit_from_estimate;
    std::optional<double> historical_mean;
    std::optional<double> historical_sigma;
};

struct DualControlChartResult {
    ControlChartResult primary;
    ControlChartResult secondary;
    double sigma = 0.0;
    double average_moving_range = 0.0;
    std::vector<DiagnosticMessage> diagnostics;
};

struct LaneyChartOptions {
    std::vector<int> enabled_special_cause_tests = {1};
    std::optional<double> historical_center;
    std::optional<double> historical_sigma_z;
};

struct EwmaOptions {
    double lambda = 0.2;
    double limit_sigma = 3.0;
    std::optional<double> historical_mean;
    std::optional<double> historical_sigma;
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
        const std::vector<std::vector<double>>& subgroups);

    static DualControlChartResult xbar_s_dual(
        const std::vector<std::vector<double>>& subgroups);

    static ControlChartResult p_chart(
        const std::vector<std::size_t>& defectives,
        const std::vector<std::size_t>& inspected);

    static ControlChartResult np_chart(
        const std::vector<std::size_t>& defectives,
        const std::vector<std::size_t>& inspected);

    static ControlChartResult c_chart(
        const std::vector<std::size_t>& defects,
        std::size_t constant_units);

    static ControlChartResult u_chart(
        const std::vector<std::size_t>& defects,
        const std::vector<std::size_t>& units);

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
};

std::vector<std::vector<double>> build_subgroups(
    const std::vector<double>& observations,
    int subgroup_size);

std::vector<std::vector<double>> build_subgroups_by_label(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& labels);

}  // namespace datalab::domain::statistics
