#include "domain/statistics/rsm_analysis.h"

#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <vector>

namespace datalab::domain::statistics {
namespace {

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const char* message)
{
    diagnostics.push_back({severity, code, message});
}

double predict_rsm(
    const RsmAnalysisResult& fit,
    const std::vector<double>& coded)
{
    if (fit.regression.coefficients.empty() || coded.size() != fit.factor_names.size()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double y = fit.regression.coefficients.front().coefficient;
    std::size_t index = 1;
    const std::size_t k = fit.factor_names.size();
    for (std::size_t i = 0; i < k && index < fit.regression.coefficients.size(); ++i, ++index) {
        y += fit.regression.coefficients[index].coefficient * coded[i];
    }
    for (std::size_t i = 0; i < k; ++i) {
        for (std::size_t j = i + 1; j < k; ++j, ++index) {
            if (index >= fit.regression.coefficients.size()) {
                return y;
            }
            y += fit.regression.coefficients[index].coefficient * coded[i] * coded[j];
        }
    }
    for (std::size_t i = 0; i < k && index < fit.regression.coefficients.size(); ++i, ++index) {
        y += fit.regression.coefficients[index].coefficient * coded[i] * coded[i];
    }
    return y;
}

}  // namespace

std::vector<std::vector<double>> code_rsm_factors(
    const std::vector<std::vector<double>>& raw_factors,
    std::vector<DiagnosticMessage>& diagnostics)
{
    std::vector<std::vector<double>> coded = raw_factors;
    if (raw_factors.empty() || raw_factors.front().empty()) {
        return coded;
    }
    const std::size_t factor_count = raw_factors.front().size();
    bool already_coded = true;
    for (const auto& row : raw_factors) {
        for (const double value : row) {
            if (std::abs(value) > 1.0 + 1.0e-9) {
                already_coded = false;
            }
        }
    }
    if (already_coded) {
        add_diagnostic(diagnostics, DiagnosticMessage::Severity::info,
                       "rsm_factors_already_coded",
                       "因子值均在 [-1,1]，按已编码单位拟合 RSM。");
        return coded;
    }
    for (std::size_t factor = 0; factor < factor_count; ++factor) {
        double minimum = raw_factors.front()[factor];
        double maximum = minimum;
        for (const auto& row : raw_factors) {
            minimum = std::min(minimum, row[factor]);
            maximum = std::max(maximum, row[factor]);
        }
        const double span = maximum - minimum;
        if (!(span > 0.0)) {
            add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                           "rsm_factor_no_variation",
                           "至少一个因子无变异，无法编码。");
            return {};
        }
        for (std::size_t row = 0; row < coded.size(); ++row) {
            coded[row][factor] = 2.0 * (raw_factors[row][factor] - minimum) / span - 1.0;
        }
    }
    add_diagnostic(diagnostics, DiagnosticMessage::Severity::info,
                   "rsm_factors_minmax_coded",
                   "因子已按列内 min/max 线性编码到 [-1,1]。");
    return coded;
}

std::vector<std::vector<double>> code_rsm_factors_from_design_bounds(
    const std::vector<std::vector<double>>& raw_factors,
    const std::vector<double>& lows,
    const std::vector<double>& highs,
    const std::vector<double>& centers,
    std::vector<DiagnosticMessage>& diagnostics)
{
    std::vector<std::vector<double>> coded = raw_factors;
    if (raw_factors.empty() || raw_factors.front().empty()) {
        return coded;
    }
    const std::size_t factor_count = raw_factors.front().size();
    if (lows.size() < factor_count || highs.size() < factor_count) {
        add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                       "rsm_design_bounds_incomplete",
                       "设计编码边界与因子数不匹配。");
        return {};
    }
    for (std::size_t factor = 0; factor < factor_count; ++factor) {
        const double low = lows[factor];
        const double high = highs[factor];
        if (!std::isfinite(low) || !std::isfinite(high) || !(low < high)) {
            add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                           "rsm_design_bounds_invalid",
                           "设计低/高水平必须有限且 low < high。");
            return {};
        }
        const double center = factor < centers.size() && std::isfinite(centers[factor])
            ? centers[factor]
            : 0.5 * (low + high);
        if (center < low || center > high) {
            add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                           "rsm_design_center_invalid",
                           "设计中心必须落在 [low, high] 内。");
            return {};
        }
        const double half = 0.5 * (high - low);
        for (std::size_t row = 0; row < coded.size(); ++row) {
            coded[row][factor] = (raw_factors[row][factor] - center) / half;
        }
    }
    add_diagnostic(diagnostics, DiagnosticMessage::Severity::info,
                   "rsm_factors_design_bounds_coded",
                   "因子已按设计 low/high/center 编码（与 CCD/BBD 一致）。");
    return coded;
}

RsmAnalysisResult fit_rsm_analysis(
    const std::vector<double>& response,
    const std::vector<std::vector<double>>& coded_factors,
    const std::vector<std::string>& factor_names,
    const std::string& response_name,
    const std::vector<std::size_t>& source_rows)
{
    RsmAnalysisResult result;
    result.response_name = response_name;
    result.factor_names = factor_names;
    if (response.size() != coded_factors.size() || factor_names.size() < 2) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_rsm_shape",
                       "RSM 需要响应与因子行数一致，且至少两个因子。");
        return result;
    }
    const std::size_t k = factor_names.size();
    for (const auto& row : coded_factors) {
        if (row.size() != k) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "ragged_rsm_factors",
                           "每行因子列数必须一致。");
            return result;
        }
    }

    std::vector<std::string> term_names;
    std::vector<std::vector<double>> predictors(response.size());
    for (std::size_t i = 0; i < k; ++i) {
        term_names.push_back(factor_names[i]);
    }
    for (std::size_t i = 0; i < k; ++i) {
        for (std::size_t j = i + 1; j < k; ++j) {
            term_names.push_back(factor_names[i] + "*" + factor_names[j]);
        }
    }
    for (std::size_t i = 0; i < k; ++i) {
        term_names.push_back(factor_names[i] + "*" + factor_names[i]);
    }
    result.term_names = term_names;

    for (std::size_t row = 0; row < response.size(); ++row) {
        std::vector<double> cols;
        cols.reserve(term_names.size());
        for (std::size_t i = 0; i < k; ++i) {
            cols.push_back(coded_factors[row][i]);
        }
        for (std::size_t i = 0; i < k; ++i) {
            for (std::size_t j = i + 1; j < k; ++j) {
                cols.push_back(coded_factors[row][i] * coded_factors[row][j]);
            }
        }
        for (std::size_t i = 0; i < k; ++i) {
            cols.push_back(coded_factors[row][i] * coded_factors[row][i]);
        }
        predictors[row] = std::move(cols);
    }

    result.regression = fit_linear_regression(
        response, predictors, term_names, 0.95, source_rows);
    result.diagnostics.insert(result.diagnostics.end(),
                              result.regression.diagnostics.cbegin(),
                              result.regression.diagnostics.cend());

    // Pure error / lack-of-fit from replicated coded points (formula_reference).
    // Only after a usable regression fit — never invent PE from residual MS alone.
    if (result.regression.coefficients.empty()
        || result.regression.observation_count != response.size()) {
        return result;
    }
    // Quantize coded levels so floating design points still group as replicates.
    struct ReplicateAccumulator {
        double sum = 0.0;
        double squared_sum = 0.0;
        std::size_t count = 0;
        bool near_center = true;
    };
    std::map<std::vector<int>, ReplicateAccumulator> replicate_groups;
    for (std::size_t row = 0; row < coded_factors.size(); ++row) {
        std::vector<int> key;
        key.reserve(k);
        bool near_center = true;
        for (std::size_t factor = 0; factor < k; ++factor) {
            const double value = coded_factors[row][factor];
            key.push_back(static_cast<int>(std::llround(value * 1000.0)));
            if (std::fabs(value) > 1.0e-6) {
                near_center = false;
            }
        }
        ReplicateAccumulator& group = replicate_groups[key];
        group.sum += response[row];
        group.squared_sum += response[row] * response[row];
        ++group.count;
        group.near_center = group.near_center && near_center;
    }
    result.replicate_group_count = replicate_groups.size();
    double pure_error_ss = 0.0;
    std::size_t pure_error_df = 0;
    bool has_replicates = false;
    for (const auto& [key, group] : replicate_groups) {
        (void)key;
        if (group.count > 1) {
            has_replicates = true;
            pure_error_ss += group.squared_sum
                - group.sum * group.sum / static_cast<double>(group.count);
            pure_error_df += group.count - 1;
            if (group.near_center) {
                result.center_like_replicate_count += group.count;
            }
        }
    }
    if (has_replicates) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                       "rsm_replicated_coded_points",
                       "检测到重复编码点，已用于纯误差 / 失拟估计。");
    }
    const std::size_t residual_df =
        result.regression.observation_count > result.regression.predictor_count + 1
            ? result.regression.observation_count - result.regression.predictor_count - 1
            : 0;
    if (pure_error_df > 0) {
        const double pure_error_ms =
            pure_error_ss / static_cast<double>(pure_error_df);
        result.pure_error_anova_row = RsmAnovaRow{
            "Pure Error", pure_error_ss, pure_error_df, pure_error_ms, 0.0,
            std::nullopt};
    } else {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "rsm_insufficient_pure_error",
                       "没有重复编码点，无法估计纯误差和失拟；不得用残差 MS 冒充纯误差。");
    }
    if (pure_error_df > 0 && residual_df >= pure_error_df) {
        const std::size_t lack_of_fit_df = residual_df - pure_error_df;
        const double lack_of_fit_ss = std::max(
            0.0, result.regression.error_sum_of_squares - pure_error_ss);
        if (lack_of_fit_df > 0) {
            const double lack_of_fit_ms =
                lack_of_fit_ss / static_cast<double>(lack_of_fit_df);
            const double pure_error_ms =
                pure_error_ss / static_cast<double>(pure_error_df);
            const double f_statistic =
                pure_error_ms > 0.0 ? lack_of_fit_ms / pure_error_ms : 0.0;
            const std::optional<double> lack_of_fit_p_value =
                pure_error_ms > 0.0
                    ? std::optional<double>(f_right_tail(
                          f_statistic,
                          static_cast<double>(lack_of_fit_df),
                          static_cast<double>(pure_error_df)))
                    : std::nullopt;
            result.lack_of_fit_anova_row = RsmAnovaRow{
                "Lack of Fit", lack_of_fit_ss, lack_of_fit_df, lack_of_fit_ms,
                f_statistic, lack_of_fit_p_value};
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                           "rsm_lof_formula_reference",
                           "失拟 ANOVA 证据类型 formula_reference；"
                           "不是 vendor_oracle。");
        } else {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                           "rsm_insufficient_lof_df",
                           "纯误差占用了全部残差自由度，无法检验失拟。");
        }
    } else if (pure_error_df > residual_df) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "rsm_pure_error_exceeds_residual_df",
                       "纯误差自由度大于残差自由度，跳过失拟分解（可能模型过参或重复点过多）。");
    }
    return result;
}

RsmCodedGrid evaluate_rsm_grid(
    const RsmAnalysisResult& fit,
    std::size_t x_factor_index,
    std::size_t y_factor_index,
    std::size_t resolution,
    const std::vector<double>* hold_coded)
{
    RsmCodedGrid grid;
    grid.x_factor_index = x_factor_index;
    grid.y_factor_index = y_factor_index;
    const std::size_t k = fit.factor_names.size();
    if (k < 2 || x_factor_index >= k || y_factor_index >= k
        || x_factor_index == y_factor_index || resolution < 2
        || fit.regression.coefficients.empty()) {
        add_diagnostic(grid.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_rsm_grid",
                       "等值线需要两个不同因子且模型已拟合。");
        return grid;
    }
    std::vector<double> hold(k, 0.0);
    if (hold_coded != nullptr && hold_coded->size() == k) {
        hold = *hold_coded;
    }
    grid.x.resize(resolution);
    grid.y.resize(resolution);
    for (std::size_t i = 0; i < resolution; ++i) {
        grid.x[i] = -1.0 + 2.0 * static_cast<double>(i) / static_cast<double>(resolution - 1);
        grid.y[i] = grid.x[i];
    }
    grid.z.assign(resolution, std::vector<double>(resolution, 0.0));
    for (std::size_t iy = 0; iy < resolution; ++iy) {
        for (std::size_t ix = 0; ix < resolution; ++ix) {
            std::vector<double> coded = hold;
            coded[x_factor_index] = grid.x[ix];
            coded[y_factor_index] = grid.y[iy];
            grid.z[iy][ix] = predict_rsm(fit, coded);
        }
    }
    return grid;
}

}  // namespace datalab::domain::statistics
