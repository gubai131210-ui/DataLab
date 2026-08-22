#include "domain/statistics/eda_plots.h"

#include "domain/statistics/quality_visuals.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

constexpr double kPi = 3.14159265358979323846;

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const char* message)
{
    diagnostics.push_back({severity, code, message});
}

double sample_sd(const std::vector<double>& values)
{
    if (values.size() < 2) {
        return 0.0;
    }
    const double mean = std::accumulate(values.cbegin(), values.cend(), 0.0)
        / static_cast<double>(values.size());
    double sum_sq = 0.0;
    for (const double value : values) {
        const double delta = value - mean;
        sum_sq += delta * delta;
    }
    return std::sqrt(sum_sq / static_cast<double>(values.size() - 1));
}

double interquartile_range(std::vector<double> values)
{
    if (values.size() < 2) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const auto summary = box_plot_summary(values);
    return summary.third_quartile - summary.first_quartile;
}

double silverman_bandwidth(const std::vector<double>& values)
{
    const double n = static_cast<double>(values.size());
    const double sd = sample_sd(values);
    const double iqr = interquartile_range(values);
    double scale = sd;
    if (iqr > 0.0) {
        const double robust = iqr / 1.34;
        scale = (sd > 0.0) ? std::min(sd, robust) : robust;
    }
    if (!(scale > 0.0)) {
        scale = 1.0;
    }
    return 0.9 * scale * std::pow(n, -0.2);
}

}  // namespace

KdeResult gaussian_kde(
    const std::vector<double>& values,
    std::size_t grid_count,
    std::optional<double> bandwidth)
{
    KdeResult result;
    result.n = values.size();
    if (values.size() < 2 || grid_count < 2) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_kde_data",
                       "密度估计至少需要两个有限观测。");
        return result;
    }
    for (const double value : values) {
        if (!std::isfinite(value)) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "non_finite_kde_value",
                           "密度估计要求全部观测为有限数值。");
            return result;
        }
    }
    result.bandwidth = bandwidth.has_value() && *bandwidth > 0.0
        ? *bandwidth
        : silverman_bandwidth(values);
    if (!(result.bandwidth > 0.0)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_kde_bandwidth",
                       "带宽必须为正。");
        return result;
    }
    add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                   "kde_silverman_bandwidth",
                   "默认带宽为 Silverman：h=0.9·min(s,IQR/1.34)·n^(-1/5)；高斯核。");
    const double minimum = *std::min_element(values.cbegin(), values.cend());
    const double maximum = *std::max_element(values.cbegin(), values.cend());
    const double left = minimum - 3.0 * result.bandwidth;
    const double right = maximum + 3.0 * result.bandwidth;
    result.x.resize(grid_count);
    result.density.assign(grid_count, 0.0);
    const double inv_h = 1.0 / result.bandwidth;
    const double coef = 1.0
        / (static_cast<double>(values.size()) * result.bandwidth * std::sqrt(2.0 * kPi));
    result.x.resize(grid_count);
    result.density.assign(grid_count, 0.0);
    for (std::size_t i = 0; i < grid_count; ++i) {
        const double x = left
            + (right - left) * static_cast<double>(i) / static_cast<double>(grid_count - 1);
        result.x[i] = x;
        double sum = 0.0;
        for (const double sample : values) {
            const double u = (x - sample) * inv_h;
            sum += std::exp(-0.5 * u * u);
        }
        result.density[i] = coef * sum;
    }
    return result;
}

HexbinResult hexbin_rectangular(
    const std::vector<double>& x_values,
    const std::vector<double>& y_values,
    const std::vector<std::size_t>& source_rows,
    int bin_hint)
{
    HexbinResult result;
    if (x_values.size() != y_values.size() || x_values.size() != source_rows.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "hexbin_shape_mismatch",
                       "Hexbin 要求 x/y/source_row 长度一致。");
        return result;
    }
    std::vector<double> xs;
    std::vector<double> ys;
    std::vector<std::size_t> rows;
    for (std::size_t i = 0; i < x_values.size(); ++i) {
        if (!std::isfinite(x_values[i]) || !std::isfinite(y_values[i])) {
            continue;
        }
        xs.push_back(x_values[i]);
        ys.push_back(y_values[i]);
        rows.push_back(source_rows[i]);
    }
    result.n = xs.size();
    result.source_rows = rows;
    if (xs.size() < 2) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_hexbin_points",
                       "二维分箱至少需要两个 complete-case 点。");
        return result;
    }
    add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                   "hexbin_rectangular_bins",
                   "使用矩形二维分箱（Binned Scatter）；非正六边形镶嵌。");
    const double x_min = *std::min_element(xs.cbegin(), xs.cend());
    const double x_max = *std::max_element(xs.cbegin(), xs.cend());
    const double y_min = *std::min_element(ys.cbegin(), ys.cend());
    const double y_max = *std::max_element(ys.cbegin(), ys.cend());
    std::size_t bins = bin_hint > 0
        ? static_cast<std::size_t>(bin_hint)
        : static_cast<std::size_t>(
              std::clamp(std::ceil(std::sqrt(static_cast<double>(xs.size()))), 8.0, 40.0));
    bins = std::max<std::size_t>(bins, 2);
    result.x_bins = bins;
    result.y_bins = bins;
    const double x_span = std::max(x_max - x_min, 1.0e-12);
    const double y_span = std::max(y_max - y_min, 1.0e-12);
    result.x_edges.resize(bins + 1);
    result.y_edges.resize(bins + 1);
    for (std::size_t i = 0; i <= bins; ++i) {
        result.x_edges[i] = x_min + x_span * static_cast<double>(i) / static_cast<double>(bins);
        result.y_edges[i] = y_min + y_span * static_cast<double>(i) / static_cast<double>(bins);
    }
    result.counts.assign(bins, std::vector<double>(bins, 0.0));
    result.cell_source_rows.assign(
        bins, std::vector<std::vector<std::size_t>>(bins));
    for (std::size_t i = 0; i < xs.size(); ++i) {
        std::size_t xi = static_cast<std::size_t>(
            std::floor((xs[i] - x_min) / x_span * static_cast<double>(bins)));
        std::size_t yi = static_cast<std::size_t>(
            std::floor((ys[i] - y_min) / y_span * static_cast<double>(bins)));
        if (xi >= bins) {
            xi = bins - 1;
        }
        if (yi >= bins) {
            yi = bins - 1;
        }
        result.counts[yi][xi] += 1.0;
        result.cell_source_rows[yi][xi].push_back(rows[i]);
        result.max_count = std::max(result.max_count, result.counts[yi][xi]);
    }
    return result;
}

ViolinResult violin_plot(
    const std::vector<double>& values,
    const std::vector<std::string>& groups,
    const std::vector<std::size_t>& source_rows)
{
    ViolinResult result;
    if (values.empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "empty_violin",
                       "小提琴图需要数值观测。");
        return result;
    }
    std::map<std::string, std::vector<double>> grouped;
    std::map<std::string, std::vector<std::size_t>> grouped_rows;
    std::vector<std::string> order;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (!std::isfinite(values[i])) {
            continue;
        }
        const std::string label = i < groups.size() && !groups[i].empty() ? groups[i] : "全部";
        if (grouped.find(label) == grouped.end()) {
            order.push_back(label);
        }
        grouped[label].push_back(values[i]);
        if (i < source_rows.size()) {
            grouped_rows[label].push_back(source_rows[i]);
        }
    }
    if (order.empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "empty_violin",
                       "小提琴图没有有限数值。");
        return result;
    }
    bool first_bandwidth = true;
    for (const std::string& label : order) {
        const auto& sample = grouped[label];
        if (sample.size() < 2) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                           "violin_group_too_small",
                           "某分组观测少于 2，已跳过密度形状。");
            continue;
        }
        const auto summary = box_plot_summary(sample);
        const auto kde = gaussian_kde(sample, 64);
        if (first_bandwidth) {
            result.bandwidth = kde.bandwidth;
            first_bandwidth = false;
        }
        ViolinGroup group;
        group.label = label;
        group.n = sample.size();
        group.whisker_low = summary.whisker_low;
        group.q1 = summary.first_quartile;
        group.median = summary.median;
        group.q3 = summary.third_quartile;
        group.whisker_high = summary.whisker_high;
        group.outliers = summary.outliers;
        group.source_rows = grouped_rows[label];
        group.density_y = kde.x;
        double peak = 0.0;
        for (const double density : kde.density) {
            peak = std::max(peak, density);
        }
        group.density_values.reserve(kde.density.size());
        for (const double density : kde.density) {
            group.density_values.push_back(peak > 0.0 ? density / peak : 0.0);
        }
        result.groups.push_back(std::move(group));
    }
    if (result.groups.empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "violin_no_groups",
                       "没有可绘制的小提琴分组。");
    }
    return result;
}

BarChartResult bar_chart_counts(
    const std::vector<std::string>& categories,
    const std::vector<double>* weights)
{
    BarChartResult result;
    std::map<std::string, double> totals;
    std::vector<std::string> order;
    for (std::size_t i = 0; i < categories.size(); ++i) {
        const std::string& category = categories[i];
        if (category.empty() || category == "*") {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                           "missing_bar_category",
                           "已忽略空类别或 *。");
            continue;
        }
        const double weight = weights == nullptr
            ? 1.0
            : (i < weights->size() ? (*weights)[i] : 1.0);
        if (!std::isfinite(weight) || weight < 0.0) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                           "invalid_bar_weight",
                           "已忽略非有限或负权重。");
            continue;
        }
        if (totals.find(category) == totals.end()) {
            order.push_back(category);
            totals[category] = 0.0;
        }
        totals[category] += weight;
    }
    for (const std::string& category : order) {
        result.categories.push_back(category);
        result.values.push_back(totals[category]);
        result.total_count += static_cast<std::size_t>(std::llround(totals[category]));
    }
    if (result.categories.empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "empty_bar_chart",
                       "没有可用于条形图的类别。");
    } else {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                       "bar_chart_not_pareto",
                       "通用条形保持出现顺序，不按计数排序，无累积百分比（与柏拉图分流）。");
    }
    return result;
}

}  // namespace datalab::domain::statistics
