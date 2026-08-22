import re, pathlib
from collections import Counter

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
exact = {a for a,b in re.findall(r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)+)"\s*\}', loc) if re.search(r"[\u4e00-\u9fff]", a)}

# Domain diagnostics: string literals with CJK in domain/statistics that look like user-facing messages
domain_files = list((root/"src/domain/statistics").glob("*.cpp"))
domain_files += list((root/"src/domain").glob("*.cpp"))
cjk = re.compile(r"[\u4e00-\u9fff]")
lit = re.compile(r'"((?:[^"\\]|\\.)*)"')

unmapped = []
for fp in domain_files:
    for i, line in enumerate(fp.read_text(encoding="utf-8").splitlines(), 1):
        if not cjk.search(line):
            continue
        if line.lstrip().startswith("//"):
            continue
        # message-like
        if not any(k in line for k in ["message", "diagnostic", "error", "warning", "status", "push_back", "return ", " = ", ".message", "diagnostics"]):
            continue
        for s in lit.findall(line):
            if not cjk.search(s) or len(s) < 4:
                continue
            if s in exact:
                continue
            # prefix match against starts_with diag prefixes
            hit = False
            for e in exact:
                if len(e) >= 8 and (s.startswith(e) or e in s):
                    hit = True
                    break
            if hit:
                continue
            unmapped.append((str(fp.relative_to(root)).replace("\\","/"), i, s))

# family
def fam(path, s):
    p = path.lower()
    if "capability" in p or "能力" in s: return "capability"
    if "control_chart" in p or "multivariate" in p: return "spc"
    if "doe" in p or "response_surface" in p or "rsm" in p: return "doe_rsm"
    if "gage" in p or "msa" in p or "expanded" in p: return "msa"
    if "reliab" in p or "km_" in p or "fine_gray" in p or "aalen" in p or "censor" in p: return "reliability"
    if "hypothesis" in p or "nonparam" in p or "inference" in p or "grubbs" in p: return "inference"
    if "eda" in p or "graph" in p: return "eda"
    return "other"

cf = Counter(fam(p,s) for p,_,s in unmapped)
out = root/"scripts/_tmp_phase3_domain_diag_gap.txt"
with out.open("w", encoding="utf-8") as f:
    f.write(f"unmapped_domain_diags={len(unmapped)}\n")
    for k,v in cf.most_common():
        f.write(f"  {v}\t{k}\n")
    for p,i,s in unmapped[:80]:
        f.write(f"{p}:{i}\t{fam(p,s)}\t{s}\n")
print(f"domain_unmapped={len(unmapped)} fam={dict(cf)}")
