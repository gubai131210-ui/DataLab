import re, pathlib
from collections import Counter

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
anal = (root / "src/application/analysis_service.cpp").read_text(encoding="utf-8")
graph = (root / "src/application/graph_service.cpp").read_text(encoding="utf-8")
doe = (root / "src/application/doe_pages.cpp").read_text(encoding="utf-8")
chart = (root / "src/application/chart_pages.cpp").read_text(encoding="utf-8")

# Collect all ZH tokens used in param/caption localization (both param.summary and graph.caption)
tokens = []
for a,b in re.findall(r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)+)"\s*\}', loc):
    if re.search(r"[\u4e00-\u9fff]", a) and (b.startswith("param.summary.") or b.startswith("graph.caption.") or b.startswith("page.") or b.startswith("plot.") or b.startswith("header.") or b.startswith("table.") or b.startswith("series.") or b.startswith("metric.")):
        tokens.append((a,b))

# Also starts_with style in localize_parameter_summary body - tokens above should be enough if replace_all walks them

def is_covered(s):
    s2 = s.lstrip()
    for t,_ in tokens:
        if s2 == t or s2.startswith(t) or (len(t)>=3 and t in s2):
            return True
        if s == t or s.startswith(t):
            return True
    # exact diag/interp maps
    return False

exact_all = {a for a,_ in re.findall(r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)+)"\s*\}', loc) if re.search(r"[\u4e00-\u9fff]", a)}

def covered(s):
    if s in exact_all or s.lstrip() in exact_all:
        return True
    return is_covered(s)

# Focus: parameter_summary construction sites - look for lines assigning to parameter_summary or appending summary strings
# Broader: Chinese literals with " = " pattern in analysis/graph/doe/chart

lit = re.compile(r'"((?:[^"\\]|\\.)*)"')
cjk = re.compile(r"[\u4e00-\u9fff]")

unmapped = []
for name, text in [("analysis", anal), ("graph", graph), ("doe", doe), ("chart", chart)]:
    for i, line in enumerate(text.splitlines(), 1):
        if not cjk.search(line):
            continue
        if line.lstrip().startswith("//"):
            continue
        for s in lit.findall(line):
            if not cjk.search(s):
                continue
            # param-like only
            if not (("=" in s) or s.strip().endswith(":") or "跳过" in s or "分布" in s):
                continue
            if "请" in s and any(x in s for x in ["选择","指定","输入","提供"]):
                continue  # gates done
            if covered(s):
                continue
            unmapped.append((name, i, s))

# Deduplicate by string
uniq = {}
for name,i,s in unmapped:
    key = s.strip()
    uniq.setdefault(key, []).append(f"{name}:{i}")

# Family
def fam(s):
    if any(x in s for x in ["操作","零件","评估","量具","偏倚","Gage"]): return "msa"
    if any(x in s for x in ["因子","区组","设计","分辨","编码","Desir","星点","中心点"]): return "doe_rsm"
    if any(x in s for x in ["寿命","删失","Weibull","失效","保证","暴露"]): return "reliability"
    if any(x in s for x in ["分布","不合格","缺陷","二项","泊松","柏拉图","原因","检验数","子组"]): return "attrib_spc"
    if any(x in s for x in ["显示 N","分析 N","分面","带宽","须线","分箱"]): return "graph_eda"
    if any(x in s for x in ["假设","样本","相关","功效","比例","方差","均值","组 1","组 2","中位数","处理"]): return "inference"
    if any(x in s for x in ["预测","周期","滞后","季节","准则","窗宽"]): return "forecast_ts"
    if any(x in s for x in ["控制限","超限","Sigma","移动极差","EWMA","CUSUM"]): return "spc"
    if any(x in s for x in ["迭代","叶数","深度","选入","得分","聚类","树"]): return "ml"
    return "misc"

cf = Counter(fam(s) for s in uniq)
out = root/"scripts/_tmp_phase3_param_true.txt"
with out.open("w", encoding="utf-8") as f:
    f.write(f"uniq_unmapped_paramish={len(uniq)}\n")
    for k,v in cf.most_common():
        f.write(f"  {v}\t{k}\n")
    for s, locs in sorted(uniq.items(), key=lambda kv: (-len(kv[1]), kv[0]))[:80]:
        f.write(f"{fam(s)}\t{locs[0]}\tn={len(locs)}\t{s}\n")
print("uniq", len(uniq), dict(cf))
for k,v in cf.most_common():
    print(f"  {v}\t{k}")
