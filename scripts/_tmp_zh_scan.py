import re
from collections import Counter, defaultdict

root = r"D:/QT_CppPrograms/DataLab"
zh_known = set()
for path in [
    root + "/src/domain/report_text_catalog.cpp",
    root + "/src/application/report_localization.cpp",
]:
    txt = open(path, encoding="utf-8").read()
    for m in re.finditer(r'"([^"\\]*[\u4e00-\u9fff][^"\\]*)"', txt):
        s = m.group(1)
        if len(s) >= 2:
            zh_known.add(s)

targets = [
    "src/application/analysis_service.cpp",
    "src/application/chart_pages.cpp",
    "src/application/graph_service.cpp",
    "src/application/output_builder.cpp",
    "src/application/doe_pages.cpp",
    # interpretation bulk excluded per user
    "src/domain/graph_assembly.cpp",
    "src/domain/statistics/analysis_rules.cpp",
    "src/domain/statistics/control_charts.cpp",
    "src/domain/statistics/graph_visuals.cpp",
    "src/domain/statistics/special_cause_rule_catalog.cpp",
]

pat = re.compile(r'"([^"\\]*[\u4e00-\u9fff][^"\\]*)"')

def wired(s: str) -> bool:
    if s in zh_known:
        return True
    for k in zh_known:
        if len(k) >= 4 and (s in k or k in s):
            return True
    return False

missing = []
for rel in targets:
    fp = root + "/" + rel
    txt = open(fp, encoding="utf-8").read()
    for i, line in enumerate(txt.splitlines(), 1):
        stripped = line.strip()
        if stripped.startswith("//"):
            continue
        for m in pat.finditer(line):
            s = m.group(1)
            if len(s) < 2:
                continue
            if not wired(s):
                missing.append((rel, i, s))

by_string = defaultdict(list)
for rel, i, s in missing:
    by_string[s].append((rel, i))

out = root + "/scripts/_tmp_zh_scan_out.txt"
with open(out, "w", encoding="utf-8") as f:
    f.write(f"TOTAL missing literal hits: {len(missing)}\n")
    f.write("By file:\n")
    for rel, n in Counter(x[0] for x in missing).most_common():
        f.write(f"  {rel}: {n}\n")
    f.write("\nTop unique strings (by hit count):\n")
    for s, locs in sorted(by_string.items(), key=lambda x: -len(x[1]))[:25]:
        f.write(f"\n[{len(locs)} hits] {s}\n")
        for rel, i in locs[:5]:
            f.write(f"  {rel}:{i}\n")
    f.write("\n--- all samples ---\n")
    for rel, i, s in missing:
        f.write(f"{rel}:{i}: {s}\n")

print("Wrote", out)
