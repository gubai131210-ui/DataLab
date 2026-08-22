# -*- coding: utf-8 -*-
"""Quick residual ZH cluster scan for en-US dual-line leftovers (read-only)."""
import re
import pathlib
from collections import Counter

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
catalog = (root / "src/domain/report_text_catalog.cpp").read_text(encoding="utf-8")

exact_pairs = re.findall(
    r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)+)"\s*\}', loc
)
exact_zh = {a for a, _ in exact_pairs if re.search(r"[\u4e00-\u9fff]", a)}
startswith_zh = set(
    re.findall(r'starts_with\([^,]+,\s*"([^"]*[\u4e00-\u9fff][^"]*)"', loc)
)
find_zh = set(re.findall(r'\.find\(\s*"([^"]*[\u4e00-\u9fff][^"]*)"', loc))
endswith_zh = set(
    re.findall(r'ends_with\([^,]+,\s*"([^"]*[\u4e00-\u9fff][^"]*)"', loc)
)

cat_zh = set()
for m in re.finditer(
    r'\{\s*"([^"]+)"\s*,\s*\n\s*"([^"]*[\u4e00-\u9fff][^"]*)"\s*,\s*\n\s*"([^"]*)"',
    catalog,
):
    cat_zh.add(m.group(2))
for m in re.finditer(
    r'\{\s*"([^"]+)"\s*,\s*"([^"]*[\u4e00-\u9fff][^"]*)"\s*,\s*"([^"]*)"\s*\}',
    catalog,
):
    cat_zh.add(m.group(2))

param_tokens = [
    a
    for a, b in exact_pairs
    if re.search(r"[\u4e00-\u9fff]", a)
    and (
        b.startswith("param.summary.")
        or b.startswith("graph.caption.")
        or b.startswith("page.")
        or b.startswith("plot.")
        or b.startswith("diag.")
        or b.startswith("header.")
        or b.startswith("table.")
        or b.startswith("series.")
        or b.startswith("metric.")
        or b.startswith("axis.")
        or b.startswith("chrome.")
        or b.startswith("interp.")
    )
]
param_tokens.sort(key=len, reverse=True)


def covered_token(s: str) -> bool:
    if s in exact_zh or s.lstrip() in exact_zh or s in cat_zh:
        return True
    for t in startswith_zh:
        if s.startswith(t) or s.lstrip().startswith(t):
            return True
    for t in endswith_zh:
        if s.endswith(t):
            return True
    for t in find_zh:
        if t and t in s:
            return True
    for t in param_tokens:
        if not t:
            continue
        if s == t or s.lstrip() == t or s.startswith(t) or s.lstrip().startswith(t):
            return True
        if len(t) >= 4 and t in s:
            return True
    return False


def fam(s: str, path: str) -> str:
    if any(
        x in s
        for x in [
            "显示 N",
            "分析 N",
            "分面",
            "（分面）",
            "带宽",
            "须线",
            "分箱",
            "Hexbin",
            "密度图",
            "小提琴",
            "条形图",
            "热图",
            "散点图",
            "矩阵图",
        ]
    ):
        return "graph_eda_facet"
    if any(
        x in s
        for x in [
            "不合格",
            "缺陷",
            "二项",
            "泊松",
            "柏拉图",
            "原因",
            "检验数",
            "子组数",
            "分布 =",
        ]
    ):
        return "attrib_spc_oc"
    if any(
        x in s
        for x in ["操作", "零件", "评估", "量具", "偏倚", "Gage", "ndc", "Kappa"]
    ):
        return "msa"
    if any(
        x in s
        for x in [
            "因子",
            "区组",
            "设计",
            "分辨",
            "编码",
            "星点",
            "中心点",
            "RSM",
            "CCD",
            "BBD",
        ]
    ):
        return "doe_rsm"
    if any(
        x in s
        for x in ["寿命", "删失", "Weibull", "失效", "保证", "暴露", "KM", "CIF"]
    ):
        return "reliability"
    if any(x in s for x in ["控制", "超限", "Sigma", "EWMA", "CUSUM", "特殊原因", "规则"]):
        return "spc"
    if any(x in s for x in ["能力", "Cpk", "Ppk"]):
        return "capability"
    if any(
        x in s
        for x in [
            "假设",
            "样本",
            "相关",
            "功效",
            "比例",
            "方差",
            "均值",
            "组 1",
            "组 2",
            "中位数",
            "t 检验",
            "Z 检验",
            "残差",
        ]
    ):
        return "inference"
    if any(x in s for x in ["预测", "周期", "滞后", "季节", "准则", "窗宽", "预报"]):
        return "forecast_ts"
    if any(x in s for x in ["迭代", "叶数", "深度", "选入", "得分", "聚类", "树"]):
        return "ml"
    if s.startswith("请") or "请选择" in s or "请指定" in s or "请输入" in s:
        return "gate_qing"
    if "domain" in path.replace("\\", "/"):
        return "domain_diag"
    return "misc_param_chrome"


# Already-closed thin slices (exclude from residual ranking)
CLOSED_MARKERS = [
    "的正态概率图",
    "的直方图",
    "的个体值图",
    "的散点图",
    " 与 ",
    " 运行图",
    "部分析因分辨度",
    "需要单位数列",
    "处理 = ",
    "区组 = ",
    "缺失 = ",
    "预测期数 = ",
    "因子 A = ",
    "因子 B = ",
    "移动极差长度 = ",
    "正态概率图相关系数 = ",
]


def is_closed(s: str) -> bool:
    for m in CLOSED_MARKERS:
        if m in s or s.strip() == m.strip():
            return True
    # METHOD skip style short "跳过 "
    if s.strip() in ("跳过 ", "跳过", "描述统计跳过 "):
        return True
    if s.startswith("请") and any(
        x in s for x in ["选择", "指定", "输入", "提供"]
    ):
        return True
    return False


files = {}
for rel in [
    "src/application/analysis_service.cpp",
    "src/application/graph_service.cpp",
    "src/application/doe_pages.cpp",
    "src/application/chart_pages.cpp",
    "src/application/output_builder.cpp",
    "src/application/interpretation_service.cpp",
]:
    files[rel] = root / rel
for p in (root / "src/domain").rglob("*.cpp"):
    if p.name in ("report_text_catalog.cpp",):
        continue
    rel = str(p.relative_to(root)).replace("\\", "/")
    files[rel] = p

lit = re.compile(r'"((?:[^"\\]|\\.)*)"')
cjk = re.compile(r"[\u4e00-\u9fff]")

unmapped = []
for rel, path in files.items():
    text = path.read_text(encoding="utf-8", errors="ignore")
    for i, line in enumerate(text.splitlines(), 1):
        if not cjk.search(line) or line.lstrip().startswith("//"):
            continue
        for s in lit.findall(line):
            if not cjk.search(s) or len(s) < 2:
                continue
            if is_closed(s) or covered_token(s):
                continue
            # ignore UI/menu-only noise heuristics: very long help? keep report-ish
            unmapped.append((fam(s, rel), rel, i, s))

uniq = {}
for f, p, i, s in unmapped:
    if s not in uniq:
        uniq[s] = (f, p, i, s)

cf = Counter(v[0] for v in uniq.values())
out = root / "scripts/_tmp_residual_cluster_scan_out.txt"
with out.open("w", encoding="utf-8") as fh:
    fh.write(f"UNIQ_UNMAPPED {len(uniq)}\n")
    for k, v in cf.most_common(20):
        fh.write(f"  {v}\t{k}\n")
    for rank, (top, _) in enumerate(cf.most_common(3), 1):
        items = [v for v in uniq.values() if v[0] == top]
        fh.write(f"\n=== #{rank} FAMILY {top} n={len(items)} ===\n")
        # subcluster by first 4-8 CJK
        sc = Counter()
        for _, _, _, s in items:
            chars = "".join(cjk.findall(s))[:8] or s[:16]
            sc[chars] += 1
        fh.write("subclusters:\n")
        for k, v in sc.most_common(15):
            fh.write(f"  {v}\t{k}\n")
        fh.write("samples:\n")
        for _, p, i, s in sorted(items, key=lambda x: (-len(x[3]), x[1], x[2]))[:25]:
            fh.write(f"  {p}:{i}\t{s}\n")

print(f"wrote {out}")
print(f"UNIQ_UNMAPPED {len(uniq)}")
for k, v in cf.most_common(10):
    print(f"  {v}\t{k}")
