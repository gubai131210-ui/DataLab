#include "domain/statistics/mixture_analyze.h"

#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <numeric>

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

std::string ascii_lower(std::string text)
{
    for (char& ch : text) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return text;
}

struct OlsFit {
    std::vector<double> coefficients;
    std::vector<double> fitted;
    std::vector<double> residuals;
    double rss = 0.0;
    std::size_t rank = 0;
    std::size_t error_df = 0;
    double mse = 0.0;
    Matrix covariance;
    bool ok = false;
};

OlsFit fit_ols_no_intercept(const Matrix& design, const std::vector<double>& response)
{
    OlsFit result;
    if (design.empty() || design.size() != response.size()) {
        return result;
    }
    const std::size_t n = response.size();
    const std::size_t p = design.front().size();
    if (p == 0 || n <= p) {
        return result;
    }

    Matrix q(n, std::vector<double>(p, 0.0));
    Matrix r(p, std::vector<double>(p, 0.0));
    constexpr double tolerance = 1.0e-10;
    std::size_t rank = 0;
    for (std::size_t column = 0; column < p; ++column) {
        std::vector<double> vector(n, 0.0);
        for (std::size_t row = 0; row < n; ++row) {
            vector[row] = design[row][column];
        }
        for (std::size_t previous = 0; previous < column; ++previous) {
            double projection = 0.0;
            for (std::size_t row = 0; row < n; ++row) {
                projection += q[row][previous] * vector[row];
            }
            r[previous][column] = projection;
            for (std::size_t row = 0; row < n; ++row) {
                vector[row] -= projection * q[row][previous];
            }
        }
        double norm = 0.0;
        for (const double value : vector) {
            norm += value * value;
        }
        norm = std::sqrt(norm);
        if (norm < tolerance) {
            continue;
        }
        r[column][column] = norm;
        for (std::size_t row = 0; row < n; ++row) {
            q[row][column] = vector[row] / norm;
        }
        ++rank;
    }
    result.rank = rank;
    if (rank == 0) {
        return result;
    }

    std::vector<double> qy(p, 0.0);
    for (std::size_t column = 0; column < p; ++column) {
        for (std::size_t row = 0; row < n; ++row) {
            qy[column] += q[row][column] * response[row];
        }
    }
    result.coefficients.assign(p, 0.0);
    for (std::size_t index = p; index-- > 0;) {
        if (r[index][index] < tolerance) {
            continue;
        }
        double value = qy[index];
        for (std::size_t next = index + 1; next < p; ++next) {
            value -= r[index][next] * result.coefficients[next];
        }
        result.coefficients[index] = value / r[index][index];
    }

    result.fitted.assign(n, 0.0);
    for (std::size_t row = 0; row < n; ++row) {
        for (std::size_t column = 0; column < p; ++column) {
            result.fitted[row] += design[row][column] * result.coefficients[column];
        }
        result.residuals.push_back(response[row] - result.fitted[row]);
        result.rss += result.residuals.back() * result.residuals.back();
    }
    result.error_df = n > rank ? n - rank : 0;
    result.mse = result.error_df > 0 ? result.rss / static_cast<double>(result.error_df) : 0.0;
    result.ok = true;
    return result;
}

Matrix build_design(
    const std::vector<std::vector<double>>& components,
    MixtureModelOrder order,
    std::vector<std::string>& term_names)
{
    const std::size_t q = components.empty() ? 0 : components.front().size();
    term_names.clear();
    for (std::size_t i = 0; i < q; ++i) {
        term_names.push_back("x" + std::to_string(i + 1));
    }
    if (order == MixtureModelOrder::quadratic) {
        for (std::size_t i = 0; i < q; ++i) {
            for (std::size_t j = i + 1; j < q; ++j) {
                term_names.push_back(
                    "x" + std::to_string(i + 1) + "*x" + std::to_string(j + 1));
            }
        }
    }
    Matrix design(components.size(), std::vector<double>(term_names.size(), 0.0));
    for (std::size_t row = 0; row < components.size(); ++row) {
        std::size_t col = 0;
        for (std::size_t i = 0; i < q; ++i) {
            design[row][col++] = components[row][i];
        }
        if (order == MixtureModelOrder::quadratic) {
            for (std::size_t i = 0; i < q; ++i) {
                for (std::size_t j = i + 1; j < q; ++j) {
                    design[row][col++] = components[row][i] * components[row][j];
                }
            }
        }
    }
    return design;
}

}  // namespace

MixtureModelOrder parse_mixture_model_order(const std::string& text)
{
    const std::string lower = ascii_lower(text);
    if (lower == "quadratic" || lower == "quad" || lower == "2") {
        return MixtureModelOrder::quadratic;
    }
    return MixtureModelOrder::linear;
}

std::string mixture_model_order_name(MixtureModelOrder order)
{
    return order == MixtureModelOrder::quadratic ? "quadratic" : "linear";
}

MixtureAnalyzeResult analyze_mixture_scheffe(
    const std::vector<std::vector<double>>& components,
    const std::vector<double>& response,
    const std::vector<std::string>& component_names,
    const std::vector<std::size_t>& source_rows,
    const MixtureAnalyzeOptions& options)
{
    MixtureAnalyzeResult result;
    result.model_order = mixture_model_order_name(options.model_order);
    result.component_names = component_names;
    result.component_count = component_names.size();
    result.observation_count = components.size();

    if (components.empty() || response.empty() || components.size() != response.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_mixture_data", "需要至少一行分量与响应数据。");
        return result;
    }
    const std::size_t q = components.front().size();
    if (q < 2 || q > 4) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_component_count", "分量数 q 仅支持 2～4。");
        return result;
    }
    for (const auto& row : components) {
        if (row.size() != q) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "inconsistent_component_width", "分量列宽不一致。");
            return result;
        }
        double sum = 0.0;
        for (double value : row) {
            if (!std::isfinite(value)) {
                add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                               "non_finite_component", "分量含非有限值。");
                return result;
            }
            sum += value;
        }
        if (std::abs(sum - 1.0) > options.sum_tolerance) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                           "component_sum_not_one",
                           "分量之和偏离 1（容差 " + std::to_string(options.sum_tolerance)
                               + "）；未自动归一化。");
        }
    }

    std::vector<std::string> term_names;
    const Matrix design = build_design(components, options.model_order, term_names);
    const OlsFit fit = fit_ols_no_intercept(design, response);
    if (!fit.ok || fit.rank < term_names.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "rank_deficient_mixture",
                       "设计矩阵秩亏；部分系数可能不可估计。");
    }
    if (fit.error_df == 0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "no_error_df", "误差自由度为 0，无法计算推断统计量。");
    }

    const double tss = std::inner_product(
        response.cbegin(), response.cend(), response.cbegin(), 0.0)
        - std::pow(std::accumulate(response.cbegin(), response.cend(), 0.0)
                       / static_cast<double>(response.size()),
                   2.0)
            * static_cast<double>(response.size());
    result.r_squared = tss > 0.0 ? 1.0 - fit.rss / tss : 0.0;
    if (fit.error_df > 0 && response.size() > fit.rank) {
        const double adj = 1.0 - (fit.rss / static_cast<double>(fit.error_df))
            / (tss / static_cast<double>(response.size() - 1));
        result.adjusted_r_squared = adj;
    }

    for (std::size_t i = 0; i < term_names.size(); ++i) {
        MixtureCoefficient coef;
        coef.term = term_names[i];
        coef.coefficient = i < fit.coefficients.size() ? fit.coefficients[i] : 0.0;
        if (fit.error_df > 0 && fit.mse > 0.0) {
            coef.standard_error = std::sqrt(fit.mse);
            if (coef.standard_error > 0.0) {
                coef.t_statistic = coef.coefficient / *coef.standard_error;
                coef.p_value = 2.0 * (1.0 - student_t_cdf(
                    std::abs(*coef.t_statistic),
                    static_cast<double>(fit.error_df)));
            }
        }
        result.coefficients.push_back(coef);
    }

    MixtureAnovaEffect model_effect;
    model_effect.term = "Model";
    model_effect.sequential_sum_of_squares = std::max(0.0, tss - fit.rss);
    model_effect.adjusted_sum_of_squares = model_effect.sequential_sum_of_squares;
    model_effect.degrees_of_freedom = fit.rank;
    if (fit.error_df > 0 && fit.rank > 0) {
        model_effect.mean_square =
            *model_effect.adjusted_sum_of_squares / static_cast<double>(fit.rank);
        if (fit.mse > 0.0) {
            model_effect.f_statistic = *model_effect.mean_square / fit.mse;
            model_effect.p_value = f_right_tail(
                *model_effect.f_statistic, static_cast<double>(fit.rank),
                static_cast<double>(fit.error_df));
        }
    }
    result.anova_effects.push_back(model_effect);

    MixtureAnovaEffect error_effect;
    error_effect.term = "Error";
    error_effect.sequential_sum_of_squares = fit.rss;
    error_effect.adjusted_sum_of_squares = fit.rss;
    error_effect.degrees_of_freedom = fit.error_df;
    if (fit.error_df > 0) {
        error_effect.mean_square = fit.mse;
    }
    result.anova_effects.push_back(error_effect);

    for (std::size_t i = 0; i < response.size(); ++i) {
        MixtureFitRow row;
        row.source_row = i < source_rows.size() ? source_rows[i] : i;
        row.observed = response[i];
        row.fitted = fit.fitted[i];
        row.residual = fit.residuals[i];
        result.fits.push_back(row);
    }

    return result;
}

}  // namespace datalab::domain::statistics
