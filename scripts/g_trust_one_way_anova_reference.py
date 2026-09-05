#!/usr/bin/env python3
"""G-Trust reference_implementation for one_way_anova.

Primary formula URL (NIST PRC 4.3):
  https://www.itl.nist.gov/div898/handbook/prc/section4/prc43.htm
Accessed: 2026-09-05 UTC+8

Input: converted/one_way_anova_ref_golden_input.csv columns Factor,Response
  (synthetic fixed table 2026-09-05; balanced 3×5).

Outputs Factor/Error/Total SS/DF/MS/F/p matching domain one_way_anova
(Tukey / post-hoc NOT frozen).

Dependencies: Python 3.10+ stdlib + g_trust_ref_math. float64.

Evidence: reference_implementation → golden. NOT vendor_oracle.

Usage:
  python scripts/g_trust_one_way_anova_reference.py
"""

from __future__ import annotations

import csv
import pathlib
import sys
from collections import OrderedDict

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from g_trust_ref_math import f_right_tail, sample_mean  # noqa: E402

INPUT = (
    ROOT
    / "tests"
    / "fixtures"
    / "minitab"
    / "converted"
    / "one_way_anova_ref_golden_input.csv"
)
OUTPUT = (
    ROOT / "tests" / "fixtures" / "minitab" / "expected" / "one_way_anova_ref_golden.tsv"
)


def load_groups(path: pathlib.Path) -> OrderedDict[str, list[float]]:
    groups: OrderedDict[str, list[float]] = OrderedDict()
    with path.open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            groups.setdefault(row["Factor"], []).append(float(row["Response"]))
    if len(groups) < 2:
        raise SystemExit("need >=2 factor levels")
    return groups


def one_way(groups: OrderedDict[str, list[float]]) -> dict[str, float | int]:
    labels = list(groups.keys())
    data = [groups[label] for label in labels]
    counts = [len(g) for g in data]
    means = [sample_mean(g) for g in data]
    total_n = sum(counts)
    grand = sum(c * m for c, m in zip(counts, means)) / total_n
    ss_between = sum(c * (m - grand) ** 2 for c, m in zip(counts, means))
    ss_error = 0.0
    ss_total = 0.0
    for group, mean in zip(data, means):
        for value in group:
            ss_error += (value - mean) ** 2
            ss_total += (value - grand) ** 2
    df_between = len(data) - 1
    df_error = total_n - len(data)
    df_total = total_n - 1
    ms_between = ss_between / df_between
    ms_error = ss_error / df_error
    f_stat = ms_between / ms_error
    p_value = f_right_tail(f_stat, float(df_between), float(df_error))
    return {
        "between_ss": ss_between,
        "error_ss": ss_error,
        "total_ss": ss_total,
        "between_df": df_between,
        "error_df": df_error,
        "total_df": df_total,
        "between_ms": ms_between,
        "error_ms": ms_error,
        "f_statistic": f_stat,
        "p_value": p_value,
    }


def write_tsv(metrics: dict[str, float | int], path: pathlib.Path) -> None:
    lines = [
        "# source: reference_implementation NIST PRC 4.3 "
        "+ scripts/g_trust_one_way_anova_reference.py (Python 3.10+ stdlib float64)",
        "# config: command_id=one_way_anova",
        "# config: factor_col=Factor",
        "# config: response_col=Response",
        "# config: input=converted/one_way_anova_ref_golden_input.csv",
        "# config: synthetic_seed_note=fixed_table_20260905_balanced_3x5",
        "# section: anova",
        "Source\tSS\tDF\tMS\tF\tP",
        (
            f"Factor\t{metrics['between_ss']:.17g}\t{metrics['between_df']}\t"
            f"{metrics['between_ms']:.17g}\t{metrics['f_statistic']:.17g}\t"
            f"{metrics['p_value']:.17g}"
        ),
        (
            f"Error\t{metrics['error_ss']:.17g}\t{metrics['error_df']}\t"
            f"{metrics['error_ms']:.17g}\t*\t*"
        ),
        f"Total\t{metrics['total_ss']:.17g}\t{metrics['total_df']}\t*\t*\t*",
    ]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    groups = load_groups(INPUT)
    metrics = one_way(groups)
    write_tsv(metrics, OUTPUT)
    print(f"Wrote {OUTPUT.relative_to(ROOT)}")
    print(
        f"  F={metrics['f_statistic']:.17g} p={metrics['p_value']:.17g} "
        f"DF={metrics['between_df']}/{metrics['error_df']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
