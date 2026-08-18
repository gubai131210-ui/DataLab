#include "domain/statistics/seasonal_forecasting.h"

#include "domain/statistics/arima.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <utility>

namespace datalab::domain::statistics {
namespace {

constexpr double kEpsilon = 1.0e-12;
constexpr double kNormal95 = 1.959963984540054;

void diagnostic(std::vector<DiagnosticMessage>& output,
                DiagnosticMessage::Severity severity, const char* code,
                const char* message)
{
    output.push_back({severity, code, message});
}

bool finite_values(const std::vector<double>& values)
{
    return std::all_of(values.cbegin(), values.cend(),
                       [](double value) { return std::isfinite(value); });
}

double normal_multiplier(double confidence_level)
{
    // The public API currently supports the common normal approximation.
    // Keep this conservative rather than silently returning a false quantile.
    if (std::abs(confidence_level - 0.95) < 1.0e-9) {
        return kNormal95;
    }
    return kNormal95;
}

void calculate_metrics(const std::vector<double>& actuals,
                       const std::vector<double>& predictions,
                       ForecastMetrics& metrics,
                       std::vector<DiagnosticMessage>& diagnostics)
{
    if (actuals.size() != predictions.size() || actuals.empty()) {
        return;
    }
    double absolute_sum = 0.0;
    double squared_sum = 0.0;
    double percentage_sum = 0.0;
    std::size_t percentage_count = 0;
    for (std::size_t index = 0; index < actuals.size(); ++index) {
        const double error = actuals[index] - predictions[index];
        absolute_sum += std::abs(error);
        squared_sum += error * error;
        if (std::abs(actuals[index]) > kEpsilon) {
            percentage_sum += std::abs(error / actuals[index]) * 100.0;
            ++percentage_count;
        }
    }
    metrics.count = actuals.size();
    metrics.mad = absolute_sum / static_cast<double>(metrics.count);
    metrics.msd = squared_sum / static_cast<double>(metrics.count);
    metrics.rmse = std::sqrt(metrics.msd);
    if (percentage_count > 0) {
        metrics.mape = percentage_sum / static_cast<double>(percentage_count);
    } else {
        diagnostic(diagnostics, DiagnosticMessage::Severity::warning,
                   "mape_unavailable",
                   "实际值全部接近零，MAPE 无法定义，已保留为 0。");
    }
}

void calculate_mase(const std::vector<double>& observations,
                    ForecastMetrics& metrics,
                    std::vector<DiagnosticMessage>& diagnostics)
{
    if (observations.size() < 2 || metrics.count == 0) {
        return;
    }
    double naive_absolute_sum = 0.0;
    for (std::size_t index = 1; index < observations.size(); ++index) {
        naive_absolute_sum +=
            std::abs(observations[index] - observations[index - 1]);
    }
    const double scale =
        naive_absolute_sum / static_cast<double>(observations.size() - 1);
    if (scale <= kEpsilon) {
        diagnostic(diagnostics, DiagnosticMessage::Severity::warning,
                   "mase_unavailable",
                   "一阶朴素基准误差接近零，MASE 无法稳定计算，已保留为 0。");
        return;
    }
    metrics.mase = metrics.mad / scale;
}

bool valid_options(const SeasonalForecastingOptions& options,
                   std::vector<DiagnosticMessage>& diagnostics)
{
    if (options.seasonal_period == 0 || options.forecast_periods == 0) {
        diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                   "invalid_forecast_options",
                   "季节周期和预测期数必须为正整数。");
        return false;
    }
    if (options.alpha < 0.0 || options.alpha > 1.0
        || options.beta < 0.0 || options.beta > 1.0
        || options.gamma < 0.0 || options.gamma > 1.0
        || options.damping_phi <= 0.0 || options.damping_phi > 1.0
        || options.confidence_level <= 0.0
        || options.confidence_level >= 1.0) {
        diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                   "invalid_smoothing_parameters",
                   "平滑参数、阻尼系数和置信水平必须在有效范围内。");
        return false;
    }
    return true;
}

double seasonal_adjustment(double observation, double seasonal,
                           SeasonalErrorModel model)
{
    if (model == SeasonalErrorModel::additive) {
        return observation - seasonal;
    }
    return observation / std::max(std::abs(seasonal), kEpsilon);
}

double damped_additive_trend(double trend, std::size_t horizon, bool damped,
                             double phi)
{
    if (!damped) {
        return static_cast<double>(horizon) * trend;
    }
    double sum = 0.0;
    double power = 1.0;
    for (std::size_t step = 0; step < horizon; ++step) {
        sum += power;
        power *= phi;
    }
    return trend * sum;
}

double damped_multiplicative_trend(double trend, std::size_t horizon,
                                   bool damped, double phi)
{
    if (!damped) {
        return std::pow(trend, static_cast<double>(horizon));
    }
    double exponent = 0.0;
    double power = 1.0;
    for (std::size_t step = 0; step < horizon; ++step) {
        exponent += power;
        power *= phi;
    }
    return std::pow(trend, exponent);
}

std::size_t differencing_lag(const SarimaOrder& order)
{
    return order.d + order.seasonal_d * order.seasonal_period;
}

std::vector<double> difference_series(const std::vector<double>& observations,
                                      const SarimaOrder& order)
{
    std::vector<double> differenced = observations;
    for (std::size_t difference = 0; difference < order.d; ++difference) {
        std::vector<double> next;
        next.reserve(differenced.size() - 1);
        for (std::size_t index = 1; index < differenced.size(); ++index) {
            next.push_back(differenced[index] - differenced[index - 1]);
        }
        differenced = std::move(next);
    }
    for (std::size_t difference = 0; difference < order.seasonal_d;
         ++difference) {
        std::vector<double> next;
        const std::size_t period = order.seasonal_period;
        next.reserve(differenced.size() - period);
        for (std::size_t index = period; index < differenced.size(); ++index) {
            next.push_back(differenced[index] - differenced[index - period]);
        }
        differenced = std::move(next);
    }
    return differenced;
}

std::vector<double> inverse_difference_series(
    const std::vector<double>& differenced_values,
    const std::vector<double>& original_values, const SarimaOrder& order)
{
    std::vector<double> values = differenced_values;
    const std::size_t period = order.seasonal_period;
    std::vector<double> regular_base = original_values;
    for (std::size_t difference = 0; difference < order.d; ++difference) {
        std::vector<double> next;
        next.reserve(regular_base.size() - 1);
        for (std::size_t index = 1; index < regular_base.size(); ++index) {
            next.push_back(regular_base[index] - regular_base[index - 1]);
        }
        regular_base = std::move(next);
    }
    for (std::size_t difference = 0; difference < order.seasonal_d;
         ++difference) {
        std::vector<double> restored;
        restored.reserve(values.size() + period);
        for (std::size_t index = 0; index < period; ++index) {
            restored.push_back(regular_base[index]);
        }
        for (std::size_t index = 0; index < values.size(); ++index) {
            restored.push_back(values[index]
                              + restored[restored.size() - period]);
        }
        values = std::move(restored);
    }
    for (std::size_t difference = 0; difference < order.d; ++difference) {
        std::vector<double> lower_order_base = original_values;
        for (std::size_t lower_difference = 0;
             lower_difference + difference + 1 < order.d;
             ++lower_difference) {
            std::vector<double> next;
            next.reserve(lower_order_base.size() - 1);
            for (std::size_t index = 1; index < lower_order_base.size();
                 ++index) {
                next.push_back(lower_order_base[index]
                               - lower_order_base[index - 1]);
            }
            lower_order_base = std::move(next);
        }
        std::vector<double> restored;
        restored.reserve(values.size() + 1);
        restored.push_back(lower_order_base.front());
        for (double value : values) {
            restored.push_back(value + restored.back());
        }
        values = std::move(restored);
    }
    return values;
}

bool valid_sarima_parameters(const FixedSarimaParameters& parameters,
                             std::vector<DiagnosticMessage>& diagnostics)
{
    const SarimaOrder& order = parameters.order;
    if (order.seasonal_period == 0 || order.d > 2 || order.seasonal_d > 1
        || order.p > 3 || order.q > 3 || order.seasonal_p > 2
        || order.seasonal_q > 2) {
        diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                   "unsupported_sarima_order",
                   "仅支持 d≤2、D≤1、p/q≤3、P/Q≤2 且季节周期为正的有限阶 SARIMA。");
        return false;
    }
    if ((order.seasonal_p > 0 || order.seasonal_d > 0
         || order.seasonal_q > 0)
        && order.seasonal_period < 2) {
        diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                   "invalid_sarima_period",
                   "使用季节项时季节周期必须至少为 2。");
        return false;
    }
    if (parameters.ar.size() != order.p || parameters.ma.size() != order.q
        || parameters.seasonal_ar.size() != order.seasonal_p
        || parameters.seasonal_ma.size() != order.seasonal_q) {
        diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                   "sarima_parameter_size_mismatch",
                   "AR、MA、季节 AR 和季节 MA 系数数量必须与模型阶数一致。");
        return false;
    }
    if (!std::isfinite(parameters.intercept)
        || !std::isfinite(parameters.innovation_variance)
        || parameters.innovation_variance < 0.0
        || !finite_values(parameters.ar) || !finite_values(parameters.ma)
        || !finite_values(parameters.seasonal_ar)
        || !finite_values(parameters.seasonal_ma)) {
        diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                   "invalid_sarima_parameters",
                   "SARIMA 固定参数必须全部为有限数值，创新方差不得为负。");
        return false;
    }
    return true;
}

std::vector<double> multiply_polynomials(
    const std::vector<double>& left,
    const std::vector<double>& right)
{
    if (left.empty()) {
        return right;
    }
    if (right.empty()) {
        return left;
    }
    std::vector<double> product(left.size() + right.size() - 1, 0.0);
    for (std::size_t i = 0; i < left.size(); ++i) {
        for (std::size_t j = 0; j < right.size(); ++j) {
            product[i + j] += left[i] * right[j];
        }
    }
    return product;
}

std::vector<double> ar_operator(const std::vector<double>& coefficients)
{
    std::vector<double> polynomial(coefficients.size() + 1, 0.0);
    polynomial[0] = 1.0;
    for (std::size_t index = 0; index < coefficients.size(); ++index) {
        polynomial[index + 1] = -coefficients[index];
    }
    return polynomial;
}

std::vector<double> ma_operator(const std::vector<double>& coefficients)
{
    std::vector<double> polynomial(coefficients.size() + 1, 0.0);
    polynomial[0] = 1.0;
    for (std::size_t index = 0; index < coefficients.size(); ++index) {
        polynomial[index + 1] = coefficients[index];
    }
    return polynomial;
}

std::vector<double> seasonal_operator(
    const std::vector<double>& coefficients,
    std::size_t period,
    bool moving_average)
{
    if (coefficients.empty() || period == 0) {
        return {1.0};
    }
    std::vector<double> polynomial(coefficients.size() * period + 1, 0.0);
    polynomial[0] = 1.0;
    for (std::size_t index = 0; index < coefficients.size(); ++index) {
        const double value = moving_average ? coefficients[index] : -coefficients[index];
        polynomial[(index + 1) * period] = value;
    }
    return polynomial;
}

bool admissible_coefficients(const std::vector<double>& values)
{
    return std::all_of(values.cbegin(), values.cend(), [](double value) {
        return std::isfinite(value) && std::abs(value) < 0.95;
    });
}

double multiplicative_css_sse(
    const std::vector<double>& series,
    double intercept,
    const std::vector<double>& ar,
    const std::vector<double>& ma)
{
    const std::size_t max_lag = std::max(ar.size(), ma.size());
    if (max_lag == 0 || series.size() <= max_lag + 2) {
        return std::numeric_limits<double>::infinity();
    }
    std::vector<double> residuals(series.size(), 0.0);
    double sse = 0.0;
    std::size_t count = 0;
    for (std::size_t time = max_lag - 1; time < series.size(); ++time) {
        double ar_term = 0.0;
        for (std::size_t lag = 0; lag < ar.size(); ++lag) {
            ar_term += ar[lag] * (series[time - lag] - intercept);
        }
        double ma_term = 0.0;
        for (std::size_t lag = 1; lag < ma.size() && lag <= time; ++lag) {
            ma_term += ma[lag] * residuals[time - lag];
        }
        residuals[time] = ar_term - ma_term;
        if (!std::isfinite(residuals[time])) {
            return std::numeric_limits<double>::infinity();
        }
        sse += residuals[time] * residuals[time];
        ++count;
    }
    if (count < 3) {
        return std::numeric_limits<double>::infinity();
    }
    return sse;
}

bool fit_multiplicative_sarima_css(
    const std::vector<double>& series,
    const SarimaOrder& order,
    double& sse,
    double& aic,
    double& aicc,
    double& bic)
{
    const double n = static_cast<double>(series.size());
    const double intercept = std::accumulate(series.cbegin(), series.cend(), 0.0) / n;
    std::vector<double> ar_nonseasonal(order.p, 0.0);
    std::vector<double> ma_nonseasonal(order.q, 0.0);
    std::vector<double> ar_seasonal(order.seasonal_p, 0.0);
    std::vector<double> ma_seasonal(order.seasonal_q, 0.0);

    const auto evaluate = [&]() {
        if (!admissible_coefficients(ar_nonseasonal)
            || !admissible_coefficients(ma_nonseasonal)
            || !admissible_coefficients(ar_seasonal)
            || !admissible_coefficients(ma_seasonal)) {
            return std::numeric_limits<double>::infinity();
        }
        const std::vector<double> ar = multiply_polynomials(
            ar_operator(ar_nonseasonal),
            seasonal_operator(ar_seasonal, order.seasonal_period, false));
        const std::vector<double> ma = multiply_polynomials(
            ma_operator(ma_nonseasonal),
            seasonal_operator(ma_seasonal, order.seasonal_period, true));
        return multiplicative_css_sse(series, intercept, ar, ma);
    };

    sse = evaluate();
    std::vector<double>* groups[] = {
        &ar_nonseasonal, &ma_nonseasonal, &ar_seasonal, &ma_seasonal};
    for (int iteration = 0; iteration < 12; ++iteration) {
        bool improved = false;
        for (std::vector<double>* group : groups) {
            for (double& coefficient : *group) {
                const double original = coefficient;
                double best_local = sse;
                double best_value = original;
                for (int step = -8; step <= 8; ++step) {
                    coefficient = static_cast<double>(step) * 0.1;
                    const double current = evaluate();
                    if (current + 1.0e-12 < best_local) {
                        best_local = current;
                        best_value = coefficient;
                        improved = true;
                    }
                }
                coefficient = best_value;
                sse = best_local;
            }
        }
        if (!improved) {
            break;
        }
    }

    if (!std::isfinite(sse) || sse <= 0.0) {
        return false;
    }
    const double parameters = 1.0 + static_cast<double>(
        order.p + order.q + order.seasonal_p + order.seasonal_q);
    const double variance = std::max(sse / n, kEpsilon);
    const double log_likelihood =
        -0.5 * n * (std::log(2.0 * std::acos(-1.0)) + 1.0 + std::log(variance));
    aic = -2.0 * log_likelihood + 2.0 * parameters;
    bic = -2.0 * log_likelihood + parameters * std::log(n);
    const double denominator = n - parameters - 1.0;
    aicc = denominator > 0.0
        ? aic + 2.0 * parameters * (parameters + 1.0) / denominator
        : aic;
    return true;
}

}  // namespace

SeasonalForecastingResult fit_seasonal_forecasting(
    const std::vector<double>& observations,
    const SeasonalForecastingOptions& options)
{
    SeasonalForecastingResult result;
    result.options = options;
    if (!valid_options(options, result.diagnostics)) {
        return result;
    }
    const std::size_t period = options.seasonal_period;
    if (observations.size() < std::max<std::size_t>(4, 2 * period)
        || !finite_values(observations)) {
        diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                   observations.empty() ? "empty_observations"
                                         : "invalid_observations",
                   observations.empty()
                       ? "季节性预测至少需要有限的非空观测序列。"
                       : "观测序列必须全部为有限数值。");
        return result;
    }
    if (options.error_model == SeasonalErrorModel::multiplicative
        && std::any_of(observations.cbegin(), observations.cend(),
                       [](double value) { return value <= 0.0; })) {
        diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                   "non_positive_observation",
                   "乘法 ETS 要求所有观测值为正数。");
        return result;
    }

    const std::size_t cycle_count = observations.size() / period;
    std::vector<double> cycle_means(cycle_count, 0.0);
    result.seasonal.assign(period, options.error_model
                                           == SeasonalErrorModel::additive
                                       ? 0.0
                                       : 1.0);
    for (std::size_t cycle = 0; cycle < cycle_count; ++cycle) {
        for (std::size_t phase = 0; phase < period; ++phase) {
            cycle_means[cycle] += observations[cycle * period + phase];
        }
        cycle_means[cycle] /= static_cast<double>(period);
    }
    const double first_mean = cycle_means.front();
    for (std::size_t phase = 0; phase < period; ++phase) {
        double phase_mean = 0.0;
        for (std::size_t cycle = 0; cycle < cycle_count; ++cycle) {
            phase_mean += observations[cycle * period + phase];
        }
        phase_mean /= static_cast<double>(cycle_count);
        if (options.error_model == SeasonalErrorModel::additive) {
            result.seasonal[phase] = phase_mean - first_mean;
        } else {
            result.seasonal[phase] =
                std::max(phase_mean / std::max(first_mean, kEpsilon), kEpsilon);
        }
    }
    if (options.error_model == SeasonalErrorModel::additive) {
        const double mean_seasonal =
            std::accumulate(result.seasonal.cbegin(), result.seasonal.cend(), 0.0)
            / static_cast<double>(period);
        for (double& value : result.seasonal) {
            value -= mean_seasonal;
        }
    } else {
        double mean_seasonal =
            std::accumulate(result.seasonal.cbegin(), result.seasonal.cend(), 0.0)
            / static_cast<double>(period);
        for (double& value : result.seasonal) {
            value /= std::max(mean_seasonal, kEpsilon);
        }
    }
    result.level = seasonal_adjustment(observations.front(), result.seasonal[0],
                                       options.error_model);
    if (options.trend_model == TrendModel::additive) {
        result.trend = cycle_count > 1
                           ? (cycle_means[1] - cycle_means[0])
                                 / static_cast<double>(period)
                           : 0.0;
    } else if (options.trend_model == TrendModel::multiplicative) {
        result.trend = cycle_count > 1
                           ? std::max(cycle_means[1]
                                          / std::max(cycle_means[0], kEpsilon),
                                      kEpsilon)
                           : 1.0;
    }
    double previous_level = result.level;
    double previous_trend = result.trend;
    result.fitted.resize(observations.size());
    result.residuals.resize(observations.size());
    for (std::size_t index = 0; index < observations.size(); ++index) {
        const std::size_t phase = index % period;
        const double old_seasonal = result.seasonal[phase];
        double prior_base = previous_level;
        if (options.trend_model == TrendModel::additive) {
            prior_base += previous_trend;
        } else if (options.trend_model == TrendModel::multiplicative) {
            prior_base *= previous_trend;
        }
        result.fitted[index] =
            options.error_model == SeasonalErrorModel::additive
                ? prior_base + old_seasonal
                : prior_base * old_seasonal;
        result.residuals[index] = observations[index] - result.fitted[index];

        double predicted_level = prior_base;
        if (options.error_model == SeasonalErrorModel::additive) {
            predicted_level = options.alpha
                                  * (observations[index] - old_seasonal)
                              + (1.0 - options.alpha) * prior_base;
        } else {
            predicted_level =
                options.alpha * (observations[index]
                                 / std::max(old_seasonal, kEpsilon))
                + (1.0 - options.alpha) * prior_base;
        }
        double updated_trend = previous_trend;
        if (options.trend_model == TrendModel::additive) {
            updated_trend = options.beta * (predicted_level - previous_level)
                            + (1.0 - options.beta)
                                  * (options.damped_trend
                                         ? options.damping_phi * previous_trend
                                         : previous_trend);
        } else if (options.trend_model == TrendModel::multiplicative) {
            const double ratio = predicted_level / std::max(
                                                      std::abs(previous_level),
                                                      kEpsilon);
            updated_trend =
                options.beta * ratio
                + (1.0 - options.beta)
                      * (options.damped_trend
                             ? std::pow(previous_trend, options.damping_phi)
                             : previous_trend);
            updated_trend = std::max(updated_trend, kEpsilon);
        }
        double updated_seasonal = old_seasonal;
        if (options.error_model == SeasonalErrorModel::additive) {
            updated_seasonal =
                options.gamma * (observations[index] - predicted_level)
                + (1.0 - options.gamma) * old_seasonal;
        } else {
            updated_seasonal =
                options.gamma
                    * (observations[index] / std::max(predicted_level, kEpsilon))
                + (1.0 - options.gamma) * old_seasonal;
            updated_seasonal = std::max(updated_seasonal, kEpsilon);
        }
        result.seasonal[phase] = updated_seasonal;
        previous_level = predicted_level;
        previous_trend = updated_trend;
    }
    result.level = previous_level;
    result.trend = previous_trend;

    const double residual_variance =
        std::max(kEpsilon, std::inner_product(result.residuals.cbegin(),
                                               result.residuals.cend(),
                                               result.residuals.cbegin(), 0.0)
                               / static_cast<double>(result.residuals.size()));
    result.forecasts.reserve(options.forecast_periods);
    result.lower.reserve(options.forecast_periods);
    result.upper.reserve(options.forecast_periods);
    const double z = normal_multiplier(options.confidence_level);
    for (std::size_t horizon = 1; horizon <= options.forecast_periods; ++horizon) {
        double base = result.level;
        if (options.trend_model == TrendModel::additive) {
            base += damped_additive_trend(result.trend, horizon,
                                          options.damped_trend,
                                          options.damping_phi);
        } else if (options.trend_model == TrendModel::multiplicative) {
            base *= damped_multiplicative_trend(result.trend, horizon,
                                                options.damped_trend,
                                                options.damping_phi);
        }
        const double seasonal =
            result.seasonal[(observations.size() + horizon - 1) % period];
        const double forecast =
            options.error_model == SeasonalErrorModel::additive
                ? base + seasonal
                : base * seasonal;
        const double margin = z * std::sqrt(residual_variance
                                            * static_cast<double>(horizon));
        result.forecasts.push_back(forecast);
        result.lower.push_back(forecast - margin);
        result.upper.push_back(forecast + margin);
    }
    calculate_metrics(observations, result.fitted, result.metrics,
                      result.diagnostics);
    calculate_mase(observations, result.metrics, result.diagnostics);
    if (options.confidence_level != 0.95) {
        diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                   "approximate_interval_quantile",
                   "当前预测区间使用 95% 正态近似系数，非 95% 置信水平仅作近似。");
    }
    return result;
}

RollingOriginResult rolling_origin_validate(
    const std::vector<double>& observations,
    const RollingOriginOptions& options)
{
    RollingOriginResult result;
    if (!finite_values(observations) || observations.empty()) {
        diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                   "invalid_validation_observations",
                   "滚动验证要求非空且全部为有限数值的观测序列。");
        return result;
    }
    const std::size_t minimum_training =
        std::max<std::size_t>(4, 2 * options.model_options.seasonal_period);
    const std::size_t initial =
        options.initial_training_size == 0 ? minimum_training
                                           : options.initial_training_size;
    if (options.horizon == 0 || options.step == 0 || initial < minimum_training
        || initial + options.horizon > observations.size()) {
        diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                   "invalid_rolling_origin_options",
                   "滚动验证的初始样本、步长和预测跨度不足以形成有效起点。");
        return result;
    }
    for (std::size_t origin = initial;
         origin + options.horizon <= observations.size();
         origin += options.step) {
        std::vector<double> training(observations.cbegin(),
                                     observations.cbegin() + origin);
        SeasonalForecastingOptions model_options = options.model_options;
        model_options.forecast_periods = options.horizon;
        const SeasonalForecastingResult fitted =
            fit_seasonal_forecasting(training, model_options);
        bool failed = false;
        for (const DiagnosticMessage& item : fitted.diagnostics) {
            if (item.severity == DiagnosticMessage::Severity::error) {
                failed = true;
                break;
            }
        }
        if (failed || fitted.forecasts.size() != options.horizon) {
            diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "rolling_origin_fit_failed",
                       "至少一个滚动起点无法完成季节性模型拟合。");
            return result;
        }
        result.origins.push_back(origin);
        for (std::size_t step = 0; step < options.horizon; ++step) {
            result.actuals.push_back(observations[origin + step]);
            result.forecasts.push_back(fitted.forecasts[step]);
            result.lower.push_back(fitted.lower[step]);
            result.upper.push_back(fitted.upper[step]);
        }
        if (origin + options.step > observations.size() - options.horizon) {
            break;
        }
    }
    calculate_metrics(result.actuals, result.forecasts, result.metrics,
                      result.diagnostics);
    calculate_mase(observations, result.metrics, result.diagnostics);
    return result;
}

SarimaForecastingResult forecast_fixed_sarima(
    const std::vector<double>& observations,
    const FixedSarimaParameters& parameters,
    std::size_t forecast_periods,
    double confidence_level)
{
    SarimaForecastingResult result;
    result.parameters = parameters;
    if (forecast_periods == 0 || confidence_level <= 0.0
        || confidence_level >= 1.0) {
        diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                   "invalid_sarima_options",
                   "SARIMA 预测期数和置信水平必须有效。");
        return result;
    }
    if (!finite_values(observations) || observations.empty()) {
        diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                   "invalid_sarima_observations",
                   "SARIMA 观测序列必须非空且全部为有限数值。");
        return result;
    }
    if (!valid_sarima_parameters(parameters, result.diagnostics)) {
        return result;
    }

    const SarimaOrder& order = parameters.order;
    const std::size_t difference_lag = differencing_lag(order);
    const std::size_t maximum_model_lag =
        std::max(order.p, order.seasonal_p * order.seasonal_period);
    const std::size_t required_history =
        difference_lag + std::max(maximum_model_lag,
                                  std::max(order.q,
                                           order.seasonal_q
                                               * order.seasonal_period));
    if (observations.size() <= difference_lag
        || observations.size() - difference_lag <= maximum_model_lag
        || observations.size() <= required_history) {
        diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                   "insufficient_sarima_observations",
                   "观测数量不足以完成指定差分和固定阶数递推。");
        return result;
    }

    // Fit the fixed recursion on the differenced observations. Future
    // innovations are set to zero, which gives deterministic forecasts.
    const std::vector<double> differenced =
        difference_series(observations, order);
    std::vector<double> fitted_differenced(differenced.size(), 0.0);
    std::vector<double> innovations(differenced.size(), 0.0);
    const std::size_t seasonal_period = order.seasonal_period;
    for (std::size_t index = 0; index < differenced.size(); ++index) {
        double fitted = parameters.intercept;
        for (std::size_t lag = 1; lag <= order.p; ++lag) {
            if (index >= lag) {
                fitted += parameters.ar[lag - 1]
                          * differenced[index - lag];
            }
        }
        for (std::size_t lag = 1; lag <= order.seasonal_p; ++lag) {
            const std::size_t offset = lag * seasonal_period;
            if (index >= offset) {
                fitted += parameters.seasonal_ar[lag - 1]
                          * differenced[index - offset];
            }
        }
        for (std::size_t lag = 1; lag <= order.q; ++lag) {
            if (index >= lag) {
                fitted += parameters.ma[lag - 1] * innovations[index - lag];
            }
        }
        for (std::size_t lag = 1; lag <= order.seasonal_q; ++lag) {
            const std::size_t offset = lag * seasonal_period;
            if (index >= offset) {
                fitted += parameters.seasonal_ma[lag - 1]
                          * innovations[index - offset];
            }
        }
        fitted_differenced[index] = fitted;
        innovations[index] = differenced[index] - fitted;
    }

    std::vector<double> recursive_values = differenced;
    std::vector<double> recursive_innovations = innovations;
    for (std::size_t step = 0; step < forecast_periods; ++step) {
        const std::size_t index = recursive_values.size();
        double forecast = parameters.intercept;
        for (std::size_t lag = 1; lag <= order.p; ++lag) {
            forecast += parameters.ar[lag - 1]
                        * recursive_values[index - lag];
        }
        for (std::size_t lag = 1; lag <= order.seasonal_p; ++lag) {
            forecast += parameters.seasonal_ar[lag - 1]
                        * recursive_values[index - lag * seasonal_period];
        }
        for (std::size_t lag = 1; lag <= order.q; ++lag) {
            forecast += parameters.ma[lag - 1]
                        * recursive_innovations[index - lag];
        }
        for (std::size_t lag = 1; lag <= order.seasonal_q; ++lag) {
            forecast += parameters.seasonal_ma[lag - 1]
                        * recursive_innovations[index - lag * seasonal_period];
        }
        recursive_values.push_back(forecast);
        recursive_innovations.push_back(0.0);
    }

    const std::vector<double> reconstructed =
        inverse_difference_series(recursive_values, observations, order);
    const std::vector<double> fitted_reconstructed =
        inverse_difference_series(fitted_differenced, observations, order);
    const std::vector<double> fitted_original(
        fitted_reconstructed.cbegin() + difference_lag,
        fitted_reconstructed.cend());
    const std::size_t forecast_start = observations.size();
    const double z = normal_multiplier(confidence_level);
    const double standard_deviation =
        std::sqrt(std::max(0.0, parameters.innovation_variance));
    for (std::size_t step = 0; step < forecast_periods; ++step) {
        const double forecast = reconstructed[forecast_start + step];
        const double margin = z * standard_deviation
                              * std::sqrt(static_cast<double>(step + 1));
        result.forecasts.push_back(forecast);
        result.lower.push_back(forecast - margin);
        result.upper.push_back(forecast + margin);
    }
    const std::vector<double> actual_tail(
        observations.cbegin() + difference_lag, observations.cend());
    calculate_metrics(actual_tail, fitted_original, result.metrics,
                      result.diagnostics);
    calculate_mase(observations, result.metrics, result.diagnostics);
    if (confidence_level != 0.95) {
        diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                   "approximate_interval_quantile",
                   "当前预测区间使用 95% 正态近似系数，非 95% 置信水平仅作近似。");
    }
    diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
               "sarima_fixed_recursion",
               "已使用固定参数 SARIMA 递推；未来创新项按零处理，区间为创新方差的正态近似。");
    return result;
}

std::vector<SarimaCandidateResult> fit_best_sarima_candidates_impl(
    const std::vector<double>& observations,
    const std::size_t seasonal_period,
    const int max_p,
    const int max_q,
    const int max_d,
    const int max_seasonal_p,
    const int max_seasonal_q,
    const int max_seasonal_d)
{
    std::vector<SarimaCandidateResult> candidates;
    if (!finite_values(observations) || observations.size() < seasonal_period + 4
        || seasonal_period < 2) {
        return candidates;
    }
    const int bounded_p = std::clamp(max_p, 0, 2);
    const int bounded_q = std::clamp(max_q, 0, 2);
    const int bounded_d = std::clamp(max_d, 0, 1);
    const int bounded_sp = std::clamp(max_seasonal_p, 0, 1);
    const int bounded_sq = std::clamp(max_seasonal_q, 0, 1);
    const int bounded_sd = std::clamp(max_seasonal_d, 0, 1);
    for (int seasonal_d = 0; seasonal_d <= bounded_sd; ++seasonal_d) {
        for (int d = 0; d <= bounded_d; ++d) {
            for (int p = 0; p <= bounded_p; ++p) {
                for (int q = 0; q <= bounded_q; ++q) {
                    for (int seasonal_p = 0; seasonal_p <= bounded_sp; ++seasonal_p) {
                        for (int seasonal_q = 0; seasonal_q <= bounded_sq; ++seasonal_q) {
                            if (p == 0 && q == 0 && d == 0 && seasonal_p == 0
                                && seasonal_q == 0 && seasonal_d == 0) {
                                continue;
                            }
                            SarimaOrder order{
                                static_cast<std::size_t>(p),
                                static_cast<std::size_t>(d),
                                static_cast<std::size_t>(q),
                                static_cast<std::size_t>(seasonal_p),
                                static_cast<std::size_t>(seasonal_d),
                                static_cast<std::size_t>(seasonal_q),
                                seasonal_period};
                            const std::vector<double> differenced =
                                difference_series(observations, order);
                            if (differenced.size() < 8) {
                                continue;
                            }
                            SarimaCandidateResult candidate;
                            candidate.order = order;
                            if (!fit_multiplicative_sarima_css(
                                    differenced, order, candidate.sse, candidate.aic,
                                    candidate.aicc, candidate.bic)) {
                                continue;
                            }
                            candidate.diagnostics.push_back({
                                DiagnosticMessage::Severity::info,
                                "sarima_css_approximation",
                                "SARIMA 候选使用乘法多项式条件最小二乘；"
                                "与 Minitab 迭代最小二乘（Meeker TSERIES，含 back forecast）"
                                "的阶或数值可能不同。"});
                            candidates.push_back(std::move(candidate));
                        }
                    }
                }
            }
        }
    }
    return candidates;
}

std::string sarima_order_label(const SarimaOrder& sarima_order)
{
    return arima_order_label({static_cast<int>(sarima_order.p),
                              static_cast<int>(sarima_order.d),
                              static_cast<int>(sarima_order.q)})
        + "(" + std::to_string(sarima_order.seasonal_p) + ','
        + std::to_string(sarima_order.seasonal_d) + ','
        + std::to_string(sarima_order.seasonal_q) + ")_"
        + std::to_string(sarima_order.seasonal_period);
}

std::vector<SarimaCandidateResult> fit_best_sarima_candidates(
    const std::vector<double>& observations,
    const std::size_t seasonal_period,
    const int max_p,
    const int max_q,
    const int max_d,
    const int max_seasonal_p,
    const int max_seasonal_q,
    const int max_seasonal_d)
{
    return fit_best_sarima_candidates_impl(
        observations, seasonal_period, max_p, max_q, max_d,
        max_seasonal_p, max_seasonal_q, max_seasonal_d);
}

}  // namespace datalab::domain::statistics
