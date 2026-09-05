#!/usr/bin/env python3
"""G-Trust reference_implementation for gage_rr (command_id=gage_rr).

Formula: crossed ANOVA Gage R&R (AIAG-shaped; ASQ terminology). Matches
domain crossed_gage_rr: retain Part×Operator interaction; truncate negative
variance components; study_var_multiplier=6; percent_study_variation in 0–100
percentage points; ndc = max(1, floor(1.41*sqrt(σ²_part/σ²_gage))).

Primary references (public): AIAG MSA concepts / ASQ Gage R&R overview.
Accessed: 2026-09-05 UTC+8

Input: converted/gage_rr_crossed.csv (from samples/measurement_system/;
  NOT Minitab official library). tolerance=0 → freeze %Study Var + ndc.

Evidence: reference_implementation → golden. NOT vendor_oracle / AIAG export.

Dependencies: Python 3.10+ stdlib only. float64.

Usage:
  python scripts/g_trust_gage_rr_reference.py
"""

from __future__ import annotations

import csv
import math
import pathlib
from collections import OrderedDict

ROOT = pathlib.Path(__file__).resolve().parents[1]
INPUT = ROOT / "tests" / "fixtures" / "minitab" / "converted" / "gage_rr_crossed.csv"
OUTPUT = ROOT / "tests" / "fixtures" / "minitab" / "expected" / "gage_rr_ref_golden.tsv"

STUDY_VAR_MULTIPLIER = 6.0
TOLERANCE = 0.0


def load_rows(path: pathlib.Path) -> tuple[list[float], list[str], list[str]]:
    measurements: list[float] = []
    parts: list[str] = []
    operators: list[str] = []
    with path.open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            measurements.append(float(row["Measurement"]))
            parts.append(row["Part"])
            operators.append(row["Operator"])
    return measurements, parts, operators


def crossed_gage_rr(
    measurements: list[float], parts: list[str], operators: list[str]
) -> dict[str, float]:
    part_index: OrderedDict[str, int] = OrderedDict()
    operator_index: OrderedDict[str, int] = OrderedDict()
    for part, oper in zip(parts, operators):
        if part not in part_index:
            part_index[part] = len(part_index)
        if oper not in operator_index:
            operator_index[oper] = len(operator_index)
    p = len(part_index)
    o = len(operator_index)
    cells: list[list[list[float]]] = [[[] for _ in range(o)] for _ in range(p)]
    for meas, part, oper in zip(measurements, parts, operators):
        cells[part_index[part]][operator_index[oper]].append(meas)
    r = len(cells[0][0])
    for part_cells in cells:
        for cell in part_cells:
            if len(cell) != r:
                raise SystemExit("unbalanced gage design")
    n = len(measurements)
    grand = sum(measurements) / n
    cell_means = [[sum(c) / r for c in row] for row in cells]
    part_means = [sum(row) / o for row in cell_means]
    operator_means = [
        sum(cell_means[i][j] for i in range(p)) / p for j in range(o)
    ]
    ss_part = sum(o * r * (m - grand) ** 2 for m in part_means)
    ss_operator = sum(p * r * (m - grand) ** 2 for m in operator_means)
    ss_interaction = 0.0
    ss_repeat = 0.0
    for i in range(p):
        for j in range(o):
            interaction = (
                cell_means[i][j] - part_means[i] - operator_means[j] + grand
            )
            ss_interaction += r * interaction * interaction
            for value in cells[i][j]:
                ss_repeat += (value - cell_means[i][j]) ** 2
    df_part = p - 1
    df_operator = o - 1
    df_interaction = df_part * df_operator
    df_repeat = p * o * (r - 1)
    ms_part = ss_part / df_part
    ms_operator = ss_operator / df_operator
    ms_interaction = ss_interaction / df_interaction
    ms_repeat = ss_repeat / df_repeat
    raw_repeat = ms_repeat
    raw_interaction = (ms_interaction - ms_repeat) / r
    raw_operator = (ms_operator - ms_interaction) / (p * r)
    raw_part = (ms_part - ms_interaction) / (o * r)
    repeatability = max(0.0, raw_repeat)
    interaction = max(0.0, raw_interaction)
    operator_var = max(0.0, raw_operator)
    part_var = max(0.0, raw_part)
    gage_rr = repeatability + interaction + operator_var
    total = gage_rr + part_var
    percent_study = (
        math.sqrt(gage_rr) / math.sqrt(total) * 100.0 if total > 0.0 else 0.0
    )
    if gage_rr > 0.0:
        ndc = math.floor(1.41 * math.sqrt(part_var / gage_rr))
        if ndc < 1.0:
            ndc = 1.0
    else:
        ndc = float("nan")
    return {
        "percent_study_variation_total_gage_rr": percent_study,
        "ndc": ndc,
        "part_count": float(p),
        "operator_count": float(o),
        "replicate_count": float(r),
        "study_var_multiplier": STUDY_VAR_MULTIPLIER,
    }


def write_tsv(metrics: dict[str, float], path: pathlib.Path) -> None:
    lines = [
        "# source: reference_implementation crossed ANOVA Gage R&R "
        "+ scripts/g_trust_gage_rr_reference.py (Python 3.10+ stdlib float64)",
        "# config: command_id=gage_rr",
        "# config: part_col=Part",
        "# config: operator_col=Operator",
        "# config: meas_col=Measurement",
        f"# config: tolerance={TOLERANCE}",
        f"# config: study_var_multiplier={STUDY_VAR_MULTIPLIER}",
        "# config: interaction_retained=1",
        "# config: input=converted/gage_rr_crossed.csv",
        "# section: summary",
        "Key\tValue",
        f"percent_study_variation_total_gage_rr\t"
        f"{metrics['percent_study_variation_total_gage_rr']:.17g}",
        f"ndc\t{metrics['ndc']:.17g}",
        f"part_count\t{metrics['part_count']:.17g}",
        f"operator_count\t{metrics['operator_count']:.17g}",
        f"replicate_count\t{metrics['replicate_count']:.17g}",
    ]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    measurements, parts, operators = load_rows(INPUT)
    metrics = crossed_gage_rr(measurements, parts, operators)
    write_tsv(metrics, OUTPUT)
    print(f"Wrote {OUTPUT.relative_to(ROOT)}")
    print(
        f"  %StudyVar={metrics['percent_study_variation_total_gage_rr']:.17g} "
        f"ndc={metrics['ndc']:.17g}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
