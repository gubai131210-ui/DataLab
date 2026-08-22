# -*- coding: utf-8 -*-
"""Pick densest remaining bilingual ROI slice after just-landed waves."""
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


exact: set[str] = set()
id_for: dict[str, str] = {}
for m in pair_re.finditer(loc):
    a, b = unesc(m.group(1)), unesc(m.group(2))
    if cjk.search(a):
        exact.add(a)
        id_for[a] = b

# starts_with prefixes used in localize_interpretation / plain message
sw = sorted(
    {
        unesc(x)
        for x in re.findall(r'starts_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)
        if cjk.search(unesc(x)) or True
    },
    key=len,
    reverse=True,
)
sw = [x for x in sw if len(x) >= 2]


def covered_exact_or_sw(s: str) -> bool:
    if s in exact:
        return True
    for p in sw:
        if s.startswith(p):
            return True
    return False


# --- Collect ALL CJK string literals in interpretation_service that look report-facing ---
# Heuristic: line has push_back / += / append / format / QString / bullet / advice / note
lines = interp.splitlines()
candidates: list[tuple[int, str, str]] = []  # line, text, kind
for i, line in enumerate(lines, 1):
    kind = None
    if "push_back" in line:
        kind = "push"
    elif "+=" in line or "append(" in line or " = " in line and ("msg" in line or "bullet" in line or "text" in line or "out" in line):
        kind = "build"
    elif "arg(" in line or "%1" in line:
        kind = "tmpl"
    else:
        # still capture long CJK strings that are likely fragments
        if cjk.search(line) and ('"' in line):
            kind = "lit"
        else:
            continue
    for m in str_re.finditer(line):
        s = unesc(m.group(1))
        if not cjk.search(s):
            continue
        if len(s) < 4:
            continue
        # skip ids / codes
        if re.match(r"^[a-z0-9_./-]+$", s):
            continue
        candidates.append((i, s, kind))

# Dedup preserving order
seen = set()
uniq = []
for i, s, k in candidates:
    key = (s, k if k != "lit" else "lit")
    if s in seen:
        continue
    # Prefer push over lit
    seen.add(s)
    uniq.append((i, s, k))

uncovered = [(i, s, k) for i, s, k in uniq if not covered_exact_or_sw(s)]

# Cluster by nearby section comments / function names
# Find enclosing function by scanning backwards for ^[a-zA-Z].*\(.*\) \{? or AnalysisType
func_at = {}
current = "unknown"
for i, line in enumerate(lines, 1):
    m = re.match(r"^(?:static\s+)?(?:void|std::|bool|QString|std::string|Interpretation)\s+(\w+)", line)
    if m:
        current = m.group(1)
    m2 = re.search(r"case\s+Analysis(?:Command|Type)::(\w+)", line)
    if m2:
        current = m2.group(1)
    # also section markers like // --- DOE
    m3 = re.match(r"\s*//\s*[=-]{3,}\s*(.+)", line)
    if m3:
        current = m3.group(1).strip()[:40]
    func_at[i] = current

by_func = defaultdict(list)
for i, s, k in uncovered:
    by_func[func_at.get(i, "?")].append((i, s, k))

# Titles/headers still unmapped in analysis_service etc.
title_files = [
    "src/application/analysis_service.cpp",
    "src/application/graph_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/chart_pages.cpp",
    "src/application/output_builder.cpp",
]
title_asgn = re.compile(
    r"(?:\.title|\.name|page\.title|table\.title|plot\.title|headers\.push_back|columns\.push_back|add_header|add_column|x_axis_title|y_axis_title)\s*(?:=\s*|\()\s*\"((?:\\.|[^\"\\])*)\""
)
title_keys = {
    a
    for a, b in ((unesc(m.group(1)), unesc(m.group(2))) for m in pair_re.finditer(loc))
    if cjk.search(a)
    and (
        b.startswith(("table.", "plot.", "page.", "model.", "header.", "param.", "chrome.", "graph."))
        or "title" in b
        or "header" in b
        or "axis" in b
    )
}

unmapped_chrome = []
for rel in title_files:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for i, line in enumerate(text.splitlines(), 1):
        for m in title_asgn.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s):
                continue
            base = s[:-len("（分面）")] if s.endswith("（分面）") else s
            for suf in (" 方法与参数", " 参数"):
                if base.endswith(suf):
                    base = base[: -len(suf)]
            if base in title_keys or s in title_keys or covered_exact_or_sw(s) or covered_exact_or_sw(base):
                continue
            unmapped_chrome.append((rel, i, s))

# Domain diags with CJK not covered (fixed message style ending 。)
diag_files = list((root / "src/domain/statistics").glob("*.cpp")) + [
    root / "src/application/analysis_service.cpp",
]
diag_kw = re.compile(r"(diagnostic|Diagnostic|message\s*=|append_diag|push_diag|add_diag|limitation)", re.I)
unmapped_diags = []
for fp in diag_files:
    text = fp.read_text(encoding="utf-8", errors="replace")
    rel = str(fp.relative_to(root)).replace("\\", "/")
    for i, line in enumerate(text.splitlines(), 1):
        if not diag_kw.search(line) and "diagnostics" not in line:
            continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 4:
                continue
            if covered_exact_or_sw(s):
                continue
            # skip dynamic mid-fragments starting with punctuation unless standalone advice
            unmapped_diags.append((rel, i, s))

# Dedup diags
sd = set()
ud = []
for r, i, s in unmapped_diags:
    if s in sd:
        continue
    sd.add(s)
    ud.append((r, i, s))

# Print cluster summary
out = []
out.append(f"exact={len(exact)} sw={len(sw)} uncovered_interp_lits={len(uncovered)} unmapped_chrome={len(unmapped_chrome)} unmapped_diags={len(ud)}")
out.append("\n=== INTERP BY FUNC (top) ===")
for fn, items in sorted(by_func.items(), key=lambda kv: -len(kv[1]))[:30]:
    out.append(f"\n## {fn} n={len(items)}")
    for i, s, k in items[:12]:
        out.append(f"  L{i} [{k}] {s[:120]}")

out.append("\n=== UNMAPPED CHROME ===")
for r, i, s in unmapped_chrome[:50]:
    out.append(f"  {r}:{i}\t{s}")

out.append("\n=== UNMAPPED DIAGS (sample 80) ===")
by_f = defaultdict(list)
for r, i, s in ud:
    by_f[pathlib.Path(r).name].append((i, s))
for fn, items in sorted(by_f.items(), key=lambda kv: -len(kv[1]))[:20]:
    out.append(f"\n## {fn} n={len(items)}")
    for i, s in items[:15]:
        out.append(f"  L{i}\t{s[:140]}")

# Coherent honesty-bullet cluster: full sentences ending with 。 or ； that are uncovered pushes
honesty = [
    (i, s)
    for i, s, k in uncovered
    if k == "push" and (s.endswith("。") or s.endswith("；") or len(s) >= 20)
    and not s.startswith(("，", "；", "）", "、", " "))
]
out.append(f"\n=== HONESTY / FULL PUSH UNCOVERED n={len(honesty)} ===")
for i, s in honesty:
    out.append(f"  L{i}\t{s}")

# Mid-sentence dynamic fragments that look like coherent %1 families
frags = [
    (i, s, k)
    for i, s, k in uncovered
    if s.startswith(("，", "；", " ", "（", "下"))
]
out.append(f"\n=== LEADING-PUNCT FRAGS n={len(frags)} (need parent template) ===")
# group by first 8 chars
g = Counter(s[:12] for _, s, _ in frags)
for pref, n in g.most_common(20):
    out.append(f"  {n}\t{pref!r}")

text = "\n".join(out)
(root / "scripts/_tmp_slice_pick_out.txt").write_text(text, encoding="utf-8")
print(text[:12000])
print("\n... wrote", len(text), "chars")
