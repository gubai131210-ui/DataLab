#include "domain/statistics/two_factor_anova.h"

#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
#include <limits>
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
    const char* message)
{
    diagnostics.push_back({severity, code, message});
}

struct Observation {
    std::string a;
    std::string b;
    double y = 0.0;
};

struct Fit {
    double rss = 0.0;
    std::size_t rank = 0;
};

Fit fit_model(const Matrix& matrix, const std::vector<double>& response)
{
    if (matrix.empty() || matrix.front().empty()) {
        const double mean = std::accumulate(
            response.cbegin(), response.cend(), 0.0)
            / static_cast<double>(response.size());
        double rss = 0.0;
        for (const double value : response) {
            rss += (value - mean) * (value - mean);
        }
        return {rss, 0};
    }
    const std::size_t n = response.size();
    const std::size_t columns = matrix.front().size();
    Matrix q;
    q.reserve(columns);
    constexpr double tolerance = 1.0e-10;
    for (std::size_t column = 0; column < columns; ++column) {
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
    double explained = 0.0;
    for (const auto& basis : q) {
        const double projection = std::inner_product(
            basis.cbegin(), basis.cend(), response.cbegin(), 0.0);
        explained += projection * projection;
    }
    double total = 0.0;
    const double mean = std::accumulate(
        response.cbegin(), response.cend(), 0.0)
        / static_cast<double>(response.size());
    for (const double value : response) {
        total += (value - mean) * (value - mean);
    }
    return {std::max(0.0, total - explained), q.size()};
}

Matrix make_model(
    const std::vector<Observation>& observations,
    const std::vector<std::string>& levels_a,
    const std::vector<std::string>& levels_b,
    AnovaFactorEncoding encoding,
    int model)
{
    // Model bits: A=1, B=2, interaction=4.
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
            if (encoding == AnovaFactorEncoding::reference) {
                a[index] = observations[row].a == levels_a[index] ? 1.0 : 0.0;
            } else {
                a[index] = observations[row].a == levels_a[index]
                    ? 1.0 : observations[row].a == levels_a.back() ? -1.0 : 0.0;
            }
        }
        for (std::size_t index = 0; index < b_columns; ++index) {
            if (encoding == AnovaFactorEncoding::reference) {
                b[index] = observations[row].b == levels_b[index] ? 1.0 : 0.0;
            } else {
                b[index] = observations[row].b == levels_b[index]
                    ? 1.0 : observations[row].b == levels_b.back() ? -1.0 : 0.0;
            }
        }
        if ((model & 1) != 0) {
            for (const double value : a) {
                matrix[row][column++] = value;
            }
        }
        if ((model & 2) != 0) {
            for (const double value : b) {
                matrix[row][column++] = value;
            }
        }
        if ((model & 4) != 0) {
            for (const double a_value : a) {
                for (const double b_value : b) {
                    matrix[row][column++] = a_value * b_value;
                }
            }
        }
    }
    return matrix;
}

void add_factor_means(
    const std::vector<Observation>& observations,
    bool first,
    std::vector<AnovaFactorMean>& means)
{
    std::map<std::string, std::vector<double>> grouped;
    for (const auto& observation : observations) {
        grouped[first ? observation.a : observation.b].push_back(observation.y);
    }
    for (const auto& [level, values] : grouped) {
        means.push_back({level, values.size(), std::accumulate(
            values.cbegin(), values.cend(), 0.0) / values.size()});
    }
}

}  // namespace

TwoFactorAnovaResult two_factor_anova(const TwoFactorAnovaInput& input)
{
    TwoFactorAnovaResult result;
    const std::size_t count = input.response.size();
    if (count == 0 || input.factor_a.size() != count
        || input.factor_b.size() != count) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_two_factor_anova_shape",
                       "两个因子和响应变量必须具有相同且非零的行数。");
        return result;
    }
    std::vector<Observation> observations;
    std::set<std::string> levels_a_set;
    std::set<std::string> levels_b_set;
    for (std::size_t row = 0; row < count; ++row) {
        if (input.factor_a[row].empty() || input.factor_b[row].empty()
            || !std::isfinite(input.response[row])) {
            ++result.omitted_observation_count;
            continue;
        }
        observations.push_back(
            {input.factor_a[row], input.factor_b[row], input.response[row]});
        levels_a_set.insert(input.factor_a[row]);
        levels_b_set.insert(input.factor_b[row]);
    }
    result.observation_count = observations.size();
    if (result.omitted_observation_count > 0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "missing_two_factor_anova_values",
                       "含缺失因子或响应值的观测已从分析中排除。");
    }
    if (levels_a_set.size() < 2 || levels_b_set.size() < 2
        || observations.size() < 4) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_two_factor_anova_levels",
                       "双因素 ANOVA 需要两个因子各至少两个水平及足够观测。");
        return result;
    }
    const std::vector<std::string> levels_a(levels_a_set.cbegin(), levels_a_set.cend());
    const std::vector<std::string> levels_b(levels_b_set.cbegin(), levels_b_set.cend());
    std::map<std::pair<std::string, std::string>, std::size_t> cell_counts;
    for (const auto& observation : observations) {
        ++cell_counts[{observation.a, observation.b}];
    }
    if (cell_counts.size() < levels_a.size() * levels_b.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "incomplete_factor_cells",
                       "因子组合存在空单元，交互项可能秩亏。");
    }
    const std::size_t minimum_count = std::min_element(
        cell_counts.cbegin(), cell_counts.cend(),
        [](const auto& first, const auto& second) {
            return first.second < second.second;
        })->second;
    if (std::any_of(cell_counts.cbegin(), cell_counts.cend(),
                    [minimum_count](const auto& cell) {
                        return cell.second != minimum_count;
                    })) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "unbalanced_design",
                       "因子组合的重复数不平衡；Seq SS 与 Adj SS 可能不同。");
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "unbalanced_two_factor_anova",
                       "因子组合的重复数不平衡；Seq SS 与 Adj SS 可能不同。");
    }

    std::vector<double> response;
    response.reserve(observations.size());
    for (const auto& observation : observations) {
        response.push_back(observation.y);
    }
    const Fit intercept_fit = fit_model(
        make_model(observations, levels_a, levels_b, input.encoding, 0), response);
    result.total_sum_of_squares = intercept_fit.rss;
    result.grand_mean = std::accumulate(
        response.cbegin(), response.cend(), 0.0) / response.size();
    const Fit full_fit = fit_model(
        make_model(observations, levels_a, levels_b, input.encoding, 7), response);
    result.error_sum_of_squares = full_fit.rss;
    result.error_degrees_of_freedom = observations.size() > full_fit.rank
        ? observations.size() - full_fit.rank : 0;
    if (full_fit.rank < levels_a.size() * levels_b.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "rank_deficient",
                       "设计矩阵秩亏；不可估计的项不输出伪造 F/P。");
    }
    if (result.error_degrees_of_freedom == 0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "no_error_degrees_of_freedom",
                       "误差自由度为 0，无法计算 F 与 P。");
    }
    result.error_mean_square = result.error_degrees_of_freedom > 0
        ? result.error_sum_of_squares / result.error_degrees_of_freedom : 0.0;
    result.evidence.method_version = "2";
    result.evidence.valid_count = observations.size();
    result.evidence.omitted_count = result.omitted_observation_count;
    result.evidence.assumption_status = "not_verified";
    result.evidence.source_rows = input.source_rows;

    const std::vector<std::string> labels = {"Factor A", "Factor B", "A*B"};
    const std::vector<Fit> sequential_fits = {
        intercept_fit,
        fit_model(make_model(observations, levels_a, levels_b, input.encoding, 1), response),
        fit_model(make_model(observations, levels_a, levels_b, input.encoding, 3), response),
        full_fit
    };
    // Sequential fits are kept separately so Seq SS follows the requested term order.
    for (std::size_t index = 0; index < 3; ++index) {
        AnovaEffectResult effect;
        effect.term = labels[index];
        const double sequential_ss =
            sequential_fits[index].rss - sequential_fits[index + 1].rss;
        effect.degrees_of_freedom = sequential_fits[index + 1].rank
            - sequential_fits[index].rank;
        const int reduced_model = index == 0 ? 2 : index == 1 ? 1 : 3;
        const Fit reduced_fit = fit_model(
            make_model(observations, levels_a, levels_b, input.encoding, reduced_model),
            response);
        const double adjusted_ss = reduced_fit.rss - full_fit.rss;
        const std::size_t adjusted_df = index == 0
            ? full_fit.rank - fit_model(make_model(
                observations, levels_a, levels_b, input.encoding, 6), response).rank
            : index == 1
                ? full_fit.rank - fit_model(make_model(
                    observations, levels_a, levels_b, input.encoding, 5), response).rank
                : full_fit.rank - fit_model(make_model(
                    observations, levels_a, levels_b, input.encoding, 3), response).rank;
        if (effect.degrees_of_freedom == 0 || adjusted_df == 0) {
            effect.estimable = false;
            effect.estimability = "rank_deficient";
            const std::string message =
                effect.term + " 不可估计，不输出伪造平方和或 F/P。";
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                           "not_estimable", message.c_str());
        } else {
            effect.sequential_sum_of_squares = std::max(0.0, sequential_ss);
            effect.adjusted_sum_of_squares = std::max(0.0, adjusted_ss);
            effect.mean_square = *effect.adjusted_sum_of_squares
                / static_cast<double>(adjusted_df);
            if (result.error_degrees_of_freedom == 0
                || !(result.error_mean_square > 0.0)) {
                effect.estimability = "no_error_degrees_of_freedom";
            } else {
                effect.f_statistic = *effect.mean_square / result.error_mean_square;
                effect.p_value = f_right_tail(
                    *effect.f_statistic, static_cast<double>(adjusted_df),
                    static_cast<double>(result.error_degrees_of_freedom));
            }
        }
        result.effects.push_back(effect);
    }
    add_factor_means(observations, true, result.factor_a_means);
    add_factor_means(observations, false, result.factor_b_means);
    std::map<std::pair<std::string, std::string>, std::vector<double>> cells;
    for (const auto& observation : observations) {
        cells[{observation.a, observation.b}].push_back(observation.y);
    }
    for (const auto& [levels, values] : cells) {
        result.interaction_means.push_back({levels.first, levels.second, values.size(),
            std::accumulate(values.cbegin(), values.cend(), 0.0) / values.size()});
    }
    return result;
}

}  // namespace datalab::domain::statistics
