#!/usr/bin/env python3
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
cpp = (ROOT / "src/domain/report_text_catalog.cpp").read_text(encoding="utf-8")
ENTRY_RE = re.compile(
    r'\{\s*"([^"]+)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}'
)
entries = {}
for m in ENTRY_RE.finditer(cpp):
    entries[m.group(1)] = (m.group(2), m.group(3))

json_data = json.loads(
    (ROOT / "translations/report_strings.json").read_text(encoding="utf-8")
)
json_map = {e["id"]: e for e in json_data["entries"]}

for tid in ["diag.prefix.first_group", "diag.prefix.second_group"]:
    print(f"{tid}:")
    print(f"  in catalog: {tid in entries}")
    print(f"  in json: {tid in json_map}")
    if tid in entries:
        print(f"  catalog: {entries[tid]}")
    if tid in json_map:
        print(f"  json: {json_map[tid]['zh_cn']!r} / {json_map[tid]['en_us']!r}")

print(f"catalog size: {len(entries)}")
print(f"json size: {len(json_data['entries'])}")
missing = sorted(set(entries) - set(json_map))
print(f"missing from json: {len(missing)}")
if missing:
    print("sample missing:", missing[:20])
