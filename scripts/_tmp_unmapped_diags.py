# -*- coding: utf-8 -*-
"""Find densest unmapped skip/complete-case diagnostics vs exact_ids."""
from __future__ import annotations

import pathlib
import re

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
svc = (root / "src/application/analysis_service.cpp").read_text(encoding="utf-8")
cjk = re.compile(r"[\u4e00-\u9fff]")
str_re = re.compile(r'"((?:\\.|[^"\\])*)"')
pair_re = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}')


def unesc(s: str) -> str:
    return s.replace(r"\n", "\n").replace(r"\"", '"').replace(r"\\", "\\")


# exact_ids from localize_known_plain_message
fn = loc.find("localize_known_plain_message")
block = loc[fn : fn + 150000]
plain = set()
for m in pair_re.finditer(block):
    a = unesc(m.group(1))
    if cjk.search(a):
        plain.add(a)

# Collect diagnostic message string literals near diagnostics.push_back in analysis_service
lines = svc.splitlines()
msgs = []
for i, line in enumerate(lines, 1):
    if "diagnostics.push_back" not in line and "DiagnosticMessage" not in line:
        # multi-line: look at window
        continue
    # gather nearby strings on this and next 3 lines
    window = "\n".join(lines[i - 1 : i + 4])
    for m in str_re.finditer(window):
        s = unesc(m.group(1))
        if not cjk.search(s) or len(s) < 6:
            continue
        if s in ("missing_values", "warning", "error", "info"):
            continue
        msgs.append((i, s))

# Also catch push_back with string on following lines after severity/code
for i, line in enumerate(lines, 1):
    if "diagnostics.push_back" not in line:
        continue
    window = "\n".join(lines[i - 1 : i + 6])
    for m in str_re.finditer(window):
        s = unesc(m.group(1))
        if not cjk.search(s) or len(s) < 6:
            continue
        msgs.append((i, s))

# unique
seen = set()
unique = []
for i, s in msgs:
    if s in seen:
        continue
    seen.add(s)
    unique.append((i, s))

# classify mapped
unmapped = []
for i, s in unique:
    if s in plain:
        continue
    # prefix covered by template?
    covered = False
    for pk in plain:
        if len(pk) >= 8 and s.startswith(pk):
            covered = True
            break
    # common dynamic prefixes handled in localize_interpretation?
    dyn_prefixes = [
        "跳过 ",
        "已跳过 ",
        "描述统计跳过 ",
        "相关分析 complete-case 跳过 ",
        "ANOVA 跳过 ",
        "Box-Cox 跳过 ",
        "Mann–Whitney 跳过 ",
        "Wilcoxon 跳过 ",
        "符号检验跳过 ",
        "Mood 跳过 ",
        "Kruskal–Wallis 跳过 ",
        "Bias/Linearity 跳过 ",
        "Type 1 Gage 跳过 ",
        "ANOM 跳过 ",
        "泊松拟合优度跳过 ",
        "游程检验跳过两端缺失或非法单元格 N* = ",
        "运行图跳过两端缺失或非法单元格 N* = ",
        "Fisher 精确检验按 complete-case 跳过 ",
        "McNemar 按 complete-case 跳过 ",
        "Cochran Q 按 complete-case 跳过 ",
        "Friedman 按 complete-case 跳过 ",
        "因果图跳过 ",
    ]
    for p in dyn_prefixes:
        if s.startswith(p):
            covered = True
            break
    if covered:
        continue
    unmapped.append((i, s))

out = root / "scripts/_tmp_unmapped_diags.txt"
with out.open("w", encoding="utf-8") as f:
    f.write(f"diag_unique={len(unique)} unmapped={len(unmapped)} plain={len(plain)}\n")
    for i, s in unmapped:
        f.write(f"analysis_service.cpp:{i}\t{s}\n")
print(f"unique={len(unique)} unmapped={len(unmapped)} -> {out}")
