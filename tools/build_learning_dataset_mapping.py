#!/usr/bin/env python3
"""Write tools/learning_data/dataset_mapping.json from the Wave lock table.

Wave-0: only `imr` gets a dataset; practice table `imr_spi_spike_b` is listed
but not mapped as any tutorial's primary dataset_id. Later waves fill more
ids via LEARNING_CENTER_WAVE (default 0).
Does not rewrite docs/research/learning-center-dataset-mapping.md (Wave-5).
"""
from __future__ import annotations

import json
import os
import sys
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "learning_data"))
from wave1_content import ROLE_MAP_BY_DATASET as WAVE1_ROLE_MAPS  # noqa: E402
from wave1_content import WAVE1_DATASETS  # noqa: E402
from wave2_content import ROLE_MAP_BY_DATASET as WAVE2_ROLE_MAPS  # noqa: E402
from wave2_content import WAVE2_DATASETS  # noqa: E402
from wave3_content import ROLE_MAP_BY_DATASET as WAVE3_ROLE_MAPS  # noqa: E402
from wave3_content import WAVE3_DATASETS  # noqa: E402
from wave4_content import ROLE_MAP_BY_COMMAND as WAVE4_ROLE_BY_CMD  # noqa: E402
from wave4_content import ROLE_MAP_BY_DATASET as WAVE4_ROLE_MAPS  # noqa: E402
from wave4_content import WAVE4_DATASETS  # noqa: E402

INVENTORY = ROOT / "docs/research/_tmp_command_inventory.json"
META = ROOT / "tools/learning_data/id_metadata.json"
OUT_JSON = ROOT / "tools/learning_data/dataset_mapping.json"

GOLD_DATASETS = {
    "imr_spi_shift": {
        "wave": 0,
        "practice_only": False,
        "title": "SMT 钢网更换后锡膏高度（阶跃+尖峰）",
        "industry": "electronics",
        "story": "单条 SMT 线 SPI 高度按片序记录。前段基线，片41起钢网更换后均值上移，片55尖峰。仅服务 I-MR 课，不用于能力分析。",
        "row_count": 60,
        "notes": "埋点：片41（行41）均值阶跃，基线约120μm→更换后约124μm，期望 I 图后段上移；片55（行55）尖峰约132μm，期望相对近期波动不寻常或越 UCL。MR 图在尖峰处应变大。禁止用本失控教学集算 Cpk。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–60 生产顺序"},
            {"index": 1, "name": "锡膏高度_um", "role_hint": "measurement", "unit": "μm",
             "description": "SPI 高度 Y"},
            {"index": 2, "name": "时段备注", "role_hint": "note",
             "description": "基线 / 钢网更换后 / 尖峰；不进对话框"},
        ],
    },
    "imr_spi_spike_b": {
        "wave": 0,
        "practice_only": True,
        "title": "SMT 锡膏高度独立练习（尖峰行号不同）",
        "industry": "electronics",
        "story": "与金标同构的练习表：片41仍有均值阶跃，尖峰改在片48。供 fade level 2 独立练，不作为任何 tutorial 的主 dataset_id。",
        "row_count": 60,
        "notes": "练习表埋点：片41（行41）均值阶跃；尖峰在片48（行48），不是金标的片55。导入工作表 demo_imr_spi_spike_b。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–60 生产顺序"},
            {"index": 1, "name": "锡膏高度_um", "role_hint": "measurement", "unit": "μm",
             "description": "SPI 高度 Y"},
            {"index": 2, "name": "时段备注", "role_hint": "note",
             "description": "基线 / 钢网更换后 / 尖峰；不进对话框"},
        ],
    },
}

ROLE_MAP_BY_DATASET = {
    "imr_spi_shift": {"variables": "锡膏高度_um"},
    **WAVE1_ROLE_MAPS,
    **WAVE2_ROLE_MAPS,
    **WAVE3_ROLE_MAPS,
    **WAVE4_ROLE_MAPS,
}

ROLE_MAP_BY_COMMAND = {
    **WAVE4_ROLE_BY_CMD,
}

# Merge Wave-1/2/3/4 dataset specs into the release pool (keyed by wave).
GOLD_DATASETS.update(WAVE1_DATASETS)
GOLD_DATASETS.update(WAVE2_DATASETS)
GOLD_DATASETS.update(WAVE3_DATASETS)
GOLD_DATASETS.update(WAVE4_DATASETS)


def current_wave() -> int:
    raw = os.environ.get("LEARNING_CENTER_WAVE", "0").strip()
    try:
        return int(raw)
    except ValueError:
        return 0


def released_datasets(wave: int) -> dict[str, dict]:
    out: dict[str, dict] = {}
    for ds_id, spec in GOLD_DATASETS.items():
        if spec["wave"] <= wave:
            item = {k: v for k, v in spec.items() if k not in {"wave", "practice_only"}}
            item["serves_commands"] = []
            out[ds_id] = item
    return out


def build_role_map(entry: dict, dataset_id: str | None) -> dict[str, str]:
    if not dataset_id:
        return {}
    cid = entry.get("id") or ""
    if cid in ROLE_MAP_BY_COMMAND:
        return dict(ROLE_MAP_BY_COMMAND[cid])
    preset = ROLE_MAP_BY_DATASET.get(dataset_id)
    if preset is not None:
        return dict(preset)
    cmd = entry.get("command") or {}
    roles = cmd.get("roles") or []
    # Wave-0 has no generic pool; later waves should add ROLE_MAP_BY_DATASET.
    if len(roles) == 1:
        return {}
    return {}


def main() -> None:
    wave = current_wave()
    inventory = json.loads(INVENTORY.read_text(encoding="utf-8"))
    meta = json.loads(META.read_text(encoding="utf-8"))
    meta_by_id = {e["id"]: e for e in meta["entries"]}
    lock_by_id = {row["command_id"]: row for row in inventory["lock_rows"]}
    datasets = released_datasets(wave)
    mappings = []
    for e in meta["entries"]:
        cid = e["id"]
        lock = lock_by_id.get(cid, {})
        planned = lock.get("dataset_id")
        released = bool(planned) and lock.get("wave", 99) <= wave and planned in datasets
        dataset_id = planned if released else None
        role_map = build_role_map(e, dataset_id)
        cmd = e.get("command") or {}
        help_info = e.get("help") or {}
        if dataset_id:
            reason = None
            worksheet = f"demo_{dataset_id}"
            datasets[dataset_id].setdefault("serves_commands", []).append(cid)
        elif planned:
            reason = f"后续 Wave-{lock.get('wave')} 将分配专用集 `{planned}`"
            worksheet = None
        else:
            reason = "无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表"
            worksheet = None
        mappings.append({
            "command_id": cid,
            "title": help_info.get("title") or cmd.get("menu_label") or cid,
            "menu_path": cmd.get("menu_path") or help_info.get("menu_path", ""),
            "implemented_status": help_info.get("implemented_status", "command_only"),
            "requires_data": cmd.get("requires_data", True) if cmd else False,
            "dataset_id": dataset_id,
            "role_map": role_map,
            "import_worksheet_name": worksheet,
            "no_import_reason": reason,
        })

    for spec in datasets.values():
        spec["serves_commands"] = sorted(spec.get("serves_commands") or [])

    out_json = {
        "generated_at": date.today().isoformat(),
        "catalog_version": "learning-center-mapping-v2",
        "learning_center_wave": wave,
        "shared_families": inventory.get("shared_families", []),
        "datasets": datasets,
        "mappings": mappings,
    }
    OUT_JSON.write_text(json.dumps(out_json, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    with_ds = sum(1 for m in mappings if m["dataset_id"])
    print(
        f"Wrote {OUT_JSON} ({len(mappings)} mappings, {len(datasets)} datasets, "
        f"{with_ds} with dataset, wave={wave})"
    )


if __name__ == "__main__":
    main()
