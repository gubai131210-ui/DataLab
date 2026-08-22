#include "domain/statistics/discriminant.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

bool invert_matrix(
    std::vector<std::vector<double>> matrix,
    std::vector<std::vector<double>>& inverse)
{
    const std::size_t n = matrix.size();
    if (n == 0) {
        return false;
    }
    inverse.assign(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        inverse[i][i] = 1.0;
    }
    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivot = col;
        for (std::size_t row = col + 1; row < n; ++row) {
            if (std::abs(matrix[row][col]) > std::abs(matrix[pivot][col])) {
                pivot = row;
            }
        }
        if (std::abs(matrix[pivot][col]) < 1.0e-12) {
            return false;
        }
        std::swap(matrix[pivot], matrix[col]);
        std::swap(inverse[pivot], inverse[col]);
        const double div = matrix[col][col];
        for (std::size_t j = 0; j < n; ++j) {
            matrix[col][j] /= div;
            inverse[col][j] /= div;
        }
        for (std::size_t row = 0; row < n; ++row) {
            if (row == col) {
                continue;
            }
            const double factor = matrix[row][col];
            for (std::size_t j = 0; j < n; ++j) {
                matrix[row][j] -= factor * matrix[col][j];
                inverse[row][j] -= factor * inverse[col][j];
            }
        }
    }
    return true;
}

double quadratic_form(
    const std::vector<double>& left,
    const std::vector<std::vector<double>>& matrix,
    const std::vector<double>& right)
{
    double sum = 0.0;
    for (std::size_t i = 0; i < left.size(); ++i) {
        double inner = 0.0;
        for (std::size_t j = 0; j < right.size(); ++j) {
            inner += matrix[i][j] * right[j];
        }
        sum += left[i] * inner;
    }
    return sum;
}

}  // namespace

DiscriminantResult linear_discriminant(
    const std::vector<std::size_t>& class_index,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& class_labels)
{
    DiscriminantResult result;
    result.class_labels = class_labels;
    if (class_index.size() < 4 || predictors.size() != class_index.size()
        || predictors.empty() || predictors.front().empty()
        || class_labels.size() < 2) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "lda_invalid",
            "线性判别需要 ≥2 类、≥1 预测变量与足够观测。"});
        return result;
    }
    const std::size_t p = predictors.front().size();
    const std::size_t c = class_labels.size();
    result.predictor_count = p;
    result.class_count = c;
    result.observation_count = class_index.size();
    result.class_sizes.assign(c, 0);
    result.class_means.assign(c, std::vector<double>(p, 0.0));
    for (std::size_t i = 0; i < class_index.size(); ++i) {
        const std::size_t cls = class_index[i];
        if (cls >= c || predictors[i].size() != p) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "lda_bad_row",
                "类别编码或预测行列不一致。"});
            return result;
        }
        ++result.class_sizes[cls];
        for (std::size_t j = 0; j < p; ++j) {
            result.class_means[cls][j] += predictors[i][j];
        }
    }
    for (std::size_t cls = 0; cls < c; ++cls) {
        if (result.class_sizes[cls] < 2) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "lda_small_class",
                "每个类至少需要 2 个观测。"});
            return result;
        }
        for (std::size_t j = 0; j < p; ++j) {
            result.class_means[cls][j] /=
                static_cast<double>(result.class_sizes[cls]);
        }
    }

    std::vector<std::vector<double>> pooled(p, std::vector<double>(p, 0.0));
    for (std::size_t i = 0; i < class_index.size(); ++i) {
        const std::size_t cls = class_index[i];
        for (std::size_t a = 0; a < p; ++a) {
            const double da = predictors[i][a] - result.class_means[cls][a];
            for (std::size_t b = 0; b < p; ++b) {
                const double db = predictors[i][b] - result.class_means[cls][b];
                pooled[a][b] += da * db;
            }
        }
    }
    const double denom =
        static_cast<double>(class_index.size() - c);
    if (!(denom > 0.0)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "lda_df",
            "合并协方差自由度不足。"});
        return result;
    }
    for (std::size_t a = 0; a < p; ++a) {
        for (std::size_t b = 0; b < p; ++b) {
            pooled[a][b] /= denom;
        }
    }
    std::vector<std::vector<double>> inv;
    if (!invert_matrix(pooled, inv)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "lda_singular",
            "合并协方差奇异。"});
        return result;
    }

    result.predicted.assign(class_index.size(), 0);
    result.confusion.assign(c, std::vector<std::size_t>(c, 0));
    result.ld1.assign(class_index.size(), 0.0);
    result.ld2.assign(class_index.size(), 0.0);
    std::size_t correct = 0;
    // Fisher direction ≈ inv * (μ1 - μ0) for first two classes when available.
    std::vector<double> fisher(p, 0.0);
    if (c >= 2) {
        std::vector<double> diff(p, 0.0);
        for (std::size_t j = 0; j < p; ++j) {
            diff[j] = result.class_means[1][j] - result.class_means[0][j];
        }
        for (std::size_t a = 0; a < p; ++a) {
            for (std::size_t b = 0; b < p; ++b) {
                fisher[a] += inv[a][b] * diff[b];
            }
        }
    }
    for (std::size_t i = 0; i < class_index.size(); ++i) {
        std::size_t best = 0;
        double best_score = -std::numeric_limits<double>::infinity();
        for (std::size_t cls = 0; cls < c; ++cls) {
            const double prior = static_cast<double>(result.class_sizes[cls])
                / static_cast<double>(class_index.size());
            const double score =
                quadratic_form(predictors[i], inv, result.class_means[cls])
                - 0.5 * quadratic_form(
                    result.class_means[cls], inv, result.class_means[cls])
                + std::log(prior);
            if (score > best_score) {
                best_score = score;
                best = cls;
            }
        }
        result.predicted[i] = best;
        ++result.confusion[class_index[i]][best];
        if (best == class_index[i]) {
            ++correct;
        }
        for (std::size_t j = 0; j < p; ++j) {
            result.ld1[i] += fisher[j] * predictors[i][j];
        }
        if (p >= 2) {
            result.ld2[i] = predictors[i][1];
        }
    }
    result.train_accuracy =
        static_cast<double>(correct) / static_cast<double>(class_index.size());
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "lda_scope",
        "线性判别（等协方差）；非 Minitab golden；不做 QDA。"});
    return result;
}

}  // namespace datalab::domain::statistics
