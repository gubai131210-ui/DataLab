#!/usr/bin/env python3
"""Surgically update only menu_path strings in algorithm_help.json."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from _patch_menu_ia_commands import MAP, load_taxonomy  # noqa: E402

HELP = ROOT / "resources" / "help" / "algorithm_help.json"


def main() -> int:
    tax = load_taxonomy(MAP)
    text = HELP.read_text(encoding="utf-8")
    data = json.loads(text)
    by_id = {e["id"]: e for e in data["entries"]}

    updated = 0
    missing = []
    for cid, (path, group) in tax.items():
        if cid not in by_id:
            missing.append(cid)
            continue
        new_mp = f"{path} > {group}"
        # Replace only the menu_path value belonging to this entry block.
        # Match "id": "<cid>" ... "menu_path": "..." within a reasonable window.
        pat = re.compile(
            rf'("id"\s*:\s*"{re.escape(cid)}"[\s\S]*?"menu_path"\s*:\s*")([^"]*)(")',
            re.M,
        )
        m = pat.search(text)
        if not m:
            print(f"WARN: cannot locate menu_path for {cid}")
            continue
        old = m.group(2)
        if old == new_mp:
            continue
        text = text[: m.start(2)] + new_mp + text[m.end(2) :]
        updated += 1
        print(f"  {cid}: {old!r} -> {new_mp!r}")

    HELP.write_text(text, encoding="utf-8", newline="\n")
    # Verify
    data2 = json.loads(HELP.read_text(encoding="utf-8"))
    wrong = []
    for e in data2["entries"]:
        if e["id"] not in tax:
            continue
        exp = f"{tax[e['id']][0]} > {tax[e['id']][1]}"
        if e.get("menu_path") != exp:
            wrong.append((e["id"], e.get("menu_path"), exp))
    print(f"updated={updated} missing_entries={len(missing)} wrong={wrong}")
    assert not wrong
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
