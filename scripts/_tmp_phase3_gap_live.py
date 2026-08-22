import re, pathlib
from collections import Counter, defaultdict

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")

# Only take exact pairs that look like ZH catalog entries (contain CJK)
exact_zh = []
for m in re.finditer(r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"(?:[^"\\]|\\.)+"\s*\}', loc):
    s = m.group(1)
    if re.search(r"[\u4e00-\u9fff]", s):
        exact_zh.append(s)
exact_set = set(exact_zh)

# starts_with / find / ends_with markers that contain CJK
sw = set()
for pat in [
    r'starts_with\(\s*bullet\s*,\s*"((?:[^"\\]|\\.)+)"',
    r'starts_with\(\s*body\s*,\s*"((?:[^"\\]|\\.)+)"',
    r'parse_leading_count_after_prefix\(\s*bullet\s*,\s*"((?:[^"\\]|\\.)+)"',
]:
    sw |= {x for x in re.findall(pat, loc) if re.search(r"[\u4e00-\u9fff]", x)}

finds = set()
for pat in [
    r'bullet\.find\(\s*"((?:[^"\\]|\\.)+)"',
    r'body\.find\(\s*"((?:[^"\\]|\\.)+)"',
    r'bullet\.rfind\(\s*"((?:[^"\\]|\\.)+)"',
]:
    finds |= {x for x in re.findall(pat, loc) if re.search(r"[\u4e00-\u9fff]", x)}

ends = {x for x in re.findall(r'ends_with\(\s*bullet\s*,\s*"((?:[^"\\]|\\.)+)"', loc) if re.search(r"[\u4e00-\u9fff]", x)}

# Also capture string literals compared with == or used as mid markers in find("...")
# Template mid-strings from find(" 下显著") style already in finds

lines = interp.splitlines()

def extract_string_parts(buf):
    return "".join(re.findall(r'"((?:[^"\\]|\\.)*)"', buf))

# Collect push_back bullets AND text.str() / ostringstream conclusions
candidates = []  # (line, kind, text)
i = 0
while i < len(lines):
    line = lines[i]
    if "bullets.push_back" in line:
        buf = line
        j = i
        while j < len(lines) and ");" not in lines[j]:
            j += 1
            if j < len(lines):
                buf += "\n" + lines[j]
        text = extract_string_parts(buf)
        if re.search(r"[\u4e00-\u9fff]", text):
            candidates.append((i+1, "push", text))
        i = j + 1
        continue
    # ostringstream pattern: text << "中文"... then push_back(text.str())
    if re.search(r'\btext\s*<<\s*"', line) and re.search(r"[\u4e00-\u9fff]", line):
        # accumulate contiguous text << block until push or blank section
        buf = ""
        j = i
        while j < len(lines):
            if "push_back(text.str())" in lines[j] or "push_back(text.str()" in lines[j]:
                buf += "\n" + lines[j]
                break
            if j > i and ("bullets.push_back" in lines[j] or (lines[j].strip().startswith("if ") and "text <<" not in lines[j] and j > i+30)):
                break
            buf += "\n" + lines[j]
            j += 1
            if j - i > 40:
                break
        text = extract_string_parts(buf)
        if re.search(r"[\u4e00-\u9fff]", text) and "push_back(text.str()" in buf:
            candidates.append((i+1, "oss", text))
            i = j + 1
            continue
    i += 1

def cover_kind(text):
    if text in exact_set:
        return "exact"
    # Prefer template markers first
    for p in sorted(sw, key=len, reverse=True):
        if text.startswith(p):
            return "starts_with"
    for m in sorted(finds, key=len, reverse=True):
        if len(m) >= 4 and m in text:
            return "find"
    for e in sorted(ends, key=len, reverse=True):
        if len(e) >= 4 and text.endswith(e):
            return "ends_with"
    # exact full honesty clause contained (must be long enough to be intentional map)
    for e in exact_zh:
        if len(e) >= 20 and e in text:
            return "exact_clause"
    return None

uncovered = []
kinds = Counter()
for ln, k, text in candidates:
    c = cover_kind(text)
    if c:
        kinds[c] += 1
    else:
        uncovered.append((ln, k, text))

# Family heuristic
def family(t):
    keys = [
        ("msa_gage", ["量具", "Gage", "ndc", "偏倚", "Wheeler", "操作者", "零件"]),
        ("doe_rsm", ["DOE", "Pareto", "等值线", "Desirability", "因子", "中心点", "失拟"]),
        ("reliability", ["寿命", "Weibull", "删失", "KM", "CIF", "Fine-Gray", "保证"]),
        ("spc_laney", ["控制图", "超限", "Sigma Z", "过度离散", "EWMA", "CUSUM", "特殊原因", "Laney"]),
        ("inference", ["P 值", "正态", "t 检验", "Z 检验", "功效", "ANOVA", "相关", "异常值", "比例", "泊松", "等价", "ANOM", "Dixon", "Grubbs"]),
        ("forecast_reg", ["MAPE", "MASE", "预测", "R²", "VIF", "Durbin"]),
        ("ml_cluster", ["聚类", "树", "孤立", "判别", "结点"]),
        ("eda", ["带宽", "密度", "小提琴", "六边形"]),
        ("capability", ["能力", "Cp", "Pp", "规格"]),
        ("gof_npar", ["GOF", "McNemar", "Cochran", "Kruskal", "Friedman", "Wilcoxon", "Mood", "Runs"]),
    ]
    for name, kws in keys:
        if any(k in t for k in kws):
            return name
    return "other"

fam = Counter(family(t) for _,_,t in uncovered)

out = root / "scripts/_tmp_phase3_gap_live.txt"
with out.open("w", encoding="utf-8") as f:
    f.write(f"candidates={len(candidates)} covered={sum(kinds.values())} uncovered={len(uncovered)}\n")
    f.write(f"kinds={dict(kinds)}\n")
    f.write(f"sw={len(sw)} finds={len(finds)} ends={len(ends)} exact_zh={len(exact_zh)}\n")
    f.write("FAMILY\n")
    for k,v in fam.most_common():
        f.write(f"  {v}\t{k}\n")
    f.write("UNCOVERED\n")
    for ln,k,t in uncovered:
        f.write(f"L{ln}\t{k}\t{family(t)}\t{t}\n")

# Also audit param summary tokens still Chinese in analysis_service that may leak:
# Find parameter_summary / append_parameter style Chinese fragments
anal = (root / "src/application/analysis_service.cpp").read_text(encoding="utf-8")
# localize_parameter_summary token map
param_tokens = set(re.findall(r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"param\.summary\.[^"]+"\s*\}', loc))
# Chinese strings assigned near parameter_summary construction is hard; sample lines with CJK that look like key= value chrome
param_cands = []
for i, line in enumerate(anal.splitlines(), 1):
    if not re.search(r"[\u4e00-\u9fff]", line):
        continue
    if any(x in line for x in ["parameter_summary", "summary +=", "summary.append", "params.push", "push_back(", "oss <<", "<< \""]):
        # extract string lit
        for s in re.findall(r'"((?:[^"\\]|\\.)*)"', line):
            if re.search(r"[\u4e00-\u9fff]", s) and len(s) >= 2:
                # covered if token exact or prefix in param_tokens
                cov = False
                if s in param_tokens or s in exact_set:
                    cov = True
                else:
                    for t in param_tokens:
                        if s.startswith(t) or t.startswith(s) or (len(t)>=4 and t in s):
                            cov = True
                            break
                if not cov and (" = " in s or s.strip().endswith("=") or "分布" in s or "假设" in s or "操作" in s):
                    param_cands.append((i, s))

with out.open("a", encoding="utf-8") as f:
    f.write(f"\nPARAM_LIKELY_UNMAPPED sample n={len(param_cands)}\n")
    for i,s in param_cands[:60]:
        f.write(f"anal:{i}\t{s}\n")

print(f"uncovered_interp={len(uncovered)} fam={dict(fam)} param_sample={len(param_cands)}")
