#!/usr/bin/env python3
"""Verify dataset_mapping.json covers all union ids with valid dataset refs."""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    meta = json.loads((ROOT / "tools/learning_data/id_metadata.json").read_text(encoding="utf-8"))
    mapping = json.loads((ROOT / "tools/learning_data/dataset_mapping.json").read_text(encoding="utf-8"))
    required = {e["id"] for e in meta["entries"]}
    mapped = {m["command_id"] for m in mapping["mappings"]}
    datasets = set(mapping["datasets"])
    missing = required - mapped
    extra = mapped - required
    bad_ds = []
    for m in mapping["mappings"]:
        ds = m.get("dataset_id")
        if ds and ds not in datasets:
            bad_ds.append((m["command_id"], ds))
    ok = not missing and not extra and not bad_ds
    print(f"VERIFY: {'PASS' if ok else 'FAIL'}")
    if missing:
        print("MISSING:", sorted(missing))
    if extra:
        print("EXTRA:", sorted(extra))
    if bad_ds:
        print("BAD DATASET:", bad_ds[:10])
    print(f"Mappings: {len(mapped)}/{len(required)}; datasets: {len(datasets)}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
