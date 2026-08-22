#!/usr/bin/env python3
"""Preflight for G6 Command Wizard track (2026-08-23). W1+W2+W3+W4 gate."""

from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED = [
    "docs/research/g6-command-wizard-ux-research-2026-08-23.md",
    "docs/research/goal-wave-2026-08-23-g6-command-wizard.md",
    "src/application/command_recommendation_engine.h",
    "src/application/command_recommendation_engine.cpp",
    "src/ui/command_wizard_dialog.h",
    "src/ui/command_wizard_dialog.cpp",
    "src/ui/mainwindow.cpp",
    "tests/g6_command_wizard_track_test.cpp",
    "samples/product_evolution/unified_track_acceptance_plan.md",
    "docs/algorithm-wiring-index.md",
]

ENGINE_MARKERS = ("recommend", "Recommendation", "CommandWizardIntent")
WIZARD_MARKERS = ("openAnalysisRequested", "CommandWizardDialog")
MAINWINDOW_MARKERS = ("open_command_wizard", "command_wizard")
TEST_MARKERS = ("G6_ENGINE", "openAnalysisRequested")


def fail(msg: str) -> None:
    print(f"FAIL: {msg}")
    sys.exit(1)


def ok(msg: str) -> None:
    print(f"OK: {msg}")


def strip_cpp_comments(text: str) -> str:
    """Remove // and /* */ comments so AnalysisService:: in prose cannot false-pass."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    text = re.sub(r"//.*?$", "", text, flags=re.MULTILINE)
    return text


def check_no_analysis_service_call(rel: str) -> None:
    raw = (ROOT / rel).read_text(encoding="utf-8")
    code = strip_cpp_comments(raw)
    if "AnalysisService::" in code:
        fail(f"{rel} must not call AnalysisService:: (found outside comments)")
    ok(f"{rel} has no AnalysisService:: call")


def count_private_slots(test_cpp: str) -> int:
    slots = re.findall(r"^\s*void\s+(t\d+_\w+|uiSmoke\w+)\s*\(", test_cpp, re.MULTILINE)
    return len(slots)


def check_dod_w1_w4(dod: str) -> None:
    if "2026-08-23" not in dod and "访问日期" not in dod:
        fail("DoD md missing access date or 2026-08-23")
    ok("DoD date / 2026-08-23 present")

    for n in (1, 2, 3, 4):
        m = re.search(
            rf"###\s*W{n}\b.*?(?=###\s*W\d|###\s*|##\s*|\Z)",
            dod,
            re.DOTALL | re.IGNORECASE,
        )
        if not m:
            fail(f"DoD missing ### W{n} section")
        body = m.group(0)
        checks = re.findall(r"- \[[ xX]\]", body)
        if not checks:
            fail(f"DoD W{n} has no checklist items")
        unchecked = [c for c in checks if c.lower() == "- [ ]"]
        if unchecked:
            fail(f"DoD W{n} still has unchecked items ({len(unchecked)})")
        ok(f"DoD W{n} all checked ({len(checks)} items)")


def main() -> None:
    for rel in REQUIRED:
        if not (ROOT / rel).is_file():
            fail(f"missing {rel}")
        ok(rel)

    dod = (ROOT / "docs/research/goal-wave-2026-08-23-g6-command-wizard.md").read_text(
        encoding="utf-8"
    )
    check_dod_w1_w4(dod)

    engine_h = (ROOT / "src/application/command_recommendation_engine.h").read_text(
        encoding="utf-8"
    )
    engine_cpp = (ROOT / "src/application/command_recommendation_engine.cpp").read_text(
        encoding="utf-8"
    )
    for marker in ENGINE_MARKERS:
        if marker not in engine_h and marker not in engine_cpp:
            fail(f"engine missing marker {marker!r}")
    ok("engine recommend / Recommendation / CommandWizardIntent")

    wizard_h = (ROOT / "src/ui/command_wizard_dialog.h").read_text(encoding="utf-8")
    wizard_cpp = (ROOT / "src/ui/command_wizard_dialog.cpp").read_text(encoding="utf-8")
    for marker in WIZARD_MARKERS:
        if marker not in wizard_h and marker not in wizard_cpp:
            fail(f"wizard missing marker {marker!r}")
    ok("wizard CommandWizardDialog / openAnalysisRequested")

    check_no_analysis_service_call("src/application/command_recommendation_engine.cpp")
    check_no_analysis_service_call("src/ui/command_wizard_dialog.cpp")

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    if "g6_command_wizard_track_test" not in cmake:
        fail("CMakeLists missing g6_command_wizard_track_test")
    ok("CMake g6_command_wizard_track_test")

    acceptance = (
        ROOT / "samples/product_evolution/unified_track_acceptance_plan.md"
    ).read_text(encoding="utf-8")
    if "G6" not in acceptance:
        fail("acceptance plan missing G6")
    if "verify_g6_command_wizard_track" not in acceptance:
        fail("acceptance plan missing verify_g6_command_wizard_track")
    g6_row = None
    for line in acceptance.splitlines():
        if re.search(r"\|\s*\*\*G6\*\*", line):
            g6_row = line
            break
    if g6_row is None:
        fail("acceptance section 2 missing **G6** table row")
    if g6_row.count("\u2705") < 2:
        fail(f"acceptance section 2 G6 row needs script+delivery checkmarks: {g6_row.strip()}")
    if "\u23f3" not in g6_row:
        fail(f"acceptance section 2 G6 unified column should remain pending: {g6_row.strip()}")
    ok("acceptance section 2 G6 row (precheck+track done, unified pending)")

    wiring = (ROOT / "docs/algorithm-wiring-index.md").read_text(encoding="utf-8")
    if "G6" not in wiring and "\u547d\u4ee4 Wizard" not in wiring:
        fail("wiring-index missing G6 / command Wizard")
    if "CommandRecommendationEngine" not in wiring and "command_recommendation_engine" not in wiring:
        fail("wiring-index missing engine reference")
    ok("wiring-index G6 present")

    test_cpp = (ROOT / "tests/g6_command_wizard_track_test.cpp").read_text(encoding="utf-8")
    for marker in TEST_MARKERS:
        if marker not in test_cpp:
            fail(f"g6_command_wizard_track_test missing marker {marker!r}")
    slot_n = count_private_slots(test_cpp)
    has_t01 = bool(re.search(r"\bt01_", test_cpp))
    has_t15 = bool(re.search(r"\bt15_", test_cpp))
    if slot_n < 12 and not (has_t01 and has_t15):
        fail(f"QtTest needs >=12 slots or t01-t15; got slots={slot_n}")
    ok(f"QtTest markers + slots={slot_n} (t01-t15 ok={has_t01 and has_t15})")

    mw = (ROOT / "src/ui/mainwindow.cpp").read_text(encoding="utf-8")
    for marker in MAINWINDOW_MARKERS:
        if marker not in mw:
            fail(f"mainwindow missing {marker!r}")
    ok("mainwindow open_command_wizard / command_wizard")

    print("\nHINT: regression - also run:")
    print("  python tools/verify_ui_menu_ia_track.py")
    print("  python tools/verify_algorithm_wave4_track.py")

    print("\ng6 command wizard preflight: PASS")


if __name__ == "__main__":
    main()