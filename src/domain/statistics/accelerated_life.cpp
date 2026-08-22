#include "domain/statistics/accelerated_life.h"

#include "domain/statistics/reliability.h"

#include <algorithm>

#include <array>

#include <cmath>

#include <limits>

#include <set>

#include <vector>

namespace datalab::domain::statistics {

namespace {

using Matrix = std::vector<std::vector<double>>;
double arrhenius_transform(double celsius)
{
    return 11604.83 / (celsius + 273.16);
}

double normal_quantile(double p)
{
    if (p <= 0.0) {
        return -8.0;
    }
    if (p >= 1.0) {
        return 8.0;
    }
    const double a[] = {-3.969683028665376e+01, 2.209460984245205e+02,
                        -2.759285939946717e+02, 1.383577518672690e+02,
                        -3.066479806614716e+01, 2.506628277459239e+00};
    const double b[] = {-5.447609879822406e+01, 1.615858368580409e+02,
                        -1.556989798598866e+02, 6.680654618783063e+01,
                        -1.328068155288572e+01};
    const double c[] = {-7.784894002430293e-03, -3.226964378048109e-01,
                        -2.762148489337111e+00, -1.429488411088870e+00,
                        -5.473931734884816e+00};
    const double d[] = {7.784695709041462e-03, 3.224671290700397e-01,
                        2.445134137142996e+00, 3.754408661907416e+00};
    const double plow = 0.02425;
    const double phigh = 1.0 - plow;
    if (p < plow) {
        const double q = std::sqrt(-2.0 * std::log(p));
        return (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5])
            / ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    }
    if (p > phigh) {
        const double q = std::sqrt(-2.0 * std::log(1.0 - p));
        return -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5])
            / ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    }
    const double q = p - 0.5;
    const double r = q * q;
    return (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q
        / (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1.0);
}

double two_sided_normal_p(double z)
{
    return std::erfc(std::abs(z) / std::sqrt(2.0));
}

bool invert_matrix(const Matrix& input, Matrix& inverse)
{
    const std::size_t size = input.size();
    if (size == 0) {
        return false;
    }
    Matrix augmented(size, std::vector<double>(size * 2, 0.0));
    double scale = 0.0;
    const double pivot_limit = 1.0e-12;
    for (std::size_t row = 0; row < size; ++row) {
        if (input[row].size() != size) {
            return false;
        }
        for (std::size_t column = 0; column < size; ++column) {
            augmented[row][column] = input[row][column];
            scale = std::max(scale, std::abs(input[row][column]));
        }
        augmented[row][size + row] = 1.0;
    }
    const double limit = std::max(1.0, scale) * pivot_limit;
    for (std::size_t column = 0; column < size; ++column) {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < size; ++row) {
            if (std::abs(augmented[row][column])
                > std::abs(augmented[pivot][column])) {
                pivot = row;
            }
        }
        if (!std::isfinite(augmented[pivot][column])
            || std::abs(augmented[pivot][column]) <= limit) {
            return false;
        }
        std::swap(augmented[pivot], augmented[column]);
        const double divisor = augmented[column][column];
        for (double& value : augmented[column]) {
            value /= divisor;
        }
        for (std::size_t row = 0; row < size; ++row) {
            if (row == column) {
                continue;
            }
            const double factor = augmented[row][column];
            for (std::size_t value = 0; value < size * 2; ++value) {
                augmented[row][value] -= factor * augmented[column][value];
            }
        }
    }
    inverse.assign(size, std::vector<double>(size, 0.0));
    for (std::size_t row = 0; row < size; ++row) {
        for (std::size_t column = 0; column < size; ++column) {
            inverse[row][column] = augmented[row][size + column];
        }
    }
    return true;
}

double weibull_log_likelihood(
    double intercept,
    double slope,
    double log_shape,
    const std::vector<double>& log_times,
    const std::vector<bool>& events,
    const std::vector<double>& stress)
{
    const double shape = std::exp(log_shape);
    if (!(shape > 0.0)) {
        return -std::numeric_limits<double>::infinity();
    }
    double ll = 0.0;
    for (std::size_t i = 0; i < log_times.size(); ++i) {
        const double eta = intercept + slope * stress[i];
        const double scale = std::exp(eta);
        if (!(scale > 0.0)) {
            return -std::numeric_limits<double>::infinity();
        }
        const double z = std::pow(std::exp(log_times[i]) / scale, shape);
        if (events[i]) {
            ll += std::log(shape / scale)
                + (shape - 1.0) * (log_times[i] - eta)
                - z;
        } else {
            ll -= z;
        }
    }
    return ll;
}

void weibull_log_likelihood_gradient(
    double intercept,
    double slope,
    double log_shape,
    const std::vector<double>& log_times,
    const std::vector<bool>& events,
    const std::vector<double>& stress,
    std::array<double, 3>& gradient)
{
    gradient = {0.0, 0.0, 0.0};
    const double shape = std::exp(log_shape);
    if (!(shape > 0.0)) {
        return;
    }
    for (std::size_t i = 0; i < log_times.size(); ++i) {
        const double eta = intercept + slope * stress[i];
        const double log_time = log_times[i];
        const double z = std::exp(shape * (log_time - eta));
        const double delta = events[i] ? 1.0 : 0.0;
        const double g_eta = shape * (z - delta);
        gradient[0] += g_eta;
        gradient[1] += stress[i] * g_eta;
        const double u = 1.0 + shape * (log_time - eta);
        gradient[2] += delta * (1.0 - z) * u - (1.0 - delta) * z * u;
    }
}

void observed_information_matrix(
    double intercept,
    double slope,
    double log_shape,
    const std::vector<double>& log_times,
    const std::vector<bool>& events,
    const std::vector<double>& stress,
    Matrix& information)
{
    information.assign(3, std::vector<double>(3, 0.0));
    const double eps = 1.0e-5;
    std::array<double, 3> plus;
    std::array<double, 3> minus;
    for (std::size_t column = 0; column < 3; ++column) {
        std::array<double, 3> params = {intercept, slope, log_shape};
        params[column] += eps;
        weibull_log_likelihood_gradient(
            params[0], params[1], params[2], log_times, events, stress, plus);
        params[column] -= 2.0 * eps;
        weibull_log_likelihood_gradient(
            params[0], params[1], params[2], log_times, events, stress, minus);
        for (std::size_t row = 0; row < 3; ++row) {
            information[row][column] = -(plus[row] - minus[row]) / (2.0 * eps);
        }
    }
}

double initial_log_shape(
    const std::vector<double>& log_times,
    const std::vector<bool>& events)
{
    double sum = 0.0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < log_times.size(); ++i) {
        if (!events[i]) {
            continue;
        }
        sum += log_times[i];
        ++count;
    }
    if (count < 2) {
        return 0.0;
    }
    const double mean = sum / static_cast<double>(count);
    double variance = 0.0;
    for (std::size_t i = 0; i < log_times.size(); ++i) {
        if (!events[i]) {
            continue;
        }
        const double delta = log_times[i] - mean;
        variance += delta * delta;
    }
    variance /= static_cast<double>(count - 1);
    if (!(variance > 1.0e-12)) {
        return 0.0;
    }
    const double cv = std::sqrt(variance) / std::max(std::abs(mean), 1.0e-6);
    const double shape = std::max(0.2, 1.0 / std::max(cv, 1.0e-3));
    return std::log(shape);
}

void append_percentile_triplet(
    std::vector<AcceleratedLifePercentile>& target,
    double intercept,
    double slope,
    double shape,
    double stress_celsius)
{
    for (const double percentile : {10.0, 50.0, 90.0}) {
        AcceleratedLifePercentile row;
        row.stress_celsius = stress_celsius;
        row.percentile = percentile;
        row.life = accelerated_life_percentile_at_stress(
            intercept, slope, shape, stress_celsius, percentile);
        target.push_back(row);
    }
}

void build_life_stress_curve(
    std::vector<LifeStressCurvePoint>& curve,
    double intercept,
    double slope,
    double shape,
    double min_stress,
    double max_stress)
{
    curve.clear();
    if (!(min_stress < max_stress) || !(shape > 0.0)) {
        return;
    }
    constexpr std::size_t point_count = 25;
    for (const double percentile : {10.0, 50.0, 90.0}) {
        for (std::size_t index = 0; index < point_count; ++index) {
            const double fraction = static_cast<double>(index)
                / static_cast<double>(point_count - 1);
            const double stress = min_stress + fraction * (max_stress - min_stress);
            LifeStressCurvePoint point;
            point.stress_celsius = stress;
            point.percentile = percentile;
            point.life = accelerated_life_percentile_at_stress(
                intercept, slope, shape, stress, percentile);
            curve.push_back(point);
        }
    }
}

AcceleratedLifeCoefficient make_coefficient(
    const std::string& term,
    double estimate,
    double standard_error,
    double confidence_level)
{
    AcceleratedLifeCoefficient coefficient;
    coefficient.term = term;
    coefficient.estimate = estimate;
    coefficient.standard_error = standard_error;
    if (standard_error > 0.0) {
        coefficient.z_statistic = estimate / standard_error;
        coefficient.p_value = two_sided_normal_p(coefficient.z_statistic);
    }
    const double z_crit = normal_quantile(0.5 + confidence_level / 2.0);
    coefficient.confidence_lower = estimate - z_crit * standard_error;
    coefficient.confidence_upper = estimate + z_crit * standard_error;
    return coefficient;
}

}  // namespace
double accelerated_life_percentile_at_stress(
    double intercept,
    double slope,
    double shape,
    double stress_celsius,
    double percentile)
{
    if (!(shape > 0.0) || !std::isfinite(stress_celsius)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double stress_transform = arrhenius_transform(stress_celsius);
    const double scale = std::exp(intercept + slope * stress_transform);
    return percentile_life_weibull(shape, scale, percentile);
}

AcceleratedLifeResult fit_accelerated_life_weibull_arrhenius(
    const std::vector<double>& times,
    const std::vector<bool>& events,
    const std::vector<double>& stress_celsius,
    const std::vector<std::size_t>& source_rows,
    double confidence_level,
    double use_stress_celsius)
{
    (void)source_rows;
    AcceleratedLifeResult result;
    result.transform = "arrhenius";
    result.distribution = "weibull";
    result.use_stress_celsius = use_stress_celsius;
    if (times.size() < 6 || times.size() != events.size()
        || times.size() != stress_celsius.size()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "alt_invalid",
            "加速寿命需要 ≥6 行且寿命/事件/应力列对齐。"});
        return result;
    }
    std::vector<double> log_times;
    std::vector<bool> aligned_events;
    std::vector<double> stress;
    std::set<double> stress_levels;
    double min_stress = std::numeric_limits<double>::infinity();
    double max_stress = -std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < times.size(); ++i) {
        if (!(times[i] > 0.0) || !std::isfinite(times[i])
            || !std::isfinite(stress_celsius[i])) {
            continue;
        }
        log_times.push_back(std::log(times[i]));
        aligned_events.push_back(events[i]);
        const double x = arrhenius_transform(stress_celsius[i]);
        stress.push_back(x);
        stress_levels.insert(stress_celsius[i]);
        min_stress = std::min(min_stress, stress_celsius[i]);
        max_stress = std::max(max_stress, stress_celsius[i]);
    }
    result.observation_count = log_times.size();
    result.stress_level_count = stress_levels.size();
    if (result.observation_count < 6 || result.stress_level_count < 2) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "alt_levels",
            "ALT 需要 ≥6 有效观测与 ≥2 应力水平。"});
        return result;
    }
    for (bool event : aligned_events) {
        if (event) {
            ++result.failure_count;
        } else {
            ++result.censored_count;
        }
    }
    if (result.failure_count < 3) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "alt_failures",
            "ALT Weibull 至少需要 3 个失效观测。"});
        return result;
    }
    double sum_x = 0.0;
    double sum_y = 0.0;
    double sum_xx = 0.0;
    double sum_xy = 0.0;
    std::size_t fail_n = 0;
    for (std::size_t i = 0; i < log_times.size(); ++i) {
        if (!aligned_events[i]) {
            continue;
        }
        sum_x += stress[i];
        sum_y += log_times[i];
        sum_xx += stress[i] * stress[i];
        sum_xy += stress[i] * log_times[i];
        ++fail_n;
    }
    double intercept = 0.0;
    double slope = 0.0;
    const double denom = static_cast<double>(fail_n) * sum_xx - sum_x * sum_x;
    if (std::abs(denom) > 1.0e-12) {
        slope = (static_cast<double>(fail_n) * sum_xy - sum_x * sum_y) / denom;
        intercept = (sum_y - slope * sum_x) / static_cast<double>(fail_n);
    }
    double log_shape = initial_log_shape(log_times, aligned_events);
    std::array<double, 3> params = {intercept, slope, log_shape};
    double previous_ll = weibull_log_likelihood(
        params[0], params[1], params[2], log_times, aligned_events, stress);
    constexpr std::size_t max_iterations = 50;
    constexpr double tolerance = 1.0e-8;
    Matrix information;
    for (std::size_t iteration = 0; iteration < max_iterations; ++iteration) {
        std::array<double, 3> gradient = {0.0, 0.0, 0.0};
        weibull_log_likelihood_gradient(
            params[0], params[1], params[2], log_times, aligned_events, stress, gradient);
        observed_information_matrix(
            params[0], params[1], params[2], log_times, aligned_events, stress, information);
        Matrix covariance;
        if (!invert_matrix(information, covariance)) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "alt_singular_information",
                "ALT 观测信息矩阵奇异，无法求标准误。"});
            break;
        }
        std::array<double, 3> delta = {0.0, 0.0, 0.0};
        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t column = 0; column < 3; ++column) {
                delta[row] += covariance[row][column] * gradient[column];
            }
        }
        double step = 1.0;
        std::array<double, 3> candidate = params;
        double candidate_ll = previous_ll;
        for (int backtrack = 0; backtrack < 12; ++backtrack) {
            candidate[0] = params[0] + step * delta[0];
            candidate[1] = params[1] + step * delta[1];
            candidate[2] = params[2] + step * delta[2];
            candidate_ll = weibull_log_likelihood(
                candidate[0], candidate[1], candidate[2],
                log_times, aligned_events, stress);
            if (std::isfinite(candidate_ll) && candidate_ll >= previous_ll) {
                break;
            }
            step *= 0.5;
        }
        params = candidate;
        if (std::isfinite(candidate_ll)
            && std::abs(candidate_ll - previous_ll) < tolerance) {
            result.converged = true;
            break;
        }
        previous_ll = candidate_ll;
    }
    result.intercept = params[0];
    result.slope = params[1];
    result.log_shape = params[2];
    result.shape = std::exp(params[2]);
    result.log_likelihood = weibull_log_likelihood(
        params[0], params[1], params[2], log_times, aligned_events, stress);
    result.converged = result.converged && std::isfinite(result.log_likelihood);
    observed_information_matrix(
        params[0], params[1], params[2], log_times, aligned_events, stress, information);
    Matrix covariance;
    const bool have_covariance = invert_matrix(information, covariance);
    std::array<double, 3> standard_errors = {0.0, 0.0, 0.0};
    if (have_covariance) {
        for (std::size_t index = 0; index < 3; ++index) {
            standard_errors[index] = std::sqrt(std::max(0.0, covariance[index][index]));
        }
        result.shape_se = result.shape * standard_errors[2];
    } else {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "alt_information_invert",
            "无法反演观测信息矩阵；标准误不可用。"});
    }
    result.coefficients.push_back(make_coefficient(
        "Intercept", params[0], standard_errors[0], confidence_level));
    result.coefficients.push_back(make_coefficient(
        "Stress", params[1], standard_errors[1], confidence_level));
    append_percentile_triplet(
        result.percentiles_at_use_stress,
        params[0], params[1], result.shape, use_stress_celsius);
    if (!result.percentiles_at_use_stress.empty()) {
        result.b10_at_use_stress = result.percentiles_at_use_stress[0].life;
        result.b50_at_use_stress = result.percentiles_at_use_stress[1].life;
        result.b90_at_use_stress = result.percentiles_at_use_stress[2].life;
    }
    for (const double level : stress_levels) {
        append_percentile_triplet(
            result.percentiles_at_stress_levels,
            params[0], params[1], result.shape, level);
    }
    build_life_stress_curve(
        result.life_stress_curve,
        params[0], params[1], result.shape, min_stress, max_stress);
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "alt_newton_mle",
        "Weibull+Arrhenius 右删失数据采用 Newton-Raphson MLE 估计。"});
    if (have_covariance) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::info, "alt_observed_information",
            "回归系数与形状参数标准误来自 MLE 处观测信息矩阵。"});
    }
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "alt_scope",
        "Weibull+Arrhenius 窄化 ALT；非 Minitab golden；参数仅供调查。"});
    return result;
}

}  // namespace datalab::domain::statistics
