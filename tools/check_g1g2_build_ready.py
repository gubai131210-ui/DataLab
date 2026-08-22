#!/usr/bin/env python3
"""Check whether Qt Creator build dir(s) are ready for G1+G2 tests."""

from __future__ import annotations

import argparse
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

G1_G2_TEST_TARGETS = [
    "formula_registry_dialog_test",
    "row_visibility_clipboard_test",
    "analysis_chart_widget_test",
    "output_workspace_test",
    "algorithm_help_dialog_test",
]

DEFAULT_BUILD_DIR = "build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug"


def inspect_build_dir(build_dir: pathlib.Path) -> tuple[int, list[str]]:
    lines: list[str] = []
    if not build_dir.is_dir():
        lines.append(f"build directory missing: {build_dir}")
        return 1, lines

    cache = build_dir / "CMakeCache.txt"
    if not cache.is_file():
        lines.append(f"CMakeCache.txt missing in {build_dir}")
        lines.append("Qt Creator: Projects -> Run CMake.")
        return 1, lines

    cache_text = cache.read_text(encoding="utf-8", errors="replace")
    if "DATALAB_TRACK_G1_G2_TESTS_REGISTERED:INTERNAL=TRUE" not in cache_text:
        lines.append("STALE CMAKE - G1+G2 track marker missing (Run CMake re-configure after pulling CMakeLists.txt)")
        lines.append("  Qt Creator: Projects -> Run CMake")
        lines.append("  If still STALE: Projects -> Clear CMake Configuration, then Run CMake")
        lines.append("  Expect configure log: Track G1+G2: 5 test targets registered")
        return 2, lines

    unregistered = [name for name in G1_G2_TEST_TARGETS if name not in cache_text]
    missing_exe = [
        name
        for name in G1_G2_TEST_TARGETS
        if not (build_dir / f"{name}.exe").is_file()
    ]

    if unregistered:
        lines.append("STALE CMAKE - test targets not registered (Run CMake re-configure):")
        for name in unregistered:
            lines.append(f"  - {name}")
        return 2, lines

    if missing_exe:
        lines.append("NOT BUILT - executables missing (Build All or build 5 targets):")
        for name in missing_exe:
            lines.append(f"  - {name}.exe")
        return 3, lines

    lines.append("OK - ready to run tools/run_g1g2_tests.ps1")
    return 0, lines


def discover_build_dirs() -> list[pathlib.Path]:
    build_root = ROOT / "build"
    if not build_root.is_dir():
        return []
    found: list[pathlib.Path] = []
    for child in sorted(build_root.iterdir()):
        if child.is_dir() and (child / "CMakeCache.txt").is_file():
            found.append(child)
    return found


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-dir",
        default=DEFAULT_BUILD_DIR,
        help="Preferred Qt Creator build directory (relative to repo root)",
    )
    parser.add_argument(
        "--scan-all",
        action="store_true",
        help="Also list other build/*/CMakeCache.txt directories",
    )
    args = parser.parse_args()

    preferred = ROOT / args.build_dir
    code, detail = inspect_build_dir(preferred)
    print(
        f"G1+G2 build ready [{preferred.relative_to(ROOT)}]: "
        + ("OK" if code == 0 else detail[0].split("-")[0].strip() if detail else "FAIL")
    )
    for line in detail:
        print(f"  {line}")

    if args.scan_all:
        others = [path for path in discover_build_dirs() if path.resolve() != preferred.resolve()]
        if others:
            print("\nOther configured build directories:")
            for path in others:
                other_code, other_detail = inspect_build_dir(path)
                status = "OK" if other_code == 0 else other_detail[0].split("-")[0].strip()
                print(f"  - {path.relative_to(ROOT)}: {status}")

    return code


if __name__ == "__main__":
    sys.exit(main())
