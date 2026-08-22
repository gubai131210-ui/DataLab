# -*- coding: utf-8 -*-
"""Highest-ROI next bilingual slice after just-landed Phase 3 chrome."""
from __future__ import annotations

import pathlib
import re
from collections import Counter, defaultdict

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
cjk = re.compile(r"[\u4e00-\u9fff]")
pair_re = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}')
str_re = re.compile(r'"((?:\\.|[^"\\])*)"')


def unesc(s: str) -> str:
    return s.replace(r"\n", "\n").replace(r"\"", '"').replace(r"\\", "\\")


# --- maps ---
exact: dict[str, str] = {}
for m in pair_re.finditer(loc):
    a, b = unesc(m.group(1)), unesc(m.group(2))
    if cjk.search(a) and b and not cjk.search(b):
        exact[a] = b

# param tokens from localize_parameter_summary
param_block_start = loc.find("void localize_parameter_summary")
param_block = loc[param_block_start : param_block_start + 12000] if param_block_start >= 0 else ""
param_tokens: list[tuple[str, str]] = []
for m in pair_re.finditer(param_block):
    a, b = unesc(m.group(1)), unesc(m.group(2))
    if cjk.search(a) and b.startswith("param.") or b.startswith("param.") or (
        cjk.search(a) and ("param." in b or "alt." in b)
    ):
        param_tokens.append((a, b))
param_tokens = sorted(param_tokens, key=lambda x: len(x[0]), reverse=True)

# graph caption tokens
graph_start = loc.find("localize_graph_caption_tokens")
graph_block = loc[graph_start : graph_start + 4000] if graph_start >= 0 else ""
for m in pair_re.finditer(graph_block):
    a, b = unesc(m.group(1)), unesc(m.group(2))
    if cjk.search(a):
        param_tokens.append((a, b))
param_tokens = sorted(set(param_tokens), key=lambda x: len(x[0]), reverse=True)

sw = sorted(
    {unesc(x) for x in re.findall(r'starts_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)},
    key=len,
    reverse=True,
)
ew = sorted(
    {unesc(x) for x in re.findall(r'ends_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)},
    key=len,
    reverse=True,
)
find_marks = sorted(
    {
        unesc(x)
        for x in re.findall(r'\.find\(\s*"((?:\\.|[^"\\])*)"\s*\)', loc)
        if cjk.search(unesc(x)) or " = " in unesc(x)
    },
    key=len,
    reverse=True,
)
count_prefs = sorted(
    {
        unesc(x)
        for x in re.findall(
            r'parse_leading_count_after_prefix\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)',
            loc,
        )
    },
    key=len,
    reverse=True,
)


def covered_plain(s: str) -> tuple[bool, str]:
    if s in exact:
        return True, exact[s]
    st = s.strip()
    if st in exact:
        return True, exact[st]
    for p in sw:
        if len(p) >= 4 and s.startswith(p):
            return True, "sw:" + p[:40]
    for p in count_prefs:
        if len(p) >= 3 and s.startswith(p):
            return True, "count:" + p
    hits = [m for m in find_marks if len(m) >= 4 and m in s]
    if len(hits) >= 2:
        return True, "find2"
    for p in ew:
        if len(p) >= 10 and s.endswith(p) and p in exact:
            return True, "ew"
    # prefix strip
    for pref in ("分析限制：", "分析错误：", "分析警告：", "分析提示：", "限制：", "错误：", "警告："):
        if s.startswith(pref):
            return covered_plain(s[len(pref) :])
    return False, ""


def covered_paramish(s: str) -> tuple[bool, str]:
    ok, how = covered_plain(s)
    if ok:
        return True, how
    # token substring replace covers fragments
    remaining = s
    hit = []
    for tok, tid in param_tokens:
        if tok in remaining:
            remaining = remaining.replace(tok, "")
            hit.append(tid)
    # if after token wipe no CJK left (except punctuation glue), covered
    rest_cjk = cjk.findall(remaining)
    if hit and not rest_cjk:
        return True, "tokens:" + ",".join(hit[:3])
    if hit and len(rest_cjk) <= 1 and len(remaining.strip()) <= 4:
        return True, "tokens-almost:" + ",".join(hit[:3])
    # facet suffix
    if s.endswith("（分面）"):
        base = s[: -len("（分面）")]
        ok2, how2 = covered_plain(base)
        if ok2:
            return True, "facet+" + how2
    return False, remaining if hit else ""


# Just-landed exclusion keywords (already done this wave)
JUST_LANDED = (
    "假设均值",
    "备择",
    "合并方差",
    "行百分比",
    "列百分比",
    "合计百分比",
    "显示 N",
    "分析 N",
    "分面",
    "Hexbin",
    "标准化坐标",
    "直线性",
    "置信水平",
    "类别数",
    "面板",
    "规格",
    "σ 来源",
    "AD 判定",
    "PPM",
    "阶段",
    "历史",
    "TOST",
    "等价",
    "Wilcoxon",
    "Sign",
    "Runs",
    "Cochran",
    "Mood",
    "Kruskal",
    "Friedman",
    "Steel-Dwass",
    "Dunn",
    "AICc",
    "ARIMA",
    "近零 SSE",
    "因素 ID",
    "中心",
    "alpha",
    "append_diagnostics",
    "没有可显示的数据",
    "跳过 ",
    "complete-case",
    "Zone",
    "Z-MR",
    "MA σ",
    "组内 σ",
    "c4",
    "LSL",
    "USL",
    "ANOM",
)


def is_just_landed(s: str) -> bool:
    # Don't exclude whole families by short tokens like 面板 appearing in unrelated strings.
    # Use stronger markers from user list for exclusion of *already mapped* chrome.
    strong = (
        "假设均值",
        "合并方差",
        "行百分比",
        "列百分比",
        "合计百分比",
        "备择：总体均值",
        "备择: ",
        "直线性不能单独",
        "坐标已按各变量",
        "显示 N",
        "分析 N",
        "规格下限",
        "规格上限",
        "σ 来源",
        "AD 判定",
        "阶段 %1",
        "阶段（",
        "TOST ",
        "等价上限",
        "等价下限",
        "等价性",
        "AICc",
        "Best ARIMA",
        "近零 SSE",
        "因素 ID",
        "没有可显示的数据",
        "跳过 N 个",
        "跳过 ",
        "Zone 图",
        "Z-MR",
        "移动平均",
        "组内 σ",
        "σ̂_MR",
    )
    return any(k in s for k in strong)


SKIP_TOPIC = ("vendor_oracle", "PDF/A", "PDF/UA", "Johnson 研究预览", "Johnson 变换后的 Pp")


files_app = [
    "src/application/analysis_service.cpp",
    "src/application/interpretation_service.cpp",
    "src/application/graph_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/chart_pages.cpp",
    "src/application/output_builder.cpp",
]


def family(s: str) -> str:
    rules = [
        ("attrib_spc_oc", [
            "二项", "泊松", "不合格", "缺陷", "柏拉图", "原因", "累计比", "Other 阈值",
            "OC", "Laney", "Sigma Z", "p̄", "np̄", "c̄", "ū", "属性", "接收",
        ]),
        ("msa", ["操作员", "操作者", "零件", "量具", "Gage", "%Contribution", "%Study", "Wheeler", "分级", "偏倚"]),
        ("forecast_ts", ["预测期", "预测", "周期", "ARIMA", "季节", "分解", "窗宽", "选择准则", "MASE", "MAPE"]),
        ("inference_corr", [
            "相关", "协方差", "偏相关", "Dixon", "Grubbs", "ANOVA", "正态", "热图",
            "相关系数", "成对", "Durbin",
        ]),
        ("reliability", ["暴露", "删失", "失效", "可靠", "Weibull", "KM", "保修"]),
        ("doe_rsm", ["分辨度", "变体", "因子数", "设计族", "中心点", "星点", "Desirability", "等值线", "RSM"]),
        ("ml_cluster", ["得分阈值", "树数", "类数", "异常", "KMeans", "Isolation", "CART", "聚类", "Z 超限"]),
        ("capability", ["能力", "Cpk", "Ppk", "Box-Cox", "截断", "判定", "门禁", "双峰", "Hartigan"]),
        ("crosstab_prop", ["行水平", "列水平", "事件水平", "第一列", "第二列", "试验", "事件", "比例"]),
        ("graph_eda", ["分面", "Hexbin", "小提琴", "密度", "条形", "多变量", "平行"]),
        ("spc_mv", ["T²", "MEWMA", "广义方差", "控制图", "超限", "子组", "EWMA", "CUSUM"]),
        ("regression", ["回归", "预测变量", "VIF", "逐步", "Logistic", "Poisson", "有序"]),
    ]
    for name, kws in rules:
        if any(k in s for k in kws):
            return name
    return "misc"


items: list[tuple[str, str, int, str, str]] = []  # kind, file, ln, zh, leftover

# 1) Interpretation bullets (reconstruct)
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")
lines = interp.splitlines()
i = 0
while i < len(lines):
    line = lines[i]
    if "push_back" not in line:
        i += 1
        continue
    if not any(k in line for k in ("bullet", "limitation", "conclusion", "advice", "note")):
        i += 1
        continue
    chunk = line
    j = i
    depth = chunk.count("(") - chunk.count(")")
    while depth > 0 and j + 1 < len(lines):
        j += 1
        chunk += "\n" + lines[j]
        depth += lines[j].count("(") - lines[j].count(")")
        if j - i > 40:
            break
    lits = [unesc(m.group(1)) for m in str_re.finditer(chunk)]
    if not any(cjk.search(x) for x in lits):
        i = j + 1
        continue
    parts = []
    for lit in lits:
        if cjk.search(lit):
            parts.append(lit)
        elif lit in ("α = ", " = ", " / ", "、", "。", "；", "，", "（", "）", "%", " 与 ", " 下", "：", ":"):
            parts.append(lit)
        elif len(lit) <= 16 and any(ch in lit for ch in " =：:;，；()[]≈"):
            parts.append(lit)
    joined = "".join(parts)
    if cjk.search(joined) and len(joined) >= 8:
        if not any(b in joined for b in SKIP_TOPIC) and not is_just_landed(joined):
            ok, how = covered_plain(joined)
            if not ok:
                # also try if long exact substring covers most
                covered_frac = False
                for a in exact:
                    if len(a) >= 24 and a in joined:
                        covered_frac = True
                        break
                if not covered_frac:
                    items.append(("interp", "src/application/interpretation_service.cpp", i + 1, joined, how))
    i = j + 1

# 2) Titles / headers / axes
title_re = re.compile(
    r"(?:\.title|\.name|page\.title|table\.title|plot\.title)\s*=\s*\"((?:\\.|[^\"\\])*)\""
)
header_re = re.compile(
    r"(?:headers\.push_back|columns\.push_back)\(\s*\"((?:\\.|[^\"\\])*)\""
)
axis_re = re.compile(
    r"(?:x_axis_title|y_axis_title|x_label|y_label|series_name)\s*=\s*\"((?:\\.|[^\"\\])*)\""
)

for rel in files_app:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for ln, line in enumerate(text.splitlines(), 1):
        for kind, rx in (("title", title_re), ("header", header_re), ("axis", axis_re)):
            for m in rx.finditer(line):
                s = unesc(m.group(1))
                if not cjk.search(s) and "（" not in s:
                    continue
                if not cjk.search(s):
                    continue
                if is_just_landed(s) or any(b in s for b in SKIP_TOPIC):
                    continue
                ok, how = covered_paramish(s) if kind != "title" else covered_plain(s)
                if kind == "title":
                    ok, how = covered_plain(s)
                    if s.endswith("（分面）"):
                        base = s[: -len("（分面）")]
                        if base in exact or covered_plain(base)[0]:
                            ok = True
                if not ok:
                    items.append((kind, rel, ln, s, how))

# 3) Parameter summary producers (CJK literals that look like chrome)
param_shape = re.compile(r"[\u4e00-\u9fff].*(?:=|：|:)|^(?:分布|模型|方法|目标|权重|排序|预测|判定|截断)")
for rel in (
    "src/application/analysis_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/graph_service.cpp",
):
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for ln, line in enumerate(text.splitlines(), 1):
        if "parameter" not in line.lower() and "summary" not in line.lower() and "push_back" not in line:
            # still catch string with = that are param rows
            if '    "' not in line and 'QStringLiteral' not in line:
                continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 2:
                continue
            if not param_shape.search(s) and s not in ("权重", "排序", "预测", "目标", "判定", "截断", "分布", "原因"):
                continue
            if is_just_landed(s) or any(b in s for b in SKIP_TOPIC):
                continue
            ok, how = covered_paramish(s)
            if not ok:
                items.append(("param", rel, ln, s, how))

# 4) Fixed diagnostics with CJK
for fp in list((root / "src/domain/statistics").glob("*.cpp")) + [
    root / "src/application/analysis_service.cpp",
    root / "src/application/chart_pages.cpp",
]:
    text = fp.read_text(encoding="utf-8", errors="replace")
    rel = str(fp.relative_to(root)).replace("\\", "/")
    for ln, line in enumerate(text.splitlines(), 1):
        if not any(k in line for k in ("message", "diagnostic", "Diagnostic", "append_diag")):
            continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 4:
                continue
            if is_just_landed(s) or any(b in s for b in SKIP_TOPIC):
                continue
            ok, how = covered_plain(s)
            if not ok and (s.endswith(("。", "：")) or "无法" in s or "失败" in s or "要求" in s or "至少" in s):
                items.append(("diag", rel, ln, s, how))

# Dedup by zh text
seen = set()
uniq = []
for kind, rel, ln, s, left in items:
    key = s.strip()
    if key in seen:
        continue
    seen.add(key)
    uniq.append((kind, rel, ln, s, left))

by_fam = Counter(family(s) for *_, s, _ in uniq)
by_kind = Counter(k for k, *_ in uniq)

out = []
out.append(f"exact={len(exact)} param_tokens={len(param_tokens)} sw={len(sw)}")
out.append(f"uniq_unmapped={len(uniq)} kinds={dict(by_kind)} families={by_fam.most_common(12)}")

# Build candidate packs (coherent slices)
packs = defaultdict(list)
for kind, rel, ln, s, left in uniq:
    fam = family(s)
    packs[fam].append((kind, rel, ln, s, left))

out.append("\n=== FAMILY SIZES ===")
for fam, n in by_fam.most_common():
    out.append(f"  {n}\t{fam}")

# Enrich attrib_spc_oc with related chrome from analysis_service near Pareto/attr
# Also scan for strings containing 原因/累计/检查假设 even if not param-shaped
extra_scan_kws = {
    "attrib_spc_oc": ["柏拉图", "累计比例", "原因数", "类别原因", "Other 阈值", "检查假设", "Sigma Z", "Laney",
                      "分布 = 二项", "分布 = 泊松", "二项分布", "泊松分布", "二项 OC", "不合格品", "缺陷数"],
    "forecast_ts": ["预测期数", "选择准则", "窗宽", "周期 =", "模型 = 线性", "有效运行数"],
    "msa": ["操作员", "操作者数", "过程变差", "分级"],
    "inference_corr": ["偏相关", "有效变量数", "Dixon", "相关系数"],
    "doe_rsm": ["分辨度", "变体", "设计族", "因子数", "线性+交互"],
    "ml_cluster": ["得分阈值", "Z 超限", "窗宽 w", "预测变量数"],
    "crosstab_prop": ["行水平数", "列水平数", "事件水平", "总体中位数"],
    "reliability": ["暴露量合计", "区组"],
}

for fam, kws in extra_scan_kws.items():
    for rel in ("src/application/analysis_service.cpp", "src/application/graph_service.cpp",
                "src/application/interpretation_service.cpp"):
        text = (root / rel).read_text(encoding="utf-8", errors="replace")
        for ln, line in enumerate(text.splitlines(), 1):
            for m in str_re.finditer(line):
                s = unesc(m.group(1))
                if not cjk.search(s):
                    continue
                if not any(k in s for k in kws):
                    continue
                if is_just_landed(s) or any(b in s for b in SKIP_TOPIC):
                    continue
                ok, how = covered_paramish(s)
                if ok:
                    continue
                key = s.strip()
                if key in seen:
                    continue
                seen.add(key)
                packs[fam].append(("chrome", rel, ln, s, how))
                uniq.append(("chrome", rel, ln, s, how))

# Recompute sizes after enrich
out.append("\n=== PACK SIZES (enriched) ===")
pack_sizes = {f: len({x[3] for x in xs}) for f, xs in packs.items()}
for fam, n in sorted(pack_sizes.items(), key=lambda x: -x[1]):
    out.append(f"  {n}\t{fam}")

# Pick densest pack in 15-25, else closest above/below preferring report-path chrome density
candidates = sorted(pack_sizes.items(), key=lambda x: (-min(abs(x[1] - 20), 20 - min(x[1], 20)), -x[1]))
# Prefer packs with size in [12, 30]
in_range = [(f, n) for f, n in pack_sizes.items() if 12 <= n <= 30]
if in_range:
    best = max(in_range, key=lambda x: x[1])
else:
    # take largest under 40 or closest to 20
    best = max(pack_sizes.items(), key=lambda x: (0 if abs(x[1] - 20) > 15 else 1, -abs(x[1] - 20), x[1]))

best_fam, best_n = best
out.append(f"\n=== SELECTED PACK: {best_fam} n={best_n} ===")

# Dedup and trim to 15-25 best items (prefer param/title/header/diag/interp order)
selected = []
seen2 = set()
priority = {"param": 0, "chrome": 1, "title": 2, "header": 3, "axis": 4, "diag": 5, "interp": 6}
for kind, rel, ln, s, left in sorted(packs[best_fam], key=lambda x: (priority.get(x[0], 9), -len(x[3]))):
    if s in seen2:
        continue
    seen2.add(s)
    selected.append((kind, rel, ln, s, left))
    if len(selected) >= 25:
        break

# If under 15, pull from misc related or second-best related families
if len(selected) < 15:
    # merge adjacent packs for quality narrative
    merge_map = {
        "attrib_spc_oc": ["spc_mv", "capability"],
        "forecast_ts": ["regression", "ml_cluster"],
        "inference_corr": ["crosstab_prop", "misc"],
        "msa": ["misc"],
        "doe_rsm": ["misc"],
        "ml_cluster": ["forecast_ts"],
        "crosstab_prop": ["inference_corr"],
    }
    for other in merge_map.get(best_fam, []):
        for kind, rel, ln, s, left in packs.get(other, []):
            if s in seen2:
                continue
            seen2.add(s)
            selected.append((kind, rel, ln, s, left))
            if len(selected) >= 20:
                break
        if len(selected) >= 15:
            break

# Propose catalog ids
def propose_id(kind: str, s: str) -> str:
    # heuristic id
    base = {
        "param": "param.summary.",
        "chrome": "param.summary.",
        "title": "table.",
        "header": "header.",
        "axis": "plot.axis.",
        "diag": "diag.",
        "interp": "interp.",
    }.get(kind, "msg.")
    # ascii slug from known tokens
    slug_map = [
        ("分布 = 二项分布", "dist_binomial_eq"),
        ("分布 = 泊松分布", "dist_poisson_eq"),
        ("分布 = 二项", "dist_binomial_short_eq"),
        ("分布 = 泊松", "dist_poisson_short_eq"),
        ("二项 OC", "binomial_oc"),
        ("Other 阈值", "other_threshold_eq"),
        ("原因数", "cause_count_eq"),
        ("类别原因计数", "category_cause_counts"),
        ("检查假设", "check_assumptions"),
        ("累计比例", "cumulative_proportion"),
        ("Sigma Z", "laney_sigma_z"),
        ("超限点", "spc_exceedance_count"),
        ("偏相关", "partial_corr"),
        ("有效变量数", "valid_variable_count_eq"),
        ("操作员", "operator_eq"),
        ("操作者数", "operator_count_eq"),
        ("过程变差", "process_variation_eq"),
        ("预测期数", "forecast_horizon_eq"),
        ("选择准则", "selection_criterion_eq"),
        ("窗宽", "window_width_eq"),
        ("周期", "period_eq"),
        ("分辨度", "resolution_eq"),
        ("变体", "variant_eq"),
        ("设计族", "design_family_eq"),
        ("因子数", "factor_count_eq"),
        ("得分阈值", "score_threshold_eq"),
        ("Z 超限", "z_exceedance_eq"),
        ("暴露量合计", "total_exposure_eq"),
        ("行水平数", "row_level_count_eq"),
        ("列水平数", "col_level_count_eq"),
        ("事件水平", "event_level_eq"),
        ("总体中位数", "population_median_eq"),
        ("有效运行数", "valid_run_count_eq"),
        ("预测变量数", "predictor_count_eq"),
        ("区组", "block_eq"),
        ("模型 = 线性", "model_linear_interact_pure_quad"),
        ("截断", "truncation"),
        ("判定", "decision"),
        ("权重", "weight"),
        ("排序", "sort"),
        ("预测", "forecast"),
        ("目标", "target"),
        ("Dixon", "dixon_r10_assumption"),
        ("相关系数", "corr_coeff_honesty"),
        ("Wheeler", "wheeler_grade_honesty"),
        ("%Contribution", "pct_contribution_vs_study_var"),
        ("正态假设", "normality_not_proven"),
        ("对照测量", "comparison_measurement_honesty"),
        ("规则「", "rule_triggered_template"),
        ("不可识别", "reliability_unidentifiable"),
        ("I-MR-R/S", "imrrs_compute_failed"),
        ("部分析因分辨度", "frac_factorial_resolution"),
    ]
    for zh, slug in slug_map:
        if zh in s:
            return base + slug
    # fallback
    import hashlib
    h = hashlib.md5(s.encode()).hexdigest()[:8]
    return base + "zh_" + h


def propose_en(s: str) -> str:
    # best-effort draft EN for the slice table (agent return; not applied)
    known = {
        "检查假设": "Check assumptions",
        "原因": "Cause",
        "原因数 = ": "Cause count = ",
        " — 类别原因计数": " — category cause counts",
        "累计比例": "Cumulative proportion",
        "Other 阈值 = ": "Other threshold = ",
        "模型 = 二项 OC    n = ": "Model = binomial OC    n = ",
        "分布 = 二项    子组数 = ": "Distribution = binomial    subgroups = ",
        "分布 = 泊松    子组数 = ": "Distribution = Poisson    subgroups = ",
        "分布 = 二项分布    p̄ = Σ不合格品数 / Σ检验数    ": "Distribution = binomial    p̄ = Σdefectives / Σtrials    ",
        "分布 = 二项分布    np̄_i = n_i p̄    ": "Distribution = binomial    np̄_i = n_i p̄    ",
        "分布 = 泊松分布    c̄ = 缺陷数均值    ": "Distribution = Poisson    c̄ = mean defects    ",
        "分布 = 泊松分布    ū = Σ缺陷数 / Σ单位数    ": "Distribution = Poisson    ū = Σdefects / Σunits    ",
        "判定": "Decision",
        "截断": "Truncation",
        "分布": "Distribution",
        "权重": "Weight",
        "排序": "Sort",
        "预测": "Forecast",
        "目标": "Target",
    }
    if s in known:
        return known[s]
    # soft replacements
    t = s
    reps = [
        ("分布 = 二项分布", "Distribution = binomial"),
        ("分布 = 泊松分布", "Distribution = Poisson"),
        ("分布 = 二项", "Distribution = binomial"),
        ("分布 = 泊松", "Distribution = Poisson"),
        ("子组数 = ", "subgroups = "),
        ("不合格品数", "defectives"),
        ("检验数", "trials"),
        ("缺陷数均值", "mean defects"),
        ("缺陷数", "defects"),
        ("单位数", "units"),
        ("模型 = 二项 OC", "Model = binomial OC"),
        ("Other 阈值 = ", "Other threshold = "),
        ("原因数 = ", "Cause count = "),
        ("类别原因计数", "category cause counts"),
        ("检查假设", "Check assumptions"),
        ("累计比例", "Cumulative proportion"),
        ("发现 ", "Found "),
        ("个控制图超限点。", " control-chart exceedance(s)."),
        ("存在过度离散，传统控制限可能过窄。", "overdispersion present; traditional limits may be too narrow."),
        ("控制限已按离散程度进行调整。", "limits adjusted for dispersion."),
        ("，分级 = ", ", grade = "),
        ("这是 Wheeler 监控能力分级，不是 AIAG 合格判定，不能写成量具合格。",
         "This is a Wheeler monitoring capability grade, not an AIAG pass/fail decision, and must not be stated as gage acceptance."),
        ("；%Contribution 与 %Study Var 口径不同，不能混用。",
         "; %Contribution and %Study Var use different bases and must not be mixed."),
        ("；用于对照测量增量是否物理合理，不是公差合格证明。",
         "; use to judge whether the measurement increment is physically sensible — not proof of tolerance conformance."),
        ("；未拒绝正态假设不等于已证明正态分布。",
         "; failing to reject normality is not proof the data are normal."),
        ("规则「", "Rule '"),
        ("」已触发。", "' has triggered."),
        ("Dixon r10 要求近似正态、至多一个异常值，且 P 可能为临界值插值近似",
         "Dixon r10 assumes approximate normality, at most one outlier, and P may be a critical-value interpolation approximation"),
        ("相关系数表示变量关联方向与强度；P-Value 反映在零相关假设下的证据强度，不能单独证明因果关系。未拒绝零相关不能写成已证明无关。",
         "The correlation coefficient describes association direction/strength; the P-value is evidence against zero correlation and does not alone prove causation. Failing to reject zero correlation is not proof of independence."),
        ("可靠性结果当前不可识别，不能估计寿命分位数。可靠性结果不可识别（",
         "Reliability results are currently unidentifiable; lifetime quantiles cannot be estimated. Reliability results unidentifiable ("),
        ("），不能估计寿命分位数。", "); lifetime quantiles cannot be estimated."),
        ("无法计算 I-MR-R/S 控制图。", "Unable to compute the I-MR-R/S control chart."),
        ("部分析因分辨度 ", "Fractional factorial resolution "),
        ("偏相关 = 是", "Partial correlation = yes"),
        ("有效变量数 = ", "Valid variables = "),
        ("缺失值 N* = ", "Missing N* = "),
        ("预测期数 = ", "Forecast horizon = "),
        ("选择准则 = ", "Selection criterion = "),
        ("窗宽 w = ", "Window width w = "),
        ("周期 = ", "Period = "),
        ("得分阈值 = ", "Score threshold = "),
        ("Z 超限 = ", "Z exceedances = "),
        ("暴露量合计 = ", "Total exposure = "),
        ("有效运行数 = ", "Valid runs = "),
        ("预测变量数 = ", "Predictors = "),
        ("操作员 = ", "Operator = "),
        ("操作者数 = ", "Operators = "),
        ("过程变差(6σ) = ", "Process variation (6σ) = "),
        ("分辨度 = ", "Resolution = "),
        ("变体 = ", "Variant = "),
        ("；设计族 = ", "; design family = "),
        ("因子数 = ", "Factors = "),
        ("模型 = 线性+交互+纯二次（编码单位）", "Model = linear+interaction+pure quadratic (coded units)"),
        ("；总体中位数 M = ", "; population median M = "),
        ("，列水平数 = ", ", column levels = "),
        ("区组 = ", "Blocks = "),
        ("事件水平 = ", "Event level = "),
        ("目标 = ", "Target = "),
        ("事件 = ", "Events = "),
        ("试验 = ", "Trials = "),
        ("第一列 = ", "First column = "),
        ("第二列 = ", "Second column = "),
        ("变换 = ", "Transform = "),
        ("参考 = ", "Reference = "),
        ("比较 = ", "Comparison = "),
        ("分组 = （单过程）", "Grouping = (single process)"),
        ("分组列 = ", "Grouping column = "),
        ("有效观测 = ", "Valid observations = "),
        ("预测变量 = ", "Predictors = "),
        ("σ 方法 = ", "σ method = "),
        ("方法 = percentile", "Method = percentile"),
    ]
    for a, b in reps:
        t = t.replace(a, b)
    return t


# If selected still wrong family size, force attrib_spc_oc as primary narrative if it's densest chrome leftover
attrib = packs.get("attrib_spc_oc", [])
if len({x[3] for x in attrib}) >= 12 and best_fam != "attrib_spc_oc":
    # Only override if attrib is denser coherent report chrome
    if pack_sizes.get("attrib_spc_oc", 0) >= pack_sizes.get(best_fam, 0):
        best_fam = "attrib_spc_oc"
        selected = []
        seen2 = set()
        for kind, rel, ln, s, left in sorted(packs[best_fam], key=lambda x: (priority.get(x[0], 9), -len(x[3]))):
            if s in seen2:
                continue
            seen2.add(s)
            selected.append((kind, rel, ln, s, left))
            if len(selected) >= 25:
                break

out.append(f"FINAL_SLICE={best_fam} count={len(selected)}")
out.append("")
out.append("| # | file:line | kind | ZH | id | en-US | map |")
out.append("|---|---|---|---|---|---|---|")
for i, (kind, rel, ln, s, left) in enumerate(selected, 1):
    tid = propose_id(kind, s)
    en = propose_en(s)
    # map path hint
    if kind in ("param", "chrome"):
        mp = "localize_parameter_summary token"
    elif kind == "interp":
        mp = "localize_known_plain_message / interp template"
    elif kind == "diag":
        mp = "localize_known_plain_message"
    elif kind == "title":
        mp = "localize_page_title / table title map"
    elif kind == "header":
        mp = "localize_table_headers"
    elif kind == "axis":
        mp = "localize_plot_axis"
    else:
        mp = "report_localization exact"
    zh_show = s.replace("|", "\\|").replace("\n", " ")
    en_show = en.replace("|", "\\|").replace("\n", " ")
    out.append(f"| {i} | `{rel}:{ln}` | {kind} | {zh_show} | `{tid}` | {en_show} | {mp} |")

# Also dump other large families for sanity
out.append("\n=== OTHER FAMILY PREVIEWS (top 8 each) ===")
for fam, n in sorted(pack_sizes.items(), key=lambda x: -x[1])[:6]:
    if fam == best_fam:
        continue
    out.append(f"\n## {fam} n={n}")
    seen3 = set()
    for kind, rel, ln, s, left in packs[fam][:20]:
        if s in seen3:
            continue
        seen3.add(s)
        out.append(f"  {kind}\t{rel}:{ln}\t{s[:100]}")

text = "\n".join(out)
outp = root / "scripts/_tmp_roi_next_slice_out.txt"
outp.write_text(text, encoding="utf-8")
print(f"wrote {outp}")
print(f"selected={best_fam} n={len(selected)} uniq={len(uniq)}")
print("top families:", by_fam.most_common(8))
