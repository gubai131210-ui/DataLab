#!/usr/bin/env python3
"""Preflight for algorithm Wave-2.5 hardening + Wave-3 infer/reliability (2026-08-22)."""

from __future__ import annotations

import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED = [
    "docs/research/goal-wave-2026-08-22-algorithm-wave3-infer-reliability.md",
    "docs/research/p3_bootstrap_two_sample.md",
    "docs/research/p3_km_logrank_multigroup.md",
    "docs/research/p3_probit_reliability.md",
    "docs/research/p3_nominal_logistic.md",
    "docs/research/p3_accelerated_life.md",
    "src/domain/statistics/nominal_logistic.cpp",
    "src/domain/statistics/accelerated_life.cpp",
    "src/domain/statistics/bootstrap_two_sample.cpp",
    "src/domain/statistics/probit_reliability.cpp",
    "tests/algorithm_wave2_hardening_test.cpp",
    "tests/algorithm_wave3_track_test.cpp",
    "samples/product_evolution/unified_track_acceptance_plan.md",
]

WAVE3 = [
    ("bootstrap_two_sample", "p3_bootstrap_two_sample.md"),
    ("probit_reliability", "p3_probit_reliability.md"),
]

HARDENING_MARKERS = [
    ("src/domain/statistics/nominal_logistic.cpp", "IRLS"),
    ("src/domain/statistics/accelerated_life.cpp", "Newton"),
    ("src/domain/statistics/accelerated_life.cpp", "观测信息矩阵"),
    ("tests/algorithm_wave2_hardening_test.cpp", "nominalLogisticUsesIrls"),
    ("tests/algorithm_wave2_hardening_test.cpp", "SerializationRoundTrip"),
]

WAVE3_MARKERS = [
    ("src/domain/statistics/reliability.h", "log_rank_k_groups"),
    ("src/domain/statistics/accelerated_life.h", "b10_at_use_stress"),
    ("tests/algorithm_wave3_track_test.cpp", "logRankKGroupsThreeGroups"),
    ("tests/algorithm_wave3_track_test.cpp", "acceleratedLifeUseStressPercentiles"),
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
    for target in ("algorithm_wave2_hardening_test", "algorithm_wave3_track_test"):
        if target not in cmake:
            fail(f"CMakeLists missing {target}")
    ok("CMake test targets")

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
    wave3_tests = (ROOT / "tests/algorithm_wave3_track_test.cpp").read_text(encoding="utf-8")
    hardening_tests = (ROOT / "tests/algorithm_wave2_hardening_test.cpp").read_text(
        encoding="utf-8"
    )

    for path, marker in HARDENING_MARKERS:
        text = (ROOT / path).read_text(encoding="utf-8")
        if marker not in text:
            fail(f"Wave-2.5 hardening marker missing {marker!r} in {path}")
    ok("Wave-2.5 hardening markers")

    for path, marker in WAVE3_MARKERS:
        text = (ROOT / path).read_text(encoding="utf-8")
        if marker not in text:
            fail(f"Wave-3 marker missing {marker!r} in {path}")
    ok("Wave-3 domain/test markers")

    for cmd_id, research in WAVE3:
        if cmd_id not in by_id:
            fail(f"help missing {cmd_id}")
        if f'QStringLiteral("{cmd_id}")' not in commands:
            fail(f"commands missing {cmd_id}")
        if f"AnalysisService::{cmd_id}" not in service:
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

    if "log_rank_k_groups" not in service:
        fail("service reliability path missing log_rank_k_groups")
    ok("reliability KM multigroup")

    if "Percentiles at Design Level" not in service:
        fail("accelerated_life missing use-stress percentile table")
    ok("accelerated_life W3 predict")

    acceptance = (ROOT / "samples/product_evolution/unified_track_acceptance_plan.md").read_text(
        encoding="utf-8"
    )
    for item in (
        "bootstrap_two_sample",
        "probit_reliability",
        "Wave-3",
        "Wave-2.5",
    ):
        if item not in acceptance:
            fail(f"acceptance plan missing {item}")
    ok("acceptance plan §2 wave3")

    backlog = (ROOT / "docs/research/minitab-market-algorithm-backlog.md").read_text(
        encoding="utf-8"
    )
    for token in ("bootstrap_two_sample", "probit_reliability", "log_rank_k_groups"):
        if token not in backlog and token.replace("_", " ") not in backlog:
            # allow partial match for km multigroup wording
            if token == "log_rank_k_groups" and "Log-rank" not in backlog:
                fail(f"backlog missing log-rank update")
            elif token != "log_rank_k_groups":
                fail(f"backlog missing {token}")
    ok("backlog references")

    if "# source: formula_reference" not in wave3_tests:
        fail("wave3 tests missing formula_reference marker")
    if "过程合格" not in hardening_tests and "ForbiddenPhrases" not in hardening_tests:
        fail("hardening tests missing interpret forbidden phrase slot")
    ok("formula_reference / interpret tests")

    print("\nalgorithm wave3 preflight: PASS (Wave-2.5 + 4 items wired)")


if __name__ == "__main__":
    main()
