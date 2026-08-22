# -*- coding: utf-8 -*-
"""Thin residual bilingual cluster scan — report surface only."""
import re
import pathlib
from collections import Counter, defaultdict

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
# also replace_all / contains patterns like bullet.find("；
contains_lit = set(
    re.findall(r'(?:replace|find|starts_with|ends_with)\([^"]*"([^"]*[\u4e00-\u9fff][^"]*)"', loc)
)

cat_ids = {}
for m in re.finditer(
    r'\{\s*"([^"]+)"\s*,\s*\n\s*"([^"]*)"\s*,\s*\n\s*"([^"]*)"',
    catalog,
):
    cat_ids[m.group(1)] = (m.group(2), m.group(3))
for m in re.finditer(
    r'\{\s*"([^"]+)"\s*,\s*"([^"]*)"\s*,\s*"([^"]*)"\s*\}',
    catalog,
):
    cat_ids[m.group(1)] = (m.group(2), m.group(3))
cat_zh = {zh for zh, en in cat_ids.values() if re.search(r"[\u4e00-\u9fff]", zh)}

# All mapped ZH from exact pairs regardless of id prefix
all_mapped_zh = set(exact_zh) | set(startswith_zh) | set(find_zh) | set(endswith_zh) | contains_lit | cat_zh
# sort longest first for substring cover
mapped_sorted = sorted(all_mapped_zh, key=len, reverse=True)


def covered(s: str) -> bool:
    s2 = s.lstrip()
    if s in all_mapped_zh or s2 in all_mapped_zh:
        return True
    for t in mapped_sorted:
        if not t or len(t) < 2:
            continue
        if s.startswith(t) or s2.startswith(t) or s.endswith(t):
            return True
        # meaningful substring (avoid 1-char)
        if len(t) >= 4 and t in s:
            return True
    return False


CLOSED = [
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
    "显示 N = ",
    "分析 N = ",
    "分析 N(水平) = ",
    "分面 = ",
    "（分面）",
]


def is_closed(s: str) -> bool:
    for m in CLOSED:
        if m in s:
            return True
    if s.strip() in ("跳过 ", "跳过", "描述统计跳过 ", " 跳过 "):
        return True
    if s.startswith("请") and any(x in s for x in ["选择", "指定", "输入", "提供"]):
        return True
    # user: do not suggest johnson/vendor_oracle
    if "vendor_oracle" in s or "Johnson" in s or "johnson" in s:
        return True
    if "formula_reference" in s:
        return True
    return False


def is_thin(s: str) -> bool:
    """Thin leftover: short title/param/diag chrome, not long honesty bullets."""
    if len(s) > 80:
        return False
    cjk_n = len(re.findall(r"[\u4e00-\u9fff]", s))
    if cjk_n < 1:
        return False
    # prefer param-like / title-like
    thin_hints = (
        "=" in s
        or s.endswith("：")
        or s.endswith(":")
        or "图" in s
        or "（" in s
        or s.endswith("。")
        and len(s) < 40
        or any(
            x in s
            for x in [
                "分布",
                "子组",
                "阶段",
                "原因",
                "类别",
                "组数",
                "中位数",
                "比例",
                "迭代",
                "缺失",
                "间隔",
                "编码",
                "没有可",
                "无法计算",
                "当前未",
                "AD ",
                "判定",
                "权重",
                "排序",
                "预测",
                "第一组",
                "第二组",
                "组 1",
                "组 2",
                "列水平",
                "截断",
                "N（",
                "因子 1",
            ]
        )
    )
    return thin_hints


def cluster_key(s: str) -> str:
    if any(x in s for x in ["分布 =", "二项", "泊松", "不合格", "缺陷", "柏拉图", "原因"]):
        return "attrib_spc_dist_param"
    if any(x in s for x in ["阶段", "AD 判定", "检查假设"]):
        return "spc_stage_assumption_param"
    if any(x in s for x in ["第一组", "第二组", "组 1", "组 2", "列水平"]):
        return "inference_group_param"
    if any(x in s for x in ["（编码）", "编码 ", "因子 1"]):
        return "doe_coding_chrome"
    if any(x in s for x in ["没有可显示", "无法计算", "当前未触发", "诊断无关联", "门禁状态"]):
        return "empty_diag_chrome"
    if any(x in s for x in ["权重", "排序", "预测下限", "预测上限", "预测", "间隔", "截断", "N（可选）"]):
        return "misc_table_header_param"
    if "图" in s or "（分面）" in s or "Hexbin" in s:
        return "plot_title_leftover"
    if any(x in s for x in ["中位数", "组数", "比例 =", "迭代", "缺失值"]):
        return "misc_eq_param"
    return "other_thin"


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
    if p.name == "report_text_catalog.cpp":
        continue
    files[str(p.relative_to(root)).replace("\\", "/")] = p

lit = re.compile(r'"((?:[^"\\]|\\.)*)"')
cjk = re.compile(r"[\u4e00-\u9fff]")

hits = []
for rel, path in files.items():
    text = path.read_text(encoding="utf-8", errors="ignore")
    for i, line in enumerate(text.splitlines(), 1):
        if not cjk.search(line) or line.lstrip().startswith("//"):
            continue
        for s in lit.findall(line):
            if not cjk.search(s):
                continue
            if is_closed(s) or covered(s) or not is_thin(s):
                continue
            hits.append((cluster_key(s), rel, i, s))

uniq = {}
for ck, rel, i, s in hits:
    if s not in uniq:
        uniq[s] = (ck, rel, i, s)

cc = Counter(v[0] for v in uniq.values())
out = root / "scripts/_tmp_residual_thin_out.txt"
with out.open("w", encoding="utf-8") as fh:
    fh.write(f"THIN_UNIQ {len(uniq)}\n")
    for k, v in cc.most_common():
        fh.write(f"  {v}\t{k}\n")
    for ck, n in cc.most_common(5):
        items = [v for v in uniq.values() if v[0] == ck]
        fh.write(f"\n=== {ck} n={len(items)} ===\n")
        for _, rel, i, s in sorted(items, key=lambda x: (x[1], x[2]))[:20]:
            fh.write(f"{rel}:{i}\t{s}\n")

print(f"wrote {out} THIN_UNIQ={len(uniq)}")
for k, v in cc.most_common():
    print(f"  {v}\t{k}")
