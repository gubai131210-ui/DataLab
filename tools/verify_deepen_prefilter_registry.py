#!/usr/bin/env python3
"""Verify F' deepen prefilter test names resolve to Qt Creator targets."""

from __future__ import annotations

import importlib.util
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
DOC = ROOT / "docs/research/phase3-cross-page-pdf-manual-acceptance.md"
TESTS = ROOT / "tests"
CMAKE = ROOT / "CMakeLists.txt"
SCENARIO_SCRIPT = ROOT / "tools/list_phase3_prefilter_by_scenario.py"

SPECIAL: dict[str, str] = {
    "nonnormal_capability_phase6_test": "nonnormal_capability_phase6_test",
    "graph_builder_faceted_page_titles_localize_to_en_us": "report_locale_phase3_test",
}


def load_section_names() -> list[str]:
    text = DOC.read_text(encoding="utf-8")
    start = text.find("### 3.1")
    end = text.find("### 3.2", start)
    if start < 0 or end < 0:
        raise RuntimeError(f"Could not locate section 3.1 in {DOC}")
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


def load_deepen_names() -> list[str]:
    spec = importlib.util.spec_from_file_location("phase3_scenario", SCENARIO_SCRIPT)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Could not load {SCENARIO_SCRIPT}")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    section = set(load_section_names())
    deepen: set[str] = set(mod.GLOBAL)
    for tests in mod.SCENARIOS.values():
        deepen.update(tests)
    return sorted(deepen - section)


def load_cmake_targets() -> dict[str, str]:
    mapping: dict[str, str] = {}
    text = CMAKE.read_text(encoding="utf-8", errors="replace")
    for line in text.splitlines():
        m = re.match(
            r"add_datalab_test\(\s*(\w+)\s+tests/(\w+\.cpp)",
            line.strip(),
        )
        if m:
            mapping[m.group(2)] = m.group(1)
    for m in re.finditer(
        r"add_datalab_test\(\s*\n\s*(\w+)\s*\n\s*tests/(\w+\.cpp)",
        text,
    ):
        mapping[m.group(2)] = m.group(1)
    return mapping


def load_test_locations() -> dict[str, str]:
    locations: dict[str, str] = {}
    for path in sorted(TESTS.glob("*.cpp")):
        text = path.read_text(encoding="utf-8", errors="replace")
        for match in re.finditer(r"\bvoid\s+(?:\w+::)?(\w+)\s*\(\s*\)", text):
            locations[match.group(1)] = path.name
    return locations


def resolve_target(name: str, locations: dict[str, str], cmake: dict[str, str]) -> str:
    if name in SPECIAL:
        return SPECIAL[name]
    cpp = locations.get(name)
    if cpp and cpp in cmake:
        return cmake[cpp]
    if cpp:
        return f"UNKNOWN_TARGET({cpp})"
    return "MISSING"


def main() -> int:
    names = load_deepen_names()
    locations = load_test_locations()
    cmake = load_cmake_targets()

    missing: list[str] = []
    for name in names:
        target = resolve_target(name, locations, cmake)
        if target in {"MISSING", "UNKNOWN_TARGET"} or target.startswith("UNKNOWN_TARGET"):
            missing.append(name)

    print(
        f"F' deepen registry: {len(names)} names, "
        f"{len(names) - len(missing)} resolved, {len(missing)} missing"
    )
    for name in missing:
        print(f"  MISSING  {name}")

    return 1 if missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
