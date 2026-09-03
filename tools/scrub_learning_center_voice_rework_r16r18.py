# -*- coding: utf-8 -*-
"""R16–R18: dedupe output_guide bloat; diversify used_for/scenario skeletons."""
from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OVERLAY_DIR = ROOT / "tools/learning_data/tutorial_overlays"
HANZI = re.compile(r"[\u4e00-\u9fff]")

PROTECT = {"imr", "between_within_capability", "c_chart", "ewma", "xbar_r"}

SKELETON_MARKERS = (
    "本集在大约第",
    "产线上对着「",
    "练习表里能看见「",
    "方便对着输出读",
    "对照埋点——大约第",
)


def hanzi_n(text: str) -> int:
    return len(HANZI.findall(text or ""))


def variant(cid: str, n: int) -> int:
    h = hashlib.md5(cid.encode("utf-8")).hexdigest()
    return int(h[:8], 16) % n


def buried(ov: dict) -> tuple[str, str, str]:
    signals = ov.get("buried_signals") or []
    if not signals or not isinstance(signals[0], dict):
        return "", "", ""
    s0 = signals[0]
    what = str(s0.get("what") or "").strip()
    expect = str(s0.get("expect") or "").strip()
    row = s0.get("row")
    where = f"大约第 {row} 行" if row not in (None, "") else ""
    return what, expect, where


def cols(ov: dict) -> list[str]:
    fill = ov.get("dialog_fill") or {}
    return [
        str(v)
        for v in fill.values()
        if v and str(v) not in ("留空", "留空（用策略）", "无需导入")
    ]


def dedupe_output_guide(cid: str, ov: dict) -> list[dict]:
    raw = ov.get("output_guide") or []
    if not isinstance(raw, list):
        raw = []
    uniq: list[dict] = []
    seen = set()
    for item in raw:
        if not isinstance(item, dict):
            continue
        name = str(item.get("name") or "读输出").strip() or "读输出"
        meaning = str(item.get("meaning") or "").strip()
        key = json.dumps({"name": name, "meaning": meaning}, ensure_ascii=False)
        if key in seen or not meaning:
            continue
        seen.add(key)
        uniq.append({"name": name, "meaning": meaning})

    title = ov.get("title") or cid
    what, expect, where = buried(ov)

    # If still empty / single generic stamp, rebuild 1–3 concrete items.
    if not uniq:
        uniq = [
            {
                "name": "读输出",
                "meaning": (
                    f"打开「{title}」后，先对照{where or '第 5 节埋点'}。"
                    + (f"应能看见：{what}。" if what else "先把看见的信号说完整。")
                    + "放行或停线通常还要对照规程。"
                ),
            }
        ]
    elif len(uniq) == 1 and (what or expect):
        base = uniq[0]
        # Expand into up to 2 distinct sections without cloning.
        uniq = [
            {
                "name": str(base.get("name") or title),
                "meaning": (
                    f"在「{title}」输出里，先找和「{what or '第 5 节埋点'}」对得上的那一块。"
                    + (f"{where}附近最容易对上。" if where else "")
                ),
            }
        ]
        if expect and expect not in uniq[0]["meaning"]:
            uniq.append(
                {
                    "name": "常见读法",
                    "meaning": f"{expect} 结论先停在信号本身；放行通常还要对照规程。",
                }
            )
        elif what:
            uniq.append(
                {
                    "name": "红线习惯",
                    "meaning": "看见信号不等于放行样板；停线或客户接受通常还要对照规程和规格。",
                }
            )

    # Hard cap and uniqueness
    cleaned: list[dict] = []
    seen2 = set()
    for item in uniq[:6]:
        meaning = re.sub(r"(先说出这一处看见了什么[。]?)+", "", item["meaning"])
        meaning = re.sub(r"(先把看见的信号说完整[。]?)+", "先把看见的信号说完整。", meaning)
        meaning = meaning.replace("。。", "。").strip()
        key = json.dumps({"name": item["name"], "meaning": meaning}, ensure_ascii=False)
        if key in seen2:
            continue
        seen2.add(key)
        cleaned.append({"name": item["name"], "meaning": meaning})
    return cleaned or [
        {
            "name": "读输出",
            "meaning": f"对照「{title}」第 5 节埋点，把看见的信号说完整。",
        }
    ]


def rewrite_used_for(cid: str, ov: dict) -> str:
    title = ov.get("title") or cid
    raw = str(ov.get("used_for") or "")
    if cid in PROTECT and not any(m in raw for m in SKELETON_MARKERS):
        return raw
    what, expect, where = buried(ov)
    col = "、".join(cols(ov)[:3])
    v = variant(cid, 4)
    if v == 0:
        text = (
            f"现场若要弄清「{title}」读出来的是什么，"
            + (f"表里通常用到 {col}。" if col else "先核对对话框真实字段。")
            + (f"练习里在{where}附近埋了「{what}」。" if what else "")
            + "不妨打开菜单跑一遍，对着输出把看见的信号说完整。"
            + "它不直接回答能不能放行。"
        )
    elif v == 1:
        text = (
            f"「{title}」更像一张对照纸条：告诉你该看哪一列、输出哪一块。"
            + (f"本课请盯 {col}。" if col else "")
            + (f"信号线索是「{what}」。" if what else "")
            + "读完先停在「看见了什么」；放行通常还要对照规程。"
        )
    elif v == 2:
        text = (
            f"当学员第一次点「{title}」时，目标不是背菜单，而是认出练习表里的变化。"
            + (f"变化大概在{where}：{what}。" if what else "对照第 5 节埋点即可。")
            + (f"对话框先填 {col}。" if col else "")
            + "停线与否留给现场规程。"
        )
    else:
        text = (
            f"用「{title}」把这一类问题摊开看一眼。"
            + (f"列名参考：{col}。" if col else "")
            + (f"期望能对上「{expect or what}」。" if (expect or what) else "")
            + "结论先别写成放行样板。"
        )
    return text


def rewrite_scenario(cid: str, ov: dict) -> str:
    title = ov.get("title") or cid
    raw = str(ov.get("scenario") or "")
    if cid in PROTECT and not any(m in raw for m in SKELETON_MARKERS):
        return raw
    what, expect, where = buried(ov)
    col = "、".join(cols(ov)[:3])
    v = variant(cid + ":sc", 4)
    if v == 0:
        text = (
            f"导入本课练习表后打开「{title}」。"
            + (f"把 {col} 按第 4 节填好。" if col else "对照第 4 节核对手里的字段。")
            + (f"顺着表看{where}：{what}。" if what else "")
            + "跑完先指着输出说看见了什么，再和规程对一下放不放行。"
        )
    elif v == 1:
        text = (
            f"这条练习线是为「{title}」准备的。"
            + (f"关键列：{col}。" if col else "")
            + (f"埋点：{what}（{where}）。" if what else "埋点见第 5 节。")
            + "不妨先运行一次，确认输出和埋点对得上。"
            + "停不停线可以稍后再谈。"
        )
    elif v == 2:
        text = (
            f"不妨先当一次助手：打开「{title}」，"
            + (f"填 {col}，" if col else "按第 4 节填写，")
            + (f"去找「{what}」。" if what else "去找第 5 节写的线索。")
            + (f"常见读法是：{expect}。" if expect else "")
            + "放行结论通常还要对照规格与规程。"
        )
    else:
        text = (
            f"还是那张本课练习表。菜单走「{title}」。"
            + (f"对话框带上 {col}。" if col else "")
            + (f"输出里优先核对{where}附近是否出现：{what}。" if what else "输出里优先核对第 5 节。")
            + "看清楚信号后，再决定要不要拿去和现场规程对齐。"
        )
    return text


def process(path: Path) -> bool:
    cid = path.stem
    ov = json.loads(path.read_text(encoding="utf-8"))
    before = json.dumps(ov, ensure_ascii=False)

    ov["output_guide"] = dedupe_output_guide(cid, ov)

    if cid not in PROTECT or any(
        m in str(ov.get("used_for") or "") + str(ov.get("scenario") or "")
        for m in SKELETON_MARKERS
    ):
        # Always diversify non-protect; protect only if contaminated by skeleton
        if cid not in PROTECT:
            ov["used_for"] = rewrite_used_for(cid, ov)
            ov["scenario"] = rewrite_scenario(cid, ov)
        else:
            # protected but skeleton-contaminated: light rewrite
            ov["used_for"] = rewrite_used_for(cid, ov)
            ov["scenario"] = rewrite_scenario(cid, ov)

    after = json.dumps(ov, ensure_ascii=False)
    if after != before:
        path.write_text(json.dumps(ov, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        return True
    return False


def main() -> None:
    n = sum(1 for p in sorted(OVERLAY_DIR.glob("*.json")) if process(p))
    print("rewrote", n)
    long = 0
    clone = 0
    skel = 0
    for p in OVERLAY_DIR.glob("*.json"):
        ov = json.loads(p.read_text(encoding="utf-8"))
        og = ov.get("output_guide") or []
        if len(og) > 8:
            long += 1
        uniq = len({json.dumps(x, ensure_ascii=False, sort_keys=True) for x in og})
        if isinstance(og, list) and len(og) > 1 and uniq == 1:
            clone += 1
        blob = str(ov.get("used_for") or "") + str(ov.get("scenario") or "")
        if any(m in blob for m in SKELETON_MARKERS):
            skel += 1
    print("output_guide>8", long, "cloned_unique1", clone, "skeleton_hits", skel)
    # sample lengths
    for cid in ("msa_type1", "two_sample_t", "histogram", "capability", "imr"):
        ov = json.loads((OVERLAY_DIR / f"{cid}.json").read_text(encoding="utf-8"))
        print(cid, "og_len", len(ov.get("output_guide") or []), "lines", len((OVERLAY_DIR / f"{cid}.json").read_text(encoding="utf-8").splitlines()))


if __name__ == "__main__":
    main()
