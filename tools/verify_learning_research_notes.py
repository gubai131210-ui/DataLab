#!/usr/bin/env python3
"""Verify learning-center-research-notes.md covers all required IDs."""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    meta = json.loads((ROOT / "tools/learning_data/id_metadata.json").read_text(encoding="utf-8"))
    required = {e["id"] for e in meta["entries"]}
    md = (ROOT / "docs/research/learning-center-research-notes.md").read_text(encoding="utf-8")
    found = set(re.findall(r"^### ([a-z0-9_]+) —", md, re.MULTILINE))
    missing = required - found
    extra = found - required
    if missing:
        print("MISSING:", sorted(missing))
    if extra:
        print("EXTRA:", sorted(extra))
    # chart/control chart must have 图形解读要点
    sections = re.split(r"^### ", md, flags=re.MULTILINE)[1:]
    no_chart_note = []
    for sec in sections:
        cid = sec.split(" —", 1)[0].strip()
        entry = next(e for e in meta["entries"] if e["id"] == cid)
        cmd = entry.get("command") or {}
        help_info = entry.get("help") or {}
        mp = cmd.get("menu_path") or help_info.get("menu_path", "")
        is_chart = mp in ("控制图", "图形") or "控制图" in str(help_info.get("category", ""))
        if is_chart and "图形解读要点" not in sec and "不替代假设检验" not in sec:
            no_chart_note.append(cid)
    if no_chart_note:
        print("CHART NOTE MISSING:", no_chart_note)
    no_source = [s.split(" —", 1)[0] for s in sections if "accessed 2026-09-03" not in s]
    if no_source:
        print("SOURCE DATE MISSING:", no_source[:5], "...")
    ok = not missing and not extra and not no_chart_note
    print(f"VERIFY: {'PASS' if ok else 'FAIL'} — {len(found)}/{len(required)} ids")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
