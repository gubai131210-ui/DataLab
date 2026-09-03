# -*- coding: utf-8 -*-
"""Agent6 rework R1–R5: strip formula shells and diversify student-visible copy."""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OVERLAY_DIR = ROOT / "tools/learning_data/tutorial_overlays"
HANZI = re.compile(r"[\u4e00-\u9fff]")

# Protect these from aggressive rewrite of used_for/scenario (already Agent6-pass).
PROTECT = {
    "imr",
    "between_within_capability",
    "c_chart",
    "ewma",
    "xbar_r",
}

USED_TAIL_PATTERNS = [
    re.compile(r"菜单「[^」]+」帮你看清[^。]*。?"),
    re.compile(r"它更像在摊开「看见了什么」[^。]*。?"),
    re.compile(r"而不是直接回答能不能放行。?"),
    re.compile(r"放不放行通常还要对照规程和规格。?"),
    re.compile(r"这一课要对着看的那类信号。?"),
    re.compile(r"这一课要对着看的信号。?"),
    re.compile(r"本课要练的那一类信号。?"),
]

SCENARIO_HEAD = re.compile(r"^车间现场要看「[^」]+」这一类问题。?")
OUTPUT_SHELL = "常见读法是先说出看见了什么；放行或停线通常还要对照规程。"
OUTPUT_SHELL2 = "常见读法是先说出看见了什么；放行或停线通常还要对照规程"
MEANING_SHELLS = [
    "软件才知道图上或表上评价的是这一列。",
    "软件才知道图上或表上评价的是这一列",
    "软件才知道图上或表上对应哪一列。",
    "软件才知道图上或表上对应哪一列",
    "对一下就行：这一项对应本课现场里",
    "这一项决定图上或表上对应哪一列。",
    "这一项决定图上或表上对应哪一列",
]

TEXT_REPL = (
    ("配套练习表", "练习表"),
    ("禁止扩共享到 density/ecdf", "不要把本课练习表硬套到别的图课"),
    ("禁止扩共享到 density/ecdf。", "不要把本课练习表硬套到别的图课。"),
    ("禁止扩共享", "不要把别的课的表硬套过来"),
    ("同构", "同一张练习表"),
    ("WAVE", "本课"),
    ("这一课要对着看的那类信号", "这一课要对着看的信号"),
    ("本课要练的那一类信号", "这一课要对着看的信号"),
)


def hanzi_n(text: str) -> int:
    return len(HANZI.findall(text or ""))


def scrub_str(text: str) -> str:
    out = text or ""
    for old, new in TEXT_REPL:
        out = out.replace(old, new)
    return out


def columns_hint(ov: dict) -> str:
    fill = ov.get("dialog_fill") or {}
    vals = [str(v) for v in fill.values() if v and str(v) not in ("留空", "留空（用策略）")]
    if not vals:
        return ""
    # keep short
    joined = "、".join(vals[:3])
    return joined


def buried_hint(ov: dict) -> str:
    signals = ov.get("buried_signals") or []
    if not signals:
        return ""
    first = signals[0] if isinstance(signals[0], dict) else {}
    row = first.get("row")
    what = str(first.get("what") or "").strip()
    if row and what:
        return f"大约第 {row} 行附近：{what}"
    if what:
        return what
    if row:
        return f"大约第 {row} 行附近的埋点"
    return ""


def rewrite_used_for(cid: str, ov: dict) -> str:
    title = ov.get("title") or cid
    raw = scrub_str(str(ov.get("used_for") or ""))
    for pat in USED_TAIL_PATTERNS:
        raw = pat.sub("", raw)
    raw = re.sub(r"。{2,}", "。", raw).strip("。") + "。" if raw.strip() else ""
    cols = columns_hint(ov)
    buried = buried_hint(ov)
    # If still short or still formulaic, rebuild.
    formula = any(
        x in raw
        for x in (
            "帮你看清",
            "摊开「看见了什么」",
            "能不能放行",
            "这一课要对着看的信号",
        )
    )
    if hanzi_n(raw) < 40 or formula:
        pieces = [
            f"「{title}」用来把这一类现场问题摊开看一眼。",
        ]
        if cols:
            pieces.append(f"表里通常会用到 {cols} 这些列。")
        if buried:
            pieces.append(f"练习里埋了信号：{buried}。")
        else:
            pieces.append("不妨先按第 4 节填对话框，再对照第 5 节读输出。")
        pieces.append("看见信号之后，放不放行、停不停线通常还要对照规程和规格。")
        raw = "".join(pieces)
    # Ensure length
    if hanzi_n(raw) < 40:
        raw += f"打开菜单「{title}」时，先核对对话框字段，再指着输出说看见了什么。"
    return scrub_str(raw)


def rewrite_scenario(cid: str, ov: dict) -> str:
    title = ov.get("title") or cid
    raw = scrub_str(str(ov.get("scenario") or ""))
    raw = SCENARIO_HEAD.sub("", raw).strip()
    cols = columns_hint(ov)
    buried = buried_hint(ov)
    if "车间现场要看" in raw or hanzi_n(raw) < 40 or raw.startswith("不妨先按第 4 节"):
        pieces = []
        if buried:
            pieces.append(f"现场练习对着这条线看：{buried}。")
        else:
            pieces.append(f"现场要弄清「{title}」读出来的是什么信号。")
        if cols:
            pieces.append(f"不妨先把 {cols} 按第 4 节填进对话框并运行。")
        else:
            pieces.append("不妨先打开菜单，对照第 4 节核对真实字段。")
        pieces.append("停不停线、放不放行，可以等信号看清楚再和现场规程对一下。")
        raw = "".join(pieces)
    if hanzi_n(raw) < 40:
        raw += f"对照菜单「{title}」的输出，先说出看见了什么。"
    return scrub_str(raw)


def rewrite_not_for(ov: dict) -> str:
    title = ov.get("title") or ""
    raw = scrub_str(str(ov.get("not_for") or ""))
    if hanzi_n(raw) < 30:
        raw = (
            f"「{title}」读的是练习信号，不等于放行样板。"
            "先别把别的课的表硬套过来；停线或客户接受通常还要对照规程和规格。"
        )
    # strip leftover ban stamps if any
    raw = raw.replace("禁止过程合格", "不等于放行样板")
    return scrub_str(raw)


def rewrite_output_guide(ov: dict) -> str:
    raw = scrub_str(str(ov.get("output_guide") or ""))
    raw = raw.replace(OUTPUT_SHELL, "").replace(OUTPUT_SHELL2, "").strip()
    buried = buried_hint(ov)
    expect = ""
    signals = ov.get("buried_signals") or []
    if signals and isinstance(signals[0], dict):
        expect = str(signals[0].get("expect") or "").strip()
    if hanzi_n(raw) < 24 or "先说出看见了什么" in raw:
        pieces = []
        if buried:
            pieces.append(f"输出里先对着 {buried} 看。")
        else:
            pieces.append("输出里先找和第 5 节埋点对得上的那一块。")
        if expect:
            pieces.append(f"常见读法是：{expect}。")
        else:
            pieces.append("常见读法是先把「看见什么」说完整。")
        pieces.append("至于要不要停线或放行，通常还要对照规程和规格。")
        raw = "".join(pieces)
    return scrub_str(raw)


def rewrite_meanings(ov: dict) -> None:
    details = ov.get("dialog_fill_detail") or []
    seen: set[str] = set()
    for i, detail in enumerate(details):
        if not isinstance(detail, dict):
            continue
        field = str(detail.get("field") or "")
        put = str(detail.get("put") or "")
        meaning = scrub_str(str(detail.get("meaning") or ""))
        for shell in MEANING_SHELLS:
            if shell in meaning:
                # cut from shell onward if it's a trailing stamp
                idx = meaning.find(shell)
                if idx >= 0:
                    meaning = meaning[:idx].rstrip("。") + "。"
        meaning = meaning.replace("对一下就行：这一项对应本课现场里", "对照现场列名：")
        if hanzi_n(meaning) < 12 or meaning in seen:
            if put in ("留空", "留空（用策略）", "无需导入"):
                meaning = f"「{field}」本课留空即可；填了可能改掉第 5 节要对着看的信号。"
            elif put:
                meaning = (
                    f"「{put}」对应对话框「{field}」。"
                    f"它在现场是这一课要用的那一列（或默认值），填错会对不准图上的读数。"
                )
            else:
                meaning = f"「{field}」按第 4 节填写，不要虚构软件里没有的框。"
        # uniquify lightly
        if meaning in seen:
            meaning = meaning.rstrip("。") + f"（本课「{field}」）。"
        seen.add(meaning)
        detail["meaning"] = scrub_str(meaning)


def rewrite_buried(ov: dict) -> None:
    for item in ov.get("buried_signals") or []:
        if not isinstance(item, dict):
            continue
        for key in ("what", "expect", "meaning"):
            if key in item and isinstance(item[key], str):
                val = scrub_str(item[key])
                if OUTPUT_SHELL in val or OUTPUT_SHELL2 in val:
                    val = val.replace(OUTPUT_SHELL, "").replace(OUTPUT_SHELL2, "")
                    what = str(item.get("what") or "")
                    val = (val + f"常见读法是对照「{what}」把看见的信号说完整。").strip()
                # unwrap nested "常见读法是对照「…」读出看见了什么"
                val = re.sub(
                    r"常见读法是对照「([^」]*)」读出看见了什么。?",
                    r"常见读法是：\1。",
                    val,
                )
                item[key] = scrub_str(val)


def deep_scrub_json(obj):
    if isinstance(obj, dict):
        return {k: deep_scrub_json(v) for k, v in obj.items()}
    if isinstance(obj, list):
        return [deep_scrub_json(v) for v in obj]
    if isinstance(obj, str):
        return scrub_str(obj)
    return obj


def process(path: Path) -> bool:
    cid = path.stem
    ov = json.loads(path.read_text(encoding="utf-8"))
    before = json.dumps(ov, ensure_ascii=False)

    if cid not in PROTECT:
        ov["used_for"] = rewrite_used_for(cid, ov)
        ov["scenario"] = rewrite_scenario(cid, ov)
        ov["not_for"] = rewrite_not_for(ov)
        if "output_guide" in ov:
            ov["output_guide"] = rewrite_output_guide(ov)
        rewrite_meanings(ov)
        rewrite_buried(ov)

    # Always scrub student-visible placeholders everywhere (incl. protect, for 配套练习表)
    ov = deep_scrub_json(ov)

    # Re-protect imr hard points if deep scrub somehow touched numbers (it shouldn't)
    after = json.dumps(ov, ensure_ascii=False, indent=2) + "\n"
    if after != before + ("\n" if not before.endswith("\n") else ""):
        # compare semantically
        if json.dumps(json.loads(before), ensure_ascii=False) != json.dumps(ov, ensure_ascii=False):
            path.write_text(after, encoding="utf-8")
            return True
    return False


def main() -> None:
    n = 0
    for path in sorted(OVERLAY_DIR.glob("*.json")):
        if process(path):
            n += 1
    print(f"rewrote {n} overlays")
    checks = [
        "车间现场要看「",
        "配套练习表",
        "禁止扩共享",
        "本课要练的那一类信号",
        "这一课要对着看的那类信号",
        "软件才知道图上或表上",
        "常见读法是先说出看见了什么；放行或停线通常还要对照规程",
    ]
    for pat in checks:
        hit = sum(1 for p in OVERLAY_DIR.glob("*.json") if pat in p.read_text(encoding="utf-8"))
        print(f"remain {hit}: {pat}")


if __name__ == "__main__":
    main()
