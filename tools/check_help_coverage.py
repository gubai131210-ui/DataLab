#!/usr/bin/env python3
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
text = (ROOT / "src/ui/analysis_commands.cpp").read_text(encoding="utf-8")
ids = []
for m in re.finditer(
    r'QStringLiteral\("([a-z0-9_]+)"\),\s*\n\s*QStringLiteral\("[^"]+"\),\s*\n\s*QStringLiteral\("[^"]+"\),\s*\n\s*QStringLiteral\("(统计|图形|控制图|质量工具)"\)',
    text,
):
    ids.append(m.group(1))

catalog = json.loads((ROOT / "resources/help/algorithm_help.json").read_text(encoding="utf-8"))
required = [
    "purpose",
    "method_overview",
    "calculation_steps",
    "symbol_definitions",
    "decision_rules",
    "invalid_input_conditions",
    "missing_value_policy",
    "output_interpretation",
]
cat_ids = {e["id"] for e in catalog["entries"]}
cmd_ids = set(ids)
print("commands", len(cmd_ids), "catalog", len(cat_ids))
print("missing in catalog", sorted(cmd_ids - cat_ids))
print("extra in catalog", sorted(cat_ids - cmd_ids))
leaks = []
incomplete = []
for entry in catalog["entries"]:
    for field in required:
        if not entry.get(field):
            incomplete.append((entry["id"], field))
    blob = " ".join(
        [
            entry.get("purpose", ""),
            entry.get("method_overview", ""),
            " ".join(b.get("plain_text", "") for b in entry.get("formula_blocks", [])),
        ]
    )
    if "docs/" in blob or ".md" in blob or "见仓库" in blob:
        leaks.append(entry["id"])
print("incomplete", incomplete)
print("markdown leaks", leaks)
if cmd_ids - cat_ids or incomplete or leaks:
    raise SystemExit(1)
