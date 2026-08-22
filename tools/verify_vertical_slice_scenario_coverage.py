#!/usr/bin/env python3
"""Verify all 13/13 vertical-slice representative tests appear in scenario prefilter."""

from __future__ import annotations

import importlib.util
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCENARIO_SCRIPT = ROOT / "tools/list_phase3_prefilter_by_scenario.py"

REPRESENTATIVE_THREE_TEMPLATE: list[str] = [
    "representative_doe_ccd_three_report_profiles_localize_without_cross_language_leak",
    "representative_doe_bbd_three_report_profiles_localize_without_cross_language_leak",
    "representative_rsm_lof_three_report_profiles_localize_without_cross_language_leak",
    "representative_graph_scatter_faceted_three_report_profiles_localize_without_cross_language_leak",
    "representative_reliability_km_three_report_profiles_localize_without_cross_language_leak",
    "representative_reliability_weibull_three_report_profiles_localize_without_cross_language_leak",
    "representative_reliability_lognormal_three_report_profiles_localize_without_cross_language_leak",
    "representative_warranty_summary_three_report_profiles_localize_without_cross_language_leak",
    "representative_warranty_strata_three_report_profiles_localize_without_cross_language_leak",
    "representative_box_cox_three_report_profiles_localize_without_cross_language_leak",
    "representative_johnson_capability_three_report_profiles_localize_without_cross_language_leak",
    "representative_nonnormal_capability_three_report_profiles_localize_without_cross_language_leak",
    "representative_normal_capability_three_report_profiles_localize_without_cross_language_leak",
]

GLOBAL_GUARD = "representative_vertical_slice_reports_localize_without_cross_language_leak"


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


def main() -> int:
    scenario = load_scenario_names()
    missing: list[str] = []
    for name in REPRESENTATIVE_THREE_TEMPLATE:
        if name not in scenario:
            missing.append(name)

    print(
        f"13/13 vertical slice scenario map: "
        f"{len(REPRESENTATIVE_THREE_TEMPLATE) - len(missing)}/"
        f"{len(REPRESENTATIVE_THREE_TEMPLATE)} representative tests in scenario prefilter"
    )
    if GLOBAL_GUARD not in scenario:
        print(f"  MISSING global guard  {GLOBAL_GUARD}")
        return 1
    for name in missing:
        print(f"  MISSING  {name}")

    return 1 if missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
