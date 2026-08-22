#!/usr/bin/env python3
"""Print Phase 3 §3.1 Qt Creator pre-filter test names from the acceptance doc."""

from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
DOC = ROOT / "docs/research/phase3-cross-page-pdf-manual-acceptance.md"


def collect_names() -> list[str]:
    text = DOC.read_text(encoding="utf-8")
    start = text.find("### 3.1")
    end = text.find("### 3.2", start)
    if start < 0 or end < 0:
        raise RuntimeError(f"Could not locate §3.1 in {DOC}")

    names: list[str] = []
    for line in text[start:end].splitlines():
        m = re.match(r"^\d+\.\s+`([^`]+)`", line.strip())
        if not m:
            continue
        name = m.group(1)
        if " — " in name:
            name = name.split(" — ", 1)[0].strip()
        names.append(name)
    return names


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--verify",
        action="store_true",
        help="Exit 1 if any §3.1 name is missing from tests/*.cpp / CMake",
    )
    args = parser.parse_args()

    try:
        names = collect_names()
    except RuntimeError as exc:
        print(exc, file=sys.stderr)
        return 1

    if args.verify:
        verify = ROOT / "tools/verify_phase3_prefilter_registry.py"
        import subprocess

        return subprocess.call([sys.executable, str(verify)])

    print(f"# Phase 3 pre-filter ({len(names)} items) — paste into Qt Creator Tests filter")
    for i, name in enumerate(names, 1):
        print(f"{i:2}. {name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
