#!/usr/bin/env python3
"""Generate G9 computation_trace_attach.cpp/.h and coverage matrix."""

from __future__ import annotations

import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

EXEMPT = {"tests", "rule_policy"}

# D-class pure graphs → display_summary (25)
D_CLASS = {
    "histogram",
    "eda_4plot",
    "boxplot",
    "pareto",
    "run_chart",
    "cause_and_effect",
    "density_plot",
    "hexbin_plot",
    "violin_plot",
    "bar_chart",
    "scatter_plot",
    "interval_plot",
    "correlation_plot",
    "bubble_plot",
    "probability_plot",
    "ecdf_plot",
    "matrix_plot",
    "marginal_plot",
    "parallel_plot",
    "heatmap_plot",
    "time_series_plot",
    "area_plot",
    "contour_plot",
    "pie_plot",
    "variability_chart",
}

# B-class design / generation (include steps)
B_CLASS = {
    "doe_plackett_burman",
    "taguchi_orthogonal_design",
    "doe_ccd",
    "doe_bbd",
    "doe_factorial",
    "doe_response",
    "rsm_response",
    "response_optimization",
    "acceptance_sampling",
    "distribution_identification",
    "box_cox",
    "multi_vari",
    "anom",
    "time_series_smoothing",
    "arima",
    "time_series_decomposition",
    "seasonal_forecasting",
    "acf_pacf",
    "ccf",
    "correlogram",
    "km_interval",
    "bootstrap_mean",
    "bootstrap_two_sample",
    "cluster_observations",
}

# C-class key equation + dim summary
C_CLASS = {
    "distribution_calculator",
    "t_power",
    "pca",
    "kmeans",
    "cart_tree",
    "random_forest",
    "isolation_forest",
    "discriminant",
    "hotelling_t2",
}

FAMILY = {
    "capability": "能力/质量",
    "binomial_capability": "能力/质量",
    "poisson_capability": "能力/质量",
    "nonnormal_capability": "能力/质量",
    "capability_sixpack": "能力/质量",
    "between_within_capability": "能力/质量",
    "batch_capability": "能力/质量",
    "nonparametric_capability": "能力/质量",
    "box_cox": "能力/质量",
    "tolerance_intervals": "能力/质量",
    "acceptance_sampling": "能力/质量",
    "distribution_identification": "能力/质量",
    "multi_vari": "能力/质量",
    "variability_chart": "能力/质量",
    "imr": "控制图",
    "xbar_r": "控制图",
    "xbar_s": "控制图",
    "imr_rs": "控制图",
    "p_chart": "控制图",
    "np_chart": "控制图",
    "c_chart": "控制图",
    "u_chart": "控制图",
    "laney_p_chart": "控制图",
    "laney_u_chart": "控制图",
    "ewma": "控制图",
    "hotelling_t2": "控制图",
    "mewma": "控制图",
    "generalized_variance": "控制图",
    "cusum": "控制图",
    "zone_chart": "控制图",
    "z_mr": "控制图",
    "moving_average": "控制图",
    "g_chart": "控制图",
    "t_chart": "控制图",
    "descriptive": "基础统计",
    "normality_test": "基础统计",
    "outlier_test": "基础统计",
    "correlation": "基础统计",
    "one_sample_t": "基础统计",
    "one_sample_z": "基础统计",
    "one_proportion": "基础统计",
    "one_poisson_rate": "基础统计",
    "two_poisson_rate": "基础统计",
    "two_sample_t": "基础统计",
    "one_sample_equivalence": "基础统计",
    "two_sample_equivalence": "基础统计",
    "two_sample_equivalence_ratio": "基础统计",
    "paired_equivalence": "基础统计",
    "one_proportion_equivalence": "基础统计",
    "two_proportion_equivalence": "基础统计",
    "one_way_anova": "基础统计",
    "paired_t": "基础统计",
    "two_proportions": "基础统计",
    "chi_square": "基础统计",
    "cross_tabulation": "基础统计",
    "chi_square_gof": "基础统计",
    "poisson_gof": "基础统计",
    "anom": "基础统计",
    "mann_whitney": "基础统计",
    "wilcoxon_signed_rank": "基础统计",
    "sign_test": "基础统计",
    "runs_test": "基础统计",
    "mcnemar": "基础统计",
    "fisher_exact": "基础统计",
    "cochran_q": "基础统计",
    "mood_median": "基础统计",
    "kruskal_wallis": "基础统计",
    "friedman": "基础统计",
    "two_factor_anova": "基础统计",
    "variance_test": "基础统计",
    "regression": "回归/多变量/ML",
    "logistic_regression": "回归/多变量/ML",
    "poisson_regression": "回归/多变量/ML",
    "ordinal_logistic": "回归/多变量/ML",
    "nominal_logistic": "回归/多变量/ML",
    "stepwise_regression": "回归/多变量/ML",
    "best_subsets_regression": "回归/多变量/ML",
    "pca": "回归/多变量/ML",
    "kmeans": "回归/多变量/ML",
    "cart_tree": "回归/多变量/ML",
    "random_forest": "回归/多变量/ML",
    "isolation_forest": "回归/多变量/ML",
    "discriminant": "回归/多变量/ML",
    "cluster_observations": "回归/多变量/ML",
    "time_series_smoothing": "回归/多变量/ML",
    "arima": "回归/多变量/ML",
    "time_series_decomposition": "回归/多变量/ML",
    "seasonal_forecasting": "回归/多变量/ML",
    "adf_test": "回归/多变量/ML",
    "ccf": "回归/多变量/ML",
    "correlogram": "回归/多变量/ML",
    "acf_pacf": "回归/多变量/ML",
    "reliability": "可靠性",
    "accelerated_life": "可靠性",
    "reliability_warranty": "可靠性",
    "cox_regression": "可靠性",
    "weibayes": "可靠性",
    "probit_reliability": "可靠性",
    "km_interval": "可靠性",
    "doe_plackett_burman": "DOE",
    "taguchi_orthogonal_design": "DOE",
    "doe_ccd": "DOE",
    "doe_bbd": "DOE",
    "doe_factorial": "DOE",
    "doe_response": "DOE",
    "rsm_response": "DOE",
    "response_optimization": "DOE",
    "gage_rr": "MSA",
    "emp_crossed": "MSA",
    "expanded_gage_rr": "MSA",
    "msa_type1": "MSA",
    "nested_gage_rr": "MSA",
    "attribute_agreement": "MSA",
    "distribution_calculator": "图形/工具",
    "t_power": "图形/工具",
    "bootstrap_mean": "图形/工具",
    "bootstrap_two_sample": "图形/工具",
    "tests": "元命令",
    "rule_policy": "元命令",
}

FORMULAS = {
    "capability": (
        "cpk_min_usl_lsl",
        "过程能力 Cpk",
        "Cpk = min( (USL-μ)/(3σ), (μ-LSL)/(3σ) )",
        "Cpk",
    ),
    "one_sample_t": (
        "one_sample_t_statistic",
        "单样本 t 检验",
        "t = (x̄ - μ₀) / (s / √n)",
        "t",
    ),
    "weibayes": (
        "weibayes_scale",
        "Weibayes 尺度 η",
        "η = ( Σ t_i^β / r )^(1/β)  （零失效时用边界）",
        "η",
    ),
    "descriptive": (
        "descriptive_mean",
        "描述性统计均值",
        "x̄ = Σx_i / n",
        "x̄",
    ),
    "histogram": (
        "histogram_display",
        "直方图显示摘要",
        "频数分箱展示（complete-case）",
        "N",
    ),
}


def list_command_ids() -> list[str]:
    out = subprocess.check_output(
        [sys.executable, str(ROOT / "tools/_list_command_ids.py")],
        cwd=ROOT,
        text=True,
        encoding="utf-8",
    )
    ids: list[str] = []
    started = False
    for line in out.splitlines():
        if line.startswith("count="):
            started = True
            continue
        if started and line.strip():
            ids.append(line.strip())
    return ids


def family_of(cid: str) -> str:
    if cid in FAMILY:
        return FAMILY[cid]
    if cid.endswith("_plot") or cid in D_CLASS:
        return "图形/工具"
    if "capability" in cid:
        return "能力/质量"
    if cid.endswith("_chart") or cid in {
        "imr",
        "ewma",
        "cusum",
        "mewma",
        "hotelling_t2",
        "generalized_variance",
        "z_mr",
        "moving_average",
        "imr_rs",
        "xbar_r",
        "xbar_s",
    }:
        return "控制图"
    if "gage" in cid or "msa" in cid or "attribute_agreement" in cid or "emp_" in cid:
        return "MSA"
    if cid.startswith("doe_") or "taguchi" in cid or "rsm" in cid:
        return "DOE"
    if any(
        k in cid
        for k in (
            "reliability",
            "weibayes",
            "cox",
            "probit",
            "km_interval",
            "accelerated",
            "warranty",
        )
    ):
        return "可靠性"
    if any(
        k in cid
        for k in (
            "regression",
            "logistic",
            "pca",
            "kmeans",
            "forest",
            "cart",
            "discriminant",
            "cluster",
            "arima",
            "forecast",
            "acf",
            "adf",
        )
    ):
        return "回归/多变量/ML"
    return "基础统计"


def status_of(cid: str) -> str:
    if cid in EXEMPT:
        return "豁免"
    if cid in D_CLASS:
        return "display_summary"
    return "实质绑定"


def evidence_of(cid: str) -> str:
    if cid in D_CLASS:
        return "display_summary"
    return "formula_reference"


def formula_meta(cid: str) -> tuple[str, str, str, str]:
    if cid in FORMULAS:
        return FORMULAS[cid]
    fid = f"{cid}_main"
    title = cid.replace("_", " ")
    if cid in D_CLASS:
        return fid, f"{title} 显示摘要", "显示摘要：有效 N（complete-case）", "N"
    if cid in B_CLASS:
        return fid, f"{title} 生成规则", f"{cid} 设计/生成主规则", "runs"
    if cid in C_CLASS:
        return fid, f"{title} 关键方程", f"{cid} 关键方程（维数摘要）", "result"
    return fid, f"{title} 主公式", f"{cid} 主公式", "result"


def cpp_escape(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def write_header() -> None:
    path = ROOT / "src/application/computation_trace_attach.h"
    path.write_text(
        """#pragma once

#include "domain/quality_types.h"

#include <string>

namespace datalab::application {

// Attach G9 computation traces for a finished OutputPage.
// No-op when command_id is empty or E-exempt (tests / rule_policy).
void attach_computation_traces(
    datalab::domain::OutputPage& page, const std::string& command_id);

// Resolve analysis_commands id from page.analysis_command_id, page.id prefix, or id itself.
std::string resolve_command_id_from_page(const datalab::domain::OutputPage& page);

}  // namespace datalab::application
""",
        encoding="utf-8",
    )


def emit_generic_case(cid: str) -> str:
    fid, title, plain, result_sym = formula_meta(cid)
    evidence = evidence_of(cid)
    lines = [
        f'    if (command_id == "{cid}") {{',
        f'        ComputationTrace tr;',
        f'        tr.command_id = "{cid}";',
        f'        tr.formula_id = "{fid}";',
        f'        tr.title = "{cpp_escape(title)}";',
        f'        tr.plain_formula = "{cpp_escape(plain)}";',
        f'        tr.evidence_type = "{evidence}";',
        f'        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";',
        f'        tr.result_symbol = "{result_sym}";',
    ]
    if evidence == "display_summary":
        lines += [
            '        const std::string n = first_nonempty({',
            '            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),',
            '            table_value(page, "N"),',
            '            table_value(page, "有效 N"),',
            '            "见结果表"});',
            '        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));',
            '        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));',
            '        tr.result_value = n;',
            '        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";',
        ]
    elif cid in B_CLASS:
        lines += [
            '        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));',
            '        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));',
            '        tr.steps.push_back({"选择阵列/设计类型"});',
            '        tr.steps.push_back({"按水平与因子生成运行表"});',
            '        tr.steps.push_back({"导出工作表或分析模型"});',
            '        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});',
            f'        tr.substituted_text = "{cpp_escape(plain)} → " + tr.result_value;',
        ]
    elif cid in C_CLASS:
        lines += [
            '        tr.bindings.push_back(bind("dim", "维数/规模", first_nonempty({table_value(page, "N", "p", "k", "Components", "Trees"), "见结果表"}), "input"));',
            '        tr.bindings.push_back(bind("key", "关键量", first_nonempty({table_value(page, "Result", "Statistic", "R²", "Inertia"), "见结果表"}), "result"));',
            '        tr.result_value = first_nonempty({table_value(page, "Result", "Statistic"), "见结果表"});',
            f'        tr.substituted_text = "{cpp_escape(plain)}（维数摘要，非全矩阵） → " + tr.result_value;',
        ]
    else:
        lines += [
            '        tr.bindings.push_back(bind("n", "样本量 N", first_nonempty({table_value(page, "N", "n", "有效 N"), "见结果表"}), "input"));',
            '        tr.bindings.push_back(bind("stat", "统计量", first_nonempty({table_value(page, "Statistic", "统计量", "T", "Z", "F", "Chi-Sq", "P"), "见结果表"}), "intermediate"));',
            '        tr.bindings.push_back(bind("p", "P 值", first_nonempty({table_value(page, "P", "P-Value", "P 值"), "见结果表"}), "result"));',
            '        tr.result_value = first_nonempty({table_value(page, "P", "P-Value", "Statistic", "统计量"), "见结果表"});',
            f'        tr.substituted_text = "{cpp_escape(plain)} → " + tr.result_value;',
        ]
    lines += [
        "        page.computation_traces.push_back(std::move(tr));",
        "        return;",
        "    }",
        "",
    ]
    return "\n".join(lines)


def emit_capability() -> str:
    return r'''    if (command_id == "capability") {
        ComputationTrace tr;
        tr.command_id = "capability";
        tr.formula_id = "cpk_min_usl_lsl";
        tr.title = "过程能力 Cpk";
        tr.plain_formula = "Cpk = min( (USL-μ)/(3σ), (μ-LSL)/(3σ) )";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Process_capability_index";
        tr.result_symbol = "Cpk";
        const auto& cfg = page.configuration.specifications;
        const std::string usl = cfg.upper.has_value() ? fmt_num(*cfg.upper)
            : first_nonempty({table_value(page, "USL"), "见结果表"});
        const std::string lsl = cfg.lower.has_value() ? fmt_num(*cfg.lower)
            : first_nonempty({table_value(page, "LSL"), "见结果表"});
        const std::string mu = first_nonempty({
            table_value(page, "Mean", "均值", "Process Mean"),
            "见结果表"});
        const std::string sigma = first_nonempty({
            table_value(page, "Within", "StDev", "σ", "Sigma", "标准差"),
            "见结果表"});
        std::string cpk = "见结果表";
        if (page.facts.capability.has_value() && page.facts.capability->cpk.has_value()) {
            cpk = fmt_num(*page.facts.capability->cpk);
        } else {
            cpk = first_nonempty({table_value(page, "Cpk"), "见结果表"});
        }
        tr.bindings.push_back(bind("USL", "规格上限", usl, "input"));
        tr.bindings.push_back(bind("LSL", "规格下限", lsl, "input"));
        tr.bindings.push_back(bind("μ", "过程均值", mu, "input"));
        tr.bindings.push_back(bind("σ", "组内标准差", sigma, "input"));
        tr.bindings.push_back(bind("Cpk", "过程能力指数", cpk, "result"));
        tr.result_value = cpk;
        tr.substituted_text = "Cpk = min( (" + usl + "-" + mu + ")/(3·" + sigma
            + "), (" + mu + "-" + lsl + ")/(3·" + sigma + ") ) = " + cpk;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
'''


def emit_one_sample_t() -> str:
    return r'''    if (command_id == "one_sample_t") {
        ComputationTrace tr;
        tr.command_id = "one_sample_t";
        tr.formula_id = "one_sample_t_statistic";
        tr.title = "单样本 t 检验";
        tr.plain_formula = "t = (x̄ - μ₀) / (s / √n)";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Student%27s_t-test";
        tr.result_symbol = "t";
        std::string n = "见结果表";
        std::string xbar = "见结果表";
        std::string s = first_nonempty({table_value(page, "StDev", "标准差", "s", "SE Mean"), "见结果表"});
        std::string mu0 = page.configuration.inference.hypothesis_mean.has_value()
            ? fmt_num(*page.configuration.inference.hypothesis_mean)
            : first_nonempty({table_value(page, "假设均值", "Hypothesized Mean", "μ0"), "见结果表"});
        std::string tstat = first_nonempty({table_value(page, "T", "t", "Statistic"), "见结果表"});
        if (page.facts.t_test.has_value()) {
            n = std::to_string(page.facts.t_test->n);
            if (page.facts.t_test->mean.has_value()) {
                xbar = fmt_num(*page.facts.t_test->mean);
            }
            if (page.facts.t_test->sample_standard_deviation.has_value()) {
                s = fmt_num(*page.facts.t_test->sample_standard_deviation);
            }
        } else {
            n = first_nonempty({table_value(page, "N", "n"), n});
            xbar = first_nonempty({table_value(page, "Mean", "均值"), xbar});
        }
        tr.bindings.push_back(bind("n", "样本量", n, "input"));
        tr.bindings.push_back(bind("x̄", "样本均值", xbar, "input"));
        tr.bindings.push_back(bind("s", "样本标准差", s, "input"));
        tr.bindings.push_back(bind("μ₀", "假设均值", mu0, "input"));
        tr.bindings.push_back(bind("t", "t 统计量", tstat, "result"));
        tr.result_value = tstat;
        tr.substituted_text = "t = (" + xbar + " - " + mu0 + ") / (" + s + " / √" + n
            + ") = " + tstat;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
'''


def emit_weibayes() -> str:
    return r'''    if (command_id == "weibayes") {
        ComputationTrace tr;
        tr.command_id = "weibayes";
        tr.formula_id = "weibayes_scale";
        tr.title = "Weibayes 尺度 η";
        tr.plain_formula = "η = ( Σ t_i^β / r )^(1/β)";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Weibull_distribution";
        tr.result_symbol = "η";
        std::string beta = fmt_num(page.configuration.weibayes.shape_prior);
        std::string r = "见结果表";
        std::string eta = "见结果表";
        if (page.facts.weibayes.has_value()) {
            beta = fmt_num(page.facts.weibayes->shape_prior);
            r = std::to_string(page.facts.weibayes->failure_count);
            if (page.facts.weibayes->scale.has_value()) {
                eta = fmt_num(*page.facts.weibayes->scale);
            }
        } else {
            r = first_nonempty({table_value(page, "Failures (r)", "Failures", "r"), r});
            eta = first_nonempty({table_value(page, "Scale η", "Scale", "η"), eta});
            beta = first_nonempty({table_value(page, "Shape β (prior)", "Shape", "β"), beta});
        }
        tr.bindings.push_back(bind("β", "形状先验", beta, "input"));
        tr.bindings.push_back(bind("r", "失效数", r, "input"));
        tr.bindings.push_back(bind("η", "尺度参数", eta, "result"));
        tr.result_value = eta;
        tr.substituted_text = "η = ( Σ t_i^" + beta + " / " + r + " )^(1/" + beta
            + ") = " + eta;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
'''


def write_cpp(ids: list[str]) -> None:
    covered = [c for c in ids if c not in EXEMPT]
    array_lines = ",\n".join(f'    "{c}"' for c in covered)

    body_parts = [emit_capability(), emit_one_sample_t(), emit_weibayes()]
    for cid in covered:
        if cid in {"capability", "one_sample_t", "weibayes"}:
            continue
        body_parts.append(emit_generic_case(cid))

    cpp = f'''#include "application/computation_trace_attach.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace datalab::application {{
namespace {{

using datalab::domain::ComputationStep;
using datalab::domain::ComputationTrace;
using datalab::domain::FormulaBinding;
using datalab::domain::OutputPage;
using datalab::domain::StatisticTable;

// Compile-time / grep catalog of every non-E G9 command id (141).
constexpr const char* k_g9_covered_commands[] = {{
{array_lines}
}};

std::string fmt_num(double value)
{{
    std::ostringstream oss;
    oss << std::setprecision(10) << value;
    return oss.str();
}}

std::string opt_size(const std::optional<std::size_t>& value)
{{
    if (!value.has_value()) {{
        return {{}};
    }}
    return std::to_string(*value);
}}

FormulaBinding bind(
    const std::string& symbol,
    const std::string& label,
    const std::string& value,
    const std::string& role)
{{
    FormulaBinding b;
    b.symbol = symbol;
    b.label = label;
    b.value = value;
    b.role = role;
    return b;
}}

std::string table_value(const OutputPage& page, auto&&... labels)
{{
    const std::string wanted[] = {{std::string(labels)...}};
    for (const StatisticTable& table : page.tables) {{
        for (const auto& row : table.rows) {{
            if (row.empty()) {{
                continue;
            }}
            for (const std::string& label : wanted) {{
                if (row.front() == label && row.size() >= 2) {{
                    return row[1];
                }}
                // Two-column label/value rows with label in any cell before last.
                for (std::size_t i = 0; i + 1 < row.size(); ++i) {{
                    if (row[i] == label) {{
                        return row[i + 1];
                    }}
                }}
            }}
        }}
    }}
    return {{}};
}}

std::string first_nonempty(std::initializer_list<std::string> values)
{{
    for (const std::string& value : values) {{
        if (!value.empty()) {{
            return value;
        }}
    }}
    return {{}};
}}

const std::unordered_map<std::string, std::string>& prefix_map()
{{
    static const std::unordered_map<std::string, std::string> map = {{
        {{"desc", "descriptive"}},
        {{"normality", "normality_test"}},
        {{"outlier_test", "outlier_test"}},
        {{"correlation", "correlation"}},
        {{"one_sample_t", "one_sample_t"}},
        {{"one_sample_z", "one_sample_z"}},
        {{"two_sample_t", "two_sample_t"}},
        {{"paired_t", "paired_t"}},
        {{"anova", "one_way_anova"}},
        {{"two_factor_anova", "two_factor_anova"}},
        {{"regression", "regression"}},
        {{"logistic", "logistic_regression"}},
        {{"two_proportions", "two_proportions"}},
        {{"one_proportion", "one_proportion"}},
        {{"one_poisson_rate", "one_poisson_rate"}},
        {{"two_poisson_rate", "two_poisson_rate"}},
        {{"chi_square", "chi_square"}},
        {{"cross_tab", "cross_tabulation"}},
        {{"chi_square_gof", "chi_square_gof"}},
        {{"poisson_gof", "poisson_gof"}},
        {{"box_cox", "box_cox"}},
        {{"gage_rr", "gage_rr"}},
        {{"emp", "emp_crossed"}},
        {{"expanded_gage", "expanded_gage_rr"}},
        {{"msa_type1", "msa_type1"}},
        {{"nested_gage", "nested_gage_rr"}},
        {{"attribute_agreement", "attribute_agreement"}},
        {{"mann_whitney", "mann_whitney"}},
        {{"wilcoxon", "wilcoxon_signed_rank"}},
        {{"sign_test", "sign_test"}},
        {{"runs_test", "runs_test"}},
        {{"fisher_exact", "fisher_exact"}},
        {{"mcnemar", "mcnemar"}},
        {{"cochran_q", "cochran_q"}},
        {{"mood_median", "mood_median"}},
        {{"kruskal_wallis", "kruskal_wallis"}},
        {{"friedman", "friedman"}},
        {{"cap", "capability"}},
        {{"binomial_cap", "binomial_capability"}},
        {{"poisson_cap", "poisson_capability"}},
        {{"sixpack", "capability_sixpack"}},
        {{"batch_capability", "batch_capability"}},
        {{"nonparametric_capability", "nonparametric_capability"}},
        {{"distribution_id", "distribution_identification"}},
        {{"tolerance", "tolerance_intervals"}},
        {{"acceptance_sampling", "acceptance_sampling"}},
        {{"multi_vari", "multi_vari"}},
        {{"variability_chart", "variability_chart"}},
        {{"imr", "imr"}},
        {{"imrrs", "imr_rs"}},
        {{"ewma", "ewma"}},
        {{"cusum", "cusum"}},
        {{"zone_chart", "zone_chart"}},
        {{"z_mr", "z_mr"}},
        {{"moving_average", "moving_average"}},
        {{"t2", "hotelling_t2"}},
        {{"mewma", "mewma"}},
        {{"gv", "generalized_variance"}},
        {{"hist", "histogram"}},
        {{"eda4", "eda_4plot"}},
        {{"box", "boxplot"}},
        {{"pareto", "pareto"}},
        {{"run_chart", "run_chart"}},
        {{"cause_and_effect", "cause_and_effect"}},
        {{"weibayes", "weibayes"}},
        {{"reliability", "reliability"}},
        {{"accelerated_life", "accelerated_life"}},
        {{"reliability_warranty", "reliability_warranty"}},
        {{"cox_regression", "cox_regression"}},
        {{"probit_reliability", "probit_reliability"}},
        {{"km_interval", "km_interval"}},
        {{"random_forest", "random_forest"}},
        {{"distribution_calculator", "distribution_calculator"}},
        {{"t_power", "t_power"}},
        {{"pca", "pca"}},
        {{"kmeans", "kmeans"}},
        {{"cart_tree", "cart_tree"}},
        {{"isolation_forest", "isolation_forest"}},
        {{"bootstrap_mean", "bootstrap_mean"}},
        {{"bootstrap_two_sample", "bootstrap_two_sample"}},
        {{"adf_test", "adf_test"}},
        {{"poisson_regression", "poisson_regression"}},
        {{"ordinal_logistic", "ordinal_logistic"}},
        {{"nominal_logistic", "nominal_logistic"}},
        {{"discriminant", "discriminant"}},
        {{"cluster_observations", "cluster_observations"}},
        {{"ccf", "ccf"}},
        {{"correlogram", "correlogram"}},
        {{"stepwise_regression", "stepwise_regression"}},
        {{"best_subsets_regression", "best_subsets_regression"}},
        {{"acf_pacf", "acf_pacf"}},
        {{"arima", "arima"}},
        {{"decomposition", "time_series_decomposition"}},
        {{"seasonal_forecast", "seasonal_forecasting"}},
        {{"time_series", "time_series_smoothing"}},
        {{"anom", "anom"}},
        {{"variance", "variance_test"}},
        {{"response_optimization", "response_optimization"}},
        {{"rsm_response", "rsm_response"}},
    }};
    return map;
}}

void attach_impl(OutputPage& page, const std::string& command_id)
{{
    if (command_id.empty() || command_id == "tests" || command_id == "rule_policy") {{
        return;
    }}
    // Touch catalog so the array is not optimized away and remains greppable.
    static_cast<void>(k_g9_covered_commands);

{"".join(body_parts)}
}}

}}  // namespace

void attach_computation_traces(OutputPage& page, const std::string& command_id)
{{
    attach_impl(page, command_id);
}}

std::string resolve_command_id_from_page(const OutputPage& page)
{{
    if (!page.analysis_command_id.empty()) {{
        return page.analysis_command_id;
    }}
    const std::string& id = page.id;
    if (id.empty()) {{
        return {{}};
    }}
    // Exact command id prefix (id may be "capability_12").
    for (const char* covered : k_g9_covered_commands) {{
        const std::string cid(covered);
        if (id == cid || id.rfind(cid + "_", 0) == 0) {{
            return cid;
        }}
    }}
    const auto underscore = id.find('_');
    const std::string prefix = underscore == std::string::npos ? id : id.substr(0, underscore);
    const auto& map = prefix_map();
    const auto it = map.find(prefix);
    if (it != map.end()) {{
        return it->second;
    }}
    // Multi-segment prefixes like "one_sample_t_3".
    for (const auto& [key, value] : map) {{
        if (id.rfind(key + "_", 0) == 0 || id == key) {{
            return value;
        }}
    }}
    return {{}};
}}

}}  // namespace datalab::application
'''
    # Fix C++20 auto&&... pack — MSVC may need a simpler overload.
    # Replace table_value template with fixed overloads for cleaner MSVC support.
    cpp = cpp.replace(
        """std::string table_value(const OutputPage& page, auto&&... labels)
{
    const std::string wanted[] = {std::string(labels)...};
    for (const StatisticTable& table : page.tables) {
        for (const auto& row : table.rows) {
            if (row.empty()) {
                continue;
            }
            for (const std::string& label : wanted) {
                if (row.front() == label && row.size() >= 2) {
                    return row[1];
                }
                // Two-column label/value rows with label in any cell before last.
                for (std::size_t i = 0; i + 1 < row.size(); ++i) {
                    if (row[i] == label) {
                        return row[i + 1];
                    }
                }
            }
        }
    }
    return {};
}""",
        """std::string table_value_impl(
    const OutputPage& page, const std::vector<std::string>& wanted)
{
    for (const StatisticTable& table : page.tables) {
        for (const auto& row : table.rows) {
            if (row.empty()) {
                continue;
            }
            for (const std::string& label : wanted) {
                if (row.front() == label && row.size() >= 2) {
                    return row[1];
                }
                for (std::size_t i = 0; i + 1 < row.size(); ++i) {
                    if (row[i] == label) {
                        return row[i + 1];
                    }
                }
            }
        }
    }
    return {};
}

std::string table_value(const OutputPage& page, const std::string& a)
{
    return table_value_impl(page, {a});
}
std::string table_value(
    const OutputPage& page, const std::string& a, const std::string& b)
{
    return table_value_impl(page, {a, b});
}
std::string table_value(
    const OutputPage& page,
    const std::string& a,
    const std::string& b,
    const std::string& c)
{
    return table_value_impl(page, {a, b, c});
}
std::string table_value(
    const OutputPage& page,
    const std::string& a,
    const std::string& b,
    const std::string& c,
    const std::string& d)
{
    return table_value_impl(page, {a, b, c, d});
}
std::string table_value(
    const OutputPage& page,
    const std::string& a,
    const std::string& b,
    const std::string& c,
    const std::string& d,
    const std::string& e)
{
    return table_value_impl(page, {a, b, c, d, e});
}
std::string table_value(
    const OutputPage& page,
    const std::string& a,
    const std::string& b,
    const std::string& c,
    const std::string& d,
    const std::string& e,
    const std::string& f)
{
    return table_value_impl(page, {a, b, c, d, e, f});
}""",
    )
    (ROOT / "src/application/computation_trace_attach.cpp").write_text(
        cpp, encoding="utf-8"
    )


def write_matrix(ids: list[str]) -> None:
    lines = [
        "# G9 公式代入覆盖矩阵",
        "",
        "> 权威命令源：`python tools/_list_command_ids.py`",
        "> status ∈ {实质绑定, display_summary, 豁免}",
        "",
        "| command_id | family | evidence_type | formula_id | status |",
        "|---|---|---|---|---|",
    ]
    for cid in ids:
        fid, _, _, _ = formula_meta(cid)
        if cid in EXEMPT:
            fid = "—"
            evidence = "—"
        else:
            evidence = evidence_of(cid)
        lines.append(
            f"| {cid} | {family_of(cid)} | {evidence} | {fid} | {status_of(cid)} |"
        )
    lines.append("")
    path = ROOT / "docs/research/g9-formula-substitution-coverage-matrix.md"
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    ids = list_command_ids()
    print(f"commands={len(ids)}")
    d = sum(1 for c in ids if c in D_CLASS)
    e = sum(1 for c in ids if c in EXEMPT)
    print(f"D={d} E={e} covered={len(ids)-e}")
    write_header()
    write_cpp(ids)
    write_matrix(ids)
    print("wrote attach + matrix")


if __name__ == "__main__":
    main()
