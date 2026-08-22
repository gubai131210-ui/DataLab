#!/usr/bin/env python3
"""Verify gate interpretation bullet tests appear in S1-S7 scenario prefilter."""

from __future__ import annotations

import importlib.util
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCENARIO_SCRIPT = ROOT / "tools/list_phase3_prefilter_by_scenario.py"
INTERP_TEST = ROOT / "tests/interpretation_service_test.cpp"

GATE_TESTS: dict[str, str] = {
    "usesWarrantyExposureGateInterpretationBullet": "S4",
    "usesJohnsonSpecLimitGateInterpretationBullet": "S6",
    "usesBoxCoxSpecLimitGateInterpretationBullet": "S6",
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


def load_cpp_tests() -> set[str]:
    text = INTERP_TEST.read_text(encoding="utf-8")
    return {name for name in GATE_TESTS if f"void {name}()" in text}


def main() -> int:
    scenario = load_scenario_names()
    cpp = load_cpp_tests()
    missing_scenario: list[str] = []
    missing_cpp: list[str] = []

    for name, sid in GATE_TESTS.items():
        if name not in cpp:
            missing_cpp.append(name)
        elif name not in scenario:
            missing_scenario.append(f"{name} ({sid})")

    print(
        f"interpretation gate scenario map: "
        f"{len(GATE_TESTS) - len(missing_scenario)}/{len(GATE_TESTS)} "
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
