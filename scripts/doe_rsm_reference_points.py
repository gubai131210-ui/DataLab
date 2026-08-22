#!/usr/bin/env python3
"""Independent reference_implementation for Phase 4 CCD/BBD standard-order points.

NIST handbook:
  CCD: https://itl.nist.gov/div898/handbook/pri/section3/pri3361.htm
  BBD: https://itl.nist.gov/div898/handbook/pri/section3/pri3362.htm

This script does NOT claim vendor_oracle alignment. It regenerates the same
standard-order coded matrices frozen in:
  samples/phase0_baselines/doe_ccd_k2_ccf_stdorder_golden.json
  samples/phase0_baselines/doe_bbd_k3_stdorder_golden.json

Usage:
  python scripts/doe_rsm_reference_points.py
"""

from __future__ import annotations

import json
import math
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASELINES = ROOT / "samples" / "phase0_baselines"


def ccd_ccf_k2(n0: int = 1) -> list[dict]:
    """Face-centered CCD, k=2, alpha=1, factorial bit-order matching DataLab."""
    runs: list[dict] = []
    order = 1
    for mask in range(4):
        coded = [1.0 if (mask >> f) & 1 else -1.0 for f in range(2)]
        runs.append({"standard_order": order, "point_type": "cube", "coded": coded})
        order += 1
    alpha = 1.0
    for f in range(2):
        for sign in (-1.0, 1.0):
            coded = [0.0, 0.0]
            coded[f] = sign * alpha
            runs.append({"standard_order": order, "point_type": "star", "coded": coded})
            order += 1
    for _ in range(n0):
        runs.append({"standard_order": order, "point_type": "center", "coded": [0.0, 0.0]})
        order += 1
    return runs


def bbd_k3(n0: int = 1) -> list[dict]:
    """Box–Behnken k=3: pairwise edge midpoints, no full corners."""
    runs: list[dict] = []
    order = 1
    k = 3
    for i in range(k):
        for j in range(i + 1, k):
            for si in (-1.0, 1.0):
                for sj in (-1.0, 1.0):
                    coded = [0.0] * k
                    coded[i] = si
                    coded[j] = sj
                    runs.append(
                        {"standard_order": order, "point_type": "edge", "coded": coded}
                    )
                    order += 1
    for _ in range(n0):
        runs.append(
            {"standard_order": order, "point_type": "center", "coded": [0.0, 0.0, 0.0]}
        )
        order += 1
    return runs


def nearly_equal(a: float, b: float, tol: float = 1e-12) -> bool:
    return math.fabs(a - b) <= tol


def assert_matches_golden(path: Path, generated: list[dict]) -> None:
    golden = json.loads(path.read_text(encoding="utf-8"))
    assert golden["evidence_type"] == "golden"
    assert golden["source_evidence_type"] == "reference_implementation"
    expected = golden["runs_standard_order"]
    assert len(generated) == len(expected), (path.name, len(generated), len(expected))
    for got, exp in zip(generated, expected):
        assert got["standard_order"] == exp["standard_order"]
        assert got["point_type"] == exp["point_type"]
        assert len(got["coded"]) == len(exp["coded"])
        for g, e in zip(got["coded"], exp["coded"]):
            assert nearly_equal(g, e), (path.name, got, exp)


def main() -> None:
    assert_matches_golden(
        BASELINES / "doe_ccd_k2_ccf_stdorder_golden.json", ccd_ccf_k2()
    )
    assert_matches_golden(BASELINES / "doe_bbd_k3_stdorder_golden.json", bbd_k3())
    print("reference_implementation OK: CCD k2 CCF + BBD k3 match frozen goldens")


if __name__ == "__main__":
    main()
