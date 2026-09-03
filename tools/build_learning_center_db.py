#!/usr/bin/env python3
"""Build resources/help/learning_center.sqlite from mapping + research data."""
from __future__ import annotations

import csv
import json
import math
import random
import sqlite3
import struct
from datetime import date, datetime, timedelta
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_SQLITE = ROOT / "resources/help/learning_center.sqlite"
CSV_DIR = ROOT / "tools/learning_data/csv"
META_VERSION = "learning-center-v1"


def gen_smt_paste_height(rng: random.Random) -> list[list[str]]:
    rows = []
    t0 = datetime(2026, 1, 6, 8, 0, 0)
    for i in range(80):
        line = "A" if i % 2 == 0 else "B"
        shift = ["早班", "中班", "晚班"][i % 3]
        h = 118 + rng.gauss(0, 2.2) + (0.8 if line == "B" else 0)
        rows.append([
            f"{h:.1f}",
            line,
            shift,
            (t0 + timedelta(minutes=i * 12)).strftime("%Y-%m-%d %H:%M:%S"),
            f"ST{i % 4 + 1:02d}",
        ])
    return rows


def gen_two_line_thickness(rng: random.Random) -> list[list[str]]:
    rows = []
    for i in range(60):
        line = "A线" if i < 30 else "B线"
        base = 12.5 if line == "A线" else 12.8
        rows.append([
            f"{base + rng.gauss(0, 0.15):.3f}",
            line,
            f"B{i // 10 + 1}",
            f"2026-01-{(i % 28) + 1:02d}",
        ])
    return rows


def gen_paired_rework(rng: random.Random) -> list[list[str]]:
    rows = []
    for i in range(40):
        before = 1.2 + rng.gauss(0, 0.05)
        after = before - 0.02 + rng.gauss(0, 0.04)
        rows.append([
            f"{before:.3f}",
            f"{after:.3f}",
            f"W{i + 1001}",
            ["虚焊", "偏移", "少锡"][i % 3],
        ])
    return rows


def gen_anova_cavity(rng: random.Random) -> list[list[str]]:
    rows = []
    for cavity in ("1", "2", "3"):
        offset = {"1": 0.0, "2": 0.03, "3": -0.02}[cavity]
        for i in range(30):
            rows.append([
                f"{10.00 + offset + rng.gauss(0, 0.02):.4f}",
                cavity,
                f"M{i % 5 + 1}",
                "IM-03",
            ])
    return rows


def gen_corr_temp_offset(rng: random.Random) -> list[list[str]]:
    rows = []
    for i in range(55):
        temp = 235 + rng.uniform(-3, 3)
        offset = 0.15 * (temp - 235) + rng.gauss(0, 1.2)
        rows.append([
            f"{temp:.1f}",
            f"{offset:.2f}",
            f"{245 + rng.uniform(-2, 2):.1f}",
            ["X1", "X2"][i % 2],
            f"T{i % 3 + 1}",
        ])
    return rows


def gen_attribute_defect(rng: random.Random) -> list[list[str]]:
    rows = []
    types = ["虚焊", "偏移", "少锡", "立碑"]
    for shift in ("早班", "中班", "晚班"):
        for week in range(4):
            trials = rng.randint(180, 220)
            bad = max(1, int(trials * (0.02 + (0.01 if shift == "晚班" else 0))))
            rows.append([
                shift,
                str(trials),
                str(bad),
                str(bad + rng.randint(0, 3)),
                types[week % len(types)],
                f"{rng.uniform(8, 12):.1f}",
                "L1",
            ])
    return rows


def gen_gage_rr_balance(rng: random.Random) -> list[list[str]]:
    rows = []
    for part in range(1, 11):
        true = 25.0 + part * 0.05
        for op in ("张三", "李四", "王五"):
            bias = {"张三": 0.01, "李四": -0.005, "王五": 0.0}[op]
            for rep in (1, 2, 3):
                rows.append([
                    f"P{part:02d}",
                    op,
                    str(rep),
                    f"{true + bias + rng.gauss(0, 0.008):.4f}",
                ])
    return rows


def gen_doe_factorial_demo(rng: random.Random) -> list[list[str]]:
    rows = []
    levels = {"温度_℃": (230, 245), "链速_mm_min": (80, 95), "氮气流量_L_min": (15, 22)}
    run = 1
    for t in levels["温度_℃"]:
        for s in levels["链速_mm_min"]:
            for n in levels["氮气流量_L_min"]:
                strength = 45 + 0.05 * (t - 230) - 0.1 * (s - 80) + rng.gauss(0, 0.5)
                rows.append([
                    str(t), str(s), str(n),
                    f"{strength:.2f}",
                    f"{max(0, 2.5 - strength * 0.03 + rng.gauss(0, 0.2)):.2f}",
                    str(run),
                    "40", "35", "25",
                ])
                run += 1
    return rows


def gen_reliability_cycles(rng: random.Random) -> list[list[str]]:
    rows = []
    for i in range(45):
        stress = rng.choice([220, 240, 260])
        cycles = int(8000 + rng.expovariate(1 / 3000) + (260 - stress) * 10)
        failed = 1 if cycles < 12000 else 0
        rows.append([
            str(cycles),
            str(failed),
            str(stress),
            f"U{i + 1:03d}",
            ["开路", "短路", "—"][failed if failed else 2],
            f"{cycles / 100:.1f}",
        ])
    return rows


def gen_ts_weekly_yield(rng: random.Random) -> list[list[str]]:
    rows = []
    for w in range(1, 53):
        seasonal = 2 * math.sin(2 * math.pi * w / 52)
        trend = -0.02 * w
        y = 96.5 + seasonal + trend + rng.gauss(0, 0.4)
        rows.append([str(w), f"{y:.2f}", str(int(8000 + rng.randint(-200, 200))), "2026"])
    return rows


GENERATORS = {
    "smt_paste_height": gen_smt_paste_height,
    "two_line_thickness": gen_two_line_thickness,
    "paired_rework": gen_paired_rework,
    "anova_cavity": gen_anova_cavity,
    "corr_temp_offset": gen_corr_temp_offset,
    "attribute_defect": gen_attribute_defect,
    "gage_rr_balance": gen_gage_rr_balance,
    "doe_factorial_demo": gen_doe_factorial_demo,
    "reliability_cycles": gen_reliability_cycles,
    "ts_weekly_yield": gen_ts_weekly_yield,
}


def write_csv(ds_id: str, columns: list[dict], rows: list[list[str]]) -> None:
    CSV_DIR.mkdir(parents=True, exist_ok=True)
    path = CSV_DIR / f"{ds_id}.csv"
    names = [c["name"] for c in columns]
    with path.open("w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(names)
        w.writerows(rows)


def build_tutorial_row(m: dict, research: dict, meta_entry: dict) -> dict:
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

    return {
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
        "dialog_fill": m.get("role_map", {}),
        "output_guide": output_guide,
        "common_mistakes": mistakes,
        "related_ids": [],
        "research_sources": sources,
    }


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
          FOREIGN KEY (dataset_id) REFERENCES datasets(dataset_id));
        CREATE INDEX idx_cells_dataset ON dataset_cells(dataset_id);
        CREATE INDEX idx_tutorials_category ON tutorials(category);
        """
    )
    c.execute("INSERT INTO meta VALUES (?,?)", ("catalog_version", META_VERSION))
    c.execute("INSERT INTO meta VALUES (?,?)", ("generated_at", date.today().isoformat()))
    c.execute("INSERT INTO meta VALUES (?,?)", ("source_git", "learning-center-agent-c"))

    meta_by_id = {e["id"]: e for e in meta["entries"]}
    rng = random.Random(42)

    for ds_id, ds in mapping_doc["datasets"].items():
        cols = ds["columns"]
        gen = GENERATORS[ds_id]
        rows = gen(rng)
        write_csv(ds_id, cols, rows)
        c.execute(
            "INSERT INTO datasets VALUES (?,?,?,?,?,?)",
            (ds_id, ds["title"], ds["industry"], ds["story"], len(rows), ""),
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
        t = build_tutorial_row(m, research, meta_by_id.get(m["command_id"], {}))
        c.execute(
            """INSERT INTO tutorials VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)""",
            (
                t["command_id"], t["title"], t["category"], t["menu_path"],
                t["implemented_status"], t["used_for"], t["not_for"], t["scenario"],
                t["dataset_id"],
                json.dumps(t["click_steps"], ensure_ascii=False),
                json.dumps(t["dialog_fill"], ensure_ascii=False),
                json.dumps(t["output_guide"], ensure_ascii=False),
                json.dumps(t["common_mistakes"], ensure_ascii=False),
                json.dumps(t["related_ids"], ensure_ascii=False),
                json.dumps(t["research_sources"], ensure_ascii=False),
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
    print(f"Wrote {OUT_SQLITE} ({OUT_SQLITE.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
