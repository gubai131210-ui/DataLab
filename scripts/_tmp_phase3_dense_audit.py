# -*- coding: utf-8 -*-
"""Live densest Phase-3 bilingual gap audit (read-only)."""
from __future__ import annotations

import re
from collections import Counter
from pathlib import Path  # noqa: I001

root = Path(__file__).resolve().parents[1]
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
cjk = re.compile(r"[\u4e00-\u9fff]")
lit = re.compile(r'"((?:[^"\\]|\\.)*)"')

exact = {
    a: b
    for a, b in re.findall(r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)+)"\s*\}', loc)
    if cjk.search(a)
}

sw = set(
    re.findall(
        r'starts_with\(\s*(?:bullet|body|text|value|label)\s*,\s*"((?:[^"\\]|\\.)+)"',
        loc,
    )
)
sw |= set(
    re.findall(
        r'parse_leading_count_after_prefix\(\s*(?:bullet|body)\s*,\s*"((?:[^"\\]|\\.)+)"',
        loc,
    )
)
# Also accept the historical helper name if present.
sw |= set(
    re.findall(
        r'parse_leading_count_after_prefix\(\s*(?:bullet|body)\s*,\s*"((?:[^"\\]|\\.)+)"',
        loc,
    )
)
finds = set(re.findall(r'(?:bullet|body)\.find\(\s*"((?:[^"\\]|\\.)+)"', loc))
ends = set(
    re.findall(r'ends_with\(\s*(?:bullet|body|text)\s*,\s*"((?:[^"\\]|\\.)+)"', loc)
)

param_start = loc.find("void localize_parameter_summary")
param_block = loc[param_start : param_start + 50000] if param_start >= 0 else ""
param_prefs = [
    a
    for a, _ in re.findall(
        r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"(param\.[^"]+)"\s*\}', param_block
    )
]


def covered_interp(t: str) -> str | None:
    if t in exact:
        return "exact"
    for e in exact:
        if len(e) >= 20 and e in t:
            return "exact_clause"
    for p in sorted(sw, key=len, reverse=True):
        if len(p) >= 4 and t.startswith(p):
            return "starts_with"
    for m in sorted(finds, key=len, reverse=True):
        if len(m) >= 8 and m in t:
            return "find"
    for e in sorted(ends, key=len, reverse=True):
        if len(e) >= 6 and t.endswith(e):
            return "ends_with"
    return None


def covered_exact_or_prefix(s: str) -> bool:
    if s in exact:
        return True
    for e in exact:
        if len(e) >= 8 and (s == e or s.startswith(e) or (len(s) > 24 and e in s)):
            return True
    for p in param_prefs:
        if s.startswith(p) or (len(p) >= 4 and p in s):
            return True
    return False


# ---- 1) interpretation bullets ----
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")
lines = interp.splitlines()
cands: list[tuple[int, str]] = []
i = 0
while i < len(lines):
    if "bullets.push_back" in lines[i]:
        buf = lines[i]
        j = i
        while j < len(lines) and ");" not in lines[j]:
            j += 1
            if j < len(lines):
                buf += "\n" + lines[j]
        text = "".join(lit.findall(buf))
        if cjk.search(text):
            cands.append((i + 1, text))
        i = j + 1
        continue
    i += 1
unc_interp = [(ln, t) for ln, t in cands if not covered_interp(t)]

# ---- 2) domain diagnostics (strict) ----
diag_keys = (
    ".message",
    "Diagnostic",
    "diagnostics.push",
    "add_diagnostic",
    "emit_diagnostic",
    "status_message",
    "push_diagnostic",
    "diagnostic.message",
)
domain_unc: list[tuple[str, int, str]] = []
domain_files = list((root / "src/domain/statistics").rglob("*.cpp"))
domain_files += list((root / "src/domain").glob("*.cpp"))
for fp in domain_files:
    for li, line in enumerate(fp.read_text(encoding="utf-8").splitlines(), 1):
        if not cjk.search(line) or line.lstrip().startswith("//"):
            continue
        if not any(k in line for k in diag_keys):
            continue
        for m in lit.findall(line):
            if not cjk.search(m) or len(m) < 4:
                continue
            if not covered_exact_or_prefix(m):
                domain_unc.append(
                    (str(fp.relative_to(root)).replace("\\", "/"), li, m)
                )


def fam_path(p: str, s: str) -> str:
    pl = p.lower() + " " + s
    if any(x in pl for x in ("control_chart", "multivariate", "special_cause", "zone")):
        return "spc"
    if any(x in pl for x in ("process_capability", "quality_extension", "capability")):
        return "capability"
    if any(x in pl for x in ("doe", "response_surface", "plackett", "factorial")):
        return "doe_rsm"
    if any(
        x in pl
        for x in ("censor", "km_interval", "fine_gray", "aalen", "reliab", "warranty")
    ):
        return "reliability"
    if any(x in pl for x in ("hypothesis", "nonparam", "inference", "grubbs")):
        return "inference"
    if "eda" in pl or "graph_assembly" in pl:
        return "eda"
    if any(x in pl for x in ("gage", "msa", "expanded_gage")):
        return "msa"
    return "other"


# ---- 3) application chrome leftovers ----
app_files = [
    root / "src/application/analysis_service.cpp",
    root / "src/application/graph_service.cpp",
    root / "src/application/doe_pages.cpp",
    root / "src/application/chart_pages.cpp",
    root / "src/application/output_builder.cpp",
]

title_keys = (
    ".title",
    "table_title",
    "page_title",
    "caption",
    "header",
    "series_name",
    "axis_label",
    "status",
    "setTitle",
    "title =",
    "headers.push",
    "columns.push",
    ".name =",
)

title_unc: list[tuple[str, int, str]] = []
param_tok: Counter = Counter()
param_ex: list[tuple[str, int, str]] = []

for fp in app_files:
    if not fp.exists():
        continue
    rel = str(fp.relative_to(root)).replace("\\", "/")
    for li, line in enumerate(fp.read_text(encoding="utf-8").splitlines(), 1):
        if not cjk.search(line) or line.lstrip().startswith("//"):
            continue
        is_title = any(k in line for k in title_keys)
        for m in lit.findall(line):
            if not cjk.search(m) or len(m) < 2:
                continue
            if covered_exact_or_prefix(m):
                continue
            if is_title:
                title_unc.append((rel, li, m))
            if len(m) <= 48 and (
                m.endswith(" = ")
                or m.endswith("=")
                or m.startswith("，")
                or "跳过" in m
                or m.endswith(" =")
                or " = " in m
            ):
                param_tok[m] += 1
                if len(param_ex) < 80:
                    param_ex.append((rel, li, m))

# ---- 4) analysis_service diagnostic messages (application-layer) ----
app_diag_unc: list[tuple[str, int, str]] = []
for fp in [
    root / "src/application/analysis_service.cpp",
    root / "src/application/graph_service.cpp",
    root / "src/application/doe_pages.cpp",
    root / "src/application/chart_pages.cpp",
]:
    if not fp.exists():
        continue
    rel = str(fp.relative_to(root)).replace("\\", "/")
    for li, line in enumerate(fp.read_text(encoding="utf-8").splitlines(), 1):
        if not cjk.search(line) or line.lstrip().startswith("//"):
            continue
        if not any(
            k in line
            for k in (
                ".message",
                "diagnostics.push",
                "Diagnostic",
                "append_diagnostic",
                "add_diagnostic",
            )
        ):
            continue
        for m in lit.findall(line):
            if not cjk.search(m) or len(m) < 4:
                continue
            if not covered_exact_or_prefix(m):
                app_diag_unc.append((rel, li, m))

out = root / "scripts/_tmp_phase3_dense_audit_out.txt"
with out.open("w", encoding="utf-8") as f:
    f.write(f"interp_cands={len(cands)} uncovered={len(unc_interp)}\n")
    f.write(
        "domain_diag_unc={} fam={}\n".format(
            len(domain_unc),
            dict(Counter(fam_path(p, s) for p, _, s in domain_unc)),
        )
    )
    f.write(
        f"app_diag_unc={len(app_diag_unc)} title_unc={len(title_unc)} "
        f"param_tok_uniq={len(param_tok)}\n"
    )
    f.write(
        f"exact_zh={len(exact)} sw={len(sw)} finds={len(finds)} "
        f"ends={len(ends)} param_prefs={len(param_prefs)}\n"
    )

    f.write("\n=== UNCOVERED INTERP ===\n")
    for ln, t in unc_interp:
        f.write(f"L{ln}\t{t}\n")

    f.write("\n=== DOMAIN DIAG BY FAM ===\n")
    for fam, n in Counter(fam_path(p, s) for p, _, s in domain_unc).most_common():
        f.write(f"  {n}\t{fam}\n")
    f.write("\n=== DOMAIN DIAG SAMPLE ===\n")
    for p, li, s in domain_unc[:50]:
        f.write(f"{p}:{li}\t{fam_path(p,s)}\t{s}\n")

    f.write("\n=== APP DIAG UNC ===\n")
    for rel, li, m in app_diag_unc[:40]:
        f.write(f"{rel}:{li}\t{m}\n")

    f.write("\n=== TITLE/HEADER/SERIES UNC ===\n")
    for rel, li, m in title_unc[:50]:
        f.write(f"{rel}:{li}\t{m}\n")

    f.write("\n=== PARAM TOKENS ===\n")
    for m, n in param_tok.most_common(40):
        f.write(f"{n}\t{m}\n")
    f.write("\n=== PARAM EXAMPLES ===\n")
    for rel, li, m in param_ex[:40]:
        f.write(f"{rel}:{li}\t{m}\n")

print(f"wrote {out}")
print(f"interp uncovered={len(unc_interp)}/{len(cands)}")
print(
    "domain_diag_unc={} fam={}".format(
        len(domain_unc), dict(Counter(fam_path(p, s) for p, _, s in domain_unc))
    )
)
print(
    f"app_diag_unc={len(app_diag_unc)} title_unc={len(title_unc)} "
    f"param_tok_uniq={len(param_tok)}"
)
print(
    f"exact_zh={len(exact)} sw={len(sw)} finds={len(finds)} "
    f"ends={len(ends)} param_prefs={len(param_prefs)}"
)
for ln, t in unc_interp[:12]:
    print(f"INTERP L{ln}\t{t[:120]}")
for p, li, s in domain_unc[:12]:
    print(f"DOM {p}:{li}\t{s[:100]}")
for rel, li, m in app_diag_unc[:12]:
    print(f"APP {rel}:{li}\t{m[:100]}")
for m, n in param_tok.most_common(12):
    print(f"PARAM {n}\t{m}")
for rel, li, m in title_unc[:12]:
    print(f"TITLE {rel}:{li}\t{m[:80]}")
