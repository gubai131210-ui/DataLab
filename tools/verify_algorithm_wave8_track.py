#!/usr/bin/env python3
"""Preflight for algorithm Wave-8 (2026-08-28)."""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED = [
    "docs/research/p8_binary_response_doe.md",
    "docs/research/p8_cluster_variables.md",
    "docs/research/p8_glm_three_factor.md",
    "docs/research/p8_life_data_regression.md",
    "docs/research/goal-wave-2026-08-28-algorithm-wave8.md",
    "docs/research/goal-wave-2026-08-28-algorithm-wave8-plan-and-mega-prompt.md",
    "src/domain/statistics/binary_response_doe.cpp",
    "src/domain/statistics/cluster_variables.cpp",
    "src/domain/statistics/glm_three_factor.cpp",
    "src/domain/statistics/life_data_regression.cpp",
    "src/ui/binary_response_doe_dialog.cpp",
    "src/ui/cluster_variables_dialog.cpp",
    "src/ui/glm_three_factor_dialog.cpp",
    "src/ui/life_data_regression_dialog.cpp",
    "src/domain/report_text_catalog_part19.cpp",
    "tests/algorithm_wave8_track_test.cpp",
    "samples/product_evolution/unified_track_acceptance_plan.md",
]

WAVE8 = [
    ("binary_response_doe", "p8_binary_response_doe.md"),
    ("cluster_variables", "p8_cluster_variables.md"),
    ("glm_three_factor", "p8_glm_three_factor.md"),
    ("life_data_regression", "p8_life_data_regression.md"),
]

WAVE8_MARKERS = [
    ("src/application/analysis_service.cpp", "AnalysisService::binary_response_doe"),
    ("src/application/analysis_service.cpp", "AnalysisService::cluster_variables"),
    ("src/application/analysis_service.cpp", "AnalysisService::glm_three_factor"),
    ("src/application/analysis_service.cpp", "AnalysisService::life_data_regression"),
    ("src/domain/statistics/binary_response_doe.cpp", "analyze_binary_response_doe"),
    ("src/domain/statistics/cluster_variables.cpp", "cluster_variables_analyze"),
    ("src/domain/statistics/glm_three_factor.cpp", "glm_three_factor_analyze"),
    ("src/domain/statistics/life_data_regression.cpp", "fit_life_data_regression_weibull"),
    ("tests/algorithm_wave8_track_test.cpp", "binaryDoeFormulaReference"),
    ("tests/algorithm_wave8_track_test.cpp", "clusterVariablesFormulaReference"),
    ("tests/algorithm_wave8_track_test.cpp", "glmThreeFactorFormulaReference"),
    ("tests/algorithm_wave8_track_test.cpp", "lifeRegressionFormulaReference"),
    ("tests/algorithm_wave8_track_test.cpp", "BinaryResponseDoeFacts"),
    ("tests/algorithm_wave8_track_test.cpp", "ClusterVariablesFacts"),
    ("tests/algorithm_wave8_track_test.cpp", "GlmThreeFactorFacts"),
    ("tests/algorithm_wave8_track_test.cpp", "LifeDataRegressionFacts"),
]

INTERP_KEYS = [
    "binary_response_doe_summary",
    "binary_response_doe_or",
    "binary_response_doe_scope",
    "cluster_variables_summary",
    "cluster_variables_dendrogram",
    "cluster_variables_scope",
    "glm_three_factor_summary",
    "glm_three_factor_fitted",
    "glm_three_factor_scope",
    "life_data_regression_summary",
    "life_data_regression_coef",
    "life_data_regression_scope",
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
    if "algorithm_wave8_track_test" not in cmake:
        fail("CMakeLists missing algorithm_wave8_track_test")
    for src in (
        "binary_response_doe.cpp",
        "cluster_variables.cpp",
        "glm_three_factor.cpp",
        "life_data_regression.cpp",
        "binary_response_doe_dialog.cpp",
        "cluster_variables_dialog.cpp",
        "glm_three_factor_dialog.cpp",
        "life_data_regression_dialog.cpp",
        "report_text_catalog_part19.cpp",
    ):
        if src not in cmake:
            fail(f"CMakeLists missing {src}")
    ok("CMake wave8 sources + dialogs + test target")

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
    dod = (ROOT / "docs/research/goal-wave-2026-08-28-algorithm-wave8.md").read_text(
        encoding="utf-8"
    )
    wave8_tests = (ROOT / "tests/algorithm_wave8_track_test.cpp").read_text(
        encoding="utf-8"
    )
    serialization = (
        ROOT / "src/infrastructure/output_serialization.cpp"
    ).read_text(encoding="utf-8")
    catalog = (ROOT / "src/domain/report_text_catalog_part19.cpp").read_text(
        encoding="utf-8"
    )
    mainwindow = (ROOT / "src/ui/mainwindow.cpp").read_text(encoding="utf-8")

    for path, marker in WAVE8_MARKERS:
        text = (ROOT / path).read_text(encoding="utf-8")
        if marker not in text:
            fail(f"Wave-8 marker missing {marker!r} in {path}")
    ok("Wave-8 service/domain/test markers")

    for cmd_id, research in WAVE8:
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
        "BinaryResponseDoeFacts",
        "ClusterVariablesFacts",
        "GlmThreeFactorFacts",
        "LifeDataRegressionFacts",
    ):
        if facts not in serialization:
            fail(f"serialization missing {facts}")
    ok("Facts serialize round-trip markers")

    if "# source: formula_reference" not in wave8_tests:
        fail("wave8 tests missing formula_reference marker")
    for required_test_marker in (
        "binary_response_doe",
        "cluster_variables",
        "glm_three_factor",
        "life_data_regression",
        "BinaryResponseDoeFacts",
        "ClusterVariablesFacts",
        "GlmThreeFactorFacts",
        "LifeDataRegressionFacts",
        "formula_reference",
    ):
        if required_test_marker not in wave8_tests:
            fail(f"wave8 tests missing marker {required_test_marker}")
    ok("formula_reference / test id markers")

    dialog_pages = {
        "src/ui/binary_response_doe_dialog.cpp": 4,
        "src/ui/cluster_variables_dialog.cpp": 4,
        "src/ui/glm_three_factor_dialog.cpp": 4,
        "src/ui/life_data_regression_dialog.cpp": 4,
    }
    for rel, min_pages in dialog_pages.items():
        text = (ROOT / rel).read_text(encoding="utf-8")
        count = text.count("stack_->addWidget")
        if count < min_pages:
            fail(f"{rel} pages {count} < required {min_pages}")
    ok("UI multi-page minimums")

    binary_dialog = (ROOT / "src/ui/binary_response_doe_dialog.cpp").read_text(
        encoding="utf-8"
    )
    glm3_dialog = (ROOT / "src/ui/glm_three_factor_dialog.cpp").read_text(encoding="utf-8")
    if "logistic_regression" in binary_dialog:
        fail("binary_response_doe dialog must not embed logistic_regression")
    if "glm_two_way" in glm3_dialog:
        fail("glm_three_factor dialog must not embed glm_two_way")
    ok("independent dialogs")

    for token in ("W8-1", "W8-2", "W8-3", "W8-4", "binary_response_doe", "life_data_regression"):
        if token not in dod:
            fail(f"DoD missing {token}")
    unchecked = re.findall(r"^- \[ \] (.+)$", dod, flags=re.M)
    allowed_unchecked = {
        "Checker 无 Critical",
        "告知用户 Qt Creator Rebuild 手测四点",
        "回归 `verify_ui_menu_ia_track.py` PASS",
        "回归 `verify_algorithm_wave7_track.py` PASS",
        "回归 `verify_algorithm_wave6_track.py` PASS",
        "Planner 映射表已产出",
        "`verify_algorithm_wave8_track.py` PASS",
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

    if "Wave-8" not in acceptance and "算法 Wave-8" not in acceptance:
        fail("acceptance missing Wave-8 section")
    if "verify_algorithm_wave8_track.py" not in acceptance:
        fail("acceptance missing wave8 verify script")
    if "15.4" not in backlog:
        fail("backlog missing §15.4 Wave-8")
    if "✅" not in backlog.split("15.4")[1][:900]:
        fail("backlog Wave-8 section missing ✅")
    ok("backlog checkmarks + acceptance Wave-8")

    expected_groups = {
        "binary_response_doe": "DOE",
        "cluster_variables": "多变量",
        "glm_three_factor": "ANOVA",
        "life_data_regression": "可靠性",
    }
    for cid, group in expected_groups.items():
        idx = commands.find(f'QStringLiteral("{cid}")')
        if idx < 0:
            fail(f"commands missing id {cid}")
        window = commands[idx : idx + 3500]
        if f'QStringLiteral("{group}")' not in window:
            fail(f"commands {cid} missing menu_group {group}")
    ok("menu_group wiring")

    print("\nalgorithm wave8 preflight: PASS (4 commands wired)")


if __name__ == "__main__":
    main()
