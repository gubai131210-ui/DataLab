#!/usr/bin/env python3
"""Preflight for algorithm Wave-2 vertical slice (2026-08-22)."""

from __future__ import annotations

import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED = [
    "docs/research/goal-wave-2026-08-22-algorithm-wave2.md",
    "docs/research/p3_nominal_logistic.md",
    "docs/research/p3_nonparametric_capability.md",
    "docs/research/p3_accelerated_life.md",
    "docs/research/p3_stepwise_regression.md",
    "src/domain/statistics/nominal_logistic.cpp",
    "src/domain/statistics/nonparametric_capability.cpp",
    "src/domain/statistics/accelerated_life.cpp",
    "tests/algorithm_wave2_track_test.cpp",
    "samples/product_evolution/unified_track_acceptance_plan.md",
]

WAVE2 = [
    ("nominal_logistic", "p3_nominal_logistic.md"),
    ("nonparametric_capability", "p3_nonparametric_capability.md"),
    ("accelerated_life", "p3_accelerated_life.md"),
]

STEPWISE_MARKERS = [
    "forward_aicc",
    "forward_bic",
    "AICc",
    "best_aicc",
    "criterion",
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
    if "algorithm_wave2_track_test" not in cmake:
        fail("CMakeLists missing algorithm_wave2_track_test")
    ok("CMake test target")

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
    tests = (ROOT / "tests/algorithm_wave2_track_test.cpp").read_text(encoding="utf-8")

    for cmd_id, research in WAVE2:
        if cmd_id not in by_id:
            fail(f"help missing {cmd_id}")
        if f'QStringLiteral("{cmd_id}")' not in commands:
            fail(f"commands missing {cmd_id}")
        if f"AnalysisService::{cmd_id}" not in service:
            fail(f"service missing {cmd_id}")
        md = (ROOT / "docs/research" / research).read_text(encoding="utf-8")
        if "support.minitab.com" not in md or "2026-08-22" not in md:
            fail(f"{research} missing Primary URL or date")
        if cmd_id not in wiring:
            fail(f"wiring-index missing {cmd_id}")
        if cmd_id not in interpretation:
            fail(f"interpretation missing {cmd_id}")
        ok(cmd_id)

    for marker in STEPWISE_MARKERS:
        if marker not in commands and marker not in service:
            fail(f"stepwise deepen missing {marker}")
    if "stepwiseForwardAiccTableShape" not in tests:
        fail("test missing stepwise AICc slot")
    ok("stepwise_regression deepen")

    acceptance = (ROOT / "samples/product_evolution/unified_track_acceptance_plan.md").read_text(
        encoding="utf-8"
    )
    for item in (
        "nominal_logistic",
        "nonparametric_capability",
        "stepwise_regression",
        "accelerated_life",
    ):
        if item not in acceptance:
            fail(f"acceptance plan missing {item}")
    ok("acceptance plan §2 wave2")

    backlog = (ROOT / "docs/research/minitab-market-algorithm-backlog.md").read_text(
        encoding="utf-8"
    )
    for token in ("nominal_logistic", "nonparametric_capability", "accelerated_life"):
        if token not in backlog:
            fail(f"backlog missing {token}")
    ok("backlog references")

    if "# source: formula_reference" not in tests:
        fail("tests missing formula_reference marker")
    ok("formula_reference tests")

    print("\nalgorithm wave2 preflight: PASS (4 items wired)")


if __name__ == "__main__":
    main()
