#!/usr/bin/env python3
"""Verify customer template limiting-evidence retention tests in scenario prefilter."""

from __future__ import annotations

import importlib.util
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCENARIO_SCRIPT = ROOT / "tools/list_phase3_prefilter_by_scenario.py"
PROFILE_TEST = ROOT / "tests/report_profile_phase1_test.cpp"

CUSTOMER_KEEPS: dict[str, str] = {
    "customer_keeps_ccd_bbd_design_limiting_evidence_under_truncation": "S2",
    "customer_keeps_cif_fine_gray_warranty_strata_limiting_evidence_under_truncation": "S4",
    "customer_keeps_warranty_exposure_gate_evidence_under_truncation": "S4",
    "customer_keeps_hexbin_rectangular_bins_limiting_evidence_under_truncation": "S3",
    "customer_keeps_density_curve_not_discrete_marks_limiting_evidence_under_truncation": "S3",
    "customer_keeps_capability_gate_limiting_evidence_under_truncation": "S6",
    "customer_keeps_rsm_lof_limiting_evidence_under_truncation": "S6",
    "customer_keeps_johnson_spec_limit_evidence_under_truncation": "S6",
    "customer_keeps_box_cox_limiting_evidence_under_truncation": "S6",
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
    text = PROFILE_TEST.read_text(encoding="utf-8")
    return {name for name in CUSTOMER_KEEPS if f"void {name}()" in text}


def main() -> int:
    scenario = load_scenario_names()
    cpp = load_cpp_tests()
    missing_scenario: list[str] = []
    missing_cpp: list[str] = []

    for name, sid in CUSTOMER_KEEPS.items():
        if name not in cpp:
            missing_cpp.append(name)
        elif name not in scenario:
            missing_scenario.append(f"{name} ({sid})")

    print(
        f"customer_keeps scenario map: "
        f"{len(CUSTOMER_KEEPS) - len(missing_scenario)}/{len(CUSTOMER_KEEPS)} "
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
