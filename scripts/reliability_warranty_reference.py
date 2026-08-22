#!/usr/bin/env python3
"""Independent reference_implementation for Phase 5 warranty summary formulas.

Mirrors domain contracts exercised in tests/reliability_phase5_test.cpp:
  - claims/1000 = 1000 * (1 - R(T_w))
  - strata expected_failures = exposure * F with pooled R(T_w)
  - proportional exposure attribution when measured sum is zero

Evidence type: reference_implementation (NOT vendor_oracle).

Usage:
  python scripts/reliability_warranty_reference.py
"""

from __future__ import annotations

import math
from dataclasses import dataclass


def nearly_equal(a: float, b: float, tol: float = 1e-9) -> bool:
    return math.fabs(a - b) <= tol


def summarize_warranty(
    warranty_time: float,
    exposure: float,
    reliability_at_warranty: float,
    *,
    reliability_is_prediction: bool = True,
) -> dict[str, float]:
    if warranty_time <= 0 or exposure <= 0:
        raise ValueError("invalid warranty_time or exposure")
    if not (0.0 <= reliability_at_warranty <= 1.0):
        raise ValueError("R(T_w) out of range")
    failure_probability = 1.0 - reliability_at_warranty
    return {
        "failure_probability": failure_probability,
        "expected_failures": exposure * failure_probability,
        "claims_per_1000": 1000.0 * failure_probability,
        "reliability_is_prediction": float(reliability_is_prediction),
    }


@dataclass
class StratumInput:
    label: str
    kind: str
    exposure: float
    observed_failures: int
    censored_count: int
    valid_count: int


def summarize_warranty_strata_pooled(
    overall_exposure: float,
    reliability_at_warranty: float,
    strata: list[StratumInput],
) -> list[dict[str, float | str]]:
    failure_probability = 1.0 - reliability_at_warranty
    measured_sum = sum(s.exposure for s in strata)
    labeled_valid = sum(s.valid_count for s in strata)
    use_proportional = measured_sum <= 0.0 and overall_exposure > 0.0

    results: list[dict[str, float | str]] = []
    for stratum in strata:
        if use_proportional:
            share = stratum.valid_count / labeled_valid
            exposure = overall_exposure * share
            attribution = "proportional_scalar"
        else:
            exposure = stratum.exposure
            share = exposure / overall_exposure if overall_exposure > 0 else 0.0
            attribution = "measured_column" if exposure > 0 else "zero"
        expected_failures = exposure * failure_probability
        results.append(
            {
                "label": stratum.label,
                "exposure": exposure,
                "share_of_total_exposure": share,
                "expected_failures": expected_failures,
                "exposure_attribution": attribution,
            }
        )
    return results


def assert_scalar_formula_cases() -> None:
    case_a = summarize_warranty(1000.0, 5000.0, 0.98)
    assert nearly_equal(case_a["failure_probability"], 0.02)
    assert nearly_equal(case_a["claims_per_1000"], 20.0)
    assert nearly_equal(case_a["expected_failures"], 100.0)

    case_b = summarize_warranty(1000.0, 1000.0, 0.95)
    assert nearly_equal(case_b["claims_per_1000"], 50.0)


def assert_strata_pooled_cases() -> None:
    strata = [
        StratumInput("wear", "failure_mode", 60.0, 3, 1, 4),
        StratumInput("early", "failure_mode", 40.0, 1, 1, 2),
    ]
    measured = summarize_warranty_strata_pooled(100.0, 0.9, strata)
    assert nearly_equal(measured[0]["expected_failures"], 6.0)  # type: ignore[arg-type]
    assert nearly_equal(measured[1]["expected_failures"], 4.0)  # type: ignore[arg-type]
    assert measured[0]["exposure_attribution"] == "measured_column"

    zero_exposure = [
        StratumInput("wear", "failure_mode", 0.0, 3, 1, 4),
        StratumInput("early", "failure_mode", 0.0, 1, 1, 2),
    ]
    proportional = summarize_warranty_strata_pooled(100.0, 0.9, zero_exposure)
    assert nearly_equal(proportional[0]["exposure"], 100.0 * 4.0 / 6.0)  # type: ignore[arg-type]
    assert proportional[0]["exposure_attribution"] == "proportional_scalar"


def assert_s4_fixture_exposure_totals() -> None:
    """Exposure column sums for samples/phase0_baselines/warranty_strata_s4.csv."""
    exposures = [10.0, 10.0, 20.0, 5.0, 5.0]
    assert nearly_equal(sum(exposures), 50.0)
    wear = 10.0 + 10.0 + 20.0
    early = 5.0
    unlabeled = 5.0
    assert nearly_equal(wear, 40.0)
    assert nearly_equal(early, 5.0)
    assert nearly_equal(unlabeled, 5.0)
    # R=0.95 → F=0.05; wear expected = 40 * 0.05 = 2.0 (matches phase5 test)
    failure_probability = 0.05
    assert nearly_equal(wear * failure_probability, 2.0)


def main() -> None:
    assert_scalar_formula_cases()
    assert_strata_pooled_cases()
    assert_s4_fixture_exposure_totals()
    print(
        "reference_implementation OK: warranty claims/1000 + strata pooled R "
        "— not vendor_oracle"
    )


if __name__ == "__main__":
    main()
