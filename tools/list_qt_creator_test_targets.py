#!/usr/bin/env python3
"""Map Phase 3 §3.1 test names to Qt Creator / CTest target executables.

Usage:
  python tools/list_qt_creator_test_targets.py
  python tools/list_qt_creator_test_targets.py --by-target
  python tools/list_qt_creator_test_targets.py --deepen --by-target
  python tools/list_qt_creator_test_targets.py --scenario --by-target
  python tools/list_qt_creator_test_targets.py --scenario-id S4 --by-target
  python tools/list_qt_creator_test_targets.py --global-only --by-target
"""

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


def load_cmake_targets() -> dict[str, str]:
    """Map tests/foo.cpp -> cmake target name."""
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
    """Map void test name -> tests/*.cpp filename."""
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


def load_scenario_module():
    spec = importlib.util.spec_from_file_location("phase3_scenario", SCENARIO_SCRIPT)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Could not load {SCENARIO_SCRIPT}")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def normalize_scenario_id(scenario_id: str) -> str:
    sid = scenario_id.strip().upper()
    if not sid.startswith("S"):
        sid = "S" + sid
    return sid


def load_scenario_id_names(scenario_id: str) -> list[str]:
    mod = load_scenario_module()
    sid = normalize_scenario_id(scenario_id)
    for key, tests in mod.SCENARIOS.items():
        if key.startswith(sid + " "):
            return list(tests)
    known = ", ".join(normalize_scenario_id(k.split(" ", 1)[0]) for k in mod.SCENARIOS)
    raise RuntimeError(f"Unknown scenario id {scenario_id!r}; expected one of: {known}")


def load_global_names() -> list[str]:
    mod = load_scenario_module()
    return list(mod.GLOBAL)


def load_scenario_names() -> list[str]:
    """All S1-S7 scenario prefilter tests (global + per-scenario)."""
    mod = load_scenario_module()
    names: set[str] = set(mod.GLOBAL)
    for tests in mod.SCENARIOS.values():
        names.update(tests)
    return sorted(names)


def load_deepen_names() -> list[str]:
    """Tests in scenario prefilter but outside section 3.1 registry."""
    mod = load_scenario_module()
    section = set(load_section_names())
    deepen: set[str] = set(mod.GLOBAL)
    for tests in mod.SCENARIOS.values():
        deepen.update(tests)
    return sorted(deepen - section)


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--by-target",
        action="store_true",
        help="Group output by CTest/Qt Creator executable target",
    )
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--deepen",
        action="store_true",
        help="List F'/E'/E'' deepen tests from scenario prefilter (outside section 3.1 61 count)",
    )
    group.add_argument(
        "--scenario",
        action="store_true",
        help="List all S1-S7 scenario prefilter tests (84 items incl. global 6)",
    )
    group.add_argument(
        "--global-only",
        action="store_true",
        help="List only the 6 global prefilter tests (run before any S1-S7 scenario)",
    )
    group.add_argument(
        "--scenario-id",
        metavar="S1",
        help="List tests for one scenario only (S1-S7; does not include global 6)",
    )
    parser.add_argument(
        "--algorithm-regression",
        action="store_true",
        help="Also list optional dual-line algorithm domain test targets (Phase 4-6)",
    )
    args = parser.parse_args()

    try:
        if args.global_only:
            names = load_global_names()
        elif args.scenario_id:
            names = load_scenario_id_names(args.scenario_id)
        elif args.scenario:
            names = load_scenario_names()
        elif args.deepen:
            names = load_deepen_names()
        else:
            names = load_section_names()
    except RuntimeError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    locations = load_test_locations()
    cmake = load_cmake_targets()

    rows: list[tuple[str, str]] = []
    missing: list[str] = []
    for name in names:
        target = resolve_target(name, locations, cmake)
        rows.append((name, target))
        if target in {"MISSING", "UNKNOWN_TARGET"} or target.startswith("UNKNOWN_TARGET"):
            missing.append(name)

    if args.by_target:
        by_target: dict[str, list[str]] = {}
        for name, target in rows:
            by_target.setdefault(target, []).append(name)
        if args.global_only:
            label = "Global prefilter (6 tests)"
        elif args.scenario_id:
            label = f"Scenario {normalize_scenario_id(args.scenario_id)} prefilter"
        elif args.scenario:
            label = "S1-S7 scenario prefilter (global + scenarios)"
        elif args.deepen:
            label = "F' deepen (scenario prefilter - section 3.1)"
        else:
            label = "Phase 3 section 3.1"
        print(f"# {label} — {len(names)} tests in {len(by_target)} Qt Creator targets\n")
        priority = [
            "report_export_phase2_test",
            "report_locale_phase3_test",
            "report_profile_phase1_test",
            "interpretation_service_test",
            "nonnormal_capability_phase6_test",
            "report_contract_phase0_test",
            "response_surface_design_phase4_test",
            "reliability_phase5_test",
        ]
        seen: set[str] = set()
        for target in priority + sorted(by_target):
            if target not in by_target or target in seen:
                continue
            seen.add(target)
            tests = by_target[target]
            print(f"## {target} ({len(tests)} tests)")
            for name in tests:
                print(f"  - {name}")
            print()
        for target in sorted(by_target):
            if target in seen:
                continue
            print(f"## {target} ({len(by_target[target])} tests)")
            for name in by_target[target]:
                print(f"  - {name}")
            print()
    else:
        if args.global_only:
            label = "Global prefilter"
        elif args.scenario_id:
            label = f"Scenario {normalize_scenario_id(args.scenario_id)} prefilter"
        elif args.scenario:
            label = "S1-S7 scenario prefilter"
        elif args.deepen:
            label = "F' deepen (scenario - section 3.1)"
        else:
            label = "Phase 3 section 3.1"
        print(f"# {label} — test → Qt Creator target ({len(names)} items)\n")
        width = max(len(n) for n in names)
        for name, target in rows:
            print(f"{name.ljust(width)}  →  {target}")

    if missing:
        print(f"\n# WARNING: {len(missing)} unresolved", file=sys.stderr)
        for name in missing:
            print(f"  {name}", file=sys.stderr)
        return 1

    print(
        "\n# Qt Creator: open Tests, run targets above (ReportExportPhase2Test is largest batch)."
    )
    if args.deepen:
        print("# Full section 3.1: python tools/list_qt_creator_test_targets.py --by-target")
        print("# Full scenario: python tools/list_qt_creator_test_targets.py --scenario --by-target")
        print("# By scenario: python tools/list_phase3_prefilter_by_scenario.py")
    elif args.scenario:
        print("# Section 3.1 only: python tools/list_qt_creator_test_targets.py --by-target")
        print("# Deepen only: python tools/list_qt_creator_test_targets.py --deepen --by-target")
        print("# Per scenario: python tools/list_qt_creator_test_targets.py --scenario-id S4 --by-target")
    elif args.scenario_id or args.global_only:
        print("# Global first: python tools/list_qt_creator_test_targets.py --global-only --by-target")
        print("# All scenarios: python tools/list_phase3_prefilter_by_scenario.py")

    if args.algorithm_regression:
        optional = [
            "response_surface_design_phase4_test",
            "reliability_phase5_test",
            "report_contract_phase0_test",
            "report_profile_phase1_test",
        ]
        print("\n# Optional algorithm / report contract regression (not in §3.1):")
        for target in optional:
            print(f"  - {target}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
