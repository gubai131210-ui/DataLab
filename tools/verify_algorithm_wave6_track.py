#!/usr/bin/env python3
"""Preflight for algorithm Wave-6 (2026-08-25)."""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED = [
    "docs/research/p6_taguchi_analyze.md",
    "docs/research/p6_mixture_design.md",
    "docs/research/p6_nhpp_repairable.md",
    "docs/research/p6_reliability_test_plan.md",
    "docs/research/goal-wave-2026-08-25-algorithm-wave6.md",
    "docs/research/goal-wave-2026-08-25-algorithm-wave6-plan-and-mega-prompt.md",
    "src/domain/statistics/taguchi_analyze.cpp",
    "src/domain/statistics/mixture_design.cpp",
    "src/domain/statistics/nhpp_repairable.cpp",
    "src/domain/statistics/reliability_test_plan.cpp",
    "src/ui/taguchi_analyze_dialog.cpp",
    "src/ui/mixture_design_dialog.cpp",
    "src/ui/nhpp_repairable_dialog.cpp",
    "src/ui/reliability_test_plan_dialog.cpp",
    "tests/algorithm_wave6_track_test.cpp",
    "samples/product_evolution/unified_track_acceptance_plan.md",
]

WAVE6 = [
    ("taguchi_analyze", "p6_taguchi_analyze.md"),
    ("mixture_design", "p6_mixture_design.md"),
    ("nhpp_repairable", "p6_nhpp_repairable.md"),
    ("reliability_test_plan", "p6_reliability_test_plan.md"),
]

WAVE6_MARKERS = [
    ("src/application/analysis_service.cpp", "AnalysisService::taguchi_analyze"),
    ("src/application/analysis_service.cpp", "AnalysisService::mixture_design"),
    ("src/application/analysis_service.cpp", "AnalysisService::nhpp_repairable"),
    ("src/application/analysis_service.cpp", "AnalysisService::reliability_test_plan"),
    ("src/domain/statistics/taguchi_analyze.cpp", "analyze_taguchi_static"),
    ("src/domain/statistics/mixture_design.cpp", "generate_mixture_simplex_lattice"),
    ("src/domain/statistics/nhpp_repairable.cpp", "fit_nhpp_crow_amsaa"),
    ("src/domain/statistics/reliability_test_plan.cpp", "plan_reliability_demonstration"),
    ("tests/algorithm_wave6_track_test.cpp", "taguchiServiceAndSerialize"),
    ("tests/algorithm_wave6_track_test.cpp", "mixtureExportClearsExcludes"),
    ("tests/algorithm_wave6_track_test.cpp", "nhppBetaFormulaReference"),
    ("tests/algorithm_wave6_track_test.cpp", "rtpMonotonicityFormulaReference"),
    ("tests/algorithm_wave6_track_test.cpp", "TaguchiAnalyzeFacts"),
    ("tests/algorithm_wave6_track_test.cpp", "MixtureDesignFacts"),
    ("tests/algorithm_wave6_track_test.cpp", "NhppRepairableFacts"),
    ("tests/algorithm_wave6_track_test.cpp", "ReliabilityTestPlanFacts"),
]

INTERP_KEYS = [
    "taguchi_analyze_summary",
    "taguchi_analyze_sn",
    "taguchi_analyze_scope",
    "mixture_design_summary",
    "mixture_design_export_hint",
    "mixture_design_scope",
    "nhpp_summary",
    "nhpp_params",
    "nhpp_scope",
    "rtp_summary",
    "rtp_assumptions",
    "rtp_scope",
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
    if "algorithm_wave6_track_test" not in cmake:
        fail("CMakeLists missing algorithm_wave6_track_test")
    for src in (
        "taguchi_analyze.cpp",
        "mixture_design.cpp",
        "nhpp_repairable.cpp",
        "reliability_test_plan.cpp",
        "taguchi_analyze_dialog.cpp",
        "mixture_design_dialog.cpp",
        "nhpp_repairable_dialog.cpp",
        "reliability_test_plan_dialog.cpp",
    ):
        if src not in cmake:
            fail(f"CMakeLists missing {src}")
    ok("CMake wave6 sources + dialogs + test target")

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
    dod = (ROOT / "docs/research/goal-wave-2026-08-25-algorithm-wave6.md").read_text(
        encoding="utf-8"
    )
    wave6_tests = (ROOT / "tests/algorithm_wave6_track_test.cpp").read_text(
        encoding="utf-8"
    )
    serialization = (
        ROOT / "src/infrastructure/output_serialization.cpp"
    ).read_text(encoding="utf-8")
    catalog = (ROOT / "src/domain/report_text_catalog_part17.cpp").read_text(
        encoding="utf-8"
    )
    mainwindow = (ROOT / "src/ui/mainwindow.cpp").read_text(encoding="utf-8")

    for path, marker in WAVE6_MARKERS:
        text = (ROOT / path).read_text(encoding="utf-8")
        if marker not in text:
            fail(f"Wave-6 marker missing {marker!r} in {path}")
    ok("Wave-6 service/domain/test markers")

    for cmd_id, research in WAVE6:
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
        if "2026-08-25" not in md:
            fail(f"{research} missing date")
        if "http" not in md and "https://" not in md:
            fail(f"{research} missing Primary URL")
        if cmd_id not in wiring:
            fail(f"wiring-index missing {cmd_id}")
        if cmd_id not in interpretation and cmd_id.replace("_", "") not in interpretation:
            # keys may appear without exact id string; require at least method name
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
        "TaguchiAnalyzeFacts",
        "MixtureDesignFacts",
        "NhppRepairableFacts",
        "ReliabilityTestPlanFacts",
    ):
        if facts not in serialization:
            fail(f"serialization missing {facts}")
    ok("Facts serialize round-trip markers")

    if "# source: formula_reference" not in wave6_tests:
        fail("wave6 tests missing formula_reference marker")
    for required_test_marker in (
        "taguchi_analyze",
        "mixture_design",
        "nhpp_repairable",
        "reliability_test_plan",
        "TaguchiAnalyzeFacts",
        "NhppRepairableFacts",
        "formula_reference",
        "worksheet_export",
    ):
        if required_test_marker not in wave6_tests:
            fail(f"wave6 tests missing marker {required_test_marker}")
    ok("formula_reference / test id markers")

    # Dialog page minimums (QStackedWidget addWidget counts)
    dialog_pages = {
        "src/ui/taguchi_analyze_dialog.cpp": 4,
        "src/ui/mixture_design_dialog.cpp": 4,
        "src/ui/nhpp_repairable_dialog.cpp": 3,
        "src/ui/reliability_test_plan_dialog.cpp": 3,
    }
    for rel, min_pages in dialog_pages.items():
        text = (ROOT / rel).read_text(encoding="utf-8")
        count = text.count("stack_->addWidget")
        if count < min_pages:
            fail(f"{rel} pages {count} < required {min_pages}")
    ok("UI multi-page minimums")

    # DoD: mark W6 items [x]; allow Checker/Rebuild unchecked
    for token in ("W6-1", "W6-2", "W6-3", "W6-4", "taguchi_analyze", "mixture_design"):
        if token not in dod:
            fail(f"DoD missing {token}")
    # Require implementation checkboxes under each W6 section to be [x]
    # Exclude Wave gate Checker / Rebuild lines that may remain [ ]
    unchecked = re.findall(r"^- \[ \] (.+)$", dod, flags=re.M)
    allowed_unchecked = {
        "Checker 无 Critical",
        "告知用户 Qt Creator Rebuild 手测四点",
        "回归 `verify_ui_menu_ia_track.py` PASS",
    }
    for item in unchecked:
        if item.strip() in allowed_unchecked:
            continue
        # Also allow "禁止偷懒核对" section items until Checker
        if item.startswith("无") or item.startswith("禁止"):
            continue
        fail(f"DoD still unchecked: {item}")
    ok("DoD implementation items [x]")

    if "Wave-6" not in acceptance and "算法 Wave-6" not in acceptance:
        fail("acceptance missing Wave-6 section")
    if "verify_algorithm_wave6_track.py" not in acceptance:
        fail("acceptance missing wave6 verify script")
    if "15.2" not in backlog:
        fail("backlog missing §15.2 Wave-6")
    if "✅" not in backlog.split("15.2")[1][:900]:
        fail("backlog Wave-6 section missing ✅")
    ok("backlog checkmarks + acceptance Wave-6")

    expected_groups = {
        "taguchi_analyze": "DOE",
        "mixture_design": "DOE",
        "nhpp_repairable": "可靠性",
        "reliability_test_plan": "功效与样本量",
    }
    for cid, group in expected_groups.items():
        idx = commands.find(f'QStringLiteral("{cid}")')
        if idx < 0:
            fail(f"commands missing id {cid}")
        window = commands[idx : idx + 3500]
        if f'QStringLiteral("{group}")' not in window:
            fail(f"commands {cid} missing menu_group {group}")
    ok("menu_group wiring")

    print("\nalgorithm wave6 preflight: PASS (4 commands wired)")


if __name__ == "__main__":
    main()
