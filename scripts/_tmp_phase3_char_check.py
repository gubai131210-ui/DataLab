# -*- coding: utf-8 -*-
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
gs = (root / "src/application/graph_service.cpp").read_text(encoding="utf-8")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
chart = (root / "src/application/chart_pages.cpp").read_text(encoding="utf-8")
spc = (root / "src/domain/statistics/special_cause_rule_catalog.cpp").read_text(
    encoding="utf-8"
)
doe = (root / "src/domain/statistics/doe_factorial.cpp").read_text(encoding="utf-8")
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")
out: list[str] = []


def dump(tag: str, text: str, pat: str) -> None:
    hits = sorted({h for h in re.findall(pat, text) if h})
    out.append(f"{tag} n={len(hits)}")
    for h in hits[:50]:
        out.append(f"  {h!r}")


dump("graph_facetish", gs, r'"([^"]{0,40}分[^"]{0,12})"')
dump("loc_facet_literals", loc, r'"(（分[^"]{1,4}）)"')
for m in re.finditer(r".{0,60}Hexbin.{0,60}", gs):
    out.append(f"GS_HEX {m.group(0)!r}")
for m in re.finditer(r".{0,60}Hexbin.{0,60}", loc):
    out.append(f"LOC_HEX {m.group(0)!r}")

dump("nodata", gs + loc + chart, r'"([^"]*(?:可显示|没有可显示)[^"]*)"')
dump("notrig", loc + spc, r'"([^"]*未触发[^"]*)"')
dump("res", doe, r'"([^"]*(?:分辨|部分析因)[^"]*)"')

param_start = loc.find("void localize_parameter_summary")
param_block = loc[param_start : param_start + 50000] if param_start >= 0 else ""
prefs = [
    a
    for a, _ in re.findall(
        r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"(param\.[^"]+)"\s*\}',
        param_block,
    )
]
candidates = [
    "区组 = ",
    "缺失 = ",
    "，列水平数 = ",
    "，组 2 = ",
    "比例 = ",
    "处理 = ",
    "预测期数 = ",
    "因子 A = ",
    "因子 B = ",
    "迭代 = ",
    "移动极差长度 = ",
    "正态概率图相关系数 = ",
    "组数 = ",
    "中位数 = ",
    "跳过 ",
    "显示 N = ",
    "分析 N = ",
    "分析 N(水平) = ",
    "分面 = ",
    "显示 N = ",
    "分析 N = ",
    "分析 N(水平) = ",
    "分面 = ",
]
# Use exact strings from dense audit output file if present.
audit = root / "scripts/_tmp_phase3_dense_audit_out.txt"
if audit.exists():
    for line in audit.read_text(encoding="utf-8").splitlines():
        if "\t" in line and (" = " in line or line.strip().endswith("跳过 ") or "跳过" in line):
            # PARAM token lines look like: 36\t    显示 N =
            parts = line.split("\t", 1)
            if len(parts) == 2 and parts[0].isdigit():
                candidates.append(parts[1])

out.append(f"PARAM_COVERAGE prefs={len(prefs)}")
seen = set()
for c in candidates:
    if c in seen:
        continue
    seen.add(c)
    ok = (c in prefs) or any(c.startswith(p) for p in prefs if p) or (f'"{c}"' in loc)
    # also allow trimmed match against caption/param maps
    ct = c.strip()
    ok = ok or any(ct.startswith(p.strip()) for p in prefs if p.strip()) or (ct in loc)
    out.append(f"  {'OK' if ok else 'UN'} {c!r}")

out.append("PLOT_OF_SUFFIXES")
for pat in ["的正态概率图", "的直方图", "的个体值图", "的散点图", " 运行图", "（编码）"]:
    out.append(f"  {pat!r} in_loc={pat in loc}")

for needle in [
    "已按 ",
    "规则「",
    "部分析因分辨度",
    "没有可显示的数据",
    "当前未触发",
    " 当前未触发。",
    "（分面）",
    "（分面）",
]:
    out.append(
        "NEEDLE {!r} loc={} interp={} graph={} chart={} doe={} spc={}".format(
            needle,
            needle in loc,
            needle in interp,
            needle in gs,
            needle in chart,
            needle in doe,
            needle in spc,
        )
    )

# Exact strings from dense audit domain/app
for needle in [
    "部分析因分辨度 ",
    " 当前未触发。",
    "诊断无关联行",
    "没有可显示的数据。",
]:
    out.append(f"EXACT_NEEDLE {needle!r} in_loc={needle in loc}")

path = root / "scripts/_tmp_phase3_char_check.txt"
path.write_text("\n".join(out), encoding="utf-8")
print(f"wrote {path} lines={len(out)}")
