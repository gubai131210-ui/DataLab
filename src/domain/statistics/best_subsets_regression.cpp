#include "domain/statistics/best_subsets_regression.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <vector>

namespace datalab::domain::statistics {
namespace {

constexpr std::size_t kMaxCandidatePredictors = 15;

std::vector<std::vector<double>> subset_predictors(
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::size_t>& indices)
{
    std::vector<std::vector<double>> out;
    out.reserve(predictors.size());
    for (const auto& row : predictors) {
        std::vector<double> selected;
        selected.reserve(indices.size());
        for (std::size_t index : indices) {
            selected.push_back(row[index]);
        }
        out.push_back(std::move(selected));
    }
    return out;
}

std::vector<std::size_t> indices_from_mask(std::size_t mask, std::size_t candidate_count)
{
    std::vector<std::size_t> indices;
    for (std::size_t index = 0; index < candidate_count; ++index) {
        if ((mask >> index) & 1U) {
            indices.push_back(index);
        }
    }
    return indices;
}

double compute_mallows_cp(
    double sse, double full_mse, std::size_t n, std::size_t predictor_count)
{
    if (!(full_mse > 0.0) || n <= predictor_count + 1) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double p_terms = static_cast<double>(predictor_count + 1);
    return sse / full_mse - static_cast<double>(n) + 2.0 * p_terms;
}

}  // namespace

BestSubsetsRegressionResult fit_best_subsets_regression(
    const std::vector<double>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& predictor_labels,
    std::size_t min_predictors,
    std::size_t max_predictors,
    std::size_t models_per_size,
    double confidence_level,
    const std::vector<std::size_t>& source_rows)
{
    BestSubsetsRegressionResult result;
    result.min_predictors = min_predictors;
    result.models_per_size = std::max<std::size_t>(1, std::min<std::size_t>(models_per_size, 5));
    if (response.size() < 4 || predictors.size() != response.size()
        || predictors.empty() || predictors.front().size() < 1
        || predictor_labels.size() != predictors.front().size()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "best_subsets_invalid",
            "Best Subsets 需要 ≥4 观测与 ≥1 候选预测变量。"});
        return result;
    }
    const std::size_t p = predictors.front().size();
    if (p > kMaxCandidatePredictors) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "best_subsets_too_many",
            "候选预测变量超过 15；请减少变量或改用逐步回归。"});
        return result;
    }
    if (min_predictors == 0) {
        min_predictors = 1;
    }
    if (max_predictors == 0 || max_predictors > p) {
        max_predictors = p;
    }
    if (min_predictors > max_predictors) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "best_subsets_range",
            "最小预测变量数不能大于最大预测变量数。"});
        return result;
    }
    if (max_predictors + 1 >= response.size()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "best_subsets_df",
            "观测数不足以拟合最大规模模型。"});
        return result;
    }

    result.observation_count = response.size();
    result.candidate_count = p;
    result.max_predictors = max_predictors;

    std::vector<std::size_t> all_indices(p);
    for (std::size_t i = 0; i < p; ++i) {
        all_indices[i] = i;
    }
    const RegressionResult full_model = fit_linear_regression(
        response, predictors, predictor_labels, confidence_level, source_rows);
    const double full_mse = full_model.error_mean_square;
    result.full_model_mse = full_mse;

    std::vector<std::vector<BestSubsetsModelSummary>> by_size(max_predictors + 1);
    const std::size_t subset_count = (1U << p) - 1U;
    for (std::size_t mask = 1; mask <= subset_count; ++mask) {
        const std::vector<std::size_t> indices = indices_from_mask(mask, p);
        if (indices.size() < min_predictors || indices.size() > max_predictors) {
            continue;
        }
        if (indices.size() + 1 >= response.size()) {
            continue;
        }
        std::vector<std::string> labels;
        for (std::size_t index : indices) {
            labels.push_back(predictor_labels[index]);
        }
        const RegressionResult fit = fit_linear_regression(
            response, subset_predictors(predictors, indices), labels,
            confidence_level, source_rows);
        BestSubsetsModelSummary summary;
        summary.predictor_count = indices.size();
        summary.predictors_in_model.assign(p, false);
        summary.term_labels = labels;
        for (std::size_t index : indices) {
            summary.predictors_in_model[index] = true;
        }
        summary.r_squared = fit.r_squared;
        summary.adjusted_r_squared = fit.adjusted_r_squared;
        summary.s = fit.residual_standard_deviation;
        summary.mallows_cp = compute_mallows_cp(
            fit.error_sum_of_squares, full_mse, response.size(), indices.size());
        by_size[indices.size()].push_back(summary);
    }

    for (std::size_t size = min_predictors; size <= max_predictors; ++size) {
        auto& bucket = by_size[size];
        std::sort(bucket.begin(), bucket.end(),
                  [](const BestSubsetsModelSummary& left,
                     const BestSubsetsModelSummary& right) {
                      if (left.r_squared != right.r_squared) {
                          return left.r_squared > right.r_squared;
                      }
                      return left.adjusted_r_squared > right.adjusted_r_squared;
                  });
        const std::size_t keep = std::min(result.models_per_size, bucket.size());
        for (std::size_t index = 0; index < keep; ++index) {
            result.model_summaries.push_back(bucket[index]);
        }
    }

    if (result.model_summaries.empty()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "best_subsets_empty",
            "未找到满足条件的子集模型。"});
        return result;
    }

    result.best_overall = *std::max_element(
        result.model_summaries.begin(), result.model_summaries.end(),
        [](const BestSubsetsModelSummary& left, const BestSubsetsModelSummary& right) {
            return left.r_squared < right.r_squared;
        });
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "best_subsets_scope",
        "子集枚举（非 Hamiltonian Walk）；formula_reference；非 Minitab golden。"});
    return result;
}

}  // namespace datalab::domain::statistics
