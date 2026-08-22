# -*- coding: utf-8 -*-
"""Highest-ROI next bilingual slice: densest remaining unmapped ZH on en-US path."""
from __future__ import annotations

import pathlib
import re
from collections import Counter, defaultdict

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
cat = (root / "src/domain/report_text_catalog.cpp").read_text(encoding="utf-8")
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")
cjk = re.compile(r"[\u4e00-\u9fff]")
pair_re = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}')
str_re = re.compile(r'"((?:\\.|[^"\\])*)"')
triple = re.compile(
    r'\{\s*"([^"]+)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}'
)


def unesc2(s: str) -> str:
    return s.replace(r"\n", "\n").replace(r"\"", '"').replace(r"\\", "\\")


exact: dict[str, str] = {}
for m in pair_re.finditer(loc):
    a, b = unesc2(m.group(1)), unesc2(m.group(2))
    if cjk.search(a) or "（" in a or "σ" in a or "β" in a:
        exact[a] = b

catalog_zh: dict[str, str] = {}
for m in triple.finditer(cat):
    tid, zh, en = m.group(1), unesc2(m.group(2)), unesc2(m.group(3))
    if cjk.search(zh) or "（" in zh:
        catalog_zh[zh] = tid

sw = sorted(
    {unesc2(x) for x in re.findall(r'starts_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)},
    key=len,
    reverse=True,
)
ew = sorted(
    {unesc2(x) for x in re.findall(r'ends_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)},
    key=len,
    reverse=True,
)
contains = sorted(
    {
        unesc2(x)
        for x in re.findall(r'contains\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)
        if cjk.search(unesc2(x)) or len(unesc2(x)) >= 4
    },
    key=len,
    reverse=True,
)

# Also parse localize_dynamic / clause fragments that are literal ZH glue
dyn_literal = set()
for m in re.finditer(r'return\s+"((?:\\.|[^"\\])*)"\s*;', loc):
    s = unesc2(m.group(1))
    if cjk.search(s) and len(s) >= 4:
        dyn_literal.add(s)


def covered(s: str) -> tuple[bool, str]:
    if s in exact:
        return True, f"exact:{exact[s]}"
    if s in catalog_zh:
        return True, f"cat:{catalog_zh[s]}"
    for p in sw:
        if len(p) >= 3 and s.startswith(p):
            return True, f"sw:{p[:40]}"
    for p in ew:
        if len(p) >= 3 and s.endswith(p):
            return True, f"ew:{p[:40]}"
    # facet / method suffixes
    if s.endswith("（分面）"):
        base = s[: -len("（分面）")]
        ok, how = covered(base)
        if ok:
            return True, "facet+" + how
    for suf in (" 方法与参数", " 参数"):
        if s.endswith(suf):
            base = s[: -len(suf)]
            ok, how = covered(base)
            if ok:
                return True, "psuf+" + how
    for pref in (
        "分析限制：",
        "分析错误：",
        "分析警告：",
        "分析提示：",
        "限制：",
        "错误：",
        "警告：",
    ):
        if s.startswith(pref):
            ok, how = covered(s[len(pref) :])
            if ok:
                return True, "pref+" + how
    # long contains glue that fully explains short emits is NOT coverage for full bullets
    return False, ""


def family_of(rel: str, s: str) -> str:
    low = (rel + " " + s).lower()
    rules = [
        ("doe_rsm", ("doe", "rsm", "响应面", "部分析因", "中心复合", "box-behnken", "plackett", "效应", "失拟", "设计生成")),
        ("msa", ("gage", "量具", "msa", "偏倚", "线性", "ndc", "操作者", "评定者", "kappa", "kendall", "属性一致性")),
        ("reliability", ("可靠", "weibull", "删失", "kaplan", "保修", "失效", "log-rank", "竞争风险", "fine-gray", "aalen")),
        ("spc", ("控制图", "spc", "nelson", "ewma", "cusum", "i-mr", "xbar", "zone", "z-mr", "超限", "特殊原因", "mewma", "hotelling", "laney")),
        ("capability", ("能力", "cpk", "ppk", "规格", "box-cox", "johnson", "非正态", "ppm", "过程能力")),
        ("nonparametric", ("wilcoxon", "kruskal", "friedman", "mood", "sign", "runs", "mann", "dunn", "nemenyi", "steel", "非参数", "秩")),
        ("tables", ("列联", "fisher", "mcnemar", "cochran", "卡方", "交叉表", "gof", "期望频数")),
        ("inference", ("t 检验", "单样本", "双样本", "比例", "泊松率", "功效", "正态性", "grubbs", "dixon", "anom", "等价", "tost", "相关", "协方差", "anova", "tukey", "方差")),
        ("regression", ("回归", "logistic", "poisson", "逐步", "vif", "r²", "hosmer", "有序", "adf", "单位根")),
        ("forecast", ("arima", "预测", "季节", "分解", "mase", "mape", "acf", "pacf", "ljung")),
        ("ml", ("kmeans", "cart", "isolation", "层次聚类", "判别", "lda", "混合", "bootstrap")),
        ("eda_graph", ("密度", "hexbin", "小提琴", "条形", "分面", "箱线", "直方图", "散点", "多变异", "因果", "graph")),
        ("skip_cc", ("跳过", "complete-case", "排除了", "隐藏了", "缺失")),
    ]
    for fam, keys in rules:
        for k in keys:
            if k in low or k in s.lower() or k in s:
                return fam
    return "misc"


# ---- Collect candidates ----
items = []  # (kind, fam, rel, ln, zh)

# Chrome from app layer
chrome_files = [
    "src/application/analysis_service.cpp",
    "src/application/graph_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/chart_pages.cpp",
    "src/application/output_builder.cpp",
]
chrome_pats = [
    ("title", re.compile(r"(?:\.title|\.name|page\.title|table\.title|plot\.title)\s*=\s*\"((?:\\.|[^\"\\])*)\"")),
    ("header", re.compile(r"(?:headers\.push_back|columns\.push_back|add_header|add_column)\(\s*\"((?:\\.|[^\"\\])*)\"")),
    ("axis", re.compile(r"(?:x_axis_title|y_axis_title|x_label|y_label)\s*=\s*\"((?:\\.|[^\"\\])*)\"")),
    ("series", re.compile(r"(?:series(?:_name)?|legend(?:_label)?)\s*=\s*\"((?:\\.|[^\"\\])*)\"")),
    ("param", re.compile(r"(?:parameters|summary|add_parameter|make_parameter|param_rows).*?\"((?:\\.|[^\"\\])*)\"")),
]

for rel in chrome_files:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for ln, line in enumerate(text.splitlines(), 1):
        for kind, rx in chrome_pats:
            for m in rx.finditer(line):
                s = unesc2(m.group(1))
                if not cjk.search(s):
                    continue
                if len(s.strip()) < 1:
                    continue
                ok, _ = covered(s)
                if ok:
                    continue
                items.append((kind, family_of(rel, s), rel, ln, s))

# Domain fixed diagnostics
for fp in sorted((root / "src/domain/statistics").glob("*.cpp")):
    text = fp.read_text(encoding="utf-8", errors="replace")
    rel = str(fp.relative_to(root)).replace("\\", "/")
    for ln, line in enumerate(text.splitlines(), 1):
        if not any(k in line for k in ("message", "Diagnostic", "diagnostic", "push_back")):
            continue
        for m in str_re.finditer(line):
            s = unesc2(m.group(1))
            if not cjk.search(s) or len(s) < 4:
                continue
            ok, _ = covered(s)
            if ok:
                continue
            # gate / honesty / fixed diag shapes
            if any(
                t in s
                for t in (
                    "必须",
                    "不能",
                    "需要",
                    "无法",
                    "至少",
                    "不允许",
                    "超出",
                    "拒绝",
                    "门禁",
                    "诚实",
                    "研究预览",
                )
            ) or s.endswith(("。", "：", ":")):
                items.append(("diag", family_of(rel, s), rel, ln, s))

# App-layer diagnostic prefixes / fixed messages
for rel in (
    "src/application/analysis_service.cpp",
    "src/application/chart_pages.cpp",
    "src/application/graph_service.cpp",
):
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for ln, line in enumerate(text.splitlines(), 1):
        if not any(k in line for k in ("message", "Diagnostic", "diagnostic", "diag.")):
            continue
        for m in str_re.finditer(line):
            s = unesc2(m.group(1))
            if not cjk.search(s) or len(s) < 3:
                continue
            ok, _ = covered(s)
            if ok:
                continue
            if s.endswith(("。", "：", ":")) or "无法" in s or "失败" in s or "没有" in s:
                items.append(("diag", family_of(rel, s), rel, ln, s))

# Interpretation bullets: static full ZH strings and dynamic glue fragments
interp_lines = interp.splitlines()
for ln, line in enumerate(interp_lines, 1):
    # skip comments
    stripped = line.lstrip()
    if stripped.startswith("//") or stripped.startswith("/*"):
        continue
    for m in str_re.finditer(line):
        s = unesc2(m.group(1))
        if not cjk.search(s):
            continue
        if len(s) < 4:
            continue
        ok, _ = covered(s)
        if ok:
            continue
        # classify as static honesty vs dyn glue
        kind = "interp_static" if (s.endswith(("。", "；", ";")) and len(s) >= 12) else "interp_dyn"
        # skip pure format tokens that are too tiny glue only if <4 already handled
        items.append((kind, family_of("interpretation_service.cpp", s), "src/application/interpretation_service.cpp", ln, s))

# Dedup by (kind, zh) keep first line
seen = set()
uniq = []
for kind, fam, rel, ln, s in items:
    key = (kind, s)
    if key in seen:
        continue
    # exclude vendor_oracle / Johnson open / PDF-A per user
    if any(
        bad in s
        for bad in (
            "vendor_oracle",
            "PDF/A",
            "PDF/UA",
            "Johnson 研究预览",
            "Johnson 变换后的 Pp/Ppk",  # gate honesty already closed as "keep gate"
        )
    ):
        continue
    if "johnson" in fam and "开放" in s:
        continue
    seen.add(key)
    uniq.append((kind, fam, rel, ln, s))

# Cluster density
by_fam = Counter(f for _, f, *_ in uniq)
by_kind = Counter(k for k, *_ in uniq)
by_file = Counter(r for *_, r, _, _ in ((k, f, r, ln, s) for k, f, r, ln, s in uniq))

out = []
out.append(f"exact={len(exact)} catalog_zh={len(catalog_zh)} sw={len(sw)} ew={len(ew)}")
out.append(f"uniq_unmapped={len(uniq)}")
out.append("")
out.append("=== BY FAMILY ===")
for fam, n in by_fam.most_common():
    out.append(f"  {n}\t{fam}")
out.append("")
out.append("=== BY KIND ===")
for k, n in by_kind.most_common():
    out.append(f"  {n}\t{k}")
out.append("")

# For each top family, show items (cap 40)
for fam, n in by_fam.most_common(12):
    out.append(f"\n## {fam} n={n}")
    fam_items = [x for x in uniq if x[1] == fam]
    # prefer denser: group by kind counts
    for kind, rel, ln, s in [(a, c, d, e) for a, b, c, d, e in fam_items][:40]:
        out.append(f"  {kind}\t{rel}:{ln}\t{s}")

# Propose coherent slices of 15-25
out.append("\n\n=== SLICE CANDIDATES (target 15-25) ===")

# Slice A: inference static+dyn honesty leftovers
inf = [x for x in uniq if x[1] == "inference"]
# Slice B: remaining dyn misc fragments that share "，X =" pattern in interp
dyn = [x for x in uniq if x[0] == "interp_dyn"]
# Slice C: chrome leftovers titles+headers+params across one family
# Slice D: skip/complete-case
skip = [x for x in uniq if x[1] == "skip_cc" or "跳过" in x[4] or "排除了" in x[4] or "隐藏了" in x[4] or "缺失" in x[4]]
# Slice E: forecast ACF/ADF leftovers
fc = [x for x in uniq if x[1] in ("forecast", "regression") and any(t in x[4] for t in ("滞后", "自相关", "单位根", "ADF", "判定区", "Durbin"))]

def dump_slice(name, xs, limit=25):
    out.append(f"\n### {name} n={len(xs)}")
    for kind, fam, rel, ln, s in xs[:limit]:
        # suggest id
        slug = re.sub(r"[^a-zA-Z0-9]+", "_", s[:40]).strip("_").lower()[:48]
        out.append(f"  {rel}:{ln}\t{s}\t[{kind}/{fam}]\tsuggest_id~{slug}")

dump_slice("inference_cluster", inf)
dump_slice("interp_dyn_all", dyn, 40)
dump_slice("skip_cc_cluster", skip)
dump_slice("forecast_adf_cluster", fc)

# Densest: look at consecutive line clusters in interpretation_service
interp_items = sorted(
    [x for x in uniq if "interpretation_service" in x[2]],
    key=lambda x: x[3],
)
# sliding window density
best = (0, 0, 0, [])  # score, start, end, items
for i in range(len(interp_items)):
    window = []
    for j in range(i, len(interp_items)):
        if interp_items[j][3] - interp_items[i][3] > 120:
            break
        window.append(interp_items[j])
        if 15 <= len(window) <= 25:
            # score: same fam bonus + fewer fams
            fams = Counter(x[1] for x in window)
            score = len(window) * 10 - len(fams) * 3 + fams.most_common(1)[0][1] * 2
            if score > best[0]:
                best = (score, interp_items[i][3], interp_items[j][3], list(window))

out.append(f"\n### densest_interp_window score={best[0]} lines L{best[1]}-L{best[2]} n={len(best[3])}")
for kind, fam, rel, ln, s in best[3]:
    out.append(f"  {rel}:{ln}\t{s}\t[{kind}/{fam}]")

# Also densest chrome by file region
chrome_items = sorted([x for x in uniq if x[0] in ("title", "header", "axis", "series", "param")], key=lambda x: (x[2], x[3]))
bestc = (0, "", 0, 0, [])
for i in range(len(chrome_items)):
    window = []
    f0 = chrome_items[i][2]
    for j in range(i, len(chrome_items)):
        if chrome_items[j][2] != f0:
            break
        if chrome_items[j][3] - chrome_items[i][3] > 800:
            break
        window.append(chrome_items[j])
        if 15 <= len(window) <= 25:
            fams = Counter(x[1] for x in window)
            score = len(window) * 10 - len(fams) * 3 + fams.most_common(1)[0][1] * 2
            if score > bestc[0]:
                bestc = (score, f0, chrome_items[i][3], chrome_items[j][3], list(window))

out.append(f"\n### densest_chrome_window score={bestc[0]} file={bestc[1]} L{bestc[2]}-L{bestc[3]} n={len(bestc[4])}")
for kind, fam, rel, ln, s in bestc[4]:
    out.append(f"  {rel}:{ln}\t{s}\t[{kind}/{fam}]")

# Misc: show all static interp + remaining diags (often highest product ROI)
static = [x for x in uniq if x[0] == "interp_static"]
diags = [x for x in uniq if x[0] == "diag"]
out.append(f"\n### all_interp_static n={len(static)}")
for kind, fam, rel, ln, s in static:
    out.append(f"  {rel}:{ln}\t{s}\t[{fam}]")
out.append(f"\n### all_diags n={len(diags)}")
for kind, fam, rel, ln, s in diags:
    out.append(f"  {rel}:{ln}\t{s}\t[{fam}]")

path = root / "scripts/_tmp_next_slice_roi_out.txt"
path.write_text("\n".join(out), encoding="utf-8")
print(f"wrote {path}")
print(f"uniq={len(uniq)} top_fam={by_fam.most_common(5)}")
print(f"best_interp={best[0]} n={len(best[3])} L{best[1]}-{best[2]}")
print(f"best_chrome={bestc[0]} n={len(bestc[4])} {bestc[1]}")
