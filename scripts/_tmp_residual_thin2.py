# -*- coding: utf-8 -*-
import re
import pathlib
from collections import Counter

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
anal = (root / "src/application/analysis_service.cpp").read_text(encoding="utf-8")
graph = (root / "src/application/graph_service.cpp").read_text(encoding="utf-8")
doe = (root / "src/application/doe_pages.cpp").read_text(encoding="utf-8")
chart = (root / "src/application/chart_pages.cpp").read_text(encoding="utf-8")
outb = (root / "src/application/output_builder.cpp").read_text(encoding="utf-8")
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")

# param.summary tokens from localize_parameter_summary
m = re.search(
    r"void localize_parameter_summary\([\s\S]*?\n\}\n\n(?:void |ReportLocalization)",
    loc,
)
body = m.group(0) if m else ""
param_toks = [
    a
    for a, b in re.findall(r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)+)"\s*\}', body)
    if b.startswith("param.summary.") or b.startswith("graph.caption.")
]
param_toks += ["显示 N = ", "分析 N = ", "分析 N(水平) = ", "分面 = "]
param_toks = sorted(set(param_toks), key=len, reverse=True)

# title/page/plot/header/metric tokens
title_toks = [
    a
    for a, b in re.findall(r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)+)"\s*\}', loc)
    if b.startswith(("page.", "plot.", "header.", "metric.", "series.", "axis.", "table.", "status."))
]
# suffix handlers
suffix_handlers = [
    "的正态概率图",
    "的直方图",
    "的个体值图",
    "的散点图",
    " 运行图",
    "（分面）",
    "（编码）",
]

CLOSED = set(param_toks) | {
    "处理 = ",
    "区组 = ",
    "缺失 = ",
    "预测期数 = ",
    "因子 A = ",
    "因子 B = ",
    "移动极差长度 = ",
    "正态概率图相关系数 = ",
    "跳过 ",
    " 跳过 ",
    "描述统计跳过 ",
}


def param_covered(s: str) -> bool:
    s2 = s.lstrip()
    for t in param_toks:
        if not t:
            continue
        if s2.startswith(t) or t in s:
            return True
    for t in CLOSED:
        if t and t in s:
            return True
    return False


lit = re.compile(r'"((?:[^"\\]|\\.)*)"')
cjk = re.compile(r"[\u4e00-\u9fff]")

# 1) Unmapped param-ish in analysis/graph/doe/chart
param_hits = []
for name, text in [
    ("analysis_service.cpp", anal),
    ("graph_service.cpp", graph),
    ("doe_pages.cpp", doe),
    ("chart_pages.cpp", chart),
]:
    for i, line in enumerate(text.splitlines(), 1):
        if not cjk.search(line) or line.lstrip().startswith("//"):
            continue
        for s in lit.findall(line):
            if not cjk.search(s):
                continue
            if not (("=" in s) or ("跳过" in s) or ("分布" in s)):
                continue
            if len(s) > 100:
                continue
            if s.startswith("请") and any(
                x in s for x in ["选择", "指定", "输入", "提供"]
            ):
                continue
            if "vendor_oracle" in s or "formula_reference" in s:
                continue
            if param_covered(s):
                continue
            param_hits.append((name, i, s))

# 2) Unmapped short titles (page.title / plot title literals)
title_mapped = set(title_toks)
title_hits = []
for name, text in [
    ("analysis_service.cpp", anal),
    ("graph_service.cpp", graph),
    ("doe_pages.cpp", doe),
    ("chart_pages.cpp", chart),
    ("output_builder.cpp", outb),
]:
    for i, line in enumerate(text.splitlines(), 1):
        if "title" not in line and "header" not in line and "name =" not in line:
            # still catch page.title assignments
            if "page.title" not in line and ".title =" not in line and "headers" not in line:
                continue
        if not cjk.search(line) or line.lstrip().startswith("//"):
            continue
        for s in lit.findall(line):
            if not cjk.search(s):
                continue
            if any(sfx in s for sfx in suffix_handlers):
                # may still be covered by suffix rewrite; skip closed plot suffixes
                if any(
                    x in s
                    for x in [
                        "的正态概率图",
                        "的直方图",
                        "的个体值图",
                        "的散点图",
                        "运行图",
                        "（分面）",
                    ]
                ):
                    continue
            if s in title_mapped or any(t == s for t in title_mapped):
                continue
            # covered if any title token equals or is contained as full title
            if any(t and (s == t or s.endswith(t)) for t in title_toks if len(t) >= 2):
                continue
            if len(s) > 60:
                continue
            title_hits.append((name, i, s))

# 3) Domain/app diag short lines not in exact diag map
diag_exact = {
    a
    for a, b in re.findall(r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)+)"\s*\}', loc)
    if b.startswith("diag.")
}
# also localize_known_plain / message maps include non-diag ids
plain_exact = {
    a
    for a, b in re.findall(r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)+)"\s*\}', loc)
    if re.search(r"[\u4e00-\u9fff]", a)
}

diag_hits = []
for p in (root / "src/domain").rglob("*.cpp"):
    if p.name == "report_text_catalog.cpp":
        continue
    text = p.read_text(encoding="utf-8", errors="ignore")
    rel = str(p.relative_to(root)).replace("\\", "/")
    for i, line in enumerate(text.splitlines(), 1):
        if not cjk.search(line) or line.lstrip().startswith("//"):
            continue
        for s in lit.findall(line):
            if not cjk.search(s):
                continue
            if s in plain_exact:
                continue
            if any(s.startswith(t) for t in sorted(plain_exact, key=len, reverse=True) if len(t) >= 6):
                continue
            if "vendor_oracle" in s or "formula_reference" in s or "Johnson" in s:
                continue
            if len(s) > 70 or len(s) < 4:
                continue
            # thin diag-ish
            if not (
                s.endswith("。")
                or "=" in s
                or "不可" in s
                or "无法" in s
                or "需要" in s
                or "至少" in s
            ):
                continue
            diag_hits.append((rel, i, s))

# Cluster params
def pfam(s):
    if any(x in s for x in ["分布", "不合格", "缺陷", "二项", "泊松", "原因", "子组"]):
        return "attrib_spc"
    if any(x in s for x in ["组 1", "组 2", "列水平", "第一组", "第二组"]):
        return "inference_groups"
    if any(x in s for x in ["阶段", "AD", "假设"]):
        return "spc_capability_meta"
    if any(x in s for x in ["迭代", "叶", "深度", "选入", "聚类", "得分"]):
        return "ml"
    if any(x in s for x in ["因子", "区组", "设计", "编码", "中心"]):
        return "doe"
    if any(x in s for x in ["寿命", "暴露", "删失", "事件"]):
        return "reliability"
    return "misc"


# dedupe params
pu = {}
for n, i, s in param_hits:
    pu.setdefault(s, (n, i, s))
pc = Counter(pfam(s) for s in pu)
tu = {}
for n, i, s in title_hits:
    tu.setdefault(s, (n, i, s))
du = {}
for n, i, s in diag_hits:
    du.setdefault(s, (n, i, s))

out = root / "scripts/_tmp_residual_thin2_out.txt"
with out.open("w", encoding="utf-8") as fh:
    fh.write(f"PARAM_UNIQ {len(pu)} families={dict(pc)}\n")
    for fam, n in pc.most_common():
        fh.write(f"\n## param/{fam} n={n}\n")
        items = [v for v in pu.values() if pfam(v[2]) == fam]
        for n, i, s in sorted(items, key=lambda x: x[0:2])[:20]:
            fh.write(f"src/application/{n}:{i}\t{s}\n")
    fh.write(f"\nTITLE_UNIQ {len(tu)}\n")
    for n, i, s in sorted(tu.values(), key=lambda x: x[0:2])[:30]:
        fh.write(f"src/application/{n}:{i}\t{s}\n")
    fh.write(f"\nDOMAIN_DIAG_THIN_UNIQ {len(du)}\n")
    # cluster domain diags
    def dfam(s):
        if "特殊原因" in s or "超限" in s or "控制" in s:
            return "spc"
        if "Gage" in s or "量具" in s or "操作" in s:
            return "msa"
        if "因子" in s or "设计" in s or "编码" in s:
            return "doe"
        if "删失" in s or "寿命" in s or "失效" in s:
            return "reliability"
        if "能力" in s or "Cpk" in s or "正态" in s:
            return "capability"
        return "other"

    dc = Counter(dfam(s) for s in du)
    fh.write(f"diag_families={dict(dc)}\n")
    for fam, n in dc.most_common(5):
        fh.write(f"\n## diag/{fam} n={n}\n")
        items = [v for v in du.values() if dfam(v[2]) == fam]
        for n, i, s in sorted(items, key=lambda x: (-len(x[2]), x[0], x[1]))[:15]:
            fh.write(f"{n}:{i}\t{s}\n")

def dfam_top(s):
    if "特殊原因" in s or "超限" in s or "控制" in s:
        return "spc"
    if "Gage" in s or "量具" in s or "操作" in s:
        return "msa"
    if "因子" in s or "设计" in s or "编码" in s:
        return "doe"
    if "删失" in s or "寿命" in s or "失效" in s:
        return "reliability"
    if "能力" in s or "Cpk" in s or "正态" in s:
        return "capability"
    return "other"


print("wrote", out)
print("PARAM", len(pu), dict(pc))
print("TITLE", len(tu))
print("DIAG", len(du), dict(Counter(dfam_top(s) for s in du)))
