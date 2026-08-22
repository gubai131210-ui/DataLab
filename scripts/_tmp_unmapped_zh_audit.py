# -*- coding: utf-8 -*-
"""One-shot audit: CJK strings emitted in report path vs localization maps."""
from __future__ import annotations

import collections
import pathlib
import re

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
cjk_re = re.compile(r"[\u4e00-\u9fff]")
str_re = re.compile(r'"((?:\\.|[^"\\])*)"')


def unescape(s: str) -> str:
    return (
        s.replace(r"\n", "\n")
        .replace(r"\"", '"')
        .replace(r"\\", "\\")
        .replace(r"\t", "\t")
    )


def cjk_strings(text: str) -> set[str]:
    out: set[str] = set()
    for m in str_re.finditer(text):
        s = unescape(m.group(1))
        if cjk_re.search(s):
            out.add(s)
    return out


def extract_map_keys(text: str, array_name_hints: list[str]) -> set[str]:
    """Extract first-of-pair ZH keys near known map arrays."""
    keys: set[str] = set()
    # Any {"ZH...", "id"} pair
    pair_re = re.compile(
        r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}'
    )
    for m in pair_re.finditer(text):
        a, b = unescape(m.group(1)), unescape(m.group(2))
        if cjk_re.search(a) and ("." in b or b.startswith("header") or b.startswith("table")
                                 or b.startswith("plot") or b.startswith("page")
                                 or b.startswith("interp") or b.startswith("diag")
                                 or b.startswith("model") or b.startswith("chrome")
                                 or b.startswith("param") or b.startswith("rule")):
            keys.add(a)
        elif cjk_re.search(a) and not cjk_re.search(b):
            # plain message -> id style
            keys.add(a)
    return keys


loc = (root / "src/application/report_localization.cpp").read_text(
    encoding="utf-8", errors="replace"
)
cat = (root / "src/domain/report_text_catalog.cpp").read_text(
    encoding="utf-8", errors="replace"
)
mapped = cjk_strings(loc) | cjk_strings(cat) | extract_map_keys(loc, [])

# Also treat catalog zh fields as mapped surface
cat_zh = re.compile(r'"zh_cn"\s*:\s*"((?:\\.|[^"\\])*)"', re.I)
# catalog is C++ not JSON; still ok via cjk_strings

emit_files = [
    "src/application/output_builder.cpp",
    "src/application/interpretation_service.cpp",
    "src/application/chart_pages.cpp",
    "src/application/doe_pages.cpp",
    "src/application/graph_service.cpp",
    "src/application/analysis_service.cpp",
]
for fp in (root / "src/domain/statistics").glob("*.cpp"):
    emit_files.append(str(fp.relative_to(root)).replace("\\", "/"))
for extra in [
    "src/domain/graph_assembly.cpp",
    "src/domain/statistics/quality_visuals.cpp",
    "src/domain/statistics/graph_visuals.cpp",
]:
    if (root / extra).exists():
        emit_files.append(extra)

# Classify by context around the string
ctx_re_patterns = {
    "title": re.compile(
        r"(?:title|table_title|plot_title|page\.title|setTitle|\.title\s*=)\s*[^\n]{0,80}\"((?:\\.|[^\"\\])*)\"",
        re.I,
    ),
    "header": re.compile(
        r"(?:headers?|columns?|push_back|emplace_back)\s*[^\n]{0,60}\"((?:\\.|[^\"\\])*)\"",
        re.I,
    ),
    "axis": re.compile(
        r"(?:x_label|y_label|axis|xlabel|ylabel)\s*[^\n]{0,60}\"((?:\\.|[^\"\\])*)\"",
        re.I,
    ),
    "series": re.compile(
        r"(?:series|legend|name)\s*[^\n]{0,60}\"((?:\\.|[^\"\\])*)\"",
        re.I,
    ),
}

unmapped: dict[str, dict] = {}
by_file = collections.Counter()
by_kind = collections.Counter()

# Line-based extraction for titles/headers/diags
line_kinds = [
    ("title", re.compile(r"\.(?:title|name)\s*=\s*\"((?:\\.|[^\"\\])*)\"")),
    ("title", re.compile(r"title\s*=\s*\"((?:\\.|[^\"\\])*)\"")),
    ("header", re.compile(r"headers?\.push_back\(\s*\"((?:\\.|[^\"\\])*)\"")),
    ("header", re.compile(r"columns?\.push_back\(\s*\"((?:\\.|[^\"\\])*)\"")),
    ("header", re.compile(r"add_column\(\s*\"((?:\\.|[^\"\\])*)\"")),
    ("axis", re.compile(r"(?:x_label|y_label|x_axis|y_axis)\s*=\s*\"((?:\\.|[^\"\\])*)\"")),
    ("series", re.compile(r"series(?:_name)?\s*=\s*\"((?:\\.|[^\"\\])*)\"")),
    ("diag", re.compile(r"push_diag(?:nostic)?\([^;\n]*\"((?:\\.|[^\"\\])*)\"")),
    ("diag", re.compile(r"message\s*=\s*\"((?:\\.|[^\"\\])*)\"")),
    ("param", re.compile(r"parameters?\[[^\]]*\]\s*=\s*\"((?:\\.|[^\"\\])*)\"")),
    ("param", re.compile(r"parameter_summary[^\n]*\"((?:\\.|[^\"\\])*)\"")),
]

for rel in emit_files:
    fp = root / rel
    if not fp.exists():
        continue
    text = fp.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    for i, line in enumerate(lines, 1):
        for kind, cre in line_kinds:
            for m in cre.finditer(line):
                s = unescape(m.group(1))
                if not cjk_re.search(s):
                    continue
                # Exact mapped?
                if s in mapped:
                    continue
                # Prefix-covered by mapped keys?
                covered = False
                for mk in mapped:
                    if len(mk) >= 2 and (s.startswith(mk) or mk in s and len(mk) >= 4):
                        # only count as covered if mapped key is substantial prefix/exact use
                        if s == mk or s.startswith(mk):
                            covered = True
                            break
                if covered:
                    continue
                key = (kind, s)
                if key not in unmapped:
                    unmapped[key] = {"count": 0, "locs": []}
                unmapped[key]["count"] += 1
                if len(unmapped[key]["locs"]) < 3:
                    unmapped[key]["locs"].append(f"{rel}:{i}")
                by_file[rel] += 1
                by_kind[kind] += 1

# Also scan interpretation bullets / limitations that are static ZH not in maps
interp = root / "src/application/interpretation_service.cpp"
if interp.exists():
    text = interp.read_text(encoding="utf-8", errors="replace")
    for i, line in enumerate(text.splitlines(), 1):
        for m in str_re.finditer(line):
            s = unescape(m.group(1))
            if not cjk_re.search(s):
                continue
            if len(s) < 4:
                continue
            if s in mapped:
                continue
            # skip format fragments that are mid-template pieces already partially mapped
            if s.startswith("+") or s.endswith("+"):
                continue
            key = ("interp", s)
            if key not in unmapped:
                unmapped[key] = {"count": 0, "locs": []}
            unmapped[key]["count"] += 1
            if len(unmapped[key]["locs"]) < 3:
                unmapped[key]["locs"].append(f"src/application/interpretation_service.cpp:{i}")
            by_file["src/application/interpretation_service.cpp"] += 1
            by_kind["interp"] += 1

out_path = root / "scripts/_tmp_unmapped_zh_audit_out.txt"
with out_path.open("w", encoding="utf-8") as f:
    f.write(f"MAPPED_CJK_LITERALS {len(mapped)}\n")
    f.write(f"UNMAPPED_ENTRIES {len(unmapped)}\n")
    f.write("BY_KIND\n")
    for k, c in by_kind.most_common():
        f.write(f"  {k}\t{c}\n")
    f.write("BY_FILE\n")
    for k, c in by_file.most_common(25):
        f.write(f"  {c}\t{k}\n")
    f.write("ITEMS\n")
    # Prefer title/header/axis/series/param chrome over long interp
    order = {"title": 0, "header": 1, "axis": 2, "series": 3, "param": 4, "diag": 5, "interp": 6}
    items = sorted(
        unmapped.items(),
        key=lambda kv: (order.get(kv[0][0], 9), -kv[1]["count"], -len(kv[0][1]), kv[0][1]),
    )
    for (kind, s), meta in items:
        locs = ";".join(meta["locs"])
        f.write(f"{kind}\t{meta['count']}\t{locs}\t{s}\n")

print(f"wrote {out_path}")
print(f"unmapped={len(unmapped)} mapped_literals={len(mapped)}")
for k, c in by_kind.most_common():
    print(f"  {k}: {c}")
for k, c in by_file.most_common(10):
    print(f"  {c} {k}")
