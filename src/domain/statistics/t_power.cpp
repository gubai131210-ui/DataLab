#include "domain/statistics/t_power.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace datalab::domain::statistics {
namespace {
constexpr double kIntegrationTolerance = 2.0e-11;

void error(std::vector<DiagnosticMessage>& diagnostics, const char* code, const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

double normal_cdf(const double value)
{
    return 0.5 * std::erfc(-value / std::sqrt(2.0));
}

// Density after the substitution x = sqrt(V), V ~ chi-square(df).
double chi_square_x_density(const double x, const double degrees_of_freedom)
{
    if (x <= 0.0) {
        return 0.0;
    }
    const double df = degrees_of_freedom;
    const double log_density = (df - 1.0) * std::log(x) - 0.5 * x * x
        - (df * 0.5) * std::log(2.0) - std::lgamma(df * 0.5)
        + std::log(2.0);
    return std::exp(log_density);
}

template <typename Function>
double adaptive_simpson(
    const Function& function, const double left, const double right,
    const double f_left, const double f_mid, const double f_right,
    const double whole, const double tolerance, const int depth, bool& converged)
{
    const double mid = (left + right) * 0.5;
    const double left_mid = (left + mid) * 0.5;
    const double right_mid = (mid + right) * 0.5;
    const double f_left_mid = function(left_mid);
    const double f_right_mid = function(right_mid);
    const double left_piece = (mid - left) / 6.0
        * (f_left + 4.0 * f_left_mid + f_mid);
    const double right_piece = (right - mid) / 6.0
        * (f_mid + 4.0 * f_right_mid + f_right);
    const double delta = left_piece + right_piece - whole;
    if (depth <= 0) {
        converged = false;
        return left_piece + right_piece;
    }
    if (std::abs(delta) <= 15.0 * tolerance) {
        return left_piece + right_piece + delta / 15.0;
    }
    return adaptive_simpson(function, left, mid, f_left, f_left_mid, f_mid,
                            left_piece, tolerance * 0.5, depth - 1, converged)
        + adaptive_simpson(function, mid, right, f_mid, f_right_mid, f_right,
                           right_piece, tolerance * 0.5, depth - 1, converged);
}

double integrate_t_expectation(
    const double degrees_of_freedom, const double noncentrality,
    const double critical, const bool two_sided, bool& converged)
{
    const double upper = std::sqrt(std::max(
        100.0, degrees_of_freedom + 14.0 * std::sqrt(2.0 * degrees_of_freedom)));
    const auto integrand = [=](const double x) {
        if (x == 0.0) {
            return 0.0;
        }
        const double scale = x / std::sqrt(degrees_of_freedom);
        const double upper_tail = 1.0 - normal_cdf(critical * scale - noncentrality);
        const double probability = two_sided
            ? normal_cdf(-critical * scale - noncentrality) + upper_tail
            : upper_tail;
        return chi_square_x_density(x, degrees_of_freedom) * probability;
    };
    const double mid = upper * 0.5;
    const double f_left = integrand(0.0);
    const double f_mid = integrand(mid);
    const double f_right = integrand(upper);
    const double whole = upper / 6.0 * (f_left + 4.0 * f_mid + f_right);
    return adaptive_simpson(integrand, 0.0, upper, f_left, f_mid, f_right,
                            whole, kIntegrationTolerance, 24, converged);
}

double central_t_cdf(const double value, const double degrees_of_freedom, bool& converged)
{
    if (value == 0.0) {
        return 0.5;
    }
    // P(T <= value) = E[Phi(value * sqrt(V / df))].
    return integrate_t_expectation(
        degrees_of_freedom, 0.0, -value, false, converged);
}

double central_t_quantile(const double probability, const double degrees_of_freedom,
                          bool& converged)
{
    double low = 0.0;
    double high = std::max(8.0, 2.0 * std::sqrt(2.0) * std::abs(std::log(
        std::max(1.0e-15, 1.0 - probability))));
    while (central_t_cdf(high, degrees_of_freedom, converged) < probability
           && high < 1.0e6) {
        high *= 2.0;
    }
    for (int iteration = 0; iteration < 80; ++iteration) {
        const double mid = (low + high) * 0.5;
        if (central_t_cdf(mid, degrees_of_freedom, converged) < probability) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return (low + high) * 0.5;
}

double regularized_gamma_q(const double shape, const double x)
{
    if (x <= 0.0) {
        return 1.0;
    }
    const double log_term = shape * std::log(x) - x - std::lgamma(shape);
    if (x < shape + 1.0) {
        double sum = 1.0 / shape;
        double term = sum;
        for (int i = 1; i < 200; ++i) {
            term *= x / (shape + static_cast<double>(i));
            sum += term;
            if (std::abs(term) <= std::abs(sum) * 2.0e-15) {
                break;
            }
        }
        return std::clamp(1.0 - std::exp(log_term) * sum, 0.0, 1.0);
    }
    constexpr double kTiny = 1.0e-300;
    double coefficient = 1.0;
    double denominator = x + 1.0 - shape;
    double inverse = 1.0 / std::max(std::abs(denominator), kTiny);
    double continued_fraction = inverse;
    for (int i = 1; i < 200; ++i) {
        const double an = -static_cast<double>(i)
            * (static_cast<double>(i) - shape);
        denominator += 2.0;
        if (std::abs(denominator) < kTiny) {
            denominator = kTiny;
        }
        inverse = an * inverse + denominator;
        if (std::abs(inverse) < kTiny) {
            inverse = kTiny;
        }
        inverse = 1.0 / inverse;
        coefficient = denominator + an / coefficient;
        if (std::abs(coefficient) < kTiny) {
            coefficient = kTiny;
        }
        const double delta = coefficient * inverse;
        continued_fraction *= delta;
        if (std::abs(delta - 1.0) < 2.0e-15) {
            break;
        }
    }
    return std::clamp(std::exp(log_term) * continued_fraction, 0.0, 1.0);
}

double noncentral_chi_square_survival(const double degrees_of_freedom,
                                      const double noncentrality, const double value)
{
    if (value <= 0.0) {
        return 1.0;
    }
    const double mean = noncentrality * 0.5;
    double weight = std::exp(-mean);
    double result = 0.0;
    for (int j = 0; j < 1000; ++j) {
        result += weight * regularized_gamma_q(
            degrees_of_freedom * 0.5 + static_cast<double>(j), value * 0.5);
        if (j > mean && weight < 1.0e-15) {
            break;
        }
        weight *= mean / static_cast<double>(j + 1);
    }
    return std::clamp(result, 0.0, 1.0);
}

double integrate_noncentral_f_power(const double numerator_df, const double denominator_df,
                                    const double noncentrality, const double critical,
                                    bool& converged)
{
    const double upper = std::sqrt(std::max(
        100.0, denominator_df + 14.0 * std::sqrt(2.0 * denominator_df)));
    const auto integrand = [=](const double x) {
        if (x <= 0.0) {
            return 0.0;
        }
        const double threshold = numerator_df * critical * x * x / denominator_df;
        return chi_square_x_density(x, denominator_df)
            * noncentral_chi_square_survival(numerator_df, noncentrality, threshold);
    };
    const double mid = upper * 0.5;
    const double left = integrand(0.0);
    const double middle = integrand(mid);
    const double right = integrand(upper);
    const double whole = upper / 6.0 * (left + 4.0 * middle + right);
    return adaptive_simpson(integrand, 0.0, upper, left, middle, right, whole,
                            kIntegrationTolerance, 24, converged);
}

double central_f_cdf(const double value, const double numerator_df,
                     const double denominator_df, bool& converged)
{
    return 1.0 - integrate_noncentral_f_power(
        numerator_df, denominator_df, 0.0, value, converged);
}

double central_chi_square_cdf(const double value, const double degrees_of_freedom)
{
    return std::clamp(
        1.0 - regularized_gamma_q(degrees_of_freedom * 0.5, value * 0.5), 0.0, 1.0);
}

double central_f_quantile(const double probability, const double numerator_df,
                          const double denominator_df, bool& converged)
{
    double low = 0.0;
    double high = 1.0;
    while (central_f_cdf(high, numerator_df, denominator_df, converged) < probability
           && high < 1.0e12) {
        high *= 2.0;
    }
    for (int iteration = 0; iteration < 80; ++iteration) {
        const double mid = (low + high) * 0.5;
        if (central_f_cdf(mid, numerator_df, denominator_df, converged) < probability) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return (low + high) * 0.5;
}

double normal_quantile(const double probability)
{
    double low = -10.0;
    double high = 10.0;
    for (int iteration = 0; iteration < 80; ++iteration) {
        const double mid = (low + high) * 0.5;
        if (normal_cdf(mid) < probability) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return (low + high) * 0.5;
}

bool valid_probability(const double value)
{
    return std::isfinite(value) && value > 0.0 && value < 1.0;
}

PowerResult invalid_power_result(const char* code, const char* message)
{
    PowerResult result;
    error(result.diagnostics, code, message);
    return result;
}

double variance_power_probability(
    const double lower_critical, const double upper_critical,
    const double scale, const double degrees_of_freedom,
    const PowerAlternative alternative)
{
    const double lower_tail = lower_critical > 0.0
        ? central_chi_square_cdf(lower_critical / scale, degrees_of_freedom)
        : 0.0;
    const double upper_tail = upper_critical > 0.0
        ? 1.0 - central_chi_square_cdf(upper_critical / scale, degrees_of_freedom)
        : 0.0;
    switch (alternative) {
    case PowerAlternative::less:
        return lower_tail;
    case PowerAlternative::greater:
        return upper_tail;
    case PowerAlternative::two_sided:
    default:
        return std::clamp(lower_tail + upper_tail, 0.0, 1.0);
    }
}

double variance_ratio_power_probability(
    const double lower_critical, const double upper_critical,
    const double scale, const double numerator_df, const double denominator_df,
    const PowerAlternative alternative)
{
    bool converged = true;
    const double lower_tail = lower_critical > 0.0
        ? central_f_cdf(lower_critical / scale, numerator_df, denominator_df, converged)
        : 0.0;
    const double upper_tail = upper_critical > 0.0
        ? 1.0 - central_f_cdf(upper_critical / scale, numerator_df, denominator_df, converged)
        : 0.0;
    switch (alternative) {
    case PowerAlternative::less:
        return lower_tail;
    case PowerAlternative::greater:
        return upper_tail;
    case PowerAlternative::two_sided:
    default:
        return std::clamp(lower_tail + upper_tail, 0.0, 1.0);
    }
}

TPowerResult power(const std::size_t n, const double effect, const double alpha,
                   const bool two_group)
{
    TPowerResult result;
    result.sample_size = n;
    result.sample_size_per_group = two_group ? n : 0;
    result.effect_size = effect;
    if (n < 2 || (two_group && n > std::numeric_limits<std::size_t>::max() / 2)
        || !std::isfinite(effect) || effect == 0.0
        || !std::isfinite(alpha) || alpha <= 0.0 || alpha >= 1.0) {
        error(result.diagnostics, "invalid_t_power_input",
              "样本量、效应量和 alpha 必须有效。");
        return result;
    }
    result.total_sample_size = two_group ? 2 * n : n;
    result.degrees_of_freedom = two_group
        ? static_cast<double>(2 * n - 2) : static_cast<double>(n - 1);
    result.noncentrality_parameter = std::abs(effect) * std::sqrt(
        two_group ? static_cast<double>(n) / 2.0 : static_cast<double>(n));
    bool converged = true;
    result.critical_value = central_t_quantile(
        1.0 - alpha * 0.5, result.degrees_of_freedom, converged);
    result.power = integrate_t_expectation(
        result.degrees_of_freedom, result.noncentrality_parameter,
        result.critical_value, true, converged);
    result.power = std::clamp(result.power, 0.0, 1.0);
    if (!converged) {
        error(result.diagnostics, "t_power_numerical_integration",
              "非中心 t 数值积分未在规定迭代次数内收敛。");
    }
    return result;
}

TPowerResult sample_size(const double effect, const double target, const double alpha,
                         const bool two_group)
{
    TPowerResult result;
    result.effect_size = effect;
    if (!std::isfinite(effect) || effect == 0.0 || !std::isfinite(target)
        || target <= 0.0 || target >= 1.0 || !std::isfinite(alpha)
        || alpha <= 0.0 || alpha >= 1.0) {
        error(result.diagnostics, "invalid_t_sample_size_input",
              "效应量、目标功效和 alpha 必须有效。");
        return result;
    }
    std::size_t high = 2;
    while (power(high, effect, alpha, two_group).power < target
           && high < 1000000) {
        high *= 2;
    }
    if (power(high, effect, alpha, two_group).power < target) {
        error(result.diagnostics, "t_sample_size_limit",
              "在允许的最大样本量内无法达到目标功效。");
        return result;
    }
    std::size_t low = 2;
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (power(mid, effect, alpha, two_group).power >= target) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return power(low, effect, alpha, two_group);
}
}

TPowerResult one_sample_t_power(std::size_t n, double effect, double alpha)
{
    return power(n, effect, alpha, false);
}

TPowerResult two_sample_t_power(std::size_t n, double effect, double alpha)
{
    return power(n, effect, alpha, true);
}

TPowerResult one_sample_t_sample_size(double effect, double target, double alpha)
{
    return sample_size(effect, target, alpha, false);
}

TPowerResult two_sample_t_sample_size(double effect, double target, double alpha)
{
    return sample_size(effect, target, alpha, true);
}

PowerResult one_way_anova_power(const std::size_t n, const std::size_t group_count,
                                const double effect, const double alpha)
{
    PowerResult result;
    result.sample_size = n;
    result.sample_size_per_group = n;
    result.effect_size = effect;
    if (n < 2 || group_count < 2 || !std::isfinite(effect) || effect <= 0.0
        || !valid_probability(alpha)
        || n > std::numeric_limits<std::size_t>::max() / group_count) {
        return invalid_power_result("invalid_anova_power_input",
                                    "每组样本量、组数、效应量和 alpha 必须有效。");
    }
    result.total_sample_size = n * group_count;
    result.degrees_of_freedom = static_cast<double>(group_count - 1);
    const double denominator_df = static_cast<double>(result.total_sample_size - group_count);
    result.noncentrality_parameter = static_cast<double>(result.total_sample_size)
        * effect * effect;
    bool converged = true;
    result.critical_value = central_f_quantile(
        1.0 - alpha, result.degrees_of_freedom, denominator_df, converged);
    result.power = integrate_noncentral_f_power(
        result.degrees_of_freedom, denominator_df, result.noncentrality_parameter,
        result.critical_value, converged);
    result.power = std::clamp(result.power, 0.0, 1.0);
    if (!converged) {
        error(result.diagnostics, "anova_power_numerical_integration",
              "非中心 F 数值积分未在规定迭代次数内收敛。");
    }
    return result;
}

PowerResult one_way_anova_sample_size(const std::size_t group_count, const double effect,
                                      const double target, const double alpha)
{
    if (group_count < 2 || !std::isfinite(effect) || effect <= 0.0
        || !valid_probability(target) || !valid_probability(alpha)) {
        return invalid_power_result("invalid_anova_sample_size_input",
                                    "组数、效应量、目标功效和 alpha 必须有效。");
    }
    std::size_t high = 2;
    while (high < 1000000
           && one_way_anova_power(high, group_count, effect, alpha).power < target) {
        high *= 2;
    }
    if (one_way_anova_power(high, group_count, effect, alpha).power < target) {
        return invalid_power_result("anova_sample_size_limit",
                                    "在允许的最大样本量内无法达到目标功效。");
    }
    std::size_t low = 2;
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (one_way_anova_power(mid, group_count, effect, alpha).power >= target) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return one_way_anova_power(low, group_count, effect, alpha);
}

double proportion_power(const double alternative_difference, const double null_se,
                        const double alternative_se, const double alpha,
                        const PowerAlternative alternative)
{
    const double critical = normal_quantile(
        alternative == PowerAlternative::two_sided ? 1.0 - alpha * 0.5 : 1.0 - alpha);
    const double mean = (alternative == PowerAlternative::less
                             ? -alternative_difference : alternative_difference) / null_se;
    const double scale = alternative_se / null_se;
    if (alternative == PowerAlternative::greater) {
        return 1.0 - normal_cdf((critical - mean) / scale);
    }
    if (alternative == PowerAlternative::less) {
        return normal_cdf((-critical - mean) / scale);
    }
    return normal_cdf((-critical - mean) / scale)
        + 1.0 - normal_cdf((critical - mean) / scale);
}

PowerResult one_sample_proportion_power_impl(
    const std::size_t n, const double null_proportion, const double alternative_proportion,
    const double alpha, const PowerAlternative alternative)
{
    PowerResult result;
    result.sample_size = n;
    result.effect_size = alternative_proportion - null_proportion;
    if (n < 2 || !std::isfinite(null_proportion) || !std::isfinite(alternative_proportion)
        || null_proportion <= 0.0 || null_proportion >= 1.0
        || alternative_proportion <= 0.0 || alternative_proportion >= 1.0
        || !valid_probability(alpha) || alternative_proportion == null_proportion) {
        return invalid_power_result("invalid_one_proportion_power_input",
                                    "比例、样本量和 alpha 必须有效，且备择比例必须不同。");
    }
    result.total_sample_size = n;
    result.degrees_of_freedom = std::numeric_limits<double>::infinity();
    const double null_se = std::sqrt(null_proportion * (1.0 - null_proportion)
                                     / static_cast<double>(n));
    const double alternative_se = std::sqrt(alternative_proportion
                                             * (1.0 - alternative_proportion)
                                             / static_cast<double>(n));
    result.noncentrality_parameter = std::abs(result.effect_size) / null_se;
    result.critical_value = normal_quantile(
        alternative == PowerAlternative::two_sided ? 1.0 - alpha * 0.5 : 1.0 - alpha);
    result.power = proportion_power(result.effect_size, null_se, alternative_se, alpha,
                                    alternative);
    result.power = std::clamp(result.power, 0.0, 1.0);
    return result;
}

PowerResult one_sample_proportion_power(const std::size_t n, const double null_proportion,
                                        const double alternative_proportion,
                                        const double alpha,
                                        const PowerAlternative alternative)
{
    return one_sample_proportion_power_impl(
        n, null_proportion, alternative_proportion, alpha, alternative);
}

PowerResult one_sample_proportion_sample_size(
    const double null_proportion, const double alternative_proportion, const double target,
    const double alpha, const PowerAlternative alternative)
{
    if (!valid_probability(target)) {
        return invalid_power_result("invalid_one_proportion_sample_size_input",
                                    "目标功效必须位于 0 和 1 之间。");
    }
    std::size_t high = 2;
    while (high < 1000000 && one_sample_proportion_power_impl(
               high, null_proportion, alternative_proportion, alpha, alternative).power < target) {
        high *= 2;
    }
    if (one_sample_proportion_power_impl(
            high, null_proportion, alternative_proportion, alpha, alternative).power < target) {
        return invalid_power_result("one_proportion_sample_size_limit",
                                    "在允许的最大样本量内无法达到目标功效。");
    }
    std::size_t low = 2;
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (one_sample_proportion_power_impl(
                mid, null_proportion, alternative_proportion, alpha, alternative).power >= target) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return one_sample_proportion_power_impl(
        low, null_proportion, alternative_proportion, alpha, alternative);
}

PowerResult two_proportion_power(
    const std::size_t n, const double first, const double second, const double alpha,
    const PowerAlternative alternative, const ProportionVarianceMethod method)
{
    PowerResult result;
    result.sample_size = n;
    result.sample_size_per_group = n;
    result.effect_size = first - second;
    if (n < 2 || !std::isfinite(first) || !std::isfinite(second)
        || first <= 0.0 || first >= 1.0 || second <= 0.0 || second >= 1.0
        || !valid_probability(alpha) || first == second
        || n > std::numeric_limits<std::size_t>::max() / 2) {
        return invalid_power_result("invalid_two_proportion_power_input",
                                    "两组比例、样本量和 alpha 必须有效，且两组比例必须不同。");
    }
    result.total_sample_size = 2 * n;
    result.degrees_of_freedom = std::numeric_limits<double>::infinity();
    const double pooled = (first + second) * 0.5;
    const double null_se = std::sqrt(2.0 * pooled * (1.0 - pooled)
                                     / static_cast<double>(n));
    const double alternative_se = method == ProportionVarianceMethod::pooled
        ? null_se
        : std::sqrt((first * (1.0 - first) + second * (1.0 - second))
                    / static_cast<double>(n));
    result.noncentrality_parameter = std::abs(result.effect_size) / null_se;
    result.critical_value = normal_quantile(
        alternative == PowerAlternative::two_sided ? 1.0 - alpha * 0.5 : 1.0 - alpha);
    result.power = proportion_power(result.effect_size, null_se, alternative_se, alpha,
                                    alternative);
    result.power = std::clamp(result.power, 0.0, 1.0);
    return result;
}

PowerResult two_proportion_sample_size(
    const double first, const double second, const double target, const double alpha,
    const PowerAlternative alternative, const ProportionVarianceMethod method)
{
    if (!valid_probability(target)) {
        return invalid_power_result("invalid_two_proportion_sample_size_input",
                                    "目标功效必须位于 0 和 1 之间。");
    }
    std::size_t high = 2;
    while (high < 1000000
           && two_proportion_power(high, first, second, alpha, alternative, method).power < target) {
        high *= 2;
    }
    if (two_proportion_power(high, first, second, alpha, alternative, method).power < target) {
        return invalid_power_result("two_proportion_sample_size_limit",
                                    "在允许的最大样本量内无法达到目标功效。");
    }
    std::size_t low = 2;
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (two_proportion_power(mid, first, second, alpha, alternative, method).power
            >= target) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return two_proportion_power(low, first, second, alpha, alternative, method);
}

PowerResult one_variance_power(
    const std::size_t sample_size, const double standard_deviation_ratio,
    const double alpha, const PowerAlternative alternative)
{
    PowerResult result;
    result.sample_size = sample_size;
    result.total_sample_size = sample_size;
    result.effect_size = standard_deviation_ratio;
    if (sample_size < 2 || !std::isfinite(standard_deviation_ratio)
        || standard_deviation_ratio <= 0.0 || !valid_probability(alpha)) {
        return invalid_power_result("invalid_one_variance_power_input",
                                    "样本量、标准差比和 alpha 必须有效。");
    }
    const double df = static_cast<double>(sample_size - 1);
    result.degrees_of_freedom = df;
    result.noncentrality_parameter = standard_deviation_ratio * standard_deviation_ratio;
    double lower_critical = 0.0;
    double upper_critical = 0.0;
    if (alternative != PowerAlternative::greater) {
        const double probability = alternative == PowerAlternative::two_sided
            ? alpha * 0.5 : alpha;
        double low = 0.0;
        double high = df;
        while (central_chi_square_cdf(high, df) < probability && high < 1.0e12) {
            high *= 2.0;
        }
        for (int iteration = 0; iteration < 80; ++iteration) {
            const double mid = (low + high) * 0.5;
            if (central_chi_square_cdf(mid, df) < probability) {
                low = mid;
            } else {
                high = mid;
            }
        }
        lower_critical = (low + high) * 0.5;
    }
    if (alternative != PowerAlternative::less) {
        const double probability = alternative == PowerAlternative::two_sided
            ? 1.0 - alpha * 0.5 : 1.0 - alpha;
        double low = 0.0;
        double high = df;
        while (central_chi_square_cdf(high, df) < probability && high < 1.0e12) {
            high *= 2.0;
        }
        for (int iteration = 0; iteration < 80; ++iteration) {
            const double mid = (low + high) * 0.5;
            if (central_chi_square_cdf(mid, df) < probability) {
                low = mid;
            } else {
                high = mid;
            }
        }
        upper_critical = (low + high) * 0.5;
    }
    result.critical_value = alternative == PowerAlternative::less
        ? lower_critical : upper_critical;
    const double scale = standard_deviation_ratio * standard_deviation_ratio;
    result.power = variance_power_probability(
        lower_critical, upper_critical, scale, df, alternative);
    return result;
}

PowerResult one_variance_sample_size(
    const double standard_deviation_ratio, const double target_power,
    const double alpha, const PowerAlternative alternative)
{
    if (!std::isfinite(standard_deviation_ratio) || standard_deviation_ratio <= 0.0
        || !valid_probability(target_power) || !valid_probability(alpha)) {
        return invalid_power_result("invalid_one_variance_sample_size_input",
                                    "标准差比、目标功效和 alpha 必须有效。");
    }
    std::size_t high = 2;
    while (high < 1000000
           && one_variance_power(high, standard_deviation_ratio, alpha, alternative).power
               < target_power) {
        high *= 2;
    }
    if (one_variance_power(high, standard_deviation_ratio, alpha, alternative).power
        < target_power) {
        return invalid_power_result("one_variance_sample_size_limit",
                                    "在允许的最大样本量内无法达到目标功效。");
    }
    std::size_t low = 2;
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (one_variance_power(mid, standard_deviation_ratio, alpha, alternative).power
            >= target_power) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return one_variance_power(low, standard_deviation_ratio, alpha, alternative);
}

PowerResult two_variance_power(
    const std::size_t sample_size_per_group, const double standard_deviation_ratio,
    const double alpha, const PowerAlternative alternative)
{
    PowerResult result;
    result.sample_size = sample_size_per_group;
    result.sample_size_per_group = sample_size_per_group;
    result.total_sample_size = sample_size_per_group * 2;
    result.effect_size = standard_deviation_ratio;
    if (sample_size_per_group < 2 || !std::isfinite(standard_deviation_ratio)
        || standard_deviation_ratio <= 0.0 || !valid_probability(alpha)) {
        return invalid_power_result("invalid_two_variance_power_input",
                                    "每组样本量、标准差比和 alpha 必须有效。");
    }
    const double numerator_df = static_cast<double>(sample_size_per_group - 1);
    const double denominator_df = static_cast<double>(sample_size_per_group - 1);
    result.degrees_of_freedom = numerator_df;
    result.noncentrality_parameter = standard_deviation_ratio * standard_deviation_ratio;
    bool converged = true;
    double lower_critical = 0.0;
    double upper_critical = 0.0;
    if (alternative != PowerAlternative::greater) {
        lower_critical = central_f_quantile(
            alternative == PowerAlternative::two_sided ? alpha * 0.5 : alpha,
            numerator_df, denominator_df, converged);
    }
    if (alternative != PowerAlternative::less) {
        upper_critical = central_f_quantile(
            alternative == PowerAlternative::two_sided ? 1.0 - alpha * 0.5 : 1.0 - alpha,
            numerator_df, denominator_df, converged);
    }
    result.critical_value = alternative == PowerAlternative::less
        ? lower_critical : upper_critical;
    const double scale = standard_deviation_ratio * standard_deviation_ratio;
    result.power = variance_ratio_power_probability(
        lower_critical, upper_critical, scale, numerator_df, denominator_df, alternative);
    return result;
}

PowerResult two_variance_sample_size(
    const double standard_deviation_ratio, const double target_power,
    const double alpha, const PowerAlternative alternative)
{
    if (!std::isfinite(standard_deviation_ratio) || standard_deviation_ratio <= 0.0
        || !valid_probability(target_power) || !valid_probability(alpha)) {
        return invalid_power_result("invalid_two_variance_sample_size_input",
                                    "标准差比、目标功效和 alpha 必须有效。");
    }
    std::size_t high = 2;
    while (high < 1000000
           && two_variance_power(high, standard_deviation_ratio, alpha, alternative).power
               < target_power) {
        high *= 2;
    }
    if (two_variance_power(high, standard_deviation_ratio, alpha, alternative).power
        < target_power) {
        return invalid_power_result("two_variance_sample_size_limit",
                                    "在允许的最大样本量内无法达到目标功效。");
    }
    std::size_t low = 2;
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (two_variance_power(mid, standard_deviation_ratio, alpha, alternative).power
            >= target_power) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return two_variance_power(low, standard_deviation_ratio, alpha, alternative);
}

PowerResult one_poisson_rate_power_impl(
    const std::size_t n, const double null_rate, const double comparison_rate,
    const double observation_length, const double alpha,
    const PowerAlternative alternative)
{
    PowerResult result;
    result.sample_size = n;
    result.effect_size = comparison_rate - null_rate;
    if (n < 1 || !std::isfinite(null_rate) || !std::isfinite(comparison_rate)
        || !std::isfinite(observation_length) || null_rate <= 0.0 || comparison_rate <= 0.0
        || observation_length <= 0.0 || !valid_probability(alpha)
        || comparison_rate == null_rate) {
        return invalid_power_result("invalid_one_poisson_power_input",
                                    "泊松率、观测长度、样本量和 alpha 必须有效，且比较率必须不同。");
    }
    result.total_sample_size = n;
    result.degrees_of_freedom = std::numeric_limits<double>::infinity();
    const double exposure = static_cast<double>(n) * observation_length;
    const double null_se = std::sqrt(null_rate / exposure);
    const double alternative_se = std::sqrt(comparison_rate / exposure);
    result.noncentrality_parameter = std::abs(result.effect_size) / null_se;
    result.critical_value = normal_quantile(
        alternative == PowerAlternative::two_sided ? 1.0 - alpha * 0.5 : 1.0 - alpha);
    result.power = proportion_power(result.effect_size, null_se, alternative_se, alpha,
                                    alternative);
    result.power = std::clamp(result.power, 0.0, 1.0);
    return result;
}

PowerResult one_poisson_rate_power(
    const std::size_t sample_size, const double null_rate, const double comparison_rate,
    const double observation_length, const double alpha,
    const PowerAlternative alternative)
{
    return one_poisson_rate_power_impl(
        sample_size, null_rate, comparison_rate, observation_length, alpha, alternative);
}

PowerResult one_poisson_rate_sample_size(
    const double null_rate, const double comparison_rate, const double target_power,
    const double observation_length, const double alpha,
    const PowerAlternative alternative)
{
    if (!valid_probability(target_power)) {
        return invalid_power_result("invalid_one_poisson_sample_size_input",
                                    "目标功效必须位于 0 和 1 之间。");
    }
    std::size_t high = 1;
    while (high < 1000000
           && one_poisson_rate_power_impl(
                  high, null_rate, comparison_rate, observation_length, alpha, alternative)
                      .power
               < target_power) {
        high *= 2;
    }
    if (one_poisson_rate_power_impl(
            high, null_rate, comparison_rate, observation_length, alpha, alternative)
            .power
        < target_power) {
        return invalid_power_result("one_poisson_sample_size_limit",
                                    "在允许的最大样本量内无法达到目标功效。");
    }
    std::size_t low = 1;
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (one_poisson_rate_power_impl(
                mid, null_rate, comparison_rate, observation_length, alpha, alternative)
                .power
            >= target_power) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return one_poisson_rate_power_impl(
        low, null_rate, comparison_rate, observation_length, alpha, alternative);
}

PowerResult two_poisson_rate_power(
    const std::size_t sample_size_per_group, const double first_rate,
    const double second_rate, const double observation_length, const double alpha,
    const PowerAlternative alternative)
{
    PowerResult result;
    result.sample_size = sample_size_per_group;
    result.sample_size_per_group = sample_size_per_group;
    result.effect_size = first_rate - second_rate;
    if (sample_size_per_group < 1 || !std::isfinite(first_rate) || !std::isfinite(second_rate)
        || !std::isfinite(observation_length) || first_rate <= 0.0 || second_rate <= 0.0
        || observation_length <= 0.0 || !valid_probability(alpha) || first_rate == second_rate
        || sample_size_per_group > std::numeric_limits<std::size_t>::max() / 2) {
        return invalid_power_result("invalid_two_poisson_power_input",
                                    "两组泊松率、观测长度、样本量和 alpha 必须有效，且两组率必须不同。");
    }
    result.total_sample_size = 2 * sample_size_per_group;
    result.degrees_of_freedom = std::numeric_limits<double>::infinity();
    const double exposure = static_cast<double>(sample_size_per_group) * observation_length;
    const double null_se = std::sqrt((first_rate + second_rate) / exposure);
    const double alternative_se = std::sqrt(first_rate / exposure + second_rate / exposure);
    result.noncentrality_parameter = std::abs(result.effect_size) / null_se;
    result.critical_value = normal_quantile(
        alternative == PowerAlternative::two_sided ? 1.0 - alpha * 0.5 : 1.0 - alpha);
    result.power = proportion_power(result.effect_size, null_se, alternative_se, alpha,
                                    alternative);
    result.power = std::clamp(result.power, 0.0, 1.0);
    return result;
}

PowerResult two_poisson_rate_sample_size(
    const double first_rate, const double second_rate, const double target_power,
    const double observation_length, const double alpha,
    const PowerAlternative alternative)
{
    if (!valid_probability(target_power)) {
        return invalid_power_result("invalid_two_poisson_sample_size_input",
                                    "目标功效必须位于 0 和 1 之间。");
    }
    std::size_t high = 1;
    while (high < 1000000
           && two_poisson_rate_power(
                  high, first_rate, second_rate, observation_length, alpha, alternative)
                      .power
               < target_power) {
        high *= 2;
    }
    if (two_poisson_rate_power(
            high, first_rate, second_rate, observation_length, alpha, alternative)
            .power
        < target_power) {
        return invalid_power_result("two_poisson_sample_size_limit",
                                    "在允许的最大样本量内无法达到目标功效。");
    }
    std::size_t low = 1;
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (two_poisson_rate_power(
                mid, first_rate, second_rate, observation_length, alpha, alternative)
                .power
            >= target_power) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return two_poisson_rate_power(
        low, first_rate, second_rate, observation_length, alpha, alternative);
}

PowerResult equivalence_power_core(
    const std::size_t n_per, const bool two_sample, const double lower_limit,
    const double upper_limit, const double true_difference, const double alpha)
{
    PowerResult result;
    result.sample_size = n_per;
    result.sample_size_per_group = two_sample ? n_per : 0;
    result.total_sample_size = two_sample ? 2 * n_per : n_per;
    result.effect_size = true_difference;
    if (n_per < 2 || !std::isfinite(lower_limit) || !std::isfinite(upper_limit)
        || !(lower_limit < upper_limit) || !std::isfinite(true_difference)
        || !valid_probability(alpha)) {
        error(result.diagnostics, "invalid_equivalence_power_input",
              "等价功效输入无效：需 n≥2 且下限 < 上限。");
        return result;
    }
    const double se = two_sample
        ? std::sqrt(2.0 / static_cast<double>(n_per))
        : 1.0 / std::sqrt(static_cast<double>(n_per));
    const double z_alpha = normal_quantile(1.0 - alpha);
    const double upper_term =
        normal_cdf((upper_limit - true_difference) / se - z_alpha);
    const double lower_term =
        normal_cdf((lower_limit - true_difference) / se + z_alpha);
    result.power = std::clamp(upper_term - lower_term, 0.0, 1.0);
    result.critical_value = z_alpha;
    result.degrees_of_freedom = std::numeric_limits<double>::infinity();
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info,
        "equivalence_power_known_sigma_normal",
        "等价功效使用已知 σ 的正态规划近似，不是精确二元非中心 t。"});
    return result;
}

PowerResult equivalence_one_sample_power(
    const std::size_t sample_size, const double lower_limit, const double upper_limit,
    const double true_difference, const double alpha)
{
    return equivalence_power_core(
        sample_size, false, lower_limit, upper_limit, true_difference, alpha);
}

PowerResult equivalence_two_sample_power(
    const std::size_t sample_size_per_group, const double lower_limit,
    const double upper_limit, const double true_difference, const double alpha)
{
    return equivalence_power_core(
        sample_size_per_group, true, lower_limit, upper_limit, true_difference, alpha);
}

PowerResult equivalence_sample_size_core(
    const bool two_sample, const double lower_limit, const double upper_limit,
    const double target_power, const double true_difference, const double alpha)
{
    if (!valid_probability(target_power)) {
        return invalid_power_result("invalid_equivalence_sample_size_input",
                                    "目标功效必须位于 0 和 1 之间。");
    }
    std::size_t high = 2;
    while (high < 1000000
           && equivalence_power_core(
                  high, two_sample, lower_limit, upper_limit, true_difference, alpha)
                      .power
               < target_power) {
        high *= 2;
    }
    if (equivalence_power_core(
            high, two_sample, lower_limit, upper_limit, true_difference, alpha)
            .power
        < target_power) {
        return invalid_power_result("equivalence_sample_size_limit",
                                    "在允许的最大样本量内无法达到目标功效。");
    }
    std::size_t low = 2;
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (equivalence_power_core(
                mid, two_sample, lower_limit, upper_limit, true_difference, alpha)
                .power
            >= target_power) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return equivalence_power_core(
        low, two_sample, lower_limit, upper_limit, true_difference, alpha);
}

PowerResult equivalence_one_sample_sample_size(
    const double lower_limit, const double upper_limit, const double target_power,
    const double true_difference, const double alpha)
{
    return equivalence_sample_size_core(
        false, lower_limit, upper_limit, target_power, true_difference, alpha);
}

PowerResult equivalence_two_sample_sample_size(
    const double lower_limit, const double upper_limit, const double target_power,
    const double true_difference, const double alpha)
{
    return equivalence_sample_size_core(
        true, lower_limit, upper_limit, target_power, true_difference, alpha);
}

PowerResult doe_factorial_power(
    const std::size_t factor_count, const std::size_t fraction_p,
    const std::size_t replicates, const double effect_over_sigma, const double alpha)
{
    PowerResult result;
    result.effect_size = effect_over_sigma;
    if (factor_count < 2 || fraction_p >= factor_count || replicates < 1
        || !std::isfinite(effect_over_sigma) || effect_over_sigma == 0.0
        || !valid_probability(alpha)) {
        error(result.diagnostics, "invalid_doe_factorial_power_input",
              "DOE 功效需要 k≥2、p<k、replicates≥1 且效应/σ≠0。");
        return result;
    }
    const std::size_t base = factor_count - fraction_p;
    if (base > 20) {
        error(result.diagnostics, "doe_factorial_power_overflow",
              "基设计过大。");
        return result;
    }
    std::size_t runs_per_replicate = 1;
    for (std::size_t i = 0; i < base; ++i) {
        runs_per_replicate *= 2;
    }
    const std::size_t n_per_level = replicates * (runs_per_replicate / 2);
    result = two_sample_t_power(n_per_level, effect_over_sigma, alpha);
    result.sample_size = replicates;
    result.sample_size_per_group = n_per_level;
    result.total_sample_size = replicates * runs_per_replicate;
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info,
        "doe_factorial_power_contrast",
        "DOE 功效按主效应高低水平对比，复用双样本非中心 t；中心点未计入。"});
    return result;
}

PowerResult doe_factorial_sample_size(
    const std::size_t factor_count, const std::size_t fraction_p,
    const double effect_over_sigma, const double target_power, const double alpha)
{
    if (!valid_probability(target_power)) {
        return invalid_power_result("invalid_doe_factorial_sample_size_input",
                                    "目标功效必须位于 0 和 1 之间。");
    }
    std::size_t high = 1;
    while (high < 100000
           && doe_factorial_power(
                  factor_count, fraction_p, high, effect_over_sigma, alpha)
                      .power
               < target_power) {
        high *= 2;
    }
    if (doe_factorial_power(factor_count, fraction_p, high, effect_over_sigma, alpha)
            .power
        < target_power) {
        return invalid_power_result("doe_factorial_sample_size_limit",
                                    "在允许的最大重复数内无法达到目标功效。");
    }
    std::size_t low = 1;
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (doe_factorial_power(
                factor_count, fraction_p, mid, effect_over_sigma, alpha)
                .power
            >= target_power) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return doe_factorial_power(factor_count, fraction_p, low, effect_over_sigma, alpha);
}

double howe_k_factor(const std::size_t n, const double coverage, const double confidence)
{
    if (n < 2 || !valid_probability(coverage) || !valid_probability(confidence)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double nn = static_cast<double>(n);
    const double nu = nn - 1.0;
    const double z_coverage = normal_quantile((1.0 + coverage) / 2.0);
    // Chi-square lower quantile via Wilson–Hilferty inverse of normal.
    const double z = normal_quantile(1.0 - confidence);
    const double h = 2.0 / (9.0 * nu);
    const double cube = 1.0 - h + z * std::sqrt(h);
    const double chi_lower = nu * cube * cube * cube;
    if (!(chi_lower > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return z_coverage * std::sqrt(nu * (1.0 + 1.0 / nn) / chi_lower);
}

PowerResult tolerance_normal_sample_size(
    const double coverage, const double confidence_level, const double max_k_factor)
{
    PowerResult result;
    result.effect_size = max_k_factor;
    if (!valid_probability(coverage) || !valid_probability(confidence_level)
        || !(max_k_factor > 0.0)) {
        error(result.diagnostics, "invalid_tolerance_sample_size_input",
              "覆盖率、置信水平须在 (0,1)，max k 须为正。");
        return result;
    }
    std::size_t found = 0;
    for (std::size_t n = 2; n <= 100000; ++n) {
        const double k = howe_k_factor(n, coverage, confidence_level);
        if (std::isfinite(k) && k <= max_k_factor) {
            found = n;
            result.power = confidence_level;
            result.sample_size = n;
            result.total_sample_size = n;
            result.critical_value = k;
            result.degrees_of_freedom = static_cast<double>(n - 1);
            break;
        }
    }
    if (found == 0) {
        error(result.diagnostics, "tolerance_sample_size_limit",
              "在允许的最大 n 内 Howe k 仍大于目标上限。");
        return result;
    }
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info,
        "tolerance_sample_size_howe",
        "容差样本量按 NIST Howe 双侧 k(n)≤max_k 求解；非 Minitab 精确积分。"});
    return result;
}

}  // namespace datalab::domain::statistics
