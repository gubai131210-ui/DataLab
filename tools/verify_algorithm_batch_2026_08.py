#!/usr/bin/env python3
"""Preflight for algorithm vertical-slice batch 2026-08-22 (A1–A3)."""

from __future__ import annotations

import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED = [
    "docs/research/algorithm-batch-2026-08-22-plan.md",
    "docs/research/p3_best_subsets_regression.md",
    "docs/research/p3_batch_capability.md",
    "docs/research/p1_logistic_diagnostics_minitab.md",
    "src/domain/statistics/best_subsets_regression.cpp",
    "src/domain/statistics/best_subsets_regression.h",
    "src/domain/statistics/batch_capability.cpp",
    "src/domain/statistics/batch_capability.h",
    "tests/algorithm_batch_2026_08_test.cpp",
    "samples/product_evolution/unified_track_acceptance_plan.md",
]

BATCH = [
    ("best_subsets_regression", "p3_best_subsets_regression.md"),
    ("batch_capability", "p3_batch_capability.md"),
]

LOGISTIC_MARKERS = [
    "concordant_pairs",
    "关联统计（配对）",
    "分类表（阈值 0.5）",
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
    if "algorithm_batch_2026_08_test" not in cmake:
        fail("CMakeLists missing algorithm_batch_2026_08_test")
    ok("CMake test target")

    help_entries = json.loads(
        (ROOT / "resources/help/algorithm_help.json").read_text(encoding="utf-8")
    ).get("entries", [])
    by_id = {e.get("id"): e for e in help_entries}

    commands = (ROOT / "src/ui/analysis_commands.cpp").read_text(encoding="utf-8")
    service = (ROOT / "src/application/analysis_service.cpp").read_text(encoding="utf-8")
    tests = (ROOT / "tests/algorithm_batch_2026_08_test.cpp").read_text(encoding="utf-8")

    for cmd_id, research in BATCH:
        if cmd_id not in by_id:
            fail(f"help missing {cmd_id}")
        if f'QStringLiteral("{cmd_id}")' not in commands:
            fail(f"commands missing {cmd_id}")
        if f"AnalysisService::{cmd_id}" not in service and cmd_id not in service:
            fail(f"service missing {cmd_id}")
        md = (ROOT / "docs/research" / research).read_text(encoding="utf-8")
        if "support.minitab.com" not in md or "2026-08-22" not in md:
            fail(f"{research} missing Primary URL or date")
        ok(cmd_id)

    for marker in LOGISTIC_MARKERS:
        if marker not in service:
            fail(f"logistic deepen missing {marker}")
    if "logisticConcordanceAndClassification" not in tests:
        fail("test missing logistic deepen slot")
    if "batchCapabilityGroupsByBatch" not in tests:
        fail("test missing batch capability slot")
    ok("logistic_regression deepen")

    acceptance = (ROOT / "samples/product_evolution/unified_track_acceptance_plan.md").read_text(
        encoding="utf-8"
    )
    for item in ("best_subsets_regression", "logistic_regression", "batch_capability"):
        if item not in acceptance:
            fail(f"acceptance plan missing {item}")
    ok("acceptance plan §2 algorithm batch")

    if "# source: formula_reference" not in tests:
        fail("tests missing formula_reference marker")
    ok("formula_reference tests")

    print("\nalgorithm batch 2026-08 preflight: PASS (A1–A3 wired)")


if __name__ == "__main__":
    main()
