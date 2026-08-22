#!/usr/bin/env python3
"""Independent reference_implementation for Phase 5 KM product-limit steps.

Matches the hand-derived baseline documented in:
  samples/phase0_baselines/reliability_km_handcalc.md
  samples/phase0_baselines/reliability_km_handcalc.csv

Evidence type: reference_implementation (NOT vendor_oracle, NOT golden).

Usage:
  python scripts/reliability_km_reference.py
"""

from __future__ import annotations

import csv
import math
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HANDCALC_CSV = ROOT / "samples" / "phase0_baselines" / "reliability_km_handcalc.csv"


@dataclass(frozen=True)
class KmStep:
    time: float
    risk_set: int
    deaths: int
    survival: float
    is_failure: bool


def nearly_equal(a: float, b: float, tol: float = 1e-9) -> bool:
    return math.fabs(a - b) <= tol


def kaplan_meier_product_limit(
    times: list[float], events: list[bool]
) -> tuple[list[KmStep], int, int]:
    """Classic right-censored KM with failure steps only on event times."""
    if len(times) != len(events):
        raise ValueError("times/events length mismatch")

    indexed = sorted(zip(times, events), key=lambda pair: pair[0])
    n = len(indexed)
    survival = 1.0
    steps: list[KmStep] = []
    failure_count = 0
    censored_count = 0

    unique_times = sorted({t for t, _ in indexed})
    for t in unique_times:
        at_t = [(time, event) for time, event in indexed if nearly_equal(time, t)]
        risk_set = sum(1 for time, _ in indexed if time >= t - 1e-12)
        deaths = sum(1 for _, event in at_t if event)
        censored = sum(1 for _, event in at_t if not event)
        censored_count += censored

        if deaths > 0:
            failure_count += deaths
            survival *= (risk_set - deaths) / risk_set
            steps.append(
                KmStep(
                    time=t,
                    risk_set=risk_set,
                    deaths=deaths,
                    survival=survival,
                    is_failure=True,
                )
            )

    return steps, failure_count, censored_count


def load_handcalc_csv(path: Path) -> tuple[list[float], list[bool]]:
    times: list[float] = []
    events: list[bool] = []
    with path.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            times.append(float(row["Time"]))
            events.append(row["Event"].strip() in {"1", "true", "True", "exact"})
    return times, events


def assert_handcalc_baseline() -> None:
    times = [10.0, 15.0, 20.0, 25.0, 30.0]
    events = [True, False, True, False, True]
    steps, failures, censored = kaplan_meier_product_limit(times, events)

    assert failures == 3, failures
    assert censored == 2, censored
    assert len(steps) == 3, [s.time for s in steps]

    assert nearly_equal(steps[0].time, 10.0)
    assert steps[0].risk_set == 5
    assert steps[0].deaths == 1
    assert nearly_equal(steps[0].survival, 0.8)

    step20 = next(s for s in steps if nearly_equal(s.time, 20.0))
    assert step20.risk_set == 3
    assert step20.deaths == 1
    assert nearly_equal(step20.survival, 0.8 * 2.0 / 3.0, 1e-6)

    step30 = next(s for s in steps if nearly_equal(s.time, 30.0))
    assert step30.risk_set == 1
    assert step30.deaths == 1
    assert nearly_equal(step30.survival, 0.0)

    failure_times = [s.time for s in steps]
    assert failure_times == [10.0, 20.0, 30.0]
    assert 15.0 not in failure_times
    assert 25.0 not in failure_times


def assert_matches_csv_fixture() -> None:
    times, events = load_handcalc_csv(HANDCALC_CSV)
    steps, failures, censored = kaplan_meier_product_limit(times, events)
    assert failures == 3
    assert censored == 2
    assert nearly_equal(steps[-1].survival, 0.0)
    assert [s.time for s in steps] == [10.0, 20.0, 30.0]


def main() -> None:
    assert_handcalc_baseline()
    assert_matches_csv_fixture()
    print(
        "reference_implementation OK: KM handcalc baseline "
        f"({HANDCALC_CSV.name}) — not vendor_oracle"
    )


if __name__ == "__main__":
    main()
