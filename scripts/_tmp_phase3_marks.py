import re, pathlib
from collections import Counter

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
interp = (root / "src/application/interpretation_service.cpp").read_text(encoding="utf-8")

finds = []
for pat in [
    r'bullet\.find\(\s*"((?:[^"\\]|\\.)+)"',
    r'body\.find\(\s*"((?:[^"\\]|\\.)+)"',
]:
    finds += re.findall(pat, loc)
zh_finds = [x for x in finds if re.search(r"[\u4e00-\u9fff]", x)]
print("zh find marks by length:")
for x in sorted(set(zh_finds), key=lambda s: (-len(s), s))[:80]:
    print(f"  {len(x):3d}  {x}")

sw = []
for pat in [
    r'starts_with\(\s*bullet\s*,\s*"((?:[^"\\]|\\.)+)"',
    r'starts_with\(\s*body\s*,\s*"((?:[^"\\]|\\.)+)"',
]:
    sw += re.findall(pat, loc)
zh_sw = [x for x in sw if re.search(r"[\u4e00-\u9fff]", x)]
print("\nzh starts_with count", len(set(zh_sw)))
short_finds = [x for x in set(zh_finds) if len(x) < 8]
print("short finds (<8):", short_finds[:40])
