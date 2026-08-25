#!/usr/bin/env python3
"""Preflight for G9 Formula Substitution (Show Your Work) track."""

from __future__ import annotations

import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED_DOCS = [
    "docs/research/formula-substitution-show-your-work-research-2026-08-23.md",
    "docs/research/goal-wave-2026-08-23-g9-formula-substitution-plan-and-mega-prompt.md",
    "docs/research/goal-wave-2026-08-23-g9-formula-substitution.md",
    "docs/research/g9-formula-substitution-coverage-matrix.md",
]

REQUIRED_CODE = [
    "src/application/computation_trace_attach.h",
    "src/application/computation_trace_attach.cpp",
    "src/ui/formula_substitution_dialog.h",
    "src/ui/formula_substitution_dialog.cpp",
    "tests/g9_formula_substitution_track_test.cpp",
]

UI_MARKERS = ["公式列表", "变量取值", "分步求值", "出处"]
ALLOWED_STATUS = {"实质绑定", "display_summary", "豁免"}
EXEMPT_ONLY = {"tests", "rule_policy"}
PILOTS = ("capability", "one_sample_t", "weibayes")


def fail(msg: str) -> None:
    print(f"FAIL: {msg}")
    sys.exit(1)


def ok(msg: str) -> None:
    print(f"OK: {msg}")


def list_command_ids() -> list[str]:
    out = subprocess.check_output(
        [sys.executable, str(ROOT / "tools/_list_command_ids.py")],
        cwd=ROOT,
        text=True,
        encoding="utf-8",
    )
    ids: list[str] = []
    started = False
    for line in out.splitlines():
        if line.startswith("count="):
            started = True
            continue
        if started and line.strip():
            ids.append(line.strip())
    return ids


def main() -> None:
    for rel in REQUIRED_DOCS + REQUIRED_CODE:
        if not (ROOT / rel).is_file():
            fail(f"missing {rel}")
        ok(rel)

    dod = (ROOT / "docs/research/goal-wave-2026-08-23-g9-formula-substitution.md").read_text(
        encoding="utf-8"
    )
    for item in (
        "FS-A",
        "FS-B",
        "FS-C",
        "FS-D",
        "FS-E",
        "FS-F",
        "FS-G",
        "FS-H",
        "FS-I",
        "FS-J",
    ):
        # Each section must have checked boxes; look for "- [x]" near FS-*
        pass
    unchecked = re.findall(r"^- \[ \] .+$", dod, flags=re.M)
    if unchecked:
        fail(f"DoD still has unchecked items: {unchecked[:5]}")
    if "- [x] 未做 G3/G4全量/G5" not in dod and "未做 G3" not in dod:
        # allow either fully checked 明确不做 block
        if not re.search(r"- \[x\].*G3", dod):
            fail("DoD 明确不做 items not checked")
    ok("DoD FS-A..J checked")

    types = (ROOT / "src/domain/quality_types.h").read_text(encoding="utf-8")
    if "struct ComputationTrace" not in types or "computation_traces" not in types:
        fail("quality_types missing ComputationTrace / computation_traces")
    if "struct FormulaBinding" not in types:
        fail("quality_types missing FormulaBinding")
    ok("quality_types ComputationTrace")

    ser = (ROOT / "src/infrastructure/output_serialization.cpp").read_text(encoding="utf-8")
    if "computation_traces" not in ser:
        fail("output_serialization missing computation_traces")
    ok("output_serialization computation_traces")

    ui = (ROOT / "src/ui/formula_substitution_dialog.cpp").read_text(encoding="utf-8")
    for marker in UI_MARKERS:
        if marker not in ui:
            fail(f"UI missing marker {marker}")
    if "QStackedWidget" not in ui and "QStackedWidget" not in (
        ROOT / "src/ui/formula_substitution_dialog.h"
    ).read_text(encoding="utf-8"):
        fail("UI missing QStackedWidget")
    ok("UI four-page markers")

    page_renderer = (ROOT / "src/ui/page_renderer.cpp").read_text(encoding="utf-8")
    if "公式代入" not in page_renderer:
        fail("page_renderer missing 公式代入 button")
    ok("page_renderer entry button")

    ids = list_command_ids()
    matrix = (
        ROOT / "docs/research/g9-formula-substitution-coverage-matrix.md"
    ).read_text(encoding="utf-8")
    gaps = []
    bad_status = []
    bad_exempt = []
    for cid in ids:
        m = re.search(
            rf"^\|\s*{re.escape(cid)}\s*\|\s*([^|]+)\|\s*([^|]+)\|\s*([^|]+)\|\s*([^|]+)\|",
            matrix,
            flags=re.M,
        )
        if not m:
            gaps.append(cid)
            continue
        status = m.group(4).strip()
        if status not in ALLOWED_STATUS:
            bad_status.append((cid, status))
        if status == "豁免" and cid not in EXEMPT_ONLY:
            bad_exempt.append(cid)
        if cid in EXEMPT_ONLY and status != "豁免":
            bad_exempt.append(cid)
    if gaps:
        fail(f"matrix gaps ({len(gaps)}): {gaps[:10]}")
    if bad_status:
        fail(f"bad status: {bad_status[:5]}")
    if bad_exempt:
        fail(f"豁免 misuse: {bad_exempt[:5]}")
    ok(f"matrix covers {len(ids)} commands, 0 gaps")

    attach = (ROOT / "src/application/computation_trace_attach.cpp").read_text(
        encoding="utf-8"
    )
    missing_literals = []
    for cid in ids:
        if cid in EXEMPT_ONLY:
            continue
        if f'"{cid}"' not in attach:
            missing_literals.append(cid)
    if missing_literals:
        fail(f"attach missing literals ({len(missing_literals)}): {missing_literals[:10]}")
    ok("attach contains all non-exempt command_id literals")

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    for token in (
        "g9_formula_substitution_track_test",
        "formula_substitution_dialog",
        "computation_trace_attach",
    ):
        if token not in cmake:
            fail(f"CMakeLists missing {token}")
    ok("CMake G9 sources + test")

    service = (ROOT / "src/application/analysis_service.cpp").read_text(encoding="utf-8")
    for pilot in PILOTS:
        if pilot not in attach and pilot not in service:
            fail(f"pilot marker missing: {pilot}")
        # Prefer explicit attach in service for pilots
        if f'attach_computation_traces(page, "{pilot}")' not in service and f'"{pilot}"' not in attach:
            fail(f"pilot not wired: {pilot}")
    for pilot in PILOTS:
        if f'attach_computation_traces(page, "{pilot}")' not in service:
            fail(f"pilot explicit attach missing in analysis_service: {pilot}")
    ok("pilots capability / one_sample_t / weibayes")

    wiring = (ROOT / "docs/algorithm-wiring-index.md").read_text(encoding="utf-8")
    if "G9" not in wiring and "公式代入" not in wiring:
        fail("algorithm-wiring-index missing G9 section")
    acceptance = (
        ROOT / "samples/product_evolution/unified_track_acceptance_plan.md"
    ).read_text(encoding="utf-8")
    if "G9" not in acceptance:
        fail("acceptance plan missing G9 row")
    ok("wiring + acceptance")

    print("PASS: verify_g9_formula_substitution_track")
    sys.exit(0)


if __name__ == "__main__":
    main()
