#include "domain/statistics/attribute_agreement.h"

#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace datalab::domain::statistics {
namespace {

void add_diagnostic(std::vector<DiagnosticMessage>& diagnostics,
                    DiagnosticMessage::Severity severity, const char* code,
                    const char* message)
{
    diagnostics.push_back({severity, code, message});
}

double critical_value(double confidence_level)
{
    return standard_normal_quantile(0.5 + confidence_level / 2.0);
}

AgreementEstimate estimate(const std::vector<std::pair<std::string, std::string>>& pairs,
                           double confidence_level)
{
    AgreementEstimate result;
    result.confidence_level = confidence_level;
    std::map<std::string, std::size_t> left_counts;
    std::map<std::string, std::size_t> right_counts;
    std::size_t observed_count = 0;
    for (const auto& [left, right] : pairs) {
        if (left.empty() || right.empty()) {
            continue;
        }
        ++result.valid_count;
        ++left_counts[left];
        ++right_counts[right];
        if (left == right) {
            ++observed_count;
        }
    }
    result.agreement_count = observed_count;
    if (result.valid_count == 0) {
        return result;
    }
    result.agreement_percent = 100.0 * observed_count / result.valid_count;
    double expected = 0.0;
    for (const auto& [label, count] : left_counts) {
        const auto right = right_counts.find(label);
        if (right != right_counts.end()) {
            expected += static_cast<double>(count * right->second)
                / static_cast<double>(result.valid_count * result.valid_count);
        }
    }
    const double observed = static_cast<double>(observed_count)
        / static_cast<double>(result.valid_count);
    result.expected_agreement = expected;
    if (std::abs(1.0 - expected) <= 1e-12) {
        result.identifiable = false;
        result.kappa = 0.0;
        result.kappa_standard_error = 0.0;
        result.kappa_ci_low = 0.0;
        result.kappa_ci_high = 0.0;
        return result;
    }
    result.kappa = (observed - expected) / (1.0 - expected);
    const double variance = observed * (1.0 - observed)
        / (static_cast<double>(result.valid_count)
           * std::pow(1.0 - expected, 2.0));
    result.kappa_standard_error = std::sqrt(std::max(0.0, variance));
    const double z = critical_value(confidence_level);
    const double margin = std::isfinite(z) ? z * result.kappa_standard_error : 0.0;
    result.kappa_ci_low = std::max(-1.0, result.kappa - margin);
    result.kappa_ci_high = std::min(1.0, result.kappa + margin);
    return result;
}

AgreementEstimate fleiss_from_item_ratings(
    const std::vector<std::vector<std::string>>& item_ratings,
    double confidence_level)
{
    AgreementEstimate result;
    result.confidence_level = confidence_level;
    result.method = "fleiss";
    result.variance_method = "fleiss_null";
    if (item_ratings.empty()) {
        result.identifiable = false;
        return result;
    }
    const std::size_t m = item_ratings.front().size();
    if (m < 2) {
        result.identifiable = false;
        return result;
    }
    std::map<std::string, std::size_t> category_index;
    std::vector<std::string> categories;
    std::vector<std::vector<std::string>> complete;
    for (const auto& ratings : item_ratings) {
        if (ratings.size() != m) {
            continue;
        }
        bool missing = false;
        for (const std::string& rating : ratings) {
            if (rating.empty()) {
                missing = true;
                break;
            }
        }
        if (missing) {
            continue;
        }
        complete.push_back(ratings);
        for (const std::string& rating : ratings) {
            if (category_index.emplace(rating, category_index.size()).second) {
                categories.push_back(rating);
            }
        }
    }
    const std::size_t n = complete.size();
    const std::size_t k = categories.size();
    result.valid_count = n;
    if (n == 0 || k == 0) {
        result.identifiable = false;
        return result;
    }
    std::vector<std::vector<double>> counts(
        n, std::vector<double>(k, 0.0));
    std::size_t exact_agreements = 0;
    for (std::size_t item = 0; item < n; ++item) {
        bool all_equal = true;
        for (const std::string& rating : complete[item]) {
            counts[item][category_index[rating]] += 1.0;
            if (rating != complete[item].front()) {
                all_equal = false;
            }
        }
        if (all_equal) {
            ++exact_agreements;
        }
    }
    result.agreement_count = exact_agreements;
    result.agreement_percent = 100.0 * static_cast<double>(exact_agreements)
        / static_cast<double>(n);

    std::vector<double> p(k, 0.0);
    double p_bar = 0.0;
    const double trials = static_cast<double>(m);
    const double samples = static_cast<double>(n);
    for (std::size_t item = 0; item < n; ++item) {
        double item_pairs = 0.0;
        for (std::size_t category = 0; category < k; ++category) {
            const double x = counts[item][category];
            p[category] += x;
            item_pairs += x * (x - 1.0);
        }
        p_bar += item_pairs / (trials * (trials - 1.0));
    }
    p_bar /= samples;
    double p_e = 0.0;
    double p_cube = 0.0;
    for (std::size_t category = 0; category < k; ++category) {
        p[category] /= samples * trials;
        p_e += p[category] * p[category];
        p_cube += p[category] * p[category] * p[category];
    }
    result.expected_agreement = p_e;
    if (std::abs(1.0 - p_e) <= 1e-12) {
        result.identifiable = false;
        result.kappa = 0.0;
        return result;
    }
    result.kappa = (p_bar - p_e) / (1.0 - p_e);
    const double variance = (2.0 / (samples * trials * (trials - 1.0)))
        * (p_e - (2.0 * trials - 3.0) * p_e * p_e
           + 2.0 * (trials - 2.0) * p_cube)
        / std::pow(1.0 - p_e, 2.0);
    result.kappa_standard_error = std::sqrt(std::max(0.0, variance));
    const double z = critical_value(confidence_level);
    const double margin = std::isfinite(z) ? z * result.kappa_standard_error : 0.0;
    result.kappa_ci_low = std::max(-1.0, result.kappa - margin);
    result.kappa_ci_high = std::min(1.0, result.kappa + margin);
    return result;
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

std::optional<double> parse_finite_number(const std::string& text)
{
    if (text.empty()) {
        return std::nullopt;
    }
    std::size_t index = 0;
    while (index < text.size()
           && std::isspace(static_cast<unsigned char>(text[index]))) {
        ++index;
    }
    if (index == text.size()) {
        return std::nullopt;
    }
    char* end = nullptr;
    const double value = std::strtod(text.c_str() + index, &end);
    if (end == text.c_str() + index || !std::isfinite(value)) {
        return std::nullopt;
    }
    while (*end != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*end))) {
            return std::nullopt;
        }
        ++end;
    }
    return value;
}

std::vector<double> midranks(const std::vector<double>& values)
{
    std::vector<double> ranks(values.size(), 0.0);
    std::vector<std::size_t> order;
    order.reserve(values.size());
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (std::isfinite(values[index])) {
            order.push_back(index);
        }
    }
    std::sort(order.begin(), order.end(), [&](std::size_t left, std::size_t right) {
        return values[left] < values[right];
    });
    std::size_t start = 0;
    while (start < order.size()) {
        std::size_t end = start + 1;
        while (end < order.size()
               && values[order[end]] == values[order[start]]) {
            ++end;
        }
        const double rank = 0.5 * static_cast<double>(start + end + 1);
        for (std::size_t index = start; index < end; ++index) {
            ranks[order[index]] = rank;
        }
        start = end;
    }
    return ranks;
}

double tie_correction(const std::vector<double>& values)
{
    std::vector<double> sorted;
    sorted.reserve(values.size());
    for (const double value : values) {
        if (std::isfinite(value)) {
            sorted.push_back(value);
        }
    }
    std::sort(sorted.begin(), sorted.end());
    double total = 0.0;
    std::size_t start = 0;
    while (start < sorted.size()) {
        std::size_t end = start + 1;
        while (end < sorted.size() && sorted[end] == sorted[start]) {
            ++end;
        }
        const double tied = static_cast<double>(end - start);
        total += tied * tied * tied - tied;
        start = end;
    }
    return total;
}

KendallConcordanceEstimate kendall_w_from_ranks(
    const std::vector<std::vector<double>>& rater_scores)
{
    KendallConcordanceEstimate result;
    if (rater_scores.size() < 2) {
        result.not_computed_reason = "insufficient_raters";
        return result;
    }
    const std::size_t k = rater_scores.size();
    const std::size_t n = rater_scores.front().size();
    if (n < 2) {
        result.not_computed_reason = "insufficient_subjects";
        return result;
    }
    for (const auto& row : rater_scores) {
        if (row.size() != n) {
            result.not_computed_reason = "unbalanced_ranks";
            return result;
        }
    }
    std::vector<double> rank_sums(n, 0.0);
    double tie_sum = 0.0;
    for (const auto& scores : rater_scores) {
        const std::vector<double> ranks = midranks(scores);
        tie_sum += tie_correction(scores);
        for (std::size_t item = 0; item < n; ++item) {
            rank_sums[item] += ranks[item];
        }
    }
    const double mean_rank = static_cast<double>(k) * (static_cast<double>(n) + 1.0) / 2.0;
    double s = 0.0;
    for (const double sum : rank_sums) {
        const double delta = sum - mean_rank;
        s += delta * delta;
    }
    const double n_double = static_cast<double>(n);
    const double k_double = static_cast<double>(k);
    const double denominator = k_double * k_double * (n_double * n_double * n_double - n_double)
        - k_double * tie_sum;
    result.subject_count = n;
    result.rater_count = k;
    if (!(denominator > 0.0) || !std::isfinite(s)) {
        result.not_computed_reason = "not_estimable";
        return result;
    }
    result.coefficient = 12.0 * s / denominator;
    result.coefficient = std::clamp(result.coefficient, 0.0, 1.0);
    result.degrees_of_freedom = n_double - 1.0;
    result.chi_square = k_double * result.degrees_of_freedom * result.coefficient;
    result.p_value = chi_square_right_tail(result.chi_square, result.degrees_of_freedom);
    result.identifiable = std::isfinite(result.coefficient)
        && std::isfinite(result.p_value);
    return result;
}

KendallCorrelationEstimate kendall_tau_b(
    const std::vector<double>& x_values,
    const std::vector<double>& y_values)
{
    KendallCorrelationEstimate result;
    if (x_values.size() != y_values.size()) {
        result.not_computed_reason = "mismatched_pairs";
        return result;
    }
    std::vector<double> x_clean;
    std::vector<double> y_clean;
    for (std::size_t index = 0; index < x_values.size(); ++index) {
        if (std::isfinite(x_values[index]) && std::isfinite(y_values[index])) {
            x_clean.push_back(x_values[index]);
            y_clean.push_back(y_values[index]);
        }
    }
    const std::size_t n = x_clean.size();
    result.pair_count = n;
    if (n < 2) {
        result.not_computed_reason = "insufficient_pairs";
        return result;
    }
    std::vector<double> x_levels = x_clean;
    std::vector<double> y_levels = y_clean;
    std::sort(x_levels.begin(), x_levels.end());
    std::sort(y_levels.begin(), y_levels.end());
    x_levels.erase(std::unique(x_levels.begin(), x_levels.end()), x_levels.end());
    y_levels.erase(std::unique(y_levels.begin(), y_levels.end()), y_levels.end());
    const std::size_t rows = x_levels.size();
    const std::size_t columns = y_levels.size();
    std::vector<std::vector<double>> counts(rows, std::vector<double>(columns, 0.0));
    const auto index_of = [](const std::vector<double>& levels, double value) {
        return static_cast<std::size_t>(
            std::lower_bound(levels.cbegin(), levels.cend(), value) - levels.cbegin());
    };
    for (std::size_t index = 0; index < n; ++index) {
        counts[index_of(x_levels, x_clean[index])]
              [index_of(y_levels, y_clean[index])] += 1.0;
    }
    double concordant = 0.0;
    double discordant = 0.0;
    for (std::size_t i = 0; i < rows; ++i) {
        for (std::size_t j = 0; j < columns; ++j) {
            const double cell = counts[i][j];
            if (!(cell > 0.0)) {
                continue;
            }
            double higher = 0.0;
            double lower = 0.0;
            for (std::size_t i2 = i + 1; i2 < rows; ++i2) {
                for (std::size_t j2 = 0; j2 < columns; ++j2) {
                    if (j2 > j) {
                        higher += counts[i2][j2];
                    } else if (j2 < j) {
                        lower += counts[i2][j2];
                    }
                }
            }
            concordant += cell * higher;
            discordant += cell * lower;
        }
    }
    std::vector<double> row_totals(rows, 0.0);
    std::vector<double> column_totals(columns, 0.0);
    for (std::size_t i = 0; i < rows; ++i) {
        for (std::size_t j = 0; j < columns; ++j) {
            row_totals[i] += counts[i][j];
            column_totals[j] += counts[i][j];
        }
    }
    double tx = 0.0;
    double ty = 0.0;
    for (const double total : row_totals) {
        tx += 0.5 * total * (total - 1.0);
    }
    for (const double total : column_totals) {
        ty += 0.5 * total * (total - 1.0);
    }
    const double n0 = 0.5 * static_cast<double>(n) * static_cast<double>(n - 1);
    const double denominator = std::sqrt(std::max(0.0, (n0 - tx) * (n0 - ty)));
    if (!(denominator > 0.0)) {
        result.not_computed_reason = "not_estimable";
        return result;
    }
    result.tau = (concordant - discordant) / denominator;
    result.tau = std::clamp(result.tau, -1.0, 1.0);
    const double n_double = static_cast<double>(n);
    result.standard_error = std::sqrt(
        2.0 * (2.0 * n_double + 5.0) / (9.0 * n_double * (n_double - 1.0)));
    if (result.standard_error > 0.0) {
        result.z = result.tau / result.standard_error;
        result.p_value = 2.0 * (1.0 - standard_normal_cdf(std::abs(result.z)));
        result.p_value = std::clamp(result.p_value, 0.0, 1.0);
    }
    result.identifiable = std::isfinite(result.tau);
    return result;
}

void append_kendall(
    AttributeAgreementResult& result,
    const std::vector<std::vector<std::vector<std::string>>>& matrix,
    const std::vector<std::string>& evaluator_names,
    const std::vector<std::string>& standard_by_item,
    bool has_standard)
{
    auto numeric_or_missing = [](const std::string& text) -> std::optional<double> {
        if (text.empty()) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return parse_finite_number(text);
    };
    std::vector<double> unique_levels;
    bool unranked = false;
    const auto collect = [&](const std::string& text) {
        if (text.empty()) {
            return;
        }
        const auto parsed = parse_finite_number(text);
        if (!parsed.has_value()) {
            unranked = true;
            return;
        }
        unique_levels.push_back(*parsed);
    };
    for (const auto& row : matrix) {
        for (const auto& cell : row) {
            for (const std::string& rating : cell) {
                collect(rating);
            }
        }
    }
    if (has_standard) {
        for (const std::string& standard : standard_by_item) {
            collect(standard);
        }
    }
    if (unranked) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "ordinal_ratings_unranked",
                       "有序评级必须全部为数值，才能计算 Kendall；未使用字典序。");
        return;
    }
    std::sort(unique_levels.begin(), unique_levels.end());
    unique_levels.erase(std::unique(unique_levels.begin(), unique_levels.end()),
                        unique_levels.end());
    if (unique_levels.size() < 3) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "kendall_requires_three_ordinal_levels",
                       "Kendall 系数需要至少三个有序等级；当前只输出 Kappa。");
        return;
    }

    std::vector<std::vector<double>> between_scores(result.evaluator_count);
    for (std::size_t item = 0; item < result.item_count; ++item) {
        bool complete = true;
        std::vector<double> scores(result.evaluator_count);
        for (std::size_t evaluator = 0; evaluator < result.evaluator_count;
             ++evaluator) {
            if (matrix[item][evaluator].empty()
                || matrix[item][evaluator].front().empty()) {
                complete = false;
                break;
            }
            const auto parsed = numeric_or_missing(matrix[item][evaluator].front());
            if (!parsed.has_value()) {
                complete = false;
                break;
            }
            scores[evaluator] = *parsed;
        }
        if (!complete) {
            continue;
        }
        for (std::size_t evaluator = 0; evaluator < result.evaluator_count;
             ++evaluator) {
            between_scores[evaluator].push_back(scores[evaluator]);
        }
    }
    result.between_kendall = kendall_w_from_ranks(between_scores);
    if (!result.between_kendall->identifiable) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "kendall_w_not_estimable",
                       "评估者间 Kendall W 不可识别；不伪造 W=1。");
    }

    for (std::size_t evaluator = 0; evaluator < result.evaluator_count; ++evaluator) {
        std::size_t trial_count = 0;
        bool uniform = true;
        for (std::size_t item = 0; item < result.item_count; ++item) {
            const std::size_t size = matrix[item][evaluator].size();
            if (trial_count == 0) {
                trial_count = size;
            } else if (size != trial_count) {
                uniform = false;
            }
        }
        if (uniform && trial_count >= 2) {
            std::vector<std::vector<double>> trial_scores(trial_count);
            bool usable = true;
            for (std::size_t item = 0; item < result.item_count; ++item) {
                for (std::size_t trial = 0; trial < trial_count; ++trial) {
                    const auto parsed =
                        numeric_or_missing(matrix[item][evaluator][trial]);
                    if (!parsed.has_value() || !std::isfinite(*parsed)) {
                        usable = false;
                        break;
                    }
                    trial_scores[trial].push_back(*parsed);
                }
                if (!usable) {
                    break;
                }
            }
            if (usable) {
                result.within_kendall.push_back(
                    {evaluator_names[evaluator],
                     kendall_w_from_ranks(trial_scores)});
            }
        }
        if (has_standard) {
            std::vector<double> ratings;
            std::vector<double> standards;
            for (std::size_t item = 0; item < result.item_count; ++item) {
                const auto standard = numeric_or_missing(standard_by_item[item]);
                if (!standard.has_value() || !std::isfinite(*standard)) {
                    continue;
                }
                for (const std::string& rating : matrix[item][evaluator]) {
                    const auto parsed = numeric_or_missing(rating);
                    if (!parsed.has_value() || !std::isfinite(*parsed)) {
                        continue;
                    }
                    ratings.push_back(*parsed);
                    standards.push_back(*standard);
                }
            }
            result.against_standard_kendall.push_back(
                {evaluator_names[evaluator], kendall_tau_b(ratings, standards)});
        }
    }
    if (has_standard && !result.against_standard_kendall.empty()) {
        KendallCorrelationEstimate overall;
        double tau_sum = 0.0;
        std::size_t identifiable_count = 0;
        std::size_t pair_count = 0;
        for (const auto& row : result.against_standard_kendall) {
            if (!row.estimate.identifiable) {
                continue;
            }
            tau_sum += row.estimate.tau;
            pair_count += row.estimate.pair_count;
            ++identifiable_count;
        }
        if (identifiable_count == 0) {
            overall.not_computed_reason = "not_estimable";
        } else {
            overall.tau = tau_sum / static_cast<double>(identifiable_count);
            overall.pair_count = pair_count;
            const double n = static_cast<double>(
                std::max(result.item_count, std::size_t{2}));
            overall.standard_error = std::sqrt(
                2.0 * (2.0 * n + 5.0)
                / (9.0 * n * (n - 1.0) * static_cast<double>(identifiable_count)));
            if (overall.standard_error > 0.0) {
                overall.z = overall.tau / overall.standard_error;
                overall.p_value =
                    2.0 * (1.0 - standard_normal_cdf(std::abs(overall.z)));
                overall.p_value = std::clamp(overall.p_value, 0.0, 1.0);
            }
            overall.identifiable = true;
        }
        result.overall_kendall = overall;
    }
}

}  // namespace

AttributeAgreementResult attribute_agreement(
    const std::vector<std::string>& ratings,
    const std::vector<std::string>& items,
    const std::vector<std::string>& evaluators,
    const std::vector<std::string>& standards,
    double confidence_level,
    bool ratings_are_ordinal)
{
    AttributeAgreementResult result;
    result.confidence_level = confidence_level;
    result.ratings_are_ordinal = ratings_are_ordinal;
    if (ratings.empty() || ratings.size() != items.size()
        || ratings.size() != evaluators.size() || confidence_level <= 0.0
        || confidence_level >= 1.0
        || (!standards.empty() && standards.size() != items.size())) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_attribute_agreement_shape",
                       "属性一致性要求列长度一致，标准列为空或与记录数一致，置信度必须在 0 与 1 之间。");
        return result;
    }
    std::map<std::string, std::size_t> item_index;
    std::map<std::string, std::size_t> evaluator_index;
    std::vector<std::string> item_names;
    std::vector<std::string> evaluator_names;
    for (std::size_t i = 0; i < ratings.size(); ++i) {
        if (items[i].empty() || evaluators[i].empty()) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_attribute_agreement_row",
                           "项目和评估者标签不能为空。");
            return result;
        }
        if (item_index.emplace(items[i], item_index.size()).second) {
            item_names.push_back(items[i]);
        }
        if (evaluator_index.emplace(evaluators[i], evaluator_index.size()).second) {
            evaluator_names.push_back(evaluators[i]);
        }
        if (ratings[i].empty()) {
            ++result.missing_rating_count;
        }
    }
    result.item_count = item_names.size();
    result.evaluator_count = evaluator_names.size();
    result.rating_count = ratings.size();
    if (result.item_count < 2 || result.evaluator_count < 2) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_attribute_agreement_levels",
                       "属性一致性至少需要两个项目和两个评估者。");
        return result;
    }

    std::vector<std::vector<std::vector<std::string>>> matrix(
        result.item_count,
        std::vector<std::vector<std::string>>(result.evaluator_count));
    std::vector<std::string> standard_by_item(result.item_count);
    for (std::size_t i = 0; i < ratings.size(); ++i) {
        const std::size_t item = item_index.at(items[i]);
        const std::size_t evaluator = evaluator_index.at(evaluators[i]);
        matrix[item][evaluator].push_back(ratings[i]);
        if (!standards.empty()) {
            standard_by_item[item] = standards[i];
        }
    }
    if (result.missing_rating_count > 0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "missing_attribute_ratings",
                       "存在缺失评级；相关配对只使用双方均有评级的项目。");
    }
    bool has_missing_cell = false;
    for (const auto& row : matrix) {
        for (const auto& cell : row) {
            if (cell.empty()) {
                has_missing_cell = true;
            }
        }
    }
    if (has_missing_cell) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "incomplete_attribute_matrix",
                       "部分项目未被某些评估者评级，评估者间比较将排除这些项目。");
    }
    if (!standards.empty()
        && std::any_of(standard_by_item.cbegin(), standard_by_item.cend(),
                       [](const std::string& value) { return value.empty(); })) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "missing_attribute_standard",
                       "部分项目缺少标准评级，相关评估者-标准比较将排除这些项目。");
    }
    bool unbalanced_replicates = false;
    for (const auto& row : matrix) {
        const std::size_t first_size = row.front().size();
        for (const auto& cell : row) {
            if (cell.size() != first_size) {
                unbalanced_replicates = true;
            }
        }
    }
    if (unbalanced_replicates) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "unbalanced_replicates",
                       "评估者重复次数不一致；不等长配对已排除，不会静默截断。");
    }

    for (std::size_t evaluator = 0; evaluator < result.evaluator_count; ++evaluator) {
        std::size_t trial_count = 0;
        bool uniform_trials = true;
        std::vector<std::vector<std::string>> item_trials;
        std::vector<std::pair<std::string, std::string>> pairs;
        for (const auto& row : matrix) {
            const auto& ratings_for_item = row[evaluator];
            if (trial_count == 0) {
                trial_count = ratings_for_item.size();
            } else if (ratings_for_item.size() != trial_count) {
                uniform_trials = false;
            }
            item_trials.push_back(ratings_for_item);
            for (std::size_t repeat = 1; repeat < ratings_for_item.size();
                 ++repeat) {
                pairs.emplace_back(ratings_for_item[repeat - 1],
                                   ratings_for_item[repeat]);
            }
        }
        AgreementEstimate within_estimate = (uniform_trials && trial_count >= 3)
            ? fleiss_from_item_ratings(item_trials, confidence_level)
            : estimate(pairs, confidence_level);
        result.within_evaluator.push_back(
            {evaluator_names[evaluator], std::move(within_estimate)});
        if (!standards.empty()) {
            std::vector<std::pair<std::string, std::string>> standard_pairs;
            for (std::size_t item = 0; item < result.item_count; ++item) {
                for (const std::string& rating : matrix[item][evaluator]) {
                    standard_pairs.emplace_back(rating, standard_by_item[item]);
                }
            }
            result.against_standard.push_back(
                {evaluator_names[evaluator],
                 estimate(standard_pairs, confidence_level)});
        }
    }
    for (std::size_t first = 0; first < result.evaluator_count; ++first) {
        for (std::size_t second = first + 1;
             second < result.evaluator_count; ++second) {
            std::vector<std::pair<std::string, std::string>> pairs;
            for (const auto& row : matrix) {
                const auto& first_ratings = row[first];
                const auto& second_ratings = row[second];
                if (first_ratings.size() != second_ratings.size()) {
                    continue;
                }
                for (std::size_t repeat = 0; repeat < first_ratings.size(); ++repeat) {
                    pairs.emplace_back(first_ratings[repeat],
                                       second_ratings[repeat]);
                }
            }
            result.between_evaluator.push_back(
                {evaluator_names[first], evaluator_names[second],
                 estimate(pairs, confidence_level)});
        }
    }
    if (result.evaluator_count >= 3) {
        std::vector<std::vector<std::string>> item_ratings;
        for (std::size_t item = 0; item < result.item_count; ++item) {
            std::vector<std::string> ratings;
            ratings.reserve(result.evaluator_count);
            bool complete = true;
            for (std::size_t evaluator = 0; evaluator < result.evaluator_count;
                 ++evaluator) {
                if (matrix[item][evaluator].empty()
                    || matrix[item][evaluator].front().empty()) {
                    complete = false;
                    break;
                }
                ratings.push_back(matrix[item][evaluator].front());
            }
            if (complete) {
                item_ratings.push_back(std::move(ratings));
            }
        }
        result.overall = fleiss_from_item_ratings(item_ratings, confidence_level);
        result.overall_available = result.overall.valid_count > 0;
    }
    auto mark_unidentifiable = [&](const AgreementEstimate& estimate) {
        if (!estimate.identifiable) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                           "not_estimable",
                           "期望一致率 P_expected=1，Kappa 不可识别，不计算无限标准误。");
        }
    };
    for (const auto& row : result.within_evaluator) {
        mark_unidentifiable(row.estimate);
    }
    for (const auto& row : result.between_evaluator) {
        mark_unidentifiable(row.estimate);
    }
    for (const auto& row : result.against_standard) {
        mark_unidentifiable(row.estimate);
    }
    if (result.overall_available) {
        mark_unidentifiable(result.overall);
    }
    if (ratings_are_ordinal) {
        append_kendall(result, matrix, evaluator_names, standard_by_item,
                       !standards.empty());
    }
    return result;
}

}  // namespace datalab::domain::statistics
