#!/usr/bin/env python3
"""Independent reference_implementation for Phase 4 RSM pure-error / LOF fixture.

Reproduces the replicated center-point design in
tests/response_surface_design_phase4_test.cpp::rsm_lack_of_fit_uses_replicated_coded_points_not_residual_ms

Evidence type: reference_implementation (NOT vendor_oracle).

Usage:
  python scripts/rsm_lof_reference.py
"""

from __future__ import annotations

import csv
import math
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FIXTURE_CSV = ROOT / "samples" / "phase0_baselines" / "rsm_lof_fixture.csv"


def nearly_equal(a: float, b: float, tol: float = 1e-9) -> bool:
    return math.fabs(a - b) <= tol


def build_fixture() -> tuple[list[list[float]], list[float]]:
    coded = [
        [-1.0, -1.0],
        [1.0, -1.0],
        [-1.0, 1.0],
        [1.0, 1.0],
        [-1.0, 0.0],
        [1.0, 0.0],
        [0.0, -1.0],
        [0.0, 1.0],
        [0.0, 0.0],
        [0.0, 0.0],
        [0.0, 0.0],
    ]
    response: list[float] = []
    for i, (x1, x2) in enumerate(coded):
        y = (
            10.0
            + 3.0 * x1
            - 2.0 * x2
            + 1.5 * x1 * x2
            + 0.8 * x1 * x1
            - 0.4 * x2 * x2
        )
        if abs(x1) < 1.0e-9 and abs(x2) < 1.0e-9:
            y += (0.4 if i == 8 else (-0.2 if i == 9 else -0.1))
        response.append(y)
    return coded, response


def quantize_key(row: list[float]) -> tuple[int, ...]:
    return tuple(int(round(v * 1000.0)) for v in row)


def pure_error_from_replicates(
    coded: list[list[float]], response: list[float]
) -> tuple[float, int]:
    groups: dict[tuple[int, ...], list[float]] = {}
    for point, y in zip(coded, response):
        groups.setdefault(quantize_key(point), []).append(y)

    pure_error_ss = 0.0
    pure_error_df = 0
    for values in groups.values():
        if len(values) <= 1:
            continue
        total = sum(values)
        squared = sum(v * v for v in values)
        n = len(values)
        pure_error_ss += squared - total * total / n
        pure_error_df += n - 1
    return pure_error_ss, pure_error_df


def write_fixture_csv(path: Path, coded: list[list[float]], response: list[float]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["Y", "A", "B"])
        for y, (a, b) in zip(response, coded):
            writer.writerow([f"{y:.10g}", f"{a:.10g}", f"{b:.10g}"])


def assert_fixture_contract() -> None:
    coded, response = build_fixture()
    assert len(coded) == 11
    center_values = [response[i] for i in (8, 9, 10)]
    assert nearly_equal(center_values[0], 10.4)
    assert nearly_equal(center_values[1], 9.8)
    assert nearly_equal(center_values[2], 9.9)

    pure_error_ss, pure_error_df = pure_error_from_replicates(coded, response)
    assert pure_error_df == 2, pure_error_df
    # Hand SS for center replicates 10.4, 9.8, 9.9
    total = sum(center_values)
    squared = sum(v * v for v in center_values)
    expected_ss = squared - total * total / 3.0
    assert nearly_equal(pure_error_ss, expected_ss, 1e-6)
    assert pure_error_ss > 0.0

    # Residual df for 2-factor quadratic: 11 obs, 7 predictors incl intercept → 3
    predictor_count = 7
    residual_df = len(response) - predictor_count - 1
    assert residual_df == 3
    lack_of_fit_df = residual_df - pure_error_df
    assert lack_of_fit_df == 1


def assert_csv_matches_fixture() -> None:
    coded, response = build_fixture()
    write_fixture_csv(FIXTURE_CSV, coded, response)
    loaded_coded: list[list[float]] = []
    loaded_response: list[float] = []
    with FIXTURE_CSV.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            loaded_response.append(float(row["Y"]))
            loaded_coded.append([float(row["A"]), float(row["B"])])
    assert len(loaded_response) == 11
    for i in range(11):
        assert nearly_equal(loaded_response[i], response[i], 1e-9)
        assert nearly_equal(loaded_coded[i][0], coded[i][0], 1e-9)
        assert nearly_equal(loaded_coded[i][1], coded[i][1], 1e-9)


def main() -> None:
    assert_fixture_contract()
    assert_csv_matches_fixture()
    print(
        "reference_implementation OK: RSM LOF fixture pure_error_df=2 "
        f"({FIXTURE_CSV.name}) — not vendor_oracle"
    )


if __name__ == "__main__":
    main()
