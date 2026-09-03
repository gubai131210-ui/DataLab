# -*- coding: utf-8 -*-
"""Agent6 recheck rework R8–R15."""
from __future__ import annotations

import ast
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OVERLAY_DIR = ROOT / "tools/learning_data/tutorial_overlays"
HANZI = re.compile(r"[\u4e00-\u9fff]")

PROTECT_CONTENT = {
    "imr",
    "between_within_capability",
    "c_chart",
    "ewma",
    "xbar_r",
}

SCENARIO_SHELL = "现场练习对着这条线看"
USED_TAIL = "把对话框填好"
USED_GRAPH = "把这一类现场问题摊开看一眼"
MEANING_SHELL = "这样软件才知道要盯的是哪一列"
EXPECT_TAIL = "现场口语可以停在信号本身"


def hanzi_n(text: str) -> int:
    return len(HANZI.findall(text or ""))


def scrub_student(text: str) -> str:
    out = text or ""
    reps = (
        ("练习表 练习表", "本课练习表"),
        ("练习安排 练习安排", "本课练习安排"),
        ("方差专项用 练习表", "方差专项应换「方差检验」那一课的表与菜单"),
        ("等价用 练习表", "等价检验应换「等价检验」那一课的表与菜单"),
        ("位置差用 练习表", "位置差应换「两样本 t」那一课的表与菜单"),
        ("双因子用 练习表", "双因子应换「双因子 ANOVA」那一课的表与菜单"),
        ("单比例用 练习表", "单比例应换「单比例」那一课的表与菜单"),
        ("两比例用 练习表", "两比例应换「两比例」那一课的表与菜单"),
        ("泊松用 练习表", "泊松率应换对应泊松课的表与菜单"),
        ("n=5 均值散度用 练习表", "n=5 时更常见的是 Xbar-R；本课是 Xbar-S"),
        ("对话框：variables", "对话框里的变量栏"),
        ("对话框：variables ", "对话框里的变量栏 "),
        ("对话框:variables", "对话框里的变量栏"),
        ("对话框：subgroup", "对话框里的子组栏"),
        ("对话框：response", "对话框里的响应栏"),
        ("对话框：", "对话框里的"),
        (MEANING_SHELL + "。", "后面的图和表才会盯这一列。"),
        (MEANING_SHELL, "后面的图和表才会盯这一列"),
        ("capability_transform", "变换选项"),
        ("two_sample_t_method", "方差方法"),
        ("anova_factor_encoding", "因子编码"),
        ("variance_test_method", "方差方法"),
        ("variance_alternative", "备择方向"),
        ("equivalence_transform", "变换"),
        ("two_poisson_comparison", "比较量"),
        ("smoothing_method", "平滑方法"),
        ("decomposition_model", "分解模型"),
        ("tolerance_method", "容差方法"),
        ("tolerance_interval_alternative", "区间方向"),
        ("yes_no_choice", "是/否选项"),
    )
    for old, new in reps:
        out = out.replace(old, new)
    # collapse duplicate sentences
    parts = [p.strip() for p in re.split(r"(?<=。)", out) if p.strip()]
    seen = set()
    kept = []
    for p in parts:
        if p in seen:
            continue
        seen.add(p)
        kept.append(p)
    return "".join(kept)


def deep_scrub(obj):
    if isinstance(obj, dict):
        return {k: deep_scrub(v) for k, v in obj.items()}
    if isinstance(obj, list):
        return [deep_scrub(v) for v in obj]
    if isinstance(obj, str):
        return scrub_student(obj)
    return obj


def parse_output_guide(raw) -> list[dict]:
    if isinstance(raw, list):
        out = []
        for item in raw:
            if isinstance(item, dict) and (item.get("name") or item.get("meaning")):
                out.append(
                    {
                        "name": scrub_student(str(item.get("name") or "读输出")),
                        "meaning": scrub_student(str(item.get("meaning") or "")),
                    }
                )
            elif isinstance(item, str) and item.strip():
                out.append({"name": "读输出", "meaning": scrub_student(item)})
        return out or [{"name": "读输出", "meaning": "对照第 5 节埋点，先说出看见了什么。"}]
    if not isinstance(raw, str):
        return [{"name": "读输出", "meaning": "对照第 5 节埋点，先说出看见了什么。"}]
    text = raw.strip()
    if text.startswith("[") and ("name" in text or "meaning" in text):
        try:
            # Python-literal style from str(list)
            data = ast.literal_eval(text)
            return parse_output_guide(data)
        except Exception:
            try:
                data = json.loads(text.replace("'", '"'))
                return parse_output_guide(data)
            except Exception:
                pass
    text = scrub_student(text)
    if not text:
        text = "对照第 5 节埋点，先说出看见了什么。至于停线或放行，通常还要对照规程。"
    return [{"name": "读输出", "meaning": text}]


def buried_line(ov: dict) -> tuple[str, str]:
    signals = ov.get("buried_signals") or []
    if not signals or not isinstance(signals[0], dict):
        return "", ""
    first = signals[0]
    what = scrub_student(str(first.get("what") or ""))
    expect = scrub_student(str(first.get("expect") or ""))
    row = first.get("row")
    where = f"大约第 {row} 行" if row not in (None, "") else ""
    return what, expect if expect else "", where  # type: ignore


def rewrite_used_for(cid: str, ov: dict) -> str:
    title = ov.get("title") or cid
    raw = scrub_student(str(ov.get("used_for") or ""))
    fill = ov.get("dialog_fill") or {}
    cols = [str(v) for v in fill.values() if v and str(v) not in ("留空", "留空（用策略）")]
    what, expect, where = "", "", ""
    signals = ov.get("buried_signals") or []
    if signals and isinstance(signals[0], dict):
        what = scrub_student(str(signals[0].get("what") or ""))
        expect = scrub_student(str(signals[0].get("expect") or ""))
        row = signals[0].get("row")
        where = f"大约第 {row} 行" if row not in (None, "") else ""

    bad = (
        USED_TAIL in raw
        or USED_GRAPH in raw
        or hanzi_n(raw) < 40
        or ("对照列名" in raw and "对话框填好" in raw)
        or ("不妨打开" in raw and "对照列名" in raw)
    )
    if not bad:
        return raw

    # Differentiated by available lesson facts — avoid one universal skeleton.
    if cols and what:
        return (
            f"「{title}」适合这种现场：表里用 { '、'.join(cols[:3]) }，"
            f"要辨认的是「{what}」。"
            f"不妨打开菜单核对对话框，再对着{where or '第 5 节埋点'}读输出。"
            "看见信号不等于放行；停线通常还要对照规程。"
        )
    if cols:
        return (
            f"当你要弄清「{title}」读出来的信号时，先确认表里的 { '、'.join(cols[:3]) } 填对。"
            "对话框按第 4 节来，输出按第 5 节指着说看见了什么。"
            "它不直接回答客户能不能放行。"
        )
    if what:
        return (
            f"「{title}」这一课主要帮你认出「{what}」。"
            f"菜单打开后，对照{where or '埋点'}看输出是否对得上。"
            "结论先停在信号本身，放行还要规程和规格。"
        )
    return (
        f"「{title}」用来练习读这一类输出。"
        "先核对对话框真实字段，再对照第 5 节把看见的信号说完整。"
        "不要把邻课的表硬套过来，也不要直接写成可以放行。"
    )


def rewrite_scenario(cid: str, ov: dict) -> str:
    title = ov.get("title") or cid
    raw = scrub_student(str(ov.get("scenario") or ""))
    if SCENARIO_SHELL not in raw and hanzi_n(raw) >= 40 and "不妨先把" not in raw[:20]:
        # still scrub internal leftovers
        return raw

    fill = ov.get("dialog_fill") or {}
    cols = [str(v) for v in fill.values() if v and str(v) not in ("留空", "留空（用策略）")]
    what = ""
    where = ""
    expect = ""
    signals = ov.get("buried_signals") or []
    if signals and isinstance(signals[0], dict):
        what = scrub_student(str(signals[0].get("what") or ""))
        expect = scrub_student(str(signals[0].get("expect") or ""))
        row = signals[0].get("row")
        where = f"大约第 {row} 行附近" if row not in (None, "") else "表里埋点附近"

    pieces = [f"还是用本课练习表练「{title}」。"]
    if what:
        pieces.append(f"顺着数据往下翻，{where}能看见：{what}。")
    if cols:
        pieces.append(f"不妨先把 { '、'.join(cols[:3]) } 按第 4 节填进对话框并运行。")
    else:
        pieces.append("不妨先打开菜单，对照第 4 节核对手里真实有的字段。")
    if expect and EXPECT_TAIL not in expect:
        pieces.append(f"输出上常见读法是：{expect}")
    pieces.append("停不停线、放不放行，可以等信号看清楚再和现场规程对一下。")
    text = "".join(pieces)
    # de-dup
    return scrub_student(text)


def rewrite_not_for(ov: dict) -> str:
    raw = scrub_student(str(ov.get("not_for") or ""))
    # dedupe exact repeated sentence
    parts = [p.strip() for p in re.split(r"(?<=。)", raw) if p.strip()]
    seen = set()
    kept = []
    for p in parts:
        if p in seen:
            continue
        seen.add(p)
        kept.append(p)
    raw = "".join(kept)
    if hanzi_n(raw) < 30:
        title = ov.get("title") or ""
        raw = (
            f"「{title}」的练习安排不等于放行样板。"
            "先别把邻课的表硬套过来；停线或客户接受通常还要对照规程和规格。"
        )
    return raw


def rewrite_meanings(ov: dict) -> None:
    seen = set()
    for detail in ov.get("dialog_fill_detail") or []:
        if not isinstance(detail, dict):
            continue
        field = str(detail.get("field") or "")
        put = str(detail.get("put") or "")
        meaning = scrub_student(str(detail.get("meaning") or ""))
        if MEANING_SHELL in meaning:
            meaning = meaning.replace(MEANING_SHELL + "。", "后面的图和表才会盯这一列。")
            meaning = meaning.replace(MEANING_SHELL, "后面的图和表才会盯这一列")
        # strip internal enum-ish tokens already handled by scrub_student
        if hanzi_n(meaning) < 12 or meaning in seen:
            if put in ("留空", "留空（用策略）", "无需导入"):
                meaning = f"「{field}」本课留空；填了可能改掉第 5 节要对着看的信号。"
            elif put:
                meaning = f"「{put}」填进「{field}」。它是本课现场要用的那一列或默认值，填错会对不准读数。"
            else:
                meaning = f"「{field}」按第 4 节填，不要虚构软件里没有的框。"
        if meaning in seen:
            meaning = meaning.rstrip("。") + f"（对应「{field}」）。"
        seen.add(meaning)
        detail["meaning"] = meaning


def rewrite_buried(ov: dict) -> None:
    for item in ov.get("buried_signals") or []:
        if not isinstance(item, dict):
            continue
        what = scrub_student(str(item.get("what") or ""))
        expect = scrub_student(str(item.get("expect") or ""))
        if EXPECT_TAIL in expect or expect.startswith("常见读法是：" + what) or expect.startswith(f"常见读法是：{what}"):
            expect = f"输出里应能对上「{what}」。先把看见的信号说完整，再谈停线或放行。"
        elif not expect:
            expect = f"输出里应能对上「{what}」。"
        # fix weird punctuation from prior templates
        expect = expect.replace("。；", "。").replace("..", "。")
        item["what"] = what
        item["expect"] = expect


def process(path: Path) -> bool:
    cid = path.stem
    ov = json.loads(path.read_text(encoding="utf-8"))
    before = json.dumps(ov, ensure_ascii=False)

    # R8 always
    ov["output_guide"] = parse_output_guide(ov.get("output_guide"))

    if cid not in PROTECT_CONTENT:
        ov["used_for"] = rewrite_used_for(cid, ov)
        ov["scenario"] = rewrite_scenario(cid, ov)
        ov["not_for"] = rewrite_not_for(ov)
        rewrite_meanings(ov)
        rewrite_buried(ov)
    else:
        # protect content but still fix output_guide shape + student scrub
        ov["used_for"] = scrub_student(str(ov.get("used_for") or ""))
        ov["scenario"] = scrub_student(str(ov.get("scenario") or ""))
        ov["not_for"] = rewrite_not_for(ov)
        for detail in ov.get("dialog_fill_detail") or []:
            if isinstance(detail, dict) and "meaning" in detail:
                detail["meaning"] = scrub_student(str(detail["meaning"]))

    ov = deep_scrub(ov)
    # ensure output_guide still list after deep_scrub
    ov["output_guide"] = parse_output_guide(ov.get("output_guide"))

    after_obj = json.dumps(ov, ensure_ascii=False)
    if after_obj != before:
        path.write_text(json.dumps(ov, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        return True
    return False


def main() -> None:
    n = sum(1 for p in sorted(OVERLAY_DIR.glob("*.json")) if process(p))
    print("rewrote", n)
    checks = [
        ("现场练习对着这条线看", True),
        ("这样软件才知道要盯的是哪一列", True),
        ("练习表 练习表", True),
        ("对话框：variables", True),
        ("配套练习表", True),
        ("把这一类现场问题摊开看一眼", True),
    ]
    for pat, _ in checks:
        hit = [p.stem for p in OVERLAY_DIR.glob("*.json") if pat in p.read_text(encoding="utf-8")]
        print(f"remain {len(hit)} {pat} -> {hit[:5]}")

    bad_og = 0
    for p in OVERLAY_DIR.glob("*.json"):
        ov = json.loads(p.read_text(encoding="utf-8"))
        og = ov.get("output_guide")
        if not isinstance(og, list) or not og or not all(isinstance(x, dict) for x in og):
            bad_og += 1
            print("bad output_guide", p.stem, type(og).__name__)
    print("bad_output_guide_count", bad_og)


if __name__ == "__main__":
    main()
