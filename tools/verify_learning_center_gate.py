#!/usr/bin/env python3
"""Agent E gate: run all learning-center verifiers and content-quality checks."""
from __future__ import annotations

import json
import os
import re
import sqlite3
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SQLITE = ROOT / "resources/help/learning_center.sqlite"
INVENTORY = ROOT / "docs/research/_tmp_command_inventory.json"

SEVEN_PLUS = (
    "prereq_quiz",
    "self_explain",
    "fade_levels",
    "retrieval_quiz",
    "misconceptions",
)

BANNED_OLD_DATASET_IDS = (
    "smt_paste_height",
    "two_line_thickness",
    "paired_rework",
    "anova_cavity",
    "corr_temp_offset",
    "attribute_defect",
    "gage_rr_balance",
    "doe_factorial_demo",
    "reliability_cycles",
    "ts_weekly_yield",
)


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


def current_wave() -> int:
    raw = os.environ.get("LEARNING_CENTER_WAVE", "0").strip().lower()
    if raw in {"all", "final", "done"}:
        return 5
    try:
        return int(raw)
    except ValueError:
        return 0


def token_pattern(old_id: str) -> re.Pattern[str]:
    return re.compile(rf"(?<![A-Za-z0-9_]){re.escape(old_id)}(?![A-Za-z0-9_])")


def line_is_ban_list_or_obsolescence(line: str) -> bool:
    stripped = line.strip()
    if "BANNED_OLD_DATASET_IDS" in line:
        return True
    if stripped.startswith("#") or stripped.startswith("//"):
        return True
    if re.fullmatch(r'["\'][a-z0-9_]+["\']\s*,?', stripped):
        return True
    if "QStringLiteral(" in line:
        return True
    lowered = line.lower()
    if any(marker in line for marker in ("作废", "删除重建", "淘汰", "禁止残留")):
        return True
    if "banned" in lowered:
        return True
    return False


def check_banned_old_ids(wave: int) -> list[str]:
    """Wave-5: banned old shared-table ids must not remain as live content."""
    if wave < 5:
        return []
    errors: list[str] = []
    patterns = {old: token_pattern(old) for old in BANNED_OLD_DATASET_IDS}
    targets: list[Path] = []
    learning_data = ROOT / "tools/learning_data"
    if learning_data.exists():
        targets.extend(
            p
            for p in learning_data.rglob("*")
            if p.is_file() and p.suffix.lower() in {".py", ".json", ".md", ".txt"}
        )
    for pattern in (
        "build_learning_*.py",
        "build_research_by_id.py",
        "build_learning_research_notes.py",
    ):
        targets.extend(ROOT.joinpath("tools").glob(pattern))
    targets.extend(ROOT.joinpath("tests").glob("learning_center_*_test.cpp"))

    skip_names = {
        "_wave5_cleanup_residues.py",
        "_wave5_scrub_build_scripts.py",
    }
    for path in targets:
        if path.name in skip_names or path.name.startswith("verify_learning_"):
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        rel = path.relative_to(ROOT).as_posix()
        for lineno, line in enumerate(text.splitlines(), 1):
            if line_is_ban_list_or_obsolescence(line):
                continue
            for old, pat in patterns.items():
                if pat.search(line):
                    errors.append(f"{rel}:{lineno}: banned old id `{old}`")
    return errors


def check_research_notes_hints(wave: int) -> list[str]:
    if wave < 5:
        return []
    errors: list[str] = []
    notes = ROOT / "docs/research/learning-center-research-notes.md"
    if not notes.exists():
        return [f"missing {notes}"]
    text = notes.read_text(encoding="utf-8")
    for lineno, line in enumerate(text.splitlines(), 1):
        if "建议 dataset_id" not in line:
            continue
        for old in BANNED_OLD_DATASET_IDS:
            if token_pattern(old).search(line):
                errors.append(
                    f"research-notes:{lineno}: 建议 dataset_id still recommends `{old}`"
                )
    research_json = ROOT / "tools/learning_data/research_by_id.json"
    if research_json.exists():
        data = json.loads(research_json.read_text(encoding="utf-8"))
        for cid, entry in data.items():
            hint = entry.get("dataset_hint") or ""
            if hint in BANNED_OLD_DATASET_IDS:
                errors.append(f"research_by_id.json[{cid}]: dataset_hint={hint}")
    return errors


def ids_through_wave(inventory: dict, wave: int) -> set[str]:
    waves = inventory.get("waves") or {}
    ids: set[str] = set()
    for key, values in waves.items():
        try:
            index = int(key)
        except (TypeError, ValueError):
            continue
        if index <= wave:
            ids.update(values)
    return ids


def parse_json(text: str, default):
    if text is None or str(text).strip() == "":
        return default
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        return None


def check_tutorial_content() -> list[str]:
    errors: list[str] = []
    conn = sqlite3.connect(SQLITE)
    try:
        rows = conn.execute(
            "SELECT command_id, used_for, not_for, scenario, click_steps, "
            "implemented_status, dialog_fill FROM tutorials"
        ).fetchall()
    finally:
        conn.close()

    for command_id, used_for, not_for, scenario, click_steps, status, dialog_fill in rows:
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
        fill = parse_json(dialog_fill, {})
        if not isinstance(fill, dict):
            errors.append(f"{command_id}: dialog_fill must be a JSON object (not array)")
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


def check_wave_pedagogy(wave: int) -> list[str]:
    errors: list[str] = []
    inventory = {}
    if INVENTORY.exists():
        inventory = json.loads(INVENTORY.read_text(encoding="utf-8"))
    required_ids = {"imr"} if wave <= 0 else ids_through_wave(inventory, wave)
    if wave >= 5:
        required_ids = None  # all 184

    conn = sqlite3.connect(SQLITE)
    try:
        cols = {row[1] for row in conn.execute("PRAGMA table_info(tutorials)")}
        extra = [
            name
            for name in (
                "glossary",
                "dialog_fill_detail",
                "buried_signals",
                *SEVEN_PLUS,
                "skill_mission",
                "implemented_status",
                "dataset_id",
            )
            if name in cols
        ]
        select = "command_id, " + ", ".join(extra)
        rows = conn.execute(f"SELECT {select} FROM tutorials").fetchall()
        col_index = {name: i + 1 for i, name in enumerate(extra)}
        dataset_notes = {
            row[0]: row[1] or ""
            for row in conn.execute("SELECT dataset_id, notes FROM datasets")
        }
    finally:
        conn.close()

    def field(row, name, default=""):
        index = col_index.get(name)
        if index is None:
            return default
        return row[index]

    for row in rows:
        command_id = row[0]
        enforce = required_ids is None or command_id in required_ids
        if not enforce:
            continue
        glossary = parse_json(field(row, "glossary", "[]"), [])
        detail = parse_json(field(row, "dialog_fill_detail", "[]"), [])
        buried = parse_json(field(row, "buried_signals", "[]"), [])
        if not isinstance(glossary, list) or len(glossary) < 3:
            errors.append(f"{command_id}: glossary < 3")
        else:
            blob = json.dumps(glossary, ensure_ascii=False)
            if command_id == "imr" and ("UCL" not in blob or "USL" not in blob):
                errors.append(f"{command_id}: glossary must distinguish UCL/USL")
        if not isinstance(detail, list) or not detail:
            errors.append(f"{command_id}: dialog_fill_detail missing/short")
        elif command_id == "imr" and len(detail) < 9:
            errors.append(f"{command_id}: dialog_fill_detail missing/short")
        dataset_id = field(row, "dataset_id")
        if dataset_id:
            if not isinstance(buried, list) or not buried:
                errors.append(f"{command_id}: buried_signals required for dataset {dataset_id}")
            notes = dataset_notes.get(dataset_id, "")
            if "行" not in notes and not re.search(r"\d+", notes):
                errors.append(f"{dataset_id}: notes must mention buried row numbers")
        for name in SEVEN_PLUS:
            value = parse_json(field(row, name, "[]"), [])
            if not value:
                errors.append(f"{command_id}: {name} empty")
        if not str(field(row, "skill_mission")).strip():
            errors.append(f"{command_id}: skill_mission empty")

        status = field(row, "implemented_status")
        control_chart_ids = ids_through_wave(inventory, 1) if inventory else {"imr"}
        if status in ("implemented", "formula_reference") and command_id in control_chart_ids:
            glossary_blob = json.dumps(glossary, ensure_ascii=False) if glossary else ""
            misc = parse_json(field(row, "misconceptions", "[]"), [])
            misc_blob = json.dumps(misc, ensure_ascii=False) if misc else ""
            combined = glossary_blob + misc_blob
            if "UCL" not in combined or "USL" not in combined:
                errors.append(f"{command_id}: control-chart lesson must mention UCL/USL")
    return errors


def main() -> int:
    if not SQLITE.exists():
        print(f"FAIL: missing {SQLITE}")
        return 1

    wave = current_wave()
    failures: list[str] = []

    for script in (
        "tools/verify_learning_center_copy_depth.py",
        "tools/verify_learning_center_db.py",
        "tools/verify_learning_research_notes.py",
        "tools/verify_learning_dataset_mapping.py",
    ):
        code, output = run_script(script)
        if code != 0:
            failures.append(f"{script} exited {code}: {output[:300]}")
        else:
            print(f"OK: {script}")

    for err in check_tutorial_content():
        failures.append(err)
    for err in check_forbidden_phrases():
        failures.append(err)
    for err in check_wave_pedagogy(wave):
        failures.append(err)
    for err in check_banned_old_ids(wave):
        failures.append(err)
    for err in check_research_notes_hints(wave):
        failures.append(err)

    if failures:
        print("FAIL: learning_center_gate")
        for item in failures:
            print(" -", item)
        return 1

    print(
        f"PASS: learning_center_gate (db + research + mapping + content quality, wave={wave})"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
