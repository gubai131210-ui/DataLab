#include "domain/statistics/msa_type1.h"

#include "domain/statistics/control_charts.h"
#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>

namespace datalab::domain::statistics {
namespace {
void error(std::vector<DiagnosticMessage>& d, const char* code, const char* message)
{
    d.push_back({DiagnosticMessage::Severity::error, code, message});
}
double mean(const std::vector<double>& x)
{
    return std::accumulate(x.begin(), x.end(), 0.0) / static_cast<double>(x.size());
}
double variance(const std::vector<double>& x, double m)
{
    double s = 0.0;
    for (double v : x) s += (v - m) * (v - m);
    return s / static_cast<double>(x.size() - 1);
}
}

MsaType1Result msa_type1(const std::vector<double>& measurements,
                         double reference, double tolerance,
                         double confidence_level)
{
    MsaType1Result r;
    if (measurements.size() < 2 || !std::isfinite(reference) || tolerance < 0.0
        || !(confidence_level > 0.0 && confidence_level < 1.0)) {
        error(r.diagnostics, "invalid_msa_type1_input",
              "Type 1 Gage 至少需要两条有限测量值，参考值和公差必须有效。");
        return r;
    }
    for (double v : measurements) {
        if (!std::isfinite(v)) {
            error(r.diagnostics, "invalid_msa_type1_value", "测量值必须为有限数。");
            return r;
        }
    }
    r.count = measurements.size();
    r.mean = mean(measurements);
    r.standard_deviation = std::sqrt(std::max(0.0, variance(measurements, r.mean)));
    r.bias = r.mean - reference;
    r.bias_standard_error = r.standard_deviation / std::sqrt(static_cast<double>(r.count));
    r.t_statistic = r.bias_standard_error > 0.0 ? r.bias / r.bias_standard_error : 0.0;
    r.degrees_of_freedom = static_cast<double>(r.count - 1);
    if (r.standard_deviation > 0.0 && r.bias_standard_error > 0.0) {
        r.inference_available = true;
        r.p_value = std::clamp(2.0 * (1.0 - student_t_cdf(
              std::abs(r.t_statistic), r.degrees_of_freedom)), 0.0, 1.0);
        const double critical = student_t_quantile(
            0.5 + confidence_level / 2.0, r.degrees_of_freedom);
        r.bias_ci_lower = r.bias - critical * r.bias_standard_error;
        r.bias_ci_upper = r.bias + critical * r.bias_standard_error;
    } else {
        error(r.diagnostics, "zero_repeatability",
              "重复性为零时 t、p 和置信区间不可用，不输出 p=0。");
        r.p_value = 1.0;
        r.t_statistic = 0.0;
        r.bias_ci_lower = r.bias;
        r.bias_ci_upper = r.bias;
        r.rules.push_back({
            "zero_repeatability", "triggered",
            "重复性为零，偏倚推断不可识别。", {},
            "不要把 p=1 或缺失推断解释为“无偏倚”。"});
    }
    if (tolerance > 0.0 && r.standard_deviation > 0.0) {
        r.cg = tolerance / (6.0 * r.standard_deviation);
        const double half_tolerance = tolerance / 2.0;
        r.cgk = std::min(half_tolerance - r.bias, half_tolerance + r.bias)
            / (3.0 * r.standard_deviation);
    }
    r.percent_tolerance = tolerance > 0.0
        ? 6.0 * r.standard_deviation / tolerance * 100.0 : 0.0;
    if (tolerance <= 0.0) {
        r.rules.push_back({
            "invalid_tolerance", "triggered",
            "未提供有效公差，Cg/Cgk/%Tolerance 不可用作能力证据。", {},
            "Type 1 能力指数需要有限正公差。"});
    }
    return r;
}

BiasLinearityResult bias_linearity(const std::vector<double>& references,
                                   const std::vector<double>& measurements,
                                   double confidence_level)
{
    BiasLinearityResult r;
    if (references.size() < 3 || references.size() != measurements.size()
        || !(confidence_level > 0.0 && confidence_level < 1.0)) {
        error(r.diagnostics, "invalid_bias_linearity_shape",
              "Bias/Linearity 要求至少三组且参考值与测量值长度一致。");
        return r;
    }
    for (std::size_t i = 0; i < references.size(); ++i) {
        if (!std::isfinite(references[i]) || !std::isfinite(measurements[i])) {
            error(r.diagnostics, "invalid_bias_linearity_value", "参考值和测量值必须为有限数。");
            return r;
        }
    }
    const double xbar = mean(references);
    std::vector<double> bias;
    bias.reserve(references.size());
    for (std::size_t i = 0; i < references.size(); ++i) bias.push_back(measurements[i] - references[i]);
    const double ybar = mean(bias);
    double sxx = 0.0;
    double sxy = 0.0;
    for (std::size_t i = 0; i < references.size(); ++i) {
        sxx += (references[i] - xbar) * (references[i] - xbar);
        sxy += (references[i] - xbar) * (bias[i] - ybar);
    }
    if (sxx <= 0.0) {
        error(r.diagnostics, "zero_reference_range", "参考值必须包含至少两个不同水平。");
        return r;
    }
    r.slope = sxy / sxx;
    r.intercept = ybar - r.slope * xbar;
    double ss_total = 0.0;
    double ss_error = 0.0;
    for (std::size_t i = 0; i < references.size(); ++i) {
        const double fitted = r.intercept + r.slope * references[i];
        ss_total += (bias[i] - ybar) * (bias[i] - ybar);
        ss_error += (bias[i] - fitted) * (bias[i] - fitted);
    }
    r.r_squared = ss_total > 0.0 ? 1.0 - ss_error / ss_total : 0.0;
    const double degrees_of_freedom = static_cast<double>(references.size() - 2);
    if (degrees_of_freedom > 0.0) {
        const double mean_square_error = ss_error / degrees_of_freedom;
        r.slope_standard_error = std::sqrt(mean_square_error / sxx);
        const double critical = student_t_quantile(
            0.5 + confidence_level / 2.0, degrees_of_freedom);
        r.slope_ci_lower = r.slope - critical * r.slope_standard_error;
        r.slope_ci_upper = r.slope + critical * r.slope_standard_error;
    }
    const auto [low, high] = std::minmax_element(references.begin(), references.end());
    r.bias_at_low = r.intercept + r.slope * *low;
    r.bias_at_high = r.intercept + r.slope * *high;
    std::map<double, BiasLinearityLevel> by_reference;
    for (std::size_t i = 0; i < references.size(); ++i) {
        BiasLinearityLevel& level = by_reference[references[i]];
        level.reference = references[i];
        ++level.valid_count;
        level.bias += measurements[i] - references[i];
        level.source_rows.push_back(i);
    }
    for (auto& [_, level] : by_reference) {
        level.bias /= static_cast<double>(level.valid_count);
        r.levels.push_back(level);
    }
    r.rules.push_back({
        "bias_linearity", "not_triggered",
        "已估计偏倚对参考值的线性关系，端点偏倚需与公差比较。", {},
        "回归关系不能替代跨操作者、跨部件的完整 Gage R&R。"});
    return r;
}

StabilityResult gage_stability(const std::vector<double>& measurements)
{
    StabilityResult r;
    if (measurements.size() < 3) {
        error(r.diagnostics, "invalid_stability_input", "Stability 至少需要三条测量值。");
        return r;
    }
    for (double v : measurements) {
        if (!std::isfinite(v)) {
            error(r.diagnostics, "invalid_stability_value", "测量值必须为有限数。");
            return r;
        }
    }
    const auto chart = ControlCharts::individuals_moving_range(measurements);
    r.values = measurements;
    r.diagnostics = chart.diagnostics;
    if (!chart.center_line.empty()) {
        r.center = chart.center_line.front();
        r.lower_control_limit = chart.lower_control_limit.empty()
            ? 0.0 : chart.lower_control_limit.front();
        r.upper_control_limit = chart.upper_control_limit.empty()
            ? 0.0 : chart.upper_control_limit.front();
    }
    r.sigma = (r.upper_control_limit - r.center) / 3.0;
    r.out_of_control = chart.test1_points;
    r.triggered_tests = chart.triggered_tests;
    r.primary_test_by_point = chart.primary_test_by_point;
    r.limit_source = "estimated_individuals";
    for (std::size_t i = 0; i < measurements.size(); ++i) {
        r.source_rows.push_back(i);
    }
    r.rules.push_back({
        "stability_signals",
        r.out_of_control.empty() ? "not_triggered" : "triggered",
        r.out_of_control.empty()
            ? "当前观测未发现稳定性图超限点。"
            : "量具稳定性图存在超限点，需要调查特殊原因。",
        {},
        "超限只是调查提示，不能直接判定量具合格或不合格。"});
    return r;
}
}  // namespace datalab::domain::statistics
