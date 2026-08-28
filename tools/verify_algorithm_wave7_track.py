#!/usr/bin/env python3
"""Preflight for algorithm Wave-7 (2026-08-28)."""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED = [
    "docs/research/p7_mixture_analyze.md",
    "docs/research/p7_glm_two_way.md",
    "docs/research/p7_analyze_variability.md",
    "docs/research/p7_factor_analysis.md",
    "docs/research/goal-wave-2026-08-28-algorithm-wave7.md",
    "docs/research/goal-wave-2026-08-28-algorithm-wave7-plan-and-mega-prompt.md",
    "src/domain/statistics/mixture_analyze.cpp",
    "src/domain/statistics/glm_two_way.cpp",
    "src/domain/statistics/analyze_variability.cpp",
    "src/domain/statistics/factor_analysis.cpp",
    "src/ui/mixture_analyze_dialog.cpp",
    "src/ui/glm_two_way_dialog.cpp",
    "src/ui/analyze_variability_dialog.cpp",
    "src/ui/factor_analysis_dialog.cpp",
    "src/domain/report_text_catalog_part18.cpp",
    "tests/algorithm_wave7_track_test.cpp",
    "samples/product_evolution/unified_track_acceptance_plan.md",
]

WAVE7 = [
    ("mixture_analyze", "p7_mixture_analyze.md"),
    ("glm_two_way", "p7_glm_two_way.md"),
    ("analyze_variability", "p7_analyze_variability.md"),
    ("factor_analysis", "p7_factor_analysis.md"),
]

WAVE7_MARKERS = [
    ("src/application/analysis_service.cpp", "AnalysisService::mixture_analyze"),
    ("src/application/analysis_service.cpp", "AnalysisService::glm_two_way"),
    ("src/application/analysis_service.cpp", "AnalysisService::analyze_variability"),
    ("src/application/analysis_service.cpp", "AnalysisService::factor_analysis"),
    ("src/domain/statistics/mixture_analyze.cpp", "analyze_mixture_scheffe"),
    ("src/domain/statistics/glm_two_way.cpp", "glm_two_way_analyze"),
    ("src/domain/statistics/analyze_variability.cpp", "analyze_variability_dispersion"),
    ("src/domain/statistics/factor_analysis.cpp", "factor_analysis_extract"),
    ("tests/algorithm_wave7_track_test.cpp", "mixtureScheffeFormulaReference"),
    ("tests/algorithm_wave7_track_test.cpp", "glmTwoWayFormulaReference"),
    ("tests/algorithm_wave7_track_test.cpp", "variabilityEffectFormulaReference"),
    ("tests/algorithm_wave7_track_test.cpp", "factorLoadingsFormulaReference"),
    ("tests/algorithm_wave7_track_test.cpp", "MixtureAnalyzeFacts"),
    ("tests/algorithm_wave7_track_test.cpp", "GlmTwoWayFacts"),
    ("tests/algorithm_wave7_track_test.cpp", "AnalyzeVariabilityFacts"),
    ("tests/algorithm_wave7_track_test.cpp", "FactorAnalysisFacts"),
]

INTERP_KEYS = [
    "mixture_analyze_summary",
    "mixture_analyze_coef",
    "mixture_analyze_scope",
    "glm_two_way_summary",
    "glm_two_way_fitted",
    "glm_two_way_scope",
    "analyze_variability_summary",
    "analyze_variability_effects",
    "analyze_variability_scope",
    "factor_analysis_summary",
    "factor_analysis_loadings",
    "factor_analysis_scope",
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
    if "algorithm_wave7_track_test" not in cmake:
        fail("CMakeLists missing algorithm_wave7_track_test")
    for src in (
        "mixture_analyze.cpp",
        "glm_two_way.cpp",
        "analyze_variability.cpp",
        "factor_analysis.cpp",
        "mixture_analyze_dialog.cpp",
        "glm_two_way_dialog.cpp",
        "analyze_variability_dialog.cpp",
        "factor_analysis_dialog.cpp",
        "report_text_catalog_part18.cpp",
    ):
        if src not in cmake:
            fail(f"CMakeLists missing {src}")
    ok("CMake wave7 sources + dialogs + test target")

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
    dod = (ROOT / "docs/research/goal-wave-2026-08-28-algorithm-wave7.md").read_text(
        encoding="utf-8"
    )
    wave7_tests = (ROOT / "tests/algorithm_wave7_track_test.cpp").read_text(
        encoding="utf-8"
    )
    serialization = (
        ROOT / "src/infrastructure/output_serialization.cpp"
    ).read_text(encoding="utf-8")
    catalog = (ROOT / "src/domain/report_text_catalog_part18.cpp").read_text(
        encoding="utf-8"
    )
    mainwindow = (ROOT / "src/ui/mainwindow.cpp").read_text(encoding="utf-8")

    for path, marker in WAVE7_MARKERS:
        text = (ROOT / path).read_text(encoding="utf-8")
        if marker not in text:
            fail(f"Wave-7 marker missing {marker!r} in {path}")
    ok("Wave-7 service/domain/test markers")

    for cmd_id, research in WAVE7:
        if cmd_id not in by_id:
            fail(f"help missing {cmd_id}")
        entry = by_id[cmd_id]
        for field in ("purpose", "method_overview", "formula_blocks"):
            if field not in entry or not entry[field]:
                fail(f"help {cmd_id} missing {field}")
        purpose = str(entry.get("purpose", ""))
        if "见 md" in purpose or "见md" in purpose:
            fail(f"help {cmd_id} contains 见 md")
        if f'QStringLiteral("{cmd_id}")' not in commands:
            fail(f"commands missing {cmd_id}")
        if f"AnalysisService::{cmd_id}" not in service:
            fail(f"service missing AnalysisService::{cmd_id}")
        md = (ROOT / "docs/research" / research).read_text(encoding="utf-8")
        if "2026-08-28" not in md:
            fail(f"{research} missing date")
        if "http" not in md and "https://" not in md:
            fail(f"{research} missing Primary URL")
        if cmd_id not in wiring:
            fail(f"wiring-index missing {cmd_id}")
        if cmd_id not in interpretation:
            fail(f"interpretation missing {cmd_id}")
        if cmd_id not in taxonomy:
            fail(f"taxonomy map missing {cmd_id}")
        if cmd_id not in mainwindow:
            fail(f"MainWindow missing dedicated dialog hook for {cmd_id}")
        ok(cmd_id)

    for key in INTERP_KEYS:
        if key not in interpretation and key not in catalog:
            fail(f"interp key missing: {key}")
    ok("interpretation / catalog keys")

    for facts in (
        "MixtureAnalyzeFacts",
        "GlmTwoWayFacts",
        "AnalyzeVariabilityFacts",
        "FactorAnalysisFacts",
    ):
        if facts not in serialization:
            fail(f"serialization missing {facts}")
    ok("Facts serialize round-trip markers")

    if "# source: formula_reference" not in wave7_tests:
        fail("wave7 tests missing formula_reference marker")
    for required_test_marker in (
        "mixture_analyze",
        "glm_two_way",
        "analyze_variability",
        "factor_analysis",
        "MixtureAnalyzeFacts",
        "GlmTwoWayFacts",
        "AnalyzeVariabilityFacts",
        "FactorAnalysisFacts",
        "formula_reference",
    ):
        if required_test_marker not in wave7_tests:
            fail(f"wave7 tests missing marker {required_test_marker}")
    ok("formula_reference / test id markers")

    dialog_pages = {
        "src/ui/mixture_analyze_dialog.cpp": 4,
        "src/ui/glm_two_way_dialog.cpp": 4,
        "src/ui/analyze_variability_dialog.cpp": 4,
        "src/ui/factor_analysis_dialog.cpp": 4,
    }
    for rel, min_pages in dialog_pages.items():
        text = (ROOT / rel).read_text(encoding="utf-8")
        count = text.count("stack_->addWidget")
        if count < min_pages:
            fail(f"{rel} pages {count} < required {min_pages}")
    ok("UI multi-page minimums")

    for token in ("W7-1", "W7-2", "W7-3", "W7-4", "mixture_analyze", "factor_analysis"):
        if token not in dod:
            fail(f"DoD missing {token}")
    unchecked = re.findall(r"^- \[ \] (.+)$", dod, flags=re.M)
    allowed_unchecked = {
        "Checker 无 Critical",
        "告知用户 Qt Creator Rebuild 手测四点",
        "回归 `verify_ui_menu_ia_track.py` PASS",
        "回归 `verify_algorithm_wave6_track.py` PASS",
        "回归 `verify_algorithm_wave5_track.py` PASS",
        "Planner 映射表已产出",
        "`verify_algorithm_wave7_track.py` PASS",
    }
    for item in unchecked:
        if item.strip() in allowed_unchecked:
            continue
        if item.startswith("无") or item.startswith("禁止"):
            continue
        if item.startswith("回归"):
            continue
        fail(f"DoD still unchecked: {item}")
    ok("DoD implementation items [x]")

    if "Wave-7" not in acceptance and "算法 Wave-7" not in acceptance:
        fail("acceptance missing Wave-7 section")
    if "verify_algorithm_wave7_track.py" not in acceptance:
        fail("acceptance missing wave7 verify script")
    if "15.3" not in backlog:
        fail("backlog missing §15.3 Wave-7")
    if "✅" not in backlog.split("15.3")[1][:900]:
        fail("backlog Wave-7 section missing ✅")
    ok("backlog checkmarks + acceptance Wave-7")

    expected_groups = {
        "mixture_analyze": "DOE",
        "glm_two_way": "ANOVA",
        "analyze_variability": "DOE",
        "factor_analysis": "多变量",
    }
    for cid, group in expected_groups.items():
        idx = commands.find(f'QStringLiteral("{cid}")')
        if idx < 0:
            fail(f"commands missing id {cid}")
        window = commands[idx : idx + 3500]
        if f'QStringLiteral("{group}")' not in window:
            fail(f"commands {cid} missing menu_group {group}")
    ok("menu_group wiring")

    if "mixture_design_dialog" in (ROOT / "src/ui/mixture_analyze_dialog.cpp").read_text(
        encoding="utf-8"
    ):
        fail("mixture_analyze dialog must not embed mixture_design")
    ok("mixture_analyze independent dialog")

    print("\nalgorithm wave7 preflight: PASS (4 commands wired)")


if __name__ == "__main__":
    main()
