# -*- coding: utf-8 -*-
"""Refine: find analysis_service titles/headers/diags not covered by exact maps or facet suffix."""
from __future__ import annotations

import pathlib
import re
from collections import Counter, defaultdict

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
cjk = re.compile(r"[\u4e00-\u9fff]")
str_re = re.compile(r'"((?:\\.|[^"\\])*)"')
pair_re = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}')


def unesc(s: str) -> str:
    return s.replace(r"\n", "\n").replace(r"\"", '"').replace(r"\\", "\\")


# Extract map keys by function region
def section(name: str) -> str:
    i = loc.find(name)
    if i < 0:
        return ""
    return loc[i : i + 80000]


title_keys = set()
header_keys = set()
plain_keys = set()
axis_keys = set()
series_keys = set()
param_token_keys = set()

for a, b in ((unesc(m.group(1)), unesc(m.group(2))) for m in pair_re.finditer(loc)):
    if not cjk.search(a):
        continue
    if b.startswith("table.") or b.startswith("plot.") or b.startswith("page.") or b.startswith("model."):
        title_keys.add(a)
    elif b.startswith("header.") or b.startswith("table.property") or b == "table.value":
        header_keys.add(a)
    elif b.startswith("plot.axis.") or b.startswith("plot.series") or "axis" in b:
        axis_keys.add(a)
    elif b.startswith("param.") or b.startswith("chrome.") or b.startswith("graph."):
        param_token_keys.add(a)
    elif b.startswith("interp.") or b.startswith("diag.") or b.startswith("status.") or b.startswith("msg."):
        plain_keys.add(a)
    else:
        # localize_known_plain_message ids often look like free ids
        if "." in b and not cjk.search(b):
            plain_keys.add(a)

# Also all first-of-pair in localize_known_plain_message block
plain_fn = loc.find("localize_known_plain_message")
plain_block = loc[plain_fn : plain_fn + 120000] if plain_fn >= 0 else ""
for m in pair_re.finditer(plain_block):
    a = unesc(m.group(1))
    if cjk.search(a):
        plain_keys.add(a)

facet_suffix = "（分面）"
method_suffixes = [" 方法与参数", " 参数"]

# All title-like assignments in analysis_service + graph_service + doe_pages + chart_pages
files = [
    "src/application/analysis_service.cpp",
    "src/application/graph_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/chart_pages.cpp",
    "src/application/output_builder.cpp",
]

title_asgn = re.compile(
    r"(?:\.title|\.name|page\.title|table\.title|plot\.title)\s*=\s*\"((?:\\.|[^\"\\])*)\""
)
header_asgn = re.compile(
    r"(?:headers\.push_back|columns\.push_back|add_header|add_column)\(\s*\"((?:\\.|[^\"\\])*)\""
)
axis_asgn = re.compile(
    r"(?:x_axis_title|y_axis_title|x_label|y_label)\s*=\s*\"((?:\\.|[^\"\\])*)\""
)
# diagnostics messages as string literals near push
diag_line = re.compile(
    r"(?:push_diagnostic|add_diagnostic|diagnostics\.push_back|message\s*=)\s*[^\n]{0,200}\"((?:\\.|[^\"\\])*)\""
)
# also Diagnostic{ ... "msg"
diag_ctor = re.compile(r'Diagnostic\s*\{[^}]*\"((?:\\.|[^\"\\])*)\"', re.S)

unmapped_titles = []
unmapped_headers = []
unmapped_axes = []
unmapped_diags = []

for rel in files:
    fp = root / rel
    text = fp.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    for i, line in enumerate(lines, 1):
        for m in title_asgn.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) and "（" not in s:
                continue
            if not cjk.search(s):
                continue
            base = s
            covered = False
            if s.endswith(facet_suffix):
                base = s[: -len(facet_suffix)]
            for suf in method_suffixes:
                if s.endswith(suf):
                    base = s[: -len(suf)]
            if base in title_keys or s in title_keys:
                covered = True
            # English-only base like Hexbin still needs map
            if not covered and base in title_keys:
                covered = True
            if not covered:
                unmapped_titles.append((rel, i, s, base))
        for m in header_asgn.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s):
                continue
            if s not in header_keys and s not in title_keys:
                unmapped_headers.append((rel, i, s))
        for m in axis_asgn.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s):
                continue
            if s not in axis_keys and s not in header_keys and s not in title_keys:
                unmapped_axes.append((rel, i, s))

# Diags: collect CJK string literals in lines containing diagnostic-ish APIs
diag_kw = re.compile(r"(push_diagnostic|add_diagnostic|Diagnostic\b|severity_message|status_message|limitation)")
for rel in files:
    fp = root / rel
    text = fp.read_text(encoding="utf-8", errors="replace")
    for i, line in enumerate(text.splitlines(), 1):
        if not diag_kw.search(line) and "diagnostics" not in line.lower():
            continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 4:
                continue
            # skip if exact in plain map
            if s in plain_keys:
                continue
            # prefix match for dynamic templates
            covered = False
            for pk in plain_keys:
                if len(pk) >= 6 and s.startswith(pk):
                    covered = True
                    break
            if covered:
                continue
            unmapped_diags.append((rel, i, s))

# Also interpretation static full sentences (not fragments)
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")
unmapped_interp = []
for i, line in enumerate(interp.splitlines(), 1):
    # full bullets typically push_back("....");
    if "push_back" not in line and "bullets" not in line and "limitations" not in line:
        # also << "..."
        if '<< "' not in line and '+= "' not in line and '= "' not in line:
            continue
    for m in str_re.finditer(line):
        s = unesc(m.group(1))
        if not cjk.search(s) or len(s) < 8:
            continue
        if s in plain_keys:
            continue
        # dynamic template prefixes often end with = or ：
        covered = False
        for pk in plain_keys:
            if len(pk) >= 8 and (s.startswith(pk) or pk.startswith(s[: min(12, len(s))])):
                covered = True
                break
        if covered:
            continue
        unmapped_interp.append((i, s))

out = root / "scripts/_tmp_unmapped_zh_refined.txt"
with out.open("w", encoding="utf-8") as f:
    f.write(f"title_keys={len(title_keys)} header_keys={len(header_keys)} plain_keys={len(plain_keys)} axis_keys={len(axis_keys)}\n")
    f.write(f"UNMAPPED_TITLES {len(unmapped_titles)}\n")
    for rel, i, s, base in unmapped_titles:
        f.write(f"title\t{rel}:{i}\tbase={base}\t{s}\n")
    f.write(f"UNMAPPED_HEADERS {len(unmapped_headers)}\n")
    for rel, i, s in unmapped_headers:
        f.write(f"header\t{rel}:{i}\t{s}\n")
    f.write(f"UNMAPPED_AXES {len(unmapped_axes)}\n")
    for rel, i, s in unmapped_axes:
        f.write(f"axis\t{rel}:{i}\t{s}\n")
    # unique diags densest
    diag_u = []
    seen = set()
    for rel, i, s in unmapped_diags:
        if s in seen:
            continue
        seen.add(s)
        diag_u.append((rel, i, s))
    f.write(f"UNMAPPED_DIAGS_UNIQUE {len(diag_u)}\n")
    for rel, i, s in diag_u[:120]:
        f.write(f"diag\t{rel}:{i}\t{s}\n")
    # cluster diags by prefix token
    clusters = Counter()
    for _, _, s in diag_u:
        # first 4-8 chars as rough cluster
        key = s[:8] if len(s) >= 8 else s
        clusters[key] += 1
    f.write("DIAG_PREFIX_CLUSTERS\n")
    for k, c in clusters.most_common(30):
        f.write(f"  {c}\t{k}\n")
    interp_u = []
    seen = set()
    for i, s in unmapped_interp:
        if s in seen:
            continue
        seen.add(s)
        interp_u.append((i, s))
    f.write(f"UNMAPPED_INTERP_UNIQUE {len(interp_u)}\n")
    for i, s in interp_u[:80]:
        f.write(f"interp\tinterpretation_service.cpp:{i}\t{s}\n")

print(f"wrote {out}")
print(f"titles={len(unmapped_titles)} headers={len(unmapped_headers)} axes={len(unmapped_axes)} diags_u={len(diag_u)} interp_u={len(interp_u)}")
print("title samples:")
for t in unmapped_titles[:25]:
    print(" ", t[0].split('/')[-1]+':'+str(t[1]), t[3])
print("diag prefix clusters:")
for k, c in clusters.most_common(15):
    print(f"  {c} {k}")
