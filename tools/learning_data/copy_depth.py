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
        return "（本课没有行号埋点）", "对照第 5 节读输出；现场口语可以停在「看见了什么」，放行通常还要对照规程。"
    first = buried[0]
    row = first.get("row")
    what = first.get("what") or ""
    expect = first.get("expect") or ""
    return f"大约第 {row} 行：{what}", expect


def _title_lookup() -> dict[str, str]:
    """Lazy map command_id → Chinese title for scrubbing backtick ids."""
    cache: dict[str, str] = getattr(_title_lookup, "_cache", {})
    if cache:
        return cache
    if OVERLAY_DIR.is_dir():
        for path in OVERLAY_DIR.glob("*.json"):
            try:
                data = json.loads(path.read_text(encoding="utf-8"))
            except (OSError, json.JSONDecodeError):
                continue
            title = str(data.get("title") or "").strip()
            if title:
                cache[path.stem] = title
    setattr(_title_lookup, "_cache", cache)
    return cache


def scrub(text: str) -> str:
    out = text or ""

    def repl(match: re.Match[str]) -> str:
        inner = match.group(1)
        if inner.startswith("demo_"):
            return match.group(0)
        title = _title_lookup().get(inner)
        if title:
            return f"「{title}」那一课"
        return "另一课的练习安排"

    out = re.sub(r"`([a-z][a-z0-9_]+)`", repl, out)
    out = out.replace("同构共享", "可以对照相关课的练习安排")
    out = out.replace("同构", "相关课")
    out = out.replace("白名单", "")
    out = out.replace("禁止扩共享", "不要把别的课的表硬套过来")
    # Repair older scrub residue where command ids became bare「练习表」.
    out = re.sub(r"禁止与\s*练习表\s*共享", "不要和别的控制图课硬共用同一张表", out)
    out = re.sub(r"禁止复用\s*练习表", "不要拿别的课的表硬套过来", out)
    out = re.sub(r"禁止挂\s*练习表", "不要拿别的课的表硬套过来", out)
    out = re.sub(r"禁止与\s*练习安排\s*共享", "不要和别的课硬共用同一张表", out)
    out = re.sub(r"用\s*练习表", "用对应专项课的练习安排", out)
    out = re.sub(r"方差专项用\s*对应专项课的练习安排", "方差专项请走方差检验那一课", out)
    out = re.sub(r"等价用\s*对应专项课的练习安排", "等价性请走等价性检验那一课", out)
    out = re.sub(r"位置差用\s*对应专项课的练习安排", "位置差请走双样本 t 那一课", out)
    out = re.sub(r"显著差用\s*对应专项课的练习安排", "显著差请走两比例检验那一课", out)
    out = re.sub(r"单率用\s*对应专项课的练习安排", "单率请走单样本泊松率那一课", out)
    out = re.sub(r"单因子用\s*对应专项课的练习安排", "单因子请走单因素 ANOVA 那一课", out)
    out = re.sub(r"差等价用\s*对应专项课的练习安排", "差值等价请走双样本等价性那一课", out)
    out = studentize_text(out)
    out = re.sub(r"\s{2,}", " ", out).strip()
    return out


def studentize_text(text: str) -> str:
    out = text or ""
    replacements = (
        ("本波锁表诚实为空", "这一课暂时没有可导入的练习表"),
        ("本波不提供演示表", "这一课暂时没有可导入的练习表"),
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
        ("配套练习表", "练习安排"),
        ("这一项决定图上或表上对应哪一列。", ""),
        ("这一项决定图上或表上对应哪一列", ""),
        ("软件才知道图上或表上评价的是这一列。", ""),
        ("软件才知道图上或表上评价的是这一列", ""),
        ("软件才知道图上或表上对应哪一列。", ""),
        ("软件才知道图上或表上对应哪一列", ""),
        ("本课要练的那一类信号", ""),
        ("这一课要对着看的那类信号", ""),
        ("这一课要对着看的信号", ""),
        ("它更像在摊开「看见了什么」，而不是直接回答能不能放行。", ""),
        ("它更像在摊开「看见了什么」；放不放行通常还要对照规程和规格。", ""),
        ("它更像在摊开「看见了什么」", ""),
        ("摊开「看见了什么」", "读出看见了什么"),
        ("常见读法是先说出看见了什么；放行或停线通常还要对照规程。", ""),
        ("常见读法是先说出看见了什么；不等于放行样板，通常还要对照规程。", ""),
        ("对一下就行：这一项对应本课现场里", "本课现场里这一项对应"),
        ("本课只练", "本课主要看"),
        ("这一课只练", "这一课主要看"),
        ("禁止过程合格。", "不等于放行样板；通常还要对照规程。"),
        ("禁止过程合格", "不等于放行样板"),
        ("禁止已证明正态。", "不等于已经证明正态。"),
        ("禁止已证明正态", "不等于已经证明正态"),
        ("禁止必须停线。", "停线通常还要对照规程。"),
        ("禁止必须停线", "停线通常还要对照规程"),
        ("抖主要", "波动主要落在"),
        ("你的任务是", "不妨先"),
        ("你应该能", "常见读法是"),
        ("写成过程合格、必须停线或已证明正态", "写成已经放行、必须立刻停线或已经证明正态"),
        ("不要写成过程合格", "不要写成已经放行"),
        ("拒绝过程合格、必须停线这类话术", "把结论停在「看见信号」，放行/停线通常还要对照规程"),
        ("并且拒绝过程合格、必须停线这类话术", "并能把结论停在「看见信号」"),
        ("禁止句", "红线习惯"),
        ("过程合格？", "能不能直接写成放行？"),
        ("禁止写", "通常还不写到放行"),
        ("图画完=过程合格", "图画完就等于可以放行"),
        ("禁止用 配套练习表", "不要拿别的课的练习安排硬套"),
        ("禁止用配套练习表", "不要拿别的课的练习安排硬套"),
        ("禁止用 练习安排", "不要拿别的课的练习安排硬套"),
        ("禁止扩共享", "不要把别的课的表硬套过来"),
        ("禁止与 练习安排 共享", "不要和别的课硬共用同一张表"),
        ("禁止复用 练习安排", "不要拿别的课的表硬套过来"),
        ("（禁止用 ", "（不要拿 "),
        ("WAVE", "本课"),
        ("Wave-", "本课-"),
    )
    for old, new in replacements:
        out = out.replace(old, new)
    out = re.sub(r"。{2,}", "。", out)
    out = re.sub(r"\s{2,}", " ", out).strip()
    return out


def clean_field_label(field: str) -> str:
    field = field or ""
    field = re.sub(r"\s*\(`[^`]+`\)", "", field)
    field = field.replace(" (配套练习表)", "").replace("（配套练习表）", "")
    return field.strip()


def _role_aware_meaning(field: str, put: str) -> str:
    """Short collaborative meaning keyed by dialog role / column, not a one-line shell."""
    f = field or ""
    p = put or ""
    if any(k in f for k in ("变量", "测量", "响应", "厚度", "高度", "宽度", "长度", "尺寸", "Y")):
        return (
            f"「{p}」是量出来的那个数。"
            f"对话框里把它放进「{f}」，后面的图和表才会盯这一列。"
        )
    if any(k in f for k in ("子组大小", "样本大小", "n=")):
        return (
            f"「{p}」是每个子组里有几件。"
            f"填进「{f}」要和表里实际抽检件数一致，否则控制限或能力算法会对不准。"
        )
    if any(k in f for k in ("子组",)):
        return (
            f"「{p}」告诉软件哪几行算一批。"
            f"放进「{f}」后，批内和批间才拆得开；漏了这一列，组间/组内就会糊在一起。"
        )
    if any(k in f for k in ("缺陷数", "不合格", "不良")):
        return (
            f"「{p}」是本课的计数列。"
            f"填进「{f}」后，图上每个点才对应这一批（或这一单位）的计数。"
        )
    if any(k in f for k in ("检验数", "单位数", "样本量", "样本数")):
        return (
            f"「{p}」是分母或单位大小。"
            f"填进「{f}」后，限宽才会跟着样本量/单位数变化；别和缺陷数列对调。"
        )
    if any(k in f for k in ("分组", "型号", "阶段")) and p not in ("留空", "留空（用策略）"):
        return (
            f"「{p}」用来告诉软件按哪一列分段或分组。"
            f"填进「{f}」后，标准化或分阶段中心才会按组各自估。"
        )
    if any(k in f for k in ("LSL", "USL", "规格", "Target", "目标")):
        return (
            f"「{p}」是规格或目标侧的数字。"
            f"填进「{f}」是为了和过程波动对照；它不是控制限 UCL/LCL。"
        )
    if any(k in f for k in ("Lambda", "λ", "Alpha", "平滑", "权重", "窗", "跨度")):
        return (
            f"把「{p}」填进「{f}」，等于告诉软件近期点有多重。"
            "改大一点更像普通均值图，改小一点更平滑、反应更慢。"
        )
    if any(k in f for k in ("规则", "测试", "tests", "Nelson")):
        return (
            f"「{f}」取「{p}」时，特殊原因规则按本课教学默认走。"
            "规则开得越多越灵敏，也越容易误报；对照第 5 节埋点读即可。"
        )
    if any(k in f for k in ("历史",)):
        return (
            f"「{f}」本课用「{p}」：先让练习表自己估中心和限宽。"
            "填了历史参数等于换尺子，第 5 节埋点可能对不准。"
        )
    return (
        f"对话框里把「{p}」填进「{f}」。"
        f"对照第 4 节核对手里的列名，别和邻格对调。"
    )


def expand_meaning(item: dict) -> dict:
    field = clean_field_label(item.get("field") or "")
    item["field"] = field
    put = item.get("put") or ""
    meaning = (item.get("meaning") or "").replace(" (配套练习表)", "").replace("（配套练习表）", "")
    meaning = meaning.replace("这一项决定图上或表上对应哪一列。", "").replace(
        "这一项决定图上或表上对应哪一列", ""
    )
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
    if "子组大小" in field and "批内和批间" in meaning:
        meaning = ""
    if hanzi_n(meaning) >= 18 and "决定图上或表上" not in meaning:
        item["meaning"] = scrub(meaning)
        return item
    if put in ("留空", "留空（用策略）", "无需导入"):
        extra = (
            f"「{field}」本课故意不填：填了会改变图上的中心或分组，第 5 节要对着看的信号就对不准。"
            if "阶段" in field or "历史" in field or "分面" in field or "分组" in field
            else f"「{field}」本课留空即可；有需要时再按现场情况补。"
        )
    elif put:
        extra = _role_aware_meaning(field, put)
    else:
        extra = f"「{field}」按第 4 节说明填写，不要虚构软件里没有的字段。"
    base = meaning.strip().rstrip("。")
    if base and hanzi_n(base) >= 6:
        # Keep short field-specific lead-in, then role-aware sentence(s).
        combined = f"{base}。{extra}"
    else:
        combined = extra
    item["meaning"] = scrub(combined.strip())
    return item


def polish_student_copy(cid: str, overlay: dict) -> dict:
    title = overlay.get("title") or cid
    empty = not (overlay.get("dialog_fill") or {}) and not overlay.get("dataset_id")
    if empty:
        overlay["used_for"] = (
            f"弄清菜单「{title}」什么时候用、对话框里有哪些真实字段。"
            "当前版本可能还没有可导入的练习表。"
        )
        overlay["not_for"] = (
            "不要把别的课的练习表硬套过来。"
            "练习跑通也不等于放行样板；停线或「已经证明正态」通常还要对照规程和专门检验。"
        )
        overlay["scenario"] = (
            f"这一课暂时没有可导入的练习表。若软件里能打开「{title}」，"
            "不妨先对照第 4 节核对字段；若菜单还没有，就只读帮助里的公式和边界。"
        )
        overlay["click_steps"] = [
            f"打开「帮助」→「学习中心」，选择本教程「{title}」。",
            "这一课暂时没有可导入的练习表，不要把别的课的表硬套过来。",
            "若软件里能打开该菜单，对照第 4 节核对对话框里真实有的字段。",
            "若菜单还没有或只是公式参考，就去「算法、公式与参考资料」读边界。",
            "看完把结论停在「看见了什么」；放行/停线通常还要对照规程。",
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
            f"常见读法是先说出「看见了什么」；放行或停线通常还要对照规程。"
        )
    return (
        f"「{title}」用来{used[:60] or '练习本菜单的读图/读表'}。"
        f"看完输出不等于放行样板。{not_for[:80]}"
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
        q3 = f"把「{title}」画完或算完，能不能写成「已经可以放行」？为什么？"
        g3 = "不能。图和检验只给线索，合格放行通常还要对照规格和现场流程"
        b3 = "可以，只要图好看或 p 值很小就证明已经放行"

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
        f"现场口语可以停在「看见了什么」；放行/停线通常还要对照规程。"
    )
    hint_read = (
        f"第 5 节要对着看的是：{buried_what}。"
        f"{buried_expect}"
        f"如果你写的是「已经放行 / 必须立刻停线 / 已经证明正态」，就偏了。"
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
                "student": f"导入 {fade_ds}，按第 4 节填对话框并运行，对着第 5 节指出埋点。",
                "scaffold": "参数表、读图话术和埋点全部给出。",
            },
            {
                "level": 1,
                "student": f"仍导入 {fade_ds}。最后请你自己写三句：埋点有没有出现、现场口语可以停在哪一句、放行通常还要对照什么。",
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
            "q": f"用自己的话说明：「{title}」主要帮你看什么？看完为什么还不等于可以放行？",
            "hint": why_for(title, overlay, is_spc),
        },
        {
            "q": f"本课读输出时，不妨指着哪一处线索？{buried_what} 对不对得上第 5 节？",
            "hint": f"{buried_what}。期望：{buried_expect}",
        },
        {
            "q": f"什么情况下不该用「{title}」？请写完整句子。",
            "hint": not_for or "不要用本课去替代规格放行、停线命令或正态性证明；通常还要对照规程。",
        },
    ]

    misc = [
        {
            "wrong": f"把「{title}」做完就等于已经合格，可以放行。",
            "right": "图和检验只提供线索。合格放行要对照规格和现场流程；本课练习表不等于放行样板。",
        },
        {
            "wrong": "例题点过一遍、不做练习闭环也算学会了。",
            "right": "还要做褪脚手架和合上教程的检索题，记忆才留得住。",
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

    mission = (
        f"能独立完成「{title}」，对着埋点读输出，"
        f"并把结论停在「看见信号」；放行/停线通常还要对照规程。"
    )

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
