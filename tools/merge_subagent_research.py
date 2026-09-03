#!/usr/bin/env python3
"""Merge subagent research JSON from agent transcripts into research_by_id.json."""
from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TRANSCRIPT_DIR = Path(
    r"C:\Users\孤白赟悫\.cursor\projects\d-QT-CppPrograms-DataLab\agent-transcripts"
)

SUBAGENT_FILES = [
    "b1a0a3c8-14c3-4f50-a88c-4445efe78b2c/subagents/c82338e5-8151-425c-a5c3-fca8f9448807.jsonl",
    "b1a0a3c8-14c3-4f50-a88c-4445efe78b2c/subagents/9d1f0cb0-c1ea-4515-ad72-a97459c344ee.jsonl",
    "b1a0a3c8-14c3-4f50-a88c-4445efe78b2c/subagents/8637fdeb-0e9c-4173-a6b3-2346c0513102.jsonl",
    "b1a0a3c8-14c3-4f50-a88c-4445efe78b2c/subagents/1d6976b0-8c06-428c-8d61-7293952e9edd.jsonl",
]

OVERLAY_JSON_FILES = [
    ROOT / "docs/research/learning-center-research-agent-a.json",
]


def extract_json_blob(text: str) -> dict | None:
    text = text.strip()
    # strip markdown fences
    if text.startswith("```"):
        text = re.sub(r"^```(?:json)?\s*", "", text)
        text = re.sub(r"\s*```$", "", text)
    start = text.find("{")
    end = text.rfind("}")
    if start < 0 or end <= start:
        return None
    try:
        return json.loads(text[start : end + 1])
    except json.JSONDecodeError:
        return None


def load_subagent_json(path: Path) -> dict:
    if not path.exists():
        print(f"WARN missing transcript: {path}")
        return {}
    merged: dict = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError:
            continue
        if row.get("role") != "assistant":
            continue
        for block in row.get("message", {}).get("content", []):
            if block.get("type") != "text":
                continue
            blob = extract_json_blob(block.get("text", ""))
            if isinstance(blob, dict) and blob:
                merged.update(blob)
    return merged


def normalize_mistakes(value) -> list[str]:
    if value is None:
        return []
    if isinstance(value, list):
        return [str(x) for x in value]
    if isinstance(value, str):
        parts = re.split(r"[；;]\s*", value)
        return [p.strip() for p in parts if p.strip()]
    return [str(value)]


def normalize_sources(value) -> list[dict]:
    out: list[dict] = []
    if not isinstance(value, list):
        return out
    for s in value:
        if isinstance(s, dict):
            label = s.get("label") or s.get("citation") or "Source"
            url = s.get("url", "")
            accessed = s.get("accessed", "2026-09-03")
            out.append({"label": label, "url": url, "accessed": accessed})
    return out


def is_mostly_english(text: str) -> bool:
    if not text:
        return False
    letters = [c for c in text if c.isalpha()]
    if not letters:
        return False
    ascii_letters = sum(1 for c in letters if c.isascii())
    return ascii_letters / len(letters) > 0.6


def normalize_entry(raw: dict, base: dict) -> dict:
    out = dict(base)
    text_fields = (
        "used_for",
        "not_for",
        "typical_sample_size",
        "manufacturing_scenario",
        "menu_path",
        "chart_note",
        "implemented_note",
        "dataset_hint",
    )
    for key in text_fields:
        if not raw.get(key):
            continue
        if key in ("used_for", "not_for", "manufacturing_scenario") and is_mostly_english(
            str(raw[key])
        ):
            # keep richer Chinese base; English subagent detail goes to sources/scenario
            if base.get(key):
                out[key] = base[key]
            else:
                out[key] = raw[key]
        else:
            out[key] = raw[key]
    if raw.get("common_mistakes") is not None:
        out["common_mistakes"] = normalize_mistakes(raw["common_mistakes"])
    if raw.get("sources"):
        out["sources"] = normalize_sources(raw["sources"])
    not_for = str(out.get("not_for", ""))
    if "当前版本菜单中可能没有此项" in not_for or "仅公式参考" in not_for:
        out["implemented_note"] = (
            "formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。"
        )
    used_for = str(out.get("used_for", ""))
    not_for_text = str(out.get("not_for", ""))
    if any(k in used_for + not_for_text for k in ("模式观察", "图形", "不替代")):
        if "chart_note" not in out:
            out["chart_note"] = "看分布/关系/趋势模式与离群；图形探索不替代假设检验。"
    return out


def main() -> int:
    # rebuild base
    subprocess.run(
        [sys.executable, str(ROOT / "tools/build_research_by_id.py")],
        check=True,
        cwd=ROOT,
    )
    base_path = ROOT / "tools/learning_data/research_by_id.json"
    base: dict = json.loads(base_path.read_text(encoding="utf-8"))

    overlay: dict = {}
    for path in OVERLAY_JSON_FILES:
        if path.exists():
            chunk = json.loads(path.read_text(encoding="utf-8"))
            print(f"Loaded {len(chunk)} ids from {path.name}")
            overlay.update(chunk)
    for rel in SUBAGENT_FILES:
        path = TRANSCRIPT_DIR / rel
        chunk = load_subagent_json(path)
        print(f"Loaded {len(chunk)} ids from {path.name}")
        overlay.update(chunk)

    merged_count = 0
    skipped = []
    for cid, raw in overlay.items():
        if cid not in base:
            skipped.append(cid)
            continue
        base[cid] = normalize_entry(raw, base[cid])
        merged_count += 1
    if skipped:
        print(f"Skipped {len(skipped)} overlay ids not in union: {skipped}")

    base_path.write_text(json.dumps(base, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Merged {merged_count} subagent overlays into {base_path}")

    subprocess.run(
        [sys.executable, str(ROOT / "tools/build_learning_research_notes.py")],
        check=True,
        cwd=ROOT,
    )
    subprocess.run(
        [sys.executable, str(ROOT / "tools/verify_learning_research_notes.py")],
        check=True,
        cwd=ROOT,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
