#!/usr/bin/env python3
"""Build resources/help/learning_center.sqlite from mapping + research + overlays."""
from __future__ import annotations

import csv
import json
import random
import sqlite3
import sys
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "learning_data"))
from wave1_content import GENERATORS as WAVE1_GENERATORS  # noqa: E402
from wave2_content import GENERATORS as WAVE2_GENERATORS  # noqa: E402
from wave3_content import GENERATORS as WAVE3_GENERATORS  # noqa: E402
from wave4_content import GENERATORS as WAVE4_GENERATORS  # noqa: E402
OUT_SQLITE = ROOT / "resources/help/learning_center.sqlite"
CSV_DIR = ROOT / "tools/learning_data/csv"
OVERLAY_DIR = ROOT / "tools/learning_data/tutorial_overlays"
META_VERSION = "learning-center-v2"

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

def _gen_imr_spi(rng: random.Random, spike_row: int, n: int = 60, shift_row: int = 41) -> list[list[str]]:
    """1-based 片号; baseline ~120, shift at shift_row ~124, spike at spike_row ~132."""
    rows: list[list[str]] = []
    for piece in range(1, n + 1):
        if piece == spike_row:
            height = 132.4
            note = "尖峰"
        elif piece < shift_row:
            height = 120.0 + rng.gauss(0, 0.55)
            note = "基线"
        elif piece == shift_row:
            height = 123.9
            note = "钢网更换后（均值阶跃开始）"
        else:
            height = 124.0 + rng.gauss(0, 0.55)
            note = "钢网更换后"
        rows.append([str(piece), f"{height:.1f}", note])
    return rows


def gen_imr_spi_shift(_rng: random.Random) -> list[list[str]]:
    return _gen_imr_spi(random.Random(42), spike_row=55)


def gen_imr_spi_spike_b(_rng: random.Random) -> list[list[str]]:
    return _gen_imr_spi(random.Random(43), spike_row=48)


GENERATORS = {
    "imr_spi_shift": gen_imr_spi_shift,
    "imr_spi_spike_b": gen_imr_spi_spike_b,
    **WAVE1_GENERATORS,
    **WAVE2_GENERATORS,
    **WAVE3_GENERATORS,
    **WAVE4_GENERATORS,
}


def write_csv(ds_id: str, columns: list[dict], rows: list[list[str]]) -> None:
    CSV_DIR.mkdir(parents=True, exist_ok=True)
    path = CSV_DIR / f"{ds_id}.csv"
    names = [c["name"] for c in columns]
    with path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(names)
        writer.writerows(rows)


def cleanup_csv_dir() -> None:
    CSV_DIR.mkdir(parents=True, exist_ok=True)
    for path in CSV_DIR.glob("*.csv"):
        if path.stem in BANNED_OLD_DATASET_IDS or path.stem not in GENERATORS:
            path.unlink()


def load_overlay(command_id: str) -> dict:
    path = OVERLAY_DIR / f"{command_id}.json"
    if not path.is_file():
        return {}
    return json.loads(path.read_text(encoding="utf-8"))


def dumps_json(value) -> str:
    return json.dumps(value, ensure_ascii=False)


def empty_pedagogy() -> dict:
    return {
        "glossary": [],
        "dialog_fill_detail": [],
        "buried_signals": [],
        "prereq_quiz": [],
        "self_explain": [],
        "fade_levels": [],
        "retrieval_quiz": [],
        "misconceptions": [],
        "skill_mission": "",
    }


def build_tutorial_row(m: dict, research: dict, meta_entry: dict, overlay: dict) -> dict:
    rid = m["command_id"]
    r = research.get(rid, {})
    cmd = meta_entry.get("command") or {}
    help_info = meta_entry.get("help") or {}
    mp = cmd.get("menu_path") or help_info.get("menu_path", "")
    label = m.get("title") or cmd.get("menu_label") or rid
    steps = ["在主窗口打开「帮助」→「学习中心」，选择本教程。"]
    if mp:
        steps.append(f"菜单路径：{mp} → {label}")
    if m.get("dataset_id"):
        steps.append(f"点击「导入测试数据到工作区」，得到工作表 `{m.get('import_worksheet_name')}`。")
        steps.append("按对话框角色将列拖入对应角色（见下方映射）。")
    else:
        steps.append("本命令无需导入数据，直接打开对应菜单。")
    steps.append("运行分析后，对照输出解读理解各表/图含义。")
    if help_info.get("implemented_status") == "formula_reference":
        steps.insert(1, "注意：当前版本可能无此菜单项，公式见「算法、公式与参考资料」。")

    mistakes = r.get("common_mistakes", [])
    if isinstance(mistakes, str):
        mistakes = [mistakes]
    output_guide = []
    out_desc = help_info.get("output_description", "")
    if out_desc:
        for part in out_desc.replace("。", "；").split("；"):
            part = part.strip()
            if part:
                output_guide.append({"name": part, "meaning": part})
    if not output_guide:
        output_guide = [{"name": "输出页", "meaning": "见 AnalysisService 实际产出表/图，勿过度解读为过程合格。"}]

    sources = r.get("sources", [])
    if sources and isinstance(sources[0], str):
        sources = []

    row = {
        "command_id": rid,
        "title": label,
        "category": (help_info.get("category") or mp or "其他").split(">")[0].strip(),
        "menu_path": mp or help_info.get("menu_path", ""),
        "implemented_status": help_info.get("implemented_status", "implemented" if cmd else "orchestration"),
        "used_for": r.get("used_for", ""),
        "not_for": r.get("not_for", ""),
        "scenario": r.get("manufacturing_scenario", ""),
        "dataset_id": m.get("dataset_id"),
        "click_steps": steps,
        "dialog_fill": m.get("role_map") or {},
        "output_guide": output_guide,
        "common_mistakes": mistakes,
        "related_ids": [],
        "research_sources": sources,
    }
    row.update(empty_pedagogy())
    for key, value in overlay.items():
        row[key] = value
    fill = row.get("dialog_fill") or {}
    if not isinstance(fill, dict):
        raise TypeError(f"{rid}: dialog_fill overlay must be an object, not {type(fill).__name__}")
    row["dialog_fill"] = {k: v for k, v in fill.items() if v not in (None, "")}
    return row


def create_db(conn: sqlite3.Connection, mapping_doc: dict, research: dict, meta: dict) -> None:
    c = conn.cursor()
    c.executescript(
        """
        CREATE TABLE meta (key TEXT PRIMARY KEY, value TEXT NOT NULL);
        CREATE TABLE datasets (
          dataset_id TEXT PRIMARY KEY, title TEXT NOT NULL, industry TEXT NOT NULL,
          story TEXT NOT NULL, row_count INTEGER NOT NULL, notes TEXT);
        CREATE TABLE dataset_columns (
          dataset_id TEXT NOT NULL, column_index INTEGER NOT NULL,
          name TEXT NOT NULL, role_hint TEXT NOT NULL, unit TEXT, description TEXT,
          PRIMARY KEY (dataset_id, column_index));
        CREATE TABLE dataset_cells (
          dataset_id TEXT NOT NULL, row_index INTEGER NOT NULL, column_index INTEGER NOT NULL,
          value TEXT NOT NULL, PRIMARY KEY (dataset_id, row_index, column_index));
        CREATE TABLE tutorials (
          command_id TEXT PRIMARY KEY, title TEXT NOT NULL, category TEXT NOT NULL,
          menu_path TEXT NOT NULL, implemented_status TEXT NOT NULL,
          used_for TEXT NOT NULL, not_for TEXT NOT NULL, scenario TEXT NOT NULL,
          dataset_id TEXT,
          click_steps TEXT NOT NULL, dialog_fill TEXT NOT NULL,
          output_guide TEXT NOT NULL, common_mistakes TEXT NOT NULL,
          related_ids TEXT NOT NULL, research_sources TEXT NOT NULL,
          glossary TEXT NOT NULL,
          dialog_fill_detail TEXT NOT NULL,
          buried_signals TEXT NOT NULL,
          prereq_quiz TEXT NOT NULL,
          self_explain TEXT NOT NULL,
          fade_levels TEXT NOT NULL,
          retrieval_quiz TEXT NOT NULL,
          misconceptions TEXT NOT NULL,
          skill_mission TEXT NOT NULL,
          FOREIGN KEY (dataset_id) REFERENCES datasets(dataset_id));
        CREATE INDEX idx_cells_dataset ON dataset_cells(dataset_id);
        CREATE INDEX idx_tutorials_category ON tutorials(category);
        """
    )
    c.execute("INSERT INTO meta VALUES (?,?)", ("catalog_version", META_VERSION))
    c.execute("INSERT INTO meta VALUES (?,?)", ("generated_at", date.today().isoformat()))
    c.execute("INSERT INTO meta VALUES (?,?)", ("source_git", "learning-center-v2-wave4"))

    meta_by_id = {e["id"]: e for e in meta["entries"]}
    rng = random.Random(42)
    cleanup_csv_dir()

    for ds_id, ds in mapping_doc["datasets"].items():
        if ds_id in BANNED_OLD_DATASET_IDS:
            raise RuntimeError(f"banned old dataset id in mapping: {ds_id}")
        if ds_id not in GENERATORS:
            raise RuntimeError(f"no generator for dataset_id={ds_id}")
        cols = ds["columns"]
        rows = GENERATORS[ds_id](rng)
        write_csv(ds_id, cols, rows)
        c.execute(
            "INSERT INTO datasets VALUES (?,?,?,?,?,?)",
            (ds_id, ds["title"], ds["industry"], ds["story"], len(rows), ds.get("notes") or ""),
        )
        for col in cols:
            c.execute(
                "INSERT INTO dataset_columns VALUES (?,?,?,?,?,?)",
                (ds_id, col["index"], col["name"], col["role_hint"],
                 col.get("unit"), col.get("description")),
            )
        for ri, row in enumerate(rows):
            for ci, val in enumerate(row):
                c.execute(
                    "INSERT INTO dataset_cells VALUES (?,?,?,?)",
                    (ds_id, ri, ci, val),
                )

    for m in mapping_doc["mappings"]:
        overlay = load_overlay(m["command_id"])
        t = build_tutorial_row(m, research, meta_by_id.get(m["command_id"], {}), overlay)
        dataset_id = t["dataset_id"] or None
        if dataset_id:
            if dataset_id in BANNED_OLD_DATASET_IDS:
                raise RuntimeError(f"banned dataset on tutorial {t['command_id']}: {dataset_id}")
            if str(dataset_id).startswith("demo_"):
                raise RuntimeError(f"dataset_id must not start with demo_: {dataset_id}")
        c.execute(
            """INSERT INTO tutorials (
                command_id, title, category, menu_path, implemented_status,
                used_for, not_for, scenario, dataset_id,
                click_steps, dialog_fill, output_guide, common_mistakes,
                related_ids, research_sources,
                glossary, dialog_fill_detail, buried_signals,
                prereq_quiz, self_explain, fade_levels, retrieval_quiz,
                misconceptions, skill_mission
            ) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)""",
            (
                t["command_id"], t["title"], t["category"], t["menu_path"],
                t["implemented_status"], t["used_for"], t["not_for"], t["scenario"],
                dataset_id,
                dumps_json(t["click_steps"]),
                dumps_json(t["dialog_fill"]),
                dumps_json(t["output_guide"]),
                dumps_json(t["common_mistakes"]),
                dumps_json(t["related_ids"]),
                dumps_json(t["research_sources"]),
                dumps_json(t["glossary"]),
                dumps_json(t["dialog_fill_detail"]),
                dumps_json(t["buried_signals"]),
                dumps_json(t["prereq_quiz"]),
                dumps_json(t["self_explain"]),
                dumps_json(t["fade_levels"]),
                dumps_json(t["retrieval_quiz"]),
                dumps_json(t["misconceptions"]),
                t.get("skill_mission") or "",
            ),
        )
    conn.commit()


def main() -> None:
    mapping = json.loads((ROOT / "tools/learning_data/dataset_mapping.json").read_text(encoding="utf-8"))
    research = json.loads((ROOT / "tools/learning_data/research_by_id.json").read_text(encoding="utf-8"))
    meta = json.loads((ROOT / "tools/learning_data/id_metadata.json").read_text(encoding="utf-8"))
    OUT_SQLITE.parent.mkdir(parents=True, exist_ok=True)
    if OUT_SQLITE.exists():
        OUT_SQLITE.unlink()
    conn = sqlite3.connect(OUT_SQLITE)
    try:
        create_db(conn, mapping, research, meta)
    finally:
        conn.close()
    print(f"Wrote {OUT_SQLITE} ({OUT_SQLITE.stat().st_size} bytes) catalog={META_VERSION}")


if __name__ == "__main__":
    main()
