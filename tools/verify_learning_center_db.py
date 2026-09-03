#!/usr/bin/env python3
"""Verify learning_center.sqlite aligns with command/help id universe (Wave-aware)."""
from __future__ import annotations

import json
import sqlite3
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SQLITE = ROOT / "resources/help/learning_center.sqlite"
META = ROOT / "tools/learning_data/id_metadata.json"
MAPPING = ROOT / "tools/learning_data/dataset_mapping.json"
INVENTORY = ROOT / "docs/research/_tmp_command_inventory.json"

BANNED_OLD_DATASET_IDS = {
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
}

NEW_TUTORIAL_COLUMNS = {
    "glossary",
    "dialog_fill_detail",
    "buried_signals",
    "prereq_quiz",
    "self_explain",
    "fade_levels",
    "retrieval_quiz",
    "misconceptions",
    "skill_mission",
}


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def parse_json(text: str, default):
    if text is None or str(text).strip() == "":
        return default
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        return None


def main() -> int:
    if not SQLITE.exists():
        print(f"FAIL: missing {SQLITE}")
        return 1

    meta = load_json(META)
    mapping = load_json(MAPPING)
    inventory = load_json(INVENTORY) if INVENTORY.exists() else {}
    expected_ids = {e["id"] for e in meta["entries"]}
    errors: list[str] = []

    conn = sqlite3.connect(SQLITE)
    try:
        version = conn.execute(
            "SELECT value FROM meta WHERE key='catalog_version'"
        ).fetchone()
        tutorial_ids = {row[0] for row in conn.execute("SELECT command_id FROM tutorials")}
        dataset_ids = {row[0] for row in conn.execute("SELECT dataset_id FROM datasets")}
        referenced = {
            row[0]
            for row in conn.execute(
                "SELECT dataset_id FROM tutorials WHERE dataset_id IS NOT NULL AND dataset_id != ''"
            )
        }
        pragma_cols = {
            row[1] for row in conn.execute("PRAGMA table_info(tutorials)")
        }
        tutorials = conn.execute(
            "SELECT command_id, dataset_id, dialog_fill, glossary, dialog_fill_detail, "
            "buried_signals, prereq_quiz, self_explain, fade_levels, retrieval_quiz, "
            "misconceptions, skill_mission FROM tutorials"
        ).fetchall()
    finally:
        conn.close()

    if not version or version[0] != "learning-center-v2":
        errors.append(f"catalog_version {version} != learning-center-v2")

    missing_cols = NEW_TUTORIAL_COLUMNS - pragma_cols
    if missing_cols:
        errors.append(f"tutorials missing columns: {sorted(missing_cols)}")

    if len(tutorial_ids) != 184:
        errors.append(f"tutorial count {len(tutorial_ids)} != 184")
    missing = expected_ids - tutorial_ids
    extra = tutorial_ids - expected_ids
    if missing:
        errors.append(f"missing tutorial ids: {sorted(missing)}")
    if extra:
        errors.append(f"extra tutorial ids: {sorted(extra)}")

    mapped_datasets = set(mapping["datasets"].keys())
    if dataset_ids != mapped_datasets:
        errors.append(f"dataset ids mismatch db={sorted(dataset_ids)} map={sorted(mapped_datasets)}")
    dangling = referenced - dataset_ids
    if dangling:
        errors.append(f"tutorials reference unknown datasets: {sorted(dangling)}")

    banned_hit = (dataset_ids | referenced) & BANNED_OLD_DATASET_IDS
    if banned_hit:
        errors.append(f"banned old dataset ids present: {sorted(banned_hit)}")

    demo_prefixed = [ds for ds in dataset_ids if ds.startswith("demo_")]
    if demo_prefixed:
        errors.append(f"dataset_id must not start with demo_: {demo_prefixed}")

    whitelist = {
        fam["dataset_id"]: set(fam["command_ids"])
        for fam in mapping.get("shared_families") or inventory.get("shared_families") or []
    }
    by_dataset: dict[str, set[str]] = {}
    gold = None
    spike_mapped = False
    for row in tutorials:
        (
            command_id, dataset_id, dialog_fill, glossary, detail, buried,
            prereq, self_explain, fade, retrieval, misconceptions, skill_mission,
        ) = row
        fill = parse_json(dialog_fill, {})
        if not isinstance(fill, dict):
            errors.append(f"{command_id}: dialog_fill is not an object")
            fill = {}
        for key, value in fill.items():
            if value == "":
                errors.append(f"{command_id}: dialog_fill empty string for {key}")
        if dataset_id:
            by_dataset.setdefault(dataset_id, set()).add(command_id)
            if dataset_id == "imr_spi_spike_b":
                spike_mapped = True
        if command_id == "imr":
            gold = {
                "dataset_id": dataset_id,
                "fill": fill,
                "glossary": parse_json(glossary, []),
                "detail": parse_json(detail, []),
                "buried": parse_json(buried, []),
                "prereq": parse_json(prereq, []),
                "self_explain": parse_json(self_explain, []),
                "fade": parse_json(fade, []),
                "retrieval": parse_json(retrieval, []),
                "misconceptions": parse_json(misconceptions, []),
                "skill_mission": skill_mission or "",
            }

    for ds_id, cmds in by_dataset.items():
        if len(cmds) < 2:
            continue
        allowed = whitelist.get(ds_id)
        if allowed is None:
            errors.append(f"{ds_id} referenced by {sorted(cmds)} but not on shared-family whitelist")
        elif cmds != allowed:
            errors.append(f"{ds_id} family commands {sorted(cmds)} != whitelist {sorted(allowed)}")

    if "imr_spi_spike_b" not in dataset_ids:
        errors.append("practice dataset imr_spi_spike_b missing")
    if spike_mapped:
        errors.append("imr_spi_spike_b must not be any tutorial's primary dataset_id")

    if gold is None:
        errors.append("missing imr tutorial")
    else:
        if gold["dataset_id"] != "imr_spi_shift":
            errors.append(f"imr.dataset_id={gold['dataset_id']} != imr_spi_shift")
        if gold["fill"] != {"variables": "锡膏高度_um"}:
            errors.append(f"imr.dialog_fill={gold['fill']} != {{variables: 锡膏高度_um}}")
        glossary = gold["glossary"] if isinstance(gold["glossary"], list) else []
        blob = json.dumps(glossary, ensure_ascii=False)
        if len(glossary) < 3:
            errors.append(f"imr glossary {len(glossary)} < 3")
        if "UCL" not in blob or "USL" not in blob:
            errors.append("imr glossary must mention UCL and USL")
        buried = gold["buried"] if isinstance(gold["buried"], list) else []
        rows = {item.get("row") for item in buried if isinstance(item, dict)}
        if 41 not in rows or 55 not in rows:
            errors.append(f"imr buried_signals rows={sorted(rows)} missing 41/55")
        detail = gold["detail"] if isinstance(gold["detail"], list) else []
        if len(detail) < 9:
            errors.append(f"imr dialog_fill_detail {len(detail)} < 9")
        for name, value in (
            ("prereq_quiz", gold["prereq"]),
            ("self_explain", gold["self_explain"]),
            ("fade_levels", gold["fade"]),
            ("retrieval_quiz", gold["retrieval"]),
            ("misconceptions", gold["misconceptions"]),
        ):
            if not value:
                errors.append(f"imr {name} empty")
        if not str(gold["skill_mission"]).strip():
            errors.append("imr skill_mission empty")

    if errors:
        print("FAIL")
        for err in errors:
            print(" -", err)
        return 1

    print(
        f"PASS: 184 tutorials, {len(dataset_ids)} datasets, catalog learning-center-v2, "
        "gold imr_spi_shift, no dangling/banned ids"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
