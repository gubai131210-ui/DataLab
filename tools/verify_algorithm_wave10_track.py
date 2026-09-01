#!/usr/bin/env python3
"""Preflight for algorithm Wave-10 (2026-08-31)."""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED = [
    "docs/research/p10_general_manova.md",
    "docs/research/p10_mixed_effects_reml.md",
    "docs/research/p10_binary_doe_probit.md",
    "docs/research/p10_life_data_lognormal.md",
    "docs/research/goal-wave-2026-08-31-algorithm-wave10.md",
    "docs/research/goal-wave-2026-08-31-algorithm-wave10-plan-and-mega-prompt.md",
    "src/domain/statistics/general_manova.cpp",
    "src/domain/statistics/mixed_effects_reml.cpp",
    "src/domain/statistics/binary_doe_probit.cpp",
    "src/domain/statistics/life_data_lognormal.cpp",
    "src/ui/general_manova_dialog.cpp",
    "src/ui/mixed_effects_reml_dialog.cpp",
    "src/ui/binary_doe_probit_dialog.cpp",
    "src/ui/life_data_lognormal_dialog.cpp",
    "src/domain/report_text_catalog_part21.cpp",
    "tests/algorithm_wave10_track_test.cpp",
    "samples/product_evolution/unified_track_acceptance_plan.md",
]

WAVE10 = [
    ("general_manova", "p10_general_manova.md"),
    ("mixed_effects_reml", "p10_mixed_effects_reml.md"),
    ("binary_doe_probit", "p10_binary_doe_probit.md"),
    ("life_data_lognormal", "p10_life_data_lognormal.md"),
]

WAVE10_MARKERS = [
    ("src/application/analysis_service.cpp", "AnalysisService::general_manova"),
    ("src/application/analysis_service.cpp", "AnalysisService::mixed_effects_reml"),
    ("src/application/analysis_service.cpp", "AnalysisService::binary_doe_probit"),
    ("src/application/analysis_service.cpp", "AnalysisService::life_data_lognormal"),
    ("src/domain/statistics/general_manova.cpp", "general_manova_analyze"),
    ("src/domain/statistics/mixed_effects_reml.cpp", "mixed_effects_reml_analyze"),
    ("src/domain/statistics/binary_doe_probit.cpp", "analyze_binary_doe_probit"),
    ("src/domain/statistics/life_data_lognormal.cpp", "fit_life_data_lognormal"),
    ("tests/algorithm_wave10_track_test.cpp", "generalManovaFormulaReference"),
    ("tests/algorithm_wave10_track_test.cpp", "mixedEffectsFormulaReference"),
    ("tests/algorithm_wave10_track_test.cpp", "binaryDoeProbitFormulaReference"),
    ("tests/algorithm_wave10_track_test.cpp", "lifeDataLognormalFormulaReference"),
    ("tests/algorithm_wave10_track_test.cpp", "GeneralManovaFacts"),
    ("tests/algorithm_wave10_track_test.cpp", "MixedEffectsRemlFacts"),
    ("tests/algorithm_wave10_track_test.cpp", "BinaryDoeProbitFacts"),
    ("tests/algorithm_wave10_track_test.cpp", "LifeDataLognormalFacts"),
]

INTERP_KEYS = [
    "general_manova_summary",
    "general_manova_tests",
    "general_manova_scope",
    "mixed_effects_reml_summary",
    "mixed_effects_reml_method",
    "mixed_effects_reml_scope",
    "binary_doe_probit_summary",
    "binary_doe_probit_irwls",
    "binary_doe_probit_scope",
    "life_data_lognormal_summary",
    "life_data_lognormal_mle",
    "life_data_lognormal_scope",
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
    if "algorithm_wave10_track_test" not in cmake:
        fail("CMakeLists missing algorithm_wave10_track_test")
    for src in (
        "general_manova.cpp",
        "mixed_effects_reml.cpp",
        "binary_doe_probit.cpp",
        "life_data_lognormal.cpp",
        "general_manova_dialog.cpp",
        "mixed_effects_reml_dialog.cpp",
        "binary_doe_probit_dialog.cpp",
        "life_data_lognormal_dialog.cpp",
        "report_text_catalog_part21.cpp",
    ):
        if src not in cmake:
            fail(f"CMakeLists missing {src}")
    ok("CMake wave10 sources + dialogs + test target")

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
    dod = (ROOT / "docs/research/goal-wave-2026-08-31-algorithm-wave10.md").read_text(
        encoding="utf-8"
    )
    wave10_tests = (ROOT / "tests/algorithm_wave10_track_test.cpp").read_text(
        encoding="utf-8"
    )
    serialization = (
        ROOT / "src/infrastructure/output_serialization.cpp"
    ).read_text(encoding="utf-8")
    catalog = (ROOT / "src/domain/report_text_catalog_part21.cpp").read_text(
        encoding="utf-8"
    )
    mainwindow = (ROOT / "src/ui/mainwindow.cpp").read_text(encoding="utf-8")

    for path, marker in WAVE10_MARKERS:
        text = (ROOT / path).read_text(encoding="utf-8")
        if marker not in text:
            fail(f"Wave-10 marker missing {marker!r} in {path}")
    ok("Wave-10 service/domain/test markers")

    for cmd_id, research in WAVE10:
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
        if "2026-08-31" not in md:
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
        "GeneralManovaFacts",
        "MixedEffectsRemlFacts",
        "BinaryDoeProbitFacts",
        "LifeDataLognormalFacts",
    ):
        if facts not in serialization:
            fail(f"serialization missing {facts}")
    ok("Facts serialize round-trip markers")

    if "# source: formula_reference" not in wave10_tests:
        fail("wave10 tests missing formula_reference marker")
    for required_test_marker in (
        "general_manova",
        "mixed_effects_reml",
        "binary_doe_probit",
        "life_data_lognormal",
        "GeneralManovaFacts",
        "MixedEffectsRemlFacts",
        "BinaryDoeProbitFacts",
        "LifeDataLognormalFacts",
        "formula_reference",
    ):
        if required_test_marker not in wave10_tests:
            fail(f"wave10 tests missing marker {required_test_marker}")
    ok("formula_reference / test id markers")

    dialog_pages = {
        "src/ui/general_manova_dialog.cpp": 4,
        "src/ui/mixed_effects_reml_dialog.cpp": 4,
        "src/ui/binary_doe_probit_dialog.cpp": 4,
        "src/ui/life_data_lognormal_dialog.cpp": 4,
    }
    for rel, min_pages in dialog_pages.items():
        text = (ROOT / rel).read_text(encoding="utf-8")
        count = text.count("stack_->addWidget")
        if count < min_pages:
            fail(f"{rel} pages {count} < required {min_pages}")
    ok("UI multi-page minimums")

    gm_dialog = (ROOT / "src/ui/general_manova_dialog.cpp").read_text(encoding="utf-8")
    bdp_dialog = (ROOT / "src/ui/binary_doe_probit_dialog.cpp").read_text(encoding="utf-8")
    ldl_dialog = (ROOT / "src/ui/life_data_lognormal_dialog.cpp").read_text(encoding="utf-8")
    if "manova_one_way" in gm_dialog:
        fail("general_manova dialog must not embed manova_one_way")
    if "binary_response_doe" in bdp_dialog:
        fail("binary_doe_probit dialog must not embed binary_response_doe")
    if "life_data_regression" in ldl_dialog:
        fail("life_data_lognormal dialog must not embed life_data_regression")
    ok("independent dialogs")

    for token in (
        "W10-1",
        "W10-2",
        "W10-3",
        "W10-4",
        "general_manova",
        "mixed_effects_reml",
        "binary_doe_probit",
        "life_data_lognormal",
    ):
        if token not in dod:
            fail(f"DoD missing {token}")
    unchecked = re.findall(r"^- \[ \] (.+)$", dod, flags=re.M)
    allowed_unchecked = {
        "Checker 无 Critical",
        "告知用户 Qt Creator Rebuild 手测四点",
        "回归 `verify_algorithm_wave9_track.py` PASS",
        "回归 `verify_algorithm_wave8_track.py` PASS",
        "回归 `verify_algorithm_wave7_track.py` PASS",
        "回归 `verify_ui_menu_ia_track.py` PASS",
        "Planner 映射表已产出",
        "`verify_algorithm_wave10_track.py` PASS",
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

    if "Wave-10" not in acceptance and "算法 Wave-10" not in acceptance:
        fail("acceptance missing Wave-10 section")
    if "verify_algorithm_wave10_track.py" not in acceptance:
        fail("acceptance missing wave10 verify script")
    if "15.6" not in backlog:
        fail("backlog missing §15.6 Wave-10")
    if "✅" not in backlog.split("15.6")[1][:900]:
        fail("backlog Wave-10 section missing ✅")
    ok("backlog checkmarks + acceptance Wave-10")

    expected_groups = {
        "general_manova": "ANOVA",
        "mixed_effects_reml": "ANOVA",
        "binary_doe_probit": "DOE",
        "life_data_lognormal": "可靠性",
    }
    for cid, group in expected_groups.items():
        idx = commands.find(f'QStringLiteral("{cid}")')
        if idx < 0:
            fail(f"commands missing id {cid}")
        window = commands[idx : idx + 3500]
        if f'QStringLiteral("{group}")' not in window:
            fail(f"commands {cid} missing menu_group {group}")
    ok("menu_group wiring")

    print("\nalgorithm wave10 preflight: PASS (4 commands wired)")


if __name__ == "__main__":
    main()
