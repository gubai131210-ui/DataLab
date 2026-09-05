#!/usr/bin/env python3
"""G-Trust reference_implementation for capability_sixpack.

Reuses capability indices from g_trust_capability_reference (same input/specs).
Plot contract is documented only (not drawn): current AnalysisService builds 6
plots when subgroup_size>1 (Xbar, hist, R, prob, last-points, capability bar).

Primary formula URL: same NIST PMC 1.6 as capability.
Accessed: 2026-09-05 UTC+8

Evidence: reference_implementation → golden. NOT vendor_oracle.
Dependencies: Python 3.10+ stdlib + g_trust_capability_reference / g_trust_ref_math.

Usage:
  python scripts/g_trust_capability_sixpack_reference.py
"""

from __future__ import annotations

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

import g_trust_capability_reference as cap  # noqa: E402

OUTPUT = (
    ROOT
    / "tests"
    / "fixtures"
    / "minitab"
    / "expected"
    / "capability_sixpack_ref_golden.tsv"
)

# Honest contract vs AnalysisService::capability_sixpack with subgroup_size=5.
MIN_PLOTS = 5
EXPECTED_PLOTS = 6


def write_tsv(metrics: dict[str, float], method: str, path: pathlib.Path) -> None:
    lines = [
        "# source: reference_implementation NIST PMC 1.6 "
        "+ scripts/g_trust_capability_sixpack_reference.py "
        "(indices via g_trust_capability_reference; plot contract only)",
        "# config: command_id=capability_sixpack",
        f"# config: lsl={cap.LSL}",
        f"# config: usl={cap.USL}",
        f"# config: target={cap.TARGET}",
        f"# config: subgroup_size={cap.SUBGROUP_SIZE}",
        f"# config: within_sigma_method={method}",
        "# config: overall_sigma_method=sample_standard_deviation",
        "# config: input=converted/PistonRingDiameter.csv",
        "# section: indices",
        "Key\tValue",
    ]
    for key in ("mean", "within_sigma", "overall_sigma", "Cp", "Cpk", "Pp", "Ppk"):
        lines.append(f"{key}\t{metrics[key]:.17g}")
    lines.extend(
        [
            "# section: contract",
            "Key\tValue",
            f"min_plots\t{MIN_PLOTS}",
            f"expected_plots\t{EXPECTED_PLOTS}",
        ]
    )
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    values = cap.load_diameter(cap.INPUT)
    metrics, method = cap.compute(values)
    write_tsv(metrics, method, OUTPUT)
    print(f"Wrote {OUTPUT.relative_to(ROOT)}")
    print(f"  contract min_plots={MIN_PLOTS} expected_plots={EXPECTED_PLOTS}")
    for key in ("Cp", "Cpk", "Pp", "Ppk"):
        print(f"  {key}={metrics[key]:.17g}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
