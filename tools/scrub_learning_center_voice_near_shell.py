# -*- coding: utf-8 -*-
"""Scrub remaining near-shell / formulaic voice in overlays (Agent3 polish)."""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OVERLAY_DIR = ROOT / "tools/learning_data/tutorial_overlays"

NEAR_SHELL = "软件才知道图上或表上评价的是这一列"
NEAR_SHELL2 = "软件才知道图上或表上对应哪一列"
PRACTICE_SIGNAL = "本课要练的那一类信号"
PRACTICE_SIGNAL_NEW = "这一课要对着看的那类信号"

REPLACEMENTS = (
    (NEAR_SHELL, "这样软件才知道要盯的是哪一列"),
    (NEAR_SHELL2, "这样软件才知道要盯的是哪一列"),
    (PRACTICE_SIGNAL, PRACTICE_SIGNAL_NEW),
    ("帮你看清本课要练的那一类信号", "帮你看清这一课要对着看的那类信号"),
    ("它更像在摊开「看见了什么」，而不是直接回答能不能放行。",
     "它更像在摊开「看见了什么」；放不放行通常还要对照规程和规格。"),
)


def scrub_text(text: str) -> str:
    out = text or ""
    for old, new in REPLACEMENTS:
        out = out.replace(old, new)
    # collapse accidental double periods
    out = out.replace("。。", "。")
    return out


def scrub_overlay(ov: dict) -> dict:
    for key in ("used_for", "not_for", "scenario", "output_guide"):
        if isinstance(ov.get(key), str):
            ov[key] = scrub_text(ov[key])
    if isinstance(ov.get("click_steps"), list):
        ov["click_steps"] = [scrub_text(s) if isinstance(s, str) else s for s in ov["click_steps"]]
    for detail in ov.get("dialog_fill_detail") or []:
        if isinstance(detail, dict) and "meaning" in detail:
            detail["meaning"] = scrub_text(str(detail["meaning"]))
            # drop redundant double-sentence if meaning repeats put/field twice
            meaning = detail["meaning"]
            # shorten if still has both 「填进」 and 「放进」 boilerplate stacking
            if meaning.count("在对话框里") >= 2:
                # keep first sentence + local column note if present
                parts = [p.strip() for p in re.split(r"(?<=。)", meaning) if p.strip()]
                kept = []
                seen_dialog = False
                for p in parts:
                    if "在对话框里" in p:
                        if seen_dialog:
                            continue
                        seen_dialog = True
                    kept.append(p)
                detail["meaning"] = "".join(kept)
    for section in ("prereq_quiz", "fade_levels", "retrieval_quiz", "misconceptions", "self_explain", "common_mistakes", "buried_signals", "glossary"):
        blob = ov.get(section)
        if blob is None:
            continue
        text = json.dumps(blob, ensure_ascii=False)
        new = scrub_text(text)
        if new != text:
            ov[section] = json.loads(new)
    return ov


def main() -> None:
    n = 0
    for path in sorted(OVERLAY_DIR.glob("*.json")):
        ov = json.loads(path.read_text(encoding="utf-8"))
        before = json.dumps(ov, ensure_ascii=False)
        ov = scrub_overlay(ov)
        after = json.dumps(ov, ensure_ascii=False)
        if after != before:
            path.write_text(json.dumps(ov, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
            n += 1
    print(f"scrubbed {n} overlays")


if __name__ == "__main__":
    main()
