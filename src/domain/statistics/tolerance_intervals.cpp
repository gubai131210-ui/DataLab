#include "domain/statistics/tolerance_intervals.h"

#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

constexpr double kEpsilon = 1.0e-12;

void add_error(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

void add_info(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::info, code, message});
}

void add_warning(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::warning, code, message});
}

double regularized_gamma_q(double shape, double value)
{
    if (!(shape > 0.0) || value < 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (value == 0.0) {
        return 1.0;
    }
    constexpr int kMaxIterations = 200;
    if (value < shape + 1.0) {
        double term = 1.0 / shape;
        double sum = term;
        for (int index = 1; index <= kMaxIterations; ++index) {
            term *= value / (shape + static_cast<double>(index));
            sum += term;
            if (std::abs(term) < std::abs(sum) * kEpsilon) {
                break;
            }
        }
        return std::clamp(
            1.0 - sum * std::exp(-value + shape * std::log(value) - std::lgamma(shape)),
            0.0, 1.0);
    }
    double factor = 1.0;
    double sum = 1.0;
    for (int index = 1; index <= kMaxIterations; ++index) {
        factor *= (shape - static_cast<double>(index)) / value;
        sum += factor;
        if (std::abs(factor) < std::abs(sum) * kEpsilon) {
            break;
        }
    }
    return std::clamp(
        std::exp(-value + shape * std::log(value) - std::lgamma(shape)) * sum,
        0.0, 1.0);
}

double chi_square_left_tail(double value, double degrees_of_freedom)
{
    return std::clamp(
        1.0 - regularized_gamma_q(degrees_of_freedom / 2.0, value / 2.0),
        0.0, 1.0);
}

double chi_square_quantile(double probability, double degrees_of_freedom)
{
    if (!(probability > 0.0 && probability < 1.0)
        || !(degrees_of_freedom > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double lower = 0.0;
    double upper = std::max(1.0, degrees_of_freedom);
    while (chi_square_left_tail(upper, degrees_of_freedom) < probability) {
        upper *= 2.0;
    }
    for (int index = 0; index < 160; ++index) {
        const double middle = (lower + upper) / 2.0;
        if (chi_square_left_tail(middle, degrees_of_freedom) < probability) {
            lower = middle;
        } else {
            upper = middle;
        }
    }
    return (lower + upper) / 2.0;
}

double log_combination(std::size_t n, std::size_t k)
{
    if (k > n) {
        return -std::numeric_limits<double>::infinity();
    }
    const std::size_t reduced = std::min(k, n - k);
    double value = 0.0;
    for (std::size_t index = 1; index <= reduced; ++index) {
        value += std::log(static_cast<double>(n - reduced + index))
            - std::log(static_cast<double>(index));
    }
    return value;
}

double combination(std::size_t n, std::size_t k)
{
    return std::exp(log_combination(n, k));
}

}  // namespace

ToleranceIntervalResult normal_tolerance_interval(
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows,
    double coverage,
    double confidence_level,
    const std::string& interval_type)
{
    ToleranceIntervalResult result;
    result.coverage = coverage;
    result.confidence_level = confidence_level;
    result.interval_type = interval_type.empty() ? "two_sided" : interval_type;
    result.assumption_status = "not_verified";
    std::vector<double> finite;
    finite.reserve(values.size());
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (std::isfinite(values[index])) {
            finite.push_back(values[index]);
            if (index < source_rows.size()) {
                result.source_rows.push_back(source_rows[index]);
            } else {
                result.source_rows.push_back(index);
            }
        } else {
            ++result.missing_count;
        }
    }
    result.valid_count = finite.size();
    if (!(coverage > 0.0 && coverage < 1.0)
        || !(confidence_level > 0.0 && confidence_level < 1.0)) {
        add_error(result.diagnostics, "invalid_tolerance_parameters",
                  "覆盖率与置信水平必须为 (0,1) 内的有限数。");
        return result;
    }
    if (finite.size() < 2) {
        add_error(result.diagnostics, "insufficient_data",
                  "正态容差区间至少需要两个有限观测。");
        return result;
    }
    result.mean = std::accumulate(finite.cbegin(), finite.cend(), 0.0)
        / static_cast<double>(finite.size());
    double sum_sq = 0.0;
    for (const double value : finite) {
        const double difference = value - result.mean;
        sum_sq += difference * difference;
    }
    result.sample_standard_deviation = std::sqrt(
        sum_sq / static_cast<double>(finite.size() - 1));
    if (!(result.sample_standard_deviation > 0.0)) {
        add_error(result.diagnostics, "zero_variance",
                  "样本标准差为 0，无法计算容差区间。");
        return result;
    }

    const double n = static_cast<double>(finite.size());
    const double nu = n - 1.0;
    const double z_coverage = standard_normal_quantile((1.0 + coverage) / 2.0);
    const double z_p = standard_normal_quantile(coverage);
    const double z_alpha = standard_normal_quantile(confidence_level);
    const std::string type = result.interval_type;
    if (type == "lower" || type == "less" || type == "upper" || type == "greater") {
        const double a = 1.0 - (z_alpha * z_alpha) / (2.0 * nu);
        const double b = z_p * z_p - (z_alpha * z_alpha) / n;
        const double discriminant = z_p * z_p - a * b;
        if (!(a > 0.0) || discriminant < 0.0) {
            add_error(result.diagnostics, "tolerance_factor_not_computed",
                      "单侧容差因子无法计算。");
            return result;
        }
        result.k_factor = (z_p + std::sqrt(discriminant)) / a;
        result.method = "natrella_one_sided";
        add_info(result.diagnostics, "one_sided_natrella_approximation",
                 "单侧容差因子使用 NIST Natrella 近似，不是 Minitab 非中心 t 精确值。");
        if (type == "lower" || type == "less") {
            result.lower = result.mean - *result.k_factor * result.sample_standard_deviation;
        } else {
            result.upper = result.mean + *result.k_factor * result.sample_standard_deviation;
        }
        return result;
    }

    const double chi_lower = chi_square_quantile(1.0 - confidence_level, nu);
    if (!(chi_lower > 0.0) || !std::isfinite(z_coverage)) {
        add_error(result.diagnostics, "tolerance_factor_not_computed",
                  "双侧容差因子无法计算。");
        return result;
    }
    result.k_factor = z_coverage * std::sqrt(nu * (1.0 + 1.0 / n) / chi_lower);
    result.method = "howe_two_sided";
    add_info(result.diagnostics, "two_sided_howe_approximation",
             "双侧容差因子使用 NIST Howe 近似，不是 Minitab 精确积分。");
    result.lower = result.mean - *result.k_factor * result.sample_standard_deviation;
    result.upper = result.mean + *result.k_factor * result.sample_standard_deviation;
    return result;
}

ToleranceIntervalResult nonparametric_tolerance_interval(
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows,
    double coverage,
    double confidence_level,
    const std::string& interval_type)
{
    ToleranceIntervalResult result;
    result.coverage = coverage;
    result.confidence_level = confidence_level;
    result.interval_type = interval_type.empty() ? "two_sided" : interval_type;
    result.assumption_status = "not_verified";
    result.method = "order_statistic_nonparametric";
    result.method_family = "nonparametric";
    std::vector<double> finite;
    finite.reserve(values.size());
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (std::isfinite(values[index])) {
            finite.push_back(values[index]);
            result.source_rows.push_back(index < source_rows.size() ? source_rows[index] : index);
        } else {
            ++result.missing_count;
        }
    }
    result.valid_count = finite.size();
    if (!(coverage > 0.0 && coverage < 1.0)
        || !(confidence_level > 0.0 && confidence_level < 1.0)) {
        add_error(result.diagnostics, "invalid_tolerance_parameters",
                  "覆盖率与置信水平必须为 (0,1) 内的有限数。");
        return result;
    }
    if (finite.size() < 2) {
        add_error(result.diagnostics, "insufficient_data",
                  "非参数容差区间至少需要两个有限观测。");
        return result;
    }
    std::sort(finite.begin(), finite.end());
    result.mean = std::accumulate(finite.cbegin(), finite.cend(), 0.0)
        / static_cast<double>(finite.size());
    double sum_sq = 0.0;
    for (const double value : finite) {
        const double difference = value - result.mean;
        sum_sq += difference * difference;
    }
    result.sample_standard_deviation = std::sqrt(
        sum_sq / static_cast<double>(finite.size() - 1));

    const std::size_t n = finite.size();
    const std::string type = result.interval_type;
    if (type == "lower" || type == "less") {
        std::size_t r = 1;
        double achieved = 1.0 - std::pow(coverage, static_cast<double>(n));
        for (std::size_t candidate = 1; candidate <= n; ++candidate) {
            const double cdf = std::pow(coverage, static_cast<double>(n - candidate + 1));
            const double confidence = 1.0 - cdf;
            if (confidence >= confidence_level) {
                r = candidate;
                achieved = confidence;
                break;
            }
            r = candidate;
            achieved = confidence;
        }
        result.lower = finite[r - 1];
        result.achieved_confidence = achieved;
    } else if (type == "upper" || type == "greater") {
        std::size_t s = n;
        double achieved = 1.0 - std::pow(coverage, static_cast<double>(n));
        for (std::size_t candidate = 1; candidate <= n; ++candidate) {
            const double cdf = std::pow(coverage, static_cast<double>(candidate));
            const double confidence = 1.0 - cdf;
            if (confidence >= confidence_level) {
                s = n - candidate + 1;
                achieved = confidence;
                break;
            }
            s = n - candidate + 1;
            achieved = confidence;
        }
        result.upper = finite[s - 1];
        result.achieved_confidence = achieved;
    } else {
        std::size_t best_r = 1;
        std::size_t best_s = n;
        double best_achieved = 0.0;
        for (std::size_t r = 1; r < n; ++r) {
            for (std::size_t s = r + 1; s <= n; ++s) {
                const std::size_t width = s - r;
                double cumulative = 0.0;
                for (std::size_t i = 0; i <= width; ++i) {
                    cumulative += combination(n, i)
                        * std::pow(coverage, static_cast<double>(i))
                        * std::pow(1.0 - coverage, static_cast<double>(n - i));
                }
                const double achieved = 1.0 - cumulative;
                if (achieved >= confidence_level) {
                    const bool narrower = (s - r) < (best_s - best_r);
                    if (best_achieved < confidence_level || narrower) {
                        best_r = r;
                        best_s = s;
                        best_achieved = achieved;
                    }
                }
                if (best_achieved < confidence_level && achieved > best_achieved) {
                    best_r = r;
                    best_s = s;
                    best_achieved = achieved;
                }
            }
        }
        result.lower = finite[best_r - 1];
        result.upper = finite[best_s - 1];
        result.achieved_confidence = best_achieved;
    }

    if (!result.achieved_confidence.has_value()) {
        result.achieved_confidence = 0.0;
    }
    if (*result.achieved_confidence + 1.0e-12 < confidence_level) {
        add_warning(result.diagnostics, "nonparametric_sample_size_limited",
                    "样本量不足以达到目标置信水平，已报告 achieved confidence。");
    }
    return result;
}

}  // namespace datalab::domain::statistics
