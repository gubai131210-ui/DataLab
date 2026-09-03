#!/usr/bin/env python3
"""Agent E gate: run all learning-center verifiers and content-quality checks."""
from __future__ import annotations

import json
import re
import sqlite3
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SQLITE = ROOT / "resources/help/learning_center.sqlite"


def run_script(relative: str) -> tuple[int, str]:
    path = ROOT / relative
    result = subprocess.run(
        [sys.executable, str(path)],
        cwd=ROOT,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    output = (result.stdout or "") + (result.stderr or "")
    return result.returncode, output.strip()


def check_tutorial_content() -> list[str]:
    errors: list[str] = []
    conn = sqlite3.connect(SQLITE)
    try:
        rows = conn.execute(
            "SELECT command_id, used_for, not_for, scenario, click_steps, "
            "implemented_status FROM tutorials"
        ).fetchall()
    finally:
        conn.close()

    for command_id, used_for, not_for, scenario, click_steps, status in rows:
        if not str(used_for).strip():
            errors.append(f"{command_id}: empty used_for")
        if not str(not_for).strip():
            errors.append(f"{command_id}: empty not_for")
        if not str(scenario).strip():
            errors.append(f"{command_id}: empty scenario")
        steps = json.loads(click_steps or "[]")
        if len(steps) < 2:
            errors.append(f"{command_id}: click_steps too short ({len(steps)})")
        if status == "formula_reference":
            joined = " ".join(steps)
            if "公式" not in joined and "菜单" not in joined:
                errors.append(f"{command_id}: formula_reference missing limitation hint")
    return errors


def check_forbidden_phrases() -> list[str]:
    forbidden = ("过程合格", "必须停线", "已证明正态", "量具通过")
    errors: list[str] = []
    conn = sqlite3.connect(SQLITE)
    try:
        rows = conn.execute("SELECT command_id, output_guide FROM tutorials").fetchall()
    finally:
        conn.close()

    for command_id, output_guide in rows:
        blob = str(output_guide)
        for phrase in forbidden:
            negation = re.compile(r"(不能|勿|不应|禁止).{0,8}" + re.escape(phrase))
            if phrase in blob and not negation.search(blob):
                errors.append(f"{command_id}: output_guide contains forbidden phrase '{phrase}'")
    return errors


def main() -> int:
    if not SQLITE.exists():
        print(f"FAIL: missing {SQLITE}")
        return 1

    failures: list[str] = []

    for script in (
        "tools/verify_learning_center_db.py",
        "tools/verify_learning_research_notes.py",
        "tools/verify_learning_dataset_mapping.py",
    ):
        code, output = run_script(script)
        label = script
        if code != 0:
            failures.append(f"{label} exited {code}: {output[:300]}")
        else:
            print(f"OK: {label}")

    for err in check_tutorial_content():
        failures.append(err)
    for err in check_forbidden_phrases():
        failures.append(err)

    if failures:
        print("FAIL: learning_center_gate")
        for item in failures:
            print(" -", item)
        return 1

    print("PASS: learning_center_gate (db + research + mapping + content quality)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
