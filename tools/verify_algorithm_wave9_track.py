#!/usr/bin/env python3
"""Preflight for algorithm Wave-9 (2026-08-28)."""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED = [
    "docs/research/p9_expanded_gage_unbalanced.md",
    "docs/research/p9_split_plot_analyze.md",
    "docs/research/p9_mixture_process_variable.md",
    "docs/research/p9_manova_one_way.md",
    "docs/research/goal-wave-2026-08-28-algorithm-wave9.md",
    "docs/research/goal-wave-2026-08-28-algorithm-wave9-plan-and-mega-prompt.md",
    "src/domain/statistics/expanded_gage_unbalanced.cpp",
    "src/domain/statistics/split_plot_analyze.cpp",
    "src/domain/statistics/mixture_process_variable.cpp",
    "src/domain/statistics/manova_one_way.cpp",
    "src/ui/expanded_gage_unbalanced_dialog.cpp",
    "src/ui/split_plot_analyze_dialog.cpp",
    "src/ui/mixture_process_variable_dialog.cpp",
    "src/ui/manova_one_way_dialog.cpp",
    "src/domain/report_text_catalog_part20.cpp",
    "tests/algorithm_wave9_track_test.cpp",
    "samples/product_evolution/unified_track_acceptance_plan.md",
]

WAVE9 = [
    ("expanded_gage_unbalanced", "p9_expanded_gage_unbalanced.md"),
    ("split_plot_analyze", "p9_split_plot_analyze.md"),
    ("mixture_process_variable", "p9_mixture_process_variable.md"),
    ("manova_one_way", "p9_manova_one_way.md"),
]

WAVE9_MARKERS = [
    ("src/application/analysis_service.cpp", "AnalysisService::expanded_gage_unbalanced"),
    ("src/application/analysis_service.cpp", "AnalysisService::split_plot_analyze"),
    ("src/application/analysis_service.cpp", "AnalysisService::mixture_process_variable"),
    ("src/application/analysis_service.cpp", "AnalysisService::manova_one_way"),
    ("src/domain/statistics/expanded_gage_unbalanced.cpp", "expanded_gage_unbalanced_analyze"),
    ("src/domain/statistics/split_plot_analyze.cpp", "split_plot_analyze"),
    ("src/domain/statistics/mixture_process_variable.cpp", "analyze_mixture_process_variable"),
    ("src/domain/statistics/manova_one_way.cpp", "manova_one_way_analyze"),
    ("tests/algorithm_wave9_track_test.cpp", "expandedGageFormulaReference"),
    ("tests/algorithm_wave9_track_test.cpp", "splitPlotFormulaReference"),
    ("tests/algorithm_wave9_track_test.cpp", "mixtureProcessFormulaReference"),
    ("tests/algorithm_wave9_track_test.cpp", "manovaFormulaReference"),
    ("tests/algorithm_wave9_track_test.cpp", "ExpandedGageUnbalancedFacts"),
    ("tests/algorithm_wave9_track_test.cpp", "SplitPlotAnalyzeFacts"),
    ("tests/algorithm_wave9_track_test.cpp", "MixtureProcessVariableFacts"),
    ("tests/algorithm_wave9_track_test.cpp", "ManovaOneWayFacts"),
]

INTERP_KEYS = [
    "expanded_gage_unbalanced_summary",
    "expanded_gage_unbalanced_varcomp",
    "expanded_gage_unbalanced_scope",
    "split_plot_analyze_summary",
    "split_plot_analyze_errors",
    "split_plot_analyze_scope",
    "mixture_process_variable_summary",
    "mixture_process_variable_scheffe",
    "mixture_process_variable_scope",
    "manova_one_way_summary",
    "manova_one_way_tests",
    "manova_one_way_scope",
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
    if "algorithm_wave9_track_test" not in cmake:
        fail("CMakeLists missing algorithm_wave9_track_test")
    for src in (
        "expanded_gage_unbalanced.cpp",
        "split_plot_analyze.cpp",
        "mixture_process_variable.cpp",
        "manova_one_way.cpp",
        "expanded_gage_unbalanced_dialog.cpp",
        "split_plot_analyze_dialog.cpp",
        "mixture_process_variable_dialog.cpp",
        "manova_one_way_dialog.cpp",
        "report_text_catalog_part20.cpp",
    ):
        if src not in cmake:
            fail(f"CMakeLists missing {src}")
    ok("CMake wave9 sources + dialogs + test target")

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
    dod = (ROOT / "docs/research/goal-wave-2026-08-28-algorithm-wave9.md").read_text(
        encoding="utf-8"
    )
    wave9_tests = (ROOT / "tests/algorithm_wave9_track_test.cpp").read_text(
        encoding="utf-8"
    )
    serialization = (
        ROOT / "src/infrastructure/output_serialization.cpp"
    ).read_text(encoding="utf-8")
    catalog = (ROOT / "src/domain/report_text_catalog_part20.cpp").read_text(
        encoding="utf-8"
    )
    mainwindow = (ROOT / "src/ui/mainwindow.cpp").read_text(encoding="utf-8")

    for path, marker in WAVE9_MARKERS:
        text = (ROOT / path).read_text(encoding="utf-8")
        if marker not in text:
            fail(f"Wave-9 marker missing {marker!r} in {path}")
    ok("Wave-9 service/domain/test markers")

    for cmd_id, research in WAVE9:
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
        if "2026-08-31" not in md and "2026-08-28" not in md:
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
        "ExpandedGageUnbalancedFacts",
        "SplitPlotAnalyzeFacts",
        "MixtureProcessVariableFacts",
        "ManovaOneWayFacts",
    ):
        if facts not in serialization:
            fail(f"serialization missing {facts}")
    ok("Facts serialize round-trip markers")

    if "# source: formula_reference" not in wave9_tests:
        fail("wave9 tests missing formula_reference marker")
    for required_test_marker in (
        "expanded_gage_unbalanced",
        "split_plot_analyze",
        "mixture_process_variable",
        "manova_one_way",
        "ExpandedGageUnbalancedFacts",
        "SplitPlotAnalyzeFacts",
        "MixtureProcessVariableFacts",
        "ManovaOneWayFacts",
        "formula_reference",
    ):
        if required_test_marker not in wave9_tests:
            fail(f"wave9 tests missing marker {required_test_marker}")
    ok("formula_reference / test id markers")

    dialog_pages = {
        "src/ui/expanded_gage_unbalanced_dialog.cpp": 4,
        "src/ui/split_plot_analyze_dialog.cpp": 4,
        "src/ui/mixture_process_variable_dialog.cpp": 4,
        "src/ui/manova_one_way_dialog.cpp": 4,
    }
    for rel, min_pages in dialog_pages.items():
        text = (ROOT / rel).read_text(encoding="utf-8")
        count = text.count("stack_->addWidget")
        if count < min_pages:
            fail(f"{rel} pages {count} < required {min_pages}")
    ok("UI multi-page minimums")

    egu_dialog = (ROOT / "src/ui/expanded_gage_unbalanced_dialog.cpp").read_text(
        encoding="utf-8"
    )
    mpv_dialog = (ROOT / "src/ui/mixture_process_variable_dialog.cpp").read_text(
        encoding="utf-8"
    )
    spa_dialog = (ROOT / "src/ui/split_plot_analyze_dialog.cpp").read_text(
        encoding="utf-8"
    )
    if "expanded_gage_rr" in egu_dialog:
        fail("expanded_gage_unbalanced dialog must not embed expanded_gage_rr")
    if "mixture_analyze" in mpv_dialog:
        fail("mixture_process_variable dialog must not embed mixture_analyze")
    if "doe_factorial" in spa_dialog:
        fail("split_plot_analyze dialog must not embed doe_factorial")
    ok("independent dialogs")

    for token in (
        "W9-1",
        "W9-2",
        "W9-3",
        "W9-4",
        "expanded_gage_unbalanced",
        "manova_one_way",
    ):
        if token not in dod:
            fail(f"DoD missing {token}")
    unchecked = re.findall(r"^- \[ \] (.+)$", dod, flags=re.M)
    allowed_unchecked = {
        "Checker 无 Critical",
        "告知用户 Qt Creator Rebuild 手测四点",
        "回归 `verify_algorithm_wave8_track.py` PASS",
        "回归 `verify_algorithm_wave7_track.py` PASS",
        "回归 `verify_ui_menu_ia_track.py` PASS",
        "Planner 映射表已产出",
        "`verify_algorithm_wave9_track.py` PASS",
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

    if "Wave-9" not in acceptance and "算法 Wave-9" not in acceptance:
        fail("acceptance missing Wave-9 section")
    if "verify_algorithm_wave9_track.py" not in acceptance:
        fail("acceptance missing wave9 verify script")
    if "15.5" not in backlog:
        fail("backlog missing §15.5 Wave-9")
    if "✅" not in backlog.split("15.5")[1][:900]:
        fail("backlog Wave-9 section missing ✅")
    ok("backlog checkmarks + acceptance Wave-9")

    expected_groups = {
        "expanded_gage_unbalanced": "MSA",
        "split_plot_analyze": "DOE",
        "mixture_process_variable": "DOE",
        "manova_one_way": "ANOVA",
    }
    for cid, group in expected_groups.items():
        idx = commands.find(f'QStringLiteral("{cid}")')
        if idx < 0:
            fail(f"commands missing id {cid}")
        window = commands[idx : idx + 3500]
        if f'QStringLiteral("{group}")' not in window:
            fail(f"commands {cid} missing menu_group {group}")
    ok("menu_group wiring")

    print("\nalgorithm wave9 preflight: PASS (4 commands wired)")


if __name__ == "__main__":
    main()
