#!/usr/bin/env python3
"""Readability gate for learning-center overlay JSON (184)."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "learning_data"))
from glossary_bank import expected_ids, glossary_for  # noqa: E402

OVERLAY_DIR = ROOT / "tools/learning_data/tutorial_overlays"
HANZI = re.compile(r"[\u4e00-\u9fff]")
ID_LIKE = re.compile(r"^[a-z][a-z0-9_]+$")
BOILER = "第一次见到时把它读成车间里能指着说的那句话"
PAD_PHRASES = (
    BOILER,
    "先对着本课例子读一遍",
    "不要写成过程合格，不要写成过程合格",
)
FORBIDDEN_TERMS = {
    "没有练习表",
    "空 dataset",
    "禁止句",
    "过程合格（禁止写成结论）",
    "空表",
    "不是放行",
    "字段对照",
    "不要补假数",
}
FORBIDDEN_GLOSS = (
    "本波锁表",
    "锁表诚实",
    "勿挂旧10表",
    "对照帮助边界",
    "输出禁写",
    "WAVE",
    "Wave-",
    "没有练习表",
    "先对着本课例子读一遍",
)
TEMPLATE = ["histogram", "scatter_plot", "graph_gallery"]
SPC = {
    "imr",
    "xbar_r",
    "xbar_s",
    "p_chart",
    "np_chart",
    "c_chart",
    "u_chart",
    "laney_p_chart",
    "laney_u_chart",
    "ewma",
    "cusum",
    "moving_average",
    "zone_chart",
    "g_chart",
    "t_chart",
    "z_mr",
    "imr_rs",
    "hotelling_t2",
    "mewma",
    "generalized_variance",
    "special_cause_rules",
}
GRAPH_MENU = {
    "histogram": "直方图",
    "probability_plot": "概率图",
    "eda_4plot": "EDA 四图",
    "boxplot": "箱线图",
    "dotplot": "点图",
    "density_plot": "密度图",
    "hexbin_plot": "Hexbin",
    "violin_plot": "小提琴图",
    "bar_chart": "条形图",
    "scatter_plot": "散点图",
    "graph_gallery": "探索性图形",
    "interval_plot": "区间散点图",
    "correlation_plot": "相关图",
    "bubble_plot": "气泡图",
    "simplex_design_plot": "混料三角图",
    "mosaic_plot": "马赛克图",
    "chi_square_mosaic_link": "卡方–马赛克联动",
    "ecdf_plot": "经验累积分布图",
    "matrix_plot": "矩阵图",
    "marginal_plot": "边际图",
    "parallel_plot": "平行坐标图",
    "heatmap_plot": "热图",
    "correlogram": "Correlogram",
    "time_series_plot": "时间序列图",
    "area_plot": "区域图",
    "contour_plot": "等值线图",
    "pie_plot": "饼图",
}
FILL_REQUIRED = {
    "scatter_plot": ("x_variable", "y_variable"),
    "interval_plot": ("response", "category"),
    "bubble_plot": ("x_variable", "y_variable", "size_variable"),
    "hexbin_plot": ("x_variable", "y_variable"),
    "marginal_plot": ("x_variable", "y_variable"),
    "time_series_plot": ("time", "value"),
    "area_plot": ("time", "value"),
    "contour_plot": ("x_variable", "y_variable", "z_variable"),
}


# Voice-warmth blacklist (student-visible). Must not remain as raw strings.
VOICE_BLACKLIST = (
    "你的任务是",
    "这一课只练",
    "本课只练",
    "禁止过程合格",
    "抖主要",
    "本课要练的那一类信号",
    "这一课要对着看的那类信号",
    "软件才知道图上或表上评价的是这一列",
    "车间现场要看「",
    "配套练习表",
    "禁止扩共享",
    "软件才知道图上或表上",
    "常见读法是先说出看见了什么；放行或停线通常还要对照规程",
    "现场练习对着这条线看",
    "这样软件才知道要盯的是哪一列",
    "练习表 练习表",
    "对话框：variables",
    "把这一类现场问题摊开看一眼",
)
SHELL_MEANING = "这一项决定图上或表上对应哪一列"
SHELL_MEANING_NEAR = (
    "对一下就行：这一项对应本课现场里",
    "软件才知道图上或表上评价的是这一列",
)
STUDENT_VISIBLE_KEYS = (
    "used_for",
    "not_for",
    "scenario",
    "output_guide",
    "common_mistakes",
    "click_steps",
)


def hanzi_n(text: str) -> int:
    return len(HANZI.findall(text or ""))


def student_visible_blob(ov: dict) -> str:
    parts: list[str] = []
    for key in STUDENT_VISIBLE_KEYS:
        val = ov.get(key)
        if isinstance(val, list):
            parts.append(json.dumps(val, ensure_ascii=False))
        elif val:
            parts.append(str(val))
    for key in (
        "glossary",
        "dialog_fill_detail",
        "buried_signals",
        "prereq_quiz",
        "fade_levels",
        "retrieval_quiz",
        "misconceptions",
        "self_explain",
    ):
        if ov.get(key):
            parts.append(json.dumps(ov.get(key), ensure_ascii=False))
    return "\n".join(parts)


def check_voice_warmth(cid: str, ov: dict, errors: list[str]) -> None:
    """V1–V4 / V6 voice-warmth gates from copy-voice-warmth wave plan."""
    used = str(ov.get("used_for") or "")
    scenario = str(ov.get("scenario") or "")
    not_for = str(ov.get("not_for") or "")
    min_used = 80 if cid == "imr" else 40
    min_scen = 80 if cid == "imr" else 40
    if hanzi_n(used) < min_used:
        errors.append(f"{cid}: used_for too short ({hanzi_n(used)} < {min_used})")
    if hanzi_n(scenario) < min_scen:
        errors.append(f"{cid}: scenario too short ({hanzi_n(scenario)} < {min_scen})")
    if hanzi_n(not_for) < 30:
        errors.append(f"{cid}: not_for too short ({hanzi_n(not_for)} < 30)")

    visible = student_visible_blob(ov)
    for needle in VOICE_BLACKLIST:
        if needle in visible:
            errors.append(f"{cid}: voice blacklist {needle!r}")

    meanings = [
        str(item.get("meaning") or "")
        for item in (ov.get("dialog_fill_detail") or [])
        if isinstance(item, dict)
    ]
    for meaning in meanings:
        if SHELL_MEANING in meaning:
            errors.append(f"{cid}: dialog_fill_detail still has shell meaning")
            break
        if any(s in meaning for s in SHELL_MEANING_NEAR):
            errors.append(f"{cid}: dialog_fill_detail near-shell meaning")
            break
    nonempty = [m for m in meanings if m.strip()]
    if len(nonempty) >= 2 and len(set(nonempty)) == 1:
        errors.append(f"{cid}: dialog_fill_detail meanings all identical shell")

    # Developer jargon already checked on full blob below; also catch WAVE/同构 here early.
    if "同构" in visible or "WAVE" in visible or "禁止扩共享" in visible:
        errors.append(f"{cid}: developer jargon in student-visible core fields")

    og = ov.get("output_guide")
    if not isinstance(og, list) or not og:
        errors.append(f"{cid}: output_guide must be non-empty list")
    elif not all(isinstance(x, dict) and (x.get("name") or x.get("meaning")) for x in og):
        errors.append(f"{cid}: output_guide items must be name/meaning objects")
    else:
        if len(og) > 8:
            errors.append(f"{cid}: output_guide too long ({len(og)} > 8)")
        uniq = {
            json.dumps({"name": x.get("name"), "meaning": x.get("meaning")}, ensure_ascii=False)
            for x in og
        }
        if len(og) > 1 and len(uniq) == 1:
            errors.append(f"{cid}: output_guide cloned identical items")
        if len(uniq) != len(og):
            errors.append(f"{cid}: output_guide has duplicate items")

    if cid == "imr":
        gloss = json.dumps(ov.get("glossary") or [], ensure_ascii=False)
        if "UCL" not in gloss or "USL" not in gloss:
            errors.append("imr glossary missing UCL/USL")
        rows = {b.get("row") for b in ov.get("buried_signals") or []}
        if 41 not in rows or 55 not in rows:
            errors.append("imr buried 41/55 missing")
        if len(ov.get("dialog_fill_detail") or []) < 9:
            errors.append("imr dialog_fill_detail < 9")


def main() -> int:
    errors: list[str] = []
    paths = sorted(OVERLAY_DIR.glob("*.json"))
    if len(paths) != 184:
        errors.append(f"overlay count {len(paths)} != 184")
    for path in paths:
        cid = path.stem
        ov = json.loads(path.read_text(encoding="utf-8"))
        prereq = ov.get("prereq_quiz") or []
        if len(prereq) < 3:
            errors.append(f"{cid}: prereq_quiz {len(prereq)} < 3")
        for item in prereq:
            q = str(item.get("q") or "")
            good = str(item.get("good") or "")
            bad = str(item.get("bad") or "")
            if hanzi_n(q) < 12:
                errors.append(f"{cid}: short prereq q {q!r}")
            if hanzi_n(good) < 8 or ID_LIKE.match(good.strip()):
                errors.append(f"{cid}: short/id prereq good {good!r}")
            if hanzi_n(bad) < 8 or ID_LIKE.match(bad.strip()):
                errors.append(f"{cid}: short/id prereq bad {bad!r}")
        explains = ov.get("self_explain") or []
        if len(explains) < 2:
            errors.append(f"{cid}: self_explain {len(explains)} < 2")
        for item in explains:
            hint = str(item.get("hint") or "")
            prompt = str(item.get("prompt") or "")
            if not hint.strip():
                errors.append(f"{cid}: missing self_explain hint")
            elif hint.strip() == prompt.strip():
                errors.append(f"{cid}: hint repeats prompt")
        # 7B UI is hidden: student-visible fade/misconception copy must not mention 自解释.
        for fade in ov.get("fade_levels") or []:
            if isinstance(fade, dict):
                for key in ("student", "scaffold"):
                    if "自解释" in str(fade.get(key) or ""):
                        errors.append(f"{cid}: fade {key} still mentions 自解释")
        for misc in ov.get("misconceptions") or []:
            if isinstance(misc, dict):
                for key in ("wrong", "right"):
                    if "自解释" in str(misc.get(key) or ""):
                        errors.append(f"{cid}: misconception {key} still mentions 自解释")
        retrieval = ov.get("retrieval_quiz") or []
        if len(retrieval) < 3:
            errors.append(f"{cid}: retrieval {len(retrieval)} < 3")
        for item in retrieval:
            if isinstance(item, str):
                errors.append(f"{cid}: retrieval still a string {item!r}")
                continue
            if hanzi_n(str(item.get("q") or "")) < 12:
                errors.append(f"{cid}: short retrieval q")
            if not str(item.get("hint") or "").strip():
                errors.append(f"{cid}: empty retrieval hint")
        blob = json.dumps(ov, ensure_ascii=False)
        if "同构" in blob or "白名单" in blob or "WAVE" in blob:
            errors.append(f"{cid}: developer jargon in overlay")
        if cid not in SPC:
            pq = json.dumps(prereq, ensure_ascii=False)
            rq = json.dumps(retrieval, ensure_ascii=False)
            if "UCL" in pq or "UCL" in rq:
                errors.append(f"{cid}: non-SPC quiz mentions UCL")
        check_voice_warmth(cid, ov, errors)
        if cid == "bar_chart" and ov.get("title") != "条形图":
            errors.append(f"bar_chart title {ov.get('title')!r} != 条形图")
        menu = GRAPH_MENU.get(cid)
        if menu and menu not in str(ov.get("title") or ""):
            errors.append(f"{cid}: title {ov.get('title')!r} missing menu {menu}")
        related = ov.get("related_ids") or []
        if related == TEMPLATE:
            errors.append(f"{cid}: template related_ids")
        for key in FILL_REQUIRED.get(cid, ()):
            fill = ov.get("dialog_fill") or {}
            if not fill.get(key):
                errors.append(f"{cid}: dialog_fill missing {key}")
        if str(ov.get("dataset_id") or "").startswith("demo_"):
            errors.append(f"{cid}: dataset_id starts with demo_")

        gloss = ov.get("glossary") or []
        gloss_blob = json.dumps(gloss, ensure_ascii=False)
        if BOILER in gloss_blob:
            errors.append(f"{cid}: glossary boiler padding")
        if "。。" in gloss_blob:
            errors.append(f"{cid}: glossary double period")
        if len(gloss) < 3:
            errors.append(f"{cid}: glossary < 3")
        for item in gloss:
            term = str(item.get("term") or "")
            if term in FORBIDDEN_TERMS:
                errors.append(f"{cid}: forbidden glossary term {term!r}")
            if cid != "imr" and hanzi_n(str(item.get("plain") or "")) < 6:
                errors.append(f"{cid}: short glossary plain {item.get('plain')!r}")
            if cid != "imr" and hanzi_n(str(item.get("remember") or "")) < 6:
                errors.append(f"{cid}: short glossary remember {item.get('remember')!r}")
        for needle in FORBIDDEN_GLOSS:
            if needle in gloss_blob:
                errors.append(f"{cid}: glossary developer note {needle!r}")
        if cid != "imr":
            expected = glossary_for(cid, ov)
            if gloss != expected:
                errors.append(f"{cid}: glossary != bank")
        if cid in SPC:
            if "UCL" not in gloss_blob or "USL" not in gloss_blob:
                errors.append(f"{cid}: SPC glossary missing UCL/USL")
        for detail in ov.get("dialog_fill_detail") or []:
            field = str(detail.get("field") or "")
            if "配套练习表" in field:
                errors.append(f"{cid}: field still 配套练习表 {field!r}")

    overlay_ids = {path.stem for path in paths}
    bank_ids = expected_ids()
    missing = sorted(overlay_ids - bank_ids)
    extra = sorted(bank_ids - overlay_ids)
    if missing:
        errors.append(f"glossary_bank missing {missing[:8]}")
    if extra:
        errors.append(f"glossary_bank extra {extra[:8]}")

    if errors:
        print("FAIL")
        for err in errors[:80]:
            print(" -", err)
        if len(errors) > 80:
            print(f" ... {len(errors) - 80} more")
        print(f"total {len(errors)}")
        return 1
    print("PASS: 184 overlays meet copy-depth readability + voice warmth")
    return 0


if __name__ == "__main__":
    sys.exit(main())
