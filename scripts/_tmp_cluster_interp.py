# -*- coding: utf-8 -*-
"""Cluster remaining unmapped interpretation ZH for next bilingual slice."""
from __future__ import annotations

import pathlib
import re
from collections import defaultdict

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
cjk = re.compile(r"[\u4e00-\u9fff]")
pair_re = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}')
str_re = re.compile(r'"((?:\\.|[^"\\])*)"')


def unesc(s: str) -> str:
    return s.replace(r"\n", "\n").replace(r"\"", '"').replace(r"\\", "\\")


plain_keys: set[str] = set()
for m in pair_re.finditer(loc):
    a, b = unesc(m.group(1)), unesc(m.group(2))
    if cjk.search(a) and (
        b.startswith("interp.")
        or b.startswith("diag.")
        or b.startswith("msg.")
        or b.startswith("status.")
        or ("." in b and not cjk.search(b))
    ):
        plain_keys.add(a)

plain_fn = loc.find("localize_known_plain_message")
plain_block = loc[plain_fn : plain_fn + 200000] if plain_fn >= 0 else ""
for m in pair_re.finditer(plain_block):
    a = unesc(m.group(1))
    if cjk.search(a):
        plain_keys.add(a)

# starts_with prefixes already handled in localize_interpretation
interp_fn = loc.find("localize_interpretation")
ib = loc[interp_fn:] if interp_fn >= 0 else ""
sw = re.findall(r'starts_with\([^,]+,\s*"((?:\\.|[^"\\])*)"\)', ib)
sw_prefixes = [unesc(x) for x in sw if cjk.search(unesc(x))]

# contains/ends_with style also count as covered fragments when exact
contains = re.findall(r'(?:contains|ends_with|starts_with)\([^,]+,\s*"((?:\\.|[^"\\])*)"\)', ib)
handled_frags = {unesc(x) for x in contains if cjk.search(unesc(x))}

interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")
unmapped: list[tuple[int, str]] = []
for i, line in enumerate(interp.splitlines(), 1):
    if "push_back" not in line and "bullets" not in line and "limitations" not in line:
        if '<< "' not in line and '+= "' not in line and '= "' not in line:
            continue
    for m in str_re.finditer(line):
        s = unesc(m.group(1))
        if not cjk.search(s) or len(s) < 8:
            continue
        if s in plain_keys:
            continue
        covered = False
        for pk in plain_keys:
            if len(pk) >= 8 and (s.startswith(pk) or pk.startswith(s[: min(12, len(s))])):
                covered = True
                break
        if covered:
            continue
        for pk in sw_prefixes:
            if len(pk) >= 6 and s.startswith(pk):
                covered = True
                break
        if covered:
            continue
        # exact fragment already handled via contains/ends_with
        if s in handled_frags:
            continue
        unmapped.append((i, s))

seen: set[str] = set()
unique: list[tuple[int, str]] = []
for i, s in unmapped:
    if s in seen:
        continue
    seen.add(s)
    unique.append((i, s))

# Prefer starts_with-friendly dynamic prefixes
def prefix_of(s: str) -> str:
    for marker in [" = ", "：", "（", " =", "="]:
        p = s.find(marker)
        if p >= 0 and p >= 4:
            return s[: p + len(marker)]
    # honesty/advice full sentences: keep first clause
    for marker in ["；", "，", "。"]:
        p = s.find(marker)
        if p >= 12:
            return s[: p + len(marker)]
    return s[:28]


clusters: dict[str, list[tuple[int, str]]] = defaultdict(list)
for i, s in unique:
    clusters[prefix_of(s)].append((i, s))

# Also group by thematic families for ROI
families = {
    "nonparam_stats": [],
    "tables_mcnemar_chi": [],
    "ml_models": [],
    "reliability_warranty": [],
    "equiv_var_prop_boxcox": [],
    "capability_gates": [],
    "spc_honesty": [],
    "doe_rsm_msa": [],
    "corr_anova_dw": [],
    "other_static": [],
    "other_dynamic": [],
}

def classify(s: str) -> str:
    keys = [
        ("nonparam_stats", ["Wilcoxon", "Friedman", "Mood", "符号检验", "比较准则 K", "Runs", "Kruskal", "Mann"]),
        ("tables_mcnemar_chi", ["McNemar", "Cochran", "期望频数", "分类，N", "缺失 N*", "调整残差", "拟合优度 Pearson", "优势比"]),
        ("ml_models", ["CART", "Poisson 回归", "Bootstrap", "有序 Logistic", "线性判别", "Isolation", "K-Means", "层次"]),
        ("reliability_warranty", ["Weibull", "对数正态", "指数", "Fine-Gray", "Turnbull", "保修", "KM", "区间删失"]),
        ("equiv_var_prop_boxcox", ["比例检验", "Box-Cox", "Bonett", "Bartlett", "Levene", "F 检验依赖", "容差区间", "等价"]),
        ("capability_gates", ["gate", "Hartigan", "双峰", "混合", "Johnson", "组间/组内", "非正态", "过程合格"]),
        ("spc_honesty", ["CUSUM", "区域图", "移动平均", "Shewhart", "超限", "特殊原因", "Laney"]),
        ("doe_rsm_msa", ["Desirability", "等值线", "RSM", "失拟", "Plackett", "Box–Behnken", "偏倚", "量具", "Gage", "中心点", "显著项"]),
        ("corr_anova_dw", ["相关", "ANOVA", "Durbin", "VIF", "Tukey", "功效"]),
    ]
    for fam, toks in keys:
        if any(t in s for t in toks):
            return fam
    if any(ch.isdigit() or ch in "=：" for ch in s[:40]) and (" = " in s or "：" in s or s.endswith(" = ") or "N =" in s):
        return "other_dynamic"
    return "other_static"


for i, s in unique:
    families[classify(s)].append((i, s))

out = root / "scripts/_tmp_cluster_interp_out.txt"
with out.open("w", encoding="utf-8") as f:
    f.write(f"plain_keys={len(plain_keys)} sw_prefixes={len(sw_prefixes)} unmapped_unique={len(unique)}\n")
    f.write("\n=== FAMILY COUNTS ===\n")
    for fam, items in sorted(families.items(), key=lambda kv: -len(kv[1])):
        f.write(f"{len(items):3d}  {fam}\n")
    f.write("\n=== TOP PREFIX CLUSTERS ===\n")
    ranked = sorted(clusters.items(), key=lambda kv: (-len(kv[1]), -len(kv[0])))
    for pref, items in ranked[:50]:
        f.write(f"\n--- n={len(items)} pref={pref!r}\n")
        for i, s in items[:5]:
            f.write(f"  L{i}\t{s}\n")
    f.write("\n=== FULL UNIQUE (line, text) ===\n")
    for i, s in unique:
        f.write(f"{i}\t{classify(s)}\t{s}\n")

print(f"wrote {out}")
print(f"unmapped_unique={len(unique)}")
for fam, items in sorted(families.items(), key=lambda kv: -len(kv[1])):
    print(f"  {len(items):3d} {fam}")
