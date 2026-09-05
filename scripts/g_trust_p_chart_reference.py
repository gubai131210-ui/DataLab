#!/usr/bin/env python3
"""G-Trust reference_implementation for P chart (command_id=p_chart).

Primary formula URL (NIST PMC 3.3.2):
  https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc332.htm
Accessed: 2026-09-05 UTC+8

Method (match ControlCharts::p_chart):
  p_bar = sum(D) / sum(n)
  plotted p_i = D_i / n_i
  UCL_i = min(1, p_bar + 3*sqrt(p_bar*(1-p_bar)/n_i))
  LCL_i = max(0, p_bar - 3*sqrt(p_bar*(1-p_bar)/n_i))

Dependencies: Python 3.10+ stdlib only (csv, math, pathlib). float64.

Evidence: reference_implementation → golden. NOT vendor_oracle.

Usage:
  python scripts/g_trust_p_chart_reference.py
"""

from __future__ import annotations

import csv
import math
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
INPUT = ROOT / "tests" / "fixtures" / "minitab" / "converted" / "UnansweredCalls.csv"
OUTPUT = ROOT / "tests" / "fixtures" / "minitab" / "expected" / "p_chart_ref_golden.tsv"

DEFECTIVES_COL = "Unanswered Calls"
INSPECTED_COL = "Total Calls"


def load_counts(path: pathlib.Path) -> tuple[list[int], list[int]]:
    defectives: list[int] = []
    inspected: list[int] = []
    with path.open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None:
            raise SystemExit(f"empty header in {path}")
        for col in (DEFECTIVES_COL, INSPECTED_COL):
            if col not in reader.fieldnames:
                raise SystemExit(f"missing column {col!r}")
        for row in reader:
            d = int(float(row[DEFECTIVES_COL]))
            n = int(float(row[INSPECTED_COL]))
            if n <= 0 or d > n:
                raise SystemExit(f"invalid counts d={d} n={n}")
            defectives.append(d)
            inspected.append(n)
    if not defectives:
        raise SystemExit("empty input")
    return defectives, inspected


def compute(
    defectives: list[int], inspected: list[int]
) -> tuple[dict[str, float], list[tuple[int, float, float, float]]]:
    total_d = sum(defectives)
    total_n = sum(inspected)
    p_bar = total_d / total_n
    limits: list[tuple[int, float, float, float]] = []
    for i, n in enumerate(inspected):
        sigma = math.sqrt(p_bar * (1.0 - p_bar) / float(n))
        ucl = min(1.0, p_bar + 3.0 * sigma)
        lcl = max(0.0, p_bar - 3.0 * sigma)
        p_i = defectives[i] / float(n)
        limits.append((i, p_i, ucl, lcl))
    summary = {
        "p_bar": p_bar,
        "total_defectives": float(total_d),
        "total_inspected": float(total_n),
        "point_count": float(len(inspected)),
    }
    return summary, limits


def write_tsv(
    summary: dict[str, float],
    limits: list[tuple[int, float, float, float]],
    path: pathlib.Path,
) -> None:
    lines = [
        "# source: reference_implementation NIST PMC 3.3.2 "
        "+ scripts/g_trust_p_chart_reference.py (Python 3.10+ stdlib float64)",
        "# config: command_id=p_chart",
        f"# config: defectives_col={DEFECTIVES_COL}",
        f"# config: inspected_col={INSPECTED_COL}",
        "# config: input=converted/UnansweredCalls.csv",
        "# section: summary",
        "Key\tValue",
        f"p_bar\t{summary['p_bar']:.17g}",
        f"total_defectives\t{int(summary['total_defectives'])}",
        f"total_inspected\t{int(summary['total_inspected'])}",
        f"point_count\t{int(summary['point_count'])}",
        "# section: limits",
        "Index\tPlotted\tUCL\tLCL",
    ]
    for index, plotted, ucl, lcl in limits:
        lines.append(f"{index}\t{plotted:.17g}\t{ucl:.17g}\t{lcl:.17g}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    defectives, inspected = load_counts(INPUT)
    summary, limits = compute(defectives, inspected)
    write_tsv(summary, limits, OUTPUT)
    print(f"Wrote {OUTPUT.relative_to(ROOT)}")
    print(f"  p_bar={summary['p_bar']:.17g} points={len(limits)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
