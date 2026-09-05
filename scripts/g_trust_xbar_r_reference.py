#!/usr/bin/env python3
"""G-Trust reference_implementation for Xbar-R (command_id=xbar_r).

Primary formula URL (NIST PMC 3.2.1):
  https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc321.htm
Accessed: 2026-09-05 UTC+8

Input note:
  Wave plan §5.2 preferred CrankshaftMovement.csv by Date, but that file has
  mixed subgroup sizes (5 and 10). Domain xbar_range_dual requires equal n.
  Therefore this script uses the plan alternate: CamshaftLength.csv,
  value_col=Machine 1, subgroup_col=Subgroup ID (20×n=5).

Method (match ControlCharts::xbar_range_dual balanced design):
  Xbar_i = mean(subgroup); R_i = max - min
  Xbar_bar = mean(Xbar_i); R_bar = mean(R_i)
  d2,d3 from ASTM/AIAG table (same as SpcConstants)
  A2 = 3 / (d2 * sqrt(n))
  D3 = max(0, 1 - 3*d3/d2); D4 = 1 + 3*d3/d2
  Xbar: CL=Xbar_bar, UCL/LCL = Xbar_bar ± A2*R_bar
  R: CL=R_bar, UCL=D4*R_bar, LCL=D3*R_bar

Dependencies: Python 3.10+ stdlib only (csv, math, collections, pathlib). float64.
Encoding: utf-8-sig (fixtures may have BOM).

Evidence: reference_implementation → golden. NOT vendor_oracle.

Usage:
  python scripts/g_trust_xbar_r_reference.py
"""

from __future__ import annotations

import csv
import math
import pathlib
from collections import OrderedDict

ROOT = pathlib.Path(__file__).resolve().parents[1]
INPUT = ROOT / "tests" / "fixtures" / "minitab" / "converted" / "CamshaftLength.csv"
OUTPUT = ROOT / "tests" / "fixtures" / "minitab" / "expected" / "xbar_r_ref_golden.tsv"

D2_TABLE = {
    2: 1.128,
    3: 1.693,
    4: 2.059,
    5: 2.326,
    6: 2.534,
    7: 2.704,
    8: 2.847,
    9: 2.970,
    10: 3.078,
}
D3_TABLE = {
    2: 0.853,
    3: 0.888,
    4: 0.880,
    5: 0.864,
    6: 0.848,
    7: 0.833,
    8: 0.820,
    9: 0.808,
    10: 0.797,
}

VALUE_COL = "Machine 1"
SUBGROUP_COL = "Subgroup ID"


def load_subgroups(path: pathlib.Path) -> list[list[float]]:
    groups: OrderedDict[str, list[float]] = OrderedDict()
    with path.open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None:
            raise SystemExit(f"empty header in {path}")
        if VALUE_COL not in reader.fieldnames or SUBGROUP_COL not in reader.fieldnames:
            raise SystemExit(f"need columns {VALUE_COL!r} and {SUBGROUP_COL!r}")
        for row in reader:
            key = row[SUBGROUP_COL]
            groups.setdefault(key, []).append(float(row[VALUE_COL]))
    subgroups = list(groups.values())
    if not subgroups:
        raise SystemExit("no subgroups")
    n0 = len(subgroups[0])
    if any(len(g) != n0 for g in subgroups):
        raise SystemExit("unbalanced subgroups (domain requires equal n)")
    if n0 < 2:
        raise SystemExit("subgroup size < 2")
    return subgroups


def constants(n: int) -> tuple[float, float, float, float]:
    d2 = D2_TABLE[n]
    d3 = D3_TABLE[n]
    a2 = 3.0 / (d2 * math.sqrt(float(n)))
    d3_lim = max(0.0, 1.0 - 3.0 * d3 / d2)
    d4 = 1.0 + 3.0 * d3 / d2
    return d2, a2, d3_lim, d4


def compute(subgroups: list[list[float]]) -> dict[str, float]:
    n = len(subgroups[0])
    d2, a2, d3_lim, d4 = constants(n)
    xbars = [sum(g) / len(g) for g in subgroups]
    ranges = [max(g) - min(g) for g in subgroups]
    xbar_bar = sum(xbars) / len(xbars)
    r_bar = sum(ranges) / len(ranges)
    return {
        "subgroup_size": float(n),
        "subgroup_count": float(len(subgroups)),
        "d2": d2,
        "a2": a2,
        "d3": d3_lim,
        "d4": d4,
        "r_bar": r_bar,
        "xbar_cl": xbar_bar,
        "xbar_ucl": xbar_bar + a2 * r_bar,
        "xbar_lcl": xbar_bar - a2 * r_bar,
        "r_cl": r_bar,
        "r_ucl": d4 * r_bar,
        "r_lcl": d3_lim * r_bar,
        "sigma": r_bar / d2,
    }


def write_tsv(metrics: dict[str, float], path: pathlib.Path) -> None:
    lines = [
        "# source: reference_implementation NIST PMC 3.2.1 "
        "+ scripts/g_trust_xbar_r_reference.py (Python 3.10+ stdlib float64)",
        "# config: command_id=xbar_r",
        f"# config: value_col={VALUE_COL}",
        f"# config: subgroup_col={SUBGROUP_COL}",
        "# config: special_causes=default",
        "# config: input=converted/CamshaftLength.csv",
        "# config: note=CrankshaftMovement Date groups unbalanced; used plan alternate",
        "# section: summary",
        "Key\tValue",
        f"subgroup_size\t{int(metrics['subgroup_size'])}",
        f"subgroup_count\t{int(metrics['subgroup_count'])}",
        f"r_bar\t{metrics['r_bar']:.17g}",
        f"a2\t{metrics['a2']:.17g}",
        f"d2\t{metrics['d2']:.17g}",
        f"d3\t{metrics['d3']:.17g}",
        f"d4\t{metrics['d4']:.17g}",
        f"sigma\t{metrics['sigma']:.17g}",
        "# section: xbar_limits",
        "Key\tValue",
        f"cl\t{metrics['xbar_cl']:.17g}",
        f"ucl\t{metrics['xbar_ucl']:.17g}",
        f"lcl\t{metrics['xbar_lcl']:.17g}",
        "# section: r_limits",
        "Key\tValue",
        f"cl\t{metrics['r_cl']:.17g}",
        f"ucl\t{metrics['r_ucl']:.17g}",
        f"lcl\t{metrics['r_lcl']:.17g}",
    ]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    subgroups = load_subgroups(INPUT)
    metrics = compute(subgroups)
    write_tsv(metrics, OUTPUT)
    print(f"Wrote {OUTPUT.relative_to(ROOT)}")
    print(
        f"  n={int(metrics['subgroup_size'])} groups={int(metrics['subgroup_count'])} "
        f"xbar_cl={metrics['xbar_cl']:.6g} r_bar={metrics['r_bar']:.6g}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
