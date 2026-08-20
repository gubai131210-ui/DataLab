#include "domain/statistics/control_charts.h"

#include "domain/statistics/reliability.h"
#include "domain/statistics/spc_constants.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>
#include <unordered_map>

namespace datalab::domain::statistics {
namespace {

void add_error(std::vector<DiagnosticMessage>& diagnostics, const char* code, const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

void add_warning(
    std::vector<DiagnosticMessage>& diagnostics,
    const std::string& code,
    const std::string& message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::warning, code, message});
}

double mean(const std::vector<double>& values)
{
    return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

double median_of(std::vector<double> values)
{
    std::sort(values.begin(), values.end());
    const std::size_t count = values.size();
    if (count % 2 == 1) {
        return values[count / 2];
    }
    return 0.5 * (values[count / 2 - 1] + values[count / 2]);
}

double c4(std::size_t subgroup_size)
{
    if (subgroup_size < 2) {
        return 0.0;
    }
    const double n = static_cast<double>(subgroup_size);
    return std::sqrt(2.0 / (n - 1.0))
        * std::tgamma(n / 2.0) / std::tgamma((n - 1.0) / 2.0);
}

bool omitted(const std::vector<std::size_t>& omit, std::size_t index)
{
    return std::find(omit.begin(), omit.end(), index) != omit.end();
}

bool usable_point(const ControlChartResult& result, std::size_t index)
{
    return index < result.plotted_values.size()
        && index < result.center_line.size()
        && std::isfinite(result.plotted_values[index])
        && std::isfinite(result.center_line[index]);
}

bool same_segment(const ControlChartResult& result, std::size_t left, std::size_t right)
{
    if (!usable_point(result, left) || !usable_point(result, right)) {
        return false;
    }
    if (!result.phase_labels.empty()) {
        if (left >= result.phase_labels.size() || right >= result.phase_labels.size()
            || result.phase_labels[left] != result.phase_labels[right]) {
            return false;
        }
    }
    return true;
}

bool contiguous_window(const ControlChartResult& result, std::size_t start, std::size_t count)
{
    if (start + count > result.plotted_values.size()) {
        return false;
    }
    for (std::size_t index = start; index < start + count; ++index) {
        if (!usable_point(result, index)) {
            return false;
        }
        if (index > start && !same_segment(result, index - 1, index)) {
            return false;
        }
    }
    return true;
}

double sigma_at(const ControlChartResult& result, std::size_t index)
{
    if (index < result.point_sigma.size() && result.point_sigma[index] > 0.0
        && std::isfinite(result.point_sigma[index])) {
        return result.point_sigma[index];
    }
    if (index >= result.center_line.size() || index >= result.upper_control_limit.size()) {
        return 0.0;
    }
    const double sigma = (result.upper_control_limit[index] - result.center_line[index]) / 3.0;
    return std::isfinite(sigma) && sigma > 0.0 ? sigma : 0.0;
}

int side_at(const ControlChartResult& result, std::size_t index)
{
    const double difference = result.plotted_values[index] - result.center_line[index];
    return difference > 0.0 ? 1 : (difference < 0.0 ? -1 : 0);
}

void record_window(ControlChartResult& result, int test, std::size_t start, std::size_t count)
{
    if (test < 1 || test > 8) {
        return;
    }
    for (std::size_t index = start; index < start + count; ++index) {
        result.special_cause_points[static_cast<std::size_t>(test - 1)].push_back(index);
    }
}

bool enabled_test(const std::vector<int>& tests, int test)
{
    return std::find(tests.cbegin(), tests.cend(), test) != tests.cend();
}

void mark_special_cause_tests(
    ControlChartResult& result,
    const std::vector<int>& enabled_tests)
{
    result.special_cause_points.assign(8, {});
    result.test1_points.clear();
    const std::size_t count = result.plotted_values.size();
    if (enabled_test(enabled_tests, 1)) {
        for (std::size_t index = 0; index < count; ++index) {
            if (!usable_point(result, index)
                || index >= result.lower_control_limit.size()
                || index >= result.upper_control_limit.size()
                || !std::isfinite(result.lower_control_limit[index])
                || !std::isfinite(result.upper_control_limit[index])) {
                continue;
            }
            if (result.plotted_values[index] < result.lower_control_limit[index]
                || result.plotted_values[index] > result.upper_control_limit[index]) {
                result.special_cause_points[0].push_back(index);
                result.test1_points.push_back(index);
            }
        }
    }
    if (enabled_test(enabled_tests, 2)) {
        int side = 0;
        std::size_t run_start = 0;
        for (std::size_t index = 0; index < count; ++index) {
            if (!usable_point(result, index)
                || (index > 0 && !same_segment(result, index - 1, index))) {
                side = 0;
                continue;
            }
            const int current_side = side_at(result, index);
            if (current_side == 0) {
                side = 0;
                continue;
            }
            if (current_side != side) {
                side = current_side;
                run_start = index;
            }
            if (index - run_start + 1 >= 9) {
                record_window(result, 2, run_start, index - run_start + 1);
            }
        }
    }
    if (enabled_test(enabled_tests, 3) && count >= 6) {
        for (std::size_t start = 0; start + 6 <= count; ++start) {
            if (!contiguous_window(result, start, 6)) {
                continue;
            }
            bool increasing = true;
            bool decreasing = true;
            for (std::size_t index = start + 1; index < start + 6; ++index) {
                increasing = increasing
                    && result.plotted_values[index] > result.plotted_values[index - 1];
                decreasing = decreasing
                    && result.plotted_values[index] < result.plotted_values[index - 1];
            }
            if (increasing || decreasing) {
                record_window(result, 3, start, 6);
            }
        }
    }
    if (enabled_test(enabled_tests, 4) && count >= 14) {
        for (std::size_t start = 0; start + 14 <= count; ++start) {
            if (!contiguous_window(result, start, 14)) {
                continue;
            }
            bool alternating = true;
            for (std::size_t index = start + 1; index < start + 14; ++index) {
                const double step =
                    result.plotted_values[index] - result.plotted_values[index - 1];
                if (index == start + 1) {
                    alternating = step != 0.0;
                    continue;
                }
                const double previous_step =
                    result.plotted_values[index - 1] - result.plotted_values[index - 2];
                alternating = alternating && previous_step * step < 0.0;
            }
            if (alternating) {
                record_window(result, 4, start, 14);
            }
        }
    }
    if (enabled_test(enabled_tests, 5) && count >= 3) {
        for (std::size_t start = 0; start + 3 <= count; ++start) {
            if (!contiguous_window(result, start, 3)) {
                continue;
            }
            for (const int side : {-1, 1}) {
                std::size_t outside = 0;
                for (std::size_t index = start; index < start + 3; ++index) {
                    const double sigma = sigma_at(result, index);
                    outside += side_at(result, index) == side && sigma > 0.0
                        && side * (result.plotted_values[index]
                            - result.center_line[index]) > 2.0 * sigma;
                }
                if (outside >= 2) {
                    record_window(result, 5, start, 3);
                }
            }
        }
    }
    if (enabled_test(enabled_tests, 6) && count >= 5) {
        for (std::size_t start = 0; start + 5 <= count; ++start) {
            if (!contiguous_window(result, start, 5)) {
                continue;
            }
            for (const int side : {-1, 1}) {
                std::size_t outside = 0;
                for (std::size_t index = start; index < start + 5; ++index) {
                    const double sigma = sigma_at(result, index);
                    outside += side_at(result, index) == side && sigma > 0.0
                        && side * (result.plotted_values[index]
                            - result.center_line[index]) > sigma;
                }
                if (outside >= 4) {
                    record_window(result, 6, start, 5);
                }
            }
        }
    }
    if (enabled_test(enabled_tests, 7) && count >= 15) {
        for (std::size_t start = 0; start + 15 <= count; ++start) {
            if (!contiguous_window(result, start, 15)) {
                continue;
            }
            bool inside = true;
            for (std::size_t index = start; index < start + 15; ++index) {
                const double sigma = sigma_at(result, index);
                inside = inside && sigma > 0.0
                    && std::abs(result.plotted_values[index]
                        - result.center_line[index]) < sigma;
            }
            if (inside) {
                record_window(result, 7, start, 15);
            }
        }
    }
    if (enabled_test(enabled_tests, 8) && count >= 8) {
        for (std::size_t start = 0; start + 8 <= count; ++start) {
            if (!contiguous_window(result, start, 8)) {
                continue;
            }
            bool outside = true;
            for (std::size_t index = start; index < start + 8; ++index) {
                const double sigma = sigma_at(result, index);
                outside = outside && sigma > 0.0 && side_at(result, index) != 0
                    && std::abs(result.plotted_values[index]
                        - result.center_line[index]) > sigma;
            }
            if (outside) {
                record_window(result, 8, start, 8);
            }
        }
    }
    for (auto& points : result.special_cause_points) {
        std::sort(points.begin(), points.end());
        points.erase(std::unique(points.begin(), points.end()), points.end());
    }
    result.triggered_tests.assign(result.plotted_values.size(), {});
    result.primary_test_by_point.assign(result.plotted_values.size(), 0);
    for (std::size_t test = 0; test < result.special_cause_points.size(); ++test) {
        for (const std::size_t point : result.special_cause_points[test]) {
            if (point >= result.triggered_tests.size()) {
                continue;
            }
            result.triggered_tests[point].push_back(static_cast<int>(test + 1));
            if (result.primary_test_by_point[point] == 0) {
                result.primary_test_by_point[point] = static_cast<int>(test + 1);
            }
        }
    }
    static const char* messages[] = {
        "检测到 1 点超出 3σ 控制限，建议复核该观测、测量和记录过程。",
        "检测到连续 9 点位于中心线同侧，建议复核阶段、设备或批次因素。",
        "检测到连续 6 点单调上升或下降，建议复核趋势、刀具磨损或过程漂移。",
        "检测到连续 14 点上下交替，建议复核系统性周期或两台设备交替影响。",
        "检测到 3 点中有 2 点同侧超过 2σ，提示可能存在较小的过程偏移。",
        "检测到 5 点中有 4 点同侧超过 1σ，提示可能存在较小的过程偏移。",
        "检测到连续 15 点落在 1σ 以内，控制限可能过宽或数据存在分层。",
        "检测到连续 8 点落在 1σ 以外，提示可能存在混合总体或双群模式。"
    };
    for (std::size_t test = 0; test < result.special_cause_points.size(); ++test) {
        if (result.special_cause_points[test].empty()) {
            continue;
        }
        add_warning(
            result.diagnostics,
            "test" + std::to_string(test + 1),
            messages[test]);
    }
}

ControlChartResult laney_chart(
    const std::vector<std::size_t>& counts,
    const std::vector<std::size_t>& denominators,
    const LaneyChartOptions& options,
    bool proportion_chart)
{
    ControlChartResult result;
    if (counts.empty() || counts.size() != denominators.size()) {
        add_error(result.diagnostics, "invalid_counts",
                  "Laney chart count arrays must have equal non-zero length.");
        return result;
    }
    std::size_t total_count = 0;
    std::size_t total_denominator = 0;
    for (std::size_t index = 0; index < counts.size(); ++index) {
        if (denominators[index] == 0 || (proportion_chart && counts[index] > denominators[index])) {
            add_error(result.diagnostics, "invalid_count",
                      "Laney chart contains an invalid count or denominator.");
            return result;
        }
        total_count += counts[index];
        total_denominator += denominators[index];
    }
    if (total_denominator == 0) {
        add_error(result.diagnostics, "invalid_denominator",
                  "Laney chart requires a positive total denominator.");
        return result;
    }
    const double center = options.historical_center.value_or(
        static_cast<double>(total_count) / static_cast<double>(total_denominator));
    if (!(center >= 0.0) || (proportion_chart && center > 1.0)) {
        add_error(result.diagnostics, "invalid_historical_center",
                  "Historical center line is outside the valid range.");
        return result;
    }
    result.plotted_values.reserve(counts.size());
    result.center_line.assign(counts.size(), center);
    result.lower_control_limit.resize(counts.size());
    result.upper_control_limit.resize(counts.size());
    result.standardized_values.assign(counts.size(), 0.0);
    result.moving_ranges.assign(counts.size(), 0.0);
    for (std::size_t index = 0; index < counts.size(); ++index) {
        const double denominator = static_cast<double>(denominators[index]);
        const double value = static_cast<double>(counts[index]) / denominator;
        const double variance = proportion_chart
            ? center * (1.0 - center) / denominator : center / denominator;
        result.plotted_values.push_back(value);
        if (variance > 0.0) {
            result.standardized_values[index] = (value - center) / std::sqrt(variance);
        }
        if (index > 0) {
            result.moving_ranges[index] = std::abs(
                result.standardized_values[index] - result.standardized_values[index - 1]);
        }
    }
    std::vector<double> moving_ranges(
        result.moving_ranges.begin() + 1, result.moving_ranges.end());
    const double average_moving_range = moving_ranges.empty()
        ? 0.0 : mean(moving_ranges);
    result.sigma_z = options.historical_sigma_z.value_or(average_moving_range / 1.128);
    if (!(result.sigma_z >= 0.0) || !std::isfinite(result.sigma_z)) {
        add_error(result.diagnostics, "invalid_sigma_z", "Sigma Z must be finite and non-negative.");
        return result;
    }
    for (std::size_t index = 0; index < counts.size(); ++index) {
        const double denominator = static_cast<double>(denominators[index]);
        const double deviation = proportion_chart
            ? 3.0 * result.sigma_z * std::sqrt(center * (1.0 - center) / denominator)
            : 3.0 * result.sigma_z * std::sqrt(center / denominator);
        result.lower_control_limit[index] = std::max(0.0, center - deviation);
        result.upper_control_limit[index] = proportion_chart
            ? std::min(1.0, center + deviation) : center + deviation;
    }
    result.phase_labels = options.phase_labels;
    mark_special_cause_tests(result, resolve_special_cause_tests(
        special_cause_selection_from_configuration(
            options.enabled_special_cause_tests, options.special_cause_rule_policy),
        ControlChartKind::laney,
        &result.diagnostics));
    if (result.sigma_z > 1.0 + 1.0e-9) {
        add_warning(result.diagnostics, "overdispersion",
                    "Sigma Z > 1: traditional control limits may be too narrow.");
    } else if (result.sigma_z < 1.0 - 1.0e-9) {
        add_warning(result.diagnostics, "underdispersion",
                    "Sigma Z < 1: traditional control limits may be too wide.");
    }
    return result;
}

}  // namespace

ControlChartResult ControlCharts::individuals_moving_range(
    const std::vector<double>& observations)
{
    return individuals_moving_range_dual(observations, {}).primary;
}

DualControlChartResult ControlCharts::individuals_moving_range_dual(
    const std::vector<double>& observations,
    const IndividualsMovingRangeOptions& options)
{
    DualControlChartResult result;
    const int length = std::max(2, options.moving_range_length);
    result.primary.plotted_values = observations;
    if (observations.size() < static_cast<std::size_t>(length)) {
        add_error(result.diagnostics, "insufficient_data", "I-MR requires enough observations for the moving range.");
        result.primary.diagnostics = result.diagnostics;
        return result;
    }
    if (!std::all_of(observations.begin(), observations.end(),
                     [](double value) { return std::isfinite(value); })) {
        add_error(result.diagnostics, "non_finite_input",
                  "I-MR 不允许 NaN 或无穷观测。");
        result.primary.diagnostics = result.diagnostics;
        return result;
    }

    std::vector<double> estimate_values;
    for (std::size_t index = 0; index < observations.size(); ++index) {
        if (!omitted(options.omit_from_estimate, index)) {
            estimate_values.push_back(observations[index]);
        }
    }
    if (estimate_values.size() < 2) {
        add_error(result.diagnostics, "insufficient_estimate", "Too few observations remain to estimate parameters.");
        result.primary.diagnostics = result.diagnostics;
        return result;
    }

    const double center = options.historical_mean.value_or(mean(estimate_values));
    std::vector<double> moving_ranges;
    std::vector<double> plotted_ranges(observations.size(), 0.0);
    for (std::size_t index = static_cast<std::size_t>(length - 1); index < observations.size(); ++index) {
        double range = 0.0;
        for (int offset = 0; offset < length; ++offset) {
            const double value = observations[index - static_cast<std::size_t>(offset)];
            if (offset == 0) {
                range = 0.0;
            }
            const std::size_t start = index - static_cast<std::size_t>(length - 1);
            double minimum = observations[start];
            double maximum = observations[start];
            for (std::size_t inner = start; inner <= index; ++inner) {
                minimum = std::min(minimum, observations[inner]);
                maximum = std::max(maximum, observations[inner]);
            }
            range = maximum - minimum;
            break;
        }
        plotted_ranges[index] = range;
        bool range_omitted = false;
        bool crosses_phase = false;
        if (!options.phase_labels.empty()
            && options.phase_labels.size() == observations.size()) {
            const std::size_t start = index - static_cast<std::size_t>(length - 1);
            for (std::size_t inner = start; inner <= index; ++inner) {
                if (options.phase_labels[inner] != options.phase_labels[start]) {
                    crosses_phase = true;
                    break;
                }
            }
        }
        if (crosses_phase) {
            plotted_ranges[index] = std::numeric_limits<double>::quiet_NaN();
            range_omitted = true;
        }
        for (int offset = 0; offset < length && !range_omitted; ++offset) {
            if (omitted(options.omit_from_estimate, index - static_cast<std::size_t>(offset))) {
                range_omitted = true;
                break;
            }
        }
        if (!range_omitted) {
            moving_ranges.push_back(range);
        }
    }
    if (moving_ranges.empty()) {
        add_error(result.diagnostics, "insufficient_ranges", "No moving ranges remain for estimation.");
        result.primary.diagnostics = result.diagnostics;
        return result;
    }

    const double range_center = options.method == SigmaEstimateMethod::median_moving_range
        ? median_of(moving_ranges)
        : mean(moving_ranges);
    result.average_moving_range = range_center;
    const std::optional<double> unbias = options.method == SigmaEstimateMethod::median_moving_range
        ? SpcConstants::median_moving_range_constant(static_cast<std::size_t>(length))
        : SpcConstants::d2(static_cast<std::size_t>(length));
    if (!unbias.has_value()) {
        add_error(result.diagnostics, "unsupported_length", "Moving range length is outside the supported constant table.");
        result.primary.diagnostics = result.diagnostics;
        return result;
    }
    result.sigma = options.historical_sigma.value_or(range_center / *unbias);
    const double lower = center - 3.0 * result.sigma;
    const double upper = center + 3.0 * result.sigma;
    result.primary.center_line.assign(observations.size(), center);
    result.primary.lower_control_limit.assign(observations.size(), lower);
    result.primary.upper_control_limit.assign(observations.size(), upper);
    result.primary.point_sigma.assign(observations.size(), result.sigma);
    if (!options.phase_labels.empty()
        && options.phase_labels.size() == observations.size()) {
        result.primary.phase_labels = options.phase_labels;
        result.secondary.phase_labels = options.phase_labels;
    }
    apply_special_cause_tests(result.primary, ControlChartKind::individuals, options.special_causes);
    const std::optional<double> d3_limit = SpcConstants::d3_limit(static_cast<std::size_t>(length));
    const std::optional<double> d4 = SpcConstants::d4(static_cast<std::size_t>(length));
    result.secondary.plotted_values = plotted_ranges;
    result.secondary.center_line.assign(observations.size(), range_center);
    result.secondary.lower_control_limit.assign(
        observations.size(), d3_limit.value_or(0.0) * range_center);
    result.secondary.upper_control_limit.assign(
        observations.size(), d4.value_or(3.267) * range_center);
    for (std::size_t index = 0; index < static_cast<std::size_t>(length - 1)
         && index < result.secondary.plotted_values.size(); ++index) {
        result.secondary.plotted_values[index] = range_center;
    }
    apply_special_cause_tests(result.secondary, ControlChartKind::moving_range, options.special_causes);
    result.primary.diagnostics.insert(
        result.primary.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
    return result;
}

ControlChartResult ControlCharts::xbar_range(
    const std::vector<std::vector<double>>& subgroups)
{
    return xbar_range_dual(subgroups).primary;
}

DualControlChartResult ControlCharts::xbar_range_dual(
    const std::vector<std::vector<double>>& subgroups,
    const SpecialCauseSelection& special_causes)
{
    DualControlChartResult result;
    if (subgroups.empty()) {
        add_error(result.diagnostics, "empty_subgroups", "Xbar-R requires at least one subgroup.");
        result.primary.diagnostics = result.diagnostics;
        return result;
    }

    std::vector<double> ranges;
    const std::size_t subgroup_size = subgroups.front().size();
    for (const auto& subgroup : subgroups) {
        if (subgroup.size() < 2) {
            add_error(result.diagnostics, "invalid_subgroup", "Each Xbar-R subgroup needs at least two values.");
            result.primary.diagnostics = result.diagnostics;
            return result;
        }
        if (subgroup.size() != subgroup_size) {
            add_error(result.diagnostics, "unbalanced_design",
                      "Xbar-R 要求各组样本量相等，不能用最后一组的 d2/A2 代替。");
            result.primary.diagnostics = result.diagnostics;
            return result;
        }
        if (!std::all_of(subgroup.begin(), subgroup.end(),
                         [](double value) { return std::isfinite(value); })) {
            add_error(result.diagnostics, "non_finite_input",
                      "Xbar-R 子组不允许 NaN 或无穷观测。");
            result.primary.diagnostics = result.diagnostics;
            return result;
        }
        const auto [minimum, maximum] =
            std::minmax_element(subgroup.begin(), subgroup.end());
        result.primary.plotted_values.push_back(mean(subgroup));
        ranges.push_back(*maximum - *minimum);
    }

    const std::optional<double> d2 = SpcConstants::d2(subgroup_size);
    const std::optional<double> a2 = SpcConstants::a2(subgroup_size);
    const std::optional<double> d3_limit = SpcConstants::d3_limit(subgroup_size);
    const std::optional<double> d4 = SpcConstants::d4(subgroup_size);
    if (!d2.has_value() || !a2.has_value() || !d4.has_value()) {
        add_error(result.diagnostics, "unsupported_size", "Subgroup size is outside the SPC constant table (2-25).");
        result.primary.diagnostics = result.diagnostics;
        return result;
    }

    const double xbar = mean(result.primary.plotted_values);
    const double average_range = mean(ranges);
    result.average_moving_range = average_range;
    result.sigma = average_range / *d2;
    result.primary.center_line.assign(result.primary.plotted_values.size(), xbar);
    result.primary.lower_control_limit.assign(
        result.primary.plotted_values.size(), xbar - (*a2) * average_range);
    result.primary.upper_control_limit.assign(
        result.primary.plotted_values.size(), xbar + (*a2) * average_range);
    apply_special_cause_tests(result.primary, ControlChartKind::xbar, special_causes);

    result.secondary.plotted_values = ranges;
    result.secondary.center_line.assign(ranges.size(), average_range);
    result.secondary.lower_control_limit.assign(ranges.size(), d3_limit.value_or(0.0) * average_range);
    result.secondary.upper_control_limit.assign(ranges.size(), (*d4) * average_range);
    apply_special_cause_tests(result.secondary, ControlChartKind::range, special_causes);
    result.primary.diagnostics.insert(
        result.primary.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
    return result;
}

DualControlChartResult ControlCharts::xbar_s_dual(
    const std::vector<std::vector<double>>& subgroups,
    const SpecialCauseSelection& special_causes)
{
    DualControlChartResult result;
    if (subgroups.empty()) {
        add_error(result.diagnostics, "empty_subgroups", "Xbar-S requires at least one subgroup.");
        result.primary.diagnostics = result.diagnostics;
        return result;
    }
    const std::size_t subgroup_size = subgroups.front().size();
    if (subgroup_size < 2) {
        add_error(result.diagnostics, "invalid_subgroup", "Each Xbar-S subgroup needs at least two values.");
        result.primary.diagnostics = result.diagnostics;
        return result;
    }
    const double c4_value = c4(subgroup_size);
    if (!(c4_value > 0.0)) {
        add_error(result.diagnostics, "invalid_c4", "Could not calculate c4 for the subgroup size.");
        result.primary.diagnostics = result.diagnostics;
        return result;
    }

    std::vector<double> standard_deviations;
    for (const auto& subgroup : subgroups) {
        if (subgroup.size() != subgroup_size) {
            add_error(result.diagnostics, "unequal_subgroup", "Xbar-S requires equal subgroup sizes.");
            result.primary.diagnostics = result.diagnostics;
            return result;
        }
        const double subgroup_mean = mean(subgroup);
        long double sum = 0.0L;
        for (const double value : subgroup) {
            const long double difference = static_cast<long double>(value) - subgroup_mean;
            sum += difference * difference;
        }
        result.primary.plotted_values.push_back(subgroup_mean);
        standard_deviations.push_back(
            std::sqrt(static_cast<double>(sum / static_cast<long double>(subgroup_size - 1))));
    }

    const double xbar = mean(result.primary.plotted_values);
    const double average_s = mean(standard_deviations);
    const double sigma = average_s / c4_value;
    const double a3 = 3.0 / (c4_value * std::sqrt(static_cast<double>(subgroup_size)));
    const double variation = std::sqrt(std::max(0.0, 1.0 - c4_value * c4_value)) / c4_value;
    const double b3 = std::max(0.0, 1.0 - 3.0 * variation);
    const double b4 = 1.0 + 3.0 * variation;

    result.sigma = sigma;
    result.average_moving_range = average_s;
    result.primary.center_line.assign(result.primary.plotted_values.size(), xbar);
    result.primary.lower_control_limit.assign(
        result.primary.plotted_values.size(), xbar - a3 * average_s);
    result.primary.upper_control_limit.assign(
        result.primary.plotted_values.size(), xbar + a3 * average_s);
    apply_special_cause_tests(result.primary, ControlChartKind::xbar, special_causes);

    result.secondary.plotted_values = standard_deviations;
    result.secondary.center_line.assign(standard_deviations.size(), average_s);
    result.secondary.lower_control_limit.assign(
        standard_deviations.size(), b3 * average_s);
    result.secondary.upper_control_limit.assign(
        standard_deviations.size(), b4 * average_s);
    apply_special_cause_tests(result.secondary, ControlChartKind::stdev, special_causes);
    result.primary.diagnostics.insert(
        result.primary.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
    return result;
}

ImrRsChartResult ControlCharts::imr_rs_triple(
    const std::vector<std::vector<double>>& subgroups,
    const SpecialCauseSelection& special_causes)
{
    ImrRsChartResult result;
    if (subgroups.size() < 2) {
        add_error(result.diagnostics, "insufficient_subgroups",
                  "I-MR-R/S 至少需要两个子组。");
        return result;
    }
    const std::size_t subgroup_size = subgroups.front().size();
    if (subgroup_size < 2) {
        add_error(result.diagnostics, "invalid_subgroup_size",
                  "各子组必须至少包含两个观测。");
        return result;
    }
    for (const auto& subgroup : subgroups) {
        if (subgroup.size() != subgroup_size) {
            add_error(result.diagnostics, "unequal_subgroups",
                      "各子组必须具有相同观测数。");
            return result;
        }
        if (!std::all_of(subgroup.begin(), subgroup.end(),
                         [](double value) { return std::isfinite(value); })) {
            add_error(result.diagnostics, "non_finite_input",
                      "I-MR-R/S 子组不允许 NaN 或无穷观测。");
            return result;
        }
    }

    std::vector<double> subgroup_means;
    subgroup_means.reserve(subgroups.size());
    for (const auto& subgroup : subgroups) {
        subgroup_means.push_back(mean(subgroup));
    }

    IndividualsMovingRangeOptions options;
    options.special_causes = special_causes;
    const DualControlChartResult imr =
        individuals_moving_range_dual(subgroup_means, options);
    result.diagnostics.insert(
        result.diagnostics.end(), imr.diagnostics.cbegin(), imr.diagnostics.cend());
    result.individuals = imr.primary;
    result.moving_range = imr.secondary;
    result.sigma_xbar = imr.sigma;

    const WithinSubgroupSigmaEstimate within_sigma =
        estimate_within_subgroup_sigma(subgroups);
    if (!within_sigma.ok) {
        add_error(result.diagnostics, within_sigma.error_code.c_str(),
                  within_sigma.error_message.c_str());
        return result;
    }
    const bool use_range = within_sigma.chart == "range";
    const DualControlChartResult within = use_range
        ? xbar_range_dual(subgroups, special_causes)
        : xbar_s_dual(subgroups, special_causes);
    result.diagnostics.insert(
        result.diagnostics.end(), within.diagnostics.cbegin(), within.diagnostics.cend());
    result.within = within.secondary;
    result.within_chart = within_sigma.chart;
    result.sigma_within = within_sigma.sigma;

    const double n = static_cast<double>(subgroup_size);
    const double raw_between_variance =
        result.sigma_xbar * result.sigma_xbar
        - result.sigma_within * result.sigma_within / n;
    result.between_variance_truncated = raw_between_variance < 0.0;
    const double between_variance =
        result.between_variance_truncated ? 0.0 : raw_between_variance;
    result.sigma_between = std::sqrt(between_variance);
    result.sigma_between_within = std::sqrt(
        between_variance + result.sigma_within * result.sigma_within);
    result.method = within_sigma.method;
    if (result.between_variance_truncated) {
        add_warning(result.diagnostics, "between_variance_truncated",
                    "估计的组间方差为负，已截断为 0；σ_B 可能低估。");
    }
    return result;
}

ControlChartResult ControlCharts::p_chart(
    const std::vector<std::size_t>& defectives,
    const std::vector<std::size_t>& inspected,
    const SpecialCauseSelection& special_causes)
{
    ControlChartResult result;
    if (defectives.empty() || defectives.size() != inspected.size()) {
        add_error(result.diagnostics, "invalid_counts", "P chart count arrays must have equal non-zero length.");
        return result;
    }

    std::size_t total_defectives = 0;
    std::size_t total_inspected = 0;
    for (std::size_t index = 0; index < defectives.size(); ++index) {
        if (inspected[index] == 0 || defectives[index] > inspected[index]) {
            add_error(result.diagnostics, "invalid_count", "Defectives must not exceed inspected units.");
            return result;
        }
        total_defectives += defectives[index];
        total_inspected += inspected[index];
        result.plotted_values.push_back(
            static_cast<double>(defectives[index]) / static_cast<double>(inspected[index]));
    }

    const double pbar = static_cast<double>(total_defectives) / static_cast<double>(total_inspected);
    for (const std::size_t sample_size : inspected) {
        const double sigma = std::sqrt(pbar * (1.0 - pbar) / static_cast<double>(sample_size));
        result.point_sigma.push_back(sigma);
        result.center_line.push_back(pbar);
        result.lower_control_limit.push_back(std::max(0.0, pbar - 3.0 * sigma));
        result.upper_control_limit.push_back(std::min(1.0, pbar + 3.0 * sigma));
    }
    apply_special_cause_tests(result, ControlChartKind::attribute, special_causes);
    return result;
}

ControlChartResult ControlCharts::np_chart(
    const std::vector<std::size_t>& defectives,
    const std::vector<std::size_t>& inspected,
    const SpecialCauseSelection& special_causes)
{
    ControlChartResult result;
    if (defectives.empty() || defectives.size() != inspected.size()) {
        add_error(result.diagnostics, "invalid_counts", "NP chart arrays must have equal non-zero length.");
        return result;
    }
    std::size_t total_defectives = 0;
    std::size_t total_inspected = 0;
    for (std::size_t index = 0; index < defectives.size(); ++index) {
        if (inspected[index] == 0 || defectives[index] > inspected[index]) {
            add_error(result.diagnostics, "invalid_count", "Defectives must not exceed inspected units.");
            return result;
        }
        total_defectives += defectives[index];
        total_inspected += inspected[index];
    }
    const double pbar = static_cast<double>(total_defectives)
        / static_cast<double>(total_inspected);
    const bool equal_size = std::all_of(
        inspected.begin(), inspected.end(),
        [&](std::size_t value) { return value == inspected.front(); });
    if (!equal_size) {
        add_warning(result.diagnostics, "unequal_sample_size",
                    "NP uses the inspected count for each point because sample sizes differ.");
    }
    for (std::size_t index = 0; index < defectives.size(); ++index) {
        const double n = static_cast<double>(inspected[index]);
        const double center = n * pbar;
        const double deviation = 3.0 * std::sqrt(n * pbar * (1.0 - pbar));
        result.plotted_values.push_back(static_cast<double>(defectives[index]));
        result.center_line.push_back(center);
        result.point_sigma.push_back(deviation / 3.0);
        result.lower_control_limit.push_back(std::max(0.0, center - deviation));
        result.upper_control_limit.push_back(center + deviation);
    }
    apply_special_cause_tests(result, ControlChartKind::attribute, special_causes);
    return result;
}

ControlChartResult ControlCharts::c_chart(
    const std::vector<std::size_t>& defects,
    std::size_t constant_units,
    const SpecialCauseSelection& special_causes)
{
    ControlChartResult result;
    if (defects.empty() || constant_units == 0) {
        add_error(result.diagnostics, "invalid_units", "C chart requires non-empty defects and positive units.");
        return result;
    }
    const double center = std::accumulate(defects.begin(), defects.end(), 0.0)
        / static_cast<double>(defects.size());
    const double deviation = 3.0 * std::sqrt(center);
    for (const std::size_t value : defects) {
        result.plotted_values.push_back(static_cast<double>(value));
        result.center_line.push_back(center);
        result.point_sigma.push_back(std::sqrt(center));
        result.lower_control_limit.push_back(std::max(0.0, center - deviation));
        result.upper_control_limit.push_back(center + deviation);
    }
    apply_special_cause_tests(result, ControlChartKind::attribute, special_causes);
    return result;
}

ControlChartResult ControlCharts::u_chart(
    const std::vector<std::size_t>& defects,
    const std::vector<std::size_t>& units,
    const SpecialCauseSelection& special_causes)
{
    ControlChartResult result;
    if (defects.empty() || defects.size() != units.size()) {
        add_error(result.diagnostics, "invalid_counts", "U chart arrays must have equal non-zero length.");
        return result;
    }
    std::size_t total_defects = 0;
    std::size_t total_units = 0;
    for (std::size_t index = 0; index < defects.size(); ++index) {
        if (units[index] == 0) {
            add_error(result.diagnostics, "invalid_units", "U chart units must be positive.");
            return result;
        }
        total_defects += defects[index];
        total_units += units[index];
    }
    const double ubar = static_cast<double>(total_defects) / static_cast<double>(total_units);
    for (std::size_t index = 0; index < defects.size(); ++index) {
        const double n = static_cast<double>(units[index]);
        const double deviation = 3.0 * std::sqrt(ubar / n);
        result.plotted_values.push_back(static_cast<double>(defects[index]) / n);
        result.center_line.push_back(ubar);
        result.point_sigma.push_back(std::sqrt(ubar / n));
        result.lower_control_limit.push_back(std::max(0.0, ubar - deviation));
        result.upper_control_limit.push_back(ubar + deviation);
    }
    apply_special_cause_tests(result, ControlChartKind::attribute, special_causes);
    return result;
}

ControlChartResult ControlCharts::laney_p_chart(
    const std::vector<std::size_t>& defectives,
    const std::vector<std::size_t>& inspected,
    const LaneyChartOptions& options)
{
    return laney_chart(defectives, inspected, options, true);
}

ControlChartResult ControlCharts::laney_u_chart(
    const std::vector<std::size_t>& defects,
    const std::vector<std::size_t>& units,
    const LaneyChartOptions& options)
{
    return laney_chart(defects, units, options, false);
}

ControlChartResult ControlCharts::ewma_chart(
    const std::vector<double>& observations,
    const EwmaOptions& options)
{
    ControlChartResult result;
    if (observations.empty() || options.lambda <= 0.0 || options.lambda > 1.0
        || options.limit_sigma <= 0.0) {
        add_error(result.diagnostics, "invalid_ewma_options",
                  "EWMA 要求非空数据、lambda 位于 (0,1] 且控制限倍数大于 0。");
        return result;
    }
    const double mean = options.historical_mean.value_or(
        std::accumulate(observations.cbegin(), observations.cend(), 0.0)
        / static_cast<double>(observations.size()));
    double sigma = options.historical_sigma.value_or(0.0);
    if (!options.historical_sigma.has_value()) {
        double sum_squared = 0.0;
        for (const double value : observations) {
            sum_squared += (value - mean) * (value - mean);
        }
        sigma = observations.size() > 1
            ? std::sqrt(sum_squared / static_cast<double>(observations.size() - 1)) : 0.0;
    }
    if (!(sigma > 0.0) || !std::isfinite(sigma)) {
        resolve_special_cause_tests(
            options.special_causes, ControlChartKind::ewma, &result.diagnostics);
        add_error(result.diagnostics, "invalid_ewma_sigma",
                  "EWMA 的过程标准差必须大于 0。");
        return result;
    }
    result.plotted_values.resize(observations.size());
    result.center_line.assign(observations.size(), mean);
    result.lower_control_limit.resize(observations.size());
    result.upper_control_limit.resize(observations.size());
    double ewma = mean;
    for (std::size_t index = 0; index < observations.size(); ++index) {
        ewma = options.lambda * observations[index]
            + (1.0 - options.lambda) * ewma;
        result.plotted_values[index] = ewma;
        const double standard_error = sigma * std::sqrt(
            options.lambda / (2.0 - options.lambda)
            * (1.0 - std::pow(1.0 - options.lambda,
                               2.0 * static_cast<double>(index + 1))));
        result.lower_control_limit[index] = mean - options.limit_sigma * standard_error;
        result.upper_control_limit[index] = mean + options.limit_sigma * standard_error;
        result.point_sigma.push_back(standard_error);
    }
    apply_special_cause_tests(result, ControlChartKind::ewma, options.special_causes);
    return result;
}

TimeWeightedControlChartResult ControlCharts::cusum_chart(
    const std::vector<double>& observations,
    const CusumOptions& options)
{
    TimeWeightedControlChartResult result;
    if (observations.empty() || !(options.sigma > 0.0)
        || !(options.h > 0.0) || !(options.k >= 0.0)) {
        add_error(result.diagnostics, "invalid_cusum_options",
                  "CUSUM 要求非空数据、sigma/h 大于 0 且 k 不小于 0。");
        return result;
    }
    const std::size_t count = observations.size();
    result.primary.plotted_values.resize(count);
    result.secondary.plotted_values.resize(count);
    result.primary.center_line.assign(count, 0.0);
    result.secondary.center_line.assign(count, 0.0);
    result.primary.lower_control_limit.assign(count, -options.h * options.sigma);
    result.primary.upper_control_limit.assign(count, options.h * options.sigma);
    result.secondary.lower_control_limit.assign(count, -options.h * options.sigma);
    result.secondary.upper_control_limit.assign(count, options.h * options.sigma);
    result.primary.signal_direction.assign(count, 0);
    result.secondary.signal_direction.assign(count, 0);
    double upper = options.fast_initial_response ? options.h * options.sigma / 2.0 : 0.0;
    double lower = options.fast_initial_response ? options.h * options.sigma / 2.0 : 0.0;
    for (std::size_t index = 0; index < count; ++index) {
        upper = std::max(0.0, upper + observations[index] - options.target
            - options.k * options.sigma);
        lower = std::max(0.0, lower + options.target - observations[index]
            - options.k * options.sigma);
        result.primary.plotted_values[index] = upper;
        result.secondary.plotted_values[index] = -lower;
        if (upper > options.h * options.sigma) {
            result.upper_signal_points.push_back(index);
            result.primary.signal_direction[index] = 1;
        }
        if (lower > options.h * options.sigma) {
            result.lower_signal_points.push_back(index);
            result.secondary.signal_direction[index] = -1;
        }
    }
    return result;
}

std::vector<std::vector<double>> build_subgroups(
    const std::vector<double>& observations,
    int subgroup_size)
{
    std::vector<std::vector<double>> subgroups;
    if (subgroup_size < 2) {
        return subgroups;
    }
    const std::size_t size = static_cast<std::size_t>(subgroup_size);
    for (std::size_t index = 0; index + size <= observations.size(); index += size) {
        subgroups.emplace_back(observations.begin() + static_cast<std::ptrdiff_t>(index),
                               observations.begin() + static_cast<std::ptrdiff_t>(index + size));
    }
    return subgroups;
}

std::vector<std::vector<double>> build_subgroups_by_label(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& labels)
{
    std::vector<std::string> order;
    std::unordered_map<std::string, std::vector<double>> grouped;
    for (std::size_t index = 0; index < observations.size(); ++index) {
        const std::size_t row = index < source_rows.size() ? source_rows[index] : index;
        const std::string label = row < labels.size() ? labels[row] : std::to_string(row);
        if (grouped.find(label) == grouped.end()) {
            order.push_back(label);
        }
        grouped[label].push_back(observations[index]);
    }
    std::vector<std::vector<double>> subgroups;
    for (const std::string& label : order) {
        if (grouped[label].size() >= 2) {
            subgroups.push_back(grouped[label]);
        }
    }
    return subgroups;
}

const std::vector<SpecialCauseTestSpec>& all_special_cause_tests()
{
    static const std::vector<SpecialCauseTestSpec> tests = {
        {1, 3, "1 点超出 3σ", "1 点与中心线的距离严格超过 3σ 控制限。"},
        {2, 9, "9 点同侧", "连续 9 点位于中心线同一侧；中心线上的点会打断该连续段。"},
        {3, 6, "6 点趋势", "连续 6 点严格单调上升或下降；相等点会打断趋势。"},
        {4, 14, "14 点交替", "连续 14 点相邻升降交替；零差分会打断模式。"},
        {5, 2, "3 点中 2 点超 2σ", "连续 3 点中至少 2 点位于中心线同侧且超过 2σ。"},
        {6, 1, "5 点中 4 点超 1σ", "连续 5 点中至少 4 点位于中心线同侧且超过 1σ。"},
        {7, 15, "15 点在 1σ 内", "连续 15 点都落在中心线 ±1σ 以内，允许任一侧。"},
        {8, 8, "8 点在 1σ 外", "连续 8 点都落在 ±1σ 以外，允许全部位于同一侧。"}
    };
    return tests;
}

std::vector<int> applicable_special_cause_tests(ControlChartKind kind)
{
    switch (kind) {
    case ControlChartKind::moving_range:
    case ControlChartKind::range:
    case ControlChartKind::stdev:
        return {1, 2, 3, 4};
    case ControlChartKind::ewma:
    case ControlChartKind::g:
    case ControlChartKind::t:
        return {1};
    case ControlChartKind::cusum:
        return {};
    case ControlChartKind::individuals:
    case ControlChartKind::xbar:
    case ControlChartKind::attribute:
    case ControlChartKind::laney:
        break;
    }
    return {1, 2, 3, 4, 5, 6, 7, 8};
}

std::vector<int> default_special_cause_tests(ControlChartKind kind)
{
    return applicable_special_cause_tests(kind);
}

ControlChartKind control_chart_kind_from_name(const std::string& name)
{
    if (name == "moving_range" || name == "mr") {
        return ControlChartKind::moving_range;
    }
    if (name == "xbar") {
        return ControlChartKind::xbar;
    }
    if (name == "range" || name == "r") {
        return ControlChartKind::range;
    }
    if (name == "stdev" || name == "s") {
        return ControlChartKind::stdev;
    }
    if (name == "attribute" || name == "p" || name == "np" || name == "c" || name == "u") {
        return ControlChartKind::attribute;
    }
    if (name == "laney") {
        return ControlChartKind::laney;
    }
    if (name == "ewma") {
        return ControlChartKind::ewma;
    }
    if (name == "cusum") {
        return ControlChartKind::cusum;
    }
    if (name == "g" || name == "g_chart") {
        return ControlChartKind::g;
    }
    if (name == "t" || name == "t_chart") {
        return ControlChartKind::t;
    }
    return ControlChartKind::individuals;
}

std::string control_chart_kind_name(ControlChartKind kind)
{
    switch (kind) {
    case ControlChartKind::moving_range:
        return "moving_range";
    case ControlChartKind::xbar:
        return "xbar";
    case ControlChartKind::range:
        return "range";
    case ControlChartKind::stdev:
        return "stdev";
    case ControlChartKind::attribute:
        return "attribute";
    case ControlChartKind::laney:
        return "laney";
    case ControlChartKind::ewma:
        return "ewma";
    case ControlChartKind::cusum:
        return "cusum";
    case ControlChartKind::g:
        return "g";
    case ControlChartKind::t:
        return "t";
    case ControlChartKind::individuals:
        break;
    }
    return "individuals";
}

SpecialCauseSelection special_cause_selection_from_configuration(
    const std::vector<int>& enabled_tests,
    const std::string& policy)
{
    return SpecialCauseSelection{enabled_tests, policy};
}

std::vector<int> parse_special_cause_tests(const std::string& text)
{
    std::string normalized = text;
    for (char& character : normalized) {
        if (character == ',' || character == ';' || character == '+' ) {
            character = ' ';
        }
    }
    std::string lowered = normalized;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
    if (lowered.find("none") != std::string::npos && lowered.find_first_of("12345678") == std::string::npos) {
        return {};
    }
    std::vector<int> tests;
    std::stringstream stream(normalized);
    std::string token;
    while (stream >> token) {
        bool ok = !token.empty();
        int value = 0;
        for (const char character : token) {
            if (character < '0' || character > '9') {
                ok = false;
                break;
            }
            value = value * 10 + (character - '0');
        }
        if (ok && value >= 1 && value <= 8
            && std::find(tests.begin(), tests.end(), value) == tests.end()) {
            tests.push_back(value);
        }
    }
    std::sort(tests.begin(), tests.end());
    return tests;
}

std::string format_special_cause_tests(const std::vector<int>& tests)
{
    if (tests.empty()) {
        return "无";
    }
    std::ostringstream stream;
    for (std::size_t index = 0; index < tests.size(); ++index) {
        if (index > 0) {
            stream << ", ";
        }
        stream << "Test " << tests[index];
    }
    return stream.str();
}

std::vector<int> resolve_special_cause_tests(
    const SpecialCauseSelection& selection,
    ControlChartKind kind,
    std::vector<DiagnosticMessage>* diagnostics)
{
    const std::vector<int> applicable = applicable_special_cause_tests(kind);
    std::vector<int> chosen;
    if (selection.policy == "default_all_applicable" && selection.enabled_tests.empty()) {
        chosen = applicable;
    } else if (!selection.enabled_tests.empty()) {
        chosen = selection.enabled_tests;
    } else if (selection.policy == "explicit") {
        chosen.clear();
    } else if (selection.policy == "default_all_applicable") {
        chosen = applicable;
    } else {
        chosen = {1};
    }
    std::vector<int> filtered;
    std::vector<int> ignored;
    for (const int test : chosen) {
        if (std::find(applicable.begin(), applicable.end(), test) != applicable.end()) {
            if (std::find(filtered.begin(), filtered.end(), test) == filtered.end()) {
                filtered.push_back(test);
            }
        } else if (test >= 1 && test <= 8) {
            ignored.push_back(test);
        }
    }
    std::sort(filtered.begin(), filtered.end());
    if (diagnostics != nullptr && !ignored.empty()) {
        add_warning(
            *diagnostics,
            "test_not_applicable",
            "已忽略不适用于此控制图的特殊原因测试：" + format_special_cause_tests(ignored)
                + "。");
    }
    if (diagnostics != nullptr && kind == ControlChartKind::cusum) {
        add_warning(
            *diagnostics,
            "cusum_signals_only",
            "CUSUM 不使用 Tests 1–8，改用上/下侧累计和首次信号。");
    }
    return filtered;
}

void apply_special_cause_tests(
    ControlChartResult& result,
    ControlChartKind kind,
    const SpecialCauseSelection& selection)
{
    const std::vector<int> enabled = resolve_special_cause_tests(
        selection, kind, &result.diagnostics);
    mark_special_cause_tests(result, enabled);
}

WithinSubgroupSigmaEstimate estimate_within_subgroup_sigma(
    const std::vector<std::vector<double>>& subgroups)
{
    WithinSubgroupSigmaEstimate estimate;
    if (subgroups.size() < 2) {
        estimate.error_code = "insufficient_subgroups";
        estimate.error_message = "组内 σ 至少需要两个子组。";
        return estimate;
    }
    const std::size_t subgroup_size = subgroups.front().size();
    if (subgroup_size < 2) {
        estimate.error_code = "invalid_subgroup_size";
        estimate.error_message = "各子组必须至少包含两个观测。";
        return estimate;
    }
    for (const auto& subgroup : subgroups) {
        if (subgroup.size() != subgroup_size) {
            estimate.error_code = "unequal_subgroups";
            estimate.error_message = "各子组必须具有相同观测数。";
            return estimate;
        }
        if (!std::all_of(subgroup.begin(), subgroup.end(),
                         [](double value) { return std::isfinite(value); })) {
            estimate.error_code = "non_finite_input";
            estimate.error_message = "子组不允许 NaN 或无穷观测。";
            return estimate;
        }
    }
    const bool use_range = subgroup_size <= 8;
    if (use_range) {
        const std::optional<double> d2 = SpcConstants::d2(subgroup_size);
        if (!d2.has_value()) {
            estimate.error_code = "unsupported_subgroup_size";
            estimate.error_message = "子组大小超出无偏常数表范围。";
            return estimate;
        }
        double range_sum = 0.0;
        for (const auto& subgroup : subgroups) {
            const auto minmax = std::minmax_element(subgroup.begin(), subgroup.end());
            range_sum += *minmax.second - *minmax.first;
        }
        estimate.sigma = (range_sum / static_cast<double>(subgroups.size())) / *d2;
        estimate.method = "R̄ / d2(n)";
        estimate.chart = "range";
        estimate.ok = true;
        return estimate;
    }
    const double c4_value = c4(subgroup_size);
    if (!(c4_value > 0.0) || !std::isfinite(c4_value)) {
        estimate.error_code = "invalid_c4";
        estimate.error_message = "无法计算该子组大小的 c4。";
        return estimate;
    }
    double s_sum = 0.0;
    for (const auto& subgroup : subgroups) {
        const double subgroup_mean = mean(subgroup);
        long double sum = 0.0L;
        for (const double value : subgroup) {
            const long double difference = static_cast<long double>(value) - subgroup_mean;
            sum += difference * difference;
        }
        s_sum += std::sqrt(static_cast<double>(
            sum / static_cast<long double>(subgroup_size - 1)));
    }
    estimate.sigma = (s_sum / static_cast<double>(subgroups.size())) / c4_value;
    estimate.method = "S̄ / c4";
    estimate.chart = "stdev";
    estimate.ok = true;
    return estimate;
}

namespace {

double geometric_trials_cdf(double k, double probability)
{
    if (k < 1.0) {
        return 0.0;
    }
    return 1.0 - std::pow(1.0 - probability, k);
}

double interpolated_geometric_invcdf(double quantile, double probability)
{
    int upper = 1;
    while (geometric_trials_cdf(static_cast<double>(upper), probability) < quantile
           && upper < 1000000) {
        ++upper;
    }
    const double g_b = static_cast<double>(upper);
    const double g_a = g_b - 1.0;
    const double p_a = geometric_trials_cdf(g_a, probability);
    const double p_b = geometric_trials_cdf(g_b, probability);
    if (!(p_b > p_a)) {
        return g_a;
    }
    return g_a + (quantile - p_a) / (p_b - p_a);
}

void fill_constant_limits(
    ControlChartResult& result, double center, double lower, double upper)
{
    result.center_line.assign(result.plotted_values.size(), center);
    result.lower_control_limit.assign(
        result.plotted_values.size(), std::isfinite(lower) ? lower : std::numeric_limits<double>::quiet_NaN());
    result.upper_control_limit.assign(
        result.plotted_values.size(), std::isfinite(upper) ? upper : std::numeric_limits<double>::quiet_NaN());
}

}  // namespace

ControlChartResult ControlCharts::g_chart(
    const std::vector<double>& intervals,
    const std::vector<RowId>& source_rows,
    const SpecialCauseSelection& special_causes)
{
    ControlChartResult result;
    result.source_rows = source_rows;
    if (intervals.size() < 2) {
        add_error(result.diagnostics, "insufficient_g_points",
                  "G 图至少需要两个有限间隔。");
        return result;
    }
    double sum = 0.0;
    for (const double value : intervals) {
        if (!(value >= 0.0) || !std::isfinite(value)) {
            add_error(result.diagnostics, "invalid_g_interval",
                      "G 图间隔必须为非负有限数。");
            return result;
        }
        sum += value;
    }
    const double mean = sum / static_cast<double>(intervals.size());
    const double probability = 1.0 / (mean + 1.0);
    if (!(probability > 0.0 && probability < 1.0)) {
        add_error(result.diagnostics, "invalid_g_probability",
                  "无法从间隔均值估计几何分布参数 p。");
        return result;
    }
    const double center = interpolated_geometric_invcdf(0.5, probability) - 1.0;
    double lower = interpolated_geometric_invcdf(0.00135, probability) - 1.0;
    const double upper = interpolated_geometric_invcdf(0.99865, probability) - 1.0;
    if (lower < 0.0) {
        lower = 0.0;
    }
    result.plotted_values = intervals;
    fill_constant_limits(result, center, lower, upper);
    apply_special_cause_tests(result, ControlChartKind::g, special_causes);
    return result;
}

ControlChartResult ControlCharts::t_chart(
    const std::vector<double>& intervals,
    const std::vector<RowId>& source_rows,
    const SpecialCauseSelection& special_causes)
{
    ControlChartResult result;
    result.source_rows = source_rows;
    std::vector<double> positive;
    bool had_zero = false;
    for (const double value : intervals) {
        if (!std::isfinite(value) || value < 0.0) {
            add_error(result.diagnostics, "invalid_t_interval",
                      "T 图间隔必须为非负有限数。");
            return result;
        }
        if (value == 0.0) {
            had_zero = true;
            continue;
        }
        positive.push_back(value);
    }
    if (positive.size() < 2) {
        add_error(result.diagnostics, "insufficient_t_points",
                  "T 图至少需要两个正间隔。");
        return result;
    }
    double shape = 0.0;
    double scale = 0.0;
    if (!had_zero) {
        const std::vector<bool> events(positive.size(), true);
        const WeibullResult fitted = fit_weibull(positive, events);
        result.diagnostics.insert(result.diagnostics.end(),
                                  fitted.diagnostics.cbegin(), fitted.diagnostics.cend());
        if (!fitted.identifiable || !fitted.converged || !(fitted.shape > 0.0)
            || !(fitted.scale > 0.0)) {
            add_error(result.diagnostics, "t_chart_weibull_failed",
                      "Weibull 参数无法识别，T 图控制限未计算。");
            return result;
        }
        shape = fitted.shape;
        scale = fitted.scale;
    } else {
        add_warning(result.diagnostics, "zero_interval_regression_used",
                    "存在 0 间隔，已排除后用 log-log 回归估计 Weibull 参数。");
        std::vector<double> ordered = positive;
        std::sort(ordered.begin(), ordered.end());
        const double n = static_cast<double>(ordered.size());
        double sum_x = 0.0;
        double sum_y = 0.0;
        double sum_xx = 0.0;
        double sum_xy = 0.0;
        for (std::size_t index = 0; index < ordered.size(); ++index) {
            const double probability = (static_cast<double>(index + 1) - 0.3) / (n + 0.4);
            const double x = std::log(-std::log(1.0 - probability));
            const double y = std::log(ordered[index]);
            sum_x += x;
            sum_y += y;
            sum_xx += x * x;
            sum_xy += x * y;
        }
        const double denominator = n * sum_xx - sum_x * sum_x;
        if (!(std::abs(denominator) > 0.0)) {
            add_error(result.diagnostics, "t_chart_regression_failed",
                      "0 间隔回归无法估计 Weibull 参数。");
            return result;
        }
        const double slope = (n * sum_xy - sum_x * sum_y) / denominator;
        const double intercept = (sum_y - slope * sum_x) / n;
        if (!(slope > 0.0) || !std::isfinite(intercept)) {
            add_error(result.diagnostics, "t_chart_regression_failed",
                      "0 间隔回归得到非正形状参数。");
            return result;
        }
        scale = std::exp(intercept);
        shape = 1.0 / slope;
    }
    const auto weibull_quantile = [shape, scale](double probability) {
        return scale * std::pow(-std::log(1.0 - probability), 1.0 / shape);
    };
    result.plotted_values = intervals;
    fill_constant_limits(
        result,
        weibull_quantile(0.5),
        weibull_quantile(0.00135),
        weibull_quantile(0.99865));
    apply_special_cause_tests(result, ControlChartKind::t, special_causes);
    return result;
}

}  // namespace datalab::domain::statistics
