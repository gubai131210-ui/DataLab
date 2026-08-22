#include "domain/statistics/quality_extensions.h"

#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

void add_error(std::vector<DiagnosticMessage>& d, const char* code, const char* msg)
{
    d.push_back({DiagnosticMessage::Severity::error, code, msg});
}

void add_warning(std::vector<DiagnosticMessage>& d, const char* code, const char* msg)
{
    d.push_back({DiagnosticMessage::Severity::warning, code, msg});
}

double log_combination(std::size_t n, std::size_t k)
{
    if (k > n) {
        return -std::numeric_limits<double>::infinity();
    }
    k = std::min(k, n - k);
    double sum = 0.0;
    for (std::size_t i = 1; i <= k; ++i) {
        sum += std::log(static_cast<double>(n - k + i)) - std::log(static_cast<double>(i));
    }
    return sum;
}

double binomial_pmf(std::size_t x, std::size_t n, double p)
{
    if (p <= 0.0) {
        return x == 0 ? 1.0 : 0.0;
    }
    if (p >= 1.0) {
        return x == n ? 1.0 : 0.0;
    }
    return std::exp(log_combination(n, x)
                    + static_cast<double>(x) * std::log(p)
                    + static_cast<double>(n - x) * std::log1p(-p));
}

double binomial_cdf_le(std::size_t x, std::size_t n, double p)
{
    double sum = 0.0;
    const std::size_t upper = std::min(x, n);
    for (std::size_t i = 0; i <= upper; ++i) {
        sum += binomial_pmf(i, n, p);
    }
    return std::clamp(sum, 0.0, 1.0);
}

double chi_square_right_tail(double value, double df)
{
    if (!(value >= 0.0) || !(df > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    // Regularized gamma Q(df/2, value/2)
    const double shape = df / 2.0;
    const double x = value / 2.0;
    if (x == 0.0) {
        return 1.0;
    }
    constexpr int max_iter = 200;
    constexpr double eps = 3.0e-14;
    if (x < shape + 1.0) {
        double term = 1.0 / shape;
        double sum = term;
        for (int i = 1; i <= max_iter; ++i) {
            term *= x / (shape + static_cast<double>(i));
            sum += term;
            if (std::abs(term) < std::abs(sum) * eps) {
                break;
            }
        }
        const double lower = sum * std::exp(-x + shape * std::log(x) - std::lgamma(shape));
        return std::clamp(1.0 - lower, 0.0, 1.0);
    }
    double factor = 1.0;
    double sum = 1.0;
    for (int i = 1; i <= max_iter; ++i) {
        factor *= (shape - static_cast<double>(i)) / x;
        sum += factor;
        if (std::abs(factor) < std::abs(sum) * eps) {
            break;
        }
    }
    return std::clamp(
        std::exp(-x + shape * std::log(x) - std::lgamma(shape)) * sum, 0.0, 1.0);
}

}  // namespace

AcceptanceSamplingResult acceptance_sampling_binomial(
    std::size_t sample_size,
    std::size_t acceptance_number,
    std::optional<double> aql,
    std::optional<double> rql,
    std::optional<std::size_t> lot_size)
{
    AcceptanceSamplingResult result;
    result.sample_size = sample_size;
    result.acceptance_number = acceptance_number;
    result.lot_size = lot_size;
    result.aql = aql;
    result.rql = rql;
    if (sample_size == 0) {
        add_error(result.diagnostics, "acceptance_n_zero", "样本量 n 必须 ≥ 1。");
        return result;
    }
    if (acceptance_number > sample_size) {
        add_error(result.diagnostics, "acceptance_c_too_large",
                  "接收数 c 不能大于样本量 n。");
        return result;
    }
    if (lot_size.has_value() && *lot_size < sample_size) {
        add_warning(result.diagnostics, "acceptance_lot_lt_n",
                    "批大小 N < n；本轮仍用二项 OC（无限批近似）。");
    }
    const double grid[] = {
        0.0, 0.005, 0.01, 0.015, 0.02, 0.03, 0.04, 0.05, 0.06, 0.08,
        0.10, 0.12, 0.15, 0.20, 0.25, 0.30, 0.40, 0.50, 0.60, 0.80, 1.0};
    for (double p : grid) {
        AcceptanceOcPoint point;
        point.fraction_defective = p;
        point.probability_accept = binomial_cdf_le(acceptance_number, sample_size, p);
        result.oc_curve.push_back(point);
    }
    if (aql.has_value() && *aql >= 0.0 && *aql <= 1.0) {
        result.pa_at_aql = binomial_cdf_le(acceptance_number, sample_size, *aql);
    }
    if (rql.has_value() && *rql >= 0.0 && *rql <= 1.0) {
        result.pa_at_rql = binomial_cdf_le(acceptance_number, sample_size, *rql);
    }
    return result;
}

AnomResult analysis_of_means(
    const std::vector<std::vector<double>>& groups,
    const std::vector<std::string>& labels,
    double alpha)
{
    AnomResult result;
    result.alpha = (alpha > 0.0 && alpha < 1.0) ? alpha : 0.05;
    if (groups.size() < 2) {
        add_error(result.diagnostics, "anom_need_two_groups",
                  "ANOM 至少需要两个组。");
        return result;
    }
    double grand_sum = 0.0;
    double sse = 0.0;
    std::size_t df_error = 0;
    bool equal_n = true;
    std::size_t ref_n = 0;
    for (std::size_t g = 0; g < groups.size(); ++g) {
        if (groups[g].size() < 2) {
            add_error(result.diagnostics, "anom_group_too_small",
                      "每组至少需要 2 个观测。");
            return result;
        }
        if (g == 0) {
            ref_n = groups[g].size();
        } else if (groups[g].size() != ref_n) {
            equal_n = false;
        }
        const double sum = std::accumulate(groups[g].begin(), groups[g].end(), 0.0);
        const double mean = sum / static_cast<double>(groups[g].size());
        AnomGroup row;
        row.label = (g < labels.size() && !labels[g].empty())
            ? labels[g] : std::to_string(g + 1);
        row.n = groups[g].size();
        row.mean = mean;
        result.groups.push_back(row);
        grand_sum += sum;
        result.total_n += groups[g].size();
        for (double value : groups[g]) {
            const double d = value - mean;
            sse += d * d;
        }
        df_error += groups[g].size() - 1;
    }
    if (df_error == 0 || result.total_n == 0) {
        add_error(result.diagnostics, "anom_degenerate", "无法估计组内方差。");
        return result;
    }
    result.overall_mean = grand_sum / static_cast<double>(result.total_n);
    result.pooled_sd = std::sqrt(sse / static_cast<double>(df_error));
    if (!(result.pooled_sd > 0.0)) {
        add_error(result.diagnostics, "anom_zero_sigma", "组内标准差为 0。");
        return result;
    }
    const double k = static_cast<double>(groups.size());
    // Nelson-style normal approximation decision half-width (formula_reference).
    const double z = standard_normal_quantile(1.0 - result.alpha / (2.0 * k));
    const double half = z * result.pooled_sd * std::sqrt((k - 1.0) / static_cast<double>(result.total_n));
    result.udl = result.overall_mean + half;
    result.ldl = result.overall_mean - half;
    result.decision_limit_method = "nelson_normal_approx";
    add_warning(result.diagnostics, "anom_limits_approx",
                "决策限采用正态/多重比较近似（formula_reference），不是 Minitab exact h 表 golden。");
    if (!equal_n) {
        add_warning(result.diagnostics, "anom_unequal_n",
                    "各组样本量不等；决策限仍用总 N 近似，解读需谨慎。");
    }
    for (auto& group : result.groups) {
        group.outside_limits = group.mean > result.udl || group.mean < result.ldl;
    }
    return result;
}

PoissonGofResult poisson_goodness_of_fit(const std::vector<double>& counts)
{
    PoissonGofResult result;
    std::vector<int> values;
    values.reserve(counts.size());
    for (double value : counts) {
        if (!std::isfinite(value) || value < 0.0) {
            continue;
        }
        const double rounded = std::round(value);
        if (std::abs(value - rounded) > 1.0e-9) {
            add_error(result.diagnostics, "poisson_gof_non_integer",
                      "泊松拟合优度要求非负整数计数。");
            return result;
        }
        values.push_back(static_cast<int>(rounded));
    }
    result.n = values.size();
    if (result.n < 2) {
        add_error(result.diagnostics, "poisson_gof_insufficient_n",
                  "至少需要 2 个有效计数。");
        return result;
    }
    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    result.lambda_hat = sum / static_cast<double>(result.n);
    std::map<int, std::size_t> freq;
    for (int value : values) {
        ++freq[value];
    }
    // Merge tail cells with E<1 into adjacent for stability.
    struct Cell {
        int value = 0;
        double observed = 0.0;
        double expected = 0.0;
    };
    std::vector<Cell> cells;
    double remaining_prob = 1.0;
    for (const auto& entry : freq) {
        Cell cell;
        cell.value = entry.first;
        cell.observed = static_cast<double>(entry.second);
        // P(X=k)=e^{-λ} λ^k / k!
        double log_p = -result.lambda_hat
            + static_cast<double>(entry.first) * std::log(std::max(result.lambda_hat, 1.0e-300))
            - std::lgamma(static_cast<double>(entry.first) + 1.0);
        cell.expected = std::exp(log_p) * static_cast<double>(result.n);
        remaining_prob -= std::exp(log_p);
        cells.push_back(cell);
    }
    // Absorb unused probability into last cell expectation lightly.
    if (!cells.empty() && remaining_prob > 0.0) {
        cells.back().expected += remaining_prob * static_cast<double>(result.n);
    }
    // Merge cells with E < 1 into neighbors.
    std::vector<Cell> merged;
    for (const Cell& cell : cells) {
        if (!merged.empty() && cell.expected < 1.0) {
            merged.back().observed += cell.observed;
            merged.back().expected += cell.expected;
        } else {
            merged.push_back(cell);
        }
    }
    while (merged.size() >= 2 && merged.front().expected < 1.0) {
        merged[1].observed += merged.front().observed;
        merged[1].expected += merged.front().expected;
        merged.erase(merged.begin());
    }
    result.category_count = merged.size();
    if (merged.size() < 2) {
        add_error(result.diagnostics, "poisson_gof_too_few_bins",
                  "合并后类别不足，无法计算拟合优度。");
        return result;
    }
    for (const Cell& cell : merged) {
        if (cell.expected < 5.0) {
            ++result.expected_below_five_count;
        }
        if (cell.expected > 0.0) {
            const double diff = cell.observed - cell.expected;
            result.chi_square += diff * diff / cell.expected;
        }
    }
    result.degrees_of_freedom = static_cast<double>(merged.size()) - 1.0 - 1.0;
    if (result.degrees_of_freedom < 1.0) {
        add_error(result.diagnostics, "poisson_gof_df",
                  "自由度 < 1（类别过少）。");
        return result;
    }
    result.p_value = chi_square_right_tail(result.chi_square, result.degrees_of_freedom);
    if (result.expected_below_five_count > 0) {
        result.validity_status = "warning";
        add_warning(result.diagnostics, "poisson_gof_small_expected",
                    "存在期望频数 < 5，卡方近似需谨慎。");
    }
    return result;
}

}  // namespace datalab::domain::statistics
