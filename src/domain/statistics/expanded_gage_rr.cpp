#include "domain/statistics/expanded_gage_rr.h"

#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <set>

namespace datalab::domain::statistics {
namespace {

void add_error(std::vector<DiagnosticMessage>& d, const char* code, const char* message)
{
    d.push_back({DiagnosticMessage::Severity::error, code, message});
}

void add_warning(std::vector<DiagnosticMessage>& d, const char* code, const char* message)
{
    d.push_back({DiagnosticMessage::Severity::warning, code, message});
}

void add_info(std::vector<DiagnosticMessage>& d, const char* code, const char* message)
{
    d.push_back({DiagnosticMessage::Severity::info, code, message});
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

struct Key3 {
    std::string part;
    std::string oper;
    std::string add;
    bool operator<(const Key3& other) const
    {
        if (part != other.part) {
            return part < other.part;
        }
        if (oper != other.oper) {
            return oper < other.oper;
        }
        return add < other.add;
    }
};

double mean_of(const std::vector<double>& values)
{
    return std::accumulate(values.begin(), values.end(), 0.0)
        / static_cast<double>(values.size());
}

}  // namespace

ExpandedGageRrResult expanded_gage_rr_three_factor(
    const std::vector<double>& measurements,
    const std::vector<std::string>& parts,
    const std::vector<std::string>& operators,
    const std::vector<std::string>& additional,
    double tolerance,
    const std::string& additional_factor_name)
{
    ExpandedGageRrResult expanded;
    expanded.additional_factor_name = additional_factor_name.empty()
        ? "Additional" : additional_factor_name;
    expanded.three_factor_model = true;
    GageRrResult& result = expanded.gage;
    result.tolerance = tolerance;
    result.method = "expanded_3factor_anova";
    result.evidence.method_version = "1";
    result.evidence.assumption_status = "not_verified";
    result.design_balanced = true;

    if (measurements.size() != parts.size()
        || measurements.size() != operators.size()
        || measurements.size() != additional.size()) {
        add_error(result.diagnostics, "length_mismatch",
                  "测量值与因子列长度必须一致。");
        return expanded;
    }
    if (measurements.size() < 8) {
        add_error(result.diagnostics, "insufficient_data",
                  "三因子 Expanded Gage 需要足够观测。");
        return expanded;
    }

    std::map<Key3, std::vector<double>> cells;
    std::set<std::string> part_set;
    std::set<std::string> oper_set;
    std::set<std::string> add_set;
    for (std::size_t i = 0; i < measurements.size(); ++i) {
        if (!std::isfinite(measurements[i])) {
            add_error(result.diagnostics, "non_finite", "测量值必须有限。");
            return expanded;
        }
        Key3 key{parts[i], operators[i], additional[i]};
        cells[key].push_back(measurements[i]);
        part_set.insert(parts[i]);
        oper_set.insert(operators[i]);
        add_set.insert(additional[i]);
    }
    const std::size_t p = part_set.size();
    const std::size_t o = oper_set.size();
    const std::size_t a = add_set.size();
    expanded.additional_level_count = a;
    result.part_count = p;
    result.operator_count = o;
    if (p < 2 || o < 2 || a < 2) {
        add_error(result.diagnostics, "insufficient_levels",
                  "Part、Operator 与附加因子各自至少需要 2 个水平。");
        return expanded;
    }
    if (cells.size() != p * o * a) {
        result.design_balanced = false;
        add_error(result.diagnostics, "unbalanced_or_incomplete",
                  "本轮 Expanded 仅支持完整平衡交叉；缺失格子请用交叉 Gage 或后续 GLM。");
        return expanded;
    }
    std::size_t replicates = cells.begin()->second.size();
    for (const auto& [key, values] : cells) {
        if (values.size() != replicates) {
            result.design_balanced = false;
            add_error(result.diagnostics, "unbalanced_replicates",
                      "各格子重复次数必须相同。");
            return expanded;
        }
    }
    result.replicate_count = replicates;
    if (replicates < 1) {
        add_error(result.diagnostics, "no_replicates", "缺少重复。");
        return expanded;
    }

    const double grand = mean_of(measurements);

    auto level_mean = [&](auto pred) {
        std::map<std::string, std::vector<double>> buckets;
        for (std::size_t i = 0; i < measurements.size(); ++i) {
            buckets[pred(i)].push_back(measurements[i]);
        }
        std::map<std::string, double> means;
        for (const auto& [label, values] : buckets) {
            means[label] = mean_of(values);
        }
        return means;
    };
    const auto part_means = level_mean([&](std::size_t i) { return parts[i]; });
    const auto oper_means = level_mean([&](std::size_t i) { return operators[i]; });
    const auto add_means = level_mean([&](std::size_t i) { return additional[i]; });

    double ss_part = 0.0;
    for (const auto& [label, mean] : part_means) {
        (void)label;
        ss_part += static_cast<double>(o * a * replicates) * (mean - grand) * (mean - grand);
    }
    double ss_oper = 0.0;
    for (const auto& [label, mean] : oper_means) {
        (void)label;
        ss_oper += static_cast<double>(p * a * replicates) * (mean - grand) * (mean - grand);
    }
    double ss_add = 0.0;
    for (const auto& [label, mean] : add_means) {
        (void)label;
        ss_add += static_cast<double>(p * o * replicates) * (mean - grand) * (mean - grand);
    }

    std::map<std::pair<std::string, std::string>, std::vector<double>> po_cells;
    std::map<std::pair<std::string, std::string>, std::vector<double>> pa_cells;
    std::map<std::pair<std::string, std::string>, std::vector<double>> oa_cells;
    for (std::size_t i = 0; i < measurements.size(); ++i) {
        po_cells[{parts[i], operators[i]}].push_back(measurements[i]);
        pa_cells[{parts[i], additional[i]}].push_back(measurements[i]);
        oa_cells[{operators[i], additional[i]}].push_back(measurements[i]);
    }
    double ss_po = 0.0;
    for (const auto& [key, values] : po_cells) {
        const double cell = mean_of(values);
        ss_po += static_cast<double>(a * replicates)
            * (cell - part_means.at(key.first) - oper_means.at(key.second) + grand)
            * (cell - part_means.at(key.first) - oper_means.at(key.second) + grand);
    }
    double ss_pa = 0.0;
    for (const auto& [key, values] : pa_cells) {
        const double cell = mean_of(values);
        ss_pa += static_cast<double>(o * replicates)
            * (cell - part_means.at(key.first) - add_means.at(key.second) + grand)
            * (cell - part_means.at(key.first) - add_means.at(key.second) + grand);
    }
    double ss_oa = 0.0;
    for (const auto& [key, values] : oa_cells) {
        const double cell = mean_of(values);
        ss_oa += static_cast<double>(p * replicates)
            * (cell - oper_means.at(key.first) - add_means.at(key.second) + grand)
            * (cell - oper_means.at(key.first) - add_means.at(key.second) + grand);
    }

    double ss_poa = 0.0;
    double ss_error = 0.0;
    for (const auto& [key, values] : cells) {
        const double cell_mean = mean_of(values);
        const double po = mean_of(po_cells[{key.part, key.oper}]);
        const double pa = mean_of(pa_cells[{key.part, key.add}]);
        const double oa = mean_of(oa_cells[{key.oper, key.add}]);
        const double three_way = cell_mean - po - pa - oa
            + part_means.at(key.part) + oper_means.at(key.oper) + add_means.at(key.add)
            - grand;
        ss_poa += static_cast<double>(replicates) * three_way * three_way;
        for (const double value : values) {
            ss_error += (value - cell_mean) * (value - cell_mean);
        }
    }

    const double df_part = static_cast<double>(p - 1);
    const double df_oper = static_cast<double>(o - 1);
    const double df_add = static_cast<double>(a - 1);
    const double df_po = df_part * df_oper;
    const double df_pa = df_part * df_add;
    const double df_oa = df_oper * df_add;
    const double df_poa = df_part * df_oper * df_add;
    const double df_error = static_cast<double>(p * o * a * (replicates - 1));

    const bool use_pure_error = replicates >= 2 && df_error > 0.0;
    if (!use_pure_error) {
        add_warning(result.diagnostics, "three_way_as_error",
                    "重复次数=1：三阶交互用作重复性误差估计。");
    }

    auto push_anova = [&](const std::string& source, double df, double ss) {
        GageAnovaRow row;
        row.source = source;
        row.degrees_of_freedom = static_cast<std::size_t>(df);
        row.sum_of_squares = ss;
        row.mean_square = df > 0.0 ? ss / df : 0.0;
        result.anova_rows.push_back(row);
    };
    push_anova("Part", df_part, ss_part);
    push_anova("Operator", df_oper, ss_oper);
    push_anova(expanded.additional_factor_name, df_add, ss_add);
    push_anova("Part * Operator", df_po, ss_po);
    push_anova("Part * " + expanded.additional_factor_name, df_pa, ss_pa);
    push_anova("Operator * " + expanded.additional_factor_name, df_oa, ss_oa);
    push_anova("Part * Operator * " + expanded.additional_factor_name, df_poa, ss_poa);
    if (use_pure_error) {
        push_anova("Repeatability", df_error, ss_error);
    }

    const double ms_part = ss_part / df_part;
    const double ms_oper = ss_oper / df_oper;
    const double ms_add = ss_add / df_add;
    const double ms_po = ss_po / df_po;
    const double ms_pa = ss_pa / df_pa;
    const double ms_oa = ss_oa / df_oa;
    const double ms_poa = ss_poa / df_poa;
    const double ms_e = use_pure_error ? (ss_error / df_error) : ms_poa;

    const double r = static_cast<double>(replicates);
    double vc_e = ms_e;
    double vc_poa = use_pure_error ? (ms_poa - ms_e) / r : 0.0;
    const double ms_int_ref = use_pure_error ? ms_poa : ms_e;
    double vc_po = (ms_po - ms_int_ref) / (static_cast<double>(a) * r);
    double vc_pa = (ms_pa - ms_int_ref) / (static_cast<double>(o) * r);
    double vc_oa = (ms_oa - ms_int_ref) / (static_cast<double>(p) * r);
    double vc_part = (ms_part - ms_po - ms_pa + ms_int_ref)
        / (static_cast<double>(o * a) * r);
    double vc_oper = (ms_oper - ms_po - ms_oa + ms_int_ref)
        / (static_cast<double>(p * a) * r);
    double vc_add = (ms_add - ms_pa - ms_oa + ms_int_ref)
        / (static_cast<double>(p * o) * r);

    if (!use_pure_error) {
        vc_e = ms_poa;  // three-way as repeatability
        vc_poa = 0.0;
    }

    const double reproducibility = std::max(0.0, vc_oper) + std::max(0.0, vc_add)
        + std::max(0.0, vc_po) + std::max(0.0, vc_pa) + std::max(0.0, vc_oa)
        + std::max(0.0, vc_poa);
    const double repeatability = std::max(0.0, vc_e);
    const double gage_rr = repeatability + reproducibility;
    const double part_var = std::max(0.0, vc_part);
    const double total = gage_rr + part_var;
    result.negative_variance_truncated =
        vc_e < 0.0 || vc_poa < 0.0 || vc_po < 0.0 || vc_pa < 0.0 || vc_oa < 0.0
        || vc_part < 0.0 || vc_oper < 0.0 || vc_add < 0.0;

    result.variance_components.push_back(make_component(
        "Repeatability", vc_e, total, tolerance, result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        "Operator", vc_oper, total, tolerance, result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        expanded.additional_factor_name, vc_add, total, tolerance,
        result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        "Part * Operator", vc_po, total, tolerance, result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        "Part * " + expanded.additional_factor_name, vc_pa, total, tolerance,
        result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        "Operator * " + expanded.additional_factor_name, vc_oa, total, tolerance,
        result.study_var_multiplier, result.diagnostics));
    if (use_pure_error) {
        result.variance_components.push_back(make_component(
            "Part * Operator * " + expanded.additional_factor_name, vc_poa, total, tolerance,
            result.study_var_multiplier, result.diagnostics));
    }
    result.variance_components.push_back(make_component(
        "Reproducibility", reproducibility, total, tolerance,
        result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        "Total Gage R&R", gage_rr, total, tolerance,
        result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        "Part-To-Part", vc_part, total, tolerance,
        result.study_var_multiplier, result.diagnostics));
    result.variance_components.push_back(make_component(
        "Total Variation", total, total, tolerance,
        result.study_var_multiplier, result.diagnostics));

    if (gage_rr > 0.0) {
        result.ndc = std::floor(1.41 * std::sqrt(part_var / gage_rr));
        if (result.ndc < 1.0) {
            result.ndc = 1.0;
        }
        result.ndc_available = true;
    }
    add_info(result.diagnostics, "expanded_scoped_3factor",
             "本命令为平衡三因子随机 Expanded；不平衡/固定效应/嵌套 GLM 仍延后。");
    result.evidence.valid_count = measurements.size();
    return expanded;
}

}  // namespace datalab::domain::statistics
