# -*- coding: utf-8 -*-
"""Find densest truly-unmapped ZH on report path after accounting for SW/EW/exact."""
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


exact = {unesc(m.group(1)): unesc(m.group(2)) for m in pair_re.finditer(loc) if cjk.search(unesc(m.group(1)))}
sw = sorted({unesc(x) for x in re.findall(r'starts_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)}, key=len, reverse=True)
ew = sorted({unesc(x) for x in re.findall(r'ends_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)}, key=len, reverse=True)
# also parse_leading_count_after_prefix and find marks
misc_pref = sorted(
    {
        unesc(x)
        for x in re.findall(
            r'(?:parse_leading_count_after_prefix|find)\(\s*[^,]*,\s*"((?:\\.|[^"\\])*)"',
            loc,
        )
    },
    key=len,
    reverse=True,
)


def covered(s: str) -> bool:
    if s in exact:
        return True
    for p in sw:
        if len(p) >= 3 and s.startswith(p):
            return True
    for p in misc_pref:
        if len(p) >= 3 and (s.startswith(p) or p in s):
            # find marks alone don't cover full string; only if used as known template glue
            pass
    # facet
    if s.endswith("（分面）"):
        base = s[: -len("（分面）")]
        if base in exact or any(base.startswith(p) for p in sw if len(p) >= 3):
            return True
    for suf in (" 方法与参数", " 参数"):
        if s.endswith(suf):
            base = s[: -len(suf)]
            if base in exact:
                return True
    return False


# Scan report-facing assignment patterns across app layer
files = [
    "src/application/analysis_service.cpp",
    "src/application/graph_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/chart_pages.cpp",
    "src/application/output_builder.cpp",
    "src/application/interpretation_service.cpp",
]

# Patterns that become visible report chrome
chrome_pats = [
    ("title", re.compile(r"(?:\.title|\.name|page\.title|table\.title|plot\.title)\s*=\s*\"((?:\\.|[^\"\\])*)\"")),
    ("header", re.compile(r"(?:headers\.push_back|columns\.push_back|add_header|add_column)\(\s*\"((?:\\.|[^\"\\])*)\"")),
    ("axis", re.compile(r"(?:x_axis_title|y_axis_title|x_label|y_label)\s*=\s*\"((?:\\.|[^\"\\])*)\"")),
    ("series", re.compile(r"(?:series_name|legend_label|add_series)\([^\"]*\"((?:\\.|[^\"\\])*)\"")),
    ("param", re.compile(r"(?:parameters\.push_back|summary\.push_back|rows\.push_back|add_parameter|make_row)\([^\"]{0,40}\"((?:\\.|[^\"\\])*)\"")),
    ("cell", re.compile(r"(?:cells\.push_back|values\.push_back|row\.push_back)\(\s*\"((?:\\.|[^\"\\])*)\"")),
]

items = []
for rel in files:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for ln, line in enumerate(text.splitlines(), 1):
        for kind, rx in chrome_pats:
            for m in rx.finditer(line):
                s = unesc(m.group(1))
                if not cjk.search(s):
                    continue
                if len(s) < 1:
                    continue
                if covered(s):
                    continue
                items.append((kind, rel, ln, s))

# Domain fixed diags (ending 。) not covered
for fp in (root / "src/domain/statistics").glob("*.cpp"):
    text = fp.read_text(encoding="utf-8", errors="replace")
    rel = str(fp.relative_to(root)).replace("\\", "/")
    for ln, line in enumerate(text.splitlines(), 1):
        if "message" not in line and "Diagnostic" not in line and "diagnostic" not in line:
            continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 4:
                continue
            if covered(s):
                continue
            # dynamic template starts ending without 。
            if s.endswith(("。", "：", ":")) or "必须" in s or "不能" in s or "需要" in s or "无法" in s or "至少" in s:
                items.append(("diag", rel, ln, s))

# Dedup by (kind, text)
seen = set()
uniq = []
for kind, rel, ln, s in items:
    key = (kind, s)
    if key in seen:
        continue
    seen.add(key)
    uniq.append((kind, rel, ln, s))

by_kind = Counter(k for k, _, _, _ in uniq)
by_file = Counter(pathlib.Path(r).name for _, r, _, _ in uniq)

# Cluster by family from path + keywords
def fam(kind, rel, s):
    p = rel.lower()
    if "interpretation" in p:
        return "interp_" + kind
    if any(x in s for x in ("Gage", "量具", "偏倚", "ndc", "EMP", "Type 1", "评估者", "零件", "Kappa", "Kendall")):
        return "msa_aa"
    if any(x in s for x in ("控制", "σ", "EWMA", "CUSUM", "区域", "I-MR", "Xbar", "超限", "Nelson", "Z-MR", "广义方差", "T²", "MEWMA")):
        return "spc"
    if any(x in s for x in ("DOE", "因子", "CCD", "BBD", "RSM", "等值", "效应", "Desirability", "中心点", "分辨")):
        return "doe_rsm"
    if any(x in s for x in ("能力", "Cpk", "Ppk", "Johnson", "双峰", "Box-Cox", "规格")):
        return "capability"
    if any(x in s for x in ("删失", "Weibull", "KM", "生存", "保修", "CIF", "失效")):
        return "reliability"
    if any(x in s for x in ("Wilcoxon", "Kruskal", "Friedman", "Mood", "Sign", "Runs", "Mann", "Dunn", "Steel")):
        return "nonparam"
    if any(x in s for x in ("相关", "回归", "ANOVA", "Tukey", "Logistic", "泊松", "比例", "功效", "样本", "等价", "TOST", "正态", "异常")):
        return "inference_reg"
    if any(x in s for x in ("密度", "Hexbin", "小提琴", "箱线", "因果", "条形", "分面", "热图", "ACF")):
        return "eda_graph"
    if any(x in s for x in ("ARIMA", "预测", "分解", "季节", "Bootstrap", "聚类", "CART", "Isolation", "K-Means", "判别", "PCA", "ADF")):
        return "ml_forecast"
    if "graph" in p or "chart" in p:
        return "chart_app"
    return "misc_" + kind

by_fam = Counter(fam(k, r, s) for k, r, _, s in uniq)

out = []
out.append(f"exact={len(exact)} sw={len(sw)} uniq_unmapped={len(uniq)}")
out.append("\nBY KIND")
for k, v in by_kind.most_common():
    out.append(f"  {v}\t{k}")
out.append("\nBY FAM")
for k, v in by_fam.most_common():
    out.append(f"  {v}\t{k}")
out.append("\nBY FILE")
for k, v in by_file.most_common(15):
    out.append(f"  {v}\t{k}")

out.append("\n=== ITEMS BY FAM ===")
grouped = defaultdict(list)
for k, r, ln, s in uniq:
    grouped[fam(k, r, s)].append((k, r, ln, s))
for f, lst in sorted(grouped.items(), key=lambda kv: -len(kv[1])):
    out.append(f"\n## {f} n={len(lst)}")
    for k, r, ln, s in lst[:30]:
        out.append(f"  {k}\t{r}:{ln}\t{s}")

text = "\n".join(out)
(root / "scripts/_tmp_slice_roi_deep_out2.txt").write_text(text, encoding="utf-8")
print(text[:12000])
