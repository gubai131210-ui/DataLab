#!/usr/bin/env python3
"""G-Trust reference_implementation for normality_test.

Primary formula URL (NIST EDA 3.5.e Anderson-Darling):
  https://www.itl.nist.gov/div898/handbook/eda/section3/eda35e.htm
Accessed: 2026-09-05 UTC+8

Method pinned: anderson_darling (Stephens A²* adjustment + p-value formula
matching domain anderson_darling.* / normality_test).

Input: converted/PistonRingDiameter.csv column Diameter.

Evidence: reference_implementation → golden. NOT vendor_oracle.
Does NOT freeze Ryan-Joiner.

Dependencies: Python 3.10+ stdlib + g_trust_ref_math. float64.

Usage:
  python scripts/g_trust_normality_test_reference.py
"""

from __future__ import annotations

import csv
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from g_trust_ref_math import anderson_darling_normal  # noqa: E402

INPUT = ROOT / "tests" / "fixtures" / "minitab" / "converted" / "PistonRingDiameter.csv"
OUTPUT = (
    ROOT / "tests" / "fixtures" / "minitab" / "expected" / "normality_test_ref_golden.tsv"
)

ALPHA = 0.05


def load_diameter(path: pathlib.Path) -> list[float]:
    with path.open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        return [float(row["Diameter"]) for row in reader]


def write_tsv(a2: float, a2_star: float, p_value: float, path: pathlib.Path) -> None:
    lines = [
        "# source: reference_implementation NIST EDA 3.5.e Anderson-Darling "
        "+ Stephens adjustment + scripts/g_trust_normality_test_reference.py "
        "(Python 3.10+ stdlib float64)",
        "# config: command_id=normality_test",
        "# config: method=anderson_darling",
        f"# config: alpha={ALPHA}",
        "# config: input=converted/PistonRingDiameter.csv",
        "# section: summary",
        "Key\tValue",
        "method\tanderson_darling",
        f"A2\t{a2:.17g}",
        f"A2_star\t{a2_star:.17g}",
        f"p_value\t{p_value:.17g}",
    ]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    values = load_diameter(INPUT)
    a2, a2_star, p_value = anderson_darling_normal(values)
    write_tsv(a2, a2_star, p_value, OUTPUT)
    print(f"Wrote {OUTPUT.relative_to(ROOT)} n={len(values)}")
    print(f"  A2={a2:.17g} A2_star={a2_star:.17g} p={p_value:.17g}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
