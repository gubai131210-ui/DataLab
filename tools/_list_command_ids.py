#!/usr/bin/env python3
"""List analysis_commands ids (helper for formula-substitution planning)."""
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
cpp = (ROOT / "src/ui/analysis_commands.cpp").read_text(encoding="utf-8")
# Match command id as first QStringLiteral in each AnalysisCommand initializer block.
# Pattern used by menu IA patcher-compatible start.
pat = re.compile(
    r"\{\s*\n\s*QStringLiteral\(\"([a-zA-Z0-9_]+)\"\)\s*,",
    re.MULTILINE,
)
ids = pat.findall(cpp)
print(f"count={len(ids)}")
for i in ids:
    print(i)

help_path = ROOT / "resources/help/algorithm_help.json"
import json

entries = json.loads(help_path.read_text(encoding="utf-8")).get("entries", [])
help_ids = {e.get("id") for e in entries if e.get("id")}
missing_help = [i for i in ids if i not in help_ids]
print(f"help_entries={len(help_ids)}", file=sys.stderr)
print(f"commands_missing_help={len(missing_help)}", file=sys.stderr)
for i in missing_help:
    print(f"missing_help:{i}", file=sys.stderr)
