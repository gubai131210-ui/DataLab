#include "domain/statistics/johnson_transform.h"

#include "domain/quality_diagnostics.h"
#include "domain/statistics/normal_distribution.h"
#include "domain/statistics/normality_test.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace datalab::domain::statistics {
namespace {

constexpr double kEpsilon = 1.0e-12;

double acosh_safe(double value)
{
    if (value < 1.0 + 1.0e-15) {
        return 0.0;
    }
    return std::log(value + std::sqrt(value * value - 1.0));
}

double asinh_safe(double value)
{
    return std::log(value + std::sqrt(value * value + 1.0));
}

double sample_quantile(const std::vector<double>& sorted, double probability)
{
    if (sorted.empty() || !(probability >= 0.0) || probability > 1.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (sorted.size() == 1) {
        return sorted.front();
    }
    const double position = probability * static_cast<double>(sorted.size() - 1);
    const std::size_t lower = static_cast<std::size_t>(std::floor(position));
    const std::size_t upper = static_cast<std::size_t>(std::ceil(position));
    if (lower == upper || upper >= sorted.size()) {
        return sorted[std::min(lower, sorted.size() - 1)];
    }
    const double weight = position - static_cast<double>(lower);
    return sorted[lower] * (1.0 - weight) + sorted[upper] * weight;
}

bool in_support(const JohnsonParameters& parameters, double value)
{
    if (!std::isfinite(value)) {
        return false;
    }
    switch (parameters.family) {
    case JohnsonFamily::sb:
        return value > parameters.epsilon + kEpsilon
            && value < parameters.epsilon + parameters.lambda - kEpsilon;
    case JohnsonFamily::sl:
        return value > parameters.epsilon + kEpsilon;
    case JohnsonFamily::su:
        return true;
    case JohnsonFamily::none:
    default:
        return false;
    }
}

std::optional<JohnsonParameters> estimate_su(
    double z,
    double x_m3,
    double x_m1,
    double x_p1,
    double x_p3)
{
    const double m = x_p3 - x_p1;
    const double n = x_m1 - x_m3;
    const double p = x_p1 - x_m1;
    if (!(m > 0.0) || !(n > 0.0) || !(p > 0.0)) {
        return std::nullopt;
    }
    const double mp = m / p;
    const double np = n / p;
    const double product = mp * np;
    if (product <= 1.0 + 1.0e-8) {
        return std::nullopt;
    }
    JohnsonParameters parameters;
    parameters.family = JohnsonFamily::su;
    parameters.eta = 2.0 * z / acosh_safe(0.5 * (mp + np));
    if (!(parameters.eta > 0.0) || !std::isfinite(parameters.eta)) {
        return std::nullopt;
    }
    parameters.gamma = parameters.eta
        * asinh_safe((np - mp) / (2.0 * std::sqrt(product - 1.0)));
    const double denom = (mp + np - 2.0)
        + std::sqrt(std::max((mp + np) * (mp + np) - 4.0, 0.0));
    if (!(std::abs(denom) > kEpsilon)) {
        return std::nullopt;
    }
    parameters.lambda = 2.0 * p * std::sqrt(product - 1.0) / denom;
    parameters.epsilon = 0.5 * (x_p1 + x_m1) + p * (np - mp) / (2.0 * denom);
    if (!(parameters.lambda > 0.0) || !std::isfinite(parameters.gamma)
        || !std::isfinite(parameters.epsilon) || !std::isfinite(parameters.lambda)) {
        return std::nullopt;
    }
    return parameters;
}

std::optional<JohnsonParameters> estimate_sb(
    double z,
    double x_m3,
    double x_m1,
    double x_p1,
    double x_p3)
{
    const double m = x_p3 - x_p1;
    const double n = x_m1 - x_m3;
    const double p = x_p1 - x_m1;
    if (!(m > 0.0) || !(n > 0.0) || !(p > 0.0)) {
        return std::nullopt;
    }
    const double pm = p / m;
    const double pn = p / n;
    const double qr = (m * n) / (p * p);
    if (!(qr < 1.0 - 1.0e-8)) {
        return std::nullopt;
    }
    const double inside = 0.5 * std::sqrt((1.0 + pm) * (1.0 + pn));
    JohnsonParameters parameters;
    parameters.family = JohnsonFamily::sb;
    parameters.eta = z / acosh_safe(inside);
    if (!(parameters.eta > 0.0) || !std::isfinite(parameters.eta)) {
        return std::nullopt;
    }
    const double p2_mn = (p * p) / (m * n);
    const double spread = std::sqrt(std::max((1.0 + pm) * (1.0 + pn) - 4.0, 0.0));
    parameters.gamma = parameters.eta
        * asinh_safe(((pn - pm) * spread) / (2.0 * (p2_mn - 1.0)));
    const double lambda_num = p * std::sqrt(std::max(
        std::pow((1.0 + pm) * (1.0 + pn) - 2.0, 2.0) - 4.0, 0.0));
    parameters.lambda = lambda_num / (p2_mn - 1.0);
    parameters.epsilon = 0.5 * (x_p1 + x_m1) - parameters.lambda / 2.0
        + p * (pn - pm) / (2.0 * (p2_mn - 1.0));
    if (!(parameters.lambda > 0.0) || !std::isfinite(parameters.gamma)
        || !std::isfinite(parameters.epsilon) || !std::isfinite(parameters.lambda)) {
        return std::nullopt;
    }
    return parameters;
}

std::optional<JohnsonParameters> estimate_sl(
    double z,
    double x_m3,
    double x_m1,
    double x_p1,
    double x_p3)
{
    const double m = x_p3 - x_p1;
    const double n = x_m1 - x_m3;
    const double p = x_p1 - x_m1;
    if (!(m > 0.0) || !(n > 0.0) || !(p > 0.0)) {
        return std::nullopt;
    }
    const double ratio = std::sqrt((m / p) * (n / p));
    if (!(ratio > 1.0 + 1.0e-8)) {
        return std::nullopt;
    }
    JohnsonParameters parameters;
    parameters.family = JohnsonFamily::sl;
    parameters.eta = 2.0 * z / std::log(ratio);
    if (!(parameters.eta > 0.0) || !std::isfinite(parameters.eta)) {
        return std::nullopt;
    }
    parameters.lambda = 1.0;
    parameters.gamma = parameters.eta
        * std::log((ratio - 1.0) / (p * std::sqrt(ratio)));
    parameters.epsilon = 0.5 * (x_p1 + x_m1)
        - 0.5 * p * (ratio + 1.0) / (ratio - 1.0);
    if (!std::isfinite(parameters.gamma) || !std::isfinite(parameters.epsilon)) {
        return std::nullopt;
    }
    return parameters;
}

std::vector<double> transform_all(
    const JohnsonParameters& parameters,
    const std::vector<double>& observations)
{
    std::vector<double> transformed;
    transformed.reserve(observations.size());
    for (const double value : observations) {
        const std::optional<double> z = johnson_transform_value(parameters, value);
        if (z.has_value()) {
            transformed.push_back(*z);
        }
    }
    return transformed;
}

}  // namespace

std::string johnson_family_name(JohnsonFamily family)
{
    switch (family) {
    case JohnsonFamily::sb:
        return "SB";
    case JohnsonFamily::sl:
        return "SL";
    case JohnsonFamily::su:
        return "SU";
    case JohnsonFamily::none:
    default:
        return "none";
    }
}

std::optional<double> johnson_transform_value(
    const JohnsonParameters& parameters,
    double value)
{
    if (!in_support(parameters, value) || !(parameters.eta > 0.0)) {
        return std::nullopt;
    }
    double argument = 0.0;
    switch (parameters.family) {
    case JohnsonFamily::sb: {
        const double left = value - parameters.epsilon;
        const double right = parameters.lambda + parameters.epsilon - value;
        if (!(left > 0.0) || !(right > 0.0)) {
            return std::nullopt;
        }
        argument = std::log(left / right);
        break;
    }
    case JohnsonFamily::sl:
        argument = std::log(value - parameters.epsilon);
        break;
    case JohnsonFamily::su:
        argument = asinh_safe((value - parameters.epsilon) / parameters.lambda);
        break;
    case JohnsonFamily::none:
    default:
        return std::nullopt;
    }
    const double z = parameters.gamma + parameters.eta * argument;
    if (!std::isfinite(z)) {
        return std::nullopt;
    }
    return z;
}

std::optional<double> johnson_inverse_value(
    const JohnsonParameters& parameters,
    double z)
{
    if (!std::isfinite(z) || !(parameters.eta > 0.0)) {
        return std::nullopt;
    }
    const double scaled = (z - parameters.gamma) / parameters.eta;
    double value = std::numeric_limits<double>::quiet_NaN();
    switch (parameters.family) {
    case JohnsonFamily::sb: {
        const double k = std::exp(scaled);
        if (!std::isfinite(k) || k <= 0.0) {
            return std::nullopt;
        }
        value = (parameters.epsilon + k * (parameters.lambda + parameters.epsilon))
            / (1.0 + k);
        break;
    }
    case JohnsonFamily::sl:
        value = parameters.epsilon + std::exp(scaled);
        break;
    case JohnsonFamily::su:
        value = parameters.epsilon + parameters.lambda * std::sinh(scaled);
        break;
    case JohnsonFamily::none:
    default:
        return std::nullopt;
    }
    if (!std::isfinite(value) || !in_support(parameters, value)) {
        return std::nullopt;
    }
    return value;
}

JohnsonTransformResult fit_johnson_transform(
    const std::vector<double>& observations,
    double p_criterion)
{
    JohnsonTransformResult result;
    result.p_criterion = std::isfinite(p_criterion) && p_criterion > 0.0
        && p_criterion < 1.0 ? p_criterion : 0.10;
    std::vector<double> valid;
    valid.reserve(observations.size());
    for (const double value : observations) {
        if (std::isfinite(value)) {
            valid.push_back(value);
        }
    }
    if (valid.size() < 8) {
        add_error(result.diagnostics, "johnson_insufficient_n",
                  "Johnson 变换至少需要 8 个有限观测。");
        return result;
    }
    std::sort(valid.begin(), valid.end());
    const double p_max = (static_cast<double>(valid.size()) - 0.5)
        / static_cast<double>(valid.size());
    const double z_cap = standard_normal_quantile(p_max) / 3.0;
    if (!std::isfinite(z_cap) || z_cap < 0.25) {
        add_warning(result.diagnostics, "johnson_transform_not_found",
                    "样本量不足以估计 Johnson 分位匹配所需的尾部分位数。");
        return result;
    }

    const double z_values[] = {
        0.25, 0.34, 0.44, 0.52, 0.61, 0.70, 0.80, 0.90, 1.00, 1.10, 1.25};
    JohnsonTransformResult best;
    best.p_criterion = result.p_criterion;
    for (const double z : z_values) {
        if (z > z_cap) {
            break;
        }
        const double x_m3 = sample_quantile(valid, standard_normal_cdf(-3.0 * z));
        const double x_m1 = sample_quantile(valid, standard_normal_cdf(-z));
        const double x_p1 = sample_quantile(valid, standard_normal_cdf(z));
        const double x_p3 = sample_quantile(valid, standard_normal_cdf(3.0 * z));
        const double m = x_p3 - x_p1;
        const double n = x_m1 - x_m3;
        const double p = x_p1 - x_m1;
        if (!(m > 0.0) || !(n > 0.0) || !(p > 0.0)) {
            continue;
        }
        const double qr = (m * n) / (p * p);
        std::optional<JohnsonParameters> parameters;
        if (qr > 1.001) {
            parameters = estimate_su(z, x_m3, x_m1, x_p1, x_p3);
        } else if (qr < 0.999) {
            parameters = estimate_sb(z, x_m3, x_m1, x_p1, x_p3);
        } else {
            parameters = estimate_sl(z, x_m3, x_m1, x_p1, x_p3);
        }
        if (!parameters.has_value()) {
            continue;
        }
        std::vector<double> transformed = transform_all(*parameters, valid);
        if (transformed.size() < static_cast<std::size_t>(
                std::lround(0.95 * static_cast<double>(valid.size())))) {
            continue;
        }
        const NormalityTestResult ad = normality_test(transformed);
        if (!ad.p_value.has_value() || !std::isfinite(*ad.p_value)) {
            continue;
        }
        if (*ad.p_value > best.p_value) {
            best.found = *ad.p_value > result.p_criterion;
            best.parameters = *parameters;
            best.p_value = *ad.p_value;
            best.anderson_darling = ad.anderson_darling.value_or(0.0);
            best.transformed = std::move(transformed);
        }
    }

    if (!best.found) {
        add_warning(result.diagnostics, "johnson_transform_not_found",
                    "没有 Johnson 变换使变换后 Anderson-Darling p 值大于准则 "
                    + std::to_string(result.p_criterion)
                    + "；不输出伪造的能力指数。");
        if (best.p_value > 0.0) {
            result.parameters = best.parameters;
            result.p_value = best.p_value;
            result.anderson_darling = best.anderson_darling;
        }
        return result;
    }
    result = std::move(best);
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info,
        "johnson_formula_reference",
        "Johnson 变换按 Chou et al. (1998) 分位匹配与 AD p 值选择；"
        "数值是公式参考，不是 Minitab Individual Distribution Identification 导出。"});
    return result;
}

}  // namespace datalab::domain::statistics
