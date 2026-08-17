#include "domain/statistics/time_series_decomposition.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <vector>

namespace datalab::domain::statistics {
namespace {

constexpr double kEpsilon = 1.0e-12;

double missing_value()
{
    return std::numeric_limits<double>::quiet_NaN();
}

void add_diagnostic(
    TimeSeriesDecompositionResult& result,
    DiagnosticMessage::Severity severity,
    const char* code,
    const char* message)
{
    result.diagnostics.push_back({severity, code, message});
}

double median(std::vector<double> values)
{
    if (values.empty()) {
        return missing_value();
    }
    const std::size_t middle = values.size() / 2;
    std::nth_element(values.begin(), values.begin() + middle, values.end());
    if (values.size() % 2 != 0) {
        return values[middle];
    }
    const double upper = values[middle];
    const double lower = *std::max_element(values.begin(), values.begin() + middle);
    return (lower + upper) / 2.0;
}

bool valid(double value)
{
    return std::isfinite(value);
}

void calculate_centered_moving_average(
    const std::vector<double>& observations,
    std::size_t period,
    std::vector<double>& centered)
{
    const std::size_t count = observations.size();
    const double nan = missing_value();
    centered.assign(count, nan);
    std::vector<double> moving_average(count, nan);
    if (period % 2 == 0) {
        for (std::size_t start = 0; start + period <= count; ++start) {
            const double sum = std::accumulate(
                observations.begin() + static_cast<std::ptrdiff_t>(start),
                observations.begin()
                    + static_cast<std::ptrdiff_t>(start + period),
                0.0);
            moving_average[start + period / 2 - 1] =
                sum / static_cast<double>(period);
        }
        const std::size_t half = period / 2;
        for (std::size_t index = half; index + half < count; ++index) {
            centered[index] = (moving_average[index - 1]
                               + moving_average[index])
                / 2.0;
        }
        return;
    }

    const std::size_t half = period / 2;
    for (std::size_t index = half; index + half < count; ++index) {
        const double sum = std::accumulate(
            observations.begin()
                + static_cast<std::ptrdiff_t>(index - half),
            observations.begin()
                + static_cast<std::ptrdiff_t>(index + half + 1),
            0.0);
        centered[index] = sum / static_cast<double>(period);
    }
}

void calculate_metrics(TimeSeriesDecompositionResult& result)
{
    double absolute_error = 0.0;
    double squared_error = 0.0;
    double percentage_error = 0.0;
    std::size_t count = 0;
    std::size_t percentage_count = 0;
    for (std::size_t index = 0; index < result.observations.size(); ++index) {
        if (!valid(result.fitted[index]) || !valid(result.residuals[index])) {
            continue;
        }
        const double error = result.residuals[index];
        absolute_error += std::abs(error);
        squared_error += error * error;
        ++count;
        if (std::abs(result.observations[index]) > kEpsilon) {
            percentage_error +=
                std::abs(error / result.observations[index]) * 100.0;
            ++percentage_count;
        }
    }
    if (count > 0) {
        result.mad = absolute_error / static_cast<double>(count);
        result.msd = squared_error / static_cast<double>(count);
    }
    if (percentage_count > 0) {
        result.mape = percentage_error / static_cast<double>(percentage_count);
    }
}

}  // namespace

TimeSeriesDecompositionResult decompose_time_series(
    const TimeSeriesDecompositionInput& input,
    const TimeSeriesDecompositionOptions& options)
{
    TimeSeriesDecompositionResult result;
    result.model = options.model;
    result.seasonal_period = options.seasonal_period;

    const std::size_t count = input.observations.size();
    if (input.time.size() != count || count == 0) {
        add_diagnostic(
            result,
            DiagnosticMessage::Severity::error,
            "invalid_time_series_time",
            "时间序列必须包含与观测值等长且非空的时间向量。");
        return result;
    }
    if (options.seasonal_period == 0
        || options.seasonal_period > count / 2) {
        add_diagnostic(
            result,
            DiagnosticMessage::Severity::error,
            "insufficient_complete_cycles",
            "季节周期必须为正整数，且输入至少包含两个完整周期。");
        return result;
    }
    for (std::size_t index = 0; index < count; ++index) {
        if (!valid(input.time[index])) {
            add_diagnostic(
                result,
                DiagnosticMessage::Severity::error,
                "missing_time",
                "时间向量包含缺失或非有限值。");
            return result;
        }
        if (!valid(input.observations[index])) {
            add_diagnostic(
                result,
                DiagnosticMessage::Severity::error,
                "missing_observation",
                "观测序列包含缺失或非有限值。");
            return result;
        }
        if (index > 0 && input.time[index] <= input.time[index - 1]) {
            add_diagnostic(
                result,
                DiagnosticMessage::Severity::error,
                "invalid_time_order",
                "时间必须严格递增。");
            return result;
        }
        if (options.model == DecompositionModel::multiplicative
            && input.observations[index] <= 0.0) {
            add_diagnostic(
                result,
                DiagnosticMessage::Severity::error,
                "non_positive_observation",
                "乘法分解要求所有观测值为正数。");
            return result;
        }
    }

    std::vector<double> intervals;
    intervals.reserve(count - 1);
    for (std::size_t index = 1; index < count; ++index) {
        intervals.push_back(input.time[index] - input.time[index - 1]);
    }
    const double typical_interval = median(intervals);
    for (const double interval : intervals) {
        if (std::abs(interval - typical_interval)
            > 1.0e-9 * std::max(1.0, std::abs(typical_interval))) {
            add_diagnostic(
                result,
                DiagnosticMessage::Severity::warning,
                "irregular_time_spacing",
                "时间间隔不完全相等，趋势拟合仍使用实际时间，预测使用中位时间间隔。");
            break;
        }
    }

    result.time = input.time;
    result.observations = input.observations;
    calculate_centered_moving_average(
        result.observations, options.seasonal_period,
        result.centered_moving_average);

    std::vector<std::vector<double>> estimates(options.seasonal_period);
    for (std::size_t index = 0; index < count; ++index) {
        if (!valid(result.centered_moving_average[index])) {
            continue;
        }
        const double moving_average = result.centered_moving_average[index];
        const double estimate =
            options.model == DecompositionModel::additive
                ? result.observations[index] - moving_average
                : result.observations[index] / moving_average;
        if (valid(estimate)) {
            estimates[index % options.seasonal_period].push_back(estimate);
        }
    }

    result.seasonal_indices.assign(options.seasonal_period, 0.0);
    for (std::size_t phase = 0; phase < options.seasonal_period; ++phase) {
        result.seasonal_indices[phase] = median(estimates[phase]);
    }
    if (options.model == DecompositionModel::multiplicative) {
        const double normalization =
            median(result.seasonal_indices);
        if (!(normalization > kEpsilon) || !valid(normalization)) {
            add_diagnostic(
                result,
                DiagnosticMessage::Severity::error,
                "invalid_seasonal_indices",
                "无法从中心移动平均估计有效的乘法季节指数。");
            return result;
        }
        for (double& seasonal : result.seasonal_indices) {
            seasonal /= normalization;
        }
    } else {
        const double normalization = median(result.seasonal_indices);
        if (!valid(normalization)) {
            add_diagnostic(
                result,
                DiagnosticMessage::Severity::error,
                "invalid_seasonal_indices",
                "无法从中心移动平均估计有效的加法季节指数。");
            return result;
        }
        for (double& seasonal : result.seasonal_indices) {
            seasonal -= normalization;
        }
    }

    std::vector<double> deseasonalized(count, 0.0);
    for (std::size_t index = 0; index < count; ++index) {
        const double seasonal = result.seasonal_indices[index % options.seasonal_period];
        deseasonalized[index] =
            options.model == DecompositionModel::additive
                ? result.observations[index] - seasonal
                : result.observations[index] / seasonal;
    }

    const double mean_time = std::accumulate(input.time.begin(), input.time.end(), 0.0)
        / static_cast<double>(count);
    const double mean_value =
        std::accumulate(deseasonalized.begin(), deseasonalized.end(), 0.0)
        / static_cast<double>(count);
    double time_sum_squares = 0.0;
    double time_value_sum = 0.0;
    for (std::size_t index = 0; index < count; ++index) {
        const double centered_time = input.time[index] - mean_time;
        time_sum_squares += centered_time * centered_time;
        time_value_sum += centered_time * (deseasonalized[index] - mean_value);
    }
    result.trend_slope =
        time_sum_squares > kEpsilon ? time_value_sum / time_sum_squares : 0.0;
    result.trend_intercept = mean_value - result.trend_slope * mean_time;

    result.trend.resize(count);
    result.fitted.resize(count);
    result.residuals.resize(count);
    for (std::size_t index = 0; index < count; ++index) {
        result.trend[index] =
            result.trend_intercept + result.trend_slope * input.time[index];
        const double seasonal =
            result.seasonal_indices[index % options.seasonal_period];
        result.fitted[index] =
            options.model == DecompositionModel::additive
                ? result.trend[index] + seasonal
                : result.trend[index] * seasonal;
        result.residuals[index] =
            result.observations[index] - result.fitted[index];
    }

    if (options.forecast_periods == 0) {
        add_diagnostic(
            result,
            DiagnosticMessage::Severity::warning,
            "no_forecast_periods",
            "预测期数为 0，因此未生成未来预测值。");
    }
    for (std::size_t step = 1; step <= options.forecast_periods; ++step) {
        const double forecast_time =
            input.time.back() + static_cast<double>(step) * typical_interval;
        const double forecast_trend =
            result.trend_intercept + result.trend_slope * forecast_time;
        const std::size_t phase =
            (count + step - 1) % options.seasonal_period;
        const double seasonal = result.seasonal_indices[phase];
        result.forecasts.push_back(
            options.model == DecompositionModel::additive
                ? forecast_trend + seasonal
                : forecast_trend * seasonal);
    }
    calculate_metrics(result);
    return result;
}

}  // namespace datalab::domain::statistics
