# -*- coding: utf-8 -*-
"""True remaining ROI: reconstruct concat bullets; find uncovered chrome/diags/interps."""
from __future__ import annotations

import pathlib
import re
from collections import Counter, defaultdict

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")
cjk = re.compile(r"[\u4e00-\u9fff]")
pair_re = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}')
str_re = re.compile(r'"((?:\\.|[^"\\])*)"')


def unesc(s: str) -> str:
    return s.replace(r"\n", "\n").replace(r"\"", '"').replace(r"\\", "\\")


exact: dict[str, str] = {}
for m in pair_re.finditer(loc):
    a, b = unesc(m.group(1)), unesc(m.group(2))
    if cjk.search(a):
        exact[a] = b

# starts_with prefixes AND ends_with suffixes used in localization
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
# also bullet.find("ZH") markers used as mid-glue in templates
find_marks = sorted(
    {
        unesc(x)
        for x in re.findall(r'\.find\(\s*"((?:\\.|[^"\\])*)"\s*\)', loc)
        if cjk.search(unesc(x)) or " = " in unesc(x)
    },
    key=len,
    reverse=True,
)


def covered_bullet(s: str) -> tuple[bool, str]:
    if s in exact:
        return True, "exact:" + exact[s]
    for p in sw:
        if len(p) >= 4 and s.startswith(p):
            # if there's an ends_with constraint in same branch we can't fully know;
            # treat starts_with as covered for dynamic templates
            return True, "sw:" + p[:40]
    # ends_with alone doesn't cover; need start
    return False, ""


# Reconstruct push_back string concatenations spanning multiple lines
lines = interp.splitlines()
bullets: list[tuple[int, str]] = []
i = 0
while i < len(lines):
    line = lines[i]
    if "bullets.push_back" not in line and "limitations.bullets" not in line:
        # also conclusion/advice push
        if ".push_back(" not in line or "bullet" not in line.lower() and "push_back" not in line:
            if "bullets.push_back" not in line:
                i += 1
                continue
    if "push_back" not in line:
        i += 1
        continue
    # gather until matching ); at paren depth, collecting string literals and skipping non-string parts
    chunk = line
    j = i
    # crude: keep appending until we see ');' that closes push_back
    depth = chunk.count("(") - chunk.count(")")
    while depth > 0 and j + 1 < len(lines):
        j += 1
        chunk += "\n" + lines[j]
        depth += lines[j].count("(") - lines[j].count(")")
        if j - i > 40:
            break
    # Extract only string literals and join them (approximation of runtime concat of literals)
    lits = [unesc(m.group(1)) for m in str_re.finditer(chunk)]
    # Filter to those with CJK or used as glue with CJK neighbors
    if any(cjk.search(x) for x in lits):
        # Join consecutive string literals only (ignore numeric to_string holes as %holes)
        # Represent dynamic holes as «N»
        # Better: join all CJK-bearing and short glue literals in order
        parts = []
        for lit in lits:
            if cjk.search(lit) or lit in ("α = ", " = ", " / ", "、", "。", "；", "，", "（", "）", "%", " 与 ", " 下"):
                parts.append(lit)
            elif len(lit) <= 12 and any(ch in lit for ch in " =：:;，；()[]"):
                parts.append(lit)
        joined = "".join(parts)
        if cjk.search(joined) and len(joined) >= 6:
            bullets.append((i + 1, joined))
    i = j + 1

# Dedup
seen = set()
uniq_bullets = []
for ln, s in bullets:
    if s in seen:
        continue
    seen.add(s)
    uniq_bullets.append((ln, s))

uncovered_bullets = []
for ln, s in uniq_bullets:
    ok, how = covered_bullet(s)
    if not ok:
        # also try if any exact is substring? no - that would false positive
        # try normalizing whitespace
        s2 = re.sub(r"\s+", "", s)
        ok2 = any(re.sub(r"\s+", "", k) == s2 for k in exact)
        if ok2:
            continue
        uncovered_bullets.append((ln, s))


# Titles/headers: broader scan
files = [
    "src/application/analysis_service.cpp",
    "src/application/graph_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/chart_pages.cpp",
    "src/application/output_builder.cpp",
]
chrome_re = re.compile(
    r"(?:\.title|\.name|page\.title|table\.title|plot\.title|"
    r"headers\.push_back|columns\.push_back|add_header|add_column|"
    r"x_axis_title|y_axis_title|series_name|legend|"
    r"make_table|make_plot|add_table|add_plot)\s*(?:=\s*|\()\s*\"((?:\\.|[^\"\\])*)\""
)
title_keys = set(exact.keys())  # any mapped ZH can match chrome too if in maps

unmapped_chrome = []
for rel in files:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for ln, line in enumerate(text.splitlines(), 1):
        for m in chrome_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s):
                continue
            base = s
            if base.endswith("（分面）"):
                base = base[: -len("（分面）")]
            for suf in (" 方法与参数", " 参数"):
                if base.endswith(suf):
                    base = base[: -len(suf)]
            if s in title_keys or base in title_keys:
                continue
            # prefix starts_with coverage for titles like "主效应图 - "
            if any(s.startswith(p) or base.startswith(p) for p in sw if len(p) >= 4):
                continue
            unmapped_chrome.append((rel, ln, s))

# Fixed diags: message ending with 。 and CJK, not covered
diag_files = list((root / "src/domain/statistics").glob("*.cpp"))
diag_kw = re.compile(r"(Diagnostic|push_diagnostic|add_diagnostic|diagnostics\.|message\s*=)")
unmapped_diags = []
for fp in diag_files:
    text = fp.read_text(encoding="utf-8", errors="replace")
    rel = str(fp.relative_to(root)).replace("\\", "/")
    for ln, line in enumerate(text.splitlines(), 1):
        if not diag_kw.search(line) and "diagnostic" not in line.lower():
            continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 4:
                continue
            if s in exact or any(s.startswith(p) for p in sw if len(p) >= 6):
                continue
            # skip mid-glue used only as find markers
            if s.startswith(("，", "；", "、", " ")) and len(s) < 20:
                continue
            unmapped_diags.append((rel, ln, s))

sd = set()
ud = []
for r, ln, s in unmapped_diags:
    if s in sd:
        continue
    sd.add(s)
    ud.append((r, ln, s))

# Cluster uncovered bullets by topic keywords
def topic(s: str) -> str:
    keys = [
        ("doe", ["DOE", "因子", "效应", "主效应", "立方", "Pareto", "中心点", "分辨度", "设计矩阵", "Desirability", "响应优化", "等值线", "失拟", "纯误差"]),
        ("msa", ["量具", "Gage", "偏倚", "线性", "ndc", "操作者", "零件", "稳定性图", "Type 1", "Cgk", "%Study", "EMP"]),
        ("spc", ["控制图", "超限", "σ", "Shewhart", "EWMA", "CUSUM", "区域图", "移动平均", "I-MR", "特殊原因", "Nelson"]),
        ("capability", ["能力", "Cpk", "Ppk", "双峰", "Hartigan", "混合", "过程合格", "Johnson", "非正态", "组间/组内"]),
        ("reliability", ["删失", "Weibull", "Kaplan", "生存", "CIF", "Fine-Gray", "保修", "失效", "对数正态", "指数模型"]),
        ("inference", ["P-Value", "P 值", "检验", "置信", "功效", "样本量", "正态", "异常值", "相关", "ANOVA", "Tukey", "比例", "泊松率", "等价"]),
        ("regression", ["回归", "R²", "VIF", "残差", "杠杆", "Cook", "Logistic", "逐步", "Hosmer"]),
        ("eda", ["密度", "hexbin", "小提琴", "条形", "因果", "四图", "NIST", "分面", "带宽"]),
        ("ml", ["K-Means", "CART", "Isolation", "聚类", "判别", "PCA"]),
        ("table", ["列联", "McNemar", "Cochran", "Fisher", "交叉表", "Kappa", "Kendall"]),
        ("forecast", ["预测", "ARIMA", "分解", "MASE", "MAPE", "季节", "ACF", "PACF", "Ljung"]),
    ]
    for name, kws in keys:
        if any(k in s for k in kws):
            return name
    return "other"


by_topic = Counter(topic(s) for _, s in uncovered_bullets)
by_file_diag = Counter(pathlib.Path(r).name for r, _, _ in ud)

out = []
out.append(
    f"exact={len(exact)} sw={len(sw)} ew={len(ew)} "
    f"reconstructed_bullets={len(uniq_bullets)} uncovered_bullets={len(uncovered_bullets)} "
    f"unmapped_chrome={len(unmapped_chrome)} unmapped_diags={len(ud)}"
)
out.append("\n=== UNCOVERED BULLET TOPICS ===")
for k, v in by_topic.most_common():
    out.append(f"  {v}\t{k}")

out.append("\n=== UNCOVERED BULLETS BY TOPIC ===")
grouped = defaultdict(list)
for ln, s in uncovered_bullets:
    grouped[topic(s)].append((ln, s))
for t, items in sorted(grouped.items(), key=lambda kv: -len(kv[1])):
    out.append(f"\n## {t} n={len(items)}")
    for ln, s in items[:25]:
        out.append(f"  L{ln}\t{s[:160]}")

out.append("\n=== UNMAPPED CHROME ===")
for r, ln, s in unmapped_chrome[:60]:
    out.append(f"  {r}:{ln}\t{s}")

out.append("\n=== UNMAPPED DIAGS BY FILE ===")
for fn, n in by_file_diag.most_common(20):
    out.append(f"  {n}\t{fn}")
out.append("\n=== UNMAPPED DIAGS ===")
for r, ln, s in ud[:80]:
    out.append(f"  {r}:{ln}\t{s[:160]}")

text = "\n".join(out)
(root / "scripts/_tmp_slice_verify.txt").write_text(text, encoding="utf-8")
print(text[:14000])
print("\n... total", len(text))
