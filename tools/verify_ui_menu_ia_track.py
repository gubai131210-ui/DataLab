#!/usr/bin/env python3
"""Preflight for UI Menu IA track (2026-08-23). U1+U2+U3 gate."""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from _patch_menu_ia_commands import (  # noqa: E402
    CMD_START,
    MAP,
    find_command_end,
    load_taxonomy,
)

REQUIRED = [
    "docs/research/ui-menu-ia-minitab-taxonomy-2026-08-23.md",
    "docs/research/ui-menu-ia-command-taxonomy-map-2026-08-23.md",
    "docs/research/goal-wave-2026-08-23-ui-menu-ia-layout-plan-and-mega-prompt.md",
    "docs/research/goal-wave-2026-08-23-ui-menu-ia-layout.md",
    "src/ui/analysis_commands.cpp",
    "src/ui/analysis_commands.h",
    "src/ui/mainwindow.cpp",
    "tests/ui_menu_ia_track_test.cpp",
    "resources/help/algorithm_help.json",
    "translations/ui_menu_strings.json",
    "samples/product_evolution/unified_track_acceptance_plan.md",
]

ALLOWED_TOP = {"统计", "控制图", "质量工具", "图形"}

# Sample anchors: id → (path, group)
ANCHORS = {
    "cox_regression": ("统计", "可靠性"),
    "logistic_regression": ("统计", "回归"),
    "stepwise_regression": ("统计", "回归"),
    "bootstrap_two_sample": ("统计", "推断 / 仿真"),
    "imr": ("控制图", "计量图"),
    "nonparametric_capability": ("质量工具", "过程能力"),
    "pareto": ("质量工具", "质量图 / 规划"),
    "histogram": ("图形", "分布与单变量"),
    "kmeans": ("统计", "多变量"),
}

# Hard-coded whitelist smell in mainwindow (banned long id chains)
BANNED_MAINWINDOW_SNIPPETS = [
    'id == QStringLiteral("descriptive")',
    'primary_analysis_menu',
    'analysis_menu_group',
]

def fail(msg: str) -> None:
    print(f"FAIL: {msg}")
    sys.exit(1)


def ok(msg: str) -> None:
    print(f"OK: {msg}")


def parse_commands(cpp: str) -> dict[str, tuple[str, str]]:
    """Return id → (menu_path, menu_group) from analysis_commands.cpp."""
    out: dict[str, tuple[str, str]] = {}
    for m in CMD_START.finditer(cpp):
        cid = m.group("id")
        path = m.group("path")
        brace = m.start() + len("\n        ")
        if cpp[brace] != "{":
            brace = cpp.find("{", m.start())
        end = find_command_end(cpp, brace)
        chunk = cpp[brace : end + 1]
        gm = re.search(
            r',\n            QStringLiteral\("([^"]+)"\)\s*\}$',
            chunk,
        )
        if not gm:
            fail(f"missing menu_group for {cid}")
        out[cid] = (path, gm.group(1))
    return out


def main() -> None:
    for rel in REQUIRED:
        if not (ROOT / rel).is_file():
            fail(f"missing {rel}")
        ok(rel)

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    if "ui_menu_ia_track_test" not in cmake:
        fail("CMakeLists missing ui_menu_ia_track_test")
    ok("CMake ui_menu_ia_track_test")

    tax = load_taxonomy(MAP)
    if len(tax) < 130:
        fail(f"taxonomy map too small: {len(tax)}")
    ok(f"taxonomy map entries={len(tax)}")

    cpp = (ROOT / "src/ui/analysis_commands.cpp").read_text(encoding="utf-8")
    commands = parse_commands(cpp)
    if len(commands) != len(tax):
        fail(f"command count {len(commands)} != taxonomy {len(tax)}")
    missing = set(tax) - set(commands)
    extra = set(commands) - set(tax)
    if missing:
        fail(f"commands missing ids: {sorted(missing)[:20]}")
    if extra:
        fail(f"commands extra ids: {sorted(extra)[:20]}")
    ok(f"commands parsed={len(commands)}")

    bad = []
    for cid, (ep, eg) in tax.items():
        path, group = commands[cid]
        if path != ep or group != eg:
            bad.append((cid, path, group, ep, eg))
        if path not in ALLOWED_TOP:
            fail(f"illegal top-level menu_path for {cid}: {path!r}")
        if not group.strip():
            fail(f"empty menu_group for {cid}")
    if bad:
        fail(f"path/group mismatch sample: {bad[:5]}")
    ok("all menu_path+menu_group match taxonomy")

    for cid, (ep, eg) in ANCHORS.items():
        path, group = commands[cid]
        if path != ep or group != eg:
            fail(f"anchor {cid}: got {(path, group)} want {(ep, eg)}")
        ok(f"anchor {cid} → {ep} > {eg}")

    # Direct leaf count under 统计 would be commands with empty group — must be 0
    flat_stat = [cid for cid, (p, g) in commands.items() if p == "统计" and not g.strip()]
    if flat_stat:
        fail(f"flat under 统计: {flat_stat}")
    ok("no flat leaves under 统计 (all grouped)")

    mw = (ROOT / "src/ui/mainwindow.cpp").read_text(encoding="utf-8")
    for snip in BANNED_MAINWINDOW_SNIPPETS:
        if snip in mw:
            # allow comment mentions of old names only if not as function defs
            if snip in ("primary_analysis_menu", "analysis_menu_group"):
                if re.search(rf"\b{snip}\s*\(", mw):
                    fail(f"mainwindow still defines/calls {snip}")
            elif snip == 'return QStringLiteral("统计");':
                # Only fail if near control-chart override remnant
                if 'menu_path == QStringLiteral("控制图")' in mw and snip in mw:
                    fail("mainwindow still forces 控制图 → 统计")
            else:
                fail(f"mainwindow whitelist smell: {snip}")
    if "command.menu_path" not in mw or "command.menu_group" not in mw:
        fail("mainwindow must read command.menu_path and command.menu_group")
    if "k_analysis_top_menus" not in mw:
        fail("mainwindow missing k_analysis_top_menus four-top order")
    ok("mainwindow declarative render (no giant whitelist)")

    # Depth: no nested submenu of submenu construction
    if mw.count("addMenu") > 0 and "menu->addMenu" in mw:
        # one level of addMenu under top is OK; forbid target->addMenu or submenu->addMenu
        if "target->addMenu" in mw or "submenu->addMenu" in mw:
            fail("possible depth>1 submenu nesting")
    ok("cascade depth hard-cap markers OK")

    help_entries = json.loads(
        (ROOT / "resources/help/algorithm_help.json").read_text(encoding="utf-8")
    ).get("entries", [])
    by_id = {e.get("id"): e for e in help_entries}
    help_wrong = []
    for cid, (ep, eg) in tax.items():
        entry = by_id.get(cid)
        if entry is None:
            continue  # allow sparse help
        expected = f"{ep} > {eg}"
        if entry.get("menu_path") != expected:
            help_wrong.append((cid, entry.get("menu_path"), expected))
    if help_wrong:
        fail(f"help menu_path mismatch: {help_wrong[:8]}")
    ok("algorithm_help.json menu_path aligned")

    strings = (ROOT / "translations/ui_menu_strings.json").read_text(encoding="utf-8")
    for label in (
        "推断 / 仿真",
        "计量图",
        "过程能力",
        "分布与单变量",
        "质量图 / 规划",
    ):
        if label not in strings:
            fail(f"ui_menu_strings missing group {label!r}")
    ok("ui_menu_strings new groups present")

    dod = (ROOT / "docs/research/goal-wave-2026-08-23-ui-menu-ia-layout.md").read_text(
        encoding="utf-8"
    )
    if "[x]" not in dod and "[X]" not in dod:
        fail("goal-wave DoD md has no checked items yet")
    ok("goal-wave DoD md exists with checks")

    acceptance = (
        ROOT / "samples/product_evolution/unified_track_acceptance_plan.md"
    ).read_text(encoding="utf-8")
    if "verify_ui_menu_ia_track" not in acceptance:
        fail("acceptance plan missing verify_ui_menu_ia_track")
    ok("acceptance §2 references UI Menu IA")

    wiring = (ROOT / "docs/algorithm-wiring-index.md").read_text(encoding="utf-8")
    if "ui-menu-ia" not in wiring and "Menu IA" not in wiring and "菜单 IA" not in wiring:
        fail("wiring-index missing Menu IA note")
    ok("wiring-index Menu IA note")

    # Regression: wave4 verify must still pass when invoked
    import subprocess

    r = subprocess.run(
        [sys.executable, str(ROOT / "tools/verify_algorithm_wave4_track.py")],
        cwd=str(ROOT),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if r.returncode != 0:
        fail(f"wave4 verify failed:\n{r.stdout}\n{r.stderr}")
    ok("verify_algorithm_wave4_track.py PASS")

    test_cpp = (ROOT / "tests/ui_menu_ia_track_test.cpp").read_text(encoding="utf-8")
    for marker in (
        "cox_regression",
        "menu_group",
        "menu_path",
        "logistic_regression",
        "nonparametric_capability",
    ):
        if marker not in test_cpp:
            fail(f"ui_menu_ia_track_test missing marker {marker}")
    ok("ui_menu_ia_track_test markers")

    print("\nui menu IA preflight: PASS")


if __name__ == "__main__":
    main()
