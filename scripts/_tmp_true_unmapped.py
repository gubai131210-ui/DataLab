# -*- coding: utf-8 -*-
"""Find truly uncovered dynamic bullet STARTS and fixed diags on en-US path."""
from __future__ import annotations

import pathlib
import re

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")
cjk = re.compile(r"[\u4e00-\u9fff]")
str_re = re.compile(r'"((?:\\.|[^"\\])*)"')
pair_re = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}')


def unesc(s: str) -> str:
    return s.replace(r"\n", "\n").replace(r"\"", '"').replace(r"\\", "\\")


exact = set()
for m in pair_re.finditer(loc):
    a = unesc(m.group(1))
    if cjk.search(a):
        exact.add(a)

sw = [unesc(x) for x in re.findall(
    r'starts_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc
)]
sw = [x for x in sw if len(x) >= 4]


def start_covered(s: str) -> bool:
    if s in exact:
        return True
    for p in sw:
        if s.startswith(p) or p.startswith(s):
            return True
    return False


# Heuristic: bullet construction often uses push_back(xxx) where xxx is built
# with leading Chinese literal that is NOT a mid connector starting with ，；）
# Collect string literals that LOOK like bullet starts (CJK early, not mid glue)

lines = interp.splitlines()
starts = []
for i, line in enumerate(lines, 1):
    # look for first literal in an expression that builds a bullet
    if "push_back" not in line and "+=" not in line and "<<" not in line and "std::string" not in line and "bullets" not in line:
        # also capture: const auto x = "中文..."
        if '= "' not in line and "(\"" not in line:
            continue
    for m in str_re.finditer(line):
        s = unesc(m.group(1))
        if not cjk.search(s) or len(s) < 6:
            continue
        # skip mid-glue fragments
        if s[0] in "，；）、,;)" or s.startswith("；") or s.startswith("，") or s.startswith("）"):
            continue
        if s.startswith("。") or s.startswith("、") or s.startswith(" "):
            # leading space often mid glue too
            if s.startswith(" ") and not any(s.lstrip().startswith(p) for p in ("下", "个", "正态")):
                continue
        starts.append((i, s, line.strip()[:140]))

# unique by string
seen = set()
uniq = []
for i, s, ctx in starts:
    if s in seen:
        continue
    seen.add(s)
    uniq.append((i, s, ctx))

uncovered = [(i, s, ctx) for i, s, ctx in uniq if not start_covered(s)]
covered_n = len(uniq) - len(uncovered)

# Also scan for multi-line builds: first literal after bullets.push_back(
# Better: find patterns like push_back( and look nearby
# Also: `out.push_back("` or `bullets.push_back("`

direct_push = []
for i, line in enumerate(lines, 1):
    if "push_back" not in line:
        continue
    for m in str_re.finditer(line):
        s = unesc(m.group(1))
        if not cjk.search(s) or len(s) < 8:
            continue
        if s[0] in "，；）。、,;)":
            continue
        direct_push.append((i, s))

seen2 = set()
dp_u = []
for i, s in direct_push:
    if s in seen2:
        continue
    seen2.add(s)
    dp_u.append((i, s))
dp_unc = [(i, s) for i, s in dp_u if not start_covered(s)]

# Domain fixed diags: complete sentences (end with 。) with CJK
diag_files = list((root / "src/domain/statistics").glob("*.cpp"))
diag_files += [
    root / "src/application/analysis_service.cpp",
    root / "src/application/doe_pages.cpp",
    root / "src/application/chart_pages.cpp",
    root / "src/application/graph_service.cpp",
]
diag_kw = re.compile(r"(push_diagnostic|add_diagnostic|Diagnostic\b)")
fixed_diags = []
for fp in diag_files:
    rel = str(fp.relative_to(root)).replace("\\", "/")
    text = fp.read_text(encoding="utf-8", errors="replace")
    for i, line in enumerate(text.splitlines(), 1):
        if not diag_kw.search(line) and "diagnostics" not in line.lower():
            continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 6:
                continue
            if start_covered(s) or s in exact:
                continue
            fixed_diags.append((rel, i, s))

seen3 = set()
fd_u = []
for rel, i, s in fixed_diags:
    if s in seen3:
        continue
    seen3.add(s)
    fd_u.append((rel, i, s))

# Cluster fixed diags by file family
from collections import Counter, defaultdict
by_file = Counter(rel.split("/")[-1] for rel, _, _ in fd_u)
by_prefix = defaultdict(list)
for rel, i, s in fd_u:
    # thematic
    key = "other"
    if "control_charts" in rel:
        key = "spc_control_charts"
    elif "process_capability" in rel or "quality_extensions" in rel:
        key = "capability"
    elif "response_surface" in rel or "doe_factorial" in rel:
        key = "doe_rsm"
    elif "analysis_service" in rel:
        key = "analysis_service_prefix"
    elif "chart_pages" in rel or "graph" in rel:
        key = "chart_graph"
    else:
        key = rel.split("/")[-1]
    by_prefix[key].append((rel, i, s))

out = root / "scripts/_tmp_true_unmapped.txt"
with out.open("w", encoding="utf-8") as f:
    f.write(f"sw_prefixes={len(sw)} exact={len(exact)}\n")
    f.write(f"interp_start_candidates={len(uniq)} uncovered_starts={len(uncovered)} covered={covered_n}\n")
    f.write(f"direct_push_unique={len(dp_u)} uncovered_direct={len(dp_unc)}\n")
    f.write(f"fixed_diags_unique={len(fd_u)}\n")
    f.write("\n=== DIAG BY FAMILY ===\n")
    for k, items in sorted(by_prefix.items(), key=lambda kv: -len(kv[1])):
        f.write(f"\n## {k} n={len(items)}\n")
        for rel, i, s in items:
            f.write(f"  {rel}:{i}\t{s}\n")
    f.write("\n=== UNCOVERED INTERP STARTS ===\n")
    for i, s, ctx in uncovered:
        f.write(f"L{i}\t{s}\n")
    f.write("\n=== UNCOVERED DIRECT PUSH ===\n")
    for i, s in dp_unc:
        f.write(f"L{i}\t{s}\n")

print(f"wrote {out}")
print(f"uncovered_starts={len(uncovered)} uncovered_direct={len(dp_unc)} fixed_diags={len(fd_u)}")
print("diag families:", {k: len(v) for k, v in sorted(by_prefix.items(), key=lambda kv: -len(kv[1]))})
print("--- uncovered starts sample ---")
for i, s, _ in uncovered[:40]:
    print(f"  L{i}: {s[:90]}")
