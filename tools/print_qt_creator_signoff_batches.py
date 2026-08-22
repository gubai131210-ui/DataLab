#!/usr/bin/env python3
"""Print Qt Creator sign-off batch checklist (no test execution)."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

BATCHES: list[tuple[str, list[str]]] = [
    ("A. script preflight (12 checks)", ["python", "tools/print_acceptance_status.py"]),
    ("B. section 3.1 (61)", ["python", "tools/list_qt_creator_test_targets.py", "--by-target"]),
    ("C. deepen (37)", ["python", "tools/list_qt_creator_test_targets.py", "--deepen", "--by-target"]),
    ("D. global 6", ["python", "tools/list_qt_creator_test_targets.py", "--global-only", "--by-target"]),
    ("E. algorithm domain", [
        "python",
        "tools/list_qt_creator_test_targets.py",
        "--by-target",
        "--algorithm-regression",
    ]),
    ("F. G1+G2 track (5 tests)", ["powershell", "-File", "tools/run_g1g2_tests.ps1"]),
]

SCENARIOS = ["S1", "S2", "S3", "S4", "S5", "S6", "S7"]


def main() -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    print("# Qt Creator sign-off batches (user local)\n")
    print("Script-side must be 12/12 before starting Qt Creator.\n")

    for label, _cmd in BATCHES:
        print(f"- [ ] {label}")

    for sid in SCENARIOS:
        print(f"- [ ] scenario {sid} (`--scenario-id {sid} --by-target`)")

    print("- [ ] manual PDF S1-S7 (see samples/phase0_baselines/phase3_manual_acceptance_index.md)")
    print("- [ ] product evolution unified gate (see samples/product_evolution/unified_track_acceptance_plan.md)")
    print("- [ ] sign phase3-cross-page-pdf-manual-acceptance.md section 6\n")

    print("--- batch details ---\n")
    for label, cmd in BATCHES:
        print(f"## {label}")
        proc = subprocess.run(
            cmd,
            cwd=ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        out = (proc.stdout or proc.stderr or "").strip()
        if label.startswith("A."):
            for line in out.splitlines()[-6:]:
                print(line)
        else:
            for line in out.splitlines()[:12]:
                print(line)
            if out.count("\n") > 12:
                print("  ...")
        print()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
