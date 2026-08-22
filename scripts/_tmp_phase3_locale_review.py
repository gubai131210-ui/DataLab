#!/usr/bin/env python3
"""Review Phase 3 bilingual localization slice coverage."""
import re
from collections import Counter
from pathlib import Path

ROOT = Path(r"d:\QT_CppPrograms\DataLab")

loc = (ROOT / "src/application/report_localization.cpp").read_text(encoding="utf-8")
pattern = re.compile(
    r'\{"([^"]+)",\s*"(diag\.(?:time_series|normality|distribution_id|doe)\.[^"]+)"\}'
)
keys = pattern.findall(loc)
print(f"exact_ids count: {len(keys)}")
for k, v in keys:
    print(f"  {v}")

dup_keys = [k for k, n in Counter(k for k, _ in keys).items() if n > 1]
dup_ids = [k for k, n in Counter(v for _, v in keys).items() if n > 1]
print(f"duplicate Chinese keys: {dup_keys}")
print(f"duplicate catalog ids: {dup_ids}")

catalog = (ROOT / "src/domain/report_text_catalog.cpp").read_text(encoding="utf-8")
cat_pattern = re.compile(
    r'\{"(diag\.(?:time_series|normality|distribution_id|doe)\.[^"]+)",\s*\n\s*"([^"]+)",\s*\n\s*"([^"]+)"\}'
)
catalog_entries = cat_pattern.findall(catalog)
print(f"\ncatalog entries: {len(catalog_entries)}")
mapped = {k for k, _ in keys}
catalog_zh = {zh: cid for cid, zh, _ in catalog_entries}
catalog_ids = {cid for cid, _, _ in catalog_entries}

# exact_ids <-> catalog consistency
for zh, cid in keys:
    if cid not in catalog_ids:
        print(f"MISMATCH: exact_ids maps to missing catalog id {cid}")
    elif catalog_zh.get(zh) != cid:
        print(f"MISMATCH: zh key catalog id differs for {zh!r}")

for cid, zh, en in catalog_entries:
    if zh not in mapped:
        print(f"CATALOG NOT IN exact_ids: {cid} -> {zh!r}")

sources = {
    "time_series.cpp": ROOT / "src/domain/statistics/time_series.cpp",
    "normality_test.cpp": ROOT / "src/domain/statistics/normality_test.cpp",
    "distribution_identification.cpp": ROOT
    / "src/domain/statistics/distribution_identification.cpp",
    "doe_factorial.cpp": ROOT / "src/domain/statistics/doe_factorial.cpp",
}

print("\n=== Producer string coverage ===")
all_missing = []
for name, p in sources.items():
    text = p.read_text(encoding="utf-8")
    strings = sorted(
        set(m.group(1) for m in re.finditer(r'"([^"]*[\u4e00-\u9fff][^"]*)"', text))
    )
    print(f"\n{name} ({len(strings)} strings):")
    for s in strings:
        status = "MAPPED" if s in mapped else "MISSING"
        if status == "MISSING":
            all_missing.append((name, s))
        print(f"  [{status}] {s}")

print(f"\nTotal missing from exact_ids: {len(all_missing)}")

# Test diagnostics count
test = (ROOT / "tests/report_locale_phase3_test.cpp").read_text(encoding="utf-8")
test_block = test.split("void forecast_normality_distid_doe_diag_localizes_to_en_us()")[1].split("void ")[0]
diag_msgs = re.findall(r'"([^"]*[\u4e00-\u9fff][^"]*)"', test_block.split("page.diagnostics = {")[1].split("};")[0])
print(f"\nTest input diagnostics: {len(diag_msgs)}")
print(f"Test expects diags.size() == 22 (21 inputs + honesty diag)")
