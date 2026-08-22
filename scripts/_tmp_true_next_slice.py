# -*- coding: utf-8 -*-
"""True densest remaining bilingual slice after just-landed chrome/gates."""
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


exact: dict[str, str] = {}
for m in pair_re.finditer(loc):
    a, b = unesc(m.group(1)), unesc(m.group(2))
    if cjk.search(a):
        exact[a] = b

sw = sorted(
    {unesc(x) for x in re.findall(r'starts_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)},
    key=len,
    reverse=True,
)
ew = sorted(
    {unesc(x) for x in re.findall(r'ends_with\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)', loc)},
    key=len,
    reverse=True,
)
find_marks = sorted(
    {
        unesc(x)
        for x in re.findall(r'\.find\(\s*"((?:\\.|[^"\\])*)"\s*\)', loc)
        if cjk.search(unesc(x)) or " = " in unesc(x) or "≈" in unesc(x)
    },
    key=len,
    reverse=True,
)
count_prefs = sorted(
    {
        unesc(x)
        for x in re.findall(
            r'parse_leading_count_after_prefix\(\s*[^,]+,\s*"((?:\\.|[^"\\])*)"\s*\)',
            loc,
        )
    },
    key=len,
    reverse=True,
)


def covered_bullet(s: str) -> tuple[bool, str]:
    if s in exact:
        return True, "exact:" + exact[s]
    for p in sw:
        if len(p) >= 4 and s.startswith(p):
            return True, "sw:" + p[:48]
    for p in count_prefs:
        if len(p) >= 3 and s.startswith(p):
            return True, "count:" + p
    # Multi-mark find templates: if >=2 distinctive find marks appear, treat as covered
    hits = [m for m in find_marks if len(m) >= 4 and m in s]
    if len(hits) >= 2:
        return True, "find2:" + hits[0][:30]
    if len(hits) == 1 and len(hits[0]) >= 10 and any(
        s.startswith(p) for p in sw if len(p) >= 3
    ):
        return True, "sw+find"
    # ends_with + long exact suffix
    for p in ew:
        if len(p) >= 12 and s.endswith(p) and p in exact:
            return True, "ew-exact"
    # If bullet equals concatenation of two exact keys (common split honesty)
    for a in list(exact.keys()):
        if len(a) < 20:
            continue
        if s.startswith(a) and s[len(a) :] in exact:
            return True, "exact+exact"
        if a in s and len(a) > len(s) * 0.7:
            # near-full exact contained (reconstruction hole)
            return True, "exact-near"
    return False, ""


# Reconstruct push_back concatenations
lines = interp.splitlines()
bullets: list[tuple[int, str, str]] = []  # ln, joined, raw_chunk_preview
i = 0
while i < len(lines):
    line = lines[i]
    if "push_back" not in line:
        i += 1
        continue
    # focus on bullets / limitations / conclusions / advice
    if not any(
        k in line
        for k in (
            "bullets.push_back",
            "limitations.bullets",
            "conclusions.push_back",
            "advice.push_back",
            "notes.push_back",
        )
    ) and "bullet" not in line.lower():
        # still catch: something.bullets.push_back
        if ".push_back(" not in line:
            i += 1
            continue
        if "bullet" not in line and "limitation" not in line and "conclusion" not in line:
            i += 1
            continue
    chunk = line
    j = i
    depth = chunk.count("(") - chunk.count(")")
    while depth > 0 and j + 1 < len(lines):
        j += 1
        chunk += "\n" + lines[j]
        depth += lines[j].count("(") - lines[j].count(")")
        if j - i > 50:
            break
    lits = [unesc(m.group(1)) for m in str_re.finditer(chunk)]
    if not any(cjk.search(x) for x in lits):
        i = j + 1
        continue
    parts = []
    for lit in lits:
        if cjk.search(lit):
            parts.append(lit)
        elif lit in ("α = ", " = ", " / ", "、", "。", "；", "，", "（", "）", "%", " 与 ", " 下", "：", ":"):
            parts.append(lit)
        elif len(lit) <= 16 and any(ch in lit for ch in " =：:;，；()[]≈"):
            parts.append(lit)
    joined = "".join(parts)
    if cjk.search(joined) and len(joined) >= 6:
        bullets.append((i + 1, joined, chunk[:120].replace("\n", " ")))
    i = j + 1

seen = set()
uniq = []
for ln, s, prev in bullets:
    if s in seen:
        continue
    seen.add(s)
    uniq.append((ln, s, prev))

uncovered = []
for ln, s, prev in uniq:
    ok, how = covered_bullet(s)
    if ok:
        continue
    # skip vendor_oracle / Johnson open / PDF
    if any(b in s for b in ("vendor_oracle", "PDF/A", "PDF/UA")):
        continue
    if "Johnson 变换后的 Pp/Ppk" in s:
        continue  # keep-gate honesty; user said skip Johnson open
    uncovered.append((ln, s))


def topic(s: str) -> str:
    keys = [
        ("pareto_attrib", ["累计占比", "类别", "80/20", "柏拉图", "原因计数", "Other 阈值"]),
        ("row_visibility", ["配置排除", "配置隐藏", "hidden", "excluded"]),
        ("inference_corr_anova", ["相关", "ANOVA", "Dixon", "Durbin", "自相关", "正态性", "热图", "成对"]),
        ("spc_mv", ["广义方差", "Hotelling", "MEWMA", "Sigma Z", "T²", "超限", "子组数", "控制图"]),
        ("msa", ["分级", "%Contribution", "%Study", "偏倚", "量具", "Gage", "Wheeler"]),
        ("capability", ["不合格品率", "能力", "Cpk", "过程合格", "非正态"]),
        ("reliability", ["失效模式", "cause-specific", "CIF", "删失", "可靠"]),
        ("regression_adf", ["单位根", "滞后", "有效回归", "ADF", "VIF", "Hosmer"]),
        ("doe_ccd", ["立方点", "星点", "中心点", "因子 =", "α ="]),
        ("equiv_power", ["功效", "等价", "TOST", "样本量"]),
        ("isolation_ml", ["树数", "标记异常", "分数阈值", "Isolation"]),
        ("acceptance_oc", ["OC", "接收概率", "抽样"]),
    ]
    for name, kws in keys:
        if any(k in s for k in kws):
            return name
    return "other"


by = Counter(topic(s) for _, s in uncovered)

# Param/chrome leftovers: parameter label rows with CJK not in exact
param_items = []
param_re = re.compile(
    r"(?:parameters\.push_back|summary\.push_back|add_parameter|"
    r"rows\.push_back|make_row|param_rows)"
)
# Broader: lines assigning Chinese labels in parameter tables
label_asgn = re.compile(
    r"(?:first|second|label|name|key|headers\.push_back|columns\.push_back|"
    r"cells\.push_back)\([^\"]*\"((?:\\.|[^\"\\])*)\""
)
# Also common pattern: {"中文", value}
kv = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,')

for rel in [
    "src/application/analysis_service.cpp",
    "src/application/output_builder.cpp",
    "src/application/doe_pages.cpp",
    "src/application/graph_service.cpp",
    "src/application/chart_pages.cpp",
]:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for ln, line in enumerate(text.splitlines(), 1):
        # parameter chrome: indented "中文 = " patterns often used in summary strings
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s):
                continue
            # likely param chrome shapes
            is_paramish = (
                s.strip().endswith("=")
                or s.strip().endswith("= ")
                or (s.startswith("    ") and "=" in s)
                or s in ("分布", "图", "判定", "截断", "方法", "目标", "权重", "预测", "排序")
            )
            if not is_paramish and "分布 =" not in s and "模型 =" not in s:
                continue
            if s in exact:
                continue
            if any(s.startswith(p) for p in sw if len(p) >= 4):
                continue
            # strip leading spaces for map key check
            st = s.strip()
            if st in exact or st.rstrip(" =") in exact:
                continue
            # facet
            if s.endswith("（分面）"):
                base = s[: -len("（分面）")]
                if base in exact:
                    continue
            param_items.append((rel, ln, s))

# Dedup params
ps = set()
params_u = []
for r, ln, s in param_items:
    if s in ps:
        continue
    ps.add(s)
    params_u.append((r, ln, s))

# Titles remaining
title_re = re.compile(
    r"(?:\.title|\.name|page\.title|table\.title|plot\.title)\s*=\s*\"((?:\\.|[^\"\\])*)\""
)
titles_u = []
for rel in [
    "src/application/analysis_service.cpp",
    "src/application/graph_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/chart_pages.cpp",
]:
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for ln, line in enumerate(text.splitlines(), 1):
        for m in title_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) and "Hexbin" not in s:
                continue
            if not cjk.search(s) and "（" not in s:
                continue
            base = s[: -len("（分面）")] if s.endswith("（分面）") else s
            if s in exact or base in exact:
                continue
            if any(s.startswith(p) or base.startswith(p) for p in sw if len(p) >= 4):
                continue
            titles_u.append((rel, ln, s))

# Diags remaining (fixed)
diags_u = []
for fp in (root / "src/domain/statistics").glob("*.cpp"):
    text = fp.read_text(encoding="utf-8", errors="replace")
    rel = str(fp.relative_to(root)).replace("\\", "/")
    for ln, line in enumerate(text.splitlines(), 1):
        if "message" not in line and "diagnostic" not in line.lower():
            continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 4:
                continue
            if s in exact or any(s.startswith(p) for p in sw if len(p) >= 6):
                continue
            if s.startswith(("，", "；", " ")) and len(s) < 24:
                continue
            if any(b in s for b in ("vendor_oracle", "Johnson 研究预览")):
                continue
            diags_u.append((rel, ln, s))
# app diags
for rel in ("src/application/analysis_service.cpp", "src/application/chart_pages.cpp"):
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for ln, line in enumerate(text.splitlines(), 1):
        if "message" not in line and "diagnostic" not in line.lower():
            continue
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s) or len(s) < 3:
                continue
            if s in exact or any(s.startswith(p) for p in sw if len(p) >= 4):
                continue
            if s.endswith(("。", "：", ":")) or "无法" in s or "失败" in s or "没有" in s:
                diags_u.append((rel, ln, s))

ds = set()
diags = []
for r, ln, s in diags_u:
    if s in ds:
        continue
    ds.add(s)
    diags.append((r, ln, s))

out = []
out.append(
    f"exact={len(exact)} sw={len(sw)} find_marks={len(find_marks)} "
    f"count_prefs={len(count_prefs)} uncovered_bullets={len(uncovered)} "
    f"params_u={len(params_u)} titles_u={len(titles_u)} diags_u={len(diags)}"
)
out.append("\n=== UNCOVERED BULLET TOPICS ===")
for k, v in by.most_common():
    out.append(f"  {v}\t{k}")

out.append("\n=== UNCOVERED BULLETS ===")
grouped = defaultdict(list)
for ln, s in uncovered:
    grouped[topic(s)].append((ln, s))
for t, n in by.most_common():
    out.append(f"\n## {t} n={n}")
    for ln, s in grouped[t]:
        out.append(f"  L{ln}\t{s}")

out.append(f"\n=== TITLES n={len(titles_u)} ===")
for r, ln, s in titles_u:
    out.append(f"  {r}:{ln}\t{s}")

out.append(f"\n=== DIAGS n={len(diags)} ===")
for r, ln, s in diags:
    out.append(f"  {r}:{ln}\t{s}")

out.append(f"\n=== PARAMS sample n={len(params_u)} ===")
# cluster params by family keywords
def pfam(s: str) -> str:
    for name, kws in [
        ("attrib_oc", ["二项", "泊松", "OC", "不合格", "缺陷", "Other"]),
        ("msa", ["零件", "操作", "过程变差", "Gage"]),
        ("doe", ["分辨度", "变体", "因子"]),
        ("dist_id", ["分布", "判定", "截断"]),
        ("prop", ["事件", "试验", "目标", "方法", "第一"]),
        ("anova", ["分组", "有效观测"]),
        ("misc", []),
    ]:
        if name == "misc" or any(k in s for k in kws):
            return name
    return "misc"

pc = Counter(pfam(s) for _, _, s in params_u)
out.append("param families: " + ", ".join(f"{k}:{v}" for k, v in pc.most_common()))
for fam, n in pc.most_common():
    out.append(f"\n## param/{fam} n={n}")
    for r, ln, s in [x for x in params_u if pfam(x[2]) == fam][:30]:
        out.append(f"  {r}:{ln}\t{s}")

# Propose ONE coherent slice: densest topic that can reach 15-25 with related chrome
# Candidate A: inference_corr_anova + related static honesty (correlation/ANOVA/Dixon/DW/ADF)
# Candidate B: attribute OC + Pareto + Laney (acceptance/quality chrome leftovers)
# Candidate C: param attribute distribution formulas + OC
# Candidate D: row_visibility + remaining honesty tails that are truly uncovered

# Build proposed slice: inference honesty pack + ADF/corr/anova/dixon/dw + heat map
inf_pack = []
for ln, s in uncovered:
    if topic(s) in ("inference_corr_anova", "regression_adf", "equiv_power"):
        inf_pack.append(("interp", f"src/application/interpretation_service.cpp:{ln}", s))

# Add related static exact-split leftovers that covered_bullet still flags nearby
# Also add related param/title if any

# Second pack: attribute/Pareto/Laney/OC quality narrative
qual_pack = []
for ln, s in uncovered:
    if topic(s) in ("pareto_attrib", "acceptance_oc", "spc_mv", "capability"):
        qual_pack.append(("interp", f"src/application/interpretation_service.cpp:{ln}", s))
for r, ln, s in params_u:
    if pfam(s) == "attrib_oc":
        qual_pack.append(("param", f"{r}:{ln}", s))

# Third: MSA leftover honesty
msa_pack = [("interp", f"src/application/interpretation_service.cpp:{ln}", s)
            for ln, s in uncovered if topic(s) == "msa"]

# Fourth: DOE/RSM hold/desirability fragments that are still uncovered as full bullets
doe_pack = [("interp", f"src/application/interpretation_service.cpp:{ln}", s)
            for ln, s in uncovered if topic(s) in ("doe_ccd",)]
for ln, s in uncovered:
    if topic(s) == "other" and any(k in s for k in ("Desirability", "hold", "等值线", "主效应", "显著项", "中心点")):
        doe_pack.append(("interp", f"src/application/interpretation_service.cpp:{ln}", s))

out.append("\n\n=== PACK SIZES ===")
out.append(f"inference_pack={len(inf_pack)}")
out.append(f"quality_attrib_pack={len(qual_pack)}")
out.append(f"msa_pack={len(msa_pack)}")
out.append(f"doe_pack={len(doe_pack)}")

# Enrich inference pack to 15-25 by pulling nearby related fragments that are
# still literal-unmapped but appear in same functions — read line contexts
# For quality pack similarly add Pareto title chrome if unmapped

# Check Pareto-related titles
for rel in ("src/application/analysis_service.cpp", "src/application/graph_service.cpp"):
    text = (root / rel).read_text(encoding="utf-8", errors="replace")
    for ln, line in enumerate(text.splitlines(), 1):
        for m in str_re.finditer(line):
            s = unesc(m.group(1))
            if not cjk.search(s):
                continue
            if any(k in s for k in ("柏拉图", "累计比例", "原因", "类别原因", "Other 阈值", "检查假设")):
                if s in exact or any(s.startswith(p) for p in sw if len(p) >= 4):
                    continue
                if ("title" in line or "header" in line or "name" in line or "axis" in line
                        or "push_back" in line or "=" in line):
                    qual_pack.append(("chrome", f"{rel}:{ln}", s))

# Dedup packs
def dedup(pack):
    seen = set()
    outp = []
    for kind, loc_, s in pack:
        if s in seen:
            continue
        seen.add(s)
        outp.append((kind, loc_, s))
    return outp

inf_pack = dedup(inf_pack)
qual_pack = dedup(qual_pack)

out.append(f"\ninference_pack_dedup={len(inf_pack)}")
out.append(f"quality_attrib_pack_dedup={len(qual_pack)}")

# If quality pack is densest in 15-25 range, pick it; else inference; else merge row_vis+inference+msa
row_pack = [("interp", f"src/application/interpretation_service.cpp:{ln}", s)
            for ln, s in uncovered if topic(s) == "row_visibility"]

# Isolation forest + ML leftovers
ml_pack = [("interp", f"src/application/interpretation_service.cpp:{ln}", s)
           for ln, s in uncovered if topic(s) == "isolation_ml"]

# Reliability leftovers
rel_pack = [("interp", f"src/application/interpretation_service.cpp:{ln}", s)
            for ln, s in uncovered if topic(s) == "reliability"]

# Build hybrid "interpretation honesty leftovers Wave-N" if needed
hybrid = dedup(inf_pack + row_pack + msa_pack + ml_pack + rel_pack + doe_pack)
# add remaining other uncovered
for ln, s in uncovered:
    hybrid.append(("interp", f"src/application/interpretation_service.cpp:{ln}", s))
hybrid = dedup(hybrid)

out.append(f"hybrid_all_uncovered={len(hybrid)}")

# Pick best pack in 15-25
candidates = [
    ("quality_attrib_oc_pareto_laney", qual_pack),
    ("inference_corr_anova_adf_dw", inf_pack),
    ("hybrid_interp_honesty_leftovers", hybrid[:25]),
]
out.append("\n=== CANDIDATE PACKS ===")
for name, pack in candidates:
    out.append(f"\n### {name} n={len(pack)}")
    for kind, loc_, s in pack[:30]:
        out.append(f"  {kind}\t{loc_}\t{s}")

path = root / "scripts/_tmp_true_next_slice_out.txt"
path.write_text("\n".join(out), encoding="utf-8")
print(f"wrote {path}")
print(f"uncovered={len(uncovered)} topics={by.most_common(8)}")
print(f"qual={len(qual_pack)} inf={len(inf_pack)} hybrid={len(hybrid)} params={len(params_u)}")
