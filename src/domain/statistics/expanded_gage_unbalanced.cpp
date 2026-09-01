#include "domain/statistics/expanded_gage_unbalanced.h"

#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <set>

namespace datalab::domain::statistics {
namespace {

void add_error(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

void add_warning(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::warning, code, message});
}

GageVarianceComponent make_component(
    const std::string& source,
    double raw_variance,
    double total,
    double tolerance,
    double multiplier,
    std::vector<DiagnosticMessage>& diagnostics)
{
    GageVarianceComponent component;
    component.source = source;
    component.raw_variance_component = raw_variance;
    component.truncated = raw_variance < 0.0;
    component.variance_component = std::max(0.0, raw_variance);
    if (component.truncated) {
        add_warning(diagnostics, "negative_variance_component",
                    "方差分量原始估计为负，已截断为 0。");
    }
    component.standard_deviation = std::sqrt(component.variance_component);
    component.percent_contribution = total > 0.0
        ? component.variance_component / total * 100.0 : 0.0;
    component.study_variation = multiplier * component.standard_deviation;
    component.percent_study_variation = total > 0.0
        ? component.standard_deviation / std::sqrt(total) * 100.0 : 0.0;
    if (std::isfinite(tolerance) && tolerance > 0.0) {
        component.percent_tolerance_available = true;
        component.percent_tolerance = component.study_variation / tolerance * 100.0;
    }
    return component;
}

struct Cell2 {
    std::vector<double> values;
};

double harmonic_mean(const std::vector<std::size_t>& counts)
{
    if (counts.empty()) {
        return 0.0;
    }
    double sum_recip = 0.0;
    for (const std::size_t count : counts) {
        if (count == 0) {
            return 0.0;
        }
        sum_recip += 1.0 / static_cast<double>(count);
    }
    return static_cast<double>(counts.size()) / sum_recip;
}

}  // namespace

ExpandedGageUnbalancedResult expanded_gage_unbalanced_analyze(
    const std::vector<double>& measurements,
    const std::vector<std::string>& parts,
    const std::vector<std::string>& operators,
    const std::vector<std::string>& additional,
    double tolerance,
    const std::vector<std::size_t>& source_rows,
    const ExpandedGageUnbalancedOptions& options)
{
    ExpandedGageUnbalancedResult result;
    result.tolerance = tolerance;
    result.has_additional_factor = options.include_additional_factor;

    if (measurements.size() < 4 || measurements.size() != parts.size()
        || measurements.size() != operators.size()) {
        add_error(result.diagnostics, "invalid_gage_shape",
                  "测量值、Part 与 Operator 列长度须一致且至少 4 条。");
        return result;
    }
    if (options.include_additional_factor
        && additional.size() != measurements.size()) {
        add_error(result.diagnostics, "additional_length_mismatch",
                  "启用附加因子时附加列长度须与测量列一致。");
        return result;
    }

    std::map<std::string, std::size_t> part_index;
    std::map<std::string, std::size_t> operator_index;
    std::map<std::string, std::size_t> additional_index;
    std::map<std::pair<std::size_t, std::size_t>, Cell2> cells2;
    std::map<std::tuple<std::size_t, std::size_t, std::size_t>, Cell2> cells3;

    for (std::size_t index = 0; index < measurements.size(); ++index) {
        if (!std::isfinite(measurements[index]) || parts[index].empty()
            || operators[index].empty()) {
            continue;
        }
        if (options.include_additional_factor && additional[index].empty()) {
            continue;
        }
        part_index.emplace(parts[index], part_index.size());
        operator_index.emplace(operators[index], operator_index.size());
        const std::size_t p = part_index.at(parts[index]);
        const std::size_t o = operator_index.at(operators[index]);
        if (options.include_additional_factor) {
            additional_index.emplace(additional[index], additional_index.size());
            const std::size_t a = additional_index.at(additional[index]);
            cells3[{p, o, a}].values.push_back(measurements[index]);
        } else {
            cells2[{p, o}].values.push_back(measurements[index]);
        }
        const std::size_t source = index < source_rows.size() ? source_rows[index] : index;
        result.observation_source_rows.push_back(source);
    }

    result.observation_count = result.observation_source_rows.size();
    result.part_count = part_index.size();
    result.operator_count = operator_index.size();
    result.additional_level_count = additional_index.size();

    if (result.part_count < 2 || result.operator_count < 2) {
        add_error(result.diagnostics, "insufficient_gage_levels",
                  "Part 与 Operator 各自至少需要 2 个水平。");
        return result;
    }
    if (options.include_additional_factor && result.additional_level_count < 2) {
        add_error(result.diagnostics, "insufficient_additional_levels",
                  "附加因子至少需要 2 个水平。");
        return result;
    }
    if (result.observation_count < 4) {
        add_error(result.diagnostics, "insufficient_observations",
                  "complete-case 后有效观测不足。");
        return result;
    }

    if (options.include_additional_factor) {
        for (const auto& [key, cell] : cells3) {
            const auto [p, o, a] = key;
            auto& merged = cells2[{p, o}];
            merged.values.insert(
                merged.values.end(), cell.values.cbegin(), cell.values.cend());
        }
        add_warning(result.diagnostics, "additional_merged_for_varcomp",
                    "附加因子水平已并入 Part×Operator 单元估计方差分量（窄化）。");
    }

    std::vector<std::size_t> replicate_counts;
    for (const auto& [key, cell] : cells2) {
        replicate_counts.push_back(cell.values.size());
    }
    const std::size_t min_rep = *std::min_element(
        replicate_counts.cbegin(), replicate_counts.cend());
    const std::size_t max_rep = *std::max_element(
        replicate_counts.cbegin(), replicate_counts.cend());
    if (min_rep < 1) {
        add_error(result.diagnostics, "empty_cell",
                  "存在无重复的 Part×Operator 单元。");
        return result;
    }
    if (min_rep != max_rep) {
        result.design_balanced = false;
        add_warning(result.diagnostics, "unbalanced_replicates",
                    "不平衡重复；使用调和平均重复数估计方差分量。");
    }

    const double n_h = harmonic_mean(replicate_counts);
    const double grand_mean = std::accumulate(
        measurements.cbegin(), measurements.cend(), 0.0)
        / static_cast<double>(measurements.size());

    std::vector<double> part_means(result.part_count, 0.0);
    std::vector<double> operator_means(result.operator_count, 0.0);
    std::vector<std::vector<double>> cell_means(
        result.part_count, std::vector<double>(result.operator_count, 0.0));
    double ss_repeatability = 0.0;
    std::size_t df_repeatability = 0;

    for (const auto& [key, cell] : cells2) {
        const auto [p, o] = key;
        const double cell_mean = std::accumulate(
            cell.values.cbegin(), cell.values.cend(), 0.0)
            / static_cast<double>(cell.values.size());
        cell_means[p][o] = cell_mean;
        part_means[p] += cell_mean;
        operator_means[o] += cell_mean;
        for (const double value : cell.values) {
            ss_repeatability += (value - cell_mean) * (value - cell_mean);
        }
        if (cell.values.size() > 1) {
            df_repeatability += cell.values.size() - 1;
        }
    }

    for (double& mean : part_means) {
        mean /= static_cast<double>(result.operator_count);
    }
    for (double& mean : operator_means) {
        mean /= static_cast<double>(result.part_count);
    }

    {
        double ss_part = 0.0;
        for (std::size_t p = 0; p < result.part_count; ++p) {
            ss_part += static_cast<double>(result.operator_count * min_rep)
                * (part_means[p] - grand_mean) * (part_means[p] - grand_mean);
        }
        double ss_operator = 0.0;
        for (std::size_t o = 0; o < result.operator_count; ++o) {
            ss_operator += static_cast<double>(result.part_count * min_rep)
                * (operator_means[o] - grand_mean) * (operator_means[o] - grand_mean);
        }
        double ss_interaction = 0.0;
        for (const auto& [key, cell] : cells2) {
            const auto [p, o] = key;
            const double interaction = cell_means[p][o]
                - part_means[p] - operator_means[o] + grand_mean;
            ss_interaction += static_cast<double>(cell.values.size())
                * interaction * interaction;
        }

        const std::size_t df_part = result.part_count - 1;
        const std::size_t df_operator = result.operator_count - 1;
        const std::size_t df_interaction = df_part * df_operator;
        const double ms_part = df_part > 0 ? ss_part / static_cast<double>(df_part) : 0.0;
        const double ms_operator = df_operator > 0
            ? ss_operator / static_cast<double>(df_operator) : 0.0;
        const double ms_interaction = df_interaction > 0
            ? ss_interaction / static_cast<double>(df_interaction) : 0.0;
        const double ms_repeatability = df_repeatability > 0
            ? ss_repeatability / static_cast<double>(df_repeatability) : 0.0;

        const double f_part = ms_interaction > 0.0 ? ms_part / ms_interaction : 0.0;
        const double f_operator = ms_interaction > 0.0 ? ms_operator / ms_interaction : 0.0;
        const double f_interaction = ms_repeatability > 0.0
            ? ms_interaction / ms_repeatability : 0.0;

        result.anova_rows = {
            {"Part", df_part, ss_part, ms_part, f_part,
             ms_interaction > 0.0
                 ? std::optional<double>(f_right_tail(
                       f_part, static_cast<double>(df_part),
                       static_cast<double>(df_interaction)))
                 : std::nullopt},
            {"Operator", df_operator, ss_operator, ms_operator, f_operator,
             ms_interaction > 0.0
                 ? std::optional<double>(f_right_tail(
                       f_operator, static_cast<double>(df_operator),
                       static_cast<double>(df_interaction)))
                 : std::nullopt},
            {"Part * Operator", df_interaction, ss_interaction, ms_interaction,
             f_interaction,
             ms_repeatability > 0.0
                 ? std::optional<double>(f_right_tail(
                       f_interaction, static_cast<double>(df_interaction),
                       static_cast<double>(df_repeatability)))
                 : std::nullopt},
            {"Repeatability", df_repeatability, ss_repeatability, ms_repeatability,
             0.0, std::nullopt}};

        const double k = n_h > 0.0 ? n_h : static_cast<double>(min_rep);
        const double raw_repeatability = ms_repeatability;
        const double raw_interaction = (ms_interaction - ms_repeatability) / k;
        const double raw_operator = (ms_operator - ms_interaction)
            / static_cast<double>(result.part_count * static_cast<std::size_t>(k));
        const double raw_part = (ms_part - ms_interaction)
            / static_cast<double>(result.operator_count * static_cast<std::size_t>(k));
        const double reproducibility = std::max(0.0, raw_interaction)
            + std::max(0.0, raw_operator);
        const double gage_rr = std::max(0.0, raw_repeatability) + reproducibility;
        const double part_variance = std::max(0.0, raw_part);
        const double total = gage_rr + part_variance;

        result.variance_components.push_back(make_component(
            "Repeatability", raw_repeatability, total, tolerance,
            options.study_var_multiplier, result.diagnostics));
        result.variance_components.push_back(make_component(
            "Operator", raw_operator, total, tolerance,
            options.study_var_multiplier, result.diagnostics));
        result.variance_components.push_back(make_component(
            "Operator * Part", raw_interaction, total, tolerance,
            options.study_var_multiplier, result.diagnostics));
        result.variance_components.push_back(make_component(
            "Reproducibility", reproducibility, total, tolerance,
            options.study_var_multiplier, result.diagnostics));
        result.variance_components.push_back(make_component(
            "Total Gage R&R", gage_rr, total, tolerance,
            options.study_var_multiplier, result.diagnostics));
        result.variance_components.push_back(make_component(
            "Part-To-Part", raw_part, total, tolerance,
            options.study_var_multiplier, result.diagnostics));
        result.variance_components.push_back(make_component(
            "Total Variation", total, total, tolerance,
            options.study_var_multiplier, result.diagnostics));

        if (gage_rr > 0.0) {
            result.ndc = std::max(
                1.0,
                std::floor(std::sqrt(2.0 * part_variance / gage_rr)));
            result.ndc_available = true;
            result.gage_rr_percent_study_var = std::sqrt(gage_rr / total) * 100.0;
        } else {
            add_error(result.diagnostics, "not_estimable",
                      "Gage R&R 方差为 0，NDC 不可估计。");
        }
    }

    return result;
}

}  // namespace datalab::domain::statistics
