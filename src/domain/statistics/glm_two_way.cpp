#include "domain/statistics/glm_two_way.h"

#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/normality_test.h"
#include "domain/statistics/two_factor_anova.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
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

struct Observation {
    std::string a;
    std::string b;
    double y = 0.0;
};

struct Fit {
    double rss = 0.0;
    std::size_t rank = 0;
    std::vector<double> coefficients;
    std::vector<double> fitted;
};

Fit fit_glm_model(const Matrix& matrix, const std::vector<double>& response)
{
    Fit result;
    if (matrix.empty() || matrix.front().empty()) {
        return result;
    }
    const std::size_t n = response.size();
    const std::size_t p = matrix.front().size();
    Matrix q;
    q.reserve(p);
    constexpr double tolerance = 1.0e-10;
    for (std::size_t column = 0; column < p; ++column) {
        std::vector<double> vector(n, 0.0);
        for (std::size_t row = 0; row < n; ++row) {
            vector[row] = matrix[row][column];
        }
        for (const auto& basis : q) {
            const double projection = std::inner_product(
                basis.cbegin(), basis.cend(), vector.cbegin(), 0.0);
            for (std::size_t row = 0; row < n; ++row) {
                vector[row] -= projection * basis[row];
            }
        }
        const double norm = std::sqrt(std::inner_product(
            vector.cbegin(), vector.cend(), vector.cbegin(), 0.0));
        if (norm > tolerance) {
            for (double& value : vector) {
                value /= norm;
            }
            q.push_back(std::move(vector));
        }
    }
    result.rank = q.size();
    result.fitted.assign(n, 0.0);
    for (const auto& basis : q) {
        const double projection = std::inner_product(
            basis.cbegin(), basis.cend(), response.cbegin(), 0.0);
        for (std::size_t row = 0; row < n; ++row) {
            result.fitted[row] += projection * basis[row];
        }
    }
    for (std::size_t row = 0; row < n; ++row) {
        const double residual = response[row] - result.fitted[row];
        result.rss += residual * residual;
    }
    return result;
}

Matrix make_glm_design(
    const std::vector<Observation>& observations,
    const std::vector<std::string>& levels_a,
    const std::vector<std::string>& levels_b,
    int model)
{
    const std::size_t a_columns = levels_a.size() - 1;
    const std::size_t b_columns = levels_b.size() - 1;
    std::size_t columns = 1;
    if ((model & 1) != 0) {
        columns += a_columns;
    }
    if ((model & 2) != 0) {
        columns += b_columns;
    }
    if ((model & 4) != 0) {
        columns += a_columns * b_columns;
    }
    Matrix matrix(observations.size(), std::vector<double>(columns, 0.0));
    for (std::size_t row = 0; row < observations.size(); ++row) {
        std::size_t column = 0;
        matrix[row][column++] = 1.0;
        std::vector<double> a(a_columns, 0.0);
        std::vector<double> b(b_columns, 0.0);
        for (std::size_t index = 0; index < a_columns; ++index) {
            if (observations[row].a == levels_a[index + 1]) {
                a[index] = 1.0;
            }
        }
        for (std::size_t index = 0; index < b_columns; ++index) {
            if (observations[row].b == levels_b[index + 1]) {
                b[index] = 1.0;
            }
        }
        if ((model & 1) != 0) {
            for (double value : a) {
                matrix[row][column++] = value;
            }
        }
        if ((model & 2) != 0) {
            for (double value : b) {
                matrix[row][column++] = value;
            }
        }
        if ((model & 4) != 0) {
            for (double value_a : a) {
                for (double value_b : b) {
                    matrix[row][column++] = value_a * value_b;
                }
            }
        }
    }
    return matrix;
}

void add_fitted_means(
    const std::vector<Observation>& observations,
    const std::vector<double>& fitted,
    const std::vector<std::string>& levels_a,
    const std::vector<std::string>& levels_b,
    std::vector<GlmFittedMean>& out)
{
    std::map<std::string, std::pair<double, std::size_t>> sums_a;
    std::map<std::string, std::pair<double, std::size_t>> sums_b;
    for (std::size_t i = 0; i < observations.size(); ++i) {
        auto& entry_a = sums_a[observations[i].a];
        entry_a.first += fitted[i];
        ++entry_a.second;
        auto& entry_b = sums_b[observations[i].b];
        entry_b.first += fitted[i];
        ++entry_b.second;
    }
    for (const auto& level : levels_a) {
        GlmFittedMean row;
        row.factor = "A";
        row.level = level;
        const auto it = sums_a.find(level);
        if (it != sums_a.end() && it->second.second > 0) {
            row.count = it->second.second;
            row.fitted_mean = it->second.first / static_cast<double>(row.count);
        }
        out.push_back(row);
    }
    for (const auto& level : levels_b) {
        GlmFittedMean row;
        row.factor = "B";
        row.level = level;
        const auto it = sums_b.find(level);
        if (it != sums_b.end() && it->second.second > 0) {
            row.count = it->second.second;
            row.fitted_mean = it->second.first / static_cast<double>(row.count);
        }
        out.push_back(row);
    }
}

}  // namespace

GlmTwoWayResult glm_two_way_analyze(
    const std::vector<std::string>& factor_a,
    const std::vector<std::string>& factor_b,
    const std::vector<double>& response,
    const std::vector<std::size_t>& source_rows,
    const GlmTwoWayOptions& options)
{
    GlmTwoWayResult result;
    result.include_interaction = options.include_interaction;
    const std::size_t count = factor_a.size();
    result.omitted_observation_count = 0;

    std::vector<Observation> observations;
    observations.reserve(count);
    std::set<std::string> levels_a_set;
    std::set<std::string> levels_b_set;
    for (std::size_t index = 0; index < count; ++index) {
        if (index >= factor_b.size() || factor_a[index].empty()
            || factor_b[index].empty() || !std::isfinite(response[index])) {
            ++result.omitted_observation_count;
            continue;
        }
        observations.push_back({factor_a[index], factor_b[index], response[index]});
        levels_a_set.insert(factor_a[index]);
        levels_b_set.insert(factor_b[index]);
    }
    result.observation_count = observations.size();
    if (result.omitted_observation_count > 0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "missing_glm_values",
                       "含缺失因子或响应值的观测已从分析中排除。");
    }
    if (levels_a_set.size() < 2 || levels_b_set.size() < 2
        || observations.size() < 4) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_glm_levels",
                       "双因子 GLM 需要两个因子各至少两个水平及至少四个观测。");
        return result;
    }

    const std::vector<std::string> levels_a(levels_a_set.cbegin(), levels_a_set.cend());
    const std::vector<std::string> levels_b(levels_b_set.cbegin(), levels_b_set.cend());
    std::map<std::pair<std::string, std::string>, std::size_t> cell_counts;
    for (const auto& observation : observations) {
        ++cell_counts[{observation.a, observation.b}];
    }
    const std::size_t min_count = std::min_element(
        cell_counts.cbegin(), cell_counts.cend(),
        [](const auto& first, const auto& second) {
            return first.second < second.second;
        })->second;
    if (std::any_of(cell_counts.cbegin(), cell_counts.cend(),
                    [min_count](const auto& cell) {
                        return cell.second != min_count;
                    })) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "unbalanced_glm_design",
                       "不平衡设计；使用 Type III Adj SS 与拟合均值。");
        result.design_balanced = false;
    }

    std::vector<double> y;
    y.reserve(observations.size());
    for (const auto& observation : observations) {
        y.push_back(observation.y);
    }

    const int full_model = options.include_interaction ? 7 : 3;
    const Fit intercept_fit = fit_glm_model(
        make_glm_design(observations, levels_a, levels_b, 0), y);
    const Fit full_fit = fit_glm_model(
        make_glm_design(observations, levels_a, levels_b, full_model), y);
    result.error_sum_of_squares = full_fit.rss;
    result.error_degrees_of_freedom = observations.size() > full_fit.rank
        ? observations.size() - full_fit.rank : 0;
    result.error_mean_square = result.error_degrees_of_freedom > 0
        ? result.error_sum_of_squares
            / static_cast<double>(result.error_degrees_of_freedom) : 0.0;
    result.fitted = full_fit.fitted;
    result.residuals.reserve(y.size());
    for (std::size_t i = 0; i < y.size(); ++i) {
        result.residuals.push_back(y[i] - full_fit.fitted[i]);
    }

    for (std::size_t index = 0; index < count; ++index) {
        if (index >= factor_b.size() || factor_a[index].empty()
            || factor_b[index].empty() || !std::isfinite(response[index])) {
            continue;
        }
        const std::size_t source = index < source_rows.size()
            ? source_rows[index] : index;
        result.observation_source_rows.push_back(source);
    }

    const std::vector<std::string> labels = options.include_interaction
        ? std::vector<std::string>{"Factor A", "Factor B", "A*B"}
        : std::vector<std::string>{"Factor A", "Factor B"};
    const std::vector<int> reduced_models = options.include_interaction
        ? std::vector<int>{2, 1, 3}
        : std::vector<int>{2, 1};
    const std::vector<Fit> sequential_fits = {
        intercept_fit,
        fit_glm_model(make_glm_design(observations, levels_a, levels_b, 1), y),
        fit_glm_model(make_glm_design(observations, levels_a, levels_b, 3), y),
        full_fit
    };

    for (std::size_t index = 0; index < labels.size(); ++index) {
        GlmAnovaEffect effect;
        effect.term = labels[index];
        const Fit reduced_fit = fit_glm_model(
            make_glm_design(observations, levels_a, levels_b, reduced_models[index]), y);
        const double adjusted_ss = reduced_fit.rss - full_fit.rss;
        const std::size_t adjusted_df = full_fit.rank - fit_glm_model(
            make_glm_design(observations, levels_a, levels_b,
                            index == 0 ? 6 : index == 1 ? 5 : 3),
            y).rank;
        if (adjusted_df == 0) {
            effect.estimable = false;
        } else {
            effect.adjusted_sum_of_squares = std::max(0.0, adjusted_ss);
            effect.degrees_of_freedom = adjusted_df;
            effect.mean_square = *effect.adjusted_sum_of_squares
                / static_cast<double>(adjusted_df);
            if (result.error_degrees_of_freedom > 0 && result.error_mean_square > 0.0) {
                effect.f_statistic = *effect.mean_square / result.error_mean_square;
                effect.p_value = f_right_tail(
                    *effect.f_statistic, static_cast<double>(adjusted_df),
                    static_cast<double>(result.error_degrees_of_freedom));
            }
        }
        result.anova_effects.push_back(effect);
    }

    add_fitted_means(observations, full_fit.fitted, levels_a, levels_b,
                     result.fitted_means);

    if (result.residuals.size() >= 3) {
        const auto normality =
            datalab::domain::statistics::normality_test(result.residuals);
        if (normality.p_value.has_value()) {
            result.residual_normality_p = normality.p_value;
        }
    }

    return result;
}

}  // namespace datalab::domain::statistics
