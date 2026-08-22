#!/usr/bin/env python3
"""Independent reference_implementation for Phase 6 Box-Cox special cases.

Mirrors tests/nonnormal_capability_phase6_test.cpp::box_cox_lambda_special_cases_and_limit_order
and domain/statistics/box_cox.cpp (classic power transform, not geometric-mean scaled).

Evidence type: reference_implementation (NOT vendor_oracle).

Usage:
  python scripts/box_cox_reference.py
"""

from __future__ import annotations

import math


def nearly_equal(a: float, b: float, tol: float = 1e-9) -> bool:
    return math.fabs(a - b) <= tol


def box_cox_apply(value: float, lam: float) -> float:
    if value <= 0.0 or not math.isfinite(value) or not math.isfinite(lam):
        return float("nan")
    if lam == 0.0:
        return math.log(value)
    return (value**lam - 1.0) / lam


def box_cox_transform_limit(limit: float, lam: float) -> float | None:
    transformed = box_cox_apply(limit, lam)
    if not math.isfinite(transformed):
        return None
    return transformed


def box_cox_limits_order_ok(lsl: float, usl: float, lam: float) -> bool:
    if not (lsl < usl):
        return False
    t_lsl = box_cox_transform_limit(lsl, lam)
    t_usl = box_cox_transform_limit(usl, lam)
    if t_lsl is None or t_usl is None:
        return False
    # Strictly increasing on (0, ∞) for all λ (matches box_cox.cpp).
    return t_lsl < t_usl


def assert_lambda_one_identity() -> None:
    values = [1.0, 2.0, 3.0, 4.0, 5.0]
    transformed = [box_cox_apply(v, 1.0) for v in values]
    assert nearly_equal(transformed[0], 0.0)
    assert nearly_equal(transformed[4], 4.0)


def assert_lambda_zero_log() -> None:
    assert nearly_equal(box_cox_apply(1.0, 0.0), 0.0)
    assert nearly_equal(box_cox_apply(2.0, 0.0), math.log(2.0))


def assert_apply_and_limits() -> None:
    assert nearly_equal(box_cox_apply(2.0, 1.0), 1.0)
    assert nearly_equal(box_cox_transform_limit(10.0, 0.0), math.log(10.0))  # type: ignore[arg-type]
    assert box_cox_limits_order_ok(2.0, 8.0, 1.0)
    assert box_cox_limits_order_ok(2.0, 8.0, 0.0)
    assert box_cox_limits_order_ok(2.0, 8.0, -1.0)


def assert_rejects_nonpositive() -> None:
    assert math.isnan(box_cox_apply(-1.0, 1.0))
    assert math.isnan(box_cox_apply(0.0, 1.0))


def main() -> None:
    assert_lambda_one_identity()
    assert_lambda_zero_log()
    assert_apply_and_limits()
    assert_rejects_nonpositive()
    print("reference_implementation OK: Box-Cox λ=0/1 + limit order — not vendor_oracle")


if __name__ == "__main__":
    main()
