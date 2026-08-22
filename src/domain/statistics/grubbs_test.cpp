#include "domain/statistics/grubbs_test.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

void add_error(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

void add_warning(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::warning, code, message});
}

std::string alternative_name(TestAlternative alternative)
{
    if (alternative == TestAlternative::less) {
        return "less";
    }
    if (alternative == TestAlternative::greater) {
        return "greater";
    }
    return "two_sided";
}

}  // namespace

GrubbsTestResult grubbs_outlier_test(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows,
    TestAlternative alternative,
    double alpha,
    std::size_t missing_count)
{
    GrubbsTestResult result;
    result.n = observations.size();
    result.missing_count = missing_count;
    result.alternative = alternative_name(alternative);
    result.alpha = (alpha > 0.0 && alpha < 1.0) ? alpha : 0.05;
    result.assumption_status = "not_verified";
    add_warning(result.diagnostics, "grubbs_normality_assumption",
                "Grubbs 检验假设观测近似正态且至多一个异常值；本分析未验证正态性。");
    if (observations.size() < 3) {
        add_error(result.diagnostics, "insufficient_observations",
                  "Grubbs 异常值检验至少需要三个有效观测。");
        return result;
    }
    const double total = std::accumulate(observations.cbegin(), observations.cend(), 0.0);
    result.mean = total / static_cast<double>(observations.size());
    double sum_squared = 0.0;
    for (const double observation : observations) {
        const double residual = observation - result.mean;
        sum_squared += residual * residual;
    }
    result.sample_standard_deviation = std::sqrt(
        sum_squared / static_cast<double>(observations.size() - 1));
    if (!(result.sample_standard_deviation > 0.0)
        || !std::isfinite(result.sample_standard_deviation)) {
        add_error(result.diagnostics, "zero_variance",
                  "样本标准差为 0，无法计算 Grubbs 统计量。");
        return result;
    }

    std::size_t selected = 0;
    if (alternative == TestAlternative::less) {
        selected = static_cast<std::size_t>(
            std::min_element(observations.cbegin(), observations.cend())
            - observations.cbegin());
    } else if (alternative == TestAlternative::greater) {
        selected = static_cast<std::size_t>(
            std::max_element(observations.cbegin(), observations.cend())
            - observations.cbegin());
    } else {
        double best = -1.0;
        for (std::size_t index = 0; index < observations.size(); ++index) {
            const double deviation = std::abs(observations[index] - result.mean);
            if (deviation > best) {
                best = deviation;
                selected = index;
            }
        }
    }
    result.outlier_index = selected;
    result.outlier_value = observations[selected];
    result.direction = observations[selected] >= result.mean ? "largest" : "smallest";
    if (selected < source_rows.size()) {
        result.source_row = source_rows[selected];
    }
    result.g_statistic = std::abs(observations[selected] - result.mean)
        / result.sample_standard_deviation;

    const double n = static_cast<double>(observations.size());
    const double g = *result.g_statistic;
    const double denominator = (n - 1.0) * (n - 1.0) - n * g * g;
    if (!(denominator > 1.0e-15) || !std::isfinite(denominator)) {
        result.p_value = 0.0;
        return result;
    }
    const double t = g * std::sqrt(n * (n - 2.0) / denominator);
    const double tail = 1.0 - student_t_cdf(t, n - 2.0);
    double p_one = n * tail;
    if (!std::isfinite(p_one) || p_one < 0.0) {
        p_one = 1.0;
    }
    const double p = alternative == TestAlternative::two_sided ? 2.0 * p_one : p_one;
    result.p_value = std::clamp(p, 0.0, 1.0);
    return result;
}

namespace {

// Dixon r10 two-sided critical values α≈0.05 for n=3..30 (formula_reference tables;
// Rorabacher / common Dixon Q 95% style; not Minitab golden).
double dixon_r10_critical_05(std::size_t n)
{
    static const double kCrit[] = {
        // n=3..30
        0.970, 0.829, 0.710, 0.625, 0.568, 0.526, 0.493, 0.466,
        0.444, 0.425, 0.410, 0.396, 0.384, 0.374, 0.365, 0.356,
        0.349, 0.342, 0.337, 0.331, 0.326, 0.321, 0.317, 0.312,
        0.308, 0.305, 0.301, 0.298
    };
    if (n < 3 || n > 30) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return kCrit[n - 3];
}

double dixon_r10_critical_01(std::size_t n)
{
    static const double kCrit[] = {
        0.994, 0.926, 0.821, 0.740, 0.680, 0.634, 0.598, 0.568,
        0.542, 0.522, 0.503, 0.488, 0.475, 0.463, 0.452, 0.442,
        0.433, 0.425, 0.418, 0.411, 0.404, 0.399, 0.393, 0.388,
        0.384, 0.379, 0.375, 0.372
    };
    if (n < 3 || n > 30) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return kCrit[n - 3];
}

double dixon_r10_critical_10(std::size_t n)
{
    static const double kCrit[] = {
        0.886, 0.679, 0.557, 0.482, 0.434, 0.399, 0.370, 0.349,
        0.332, 0.317, 0.303, 0.290, 0.281, 0.273, 0.266, 0.260,
        0.254, 0.249, 0.244, 0.239, 0.235, 0.232, 0.228, 0.225,
        0.222, 0.219, 0.216, 0.213
    };
    if (n < 3 || n > 30) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return kCrit[n - 3];
}

}  // namespace

DixonTestResult dixon_r10_outlier_test(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows,
    TestAlternative alternative,
    double alpha,
    std::size_t missing_count)
{
    DixonTestResult result;
    result.n = observations.size();
    result.missing_count = missing_count;
    result.alternative = alternative_name(alternative);
    result.alpha = (alpha > 0.0 && alpha < 1.0) ? alpha : 0.05;
    result.assumption_status = "not_verified";
    result.ratio = "r10";
    add_warning(result.diagnostics, "dixon_normality_assumption",
                "Dixon r10 假设近似正态；本分析未验证正态性。");
    if (observations.size() < 3) {
        add_error(result.diagnostics, "insufficient_observations",
                  "Dixon r10 至少需要三个有效观测。");
        return result;
    }
    if (observations.size() > 30) {
        add_error(result.diagnostics, "dixon_n_too_large",
                  "Dixon r10 产品锁定 n≤30；更大样本请用 Grubbs。");
        return result;
    }

    const double total = std::accumulate(observations.cbegin(), observations.cend(), 0.0);
    result.mean = total / static_cast<double>(observations.size());
    double sum_squared = 0.0;
    for (const double observation : observations) {
        const double residual = observation - result.mean;
        sum_squared += residual * residual;
    }
    result.sample_standard_deviation = std::sqrt(
        sum_squared / static_cast<double>(observations.size() - 1));

    std::vector<std::pair<double, std::size_t>> ordered;
    ordered.reserve(observations.size());
    for (std::size_t i = 0; i < observations.size(); ++i) {
        ordered.push_back({observations[i], i});
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    const double y1 = ordered.front().first;
    const double yn = ordered.back().first;
    const double range = yn - y1;
    if (!(range > 0.0) || !std::isfinite(range)) {
        add_error(result.diagnostics, "zero_range",
                  "样本极差为 0，无法计算 Dixon r10。");
        return result;
    }
    const double r_low = (ordered[1].first - y1) / range;
    const double r_high = (yn - ordered[ordered.size() - 2].first) / range;

    std::size_t selected = ordered.front().second;
    double r = r_low;
    std::string direction = "smallest";
    if (alternative == TestAlternative::greater) {
        selected = ordered.back().second;
        r = r_high;
        direction = "largest";
    } else if (alternative == TestAlternative::less) {
        selected = ordered.front().second;
        r = r_low;
        direction = "smallest";
    } else if (r_high >= r_low) {
        selected = ordered.back().second;
        r = r_high;
        direction = "largest";
    }

    result.r_statistic = r;
    result.outlier_index = selected;
    result.outlier_value = observations[selected];
    result.direction = direction;
    if (selected < source_rows.size()) {
        result.source_row = source_rows[selected];
    }

    const double c05 = dixon_r10_critical_05(observations.size());
    const double c01 = dixon_r10_critical_01(observations.size());
    const double c10 = dixon_r10_critical_10(observations.size());
    result.critical_value = c05;
    // Approximate p by bracketing against α=0.10/0.05/0.01 criticals (formula_reference).
    if (r >= c01) {
        result.p_value = 0.005;
    } else if (r >= c05) {
        result.p_value = 0.05 * (c01 - r) / (c01 - c05) + 0.01 * (r - c05) / (c01 - c05);
        result.p_value = std::clamp(*result.p_value, 0.01, 0.05);
    } else if (r >= c10) {
        result.p_value = 0.10 * (c05 - r) / (c05 - c10) + 0.05 * (r - c10) / (c05 - c10);
        result.p_value = std::clamp(*result.p_value, 0.05, 0.10);
    } else {
        result.p_value = std::min(1.0, 0.10 + 0.5 * (c10 - r) / std::max(c10, 1.0e-12));
    }
    if (alternative == TestAlternative::two_sided) {
        // Tables are one-sided style; inflate slightly for two-sided product note.
        result.p_value = std::min(1.0, *result.p_value * 1.5);
    }
    add_warning(result.diagnostics, "dixon_p_interpolated",
                "Dixon P 由临界值插值近似（formula_reference），不是 Minitab 积分 golden。");
    return result;
}

}  // namespace datalab::domain::statistics
