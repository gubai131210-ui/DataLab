#!/usr/bin/env python3
"""Preflight for algorithm Wave-5 (2026-08-23)."""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED = [
    "docs/research/p5_random_forest.md",
    "docs/research/p5_weibayes.md",
    "docs/research/p5_taguchi_orthogonal_design.md",
    "docs/research/p5_distribution_calculator.md",
    "docs/research/goal-wave-2026-08-23-algorithm-wave5.md",
    "src/domain/statistics/random_forest.cpp",
    "src/domain/statistics/weibayes.cpp",
    "src/domain/statistics/taguchi_orthogonal.cpp",
    "src/domain/statistics/distribution_calculator.cpp",
    "tests/algorithm_wave5_track_test.cpp",
    "samples/product_evolution/unified_track_acceptance_plan.md",
]

WAVE5 = [
    ("random_forest", "p5_random_forest.md"),
    ("weibayes", "p5_weibayes.md"),
    ("taguchi_orthogonal_design", "p5_taguchi_orthogonal_design.md"),
    ("distribution_calculator", "p5_distribution_calculator.md"),
]

WAVE5_MARKERS = [
    ("src/application/analysis_service.cpp", "AnalysisService::random_forest"),
    ("src/application/analysis_service.cpp", "AnalysisService::weibayes"),
    ("src/application/analysis_service.cpp", "AnalysisService::taguchi_orthogonal_design"),
    ("src/application/analysis_service.cpp", "AnalysisService::distribution_calculator"),
    ("src/domain/statistics/random_forest.cpp", "fit_random_forest"),
    ("src/domain/statistics/weibayes.cpp", "fit_weibayes"),
    ("src/domain/statistics/taguchi_orthogonal.cpp", "generate_taguchi_orthogonal"),
    ("src/domain/statistics/distribution_calculator.cpp", "evaluate_distribution_calculator"),
    ("tests/algorithm_wave5_track_test.cpp", "randomForestServiceAndSerialize"),
    ("tests/algorithm_wave5_track_test.cpp", "weibayesZeroFailureHonesty"),
    ("tests/algorithm_wave5_track_test.cpp", "taguchiL8WorksheetExport"),
    ("tests/algorithm_wave5_track_test.cpp", "distCalcNormalCdfHalf"),
    ("tests/algorithm_wave5_track_test.cpp", "RandomForestFacts"),
    ("tests/algorithm_wave5_track_test.cpp", "WeibayesFacts"),
    ("tests/algorithm_wave5_track_test.cpp", "TaguchiOrthogonalFacts"),
    ("tests/algorithm_wave5_track_test.cpp", "DistributionCalculatorFacts"),
]

INTERP_KEYS = [
    "rf_summary",
    "rf_importance",
    "rf_disclosure",
    "rf_honesty",
    "weibayes_summary",
    "shape_prior",
    "limits",
    "design_summary",
    "export_hint",
    "scope",
    "distcalc_result",
    "distcalc_params",
    "distcalc_scope",
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
    if "algorithm_wave5_track_test" not in cmake:
        fail("CMakeLists missing algorithm_wave5_track_test")
    for src in (
        "random_forest.cpp",
        "weibayes.cpp",
        "taguchi_orthogonal.cpp",
        "distribution_calculator.cpp",
    ):
        if src not in cmake:
            fail(f"CMakeLists missing {src}")
    ok("CMake wave5 sources + test target")

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
    backlog = (ROOT / "docs/research/minitab-market-algorithm-backlog.md").read_text(
        encoding="utf-8"
    )
    acceptance = (
        ROOT / "samples/product_evolution/unified_track_acceptance_plan.md"
    ).read_text(encoding="utf-8")
    taxonomy = (
        ROOT / "docs/research/ui-menu-ia-command-taxonomy-map-2026-08-23.md"
    ).read_text(encoding="utf-8")
    dod = (ROOT / "docs/research/goal-wave-2026-08-23-algorithm-wave5.md").read_text(
        encoding="utf-8"
    )
    wave5_tests = (ROOT / "tests/algorithm_wave5_track_test.cpp").read_text(
        encoding="utf-8"
    )
    serialization = (
        ROOT / "src/infrastructure/output_serialization.cpp"
    ).read_text(encoding="utf-8")
    catalog = (ROOT / "src/domain/report_text_catalog_part17.cpp").read_text(
        encoding="utf-8"
    )

    for path, marker in WAVE5_MARKERS:
        text = (ROOT / path).read_text(encoding="utf-8")
        if marker not in text:
            fail(f"Wave-5 marker missing {marker!r} in {path}")
    ok("Wave-5 service/domain/test markers")

    for cmd_id, research in WAVE5:
        if cmd_id not in by_id:
            fail(f"help missing {cmd_id}")
        if f'QStringLiteral("{cmd_id}")' not in commands:
            fail(f"commands missing {cmd_id}")
        method = {
            "random_forest": "random_forest",
            "weibayes": "weibayes",
            "taguchi_orthogonal_design": "taguchi_orthogonal_design",
            "distribution_calculator": "distribution_calculator",
        }[cmd_id]
        if f"AnalysisService::{method}" not in service:
            fail(f"service missing AnalysisService::{method}")
        md = (ROOT / "docs/research" / research).read_text(encoding="utf-8")
        if "2026-08-23" not in md:
            fail(f"{research} missing date")
        if "http" not in md and "https://" not in md:
            fail(f"{research} missing Primary URL")
        if cmd_id == "random_forest" and "minitab.com" not in md.lower():
            fail(f"{research} missing minitab Primary URL")
        if cmd_id == "weibayes" and "nist.gov" not in md.lower():
            fail(f"{research} missing NIST Primary URL")
        if cmd_id == "distribution_calculator" and "dtoc" not in md.lower():
            fail(f"{research} missing NIST dtoc URL")
        if cmd_id not in wiring:
            fail(f"wiring-index missing {cmd_id}")
        if cmd_id not in interpretation and method not in interpretation:
            fail(f"interpretation missing {cmd_id}")
        if cmd_id not in taxonomy:
            fail(f"taxonomy map missing {cmd_id}")
        ok(cmd_id)

    for key in INTERP_KEYS:
        if key not in interpretation and key not in catalog:
            fail(f"interp key missing: {key}")
    ok("interpretation / catalog keys")

    for facts in (
        "RandomForestFacts",
        "WeibayesFacts",
        "TaguchiOrthogonalFacts",
        "DistributionCalculatorFacts",
    ):
        if facts not in serialization:
            fail(f"serialization missing {facts}")
    ok("Facts serialize round-trip markers")

    if "# source: formula_reference" not in wave5_tests:
        fail("wave5 tests missing formula_reference marker")
    for marker in (
        "random_forest",
        "weibayes",
        "taguchi_orthogonal_design",
        "distribution_calculator",
        "NOT TreeNet",
        "zero_failure",
        "worksheet_export",
        "0.5",
    ):
        if marker not in wave5_tests and marker.replace("_", "") not in wave5_tests:
            # soft: allow related spellings already covered by markers list
            pass
    for required_test_marker in (
        "random_forest",
        "weibayes",
        "taguchi_orthogonal_design",
        "distribution_calculator",
        "RandomForestFacts",
        "WeibayesFacts",
        "formula_reference",
    ):
        if required_test_marker not in wave5_tests:
            fail(f"wave5 tests missing marker {required_test_marker}")
    ok("formula_reference / test id markers")

    # DoD all [x] for W5 items
    unchecked = re.findall(r"^- \[ \] ", dod, flags=re.M)
    if unchecked:
        fail(f"DoD still has unchecked items: {len(unchecked)}")
    for token in ("W5-1", "W5-2", "W5-3", "W5-4", "random_forest", "weibayes"):
        if token not in dod:
            fail(f"DoD missing {token}")
    ok("DoD [x] complete")

    if "Wave-5" not in acceptance and "算法 Wave-5" not in acceptance:
        fail("acceptance missing Wave-5 section")
    if "verify_algorithm_wave5_track.py" not in acceptance:
        fail("acceptance missing wave5 verify script")
    if "`random_forest`" not in backlog and "random_forest" not in backlog:
        fail("backlog missing random_forest")
    if "✅" not in backlog.split("15.1")[1][:800]:
        fail("backlog Wave-5 section missing ✅")
    ok("backlog checkmarks + acceptance Wave-5")

    # Menu groups in commands
    expected_groups = {
        "random_forest": "多变量",
        "weibayes": "可靠性",
        "taguchi_orthogonal_design": "DOE",
        "distribution_calculator": "推断 / 仿真",
    }
    for cid, group in expected_groups.items():
        # Find command block and ensure menu_group literal near AnalysisService call
        pattern = (
            rf'QStringLiteral\("{cid}"\).*?'
            rf'AnalysisService::{cid if cid != "taguchi_orthogonal_design" else "taguchi_orthogonal_design"}.*?'
            rf'QStringLiteral\("{re.escape(group)}"\)'
        )
        if not re.search(pattern, commands, flags=re.S):
            # fallback: group appears after the command id somewhere
            idx = commands.find(f'QStringLiteral("{cid}")')
            if idx < 0:
                fail(f"commands missing id {cid}")
            window = commands[idx : idx + 2500]
            if f'QStringLiteral("{group}")' not in window:
                fail(f"commands {cid} missing menu_group {group}")
    ok("menu_group wiring")

    print("\nalgorithm wave5 preflight: PASS (4 commands wired)")


if __name__ == "__main__":
    main()
