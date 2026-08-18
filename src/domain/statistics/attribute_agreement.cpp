#include "domain/statistics/attribute_agreement.h"

#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <utility>

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

}  // namespace

AttributeAgreementResult attribute_agreement(
    const std::vector<std::string>& ratings,
    const std::vector<std::string>& items,
    const std::vector<std::string>& evaluators,
    const std::vector<std::string>& standards,
    double confidence_level)
{
    AttributeAgreementResult result;
    result.confidence_level = confidence_level;
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
        std::vector<std::pair<std::string, std::string>> pairs;
        for (const auto& row : matrix) {
            const auto& ratings_for_item = row[evaluator];
            for (std::size_t repeat = 1; repeat < ratings_for_item.size();
                 ++repeat) {
                pairs.emplace_back(ratings_for_item[repeat - 1],
                                   ratings_for_item[repeat]);
            }
        }
        result.within_evaluator.push_back(
            {evaluator_names[evaluator], estimate(pairs, confidence_level)});
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
    return result;
}

}  // namespace datalab::domain::statistics
