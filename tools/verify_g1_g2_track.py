#!/usr/bin/env python3
"""Preflight for Track G1+G2 (formula registry + chart/table copy)."""

from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED_SOURCES = [
    "src/ui/formula_registry_dialog.cpp",
    "src/ui/formula_registry_dialog.h",
    "src/ui/row_visibility_clipboard.cpp",
    "src/ui/row_visibility_clipboard.h",
    "src/ui/output_workspace.cpp",
    "src/ui/output_workspace.h",
    "docs/research/g1-g2-formula-registry-chart-copy.md",
    "samples/product_evolution/g1_g2_manual_acceptance.md",
    "tools/run_g1g2_tests.ps1",
    "docs/algorithm-wiring-index.md",
]

REQUIRED_TESTS = [
    "formula_registry_dialog_test",
    "row_visibility_clipboard_test",
    "analysis_chart_widget_test",
    "output_workspace_test",
    "algorithm_help_dialog_test",
]


def py_row_visibility_footnote(
    excluded: int, hidden: int, analysis_n: int = 0, display_n: int = 0
) -> str:
    if excluded == 0 and hidden == 0 and analysis_n == 0 and display_n == 0:
        return ""
    footnote = (
        f"行可见性契约：排除 {excluded} 行（分析与显示均省略）· "
        f"隐藏 {hidden} 行（仅显示省略，分析仍纳入）"
    )
    if analysis_n != 0 or display_n != 0:
        footnote += f" · 分析 N = {analysis_n} · 显示 N = {display_n}"
    footnote += "（hidden 与 excluded 不得合并叙述）"
    return footnote


def py_append_clipboard_footnote_comments(body: str, footnote: str) -> str:
    if not footnote:
        return body
    lines = body.split("\n")
    for line in footnote.split("\n"):
        if line:
            lines.append(f"# {line}")
    return "\n".join(lines)


def verify_footnote_logic() -> list[str]:
    errors: list[str] = []
    if py_row_visibility_footnote(0, 0, 0, 0) != "":
        errors.append("footnote should be empty when all counts are zero")
    sample = py_row_visibility_footnote(2, 1, 20, 22)
    for token in ("排除 2", "隐藏 1", "分析 N = 20", "hidden"):
        if token not in sample:
            errors.append(f"footnote missing token: {token}")
    tsv = py_append_clipboard_footnote_comments("x\ty\n1\t2", sample)
    if not tsv.startswith("x\ty"):
        errors.append("TSV body prefix lost after footnote append")
    if "# 行可见性契约" not in tsv:
        errors.append("TSV missing # comment footnote line")
    return errors


def main() -> int:
    errors: list[str] = []
    for rel in REQUIRED_SOURCES:
        if not (ROOT / rel).is_file():
            errors.append(f"missing source/doc: {rel}")

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8", errors="replace")
    if "DATALAB_TRACK_G1_G2_TESTS_REGISTERED" not in cmake:
        errors.append("CMakeLists.txt missing G1+G2 track registration marker")
    for target in REQUIRED_TESTS:
        if target not in cmake:
            errors.append(f"cmake target not registered: {target}")

    mainwindow = (ROOT / "src/ui/mainwindow.cpp").read_text(encoding="utf-8", errors="replace")
    if "formula_registry_dialog" not in mainwindow:
        errors.append("mainwindow missing formula registry menu wiring")
    if "chart_for_copy" not in mainwindow:
        errors.append("mainwindow missing chart_for_copy routing")
    if "output_table_view_from_focus" not in mainwindow:
        errors.append("mainwindow missing output table Edit->Copy routing")

    wiring = (ROOT / "docs/algorithm-wiring-index.md").read_text(encoding="utf-8", errors="replace")
    for token in ("Track G1+G2", "FormulaRegistryDialog", "row_visibility_clipboard", "chart_for_copy"):
        if token not in wiring:
            errors.append(f"algorithm-wiring-index.md missing G1+G2 wiring token: {token}")

    header = (ROOT / "src/ui/row_visibility_clipboard.h").read_text(encoding="utf-8")
    if "compose_chart_pixmap_with_footnote" not in header:
        errors.append("row_visibility_clipboard.h missing compose_chart_pixmap_with_footnote declaration")
    if "append_clipboard_footnote_comments" not in header:
        errors.append("row_visibility_clipboard.h missing append_clipboard_footnote_comments declaration")

    errors.extend(verify_footnote_logic())

    output_workspace = (ROOT / "src/ui/output_workspace.cpp").read_text(encoding="utf-8")
    if "chart_for_copy" not in output_workspace:
        errors.append("output_workspace missing chart_for_copy")
    if "QShortcut" not in output_workspace or "copy_chart_requested" not in output_workspace:
        errors.append("output_workspace missing Ctrl+C copy chart shortcut wiring")

    if errors:
        print("G1+G2 track preflight: FAIL")
        for item in errors:
            print(f"  - {item}")
        return 1

    print("G1+G2 track preflight: OK")
    print(f"  sources/docs: {len(REQUIRED_SOURCES)} present")
    print(f"  cmake tests: {len(REQUIRED_TESTS)} registered")
    print("  footnote/TSV logic: OK (python mirror)")
    print("  qt tests: python tools/check_g1g2_build_ready.py then tools/run_g1g2_tests.ps1")
    return 0


if __name__ == "__main__":
    sys.exit(main())
