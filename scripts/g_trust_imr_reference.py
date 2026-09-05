#!/usr/bin/env python3
"""G-Trust reference_implementation for I-MR (command_id=imr).

Primary formula URL (NIST PMC 3.2.2):
  https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc322.htm
Accessed: 2026-09-05 UTC+8

Method (match domain IndividualsMovingRangeOptions defaults):
  MR_i = max(window) - min(window), length=2
  MR_bar = mean(MR)
  d2(2) = 1.128 (ASTM/AIAG table as in SpcConstants)
  sigma = MR_bar / d2
  I: CL = mean(x), UCL/LCL = CL ± 3*sigma
  MR: CL = MR_bar, UCL = D4*MR_bar, LCL = D3*MR_bar
  D4 = 1 + 3*d3/d2, D3 = max(0, 1 - 3*d3/d2), d3(2)=0.853

Dependencies: Python 3.10+ stdlib only (csv, math, pathlib). float64.

Evidence: reference_implementation → golden. NOT vendor_oracle.

Usage:
  python scripts/g_trust_imr_reference.py
"""

from __future__ import annotations

import csv
import math
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
INPUT = ROOT / "tests" / "fixtures" / "minitab" / "converted" / "imr_ref_golden_input.csv"
OUTPUT = ROOT / "tests" / "fixtures" / "minitab" / "expected" / "imr_ref_golden.tsv"

D2 = 1.128
D3 = 0.853


def d4(d2: float, d3: float) -> float:
    return 1.0 + 3.0 * d3 / d2


def d3_limit(d2: float, d3: float) -> float:
    return max(0.0, 1.0 - 3.0 * d3 / d2)


def load_values(path: pathlib.Path) -> list[float]:
    with path.open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None or "Value" not in reader.fieldnames:
            raise SystemExit(f"expected column Value in {path}")
        values = [float(row["Value"]) for row in reader]
    if len(values) < 2:
        raise SystemExit("need at least 2 observations")
    return values


def compute(values: list[float]) -> dict[str, float]:
    mrs: list[float] = []
    for i in range(1, len(values)):
        mrs.append(abs(values[i] - values[i - 1]))
    # domain uses max-min of window length 2 == abs diff
    mr_bar = sum(mrs) / len(mrs)
    center = sum(values) / len(values)
    sigma = mr_bar / D2
    i_ucl = center + 3.0 * sigma
    i_lcl = center - 3.0 * sigma
    mr_ucl = d4(D2, D3) * mr_bar
    mr_lcl = d3_limit(D2, D3) * mr_bar
    return {
        "i_cl": center,
        "i_ucl": i_ucl,
        "i_lcl": i_lcl,
        "sigma": sigma,
        "mr_bar": mr_bar,
        "mr_ucl": mr_ucl,
        "mr_lcl": mr_lcl,
    }


def write_tsv(metrics: dict[str, float], path: pathlib.Path) -> None:
    lines = [
        "# source: reference_implementation NIST PMC 3.2.2 "
        "+ scripts/g_trust_imr_reference.py (Python 3.10+ stdlib float64)",
        "# config: command_id=imr",
        "# config: method=average_moving_range",
        "# config: mr_length=2",
        "# config: nelson=0",
        "# config: input=converted/imr_ref_golden_input.csv",
        "# section: metrics",
        "Key\tValue",
    ]
    for key in ("i_cl", "i_ucl", "i_lcl", "sigma", "mr_bar", "mr_ucl", "mr_lcl"):
        lines.append(f"{key}\t{metrics[key]:.17g}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    values = load_values(INPUT)
    metrics = compute(values)
    write_tsv(metrics, OUTPUT)
    print(f"Wrote {OUTPUT.relative_to(ROOT)}")
    for key, value in metrics.items():
        print(f"  {key}={value:.17g}")
    # sanity vs known small-N
    assert math.isclose(metrics["mr_bar"], 1.0, rel_tol=0, abs_tol=1e-15)
    assert math.isclose(metrics["i_cl"], 2.0, rel_tol=0, abs_tol=1e-15)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
