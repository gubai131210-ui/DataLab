"""Shared copy-depth helpers: readable 7+, short 0–6, graph name/fill/related.

Import from wave*_content.py write_overlays, or run:
    python tools/learning_data/copy_depth.py
"""
from __future__ import annotations

import json
import re
from pathlib import Path

from glossary_bank import glossary_for

HERE = Path(__file__).resolve().parent
OVERLAY_DIR = HERE / "tutorial_overlays"
HANZI = re.compile(r"[\u4e00-\u9fff]")
ID_LIKE = re.compile(r"^[a-z][a-z0-9_]+$")

SPC_COMMANDS = frozenset(
    {
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
)

RELATED_BY_ID = {
    "histogram": ["probability_plot", "boxplot"],
    "probability_plot": ["histogram", "normality_test"],
    "density_plot": ["histogram", "ecdf_plot"],
    "ecdf_plot": ["density_plot", "histogram"],
    "boxplot": ["violin_plot", "interval_plot"],
    "violin_plot": ["boxplot", "density_plot"],
    "bar_chart": ["pie_plot", "mosaic_plot"],
    "pie_plot": ["bar_chart", "mosaic_plot"],
    "area_plot": ["time_series_plot"],
    "time_series_plot": ["area_plot", "time_series_smoothing"],
    "scatter_plot": ["hexbin_plot", "marginal_plot"],
    "hexbin_plot": ["scatter_plot", "heatmap_plot"],
    "interval_plot": ["boxplot", "one_way_anova"],
    "bubble_plot": ["scatter_plot"],
    "matrix_plot": ["scatter_plot", "correlation_plot"],
    "correlation_plot": ["heatmap_plot", "matrix_plot"],
    "heatmap_plot": ["correlation_plot", "correlogram"],
    "marginal_plot": ["scatter_plot", "histogram"],
    "parallel_plot": ["matrix_plot"],
    "contour_plot": ["scatter_plot"],
    "mosaic_plot": ["pie_plot", "chi_square_mosaic_link"],
    "eda_4plot": ["histogram", "probability_plot", "time_series_plot"],
    "simplex_design_plot": ["mixture_design", "mixture_analyze"],
    "graph_gallery": ["histogram", "scatter_plot"],
    "correlogram": ["heatmap_plot", "correlation_plot"],
    "dotplot": ["histogram", "boxplot"],
    "chi_square_mosaic_link": ["mosaic_plot", "chi_square"],
}

FILL_PATCH = {
    "scatter_plot": {"x_variable": "温度_C", "y_variable": "偏移_um"},
    "interval_plot": {"response": "厚度_um", "category": "腔号"},
    "bubble_plot": {
        "x_variable": "X坐标",
        "y_variable": "Y坐标",
        "size_variable": "尺寸",
    },
    "hexbin_plot": {"x_variable": "X坐标", "y_variable": "Y坐标"},
    "marginal_plot": {"x_variable": "X值", "y_variable": "Y值"},
    "time_series_plot": {"time": "周次", "value": "产量"},
    "area_plot": {"time": "周次", "value": "产量"},
    "contour_plot": {
        "x_variable": "X因子",
        "y_variable": "Y因子",
        "z_variable": "响应Z",
    },
}

TITLE_PATCH = {
    "interval_plot": "区间散点图",
    "probability_plot": "概率图——正态参考线",
    "graph_gallery": "探索性图形",
}

ICON_NOTE = {
    "bar_chart": "软件里这条菜单的图标可能看起来像 Pareto；左侧树名称仍是「条形图」，不要当成 Pareto 分析。",
    "eda_4plot": "图标可能像直方图，但本课是四宫格（游程/直方/概率/滞后），不是「再画一张直方图」。",
    "density_plot": "图标可能像直方图；菜单名称以左侧树「密度图」为准。",
    "dotplot": "图标可能像直方图；菜单名称以左侧树「点图」为准。",
    "violin_plot": "图标可能像箱线图；本课看分布形状，菜单仍是小提琴图。",
    "hexbin_plot": "图标可能像散点；本课是二维分箱密度。",
    "graph_gallery": "图标可能像散点；本课是探索性图形入口，不是散点图菜单。",
    "simplex_design_plot": "图标可能像混料设计；本课出的是单纯形图，设计命令是另一课。",
    "mosaic_plot": "图标可能像条形图；本课看两个分类的交叉占比。",
    "correlogram": "图标可能像热图；菜单名是 Correlogram（相关热图），不要和「热图」菜单当成同一个。",
    "chi_square_mosaic_link": "图标可能像卡方检验；本课是卡方与马赛克的联动入口。",
}

DEV_WORDS = ("同构", "白名单", "WAVE", "Wave-", "role_map", "command_id")

TEMPLATE_RELATED = ["histogram", "scatter_plot", "graph_gallery"]


def hanzi_n(text: str) -> int:
    return len(HANZI.findall(text or ""))


def worksheet_name(overlay: dict) -> str | None:
    for step in overlay.get("click_steps") or []:
        found = re.search(r"demo_[a-z0-9_]+", step)
        if found:
            return found.group(0)
    scenario = overlay.get("scenario") or ""
    found = re.search(r"demo_[a-z0-9_]+", scenario)
    if found:
        return found.group(0)
    return None


def fill_desc(overlay: dict) -> str:
    fill = overlay.get("dialog_fill") or {}
    if not fill:
        return "本课对话框没有必须预填的列，按第 4 节字段说明操作。"
    parts = [f"{key} 填「{value}」" for key, value in fill.items()]
    return "；".join(parts)


def buried_line(overlay: dict) -> tuple[str, str]:
    buried = overlay.get("buried_signals") or []
    if not buried:
        return "（本课没有行号埋点）", "对照第 5 节读输出，不要写成过程合格。"
    first = buried[0]
    row = first.get("row")
    what = first.get("what") or ""
    expect = first.get("expect") or ""
    return f"大约第 {row} 行：{what}", expect


def scrub(text: str) -> str:
    out = text or ""

    def repl(match: re.Match[str]) -> str:
        inner = match.group(1)
        if inner.startswith("demo_"):
            return match.group(0)
        return "配套练习表"

    out = re.sub(r"`([a-z][a-z0-9_]+)`", repl, out)
    out = out.replace("同构共享", "可以共用同一张练习表")
    out = out.replace("同构", "同一张练习表")
    out = out.replace("白名单", "")
    out = studentize_text(out)
    out = re.sub(r"\s{2,}", " ", out).strip()
    return out


def studentize_text(text: str) -> str:
    out = text or ""
    replacements = (
        ("本波锁表诚实为空", "这一课暂时没有配套练习表"),
        ("本波不提供演示表", "这一课暂时没有配套练习表"),
        ("勿把旧共享表硬挂过来", "不要把别的课的表硬套过来"),
        ("勿挂旧10表或 demo_ 前缀", "不要拿以前的练习表顶替，工作表名字也不要乱加前缀"),
        ("锁表 dataset 空。", ""),
        ("锁表 dataset 空", "暂时没有可导入的练习表"),
        ("本波 dataset 空", "本课没有练习表"),
        ("dataset 空", "没有练习表"),
        ("analysis_commands", "软件菜单"),
        ("roles/inputs", "对话框字段"),
        ("（配套练习表）", ""),
        ("(配套练习表)", ""),
    )
    for old, new in replacements:
        out = out.replace(old, new)
    return out


def clean_field_label(field: str) -> str:
    field = field or ""
    field = re.sub(r"\s*\(`[^`]+`\)", "", field)
    field = field.replace(" (配套练习表)", "").replace("（配套练习表）", "")
    return field.strip()


def expand_meaning(item: dict) -> dict:
    field = clean_field_label(item.get("field") or "")
    item["field"] = field
    put = item.get("put") or ""
    meaning = (item.get("meaning") or "").replace(" (配套练习表)", "").replace("（配套练习表）", "")
    if (
        "无演示表" in put
        or "dataset 空" in meaning
        or "本波" in meaning
        or "analysis_commands" in meaning
        or put.startswith("（")
    ):
        item["put"] = "按菜单实填"
        item["meaning"] = (
            f"「{field}」在软件对话框里有这一项。"
            "本课没有预填练习表，打开菜单后按你手头的列来填，不要虚构没有的框。"
        )
        return item
    if hanzi_n(meaning) >= 12:
        item["meaning"] = scrub(meaning)
        return item
    if put in ("留空", "留空（用策略）", "无需导入"):
        extra = (
            f"「{field}」本课故意不填：填了会改变图上的中心或分组，第 5 节要对着看的信号就对不准。"
            if "阶段" in field or "历史" in field or "分面" in field or "分组" in field
            else f"「{field}」本课留空即可；有需要时再按现场情况补。"
        )
    elif put:
        extra = f"在对话框里把「{put}」填进「{field}」。这一项决定图上或表上对应哪一列。"
    else:
        extra = f"「{field}」按第 4 节说明填写，不要虚构软件里没有的字段。"
    item["meaning"] = scrub((meaning.rstrip("。") + "。" + extra).strip())
    return item


def polish_student_copy(cid: str, overlay: dict) -> dict:
    title = overlay.get("title") or cid
    empty = not (overlay.get("dialog_fill") or {}) and not overlay.get("dataset_id")
    if empty:
        overlay["used_for"] = (
            f"弄清菜单「{title}」什么时候用、对话框里有哪些真实字段。"
            "当前版本可能还没有配套练习表。"
        )
        overlay["not_for"] = (
            "不要把别的课的练习表硬套过来，也不要写成过程合格、必须停线或已证明正态。"
        )
        overlay["scenario"] = (
            f"这一课暂时没有可导入的练习表。若软件里能打开「{title}」，"
            "对照第 4 节核对字段；若菜单还没有，就只读帮助里的公式和边界。"
        )
        overlay["click_steps"] = [
            f"打开「帮助」→「学习中心」，选择本教程「{title}」。",
            "这一课暂时没有配套练习表，不要把别的课的表硬套过来。",
            "若软件里能打开该菜单，对照第 4 节核对对话框里真实有的字段。",
            "若菜单还没有或只是公式参考，就去「算法、公式与参考资料」读边界。",
            "看完不要写成过程合格、必须停线或已证明正态。",
        ]
        return overlay
    for key in ("used_for", "not_for", "scenario"):
        if overlay.get(key):
            overlay[key] = studentize_text(overlay[key])
    overlay["click_steps"] = [studentize_text(step) for step in overlay.get("click_steps") or []]
    return overlay


def pad_option(text: str, title: str, *, good: bool) -> str:
    text = scrub(str(text))
    if ID_LIKE.match(text):
        if good:
            return f"用本课配套的练习工作表来学「{title}」，不要拿内部编号当答案背"
        return "另做一张什么算法都能跑的大宽表，把所有课混在一起"
    if hanzi_n(text) < 8:
        if good:
            return f"{text}——这才是学「{title}」时要抓住的点"
        return f"{text}——这是常见误读，本课不要这样写"
    return text


def pad_question(text: str, title: str) -> str:
    text = scrub(str(text))
    if hanzi_n(text) >= 12 and "？" in text:
        return text if text.endswith("？") else text + "？"
    return f"学习「{title}」时，下面哪一句更符合本课要教的做法？"


def why_for(title: str, overlay: dict, is_spc: bool) -> str:
    used = scrub(overlay.get("used_for") or "")
    not_for = scrub(overlay.get("not_for") or "")
    if not is_spc:
        not_for = not_for.replace("UCL", "控制限").replace("LCL", "控制限")
    if is_spc:
        return (
            f"「{title}」用来看过程有没有特殊原因线索。"
            f"{used[:80]}控制限（UCL/LCL）是过程自己的警戒线，不是客户规格（USL/LSL）。"
            f"不要把点出控制限写成废品或必须停线。"
        )
    return (
        f"「{title}」用来{used[:60] or '练习本菜单的读图/读表'}。"
        f"看完输出不要写成过程合格。{not_for[:80]}"
    )


def make_seven_plus(cid: str, overlay: dict) -> dict:
    title = overlay.get("title") or cid
    is_spc = cid in SPC_COMMANDS
    sheet = worksheet_name(overlay)
    fill = fill_desc(overlay)
    buried_what, buried_expect = buried_line(overlay)
    used = scrub(overlay.get("used_for") or "")
    not_for = scrub(overlay.get("not_for") or "")
    if not is_spc:
        not_for = not_for.replace("UCL", "控制限").replace("LCL", "控制限")
        buried_what = buried_what.replace("UCL", "控制限").replace("LCL", "控制限")
        buried_expect = buried_expect.replace("UCL", "规格").replace("LCL", "规格")

    if sheet:
        q_data_good = (
            f"导入后左上角工作表显示名是 {sheet}（这只是表名，不是另一张图的名字）"
        )
        q_data_bad = "必须把内部编号当成正确答案背下来，否则就算没学会"
        q_data = (
            f"导入「{title}」的练习数据之后，你应该怎样认出这张工作表？"
        )
        fade_ds = sheet
    else:
        q_data = f"「{title}」这一课为什么常常没有练习表可导入？"
        q_data_good = "当前版本可能只是公式参考或设计生成，步骤会如实写明；不要虚构一张旧宽表硬跑"
        q_data_bad = "随便挂一张以前的大宽表，假装已经练过"
        fade_ds = None

    if is_spc:
        q3 = f"在「{title}」上，UCL（上控制限）和客户规格上限是一回事吗？"
        g3 = "不是。UCL 是过程警戒线，USL 才是规格；点出 UCL 不能写成废品"
        b3 = "是的，点一出 UCL 就等于超规格报废"
    else:
        q3 = f"把「{title}」画完或算完，能不能写成「过程已经合格」？为什么？"
        g3 = "不能。图和检验只给线索，合格放行要另按规格和流程判断"
        b3 = "可以，只要图好看或 p 值很小就证明过程合格"

    prereq = [
        {
            "q": f"「{title}」这一课主要想让你学会看什么、不要把它当成什么？",
            "good": pad_option(
                used.split("。")[0] if used else f"按菜单学会「{title}」的读图或读表",
                title,
                good=True,
            ),
            "bad": pad_option(
                not_for.split("。")[0] if not_for else "当成放行合格或已经证明分布的证据",
                title,
                good=False,
            ),
            "why": why_for(title, overlay, is_spc),
        },
        {
            "q": q_data,
            "good": q_data_good,
            "bad": q_data_bad,
            "why": (
                f"练习表是为这一课埋信号准备的。"
                f"{'导入后看左上角表名，它只是工作表显示名。' if sheet else '没有表时不要编造列。'}"
                f"不要把内部编号当成考题答案。"
            ),
        },
        {
            "q": q3,
            "good": g3,
            "bad": b3,
            "why": why_for(title, overlay, is_spc),
        },
    ]

    hint_import = (
        f"本课菜单是「{title}」。{used or '按第 1–5 节把图或表跑通。'}。"
        f"{'请导入 ' + sheet + '。这是工作表显示名，解释一次即可，以后用菜单中文说话。' if sheet else '本课没有演示表，对照第 4 节字段，不要虚构列名。'}"
        f"对话框：{fill}。"
        f"读输出时对照：{buried_what}。{buried_expect}"
        f"不要写成过程合格、必须停线或已证明正态。"
    )
    hint_read = (
        f"第 5 节要对着看的是：{buried_what}。"
        f"{buried_expect}"
        f"如果你写的是「已经合格 / 必须停线 / 已证明正态」，就偏了。"
        + (
            "控制限课请把 UCL 和 USL 分开说。"
            if is_spc
            else "本课不是控制图，不要把图形尺度当成规格或放行结论。"
        )
    )

    self_explain = [
        {
            "after": "导入演示工作表之后" if sheet else "读完第 4 节字段清单之后",
            "prompt": (
                f"为什么「{title}」要用现在这张练习安排，而不是一张啥算法都能跑的大宽表？"
                if sheet
                else f"「{title}」没有练习表时，你怎样避免虚构菜单字段？"
            ),
            "hint": hint_import,
        },
        {
            "after": "对着第 5 节读完输出之后",
            "prompt": f"看完「{title}」的输出，哪句话你绝对不能写进报告？为什么？",
            "hint": hint_read,
        },
    ]

    if fade_ds:
        fade = [
            {
                "level": 0,
                "student": f"导入 {fade_ds}，按第 4 节填对话框并运行，对着第 5 节指出埋点；每一步写下自解释。",
                "scaffold": "参数表、读图话术和埋点全部给出。",
            },
            {
                "level": 1,
                "student": f"仍导入 {fade_ds}。最后请你自己写三句：埋点有没有出现、你为什么不能写成过程合格。",
                "scaffold": "对话框字段仍给出；不给标准答案。",
            },
            {
                "level": 2,
                "student": f"再导入 {fade_ds}，自己改一个可选项（例如分组或规格留空），不看埋点剧透写结论。",
                "scaffold": "仅术语表和误用纠正。",
            },
        ]
    else:
        fade = [
            {
                "level": 0,
                "student": f"打开「{title}」教程，对照第 4 节把真实字段抄一遍，并写清当前版本可能只是公式参考。",
                "scaffold": "字段清单和公式帮助路径。",
            },
            {
                "level": 1,
                "student": f"合上第 4 节，用自己的话写出「{title}」对话框里你记得的字段，以及不能写进报告的误读。",
                "scaffold": "只给菜单路径。",
            },
            {
                "level": 2,
                "student": "写一段：本课为何没有练习表，以及你会怎样避免虚构字段。",
                "scaffold": "仅本课任务句。",
            },
        ]

    retrieval = [
        {
            "q": f"用自己的话说明：「{title}」主要帮你看什么？看完为什么不能写成过程合格？",
            "hint": why_for(title, overlay, is_spc),
        },
        {
            "q": f"本课读输出时，你应该指着哪一处线索？{buried_what} 对不对得上第 5 节？",
            "hint": f"{buried_what}。期望：{buried_expect}",
        },
        {
            "q": f"什么情况下不该用「{title}」？请写完整句子。",
            "hint": not_for or "不要用本课去替代规格放行、停线命令或正态性证明。",
        },
    ]

    misc = [
        {
            "wrong": f"把「{title}」做完就等于过程合格，可以放行。",
            "right": "图和检验只提供线索。合格放行要对照规格和现场流程，本课练习表不能当客户证据。",
        },
        {
            "wrong": "例题点过一遍、不写自解释也算学会了。",
            "right": "还要自己写原因、做褪脚手架和合上教程的检索题，记忆才留得住。",
        },
    ]
    if is_spc:
        misc.insert(
            0,
            {
                "wrong": "点落在 UCL 外面就等于产品超规格、该报废。",
                "right": "UCL 是过程警戒线；超规格要看 USL 或能力分析。UCL 不等于 USL。",
            },
        )

    mission = f"能独立完成「{title}」，对着埋点读输出，并且拒绝过程合格、必须停线这类话术。"

    return {
        "skill_mission": mission,
        "prereq_quiz": prereq,
        "self_explain": self_explain,
        "fade_levels": fade,
        "retrieval_quiz": retrieval,
        "misconceptions": misc,
    }


def apply_graph_fixes(cid: str, overlay: dict) -> dict:
    if cid in TITLE_PATCH:
        overlay["title"] = TITLE_PATCH[cid]
    if cid in FILL_PATCH:
        fill = dict(overlay.get("dialog_fill") or {})
        fill.update(FILL_PATCH[cid])
        overlay["dialog_fill"] = fill
    related = overlay.get("related_ids") or []
    if cid in RELATED_BY_ID:
        overlay["related_ids"] = list(RELATED_BY_ID[cid])
    elif related == TEMPLATE_RELATED:
        overlay["related_ids"] = ["descriptive"] if cid != "graph_gallery" else ["histogram", "scatter_plot"]
    note = ICON_NOTE.get(cid)
    if note:
        used = overlay.get("used_for") or ""
        if "图标" not in used:
            overlay["used_for"] = (used.rstrip("。") + "。" + note).strip()
    if cid == "interval_plot":
        overlay["click_steps"] = [
            step.replace("图形 → 区间图", "图形 → 区间散点图").replace("练习「区间图」", "练习「区间散点图」")
            for step in overlay.get("click_steps") or []
        ]
        overlay["used_for"] = (overlay.get("used_for") or "").replace("「区间图」", "「区间散点图」")
    if cid == "bar_chart":
        overlay["title"] = "条形图"
    return overlay


def deep_scrub_obj(value):
    if isinstance(value, str):
        return scrub(value)
    if isinstance(value, list):
        return [deep_scrub_obj(item) for item in value]
    if isinstance(value, dict):
        return {key: deep_scrub_obj(item) for key, item in value.items()}
    return value


def polish_overlay(cid: str, overlay: dict) -> dict:
    overlay = dict(overlay)
    if cid != "imr":
        overlay = apply_graph_fixes(cid, overlay)
        if overlay.get("used_for"):
            overlay["used_for"] = scrub(overlay["used_for"])
            if "用专用集" in overlay["used_for"]:
                title = overlay.get("title") or cid
                overlay["used_for"] = (
                    f"练习菜单「{title}」：导入配套工作表后按第 4 节填对话框，对照第 5 节读输出。"
                    f"不要把它当成放行或分布证明。"
                    + (ICON_NOTE.get(cid, ""))
                )
        if overlay.get("not_for"):
            overlay["not_for"] = scrub(overlay["not_for"])
        if overlay.get("scenario"):
            overlay["scenario"] = scrub(overlay["scenario"])
        overlay["dialog_fill_detail"] = [
            expand_meaning(dict(item)) for item in overlay.get("dialog_fill_detail") or []
        ]
        overlay["glossary"] = glossary_for(cid, overlay)
        overlay = polish_student_copy(cid, overlay)
        overlay.update(make_seven_plus(cid, overlay))
        overlay = deep_scrub_obj(overlay)
        overlay["glossary"] = glossary_for(cid, overlay)
        common = overlay.get("common_mistakes") or []
        overlay["common_mistakes"] = [
            scrub(x).replace("不对照 analysis_commands 的 roles/inputs", "不要虚构软件里没有的对话框字段")
            for x in common
        ]
    return overlay


def rewrite_all_overlays() -> int:
    count = 0
    for path in sorted(OVERLAY_DIR.glob("*.json")):
        cid = path.stem
        overlay = json.loads(path.read_text(encoding="utf-8"))
        polished = polish_overlay(cid, overlay)
        path.write_text(json.dumps(polished, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        count += 1
    return count


if __name__ == "__main__":
    n = rewrite_all_overlays()
    print(f"Rewrote {n} overlays in {OVERLAY_DIR}")
