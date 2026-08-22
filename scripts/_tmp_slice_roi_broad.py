# -*- coding: utf-8 -*-
"""Broader ROI: titles/headers/param chrome + full interp bullets still mixed under en-US."""
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
    return bytes(s, "utf-8").decode("unicode_escape") if False else s.replace(r"\n", "\n").replace(r"\"", '"').replace(r"\\", "\\")


# Fix unesc properly
def unesc(s: str) -> str:
    out = []
    i = 0
    while i < len(s):
        if s[i] == "\\" and i + 1 < len(s):
            n = s[i + 1]
            if n == "n":
                out.append("\n")
            elif n == '"':
                out.append('"')
            elif n == "\\":
                out.append("\\")
            else:
                out.append(n)
            i += 2
        else:
            out.append(s[i])
            i += 1
    return "".join(out)


exact: dict[str, str] = {}
for m in pair_re.finditer(loc):
    a, b = unesc(m.group(1)), unesc(m.group(2))
    if cjk.search(a):
        exact[a] = b

sw = sorted(
    [
        unesc(x)
        for x in re.findall(r'starts_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)
        if len(unesc(x)) >= 2
    ],
    key=len,
    reverse=True,
)


def covered(s: str) -> tuple[bool, str]:
    if s in exact:
        return True, exact[s]
    for p in sw:
        if s.startswith(p):
            return True, f"sw:{p[:40]}"
    # special handlers in localize path
    specials = [
        "没有可显示的数据。",
        " 当前未触发。",
        "当前未触发。",
    ]
    if s in specials or any(s.endswith(x) for x in specials):
        return True, "special"
    return False, ""


# Classify map keys
title_keys = {a for a, b in exact.items() if b.startswith(("table.", "plot.", "page.", "model.", "chrome.")) or "title" in b or b.startswith("header.")}
header_keys = {a for a, b in exact.items() if b.startswith("header.") or b in ("table.property", "table.value")}
plain_keys = set(exact.keys())

# ---- TITLES: any assignment of CJK to title-ish fields OR string before make_* ----
app_files = [
    "src/application/analysis_service.cpp",
    "src/application/graph_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/chart_pages.cpp",
    "src/application/output_builder.cpp",
]

title_patterns = [
    re.compile(r"(?:title|name)\s*=\s*\"((?:\\.|[^\"\\])*)\""),
    re.compile(r"(?:make_table|make_plot|add_table|add_plot|OutputTable|OutputPlot)\(\s*\"((?:\\.|[^\"\\])*)\""),
    re.compile(r"page\.title\s*=\s*\"((?:\\.|[^\"\\])*)\""),
]

# Broader: lines with .title = Chinese OR Chinese in quotes near Table/Plot
unmapped_titles = []
all_titles = []
for rel in app_files:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for i, line in enumerate(text.splitlines(), 1):
        if "title" not in line.lower() and "make_table" not in line and "make_plot" not in line:
            continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 2:
                continue
            # skip long sentences (diags/interp)
            if len(s) > 80 or s.endswith("。"):
                continue
            base = s[:-len("（分面）")] if s.endswith("（分面）") else s
            all_titles.append((rel, i, s))
            ok = (
                s in title_keys
                or base in title_keys
                or s in plain_keys
                or base in plain_keys
                or covered(s)[0]
                or covered(base)[0]
            )
            # also try stripping common suffixes
            for suf in (" 方法与参数", " 参数", "（分面）", " - ", "："):
                if base.endswith(suf.strip()) or True:
                    pass
            if not ok:
                # try prefix match against title keys (ANOM variable suffix etc)
                pref_ok = False
                for tk in title_keys:
                    if s.startswith(tk) or tk.startswith(s):
                        pref_ok = True
                        break
                if not pref_ok:
                    unmapped_titles.append((rel, i, s))

seen = set()
uniq_titles = []
for item in unmapped_titles:
    if item[2] in seen:
        continue
    seen.add(item[2])
    uniq_titles.append(item)

# ---- HEADERS ----
header_asgn = re.compile(
    r"(?:headers\.push_back|columns\.push_back|add_header|add_column|push_back)\(\s*\"((?:\\.|[^\"\\])*)\""
)
unmapped_headers = []
for rel in app_files:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for i, line in enumerate(text.splitlines(), 1):
        if "header" not in line and "column" not in line and "push_back" not in line:
            continue
        if not any(k in line for k in ("header", "column", "headers", "columns")):
            continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 1:
                continue
            if s in header_keys or s in plain_keys or covered(s)[0]:
                continue
            unmapped_headers.append((rel, i, s))

seen_h = set()
uniq_headers = []
for item in unmapped_headers:
    if item[2] in seen_h:
        continue
    seen_h.add(item[2])
    uniq_headers.append(item)

# ---- Domain diags: ALL CJK strings in Diagnostic constructors (broader) ----
# Look for message fields and push patterns more carefully
diag_msg_re = re.compile(
    r"(?:message\s*=\s*|,\s*)\"((?:\\.|[^\"\\])*[\u4e00-\u9fff](?:\\.|[^\"\\])*)\""
)

domain_dir = root / "src/domain/statistics"
unmapped_domain = []
for fp in sorted(domain_dir.glob("*.cpp")):
    text = fp.read_text(encoding="utf-8", errors="replace")
    rel = fp.as_posix().replace(root.as_posix() + "/", "")
    # Only lines that look like diagnostic emission
    for i, line in enumerate(text.splitlines(), 1):
        if not any(
            k in line
            for k in (
                "Diagnostic",
                "diagnostics",
                "message",
                "push_back",
                "emplace",
                "add_error",
                "add_warning",
                "add_info",
                "fail",
            )
        ):
            continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 4:
                continue
            if re.match(r"^[a-z0-9_.]+$", s):
                continue
            # skip mid-glue starting with punctuation (dynamic)
            if s[0] in "，；）、,;) 。、;":
                continue
            ok, how = covered(s)
            if not ok:
                unmapped_domain.append((rel, i, s))

seen_d = set()
uniq_domain = []
for item in unmapped_domain:
    if item[2] in seen_d:
        continue
    seen_d.add(item[2])
    uniq_domain.append(item)

by_file = Counter(pathlib.Path(r).name for r, _, _ in uniq_domain)

# ---- Interp: reconstruct likely FULL bullets that are push_back of literal-only ----
# Find push_back("....。") complete sentences
full_bullets = []
partial_starts = []
for i, line in enumerate(interp.splitlines(), 1):
    if "push_back" not in line:
        continue
    for m in str_re.finditer(line):
        s = unesc(m.group(1))
        if not cjk.search(s) or len(s) < 10:
            continue
        if s.endswith("。") or s.endswith("；"):
            full_bullets.append((i, s))
        elif s[0] not in "，；）、,;) 。、" and not s.startswith(" "):
            partial_starts.append((i, s))

unmapped_full = [(i, s) for i, s in full_bullets if not covered(s)[0]]
# For partial starts: check if ANY exact key starts with this fragment (assembled bullet)
unmapped_partial = []
for i, s in partial_starts:
    if covered(s)[0]:
        continue
    # if some exact key starts with s, the full bullet is mapped
    if any(k.startswith(s) for k in exact):
        continue
    # if s is mid-glue only
    unmapped_partial.append((i, s))

# Cluster partial by nearby function - rough: look at preceding comment or function name
def nearest_fn(lineno: int) -> str:
    lines = interp.splitlines()
    for j in range(lineno - 1, max(0, lineno - 80), -1):
        m = re.search(r"^(?:static\s+)?(?:void|std::vector|auto|bool|std::string)\s+(\w+)", lines[j])
        if m:
            return m.group(1)
        m = re.search(r"^(\w+)\s*\(", lines[j])
        if m and lines[j].strip().endswith("{") or (j + 1 < len(lines) and lines[j + 1].strip() == "{"):
            return m.group(1)
    return "?"


fn_cluster = Counter()
for i, s in unmapped_partial:
    fn_cluster[nearest_fn(i)] += 1

# ---- analysis_service diagnostic prefixes still Chinese ----
svc = (root / "src/application/analysis_service.cpp").read_text(encoding="utf-8")
# pattern: "中文：" prepended to diagnostics
prefix_re = re.compile(r'\"([^\"]*[\u4e00-\u9fff][^\"]*[:：])\"')
svc_prefixes = []
for i, line in enumerate(svc.splitlines(), 1):
    if "diagnostic" not in line.lower() and "message" not in line and "prefix" not in line.lower():
        if "：" not in line and ":" not in line:
            continue
    for m in str_re.finditer(line):
        s = unesc(m.group(1))
        if cjk.search(s) and s.endswith(("：", ":")) and 2 <= len(s) <= 20:
            ok, _ = covered(s)
            if not ok:
                svc_prefixes.append((i, s))

seen_p = set()
uniq_pref = []
for i, s in svc_prefixes:
    if s in seen_p:
        continue
    seen_p.add(s)
    uniq_pref.append((i, s))

out = []
out.append(
    f"exact={len(exact)} sw={len(sw)} titles_u={len(uniq_titles)} headers_u={len(uniq_headers)} "
    f"domain_diag_u={len(uniq_domain)} interp_full_u={len(unmapped_full)} interp_partial_u={len(unmapped_partial)} "
    f"svc_pref_u={len(uniq_pref)}"
)
out.append("\n=== TITLE UNMAPPED ===")
for r, i, s in uniq_titles[:60]:
    out.append(f"title\t{r}:{i}\t{s}")
out.append("\n=== HEADER UNMAPPED ===")
for r, i, s in uniq_headers[:60]:
    out.append(f"header\t{r}:{i}\t{s}")
out.append("\n=== DOMAIN DIAG BY FILE ===")
for k, v in by_file.most_common(30):
    out.append(f"  {v}\t{k}")
out.append("\n=== DOMAIN DIAG UNMAPPED ===")
for r, i, s in uniq_domain:
    out.append(f"diag\t{r}:{i}\t{s}")
out.append("\n=== INTERP FULL UNMAPPED ===")
for i, s in unmapped_full:
    out.append(f"full\tL{i}\t{s}")
out.append("\n=== INTERP PARTIAL FN CLUSTER ===")
for k, v in fn_cluster.most_common(20):
    out.append(f"  {v}\t{k}")
out.append("\n=== INTERP PARTIAL SAMPLE (by fn) ===")
by_fn = defaultdict(list)
for i, s in unmapped_partial:
    by_fn[nearest_fn(i)].append((i, s))
for fn, items in sorted(by_fn.items(), key=lambda x: -len(x[1]))[:8]:
    out.append(f"-- {fn} n={len(items)}")
    for i, s in items[:12]:
        out.append(f"  L{i}\t{s}")
out.append("\n=== SVC PREFIXES ===")
for i, s in uniq_pref:
    out.append(f"pref\tanalysis_service.cpp:{i}\t{s}")

text = "\n".join(out)
(root / "scripts/_tmp_slice_roi_broad_out.txt").write_text(text, encoding="utf-8")
print(text[:12000])
