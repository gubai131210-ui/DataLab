#!/usr/bin/env python3
"""Generate a self-contained algorithm help catalog for end users."""

from __future__ import annotations

import json
from pathlib import Path

ACCESSED = "2026-08-20"
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "resources" / "help" / "algorithm_help.json"

MISSING = (
    "空单元格、*、NA、N/A、NaN 视为缺失。N 为有效数值个数，N* 为缺失个数。"
    "分析跳过缺失单元格，不从工作表删除行。图上保留原始行号 source_row。"
    "重导入另一份数据后，旧排除行、旧输出页与旧撤销记录全部失效。"
)
INTERP_LIMIT = (
    "解释层只陈述统计证据、假设状态和不可计算原因，"
    "不写过程合格、量具通过、分布已证明、规格已满足或必须删点。"
)
COMPLETE = (
    "默认 complete-case：同一分析用到的各列在同一行都必须有效，才进入计算。"
    "错位缺失不会拿上一行的值去填补。"
)

MINITAB = {
    "desc": ("官方描述统计说明", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/display-descriptive-statistics/before-you-start/overview/"),
    "t": ("官方 t 检验方法", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-sample-t/methods-and-formulas/methods-and-formulas/"),
    "ad": ("官方正态性检验", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/normality-test/methods-and-formulas/methods-and-formulas/"),
    "corr": ("官方相关分析", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/correlation/methods-and-formulas/methods-and-formulas/"),
    "prop": ("官方单比例方法", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-proportion/methods-and-formulas/methods-and-formulas/"),
    "poisson": ("官方泊松率方法", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-sample-poisson-rate/methods-and-formulas/methods-and-formulas/"),
    "anova": ("官方单因素 ANOVA", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/one-way-anova/methods-and-formulas/methods-and-formulas/"),
    "crosstab": ("官方交叉表与卡方", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/how-to/cross-tabulation-and-chi-square/interpret-the-results/all-statistics-and-graphs/tabulated-statistics/"),
    "reg": ("官方回归 ANOVA", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-regression-model/methods-and-formulas/analysis-of-variance/"),
    "imr": ("官方 I-MR 估计选项", "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/i-mr-chart/perform-the-analysis/i-mr-options/specify-estimation-options/"),
    "xbar": ("官方 Xbar-R 方法", "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-subgroups/xbar-r-chart/methods-and-formulas/r-chart/"),
    "laney": ("官方 Laney P' 方法", "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/attributes-charts/laney-p-chart/methods-and-formulas/methods-and-formulas/"),
    "tests": ("官方特殊原因测试", "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/basics/using-tests-for-special-causes/"),
    "cap": ("官方正态能力方法", "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/methods-and-formulas/methods/"),
    "gage": ("官方交叉 Gage R&R", "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/crossed-gage-r-r-study/"),
    "lin": ("官方 Gage Linearity 方法", "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-linearity-and-bias-study/methods-and-formulas/gage-linearity/"),
    "kappa": ("官方 Kappa 统计量", "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/attribute-agreement-analysis/attribute-agreement-analysis/methods-and-formulas/kappa-statistics/"),
    "rel": ("官方可靠性分析", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/supporting-topics/basics/reliability-analyses-in-minitab/"),
    "arima": ("官方 ARIMA 方法", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/time-series/how-to/arima/methods-and-formulas/methods-and-formulas/"),
    "doe": ("官方析因 ANOVA 方法", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-factorial-design/methods-and-formulas/analysis-of-variance/"),
    "rsm": ("官方响应曲面模型信息", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/response-surface/analyze-response-surface-design/methods-and-formulas/model-information/"),
    "spc": ("官方特殊原因测试", "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/basics/using-tests-for-special-causes/"),
    "rules": ("官方控制图测试默认选项", "https://support.minitab.com/en-us/minitab/help-and-how-to/minitab-environment/settings-and-defaults/control-charts-and-quality-tools/tests/"),
    "pca": ("官方主成分方法", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/principal-components/methods-and-formulas/methods-and-formulas/"),
    "graph": ("官方图形编辑说明", "https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-editing-basics/edit-graphs/"),
    "tol": ("官方正态容差区间", "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/tolerance-intervals-normal-distribution/methods-and-formulas/methods-and-formulas/"),
    "grubbs": ("NIST Grubbs 检验", "https://www.itl.nist.gov/div898/handbook/eda/section3/eda35h1.htm"),
    "gof": ("官方卡方拟合优度", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/how-to/chi-square-goodness-of-fit-test/methods-and-formulas/methods-and-formulas/"),
    "gchart": ("官方 G 图方法", "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/rare-event-charts/g-chart/methods-and-formulas/methods-and-formulas/"),
    "tchart": ("官方 T 图方法", "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/rare-event-charts/t-chart/methods-and-formulas/methods-and-formulas/"),
    "power": ("官方单样本 t 功效", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/power-and-sample-size/how-to/hypothesis-tests/power-and-sample-size-for-1-sample-t/methods-and-formulas/methods-and-formulas/"),
    "mood": ("官方 Mood 中位数检验计算", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/mood-s-median-test/methods-and-formulas/calculation-method/"),
    "wilcoxon1": ("官方 1-Sample Wilcoxon 方法", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/1-sample-wilcoxon/methods-and-formulas/methods-and-formulas/"),
    "mcnemar": ("官方 McNemar 说明", "https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/supporting-topics/other-statistics-and-tests/why-should-i-use-mcnemar-s-test/"),
    "nist_np": ("NIST 非参数入口", "https://www.itl.nist.gov/div898/handbook/prd/section4/prd4.htm"),
}


def t(value: str) -> dict:
    return {"type": "text", "value": value}


def frac(num, den) -> dict:
    return {"type": "frac", "num": num if isinstance(num, list) else [t(num)],
            "den": den if isinstance(den, list) else [t(den)]}


def sqrt(content) -> dict:
    return {"type": "sqrt", "content": content if isinstance(content, list) else [t(content)]}


def sub(text: str) -> dict:
    return {"type": "sub", "content": [t(text)]}


def sup(text: str) -> dict:
    return {"type": "sup", "content": [t(text)]}


def abs_n(content) -> dict:
    return {"type": "abs", "content": content if isinstance(content, list) else [t(content)]}


def piecewise(*cases: tuple[str, str]) -> dict:
    content = []
    for result, when in cases:
        content.append({
            "type": "case",
            "content": [t(result)],
            "when": [t(when)],
        })
    return {"type": "piecewise", "content": content}


def as_nodes(item) -> list:
    if isinstance(item, str):
        return [t(item)]
    if isinstance(item, dict):
        return [item]
    return list(item)


def bar(inner) -> dict:
    return {"type": "bar", "content": as_nodes(inner)}


def dbar(inner) -> dict:
    return {"type": "bar", "content": [bar(inner)]}


def heading(text: str) -> dict:
    return {"type": "heading", "value": text}


def line(*parts) -> dict:
    content = []
    for part in parts:
        content.extend(as_nodes(part))
    return {"type": "line", "content": content}


def cases(*lines) -> dict:
    content = []
    for ln in lines:
        content.append({"type": "case", "content": as_nodes(ln), "when": []})
    return {"type": "piecewise", "content": content}


def stack(*nodes) -> dict:
    content = []
    for node in nodes:
        content.extend(as_nodes(node))
    return {"type": "stack", "content": content}


def fb(title: str, plain: str, nodes=None, explanation: str = "", conditions: str = "", note: str = "") -> dict:
    block = {
        "title": title,
        "plain_text": plain,
        "explanation": explanation,
        "conditions": conditions,
        "note": note,
    }
    if nodes:
        block["nodes"] = nodes
    return block


def ref(key: str) -> dict:
    label, url = MINITAB[key]
    return {"label": label, "url": url, "accessed": ACCESSED, "kind": "minitab"}


def entry(
    entry_id: str,
    title: str,
    category: str,
    menu_path: str,
    service: str,
    facts: str,
    test: str,
    purpose: str,
    overview: str,
    inputs: str,
    steps: list[str],
    symbols: list[tuple[str, str]],
    formulas: list[dict],
    decisions: list[str],
    invalid: str,
    outputs: str,
    how_to_read: str,
    assumptions: str,
    refs: list[str],
    status: str = "implemented",
    aliases: list[str] | None = None,
    missing: str = MISSING,
    limits: str = INTERP_LIMIT,
    source: str = "",
) -> dict:
    return {
        "id": entry_id,
        "title": title,
        "category": category,
        "menu_path": menu_path,
        "aliases": aliases or [entry_id, title],
        "implemented_status": status,
        "purpose": purpose,
        "method_overview": overview,
        "input_description": inputs + " " + COMPLETE,
        "missing_value_policy": missing,
        "calculation_steps": steps,
        "symbol_definitions": [{"symbol": s, "meaning": m} for s, m in symbols],
        "decision_rules": decisions,
        "invalid_input_conditions": invalid,
        "output_description": outputs,
        "output_interpretation": how_to_read,
        "assumptions_and_boundaries": assumptions,
        "interpretation_limits": limits,
        "formula_blocks": formulas,
        "reference_links": [ref(k) for k in refs],
        "source_documents": [
            {"label": "维护用公式底稿", "path": source or "docs/statistical-methodology.md", "section": ""},
        ],
        "wiring": {
            "command_id": entry_id,
            "service_method": service,
            "facts_type": facts,
            "primary_test": test,
        },
    }


SPC_TESTS = [
    "Test 1：一点超出该点子组的控制限（y < LCL 或 y > UCL）。",
    "Test 2：连续 9 点严格在中心线同一侧。",
    "Test 3：连续 6 点持续上升或持续下降。",
    "Test 4：连续 14 点上下交替。",
    "Test 5：连续 3 点中至少 2 点位于同侧 2σ 外。",
    "Test 6：连续 5 点中至少 4 点位于同侧 1σ 外。",
    "Test 7：连续 15 点都严格落在中心线两侧 1σ 以内；恰在 1σ 上的点不触发。",
    "Test 8：连续 8 点都在 1σ 外（两侧都算，不要求交替）。",
    "同一点触发多个测试时，图上标签用最小测试编号；表格列出全部失败测试。",
    "R/S/MR 图只适用 Test 1–4；EWMA 只适用 Test 1。",
]


def spc_invalid() -> str:
    return (
        "测量值必须为有限数。子组图：缺失/非法值、固定子组尾部不完整、标签子组大小不一致时只诊断不计算。"
        "属性图：计数与分母必须是非负整数，分母不能为 0；P/NP 还要求不合格品数 ≤ 检验数。"
        "未提供历史参数且无法估计 σ 或 Sigma Z 时中止。"
    )


ENTRIES: list[dict] = []

ENTRIES.append(entry(
    "descriptive", "显示描述性统计", "基础统计", "统计 > 基础统计",
    "descriptive", "descriptive", "descriptive_statistics_test",
    "概括一列或多列数值的位置、散布和形状，并可按 By 变量分组。",
    "对每列有效观测计算均值、样本标准差、标准误、四分位、偏度和超额峰度。四分位使用顺序统计量线性插值。",
    "选择一个或多个数值列；可选一个 By 分类列。",
    ["筛选有限数值，统计 N 与 N*。",
     "若有 By 列，按分类水平分别计算。",
     "计算均值、方差、标准差、SE Mean、最小/最大、范围、四分位、IQR、偏度与超额峰度。",
     "n<2 时不计算标准差；常量列或 n 不足时偏度/峰度显示为不可用，不填 0。"],
    [("n / N", "有效观测个数"), ("N*", "缺失个数"), ("x̄", "样本均值"),
     ("s", "样本标准差，分母 n−1"), ("SE Mean", "s/√n")],
    [fb("均值与标准差", "x̄ = (1/n) Σ x_i ；  s = sqrt[ Σ(x_i−x̄)² / (n−1) ]",
        [t("x̄ = "), frac("Σ xᵢ", "n"), t(" ， s = "), sqrt("Σ(xᵢ − x̄)² / (n−1)")],
        "均值是位置；s 是无偏样本标准差。", "n≥1 才能算均值；n≥2 且不是全部相等才能算 s。"),
     fb("四分位", "位置 p·(n−1) 做线性插值得到 Q1/Q2/Q3；IQR = Q3−Q1；Range = Max−Min。")],
    ["N* 只报告缺失，不参与均值。", "By 水平若有效观测为 0，该水平无统计量。"],
    "未选择数值列则无法运行。非有限值不计入 N。",
    "描述统计表；可选箱线与个体值图。",
    "用 N、N*、均值和 s 了解数据规模与散布。偏度/峰度只是形状描述。",
    "不假设正态。界面小数位不影响内部双精度。",
    ["desc"],
))

ENTRIES.append(entry(
    "normality_test", "正态性检验", "基础统计", "统计 > 基础统计",
    "normality_test", "normality", "quality_statistics_test",
    "用 Anderson–Darling 或 Ryan–Joiner 检验一列数据是否与正态分布一致。",
    "默认 Anderson–Darling：对标准化后的排序观测计算 A² 与 Stephens 修正 A²*，再分段近似 p。可选 Ryan–Joiner：有序样本与正态得分相关 R，再用临界近似与线性插值得到 p。未拒绝不等于已证明正态。",
    "选择一个数值列。方法 anderson_darling（默认）或 ryan_joiner。默认 α=0.05。",
    ["取有限值，排序。",
     "AD：标准化并计算 A²/A²*/p。",
     "RJ：平均秩→Blom 正态得分，计算 R 并插值 p。",
     "比较 p 与 α，输出 reject / fail_to_reject / not_computed。"],
    [("A²", "Anderson–Darling 统计量"), ("A²*", "Stephens 样本量修正"),
     ("R", "Ryan–Joiner 相关统计量"), ("Φ", "标准正态 CDF"), ("α", "显著性水平，默认 0.05")],
    [fb("Anderson–Darling",
        "z_i=(x_(i)−x̄)/s，F_i=Φ(z_i)，A²=−n−(1/n)Σ[(2i−1)ln F_i+(2n+1−2i)ln(1−F_i)]",
        [t("A² = −n − (1/n) Σ [(2i−1) ln Fᵢ + (2n+1−2i) ln(1−Fᵢ)]")],
        "F_i 越偏离均匀，A² 越大。", "n≥3 且 s>0。3≤n<8 仍计算但发出样本量警告。"),
     fb("Ryan–Joiner",
        "R=Σ Y_{(i)} b_i / √[s²(n−1)Σ b_i²]，b_i=Φ⁻¹((r_i−0.375)/(n+0.25))；p 由 cor₀.₁₀/₀.₀₅/₀.₀₁ 临界线性插值。",
        conditions="method=ryan_joiner；formula_reference ≠ Minitab golden。"),
     fb("正态概率图位置", "Blom：p_i=(i+0.625)/(n+0.25)，i 从 0 起；y_i=Φ⁻¹(p_i)。")],
    ["p<α：拒绝正态假设。", "p≥α：未拒绝，不得写成数据服从正态。", "n<3 或 s=0：not_computed。"],
    "未选择测量列、有效观测不足 3、或样本标准差为 0 时不计算。",
    "AD 或 RJ 表 + 正态概率图 + 直方图。悬停点对应原始行。",
    "先看判定状态和 p，再看概率图是否大致直线。",
    "默认 α=0.05、方法 AD。p 限制在 [0,1]。RJ 边界可能报告 p>0.10 或 p<0.01。",
    ["ad"],
))

ENTRIES.append(entry(
    "outlier_test", "异常值检验", "基础统计", "统计 > 基础统计",
    "outlier_test", "outlier_test", "grubbs_test",
    "检验单个最极端观测是否能被看作异常值（Grubbs 或 Dixon r10）。",
    "默认 Grubbs：G=max|y−x̄|/s，P 由 t_{n−2} 反解。可选 Dixon r10：r=(y₂−y₁)/(yₙ−y₁) 或高端对称式；本产品 P 为临界值插值近似（诊断 dixon_p_interpolated），不是 Minitab 积分 golden。检出不等于必须删除。",
    "一个数值列；方法 grubbs（默认）或 dixon_r10；置信水平默认 95%；备择 two_sided / less / greater。",
    ["取有限值。", "按 method 选 Grubbs 或 Dixon r10。", "报告统计量、p、嫌疑值与 source_row。", "画个体值图并高亮嫌疑点。"],
    [("G", "Grubbs 统计量"), ("r", "Dixon r10 统计量"), ("s", "样本标准差"), ("α", "显著性水平")],
    [fb("Grubbs", "G = max|y−x̄| / s ；双侧 p 由 t_{n−2} 反解。",
        [t("G = "), frac([abs_n("y − x̄"), t(" 的最大值")], "s")],
        "只检验最极端的一个点。", "n≥3 且 s>0。"),
     fb("Dixon r10", "低端 r=(y₂−y₁)/(yₙ−y₁)，高端 r=(yₙ−y_{n−1})/(yₙ−y₁)；双侧取较大端。",
        conditions="method=dixon_r10；P 为临界值插值近似（formula_reference）。")],
    ["p<α 提示最极端点与其余点不一致。", "不得据此自动删点或写已确认异常。", "Dixon P 插值不得写成 exact Minitab P。"],
    "n<3、s=0、Dixon n>30 或未选列时不计算。",
    "方法表、检验表与个体值图；异常候选点可识别。",
    "结合专业知识和测量过程判断，而不是只看 p。",
    "一次只针对一个最极端值；Grubbs 为默认，Dixon 为 method 开关。",
    ["grubbs"],
))

ENTRIES.append(entry(
    "correlation", "相关分析", "基础统计", "统计 > 基础统计",
    "correlation", "correlation", "quality_statistics_test",
    "度量两列及以上数值的线性（Pearson）或单调（Spearman）相关，并给出协方差矩阵与可选偏相关。",
    "多列 complete-case 对齐后算相关矩阵；协方差为样本协方差（complete-case）；偏相关仅 Pearson 且 ≥3 列，用精度矩阵法。H0: ρ=0。",
    "至少两列数值。方法 pearson（默认）或 spearman。置信水平默认 95%。可选 partial=yes 求偏相关。",
    ["对所有列 complete-case 对齐。", "Pearson/Spearman 相关矩阵与成对 p/CI。",
     "输出协方差矩阵表。", "partial=yes 且 Pearson 时输出偏相关矩阵。"],
    [("r", "相关系数"), ("Cov", "样本协方差"), ("r_ij·rest", "偏相关系数")],
    [fb("Pearson", "r=Σ[(x−x̄)(y−ȳ)] / sqrt[Σ(x−x̄)² Σ(y−ȳ)²] ； t=r√[(n−2)/(1−r²)]，DF=n−2",
        [t("r = "), frac("Σ(xᵢ−x̄)(yᵢ−ȳ)", "√[Σ(xᵢ−x̄)² Σ(yᵢ−ȳ)²]")]),
     fb("协方差", "Cov(X,Y)=Σ(x−x̄)(y−ȳ)/(n−1)；对角为方差。", conditions="complete-case。"),
     fb("偏相关", "r_ij·rest=−Ω_ij/√(Ω_ii Ω_jj)，Ω 为相关矩阵逆。", conditions="Pearson；n≥p+2。"),
     fb("Fisher z 区间", "z=atanh(r)，SE=1/√(n−3)，z±z_{1−α/2}SE 后再 tanh 还原。",
        conditions="n≥4 且 |r|<1。")],
    ["p 小只说明与 0 相关的证据强，不是因果。", "偏相关不能写成已排除混杂。", "常量列或配对数不足则诊断。"],
    "少于两列、有效配对数不足、偏相关 n 不足或相关矩阵奇异时返回诊断。",
    "相关矩阵、协方差矩阵、可选偏相关矩阵、成对详细结果、散点/矩阵图。",
    "同时看 r、协方差尺度、偏相关与散点形态。",
    "Spearman 用平均秩；偏相关本轮仅 Pearson。",
    ["corr"],
))

ENTRIES.append(entry(
    "one_sample_t", "单样本 t 检验", "基础统计", "统计 > 基础统计",
    "one_sample_t", "t_test", "quality_statistics_test",
    "检验一列均值是否等于给定 μ₀。",
    "t=(x̄−μ₀)/(s/√n)，DF=n−1。默认双侧 95% 区间。s=0 且差值不为 0 时不输出虚假无限 t。",
    "一个数值列、假设均值、置信水平、备择方向。",
    ["取有限值，计算 x̄、s、SE。", "t=(x̄−μ₀)/SE，DF=n−1。", "按双侧或单侧给 p 与区间。"],
    [("μ₀", "假设均值"), ("SE", "s/√n"), ("t", "检验统计量"), ("DF", "n−1")],
    [fb("单样本 t", "d=x̄−μ₀，SE=s/√n，t=d/SE，DF=n−1。双侧区间 d ± t_{1−α/2,DF}·SE。",
        [t("t = "), frac("x̄ − μ₀", "s / √n")])],
    ["默认 H0: μ=μ₀。", "单侧只给对应方向的界限。", "区间含 μ₀ 不得写成显著差异。"],
    "未选列、未给 μ₀、n<2 或 s=0 且 d≠0 时诊断。",
    "均值/标准差/SE、t、DF、p、区间；双侧可有均值区间图。",
    "同时看点估计、区间和 p，不要只看是否显著。",
    "不假设已证明正态；小样本要谨慎。",
    ["t"],
))

ENTRIES.append(entry(
    "one_sample_z", "单样本 Z 检验", "基础统计", "统计 > 基础统计",
    "one_sample_z", "t_test", "quality_statistics_test",
    "在已知总体标准差 σ 时，检验一列均值是否等于给定 μ₀。",
    "独立于单样本 t。Z=(x̄−μ₀)/(σ/√n)，SE=σ/√n；样本 StDev 只展示不参与 Z/CI。禁止用样本 s 冒充 σ。",
    "一个数值列、假设均值、已知 σ>0、置信水平、备择方向。",
    ["取有限值，计算 x̄ 与 SE=σ/√n。", "Z=(x̄−μ₀)/SE，按备择用 Φ 得 p。", "双侧/单侧给出均值置信区间。"],
    [("μ₀", "假设均值"), ("σ", "已知总体标准差"), ("SE", "σ/√n"), ("Z", "检验统计量")],
    [fb("单样本 Z", "Z=(x̄−μ₀)/(σ/√n)；双侧 p=2(1−Φ(|Z|))；双侧 CI：x̄±z_{α/2}·SE。",
        [t("Z = "), frac("x̄ − μ₀", "σ / √n")],
        conditions="formula_reference；σ 必须来自配置。")],
    ["不得写已证明均值等于假设值。", "不得用样本 SD 代替 Known σ。"],
    "未选列、未给 μ₀、σ≤0 或无有效观测时诊断。",
    "描述表（含 Known σ 与样本 StDev）、Z/P、置信区间；双侧可有区间图。",
    "先确认 σ 来源可信，再看 Z 与区间。",
    "与 one_sample_t 命令独立；不改 t 核行为。",
    ["t"],
))

ENTRIES.append(entry(
    "two_sample_t", "双样本 t 检验", "基础统计", "统计 > 基础统计",
    "two_sample_t", "t_test", "quality_statistics_test",
    "比较两组均值差是否为 0。",
    "默认 Welch（不假设等方差）；可选 pooled 合并方差。两种 SE 都可展示，计算不混用。",
    "两个数值样本（两列或一列+分组）。方法 welch 或 pooled。",
    ["各组 complete-case，计算均值与 s。", "按 Welch 或 pooled 算 SE 与 DF。", "t=d/SE，给出 p 与差值区间。"],
    [("d", "x̄₁−x̄₂"), ("s_p²", "合并方差"), ("DF_Welch", "Satterthwaite 自由度")],
    [fb("Welch", "SE=√(s₁²/n₁+s₂²/n₂)；DF=(s₁²/n₁+s₂²/n₂)² / [(s₁²/n₁)²/(n₁−1)+(s₂²/n₂)²/(n₂−1)]。"),
     fb("Pooled", "s_p²=[(n₁−1)s₁²+(n₂−1)s₂²]/(n₁+n₂−2)；SE=s_p√(1/n₁+1/n₂)；DF=n₁+n₂−2。")],
    ["H0: μ₁−μ₂=0。", "Welch 与 pooled 的 t/p 不同，以所选方法为准。"],
    "任一组有效观测不足或方差无法估计时诊断。",
    "两组描述 + 检验表 + 差值区间。",
    "先确认用的是 Welch 还是 pooled，再读区间是否含 0。",
    "不自动做等方差检验来改方法。",
    ["t"],
))

ENTRIES.append(entry(
    "paired_t", "配对 t 检验", "基础统计", "统计 > 基础统计",
    "paired_t", "t_test", "quality_statistics_test",
    "对成对观测的差值做单样本 t，检验平均差值是否为 0。",
    "只使用两列都有效的行。d_i=After−Before（实现为配对差），再套单样本 t。",
    "两个等长数值列。",
    ["对齐 complete-case 配对。", "d_i=x_{1i}−x_{2i}。", "对 d 做单样本 t。"],
    [("d_i", "一对观测的差"), ("d̄", "平均差"), ("s_d", "差值标准差")],
    [fb("配对 t", "d̄=Σd_i/n，s_d=√[Σ(d_i−d̄)²/(n−1)]，SE=s_d/√n，t=d̄/SE，DF=n−1。",
        [t("t = "), frac("d̄", "s_d / √n")])],
    ["H0: 平均差为 0。", "缺失的一侧会使整对进入 N* 而不进图。"],
    "有效配对数 <2 时不计算。",
    "差值描述、t/p/区间、配对散点与差值区间图。",
    "看平均差方向和区间，而不是只看 p。",
    "配对必须是同一对象前后或匹配对，不能把无关两列当配对。",
    ["t"],
))

ENTRIES.append(entry(
    "one_proportion", "单比例检验", "基础统计", "统计 > 基础统计",
    "one_proportion", "proportion", "proportion_test",
    "检验事件比例是否等于假设 p₀。",
    "多行事件/试验求和。exact=Clopper–Pearson；normal=Wald 区间 + score z；wilson=Wilson score 区间 + 同一 score z；agresti_coull=Agresti–Coull 区间 + 同一 score z。",
    "事件数列、试验数列、假设比例（0 与 1 之间）、方法 exact/normal/wilson/agresti_coull。",
    ["按行读取非负整数，求和 x 与 n。", "p̂=x/n。", "按 exact、normal、wilson 或 agresti_coull 给出区间和检验。"],
    [("x", "事件合计"), ("n", "试验合计"), ("p̂", "x/n"), ("p₀", "假设比例"), ("z", "标准正态临界值")],
    [fb("点估计", "p̂ = x/n。", [t("p̂ = "), frac("x", "n")]),
     fb("精确法", "Clopper–Pearson 用二项尾概率求区间；不是正态近似。",
        conditions="method=exact。"),
     fb("正态近似", "Wald：p̂ ± z √[p̂(1−p̂)/n]；score z 用于检验。",
        conditions="method=normal。n 较小时近似变差。"),
     fb("Wilson score", "center=(p̂+z²/(2n))/(1+z²/n)，half=z√[p̂(1−p̂)/n+z²/(4n²)]/(1+z²/n)。"
        "x=0 时下限为 0；x=n 时上限为 1。检验仍用 score z under p₀。",
        [t("CI = center ± half"), t("denom = 1 + z²/n")],
        conditions="method=wilson。不做 Wilson 连续性校正。"),
     fb("Agresti–Coull", "ñ=n+z²，p̃=(x+z²/2)/ñ，区间 p̃±z√[p̃(1−p̃)/ñ]。"
        "x=0 时下限为 0；x=n 时上限为 1。检验仍用 score z under p₀。",
        [t("p̃ = (x + z²/2) / (n + z²)")],
        conditions="method=agresti_coull。不做 Blaker。")],
    ["假设比例必须在 (0,1)。", "解释是比例证据，不是规格合格。",
     "Facts.ci_method 区分 clopper_pearson / wald / wilson_score / agresti_coull。"],
    "事件>试验、负数、非整数、p₀ 越界或 n=0 时诊断。",
    "比例、区间、检验表（含 CI 方法列）。",
    "小样本优先看 exact；中等 n 可看 Wilson 或 Agresti–Coull。区间含 p₀ 不得写成显著差异。",
    "行是可加的计数，不是已经是比例的小数（除非你确实这样编码）。",
    ["prop"],
))

ENTRIES.append(entry(
    "two_proportions", "两比例检验", "基础统计", "统计 > 基础统计",
    "two_proportions", "proportion", "proportion_test",
    "比较两组事件比例之差。",
    "每组可多行求和。差值 Δ=p̂1−p̂2；检验 Z 始终用分位 unpooled Wald。method=normal（默认）时 CI 为 Wald；method=wilson 时 CI 为 Newcombe–Wilson；method=agresti_coull 时 CI 为 Agresti–Coull 差值区间。适用时另给 Fisher 精确检验。",
    "两组各自的事件列与试验列；可选 method=normal|wilson|agresti_coull。",
    ["各组求和得到 x1,n1,x2,n2。", "p̂i=xi/ni。", "Δ 与 SE_sep 得 Z/P；按 method 输出 Wald、Newcombe–Wilson 或 Agresti–Coull 差值区间；必要时 Fisher。"],
    [("Δ", "p̂1−p̂2"), ("SE_sep", "√[p̂1(1−p̂1)/n1+p̂2(1−p̂2)/n2]"),
     ("ci_method", "wald / newcombe_wilson / agresti_coull_diff")],
    [fb("两比例差检验", "Δ=p̂1−p̂2，SE_sep=√[p̂1(1−p̂1)/n1+p̂2(1−p̂2)/n2]。",
        [t("SE_sep = "), sqrt("p̂1(1−p̂1)/n1 + p̂2(1−p̂2)/n2")]),
     fb("Newcombe–Wilson 差值 CI", "先算两组 Wilson 区间 [l_i,u_i]，再 "
        "CI_L=(p̂1−p̂2)−√[(p̂1−l1)²+(u2−p̂2)²]，"
        "CI_U=(p̂1−p̂2)+√[(u1−p̂1)²+(p̂2−l2)²]。",
        conditions="method=wilson；检验 Z 仍用 Wald。"),
     fb("Agresti–Coull 差值 CI", "ñ_i=n_i+z²，x̃_i=x_i+z²/2，p̃_i=x̃_i/ñ_i；"
        "CI=(p̃1−p̃2)±z√[p̃1(1−p̃1)/ñ1+p̃2(1−p̃2)/ñ2]。",
        conditions="method=agresti_coull；检验 Z 仍用 Wald。")],
    ["一组缺失不拿另一组去填。", "解释是差值证据不是规格判定。", "不做 Blaker。"],
    "任一组 n=0、计数非法或事件>试验时诊断。",
    "两组比例、差值区间图、检验表（含方法与 CI）。",
    "同时看 Δ 的区间是否含 0；切换 method 时核对 Z 不变、CI 可变。",
    "Fisher 只在计数适用时出现。",
    ["prop"],
))

ENTRIES.append(entry(
    "one_poisson_rate", "单样本泊松率", "基础统计", "统计 > 基础统计",
    "one_poisson_rate", "poisson_rate", "poisson_rate_test",
    "检验缺陷发生速率 λ 是否等于假设值。",
    "多行缺陷与长度（暴露量）求和。λ̂=x/t。exact=Garwood 区间+泊松尾；normal=score/Wald。",
    "缺陷列、长度列、假设速率、方法 exact/normal。",
    ["求和缺陷 x 与暴露 t。", "λ̂=x/t。", "按 exact 或 normal 给区间与 p。"],
    [("x", "缺陷合计"), ("t", "长度/暴露合计"), ("λ̂", "x/t")],
    [fb("率估计", "λ̂ = x/t。", [t("λ̂ = "), frac("x", "t")]),
     fb("Garwood 精确区间", "由泊松累积尾概率反解得到率的精确区间。",
        conditions="method=exact。公式参考，不作为 Minitab 导出 golden。")],
    ["长度必须为正。", "解释不是过程合格。"],
    "负计数、非有限长度或 t≤0 时诊断。",
    "率、区间、检验表。",
    "小计数优先 exact。",
    "假设单位时间内缺陷服从泊松。",
    ["poisson"], status="implemented",
))

ENTRIES.append(entry(
    "two_poisson_rate", "双样本泊松率", "基础统计", "统计 > 基础统计",
    "two_poisson_rate", "poisson_rate", "poisson_rate_test",
    "比较两组泊松率（差值或率比）。",
    "2-sample 每组一行（四列）。默认 comparison=difference。comparison=ratio 时主输出 ρ=λ1/λ2，CI 用 log-Wald；exact 的 P 仍用条件二项（H0: ρ=1）。",
    "两组缺陷与长度各一列；可选 comparison=difference|ratio。",
    ["读取两组 x,t。", "λ̂i=xi/ti。", "差值或率比的区间与 p。"],
    [("λ̂1, λ̂2", "两组率"), ("ρ", "率比 λ1/λ2"), ("条件二项", "exact 法在合计缺陷固定下的条件分布")],
    [fb("两组率", "λ̂i=x_i/t_i。exact：在 x1+x2 固定时 x1~Binomial。"),
     fb("率比 log-Wald", "SE=√(1/x1+1/x2)；CI=exp(log ρ̂ ± z·SE)。x1 或 x2 为 0 时区间不可用。")],
    ["多行输入会诊断，因为此命令按每组一行读取。", "解释不是规格判定。"],
    "长度非正或计数非法时诊断。",
    "两组率、差值或率比检验表；ratio 模式可有率比区间图。",
    "核对每组只有一行有效计数。",
    "默认差值表形保持兼容；率比是可选比较量。",
    ["poisson"],
))

ENTRIES.append(entry(
    "one_sample_equivalence", "单样本等价性检验", "基础统计", "统计 > 基础统计",
    "one_sample_equivalence", "equivalence", "equivalence_test",
    "检验均值是否落入规定等价界限内（TOST），而不是“是否等于目标”。",
    "做两个单侧 t：均值不低于下限且不高于上限。输出同时保留两个单侧 p、`ci_method=tost_1_minus_alpha` 与 `both_pvalues_below_alpha`。100(1−2α)% 区间落入界限内等价于两个单侧 p 都 ≤α。",
    "测量列、目标或差的上下界限、α。",
    ["计算均值、s、SE。", "构造 `ci_method=tost_1_minus_alpha` 的 100(1−2α)% 区间。", "分别对下界、上界做单侧 t 得到 p1、p2，并写出 `both_pvalues_below_alpha`。"],
    [("L,U", "等价下/上界"), ("p1,p2", "两个单侧检验的 p"), ("α", "单侧水平"), ("ci_method", "区间口径标记"), ("both_pvalues_below_alpha", "两个单侧 p 是否都 ≤ α")],
    [fb("TOST", "within_limits 当且仅当 p1≤α 且 p2≤α，等价于 100(1−2α)% CI 完全落在 [L,U] 内。`both_pvalues_below_alpha` 与这个判定保持一致。")],
    ["区间越出界限≠过程不合格，只说明未落入声明的等价范围。", "不得写成已证明等价。"],
    "界限非法、s=0 或 n 不足时诊断。",
    "区间、两个单侧 p、`ci_method`、`both_pvalues_below_alpha` 与是否落入界限。",
    "先看界限是否来自工程定义，再看区间位置。",
    "默认差的等价，不是比率等价。",
    ["t"],
))

ENTRIES.append(entry(
    "two_sample_equivalence", "双样本等价性检验", "基础统计", "统计 > 基础统计",
    "two_sample_equivalence", "equivalence", "equivalence_test",
    "检验两组均值差是否落入等价界限。",
    "对差值做 TOST，SE 按双样本方法。输出两个单侧 p、`ci_method=tost_1_minus_alpha` 与 `both_pvalues_below_alpha`。判定规则与单样本 TOST 相同。",
    "两组样本与差的下/上界限。",
    ["计算两组均值差与 SE。", "输出 `ci_method=tost_1_minus_alpha` 的 100(1−2α)% 差值区间。", "两个单侧 p 与 `both_pvalues_below_alpha`。"],
    [("Δ", "均值差"), ("L,U", "差的等价界限"), ("both_pvalues_below_alpha", "两个单侧 p 是否都 ≤ α")],
    [fb("双样本 TOST", "对 Δ 使用 100(1−2α)% CI；p1≤α 且 p2≤α 才算落入界限，`both_pvalues_below_alpha` 只负责如实回报这一状态。")],
    ["未落入界限只说明证据不足，不是不合格判定。"],
    "任一组无法估计均值/方差或界限交叉时诊断。",
    "差值区间、TOST 表、`ci_method` 与 `both_pvalues_below_alpha`。",
    "确认界限单位与测量单位一致。",
    "配对等价见独立命令 paired_equivalence；均值比见 two_sample_equivalence_ratio；本命令不做对数变换。",
    ["t"],
))

ENTRIES.append(entry(
    "two_sample_equivalence_ratio", "双样本均值比等价性检验", "基础统计", "统计 > 基础统计",
    "two_sample_equivalence_ratio", "equivalence", "equivalence_test",
    "检验两组均值比值 ρ=μ_test/μ_ref 是否落入比值等价界限（TOST）。",
    "默认 transform=none：非对数 Fieller TOST，`ci_method=tost_ratio_1_minus_alpha`。transform=log：两侧取 ln 后做差值 TOST，界限 log(δ)，点估计与 CI 回变换为几何均值比，`ci_method=tost_ratio_log_1_minus_alpha`。`EquivalenceFacts.difference` 存比值尺度估计。",
    "检验列、参考列、比值下/上界限（须 >0）、Welch 或 pooled、transform=none|log、置信水平。",
    ["计算两组均值与 SD（原始尺度）。", "none：ρ̂=ȳ_test/ȳ_ref 与 Fieller CI。", "log：全正值取 ln 后差值 TOST，再 exp 回比值。"],
    [("ρ̂ / ρ̂_g", "算术或几何均值比"), ("δ1,δ2", "比值等价界限"),
     ("ci_method", "tost_ratio_1_minus_alpha 或 tost_ratio_log_1_minus_alpha")],
    [fb("均值比 TOST（非对数）", "t1=(ȳ1−δ1ȳ2)/SE(δ1)，t2=(ȳ1−δ2ȳ2)/SE(δ2)。within_limits 当且仅当两单侧 p≤α。",
        conditions="transform=none。"),
     fb("均值比 TOST（对数）", "y=ln x；对 θ=ȳ1−ȳ2 做差值 TOST，界限 ln δ；ρ̂_g=exp(θ̂)，CI=exp(CI_θ)。",
        conditions="transform=log；全部观测 >0。")],
    ["不得写成已证明等价。", "非对数路径参考均值须为正；对数路径全部观测须为正。"],
    "参考均值≤0（none）、非正观测（log）、界限非法、Fieller 条件失败或方差不可用时诊断。",
    "描述统计、TOST 表、比值区间图。",
    "先确认界限是比值尺度（如 0.8～1.25），再看区间是否完整落入；切换 transform 时核对 ci_method。",
    "差值等价仍用 two_sample_equivalence。",
    ["t"],
))

ENTRIES.append(entry(
    "paired_equivalence", "配对等价性检验", "基础统计", "统计 > 基础统计",
    "paired_equivalence", "equivalence", "equivalence_test",
    "检验配对差值是否落入规定等价界限内（Paired TOST）。",
    "先对配对差值 d_i=X_i−Y_i 计算均值与 SE，再做两个单侧 t。输出 `ci_method=tost_1_minus_alpha`、两个单侧 p 与 `both_pvalues_below_alpha`。",
    "两列配对测量值与差值的等价下/上界限。",
    ["按 complete-case 对齐两列，只保留成对有效观测。", "计算配对差值均值 d̄、差值标准差 s_d 与 SE=s_d/√n。", "构造 `ci_method=tost_1_minus_alpha` 的 100(1−2α)% 区间。", "分别对下界、上界做单侧 t。"],
    [("d_i", "第 i 对差值 X_i−Y_i"), ("d̄", "配对差值均值"), ("SE", "差值均值标准误"), ("both_pvalues_below_alpha", "两个单侧 p 是否都 ≤ α")],
    [fb("Paired TOST", "t_L=(d̄−LEL)/SE，t_U=(d̄−UEL)/SE。p_L≤α 且 p_U≤α 时，配对差值区间落入等价界限内。")],
    ["未落入界限只说明证据不足，不是过程或仪器不合格判定。", "不得写成已证明等价。"],
    "成对有效观测不足、差值标准误为 0 或界限交叉时诊断。",
    "配对差值描述统计、TOST 表、等价区间图。",
    "先看 complete-case 后剩余的配对数，再看区间是否完整落入界限。",
    "当前只做差值等价，不做比值或对数变换等价。",
    ["t"],
))

ENTRIES.append(entry(
    "one_proportion_equivalence", "单比例等价性检验", "基础统计", "统计 > 基础统计",
    "one_proportion_equivalence", "equivalence", "equivalence_test",
    "检验样本比例相对目标比例的差是否落入等价界限（Wald z-TOST）。",
    "对 d=p̂−p0 做两个单侧 z 检验。SE=√[p̂(1−p̂)/n]。输出 `ci_method=wald_z_tost` 的 100(1−2α)% Wald 区间与两个单侧 P。不做 Wilson/Blaker。",
    "事件列、试验列、目标比例、等价下/上界限。",
    ["complete-case 多行求和得到 x、n。", "计算 p̂、d、SE。", "z_L=(d−LEL)/SE，z_U=(d−UEL)/SE。", "CI = d ± z_(1−α)·SE。"],
    [("p̂", "样本比例 x/n"), ("p0", "目标比例"), ("LEL,UEL", "比例差等价界限"), ("wald_z_tost", "区间口径")],
    [fb("比例 z-TOST", "p_L=1−Φ(z_L)，p_U=Φ(z_U)。within_limits 当且仅当两个单侧 P 都 ≤α。",
        [t("SE = "), sqrt([t("p̂(1−p̂)/n")])]),
     fb("判定", "不得把落入界限写成已证明等价或规格合格。")],
    ["大样本近似；p̂=0 或 1 时 SE=0 只诊断。"],
    "事件数>试验数、试验为 0、界限交叉时诊断。",
    "事件/试验/比例表、z-TOST 表、等价区间图。",
    "先确认界限是比例差尺度，再看区间。",
    "与均值 TOST 同构，统计量用标准正态而非 t。",
    ["prop", "t"],
))

ENTRIES.append(entry(
    "two_proportion_equivalence", "两比例等价性检验", "基础统计", "统计 > 基础统计",
    "two_proportion_equivalence", "equivalence", "equivalence_test",
    "检验两组比例差是否落入等价界限（Wald z-TOST）。",
    "对 d=p̂1−p̂2 使用未合并双方差 SE。输出 `ci_method=wald_z_tost`。每组独立 complete-case 求和。",
    "两组事件列与试验列，以及差的等价界限。",
    ["各组分别求和。", "计算 p̂1、p̂2、d 与 SE。", "双单侧 z 与 100(1−2α)% Wald CI。"],
    [("p̂1,p̂2", "两组样本比例"), ("d", "比例差"), ("SE", "√[p̂1(1−p̂1)/n1+p̂2(1−p̂2)/n2]")],
    [fb("两比例 z-TOST", "判定规则与单比例相同：两个单侧 P 都 ≤α 才算落入界限。")],
    ["一组缺失不进另一组求和。", "不得写成已证明两组等价。"],
    "任一组试验为 0 或事件>试验时诊断。",
    "两组描述、z-TOST 表、等价区间图。",
    "核对每组有效行后再解释区间。",
    "不做比值/对数等价。",
    ["prop", "t"],
))

from help_catalog_families import add_remaining

add_remaining(entry, fb, t, frac, sqrt, ENTRIES, bar, dbar, heading, line, cases, stack, sub, sup)


def main() -> None:
    OUT.parent.mkdir(parents=True, exist_ok=True)
    catalog = {
        "catalog_version": "2026-08-20.2",
        "last_reviewed": ACCESSED,
        "entries": ENTRIES,
    }
    OUT.write_text(json.dumps(catalog, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Wrote {len(ENTRIES)} entries to {OUT}")
    forbidden = 0
    for item in ENTRIES:
        blob = item["purpose"] + item["method_overview"] + "".join(
            b.get("plain_text", "") + b.get("explanation", "") for b in item["formula_blocks"]
        )
        if "docs/" in blob or ".md" in blob or "见仓库" in blob:
            forbidden += 1
            print("markdown leak", item["id"])
    if forbidden:
        raise SystemExit(f"{forbidden} entries still point users to markdown")


if __name__ == "__main__":
    import sys

    sys.path.insert(0, str(Path(__file__).resolve().parent))
    main()
