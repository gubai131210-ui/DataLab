#include "domain/statistics/time_series.h"

#include <cmath>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

void invalid(ForecastResult& result, const char* message)
{
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::error, "invalid_time_series", message});
}

void metrics(ForecastResult& result, const std::vector<double>& observations)
{
    double absolute = 0.0;
    double squared = 0.0;
    double percentage = 0.0;
    std::size_t count = 0;
    for (std::size_t index = 0; index < observations.size()
         && index < result.fitted.size(); ++index) {
        if (!std::isfinite(result.fitted[index])) {
            continue;
        }
        const double error = observations[index] - result.fitted[index];
        absolute += std::abs(error);
        squared += error * error;
        if (observations[index] != 0.0) {
            percentage += std::abs(error / observations[index]) * 100.0;
        }
        ++count;
    }
    if (count > 0) {
        result.mad = absolute / static_cast<double>(count);
        result.msd = squared / static_cast<double>(count);
        result.mape = percentage / static_cast<double>(count);
    }
}

void limits(ForecastResult& result, std::size_t count)
{
    const double margin = 1.96 * 1.25 * result.mad;
    for (std::size_t index = 0; index < count; ++index) {
        const double forecast = result.forecasts[index];
        result.lower.push_back(forecast - margin);
        result.upper.push_back(forecast + margin);
    }
}

}  // namespace

ForecastResult single_exponential_smoothing(
    const std::vector<double>& observations,
    double alpha,
    std::size_t forecast_periods)
{
    ForecastResult result;
    if (observations.size() < 2 || alpha <= 0.0 || alpha > 1.0
        || forecast_periods == 0) {
        invalid(result, "单指数平滑要求至少两个观测、alpha 位于 (0,1] 且预测期数大于 0。");
        return result;
    }
    result.fitted.resize(observations.size());
    double level = observations.front();
    result.fitted[0] = level;
    for (std::size_t index = 1; index < observations.size(); ++index) {
        result.fitted[index] = level;
        level = alpha * observations[index] + (1.0 - alpha) * level;
    }
    result.forecasts.assign(forecast_periods, level);
    metrics(result, observations);
    limits(result, forecast_periods);
    return result;
}

ForecastResult double_exponential_smoothing(
    const std::vector<double>& observations,
    double alpha,
    double gamma,
    std::size_t forecast_periods)
{
    ForecastResult result;
    if (observations.size() < 3 || alpha <= 0.0 || alpha > 1.0
        || gamma <= 0.0 || gamma > 1.0 || forecast_periods == 0) {
        invalid(result, "双指数平滑要求至少三个观测，alpha/gamma 位于 (0,1] 且预测期数大于 0。");
        return result;
    }
    result.fitted.resize(observations.size());
    double level = observations.front();
    double trend = observations[1] - observations.front();
    result.fitted[0] = observations.front();
    for (std::size_t index = 1; index < observations.size(); ++index) {
        result.fitted[index] = level + trend;
        const double previous_level = level;
        level = alpha * observations[index] + (1.0 - alpha) * (level + trend);
        trend = gamma * (level - previous_level) + (1.0 - gamma) * trend;
    }
    for (std::size_t period = 1; period <= forecast_periods; ++period) {
        result.forecasts.push_back(level + static_cast<double>(period) * trend);
    }
    metrics(result, observations);
    limits(result, forecast_periods);
    return result;
}

}  // namespace datalab::domain::statistics
