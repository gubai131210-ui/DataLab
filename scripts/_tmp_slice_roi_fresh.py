# -*- coding: utf-8 -*-
"""Fresh ROI audit: unmapped ZH on en-US report path after recent landings."""
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

sw = [
    unesc(x)
    for x in re.findall(r'starts_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)
]
sw = sorted([x for x in sw if len(x) >= 2], key=len, reverse=True)

# Also capture QStringLiteral / contains / ends_with style if any
contains_zh = [
    unesc(x)
    for x in re.findall(r'(?:contains|ends_with|find)\(\s*[^,)]*?,\s*"((?:\\.|[^"\\])*)"', loc)
]
contains_zh = [x for x in contains_zh if cjk.search(x)]


def covered(s: str) -> tuple[bool, str]:
    if s in exact:
        return True, "exact:" + id_for.get(s, "?")
    for p in sw:
        if s.startswith(p):
            return True, f"sw:{p[:50]}"
    # strip common append_diagnostics prefixes already handled
    for pref in (
        "分析限制：",
        "分析错误：",
        "分析警告：",
        "分析提示：",
        "限制：",
        "错误：",
        "警告：",
    ):
        if s.startswith(pref):
            rest = s[len(pref) :]
            if rest in exact:
                return True, "pref+exact"
            for p in sw:
                if rest.startswith(p):
                    return True, "pref+sw"
    return False, ""


# ---------- FIXED DIAGS ----------
diag_files = list((root / "src/domain/statistics").glob("*.cpp")) + [
    root / "src/application/analysis_service.cpp",
    root / "src/application/chart_pages.cpp",
    root / "src/application/graph_service.cpp",
    root / "src/application/output_builder.cpp",
    root / "src/application/doe_pages.cpp",
]

diag_kw = (
    "diagnostic",
    "message",
    "append_diag",
    "push_diag",
    "add_diag",
    "Diagnostic",
)

unmapped_diags: list[tuple[str, int, str]] = []
for fp in diag_files:
    text = fp.read_text(encoding="utf-8", errors="replace")
    rel = fp.as_posix().replace(root.as_posix() + "/", "")
    for i, line in enumerate(text.splitlines(), 1):
        if not any(k in line for k in diag_kw):
            continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 2:
                continue
            if re.match(r"^[a-z0-9_.]+$", s):
                continue
            # skip very short glue / prefixes alone if mapped via sw
            ok, _ = covered(s)
            if not ok:
                unmapped_diags.append((rel, i, s))

seen: set[str] = set()
uniq_diags: list[tuple[str, int, str]] = []
for item in unmapped_diags:
    if item[2] in seen:
        continue
    seen.add(item[2])
    uniq_diags.append(item)


def diag_fam(path: str) -> str:
    p = path.lower()
    if "control_charts" in p or "multivariate_control" in p:
        return "spc"
    if "process_capability" in p or "quality_extensions" in p:
        return "capability"
    if "response_surface" in p or "rsm_" in p or "doe_" in p or "plackett" in p:
        return "doe_rsm"
    if "hypothesis" in p or "inference" in p or "nonparametric" in p or "equivalence" in p:
        return "inference"
    if "gage" in p or "msa" in p or "expanded_gage" in p:
        return "msa"
    if any(
        x in p
        for x in (
            "censor",
            "weibull",
            "km_",
            "fine_gray",
            "aalen",
            "reliability",
            "warranty",
        )
    ):
        return "reliability"
    if "regression" in p or "logistic" in p or "poisson_reg" in p or "stepwise" in p:
        return "regression"
    if "analysis_service" in p:
        return "app_service"
    if "chart" in p or "graph" in p or "output_builder" in p:
        return "chart_app"
    if "eda" in p or "graph_visual" in p or "quality_visual" in p:
        return "eda"
    if "cluster" in p or "kmeans" in p or "cart" in p or "isolation" in p or "discriminant" in p:
        return "ml"
    return "other"


fam = Counter(diag_fam(r) for r, _, _ in uniq_diags)

# ---------- TITLES ----------
title_files = [
    "src/application/analysis_service.cpp",
    "src/application/graph_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/chart_pages.cpp",
    "src/application/output_builder.cpp",
]
title_asgn = re.compile(
    r"(?:\.title|\.name|page\.title|table\.title|plot\.title)\s*=\s*\"((?:\\.|[^\"\\])*)\""
)
# also title via function args like make_table("中文"
title_call = re.compile(
    r"(?:make_table|make_plot|add_table|add_plot|set_title)\(\s*\"((?:\\.|[^\"\\])*)\""
)

title_keys = {a for a, b in ((unesc(m.group(1)), unesc(m.group(2))) for m in pair_re.finditer(loc))
              if cjk.search(a) and (b.startswith(("table.", "plot.", "page.", "model.")) or "title" in b)}

unmapped_titles: list[tuple[str, int, str]] = []
for rel in title_files:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for i, line in enumerate(text.splitlines(), 1):
        for rx in (title_asgn, title_call):
            for m in rx.finditer(line):
                s = unesc(m.group(1))
                if not cjk.search(s):
                    continue
                # strip facet suffix for lookup
                base = s
                if base.endswith("（分面）"):
                    base = base[: -len("（分面）")]
                ok = base in title_keys or s in title_keys or covered(s)[0] or covered(base)[0]
                if not ok:
                    unmapped_titles.append((rel, i, s))

seen_t: set[str] = set()
uniq_titles = []
for item in unmapped_titles:
    if item[2] in seen_t:
        continue
    seen_t.add(item[2])
    uniq_titles.append(item)

# ---------- INTERP starts (coherent templates) ----------
# Capture QString::arg style and %1 templates in localization, and bullet builders in interp
lines = interp.splitlines()
direct_push: list[tuple[int, str]] = []
for i, line in enumerate(lines, 1):
    if "push_back" not in line:
        continue
    for m in str_re.finditer(line):
        s = unesc(m.group(1))
        if not cjk.search(s) or len(s) < 8:
            continue
        if s[0] in "，；）、,;) 。、":
            continue
        if s.startswith(" ") and not any(
            s.lstrip().startswith(p) for p in ("下", "个", "正态")
        ):
            continue
        direct_push.append((i, s))

seen_i: set[str] = set()
uniq_interp = []
for i, s in direct_push:
    if s in seen_i:
        continue
    seen_i.add(s)
    if not covered(s)[0]:
        uniq_interp.append((i, s))

# Also find starts_with candidates that look like coherent templates still missing
# from localization itself: look for %1 patterns in interpretation
pct_templates: list[tuple[int, str]] = []
for i, line in enumerate(lines, 1):
    if "%1" not in line and "arg(" not in line:
        # also string concat templates
        pass
    for m in str_re.finditer(line):
        s = unesc(m.group(1))
        if not cjk.search(s):
            continue
        if "%1" in s or "%2" in s:
            if not covered(s)[0]:
                pct_templates.append((i, s))

# Cluster unmapped diags by directory stem
by_file = Counter()
for r, i, s in uniq_diags:
    by_file[pathlib.Path(r).name] += 1

out = []
out.append(f"exact={len(exact)} sw={len(sw)} uniq_unmapped_diags={len(uniq_diags)} uniq_titles={len(uniq_titles)} uniq_interp_push={len(uniq_interp)} pct_tmpl={len(pct_templates)}")
out.append("\n=== DIAG FAMILIES ===")
for k, v in fam.most_common():
    out.append(f"  {v}\t{k}")
out.append("\n=== DIAG BY FILE ===")
for k, v in by_file.most_common(25):
    out.append(f"  {v}\t{k}")
out.append("\n=== UNMAPPED DIAGS ===")
for r, i, s in uniq_diags:
    out.append(f"diag\t{diag_fam(r)}\t{r}:{i}\t{s}")
out.append("\n=== UNMAPPED TITLES (sample 40) ===")
for r, i, s in uniq_titles[:40]:
    out.append(f"title\t{r}:{i}\t{s}")
out.append("\n=== UNMAPPED INTERP DIRECT PUSH ===")
for i, s in uniq_interp:
    out.append(f"interp\tL{i}\t{s}")
out.append("\n=== UNMAPPED %N TEMPLATES ===")
for i, s in pct_templates[:40]:
    out.append(f"pct\tL{i}\t{s}")

text = "\n".join(out)
(root / "scripts/_tmp_slice_roi_fresh_out.txt").write_text(text, encoding="utf-8")
print(text[:8000])
print("\n... wrote scripts/_tmp_slice_roi_fresh_out.txt total_chars", len(text))
