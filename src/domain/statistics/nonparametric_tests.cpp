#include "domain/statistics/nonparametric_tests.h"

#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <optional>

namespace datalab::domain::statistics {
namespace {

struct RankedValue {
    double value;
    std::size_t group;
    double rank = 0.0;
    std::size_t origin = 0;
};

void error(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

void warning(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::warning, code, message});
}

std::vector<RankedValue> rank_values(const std::vector<std::vector<double>>& groups)
{
    std::vector<RankedValue> values;
    for (std::size_t group = 0; group < groups.size(); ++group) {
        for (std::size_t origin = 0; origin < groups[group].size(); ++origin) {
            values.push_back({groups[group][origin], group, 0.0, origin});
        }
    }
    std::sort(values.begin(), values.end(),
              [](const RankedValue& first, const RankedValue& second) {
                  return first.value < second.value;
              });
    std::size_t start = 0;
    while (start < values.size()) {
        std::size_t end = start + 1;
        while (end < values.size() && values[end].value == values[start].value) {
            ++end;
        }
        const double rank = (static_cast<double>(start + 1)
            + static_cast<double>(end)) / 2.0;
        for (std::size_t index = start; index < end; ++index) {
            values[index].rank = rank;
        }
        start = end;
    }
    return values;
}

double chi_square_right_tail(double value, double degrees_of_freedom)
{
    if (!(value >= 0.0) || !(degrees_of_freedom > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double shape = degrees_of_freedom / 2.0;
    const double x = value / 2.0;
    if (x == 0.0) {
        return 1.0;
    }
    double term = 1.0 / shape;
    double sum = term;
    for (int index = 1; index < 200; ++index) {
        term *= x / (shape + index);
        sum += term;
        if (std::abs(term) < std::abs(sum) * 1.0e-14) {
            break;
        }
    }
    if (x < shape + 1.0) {
        return std::clamp(1.0 - sum * std::exp(-x + shape * std::log(x)
            - std::lgamma(shape)), 0.0, 1.0);
    }
    double continued = 1.0;
    double factor = 1.0;
    for (int index = 1; index < 200; ++index) {
        factor *= (shape - index) / x;
        continued += factor;
        if (std::abs(factor) < std::abs(continued) * 1.0e-14) {
            break;
        }
    }
    return std::clamp(std::exp(-x + shape * std::log(x)
        - std::lgamma(shape)) * continued, 0.0, 1.0);
}

}  // namespace

RankSumResult mann_whitney(
    const std::vector<double>& first,
    const std::vector<double>& second,
    TestAlternative alternative)
{
    RankSumResult result;
    result.first_count = first.size();
    result.second_count = second.size();
    if (first.empty() || second.empty()) {
        error(result.diagnostics, "insufficient_samples",
              "Mann–Whitney 检验要求两组均有有效观测。");
        return result;
    }
    const auto ranked = rank_values({first, second});
    double tie_sum = 0.0;
    for (std::size_t index = 0; index < ranked.size();) {
        std::size_t end = index + 1;
        while (end < ranked.size() && ranked[end].value == ranked[index].value) {
            ++end;
        }
        const double tie_size = static_cast<double>(end - index);
        tie_sum += tie_size * tie_size * tie_size - tie_size;
        index = end;
    }
    for (const auto& value : ranked) {
        if (value.group == 0) {
            result.rank_sum += value.rank;
        }
    }
    const double n1 = static_cast<double>(first.size());
    const double n2 = static_cast<double>(second.size());
    const double total = n1 + n2;
    const double expected = n1 * (total + 1.0) / 2.0;
    const double variance = n1 * n2 / 12.0
        * (total + 1.0 - tie_sum / (total * (total - 1.0)));
    const double uncorrected_variance = n1 * n2 * (total + 1.0) / 12.0;
    if (variance <= 0.0) {
        error(result.diagnostics, "zero_rank_variance",
              "秩方差为 0，无法计算 Mann–Whitney P 值。");
        return result;
    }
    const double difference = result.rank_sum - expected;
    const double continuity = difference > 0.0 ? 0.5 : difference < 0.0 ? -0.5 : 0.0;
    result.z_statistic = (difference - continuity) / std::sqrt(variance);
    const double cdf = standard_normal_cdf(result.z_statistic);
    result.p_value = alternative == TestAlternative::less
        ? cdf : alternative == TestAlternative::greater
            ? 1.0 - cdf
            : std::clamp(2.0 * (1.0 - standard_normal_cdf(
                std::abs(result.z_statistic))), 0.0, 1.0);
    const double uncorrected_z = (difference - continuity)
        / std::sqrt(uncorrected_variance);
    const double uncorrected_cdf = standard_normal_cdf(uncorrected_z);
    result.p_value_without_tie_correction = alternative == TestAlternative::less
        ? uncorrected_cdf : alternative == TestAlternative::greater
            ? 1.0 - uncorrected_cdf
            : std::clamp(2.0 * (1.0 - standard_normal_cdf(
                std::abs(uncorrected_z))), 0.0, 1.0);
    result.tie_correction = tie_sum > 0.0;
    result.small_sample_warning = first.size() < 10 || second.size() < 10;
    if (result.small_sample_warning) {
        warning(result.diagnostics, "small_sample_normal_approximation",
                "存在样本量小于 10 的组，Mann–Whitney 正态近似只作提示。");
    }
    const double u_statistic = result.rank_sum - n1 * (n1 + 1.0) / 2.0;
    if (n1 * n2 > 0.0) {
        result.effect_size = 1.0 - 2.0 * u_statistic / (n1 * n2);
    }
    std::vector<double> differences;
    for (const double first_value : first) {
        for (const double second_value : second) {
            differences.push_back(first_value - second_value);
        }
    }
    std::sort(differences.begin(), differences.end());
    result.location_difference = differences[differences.size() / 2];
    return result;
}

SignedRankResult wilcoxon_signed_rank(
    const std::vector<double>& first,
    const std::vector<double>& second,
    TestAlternative alternative)
{
    SignedRankResult result;
    if (first.size() != second.size()) {
        error(result.diagnostics, "unequal_pair_count",
              "Wilcoxon signed-rank 检验要求两列行数相同。");
        return result;
    }
    std::vector<std::vector<double>> absolute_differences(2);
    std::vector<int> signs;
    for (std::size_t index = 0; index < first.size(); ++index) {
        const double difference = first[index] - second[index];
        if (difference != 0.0 && std::isfinite(difference)) {
            absolute_differences[0].push_back(std::abs(difference));
            signs.push_back(difference > 0.0 ? 1 : -1);
        }
    }
    result.count = signs.size();
    if (result.count < 2) {
        error(result.diagnostics, "insufficient_nonzero_differences",
              "Wilcoxon signed-rank 检验至少需要两个非零差值。");
        return result;
    }
    const auto ranks = rank_values(absolute_differences);
    double tie_sum = 0.0;
    for (std::size_t index = 0; index < ranks.size();) {
        std::size_t end = index + 1;
        while (end < ranks.size() && ranks[end].value == ranks[index].value) {
            ++end;
        }
        const double size = static_cast<double>(end - index);
        tie_sum += size * size * size - size;
        index = end;
    }
    for (const auto& ranked : ranks) {
        if (signs[ranked.origin] > 0) {
            result.positive_rank_sum += ranked.rank;
        } else {
            result.negative_rank_sum += ranked.rank;
        }
    }
    const double count = static_cast<double>(result.count);
    const double expected = count * (count + 1.0) / 4.0;
    const double variance = count * (count + 1.0) * (2.0 * count + 1.0) / 24.0
        - tie_sum / 48.0;
    result.tie_correction = tie_sum > 0.0;
    result.small_sample_warning = result.count < 10;
    if (result.small_sample_warning) {
        warning(result.diagnostics, "small_sample_normal_approximation",
                "非零差值少于 10，Wilcoxon 正态近似只作提示。");
    }
    if (variance <= 0.0) {
        error(result.diagnostics, "zero_rank_variance",
              "符号秩方差为 0，无法计算 Wilcoxon P 值。");
        return result;
    }
    const double statistic = result.positive_rank_sum - expected;
    result.z_statistic = (statistic - (statistic > 0.0 ? 0.5 : statistic < 0.0 ? -0.5 : 0.0))
        / std::sqrt(variance);
    const double cdf = standard_normal_cdf(result.z_statistic);
    result.p_value = alternative == TestAlternative::less
        ? cdf : alternative == TestAlternative::greater
            ? 1.0 - cdf
            : std::clamp(2.0 * (1.0 - standard_normal_cdf(
                std::abs(result.z_statistic))), 0.0, 1.0);
    return result;
}

KruskalWallisResult kruskal_wallis(
    const std::vector<std::vector<double>>& groups,
    const std::vector<std::string>& labels)
{
    KruskalWallisResult result;
    if (groups.size() < 2 || (!labels.empty() && labels.size() != groups.size())) {
        error(result.diagnostics, "invalid_groups",
              "Kruskal–Wallis 检验至少需要两个且标签匹配的组。");
        return result;
    }
    const auto ranked = rank_values(groups);
    const double total = static_cast<double>(ranked.size());
    if (total <= static_cast<double>(groups.size())) {
        error(result.diagnostics, "insufficient_observations",
              "Kruskal–Wallis 检验需要每个组至少一个有效观测。");
        return result;
    }
    std::vector<double> rank_sums(groups.size(), 0.0);
    for (const auto& value : ranked) {
        rank_sums[value.group] += value.rank;
    }
    double tie_sum = 0.0;
    for (std::size_t index = 0; index < ranked.size();) {
        std::size_t end = index + 1;
        while (end < ranked.size() && ranked[end].value == ranked[index].value) {
            ++end;
        }
        const double size = static_cast<double>(end - index);
        tie_sum += size * size * size - size;
        index = end;
    }
    for (std::size_t group = 0; group < groups.size(); ++group) {
        std::vector<double> sorted = groups[group];
        std::sort(sorted.begin(), sorted.end());
        result.groups.push_back({
            labels.empty() ? std::to_string(group + 1) : labels[group],
            groups[group].size(),
            sorted[sorted.size() / 2],
            rank_sums[group] / static_cast<double>(groups[group].size()),
            std::nullopt});
        result.h_statistic += rank_sums[group] * rank_sums[group]
            / static_cast<double>(groups[group].size());
        if (groups[group].size() < 5) {
            warning(result.diagnostics, "small_group_approximation",
                    "存在少于 5 个观测的组，卡方近似可能不可靠。");
        }
    }
    result.h_statistic = 12.0 / (total * (total + 1.0))
        * result.h_statistic - 3.0 * (total + 1.0);
    const double correction = 1.0 - tie_sum / (total * (total * total - 1.0));
    result.adjusted_h_statistic = correction > 0.0
        ? result.h_statistic / correction : result.h_statistic;
    result.degrees_of_freedom = static_cast<double>(groups.size() - 1);
    result.p_value_unadjusted = chi_square_right_tail(
        result.h_statistic, result.degrees_of_freedom);
    result.p_value = chi_square_right_tail(
        result.adjusted_h_statistic, result.degrees_of_freedom);
    result.tie_correction = tie_sum > 0.0;
    const double grand_mean_rank = (total + 1.0) / 2.0;
    for (std::size_t group = 0; group < result.groups.size(); ++group) {
        const double n_j = static_cast<double>(groups[group].size());
        double variance = (total + 1.0) * (total - n_j) / (12.0 * n_j);
        if (correction > 0.0) {
            variance *= correction;
        }
        if (variance > 0.0) {
            result.groups[group].z_value =
                (result.groups[group].mean_rank - grand_mean_rank) / std::sqrt(variance);
        }
    }
    result.small_sample_warning = std::any_of(
        groups.cbegin(), groups.cend(),
        [](const std::vector<double>& group) { return group.size() < 5; });
    if (total > 1.0) {
        result.effect_size = result.adjusted_h_statistic / (total - 1.0);
    }
    return result;
}

}  // namespace datalab::domain::statistics
