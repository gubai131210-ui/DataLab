#!/usr/bin/env python3
"""Print dual-line Phase 3 script-side acceptance status (no Qt Creator run)."""

from __future__ import annotations

import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

CHECKS: list[tuple[str, list[str]]] = [
    ("section 3.1 registry", ["tools/verify_phase3_prefilter_registry.py"]),
    ("scenario registry (84)", ["tools/verify_scenario_prefilter_registry.py"]),
    ("deepen registry (37)", ["tools/verify_deepen_prefilter_registry.py"]),
    ("13/13 vertical slice scenario", ["tools/verify_vertical_slice_scenario_coverage.py"]),
    ("3/3 interpretation gate scenario", ["tools/verify_interpretation_gate_scenario_coverage.py"]),
    ("9/9 customer_keeps scenario", ["tools/verify_customer_keeps_scenario_coverage.py"]),
    ("3/3 domain gate scenario", ["tools/verify_domain_gate_scenario_coverage.py"]),
    ("interpretation audit", ["tools/audit_interpretation_localization.py"]),
    ("prefilter list verify", ["tools/list_phase3_prefilter_tests.py", "--verify"]),
    ("qt target map", ["tools/list_qt_creator_test_targets.py"]),
    ("doe k4 fixture", ["tools/verify_doe_ccd_k4_fixture.py"]),
    ("G1+G2 track preflight", ["tools/verify_g1_g2_track.py"]),
]

REF_SCRIPT = ROOT / "tools/reference_implementation_preflight.ps1"


def safe_console(text: str) -> str:
    if hasattr(sys.stdout, "encoding") and sys.stdout.encoding and sys.stdout.encoding.lower().replace("-", "") in {
        "utf8",
        "utf_8",
        "cp65001",
    }:
        return text
    return text.encode("ascii", errors="replace").decode("ascii")


def run_check(label: str, args: list[str]) -> tuple[bool, str]:
    cmd = [sys.executable, str(ROOT / args[0]), *args[1:]]
    proc = subprocess.run(
        cmd,
        cwd=ROOT,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    detail = (proc.stdout or proc.stderr or "").strip().splitlines()
    summary = detail[-1] if detail else f"exit {proc.returncode}"
    for line in detail:
        if (
            "registry:" in line
            or line.startswith("covered=")
            or "scenario map:" in line
        ):
            summary = line
            break
    return proc.returncode == 0, summary


def main() -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    print("# DataLab dual-line acceptance — script-side status\n")
    failed = 0
    width = max(len(label) for label, _ in CHECKS)
    for label, args in CHECKS:
        ok, summary = run_check(label, args)
        status = "OK" if ok else "FAIL"
        if not ok:
            failed += 1
        print(f"  {label.ljust(width)}  {status}  ({safe_console(summary)})")

    ref_ok = True
    ref_summary = "skipped"
    if REF_SCRIPT.is_file():
        proc = subprocess.run(
            ["powershell", "-File", str(REF_SCRIPT)],
            cwd=ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        ref_ok = proc.returncode == 0
        ref_summary = "reference_implementation preflight"
        if not ref_ok:
            failed += 1
        print(f"  {'reference scripts'.ljust(width)}  {'OK' if ref_ok else 'FAIL'}  ({ref_summary})")

    print()
    total = len(CHECKS) + (1 if REF_SCRIPT.is_file() else 0)
    passed = total - failed
    if failed == 0:
        print(f"Script-side: {passed}/{total} checks passed — ready for Qt Creator.")
    else:
        print(f"Script-side: {passed}/{total} checks passed — fix failures before Qt Creator.")

    print("\nQt Creator (user local):")
    print("  python tools/print_qt_creator_signoff_batches.py          # checklist")
    print("  python tools/list_qt_creator_test_targets.py --by-target              # 61")
    print("  python tools/list_qt_creator_test_targets.py --deepen --by-target       # 37")
    print("  python tools/list_qt_creator_test_targets.py --by-target --algorithm-regression")
    print("\nManual: samples/phase0_baselines/phase3_manual_acceptance_index.md")
    print("        samples/product_evolution/unified_track_acceptance_plan.md  # G1-G8 末尾统一测")
    print("        samples/product_evolution/g1_g2_manual_acceptance.md       # G1+G2 明细（已签）")
    print("Runbook: docs/research/qt-creator-dual-line-acceptance-runbook.md")
    print("\nProduct evolution: continuous delivery — Qt Creator unified gate when YOU ready (see unified_track_acceptance_plan.md).")
    print("Phase 3 PDF: still signed separately in phase3-cross-page-pdf-manual-acceptance.md §6.")

    build_proc = subprocess.run(
        [sys.executable, str(ROOT / "tools/check_g1g2_build_ready.py")],
        cwd=ROOT,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    build_line = (build_proc.stdout or build_proc.stderr or "").strip().splitlines()
    build_head = build_line[0] if build_line else f"exit {build_proc.returncode}"
    print(f"\nG1+G2 Qt build: {safe_console(build_head)}")
    if build_proc.returncode == 2:
        print("  -> Projects -> Run CMake (re-configure), then Build All.")
    elif build_proc.returncode == 3:
        print("  -> Build the 5 G1+G2 test targets (or Build All).")
    elif build_proc.returncode == 0:
        print("  -> powershell -File tools/run_g1g2_tests.ps1")

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
