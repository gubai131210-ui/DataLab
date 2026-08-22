# -*- coding: utf-8 -*-
"""Fresh ROI audit: densest remaining unmapped ZH on en-US report path."""
from __future__ import annotations

import pathlib
import re
from collections import Counter, defaultdict

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
out = root / "scripts" / "_tmp_slice_roi_audit_out.txt"
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
catalog = (root / "src/domain/report_text_catalog.cpp").read_text(encoding="utf-8")
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")
cjk = re.compile(r"[\u4e00-\u9fff]")
str_re = re.compile(r'"((?:\\.|[^"\\])*)"')
pair_re = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}')


def unesc(s: str) -> str:
    return bytes(s, "utf-8").decode("unicode_escape") if "\\" in s else s


def unesc2(s: str) -> str:
    return s.replace(r"\n", "\n").replace(r"\"", '"').replace(r"\\", "\\")


exact: set[str] = set()
for m in pair_re.finditer(loc):
    a = unesc2(m.group(1))
    if cjk.search(a):
        exact.add(a)

# catalog entries: {"id", "zh", "en"}
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
ew = [
    unesc2(x)
    for x in re.findall(r'ends_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)
]


def covered(s: str) -> bool:
    if s in exact:
        return True
    for p in sw:
        if s.startswith(p):
            return True
    for p in ew:
        if len(p) >= 8 and s.endswith(p):
            return True
    return False


skip_terms = ("Johnson", "vendor_oracle", "golden/尾部", "PDF/A", "PDF/UA")


def should_skip(s: str) -> bool:
    return any(t in s for t in skip_terms)


diag_files = list((root / "src/domain/statistics").glob("*.cpp"))
diag_files += [
    root / "src/application/analysis_service.cpp",
    root / "src/application/doe_pages.cpp",
    root / "src/application/chart_pages.cpp",
    root / "src/application/graph_service.cpp",
]
diag_kw = re.compile(
    r"(push_diagnostic|add_diagnostic|Diagnostic\b|error_diag|warning_diag|"
    r"info_diag|diagnostics\.push|append_diagnostic)"
)

fixed: list[tuple[str, int, str]] = []
for fp in diag_files:
    rel = str(fp.relative_to(root)).replace("\\", "/")
    text = fp.read_text(encoding="utf-8", errors="replace")
    for i, line in enumerate(text.splitlines(), 1):
        if not diag_kw.search(line) and "diagnostics" not in line.lower():
            continue
        for m in str_re.finditer(line):
            s = unesc2(m.group(1))
            if not cjk.search(s) or len(s) < 4:
                continue
            if should_skip(s):
                continue
            fixed.append((rel, i, s))

seen: set[str] = set()
uniq: list[tuple[str, int, str]] = []
for r, i, s in fixed:
    if s in seen:
        continue
    seen.add(s)
    uniq.append((r, i, s))
unc = [(r, i, s) for r, i, s in uniq if not covered(s)]

# Cluster families
def family(rel: str, s: str) -> str:
    if "response_surface" in rel or "rsm_" in rel:
        return "rsm_design"
    if "doe_factorial" in rel or "plackett" in rel:
        return "doe"
    if "control_charts" in rel or "multivariate_control" in rel:
        return "spc"
    if "process_capability" in rel or "quality_extensions" in rel:
        return "capability"
    if "analysis_service" in rel:
        # prefixes like "单样本 t："
        if s.endswith("：") or s.endswith(": ") or s.endswith(":"):
            return "append_prefix"
        if "跳过" in s or "complete-case" in s or "缺失" in s:
            return "skip_complete"
        return "analysis_service"
    if "chart_pages" in rel or "graph_service" in rel:
        return "chart_graph"
    return "other"


by_fam = defaultdict(list)
for r, i, s in unc:
    by_fam[family(r, s)].append((r, i, s))

# Interp dynamic starts
lines = interp.splitlines()
starts: list[tuple[int, str]] = []
for i, line in enumerate(lines, 1):
    if "push_back" not in line and "+=" not in line and "bullets" not in line:
        if '= "' not in line and '("' not in line:
            continue
    for m in str_re.finditer(line):
        s = unesc2(m.group(1))
        if not cjk.search(s) or len(s) < 6:
            continue
        if s[0] in "，；）、,;)" or s.startswith("。") or s.startswith("、"):
            continue
        if s.startswith(" ") and not any(
            s.lstrip().startswith(p) for p in ("下", "个", "正态")
        ):
            continue
        if should_skip(s):
            continue
        starts.append((i, s))

seen2: set[str] = set()
uniq_starts: list[tuple[int, str]] = []
for i, s in starts:
    if s in seen2:
        continue
    seen2.add(s)
    uniq_starts.append((i, s))
unc_starts = [(i, s) for i, s in uniq_starts if not covered(s)]

# Acceptance OC specific
oc_hits = []
for i, line in enumerate(lines, 1):
    if "OC" in line or "acceptance" in line.lower() or "AQL" in line or "RQL" in line:
        for m in str_re.finditer(line):
            s = unesc2(m.group(1))
            if cjk.search(s):
                oc_hits.append((i, s, covered(s)))

# RSM design file all ZH diags
rsd = root / "src/domain/statistics/response_surface_design.cpp"
rsd_items = []
if rsd.exists():
    text = rsd.read_text(encoding="utf-8")
    for i, line in enumerate(text.splitlines(), 1):
        for m in str_re.finditer(line):
            s = unesc2(m.group(1))
            if cjk.search(s) and len(s) >= 4:
                rsd_items.append((i, s, covered(s)))

# analysis_service append prefixes (short ending with ：)
as_text = (root / "src/application/analysis_service.cpp").read_text(encoding="utf-8")
prefix_items = []
for i, line in enumerate(as_text.splitlines(), 1):
    if "append_diagnostic" not in line and "diagnostics" not in line.lower():
        continue
    for m in str_re.finditer(line):
        s = unesc2(m.group(1))
        if cjk.search(s) and (s.endswith("：") or s.endswith(": ") or s.endswith(":")):
            prefix_items.append((i, s, covered(s)))

buf: list[str] = []
buf.append(f"exact={len(exact)} sw={len(sw)} ew={len(ew)}")
buf.append(f"fixed_diag_unique={len(uniq)} unmapped={len(unc)}")
buf.append(f"interp_starts_unique={len(uniq_starts)} uncovered={len(unc_starts)}")
buf.append("")
buf.append("=== DIAG BY FAMILY (unmapped) ===")
for fam, items in sorted(by_fam.items(), key=lambda x: -len(x[1])):
    buf.append(f"## {fam} n={len(items)}")
    for r, i, s in items[:40]:
        buf.append(f"  {r}:{i}\t{s}")
    buf.append("")

buf.append("=== RSM DESIGN ALL ZH ===")
for i, s, cov in rsd_items:
    buf.append(f"  {'OK' if cov else 'UN'} L{i}\t{s}")
buf.append("")

buf.append("=== APPEND PREFIXES ===")
for i, s, cov in prefix_items:
    buf.append(f"  {'OK' if cov else 'UN'} L{i}\t{s}")
buf.append("")

buf.append("=== ACCEPTANCE/OC HITS ===")
for i, s, cov in oc_hits:
    buf.append(f"  {'OK' if cov else 'UN'} L{i}\t{s}")
buf.append("")

buf.append("=== UNCOVERED INTERP STARTS (first 60) ===")
for i, s in unc_starts[:60]:
    buf.append(f"  L{i}\t{s}")

out.write_text("\n".join(buf), encoding="utf-8")
print(f"wrote {out}")
print(f"unmapped_diags={len(unc)} uncovered_interp={len(unc_starts)}")
for fam, items in sorted(by_fam.items(), key=lambda x: -len(x[1])):
    print(f"  {fam}: {len(items)}")
