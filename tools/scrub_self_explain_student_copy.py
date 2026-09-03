#!/usr/bin/env python3
"""Remove student-facing 自解释 prompts after hiding learning-center 7B."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OVERLAY_DIR = ROOT / "tools" / "learning_data" / "tutorial_overlays"

REPLACEMENTS = (
    ("；每一步写下自解释。", "。"),
    ("；每做完一步写下上面的自解释。", "。"),
    ("；回答自解释题。", "。"),
    ("回答自解释题。", "按第 4 节对照字段说明练习。"),
    ("例题点过一遍、不写自解释也算学会了。", "例题点过一遍、不做练习闭环也算学会了。"),
    (
        "还要自己写自解释、做褪脚手架和合上教程的检索题，记忆才留得住。",
        "还要做褪脚手架和合上教程的检索题，记忆才留得住。",
    ),
    (
        "还要自己写原因、做褪脚手架和合上教程的检索题，记忆才留得住。",
        "还要做褪脚手架和合上教程的检索题，记忆才留得住。",
    ),
    ("必须有自解释 + 褪脚手架 + 检索小测。", "必须做褪脚手架和检索小测。"),
)


def scrub_text(text: str) -> str:
    out = text
    for old, new in REPLACEMENTS:
        out = out.replace(old, new)
    return out


def scrub_node(node: object) -> bool:
    changed = False
    if isinstance(node, dict):
        for key, value in node.items():
            if key == "self_explain":
                # Keep hidden data payload; only scrub student-visible fields elsewhere.
                continue
            if isinstance(value, str):
                scrubbed = scrub_text(value)
                if scrubbed != value:
                    node[key] = scrubbed
                    changed = True
            else:
                changed = scrub_node(value) or changed
    elif isinstance(node, list):
        for item in node:
            changed = scrub_node(item) or changed
    return changed


def count_visible_mentions(node: object) -> int:
    count = 0
    if isinstance(node, dict):
        for key, value in node.items():
            if key == "self_explain":
                continue
            count += count_visible_mentions(value)
    elif isinstance(node, list):
        for item in node:
            count += count_visible_mentions(item)
    elif isinstance(node, str):
        if "自解释" in node:
            count += 1
    return count


def main() -> int:
    touched = 0
    remaining = 0
    for path in sorted(OVERLAY_DIR.glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        if scrub_node(data):
            path.write_text(
                json.dumps(data, ensure_ascii=False, indent=2) + "\n",
                encoding="utf-8",
            )
            touched += 1
            data = json.loads(path.read_text(encoding="utf-8"))
        left = count_visible_mentions(data)
        if left:
            remaining += left
            print(f"REMAINING {left}: {path.name}")
    print(f"touched={touched} remaining_visible={remaining}")
    return 0 if remaining == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
