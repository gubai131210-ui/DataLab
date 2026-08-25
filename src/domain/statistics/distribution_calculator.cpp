#include "domain/statistics/distribution_calculator.h"

#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/normal_distribution.h"
#include "domain/statistics/reliability.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>

namespace datalab::domain::statistics {
namespace {

std::string ascii_lower(std::string text)
{
    for (char& ch : text) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return text;
}

// Local incomplete-gamma Q (same style as variance_tests; does not alter GOF cores).
double regularized_gamma_q(double shape, double value)
{
    if (!(shape > 0.0) || !(value >= 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (value == 0.0) {
        return 1.0;
    }
    const double epsilon = 1.0e-12;
    if (value < shape + 1.0) {
        double sum = 1.0 / shape;
        double term = sum;
        for (int n = 1; n < 200; ++n) {
            term *= value / (shape + static_cast<double>(n));
            sum += term;
            if (std::abs(term) < std::abs(sum) * epsilon) {
                break;
            }
        }
        const double lower = sum * std::exp(-value + shape * std::log(value)
                                            - std::lgamma(shape));
        return std::clamp(1.0 - lower, 0.0, 1.0);
    }
    double a = 1.0 + value - shape;
    double b = 1.0;
    double c = 1.0 / epsilon;
    double d = 1.0 / a;
    double h = d;
    for (int i = 1; i < 200; ++i) {
        const double an = -static_cast<double>(i) * (static_cast<double>(i) - shape);
        a += 2.0;
        d = an * d + a;
        if (std::abs(d) < epsilon) {
            d = epsilon;
        }
        c = a + an / c;
        if (std::abs(c) < epsilon) {
            c = epsilon;
        }
        d = 1.0 / d;
        const double delta = d * c;
        h *= delta;
        if (std::abs(delta - 1.0) < epsilon) {
            break;
        }
    }
    return std::clamp(
        std::exp(-value + shape * std::log(value) - std::lgamma(shape)) * h,
        0.0, 1.0);
}

double chi_square_cdf(double value, double df)
{
    if (!(df > 0.0) || !std::isfinite(value)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (value <= 0.0) {
        return 0.0;
    }
    return std::clamp(1.0 - regularized_gamma_q(df / 2.0, value / 2.0), 0.0, 1.0);
}

double chi_square_pdf(double value, double df)
{
    if (!(df > 0.0) || !(value >= 0.0) || !std::isfinite(value)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (value == 0.0) {
        return df < 2.0 ? std::numeric_limits<double>::infinity()
                        : (df == 2.0 ? 0.5 : 0.0);
    }
    const double k = df / 2.0;
    return std::exp(-value / 2.0 + (k - 1.0) * std::log(value)
                    - k * std::log(2.0) - std::lgamma(k));
}

double chi_square_quantile(double probability, double df)
{
    if (!(probability > 0.0 && probability < 1.0) || !(df > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double lower = 0.0;
    double upper = std::max(1.0, df);
    while (chi_square_cdf(upper, df) < probability && upper < 1.0e12) {
        upper *= 2.0;
    }
    for (int i = 0; i < 160; ++i) {
        const double mid = 0.5 * (lower + upper);
        if (chi_square_cdf(mid, df) < probability) {
            lower = mid;
        } else {
            upper = mid;
        }
    }
    return 0.5 * (lower + upper);
}

double student_t_pdf(double value, double df)
{
    if (!(df > 0.0) || !std::isfinite(value)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double num = std::lgamma((df + 1.0) / 2.0);
    const double den = std::lgamma(df / 2.0);
    const double base = std::sqrt(df * 3.14159265358979323846);
    return std::exp(num - den) / base
        * std::pow(1.0 + value * value / df, -(df + 1.0) / 2.0);
}

double f_cdf(double value, double d1, double d2)
{
    if (!(d1 > 0.0) || !(d2 > 0.0) || !std::isfinite(value)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (value <= 0.0) {
        return 0.0;
    }
    return std::clamp(1.0 - f_right_tail(value, d1, d2), 0.0, 1.0);
}

double f_pdf(double value, double d1, double d2)
{
    if (!(d1 > 0.0) || !(d2 > 0.0) || !(value >= 0.0) || !std::isfinite(value)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (value == 0.0) {
        return d1 < 2.0 ? std::numeric_limits<double>::infinity() : 0.0;
    }
    const double a = d1 / 2.0;
    const double b = d2 / 2.0;
    const double log_c = a * std::log(d1) + b * std::log(d2)
        - std::lgamma(a) - std::lgamma(b) + std::lgamma(a + b);
    return std::exp(log_c + (a - 1.0) * std::log(value)
                    - (a + b) * std::log(d1 * value + d2));
}

double f_quantile(double probability, double d1, double d2)
{
    if (!(probability > 0.0 && probability < 1.0) || !(d1 > 0.0) || !(d2 > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double lower = 0.0;
    double upper = 1.0;
    while (f_cdf(upper, d1, d2) < probability && upper < 1.0e12) {
        upper *= 2.0;
    }
    for (int i = 0; i < 160; ++i) {
        const double mid = 0.5 * (lower + upper);
        if (f_cdf(mid, d1, d2) < probability) {
            lower = mid;
        } else {
            upper = mid;
        }
    }
    return 0.5 * (lower + upper);
}

double weibull_pdf(double x, double shape, double scale)
{
    if (!(shape > 0.0) || !(scale > 0.0) || !(x >= 0.0) || !std::isfinite(x)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (x == 0.0) {
        return shape <= 1.0 ? std::numeric_limits<double>::infinity() : 0.0;
    }
    const double z = x / scale;
    return (shape / scale) * std::pow(z, shape - 1.0) * std::exp(-std::pow(z, shape));
}

double weibull_cdf(double x, double shape, double scale)
{
    return cdf_weibull3(x, shape, scale, 0.0);
}

double weibull_quantile(double p, double shape, double scale)
{
    return percentile_life_weibull(shape, scale, p * 100.0);
}

}  // namespace

DistCalcDistribution parse_distcalc_distribution(const std::string& text)
{
    const std::string lower = ascii_lower(text);
    if (lower == "t" || lower == "student_t" || lower == "student-t") {
        return DistCalcDistribution::student_t;
    }
    if (lower == "chi2" || lower == "chi_square" || lower == "chisquare"
        || lower == "chi-square") {
        return DistCalcDistribution::chi_square;
    }
    if (lower == "f") {
        return DistCalcDistribution::f;
    }
    if (lower == "weibull") {
        return DistCalcDistribution::weibull;
    }
    return DistCalcDistribution::normal;
}

DistCalcOperation parse_distcalc_operation(const std::string& text)
{
    const std::string lower = ascii_lower(text);
    if (lower == "pdf" || lower == "density") {
        return DistCalcOperation::pdf;
    }
    if (lower == "quantile" || lower == "invcdf" || lower == "ppf") {
        return DistCalcOperation::quantile;
    }
    return DistCalcOperation::cdf;
}

std::string distcalc_distribution_name(DistCalcDistribution d)
{
    switch (d) {
    case DistCalcDistribution::student_t:
        return "t";
    case DistCalcDistribution::chi_square:
        return "chi_square";
    case DistCalcDistribution::f:
        return "f";
    case DistCalcDistribution::weibull:
        return "weibull";
    case DistCalcDistribution::normal:
    default:
        return "normal";
    }
}

std::string distcalc_operation_name(DistCalcOperation op)
{
    switch (op) {
    case DistCalcOperation::pdf:
        return "pdf";
    case DistCalcOperation::quantile:
        return "quantile";
    case DistCalcOperation::cdf:
    default:
        return "cdf";
    }
}

DistributionCalculatorResult evaluate_distribution_calculator(
    const DistributionCalculatorOptions& options)
{
    DistributionCalculatorResult out;
    out.distribution = distcalc_distribution_name(options.distribution);
    out.operation = distcalc_operation_name(options.operation);
    out.param1 = options.param1;
    out.param2 = options.param2;
    out.param3 = options.param3;
    out.value = options.value;

    double answer = std::numeric_limits<double>::quiet_NaN();
    switch (options.distribution) {
    case DistCalcDistribution::normal: {
        const double mean = options.param1;
        const double sd = options.param2 > 0.0 ? options.param2 : 1.0;
        if (!(sd > 0.0)) {
            out.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "distcalc_bad_sd",
                "正态标准差必须为正。"});
            return out;
        }
        if (options.operation == DistCalcOperation::pdf) {
            answer = normal_pdf(options.value, mean, sd);
        } else if (options.operation == DistCalcOperation::cdf) {
            answer = standard_normal_cdf((options.value - mean) / sd);
        } else {
            answer = mean + sd * standard_normal_quantile(options.value);
        }
        break;
    }
    case DistCalcDistribution::student_t: {
        const double df = options.param1;
        if (!(df > 0.0)) {
            out.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "distcalc_bad_df",
                "t 分布自由度必须为正。"});
            return out;
        }
        if (options.operation == DistCalcOperation::pdf) {
            answer = student_t_pdf(options.value, df);
        } else if (options.operation == DistCalcOperation::cdf) {
            answer = student_t_cdf(options.value, df);
        } else {
            answer = student_t_quantile(options.value, df);
        }
        break;
    }
    case DistCalcDistribution::chi_square: {
        const double df = options.param1;
        if (!(df > 0.0)) {
            out.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "distcalc_bad_df",
                "χ² 自由度必须为正。"});
            return out;
        }
        if (options.operation == DistCalcOperation::pdf) {
            answer = chi_square_pdf(options.value, df);
        } else if (options.operation == DistCalcOperation::cdf) {
            answer = chi_square_cdf(options.value, df);
        } else {
            answer = chi_square_quantile(options.value, df);
        }
        break;
    }
    case DistCalcDistribution::f: {
        const double d1 = options.param1;
        const double d2 = options.param2;
        if (!(d1 > 0.0) || !(d2 > 0.0)) {
            out.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "distcalc_bad_df",
                "F 分布两个自由度必须为正。"});
            return out;
        }
        if (options.operation == DistCalcOperation::pdf) {
            answer = f_pdf(options.value, d1, d2);
        } else if (options.operation == DistCalcOperation::cdf) {
            answer = f_cdf(options.value, d1, d2);
        } else {
            answer = f_quantile(options.value, d1, d2);
        }
        break;
    }
    case DistCalcDistribution::weibull: {
        const double shape = options.param1;
        const double scale = options.param2;
        if (!(shape > 0.0) || !(scale > 0.0)) {
            out.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "distcalc_bad_weibull",
                "Weibull 形状与尺度必须为正。"});
            return out;
        }
        if (options.operation == DistCalcOperation::pdf) {
            answer = weibull_pdf(options.value, shape, scale);
        } else if (options.operation == DistCalcOperation::cdf) {
            answer = weibull_cdf(options.value, shape, scale);
        } else {
            if (!(options.value > 0.0 && options.value < 1.0)) {
                out.diagnostics.push_back({
                    DiagnosticMessage::Severity::error, "distcalc_bad_p",
                    "分位数概率须在 (0,1)。"});
                return out;
            }
            answer = weibull_quantile(options.value, shape, scale);
        }
        break;
    }
    }

    if (!std::isfinite(answer)) {
        out.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "distcalc_failed",
            "无法计算该分布函数值（检查参数与操作）。"});
        return out;
    }
    out.result = answer;
    return out;
}

}  // namespace datalab::domain::statistics
