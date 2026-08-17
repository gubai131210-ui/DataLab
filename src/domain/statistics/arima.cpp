#include "domain/statistics/arima.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

constexpr double kNormal95 = 1.959963984540054;
constexpr double kMinimumVariance = 1.0e-12;

void error(ArimaResult& result, const char* code, const char* message)
{
    result.diagnostics.push_back(
        {DiagnosticMessage::Severity::error, code, message});
}

void warning(ArimaResult& result, const char* code, const char* message)
{
    result.diagnostics.push_back(
        {DiagnosticMessage::Severity::warning, code, message});
}

bool finite_series(const std::vector<double>& observations)
{
    return std::all_of(observations.begin(), observations.end(),
                       [](double value) { return std::isfinite(value); });
}

std::size_t minimum_sample_size(ArimaModel model)
{
    switch (model) {
    case ArimaModel::arima_0_1_0:
        return 3;
    case ArimaModel::ar_1:
        return 3;
    case ArimaModel::ma_1:
        return 4;
    }
    return std::numeric_limits<std::size_t>::max();
}

std::size_t parameter_count(ArimaModel model)
{
    return model == ArimaModel::arima_0_1_0 ? 2 : 3;
}

void information_criteria(ArimaResult& result, std::size_t n)
{
    const double variance = std::max(result.sse / static_cast<double>(n),
                                      kMinimumVariance);
    const double parameter_count_value =
        static_cast<double>(parameter_count(result.model));
    result.log_likelihood =
        -0.5 * static_cast<double>(n)
        * (std::log(2.0 * std::acos(-1.0)) + 1.0 + std::log(variance));
    result.aic = -2.0 * result.log_likelihood + 2.0 * parameter_count_value;
    result.bic = -2.0 * result.log_likelihood
                 + parameter_count_value * std::log(static_cast<double>(n));
    const double denominator = static_cast<double>(n)
                               - parameter_count_value - 1.0;
    if (denominator > 0.0) {
        result.aicc = result.aic
                      + 2.0 * parameter_count_value
                            * (parameter_count_value + 1.0) / denominator;
    } else {
        result.aicc = result.aic;
        warning(result, "aicc_unavailable",
                "有效样本不足以进行 AICc 小样本修正，已返回未修正的 AIC。");
    }
    if (result.sse <= kMinimumVariance) {
        warning(result, "near_zero_residual_variance",
                "残差平方和接近零，信息准则使用数值下限稳定计算。");
    }
}

void intervals(ArimaResult& result, double variance, ArimaModel model,
               double coefficient)
{
    double accumulated_multiplier = 0.0;
    for (std::size_t index = 0; index < result.forecasts.size(); ++index) {
        const double horizon = static_cast<double>(index + 1);
        if (model == ArimaModel::arima_0_1_0) {
            accumulated_multiplier = horizon;
        } else if (model == ArimaModel::ar_1) {
            accumulated_multiplier = 0.0;
            double impulse = 1.0;
            for (std::size_t step = 0; step < index + 1; ++step) {
                accumulated_multiplier += impulse * impulse;
                impulse *= coefficient;
            }
        } else {
            accumulated_multiplier = 1.0 + (index == 0 ? coefficient * coefficient
                                                       : 0.0);
        }
        const double margin =
            kNormal95 * std::sqrt(std::max(0.0, variance * accumulated_multiplier));
        result.lower.push_back(result.forecasts[index] - margin);
        result.upper.push_back(result.forecasts[index] + margin);
    }
}

void fit_random_walk_with_drift(ArimaResult& result,
                                const std::vector<double>& observations,
                                std::size_t forecast_periods)
{
    const std::size_t n = observations.size() - 1;
    result.drift = (observations.back() - observations.front())
                   / static_cast<double>(n);
    result.fitted.resize(observations.size());
    result.residuals.resize(observations.size());
    result.fitted.front() = observations.front();
    result.residuals.front() = 0.0;
    for (std::size_t index = 1; index < observations.size(); ++index) {
        result.fitted[index] = observations[index - 1] + result.drift;
        result.residuals[index] =
            observations[index] - result.fitted[index];
        result.sse += result.residuals[index] * result.residuals[index];
    }
    result.forecasts.reserve(forecast_periods);
    for (std::size_t horizon = 1; horizon <= forecast_periods; ++horizon) {
        result.forecasts.push_back(
            observations.back() + static_cast<double>(horizon) * result.drift);
    }
    information_criteria(result, n);
    intervals(result, std::max(result.sse / static_cast<double>(n),
                               kMinimumVariance),
              result.model, 0.0);
}

void fit_ar_one(ArimaResult& result, const std::vector<double>& observations,
                std::size_t forecast_periods)
{
    const std::size_t n = observations.size() - 1;
    const double mean_x =
        std::accumulate(observations.begin(), observations.end() - 1, 0.0)
        / static_cast<double>(n);
    const double mean_y =
        std::accumulate(observations.begin() + 1, observations.end(), 0.0)
        / static_cast<double>(n);
    double denominator = 0.0;
    double numerator = 0.0;
    for (std::size_t index = 1; index < observations.size(); ++index) {
        const double x = observations[index - 1] - mean_x;
        numerator += x * (observations[index] - mean_y);
        denominator += x * x;
    }
    result.coefficient = denominator > kMinimumVariance
                             ? numerator / denominator
                             : 0.0;
    result.intercept = mean_y - result.coefficient * mean_x;
    result.fitted.resize(observations.size());
    result.residuals.resize(observations.size());
    result.fitted.front() = observations.front();
    result.residuals.front() = 0.0;
    for (std::size_t index = 1; index < observations.size(); ++index) {
        result.fitted[index] = result.intercept
                               + result.coefficient * observations[index - 1];
        result.residuals[index] = observations[index] - result.fitted[index];
        result.sse += result.residuals[index] * result.residuals[index];
    }
    result.forecasts.reserve(forecast_periods);
    double previous = observations.back();
    for (std::size_t index = 0; index < forecast_periods; ++index) {
        previous = result.intercept + result.coefficient * previous;
        result.forecasts.push_back(previous);
    }
    information_criteria(result, n);
    intervals(result, std::max(result.sse / static_cast<double>(n),
                               kMinimumVariance),
              result.model, result.coefficient);
}

void fit_ma_one(ArimaResult& result, const std::vector<double>& observations,
                std::size_t forecast_periods)
{
    const std::size_t n = observations.size() - 1;
    const double mean =
        std::accumulate(observations.begin() + 1, observations.end(), 0.0)
        / static_cast<double>(n);
    double best_sse = std::numeric_limits<double>::infinity();
    double best_theta = 0.0;
    for (int step = -99; step <= 99; ++step) {
        const double theta = static_cast<double>(step) / 100.0;
        double sse = 0.0;
        double previous_error = 0.0;
        for (std::size_t index = 1; index < observations.size(); ++index) {
            const double residual =
                observations[index] - mean - theta * previous_error;
            previous_error = residual;
            sse += residual * residual;
        }
        if (sse < best_sse) {
            best_sse = sse;
            best_theta = theta;
        }
    }
    result.intercept = mean;
    result.coefficient = best_theta;
    result.fitted.resize(observations.size());
    result.residuals.resize(observations.size());
    result.fitted.front() = observations.front();
    result.residuals.front() = 0.0;
    for (std::size_t index = 1; index < observations.size(); ++index) {
        result.fitted[index] = result.intercept
                               + result.coefficient * result.residuals[index - 1];
        result.residuals[index] = observations[index] - result.fitted[index];
        result.sse += result.residuals[index] * result.residuals[index];
    }
    result.forecasts.reserve(forecast_periods);
    result.forecasts.push_back(
        result.intercept + result.coefficient * result.residuals.back());
    for (std::size_t index = 1; index < forecast_periods; ++index) {
        result.forecasts.push_back(result.intercept);
    }
    information_criteria(result, n);
    intervals(result, std::max(result.sse / static_cast<double>(n),
                               kMinimumVariance),
              result.model, result.coefficient);
}

}  // namespace

ArimaResult fit_arima(const std::vector<double>& observations, ArimaModel model,
                      std::size_t forecast_periods)
{
    ArimaResult result;
    result.model = model;
    if (observations.size() < minimum_sample_size(model)) {
        error(result, "insufficient_samples",
              "ARIMA 候选要求更多的有限观测值才能拟合。");
        return result;
    }
    if (forecast_periods == 0) {
        error(result, "invalid_forecast_horizon",
              "预测期数必须大于零。");
        return result;
    }
    if (!finite_series(observations)) {
        error(result, "non_finite_observation",
              "观测序列必须全部为有限数值。");
        return result;
    }
    switch (model) {
    case ArimaModel::arima_0_1_0:
        fit_random_walk_with_drift(result, observations, forecast_periods);
        break;
    case ArimaModel::ar_1:
        fit_ar_one(result, observations, forecast_periods);
        break;
    case ArimaModel::ma_1:
        fit_ma_one(result, observations, forecast_periods);
        break;
    }
    if (!std::isfinite(result.sse) || !std::isfinite(result.aic)
        || !std::isfinite(result.aicc) || !std::isfinite(result.bic)
        || !finite_series(result.fitted) || !finite_series(result.residuals)
        || !finite_series(result.forecasts) || !finite_series(result.lower)
        || !finite_series(result.upper)) {
        error(result, "non_finite_result",
              "模型计算产生了非有限结果，请检查数据尺度后重试。");
    }
    return result;
}

std::vector<ArimaResult> fit_arima_candidates(
    const std::vector<double>& observations, std::size_t forecast_periods)
{
    return {fit_arima(observations, ArimaModel::arima_0_1_0, forecast_periods),
            fit_arima(observations, ArimaModel::ar_1, forecast_periods),
            fit_arima(observations, ArimaModel::ma_1, forecast_periods)};
}

}  // namespace datalab::domain::statistics
