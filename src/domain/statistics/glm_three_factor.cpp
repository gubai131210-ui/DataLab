#include "domain/statistics/glm_three_factor.h"

#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/normality_test.h"

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
    std::string c;
    double y = 0.0;
};

struct Fit {
    double rss = 0.0;
    std::size_t rank = 0;
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
    const std::vector<std::string>& levels_c,
    int model)
{
    const std::size_t a_columns = levels_a.size() - 1;
    const std::size_t b_columns = levels_b.size() - 1;
    const std::size_t c_columns = levels_c.size() - 1;
    std::size_t columns = 1;
    if ((model & 1) != 0) {
        columns += a_columns;
    }
    if ((model & 2) != 0) {
        columns += b_columns;
    }
    if ((model & 4) != 0) {
        columns += c_columns;
    }
    if ((model & 8) != 0) {
        columns += a_columns * b_columns;
    }
    if ((model & 16) != 0) {
        columns += a_columns * c_columns;
    }
    if ((model & 32) != 0) {
        columns += b_columns * c_columns;
    }
    Matrix matrix(observations.size(), std::vector<double>(columns, 0.0));
    for (std::size_t row = 0; row < observations.size(); ++row) {
        std::size_t column = 0;
        matrix[row][column++] = 1.0;
        std::vector<double> a(a_columns, 0.0);
        std::vector<double> b(b_columns, 0.0);
        std::vector<double> c(c_columns, 0.0);
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
        for (std::size_t index = 0; index < c_columns; ++index) {
            if (observations[row].c == levels_c[index + 1]) {
                c[index] = 1.0;
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
            for (double value : c) {
                matrix[row][column++] = value;
            }
        }
        if ((model & 8) != 0) {
            for (double value_a : a) {
                for (double value_b : b) {
                    matrix[row][column++] = value_a * value_b;
                }
            }
        }
        if ((model & 16) != 0) {
            for (double value_a : a) {
                for (double value_c : c) {
                    matrix[row][column++] = value_a * value_c;
                }
            }
        }
        if ((model & 32) != 0) {
            for (double value_b : b) {
                for (double value_c : c) {
                    matrix[row][column++] = value_b * value_c;
                }
            }
        }
    }
    return matrix;
}

int full_model_mask(const GlmThreeFactorOptions& options)
{
    int mask = 7;
    if (options.include_ab_interaction) {
        mask |= 8;
    }
    if (options.include_ac_interaction) {
        mask |= 16;
    }
    if (options.include_bc_interaction) {
        mask |= 32;
    }
    return mask;
}

void add_fitted_means(
    const std::vector<Observation>& observations,
    const std::vector<double>& fitted,
    const std::vector<std::string>& levels_a,
    const std::vector<std::string>& levels_b,
    const std::vector<std::string>& levels_c,
    std::vector<GlmFittedMean>& out)
{
    auto accumulate = [&](char factor, const std::vector<std::string>& levels,
                          std::string Observation::*member) {
        std::map<std::string, std::pair<double, std::size_t>> sums;
        for (std::size_t i = 0; i < observations.size(); ++i) {
            auto& entry = sums[observations[i].*member];
            entry.first += fitted[i];
            ++entry.second;
        }
        for (const auto& level : levels) {
            GlmFittedMean row;
            row.factor = std::string(1, factor);
            row.level = level;
            const auto it = sums.find(level);
            if (it != sums.end() && it->second.second > 0) {
                row.count = it->second.second;
                row.fitted_mean = it->second.first / static_cast<double>(row.count);
            }
            out.push_back(row);
        }
    };
    accumulate('A', levels_a, &Observation::a);
    accumulate('B', levels_b, &Observation::b);
    accumulate('C', levels_c, &Observation::c);
}

}  // namespace

GlmThreeFactorResult glm_three_factor_analyze(
    const std::vector<std::string>& factor_a,
    const std::vector<std::string>& factor_b,
    const std::vector<std::string>& factor_c,
    const std::vector<double>& response,
    const std::vector<std::size_t>& source_rows,
    const GlmThreeFactorOptions& options)
{
    GlmThreeFactorResult result;
    result.include_ab_interaction = options.include_ab_interaction;
    result.include_ac_interaction = options.include_ac_interaction;
    result.include_bc_interaction = options.include_bc_interaction;
    const std::size_t count = factor_a.size();

    std::vector<Observation> observations;
    std::set<std::string> levels_a_set;
    std::set<std::string> levels_b_set;
    std::set<std::string> levels_c_set;
    for (std::size_t index = 0; index < count; ++index) {
        if (index >= factor_b.size() || index >= factor_c.size()
            || factor_a[index].empty() || factor_b[index].empty()
            || factor_c[index].empty() || !std::isfinite(response[index])) {
            ++result.omitted_observation_count;
            continue;
        }
        observations.push_back(
            {factor_a[index], factor_b[index], factor_c[index], response[index]});
        levels_a_set.insert(factor_a[index]);
        levels_b_set.insert(factor_b[index]);
        levels_c_set.insert(factor_c[index]);
    }
    result.observation_count = observations.size();
    if (levels_a_set.size() < 2 || levels_b_set.size() < 2 || levels_c_set.size() < 2
        || observations.size() < 6) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_glm3_levels",
                       "三因子 GLM 需要各因子≥2 水平且至少 6 个观测。");
        return result;
    }

    const std::vector<std::string> levels_a(levels_a_set.cbegin(), levels_a_set.cend());
    const std::vector<std::string> levels_b(levels_b_set.cbegin(), levels_b_set.cend());
    const std::vector<std::string> levels_c(levels_c_set.cbegin(), levels_c_set.cend());

    std::map<std::tuple<std::string, std::string, std::string>, std::size_t> cell_counts;
    for (const auto& observation : observations) {
        ++cell_counts[{observation.a, observation.b, observation.c}];
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
                       "unbalanced_glm3_design",
                       "不平衡设计；使用 Type III Adj SS 与拟合均值。");
        result.design_balanced = false;
    }

    std::vector<double> y;
    y.reserve(observations.size());
    for (const auto& observation : observations) {
        y.push_back(observation.y);
    }

    const int full_model = full_model_mask(options);
    const Fit full_fit = fit_glm_model(
        make_glm_design(observations, levels_a, levels_b, levels_c, full_model), y);
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
        if (index >= factor_b.size() || index >= factor_c.size()
            || factor_a[index].empty() || factor_b[index].empty()
            || factor_c[index].empty() || !std::isfinite(response[index])) {
            continue;
        }
        const std::size_t source = index < source_rows.size() ? source_rows[index] : index;
        result.observation_source_rows.push_back(source);
    }

    struct TermSpec {
        std::string label;
        int reduced_model;
        int without_term_model;
    };
    std::vector<TermSpec> terms = {
        {"Factor A", full_model & ~1 & ~8 & ~16, full_model & ~1},
        {"Factor B", full_model & ~2 & ~8 & ~32, full_model & ~2},
        {"Factor C", full_model & ~4 & ~16 & ~32, full_model & ~4}};
    if (options.include_ab_interaction) {
        terms.push_back({"A*B", full_model & ~8, full_model & ~8});
    }
    if (options.include_ac_interaction) {
        terms.push_back({"A*C", full_model & ~16, full_model & ~16});
    }
    if (options.include_bc_interaction) {
        terms.push_back({"B*C", full_model & ~32, full_model & ~32});
    }

    for (const TermSpec& term : terms) {
        GlmAnovaEffect effect;
        effect.term = term.label;
        const Fit reduced_fit = fit_glm_model(
            make_glm_design(observations, levels_a, levels_b, levels_c,
                            term.reduced_model),
            y);
        const Fit without_fit = fit_glm_model(
            make_glm_design(observations, levels_a, levels_b, levels_c,
                            term.without_term_model),
            y);
        const double adjusted_ss = reduced_fit.rss - full_fit.rss;
        const std::size_t adjusted_df = full_fit.rank - without_fit.rank;
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

    add_fitted_means(observations, full_fit.fitted, levels_a, levels_b, levels_c,
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
