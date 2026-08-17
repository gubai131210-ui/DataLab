#include "domain/statistics/control_charts.h"

#include "domain/statistics/spc_constants.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_map>

namespace datalab::domain::statistics {
namespace {

void add_error(std::vector<DiagnosticMessage>& diagnostics, const char* code, const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

void add_warning(std::vector<DiagnosticMessage>& diagnostics, const char* code, const char* message)
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

void mark_test1(ControlChartResult& result)
{
    result.test1_points.clear();
    const std::size_t count = result.plotted_values.size();
    for (std::size_t index = 0; index < count; ++index) {
        if (index >= result.lower_control_limit.size() || index >= result.upper_control_limit.size()) {
            continue;
        }
        if (result.plotted_values[index] < result.lower_control_limit[index]
            || result.plotted_values[index] > result.upper_control_limit[index]) {
            result.test1_points.push_back(index);
        }
    }
    if (!result.test1_points.empty()) {
        add_warning(
            result.diagnostics,
            "test1",
            "Test 1: one or more points are outside the control limits.");
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
    if (enabled_test(enabled_tests, 1)) {
        for (std::size_t index = 0; index < result.plotted_values.size(); ++index) {
            if (index < result.lower_control_limit.size()
                && index < result.upper_control_limit.size()
                && (result.plotted_values[index] < result.lower_control_limit[index]
                    || result.plotted_values[index] > result.upper_control_limit[index])) {
                result.special_cause_points[0].push_back(index);
                result.test1_points.push_back(index);
            }
        }
    }
    if (enabled_test(enabled_tests, 2)) {
        int side = 0;
        std::size_t run_start = 0;
        for (std::size_t index = 0; index < result.plotted_values.size(); ++index) {
            const double difference = result.plotted_values[index] - result.center_line[index];
            const int current_side = difference > 0.0 ? 1 : (difference < 0.0 ? -1 : 0);
            if (current_side == 0) {
                side = 0;
                continue;
            }
            if (current_side != side) {
                side = current_side;
                run_start = index;
            }
            if (index - run_start + 1 >= 9) {
                for (std::size_t point = run_start; point <= index; ++point) {
                    result.special_cause_points[1].push_back(point);
                }
            }
        }
    }
    if (enabled_test(enabled_tests, 3) && result.plotted_values.size() >= 6) {
        for (std::size_t start = 0; start + 6 <= result.plotted_values.size(); ++start) {
            bool increasing = true;
            bool decreasing = true;
            for (std::size_t index = start + 1; index < start + 6; ++index) {
                increasing = increasing && result.plotted_values[index] > result.plotted_values[index - 1];
                decreasing = decreasing && result.plotted_values[index] < result.plotted_values[index - 1];
            }
            if (increasing || decreasing) {
                for (std::size_t point = start; point < start + 6; ++point) {
                    result.special_cause_points[2].push_back(point);
                }
            }
        }
    }
    if (enabled_test(enabled_tests, 4) && result.plotted_values.size() >= 14) {
        for (std::size_t start = 0; start + 14 <= result.plotted_values.size(); ++start) {
            bool alternating = true;
            for (std::size_t index = start + 2; index < start + 14; ++index) {
                const double previous = result.plotted_values[index - 1];
                const double before = result.plotted_values[index - 2];
                alternating = alternating
                    && ((previous - result.center_line[index - 1])
                        * (result.plotted_values[index] - result.center_line[index]) < 0.0)
                    && ((before - result.center_line[index - 2])
                        * (previous - result.center_line[index - 1]) < 0.0);
            }
            if (alternating) {
                for (std::size_t point = start; point < start + 14; ++point) {
                    result.special_cause_points[3].push_back(point);
                }
            }
        }
    }
    auto sigma_at = [&result](std::size_t index) {
        if (index >= result.upper_control_limit.size()
            || index >= result.lower_control_limit.size()) {
            return 0.0;
        }
        return (result.upper_control_limit[index]
            - result.lower_control_limit[index]) / 6.0;
    };
    auto side_at = [&result](std::size_t index) {
        const double difference = result.plotted_values[index]
            - result.center_line[index];
        return difference > 0.0 ? 1 : (difference < 0.0 ? -1 : 0);
    };
    if (enabled_test(enabled_tests, 5) && result.plotted_values.size() >= 3) {
        for (std::size_t start = 0; start + 3 <= result.plotted_values.size(); ++start) {
            for (const int side : {-1, 1}) {
                std::size_t outside = 0;
                for (std::size_t index = start; index < start + 3; ++index) {
                    const double sigma = sigma_at(index);
                    outside += side_at(index) == side && sigma > 0.0
                        && side * (result.plotted_values[index]
                            - result.center_line[index]) > 2.0 * sigma;
                }
                if (outside >= 2) {
                    for (std::size_t index = start; index < start + 3; ++index) {
                        result.special_cause_points[4].push_back(index);
                    }
                }
            }
        }
    }
    if (enabled_test(enabled_tests, 6) && result.plotted_values.size() >= 5) {
        for (std::size_t start = 0; start + 5 <= result.plotted_values.size(); ++start) {
            for (const int side : {-1, 1}) {
                std::size_t outside = 0;
                for (std::size_t index = start; index < start + 5; ++index) {
                    const double sigma = sigma_at(index);
                    outside += side_at(index) == side && sigma > 0.0
                        && side * (result.plotted_values[index]
                            - result.center_line[index]) > sigma;
                }
                if (outside >= 4) {
                    for (std::size_t index = start; index < start + 5; ++index) {
                        result.special_cause_points[5].push_back(index);
                    }
                }
            }
        }
    }
    if (enabled_test(enabled_tests, 7) && result.plotted_values.size() >= 15) {
        for (std::size_t start = 0; start + 15 <= result.plotted_values.size(); ++start) {
            bool inside = true;
            for (std::size_t index = start; index < start + 15; ++index) {
                const double sigma = sigma_at(index);
                inside = inside && sigma > 0.0
                    && std::abs(result.plotted_values[index]
                        - result.center_line[index]) < sigma;
            }
            if (inside) {
                for (std::size_t index = start; index < start + 15; ++index) {
                    result.special_cause_points[6].push_back(index);
                }
            }
        }
    }
    if (enabled_test(enabled_tests, 8) && result.plotted_values.size() >= 8) {
        for (std::size_t start = 0; start + 8 <= result.plotted_values.size(); ++start) {
            bool alternating = true;
            for (std::size_t index = start + 1; index < start + 8; ++index) {
                const double sigma = sigma_at(index);
                alternating = alternating && sigma > 0.0
                    && std::abs(result.plotted_values[index]
                        - result.center_line[index]) >= sigma
                    && side_at(index) != 0
                    && side_at(index) != side_at(index - 1);
            }
            if (alternating) {
                for (std::size_t index = start; index < start + 8; ++index) {
                    result.special_cause_points[7].push_back(index);
                }
            }
        }
    }
    for (auto& points : result.special_cause_points) {
        std::sort(points.begin(), points.end());
        points.erase(std::unique(points.begin(), points.end()), points.end());
    }
    if (!result.test1_points.empty()) {
        add_warning(result.diagnostics, "test1",
                    "Test 1: one or more points are outside the control limits.");
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
    mark_special_cause_tests(result, options.enabled_special_cause_tests);
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
        for (int offset = 0; offset < length; ++offset) {
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
    mark_test1(result.primary);

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
    mark_test1(result.secondary);
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
    const std::vector<std::vector<double>>& subgroups)
{
    DualControlChartResult result;
    if (subgroups.empty()) {
        add_error(result.diagnostics, "empty_subgroups", "Xbar-R requires at least one subgroup.");
        result.primary.diagnostics = result.diagnostics;
        return result;
    }

    std::vector<double> ranges;
    std::size_t subgroup_size = subgroups.front().size();
    for (const auto& subgroup : subgroups) {
        if (subgroup.size() < 2) {
            add_error(result.diagnostics, "invalid_subgroup", "Each Xbar-R subgroup needs at least two values.");
            result.primary.diagnostics = result.diagnostics;
            return result;
        }
        subgroup_size = subgroup.size();
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
    mark_test1(result.primary);

    result.secondary.plotted_values = ranges;
    result.secondary.center_line.assign(ranges.size(), average_range);
    result.secondary.lower_control_limit.assign(ranges.size(), d3_limit.value_or(0.0) * average_range);
    result.secondary.upper_control_limit.assign(ranges.size(), (*d4) * average_range);
    mark_test1(result.secondary);
    result.primary.diagnostics.insert(
        result.primary.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
    return result;
}

DualControlChartResult ControlCharts::xbar_s_dual(
    const std::vector<std::vector<double>>& subgroups)
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
    mark_test1(result.primary);

    result.secondary.plotted_values = standard_deviations;
    result.secondary.center_line.assign(standard_deviations.size(), average_s);
    result.secondary.lower_control_limit.assign(
        standard_deviations.size(), b3 * average_s);
    result.secondary.upper_control_limit.assign(
        standard_deviations.size(), b4 * average_s);
    mark_test1(result.secondary);
    result.primary.diagnostics.insert(
        result.primary.diagnostics.end(), result.diagnostics.begin(), result.diagnostics.end());
    return result;
}

ControlChartResult ControlCharts::p_chart(
    const std::vector<std::size_t>& defectives,
    const std::vector<std::size_t>& inspected)
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
        result.center_line.push_back(pbar);
        result.lower_control_limit.push_back(std::max(0.0, pbar - 3.0 * sigma));
        result.upper_control_limit.push_back(std::min(1.0, pbar + 3.0 * sigma));
    }
    mark_test1(result);
    return result;
}

ControlChartResult ControlCharts::np_chart(
    const std::vector<std::size_t>& defectives,
    const std::vector<std::size_t>& inspected)
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
        result.lower_control_limit.push_back(std::max(0.0, center - deviation));
        result.upper_control_limit.push_back(center + deviation);
    }
    mark_test1(result);
    return result;
}

ControlChartResult ControlCharts::c_chart(
    const std::vector<std::size_t>& defects,
    std::size_t constant_units)
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
        result.lower_control_limit.push_back(std::max(0.0, center - deviation));
        result.upper_control_limit.push_back(center + deviation);
    }
    mark_test1(result);
    return result;
}

ControlChartResult ControlCharts::u_chart(
    const std::vector<std::size_t>& defects,
    const std::vector<std::size_t>& units)
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
        result.lower_control_limit.push_back(std::max(0.0, ubar - deviation));
        result.upper_control_limit.push_back(ubar + deviation);
    }
    mark_test1(result);
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
        if (ewma < result.lower_control_limit[index]
            || ewma > result.upper_control_limit[index]) {
            result.test1_points.push_back(index);
        }
    }
    mark_special_cause_tests(result, {1});
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
        }
        if (lower > options.h * options.sigma) {
            result.lower_signal_points.push_back(index);
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

}  // namespace datalab::domain::statistics
