#include "domain/statistics/taguchi_analyze.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <map>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

std::string ascii_lower(std::string text)
{
    for (char& ch : text) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return text;
}

double sample_variance(const std::vector<double>& y)
{
    if (y.size() < 2) {
        return 0.0;
    }
    const double mean =
        std::accumulate(y.begin(), y.end(), 0.0) / static_cast<double>(y.size());
    double sum_sq = 0.0;
    for (double value : y) {
        const double d = value - mean;
        sum_sq += d * d;
    }
    return sum_sq / static_cast<double>(y.size() - 1);
}

void assign_ranks(std::vector<TaguchiFactorResponse>& table)
{
    std::vector<std::size_t> order(table.size());
    for (std::size_t i = 0; i < order.size(); ++i) {
        order[i] = i;
    }
    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        if (table[a].delta != table[b].delta) {
            return table[a].delta > table[b].delta;
        }
        return a < b;
    });
    for (std::size_t rank = 0; rank < order.size(); ++rank) {
        table[order[rank]].rank = static_cast<int>(rank + 1);
    }
}

std::vector<TaguchiFactorResponse> build_response_table(
    const std::vector<TaguchiRunSummary>& runs,
    const std::vector<std::string>& factor_names,
    bool use_sn)
{
    std::vector<TaguchiFactorResponse> table;
    table.resize(factor_names.size());
    for (std::size_t f = 0; f < factor_names.size(); ++f) {
        table[f].factor_name = factor_names[f];
        std::map<std::string, std::pair<double, std::size_t>> sums;
        for (const auto& run : runs) {
            if (f >= run.factor_levels.size()) {
                continue;
            }
            double value = run.mean;
            if (use_sn) {
                if (!run.sn_ratio.has_value()) {
                    continue;
                }
                value = *run.sn_ratio;
            }
            auto& entry = sums[run.factor_levels[f]];
            entry.first += value;
            ++entry.second;
        }
        double min_avg = 0.0;
        double max_avg = 0.0;
        bool first = true;
        for (const auto& [level, pair] : sums) {
            TaguchiLevelAverage row;
            row.level = level;
            row.count = pair.second;
            row.average = pair.second == 0
                ? 0.0
                : pair.first / static_cast<double>(pair.second);
            if (first) {
                min_avg = row.average;
                max_avg = row.average;
                first = false;
            } else {
                min_avg = std::min(min_avg, row.average);
                max_avg = std::max(max_avg, row.average);
            }
            table[f].level_averages.push_back(std::move(row));
        }
        table[f].delta = first ? 0.0 : (max_avg - min_avg);
    }
    assign_ranks(table);
    return table;
}

}  // namespace

TaguchiSnType parse_taguchi_sn_type(const std::string& text)
{
    const std::string lower = ascii_lower(text);
    if (lower == "smaller" || lower == "smaller_is_better" || lower == "stb") {
        return TaguchiSnType::smaller_is_better;
    }
    if (lower == "nominal" || lower == "nominal_is_best" || lower == "ntb"
        || lower == "nominal_ii") {
        return TaguchiSnType::nominal_is_best;
    }
    return TaguchiSnType::larger_is_better;
}

std::string taguchi_sn_type_name(TaguchiSnType type)
{
    switch (type) {
    case TaguchiSnType::smaller_is_better:
        return "smaller";
    case TaguchiSnType::nominal_is_best:
        return "nominal";
    case TaguchiSnType::larger_is_better:
    default:
        return "larger";
    }
}

std::optional<double> sn_ratio_larger(const std::vector<double>& y)
{
    if (y.empty()) {
        return std::nullopt;
    }
    double sum_inv_sq = 0.0;
    for (double value : y) {
        if (!(std::isfinite(value)) || value == 0.0) {
            return std::nullopt;
        }
        sum_inv_sq += 1.0 / (value * value);
    }
    const double mean_inv_sq = sum_inv_sq / static_cast<double>(y.size());
    if (!(mean_inv_sq > 0.0) || !std::isfinite(mean_inv_sq)) {
        return std::nullopt;
    }
    return -10.0 * std::log10(mean_inv_sq);
}

std::optional<double> sn_ratio_smaller(const std::vector<double>& y)
{
    if (y.empty()) {
        return std::nullopt;
    }
    double sum_sq = 0.0;
    for (double value : y) {
        if (!std::isfinite(value)) {
            return std::nullopt;
        }
        sum_sq += value * value;
    }
    const double mean_sq = sum_sq / static_cast<double>(y.size());
    if (!(mean_sq > 0.0) || !std::isfinite(mean_sq)) {
        return std::nullopt;
    }
    return -10.0 * std::log10(mean_sq);
}

std::optional<double> sn_ratio_nominal(const std::vector<double>& y)
{
    if (y.size() < 2) {
        return std::nullopt;
    }
    for (double value : y) {
        if (!std::isfinite(value)) {
            return std::nullopt;
        }
    }
    const double s2 = sample_variance(y);
    if (!(s2 > 0.0) || !std::isfinite(s2)) {
        return std::nullopt;
    }
    return -10.0 * std::log10(s2);
}

std::optional<double> sn_ratio(const std::vector<double>& y, TaguchiSnType type)
{
    switch (type) {
    case TaguchiSnType::smaller_is_better:
        return sn_ratio_smaller(y);
    case TaguchiSnType::nominal_is_best:
        return sn_ratio_nominal(y);
    case TaguchiSnType::larger_is_better:
    default:
        return sn_ratio_larger(y);
    }
}

TaguchiAnalyzeResult analyze_taguchi_static(
    const std::vector<std::vector<std::string>>& factor_levels,
    const std::vector<std::vector<double>>& responses,
    const std::vector<std::string>& factor_names,
    const std::vector<std::size_t>& source_rows,
    const TaguchiAnalyzeOptions& options)
{
    TaguchiAnalyzeResult result;
    result.sn_type = options.sn_type;
    result.sn_type_name = taguchi_sn_type_name(options.sn_type);
    result.factor_names = factor_names;
    result.factor_count = factor_names.size();

    if (factor_names.empty()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "taguchi_analyze_no_factors",
            "Taguchi 分析至少需要一个控制因子列。"});
        return result;
    }
    if (factor_levels.empty() || responses.empty()
        || factor_levels.size() != responses.size()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "taguchi_analyze_empty",
            "Taguchi 分析需要对齐的因子水平与响应重复。"});
        return result;
    }

    result.response_count = 0;
    for (const auto& row : responses) {
        result.response_count = std::max(result.response_count, row.size());
    }
    if (result.response_count == 0) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "taguchi_analyze_no_responses",
            "请至少选择一个响应列（外阵重复）。"});
        return result;
    }

    for (std::size_t row = 0; row < factor_levels.size(); ++row) {
        if (factor_levels[row].size() != factor_names.size()) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "taguchi_analyze_factor_shape",
                "因子水平列数与因子名不一致。"});
            return result;
        }
        TaguchiRunSummary summary;
        summary.source_row =
            row < source_rows.size() ? source_rows[row] : row;
        summary.factor_levels = factor_levels[row];
        const auto& y = responses[row];
        if (y.empty()) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::warning, "taguchi_analyze_empty_run",
                "存在无有效响应的运行，已跳过。"});
            continue;
        }
        for (double value : y) {
            if (!std::isfinite(value)) {
                result.diagnostics.push_back({
                    DiagnosticMessage::Severity::error, "taguchi_analyze_bad_response",
                    "响应必须为有限数值（complete-case）。"});
                return result;
            }
        }
        summary.replicate_count = y.size();
        summary.mean =
            std::accumulate(y.begin(), y.end(), 0.0) / static_cast<double>(y.size());
        summary.sn_ratio = sn_ratio(y, options.sn_type);
        if (!summary.sn_ratio.has_value()) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::warning, "taguchi_analyze_sn_skip",
                "部分运行无法计算 S/N（例如 nominal 需 n≥2，或存在零响应）。"});
        }
        result.runs.push_back(std::move(summary));
    }

    result.run_count = result.runs.size();
    if (result.run_count == 0) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "taguchi_analyze_no_runs",
            "无有效运行可用于 Taguchi 静态分析。"});
        return result;
    }

    result.means_table = build_response_table(result.runs, factor_names, false);
    result.sn_table = build_response_table(result.runs, factor_names, true);
    return result;
}

}  // namespace datalab::domain::statistics
