import re, pathlib
from collections import Counter, defaultdict

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")

# Build maps
exact_pairs = re.findall(r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)+)"\s*\}', loc)
exact_zh = {a for a,b in exact_pairs if re.search(r"[\u4e00-\u9fff]", a)}
param_tokens = {a for a,b in exact_pairs if b.startswith("param.summary.")}
header_tokens = {a for a,b in exact_pairs if b.startswith("table.") or b.startswith("header.")}
plot_tokens = {a for a,b in exact_pairs if b.startswith("plot.") or b.startswith("series.")}
page_tokens = {a for a,b in exact_pairs if b.startswith("page.")}

# starts_with prefixes used in localize_parameter_summary and related
# Read localize_parameter_summary section
m = re.search(r"void localize_parameter_summary\([\s\S]*?\n\}", loc)
param_section = m.group(0) if m else ""
param_sw = set(re.findall(r'"((?:[^"\\]|\\.)+)"\s*,\s*"param\.summary\.', param_section))
# also generic replace loops
all_param_pref = set(re.findall(r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"param\.summary\.[^"]+"', loc))

def covered_param(s):
    if s in exact_zh or s in all_param_pref:
        return True
    for t in sorted(all_param_pref, key=len, reverse=True):
        if len(t) >= 2 and (s.startswith(t) or t in s):
            return True
    return False

files = [
    root/"src/application/analysis_service.cpp",
    root/"src/application/graph_service.cpp",
    root/"src/application/doe_pages.cpp",
    root/"src/application/chart_pages.cpp",
]

# Extract Chinese string literals that look like dual-line chrome: param lines, series names, status
zh_lit = re.compile(r'"((?:[^"\\]|\\.)*)"')
cjk = re.compile(r"[\u4e00-\u9fff]")

# Focus: lines building parameter_summary / series / status / header
keywords = [
    "parameter_summary", "ParameterSummary", "summary_lines", "summary.push",
    "series_name", "SeriesName", "series.name", "setName",
    "status_message", "status =", "Status",
    "header", "column_headers", "headers.push",
    "axis_label", "title =", "page_title",
]

unmapped = []
mapped = 0
for fp in files:
    text = fp.read_text(encoding="utf-8")
    lines = text.splitlines()
    for i, line in enumerate(lines, 1):
        if not cjk.search(line):
            continue
        # skip comments
        stripped = line.lstrip()
        if stripped.startswith("//") or stripped.startswith("/*"):
            continue
        # skip 请* gate diagnostics - user says mapped
        # still collect if interesting
        for s in zh_lit.findall(line):
            if not cjk.search(s):
                continue
            if len(s) < 2:
                continue
            # classify
            kind = "misc"
            if "请" in s and ("选择" in s or "指定" in s or "输入" in s or "提供" in s):
                kind = "qing_gate"
            elif any(k in line for k in ["parameter_summary", "summary +=", "summary.append", "oss <<", "lines.push", "push_back(param", "parameter_lines"]):
                kind = "param"
            elif any(k in line.lower() for k in ["series", "setname", "legend"]):
                kind = "series"
            elif "header" in line.lower() or "column" in line.lower():
                kind = "header"
            elif "title" in line.lower() or "page_title" in line.lower():
                kind = "title"
            elif "status" in line.lower() or "diagnostic" in line.lower() or "message" in line.lower():
                kind = "status_diag"
            else:
                # heuristic: looks like key= value chrome
                if " = " in s or s.strip().endswith("=") or s.strip().endswith(" ="):
                    kind = "param_like"
                else:
                    continue  # skip narrative/other for this pass
            if kind == "qing_gate":
                continue
            if covered_param(s) or s in exact_zh:
                mapped += 1
                continue
            # also check starts_with against param tokens
            hit = False
            for t in all_param_pref:
                if s.startswith(t) or (len(t)>=4 and t in s):
                    hit = True
                    break
            if hit:
                mapped += 1
                continue
            unmapped.append((str(fp.relative_to(root)).replace("\\","/"), i, kind, s))

fam = Counter()
for _,_,_,s in unmapped:
    if any(x in s for x in ["分布", "不合格", "缺陷", "二项", "泊松", "柏拉图", "原因", "OC", "检验数"]):
        fam["attrib_spc"] += 1
    elif any(x in s for x in ["操作", "零件", "量具", "Gage", "偏倚", "评估"]):
        fam["msa"] += 1
    elif any(x in s for x in ["因子", "分辨", "区组", "Desirability", "编码", "星点"]):
        fam["doe_rsm"] += 1
    elif any(x in s for x in ["寿命", "删失", "Weibull", "保证", "失效"]):
        fam["reliability"] += 1
    elif any(x in s for x in ["相关", "假设", "样本", "功效", "方差", "均值", "比例"]):
        fam["inference"] += 1
    elif any(x in s for x in ["预测", "滞后", "周期", "季节"]):
        fam["forecast_ts"] += 1
    else:
        fam["misc"] += 1

out = root/"scripts/_tmp_phase3_param_gap.txt"
with out.open("w", encoding="utf-8") as f:
    f.write(f"mapped={mapped} unmapped={len(unmapped)}\n")
    f.write("FAMILY\n")
    for k,v in fam.most_common():
        f.write(f"  {v}\t{k}\n")
    f.write("BY_KIND\n")
    ck = Counter(k for _,_,k,_ in unmapped)
    for k,v in ck.most_common():
        f.write(f"  {v}\t{k}\n")
    f.write("ITEMS\n")
    for fp,i,k,s in unmapped[:120]:
        f.write(f"{fp}:{i}\t{k}\t{s}\n")
print(f"mapped={mapped} unmapped={len(unmapped)} fam={dict(fam)} kinds={dict(ck)}")
