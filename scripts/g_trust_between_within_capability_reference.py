#!/usr/bin/env python3
"""G-Trust reference_implementation for between_within_capability.

Primary formula URL (NIST PMC 1.6 + between/within decomposition matching domain):
  https://www.itl.nist.gov/div898/handbook/pmc/section1/pmc16.htm
Accessed: 2026-09-05 UTC+8

Input: converted/cap_between_within.csv (copied from tools/learning_data/csv/;
  NOT Minitab official library). Columns: 子组, 厚度_um.

Specs pinned from learning overlay between_within_capability.json (not silent):
  LSL=95, USL=105, Target=100.

Domain ProcessCapability::calculate_between_within:
  σ_within = estimate_within_subgroup_sigma (R̄/d2 for n<=8)
  σ_xbar = MR̄(subgroup means)/d2(2)
  σ_B^2 = max(0, σ_xbar^2 − σ_within^2/n)
  σ_BW = sqrt(σ_B^2 + σ_within^2)
  Indices use within_sigma := σ_BW and overall := sample s.

Dependencies: Python 3.10+ stdlib + g_trust_ref_math. float64.

Evidence: reference_implementation → golden. NOT vendor_oracle.

Usage:
  python scripts/g_trust_between_within_capability_reference.py
"""

from __future__ import annotations

import csv
import math
import pathlib
import sys
from collections import OrderedDict

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from g_trust_ref_math import (  # noqa: E402
    D2_TABLE,
    estimate_within_subgroup_sigma,
    sample_mean,
    sample_stdev,
)

INPUT = ROOT / "tests" / "fixtures" / "minitab" / "converted" / "cap_between_within.csv"
OUTPUT = (
    ROOT
    / "tests"
    / "fixtures"
    / "minitab"
    / "expected"
    / "between_within_capability_ref_golden.tsv"
)

# Pinned from tools/learning_data/tutorial_overlays/between_within_capability.json
LSL = 95.0
USL = 105.0
TARGET = 100.0
SUBGROUP_COL = "子组"
VALUE_COL = "厚度_um"


def load_subgroups(path: pathlib.Path) -> tuple[list[float], list[list[float]]]:
    groups: OrderedDict[str, list[float]] = OrderedDict()
    with path.open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None:
            raise SystemExit(f"empty header in {path}")
        if SUBGROUP_COL not in reader.fieldnames or VALUE_COL not in reader.fieldnames:
            raise SystemExit(f"need {SUBGROUP_COL}/{VALUE_COL} in {path}")
        for row in reader:
            label = row[SUBGROUP_COL]
            value = float(row[VALUE_COL])
            groups.setdefault(label, []).append(value)
    subgroups = list(groups.values())
    if len(subgroups) < 2:
        raise SystemExit("need >=2 subgroups")
    n = len(subgroups[0])
    if any(len(g) != n for g in subgroups):
        raise SystemExit("unequal subgroup sizes")
    observations = [v for g in subgroups for v in g]
    return observations, subgroups


def compute(observations: list[float], subgroups: list[list[float]]) -> dict[str, float | str]:
    within_sigma, within_method = estimate_within_subgroup_sigma(subgroups)
    means = [sample_mean(g) for g in subgroups]
    mr_bar = sum(abs(means[i] - means[i - 1]) for i in range(1, len(means))) / (
        len(means) - 1
    )
    d2_pair = D2_TABLE[2]
    sigma_xbar = mr_bar / d2_pair
    n = len(subgroups[0])
    raw_between_var = sigma_xbar * sigma_xbar - (within_sigma * within_sigma) / n
    between_var = 0.0 if raw_between_var < 0.0 else raw_between_var
    between_sigma = math.sqrt(between_var)
    bw_sigma = math.sqrt(between_var + within_sigma * within_sigma)
    mean = sample_mean(observations)
    overall = sample_stdev(observations)
    # Indices: domain passes between_within_sigma as "within" argument.
    cp = (USL - LSL) / (6.0 * bw_sigma)
    cpk = min((mean - LSL) / (3.0 * bw_sigma), (USL - mean) / (3.0 * bw_sigma))
    pp = (USL - LSL) / (6.0 * overall)
    ppk = min((mean - LSL) / (3.0 * overall), (USL - mean) / (3.0 * overall))
    return {
        "sigma_within": within_sigma,
        "sigma_between": between_sigma,
        "sigma_bw": bw_sigma,
        "mean": mean,
        "overall_sigma": overall,
        "Cp": cp,
        "Cpk": cpk,
        "Pp": pp,
        "Ppk": ppk,
        "within_sigma_method": within_method,
        "between_sigma_method": "MR̄(子组均值) / d2(2)",
        "between_within_sigma_method": "sqrt(σ²_B + σ²_within)",
        "subgroup_size": float(n),
    }


def write_tsv(metrics: dict[str, float | str], path: pathlib.Path) -> None:
    lines = [
        "# source: reference_implementation NIST PMC 1.6 + domain between/within "
        "+ scripts/g_trust_between_within_capability_reference.py "
        "(Python 3.10+ stdlib float64)",
        "# config: command_id=between_within_capability",
        f"# config: subgroup_col={SUBGROUP_COL}",
        f"# config: value_col={VALUE_COL}",
        f"# config: lsl={LSL}",
        f"# config: usl={USL}",
        f"# config: target={TARGET}",
        f"# config: within_sigma_method={metrics['within_sigma_method']}",
        f"# config: between_sigma_method={metrics['between_sigma_method']}",
        f"# config: between_within_sigma_method={metrics['between_within_sigma_method']}",
        "# config: specs_source=learning_overlay between_within_capability.json",
        "# config: input=converted/cap_between_within.csv",
        "# section: sigma",
        "Key\tValue",
        f"sigma_within\t{metrics['sigma_within']:.17g}",
        f"sigma_between\t{metrics['sigma_between']:.17g}",
        f"sigma_bw\t{metrics['sigma_bw']:.17g}",
        "# section: indices",
        "Key\tValue",
        f"mean\t{metrics['mean']:.17g}",
        f"Cp\t{metrics['Cp']:.17g}",
        f"Cpk\t{metrics['Cpk']:.17g}",
        f"Pp\t{metrics['Pp']:.17g}",
        f"Ppk\t{metrics['Ppk']:.17g}",
    ]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    observations, subgroups = load_subgroups(INPUT)
    metrics = compute(observations, subgroups)
    write_tsv(metrics, OUTPUT)
    print(f"Wrote {OUTPUT.relative_to(ROOT)}")
    for key in (
        "sigma_within",
        "sigma_between",
        "sigma_bw",
        "Cp",
        "Cpk",
        "Pp",
        "Ppk",
    ):
        print(f"  {key}={metrics[key]:.17g}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
