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

}  // namespace datalab::domain::statistics
