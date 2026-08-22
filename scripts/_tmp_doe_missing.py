import re
from pathlib import Path

ROOT = Path(r"d:\QT_CppPrograms\DataLab")
loc = (ROOT / "src/application/report_localization.cpp").read_text(encoding="utf-8")
all_mapped = {
    m.group(1)
    for m in re.finditer(r'\{"([^"]*[\u4e00-\u9fff][^"]*)",\s*"([^"]+)"\}', loc)
}
p = ROOT / "src/domain/statistics/doe_factorial.cpp"
strings = sorted(
    set(
        m.group(1)
        for m in re.finditer(r'"([^"]*[\u4e00-\u9fff][^"]*)"', p.read_text(encoding="utf-8"))
    )
)
missing = [s for s in strings if s not in all_mapped]
Path(r"d:\QT_CppPrograms\DataLab\scripts\_tmp_doe_missing.txt").write_text(
    "\n".join(missing), encoding="utf-8"
)
print(len(missing))
