#!/usr/bin/env python3
"""Wave-1 control-chart overlays → collaborative (并肩) voice.

Preserves title / dialog_fill / structure; rewrites student-visible copy.
Does NOT touch imr.json (Wave-0 gold). Does NOT bump catalog version.
"""
from __future__ import annotations

import json
import re
from copy import deepcopy
from pathlib import Path

from copy_depth import expand_meaning, scrub

OVERLAY_DIR = Path(__file__).resolve().parent / "tutorial_overlays"

WAVE1_IDS = [
    "c_chart",
    "cusum",
    "ewma",
    "g_chart",
    "generalized_variance",
    "hotelling_t2",
    "imr_rs",
    "laney_p_chart",
    "laney_u_chart",
    "mewma",
    "moving_average",
    "np_chart",
    "p_chart",
    "special_cause_rules",
    "t_chart",
    "u_chart",
    "xbar_r",
    "xbar_s",
    "z_mr",
    "zone_chart",
]

SHELL = "这一项决定图上或表上对应哪一列"
BAN = ("你的任务是", "这一课只练", "本课只练", "禁止过程合格", "抖主要")

# Per-lesson collaborative packs (used_for / not_for / scenario + key field meanings).
PACKS: dict[str, dict] = {
    "c_chart": {
        "used_for": (
            "回流焊一类场景里，每炉固定检同一个标准托盘时，缺陷个数可以排成一条计数流。"
            "不妨打开 C 图，看看缺陷计数有没有整段抬高。"
            "它更像在问「固定单位下的缺陷数有没有特殊原因线索」，而不是直接回答能不能放行。"
        ),
        "not_for": (
            "要是每炉检验面积或件数在变，菜单往往更该走 U 图；若盯的是「不合格品件数」而不是一件上的多处缺陷，则更像 p/np。"
            "UCL 也不是规格上限。练习表故意埋了缺陷台阶，不等于放行样板；通常还要对照规程。"
        ),
        "scenario": (
            "还是回流焊线：每炉固定检 1 个标准托盘，记下焊点缺陷数。"
            "前半段挺老实；走到大约批 26，缺陷数像抬了一级台阶。"
            "不妨先打开「控制图 → C 图」，对着批 26 看后段有没有上移——停不停线可以等信号看清楚再和规程对一下。"
        ),
        "meanings": {
            "缺陷数": "「焊点缺陷数」是固定托盘里数出来的缺陷个数。对话框里把它放进缺陷数，C 图每个点才对应这一炉的计数。",
            "每个子组单位数": "本集单位固定为 1（一个标准托盘）。填 1 是在说「每炉可比」；单位一变就该改想 U 图。",
            "阶段列": "本课故意不分阶段，好让批 26 的台阶留在同一套限里。填了阶段列，台阶可能被阶段均值吃掉。",
        },
        "buried_expect": "C 图后段多半上移，或相对 UCL 更紧。现场口语可以停在「缺陷计数抬高了」；不等于放行样板，通常还要对照规程。UCL ≠ USL。",
        "output": [
            ("C 图", "指着 CL/UCL：批 26 后曲线或点子整体抬高。常见读法是「缺陷计数上台阶了」；UCL ≠ USL。"),
            ("逐点表", "核对触发行号，回备注列「缺陷台阶」对一下就行。"),
        ],
        "mistakes": [
            "把 UCL 当成规格上限，或把点出 UCL 写成废品。",
            "检验单位大小在变仍硬用 C 图。",
            "把不合格品件数当成「一件上的多处缺陷」来画 C 图。",
            "把练习表台阶直接写成已经放行或必须立刻停线。",
        ],
    },
    "ewma": {
        "used_for": (
            "单值厚度慢慢偏厚这类「小而持续」的走动，Shewhart 有时要等很久才越界。"
            "不妨打开 EWMA：越近的点权重越大，对小台阶往往更敏感。"
            "它回答的是「平滑后的水平有没有慢慢爬升」，不等于放行样板。"
        ),
        "not_for": (
            "单点大尖峰仍优先看 I-MR 一类 Shewhart；属性计数不要硬套 EWMA。"
            "也不要用 I-MR 金标失控表来顶替本课小漂移表。练习表不等于放行样板；通常还要对照规程。"
        ),
        "scenario": (
            "贴片线有人抱怨厚度「慢慢偏厚」。表里是单值「厚度_um」。"
            "前段挺稳；大约片 31 起有约 +0.8μm 的小台阶，不是尖峰。"
            "不妨打开「控制图 → EWMA 控制图」，看看平滑曲线会不会比普通 I 图更早往上爬。"
        ),
        "meanings": {
            "测量值": "「厚度_um」是单值厚度。放进测量值后，EWMA 才知道要平滑哪一列。",
            "Lambda": "λ=0.2 表示近期点权重大约两成。λ 越大越像普通均值图，越小越平滑、反应越慢。",
            "控制限倍数": "教学默认用 3 倍限宽。改它会改变越界早晚，对照片 31 读时先保持默认更顺手。",
            "历史均值": "本课留空，让练习表自己估中心，避免盖住片 31 的小漂移。",
            "历史 Sigma": "本课留空，用本集估 σ；填了历史 Sigma 等于换尺子。",
        },
        "buried_expect": "EWMA 后段多半缓慢爬升，并可能相对 UCL 更紧。现场口语可以停在「小台阶被平滑曲线接住了」；UCL ≠ USL。",
        "output": [
            ("EWMA 图", "片 31 后曲线上移。常见读法是小而持续的漂移；UCL ≠ USL。"),
            ("参数表", "核对 λ 与限倍数是否仍是教学默认。"),
        ],
    },
    "cusum": {
        "used_for": (
            "同样是小幅、持续的均值漂移，CUSUM 把偏离一点点地累加起来看。"
            "不妨和 EWMA 对照：谁先爬升、爬升形状像什么。"
            "它更像在摊开「累积偏移线索」，而不是盖放行章。"
        ),
        "not_for": (
            "大尖峰仍优先 Shewhart；CUSUM 也不是假设检验的 p 值。"
            "不要挂 I-MR 金标失控表。练习集不等于放行样板；通常还要对照规程。"
        ),
        "scenario": (
            "还是那条小漂移厚度序列（与 EWMA 同一张练习安排）。"
            "大约片 31 起有约 +0.8μm 小台阶。"
            "不妨打开 CUSUM，看看累积和会不会在后段慢慢抬高——停线/放行可以等信号清楚再对规程。"
        ),
        "meanings": {
            "测量值": "「厚度_um」是要累积偏离的那一列单值。",
            "历史均值": "本课留空或按对话框默认；目标是让片 31 小台阶还能被累积出来。",
            "历史 Sigma": "留空则用本集估波动尺度；填死历史值可能改写灵敏度。",
        },
        "buried_expect": "CUSUM 后段多半累积上移。常见读法是「小偏移被加总后更显眼」；不等于必须立刻停线。UCL ≠ USL。",
        "output": [
            ("CUSUM 图", "片 31 后累积路径上移。口头区分过程警戒与规格。"),
            ("参数/诊断", "对照 EWMA 同一埋点，看谁先亮、亮成什么样。"),
        ],
    },
    "g_chart": {
        "used_for": (
            "重大投诉这类稀有事件，常常只记得「距上次过了多少天」。"
            "不妨用 G 图看间隔有没有突然变短——也就是事件是不是变密了。"
            "它回答的是时间间隔线索，不等于放行样板。"
        ),
        "not_for": (
            "高频计量单值更该走 I-MR；缺陷计数更像 c/u。"
            "练习表埋的是间隔变短，不是尖峰废品课。通常还要对照规程再谈停线。"
        ),
        "scenario": (
            "客诉台账里记着相邻重大投诉的间隔天数。"
            "前段大约十来天一次；走到事件 28 一带，间隔缩到两三天。"
            "不妨打开 G 图，看看后段点子是不是往下掉（间隔变短=更密）。"
        ),
        "meanings": {
            "测量值": "「间隔天数」是距上次投诉过了多久。放进测量值后，G 图才按间隔画点。",
        },
        "buried_expect": "G 图后段点子下移（间隔变短）。现场口语可以停在「投诉变密了」；不等于已经定性责任。",
        "output": [
            ("G 图", "事件 28 后间隔变短、点子偏低。UCL/LCL 仍是过程警戒，不是规格。"),
        ],
    },
    "generalized_variance": {
        "used_for": (
            "同一子组里同时测高度和宽度时，两列可能「一起抖开」。"
            "不妨看广义方差（|S|）图：联合波动有没有整段胀大。"
            "它问的是多变量散度结构，不是单列能不能放行。"
        ),
        "not_for": (
            "子组大小要大于变量个数；不要和一元 I-MR 场景混用。"
            "练习表不等于放行样板；通常还要对照规程。"
        ),
        "scenario": (
            "焊盘高度与宽度按子组采，每组 5 件。"
            "大约子组 18 起，两列联合方差像胀了一圈。"
            "不妨打开广义方差图，看看 |S| 后段有没有抬高。"
        ),
        "meanings": {
            "变量": "同时选「高度_um」和「宽度_um」。软件要两列一起估协方差；只丢一列就不成联合波动课。",
            "子组列": "「子组」标明哪几行算一批。漏了它，联合方差按子组估就不稳。",
        },
        "buried_expect": " |S| 图后段抬高。常见读法是联合散度胀大；不等于产品超规格。",
        "output": [
            ("广义方差图", "子组 18 后 |S| 上移。先说出「两列一起更散」，再谈要不要停线。"),
        ],
    },
    "hotelling_t2": {
        "used_for": (
            "一片板上同时量长度和宽度时，可能只有「合在一起」才看得出偏移。"
            "不妨打开 Hotelling T²，看联合均值有没有上台阶。"
            "它摊开的是多变量位置线索，不等于放行结论。"
        ),
        "not_for": (
            "不能替代各变量单图去抠根因；本课至少两列。"
            "练习表故意抬高长度、宽度仍稳，方便对照。通常还要对照规程。"
        ),
        "scenario": (
            "每片测长度与宽度。大约片 36 起，长度均值抬了一截，宽度仍相对老实。"
            "不妨先看 T² 后段会不会亮——一元图有时只盯宽度就会漏掉联合偏移。"
        ),
        "meanings": {
            "变量": "把「长度_mm」「宽度_mm」都放进变量。T² 吃的是这两列的联合位置。",
        },
        "buried_expect": "T² 后段抬高。常见读法是联合均值偏移；宽度单图可能仍安静。UCL ≠ USL。",
        "output": [
            ("T² 图", "片 36 后统计量上移。口头点出「联合位置变了」。"),
        ],
    },
    "imr_rs": {
        "used_for": (
            "既有子组结构、又想同时看「批均值」和「组内散度」时，I-MR-R/S 一类图很顺手。"
            "不妨对照：均值侧有没有台阶，R/S 侧是否仍老实。"
            "它回答变差落在哪一侧，不等于放行样板。"
        ),
        "not_for": (
            "不要当成单纯 I-MR 金标课；这里需要子组列。"
            "也别用失控尖峰集硬套。通常还要对照规程。"
        ),
        "scenario": (
            "每批抽 5 件测厚度。大约子组 16 起批均值往上抬，组内极差仍相对稳。"
            "不妨打开 I-MR-R/S（或菜单等价项），先看均值侧台阶，再确认 R/S 侧有没有一起乱。"
        ),
        "meanings": {
            "变量": "「厚度_um」是测量 Y；放进变量后才画得到子组均值与组内波动。",
            "子组列": "「子组」把每 5 行捆成一批。漏选的话，组内极差/标准差就拆不开。",
        },
        "buried_expect": "均值侧后段上移；R/S 侧多半仍稳。波动主要落在批间位置，而不是批内散度爆炸。",
        "output": [
            ("均值 / I 侧", "子组 16 后上移。现场口语可以停在「批均值抬高了」。"),
            ("R/S 侧", "对照是否明显乱；本课期望相对老实。"),
        ],
    },
    "laney_p_chart": {
        "used_for": (
            "不合格率按批汇总、检验数还在变时，普通 P 限有时会显得过窄——批间额外波动一来就假性报警。"
            "不妨打开 Laney P'，看看加宽后的限是不是更贴现场。"
            "它问的是「过离散下的比率监视」，不等于放行。"
        ),
        "not_for": (
            "普通二项波动用 p 图即可；不要和普通 p_chart 练习表混挂。"
            "练习集不等于放行样板；通常还要对照规程。"
        ),
        "scenario": (
            "可变检验数下的不合格品。批与批之间额外抖得比较大，普通 P 限容易过敏。"
            "不妨对照普通 P 与 Laney P'：大约批 22、31 一带，谁在假性越界、谁把限放宽了。"
        ),
        "meanings": {
            "不合格品数": "「不合格品数」是不良件数分子。",
            "检验数": "「检验数」是可变分母；限宽应随 n 变化。Laney 还会再估一层批间 σ。",
        },
        "buried_expect": "Laney Sigma Z 往往 >1，P' 限更宽；相对普通 P 的假性越界会少一些。不等于已经合格。",
        "output": [
            ("Laney P' 图", "对照加宽后的限与普通 P。口头说出过离散线索。"),
        ],
    },
    "laney_u_chart": {
        "used_for": (
            "缺陷率按单位面积/件数汇总、又有批间额外波动时，普通 U 限也可能过窄。"
            "不妨用 Laney U' 把限放宽一点再读信号。"
            "它摊开过离散下的缺陷率线索，不等于放行。"
        ),
        "not_for": (
            "普通泊松波动用 u 即可；不要和普通 u_chart 共享练习表。"
            "通常还要对照规程再谈停线。"
        ),
        "scenario": (
            "可变单位数下的缺陷计数，批与批之间额外抖得比较大。"
            "大约批 20、29 一带，相对普通 u 限容易假性报警。"
            "不妨打开 Laney U'，对照普通 u：谁在过敏、谁把限放宽了——"
            "停不停线可以等信号清楚再对规程。"
        ),
        "meanings": {
            "缺陷数": "「缺陷数」是分子计数。",
            "单位数": "「单位数」是可变面积/件数分母；和缺陷数列对一下，别对调。",
        },
        "buried_expect": "Laney U' 限更宽；假性报警相对普通 u 减少。现场口语停在「过离散」即可。",
        "output": [
            ("Laney U' 图", "对照加宽限与埋点批次。UCL ≠ USL。"),
        ],
    },
    "mewma": {
        "used_for": (
            "长度和宽度同时慢慢漂移时，一元 EWMA 可能各看各的。"
            "不妨打开 MEWMA，看联合平滑统计量会不会一起爬升。"
            "它问的是多变量小漂移，不等于放行。"
        ),
        "not_for": (
            "不要和一元 ewma 练习表混用；本课至少两列。"
            "大尖峰仍优先 Shewhart。通常还要对照规程。"
        ),
        "scenario": (
            "每片测长度与宽度。大约片 28 起，两列各约 +0.4σ 的小幅上移。"
            "不妨看 MEWMA 后段会不会爬升——停线可以等联合信号清楚再对规程。"
        ),
        "meanings": {
            "变量": "同时放入「长度_mm」「宽度_mm」。MEWMA 平滑的是这两列的联合状态。",
        },
        "buried_expect": "MEWMA 统计量后段爬升。常见读法是联合小漂移；不等于废品判决。",
        "output": [
            ("MEWMA 图", "片 28 后上移。口头区分联合漂移与单列尖峰。"),
        ],
    },
    "moving_average": {
        "used_for": (
            "想用固定窗宽把单值序列抹平滑一点、盯小台阶时，移动平均图很直观。"
            "不妨对照原始点与移动平均曲线：后段有没有慢慢抬高。"
            "它不等于放行样板。"
        ),
        "not_for": (
            "尖峰课仍优先 Shewhart；也不要挂到 EWMA/CUSUM 那张小漂移表上（本课有独立表）。"
            "通常还要对照规程。"
        ),
        "scenario": (
            "独立的小漂移厚度表：大约片 30 起厚度由约 50 抬到约 50.6。"
            "不妨打开移动平均图，看看平滑轨迹后段是否上移。"
        ),
        "meanings": {
            "测量值": "「厚度_um」是单值 Y。",
            "跨度": "窗宽决定平滑力度；教学默认先按第 4 节填，改太大可能把小台阶抹没。",
        },
        "buried_expect": "移动平均曲线后段上移。现场口语可以停在「小台阶被平滑接住了」。UCL ≠ USL。",
        "output": [
            ("移动平均图", "片 30 后平滑轨迹抬高。对照原始点别把尖峰课故事混进来。"),
        ],
    },
    "np_chart": {
        "used_for": (
            "每批检验件数固定时，不合格品件数可以直接画 NP 图。"
            "不妨看计数有没有整段抬高。"
            "它盯的是固定 n 下的不良件数线索，不等于放行。"
        ),
        "not_for": (
            "检验数随批变化时应改用 P 图；不要和 P 图练习表挂在一起。"
            "UCL 不是规格上限。练习台阶不等于放行样板；通常还要对照规程。"
        ),
        "scenario": (
            "每批固定检 100 件，记下不合格品数。"
            "前半段大约三件上下；走到大约批 21，不合格品数抬到大约九件。"
            "不妨打开 NP 图，对着批 21 看后段有没有上台阶。"
        ),
        "meanings": {
            "不合格品数": "「不合格品数」是不良件数；本课 n 恒定，直接盯件数即可。",
            "检验数": "本集恒定 100。填进检验数是为了声明分母固定；若实际 n 在变，应改 p 图。",
        },
        "buried_expect": "NP 图后段上移。常见读法是不良件数抬高；不等于废品自动判决。",
        "output": [
            ("NP 图", "批 21 后上移。口头 UCL ≠ USL。"),
        ],
    },
    "p_chart": {
        "used_for": (
            "检验数随批变化时，更该看不合格品率，并让限宽跟着 n 变。"
            "不妨打开 P 图：比率有没有台阶，限是否随样本量宽窄变化。"
            "它不等于放行样板。"
        ),
        "not_for": (
            "恒定检验数时可考虑 NP 图；缺陷计数更像 C/U。不要和 NP 练习表共享。"
            "UCL≠USL。练习比率台阶不等于放行样板；通常还要对照规程。"
        ),
        "scenario": (
            "检验数在大约 50–180 间变化。批 22 起不合格率由约 3% 抬到约 8%。"
            "不妨打开 P 图，既看后段比率上移，也顺便看限宽是否随 n 变化。"
        ),
        "meanings": {
            "不合格品数": "分子：不良件数。",
            "检验数": "可变分母。和分子对一下就行，别对调；限宽会跟着 n 走。",
        },
        "buried_expect": "p 后段上移，且限宽随 n 变化。现场口语停在「不合格率抬高了」。",
        "output": [
            ("P 图", "批 22 后比率抬高；对照不同 n 的限宽。UCL ≠ USL。"),
        ],
    },
    "special_cause_rules": {
        "used_for": (
            "控制图除了「一点越界」，还有连续同侧、趋势等判异规则。"
            "不妨先弄清规则在说什么、灵敏和误报怎么权衡。"
            "本课偏术语与边界，不是拿来盖放行章的。"
        ),
        "not_for": (
            "它不是一条独立分析菜单去替代各张控制图；也不要为本术语再造一个假菜单编号。"
            "触发规则不等于必须立刻停线；通常还要对照规程。"
        ),
        "scenario": (
            "读控制图帮助时，常会看到 Nelson / Western Electric 一类规则列表。"
            "不妨对照已有的 I-MR、Xbar-R 等课：同一埋点可能触发不同规则。"
            "先能用完整句子说出「规则是统计线索」，再谈现场动作。"
        ),
        "meanings": {
            "（规则说明）": "这里主要陈述规则定义与误报权衡；对照具体控制图课的埋点来练，不要写成已经放行。",
            "规则说明": "这里主要陈述规则定义与误报权衡；对照具体控制图课的埋点来练，不要写成已经放行。",
        },
        "buried_expect": "能指着某条规则说出它在图上对应什么模式；触发≠必须停线。",
        "output": [
            ("规则列表 / 帮助", "用完整句子区分「统计线索」与「现场停线命令」。"),
        ],
    },
    "t_chart": {
        "used_for": (
            "设备宕机这类事件，常常记的是「距上次过了多少小时」。"
            "不妨用 T 图看间隔有没有变短——维护是不是更频繁地被打断。"
            "它摊开时间间隔线索，不等于放行。"
        ),
        "not_for": (
            "高频计量单值流仍优先 I-MR；缺陷计数走 C/U。"
            "练习表埋的是宕机间隔变短，不是尖峰废品课。"
            "停机策略通常还要对照规程，不等于放行样板。"
        ),
        "scenario": (
            "相邻宕机间隔（小时）。前段大约几十小时一次；事件 25 起缩到大约十来小时。"
            "不妨打开 T 图，看后段点子是否下移。"
        ),
        "meanings": {
            "测量值": "「间隔小时」是距上次宕机的时间。放进测量值后 T 图才按间隔画。",
        },
        "buried_expect": "T 图后段下移（间隔变短）。现场口语可以停在「宕机变密了」。",
        "output": [
            ("T 图", "事件 25 后间隔变短。过程警戒≠规格。"),
        ],
    },
    "u_chart": {
        "used_for": (
            "检验单位面积或件数在变时，更该看单位缺陷率，并让限跟着单位数变。"
            "不妨打开 U 图盯缺陷率台阶。"
            "它不等于放行样板。"
        ),
        "not_for": (
            "固定单位可用 C；不合格品用 p/np；过离散可另看 Laney U'。"
            "不要和 Laney U 练习表混挂。通常还要对照规程。"
        ),
        "scenario": (
            "单位数随批变化。大约批 20 起单位缺陷率由约 0.04 抬到约 0.12。"
            "不妨打开 U 图，看后段上移，并留意限宽是否随单位数变化。"
        ),
        "meanings": {
            "缺陷数": "分子缺陷计数。",
            "单位数": "可变单位分母；和缺陷数对一下，别对调。",
        },
        "buried_expect": "u 后段上移且限随单位数变化。口语停在「单位缺陷率抬高了」。",
        "output": [
            ("U 图", "批 20 后上移。UCL ≠ USL。"),
        ],
    },
    "xbar_r": {
        "used_for": (
            "每批固定抽几件时，厚度其实有两层信息：批均值怎么走，同一批里几件散不散。"
            "不妨打开 Xbar-R：Xbar 看位置，R 看组内极差。"
            "它更像在问「波动主要落在批均值还是批内散度」，而不是直接放行。"
        ),
        "not_for": (
            "单值流更该走 I-MR；子组更大、更想盯标准差时看 Xbar-S。"
            "两张练习表不要合并成一张。练习台阶≠放行样板；通常还要对照规程。"
        ),
        "scenario": (
            "每批抽 5 件测厚度。子组 12 那一组里极差突然拉大；走到子组 20，批均值又抬了一级台阶。"
            "不妨先看 R 图子组 12，再看 Xbar 子组 20——R 先乱时，先别急着读那段 Xbar 的限。"
        ),
        "meanings": {
            "变量": "「厚度_um」是测量 Y。放进变量后，每个子组才会算出均值和极差。",
            "子组列": "「子组」标明哪 5 行算一批。漏了它，Xbar/R 都拆不开。",
            "子组大小": "填 5 要和数据一致。写错子组大小，控制限算法会对不准埋点。",
            "阶段列": "本课留空，好让子组 12/20 的信号留在同一套限里。",
        },
        "buried_expect": "R：子组 12 极差尖峰；Xbar：子组 20 后上移。先读 R 再读 Xbar。UCL ≠ USL。",
        "output": [
            ("R 图", "子组 12 极差尖峰。常见读法是批内突然变野。"),
            ("Xbar 图", "子组 20 后均值上移。口语停在「批均值抬高了」。"),
        ],
        "mistakes": [
            "只看 Xbar、忽略 R，在 R 失控段仍盲目读均值限。",
            "把 UCL 当成 USL。",
            "与 Xbar-S 练习表合并成一张。",
            "把教学台阶写成已经放行。",
        ],
    },
    "xbar_s": {
        "used_for": (
            "子组稍大（本课 n=8）时，用标准差 S 盯组内散度往往比极差更稳。"
            "不妨打开 Xbar-S，看组内 σ 有没有整段抬高。"
            "它问散度结构，不等于放行。"
        ),
        "not_for": (
            "子组 n=5、主信号在极差时，更该走 Xbar-R；两课练习表不要共享。"
            "本课主戏在组内标准差台阶，不等于放行样板；通常还要对照规程。"
        ),
        "scenario": (
            "每批抽 8 件。大约子组 14 起，组内标准差整体放大。"
            "不妨先看 S 图后段，再顺便看 Xbar 有没有跟着走——本课主信号在散度侧。"
        ),
        "meanings": {
            "变量": "「厚度_um」是 Y。",
            "子组列": "「子组」每组 8 行；漏选就算不出组内 S。",
            "子组大小": "填 8，与数据一致。",
        },
        "buried_expect": "S 图后段上移。波动主要落在批内散度放大；不等于废品判决。",
        "output": [
            ("S 图", "子组 14 后上移。口语停在「组内更散了」。"),
            ("Xbar 图", "对照是否同步大移；本课主戏在 S。"),
        ],
    },
    "z_mr": {
        "used_for": (
            "短跑混排多种型号时，绝对尺寸不好直接比，常常先按型号标准化再画 Z-MR。"
            "不妨看标准化后有没有某型号后段抬高。"
            "它不等于放行样板。"
        ),
        "not_for": (
            "单一稳定长跑优先普通 I-MR。分组列（产品型号）漏了，标准化就会乱。"
            "练习短跑台阶不等于放行样板；通常还要对照规程。"
        ),
        "scenario": (
            "三种产品短跑混排。行 33–40 一带是型号 B，且相对该型号目标均值上移。"
            "不妨打开 Z-MR，分组选产品型号，看对应 Z 点是否抬高。"
        ),
        "meanings": {
            "测量值": "「尺寸_mm」是原始测量；软件会按型号标准化成 Z。",
            "分组": "「产品型号」告诉软件按 A/B/C 各自估中心再标准化。漏了分组，短跑课就糊了。",
        },
        "buried_expect": "型号 B 后段 Z 点抬高。口语停在「该型号相对自己的目标偏了」。",
        "output": [
            ("Z 图", "对照行 33–40 型号 B。UCL 仍是标准化后的过程警戒。"),
            ("MR 图", "看标准化后的相邻波动是否异常。"),
        ],
    },
    "zone_chart": {
        "used_for": (
            "有时没有大尖峰，只是一串点连续落在中心线同一侧的区域里，积分会慢慢涨。"
            "不妨打开区域图，看同侧游程有没有触发警戒。"
            "它盯模式，不是尖峰课，也不等于放行。"
        ),
        "not_for": (
            "不要当成能力分析，也不是单点尖峰课。"
            "区域积分抬高只是同侧游程线索，不等于放行样板；停线通常还要对照规程。"
        ),
        "scenario": (
            "单值厚度序列。片 24–33 连续落在中心线同侧的 Zone C/B 一带。"
            "不妨看区域图累计分是否抬高——信号在「游程」，不在某一个飞点。"
        ),
        "meanings": {
            "测量值": "「厚度_um」按片号排列的单值。",
        },
        "buried_expect": "区域累计分抬高。常见读法是同侧游程；UCL ≠ USL。",
        "output": [
            ("区域图", "片 24–33 同侧积分抬高。口语停在「连续偏在一侧」。"),
        ],
    },
}


def warm_text(s: str) -> str:
    s = scrub(s or "")
    for b in BAN:
        if b in s:
            s = s.replace("本课只练", "本课主要看").replace("这一课只练", "这一课主要看")
            s = s.replace("禁止过程合格。", "不等于放行样板；通常还要对照规程。")
            s = s.replace("禁止过程合格", "不等于放行样板")
            s = s.replace("抖主要", "波动主要落在")
            s = s.replace("你的任务是", "不妨先")
    s = s.replace(SHELL + "。", "").replace(SHELL, "")
    return re.sub(r"。{2,}", "。", s).strip()


def warm_detail(detail: list, meanings: dict[str, str]) -> list:
    out = []
    seen = set()
    for i, item in enumerate(detail or []):
        it = dict(item)
        field = (it.get("field") or "").strip()
        # strip backticks for matching
        key = re.sub(r"\s*\(`[^`]+`\)", "", field).strip()
        if key in meanings:
            it["meaning"] = meanings[key]
        elif field in meanings:
            it["meaning"] = meanings[field]
        else:
            # unique fallback per field via expand_meaning
            it = expand_meaning(it)
        it["meaning"] = warm_text(it["meaning"])
        # ensure uniqueness within lesson
        m = it["meaning"]
        if m in seen:
            it["meaning"] = m.rstrip("。") + f"。对一下「{key or field}」这一项在本课现场的含义即可。"
        seen.add(it["meaning"])
        out.append(it)
    return out


def warm_seven(overlay: dict, pack: dict) -> None:
    title = overlay.get("title") or ""
    # prereq why
    for q in overlay.get("prereq_quiz") or []:
        if isinstance(q, dict):
            for k in ("q", "good", "bad", "why"):
                if k in q:
                    q[k] = warm_text(str(q[k]))
            if not q.get("why") or len(q["why"]) < 20:
                q["why"] = (
                    f"「{title}」用来摊开过程信号。"
                    "常见读法是先说出看见了什么；放行或停线通常还要对照规程。控制限课请分开 UCL 与 USL。"
                )
    for block_name in ("fade_levels", "retrieval_quiz", "misconceptions", "self_explain"):
        block = overlay.get(block_name) or []
        for item in block:
            if not isinstance(item, dict):
                continue
            for k, v in list(item.items()):
                if isinstance(v, str):
                    item[k] = warm_text(v)
                    if "过程合格" in item[k]:
                        item[k] = item[k].replace("过程合格", "已经放行")
                    if "禁止" in item[k] and "过程" in item[k]:
                        item[k] = warm_text(item[k])
    # soften fade student lines
    for fl in overlay.get("fade_levels") or []:
        st = fl.get("student") or ""
        if st and not st.startswith("不妨") and "请你" not in st:
            fl["student"] = "不妨" + st if not st.startswith("导入") else st.replace("导入", "不妨先导入", 1)
        fl["student"] = warm_text(fl.get("student") or "")
    # misconceptions right side
    for m in overlay.get("misconceptions") or []:
        if isinstance(m, dict) and m.get("right"):
            r = m["right"]
            if r.startswith("禁止") or "禁止过程" in r:
                m["right"] = (
                    "图和规则只提供线索。合格放行要对照规格和现场流程；"
                    "本课练习安排不等于放行样板。"
                )
            m["right"] = warm_text(m["right"])
            m["wrong"] = warm_text(m.get("wrong") or "")


def apply_pack(cid: str, overlay: dict) -> dict:
    pack = PACKS[cid]
    o = deepcopy(overlay)
    o["used_for"] = pack["used_for"]
    o["not_for"] = pack["not_for"]
    o["scenario"] = pack["scenario"]
    o["dialog_fill_detail"] = warm_detail(o.get("dialog_fill_detail") or [], pack.get("meanings") or {})

    # buried
    be = pack.get("buried_expect")
    if be and o.get("buried_signals"):
        for b in o["buried_signals"]:
            b["what"] = warm_text(b.get("what") or "")
            b["expect"] = warm_text(be if len(o["buried_signals"]) == 1 else (b.get("expect") or be))
            b["expect"] = warm_text(b["expect"])
    elif o.get("buried_signals"):
        for b in o["buried_signals"]:
            b["what"] = warm_text(b.get("what") or "")
            b["expect"] = warm_text(b.get("expect") or "")

    # output guide
    if pack.get("output"):
        o["output_guide"] = [{"name": n, "meaning": warm_text(m)} for n, m in pack["output"]]
    else:
        for g in o.get("output_guide") or []:
            g["meaning"] = warm_text(g.get("meaning") or "")

    if pack.get("mistakes"):
        o["common_mistakes"] = [warm_text(x) for x in pack["mistakes"]]
    else:
        o["common_mistakes"] = [warm_text(x) for x in (o.get("common_mistakes") or [])]

    # glossary
    for g in o.get("glossary") or []:
        for k in ("plain", "remember", "term"):
            if k in g:
                g[k] = warm_text(g[k])

    # click steps
    o["click_steps"] = [warm_text(s) for s in (o.get("click_steps") or [])]

    if o.get("skill_mission"):
        o["skill_mission"] = warm_text(o["skill_mission"])
        if "拒绝" in o["skill_mission"] or "禁止" in o["skill_mission"]:
            o["skill_mission"] = (
                f"能独立完成「{o.get('title') or cid}」，对着埋点读输出，"
                "并把结论停在「看见信号」；放行/停线通常还要对照规程。"
            )

    warm_seven(o, pack)

    # final ban sweep
    blob = json.dumps(o, ensure_ascii=False)
    for b in BAN:
        if b in blob and b != "本课只练":  # already replaced
            blob = blob.replace(b, "")
    # re-parse only if needed — instead deep walk
    def walk(x):
        if isinstance(x, str):
            return warm_text(x)
        if isinstance(x, list):
            return [walk(i) for i in x]
        if isinstance(x, dict):
            return {k: walk(v) for k, v in x.items()}
        return x

    o = walk(o)
    return o


def main() -> None:
    ok = 0
    for cid in WAVE1_IDS:
        path = OVERLAY_DIR / f"{cid}.json"
        raw = json.loads(path.read_text(encoding="utf-8"))
        warmed = apply_pack(cid, raw)
        # uniqueness check on meanings
        meanings = [d.get("meaning") for d in (warmed.get("dialog_fill_detail") or [])]
        if len(meanings) >= 2 and len(set(meanings)) < len(meanings):
            # force uniquify
            seen = {}
            for d in warmed["dialog_fill_detail"]:
                m = d["meaning"]
                if m in seen:
                    d["meaning"] = m.rstrip("。") + f"。（「{d.get('field')}」本课现场含义如上，勿与邻项混读。）"
                seen[d["meaning"]] = True
        path.write_text(json.dumps(warmed, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        uf = warmed["used_for"]
        sc = warmed["scenario"]
        print(f"{cid}: uf={len(uf)} sc={len(sc)} detail={len(meanings)}")
        ok += 1
    print(f"Warmed {ok} Wave-1 overlays")


if __name__ == "__main__":
    main()
