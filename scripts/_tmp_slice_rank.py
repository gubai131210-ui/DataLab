# -*- coding: utf-8 -*-
"""Rank coherent slices for next bilingual ROI pick."""
from __future__ import annotations

import pathlib
import re

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
cjk = re.compile(r"[\u4e00-\u9fff]")
pair_re = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}')
str_re = re.compile(r'"((?:\\.|[^"\\])*)"')


def unesc(s: str) -> str:
    return s.replace(r"\n", "\n").replace(r"\"", '"').replace(r"\\", "\\")


m = re.search(r"localize_parameter_summary.*?tokens\[\] = \{(.*?)\};", loc, re.S)
tokens = [(unesc(a), b) for a, b in pair_re.findall(m.group(1))]

# caption tokens (NOT applied to parameter_summary today)
cap_m = re.search(r"localize_graph_caption_tokens.*?tokens\[\] = \{(.*?)\};", loc, re.S)
cap_tokens = [(unesc(a), b) for a, b in pair_re.findall(cap_m.group(1))]


def apply_param(s: str) -> str:
    for zh, tid in tokens:
        s = s.replace(zh, f"⟦{tid}⟧")
    return s


def apply_param_and_caption(s: str) -> str:
    s = apply_param(s)
    for zh, tid in cap_tokens:
        s = s.replace(zh, f"⟦{tid}⟧")
    # also common leftovers we might add
    return s


# Collect all parameter_summary joined strings
summaries = []
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
                odd = chunk.count('"') % 2 == 1
                done = chunk.rstrip().endswith(";") and not odd
                if done or j + 1 >= len(lines) or j - i > 20:
                    break
                j += 1
                chunk += "\n" + lines[j]
            joined = "".join(unesc(x) for x in str_re.findall(chunk))
            if cjk.search(joined):
                summaries.append((rel, i + 1, joined))
            i = j + 1
        else:
            i += 1

# Slice A: graph facet/N tokens — sites still leaking after param tokens, fixed if caption tokens also applied
graph_sites = []
for rel, ln, s in summaries:
    after = apply_param(s)
    after2 = apply_param_and_caption(s)
    if cjk.search(after) and (
        "显示 N" in s
        or "分析 N" in s
        or "分面" in s
        or "面板" in s
        or "分组 =" in s
        or rel.endswith("graph_service.cpp")
    ):
        still = cjk.search(after2) is not None
        graph_sites.append((rel, ln, s, after, after2, still))

# Slice B: inference classic
inf_keys = (
    "假设均值",
    "备择",
    "合并方差",
    "试验",
    "目标比例",
    "假设比例",
    "假设发生率",
    "比较 =",
    "行:",
    "列:",
    "置信",
    "缺失值 N",
    "偏相关",
    "总体均值",
    "Known σ",
    "Welch",
    "pooled",
)
inf_sites = []
for rel, ln, s in summaries:
    after = apply_param(s)
    if cjk.search(after) and any(k in s for k in inf_keys):
        inf_sites.append((rel, ln, s, after))

# Unique token candidates for inference slice
inf_token_cands = [
    "方法: ",
    "备择: ",
    "备择：",
    "假设均值 = ",
    "缺失值 N* = ",
    "置信",  # careful - too short?
    "合并方差",
    "试验 = ",
    "目标比例 = ",
    "假设比例 = ",
    "假设发生率 = ",
    "比较 = ",
    "行: ",
    "列: ",
    "比例 = ",
    "偏相关 = 是",
    "有效",  # before 变量数 - broken by 变量数 token
]

out = []
out.append(f"summaries_with_cjk={len(summaries)}")
out.append(f"graph_slice_sites={len(graph_sites)} still_after_caption={sum(1 for *_,s in graph_sites if s)}")
out.append(f"inference_slice_sites={len(inf_sites)}")

out.append("\n=== GRAPH SITES (param leak; caption would clear most) ===")
for rel, ln, s, a1, a2, still in graph_sites:
    out.append(f"  {rel}:{ln} still_after_cap={still}")
    out.append(f"    AFTER_PARAM: {a1[:180]}")
    if still:
        out.append(f"    AFTER_CAP:   {a2[:180]}")

out.append("\n=== INFERENCE SITES ===")
for rel, ln, s, after in inf_sites:
    out.append(f"  {rel}:{ln}")
    out.append(f"    AFTER: {after[:200]}")
    out.append(f"    RAW:   {s[:200]}")

# Extract exact Chinese literals around inference summaries for the map table
# Read the actual source lines for key inference summaries
keys_lines = [
    875, 1791, 1958, 2095, 2226, 2308, 2420, 2714, 2754, 2851,
    3347, 3775, 3904, 3991, 4131, 4364, 4537,
]
text = (root / "src/application/analysis_service.cpp").read_text(encoding="utf-8")
lines = text.splitlines()
out.append("\n=== INFERENCE RAW SNIPPETS ===")
for ln in keys_lines:
    # print nearby parameter_summary
    for i in range(max(0, ln - 3), min(len(lines), ln + 8)):
        if "parameter_summary" in lines[i] or (i >= ln - 1 and i <= ln + 5 and '"' in lines[i] and cjk.search(lines[i])):
            out.append(f"  {i+1}:{lines[i].rstrip()[:220]}")

path = root / "scripts/_tmp_slice_rank_out.txt"
path.write_text("\n".join(out), encoding="utf-8")
print(f"wrote {path} graph={len(graph_sites)} inf={len(inf_sites)}")
