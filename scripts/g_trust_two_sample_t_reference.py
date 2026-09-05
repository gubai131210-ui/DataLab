#!/usr/bin/env python3
"""G-Trust reference_implementation for two_sample_t (command_id=two_sample_t).

Primary formula URL (NIST EDA 3.5.3):
  https://www.itl.nist.gov/div898/handbook/eda/section3/eda353.htm
Accessed: 2026-09-05 UTC+8

Method pinned: Welch (VarianceMethod::welch), two-sided, confidence=0.95.
Input: converted/two_sample_t_ref_golden_input.csv wide columns Sample1,Sample2
  (synthetic seed note: fixed table authored 2026-09-05; not random at runtime).

Dependencies: Python 3.10+ stdlib + g_trust_ref_math. float64.

Evidence: reference_implementation → golden. NOT vendor_oracle.

Usage:
  python scripts/g_trust_two_sample_t_reference.py
"""

from __future__ import annotations

import csv
import math
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from g_trust_ref_math import (  # noqa: E402
    sample_mean,
    sample_stdev,
    student_t_quantile,
    two_sided_t_p_value,
)

INPUT = (
    ROOT
    / "tests"
    / "fixtures"
    / "minitab"
    / "converted"
    / "two_sample_t_ref_golden_input.csv"
)
OUTPUT = (
    ROOT / "tests" / "fixtures" / "minitab" / "expected" / "two_sample_t_ref_golden.tsv"
)

CONFIDENCE = 0.95


def load_samples(path: pathlib.Path) -> tuple[list[float], list[float]]:
    first: list[float] = []
    second: list[float] = []
    with path.open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            first.append(float(row["Sample1"]))
            second.append(float(row["Sample2"]))
    if len(first) < 2 or len(second) < 2:
        raise SystemExit("need >=2 observations per sample")
    return first, second


def welch_two_sample(first: list[float], second: list[float]) -> dict[str, float]:
    mean1 = sample_mean(first)
    mean2 = sample_mean(second)
    s1 = sample_stdev(first)
    s2 = sample_stdev(second)
    n1 = float(len(first))
    n2 = float(len(second))
    mean_diff = mean1 - mean2
    v1 = s1 * s1
    v2 = s2 * s2
    t1 = v1 / n1
    t2 = v2 / n2
    se = math.sqrt(t1 + t2)
    denom = (t1 * t1) / (n1 - 1.0) + (t2 * t2) / (n2 - 1.0)
    df = (t1 + t2) * (t1 + t2) / denom if denom > 0.0 else 0.0
    t_stat = mean_diff / se if se != 0.0 else 0.0
    p_value = two_sided_t_p_value(t_stat, df)
    alpha = 1.0 - CONFIDENCE
    critical = student_t_quantile(1.0 - alpha / 2.0, df)
    return {
        "mean_difference": mean_diff,
        "t_statistic": t_stat,
        "degrees_of_freedom": df,
        "p_value": p_value,
        "confidence_lower": mean_diff - critical * se,
        "confidence_upper": mean_diff + critical * se,
        "standard_error_difference": se,
    }


def write_tsv(metrics: dict[str, float], path: pathlib.Path) -> None:
    lines = [
        "# source: reference_implementation NIST EDA 3.5.3 Welch "
        "+ scripts/g_trust_two_sample_t_reference.py (Python 3.10+ stdlib float64)",
        "# config: command_id=two_sample_t",
        "# config: variance_method=welch",
        "# config: alternative=two_sided",
        f"# config: confidence={CONFIDENCE}",
        "# config: input=converted/two_sample_t_ref_golden_input.csv",
        "# config: synthetic_seed_note=fixed_table_20260905",
        "# section: summary",
        "Key\tValue",
    ]
    for key in (
        "mean_difference",
        "t_statistic",
        "degrees_of_freedom",
        "p_value",
        "confidence_lower",
        "confidence_upper",
    ):
        lines.append(f"{key}\t{metrics[key]:.17g}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    first, second = load_samples(INPUT)
    metrics = welch_two_sample(first, second)
    write_tsv(metrics, OUTPUT)
    print(f"Wrote {OUTPUT.relative_to(ROOT)}")
    for key, value in metrics.items():
        if key == "standard_error_difference":
            continue
        print(f"  {key}={value:.17g}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
