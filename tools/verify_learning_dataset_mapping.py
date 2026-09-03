#!/usr/bin/env python3
"""Verify dataset_mapping.json covers all union ids with the Wave lock table."""
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
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


def role_ids_for(entry: dict) -> set[str]:
    cmd = entry.get("command") or {}
    return {r["id"] for r in (cmd.get("roles") or [])}


def main() -> int:
    meta = json.loads(META.read_text(encoding="utf-8"))
    mapping = json.loads(MAPPING.read_text(encoding="utf-8"))
    inventory = json.loads(INVENTORY.read_text(encoding="utf-8")) if INVENTORY.exists() else {}
    required = {e["id"] for e in meta["entries"]}
    meta_by_id = {e["id"]: e for e in meta["entries"]}
    mapped = {m["command_id"] for m in mapping["mappings"]}
    datasets = set(mapping.get("datasets") or {})
    errors: list[str] = []

    missing = required - mapped
    extra = mapped - required
    if missing:
        errors.append(f"MISSING: {sorted(missing)}")
    if extra:
        errors.append(f"EXTRA: {sorted(extra)}")

    for m in mapping["mappings"]:
        ds = m.get("dataset_id")
        if ds and ds not in datasets:
            errors.append(f"BAD DATASET: {m['command_id']} -> {ds}")
        if ds and ds.startswith("demo_"):
            errors.append(f"dataset_id starts with demo_: {ds}")
        if ds and ds in BANNED_OLD_DATASET_IDS:
            errors.append(f"banned dataset on {m['command_id']}: {ds}")
        if ds:
            expected_ws = f"demo_{ds}"
            if m.get("import_worksheet_name") != expected_ws:
                errors.append(
                    f"{m['command_id']} import_worksheet_name={m.get('import_worksheet_name')} "
                    f"!= {expected_ws}"
                )
            entry = meta_by_id.get(m["command_id"], {})
            allowed_roles = role_ids_for(entry)
            for key in (m.get("role_map") or {}):
                if allowed_roles and key not in allowed_roles:
                    errors.append(f"{m['command_id']} role_map key {key} not in command roles")
            for value in (m.get("role_map") or {}).values():
                if value == "":
                    errors.append(f"{m['command_id']} role_map empty string")

    banned_in_datasets = datasets & BANNED_OLD_DATASET_IDS
    if banned_in_datasets:
        errors.append(f"banned datasets listed: {sorted(banned_in_datasets)}")

    expected_families = {
        (fam["dataset_id"], tuple(sorted(fam["command_ids"])))
        for fam in inventory.get("shared_families") or []
    }
    actual_families = {
        (fam["dataset_id"], tuple(sorted(fam["command_ids"])))
        for fam in mapping.get("shared_families") or []
    }
    if expected_families and actual_families != expected_families:
        errors.append("shared_families mismatch vs wave lock table")

    if "imr" in mapped:
        imr = next(m for m in mapping["mappings"] if m["command_id"] == "imr")
        if imr.get("dataset_id") != "imr_spi_shift":
            errors.append(f"imr dataset_id={imr.get('dataset_id')} != imr_spi_shift")
        if (imr.get("role_map") or {}) != {"variables": "锡膏高度_um"}:
            errors.append(f"imr role_map={imr.get('role_map')}")

    if "imr_spi_spike_b" not in datasets:
        errors.append("practice dataset imr_spi_spike_b missing from mapping.datasets")

    ok = not errors
    print(f"VERIFY: {'PASS' if ok else 'FAIL'}")
    for err in errors:
        print(" -", err)
    print(f"Mappings: {len(mapped)}/{len(required)}; datasets: {len(datasets)}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
