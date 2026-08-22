#!/usr/bin/env python3
"""Verify Phase 3 §3.1 pre-filter test names exist in tests/*.cpp (or CMake target)."""

from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
DOC = ROOT / "docs/research/phase3-cross-page-pdf-manual-acceptance.md"
TESTS = ROOT / "tests"
CMAKE = ROOT / "CMakeLists.txt"

SPECIAL_TARGETS = {
    "nonnormal_capability_phase6_test": "nonnormal_capability_phase6_test",
    "graph_builder_faceted_page_titles_localize_to_en_us": "report_locale_phase3_test",
}


def load_section_names() -> list[str]:
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


def load_cpp_blob() -> str:
    parts: list[str] = []
    for path in sorted(TESTS.glob("*.cpp")):
        parts.append(path.read_text(encoding="utf-8", errors="replace"))
    return "\n".join(parts)


def resolve(name: str, cpp: str, cmake: str) -> tuple[bool, str]:
    if name in SPECIAL_TARGETS:
        target = SPECIAL_TARGETS[name]
        if target in cmake:
            return True, f"cmake target {target}"
        return False, f"missing cmake target {target}"

    pattern = rf"\bvoid\s+{re.escape(name)}\s*\("
    if re.search(pattern, cpp):
        return True, "ReportExportPhase2Test or sibling void test"
    return False, "no void test definition in tests/*.cpp"


def main() -> int:
    names = load_section_names()
    cpp = load_cpp_blob()
    cmake = CMAKE.read_text(encoding="utf-8", errors="replace")

    missing: list[tuple[str, str]] = []
    for name in names:
        ok, detail = resolve(name, cpp, cmake)
        if not ok:
            missing.append((name, detail))

    print(f"section 3.1 registry: {len(names)} names, {len(names) - len(missing)} resolved, {len(missing)} missing")
    for name, detail in missing:
        print(f"  MISSING  {name}  ({detail})")

    return 1 if missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
