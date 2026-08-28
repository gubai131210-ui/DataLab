#include "domain/statistics/analyze_variability.h"

#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace datalab::domain::statistics {
namespace {

using Matrix = std::vector<std::vector<double>>;

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const std::string& message)
{
    diagnostics.push_back({severity, std::string{code}, message, {}});
}

double sample_std_dev(const std::vector<double>& values)
{
    if (values.size() < 2) {
        return 0.0;
    }
    const double mean =
        std::accumulate(values.begin(), values.end(), 0.0)
        / static_cast<double>(values.size());
    double sum_sq = 0.0;
    for (double value : values) {
        const double d = value - mean;
        sum_sq += d * d;
    }
    return std::sqrt(sum_sq / static_cast<double>(values.size() - 1));
}

// Map factor level to ±1 coding (first level = -1, second = +1).
double encode_level(const std::string& level, const std::set<std::string>& levels)
{
    if (levels.size() < 2) {
        return 0.0;
    }
    auto it = levels.cbegin();
    const std::string low = *it;
    ++it;
    const std::string high = *it;
    if (level == low) {
        return -1.0;
    }
    if (level == high) {
        return 1.0;
    }
    return 0.0;
}

std::vector<double> fit_lse_no_intercept(
    const Matrix& design, const std::vector<double>& response, std::size_t& rank)
{
    const std::size_t n = response.size();
    const std::size_t p = design.empty() ? 0 : design.front().size();
    std::vector<double> coefficients(p, 0.0);
    rank = 0;
    if (p == 0 || n == 0) {
        return coefficients;
    }
    Matrix xtx(p, std::vector<double>(p, 0.0));
    std::vector<double> xty(p, 0.0);
    for (std::size_t row = 0; row < n; ++row) {
        for (std::size_t i = 0; i < p; ++i) {
            xty[i] += design[row][i] * response[row];
            for (std::size_t j = 0; j < p; ++j) {
                xtx[i][j] += design[row][i] * design[row][j];
            }
        }
    }
    Matrix augmented = xtx;
    for (std::size_t i = 0; i < p; ++i) {
        augmented[i].push_back(xty[i]);
    }
    for (std::size_t pivot = 0; pivot < p; ++pivot) {
        std::size_t best = pivot;
        for (std::size_t row = pivot + 1; row < p; ++row) {
            if (std::abs(augmented[row][pivot]) > std::abs(augmented[best][pivot])) {
                best = row;
            }
        }
        if (std::abs(augmented[best][pivot]) < 1.0e-12) {
            continue;
        }
        if (best != pivot) {
            std::swap(augmented[pivot], augmented[best]);
        }
        const double divisor = augmented[pivot][pivot];
        for (std::size_t col = pivot; col <= p; ++col) {
            augmented[pivot][col] /= divisor;
        }
        for (std::size_t row = 0; row < p; ++row) {
            if (row == pivot) {
                continue;
            }
            const double factor = augmented[row][pivot];
            for (std::size_t col = pivot; col <= p; ++col) {
                augmented[row][col] -= factor * augmented[pivot][col];
            }
        }
        ++rank;
    }
    for (std::size_t i = 0; i < p; ++i) {
        coefficients[i] = augmented[i][p];
    }
    return coefficients;
}

}  // namespace

AnalyzeVariabilityResult analyze_variability_dispersion(
    const std::vector<std::vector<std::string>>& factor_levels,
    const std::vector<std::vector<double>>& replicates,
    const std::vector<std::string>& factor_names,
    const std::vector<std::size_t>& source_rows,
    const AnalyzeVariabilityOptions& options)
{
    AnalyzeVariabilityResult result;
    result.factor_names = factor_names;
    result.factor_count = factor_names.size();
    result.estimation_method = options.estimation_method;
    result.run_count = factor_levels.size();
    result.replicate_count = replicates.empty() ? 0 : replicates.front().size();

    if (factor_levels.empty() || replicates.empty()
        || factor_levels.size() != replicates.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_variability_data", "需要因子水平与重复列对齐。");
        return result;
    }
    if (result.replicate_count < 2) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_replicates",
                       "Analyze Variability 需要每运行至少 2 个重复。");
        return result;
    }

    std::vector<std::set<std::string>> level_sets(factor_names.size());
    for (std::size_t f = 0; f < factor_names.size(); ++f) {
        for (const auto& row : factor_levels) {
            if (f < row.size() && !row[f].empty()) {
                level_sets[f].insert(row[f]);
            }
        }
        if (level_sets[f].size() != 2) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "non_two_level_factor",
                           "因子 " + factor_names[f] + " 必须为 2 水平。");
            return result;
        }
    }

    std::vector<double> log_std;
    Matrix design;
    design.reserve(factor_levels.size());
    for (std::size_t i = 0; i < factor_levels.size(); ++i) {
        const double s = sample_std_dev(replicates[i]);
        if (!(s > 0.0) || !std::isfinite(s)) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                           "non_positive_std_dev",
                           "运行 " + std::to_string(i + 1) + " 标准差非正，已跳过。");
            continue;
        }
        VariabilityRunRow run_row;
        run_row.source_row = i < source_rows.size() ? source_rows[i] : i;
        run_row.std_dev = s;
        run_row.log_std_dev = std::log(s);
        run_row.replicate_count = replicates[i].size();
        result.runs.push_back(run_row);
        log_std.push_back(run_row.log_std_dev);

        std::vector<double> coded(factor_names.size(), 0.0);
        for (std::size_t f = 0; f < factor_names.size(); ++f) {
            coded[f] = factor_levels[i].size() > f
                ? encode_level(factor_levels[i][f], level_sets[f]) : 0.0;
        }
        design.push_back(std::move(coded));
    }
    result.run_count = result.runs.size();
    if (result.run_count < static_cast<std::size_t>(factor_names.size()) + 1) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_variability_runs",
                       "有效运行数不足以拟合分散模型。");
        return result;
    }

    std::size_t rank = 0;
    const std::vector<double> coefficients =
        fit_lse_no_intercept(design, log_std, rank);
    std::vector<double> fitted(log_std.size(), 0.0);
    double rss = 0.0;
    for (std::size_t row = 0; row < design.size(); ++row) {
        for (std::size_t col = 0; col < coefficients.size(); ++col) {
            fitted[row] += design[row][col] * coefficients[col];
        }
        const double residual = log_std[row] - fitted[row];
        rss += residual * residual;
    }
    const std::size_t error_df = design.size() > rank ? design.size() - rank : 0;
    const double mse = error_df > 0 ? rss / static_cast<double>(error_df) : 0.0;

    for (std::size_t f = 0; f < factor_names.size(); ++f) {
        DispersionCoefficient coef;
        coef.term = factor_names[f];
        coef.coefficient = f < coefficients.size() ? coefficients[f] : 0.0;
        coef.effect = 2.0 * coef.coefficient;
        if (error_df > 0 && mse > 0.0) {
            coef.standard_error = std::sqrt(mse);
        }
        result.coefficients.push_back(coef);

        DispersionAnovaEffect effect;
        effect.term = factor_names[f];
        effect.degrees_of_freedom = 1;
        if (mse > 0.0 && error_df > 0) {
            effect.sum_of_squares = coef.coefficient * coef.coefficient;
            effect.mean_square = effect.sum_of_squares;
            effect.f_statistic = *effect.mean_square / mse;
            effect.p_value = f_right_tail(
                *effect.f_statistic, 1.0, static_cast<double>(error_df));
        }
        result.anova_effects.push_back(effect);
    }

    return result;
}

}  // namespace datalab::domain::statistics
