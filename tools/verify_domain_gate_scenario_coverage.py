#!/usr/bin/env python3
"""Verify domain-layer gate tests (S6) appear in scenario prefilter."""

from __future__ import annotations

import importlib.util
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCENARIO_SCRIPT = ROOT / "tools/list_phase3_prefilter_by_scenario.py"

DOMAIN_GATE: dict[str, tuple[str, str]] = {
    "box_cox_service_skips_capability_on_invalid_spec_limits": (
        "S6",
        "tests/quality_statistics_test.cpp",
    ),
    "box_cox_service_skips_capability_on_inverted_spec_limits": (
        "S6",
        "tests/quality_statistics_test.cpp",
    ),
    "johnson_spec_outside_support_skips_overall_capability": (
        "S6",
        "tests/nonnormal_capability_phase6_test.cpp",
    ),
}


def load_scenario_names() -> set[str]:
    spec = importlib.util.spec_from_file_location("phase3_scenario", SCENARIO_SCRIPT)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Could not load {SCENARIO_SCRIPT}")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    names: set[str] = set(mod.GLOBAL)
    for tests in mod.SCENARIOS.values():
        names.update(tests)
    return names


def cpp_has_test(path: str, name: str) -> bool:
    text = (ROOT / path).read_text(encoding="utf-8")
    return f"void {name}()" in text or f"void {name} (" in text


def main() -> int:
    scenario = load_scenario_names()
    missing_scenario: list[str] = []
    missing_cpp: list[str] = []

    for name, (sid, cpp_path) in DOMAIN_GATE.items():
        if not cpp_has_test(cpp_path, name):
            missing_cpp.append(name)
        elif name not in scenario:
            missing_scenario.append(f"{name} ({sid})")

    print(
        f"domain gate scenario map: "
        f"{len(DOMAIN_GATE) - len(missing_scenario)}/{len(DOMAIN_GATE)} "
        f"in scenario prefilter"
    )
    if missing_cpp:
        for name in missing_cpp:
            print(f"  MISSING cpp  {name}")
    for item in missing_scenario:
        print(f"  MISSING scenario  {item}")

    return 1 if missing_cpp or missing_scenario else 0


if __name__ == "__main__":
    raise SystemExit(main())
