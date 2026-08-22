# -*- coding: utf-8 -*-
"""Accurate remaining unmapped ZH on en-US report path after chrome/skip slices."""
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


# Exact ZH keys from localize maps / catalog pairs in localization cpp
exact_keys: set[str] = set()
for m in pair_re.finditer(loc):
    a, b = unesc(m.group(1)), unesc(m.group(2))
    if cjk.search(a):
        exact_keys.add(a)

# Also catalog ZH third field: {"id", "zh", "en"}
cat = (root / "src/domain/report_text_catalog.cpp").read_text(encoding="utf-8")
# triples roughly: {"id", "zh", "en"}
triple = re.compile(
    r'\{\s*"([^"]+)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}'
)
catalog_zh: set[str] = set()
catalog_ids: dict[str, str] = {}
for m in triple.finditer(cat):
    tid, zh, en = m.group(1), unesc(m.group(2)), unesc(m.group(3))
    if cjk.search(zh):
        catalog_zh.add(zh)
        catalog_ids[zh] = tid

# starts_with / ends_with / contains prefixes anywhere in localization
sw_all = re.findall(
    r'(?:starts_with|ends_with|contains)\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)',
    loc,
)
dyn_frags = [unesc(x) for x in sw_all if cjk.search(unesc(x)) or any(ch in unesc(x) for ch in "βσχ²")]
# keep meaningful length
dyn_frags = [x for x in dyn_frags if len(x) >= 4]
dyn_set = set(dyn_frags)


def covered(s: str) -> bool:
    if s in exact_keys or s in catalog_zh:
        return True
    for frag in dyn_set:
        if s.startswith(frag) or frag.startswith(s) or s == frag:
            return True
        # mid-sentence fragment already used as contains/ends_with
        if frag in s and len(frag) >= 10:
            # only if the emit is exactly the frag or starts with it
            pass
    # also: if any dyn frag starts with this emit (emit is a prefix piece already handled)
    for frag in dyn_set:
        if frag.startswith(s) and len(s) >= 6:
            return True
        if s.startswith(frag) and len(frag) >= 6:
            return True
    return False


# Title/header/axis maps
title_keys = set()
header_keys = set()
axis_keys = set()
for m in pair_re.finditer(loc):
    a, b = unesc(m.group(1)), unesc(m.group(2))
    if not cjk.search(a) and "（" not in a:
        # Hexbin-like English bases still mapped
        if b.startswith("table.") or b.startswith("plot.") or b.startswith("page.") or b.startswith("model."):
            title_keys.add(a)
        continue
    if b.startswith("table.") or b.startswith("plot.") or b.startswith("page.") or b.startswith("model."):
        title_keys.add(a)
    elif b.startswith("header.") or b in ("table.property", "table.value"):
        header_keys.add(a)
    elif "axis" in b or b.startswith("plot.series"):
        axis_keys.add(a)

facet_suffix = "（分面）"
method_suffixes = [" 方法与参数", " 参数"]

title_asgn = re.compile(
    r"(?:\.title|\.name|page\.title|table\.title|plot\.title)\s*=\s*\"((?:\\.|[^\"\\])*)\""
)
header_asgn = re.compile(
    r"(?:headers\.push_back|columns\.push_back|add_header|add_column)\(\s*\"((?:\\.|[^\"\\])*)\""
)
axis_asgn = re.compile(
    r"(?:x_axis_title|y_axis_title|x_label|y_label)\s*=\s*\"((?:\\.|[^\"\\])*)\""
)

files = [
    "src/application/analysis_service.cpp",
    "src/application/graph_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/chart_pages.cpp",
    "src/application/output_builder.cpp",
]

unmapped_titles = []
unmapped_headers = []
unmapped_axes = []
for rel in files:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for i, line in enumerate(text.splitlines(), 1):
        for m in title_asgn.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) and "（" not in s:
                continue
            if not cjk.search(s) and facet_suffix not in s:
                continue
            base = s
            if s.endswith(facet_suffix):
                base = s[: -len(facet_suffix)]
            for suf in method_suffixes:
                if s.endswith(suf):
                    base = s[: -len(suf)]
            if base in title_keys or s in title_keys or s in catalog_zh or base in catalog_zh:
                continue
            # facet: English base Hexbin may be mapped without CJK
            if base in title_keys:
                continue
            unmapped_titles.append((rel, i, s, base))
        for m in header_asgn.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s):
                continue
            if s in header_keys or s in title_keys or s in catalog_zh:
                continue
            unmapped_headers.append((rel, i, s))
        for m in axis_asgn.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s):
                continue
            if s in axis_keys or s in header_keys or s in title_keys or s in catalog_zh:
                continue
            unmapped_axes.append((rel, i, s))

# Domain diagnostics: fixed CJK messages from domain/*.cpp push diagnostics
diag_files = list((root / "src/domain/statistics").glob("*.cpp"))
diag_files += [
    root / "src/application/analysis_service.cpp",
    root / "src/application/doe_pages.cpp",
    root / "src/application/chart_pages.cpp",
    root / "src/application/graph_service.cpp",
]
diag_kw = re.compile(
    r"(push_diagnostic|add_diagnostic|Diagnostic\b|limitation|status_message|message\s*=)"
)
unmapped_diags = []
for fp in diag_files:
    rel = str(fp.relative_to(root)).replace("\\", "/")
    text = fp.read_text(encoding="utf-8", errors="replace")
    for i, line in enumerate(text.splitlines(), 1):
        if not diag_kw.search(line) and "diagnostics" not in line.lower():
            continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 4:
                continue
            if covered(s):
                continue
            # skip tiny labels that look like table titles mis-detected
            unmapped_diags.append((rel, i, s))

# Interpretation bullets
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")
unmapped_interp = []
for i, line in enumerate(interp.splitlines(), 1):
    if "push_back" not in line and "bullets" not in line and "limitations" not in line:
        if '<< "' not in line and '+= "' not in line and '= "' not in line:
            continue
    for m in str_re.finditer(line):
        s = unesc(m.group(1))
        if not cjk.search(s) or len(s) < 6:
            continue
        if covered(s):
            continue
        unmapped_interp.append((i, s, line.strip()[:120]))

# Unique
def uniq(items, key_idx=2):
    seen = set()
    out = []
    for it in items:
        k = it[key_idx] if not isinstance(it[key_idx], tuple) else it[key_idx]
        # for titles key is s at index 2
        if isinstance(it, tuple) and len(it) >= 3:
            k = it[2] if key_idx == 2 else it[key_idx]
        if k in seen:
            continue
        seen.add(k)
        out.append(it)
    return out


diag_u = uniq(unmapped_diags, 2)
interp_u = []
seen = set()
for i, s, ctx in unmapped_interp:
    if s in seen:
        continue
    seen.add(s)
    interp_u.append((i, s, ctx))

# Classify interp: dynamic vs static honesty
dyn_interp = []
static_interp = []
for i, s, ctx in interp_u:
    is_dyn = (
        s.endswith(" = ")
        or s.endswith("=")
        or s.endswith("：")
        or s.endswith("（")
        or " = " in s[:40]
        or any(ch.isdigit() for ch in s) is False
        and (s.endswith(" =") or "N =" in s or "P =" in s)
    )
    # better heuristic
    looks_template = bool(
        re.search(r"( = |=|：|（N|χ²|β|λ|P =|N =|DF =)", s)
    ) and not s.endswith("。")
    if looks_template or s.endswith("：") or s.endswith(" = ") or s.endswith("="):
        dyn_interp.append((i, s, ctx))
    else:
        static_interp.append((i, s, ctx))


def family(s: str) -> str:
    rules = [
        ("nonparam", ["Wilcoxon", "Friedman", "Mood", "符号检验", "比较准则", "Kruskal", "Mann", "Runs", "Nemenyi", "Dunn", "Steel"]),
        ("tables", ["McNemar", "Cochran", "期望频数", "调整残差", "优势比", "分类，N", "缺失 N*", "交叉表", "拟合优度 Pearson", "卡方"]),
        ("ml", ["CART", "Poisson 回归", "Bootstrap", "有序 Logistic", "线性判别", "Isolation", "K-Means", "层次", "ADF", "PCA"]),
        ("reliability", ["Weibull", "对数正态", "指数", "Fine-Gray", "Turnbull", "保修", "删失", "KM", "CIF"]),
        ("inference", ["比例", "泊松率", "Box-Cox", "Bonett", "Bartlett", "Levene", "容差", "等价", "功效", "TOST", "ANOM", "正态", "异常值", "Grubbs", "Dixon"]),
        ("spc", ["CUSUM", "区域图", "移动平均", "I-MR", "Shewhart", "超限", "控制限", "阶段", "Laney", "EWMA", "Z-MR", "运行图"]),
        ("doe_msa", ["Desirability", "等值线", "RSM", "失拟", "Plackett", "Box–Behnken", "偏倚", "量具", "Gage", "中心点", "CCD", "BBD", "因子"]),
        ("regression", ["VIF", "Tukey", "ANOVA", "回归", "残差", "R²", "Hosmer", "Logistic", "DW", "Durbin"]),
        ("eda_graph", ["ACF", "PACF", "NIST", "四图", "Silverman", "密度", "hexbin", "小提琴", "Multi-Vari", "变异性", "鱼骨"]),
        ("capability", ["Cpk", "Ppk", "能力", "gate", "Hartigan", "双峰", "混合", "Johnson", "组间", "属性能力"]),
    ]
    for name, toks in rules:
        if any(t in s for t in toks):
            return name
    return "misc"


fam_dyn = Counter(family(s) for _, s, _ in dyn_interp)
fam_static = Counter(family(s) for _, s, _ in static_interp)
fam_diag = Counter(family(s) for _, _, s in diag_u)

out = root / "scripts/_tmp_remaining_unmapped.txt"
with out.open("w", encoding="utf-8") as f:
    f.write(
        f"exact={len(exact_keys)} dyn_frags={len(dyn_set)} catalog_zh={len(catalog_zh)}\n"
    )
    f.write(
        f"titles={len(unmapped_titles)} headers={len(unmapped_headers)} axes={len(unmapped_axes)} "
        f"diags_u={len(diag_u)} interp_u={len(interp_u)} dyn_interp={len(dyn_interp)} static_interp={len(static_interp)}\n"
    )
    f.write("\n=== UNMAPPED TITLES ===\n")
    for rel, i, s, base in unmapped_titles:
        f.write(f"title\t{rel}:{i}\t{s}\n")
    f.write("\n=== UNMAPPED HEADERS ===\n")
    for rel, i, s in unmapped_headers:
        f.write(f"header\t{rel}:{i}\t{s}\n")
    f.write("\n=== UNMAPPED AXES ===\n")
    for rel, i, s in unmapped_axes:
        f.write(f"axis\t{rel}:{i}\t{s}\n")
    f.write("\n=== DIAG FAMILY ===\n")
    for k, c in fam_diag.most_common():
        f.write(f"  {c}\t{k}\n")
    f.write("\n=== DYN INTERP FAMILY ===\n")
    for k, c in fam_dyn.most_common():
        f.write(f"  {c}\t{k}\n")
    f.write("\n=== STATIC INTERP FAMILY ===\n")
    for k, c in fam_static.most_common():
        f.write(f"  {c}\t{k}\n")
    f.write("\n=== UNMAPPED DIAGS ===\n")
    for rel, i, s in diag_u[:80]:
        f.write(f"diag\t{family(s)}\t{rel}:{i}\t{s}\n")
    f.write("\n=== UNMAPPED DYN INTERP ===\n")
    for i, s, ctx in sorted(dyn_interp, key=lambda t: (family(t[1]), t[0])):
        f.write(f"dyn\t{family(s)}\tL{i}\t{s}\n")
    f.write("\n=== UNMAPPED STATIC INTERP ===\n")
    for i, s, ctx in sorted(static_interp, key=lambda t: (family(t[1]), t[0])):
        f.write(f"static\t{family(s)}\tL{i}\t{s}\n")

print(f"wrote {out}")
print(
    f"titles={len(unmapped_titles)} headers={len(unmapped_headers)} axes={len(unmapped_axes)} "
    f"diags={len(diag_u)} interp={len(interp_u)} dyn={len(dyn_interp)} static={len(static_interp)}"
)
print("dyn families:", fam_dyn.most_common())
print("static families:", fam_static.most_common())
print("diag families:", fam_diag.most_common())
print("title samples:", [(t[0].split('/')[-1], t[1], t[2]) for t in unmapped_titles[:10]])
