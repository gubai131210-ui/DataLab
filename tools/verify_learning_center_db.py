#!/usr/bin/env python3
"""Verify learning_center.sqlite aligns with command/help id universe."""
from __future__ import annotations

import json
import sqlite3
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SQLITE = ROOT / "resources/help/learning_center.sqlite"
META = ROOT / "tools/learning_data/id_metadata.json"
MAPPING = ROOT / "tools/learning_data/dataset_mapping.json"


def main() -> int:
    if not SQLITE.exists():
        print(f"FAIL: missing {SQLITE}")
        return 1

    meta = json.loads(META.read_text(encoding="utf-8"))
    mapping = json.loads(MAPPING.read_text(encoding="utf-8"))
    expected_ids = {e["id"] for e in meta["entries"]}

    conn = sqlite3.connect(SQLITE)
    try:
        tutorial_ids = {row[0] for row in conn.execute("SELECT command_id FROM tutorials")}
        dataset_ids = {row[0] for row in conn.execute("SELECT dataset_id FROM datasets")}
        referenced = {
            row[0]
            for row in conn.execute(
                "SELECT dataset_id FROM tutorials WHERE dataset_id IS NOT NULL AND dataset_id != ''"
            )
        }
    finally:
        conn.close()

    errors: list[str] = []
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

    if errors:
        print("FAIL")
        for err in errors:
            print(" -", err)
        return 1

    print("PASS: 184 tutorials, 10 datasets, no dangling dataset refs")
    return 0


if __name__ == "__main__":
    sys.exit(main())
