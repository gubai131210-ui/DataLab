#!/usr/bin/env python3
"""Validate doe_ccd_k4_factors.json matches pdf_doe_ccd_k4_long test contract."""

from __future__ import annotations

import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "samples/phase0_baselines/doe_ccd_k4_factors.json"


def main() -> int:
    data = json.loads(FIXTURE.read_text(encoding="utf-8"))
    assert data["design_type"] == "ccd"
    assert data["ccd_variant"] == "ccf"
    assert len(data["factors"]) == 4
    assert data["center_point_count"] == 30
    assert data["randomization_seed"] == 11
    assert data["randomize"] is True
    assert data["expected_minimums"]["run_count"] >= 50
    print(f"OK: {FIXTURE.name} matches S2 path B / pdf_doe_ccd_k4_long contract")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
