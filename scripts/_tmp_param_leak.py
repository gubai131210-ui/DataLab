# -*- coding: utf-8 -*-
"""Find ZH still leaking in parameter_summary after token replace; rank chrome gaps."""
from __future__ import annotations

import pathlib
import re
from collections import Counter

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
cjk = re.compile(r"[\u4e00-\u9fff]")
pair_re = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}')
str_re = re.compile(r'"((?:\\.|[^"\\])*)"')


def unesc(s: str) -> str:
    return s.replace(r"\n", "\n").replace(r"\"", '"').replace(r"\\", "\\")


# parameter summary tokens
m = re.search(r"localize_parameter_summary.*?tokens\[\] = \{(.*?)\};", loc, re.S)
tokens = [(unesc(a), b) for a, b in pair_re.findall(m.group(1))]
# sort longer first like C++ static order (already longer-first in source)
tokens_sorted = sorted(tokens, key=lambda x: len(x[0]), reverse=True)


def apply_tokens(s: str) -> str:
    for zh, tid in tokens:  # keep source order
        s = s.replace(zh, f"⟦{tid}⟧")
    return s


# title/header/axis exact maps
exact = {}
for a, b in pair_re.findall(loc):
    a, b = unesc(a), unesc(b)
    if cjk.search(a) or "（" in a:
        exact[a] = b


def covered_chrome(s: str) -> bool:
    if s in exact:
        return True
    if s.endswith("（分面）") and s[: -len("（分面）")] in exact:
        return True
    for suf in (" 方法与参数", " 参数"):
        if s.endswith(suf) and s[: -len(suf)] in exact:
            return True
    return False


out = []

# --- parameter_summary leaks ---
leaks = []
for rel in [
    "src/application/analysis_service.cpp",
    "src/application/graph_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/chart_pages.cpp",
]:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        if "parameter_summary" in lines[i] and "=" in lines[i]:
            chunk = lines[i]
            j = i
            while True:
                odd_quotes = chunk.count('"') % 2 == 1
                done = chunk.rstrip().endswith(";") and not odd_quotes
                if done or j + 1 >= len(lines) or j - i > 20:
                    break
                j += 1
                chunk += "\n" + lines[j]
            strs = [unesc(x) for x in str_re.findall(chunk)]
            joined = "".join(strs)
            if cjk.search(joined):
                after = apply_tokens(joined)
                if cjk.search(after):
                    # extract remaining CJK spans
                    spans = re.findall(r"[\u4e00-\u9fff「」σ̄_i（）/%A-Za-z0-9 =\.·\-≈]+", after)
                    cjk_spans = [x.strip() for x in spans if cjk.search(x)]
                    leaks.append((rel, i + 1, joined, after, cjk_spans))
            i = j + 1
        else:
            i += 1

out.append(f"param_summary_leaks={len(leaks)}")
# cluster remaining span tokens
span_c = Counter()
for *_, spans in leaks:
    for sp in spans:
        # normalize numbers out
        key = re.sub(r"\d+(\.\d+)?", "N", sp)
        if len(key) >= 2:
            span_c[key] += 1

out.append("\n=== TOP LEAKING SPANS AFTER TOKEN REPLACE ===")
for k, n in span_c.most_common(40):
    out.append(f"  {n}\t{k}")

out.append("\n=== LEAK EXAMPLES ===")
for rel, ln, before, after, spans in leaks[:35]:
    out.append(f"\n{rel}:{ln}")
    out.append(f"  AFTER: {after[:220]}")
    out.append(f"  SPANS: {spans[:8]}")

# --- table cell ZH not in exact (headers + cells via push patterns) ---
cell_items = []
for rel in [
    "src/application/analysis_service.cpp",
    "src/application/graph_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/chart_pages.cpp",
    "src/application/output_builder.cpp",
]:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for ln, line in enumerate(text.splitlines(), 1):
        if "rows.push_back" not in line and "headers" not in line and "summary.rows" not in line:
            # also inline {"中文", ...}
            if "{" not in line or '"' not in line:
                continue
        for m2 in str_re.finditer(line):
            s = unesc(m2.group(1))
            if not cjk.search(s):
                continue
            if covered_chrome(s):
                continue
            # skip long diagnostics / sentences (handled elsewhere)
            if len(s) > 60 and s.endswith("。"):
                continue
            if s.startswith(("分析", "跳过", "无法", "请")) and s.endswith("。"):
                continue
            cell_items.append((rel, ln, s))

# dedup
seen = set()
cells = []
for r, ln, s in cell_items:
    if s in seen:
        continue
    seen.add(s)
    cells.append((r, ln, s))

# family for cells
def cfam(s: str) -> str:
    keys = [
        ("eda4", ["位置是否", "相邻观测", "分布形态", "正态分位", "检查假设", "四图"]),
        ("attrib", ["不合格", "检验数", "缺陷", "二项", "泊松", "Average", "子组"]),
        ("cause", ["原因", "类别", "效应"]),
        ("pareto", ["柏拉图", "累计", "Other"]),
        ("dist_id", ["判定", "截断", "分布", "位置/形状", "尺度"]),
        ("optim", ["目标", "权重", "预测", "排序", "最佳", "Desirab"]),
        ("corr", ["偏相关", "有效变量"]),
        ("spc", ["超限", "阶段", "窗宽", "Sigma"]),
    ]
    for name, kws in keys:
        if any(k in s for k in kws):
            return name
    return "misc"

cc = Counter(cfam(s) for *_, s in cells)
out.append(f"\n\nunmapped_short_chrome_ish={len(cells)}")
out.append("cell fams: " + ", ".join(f"{k}:{v}" for k, v in cc.most_common()))
for fam, n in cc.most_common(10):
    out.append(f"\n## chrome/{fam} n={n}")
    for r, ln, s in [x for x in cells if cfam(x[2]) == fam][:25]:
        out.append(f"  {r}:{ln}\t{s}")

# Propose slice: attribute SPC param leftover spans + capability binomial/poisson + OC + related
# Collect concrete items from leaks that still have distinctive ZH
attrib_items = []
for rel, ln, before, after, spans in leaks:
    if any(
        k in before
        for k in (
            "二项",
            "泊松",
            "不合格",
            "缺陷",
            "OC",
            "Other 阈值",
            "单点超出",
            "P 图",
            "np̄",
            "p̄",
            "c̄",
            "ū",
        )
    ):
        attrib_items.append((rel, ln, before, spans))

out.append(f"\n\n=== ATTRIB-RELATED PARAM LEAKS n={len(attrib_items)} ===")
for rel, ln, before, spans in attrib_items:
    out.append(f"  {rel}:{ln}\tspans={spans}\tBEFORE={before[:140]}")

# EDA4 cell pack
eda = [x for x in cells if cfam(x[2]) == "eda4" or x[2] in ("图", "检查假设")]
out.append(f"\n=== EDA4 PACK n={len(eda)} ===")
for r, ln, s in eda:
    out.append(f"  {r}:{ln}\t{s}")

# Cause-effect pack
cause = [x for x in cells if cfam(x[2]) == "cause"]
out.append(f"\n=== CAUSE PACK n={len(cause)} ===")
for r, ln, s in cause[:20]:
    out.append(f"  {r}:{ln}\t{s}")

path = root / "scripts/_tmp_param_leak_out.txt"
path.write_text("\n".join(out), encoding="utf-8")
print(f"wrote {path} leaks={len(leaks)} cells={len(cells)} attrib_leaks={len(attrib_items)} eda4={len(eda)}")
