# -*- coding: utf-8 -*-
"""Merge analysis dialog role/input/chrome labels into ui_menu_strings.json."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

# Avoid Windows console GBK crashes when printing CJK.
sys.stdout.reconfigure(encoding="utf-8", errors="replace")

root = Path(__file__).resolve().parents[1]
cpp = (root / "src/ui/analysis_commands.cpp").read_text(encoding="utf-8")
json_path = root / "translations/ui_menu_strings.json"

# --- Extract role labels (4-field RoleSpec aggregate) ---
role_pat = re.compile(
    r"\{\s*QStringLiteral\(\s*\"([^\"]+)\"\s*\)\s*,\s*"
    r"QStringLiteral\(\s*\"([^\"]+)\"\s*\)\s*,\s*"
    r"(true|false)\s*,\s*(true|false)",
    re.M,
)
role_labels = sorted({label for _id, label, _a, _b in role_pat.findall(cpp)})

# --- Extract input labels from CommandSpec inputs vectors ---
# Match: icon QSL, bool, bool, {roles...}, {inputs...}
header_pat = re.compile(
    r"QStringLiteral\(\s*\"[^\"]+\"\s*\)\s*,\s*"  # id
    r"QStringLiteral\(\s*\"[^\"]+\"\s*\)\s*,\s*"  # menu
    r"QStringLiteral\(\s*\"[^\"]+\"\s*\)\s*,\s*"  # title
    r"QStringLiteral\(\s*\"[^\"]+\"\s*\)\s*,\s*"  # path
    r"QStringLiteral\(\s*\"[^\"]+\"\s*\)\s*,\s*"  # icon
    r"(?:true|false)\s*,\s*(?:true|false)\s*,",
    re.M,
)

triple_pat = re.compile(
    r"\{\s*QStringLiteral\(\s*\"([^\"]+)\"\s*\)\s*,\s*"
    r"QStringLiteral\(\s*\"([^\"]+)\"\s*\)\s*,\s*"
    r"QStringLiteral\(\s*\"([^\"]*)\"\s*\)",
    re.M,
)


def extract_balanced_brace(text: str, start: int) -> tuple[str, int] | None:
    """start points at '{'; return content including braces and end index."""
    if start >= len(text) or text[start] != "{":
        return None
    depth = 0
    i = start
    while i < len(text):
        ch = text[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return text[start : i + 1], i + 1
        i += 1
    return None


input_labels: set[str] = set()
choice_labels: set[str] = set()
for m in header_pat.finditer(cpp):
    pos = m.end()
    while pos < len(cpp) and cpp[pos].isspace():
        pos += 1
    roles_blob = extract_balanced_brace(cpp, pos)
    if roles_blob is None:
        continue
    _roles, pos = roles_blob
    while pos < len(cpp) and (cpp[pos].isspace() or cpp[pos] == ","):
        pos += 1
    inputs_blob = extract_balanced_brace(cpp, pos)
    if inputs_blob is None:
        continue
    inputs_text, _ = inputs_blob
    for _id, label, _ph in triple_pat.findall(inputs_text):
        input_labels.add(label)

# Named InputSpec helpers outside CommandSpec
for m in re.finditer(
    r"InputSpec(?:\s+\w+)?\s*\{?\s*QStringLiteral\(\s*\"[^\"]+\"\s*\)\s*,\s*"
    r"QStringLiteral\(\s*\"([^\"]+)\"\s*\)",
    cpp,
):
    input_labels.add(m.group(1))

# Choice display labels inside .choices = { ... }
for block in re.finditer(r"\.choices\s*=\s*\{(.*?)\};", cpp, re.S):
    for dm in re.finditer(
        r"QStringLiteral\(\s*\"[^\"]+\"\s*\)\s*,\s*QStringLiteral\(\s*\"([^\"]+)\"\s*\)",
        block.group(1),
    ):
        choice_labels.add(dm.group(1))

# --- Chrome strings used by analysis_setup_dialog (source zh_CN) ---
CHROME: dict[str, str] = {
    "（可选）": " (optional)",
    "重置默认值": "Reset defaults",
    "全选": "Select all",
    "清空": "Clear",
    "恢复默认": "Restore defaults",
    "数值": "Numeric",
    "分类": "Categorical",
    "时间": "Time",
    "未知": "Unknown",
    "，默认值：": ", default: ",
    "取消": "Cancel",
    "运行分析": "Run analysis",
    "高级选项": "Advanced options",
    "分析设置": "Analysis settings",
    "工作表列": "Worksheet columns",
    "搜索列名…": "Search columns…",
    "选择 >": "Select >",
    "移除选中列": "Remove selected",
    "先点击右侧角色框，再点“选择 >”或双击左侧列。": (
        "Click a role box on the right, then Select > or double-click a column."
    ),
    "列类型为“%1”，与角色建议的类型（%2）不完全匹配；"
    "命令仍会在提交时进行最终校验。": (
        "Column type is “%1”, which does not fully match the role suggestion (%2); "
        "the command still performs final validation on submit."
    ),
    "此角色至少需要选择 %1 列。": "This role requires at least %1 column(s).",
    "此角色最多只能选择 %1 列。": "This role allows at most %1 column(s).",
    "最大化": "Maximize",
    "最小化": "Minimize",
    "目标值": "Target",
    "响应": "Response",
    "目标": "Target",
    "下限": "Lower",
    "上限": "Upper",
    "权重": "Weight",
    "选择多个响应后，可为每个响应指定独立目标与权重。": (
        "After selecting multiple responses, set a goal and weight for each."
    ),
    "此规则不适用于当前控制图，已置灰。": (
        "This rule does not apply to the current control chart (disabled)."
    ),
    "CUSUM 不使用 Shewhart 特殊原因规则（单点超出 3σ 控制限等），"
    "改为报告上侧/下侧累计和的首次信号。": (
        "CUSUM does not use Shewhart special-cause rules "
        "(e.g. one point beyond 3σ); instead report first upper/lower CUSUM signal."
    ),
    "CUSUM 使用专用信号，不勾选 Shewhart 特殊原因规则。": (
        "CUSUM uses dedicated signals; Shewhart special-cause rules are not selected."
    ),
    "勾选=全部适用默认 → 由「规则默认策略」决定"
    "（all_applicable 或 minitab_like）。已选 %1 / %2。": (
        "Checked = all applicable defaults → decided by Default rule policy "
        "(all_applicable or minitab_like). Selected %1 / %2."
    ),
    "已显式勾选 %1 / %2 条（覆盖策略下拉）。"
    "多规则提高误报风险。": (
        "Explicitly checked %1 / %2 (overrides policy dropdown). "
        "More rules raise false-alarm risk."
    ),
}

# Auto machine-assisted EN for role/input labels (curated overrides win).
# Keep English-looking sources as themselves.
CURATED: dict[str, str] = {
    "响应": "Response",
    "响应变量": "Response variable",
    "响应列（可多选）": "Response columns (multi)",
    "响应列（可选，选择后进行响应分析）": "Response column (optional; for response analysis)",
    "因子 A": "Factor A",
    "因子 B": "Factor B",
    "因子/分组列": "Factor / grouping column",
    "因子（≥2）": "Factors (≥2)",
    "因子（2～4 列）": "Factors (2–4 columns)",
    "因子（1～2 列）": "Factors (1–2 columns)",
    "因子名（逗号分隔）": "Factor names (comma-separated)",
    "已导入因子列（多选）": "Imported factor columns (multi)",
    "已导入因子列（可选，多选）": "Imported factor columns (optional, multi)",
    "分组": "Group",
    "分组列": "Grouping column",
    "分组变量": "Grouping variable",
    "分组列（可选）": "Grouping column (optional)",
    "分组列（Log-rank，可选）": "Grouping column (Log-rank, optional)",
    "分组列（k 组等方差）": "Grouping column (k-group equal variances)",
    "分组变量（图内着色）": "Grouping variable (in-plot color)",
    "分面变量（多面板）": "Facet variable (multi-panel)",
    "By 变量": "By variable",
    "X 变量": "X variable",
    "Y 变量": "Y variable",
    "Z 变量": "Z variable",
    "变量": "Variable",
    "变量（多列）": "Variables (multi)",
    "变量（至少两列）": "Variables (at least two)",
    "变量（第一列响应，其余为预测变量）": "Variables (first = response; rest = predictors)",
    "数值": "Numeric",
    "数值变量": "Numeric variable",
    "数值变量（可多选）": "Numeric variables (multi)",
    "分类变量": "Categorical variable",
    "分类列": "Categorical column",
    "时间变量": "Time variable",
    "时间列（可选）": "Time column (optional)",
    "时间序列": "Time series",
    "时间序列值": "Time series values",
    "寿命/时间": "Lifetime / time",
    "删失类型列（exact/right/left/interval，可选）": (
        "Censoring type column (exact/right/left/interval, optional)"
    ),
    "失效指示（1=失效，0=删失；可与删失类型列二选一）": (
        "Failure indicator (1=fail, 0=censor; or use censoring-type column)"
    ),
    "失效模式列（可选）": "Failure mode column (optional)",
    "区间左端 L": "Interval left L",
    "区间右端 R（空/Inf=右删失）": "Interval right R (empty/Inf = right-censored)",
    "区间左界列（interval 行必填）": "Interval left column (required for interval rows)",
    "区间右界列（interval 行必填）": "Interval right column (required for interval rows)",
    "暴露量列（可选，列求和）": "Exposure column (optional; column sum)",
    "暴露量列（可选，列求和优先于标量）": (
        "Exposure column (optional; column sum preferred over scalar)"
    ),
    "原因列": "Cause column",
    "特殊原因测试": "Special-cause tests",
    "规则默认策略": "Default rule policy",
    "置信水平": "Confidence level",
    "显著性水平 α": "Significance level α",
    "显著性水平": "Significance level",
    "样本量 n": "Sample size n",
    "中心点数": "Center points",
    "区组数": "Number of blocks",
    "区组": "Block",
    "随机种子": "Random seed",
    "低水平（逗号分隔）": "Low levels (comma-separated)",
    "高水平（逗号分隔）": "High levels (comma-separated)",
    "生成元（可选）": "Generators (optional)",
    "部分析因 p（0=全因子）": "Fraction p (0 = full factorial)",
    "DOE 部分析因 p（0=全因子）": "DOE fraction p (0 = full factorial)",
    "等值线 X 因子名": "Contour X factor name",
    "等值线 Y 因子名": "Contour Y factor name",
    "固定水平（name=value;…）": "Hold levels (name=value;…)",
    "优化目标": "Optimization goal",
    "变换方法": "Transformation method",
    "分布": "Distribution",
    "测量值": "Measurement",
    "操作员": "Operator",
    "操作者": "Operator",
    "零件": "Part",
    "部件": "Part",
    "子组": "Subgroup",
    "子组列": "Subgroup column",
    "缺陷数": "Defect count",
    "不合格品数": "Nonconforming count",
    "试验数": "Trials",
    "事件数": "Event count",
    "计数列": "Count column",
    "计数列（非负整数）": "Count column (non-negative integer)",
    "类别": "Category",
    "类别列": "Category column",
    "气泡大小变量": "Bubble size variable",
    "预测变量（数值）": "Predictor (numeric)",
    "候选预测变量": "Candidate predictors",
    "数值预测变量": "Numeric predictors",
    "相关变量": "Correlation variables",
    "标准（可选）": "Standard (optional)",
    "参考值列（Linearity）": "Reference column (Linearity)",
    "评估者": "Appraiser",
    "评级": "Rating",
    "有序响应": "Ordinal response",
    "二元响应": "Binary response",
    "类别响应": "Categorical response",
    "计数响应": "Count response",
    "权重/计数": "Weight / count",
    "标签变量": "Label variable",
    "顺序/时间变量": "Order / time variable",
    "阶段列": "Stage column",
    "附加因子": "Additional factors",
    "处理": "Treatment",
    "序列": "Series",
    "序列 X": "Series X",
    "序列 Y": "Series Y",
    "正值变量": "Positive-valued variable",
    "数值序列（一列）": "Numeric series (one column)",
    "数值观测（一列）": "Numeric observations (one column)",
    "两列独立样本": "Two independent sample columns",
    "两列配对样本": "Two paired sample columns",
    "两列配对二元结果": "Two paired binary outcome columns",
    "两列分类（2×2）": "Two categorical columns (2×2)",
    "≥3 列配对二元": "≥3 paired binary columns",
    "一列或两列配对": "One or two paired columns",
    "配对变量（两列）": "Paired variables (two columns)",
    "第一样本 / 测量列": "First sample / measurement column",
    "第二样本（两方差）": "Second sample (two variances)",
    "第一组事件数": "Group 1 event count",
    "第二组事件数": "Group 2 event count",
    "第一组试验数": "Group 1 trials",
    "第二组试验数": "Group 2 trials",
    "第一组缺陷数": "Group 1 defect count",
    "第二组缺陷数": "Group 2 defect count",
    "第一组观测长度": "Group 1 observation length",
    "第二组观测长度": "Group 2 observation length",
    "检验列 + 参考列": "Test column + reference column",
    "检验数（列）": "Inspections (column)",
    "检验数列": "Inspections column",
    "单位数（列）": "Units (column)",
    "单位数列": "Units column",
    "观测长度（列）": "Observation length (column)",
    "间隔列": "Interval column",
    "行分类列": "Row categorical column",
    "列分类列": "Column categorical column",
    "行类别": "Row category",
    "列类别": "Column category",
    "缺陷类别": "Defect category",
    "all_applicable（全部适用特殊原因规则）": (
        "all_applicable (all applicable special-cause rules)"
    ),
    "minitab_like（仅「单点超出 3σ 控制限」）": (
        "minitab_like (only '1 point beyond 3σ limits')"
    ),
    # Remaining high-frequency input labels (still_zh pass)
    "ANOVA 组数 / DOE 因子数 k": "ANOVA groups / DOE factors k",
    "Gamma（双指数）": "Gamma (double exponential)",
    "Other 合并阈值（可选 %）": "Other merge threshold (optional %)",
    "α": "α",
    "α enter": "α enter",
    "α remove": "α remove",
    "σ 方法": "σ method",
    "上限（可空，默认观测最大）": "Upper (empty = observed max)",
    "下限（可空，默认观测最小）": "Lower (empty = observed min)",
    "两方差 / 等方差方法": "Two-variances / equal-variance method",
    "中心（逗号，可空=中点）": "Centers (comma; empty = midpoints)",
    "事件水平": "Event level",
    "任务": "Task",
    "低水平（逗号分隔，可选）": "Low levels (comma-separated, optional)",
    "低水平（逗号数值）": "Low levels (comma-separated numbers)",
    "低水平（逗号，可空）": "Low levels (comma; may be empty)",
    "保修窗口 T_w": "Warranty window T_w",
    "保留主成分数": "Principal components to keep",
    "假设中位数 η0": "Hypothesized median η0",
    "假设发生率": "Hypothesized rate",
    "假设均值": "Hypothesized mean",
    "假设方差（一方差）": "Hypothesized variance (1-variance)",
    "假设比例": "Hypothesized proportion",
    "偏相关": "Partial correlation",
    "允许超范围星点(CCC)": "Allow axial points beyond range (CCC)",
    "公差": "Tolerance",
    "决策间隔 h": "Decision interval h",
    "分数分位阈值": "Fractional quantile threshold",
    "分箱数（0=自动）": "Bins (0 = automatic)",
    "分面最大面板数": "Max facet panels",
    "切簇数 k": "Clusters k",
    "删失数": "Censoring count",
    "区间方向": "Interval direction",
    "单位（逗号，可空）": "Units (comma; may be empty)",
    "历史 Sigma": "Historical Sigma",
    "历史 Sigma Z": "Historical Sigma Z",
    "历史 Sigma（可选）": "Historical Sigma (optional)",
    "历史中心线": "Historical center line",
    "历史均值": "Historical mean",
    "历史均值（可选）": "Historical mean (optional)",
    "参考值 k": "Reference value k",
    "变体 ccf/cci/ccc": "Variant ccf/cci/ccc",
    "各响应独立目标": "Per-response objectives",
    "回归规格": "Regression specification",
    "因子编码": "Factor coding",
    "因素 ID（逗号）": "Factor IDs (comma)",
    "因素 ID（逗号，≥3）": "Factor IDs (comma, ≥3)",
    "因素名（逗号，可空）": "Factor names (comma; may be empty)",
    "备择方向": "Alternative direction",
    "多重比较": "Multiple comparisons",
    "子组大小": "Subgroup size",
    "子组大小 n": "Subgroup size n",
    "季节周期": "Seasonal period",
    "小类别合并阈值 %": "Small-category merge threshold %",
    "已知标准差 σ": "Known standard deviation σ",
    "常数 K（criterion=value）": "Constant K (criterion=value)",
    "接收数 c": "Acceptance number c",
    "控制限倍数": "Control-limit multiplier",
    "收敛阈值": "Convergence threshold",
    "效应标题": "Effect title",
    "效应量 / 覆盖率 P / 等价半宽(σ单位)": (
        "Effect size / coverage P / equivalence half-width (σ units)"
    ),
    "方差方法": "Variance method",
    "方法": "Method",
    "族误差率 α": "Family-wise error rate α",
    "时间单位": "Time unit",
    "是否预测口径 true/false": "Prediction metric true/false",
    "暴露量标量（无列时使用）": "Exposure scalar (used when no column)",
    "最大深度": "Max depth",
    "最大滞后（可空=自动）": "Max lag (empty = automatic)",
    "最大滞后（可空）": "Max lag (optional)",
    "最大迭代": "Max iterations",
    "最大迭代次数": "Max iterations",
    "最小叶样本": "Min leaf samples",
    "有序评级（true 时计算 Kendall W/τ）": (
        "Ordinal ratings (compute Kendall W/τ when true)"
    ),
    "有效观测数": "Effective observations",
    "期望比例（逗号，可选）": "Expected proportions (comma, optional)",
    "标准化": "Standardize",
    "树数": "Number of trees",
    "检验数常数": "Inspections constant",
    "模型": "Model",
    "模型标签": "Model label",
    "模式": "Mode",
    "每个子组单位数": "Units per subgroup",
    "每树样本上限": "Max samples per tree",
    "比例方差": "Proportion variance",
    "比值等价上限": "Ratio equivalence upper",
    "比值等价下限": "Ratio equivalence lower",
    "比较准则": "Comparison criterion",
    "比较量": "Comparison measure",
    "泊松观测长度 L": "Poisson observation length L",
    "滞后（可空=自动）": "Lag (empty = automatic)",
    "生成器（可空=默认；如 D=ABC）": "Generators (empty = default; e.g. D=ABC)",
    "生成器（设计生成用）": "Generators (for design generation)",
    "目标不合格品率": "Target nonconforming rate",
    "目标值（目标优化时使用）": "Target value (for target optimization)",
    "目标功效或容差置信度": "Target power or tolerance confidence",
    "目标比例": "Target proportion",
    "直方图组数（可选）": "Histogram bins (optional)",
    "相关方法": "Correlation method",
    "矩阵模式": "Matrix mode",
    "移动极差长度": "Moving-range length",
    "窗宽 w": "Window width w",
    "第一/假设比例或泊松率 / 容差 max k": (
        "First/hypothesized proportion or Poisson rate / tolerance max k"
    ),
    "第二/备择比例或比较率 λ1/λ2": (
        "Second/alternative proportion or rate ratio λ1/λ2"
    ),
    "等价上限": "Equivalence upper",
    "等价下限": "Equivalence lower",
    "等价真实差(σ单位)": "True equivalence difference (σ units)",
    "等值线 X 因子（可空=第1个）": "Contour X factor (empty = first)",
    "等值线 Y 因子（可空=第2个）": "Contour Y factor (empty = second)",
    "等值线层数": "Contour levels",
    "簇数 k": "Clusters k",
    "组数（可选）": "Groups (optional)",
    "置信水平（带宽）": "Confidence level (bandwidth)",
    "覆盖率 (%)": "Coverage (%)",
    "观察失效数": "Observed failures",
    "设计中心（逗号，可选）": "Design center (comma, optional)",
    "设计低水平（逗号，可选）": "Design low levels (comma, optional)",
    "设计族 ccd/bbd（可选）": "Design family ccd/bbd (optional)",
    "设计来源 ID（可选）": "Design source ID (optional)",
    "设计高水平（逗号，可选）": "Design high levels (comma, optional)",
    "误差模型": "Error model",
    "趋势模型": "Trend model",
    "过程 Sigma": "Process Sigma",
    "过程变差 6σ（Linearity 可选）": "Process variation 6σ (Linearity optional)",
    "连接缺失间隔": "Connect across missing gaps",
    "选模准则": "Model-selection criterion",
    "部分析因 p（设计生成用）": "Fraction p (for design generation)",
    "重抽样次数 B": "Resamples B",
    "随机化": "Randomize",
    "非轴因子实际 hold（名=值;…，可空=编码0）": (
        "Non-axis factor hold levels (name=value;…; empty = coded 0)"
    ),
    "预测期数": "Forecast periods",
    "高水平（逗号分隔，可选）": "High levels (comma-separated, optional)",
    "高水平（逗号数值）": "High levels (comma-separated numbers)",
    "高水平（逗号，可空）": "High levels (comma; may be empty)",
    "低水平（逗号分隔）": "Low levels (comma-separated)",
    "高水平（逗号分隔）": "High levels (comma-separated)",
    "随机种子": "Random seed",
    "中心点数": "Center points",
    "置信水平": "Confidence level",
}

# Simple phrase replacements for remaining Chinese labels (honest-enough EN).
REPLACEMENTS: list[tuple[str, str]] = [
    ("（可选）", " (optional)"),
    ("（可多选）", " (multi)"),
    ("（多选）", " (multi)"),
    ("可选，", "optional, "),
    ("响应变量", "Response variable"),
    ("响应列", "Response column"),
    ("响应", "Response"),
    ("因子", "Factor"),
    ("分组列", "Grouping column"),
    ("分组变量", "Grouping variable"),
    ("分组", "Group"),
    ("变量", "Variable"),
    ("数值", "Numeric"),
    ("分类", "Categorical"),
    ("时间序列", "Time series"),
    ("时间", "Time"),
    ("删失", "Censoring"),
    ("失效", "Failure"),
    ("区间", "Interval"),
    ("暴露量", "Exposure"),
    ("样本量", "Sample size"),
    ("置信水平", "Confidence level"),
    ("显著性水平", "Significance level"),
    ("中心点", "Center points"),
    ("区组", "Blocks"),
    ("随机种子", "Random seed"),
    ("低水平", "Low levels"),
    ("高水平", "High levels"),
    ("生成元", "Generators"),
    ("变换", "Transformation"),
    ("分布", "Distribution"),
    ("测量值", "Measurement"),
    ("操作员", "Operator"),
    ("操作者", "Operator"),
    ("零件", "Part"),
    ("部件", "Part"),
    ("子组", "Subgroup"),
    ("缺陷", "Defect"),
    ("不合格品", "Nonconforming"),
    ("试验数", "Trials"),
    ("事件数", "Events"),
    ("计数", "Count"),
    ("类别", "Category"),
    ("预测变量", "Predictors"),
    ("权重", "Weight"),
    ("目标值", "Target"),
    ("目标", "Target"),
    ("下限", "Lower"),
    ("上限", "Upper"),
    ("逗号分隔", "comma-separated"),
    ("列", " column"),
    ("至少", "at least "),
    ("两列", "two columns"),
    ("一列", "one column"),
]


def looks_english(s: str) -> bool:
    # Mostly ASCII letters/digits/punct → keep as-is.
    non_ascii = sum(1 for ch in s if ord(ch) > 127)
    return non_ascii == 0


def auto_en(zh: str) -> str:
    if zh in CURATED:
        return CURATED[zh]
    if looks_english(zh):
        return zh
    out = zh
    for a, b in REPLACEMENTS:
        out = out.replace(a, b)
    # If still mostly CJK, leave zh (honest fallback — sync registers source).
    if sum(1 for ch in out if "\u4e00" <= ch <= "\u9fff") > 0:
        return zh
    return out.strip()


TRANSLATIONS: dict[str, str] = {}
TRANSLATIONS.update(CHROME)
TRANSLATIONS.update(CURATED)

all_labels = set(role_labels) | set(input_labels) | set(choice_labels)
for label in sorted(all_labels):
    TRANSLATIONS.setdefault(label, auto_en(label))

data = json.loads(json_path.read_text(encoding="utf-8"))
entries = data.setdefault("entries", [])
by_zh = {e["zh_cn"]: e for e in entries if "zh_cn" in e}
existing_ids = {e["id"] for e in entries if "id" in e}

added = updated = 0


def make_id(en: str) -> str:
    base = "dlg." + re.sub(r"[^A-Za-z0-9]+", "_", en.lower()).strip("_")[:48]
    if not base or base == "dlg.":
        base = "dlg.label"
    entry_id = base
    n = 2
    while entry_id in existing_ids:
        entry_id = f"{base}_{n}"
        n += 1
    return entry_id


for zh, en in sorted(TRANSLATIONS.items(), key=lambda x: x[0]):
    if zh in by_zh:
        if by_zh[zh].get("en_us") != en:
            by_zh[zh]["en_us"] = en
            updated += 1
        continue
    eid = make_id(en if en != zh else zh)
    entry = {"id": eid, "zh_cn": zh, "en_us": en}
    entries.append(entry)
    by_zh[zh] = entry
    existing_ids.add(eid)
    added += 1

data["catalog_version"] = "2026-08-21.phase3-dialog-roles"
json_path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

still_zh = sorted(
    zh
    for zh, en in TRANSLATIONS.items()
    if zh in all_labels and en == zh and not looks_english(zh)
)
dump = root / "tools/_dialog_labels_dump.txt"
dump.write_text(
    "roles={}\n{}\n\ninputs={}\n{}\n\nchoices={}\n{}\n\nstill_zh={}\n{}\n".format(
        len(role_labels),
        "\n".join(role_labels),
        len(input_labels),
        "\n".join(sorted(input_labels)),
        len(choice_labels),
        "\n".join(sorted(choice_labels)),
        len(still_zh),
        "\n".join(still_zh),
    ),
    encoding="utf-8",
)
print(
    f"roles={len(role_labels)} inputs={len(input_labels)} choices={len(choice_labels)} "
    f"added={added} updated={updated} still_zh={len(still_zh)} entries={len(entries)}"
)
