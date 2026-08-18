#include "domain/statistics/arima.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>

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

std::size_t minimum_sample_size(const ArimaOrder& order)
{
    return static_cast<std::size_t>(std::max({order.p, order.q, order.d, 1}) + 3);
}

int parameter_count(const ArimaOrder& order)
{
    int count = 1;
    if (order.p > 0 || order.q > 0) {
        count += order.p + order.q;
    } else if (order.d > 0) {
        ++count;
    }
    return count;
}

int parameter_count(ArimaModel model)
{
    return model == ArimaModel::arima_0_1_0 ? 2 : 3;
}

void information_criteria(ArimaResult& result, std::size_t n, int parameters)
{
    const double variance = std::max(result.sse / static_cast<double>(n),
                                      kMinimumVariance);
    const double parameter_count_value = static_cast<double>(parameters);
    result.log_likelihood =
        -0.5 * static_cast<double>(n)
        * (std::log(2.0 * std::acos(-1.0)) + 1.0 + std::log(variance));
    result.aic = -2.0 * result.log_likelihood + 2.0 * parameter_count_value;
    result.bic = -2.0 * result.log_likelihood
                 + parameter_count_value * std::log(static_cast<double>(n));
    const double denominator = static_cast<double>(n) - parameter_count_value - 1.0;
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

void intervals(ArimaResult& result, double variance, int forecast_periods, int q)
{
    for (int index = 0; index < forecast_periods; ++index) {
        const double horizon = static_cast<double>(index + 1);
        const double multiplier = q > 0 ? 1.0 + 0.25 * horizon : horizon;
        const double margin =
            kNormal95 * std::sqrt(std::max(0.0, variance * multiplier));
        result.lower.push_back(result.forecasts[static_cast<std::size_t>(index)] - margin);
        result.upper.push_back(result.forecasts[static_cast<std::size_t>(index)] + margin);
    }
}

std::vector<double> difference_series(const std::vector<double>& observations, int d)
{
    std::vector<double> current = observations;
    for (int step = 0; step < d; ++step) {
        if (current.size() < 2) {
            return {};
        }
        std::vector<double> differenced;
        differenced.reserve(current.size() - 1);
        for (std::size_t index = 1; index < current.size(); ++index) {
            differenced.push_back(current[index] - current[index - 1]);
        }
        current = std::move(differenced);
    }
    return current;
}

std::vector<double> integrate_forecasts(
    const std::vector<double>& last_levels,
    const std::vector<double>& diff_forecasts,
    int d)
{
    if (d <= 0) {
        return diff_forecasts;
    }
    std::vector<double> levels = last_levels;
    std::vector<double> forecasts;
    forecasts.reserve(diff_forecasts.size());
    for (const double increment : diff_forecasts) {
        levels.push_back(levels.back() + increment);
        forecasts.push_back(levels.back());
    }
    return forecasts;
}

bool fit_ar_ols(
    const std::vector<double>& series,
    int p,
    std::vector<double>& ar_coefficients,
    double& intercept,
    std::vector<double>& residuals,
    double& sse)
{
    if (p <= 0 || series.size() <= static_cast<std::size_t>(p)) {
        return false;
    }
    const std::size_t n = series.size() - static_cast<std::size_t>(p);
    const std::size_t columns = static_cast<std::size_t>(p) + 1;
    std::vector<std::vector<double>> normal(columns, std::vector<double>(columns, 0.0));
    std::vector<double> rhs(columns, 0.0);
    for (std::size_t row = static_cast<std::size_t>(p); row < series.size(); ++row) {
        std::vector<double> design(columns, 1.0);
        for (int lag = 1; lag <= p; ++lag) {
            design[static_cast<std::size_t>(lag)] =
                series[row - static_cast<std::size_t>(lag)];
        }
        for (std::size_t i = 0; i < columns; ++i) {
            rhs[i] += design[i] * series[row];
            for (std::size_t j = 0; j < columns; ++j) {
                normal[i][j] += design[i] * design[j];
            }
        }
    }
    std::vector<std::vector<double>> augmented = normal;
    for (std::size_t row = 0; row < columns; ++row) {
        augmented[row].push_back(rhs[row]);
    }
    for (std::size_t column = 0; column < columns; ++column) {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < columns; ++row) {
            if (std::abs(augmented[row][column])
                > std::abs(augmented[pivot][column])) {
                pivot = row;
            }
        }
        if (std::abs(augmented[pivot][column]) < 1.0e-10) {
            return false;
        }
        std::swap(augmented[pivot], augmented[column]);
        const double divisor = augmented[column][column];
        for (double& value : augmented[column]) {
            value /= divisor;
        }
        for (std::size_t row = 0; row < columns; ++row) {
            if (row == column) {
                continue;
            }
            const double factor = augmented[row][column];
            for (std::size_t value = 0; value <= columns; ++value) {
                augmented[row][value] -= factor * augmented[column][value];
            }
        }
    }
    std::vector<double> beta(columns, 0.0);
    for (std::size_t row = 0; row < columns; ++row) {
        beta[row] = augmented[row][columns];
    }
    intercept = beta[0];
    ar_coefficients.assign(static_cast<std::size_t>(p), 0.0);
    for (int lag = 1; lag <= p; ++lag) {
        ar_coefficients[static_cast<std::size_t>(lag - 1)] = beta[static_cast<std::size_t>(lag)];
    }
    residuals.assign(series.size(), 0.0);
    sse = 0.0;
    for (std::size_t row = static_cast<std::size_t>(p); row < series.size(); ++row) {
        double fitted = intercept;
        for (int lag = 1; lag <= p; ++lag) {
            fitted += ar_coefficients[static_cast<std::size_t>(lag - 1)]
                * series[row - static_cast<std::size_t>(lag)];
        }
        residuals[row] = series[row] - fitted;
        sse += residuals[row] * residuals[row];
    }
    return std::isfinite(sse);
}

bool fit_ma_grid(
    const std::vector<double>& series,
    int q,
    std::vector<double>& ma_coefficients,
    double& intercept,
    std::vector<double>& residuals,
    double& sse)
{
    if (q <= 0 || series.size() < 3) {
        return false;
    }
    intercept = std::accumulate(series.begin(), series.end(), 0.0)
        / static_cast<double>(series.size());
    double best_sse = std::numeric_limits<double>::infinity();
    std::vector<double> best_theta(static_cast<std::size_t>(q), 0.0);
    const int grid = q == 1 ? 199 : 41;
    if (q == 1) {
        for (int step = -99; step <= 99; ++step) {
            const double theta = static_cast<double>(step) / 100.0;
            std::vector<double> errors(series.size(), 0.0);
            double current_sse = 0.0;
            for (std::size_t index = 1; index < series.size(); ++index) {
                const double residual =
                    series[index] - intercept - theta * errors[index - 1];
                errors[index] = residual;
                current_sse += residual * residual;
            }
            if (current_sse < best_sse) {
                best_sse = current_sse;
                best_theta[0] = theta;
            }
        }
    } else {
        return false;
    }
    ma_coefficients = best_theta;
    residuals.assign(series.size(), 0.0);
    sse = 0.0;
    for (std::size_t index = 1; index < series.size(); ++index) {
        const double fitted = intercept + ma_coefficients[0] * residuals[index - 1];
        residuals[index] = series[index] - fitted;
        sse += residuals[index] * residuals[index];
    }
    return std::isfinite(sse);
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
    information_criteria(result, n, parameter_count(result.model));
    intervals(result, std::max(result.sse / static_cast<double>(n), kMinimumVariance),
              static_cast<int>(forecast_periods), 0);
}

void fit_ar_one(ArimaResult& result, const std::vector<double>& observations,
                std::size_t forecast_periods)
{
    const std::size_t n = observations.size() - 1;
    if (!fit_ar_ols(observations, 1, result.ar_coefficients, result.intercept,
                    result.residuals, result.sse)) {
        error(result, "ar_fit_failed", "AR(1) 拟合失败。");
        return;
    }
    result.coefficient = result.ar_coefficients.front();
    result.fitted.resize(observations.size());
    result.fitted.front() = observations.front();
    for (std::size_t index = 1; index < observations.size(); ++index) {
        result.fitted[index] = result.intercept
            + result.coefficient * observations[index - 1];
    }
    result.forecasts.reserve(forecast_periods);
    double previous = observations.back();
    for (std::size_t index = 0; index < forecast_periods; ++index) {
        previous = result.intercept + result.coefficient * previous;
        result.forecasts.push_back(previous);
    }
    information_criteria(result, n, parameter_count(result.model));
    intervals(result, std::max(result.sse / static_cast<double>(n), kMinimumVariance),
              static_cast<int>(forecast_periods), 0);
}

void fit_ma_one(ArimaResult& result, const std::vector<double>& observations,
                std::size_t forecast_periods)
{
    const std::size_t n = observations.size() - 1;
    if (!fit_ma_grid(observations, 1, result.ma_coefficients, result.intercept,
                     result.residuals, result.sse)) {
        error(result, "ma_fit_failed", "MA(1) 拟合失败。");
        return;
    }
    result.coefficient = result.ma_coefficients.front();
    result.fitted.resize(observations.size());
    result.fitted.front() = observations.front();
    for (std::size_t index = 1; index < observations.size(); ++index) {
        result.fitted[index] = result.intercept
            + result.coefficient * result.residuals[index - 1];
    }
    result.forecasts.reserve(forecast_periods);
    result.forecasts.push_back(
        result.intercept + result.coefficient * result.residuals.back());
    for (std::size_t index = 1; index < forecast_periods; ++index) {
        result.forecasts.push_back(result.intercept);
    }
    information_criteria(result, n, parameter_count(result.model));
    intervals(result, std::max(result.sse / static_cast<double>(n), kMinimumVariance),
              static_cast<int>(forecast_periods), 1);
}

bool fit_on_differenced_series(
    ArimaResult& result,
    const std::vector<double>& observations,
    const ArimaOrder& order,
    std::size_t forecast_periods)
{
    const std::vector<double> differenced =
        order.d > 0 ? difference_series(observations, order.d) : observations;
    if (differenced.size() < minimum_sample_size(order)) {
        error(result, "insufficient_differenced_samples",
              "差分后样本不足以拟合 ARIMA 阶数。");
        return false;
    }
    std::vector<double> diff_forecasts;
    const std::size_t effective_n = differenced.size()
        - static_cast<std::size_t>(std::max(order.p, 1));
    if (order.p > 0 && order.q == 0) {
        if (!fit_ar_ols(differenced, order.p, result.ar_coefficients, result.intercept,
                        result.residuals, result.sse)) {
            error(result, "ar_fit_failed", "AR 拟合失败或设计矩阵秩亏。");
            return false;
        }
        result.coefficient = result.ar_coefficients.empty() ? 0.0 : result.ar_coefficients.back();
        diff_forecasts.assign(forecast_periods, result.intercept);
        std::vector<double> extended = differenced;
        for (std::size_t horizon = 0; horizon < forecast_periods; ++horizon) {
            double next = result.intercept;
            for (int lag = 1; lag <= order.p; ++lag) {
                const std::size_t index = extended.size() - static_cast<std::size_t>(lag);
                next += result.ar_coefficients[static_cast<std::size_t>(lag - 1)] * extended[index];
            }
            diff_forecasts[horizon] = next;
            extended.push_back(next);
        }
    } else if (order.q > 0 && order.p == 0) {
        if (!fit_ma_grid(differenced, order.q, result.ma_coefficients, result.intercept,
                         result.residuals, result.sse)) {
            error(result, "ma_fit_failed", "MA 拟合失败。");
            return false;
        }
        result.coefficient = result.ma_coefficients.empty() ? 0.0 : result.ma_coefficients.back();
        diff_forecasts.assign(forecast_periods, result.intercept);
        if (!result.residuals.empty()) {
            diff_forecasts[0] = result.intercept + result.coefficient * result.residuals.back();
        }
    } else if (order.p == 0 && order.q == 0) {
        result.intercept = std::accumulate(differenced.begin(), differenced.end(), 0.0)
            / static_cast<double>(differenced.size());
        result.sse = 0.0;
        for (const double value : differenced) {
            const double residual = value - result.intercept;
            result.sse += residual * residual;
        }
        diff_forecasts.assign(forecast_periods, result.intercept);
    } else {
        error(result, "unsupported_arima_order",
              "当前仅支持 AR(p,d,0) 与 MA(0,d,q) 候选，不含混合 ARMA 阶。");
        return false;
    }
    std::vector<double> last_levels;
    if (order.d > 0) {
        last_levels.push_back(observations[observations.size() - static_cast<std::size_t>(order.d) - 1]);
        for (int step = order.d - 1; step >= 0; --step) {
            last_levels.push_back(observations[observations.size() - static_cast<std::size_t>(step) - 1]);
        }
    }
    result.forecasts = order.d > 0
        ? integrate_forecasts(last_levels, diff_forecasts, order.d)
        : diff_forecasts;
    result.fitted.assign(observations.size(), observations.front());
    result.residuals.assign(observations.size(), 0.0);
    for (std::size_t index = 1; index < observations.size(); ++index) {
        result.fitted[index] = observations[index - 1];
        result.residuals[index] = observations[index] - result.fitted[index];
    }
    information_criteria(result, std::max<std::size_t>(1, effective_n), parameter_count(order));
    intervals(result, std::max(result.sse / static_cast<double>(std::max<std::size_t>(1, effective_n)),
                               kMinimumVariance),
              static_cast<int>(forecast_periods), order.q);
    return true;
}

bool has_error(const ArimaResult& result)
{
    return std::any_of(result.diagnostics.begin(), result.diagnostics.end(),
                       [](const DiagnosticMessage& message) {
                           return message.severity == DiagnosticMessage::Severity::error;
                       });
}

}  // namespace

std::string arima_order_label(const ArimaOrder& order)
{
    std::ostringstream stream;
    stream << "ARIMA(" << order.p << ',' << order.d << ',' << order.q << ')';
    return stream.str();
}

ArimaResult fit_arima(const std::vector<double>& observations, ArimaModel model,
                      std::size_t forecast_periods)
{
    ArimaResult result;
    result.model = model;
    switch (model) {
    case ArimaModel::arima_0_1_0:
        result.order = {0, 1, 0};
        break;
    case ArimaModel::ar_1:
        result.order = {1, 0, 0};
        break;
    case ArimaModel::ma_1:
        result.order = {0, 0, 1};
        break;
    }
    if (observations.size() < minimum_sample_size(model)) {
        error(result, "insufficient_samples",
              "ARIMA 候选要求更多的有限观测值才能拟合。");
        return result;
    }
    if (forecast_periods == 0) {
        error(result, "invalid_forecast_horizon", "预测期数必须大于零。");
        return result;
    }
    if (!finite_series(observations)) {
        error(result, "non_finite_observation", "观测序列必须全部为有限数值。");
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

ArimaResult fit_arima_order(
    const std::vector<double>& observations,
    const ArimaOrder& order,
    std::size_t forecast_periods)
{
    ArimaResult result;
    result.order = order;
    if (order.p < 0 || order.d < 0 || order.q < 0 || order.p > 3 || order.d > 2 || order.q > 3) {
        error(result, "unsupported_arima_order", "仅支持 p/q≤3、d≤2 的有限阶 ARIMA。");
        return result;
    }
    if (forecast_periods == 0) {
        error(result, "invalid_forecast_horizon", "预测期数必须大于零。");
        return result;
    }
    if (!finite_series(observations)) {
        error(result, "non_finite_observation", "观测序列必须全部为有限数值。");
        return result;
    }
    if (observations.size() < minimum_sample_size(order)) {
        error(result, "insufficient_samples",
              "ARIMA 候选要求更多的有限观测值才能拟合。");
        return result;
    }
    if (order.p == 0 && order.q == 0 && order.d == 0) {
        error(result, "degenerate_arima_order", "ARIMA(0,0,0) 不在候选网格内。");
        return result;
    }
    if (order.p > 0 && order.q > 0) {
        error(result, "unsupported_arima_order",
              "混合 ARMA 阶尚未纳入 Best ARIMA 网格。");
        return result;
    }
    if (order.p == 1 && order.d == 0 && order.q == 0) {
        result.model = ArimaModel::ar_1;
        fit_ar_one(result, observations, forecast_periods);
    } else if (order.p == 0 && order.d == 0 && order.q == 1) {
        result.model = ArimaModel::ma_1;
        fit_ma_one(result, observations, forecast_periods);
    } else if (order.p == 0 && order.d == 1 && order.q == 0) {
        result.model = ArimaModel::arima_0_1_0;
        fit_random_walk_with_drift(result, observations, forecast_periods);
    } else if (!fit_on_differenced_series(result, observations, order, forecast_periods)) {
        return result;
    }
    if (!std::isfinite(result.sse) || !std::isfinite(result.aic)
        || !std::isfinite(result.aicc) || !std::isfinite(result.bic)
        || !finite_series(result.forecasts) || !finite_series(result.lower)
        || !finite_series(result.upper)) {
        error(result, "non_finite_result",
              "模型计算产生了非有限结果，请检查数据尺度后重试。");
    }
    return result;
}

std::vector<ArimaResult> fit_arima_candidates(
    const std::vector<double>& observations,
    std::size_t forecast_periods,
    int max_p,
    int max_q,
    int max_d)
{
    const int bounded_p = std::clamp(max_p, 0, 3);
    const int bounded_q = std::clamp(max_q, 0, 3);
    const int bounded_d = std::clamp(max_d, 0, 2);
    std::vector<ArimaResult> candidates;
    for (int d = 0; d <= bounded_d; ++d) {
        for (int p = 0; p <= bounded_p; ++p) {
            for (int q = 0; q <= bounded_q; ++q) {
                if (p == 0 && q == 0 && d == 0) {
                    continue;
                }
                if (p > 0 && q > 0) {
                    continue;
                }
                ArimaOrder order{p, d, q};
                ArimaResult candidate =
                    fit_arima_order(observations, order, forecast_periods);
                if (!has_error(candidate)) {
                    candidates.push_back(std::move(candidate));
                }
            }
        }
    }
    if (candidates.empty()) {
        candidates.push_back(fit_arima(observations, ArimaModel::arima_0_1_0, forecast_periods));
    } else {
        const auto best_it = std::min_element(
            candidates.cbegin(), candidates.cend(),
            [](const ArimaResult& first, const ArimaResult& second) {
                return first.aicc < second.aicc;
            });
        if (best_it->order.p > 0 && best_it->order.d > 0) {
            for (ArimaResult& candidate : candidates) {
                if (candidate.order.p == best_it->order.p
                    && candidate.order.d == best_it->order.d
                    && candidate.order.q == best_it->order.q) {
                    warning(candidate, "arima_css_approximation",
                            "Best ARIMA 候选基于条件最小二乘与差分尺度 AICc；"
                            "与 Minitab 迭代最小二乘（Meeker TSERIES）的最优阶可能不同。");
                    break;
                }
            }
        }
    }
    return candidates;
}

}  // namespace datalab::domain::statistics
