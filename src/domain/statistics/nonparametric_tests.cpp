#include "domain/statistics/nonparametric_tests.h"

#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <optional>
#include <map>

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

double tie_correction_sum(const std::vector<RankedValue>& ranked)
{
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
    return tie_sum;
}

std::optional<double> hodges_lehmann_estimate(std::vector<double> differences)
{
    if (differences.empty()) {
        return std::nullopt;
    }
    std::sort(differences.begin(), differences.end());
    const std::size_t middle = differences.size() / 2;
    if (differences.size() % 2 == 1) {
        return differences[middle];
    }
    return (differences[middle - 1] + differences[middle]) / 2.0;
}

void compute_mckean_ryan_ci(
    RankSumResult& result,
    const std::vector<double>& first,
    const std::vector<double>& second,
    double confidence_level,
    TestAlternative alternative)
{
    std::vector<double> differences;
    differences.reserve(first.size() * second.size());
    for (const double first_value : first) {
        for (const double second_value : second) {
            differences.push_back(first_value - second_value);
        }
    }
    const auto estimate = hodges_lehmann_estimate(differences);
    if (!estimate.has_value()) {
        return;
    }
    result.location_difference = estimate;
    result.location_estimate = estimate;
    if (!(confidence_level > 0.0 && confidence_level < 1.0) || differences.size() < 2) {
        return;
    }
    std::sort(differences.begin(), differences.end());
    const std::size_t m = differences.size();
    const auto ranked = rank_values({first, second});
    const double tie_sum = tie_correction_sum(ranked);
    const double n1 = static_cast<double>(first.size());
    const double n2 = static_cast<double>(second.size());
    const double total = n1 + n2;
    const double tie_factor = total > 1.0
        ? 1.0 - tie_sum / (total * (total - 1.0)) : 1.0;
    const double sigma_u = std::sqrt(n1 * n2 * (total + 1.0) / 12.0 * tie_factor);
    const double alpha = 1.0 - confidence_level;
    const double z = alternative == TestAlternative::two_sided
        ? standard_normal_quantile(1.0 - alpha / 2.0)
        : standard_normal_quantile(1.0 - alpha);
    long lower_index = static_cast<long>(std::floor(
        static_cast<double>(m) / 2.0 - z * sigma_u));
    lower_index = std::max(0L, lower_index);
    const long upper_index = static_cast<long>(m) - lower_index - 1;
    if (lower_index >= static_cast<long>(m) || upper_index < lower_index) {
        warning(result.diagnostics, "ci_not_computed",
                "样本量过小，无法计算 McKean–Ryan 置信区间。");
        return;
    }
    const double lower = differences[static_cast<std::size_t>(lower_index)];
    const double upper = differences[static_cast<std::size_t>(upper_index)];
    if (alternative == TestAlternative::less) {
        result.ci_upper = upper;
    } else if (alternative == TestAlternative::greater) {
        result.ci_lower = lower;
    } else {
        result.ci_lower = lower;
        result.ci_upper = upper;
    }
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
    TestAlternative alternative,
    double confidence_level)
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
    compute_mckean_ryan_ci(result, first, second, confidence_level, alternative);
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

    // Dunn pairwise (global ranks) + Bonferroni family-wise α.
    const double c_tie = (total > 1.0)
        ? tie_sum / (12.0 * (total - 1.0)) : 0.0;
    const double base_variance = total * (total + 1.0) / 12.0 - c_tie;
    const std::size_t k = result.groups.size();
    const double pair_count = static_cast<double>(k * (k - 1) / 2);
    result.family_alpha = 0.05;
    for (std::size_t i = 0; i < k; ++i) {
        for (std::size_t j = i + 1; j < k; ++j) {
            DunnComparison comparison;
            comparison.first_label = result.groups[i].label;
            comparison.second_label = result.groups[j].label;
            comparison.mean_rank_difference =
                result.groups[i].mean_rank - result.groups[j].mean_rank;
            const double ni = static_cast<double>(result.groups[i].count);
            const double nj = static_cast<double>(result.groups[j].count);
            comparison.standard_error =
                std::sqrt(base_variance * (1.0 / ni + 1.0 / nj));
            if (!(comparison.standard_error > 0.0)) {
                comparison.z_statistic = 0.0;
                comparison.p_value = 1.0;
                comparison.adjusted_p_value = 1.0;
                comparison.significant = false;
                result.dunn_comparisons.push_back(comparison);
                continue;
            }
            comparison.z_statistic =
                std::abs(comparison.mean_rank_difference)
                / comparison.standard_error;
            const double p_raw = std::clamp(
                2.0 * (1.0 - standard_normal_cdf(comparison.z_statistic)),
                0.0, 1.0);
            comparison.p_value = p_raw;
            comparison.adjusted_p_value =
                std::clamp(pair_count * p_raw, 0.0, 1.0);
            comparison.significant =
                *comparison.adjusted_p_value <= result.family_alpha;
            result.dunn_comparisons.push_back(comparison);
        }
    }
    return result;
}

std::vector<DunnComparison> steel_dwass_pairwise(
    const std::vector<std::vector<double>>& groups,
    const std::vector<std::string>& labels,
    const double family_alpha)
{
    std::vector<DunnComparison> comparisons;
    if (groups.size() < 2) {
        return comparisons;
    }
    const std::size_t k = groups.size();
    const double pair_count = static_cast<double>(k * (k - 1) / 2);
    // Asymptotic Tukey–Kramer critical for |Z|: q_{α,k,∞}/√2 ≈ z_{1-α/(2m)}
    // (same ∞ approximation family as product Tukey).
    const double z_critical = pair_count > 0.0
        ? standard_normal_quantile(1.0 - family_alpha / (2.0 * pair_count))
        : standard_normal_quantile(1.0 - family_alpha / 2.0);
    for (std::size_t i = 0; i < k; ++i) {
        for (std::size_t j = i + 1; j < k; ++j) {
            DunnComparison comparison;
            comparison.first_label = labels.size() == groups.size()
                ? labels[i] : std::to_string(i + 1);
            comparison.second_label = labels.size() == groups.size()
                ? labels[j] : std::to_string(j + 1);
            if (groups[i].empty() || groups[j].empty()) {
                comparison.significant = false;
                comparison.p_value = 1.0;
                comparison.adjusted_p_value = 1.0;
                comparisons.push_back(comparison);
                continue;
            }
            const auto ranked = rank_values({groups[i], groups[j]});
            double rank_sum = 0.0;
            double tie_sum = 0.0;
            for (std::size_t index = 0; index < ranked.size();) {
                std::size_t end = index + 1;
                while (end < ranked.size()
                       && ranked[end].value == ranked[index].value) {
                    ++end;
                }
                const double tie_size = static_cast<double>(end - index);
                tie_sum += tie_size * tie_size * tie_size - tie_size;
                index = end;
            }
            for (const auto& value : ranked) {
                if (value.group == 0) {
                    rank_sum += value.rank;
                }
            }
            const double n1 = static_cast<double>(groups[i].size());
            const double n2 = static_cast<double>(groups[j].size());
            const double total = n1 + n2;
            const double expected = n1 * (total + 1.0) / 2.0;
            const double variance = n1 * n2 / 12.0
                * (total + 1.0 - tie_sum / (total * (total - 1.0)));
            comparison.mean_rank_difference = rank_sum - expected;
            comparison.standard_error =
                variance > 0.0 ? std::sqrt(variance) : 0.0;
            if (!(comparison.standard_error > 0.0)) {
                comparison.z_statistic = 0.0;
                comparison.p_value = 1.0;
                comparison.adjusted_p_value = 1.0;
                comparison.significant = false;
                comparisons.push_back(comparison);
                continue;
            }
            comparison.z_statistic =
                std::abs(rank_sum - expected) / comparison.standard_error;
            const double p_raw = std::clamp(
                2.0 * (1.0 - standard_normal_cdf(comparison.z_statistic)),
                0.0, 1.0);
            comparison.p_value = p_raw;
            comparison.adjusted_p_value =
                std::clamp(pair_count * p_raw, 0.0, 1.0);
            comparison.significant = comparison.z_statistic >= z_critical;
            comparisons.push_back(comparison);
        }
    }
    return comparisons;
}

namespace {

std::size_t local_group_index(std::vector<std::string>& labels, const std::string& value)
{
    for (std::size_t index = 0; index < labels.size(); ++index) {
        if (labels[index] == value) {
            return index;
        }
    }
    labels.push_back(value);
    return labels.size() - 1;
}

double median_of(std::vector<double> values)
{
    if (values.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    std::sort(values.begin(), values.end());
    const std::size_t mid = values.size() / 2;
    if (values.size() % 2 == 1) {
        return values[mid];
    }
    return 0.5 * (values[mid - 1] + values[mid]);
}

}  // namespace

FriedmanResult friedman_test(
    const std::vector<double>& responses,
    const std::vector<std::string>& treatments,
    const std::vector<std::string>& blocks)
{
    FriedmanResult result;
    if (responses.size() != treatments.size() || responses.size() != blocks.size()
        || responses.empty()) {
        error(result.diagnostics, "friedman_input_mismatch",
              "Friedman 检验要求响应、处理与区组等长且非空。");
        return result;
    }
    std::vector<std::string> treatment_order;
    std::vector<std::string> block_order;
    for (std::size_t index = 0; index < responses.size(); ++index) {
        if (!std::isfinite(responses[index]) || treatments[index].empty()
            || blocks[index].empty()) {
            error(result.diagnostics, "friedman_invalid_row",
                  "Friedman 检验要求数值响应与非空处理/区组标签。");
            return result;
        }
        local_group_index(treatment_order, treatments[index]);
        local_group_index(block_order, blocks[index]);
    }
    const std::size_t k = treatment_order.size();
    const std::size_t b = block_order.size();
    result.treatment_count = k;
    result.block_count = b;
    if (k < 2 || b < 2) {
        error(result.diagnostics, "friedman_insufficient_levels",
              "Friedman 检验至少需要 2 个处理与 2 个区组。");
        return result;
    }
    // cell[block][treatment] = optional value; must be exactly one per cell.
    std::vector<std::vector<std::optional<double>>> cells(
        b, std::vector<std::optional<double>>(k));
    for (std::size_t index = 0; index < responses.size(); ++index) {
        const std::size_t treatment =
            local_group_index(treatment_order, treatments[index]);
        const std::size_t block = local_group_index(block_order, blocks[index]);
        if (cells[block][treatment].has_value()) {
            error(result.diagnostics, "friedman_duplicate_cell",
                  "同一区组与处理出现重复观测，Friedman 要求每格恰 1 个观测。");
            return result;
        }
        cells[block][treatment] = responses[index];
    }
    for (std::size_t block = 0; block < b; ++block) {
        for (std::size_t treatment = 0; treatment < k; ++treatment) {
            if (!cells[block][treatment].has_value()) {
                error(result.diagnostics, "friedman_unbalanced",
                      "存在缺失的区组×处理组合，Friedman 要求平衡设计。");
                return result;
            }
        }
    }

    std::vector<double> rank_sums(k, 0.0);
    double tie_cube_sum = 0.0;
    std::vector<std::vector<double>> treatment_values(k);
    for (std::size_t block = 0; block < b; ++block) {
        std::vector<double> block_values;
        block_values.reserve(k);
        for (std::size_t treatment = 0; treatment < k; ++treatment) {
            block_values.push_back(*cells[block][treatment]);
            treatment_values[treatment].push_back(*cells[block][treatment]);
        }
        std::vector<RankedValue> ranked;
        ranked.reserve(k);
        for (std::size_t treatment = 0; treatment < k; ++treatment) {
            ranked.push_back({block_values[treatment], treatment, 0.0, treatment});
        }
        std::sort(ranked.begin(), ranked.end(),
                  [](const RankedValue& first, const RankedValue& second) {
                      return first.value < second.value;
                  });
        std::size_t start = 0;
        while (start < ranked.size()) {
            std::size_t end = start + 1;
            while (end < ranked.size()
                   && ranked[end].value == ranked[start].value) {
                ++end;
            }
            const double rank = (static_cast<double>(start + 1)
                + static_cast<double>(end)) / 2.0;
            const double tie_size = static_cast<double>(end - start);
            tie_cube_sum += tie_size * tie_size * tie_size - tie_size;
            for (std::size_t index = start; index < end; ++index) {
                ranked[index].rank = rank;
            }
            start = end;
        }
        for (const auto& value : ranked) {
            rank_sums[value.group] += value.rank;
        }
    }

    double sum_sq = 0.0;
    for (const double rank_sum : rank_sums) {
        sum_sq += rank_sum * rank_sum;
    }
    const double bk = static_cast<double>(b) * static_cast<double>(k);
    result.s_statistic =
        12.0 / (bk * (static_cast<double>(k) + 1.0)) * sum_sq
        - 3.0 * static_cast<double>(b) * (static_cast<double>(k) + 1.0);
    const double denom =
        1.0 - tie_cube_sum / (bk * (static_cast<double>(k) * static_cast<double>(k) - 1.0));
    result.tie_correction = tie_cube_sum > 0.0;
    result.adjusted_s_statistic =
        (result.tie_correction && denom > 0.0) ? result.s_statistic / denom
                                               : result.s_statistic;
    result.degrees_of_freedom = static_cast<double>(k - 1);
    result.p_value = chi_square_right_tail(
        result.adjusted_s_statistic, result.degrees_of_freedom);

    result.treatments.reserve(k);
    for (std::size_t treatment = 0; treatment < k; ++treatment) {
        FriedmanTreatment row;
        row.label = treatment_order[treatment];
        row.count = b;
        row.median = median_of(treatment_values[treatment]);
        row.mean_rank = rank_sums[treatment] / static_cast<double>(b);
        result.treatments.push_back(std::move(row));
    }
    return result;
}

}  // namespace datalab::domain::statistics
