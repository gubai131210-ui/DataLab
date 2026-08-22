# -*- coding: utf-8 -*-
"""Find unmapped table row-label / cell ZH (status_ids + plain + header path)."""
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


# All mapped ZH keys
mapped = {unesc(m.group(1)) for m in pair_re.finditer(loc) if cjk.search(unesc(m.group(1)))}
sw = sorted(
    {unesc(x) for x in re.findall(r'starts_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)},
    key=len,
    reverse=True,
)


def covered(s: str) -> bool:
    if s in mapped:
        return True
    for p in sw:
        if len(p) >= 4 and s.startswith(p):
            return True
    # dynamic stage row template
    if s.startswith("阶段 ") and "（N / 均值" in s:
        return False  # template needs starts_with
    return False


files = [
    "src/application/analysis_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/graph_service.cpp",
    "src/application/chart_pages.cpp",
    "src/application/output_builder.cpp",
]

# Capture first-column-ish string literals in row constructions
# Patterns: {"中文", ...} or push_back({"中文" or rows = { {"中文"
row_first = re.compile(
    r'\{\s*"((?:\\.|[^"\\])*)"\s*,'
)
# Also: push_back({\n "中文"
row_multiline = re.compile(
    r'(?:rows\.push_back|push_back)\(\s*\{\s*"((?:\\.|[^"\\])*)"',
    re.S,
)

items = []
for rel in files:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    for i, line in enumerate(lines, 1):
        # skip titles/headers already handled densely
        if ".title" in line or "headers" in line or "x_axis" in line or "y_axis" in line:
            continue
        for m in row_first.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s):
                continue
            if len(s) < 2:
                continue
            # skip long honesty bullets (interp path)
            if len(s) > 80:
                continue
            if covered(s):
                continue
            items.append((rel, i, s, "row_first"))
        # also string after = for decision cells on same line as push
        if "rows.push_back" in line or "process.rows" in line or ".rows =" in line or "hist.rows" in line:
            for m in str_re.finditer(line):
                s = unesc(m.group(1))
                if not cjk.search(s) or len(s) < 2:
                    continue
                if covered(s):
                    continue
                if len(s) > 100:
                    continue
                items.append((rel, i, s, "row_any"))

# Also multi-line rows.push_back({ "..." 
for rel in files:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for m in re.finditer(
        r'rows\.push_back\(\s*\{\s*"((?:\\.|[^"\\])*)"',
        text,
    ):
        s = unesc(m.group(1))
        if not cjk.search(s) or covered(s):
            continue
        # find line number
        pos = m.start()
        ln = text.count("\n", 0, pos) + 1
        items.append((rel, ln, s, "push_row"))

# Dedup by text, keep first
seen = set()
uniq = []
for rel, ln, s, kind in items:
    if s in seen:
        continue
    seen.add(s)
    uniq.append((rel, ln, s, kind))


def fam(s, rel):
    if any(k in s for k in ("阶段", "历史", "控制限", "σ", "I-MR", "子组", "超限", "规则")):
        return "spc_nested"
    if any(k in s for k in ("AD", "正态", "能力", "Cpk", "Ppk", "PPM", "规格", "Johnson", "假设", "变换", "低于", "高于")):
        return "capability_nested"
    if any(k in s for k in ("Gage", "偏倚", "量具", "ndc", "零件", "评估", "Type", "Cg")):
        return "msa_nested"
    if any(k in s for k in ("因子", "效应", "DOE", "中心", "分辨", "Desirability", "纯误差", "失拟")):
        return "doe_nested"
    if any(k in s for k in ("删失", "生存", "Weibull", "失效", "保修")):
        return "rel_nested"
    if "analysis_service" in rel:
        return "analysis_misc"
    return "other"


by_fam = Counter(fam(s, r) for r, _, s, _ in uniq)
out = []
out.append(f"mapped={len(mapped)} uniq_unmapped_rowish={len(uniq)}")
out.append("\nBY FAM")
for k, v in by_fam.most_common():
    out.append(f"  {v}\t{k}")

grouped = defaultdict(list)
for r, ln, s, k in uniq:
    grouped[fam(s, r)].append((r, ln, s, k))

for f, lst in sorted(grouped.items(), key=lambda kv: -len(kv[1])):
    out.append(f"\n## {f} n={len(lst)}")
    for r, ln, s, k in lst[:40]:
        out.append(f"  {k}\t{r}:{ln}\t{s}")

text = "\n".join(out)
(root / "scripts/_tmp_nested_cells_out.txt").write_text(text, encoding="utf-8")
print(text[:14000])
