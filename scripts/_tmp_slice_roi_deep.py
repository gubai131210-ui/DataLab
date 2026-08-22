# -*- coding: utf-8 -*-
"""Deeper ROI: acceptance diags + RSM multiline + remaining %1 templates."""
from __future__ import annotations

import pathlib
import re

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
out = root / "scripts" / "_tmp_slice_roi_deep_out.txt"
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
catalog = (root / "src/domain/report_text_catalog.cpp").read_text(encoding="utf-8")
cjk = re.compile(r"[\u4e00-\u9fff]")
str_re = re.compile(r'"((?:\\.|[^"\\])*)"')
pair_re = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}')


def unesc2(s: str) -> str:
    return s.replace(r"\n", "\n").replace(r"\"", '"').replace(r"\\", "\\")


exact: set[str] = set()
for m in pair_re.finditer(loc):
    a = unesc2(m.group(1))
    if cjk.search(a):
        exact.add(a)
for m in re.finditer(
    r'\{\s*"([^"]+)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}',
    catalog,
):
    zh = unesc2(m.group(2))
    if cjk.search(zh):
        exact.add(zh)

sw = [
    unesc2(x)
    for x in re.findall(r'starts_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)
]
sw = [x for x in sw if len(x) >= 3]


def covered(s: str) -> bool:
    if s in exact:
        return True
    for p in sw:
        if s.startswith(p):
            return True
    return False


# Scan quality_extensions for all ZH
qe = (root / "src/domain/statistics/quality_extensions.cpp").read_text(encoding="utf-8")
# Scan response_surface_design
rsd = (root / "src/domain/statistics/response_surface_design.cpp").read_text(encoding="utf-8")
# Scan rsm_analysis
rsm = (root / "src/domain/statistics/rsm_analysis.cpp").read_text(encoding="utf-8")
# doe_pages
doe = (root / "src/application/doe_pages.cpp").read_text(encoding="utf-8")

buf = []


def scan(name: str, text: str):
    buf.append(f"=== {name} ===")
    seen = set()
    for i, line in enumerate(text.splitlines(), 1):
        for m in str_re.finditer(line):
            s = unesc2(m.group(1))
            if not cjk.search(s) or len(s) < 4:
                continue
            if "vendor_oracle" in s or "Johnson" in s:
                continue
            if s in seen:
                continue
            seen.add(s)
            flag = "OK" if covered(s) else "UN"
            buf.append(f"  {flag} L{i}\t{s}")
    buf.append("")


scan("quality_extensions", qe)
scan("response_surface_design", rsd)

# Concat multiline string pairs in rsd for center etc
# Find all CJK strings even if previous line has error_diag
rsd_all = []
lines = rsd.splitlines()
for i, line in enumerate(lines):
    for m in str_re.finditer(line):
        s = unesc2(m.group(1))
        if cjk.search(s) and len(s) >= 4:
            rsd_all.append((i + 1, s))

# Also check doe_pages design chrome
scan("doe_pages (CJK sample)", doe)

# Find unmapped starts_with candidates in interpretation that look like %1 templates
# i.e. strings ending with = or ： that are mid-template
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")
buf.append("=== INTERP TEMPLATE FRAGMENTS (CJK, likely dynamic) ===")
seen = set()
for i, line in enumerate(interp.splitlines(), 1):
    if "push_back" not in line and "+=" not in line and "+" not in line:
        continue
    for m in str_re.finditer(line):
        s = unesc2(m.group(1))
        if not cjk.search(s):
            continue
        if len(s) < 4:
            continue
        # fragments that look like template glue
        if s.endswith("=") or s.endswith("= ") or " = " in s[:8] or s.startswith("，") or s.startswith("；") or s.startswith("（"):
            key = s
            if key in seen:
                continue
            seen.add(key)
            # check if any starts_with/ends_with covers via being part of known template
            # Heuristic: covered if exact match of full templates elsewhere
            flag = "OK" if covered(s) else "UN"
            buf.append(f"  {flag} L{i}\t{s}")

# Check catalog for missing suggested ids near rsd/oc
buf.append("")
buf.append("=== CATALOG RSD/OC KEYS ===")
for m in re.finditer(r'"(interp\.(?:rsd|ccd|bbd|oc)[^"]*)"', catalog):
    buf.append(f"  {m.group(1)}")

out.write_text("\n".join(buf), encoding="utf-8")
print(f"wrote {out}")
un_qe = [l for l in buf if l.startswith("  UN") and "quality" in "\n".join(buf[: buf.index(l)] if False else [])]
# count UN
print("UN count:", sum(1 for l in buf if l.startswith("  UN")))
