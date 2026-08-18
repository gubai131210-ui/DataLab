#include "domain/statistics/inference_extensions.h"

#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <string>

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

bool valid_confidence(double value)
{
    return value > 0.0 && value < 1.0;
}

double normal_quantile(double probability)
{
    if (!(probability > 0.0 && probability < 1.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double lower = -10.0;
    double upper = 10.0;
    for (int index = 0; index < 120; ++index) {
        const double middle = (lower + upper) / 2.0;
        if (standard_normal_cdf(middle) < probability) {
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

double hypergeometric_probability(
    std::size_t x,
    std::size_t population,
    std::size_t successes,
    std::size_t sample)
{
    if (x > successes || x > sample
        || sample - x > population - successes) {
        return 0.0;
    }
    const double logarithm = log_combination(successes, x)
        + log_combination(population - successes, sample - x)
        - log_combination(population, sample);
    return std::exp(logarithm);
}

double fisher_two_sided(
    std::size_t first_events,
    std::size_t first_trials,
    std::size_t second_events,
    std::size_t second_trials)
{
    const std::size_t population = first_trials + second_trials;
    const std::size_t successes = first_events + second_events;
    const std::size_t minimum = successes > second_trials
        ? successes - second_trials : 0;
    const std::size_t maximum = std::min(successes, first_trials);
    const double observed = hypergeometric_probability(
        first_events, population, successes, first_trials);
    double probability = 0.0;
    for (std::size_t value = minimum; value <= maximum; ++value) {
        const double candidate = hypergeometric_probability(
            value, population, successes, first_trials);
        if (candidate <= observed * (1.0 + 1.0e-12)) {
            probability += candidate;
        }
    }
    return std::clamp(probability, 0.0, 1.0);
}

double regularized_gamma_q(double shape, double value)
{
    if (!(shape > 0.0) || value < 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (value == 0.0) {
        return 1.0;
    }
    constexpr int max_iterations = 200;
    constexpr double epsilon = 3.0e-14;
    constexpr double tiny = 1.0e-300;
    if (value < shape + 1.0) {
        double term = 1.0 / shape;
        double sum = term;
        for (int index = 1; index <= max_iterations; ++index) {
            term *= value / (shape + static_cast<double>(index));
            sum += term;
            if (std::abs(term) < std::abs(sum) * epsilon) {
                break;
            }
        }
        return 1.0 - sum * std::exp(-value + shape * std::log(value)
            - std::lgamma(shape));
    }
    double inverse = 1.0 / value;
    double factor = 1.0;
    double sum = 1.0;
    for (int index = 1; index <= max_iterations; ++index) {
        factor *= (shape - static_cast<double>(index)) * inverse;
        sum += factor;
        if (std::abs(factor) < std::abs(sum) * epsilon) {
            break;
        }
    }
    const double continued = std::exp(-value + shape * std::log(value)
        - std::lgamma(shape)) * sum;
    return std::clamp(continued, 0.0, 1.0);
}

}  // namespace

PairedTTestResult paired_t_test(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double confidence_level,
    TestAlternative alternative)
{
    PairedTTestResult result;
    if (first.size() != second.size()) {
        add_error(result.diagnostics, "unequal_pair_count",
                  "配对 t 检验要求两列具有相同的行数。");
        return result;
    }
    if (!valid_confidence(confidence_level)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    std::vector<double> differences;
    differences.reserve(first.size());
    for (std::size_t index = 0; index < first.size(); ++index) {
        if (std::isfinite(first[index]) && std::isfinite(second[index])) {
            differences.push_back(first[index] - second[index]);
        }
    }
    result.count = differences.size();
    if (differences.size() < 2) {
        add_error(result.diagnostics, "insufficient_pairs",
                  "配对 t 检验至少需要两个完整有效配对。");
        return result;
    }
    result.mean_difference = std::accumulate(
        differences.cbegin(), differences.cend(), 0.0)
        / static_cast<double>(differences.size());
    double sum_squared = 0.0;
    for (const double difference : differences) {
        sum_squared += (difference - result.mean_difference)
            * (difference - result.mean_difference);
    }
    result.sample_standard_deviation = std::sqrt(
        sum_squared / static_cast<double>(differences.size() - 1));
    result.standard_error = result.sample_standard_deviation
        / std::sqrt(static_cast<double>(differences.size()));
    result.degrees_of_freedom = static_cast<double>(differences.size() - 1);
    if (result.standard_error == 0.0) {
        if (result.mean_difference == 0.0) {
            result.p_value = 1.0;
        } else {
            add_error(result.diagnostics, "zero_variance",
                      "配对差值标准差为 0，无法计算有限 t 统计量。");
            return result;
        }
    } else {
        result.t_statistic = result.mean_difference / result.standard_error;
        const double cdf = student_t_cdf(
            result.t_statistic, result.degrees_of_freedom);
        result.p_value = alternative == TestAlternative::less
            ? cdf
            : alternative == TestAlternative::greater
                ? 1.0 - cdf
                : std::clamp(2.0 * (1.0 - student_t_cdf(
                    std::abs(result.t_statistic), result.degrees_of_freedom)), 0.0, 1.0);
    }
    const double alpha = 1.0 - confidence_level;
    const double critical = alternative == TestAlternative::two_sided
        ? student_t_quantile(1.0 - alpha / 2.0, result.degrees_of_freedom)
        : student_t_quantile(1.0 - alpha, result.degrees_of_freedom);
    if (alternative == TestAlternative::less) {
        result.confidence_upper = result.mean_difference
            + critical * result.standard_error;
    } else if (alternative == TestAlternative::greater) {
        result.confidence_lower = result.mean_difference
            - critical * result.standard_error;
    } else {
        result.confidence_lower = result.mean_difference
            - critical * result.standard_error;
        result.confidence_upper = result.mean_difference
            + critical * result.standard_error;
    }
    return result;
}

TukeyResult tukey_multiple_comparisons(
    const std::vector<std::vector<double>>& groups,
    const std::vector<std::string>& labels,
    double confidence_level)
{
    TukeyResult result;
    result.confidence_level = confidence_level;
    result.family_confidence_level = confidence_level;
    result.alpha = 1.0 - confidence_level;
    if (groups.size() < 2 || (!labels.empty() && labels.size() != groups.size())) {
        add_error(result.diagnostics, "invalid_groups",
                  "Tukey 比较至少需要两个且标签数量必须匹配的有效组。");
        return result;
    }
    if (!valid_confidence(confidence_level)) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    double grand_total = 0.0;
    std::size_t total_count = 0;
    std::vector<double> means;
    means.reserve(groups.size());
    for (const auto& group : groups) {
        if (group.empty()) {
            add_error(result.diagnostics, "empty_group",
                      "Tukey 比较不允许空组。");
            return result;
        }
        if (!std::all_of(group.cbegin(), group.cend(),
                         [](double value) { return std::isfinite(value); })) {
            add_error(result.diagnostics, "non_finite_group",
                      "Tukey 比较不允许无穷或非数值观测。");
            return result;
        }
        const double mean = std::accumulate(group.cbegin(), group.cend(), 0.0)
            / static_cast<double>(group.size());
        means.push_back(mean);
        grand_total += std::accumulate(group.cbegin(), group.cend(), 0.0);
        total_count += group.size();
    }
    const double grand_mean = grand_total / static_cast<double>(total_count);
    double between = 0.0;
    double error = 0.0;
    for (std::size_t index = 0; index < groups.size(); ++index) {
        between += static_cast<double>(groups[index].size())
            * (means[index] - grand_mean) * (means[index] - grand_mean);
        for (const double value : groups[index]) {
            error += (value - means[index]) * (value - means[index]);
        }
    }
    const std::size_t error_df = total_count - groups.size();
    if (error_df == 0) {
        add_error(result.diagnostics, "insufficient_error_degrees_of_freedom",
                  "Tukey 比较需要正的组内误差自由度。");
        return result;
    }
    result.error_mean_square = error / static_cast<double>(error_df);
    result.error_degrees_of_freedom = static_cast<double>(error_df);
    const std::size_t comparison_count = groups.size() * (groups.size() - 1) / 2;
    const double alpha = 1.0 - confidence_level;
    result.individual_confidence_level =
        1.0 - alpha / static_cast<double>(comparison_count);
    const double critical = std::sqrt(2.0) * student_t_quantile(
        1.0 - alpha / static_cast<double>(comparison_count * 2), result.error_degrees_of_freedom);
    for (std::size_t first = 0; first < groups.size(); ++first) {
        for (std::size_t second = first + 1; second < groups.size(); ++second) {
            TukeyComparison comparison;
            comparison.first_label = labels.empty()
                ? std::to_string(first + 1) : labels[first];
            comparison.second_label = labels.empty()
                ? std::to_string(second + 1) : labels[second];
            comparison.mean_difference = means[first] - means[second];
            comparison.standard_error = std::sqrt(result.error_mean_square / 2.0
                * (1.0 / static_cast<double>(groups[first].size())
                    + 1.0 / static_cast<double>(groups[second].size())));
            if (!(comparison.standard_error > 0.0)
                || !std::isfinite(comparison.standard_error)) {
                comparison.standardized_difference = 0.0;
                comparison.q_statistic =
                    std::abs(comparison.mean_difference) > 0.0
                    ? std::numeric_limits<double>::infinity() : 0.0;
                comparison.adjusted_p_value =
                    std::abs(comparison.mean_difference) > 0.0 ? 0.0 : 1.0;
                comparison.confidence_lower = comparison.mean_difference;
                comparison.confidence_upper = comparison.mean_difference;
                result.comparisons.push_back(comparison);
                continue;
            } else {
                comparison.standardized_difference =
                    comparison.mean_difference / std::sqrt(result.error_mean_square);
            }
            comparison.q_statistic = std::abs(comparison.mean_difference)
                / comparison.standard_error * std::sqrt(2.0);
            comparison.confidence_lower = comparison.mean_difference - critical
                * comparison.standard_error / std::sqrt(2.0);
            comparison.confidence_upper = comparison.mean_difference + critical
                * comparison.standard_error / std::sqrt(2.0);
            const double t = std::abs(comparison.mean_difference)
                / comparison.standard_error;
            comparison.adjusted_p_value = std::clamp(
                static_cast<double>(comparison_count) * 2.0
                    * (1.0 - student_t_cdf(t, result.error_degrees_of_freedom)),
                0.0, 1.0);
            comparison.significant = comparison.confidence_lower > 0.0
                || comparison.confidence_upper < 0.0;
            result.comparisons.push_back(comparison);
        }
    }
    add_warning(result.diagnostics, "tukey_studentized_range_approximation",
                "当前 Tukey 调整使用 Studentized range 的保守 t 分布近似。");
    result.rules.push_back({
        "family_error_rate", "not_triggered",
        "Tukey 同时置信水平 = "
            + std::to_string(result.family_confidence_level)
            + "；显著性由同时置信区间是否包含 0 决定。",
        {},
        "不要把逐比较 alpha 当成家族错误率。"});
    return result;
}

TwoProportionsResult two_proportions_test(
    std::size_t first_events,
    std::size_t first_trials,
    std::size_t second_events,
    std::size_t second_trials,
    double confidence_level,
    TestAlternative alternative)
{
    TwoProportionsResult result;
    result.first_events = first_events;
    result.first_trials = first_trials;
    result.second_events = second_events;
    result.second_trials = second_trials;
    if (!valid_confidence(confidence_level)
        || first_trials == 0 || second_trials == 0
        || first_events > first_trials || second_events > second_trials) {
        add_error(result.diagnostics, "invalid_proportion_counts",
                  "两比例检验要求非零分母，事件数不能超过试验数，且置信水平有效。");
        return result;
    }
    result.first_proportion = static_cast<double>(first_events)
        / static_cast<double>(first_trials);
    result.second_proportion = static_cast<double>(second_events)
        / static_cast<double>(second_trials);
    result.difference = result.first_proportion - result.second_proportion;
    const double separate_se = std::sqrt(
        result.first_proportion * (1.0 - result.first_proportion)
            / static_cast<double>(first_trials)
        + result.second_proportion * (1.0 - result.second_proportion)
            / static_cast<double>(second_trials));
    if (separate_se == 0.0) {
        add_error(result.diagnostics, "zero_proportion_variance",
                  "比例标准误为 0，无法计算正态近似检验。");
        return result;
    }
    result.z_statistic = result.difference / separate_se;
    const double cdf = standard_normal_cdf(result.z_statistic);
    result.p_value = alternative == TestAlternative::less
        ? cdf
        : alternative == TestAlternative::greater
            ? 1.0 - cdf
            : std::clamp(2.0 * (1.0 - standard_normal_cdf(
                std::abs(result.z_statistic))), 0.0, 1.0);
    const double critical = alternative == TestAlternative::two_sided
        ? normal_quantile(0.5 + confidence_level / 2.0)
        : normal_quantile(confidence_level);
    if (alternative == TestAlternative::less) {
        result.confidence_upper = result.difference + critical * separate_se;
    } else if (alternative == TestAlternative::greater) {
        result.confidence_lower = result.difference - critical * separate_se;
    } else {
        result.confidence_lower = result.difference - critical * separate_se;
        result.confidence_upper = result.difference + critical * separate_se;
    }
    if (first_events + second_events > 0
        && first_trials + second_trials > first_events + second_events) {
        result.fisher_p_value = fisher_two_sided(
            first_events, first_trials, second_events, second_trials);
    }
    if (first_events < 5 || first_trials - first_events < 5
        || second_events < 5 || second_trials - second_events < 5) {
        add_warning(result.diagnostics, "small_count_normal_approximation",
                    "事件数或非事件数小于 5，正态近似可能不准确；请参考 Fisher 精确检验。");
    }
    return result;
}

ChiSquareResult chi_square_association(
    const std::vector<std::vector<double>>& observed,
    const std::vector<std::string>& row_labels,
    const std::vector<std::string>& column_labels)
{
    ChiSquareResult result;
    if (observed.size() < 2 || observed.front().size() < 2
        || (!row_labels.empty() && row_labels.size() != observed.size())
        || (!column_labels.empty() && column_labels.size() != observed.front().size())) {
        add_error(result.diagnostics, "invalid_contingency_table",
                  "列联表至少需要两行两列，且标签数量必须匹配。");
        return result;
    }
    result.rows = observed.size();
    result.columns = observed.front().size();
    std::vector<double> row_totals(result.rows, 0.0);
    std::vector<double> column_totals(result.columns, 0.0);
    double total = 0.0;
    for (std::size_t row = 0; row < result.rows; ++row) {
        if (observed[row].size() != result.columns) {
            add_error(result.diagnostics, "ragged_contingency_table",
                      "列联表每一行必须具有相同列数。");
            return result;
        }
        for (std::size_t column = 0; column < result.columns; ++column) {
            if (observed[row][column] < 0.0 || !std::isfinite(observed[row][column])) {
                add_error(result.diagnostics, "invalid_cell_count",
                          "列联表单元格必须是非负有限计数。");
                return result;
            }
            row_totals[row] += observed[row][column];
            column_totals[column] += observed[row][column];
            total += observed[row][column];
        }
    }
    if (total <= 0.0) {
        add_error(result.diagnostics, "empty_contingency_table",
                  "列联表总计数必须大于 0。");
        return result;
    }
    for (std::size_t row = 0; row < result.rows; ++row) {
        for (std::size_t column = 0; column < result.columns; ++column) {
            const double expected = row_totals[row] * column_totals[column] / total;
            ChiSquareCell cell;
            cell.row_label = row_labels.empty()
                ? std::to_string(row + 1) : row_labels[row];
            cell.column_label = column_labels.empty()
                ? std::to_string(column + 1) : column_labels[column];
            cell.observed = observed[row][column];
            cell.expected = expected;
            if (expected > 0.0) {
                cell.raw_residual = cell.observed - expected;
                cell.standardized_residual = cell.raw_residual / std::sqrt(expected);
                cell.contribution = cell.raw_residual * cell.raw_residual / expected;
                const double row_fraction = row_totals[row] / total;
                const double column_fraction = column_totals[column] / total;
                const double denominator = std::sqrt(
                    expected * (1.0 - row_fraction) * (1.0 - column_fraction));
                cell.adjusted_residual = denominator > 0.0
                    ? cell.raw_residual / denominator : 0.0;
                result.pearson_statistic += cell.contribution;
                if (cell.observed > 0.0) {
                    result.likelihood_ratio_statistic += 2.0 * cell.observed
                        * std::log(cell.observed / expected);
                }
            }
            result.cells.push_back(cell);
        }
    }
    result.degrees_of_freedom = static_cast<double>(
        (result.rows - 1) * (result.columns - 1));
    result.p_value = regularized_gamma_q(
        result.degrees_of_freedom / 2.0, result.pearson_statistic / 2.0);
    result.likelihood_ratio_p_value = regularized_gamma_q(
        result.degrees_of_freedom / 2.0,
        result.likelihood_ratio_statistic / 2.0);
    for (const ChiSquareCell& cell : result.cells) {
        if (cell.expected < 1.0) {
            add_warning(result.diagnostics, "expected_count_below_one",
                        "存在期望频数小于 1 的单元格，卡方近似 P 值不显示。");
            result.p_value.reset();
            result.likelihood_ratio_p_value.reset();
            break;
        }
    }
    return result;
}

}  // namespace datalab::domain::statistics
