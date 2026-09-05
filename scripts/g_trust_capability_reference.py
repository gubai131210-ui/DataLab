#!/usr/bin/env python3
"""G-Trust reference_implementation for capability (command_id=capability).

Primary formula URL (NIST PMC 1.6):
  https://www.itl.nist.gov/div898/handbook/pmc/section1/pmc16.htm
Accessed: 2026-09-05 UTC+8

Input: converted/PistonRingDiameter.csv column Diameter
Specs (SOURCE.md): LSL=73.95, USL=74.05, Target=74.00; subgroup_size=5

Within σ: estimate_within_subgroup_sigma → R̄ / d2(n) for n=5 (domain match).
Overall σ: sample standard deviation (n−1).
Cp=(USL−LSL)/(6σ_within); Cpk=min((USL−μ)/(3σ),(μ−LSL)/(3σ))
Pp/Ppk use overall σ similarly.

Dependencies: Python 3.10+ stdlib + scripts/g_trust_ref_math.py. float64.

Evidence: reference_implementation → golden. NOT vendor_oracle.

Usage:
  python scripts/g_trust_capability_reference.py
"""

from __future__ import annotations

import csv
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from g_trust_ref_math import (  # noqa: E402
    estimate_within_subgroup_sigma,
    sample_mean,
    sample_stdev,
)

INPUT = ROOT / "tests" / "fixtures" / "minitab" / "converted" / "PistonRingDiameter.csv"
OUTPUT = ROOT / "tests" / "fixtures" / "minitab" / "expected" / "capability_ref_golden.tsv"

LSL = 73.95
USL = 74.05
TARGET = 74.00
SUBGROUP_SIZE = 5


def load_diameter(path: pathlib.Path) -> list[float]:
    with path.open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None or "Diameter" not in reader.fieldnames:
            raise SystemExit(f"expected Diameter in {path}")
        values = [float(row["Diameter"]) for row in reader]
    if len(values) % SUBGROUP_SIZE != 0:
        raise SystemExit(
            f"row count {len(values)} not divisible by subgroup_size={SUBGROUP_SIZE}"
        )
    return values


def build_subgroups(values: list[float], size: int) -> list[list[float]]:
    return [values[i : i + size] for i in range(0, len(values), size)]


def capability_indices(
    values: list[float], within_sigma: float
) -> dict[str, float]:
    mean = sample_mean(values)
    overall = sample_stdev(values)
    cp = (USL - LSL) / (6.0 * within_sigma)
    cpl = (mean - LSL) / (3.0 * within_sigma)
    cpu = (USL - mean) / (3.0 * within_sigma)
    cpk = min(cpl, cpu)
    pp = (USL - LSL) / (6.0 * overall)
    ppl = (mean - LSL) / (3.0 * overall)
    ppu = (USL - mean) / (3.0 * overall)
    ppk = min(ppl, ppu)
    return {
        "mean": mean,
        "within_sigma": within_sigma,
        "overall_sigma": overall,
        "Cp": cp,
        "Cpk": cpk,
        "Pp": pp,
        "Ppk": ppk,
    }


def compute(values: list[float]) -> tuple[dict[str, float], str]:
    subgroups = build_subgroups(values, SUBGROUP_SIZE)
    within_sigma, method = estimate_within_subgroup_sigma(subgroups)
    return capability_indices(values, within_sigma), method


def write_tsv(metrics: dict[str, float], method: str, path: pathlib.Path) -> None:
    lines = [
        "# source: reference_implementation NIST PMC 1.6 "
        "+ scripts/g_trust_capability_reference.py (Python 3.10+ stdlib float64)",
        "# config: command_id=capability",
        f"# config: lsl={LSL}",
        f"# config: usl={USL}",
        f"# config: target={TARGET}",
        f"# config: subgroup_size={SUBGROUP_SIZE}",
        f"# config: within_sigma_method={method}",
        "# config: overall_sigma_method=sample_standard_deviation",
        "# config: input=converted/PistonRingDiameter.csv",
        "# section: indices",
        "Key\tValue",
    ]
    for key in ("mean", "within_sigma", "overall_sigma", "Cp", "Cpk", "Pp", "Ppk"):
        lines.append(f"{key}\t{metrics[key]:.17g}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    values = load_diameter(INPUT)
    metrics, method = compute(values)
    write_tsv(metrics, method, OUTPUT)
    # Avoid Windows GBK console crash on R̄ in method label.
    safe_method = method.encode("ascii", "backslashreplace").decode("ascii")
    print(f"Wrote {OUTPUT.relative_to(ROOT)} (n={len(values)}, method={safe_method})")
    for key, value in metrics.items():
        print(f"  {key}={value:.17g}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
