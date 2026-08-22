import re, pathlib
from collections import Counter

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")

exact = {a for a,b in re.findall(r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)+)"\s*\}', loc) if re.search(r"[\u4e00-\u9fff]", a)}

# Strict markers only
MIN = 8
sw = {x for x in re.findall(r'starts_with\(\s*(?:bullet|body)\s*,\s*"((?:[^"\\]|\\.)+)"', loc) if re.search(r"[\u4e00-\u9fff]", x) and len(x)>=MIN}
sw |= {x for x in re.findall(r'parse_leading_count_after_prefix\(\s*bullet\s*,\s*"((?:[^"\\]|\\.)+)"', loc) if len(x)>=4}
# Inline handlers in else-if chains (may not appear in exact map)
sw |= {x for x in re.findall(r'else if \(starts_with\(bullet,\s*"((?:[^"\\]|\\.)+)"\)\)', loc) if re.search(r"[\u4e00-\u9fff]", x) and len(x)>=MIN}
finds = {x for x in re.findall(r'(?:bullet|body)\.find\(\s*"((?:[^"\\]|\\.)+)"', loc) if re.search(r"[\u4e00-\u9fff]", x) and len(x)>=MIN}
ends = {x for x in re.findall(r'ends_with\(\s*bullet\s*,\s*"((?:[^"\\]|\\.)+)"', loc) if re.search(r"[\u4e00-\u9fff]", x) and len(x)>=MIN}

lines = interp.splitlines()
cands = []
i=0
while i < len(lines):
    if "bullets.push_back" in lines[i]:
        buf=lines[i]; j=i
        while j < len(lines) and ");" not in lines[j]:
            j+=1
            if j < len(lines): buf += "\n"+lines[j]
        text="".join(re.findall(r'"((?:[^"\\]|\\.)*)"', buf))
        if re.search(r"[\u4e00-\u9fff]", text):
            cands.append((i+1, text))
        i=j+1; continue
    i+=1

def cover(t):
    if t in exact: return "exact"
    for e in exact:
        if len(e)>=24 and e in t: return "exact_clause"
    for p in sorted(sw, key=len, reverse=True):
        if t.startswith(p): return "starts_with"
    # Static extractor may drop dynamic suffixes (e.g. "分析限制：" + diagnostic.message).
    if t.endswith("：") and len(t) >= MIN:
        prefix = t
        for p in sorted(sw, key=len, reverse=True):
            if prefix.startswith(p) or p.startswith(prefix): return "starts_with_prefix"
    for m in sorted(finds, key=len, reverse=True):
        if m in t: return "find"
    for e in sorted(ends, key=len, reverse=True):
        if t.endswith(e): return "ends_with"
    # English-prefixed starts like "RSM ", "ANOM ", "Dixon", "Hotelling", "Durbin", "I-MR", "EMP", "ndc", "MAPE", "R²", "Pa(RQL)", "McNemar", "Cochran", "GOF", "Pearson", "Spearman"
    for p in re.findall(r'starts_with\(\s*bullet\s*,\s*"((?:[^"\\]|\\.)+)"', loc):
        if t.startswith(p) and len(p)>=6: return "starts_with_en"
    return None

unc=[]
for ln,t in cands:
    c=cover(t)
    if not c: unc.append((ln,t))

def fam(t):
    for name,kws in [
        ("msa",["量具","Gage","ndc","偏倚","Wheeler","EMP","%Study","操作者","零件","稳定性图"]),
        ("doe_rsm",["DOE","Pareto","等值线","Desirability","因子","中心点","失拟","编码","立方"]),
        ("reliability",["寿命","Weibull","删失","KM","CIF","Fine-Gray","保证","可靠"]),
        ("spc",["控制图","超限","Sigma Z","过度离散","EWMA","CUSUM","特殊原因","Laney","I-MR","阶段"]),
        ("inference",["P 值","正态","t 检验","Z 检验","功效","ANOVA","相关","异常值","比例","泊松","等价","ANOM","Dixon","Grubbs","方差","中位数"]),
        ("gof_npar",["GOF","McNemar","Cochran","Kruskal","Friedman","Wilcoxon","Mood","Runs","卡方","拟合优度"]),
        ("forecast_reg",["MAPE","MASE","预测","R²","VIF","Durbin","回归"]),
        ("ml",["聚类","树","孤立","判别","结点","k-means","CART"]),
        ("eda",["带宽","密度","小提琴","六边形","分面","条形"]),
        ("capability",["能力","Cp","Pp","规格","门禁"]),
    ]:
        if any(k in t for k in kws): return name
    return "other"

cf=Counter(fam(t) for _,t in unc)
out=root/"scripts/_tmp_phase3_interp_strict.txt"
with out.open("w",encoding="utf-8") as f:
    f.write(f"cands={len(cands)} uncovered={len(unc)}\n")
    for k,v in cf.most_common(): f.write(f"  {v}\t{k}\n")
    for ln,t in unc:
        f.write(f"L{ln}\t{fam(t)}\t{t}\n")
print(f"uncovered={len(unc)}", dict(cf))
for ln,t in unc[:25]:
    print(f"L{ln}\t{fam(t)}\t{t[:100]}")
