#!/usr/bin/env python3
"""Warm Wave-2 / Wave-3 / Wave-4 learning-center overlays to collaborative voice.

Skips imr.json (Wave-0 gold). Preserves title / dialog_fill / structure.
"""
from __future__ import annotations

import json
import re
import sys
from copy import deepcopy
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from copy_depth import clean_field_label, expand_meaning, scrub  # noqa: E402

OVERLAY_DIR = Path(__file__).resolve().parent / "tutorial_overlays"
HANZI = re.compile(r"[\u4e00-\u9fff]")
SHELL = "这一项决定图上或表上对应哪一列"
BAN = ("你的任务是", "这一课只练", "本课只练", "禁止过程合格", "抖主要")
SKIP = frozenset({"imr", "between_within_capability"})  # gold / handcrafted — never overwrite
# Near-shell phrases that make already_good() return False (Agent6 NO-GO list).
NEAR_SHELLS = (
    "本课要练的那一类信号",
    "这一课要对着看的那类信号",
    "这一课要对着看的信号",
    "摊开「看见了什么」",
    "而不是直接回答能不能放行。",  # shell without 这批货 quotes — gold imr/bwc differ
    "软件才知道图上或表上",
    "车间现场要看「",
    "配套练习表",
    "禁止扩共享",
    "对一下就行：这一项对应本课现场里",
    "常见读法是先说出看见了什么；放行或停线通常还要对照规程",
    "常见读法是先说出看见了什么；不等于放行样板，通常还要对照规程",
)

WAVE2 = [
    "acceptance_sampling",
    "attribute_agreement",
    "batch_capability",
    "between_within_capability",
    "binomial_capability",
    "box_cox",
    "capability",
    "capability_sixpack",
    "cause_and_effect",
    "distribution_identification",
    "emp_crossed",
    "expanded_gage_rr",
    "expanded_gage_unbalanced",
    "gage_rr",
    "msa_type1",
    "multi_vari",
    "nested_gage_rr",
    "nonnormal_capability",
    "nonparametric_capability",
    "pareto",
    "poisson_capability",
    "run_chart",
    "tolerance_intervals",
    "variability_chart",
]
WAVE3 = [
    "anom",
    "anom_attribute",
    "best_subsets_regression",
    "bootstrap_mean",
    "bootstrap_two_sample",
    "chi_square",
    "chi_square_gof",
    "cochran_q",
    "correlation",
    "cross_tabulation",
    "descriptive",
    "fisher_exact",
    "friedman",
    "general_manova",
    "glm_three_factor",
    "glm_two_way",
    "kruskal_wallis",
    "logistic_regression",
    "mann_whitney",
    "manova_one_way",
    "mcnemar",
    "mixed_effects_reml",
    "mood_median",
    "nominal_logistic",
    "nonlinear_regression",
    "normality_test",
    "one_poisson_rate",
    "one_proportion",
    "one_proportion_equivalence",
    "one_sample_equivalence",
    "one_sample_t",
    "one_sample_z",
    "one_way_anova",
    "ordinal_logistic",
    "orthogonal_regression",
    "outlier_test",
    "paired_equivalence",
    "paired_t",
    "pls_regression",
    "poisson_gof",
    "poisson_regression",
    "randomization_test",
    "regression",
    "runs_test",
    "sign_test",
    "stepwise_regression",
    "t_power",
    "two_factor_anova",
    "two_poisson_rate",
    "two_proportion_equivalence",
    "two_proportions",
    "two_sample_equivalence",
    "two_sample_equivalence_ratio",
    "two_sample_t",
    "variance_test",
    "wilcoxon_signed_rank",
]
WAVE4 = [
    "accelerated_life",
    "acf_pacf",
    "adf_test",
    "analyze_definitive_screening",
    "analyze_variability",
    "area_plot",
    "arima",
    "bar_chart",
    "binary_doe_probit",
    "binary_response_doe",
    "boxplot",
    "bubble_plot",
    "cart_tree",
    "ccf",
    "chi_square_mosaic_link",
    "cluster_observations",
    "cluster_variables",
    "contour_plot",
    "correlation_plot",
    "correlogram",
    "cox_counting_process",
    "cox_regression",
    "database_import",
    "definitive_screening_design",
    "density_plot",
    "discriminant",
    "distribution_calculator",
    "doe_bbd",
    "doe_ccd",
    "doe_d_optimal",
    "doe_factorial",
    "doe_plackett_burman",
    "doe_response",
    "dotplot",
    "ecdf_plot",
    "eda_4plot",
    "factor_analysis",
    "fine_gray_regression",
    "graph_gallery",
    "heatmap_plot",
    "hexbin_plot",
    "histogram",
    "interval_plot",
    "isolation_forest",
    "km_interval",
    "kmeans",
    "life_data_lognormal",
    "life_data_regression",
    "marginal_plot",
    "matrix_plot",
    "mixture_analyze",
    "mixture_design",
    "mixture_extreme_vertices_design",
    "mixture_process_variable",
    "mosaic_plot",
    "multiple_correspondence",
    "nhpp_repairable",
    "parallel_plot",
    "pca",
    "pie_plot",
    "probability_plot",
    "probit_reliability",
    "random_forest",
    "reliability",
    "reliability_test_plan",
    "reliability_warranty",
    "report_templates",
    "response_optimization",
    "rsm_response",
    "scatter_plot",
    "seasonal_forecasting",
    "simple_correspondence",
    "simplex_design_plot",
    "split_plot_analyze",
    "split_plot_design",
    "taguchi_analyze",
    "taguchi_orthogonal_design",
    "time_series_decomposition",
    "time_series_plot",
    "time_series_smoothing",
    "trend_analysis",
    "violin_plot",
    "weibayes",
]

ALL = WAVE2 + WAVE3 + WAVE4

def hn(text: str) -> int:
    return len(HANZI.findall(text or ""))


def scrub_ban(text: str) -> str:
    out = scrub(text or "")
    reps = (
        ("禁止过程合格。", "不等于放行样板；通常还要对照规程。"),
        ("禁止过程合格", "不等于放行样板"),
        ("本课只练", "本课主要看"),
        ("这一课只练", "这一课主要看"),
        ("你的任务是", "不妨先"),
        ("你应该能", "常见读法是"),
        ("抖主要", "波动主要落在"),
        (SHELL + "。", ""),
        (SHELL, ""),
        ("禁止已证明正态。", "不等于已经证明正态。"),
        ("禁止已证明正态", "不等于已经证明正态"),
        ("禁止必须停线。", "停线通常还要对照规程。"),
        ("禁止必须停线", "停线通常还要对照规程"),
        ("勿写过程合格。", "不等于放行样板。"),
        ("勿写过程合格", "不等于放行样板"),
        ("写成过程合格", "写成已经放行"),
        ("过程合格", "已经放行"),
        ("必须停线", "必须立刻停线"),
        ("已证明正态", "已经证明正态"),
        ("禁止量具通过。", "不等于量具已经通过。"),
        ("禁止量具通过", "不等于量具已经通过"),
        ("拒绝已经放行、必须立刻停线这类话术", "把结论停在「看见信号」，放行/停线通常还要对照规程"),
        ("并且拒绝已经放行", "并能把结论停在「看见信号」"),
        ("禁止用 配套练习表", "不要拿别的课的练习安排硬套"),
        ("禁止用配套练习表", "不要拿别的课的练习安排硬套"),
        ("配套练习表", "练习安排"),
        ("禁止扩共享", "不要把别的课的表硬套过来"),
        ("（禁止用 ", "（不要拿 "),
        ("失控数据（禁止用", "失控数据（不要拿"),
        ("本课要练的那一类信号", ""),
        ("这一课要对着看的那类信号", ""),
        ("这一课要对着看的信号", ""),
        ("它更像在摊开「看见了什么」，而不是直接回答能不能放行。", ""),
        ("它更像在摊开「看见了什么」；放不放行通常还要对照规程和规格。", ""),
        ("它更像在摊开「看见了什么」", ""),
        ("常见读法是先说出看见了什么；放行或停线通常还要对照规程。", ""),
        ("常见读法是先说出看见了什么；不等于放行样板，通常还要对照规程。", ""),
        ("车间现场要看「", "现场要看「"),
    )
    for a, b in reps:
        out = out.replace(a, b)
    out = re.sub(r"现场要看「[^」]+」这一类问题。?", "", out)
    out = re.sub(r"菜单「[^」]+」帮你看清。?", "", out)
    out = re.sub(r"。{2,}", "。", out)
    out = re.sub(r"\s{2,}", " ", out).strip()
    return out


def col_phrase(overlay: dict) -> str:
    fill = overlay.get("dialog_fill") or {}
    vals = [str(v) for v in fill.values() if v and str(v) not in ("留空", "无需导入", "按菜单实填")]
    if not vals:
        return ""
    return "、".join(vals[:3])


def buried_bits(overlay: dict) -> tuple[str, str, str]:
    buried = overlay.get("buried_signals") or []
    if not buried:
        return "", "", ""
    first = buried[0]
    row = first.get("row")
    what = scrub_ban(first.get("what") or "")
    expect = scrub_ban(first.get("expect") or "")
    row_phrase = f"大约第 {row} 行" if row is not None else ""
    return row_phrase, what, expect


def family_of(cid: str) -> str:
    if cid in WAVE2:
        if any(k in cid for k in ("gage", "msa", "emp_", "attribute_agreement", "nested_gage", "expanded_gage")):
            return "msa"
        if any(
            k in cid
            for k in (
                "capability",
                "binomial",
                "poisson_capability",
                "nonnormal",
                "nonparametric",
                "batch_capability",
                "box_cox",
                "tolerance",
            )
        ):
            return "cap"
        return "qual"
    if cid in WAVE3:
        if any(k in cid for k in ("anova", "glm", "manova", "friedman", "kruskal", "mixed_effects")):
            return "anova"
        if any(
            k in cid
            for k in (
                "regression",
                "logistic",
                "pls",
                "correlation",
                "best_subsets",
                "stepwise",
                "orthogonal",
                "poisson_regression",
                "nonlinear",
            )
        ):
            return "regr"
        return "infer"
    if any(
        k in cid
        for k in (
            "doe",
            "taguchi",
            "mixture",
            "split_plot",
            "response_optimization",
            "rsm",
            "definitive",
            "analyze_variability",
            "binary_doe",
            "binary_response",
        )
    ):
        return "doe"
    if any(k in cid for k in ("reliab", "cox", "weib", "life_", "nhpp", "accelerated", "probit_reliab", "km_interval")):
        return "rel"
    if any(k in cid for k in ("time_series", "arima", "acf", "adf", "seasonal", "trend", "ccf", "correlogram")):
        return "ts"
    if cid in ("database_import", "report_templates", "graph_gallery", "distribution_calculator"):
        return "util"
    return "graph"


USED_LEAD = {
    "msa": "量具研究先问「尺子稳不稳、人与人差不差」，再谈过程能力。",
    "cap": "能力分析是把过程中心和波动，对照规格宽窄看一眼。",
    "qual": "质量工具课多半先把缺陷、原因或趋势摊开看清楚。",
    "infer": "推断课是用完整问句对照假设，看数据站不站得住。",
    "anova": "多组比较时，不妨先看各组位置差在哪，再读模型摘要。",
    "regr": "回归/相关课是看预测与响应怎么一起走，不是盖放行章。",
    "doe": "试验设计课是把因子安排和响应读清楚，效应不等于放行。",
    "rel": "可靠性课看寿命或失效风险摘要，不等于已经放行。",
    "ts": "时间序列课顺着时间看趋势、季节或平滑轨迹。",
    "util": "这一课偏菜单/帮助边界：先弄清字段和适用场合。",
    "graph": "图形课先把形状、分组或关联摊开看一眼。",
}

USED_CLOSE = {
    "msa": "读完先停在「量具波动里哪一块更大」；合不合格量具、能不能放行，通常还要对照规程。",
    "cap": "读完先停在「相对规格偏了多少、散了多少」；放不放行通常还要对照规程。",
    "qual": "读完先停在「哪一类原因/缺陷更扎眼」；根因判决和放行通常还要对照规程。",
    "infer": "读完先停在「和假设合不合、差有多大」；放行或停线通常还要对照规程。",
    "anova": "读完先停在「哪一组抬高、交互有没有」；改工艺通常还要对照规程。",
    "regr": "读完先停在「谁跟着谁走、拟合好不好」；因果和放行通常还要另证。",
    "doe": "读完先停在「哪个因子效应更亮」；最优配方放行通常还要对照规程。",
    "rel": "读完先停在「寿命/风险摘要长什么样」；批次放行通常还要对照规程。",
    "ts": "读完先停在「趋势、季节还是噪声在说话」；预测动作通常还要对照规程。",
    "util": "先把字段和适用场合分清；不要虚构没有的框，也不要写成已经放行。",
    "graph": "读完先停在「形状/分组/关联看见了什么」；证明正态或放行通常还要另做。",
}

NOT_FOR_TAIL = "练习安排不等于放行样板；停线或客户接受通常还要对照规程和规格。"


def ensure_min(text: str, minimum: int, pad: str) -> str:
    text = scrub_ban(text).strip()
    if hn(text) >= minimum:
        return text
    if text and not text.endswith("。"):
        text += "。"
    return (text + pad).strip()


def _strip_used_shells(text: str) -> str:
    text = scrub_ban(text)
    text = re.sub(r"菜单「[^」]+」帮你看清[^。]*。", "", text)
    text = re.sub(r"不妨打开「[^」]+」，对照列名（如 [^）]+）把对话框填好。", "", text)
    for lead in USED_LEAD.values():
        if text.startswith(lead):
            text = text[len(lead) :]
    return re.sub(r"。{2,}", "。", text).strip()


def warm_used_for(cid: str, overlay: dict) -> str:
    title = overlay.get("title") or cid
    fam = family_of(cid)
    cols = col_phrase(overlay)
    row_phrase, what, _ = buried_bits(overlay)
    lead = USED_LEAD[fam]
    close = USED_CLOSE[fam]

    mid: list[str] = []
    if cols:
        mid.append(f"不妨打开「{title}」，把「{cols}」填进对话框。")
    else:
        mid.append(f"不妨打开「{title}」，先对照第 4 节核对真实字段。")
    if what:
        loc = f"{row_phrase}附近" if row_phrase else "练习表里"
        mid.append(f"本集在{loc}埋了「{what}」，方便对着输出读。")
    else:
        scrap = _strip_used_shells(overlay.get("used_for") or "")
        scrap = re.sub(r"放不放行通常还要对照规程和规格。?", "", scrap)
        if 12 <= hn(scrap) <= 60 and not any(s in scrap for s in NEAR_SHELLS):
            mid.append(scrap if scrap.endswith("。") else scrap + "。")
        else:
            mid.append(f"这一课盯的是「{title}」现场里那一类读图/读表问题。")
    return ensure_min(lead + "".join(mid) + close, 40, "常见读法是先指着本课埋点说完，再和规程对一下。")


def warm_not_for(cid: str, overlay: dict) -> str:
    title = overlay.get("title") or cid
    fam = family_of(cid)
    existing = scrub_ban(overlay.get("not_for") or "")
    existing = re.sub(r"练习安排不等于放行样板[^。]*。?", "", existing)
    tips = {
        "msa": f"「{title}」回答量具波动，不替代过程能力或客户放行。",
        "cap": f"「{title}」要在相对稳定的数据上读；失控尖峰集不要硬拿来谈能力。",
        "qual": f"「{title}」是探索/归类线索，不能自动写成根因已判决。",
        "infer": f"「{title}」的 p 值或区间只说明和假设合不合，不等于已经放行。",
        "anova": f"「{title}」看组间差，不等于必须立刻改工艺或停线。",
        "regr": f"「{title}」的拟合好不等于因果已证明，也不等于放行。",
        "doe": f"「{title}」的显著效应不等于最优配方已放行。",
        "rel": f"「{title}」的风险摘要不等于批次已经放行。",
        "ts": f"「{title}」的平滑或分解不等于预测必须停线。",
        "util": f"「{title}」若没有练习表，不要把别的课的表硬套过来。",
        "graph": f"「{title}」是读图探索，不等于已经证明正态或可以放行。",
    }
    return ensure_min(f"{tips[fam]}{existing}{NOT_FOR_TAIL}", 30, "通常还要对照规程，把结论停在看见了什么。")


def warm_scenario(cid: str, overlay: dict) -> str:
    title = overlay.get("title") or cid
    cols = col_phrase(overlay)
    row_phrase, what, _ = buried_bits(overlay)
    existing = scrub_ban(overlay.get("scenario") or "")
    existing = re.sub(r"^现场要看「[^」]+」这一类问题。?", "", existing)
    existing = re.sub(r"^车间现场要看「[^」]+」这一类问题。?", "", existing)
    existing = re.sub(r"现场练习对着这条线看：?", "", existing)
    existing = re.sub(r"不妨先按第 4 节把 [^。]+填进对话框并运行。?", "", existing)
    existing = re.sub(r"不妨先把 [^。]+按第 4 节填进对话框并运行。?", "", existing)
    existing = re.sub(r"不妨先打开「[^」]+」[^。]*。?", "", existing)
    existing = re.sub(r"对照埋点[：—\-]?[^。]*。?", "", existing)
    existing = re.sub(r"停不停线、放不放行，可以等信号看清楚再和现场规程对一下。?", "", existing)
    existing = re.sub(r"大约第 \d+ 行附近：[^。]*。?", "", existing)
    existing = re.sub(r"。{2,}", "。", existing).strip("。").strip()

    empty = not (overlay.get("dialog_fill") or {}) and not any(overlay.get("buried_signals") or [])
    if empty or "暂时没有" in existing or "没有练习表" in existing or "锁表" in (overlay.get("scenario") or ""):
        col_bit = f"（例如 {cols}）" if cols else ""
        text = (
            f"这一课暂时可能没有可导入的练习表。"
            f"不妨先打开「{title}」，对照第 4 节核对对话框真实字段{col_bit}。"
            f"若菜单还没有，就只读帮助里的公式和边界——先分清适用场合；放行通常还要对照规程。"
        )
    else:
        # Keep only a short production scrap if it still smells like a line/measurement.
        scrap = existing if hn(existing) >= 8 and "这一类问题" not in existing else ""
        if scrap and not scrap.endswith("。"):
            scrap += "。"
        if not scrap:
            if cols and what:
                scrap = f"产线上对着「{cols}」采数；练习表里能看见「{what}」。"
            elif cols:
                scrap = f"测量/产线数据里有「{cols}」这些列，按第 4 节填进对话框。"
            else:
                scrap = f"打开「{title}」前，先核对手头列名是否对得上第 4 节。"
        invite = f"不妨先打开「{title}」"
        if cols:
            invite += f"，把 {cols} 填好并运行"
        invite += "。"
        buried_line = ""
        if what:
            loc = f"{row_phrase}：" if row_phrase else ""
            buried_line = f"对照埋点——{loc}{what}。"
        text = f"{scrap}{invite}{buried_line}停不停线、放不放行，可以等信号看清楚再和现场规程对一下。"
    return ensure_min(text, 40, "不妨先把本课埋点指清楚，再谈规程。")


def warm_meanings(overlay: dict) -> list:
    detail = overlay.get("dialog_fill_detail") or []
    out = []
    seen: set[str] = set()
    for item in detail:
        it = expand_meaning(dict(item))
        field = clean_field_label(it.get("field") or "")
        put = it.get("put") or ""
        meaning = scrub_ban(it.get("meaning") or "")
        for bad in (
            "软件才知道图上或表上",
            "对一下就行：这一项对应本课现场里",
            SHELL,
            "这样软件才知道要盯的是哪一列",
        ):
            if bad in meaning:
                meaning = ""
                break
        if hn(meaning) < 18:
            meaning = scrub_ban(expand_meaning({"field": field, "put": put, "meaning": ""})["meaning"])
        if put and put not in meaning and put not in ("留空", "留空（用策略）", "按菜单实填"):
            meaning = meaning.rstrip("。") + f"。本课「{field}」对应「{put}」。"
        elif field and field not in meaning:
            meaning = meaning.rstrip("。") + f"。这一项是对话框里的「{field}」。"
        if meaning in seen:
            meaning = meaning.rstrip("。") + f"。对一下「{field}」与「{put}」在本课现场的对应，勿与邻项混读。"
        n = 2
        base = meaning
        while meaning in seen:
            meaning = base.rstrip("。") + f"。（字段「{field}」第{n}处说明。）"
            n += 1
        seen.add(meaning)
        it["field"] = field
        it["meaning"] = meaning
        out.append(it)
    return out


def warm_buried(overlay: dict) -> list:
    out = []
    for b in overlay.get("buried_signals") or []:
        item = dict(b)
        what = scrub_ban(item.get("what") or "")
        item["what"] = what
        expect = scrub_ban(item.get("expect") or "")
        universal = (
            "常见读法是先说出看见了什么；放行或停线通常还要对照规程" in expect
            or "常见读法是对照「" in expect
            or hn(expect) < 16
            or any(x in (item.get("expect") or "") for x in ("禁止", "勿写"))
        )
        if universal:
            expect = (
                f"输出里对着「{what or '该埋点'}」读：先说出图上或表上哪一块在动。"
                "现场口语可以停在信号本身；不等于放行样板，通常还要对照规程。"
            )
        elif "通常还要" not in expect and "不等于" not in expect:
            expect = expect.rstrip("。") + "。不等于放行样板；通常还要对照规程。"
        item["expect"] = expect
        out.append(item)
    return out


def warm_output(overlay: dict) -> list:
    _, what, _ = buried_bits(overlay)
    out = []
    for g in overlay.get("output_guide") or []:
        if isinstance(g, str):
            item = {"name": overlay.get("title") or "输出", "meaning": scrub_ban(g)}
        elif isinstance(g, dict):
            item = dict(g)
        else:
            continue
        meaning = scrub_ban(item.get("meaning") or "")
        name = item.get("name") or "输出"
        if (
            hn(meaning) < 16
            or "禁止" in (item.get("meaning") or "")
            or "常见读法是先说出看见了什么；放行或停线通常还要对照规程" in meaning
            or (meaning.startswith("在「") and "对照第 5 节埋点" in meaning)
        ):
            if what:
                meaning = (
                    f"在「{name}」里指着「{what}」相关的那一块。"
                    "常见读法是先说出这一处看见了什么；不等于放行样板，通常还要对照规程。"
                )
            else:
                meaning = (
                    f"在「{name}」里对照第 5 节。"
                    "先说出本课图/表上最扎眼的那一处；不等于放行样板，通常还要对照规程。"
                )
        elif "常见读法" not in meaning and "不妨" not in meaning and "通常还要" not in meaning:
            meaning = meaning.rstrip("。") + "。先指着信号说完，通常还要对照规程。"
        item["name"] = name
        item["meaning"] = meaning
        out.append(item)
    if not out:
        tip = f"（本课埋点：{what}）" if what else ""
        out = [
            {
                "name": overlay.get("title") or "输出",
                "meaning": f"对照第 5 节读出看见了什么{tip}；不等于放行样板，通常还要对照规程。",
            }
        ]
    return out


def warm_mistakes(overlay: dict) -> list:
    out = []
    for m in overlay.get("common_mistakes") or []:
        s = scrub_ban(str(m))
        if hn(s) < 8:
            s = s.rstrip("。") + "。对一下列名有没有对调，结论不要写成已经放行。"
        out.append(s)
    if not out:
        out = [
            "漏选关键列或把列对调，图/表会对不准埋点。",
            "把练习信号直接写成已经放行或必须立刻停线；通常还要对照规程。",
        ]
    if not any("放行" in x or "规程" in x for x in out):
        out.append("练习安排不等于放行样板；通常还要对照规程。")
    return out


def warm_quiz_block(items: list, kind: str, title: str, overlay: dict) -> list:
    _, what, _ = buried_bits(overlay)
    cols = col_phrase(overlay)
    out = []
    for it in items or []:
        if not isinstance(it, dict):
            continue
        row = {k: scrub_ban(str(v)) if isinstance(v, str) else v for k, v in it.items()}
        if kind == "prereq":
            if hn(row.get("q") or "") < 10:
                row["q"] = f"学习「{title}」时，下面哪一句更符合本课要教的做法？"
            why = row.get("why") or ""
            if (
                hn(why) < 20
                or "配套练习表" in why
                or ("练习安排" in why and hn(why) < 36)
                or "常见读法是先说出看见了什么；放行或停线通常还要对照规程" in why
                or "摊开本课信号" in why
            ):
                tip = f"埋点「{what}」" if what else (f"列「{cols}」" if cols else "第 4–5 节")
                row["why"] = (
                    f"「{title}」用来对着{tip}读输出。"
                    "助教笔记：先说出看见了什么；放行或停线通常还要对照规程。"
                )
            for opt in ("good", "bad"):
                val = scrub_ban(row.get(opt) or "")
                raw = str(row.get(opt) or "")
                if "配套练习表" in raw or val.strip() in ("练习安排", "") or hn(val) < 6:
                    val = (
                        "拿别的课的表硬套，或把内部编号当答案背"
                        if opt == "bad"
                        else f"用本课练习安排学「{title}」，对着第 5 节读信号"
                    )
                row[opt] = val
        elif kind == "fade":
            st = scrub_ban(row.get("student") or "")
            if st and not st.startswith(("不妨", "导入", "仍", "再", "打开", "合上", "写")):
                st = "不妨" + st
            row["student"] = st
            if row.get("scaffold"):
                row["scaffold"] = scrub_ban(row["scaffold"])
        elif kind == "retrieval":
            if hn(row.get("q") or "") < 12:
                row["q"] = f"用自己的话说明：「{title}」主要帮你看什么？看完为什么还不等于可以放行？"
            hint = scrub_ban(row.get("hint") or "")
            if (
                hn(hint) < 16
                or "配套练习表" in hint
                or "禁止扩共享" in hint
                or "摊开本课信号" in hint
                or "常见读法是先说出看见了什么；不等于放行样板" in hint
            ):
                tip = f"指着「{what}」" if what else "对照第 5 节"
                hint = (
                    f"助教笔记：「{title}」{tip}说出看见了什么。"
                    "不等于放行样板，通常还要对照规程。"
                )
            row["hint"] = hint
        elif kind == "misc":
            row["wrong"] = scrub_ban(row.get("wrong") or "")
            right = scrub_ban(row.get("right") or "")
            if right.startswith("禁止") or hn(right) < 12 or "配套练习表" in right:
                right = (
                    "图和检验只提供线索。合格放行要对照规格和现场流程；"
                    "本课练习安排不等于放行样板。"
                )
            row["right"] = right
        out.append(row)
    return out


def warm_self_explain(items: list, title: str, overlay: dict) -> list:
    _, what, _ = buried_bits(overlay)
    cols = col_phrase(overlay)
    out = []
    for it in items or []:
        if not isinstance(it, dict):
            continue
        row = {k: scrub_ban(str(v)) if isinstance(v, str) else v for k, v in it.items()}
        hint = row.get("hint") or ""
        if (
            hn(hint) < 20
            or "配套练习表" in hint
            or ("对话框：" in hint and re.search(r"[a-z_]{3,}", hint))
            or "禁止扩共享" in hint
        ):
            tip = f"埋点「{what}」" if what else (f"字段「{cols}」" if cols else "第 4–5 节")
            row["hint"] = (
                f"助教笔记：菜单是「{title}」。对着{tip}读输出，"
                "把结论停在看见了什么；放行通常还要对照规程。"
            )
        out.append(row)
    return out


def warm_mission(overlay: dict, title: str) -> str:
    m = scrub_ban(overlay.get("skill_mission") or "")
    if hn(m) < 12 or "拒绝" in m or "禁止" in m:
        m = (
            f"能独立完成「{title}」，对着埋点读输出，"
            "并把结论停在「看见信号」；放行/停线通常还要对照规程。"
        )
    return m


def warm_click_steps(overlay: dict) -> list:
    return [scrub_ban(str(s)) for s in overlay.get("click_steps") or []]


def warm_glossary(overlay: dict) -> list:
    out = []
    for g in overlay.get("glossary") or []:
        if isinstance(g, dict):
            out.append({k: scrub_ban(str(v)) if isinstance(v, str) else v for k, v in g.items()})
    return out


def has_near_shell(overlay: dict) -> bool:
    blob = json.dumps(overlay, ensure_ascii=False)
    return any(s in blob for s in NEAR_SHELLS)


def already_good(overlay: dict) -> bool:
    blob = json.dumps(overlay, ensure_ascii=False)
    if any(b in blob for b in BAN):
        return False
    if has_near_shell(overlay):
        return False
    if SHELL in blob:
        return False
    # Incomplete / previous-generation wrappers still need a full rewrite.
    used = overlay.get("used_for") or ""
    scenario = overlay.get("scenario") or ""
    if "对照列名（如" in used or "现场练习对着这条线看" in scenario:
        return False
    if "帮你看清" in used or "不妨先按第 4 节把" in scenario or "不妨先把" in scenario and "按第 4 节填进对话框并运行" in scenario:
        return False
    if scenario.count("不妨先打开") >= 1 and "对照埋点——" in scenario and "产线上对着" not in scenario and "测量/产线" not in scenario and "还是那条" not in scenario:
        # Likely still carries an unfinished scrap head — rewrite once more.
        if "大约第" in scenario.split("不妨先打开")[0]:
            return False
    if hn(used) < 40:
        return False
    if hn(scenario) < 40:
        return False
    if hn(overlay.get("not_for") or "") < 30:
        return False
    if not re.search(r"不妨|常见读法|通常还要", blob):
        return False
    # used_for itself should invite or leave a red-line habit (not only scenario).
    if not re.search(r"不妨|通常还要|规程", used):
        return False
    meanings = [d.get("meaning") for d in (overlay.get("dialog_fill_detail") or [])]
    if len(meanings) >= 2 and len(set(meanings)) < len(meanings):
        return False
    return True


def apply_warmth(cid: str, overlay: dict) -> dict:
    o = deepcopy(overlay)
    title = o.get("title") or cid
    if cid in SKIP:
        return o
    if already_good(o):
        return o

    o["used_for"] = warm_used_for(cid, o)
    o["not_for"] = warm_not_for(cid, o)
    o["scenario"] = warm_scenario(cid, o)
    o["dialog_fill_detail"] = warm_meanings(o)
    o["buried_signals"] = warm_buried(o)
    o["output_guide"] = warm_output(o)
    o["common_mistakes"] = warm_mistakes(o)
    o["click_steps"] = warm_click_steps(o)
    # glossary is owned by glossary_bank — do not scrub-rewrite here
    o["skill_mission"] = warm_mission(o, title)
    o["prereq_quiz"] = warm_quiz_block(o.get("prereq_quiz") or [], "prereq", title, o)
    o["fade_levels"] = warm_quiz_block(o.get("fade_levels") or [], "fade", title, o)
    o["retrieval_quiz"] = warm_quiz_block(o.get("retrieval_quiz") or [], "retrieval", title, o)
    o["misconceptions"] = warm_quiz_block(o.get("misconceptions") or [], "misc", title, o)
    o["self_explain"] = warm_self_explain(o.get("self_explain") or [], title, o)

    def walk(x):
        if isinstance(x, str):
            return scrub_ban(x)
        if isinstance(x, list):
            return [walk(i) for i in x]
        if isinstance(x, dict):
            return {k: walk(v) for k, v in x.items()}
        return x

    o = walk(o)
    # Restore glossary from bank after deep scrub (bank is source of truth).
    try:
        from glossary_bank import glossary_for

        o["glossary"] = glossary_for(cid, o)
    except Exception:
        pass
    for key in ("used_for", "scenario", "not_for"):
        if not re.search(r"不妨|常见读法|通常还要", o.get(key) or ""):
            o[key] = (o.get(key) or "").rstrip("。") + "。通常还要对照规程。"
    return o


def audit(ids: list[str], name: str) -> dict:
    short = ban_n = shell_n = near_n = collab_n = 0
    for i in ids:
        d = json.loads((OVERLAY_DIR / f"{i}.json").read_text(encoding="utf-8"))
        blob = json.dumps(d, ensure_ascii=False)
        if any(b in blob for b in BAN):
            ban_n += 1
        if SHELL in blob:
            shell_n += 1
        if any(s in blob for s in NEAR_SHELLS):
            near_n += 1
        if re.search(r"不妨|常见读法|通常还要", blob):
            collab_n += 1
        if hn(d.get("used_for")) < 40 or hn(d.get("scenario")) < 40:
            short += 1
    print(f"{name}: n={len(ids)} collab={collab_n} ban={ban_n} shell={shell_n} near={near_n} short={short}")
    return {"collab": collab_n, "ban": ban_n, "shell": shell_n, "near": near_n, "short": short, "n": len(ids)}


def micro_scrub_overlay(overlay: dict) -> dict:
    """String scrub only — used for Wave-1 / imr contamination."""

    def walk(x):
        if isinstance(x, str):
            return scrub_ban(x)
        if isinstance(x, list):
            return [walk(i) for i in x]
        if isinstance(x, dict):
            return {k: walk(v) for k, v in x.items()}
        return x

    return walk(overlay)


def main() -> None:
    ok = 0
    for cid in ALL:
        if cid in SKIP:
            continue
        path = OVERLAY_DIR / f"{cid}.json"
        if not path.is_file():
            print("MISSING", cid)
            continue
        raw = json.loads(path.read_text(encoding="utf-8"))
        warmed = apply_warmth(cid, raw)
        path.write_text(json.dumps(warmed, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        ok += 1
    print(f"Warmed {ok} overlays (skipped {sorted(SKIP)})")

    # Wave-1 + between_within micro-scrub if residual shells remain
    from warmth_wave1_overlays import WAVE1_IDS

    scrubbed = 0
    for cid in list(WAVE1_IDS) + ["between_within_capability"]:
        path = OVERLAY_DIR / f"{cid}.json"
        if not path.is_file():
            continue
        raw = json.loads(path.read_text(encoding="utf-8"))
        blob = json.dumps(raw, ensure_ascii=False)
        if any(s in blob for s in NEAR_SHELLS):
            cleaned = micro_scrub_overlay(raw)
            path.write_text(json.dumps(cleaned, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
            scrubbed += 1
    print(f"Wave-1/handcrafted micro-scrub: {scrubbed}")

    imr = json.loads((OVERLAY_DIR / "imr.json").read_text(encoding="utf-8"))
    blob = json.dumps(imr, ensure_ascii=False)
    assert "41" in blob and "55" in blob
    assert "UCL" in blob and "USL" in blob
    assert len(imr.get("dialog_fill_detail") or []) >= 9
    assert "本课只练" not in blob
    print("imr gold intact")

    r2 = audit(WAVE2, "W2")
    r3 = audit(WAVE3, "W3")
    r4 = audit(WAVE4, "W4")
    if r2["ban"] or r3["ban"] or r4["ban"] or r2["shell"] or r3["shell"] or r4["shell"]:
        raise SystemExit("audit failed: ban/shell remaining")
    if r2["near"] or r3["near"] or r4["near"]:
        raise SystemExit("audit failed: near-shell remaining")
    if r2["short"] or r3["short"] or r4["short"]:
        raise SystemExit("audit failed: short remaining")
    if r2["collab"] < r2["n"] or r3["collab"] < r3["n"] or r4["collab"] < r4["n"]:
        raise SystemExit("audit failed: collab incomplete")
    print("AUDIT PASS")


if __name__ == "__main__":
    main()
