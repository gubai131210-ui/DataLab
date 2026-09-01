#!/usr/bin/env python3
"""Preflight for algorithm Wave-11 (2026-09-01)."""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED = [
    "docs/research/p11_simple_correspondence.md",
    "docs/research/p11_multiple_correspondence.md",
    "docs/research/p11_nonlinear_regression.md",
    "docs/research/p11_split_plot_design.md",
    "docs/research/goal-wave-2026-09-01-algorithm-wave11.md",
    "docs/research/goal-wave-2026-09-01-algorithm-wave11-plan-and-mega-prompt.md",
    "src/domain/statistics/simple_correspondence.cpp",
    "src/domain/statistics/multiple_correspondence.cpp",
    "src/domain/statistics/nonlinear_regression.cpp",
    "src/domain/statistics/split_plot_design.cpp",
    "src/ui/simple_correspondence_dialog.cpp",
    "src/ui/multiple_correspondence_dialog.cpp",
    "src/ui/nonlinear_regression_dialog.cpp",
    "src/ui/split_plot_design_dialog.cpp",
    "src/domain/report_text_catalog_part22.cpp",
    "tests/algorithm_wave11_track_test.cpp",
    "samples/product_evolution/unified_track_acceptance_plan.md",
]

WAVE11 = [
    ("simple_correspondence", "p11_simple_correspondence.md"),
    ("multiple_correspondence", "p11_multiple_correspondence.md"),
    ("nonlinear_regression", "p11_nonlinear_regression.md"),
    ("split_plot_design", "p11_split_plot_design.md"),
]

WAVE11_MARKERS = [
    ("src/application/analysis_service.cpp", "AnalysisService::simple_correspondence"),
    ("src/application/analysis_service.cpp", "AnalysisService::multiple_correspondence"),
    ("src/application/analysis_service.cpp", "AnalysisService::nonlinear_regression"),
    ("src/application/analysis_service.cpp", "AnalysisService::split_plot_design"),
    ("src/domain/statistics/simple_correspondence.cpp", "simple_correspondence_analyze"),
    ("src/domain/statistics/multiple_correspondence.cpp", "multiple_correspondence_analyze"),
    ("src/domain/statistics/nonlinear_regression.cpp", "fit_nonlinear_regression"),
    ("src/domain/statistics/split_plot_design.cpp", "generate_split_plot_design"),
    ("tests/algorithm_wave11_track_test.cpp", "simpleCorrespondenceFormulaReference"),
    ("tests/algorithm_wave11_track_test.cpp", "multipleCorrespondenceFormulaReference"),
    ("tests/algorithm_wave11_track_test.cpp", "nonlinearRegressionFormulaReference"),
    ("tests/algorithm_wave11_track_test.cpp", "splitPlotDesignFormulaReference"),
    ("tests/algorithm_wave11_track_test.cpp", "SimpleCorrespondenceFacts"),
    ("tests/algorithm_wave11_track_test.cpp", "MultipleCorrespondenceFacts"),
    ("tests/algorithm_wave11_track_test.cpp", "NonlinearRegressionFacts"),
    ("tests/algorithm_wave11_track_test.cpp", "SplitPlotDesignFacts"),
]

INTERP_KEYS = [
    "simple_correspondence_summary",
    "simple_correspondence_method",
    "simple_correspondence_scope",
    "multiple_correspondence_summary",
    "multiple_correspondence_method",
    "multiple_correspondence_scope",
    "nonlinear_regression_summary",
    "nonlinear_regression_method",
    "nonlinear_regression_scope",
    "split_plot_design_summary",
    "split_plot_design_method",
    "split_plot_design_scope",
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
    if "algorithm_wave11_track_test" not in cmake:
        fail("CMakeLists missing algorithm_wave11_track_test")
    for src in (
        "simple_correspondence.cpp",
        "multiple_correspondence.cpp",
        "nonlinear_regression.cpp",
        "split_plot_design.cpp",
        "simple_correspondence_dialog.cpp",
        "multiple_correspondence_dialog.cpp",
        "nonlinear_regression_dialog.cpp",
        "split_plot_design_dialog.cpp",
        "report_text_catalog_part22.cpp",
    ):
        if src not in cmake:
            fail(f"CMakeLists missing {src}")
    ok("CMake wave11 sources + dialogs + test target")

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
    dod = (ROOT / "docs/research/goal-wave-2026-09-01-algorithm-wave11.md").read_text(
        encoding="utf-8"
    )
    wave11_tests = (ROOT / "tests/algorithm_wave11_track_test.cpp").read_text(
        encoding="utf-8"
    )
    serialization = (
        ROOT / "src/infrastructure/output_serialization.cpp"
    ).read_text(encoding="utf-8")
    catalog = (ROOT / "src/domain/report_text_catalog_part22.cpp").read_text(
        encoding="utf-8"
    )
    mainwindow = (ROOT / "src/ui/mainwindow.cpp").read_text(encoding="utf-8")

    for path, marker in WAVE11_MARKERS:
        text = (ROOT / path).read_text(encoding="utf-8")
        if marker not in text:
            fail(f"Wave-11 marker missing {marker!r} in {path}")
    ok("Wave-11 service/domain/test markers")

    for cmd_id, research in WAVE11:
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
        if "2026-09-01" not in md:
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
        "SimpleCorrespondenceFacts",
        "MultipleCorrespondenceFacts",
        "NonlinearRegressionFacts",
        "SplitPlotDesignFacts",
    ):
        if facts not in serialization:
            fail(f"serialization missing {facts}")
    ok("Facts serialize round-trip markers")

    if "# source: formula_reference" not in wave11_tests:
        fail("wave11 tests missing formula_reference marker")
    ok("formula_reference / test id markers")

    dialog_pages = {
        "src/ui/simple_correspondence_dialog.cpp": 4,
        "src/ui/multiple_correspondence_dialog.cpp": 4,
        "src/ui/nonlinear_regression_dialog.cpp": 4,
        "src/ui/split_plot_design_dialog.cpp": 4,
    }
    for rel, min_pages in dialog_pages.items():
        text = (ROOT / rel).read_text(encoding="utf-8")
        count = text.count("stack_->addWidget")
        if count < min_pages:
            fail(f"{rel} pages {count} < required {min_pages}")
    ok("UI multi-page minimums")

    sc_dialog = (ROOT / "src/ui/simple_correspondence_dialog.cpp").read_text(encoding="utf-8")
    mc_dialog = (ROOT / "src/ui/multiple_correspondence_dialog.cpp").read_text(encoding="utf-8")
    nr_dialog = (ROOT / "src/ui/nonlinear_regression_dialog.cpp").read_text(encoding="utf-8")
    spd_dialog = (ROOT / "src/ui/split_plot_design_dialog.cpp").read_text(encoding="utf-8")
    def embeds_foreign_dialog(text: str, foreign_token: str, own_token: str) -> bool:
        for line in text.splitlines():
            stripped = line.strip()
            if stripped.startswith("#include"):
                continue
            if foreign_token in line and own_token not in line:
                return True
        return False

    if embeds_foreign_dialog(sc_dialog, "multiple_correspondence", "simple_correspondence"):
        fail("simple_correspondence dialog must not embed multiple_correspondence")
    if embeds_foreign_dialog(mc_dialog, "simple_correspondence", "multiple_correspondence"):
        fail("multiple_correspondence dialog must not embed simple_correspondence")
    if embeds_foreign_dialog(nr_dialog, "linear_regression_dialog", "nonlinear_regression"):
        fail("nonlinear_regression dialog must not embed linear_regression")
    if embeds_foreign_dialog(spd_dialog, "split_plot_analyze_dialog", "split_plot_design"):
        fail("split_plot_design dialog must not embed split_plot_analyze workflow")
    ok("independent dialogs")

    for token in (
        "W11-1",
        "W11-2",
        "W11-3",
        "W11-4",
        "simple_correspondence",
        "multiple_correspondence",
        "nonlinear_regression",
        "split_plot_design",
    ):
        if token not in dod:
            fail(f"DoD missing {token}")
    unchecked = re.findall(r"^- \[ \] (.+)$", dod, flags=re.M)
    allowed_unchecked = {
        "Checker 无 Critical",
        "告知用户 Qt Creator Rebuild 手测四点",
        "回归 `verify_algorithm_wave10_track.py` PASS",
        "回归 `verify_algorithm_wave9_track.py` PASS",
        "回归 `verify_ui_menu_ia_track.py` PASS",
        "Planner 映射表已产出",
        "`verify_algorithm_wave11_track.py` PASS",
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

    if "Wave-11" not in acceptance and "算法 Wave-11" not in acceptance:
        fail("acceptance missing Wave-11 section")
    if "verify_algorithm_wave11_track.py" not in acceptance:
        fail("acceptance missing wave11 verify script")
    if "15.7" not in backlog:
        fail("backlog missing §15.7 Wave-11")
    if "✅" not in backlog.split("15.7")[1][:900]:
        fail("backlog Wave-11 section missing ✅")
    ok("backlog checkmarks + acceptance Wave-11")

    expected_groups = {
        "simple_correspondence": "多变量",
        "multiple_correspondence": "多变量",
        "nonlinear_regression": "回归",
        "split_plot_design": "DOE",
    }
    for cid, group in expected_groups.items():
        idx = commands.find(f'QStringLiteral("{cid}")')
        if idx < 0:
            fail(f"commands missing id {cid}")
        window = commands[idx : idx + 3500]
        if f'QStringLiteral("{group}")' not in window:
            fail(f"commands {cid} missing menu_group {group}")
    ok("menu_group wiring")

    print("\nalgorithm wave11 preflight: PASS (4 commands wired)")


if __name__ == "__main__":
    main()
