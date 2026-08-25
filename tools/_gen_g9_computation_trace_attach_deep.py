#!/usr/bin/env python3
"""Generate G9 L3 deep computation trace attach module (79 stub replacements)."""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
CPP_ATTACH = ROOT / "src/application/computation_trace_attach.cpp"
HELP = ROOT / "resources/help/algorithm_help.json"
OUT_H = ROOT / "src/application/computation_trace_attach_deep.h"
OUT_CPP = ROOT / "src/application/computation_trace_attach_deep.cpp"


def cpp_escape(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def load_help_formulas() -> dict[str, list[str]]:
    data = json.loads(HELP.read_text(encoding="utf-8"))
    out: dict[str, list[str]] = {}
    for entry in data.get("entries", []):
        if not isinstance(entry, dict):
            continue
        cid = entry.get("id") or entry.get("command_id")
        if not cid:
            continue
        blocks = entry.get("formula_blocks") or []
        formulas: list[str] = []
        for b in blocks:
            if isinstance(b, str):
                formulas.append(b.replace("\n", " "))
            elif isinstance(b, dict):
                for k in ("plain_text", "formula", "plain", "text", "content"):
                    if k in b and b[k]:
                        formulas.append(str(b[k]).replace("\n", " "))
                        break
        if formulas:
            out[cid] = formulas
    return out


# Authoritative list of 79 L3 deep-trace command ids (filled after DEEP_META).
STUB_COMMAND_IDS: list[str] = []


def extract_stubs() -> list[str]:
    """Read stubs from attach.cpp when present; else fall back to STUB_COMMAND_IDS."""
    if not CPP_ATTACH.is_file():
        return STUB_COMMAND_IDS
    cpp = CPP_ATTACH.read_text(encoding="utf-8")
    ids: list[str] = []
    seen: set[str] = set()
    for m in re.finditer(r'if \(command_id == "([^"]+)"\)', cpp):
        cid = m.group(1)
        block = cpp[m.start() : m.start() + 2500]
        pm = re.search(r'tr\.plain_formula = "([^"]+)"', block)
        if pm and "主公式" in pm.group(1) and cid not in seen:
            seen.add(cid)
            ids.append(cid)
    return ids if ids else STUB_COMMAND_IDS


# Hand-curated L3 metadata (plain_formula from algorithm_help / Minitab-NIST when missing).
DEEP_META: dict[str, dict[str, str]] = {
    "normality_test": {
        "formula_id": "anderson_darling",
        "title": "正态性检验 Anderson-Darling",
        "plain_formula": "A² = −n − (1/n)Σ[(2i−1)(ln F(x_(i))+ln(1−F(x_(n+1−i))))]",
        "result_symbol": "P",
        "family": "normality",
    },
    "outlier_test": {
        "formula_id": "grubbs_g",
        "title": "Grubbs 离群值检验",
        "plain_formula": "G = max|x_i − x̄| / s",
        "result_symbol": "G",
        "family": "outlier",
    },
    "correlation": {
        "formula_id": "pearson_r",
        "title": "Pearson 相关系数",
        "plain_formula": "r = Σ[(x−x̄)(y−ȳ)] / √[Σ(x−x̄)² Σ(y−ȳ)²]",
        "result_symbol": "r",
        "family": "correlation",
    },
    "one_sample_z": {
        "formula_id": "one_sample_z",
        "title": "单样本 Z 检验",
        "plain_formula": "Z = (x̄ − μ₀) / (σ / √n)",
        "result_symbol": "Z",
        "family": "t_test",
    },
    "one_proportion": {
        "formula_id": "one_proportion",
        "title": "单样本比例检验",
        "plain_formula": "p̂ = x/n；Z = (p̂ − p₀) / √[p₀(1−p₀)/n]",
        "result_symbol": "Z",
        "family": "proportion",
    },
    "one_poisson_rate": {
        "formula_id": "one_poisson_rate",
        "title": "单样本 Poisson 率",
        "plain_formula": "λ̂ = events/exposure；Z = (λ̂ − λ₀)/√(λ₀/exposure)",
        "result_symbol": "Z",
        "family": "poisson_rate",
    },
    "two_poisson_rate": {
        "formula_id": "two_poisson_rate",
        "title": "双样本 Poisson 率",
        "plain_formula": "Z = (λ̂₁ − λ̂₂) / √[λ̂(1/n₁ + 1/n₂)]",
        "result_symbol": "Z",
        "family": "poisson_rate",
    },
    "two_sample_t": {
        "formula_id": "welch_t",
        "title": "双样本 t 检验 (Welch)",
        "plain_formula": "t = (x̄₁ − x̄₂) / SE；SE = √(s₁²/n₁ + s₂²/n₂)",
        "result_symbol": "t",
        "family": "t_test",
    },
    "one_sample_equivalence": {
        "formula_id": "tost_one_sample",
        "title": "单样本等价性 (TOST)",
        "plain_formula": "H₀: |μ − μ₀| ≥ δ；TOST 用两侧 1−α 置信区间",
        "result_symbol": "P",
        "family": "equivalence",
    },
    "two_sample_equivalence": {
        "formula_id": "tost_two_sample",
        "title": "双样本等价性 (TOST)",
        "plain_formula": "H₀: |μ₁ − μ₂| ≥ δ；TOST 用差值 1−α 置信区间",
        "result_symbol": "P",
        "family": "equivalence",
    },
    "two_sample_equivalence_ratio": {
        "formula_id": "tost_ratio",
        "title": "双样本等价性 (比率)",
        "plain_formula": "H₀: ratio 超出 [θ_L, θ_U]；TOST 对 ln(ratio)",
        "result_symbol": "P",
        "family": "equivalence",
    },
    "paired_equivalence": {
        "formula_id": "tost_paired",
        "title": "配对等价性 (TOST)",
        "plain_formula": "H₀: |d̄| ≥ δ；d_i = x_i − y_i",
        "result_symbol": "P",
        "family": "equivalence",
    },
    "one_proportion_equivalence": {
        "formula_id": "tost_one_proportion",
        "title": "单比例等价性",
        "plain_formula": "H₀: |p̂ − p₀| ≥ δ",
        "result_symbol": "P",
        "family": "equivalence",
    },
    "two_proportion_equivalence": {
        "formula_id": "tost_two_proportion",
        "title": "双比例等价性",
        "plain_formula": "H₀: |p̂₁ − p̂₂| ≥ δ",
        "result_symbol": "P",
        "family": "equivalence",
    },
    "one_way_anova": {
        "formula_id": "one_way_anova_f",
        "title": "单因子 ANOVA",
        "plain_formula": "F = MSB/MSW = [SSB/(k−1)] / [SSW/(N−k)]",
        "result_symbol": "F",
        "family": "anova",
    },
    "paired_t": {
        "formula_id": "paired_t",
        "title": "配对 t 检验",
        "plain_formula": "t = d̄ / (s_d / √n)；d_i = x_i − y_i",
        "result_symbol": "t",
        "family": "t_test",
    },
    "regression": {
        "formula_id": "ols_regression",
        "title": "最小二乘线性回归",
        "plain_formula": "Y = β₀ + Σ β_j X_j + ε；β̂ = (X'X)⁻¹X'Y",
        "result_symbol": "R²",
        "family": "regression",
    },
    "two_proportions": {
        "formula_id": "two_proportions",
        "title": "双样本比例检验",
        "plain_formula": "Z = (p̂₁ − p̂₂) / √[p̂(1−p̂)(1/n₁ + 1/n₂)]",
        "result_symbol": "Z",
        "family": "proportion",
    },
    "chi_square": {
        "formula_id": "chi_square",
        "title": "卡方独立性检验",
        "plain_formula": "χ² = Σ (O − E)² / E",
        "result_symbol": "χ²",
        "family": "chi_square",
    },
    "cross_tabulation": {
        "formula_id": "cross_tabulation",
        "title": "列联表比例",
        "plain_formula": "cell% = count / row(or col) total",
        "result_symbol": "N",
        "family": "cross_tab",
    },
    "chi_square_gof": {
        "formula_id": "chi_square_gof",
        "title": "卡方拟合优度",
        "plain_formula": "χ² = Σ (O − E)² / E，E = p·N",
        "result_symbol": "χ²",
        "family": "chi_square",
    },
    "poisson_gof": {
        "formula_id": "poisson_gof",
        "title": "Poisson 拟合优度",
        "plain_formula": "χ² = Σ (O − E)² / E，E = λ̂·exposure",
        "result_symbol": "χ²",
        "family": "chi_square",
    },
    "mann_whitney": {
        "formula_id": "mann_whitney_u",
        "title": "Mann-Whitney U 检验",
        "plain_formula": "U = n₁n₂ + n₁(n₁+1)/2 − R₁",
        "result_symbol": "U",
        "family": "nonparametric",
    },
    "wilcoxon_signed_rank": {
        "formula_id": "wilcoxon_signed_rank",
        "title": "Wilcoxon 符号秩检验",
        "plain_formula": "W⁺ = Σ R_i (d_i > 0)",
        "result_symbol": "W",
        "family": "nonparametric",
    },
    "sign_test": {
        "formula_id": "sign_test",
        "title": "符号检验",
        "plain_formula": "S = #{d_i > 0} ~ Binomial(n, 0.5)",
        "result_symbol": "S",
        "family": "nonparametric",
    },
    "runs_test": {
        "formula_id": "runs_test",
        "title": "游程检验",
        "plain_formula": "Z = (R − μ_R) / σ_R",
        "result_symbol": "Z",
        "family": "nonparametric",
    },
    "mcnemar": {
        "formula_id": "mcnemar",
        "title": "McNemar 检验",
        "plain_formula": "χ² = (b − c)² / (b + c)",
        "result_symbol": "χ²",
        "family": "mcnemar",
    },
    "fisher_exact": {
        "formula_id": "fisher_exact",
        "title": "Fisher 精确检验",
        "plain_formula": "P = Σ hypergeom(a|n,m,k)",
        "result_symbol": "P",
        "family": "chi_square",
    },
    "cochran_q": {
        "formula_id": "cochran_q",
        "title": "Cochran Q 检验",
        "plain_formula": "Q = (k−1)(kΣC_j² − T²) / (kT − ΣR_i²)",
        "result_symbol": "Q",
        "family": "cochran_q",
    },
    "mood_median": {
        "formula_id": "mood_median",
        "title": "Mood 中位数检验",
        "plain_formula": "χ² = Σ (O − E)² / E (above/below grand median)",
        "result_symbol": "χ²",
        "family": "nonparametric",
    },
    "kruskal_wallis": {
        "formula_id": "kruskal_wallis",
        "title": "Kruskal-Wallis 检验",
        "plain_formula": "H = [12/(N(N+1))] Σ (R_j²/n_j) − 3(N+1)",
        "result_symbol": "H",
        "family": "nonparametric",
    },
    "friedman": {
        "formula_id": "friedman",
        "title": "Friedman 检验",
        "plain_formula": "χ²_r = [12/(bk(k+1))] Σ R_j² − 3b(k+1)",
        "result_symbol": "χ²",
        "family": "nonparametric",
    },
    "two_factor_anova": {
        "formula_id": "two_factor_anova",
        "title": "双因子 ANOVA",
        "plain_formula": "F = MS_effect / MS_error",
        "result_symbol": "F",
        "family": "anova",
    },
    "logistic_regression": {
        "formula_id": "logistic_regression",
        "title": "Logistic 回归",
        "plain_formula": "logit(π) = ln(π/(1−π)) = x'β",
        "result_symbol": "Deviance",
        "family": "regression",
    },
    "variance_test": {
        "formula_id": "levene_f",
        "title": "方差齐性检验",
        "plain_formula": "F = s_max² / s_min² (或 Levene W)",
        "result_symbol": "F",
        "family": "variance",
    },
    "adf_test": {
        "formula_id": "adf_test",
        "title": "ADF 单位根检验",
        "plain_formula": "Δy_t = α + γ y_{t−1} + Σ δ_i Δy_{t−i} + ε_t",
        "result_symbol": "t",
        "family": "adf",
    },
    "poisson_regression": {
        "formula_id": "poisson_regression",
        "title": "Poisson 回归",
        "plain_formula": "ln(E[Y]) = x'β；Y ~ Poisson(μ)",
        "result_symbol": "Deviance",
        "family": "regression",
    },
    "probit_reliability": {
        "formula_id": "probit_reliability",
        "title": "Probit 可靠性",
        "plain_formula": "logit(p) = β₀ + β₁·stress；LD50 = −β₀/β₁",
        "result_symbol": "LD50",
        "family": "reliability",
    },
    "ordinal_logistic": {
        "formula_id": "ordinal_logistic",
        "title": "有序 Logistic",
        "plain_formula": "logit P(Y≤k) = θ_k + x'β",
        "result_symbol": "Deviance",
        "family": "regression",
    },
    "nominal_logistic": {
        "formula_id": "nominal_logistic",
        "title": "名义 Logistic",
        "plain_formula": "log(P(Y=k)/P(Y=ref)) = η_k",
        "result_symbol": "Deviance",
        "family": "regression",
    },
    "stepwise_regression": {
        "formula_id": "stepwise_regression",
        "title": "逐步回归",
        "plain_formula": "进入/剔除按 F 或 AIC 准则",
        "result_symbol": "R²",
        "family": "regression",
    },
    "best_subsets_regression": {
        "formula_id": "best_subsets_cp",
        "title": "最优子集回归",
        "plain_formula": "Cp = SSE_p / s²_full − (n − 2(p+1))",
        "result_symbol": "Cp",
        "family": "regression",
    },
    "reliability": {
        "formula_id": "kaplan_meier",
        "title": "Kaplan-Meier 生存",
        "plain_formula": "Ŝ(t) = Π (1 − d_i/n_i) at death times",
        "result_symbol": "Ŝ",
        "family": "reliability",
    },
    "accelerated_life": {
        "formula_id": "accelerated_life",
        "title": "加速寿命模型",
        "plain_formula": "log(Y) = β₀ + β₁·x + (1/shape)·Φ⁻¹(p)",
        "result_symbol": "β₁",
        "family": "reliability",
    },
    "reliability_warranty": {
        "formula_id": "reliability_warranty",
        "title": "保修可靠性预测",
        "plain_formula": "E[claims] = exposure × (1 − R(t_w))",
        "result_symbol": "Claims",
        "family": "reliability",
    },
    "imr": {
        "formula_id": "imr_limits",
        "title": "I-MR 控制限",
        "plain_formula": "X̄ = mean(x_i)；σ = MR̄/d₂；UCL_I = X̄ + 3σ",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "imr",
    },
    "xbar_r": {
        "formula_id": "xbar_r_limits",
        "title": "X̄-R 控制限",
        "plain_formula": "X̄̄ = mean(X̄_i)；R̄ = mean(R_i)；UCL = X̄̄ + A₂R̄",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "xbar_r",
    },
    "xbar_s": {
        "formula_id": "xbar_s_limits",
        "title": "X̄-S 控制限",
        "plain_formula": "X̄̄ = mean(X̄_i)；S̄ = mean(S_i)；UCL = X̄̄ + A₃S̄",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "xbar_s",
    },
    "imr_rs": {
        "formula_id": "imr_rs_limits",
        "title": "I-MR-R/S 控制限",
        "plain_formula": "I 图用 X̄±3σ；MR/R 图用 MR̄/d₂ 或 R̄/d₂",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "imr",
    },
    "p_chart": {
        "formula_id": "p_chart_limits",
        "title": "P 控制图",
        "plain_formula": "p̄ = Σd_i/Σn_i；UCL = p̄ + 3√[p̄(1−p̄)/n_i]",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "p",
    },
    "np_chart": {
        "formula_id": "np_chart_limits",
        "title": "NP 控制图",
        "plain_formula": "n̄p̄ = mean(np_i)；UCL = n̄p̄ + 3√[n̄p̄(1−p̄)]",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "np",
    },
    "c_chart": {
        "formula_id": "c_chart_limits",
        "title": "C 控制图",
        "plain_formula": "c̄ = mean(c_i)；UCL = c̄ + 3√c̄",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "c",
    },
    "u_chart": {
        "formula_id": "u_chart_limits",
        "title": "U 控制图",
        "plain_formula": "ū = Σc_i/Σn_i；UCL = ū + 3√(ū/n_i)",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "u",
    },
    "laney_p_chart": {
        "formula_id": "laney_p_limits",
        "title": "Laney P 控制图",
        "plain_formula": "UCL = p̄ + 3σ_Z √[p̄(1−p̄)/n]",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "p",
    },
    "laney_u_chart": {
        "formula_id": "laney_u_limits",
        "title": "Laney U 控制图",
        "plain_formula": "UCL = ū + 3σ_Z √(ū/n)",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "u",
    },
    "ewma": {
        "formula_id": "ewma_limits",
        "title": "EWMA 控制图",
        "plain_formula": "z_t = λ x_t + (1−λ)z_{t−1}；限 = μ ± Lσ√[λ/(2−λ)]",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "ewma",
    },
    "mewma": {
        "formula_id": "mewma_limits",
        "title": "MEWMA 控制图",
        "plain_formula": "T² = λ y_t' Σ⁻¹ y_t + (1−λ)T²_{t−1}",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "multivariate",
    },
    "generalized_variance": {
        "formula_id": "generalized_variance",
        "title": "广义方差控制图",
        "plain_formula": "UCL = |Σ̂|(b₁ + 3√b₂)",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "multivariate",
    },
    "cusum": {
        "formula_id": "cusum",
        "title": "CUSUM 控制图",
        "plain_formula": "C⁺_t = max(0, C⁺_{t−1} + (x_t−μ₀)/σ − k)",
        "result_symbol": "C",
        "family": "spc",
        "spc_kind": "cusum",
    },
    "zone_chart": {
        "formula_id": "zone_chart",
        "title": "Zone 控制图",
        "plain_formula": "Z = (x − μ)/σ；分区 ±1σ, ±2σ, ±3σ",
        "result_symbol": "Z",
        "family": "spc",
        "spc_kind": "zone",
    },
    "z_mr": {
        "formula_id": "z_mr",
        "title": "Z-MR 控制图",
        "plain_formula": "Z_i = (x_i − μ)/σ；MR_i = |Z_i − Z_{i−1}|",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "imr",
    },
    "moving_average": {
        "formula_id": "moving_average",
        "title": "移动平均控制图",
        "plain_formula": "MA_t = (1/w) Σ_{i=0}^{w−1} x_{t−i}",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "ma",
    },
    "g_chart": {
        "formula_id": "g_chart",
        "title": "G 控制图",
        "plain_formula": "p̂ = 1/(x̄+1)；CL/LCL/UCL 为几何分位",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "g",
    },
    "t_chart": {
        "formula_id": "t_chart",
        "title": "T 控制图",
        "plain_formula": "基于 Weibull/Gamma 分位构造 CL/UCL/LCL",
        "result_symbol": "UCL",
        "family": "spc",
        "spc_kind": "t",
    },
    "tolerance_intervals": {
        "formula_id": "tolerance_interval",
        "title": "公差区间",
        "plain_formula": "TI = x̄ ± k·s (正态) 或非参数次序统计量",
        "result_symbol": "k",
        "family": "tolerance",
    },
    "between_within_capability": {
        "formula_id": "between_within_cp",
        "title": "组间组内能力",
        "plain_formula": "σ_BW = √(σ_B² + σ_w²)；Cp = (USL−LSL)/(6σ_BW)",
        "result_symbol": "Cp",
        "family": "capability",
    },
    "batch_capability": {
        "formula_id": "batch_capability",
        "title": "批次能力",
        "plain_formula": "各批次分别估计 Cpk 后汇总",
        "result_symbol": "Cpk",
        "family": "capability",
    },
    "nonparametric_capability": {
        "formula_id": "nonparametric_capability",
        "title": "非参数能力",
        "plain_formula": "Ppk 由经验分位数与规格限计算",
        "result_symbol": "Ppk",
        "family": "capability",
    },
    "cox_regression": {
        "formula_id": "cox_regression",
        "title": "Cox 比例风险",
        "plain_formula": "λ(t|x) = λ₀(t) exp(x'β)",
        "result_symbol": "HR",
        "family": "reliability",
    },
    "binomial_capability": {
        "formula_id": "binomial_capability",
        "title": "二项能力",
        "plain_formula": "p̂ = Σd_i/Σn_i；PPM = 10⁶·p̂",
        "result_symbol": "Z",
        "family": "capability",
    },
    "poisson_capability": {
        "formula_id": "poisson_capability",
        "title": "Poisson 能力",
        "plain_formula": "DPU = defects/units；Z 由 Poisson 分位",
        "result_symbol": "Z",
        "family": "capability",
    },
    "nonnormal_capability": {
        "formula_id": "nonnormal_capability",
        "title": "非正态能力",
        "plain_formula": "Ppk = min(PPL, PPU)；PPL = −Z.LSL/3",
        "result_symbol": "Ppk",
        "family": "capability",
    },
    "capability_sixpack": {
        "formula_id": "capability_sixpack",
        "title": "能力六合一",
        "plain_formula": "p_i = (i+0.625)/(n+0.25)；z_i = Φ⁻¹(p_i)",
        "result_symbol": "Cpk",
        "family": "capability",
    },
    "gage_rr": {
        "formula_id": "gage_rr_ndc",
        "title": "Gage R&R",
        "plain_formula": "ndc = floor(1.41 × σ_part / σ_gage)",
        "result_symbol": "%GRR",
        "family": "msa",
    },
    "emp_crossed": {
        "formula_id": "emp_crossed",
        "title": "EMP Crossed",
        "plain_formula": "Part / (Part + Repeat + Operator + Part×Operator)",
        "result_symbol": "%Repeat",
        "family": "msa",
    },
    "expanded_gage_rr": {
        "formula_id": "expanded_gage_rr",
        "title": "Expanded Gage R&R",
        "plain_formula": "三因子 ANOVA 方差分量",
        "result_symbol": "%GRR",
        "family": "msa",
    },
    "msa_type1": {
        "formula_id": "msa_type1",
        "title": "MSA Type 1",
        "plain_formula": "Cg = Tol/(6s)；Cgk 扣偏倚",
        "result_symbol": "Cg",
        "family": "msa",
    },
    "nested_gage_rr": {
        "formula_id": "nested_gage_rr",
        "title": "Nested Gage R&R",
        "plain_formula": "嵌套 ANOVA 方差分量",
        "result_symbol": "%GRR",
        "family": "msa",
    },
    "attribute_agreement": {
        "formula_id": "attribute_agreement",
        "title": "属性一致性",
        "plain_formula": "κ = (P_o − P_e)/(1 − P_e)",
        "result_symbol": "κ",
        "family": "msa",
    },
}


STUB_COMMAND_IDS.extend(DEEP_META.keys())


def merge_help_formulas(stubs: list[str], help_map: dict[str, list[str]]) -> None:
    for cid in stubs:
        meta = DEEP_META.setdefault(cid, {})
        if help_map.get(cid) and "plain_formula" not in meta:
            meta["plain_formula"] = help_map[cid][0]
        meta.setdefault("formula_id", f"{cid}_l3")
        meta.setdefault("title", cid.replace("_", " "))
        meta.setdefault("result_symbol", "result")
        meta.setdefault("family", "generic")


def emit_header(stubs: list[str]) -> str:
    return """#pragma once

#include "domain/quality_types.h"

#include <string>

namespace datalab::application {

// G9-D L3 deep computation traces (79 former generic stubs).
bool attach_deep_trace(datalab::domain::OutputPage& page, const std::string& command_id);

}  // namespace datalab::application
"""


def emit_family_helpers() -> str:
    return r'''
// ---------- Shared finish ----------
bool finish_l3(OutputPage& page, ComputationTrace tr)
{
    if (tr.plain_formula.empty()) {
        return false;
    }
    attach_trace(page, std::move(tr));
    return true;
}

// ---------- SPC / control chart family ----------
bool attach_spc_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol,
    const char* sigma_step_desc,
    const char* sigma_step_before)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Control_chart";
    tr.result_symbol = result_symbol;

    std::string cl = require_value(
        table_value(page, "CL", "Center", "中心线", "X-bar", "X̄"),
        page.facts.spc.has_value() ? opt_fmt(page.facts.spc->estimated_sigma) : std::string{});
    std::string ucl = require_value(table_value(page, "UCL", "UCL (I)", "上控制限"));
    std::string lcl = require_value(table_value(page, "LCL", "LCL (I)", "下控制限"));
    std::string sigma = require_value(
        page.facts.spc.has_value() ? opt_fmt(page.facts.spc->sigma_within) : std::string{},
        page.facts.spc.has_value() ? opt_fmt(page.facts.spc->estimated_sigma) : std::string{},
        table_value(page, "StDev", "Sigma", "σ", "Std Dev"));
    std::string mr_bar = table_value(page, "MR-bar", "MR Bar", "R-bar", "R̄", "MR̄");

    tr.bindings.push_back(bind("CL", "中心线", cl, "input"));
    tr.bindings.push_back(bind("UCL", "上控制限", ucl, "result"));
    tr.bindings.push_back(bind("LCL", "下控制限", lcl, "intermediate"));
    tr.bindings.push_back(bind("σ", "过程 σ", sigma, "input"));
    if (!mr_bar.empty()) {
        tr.bindings.push_back(bind("MR̄", "平均移动极差", mr_bar, "intermediate"));
    }

    push_step(tr.steps, make_step(1, "估计中心线", "CL = x̄", "CL = " + cl, cl));
    if (!mr_bar.empty() && !sigma.empty()) {
        push_step(tr.steps, make_step(
            2, sigma_step_desc, sigma_step_before,
            "σ = " + sigma + " (MR̄=" + mr_bar + ")", sigma));
    } else if (!sigma.empty()) {
        push_step(tr.steps, make_step(2, "估计 σ", "σ = MR̄/d₂", "σ = " + sigma, sigma));
    }
    push_step(tr.steps, make_step(
        3, "构造控制限", "UCL = CL + 3σ", "UCL = " + ucl + ", LCL = " + lcl, ucl));

    tr.result_value = require_value(ucl, cl);
    tr.substituted_text = plain_formula + std::string(" → UCL=") + ucl
        + ", CL=" + cl + ", LCL=" + lcl;
    return finish_l3(page, std::move(tr));
}

// ---------- t-test family ----------
bool attach_t_test_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Student%27s_t-test";
    tr.result_symbol = result_symbol;

    std::string n = require_value(
        page.facts.t_test.has_value() ? std::to_string(page.facts.t_test->n) : std::string{},
        table_value(page, "N", "n"));
    std::string xbar = require_value(
        page.facts.t_test.has_value() ? opt_fmt(page.facts.t_test->mean) : std::string{},
        table_value(page, "Mean", "均值", "Sample Mean"));
    std::string s = require_value(
        page.facts.t_test.has_value()
            ? opt_fmt(page.facts.t_test->sample_standard_deviation) : std::string{},
        table_value(page, "StDev", "标准差", "s", "SE Mean"));
    std::string stat = require_value(
        page.facts.t_test.has_value() ? opt_fmt(page.facts.t_test->z_statistic) : std::string{},
        table_value(page, "T", "t", "Z", "Statistic", "统计量"));
    std::string p = require_value(
        page.facts.t_test.has_value() ? opt_fmt(page.facts.t_test->p_value) : std::string{},
        table_value(page, "P", "P-Value", "P 值"));

    tr.bindings.push_back(bind("n", "样本量", n, "input"));
    tr.bindings.push_back(bind("x̄", "样本均值", xbar, "input"));
    tr.bindings.push_back(bind("s", "标准差", s, "input"));
    tr.bindings.push_back(bind(result_symbol, "检验统计量", stat, "intermediate"));
    tr.bindings.push_back(bind("P", "P 值", p, "result"));

    push_step(tr.steps, make_step(1, "样本均值", "x̄ = Σx/n", "x̄ = " + xbar, xbar));
    push_step(tr.steps, make_step(
        2, "标准误", "SE = s/√n", "SE = " + s + "/√" + n, s));
    push_step(tr.steps, make_step(
        3, "检验统计量", plain_formula, result_symbol + " = " + stat, stat));

    tr.result_value = require_value(stat, p);
    tr.substituted_text = std::string(result_symbol) + " = (" + xbar + ") / (" + s
        + " / √" + n + ") = " + stat;
    return finish_l3(page, std::move(tr));
}

// ---------- proportion family ----------
bool attach_proportion_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Binomial_test";
    tr.result_symbol = result_symbol;

    std::string events = require_value(
        page.facts.proportion.has_value()
            ? std::to_string(page.facts.proportion->events) : std::string{},
        table_value(page, "Events", "Events (x)", "x"));
    std::string trials = require_value(
        page.facts.proportion.has_value()
            ? std::to_string(page.facts.proportion->trials) : std::string{},
        table_value(page, "Trials", "N", "n"));
    std::string phat = require_value(
        page.facts.proportion.has_value()
            ? opt_fmt(page.facts.proportion->proportion) : std::string{},
        table_value(page, "Sample p", "p̂", "Proportion"));
    std::string stat = require_value(
        table_value(page, "Z", "Statistic", "统计量"));
    std::string p = require_value(
        page.facts.proportion.has_value()
            ? opt_fmt(page.facts.proportion->p_value) : std::string{},
        table_value(page, "P", "P-Value", "P 值"));

    tr.bindings.push_back(bind("x", "事件数", events, "input"));
    tr.bindings.push_back(bind("n", "试验数", trials, "input"));
    tr.bindings.push_back(bind("p̂", "样本比例", phat, "intermediate"));
    tr.bindings.push_back(bind("Z", "Z 统计量", stat, "intermediate"));
    tr.bindings.push_back(bind("P", "P 值", p, "result"));

    push_step(tr.steps, make_step(
        1, "样本比例", "p̂ = x/n", "p̂ = " + events + "/" + trials + " = " + phat, phat));
    push_step(tr.steps, make_step(
        2, "标准误", "SE = √[p̂(1−p̂)/n]", "SE from p̂=" + phat, phat));
    push_step(tr.steps, make_step(
        3, "Z 统计量", plain_formula, "Z = " + stat, stat));

    tr.result_value = require_value(stat, p);
    tr.substituted_text = "p̂ = " + events + "/" + trials + " = " + phat + " → Z = " + stat;
    return finish_l3(page, std::move(tr));
}

// ---------- regression family ----------
bool attach_regression_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Linear_regression";
    tr.result_symbol = result_symbol;

    std::string r2 = require_value(
        page.facts.regression.has_value()
            ? opt_fmt(page.facts.regression->r_squared) : std::string{},
        table_value(page, "R-Sq", "R²", "R-Square"));
    std::string coef = require_value(
        table_value(page, "Constant", "Intercept", "截距", "Coef", "Coefficient"));
    std::string se = require_value(table_value(page, "SE Coef", "SE", "标准误"));
    std::string t = require_value(table_value(page, "T", "t", "T-Value"));

    tr.bindings.push_back(bind("R²", "决定系数", r2, "result"));
    tr.bindings.push_back(bind("β₀", "截距/系数", coef, "input"));
    tr.bindings.push_back(bind("SE", "标准误", se, "intermediate"));
    tr.bindings.push_back(bind("t", "t 值", t, "intermediate"));

    push_step(tr.steps, make_step(
        1, "拟合模型", plain_formula, "R² = " + r2, r2));
    push_step(tr.steps, make_step(
        2, "系数估计", "β̂ = (X'X)⁻¹X'Y", "β₀ = " + coef + " (SE=" + se + ")", coef));
    push_step(tr.steps, make_step(
        3, "系数检验", "t = β/SE", "t = " + t, t));

    tr.result_value = require_value(r2, coef);
    tr.substituted_text = plain_formula + " → R² = " + r2 + ", β₀ = " + coef;
    return finish_l3(page, std::move(tr));
}

// ---------- MSA / gage R&R family ----------
bool attach_msa_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Measurement_system_analysis";
    tr.result_symbol = result_symbol;

    std::string grr = require_value(
        page.facts.msa.has_value()
            ? opt_fmt(page.facts.msa->gage_percent_study_variation) : std::string{},
        table_value(page, "%Study Var", "%GRR", "Gage R&R", "Study Var"));
    std::string ndc = require_value(
        page.facts.msa.has_value() ? opt_fmt(page.facts.msa->ndc) : std::string{},
        table_value(page, "ndc", "NDC", "Distinct Categories"));
    std::string repeat = require_value(
        page.facts.msa.has_value()
            ? opt_fmt(page.facts.msa->gage_percent_contribution) : std::string{},
        table_value(page, "%Contribution", "Repeatability"));

    tr.bindings.push_back(bind("%GRR", "Gage R&R %", grr, "result"));
    tr.bindings.push_back(bind("ndc", "可区分类别数", ndc, "intermediate"));
    tr.bindings.push_back(bind("%Repeat", "重复性 %", repeat, "input"));

    push_step(tr.steps, make_step(
        1, "方差分量", "ANOVA → σ²_part, σ²_gage", "%GRR = " + grr, grr));
    push_step(tr.steps, make_step(
        2, "ndc 计算", "ndc = floor(1.41·σ_part/σ_gage)", "ndc = " + ndc, ndc));
    push_step(tr.steps, make_step(
        3, "研究变异", "%Study Var = 6σ_gage/TV", "%GRR = " + grr, grr));

    tr.result_value = require_value(grr, ndc);
    tr.substituted_text = plain_formula + " → %GRR = " + grr + ", ndc = " + ndc;
    return finish_l3(page, std::move(tr));
}

// ---------- generic hypothesis test (chi-square, anova, nonparametric) ----------
bool attach_generic_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
    tr.result_symbol = result_symbol;

    std::string n = require_value(
        page.facts.chi_square.has_value()
            ? std::to_string(page.facts.chi_square->total_count) : std::string{},
        page.facts.nonparametric.has_value()
            ? std::to_string(page.facts.nonparametric->group_count) : std::string{},
        table_value(page, "N", "n", "Total", "有效 N"));
    std::string stat = require_value(
        page.facts.chi_square.has_value()
            ? opt_fmt(page.facts.chi_square->statistic) : std::string{},
        page.facts.nonparametric.has_value()
            ? opt_fmt(page.facts.nonparametric->statistic) : std::string{},
        page.facts.variance.has_value()
            ? opt_fmt(page.facts.variance->statistic) : std::string{},
        table_value(page, "Statistic", "Chi-Sq", "F", "H", "U", "Z", "统计量", "T"));
    std::string p = require_value(
        page.facts.chi_square.has_value()
            ? opt_fmt(page.facts.chi_square->p_value)
            : page.facts.anova.has_value()
                ? opt_fmt(page.facts.anova->p_value)
                : page.facts.nonparametric.has_value()
                    ? opt_fmt(page.facts.nonparametric->p_value)
                    : page.facts.variance.has_value()
                        ? opt_fmt(page.facts.variance->p_value)
                        : std::string{},
        table_value(page, "P", "P-Value", "P 值"));

    tr.bindings.push_back(bind("N", "样本量/规模", n, "input"));
    tr.bindings.push_back(bind(result_symbol, "检验统计量", stat, "intermediate"));
    tr.bindings.push_back(bind("P", "P 值", p, "result"));

    push_step(tr.steps, make_step(1, "汇总数据", "N = " + n, "N = " + n, n));
    push_step(tr.steps, make_step(
        2, "计算统计量", plain_formula, result_symbol + " = " + stat, stat));
    push_step(tr.steps, make_step(
        3, "显著性", "P = P(" + std::string(result_symbol) + " ≥ obs)", "P = " + p, p));

    tr.result_value = require_value(stat, p);
    tr.substituted_text = plain_formula + " → " + result_symbol + " = " + stat + ", P = " + p;
    return finish_l3(page, std::move(tr));
}

// ---------- capability variant ----------
bool attach_capability_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Process_capability_index";
    tr.result_symbol = result_symbol;

    const auto& cfg = page.configuration.specifications;
    std::string usl = require_value(
        cfg.upper.has_value() ? fmt_num(*cfg.upper) : std::string{},
        table_value(page, "USL"));
    std::string lsl = require_value(
        cfg.lower.has_value() ? fmt_num(*cfg.lower) : std::string{},
        table_value(page, "LSL"));
    std::string cpk = require_value(
        page.facts.capability.has_value()
            ? opt_fmt(page.facts.capability->cpk) : std::string{},
        page.facts.nonparametric_capability.has_value()
            ? opt_fmt(page.facts.nonparametric_capability->cnpk) : std::string{},
        table_value(page, "Cpk", "Ppk", "Cp"));
    std::string sigma = require_value(
        page.facts.tolerance.has_value()
            ? opt_fmt(page.facts.tolerance->standard_deviation) : std::string{},
        table_value(page, "StDev", "Within", "σ"));

    tr.bindings.push_back(bind("USL", "规格上限", usl, "input"));
    tr.bindings.push_back(bind("LSL", "规格下限", lsl, "input"));
    tr.bindings.push_back(bind("σ", "标准差", sigma, "input"));
    tr.bindings.push_back(bind(result_symbol, "能力指数", cpk, "result"));

    push_step(tr.steps, make_step(
        1, "规格宽度", "USL−LSL", "(" + usl + "−" + lsl + ")", usl));
    push_step(tr.steps, make_step(
        2, "过程 σ", "σ from within/between", "σ = " + sigma, sigma));
    push_step(tr.steps, make_step(
        3, "能力指数", plain_formula, result_symbol + " = " + cpk, cpk));

    tr.result_value = cpk;
    tr.substituted_text = plain_formula + " → " + result_symbol + " = " + cpk;
    return finish_l3(page, std::move(tr));
}
'''


def emit_try_attach(cid: str, meta: dict[str, str]) -> str:
    fam = meta.get("family", "generic")
    fid = cpp_escape(meta["formula_id"])
    title = cpp_escape(meta["title"])
    plain = cpp_escape(meta["plain_formula"])
    rsym = cpp_escape(meta["result_symbol"])

    if fam == "spc":
        spc_kind = meta.get("spc_kind", "imr")
        if spc_kind == "imr":
            sd, sb = "移动极差 σ", "σ = MR̄/d₂"
        elif spc_kind in ("p", "np", "c", "u"):
            sd, sb = "比例/计数 σ", "σ = √[p̄(1−p̄)/n]"
        elif spc_kind == "ewma":
            sd, sb = "EWMA 渐近 σ", "σ_Z = σ√[λ/(2−λ)]"
        else:
            sd, sb = "估计 σ", "σ from data"
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    return attach_spc_l3(page, "{cid}", "{fid}", "{title}", "{plain}", "{rsym}",
        "{cpp_escape(sd)}", "{cpp_escape(sb)}");
}}
"""
    if fam == "t_test":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    return attach_t_test_l3(page, "{cid}", "{fid}", "{title}", "{plain}", "{rsym}");
}}
"""
    if fam == "proportion":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    return attach_proportion_l3(page, "{cid}", "{fid}", "{title}", "{plain}", "{rsym}");
}}
"""
    if fam == "regression":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    return attach_regression_l3(page, "{cid}", "{fid}", "{title}", "{plain}", "{rsym}");
}}
"""
    if fam == "msa":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    return attach_msa_l3(page, "{cid}", "{fid}", "{title}", "{plain}", "{rsym}");
}}
"""
    if fam == "capability":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    return attach_capability_l3(page, "{cid}", "{fid}", "{title}", "{plain}", "{rsym}");
}}
"""
    if fam == "normality":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    ComputationTrace tr;
    tr.command_id = "{cid}";
    tr.formula_id = "{fid}";
    tr.title = "{title}";
    tr.plain_formula = "{plain}";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Normality_test";
    tr.result_symbol = "{rsym}";
    std::string n = require_value(
        page.facts.normality.has_value()
            ? std::to_string(page.facts.normality->n) : std::string{{}},
        table_value(page, "N", "n"));
    std::string ad = require_value(
        page.facts.normality.has_value()
            ? opt_fmt(page.facts.normality->anderson_darling) : std::string{{}},
        table_value(page, "AD", "A-Squared", "Anderson-Darling"));
    std::string p = require_value(
        page.facts.normality.has_value()
            ? opt_fmt(page.facts.normality->p_value) : std::string{{}},
        table_value(page, "P", "P-Value", "P 值"));
    tr.bindings.push_back(bind("N", "样本量", n, "input"));
    tr.bindings.push_back(bind("A²", "Anderson-Darling", ad, "intermediate"));
    tr.bindings.push_back(bind("P", "P 值", p, "result"));
    push_step(tr.steps, make_step(1, "排序标准化", "F(x_(i)) from sorted x", "N = " + n, n));
    push_step(tr.steps, make_step(2, "A² 统计量", "{plain}", "A² = " + ad, ad));
    push_step(tr.steps, make_step(3, "P 值", "Stephens 修正", "P = " + p, p));
    tr.result_value = require_value(p, ad);
    tr.substituted_text = "{plain} → A² = " + ad + ", P = " + p;
    return finish_l3(page, std::move(tr));
}}
"""
    if fam == "outlier":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    ComputationTrace tr;
    tr.command_id = "{cid}";
    tr.formula_id = "{fid}";
    tr.title = "{title}";
    tr.plain_formula = "{plain}";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Grubbs%27_test_for_outliers";
    tr.result_symbol = "{rsym}";
    std::string n = require_value(
        page.facts.outlier_test.has_value()
            ? std::to_string(page.facts.outlier_test->n) : std::string{{}},
        table_value(page, "N", "n"));
    std::string xbar = require_value(
        page.facts.outlier_test.has_value()
            ? opt_fmt(page.facts.outlier_test->mean) : std::string{{}},
        table_value(page, "Mean", "均值"));
    std::string s = require_value(
        page.facts.outlier_test.has_value()
            ? opt_fmt(page.facts.outlier_test->standard_deviation) : std::string{{}},
        table_value(page, "StDev", "StDev(Overall)"));
    std::string g = require_value(
        page.facts.outlier_test.has_value()
            ? opt_fmt(page.facts.outlier_test->g_statistic) : std::string{{}},
        table_value(page, "G", "Statistic", "统计量"));
    std::string p = require_value(
        page.facts.outlier_test.has_value()
            ? opt_fmt(page.facts.outlier_test->p_value) : std::string{{}},
        table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("n", "样本量", n, "input"));
    tr.bindings.push_back(bind("x̄", "均值", xbar, "input"));
    tr.bindings.push_back(bind("s", "标准差", s, "input"));
    tr.bindings.push_back(bind("G", "Grubbs G", g, "result"));
    tr.bindings.push_back(bind("P", "P 值", p, "intermediate"));
    push_step(tr.steps, make_step(1, "样本均值", "x̄ = Σx/n", "x̄ = " + xbar, xbar));
    push_step(tr.steps, make_step(2, "Grubbs G", "{plain}", "G = " + g, g));
    push_step(tr.steps, make_step(3, "P 值", "G critical table", "P = " + p, p));
    tr.result_value = require_value(g, p);
    tr.substituted_text = "G = max|x_i − " + xbar + "| / " + s + " = " + g;
    return finish_l3(page, std::move(tr));
}}
"""
    if fam == "correlation":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    ComputationTrace tr;
    tr.command_id = "{cid}";
    tr.formula_id = "{fid}";
    tr.title = "{title}";
    tr.plain_formula = "{plain}";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Pearson_correlation_coefficient";
    tr.result_symbol = "{rsym}";
    std::string n = require_value(
        page.facts.correlation.has_value()
            ? std::to_string(page.facts.correlation->n) : std::string{{}},
        table_value(page, "N", "n"));
    std::string r = require_value(table_value(page, "r", "R", "Correlation", "Pearson"));
    std::string p = require_value(table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("n", "样本量", n, "input"));
    tr.bindings.push_back(bind("r", "相关系数", r, "result"));
    tr.bindings.push_back(bind("P", "P 值", p, "intermediate"));
    push_step(tr.steps, make_step(1, "离差乘积和", "Σ(x−x̄)(y−ȳ)", "n = " + n, n));
    push_step(tr.steps, make_step(2, "Pearson r", "{plain}", "r = " + r, r));
    push_step(tr.steps, make_step(3, "显著性", "t = r√[(n−2)/(1−r²)]", "P = " + p, p));
    tr.result_value = require_value(r, p);
    tr.substituted_text = "{plain} → r = " + r;
    return finish_l3(page, std::move(tr));
}}
"""
    if fam == "equivalence":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    ComputationTrace tr;
    tr.command_id = "{cid}";
    tr.formula_id = "{fid}";
    tr.title = "{title}";
    tr.plain_formula = "{plain}";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/equivalence_test";
    tr.result_symbol = "{rsym}";
    std::string diff = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->difference) : std::string{{}},
        table_value(page, "Difference", "差值", "Mean"));
    std::string lo = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_lower) : std::string{{}},
        table_value(page, "Lower", "CI Lower", "下限"));
    std::string hi = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_upper) : std::string{{}},
        table_value(page, "Upper", "CI Upper", "上限"));
    std::string p = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->p_lower) : std::string{{}},
        table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("Δ", "点估计", diff, "input"));
    tr.bindings.push_back(bind("CI", "1−2α 置信区间", lo + " ~ " + hi, "intermediate"));
    tr.bindings.push_back(bind("P", "TOST P", p, "result"));
    push_step(tr.steps, make_step(1, "点估计", "Δ = estimate", "Δ = " + diff, diff));
    push_step(tr.steps, make_step(2, "TOST 区间", "{plain}", lo + " ≤ Δ ≤ " + hi, hi));
    push_step(tr.steps, make_step(3, "等价判定", "P(Δ ≤ −δ) & P(Δ ≥ δ)", "P = " + p, p));
    tr.result_value = require_value(p, diff);
    tr.substituted_text = "{plain} → Δ = " + diff + ", CI=[" + lo + "," + hi + "]";
    return finish_l3(page, std::move(tr));
}}
"""
    if fam == "poisson_rate":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    ComputationTrace tr;
    tr.command_id = "{cid}";
    tr.formula_id = "{fid}";
    tr.title = "{title}";
    tr.plain_formula = "{plain}";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Poisson_distribution";
    tr.result_symbol = "{rsym}";
    std::string events = require_value(
        page.facts.poisson_rate.has_value()
            ? std::to_string(page.facts.poisson_rate->events) : std::string{{}},
        table_value(page, "Events", "Events (x)"));
    std::string exposure = require_value(
        page.facts.poisson_rate.has_value()
            ? fmt_num(page.facts.poisson_rate->exposure) : std::string{{}},
        table_value(page, "Exposure", "Length of exposure"));
    std::string rate = require_value(
        page.facts.poisson_rate.has_value()
            ? opt_fmt(page.facts.poisson_rate->rate) : std::string{{}},
        table_value(page, "Sample Rate", "Rate"));
    std::string z = require_value(
        page.facts.poisson_rate.has_value()
            ? opt_fmt(page.facts.poisson_rate->z_statistic) : std::string{{}},
        table_value(page, "Z", "Statistic"));
    tr.bindings.push_back(bind("events", "事件数", events, "input"));
    tr.bindings.push_back(bind("T", "暴露量", exposure, "input"));
    tr.bindings.push_back(bind("λ̂", "样本率", rate, "intermediate"));
    tr.bindings.push_back(bind("Z", "Z", z, "result"));
    push_step(tr.steps, make_step(1, "样本率", "λ̂ = events/T", "λ̂ = " + rate, rate));
    push_step(tr.steps, make_step(2, "标准误", "SE = √(λ/T)", "T = " + exposure, exposure));
    push_step(tr.steps, make_step(3, "Z 统计量", "{plain}", "Z = " + z, z));
    tr.result_value = require_value(z, rate);
    tr.substituted_text = "λ̂ = " + events + "/" + exposure + " = " + rate;
    return finish_l3(page, std::move(tr));
}}
"""
    if fam == "tolerance":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    ComputationTrace tr;
    tr.command_id = "{cid}";
    tr.formula_id = "{fid}";
    tr.title = "{title}";
    tr.plain_formula = "{plain}";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Tolerance_interval";
    tr.result_symbol = "{rsym}";
    std::string n = require_value(
        page.facts.tolerance.has_value()
            ? std::to_string(page.facts.tolerance->valid_count) : std::string{{}},
        table_value(page, "N", "n"));
    std::string mean = require_value(
        page.facts.tolerance.has_value()
            ? opt_fmt(page.facts.tolerance->mean) : std::string{{}},
        table_value(page, "Mean", "均值"));
    std::string s = require_value(
        page.facts.tolerance.has_value()
            ? opt_fmt(page.facts.tolerance->standard_deviation) : std::string{{}},
        table_value(page, "StDev", "StDev"));
    std::string k = require_value(
        page.facts.tolerance.has_value()
            ? opt_fmt(page.facts.tolerance->k_factor) : std::string{{}},
        table_value(page, "K", "k", "Factor"));
    tr.bindings.push_back(bind("n", "N", n, "input"));
    tr.bindings.push_back(bind("x̄", "均值", mean, "input"));
    tr.bindings.push_back(bind("s", "标准差", s, "input"));
    tr.bindings.push_back(bind("k", "k 因子", k, "result"));
    push_step(tr.steps, make_step(1, "x̄, s", "descriptive", "x̄ = " + mean + ", s = " + s, mean));
    push_step(tr.steps, make_step(2, "k 因子", "from (p, γ, n)", "k = " + k, k));
    push_step(tr.steps, make_step(3, "区间", "x̄ ± k·s", "{plain}", k));
    tr.result_value = k;
    tr.substituted_text = "{plain} → k = " + k + ", x̄ = " + mean;
    return finish_l3(page, std::move(tr));
}}
"""
    if fam == "reliability":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    ComputationTrace tr;
    tr.command_id = "{cid}";
    tr.formula_id = "{fid}";
    tr.title = "{title}";
    tr.plain_formula = "{plain}";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Survival_analysis";
    tr.result_symbol = "{rsym}";
    std::string n = require_value(
        page.facts.reliability.has_value() && page.facts.reliability->valid_count.has_value()
            ? std::to_string(*page.facts.reliability->valid_count) : std::string{{}},
        page.facts.warranty.has_value()
            ? std::to_string(page.facts.warranty->valid_count) : std::string{{}},
        table_value(page, "N", "Valid"));
    std::string param = require_value(
        page.facts.reliability.has_value()
            ? opt_fmt(page.facts.reliability->scale) : std::string{{}},
        page.facts.warranty.has_value()
            ? fmt_num(page.facts.warranty->reliability_at_warranty) : std::string{{}},
        table_value(page, "Scale", "η", "Reliability"));
    std::string p = require_value(table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("N", "有效观测", n, "input"));
    tr.bindings.push_back(bind("{rsym}", "关键参数", param, "result"));
    push_step(tr.steps, make_step(1, "数据汇总", "failures/censoring", "N = " + n, n));
    push_step(tr.steps, make_step(2, "模型拟合", "{plain}", "{rsym} = " + param, param));
    push_step(tr.steps, make_step(3, "推断", "SE/CI", param, param));
    tr.result_value = param;
    tr.substituted_text = "{plain} → " + param;
    return finish_l3(page, std::move(tr));
}}
"""
    if fam == "mcnemar":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    ComputationTrace tr;
    tr.command_id = "{cid}";
    tr.formula_id = "{fid}";
    tr.title = "{title}";
    tr.plain_formula = "{plain}";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/McNemar%27s_test";
    tr.result_symbol = "{rsym}";
    std::string b = require_value(
        page.facts.mcnemar.has_value()
            ? std::to_string(page.facts.mcnemar->b) : std::string{{}},
        table_value(page, "b", "B"));
    std::string c = require_value(
        page.facts.mcnemar.has_value()
            ? std::to_string(page.facts.mcnemar->c) : std::string{{}},
        table_value(page, "c", "C"));
    std::string chi = require_value(
        page.facts.mcnemar.has_value()
            ? opt_fmt(page.facts.mcnemar->chi_square) : std::string{{}},
        table_value(page, "Chi-Sq", "Statistic"));
    tr.bindings.push_back(bind("b", "discordant b", b, "input"));
    tr.bindings.push_back(bind("c", "discordant c", c, "input"));
    tr.bindings.push_back(bind("χ²", "McNemar χ²", chi, "result"));
    push_step(tr.steps, make_step(1, "discordant", "b, c counts", "b=" + b + ", c=" + c, b));
    push_step(tr.steps, make_step(2, "χ²", "{plain}", "χ² = " + chi, chi));
    push_step(tr.steps, make_step(3, "df=1", "χ²_{1}", chi, chi));
    tr.result_value = chi;
    tr.substituted_text = "χ² = (" + b + "−" + c + ")²/(" + b + "+" + c + ") = " + chi;
    return finish_l3(page, std::move(tr));
}}
"""
    if fam == "cochran_q":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    return attach_generic_l3(page, "{cid}", "{fid}", "{title}", "{plain}", "{rsym}");
}}
"""
    if fam == "cross_tab":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    ComputationTrace tr;
    tr.command_id = "{cid}";
    tr.formula_id = "{fid}";
    tr.title = "{title}";
    tr.plain_formula = "{plain}";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Contingency_table";
    tr.result_symbol = "{rsym}";
    std::string n = require_value(
        page.facts.cross_tab.has_value()
            ? std::to_string(page.facts.cross_tab->total_count) : std::string{{}},
        table_value(page, "Total", "N"));
    tr.bindings.push_back(bind("N", "合计", n, "result"));
    push_step(tr.steps, make_step(1, "行合计", "row sums", "N = " + n, n));
    push_step(tr.steps, make_step(2, "列合计", "col sums", n, n));
    push_step(tr.steps, make_step(3, "cell %", "{plain}", n, n));
    tr.result_value = n;
    tr.substituted_text = "{plain} → N = " + n;
    return finish_l3(page, std::move(tr));
}}
"""
    if fam == "adf":
        return f"""bool try_attach_{cid}(OutputPage& page)
{{
    ComputationTrace tr;
    tr.command_id = "{cid}";
    tr.formula_id = "{fid}";
    tr.title = "{title}";
    tr.plain_formula = "{plain}";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Augmented_Dickey%E2%80%93Fuller_test";
    tr.result_symbol = "{rsym}";
    std::string gamma = require_value(
        page.facts.adf.has_value()
            ? opt_fmt(page.facts.adf->tau) : std::string{{}},
        table_value(page, "Gamma", "t", "Statistic"));
    std::string p = require_value(table_value(page, "P", "P-Value", "MacKinnon P"));
    tr.bindings.push_back(bind("γ", "γ (unit root)", gamma, "result"));
    tr.bindings.push_back(bind("P", "P 值", p, "intermediate"));
    push_step(tr.steps, make_step(1, "差分回归", "{plain}", "γ = " + gamma, gamma));
    push_step(tr.steps, make_step(2, "ADF t", "t on γ", gamma, gamma));
    push_step(tr.steps, make_step(3, "MacKinnon P", "critical values", "P = " + p, p));
    tr.result_value = require_value(gamma, p);
    tr.substituted_text = "{plain} → γ = " + gamma;
    return finish_l3(page, std::move(tr));
}}
"""
    # default: generic
    return f"""bool try_attach_{cid}(OutputPage& page)
{{
    return attach_generic_l3(page, "{cid}", "{fid}", "{title}", "{plain}", "{rsym}");
}}
"""


def emit_cpp(stubs: list[str]) -> str:
    try_fns = "\n".join(emit_try_attach(cid, DEEP_META[cid]) for cid in stubs)
    dispatch = "\n".join(
        f'    if (command_id == "{cid}") {{\n        return try_attach_{cid}(page);\n    }}'
        for cid in stubs
    )
    return f"""#include "application/computation_trace_attach_deep.h"

#include "application/computation_trace_helpers.h"

#include <string>
#include <utility>
#include <vector>

namespace datalab::application {{
namespace {{

using datalab::domain::ComputationStep;
using datalab::domain::ComputationTrace;
using datalab::domain::OutputPage;

using namespace datalab::application::trace_helpers;

{emit_family_helpers()}

{try_fns}

}}  // namespace

bool attach_deep_trace(OutputPage& page, const std::string& command_id)
{{
{dispatch}
    return false;
}}

}}  // namespace datalab::application
"""


def patch_attach_cpp(stubs: list[str]) -> None:
    cpp = CPP_ATTACH.read_text(encoding="utf-8")

    if '#include "application/computation_trace_attach_deep.h"' not in cpp:
        cpp = cpp.replace(
            '#include "application/computation_trace_attach.h"',
            '#include "application/computation_trace_attach.h"\n'
            '#include "application/computation_trace_attach_deep.h"',
            1,
        )

    if "attach_deep_trace(page, command_id)" not in cpp:
        cpp = cpp.replace(
            "    static_cast<void>(k_g9_covered_commands);\n",
            "    static_cast<void>(k_g9_covered_commands);\n\n"
            "    if (attach_deep_trace(page, command_id)) {\n"
            "        return;\n"
            "    }\n",
            1,
        )

    # Remove stub blocks (plain_formula contains 主公式)
    pattern = re.compile(
        r'\n    if \(command_id == "[^"]+"\) \{[\s\S]*?'
        r'tr\.plain_formula = "[^"]*主公式[^"]*";[\s\S]*?'
        r'        return;\n    \}',
        re.MULTILINE,
    )
    cpp, n_removed = pattern.subn("", cpp)

    # Enhance kept pilots with make_step (add trace_helpers include usage inline)
    enhancements = {
        "capability": '''
        push_step(tr.steps, make_step(1, "上规格距离", "Cpu = (USL−μ)/(3σ)",
            "(" + usl + "−" + mu + ")/(3·" + sigma + ")", usl));
        push_step(tr.steps, make_step(2, "下规格距离", "Cpl = (μ−LSL)/(3σ)",
            "(" + mu + "−" + lsl + ")/(3·" + sigma + ")", lsl));''',
        "one_sample_t": '''
        push_step(tr.steps, make_step(1, "样本均值", "x̄ = Σx/n", "x̄ = " + xbar, xbar));
        push_step(tr.steps, make_step(2, "标准误", "SE = s/√n", "SE = " + s + "/√" + n, s));
        push_step(tr.steps, make_step(3, "t 统计量", "t = (x̄−μ₀)/SE",
            "t = " + tstat, tstat));''',
        "weibayes": '''
        push_step(tr.steps, make_step(1, "形状先验 β", "β fixed", "β = " + beta, beta));
        push_step(tr.steps, make_step(2, "失效数 r", "r = failures", "r = " + r, r));
        push_step(tr.steps, make_step(3, "尺度 η", "η = (Σ t_i^β / r)^(1/β)",
            "η = " + eta, eta));''',
        "descriptive": '''
        const std::string xbar = first_nonempty({
            page.facts.descriptive.has_value()
                ? opt_fmt(page.facts.descriptive->mean) : std::string{},
            table_value(page, "Mean", "均值", "Average")});
        const std::string n_desc = first_nonempty({
            page.facts.descriptive.has_value()
                ? std::to_string(page.facts.descriptive->n) : std::string{},
            table_value(page, "N", "n", "有效 N")});
        tr.bindings.clear();
        tr.bindings.push_back(bind("n", "样本量 N", n_desc, "input"));
        tr.bindings.push_back(bind("x̄", "均值", xbar, "result"));
        tr.result_value = xbar;
        push_step(tr.steps, make_step(1, "求和", "Σx_i", "n = " + n_desc, n_desc));
        push_step(tr.steps, make_step(2, "均值", "x̄ = Σx_i/n", "x̄ = " + xbar, xbar));
        tr.substituted_text = "x̄ = Σx_i / " + n_desc + " = " + xbar;''',
    }

    # Add trace_helpers for make_step in attach.cpp
    if '#include "application/computation_trace_helpers.h"' not in cpp:
        cpp = cpp.replace(
            '#include "application/computation_trace_attach_deep.h"',
            '#include "application/computation_trace_attach_deep.h"\n'
            '#include "application/computation_trace_helpers.h"',
            1,
        )
    if "using namespace datalab::application::trace_helpers" not in cpp:
        cpp = cpp.replace(
            "namespace {\n",
            "namespace {\n\nusing namespace datalab::application::trace_helpers;\n",
            1,
        )

    for cid, enh in enhancements.items():
        marker = f'        tr.substituted_text = '
        block_pat = re.compile(
            rf'(    if \(command_id == "{cid}"\) \{{[\s\S]*?)(        tr\.substituted_text = )',
            re.MULTILINE,
        )
        m = block_pat.search(cpp)
        if m and "push_step(tr.steps" not in m.group(1):
            cpp = block_pat.sub(rf"\1{enh}\n\2", cpp, count=1)

    CPP_ATTACH.write_text(cpp, encoding="utf-8")
    print(f"patched attach.cpp: removed {n_removed} stub blocks")


def patch_cmake() -> None:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    if "computation_trace_attach_deep.cpp" not in cmake:
        cmake = cmake.replace(
            "    src/application/computation_trace_attach.cpp\n"
            "    src/application/computation_trace_attach.h\n",
            "    src/application/computation_trace_attach.cpp\n"
            "    src/application/computation_trace_attach.h\n"
            "    src/application/computation_trace_attach_deep.cpp\n"
            "    src/application/computation_trace_attach_deep.h\n",
            1,
        )
        (ROOT / "CMakeLists.txt").write_text(cmake, encoding="utf-8")
        print("patched CMakeLists.txt")


def main() -> None:
    stubs = extract_stubs()
    help_map = load_help_formulas()
    merge_help_formulas(stubs, help_map)
    print(f"stubs={len(stubs)}")
    if len(stubs) != 79:
        print(f"WARNING: expected 79 stubs, got {len(stubs)}", file=sys.stderr)
    OUT_H.write_text(emit_header(stubs), encoding="utf-8")
    OUT_CPP.write_text(emit_cpp(stubs), encoding="utf-8")
    patch_cmake()
    print(f"wrote {OUT_H.name}, {OUT_CPP.name}")


def patch_only() -> None:
    stubs = extract_stubs()
    patch_attach_cpp(stubs)
    patch_cmake()


if __name__ == "__main__":
    import sys as _sys
    if len(_sys.argv) > 1 and _sys.argv[1] == "--patch-only":
        patch_only()
    else:
        main()
