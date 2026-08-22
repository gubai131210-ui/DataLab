#!/usr/bin/env python3
"""Preflight for algorithm Wave-4 quality/reliability deepen (2026-08-22)."""

from __future__ import annotations

import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED = [
    "docs/research/p4_nonparametric_capability_deepen.md",
    "docs/research/p4_reliability_competing_risks_cif.md",
    "docs/research/p4_cox_regression.md",
    "docs/research/p4_logistic_regression_deepen.md",
    "src/domain/statistics/cox_regression.cpp",
    "src/domain/statistics/gray_test.cpp",
    "tests/algorithm_wave4_track_test.cpp",
    "samples/product_evolution/unified_track_acceptance_plan.md",
]

WAVE4 = [
    ("nonparametric_capability", "p4_nonparametric_capability_deepen.md"),
    ("reliability", "p4_reliability_competing_risks_cif.md"),
    ("cox_regression", "p4_cox_regression.md"),
    ("logistic_regression", "p4_logistic_regression_deepen.md"),
]

WAVE4_MARKERS = [
    ("src/application/analysis_service.cpp", "Capability Histogram"),
    ("src/application/analysis_service.cpp", "observed_ppm_total"),
    ("src/application/analysis_service.cpp", "gray_test_cif"),
    ("src/application/analysis_service.cpp", "CIF 曲线"),
    ("src/application/analysis_service.cpp", "AnalysisService::cox_regression"),
    ("src/application/analysis_service.cpp", "Stepwise Details"),
    ("src/domain/statistics/cox_regression.cpp", "cox_ph_fixed_covariates"),
    ("src/domain/statistics/gray_test.cpp", "gray_test_cif"),
    ("src/domain/statistics/logistic_regression.cpp", "fit_logistic_stepwise"),
    ("tests/algorithm_wave4_track_test.cpp", "nonparametricHistogramAndPpmDomain"),
    ("tests/algorithm_wave4_track_test.cpp", "coxRegressionServiceAndSerialize"),
]

HARDENING_MARKERS = [
    ("src/domain/statistics/nominal_logistic.cpp", "IRLS"),
    ("src/domain/statistics/accelerated_life.cpp", "Newton"),
    ("src/domain/statistics/accelerated_life.cpp", "观测信息矩阵"),
    ("tests/algorithm_wave2_hardening_test.cpp", "nominalLogisticUsesIrls"),
]


def fail(msg: str) -> None:
    print(f"FAIL: {msg}")
    sys.exit(1)


def ok(msg: str) -> None:
    print(f"OK: {msg}")


def main() -> None:
    for rel in REQUIRED:
        if not (ROOT / rel).is_file():
            fail(f"missing {rel}")
        ok(rel)

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    if "algorithm_wave4_track_test" not in cmake:
        fail("CMakeLists missing algorithm_wave4_track_test")
    ok("CMake wave4 test target")

    help_entries = json.loads(
        (ROOT / "resources/help/algorithm_help.json").read_text(encoding="utf-8")
    ).get("entries", [])
    by_id = {e.get("id"): e for e in help_entries}

    commands = (ROOT / "src/ui/analysis_commands.cpp").read_text(encoding="utf-8")
    service = (ROOT / "src/application/analysis_service.cpp").read_text(encoding="utf-8")
    interpretation = (ROOT / "src/application/interpretation_service.cpp").read_text(
        encoding="utf-8"
    )
    wiring = (ROOT / "docs/algorithm-wiring-index.md").read_text(encoding="utf-8")
    wave4_tests = (ROOT / "tests/algorithm_wave4_track_test.cpp").read_text(encoding="utf-8")

    for path, marker in HARDENING_MARKERS:
        text = (ROOT / path).read_text(encoding="utf-8")
        if marker not in text:
            fail(f"Wave-2.5 hardening marker missing {marker!r} in {path}")
    ok("Wave-2.5 IRLS/Newton regression markers")

    for path, marker in WAVE4_MARKERS:
        text = (ROOT / path).read_text(encoding="utf-8")
        if marker not in text:
            fail(f"Wave-4 marker missing {marker!r} in {path}")
    ok("Wave-4 service/domain/test markers")

    for cmd_id, research in WAVE4:
        if cmd_id not in by_id:
            fail(f"help missing {cmd_id}")
        if cmd_id == "cox_regression":
            if f'QStringLiteral("{cmd_id}")' not in commands:
                fail(f"commands missing {cmd_id}")
        elif cmd_id not in commands and f'QStringLiteral("{cmd_id}")' not in commands:
            fail(f"commands missing {cmd_id}")
        if cmd_id == "cox_regression":
            if "AnalysisService::cox_regression" not in service:
                fail(f"service missing {cmd_id}")
        elif f"AnalysisService::{cmd_id}" not in service and cmd_id not in service:
            fail(f"service missing {cmd_id}")
        md = (ROOT / "docs/research" / research).read_text(encoding="utf-8")
        if "support.minitab.com" not in md and "minitab.com" not in md:
            fail(f"{research} missing Primary URL")
        if "2026-08-22" not in md:
            fail(f"{research} missing date")
        if cmd_id not in wiring:
            fail(f"wiring-index missing {cmd_id}")
        if cmd_id not in interpretation:
            fail(f"interpretation missing {cmd_id}")
        ok(cmd_id)

    if "CoxRegressionFacts" not in (ROOT / "src/infrastructure/output_serialization.cpp").read_text(
        encoding="utf-8"
    ):
        fail("serialization missing CoxRegressionFacts round-trip")
    ok("CoxRegressionFacts serialize")

    if "# source: formula_reference" not in wave4_tests:
        fail("wave4 tests missing formula_reference marker")
    if "ForbiddenPhrases" not in wave4_tests and "过程合格" not in wave4_tests:
        fail("wave4 tests missing interpret forbidden phrase slot")
    ok("formula_reference / interpret tests")

    print("\nalgorithm wave4 preflight: PASS (4 commands wired)")


if __name__ == "__main__":
    main()
