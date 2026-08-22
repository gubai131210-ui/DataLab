#!/usr/bin/env python3
"""Post-patch acceptance checks for Implementer-A."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from _patch_menu_ia_commands import (  # noqa: E402
    CMD_START,
    MAP,
    find_command_end,
    load_taxonomy,
)

CPP = ROOT / "src" / "ui" / "analysis_commands.cpp"
HELP = ROOT / "resources" / "help" / "algorithm_help.json"


def main() -> int:
    tax = load_taxonomy(MAP)
    cpp = CPP.read_text(encoding="utf-8")
    matches = list(CMD_START.finditer(cpp))
    assert len(matches) == 137, len(matches)

    groups = {}
    paths = {}
    for m in matches:
        cid = m.group("id")
        paths[cid] = m.group("path")
        brace = m.start() + len("\n        ")
        if cpp[brace] != "{":
            brace = cpp.find("{", m.start())
        end = find_command_end(cpp, brace)
        chunk = cpp[brace : end + 1]
        gm = re.search(r',\n            QStringLiteral\("([^"]+)"\)\s*\}$', chunk)
        assert gm, f"no group for {cid}"
        groups[cid] = gm.group(1)

    assert len(groups) == 137
    assert set(groups) == set(tax)

    bad = []
    for cid, (ep, eg) in tax.items():
        if paths[cid] != ep or groups[cid] != eg:
            bad.append((cid, paths[cid], groups[cid], ep, eg))
    assert not bad, bad

    assert paths["cox_regression"] == "统计"
    assert groups["cox_regression"] == "可靠性"
    assert paths["pareto"] == "质量工具"
    assert groups["pareto"] == "质量图 / 规划"
    for cid in (
        "doe_factorial",
        "doe_response",
        "response_optimization",
        "rsm_response",
        "doe_plackett_burman",
        "doe_ccd",
        "doe_bbd",
    ):
        assert paths[cid] == "统计", (cid, paths[cid])
        assert groups[cid] == "DOE", (cid, groups[cid])

    # Sample endings printable
    for cid in ("descriptive", "cox_regression", "pareto", "doe_factorial"):
        print(f"{cid}: path={paths[cid]!r} group={groups[cid]!r}")

    help_data = json.loads(HELP.read_text(encoding="utf-8"))
    wrong = []
    aligned = 0
    no_entry = []
    for cid, (ep, eg) in tax.items():
        entry = next((e for e in help_data["entries"] if e.get("id") == cid), None)
        if entry is None:
            no_entry.append(cid)
            continue
        exp = f"{ep} > {eg}"
        if entry.get("menu_path") != exp:
            wrong.append((cid, entry.get("menu_path"), exp))
        else:
            aligned += 1
    print(f"help aligned={aligned} wrong={len(wrong)} no_entry={len(no_entry)}")
    if wrong:
        print("WRONG", wrong[:10])
    print("no_entry sample", no_entry[:15])
    assert not wrong

    # Ensure no leftover top-level 可靠性 as menu_path for commands
    for cid, p in paths.items():
        assert p in ("统计", "控制图", "质量工具", "图形"), (cid, p)

    print("ACCEPTANCE OK")
    print("missing_ids=[]")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
