#!/usr/bin/env python3
"""Wave-2 quality / MSA / capability datasets, generators, and overlay writers.

Import from build_learning_dataset_mapping / build_learning_center_db.
Run `python tools/learning_data/wave2_content.py` to (re)write overlay JSON files.
"""
from __future__ import annotations

import json
import math
import random
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OVERLAY_DIR = Path(__file__).resolve().parent / "tutorial_overlays"

# ---------------------------------------------------------------------------
# Dataset catalog
# ---------------------------------------------------------------------------

WAVE2_DATASETS: dict[str, dict] = {
    "msa_crossed_aiag": {
        "wave": 2,
        "practice_only": False,
        "title": "交叉 Gage R&R（10×3×3 AIAG 型）",
        "industry": "electronics",
        "story": "10 个零件×3 名操作员×3 次重复。零件覆盖过程范围；操作员B 有固定正偏倚。服务 gage_rr+emp_crossed。",
        "row_count": 90,
        "notes": "埋点：操作员B（全部其重复行）测量值系统偏高约+2μm；零件1–10覆盖约98–110μm 过程范围。期望 %GR&R/重复性可分，且操作员方差可见。禁止挂 nested/能力规格列当失控课。行按零件×操作员×重复堆叠。",
        "columns": [
            {"index": 0, "name": "零件号", "role_hint": "part", "description": "P01–P10"},
            {"index": 1, "name": "操作员", "role_hint": "operator", "description": "A/B/C"},
            {"index": 2, "name": "测量值_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 3, "name": "重复次", "role_hint": "note", "description": "1–3；不进对话框"},
        ],
    },
    "msa_expanded_crossed": {
        "wave": 2,
        "practice_only": False,
        "title": "三因子平衡 Expanded Gage R&R",
        "industry": "electronics",
        "story": "零件×操作员×工装 三因子平衡设计。工装2 抬高测量噪声。",
        "row_count": 72,
        "notes": "埋点：工装=F2 的全部行（约行25–48）测量噪声放大；期望附加因子方差可见。6零件×2操作员×2工装×3重复=72。禁止与交叉两因子表共享。",
        "columns": [
            {"index": 0, "name": "零件号", "role_hint": "part", "description": "P01–P06"},
            {"index": 1, "name": "操作员", "role_hint": "operator", "description": "A/B"},
            {"index": 2, "name": "工装", "role_hint": "additional", "description": "F1/F2"},
            {"index": 3, "name": "测量值_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
        ],
    },
    "msa_nested_operator": {
        "wave": 2,
        "practice_only": False,
        "title": "嵌套 Gage R&R（操作员嵌套于零件批）",
        "industry": "electronics",
        "story": "每批零件只由一名操作员测（嵌套）。批内重复可见；操作员不可交叉比较。",
        "row_count": 60,
        "notes": "埋点：操作员C 负责的零件批（约行41–60）组内重复性更差（σ放大）。嵌套结构：零件→操作员一对多，禁止当交叉表用。",
        "columns": [
            {"index": 0, "name": "零件号", "role_hint": "part", "description": "N01–N20"},
            {"index": 1, "name": "操作员", "role_hint": "operator", "description": "嵌套于零件批"},
            {"index": 2, "name": "测量值_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
        ],
    },
    "msa_type1_ref": {
        "wave": 2,
        "practice_only": False,
        "title": "Type 1 量具偏倚（参考件重复）",
        "industry": "electronics",
        "story": "同一参考件重复测量 50 次。参考真值 100μm；本集均值偏高约 +1.5μm。",
        "row_count": 50,
        "notes": "埋点：全列相对参考值100μm 有约+1.5μm 正偏倚；行1–50 均可对照 Bias。期望 Type1 Cg/偏倚表显示偏倚，不是「量具通过」。禁止写成量具合格结论。",
        "columns": [
            {"index": 0, "name": "次序", "role_hint": "order", "description": "1–50"},
            {"index": 1, "name": "测量值_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "参考件重复"},
        ],
    },
    "cap_stable_spec": {
        "wave": 2,
        "practice_only": False,
        "title": "稳定略偏心单值（能力/Sixpack 同构）",
        "industry": "electronics",
        "story": "稳定近正态厚度；均值略偏高使 Cpk<Cp。无片41/55 失控尖峰。服务 capability+capability_sixpack。",
        "row_count": 80,
        "notes": "埋点：全列稳定、无特殊原因尖峰；均值约101.2μm（目标100），LSL=95 USL=105 时 Cpk 低于 Cp（偏心）。禁止与 I-MR 金标/Box-Cox 偏态集共享。行号仅作顺序，无失控行。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–80"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "稳定略偏心"},
        ],
    },
    "cap_between_within": {
        "wave": 2,
        "practice_only": False,
        "title": "组间/组内能力（子组均值台阶）",
        "industry": "electronics",
        "story": "每子组 n=5。子组12起批均值上移，组内散度仍稳。",
        "row_count": 100,
        "notes": "埋点：子组12起（约行56）批均值由约100抬到约102；期望组间方差抬高、组内相对稳。子组列必选。",
        "columns": [
            {"index": 0, "name": "子组", "role_hint": "subgroup", "description": "1–20，每组5行"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "基线/组间台阶"},
        ],
    },
    "cap_binomial_lots": {
        "wave": 2,
        "practice_only": False,
        "title": "批次不合格品（二项能力）",
        "industry": "electronics",
        "story": "可变检验数下的不合格品。批18起不合格率抬高。",
        "row_count": 36,
        "notes": "埋点：批18（行18）起不合格率由约2%抬到约7%；检验数50–150可变。期望二项能力/PPM 线索反映后段抬高。勿写成过程合格。",
        "columns": [
            {"index": 0, "name": "批号", "role_hint": "order", "description": "1–36"},
            {"index": 1, "name": "不合格品数", "role_hint": "defectives", "description": "不良件数"},
            {"index": 2, "name": "检验数", "role_hint": "inspected", "description": "可变 n"},
        ],
    },
    "cap_poisson_counts": {
        "wave": 2,
        "practice_only": False,
        "title": "单位缺陷计数（泊松能力）",
        "industry": "electronics",
        "story": "可变单位数下的缺陷。批16起 DPU 抬高。",
        "row_count": 35,
        "notes": "埋点：批16（行16）起单位缺陷率由约0.03抬到约0.10；单位数可变。期望泊松能力/DPU 反映后段抬高。",
        "columns": [
            {"index": 0, "name": "批号", "role_hint": "order", "description": "1–35"},
            {"index": 1, "name": "缺陷数", "role_hint": "defects", "description": "缺陷计数"},
            {"index": 2, "name": "单位数", "role_hint": "units", "description": "可变单位"},
        ],
    },
    "dist_skew_boxcox": {
        "wave": 2,
        "practice_only": False,
        "title": "右偏正值厚度（Box-Cox）",
        "industry": "electronics",
        "story": "强右偏正值序列，适合讨论 λ 变换，不是稳定正态能力课。",
        "row_count": 60,
        "notes": "埋点：全列右偏（近似对数正态）；约行45–48 有若干极大值拉长右尾。期望 Box-Cox 推荐 λ 远离1。禁止与 cap_stable_spec 共享。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–60"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "正值 Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "右偏/尾部"},
        ],
    },
    "dist_id_candidates": {
        "wave": 2,
        "practice_only": False,
        "title": "候选分布识别（近对数正态）",
        "industry": "electronics",
        "story": "正值寿命/厚度候选分布。形状更接近对数正态而非正态。",
        "row_count": 80,
        "notes": "埋点：近似 lognormal(μ=4.6,σ=0.35) 生成；期望个体分布识别中对数正态/Weibull 优于正态。行无失控尖峰剧情。",
        "columns": [
            {"index": 0, "name": "序号", "role_hint": "order", "description": "1–80"},
            {"index": 1, "name": "测量值", "role_hint": "measurement", "description": "正值 Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "候选分布"},
        ],
    },
    "fishbone_solder_causes": {
        "wave": 2,
        "practice_only": False,
        "title": "焊点不良鱼骨原因清单",
        "industry": "electronics",
        "story": "头脑风暴列出的 5M1E 类别与具体原因，供因果图展示结构。",
        "row_count": 24,
        "notes": "埋点：类别「方法」下列出「钢网张力未校准」（约行9）；「设备」列「回流峰值偏低」（约行4）。期望鱼骨图按类别分枝。非 SPC 读图课。",
        "columns": [
            {"index": 0, "name": "类别", "role_hint": "category", "description": "人机料法环测"},
            {"index": 1, "name": "原因", "role_hint": "cause", "description": "具体原因短句"},
        ],
    },
    "pareto_defect_tail": {
        "wave": 2,
        "practice_only": False,
        "title": "缺陷类别长尾（柏拉图）",
        "industry": "electronics",
        "story": "焊点缺陷类别。虚焊/偏移占绝大多数，其余长尾。",
        "row_count": 120,
        "notes": "埋点：类别「虚焊」「偏移」合计约前80%计数（行随机但频数固定配方）；期望柏拉图累计到约80%时主要看前2–3类。Other 阈值可试用95。",
        "columns": [
            {"index": 0, "name": "缺陷类别", "role_hint": "category", "description": "分类标签"},
            {"index": 1, "name": "备注", "role_hint": "note", "description": "可选"},
        ],
    },
    "multi_vari_pos_time": {
        "wave": 2,
        "practice_only": False,
        "title": "位置×时段 Multi-Vari",
        "industry": "electronics",
        "story": "同一厚度在腔位与时段两因子下的分层。后段时段偏高。",
        "row_count": 48,
        "notes": "埋点：时段=晚班 的行（约行25–48）均值上移；腔位间差异较小。期望 Multi-Vari 显示时段主效应大于腔位。因子列≥2。",
        "columns": [
            {"index": 0, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 1, "name": "腔位", "role_hint": "factor", "description": "左/中/右"},
            {"index": 2, "name": "时段", "role_hint": "factor", "description": "早班/晚班"},
            {"index": 3, "name": "备注", "role_hint": "note", "description": "基线/晚班抬高"},
        ],
    },
    "run_chart_median_trend": {
        "wave": 2,
        "practice_only": False,
        "title": "运行图中位数趋势",
        "industry": "electronics",
        "story": "单值序列。片28起相对中位数同侧偏高游程。",
        "row_count": 50,
        "notes": "埋点：片28–40（行28–40）连续落在总体中位数上方；期望运行图标记簇/趋势线索。不是控制限课，勿把中位数当 UCL。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–50"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "基线/同侧游程"},
        ],
    },
}

# Mapping role_map keys must ⊆ id_metadata roles (often only first role).
ROLE_MAP_BY_DATASET: dict[str, dict[str, str]] = {
    "msa_crossed_aiag": {"measurement": "测量值_um"},
    "msa_expanded_crossed": {"measurement": "测量值_um"},
    "msa_nested_operator": {"measurement": "测量值_um"},
    "msa_type1_ref": {"measurement": "测量值_um"},
    "cap_stable_spec": {"variables": "厚度_um"},
    "cap_between_within": {"variables": "厚度_um"},
    "cap_binomial_lots": {"defectives": "不合格品数"},
    "cap_poisson_counts": {"defects": "缺陷数"},
    "dist_skew_boxcox": {"variables": "厚度_um"},
    "dist_id_candidates": {"variables": "测量值"},
    "fishbone_solder_causes": {"category": "类别"},
    "pareto_defect_tail": {"category": "缺陷类别"},
    "multi_vari_pos_time": {"measurement": "厚度_um"},
    "run_chart_median_trend": {"variables": "厚度_um"},
}

# ---------------------------------------------------------------------------
# Generators
# ---------------------------------------------------------------------------


def _poisson(rng: random.Random, lam: float) -> int:
    L = math.exp(-lam)
    k = 0
    p = 1.0
    while p > L:
        k += 1
        p *= rng.random()
    return max(0, k - 1)


def gen_msa_crossed_aiag(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(201)
    part_true = {i: 98.0 + i * 1.2 for i in range(1, 11)}  # process range
    rows = []
    for part in range(1, 11):
        for oper in ("A", "B", "C"):
            bias = 2.0 if oper == "B" else 0.0
            for rep in range(1, 4):
                y = part_true[part] + bias + rng.gauss(0, 0.45)
                rows.append([f"P{part:02d}", oper, f"{y:.2f}", str(rep)])
    return rows


def gen_msa_expanded_crossed(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(202)
    rows = []
    for part in range(1, 7):
        base = 100.0 + part * 0.8
        for oper in ("A", "B"):
            for fixture in ("F1", "F2"):
                noise = 1.4 if fixture == "F2" else 0.4
                for _rep in range(3):
                    y = base + rng.gauss(0, noise)
                    rows.append([f"P{part:02d}", oper, fixture, f"{y:.2f}"])
    return rows


def gen_msa_nested_operator(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(203)
    # 20 parts, 3 reps each; operators A/B/C each own a block of parts
    schedule = (["A"] * 7 + ["B"] * 7 + ["C"] * 6)
    rows = []
    for idx, oper in enumerate(schedule, start=1):
        sd = 1.6 if oper == "C" else 0.5
        base = 100.0 + (idx % 5) * 0.6
        for _rep in range(3):
            y = base + rng.gauss(0, sd)
            rows.append([f"N{idx:02d}", oper, f"{y:.2f}"])
    return rows


def gen_msa_type1_ref(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(204)
    rows = []
    for i in range(1, 51):
        y = 101.5 + rng.gauss(0, 0.35)  # bias vs ref 100
        rows.append([str(i), f"{y:.2f}", "参考件重复"])
    return rows


def gen_cap_stable_spec(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(205)
    rows = []
    for i in range(1, 81):
        y = 101.2 + rng.gauss(0, 1.1)  # slight off-center vs target 100
        rows.append([str(i), f"{y:.2f}", "稳定略偏心"])
    return rows


def gen_cap_between_within(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(206)
    rows = []
    for sg in range(1, 21):
        mean = 102.0 if sg >= 12 else 100.0
        note = "组间台阶" if sg >= 12 else "基线"
        for _ in range(5):
            y = mean + rng.gauss(0, 0.7)
            rows.append([str(sg), f"{y:.2f}", note])
    return rows


def gen_cap_binomial_lots(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(207)
    rows = []
    for i in range(1, 37):
        n = rng.choice([50, 80, 100, 120, 150])
        p = 0.07 if i >= 18 else 0.02
        d = sum(1 for _ in range(n) if rng.random() < p)
        rows.append([str(i), str(d), str(n)])
    return rows


def gen_cap_poisson_counts(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(208)
    rows = []
    for i in range(1, 36):
        units = rng.randint(20, 80)
        rate = 0.10 if i >= 16 else 0.03
        defects = _poisson(rng, rate * units)
        rows.append([str(i), str(defects), str(units)])
    return rows


def gen_dist_skew_boxcox(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(209)
    rows = []
    for i in range(1, 61):
        # lognormal-ish right skew
        y = math.exp(rng.gauss(4.0, 0.45))
        note = "右偏"
        if 45 <= i <= 48:
            y *= 2.2
            note = "右尾极大"
        rows.append([str(i), f"{y:.2f}", note])
    return rows


def gen_dist_id_candidates(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(210)
    rows = []
    for i in range(1, 81):
        y = math.exp(rng.gauss(4.6, 0.35))
        rows.append([str(i), f"{y:.3f}", "候选分布"])
    return rows


def gen_fishbone_solder_causes(_rng: random.Random) -> list[list[str]]:
    items = [
        ("人员", "培训不足"),
        ("人员", "目检疲劳"),
        ("设备", "回流峰值偏低"),
        ("设备", "钢网张力机漂移"),
        ("材料", "锡膏过期"),
        ("材料", "焊盘氧化"),
        ("方法", "印刷压力偏大"),
        ("方法", "钢网张力未校准"),
        ("方法", "回流曲线未验证"),
        ("环境", "温湿度超窗"),
        ("环境", "振动干扰"),
        ("测量", "SPI 校准过期"),
        ("测量", "量具分辨率不足"),
        ("人员", "换班交接不清"),
        ("设备", "传送带速度偏快"),
        ("材料", "助焊剂批次差异"),
        ("方法", "返工温度过高"),
        ("环境", "静电防护失效"),
        ("测量", "抽样位置偏置"),
        ("人员", "新品导入经验不足"),
        ("设备", "冷却风扇堵塞"),
        ("材料", "钢网磨损"),
        ("方法", "停线重启未预热"),
        ("环境", "洁净度下降"),
    ]
    return [[cat, cause] for cat, cause in items]


def gen_pareto_defect_tail(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(211)
    # Fixed multinomial-ish frequencies
    bag = (
        ["虚焊"] * 48
        + ["偏移"] * 30
        + ["桥连"] * 15
        + ["少锡"] * 10
        + ["多锡"] * 7
        + ["其他"] * 10
    )
    rng.shuffle(bag)
    return [[cat, "缺陷记录"] for cat in bag]


def gen_multi_vari_pos_time(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(212)
    rows = []
    for time in ("早班", "晚班"):
        for pos in ("左", "中", "右"):
            for _ in range(8):
                y = 100.0 + rng.gauss(0, 0.6)
                if time == "晚班":
                    y += 1.8
                note = "晚班抬高" if time == "晚班" else "基线"
                rows.append([f"{y:.2f}", pos, time, note])
    return rows


def gen_run_chart_median_trend(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(213)
    rows = []
    for i in range(1, 51):
        if 28 <= i <= 40:
            y = 100.6 + abs(rng.gauss(0, 0.2))
            note = "同侧游程"
        else:
            y = 100.0 + rng.gauss(0, 0.5)
            note = "基线"
        rows.append([str(i), f"{y:.2f}", note])
    return rows


GENERATORS = {
    "msa_crossed_aiag": gen_msa_crossed_aiag,
    "msa_expanded_crossed": gen_msa_expanded_crossed,
    "msa_nested_operator": gen_msa_nested_operator,
    "msa_type1_ref": gen_msa_type1_ref,
    "cap_stable_spec": gen_cap_stable_spec,
    "cap_between_within": gen_cap_between_within,
    "cap_binomial_lots": gen_cap_binomial_lots,
    "cap_poisson_counts": gen_cap_poisson_counts,
    "dist_skew_boxcox": gen_dist_skew_boxcox,
    "dist_id_candidates": gen_dist_id_candidates,
    "fishbone_solder_causes": gen_fishbone_solder_causes,
    "pareto_defect_tail": gen_pareto_defect_tail,
    "multi_vari_pos_time": gen_multi_vari_pos_time,
    "run_chart_median_trend": gen_run_chart_median_trend,
}

# ---------------------------------------------------------------------------
# Overlay helpers
# ---------------------------------------------------------------------------


def _seven_plus(
    mission: str,
    prereq: list,
    self_explain: list,
    fade_ds: str | None,
    retrieval: list,
    misc: list,
) -> dict:
    if fade_ds:
        fade = [
            {
                "level": 0,
                "student": "跟着 §4 填对话框、跟着 §5 对照埋点读输出。",
                "scaffold": "参数表 + 读图/读表话术 + 埋点全部给出。",
            },
            {
                "level": 1,
                "student": f"仍导入 `demo_{fade_ds}`；最后「埋点是否出现、禁止句」自己写三句。",
                "scaffold": "对话框字段仍给出；不给标准答案。",
            },
            {
                "level": 2,
                "student": f"再导入 `demo_{fade_ds}`，自己改一个输入（如规格/公差留空与否），不看埋点剧透写结论。",
                "scaffold": "仅术语表 + 误用禁止句。",
            },
        ]
    else:
        fade = [
            {
                "level": 0,
                "student": "本课无演示表：对照 §4 字段清单与公式帮助，写清菜单可能不可用。",
                "scaffold": "dialog_fill_detail + 公式帮助路径。",
            },
            {
                "level": 1,
                "student": "不看 detail，默写本命令真实 roles/inputs 名称（对照 analysis_commands）。",
                "scaffold": "只给 menu_path。",
            },
            {
                "level": 2,
                "student": "写一段：本课为何空 dataset，以及误用一句禁止句。",
                "scaffold": "仅 skill_mission。",
            },
        ]
    return {
        "skill_mission": mission,
        "prereq_quiz": prereq,
        "self_explain": self_explain,
        "fade_levels": fade,
        "retrieval_quiz": retrieval,
        "misconceptions": misc
        + [
            {
                "wrong": "看完例题不做练习也会了",
                "right": "必须做褪脚手架和检索小测。",
            },
        ],
    }


def _base_mistakes(extra: list[str] | None = None) -> list[str]:
    base = [
        "写成过程合格、必须停线、已证明正态或量具通过。",
        "把教学演示集直接当成客户放行证据。",
        "虚构菜单字段或不对照 analysis_commands 的 roles/inputs。",
    ]
    return base + (extra or [])


def _formula_skeleton(
    *,
    cid: str,
    title: str,
    menu: str,
    used_for: str,
    not_for: str,
    scenario: str,
    related: list[str],
    glossary: list[dict],
    detail: list[dict],
    mission: str,
    prereq: list,
    self_explain: list,
    retrieval: list,
    misc: list,
    extra_mistakes: list[str] | None = None,
) -> dict:
    return {
        "title": title,
        "used_for": used_for,
        "not_for": not_for,
        "scenario": scenario,
        "related_ids": related,
        "dialog_fill": {},
        "click_steps": [
            f"打开「帮助」→「学习中心」，选择本教程「{title}」。",
            "注意：当前版本可能无此菜单项或仅为公式参考；公式见「算法、公式与参考资料」。",
            f"若菜单可用：{menu}。本课不提供专用演示表（dataset 空），步骤保持诚实。",
            "对照下方 dialog_fill_detail 核对该命令真实 roles/inputs；勿虚构字段。",
            "输出解读禁止：过程合格 / 必须停线 / 已证明正态 / 量具通过。",
        ],
        "dialog_fill_detail": detail,
        "glossary": glossary,
        "buried_signals": [],
        "output_guide": [
            {
                "name": "帮助/公式页",
                "meaning": "只陈述定义与边界；禁止过程合格；禁止必须停线；禁止已证明正态；禁止量具通过。",
            }
        ],
        "common_mistakes": _base_mistakes(extra_mistakes),
        **_seven_plus(mission, prereq, self_explain, None, retrieval, misc),
    }


def build_overlays() -> dict[str, dict]:
    out: dict[str, dict] = {}

    # ---- shared MSA crossed ----
    out["gage_rr"] = {
        "title": "Crossed Gage R&R",
        "used_for": "交叉设计下分解重复性/再现性。本课练 10×3×3 平衡表 + 操作员偏倚线索。",
        "not_for": "嵌套结构应选 Nested；属性评级应选属性一致性。不能写成「量具通过」。",
        "scenario": "SPI 高度量具评估。零件覆盖过程范围；操作员B 系统性偏高。用 Crossed Gage R&R 看方差分量。",
        "related_ids": ["emp_crossed", "nested_gage_rr", "msa_type1"],
        "dialog_fill": {
            "measurement": "测量值_um",
            "part": "零件号",
            "operator": "操作员",
        },
        "click_steps": [
            "学习中心导入 `demo_msa_crossed_aiag`。",
            "菜单：质量工具 → Crossed Gage R&R。",
            "测量值=`测量值_um`；零件=`零件号`；操作员=`操作员`；LSL/USL 可填 95/110 作公差对照（可选）。",
            "对照操作员B 偏倚：再现性/操作员分量应抬高；禁止写成量具通过。",
        ],
        "dialog_fill_detail": [
            {"field": "测量值 (`measurement`)", "put": "测量值_um", "meaning": "响应 Y。"},
            {"field": "零件 (`part`)", "put": "零件号", "meaning": "交叉设计的零件因子。"},
            {"field": "操作员 (`operator`)", "put": "操作员", "meaning": "交叉设计的操作员因子。"},
            {"field": "LSL（可选） (`lsl`)", "put": "95（可选）", "meaning": "规格下限；用于 %Tolerance，不是控制限。"},
            {"field": "USL（可选） (`usl`)", "put": "110（可选）", "meaning": "规格上限；UCL≠USL 的「USL」在此是规格。"},
        ],
        "glossary": [
            {"term": "%GR&R", "plain": "量具重复性与再现性占研究变差的比例。", "remember": "是量具研究指标，不是过程合格证明。"},
            {"term": "重复性 / 再现性", "plain": "同一条件再测波动 vs 不同操作员差异。", "remember": "本课操作员B 偏倚主要抬高再现性线索。"},
            {"term": "交叉设计", "plain": "每名操作员测同一组零件。", "remember": "嵌套设计不能硬套交叉模型。"},
        ],
        "buried_signals": [
            {
                "row": 4,
                "what": "操作员B 首次出现（零件P01）起系统 +2μm 偏倚贯穿其所有行",
                "expect": "操作员方差/%GR&R 抬高线索；勿写量具通过或过程合格。",
            }
        ],
        "output_guide": [
            {"name": "方差分量/%GR&R", "meaning": "指着操作员分量：B 偏倚应可见。禁止量具通过。"},
            {"name": "交互图", "meaning": "核对零件×操作员是否交叉完整。"},
        ],
        "common_mistakes": _base_mistakes(["把 %GR&R 阈值当法律放行", "用嵌套数据跑交叉模型"]),
        **_seven_plus(
            "在交叉平衡表上完成 Gage R&R，指出操作员B 偏倚，并拒绝「量具通过」话术。",
            [
                {"q": "交叉设计要求？", "good": "操作员与零件交叉测量", "bad": "每零件只绑一名操作员"},
                {"q": "本课能否写量具通过？", "good": "不能", "bad": "能，%GR&R<10%就算"},
                {"q": "LSL/USL 是控制限吗？", "good": "否，是规格", "bad": "是 UCL/LCL"},
            ],
            [
                {"after": "选三列", "prompt": "为何必须同时选零件与操作员？"},
                {"after": "读 %GR&R", "prompt": "为何不能写成量具通过？"},
            ],
            "msa_crossed_aiag",
            ["操作员B埋了什么？", "交叉 vs 嵌套一句", "禁止句有哪些？"],
            [{"wrong": "%GR&R 低=过程合格", "right": "量具研究≠过程能力结论；禁止过程合格。"}],
        ),
    }

    out["emp_crossed"] = {
        "title": "EMP Crossed",
        "used_for": "Wheeler EMP 观点下的交叉量具评估。与 gage_rr 同构共享 `msa_crossed_aiag`。",
        "not_for": "不替代 AIAG %GR&R 客户表格；无规格输入时勿硬扯公差比。禁止量具通过。",
        "scenario": "同一交叉表。用 EMP Crossed 看分类/偏倚线索，对照操作员B。",
        "related_ids": ["gage_rr", "msa_type1"],
        "dialog_fill": {
            "measurement": "测量值_um",
            "part": "零件号",
            "operator": "操作员",
        },
        "click_steps": [
            "导入 `demo_msa_crossed_aiag`（与 Crossed Gage R&R 同构白名单）。",
            "菜单：质量工具 → EMP Crossed。",
            "测量值/零件/操作员三列同上；本命令无 LSL/USL 输入。",
            "对照操作员B 偏倚；禁止量具通过。",
        ],
        "dialog_fill_detail": [
            {"field": "测量值 (`measurement`)", "put": "测量值_um", "meaning": "Y。"},
            {"field": "零件 (`part`)", "put": "零件号", "meaning": "交叉零件因子。"},
            {"field": "操作员 (`operator`)", "put": "操作员", "meaning": "交叉操作员因子；本课无规格 inputs。"},
        ],
        "glossary": [
            {"term": "EMP", "plain": "Evaluating the Measurement Process（Wheeler）。", "remember": "与 AIAG %GR&R 报告口径不同。"},
            {"term": "交叉同构共享", "plain": "本课与 gage_rr 共享 `msa_crossed_aiag`。", "remember": "白名单极小族；禁止再挂 nested。"},
            {"term": "偏倚线索", "plain": "某操作员系统偏离。", "remember": "是研究信号，不是停线指令。"},
        ],
        "buried_signals": [
            {
                "row": 4,
                "what": "操作员B 偏倚（同 gage_rr）",
                "expect": "EMP 分类/图形中操作员差异可见；勿写必须停线。",
            }
        ],
        "output_guide": [
            {"name": "EMP 输出", "meaning": "对照操作员B；禁止量具通过/过程合格。"},
        ],
        "common_mistakes": _base_mistakes(["把 EMP 与 AIAG 阈值混谈", "给本命令虚构 LSL 字段"]),
        **_seven_plus(
            "用同构交叉表跑 EMP Crossed，说明与 gage_rr 同构共享理由。",
            [
                {"q": "本课 dataset？", "good": "msa_crossed_aiag", "bad": "各自一张宽表"},
                {"q": "有无 LSL 输入？", "good": "无", "bad": "有，必须填"},
                {"q": "能量具通过吗？", "good": "不能写", "bad": "能"},
            ],
            [
                {"after": "导入", "prompt": "为何可以与 gage_rr 共享表？"},
                {"after": "读图", "prompt": "EMP 结论能否写成停线？"},
            ],
            "msa_crossed_aiag",
            ["白名单族成员？", "操作员B？", "禁止句"],
            [{"wrong": "EMP 证明量具合格", "right": "禁止量具通过；只给研究线索。"}],
        ),
    }

    out.update(_build_msa_rest())
    out.update(_build_capability())
    out.update(_build_quality_graphs())
    out.update(_build_formula_refs())
    return out


def _build_msa_rest() -> dict[str, dict]:
    out: dict[str, dict] = {}
    out["expanded_gage_rr"] = {
        "title": "Expanded Gage R&R",
        "used_for": "三因子平衡交叉量具：零件×操作员×附加因子。",
        "not_for": "不平衡设计见公式参考课；两因子交叉用 gage_rr。禁止量具通过。",
        "scenario": "工装作为附加因子。工装 F2 噪声更大。",
        "related_ids": ["gage_rr", "expanded_gage_unbalanced"],
        "dialog_fill": {
            "measurement": "测量值_um",
            "part": "零件号",
            "operator": "操作员",
            "additional": "工装",
        },
        "click_steps": [
            "导入 `demo_msa_expanded_crossed`。",
            "菜单：质量工具 → Expanded Gage R&R。",
            "测量/零件/操作员/附加因子=工装；公差可选 15。",
            "对照工装 F2 噪声放大；禁止量具通过。",
        ],
        "dialog_fill_detail": [
            {"field": "测量值 (`measurement`)", "put": "测量值_um", "meaning": "Y。"},
            {"field": "零件 (`part`)", "put": "零件号", "meaning": "零件因子。"},
            {"field": "操作员 (`operator`)", "put": "操作员", "meaning": "操作员因子。"},
            {"field": "附加因子 (`additional`)", "put": "工装", "meaning": "第三交叉因子。"},
            {"field": "公差（可选） (`tolerance`)", "put": "15（可选）", "meaning": "公差宽度；不是 UCL。"},
        ],
        "glossary": [
            {"term": "附加因子", "plain": "零件/操作员之外的第三交叉因子（如工装）。", "remember": "本课工装 F2 抬高噪声。"},
            {"term": "平衡设计", "plain": "各水平组合重复次数齐。", "remember": "不平衡另课。"},
            {"term": "公差宽度", "plain": "规格上下限之差一类尺度。", "remember": "不是控制限。"},
        ],
        "buried_signals": [
            {
                "row": 25,
                "what": "约行25起进入工装F2 区块，测量噪声放大",
                "expect": "附加因子方差抬高；勿写量具通过。",
            }
        ],
        "output_guide": [
            {"name": "三因子方差", "meaning": "F2 噪声应可见。禁止过程合格。"},
        ],
        "common_mistakes": _base_mistakes(["漏选附加因子", "与两因子表混用"]),
        **_seven_plus(
            "完成三因子 Expanded Gage R&R，指出工装 F2 噪声。",
            [
                {"q": "附加因子列？", "good": "工装", "bad": "备注"},
                {"q": "F2 埋点？", "good": "噪声放大", "bad": "均值合格"},
                {"q": "能量具通过？", "good": "不能写", "bad": "能"},
            ],
            [
                {"after": "选四列", "prompt": "少选附加因子会怎样？"},
                {"after": "读输出", "prompt": "公差输入是控制限吗？"},
            ],
            "msa_expanded_crossed",
            ["F2？", "与 gage_rr 区别", "禁止句"],
            [{"wrong": "三因子=%GR&R 自动更准", "right": "设计匹配才有意义；禁止量具通过。"}],
        ),
    }

    out["nested_gage_rr"] = {
        "title": "Nested Gage R&R",
        "used_for": "操作员嵌套于零件批时的量具分量。",
        "not_for": "交叉数据应选 Crossed。禁止量具通过。",
        "scenario": "每批零件只由一名操作员测。操作员C 重复性更差。",
        "related_ids": ["gage_rr", "emp_crossed"],
        "dialog_fill": {
            "measurement": "测量值_um",
            "part": "零件号",
            "operator": "操作员",
        },
        "click_steps": [
            "导入 `demo_msa_nested_operator`。",
            "菜单：质量工具 → Nested Gage R&R。",
            "测量/部件/操作者三列；公差可选。",
            "对照操作员C 区块重复性变差；禁止与交叉表共享。",
        ],
        "dialog_fill_detail": [
            {"field": "测量值 (`measurement`)", "put": "测量值_um", "meaning": "Y。"},
            {"field": "部件 (`part`)", "put": "零件号", "meaning": "嵌套结构中的零件。"},
            {"field": "操作者 (`operator`)", "put": "操作员", "meaning": "嵌套于零件批，非完全交叉。"},
            {"field": "公差 (`tolerance`)", "put": "留空或 15", "meaning": "可选公差宽度。"},
        ],
        "glossary": [
            {"term": "嵌套", "plain": "某因子水平只出现在另一因子的部分水平内。", "remember": "不能当成交叉。"},
            {"term": "重复性", "plain": "同一条件下再测波动。", "remember": "本课 C 区块 σ 更大。"},
            {"term": "专用集", "plain": "本课独立 `msa_nested_operator`。", "remember": "禁止挂 msa_crossed_aiag。"},
        ],
        "buried_signals": [
            {
                "row": 41,
                "what": "约行41起操作员C 零件批，组内重复 σ 放大",
                "expect": "嵌套模型下重复性/操作员相关分量抬高；勿写量具通过。",
            }
        ],
        "output_guide": [
            {"name": "嵌套方差", "meaning": "C 区块更散。禁止过程合格。"},
        ],
        "common_mistakes": _base_mistakes(["用交叉表跑 nested", "写成量具通过"]),
        **_seven_plus(
            "用嵌套专用集完成 Nested Gage R&R，说明为何不能共享交叉表。",
            [
                {"q": "能否共享 msa_crossed_aiag？", "good": "不能", "bad": "能，反正都是 MSA"},
                {"q": "C 埋点？", "good": "重复性变差", "bad": "过程合格"},
                {"q": "嵌套含义？", "good": "操作员绑在零件批内", "bad": "全交叉"},
            ],
            [
                {"after": "导入", "prompt": "如何从列结构看出嵌套？"},
                {"after": "读输出", "prompt": "为何禁止量具通过？"},
            ],
            "msa_nested_operator",
            ["与交叉区别", "行41？", "禁止句"],
            [{"wrong": "嵌套=交叉少选一列", "right": "数据结构不同，模型不同。"}],
        ),
    }

    out["msa_type1"] = {
        "title": "MSA Type 1 / Bias / Stability",
        "used_for": "参考件重复测量评估偏倚/Type1 指标。",
        "not_for": "多零件交叉 GR&R；稳定性长期监控需时间设计。禁止量具通过。",
        "scenario": "参考真值 100μm；重复测量均值约 101.5μm。",
        "related_ids": ["gage_rr", "emp_crossed"],
        "dialog_fill": {"measurement": "测量值_um"},
        "click_steps": [
            "导入 `demo_msa_type1_ref`。",
            "菜单：质量工具 → MSA Type 1 / Bias / Stability。",
            "测量值=`测量值_um`；模式选 Type 1；参考值=100；公差宽度可选 10；LSL/USL 可选；参考值列留空；过程变差留空。",
            "对照全列相对 100 的正偏倚；禁止量具通过。",
        ],
        "dialog_fill_detail": [
            {"field": "测量值 (`measurement`)", "put": "测量值_um", "meaning": "重复测量 Y。"},
            {"field": "参考值列（Linearity） (`reference`)", "put": "留空", "meaning": "Type1 本课用常数参考值，不用列。"},
            {"field": "模式 (`mode`)", "put": "Type 1", "meaning": "catalog：msa_type1_mode。"},
            {"field": "参考值（Type 1） (`reference_value`)", "put": "100", "meaning": "参考件真值。"},
            {"field": "公差宽度（可选） (`tolerance`)", "put": "10（可选）", "meaning": "Cg 等尺度。"},
            {"field": "LSL（可选） (`lsl`)", "put": "留空或 95", "meaning": "规格下限可选。"},
            {"field": "USL（可选） (`usl`)", "put": "留空或 105", "meaning": "规格上限可选。"},
            {"field": "过程变差 6σ（Linearity 可选） (`process_variation`)", "put": "留空", "meaning": "Linearity 模式才常用。"},
        ],
        "glossary": [
            {"term": "Bias（偏倚）", "plain": "测量均值相对参考真值的系统差。", "remember": "本课约 +1.5μm。"},
            {"term": "Type 1", "plain": "单件参考重复研究。", "remember": "不是交叉 GR&R。"},
            {"term": "Cg/Cgk", "plain": "量具能力类指标（视输出）。", "remember": "仍禁止写量具通过。"},
        ],
        "buried_signals": [
            {
                "row": 1,
                "what": "行1–50 相对参考100 系统性正偏倚约 +1.5μm",
                "expect": "Bias 表显示正偏倚；勿写量具通过或过程合格。",
            }
        ],
        "output_guide": [
            {"name": "Bias / Type1 表", "meaning": "指着偏倚方向；禁止量具通过。"},
        ],
        "common_mistakes": _base_mistakes(["参考值填成规格中心却当控制限", "写成已证明正态"]),
        **_seven_plus(
            "完成 Type1 偏倚课，对照参考值 100 读正偏倚。",
            [
                {"q": "参考值填？", "good": "100", "bad": "USL"},
                {"q": "本课模式？", "good": "Type 1", "bad": "随便"},
                {"q": "能量具通过？", "good": "不能写", "bad": "能"},
            ],
            [
                {"after": "填参考值", "prompt": "为何参考值列要留空？"},
                {"after": "读 Bias", "prompt": "偏倚等于必须停线吗？"},
            ],
            "msa_type1_ref",
            ["偏倚多少？", "与 GR&R 区别", "禁止句"],
            [{"wrong": "Cg 高=量具通过可放行", "right": "禁止量具通过；需结合程序与风险。"}],
        ),
    }
    return out


def _build_capability() -> dict[str, dict]:
    out: dict[str, dict] = {}
    out["capability"] = {
        "title": "正态过程能力",
        "used_for": "稳定近正态数据上估算 Cp/Cpk 等。本课强调偏心使 Cpk<Cp。",
        "not_for": "失控数据（禁止用 imr_spi_shift）；强偏态先变换/非正态课。禁止过程合格。",
        "scenario": "稳定厚度略偏高。LSL=95 USL=105 目标100。",
        "related_ids": ["capability_sixpack", "imr", "box_cox"],
        "dialog_fill": {"variables": "厚度_um"},
        "click_steps": [
            "导入 `demo_cap_stable_spec`。",
            "菜单：质量工具 → 正态过程能力。",
            "变量=`厚度_um`；子组大小=1；LSL=95；USL=105；Target=100；变换=无。",
            "对照：Cpk 应低于 Cp（偏心）；确认无片41/55 失控尖峰。禁止过程合格。",
        ],
        "dialog_fill_detail": [
            {"field": "变量 (`variables`)", "put": "厚度_um", "meaning": "单值 Y。"},
            {"field": "子组大小 (`subgroup_size`)", "put": "1", "meaning": "本集按单值。"},
            {"field": "LSL (`lsl`)", "put": "95", "meaning": "规格下限。"},
            {"field": "USL (`usl`)", "put": "105", "meaning": "规格上限；不是 UCL。"},
            {"field": "Target (`target`)", "put": "100", "meaning": "目标；偏心对照。"},
            {"field": "变换 (`transform`)", "put": "无", "meaning": "capability_transform 目录；本课不变换。"},
        ],
        "glossary": [
            {"term": "Cp", "plain": "规格宽度相对过程散度（不考虑居中）。", "remember": "只看散度潜力。"},
            {"term": "Cpk", "plain": "考虑均值相对最近规格的能力。", "remember": "偏心时 Cpk<Cp。"},
            {"term": "稳定前提", "plain": "能力指数默认过程统计受控。", "remember": "失控教学集不算 Cpk 当合格证据。"},
        ],
        "buried_signals": [
            {
                "row": 1,
                "what": "全列稳定略偏心（均值约101.2），无特殊原因尖峰",
                "expect": "Cpk<Cp；勿写过程合格或已证明正态。",
            }
        ],
        "output_guide": [
            {"name": "Cp/Cpk 表", "meaning": "指着 Cpk<Cp。禁止过程合格。"},
            {"name": "直方图/正态叠加", "meaning": "只作形状线索；禁止已证明正态。"},
        ],
        "common_mistakes": _base_mistakes(["用 I-MR 失控金标算 Cpk", "把 Cpk>1.33 写成过程合格"]),
        **_seven_plus(
            "在稳定略偏心集上读出 Cpk<Cp，并拒绝失控集滥算能力。",
            [
                {"q": "本课能否用 imr_spi_shift？", "good": "不能", "bad": "能"},
                {"q": "偏心时？", "good": "Cpk<Cp", "bad": "Cpk>Cp"},
                {"q": "USL=UCL？", "good": "否", "bad": "是"},
            ],
            [
                {"after": "填规格", "prompt": "为何 Target=100 而均值约101.2？"},
                {"after": "读 Cpk", "prompt": "能否写成过程合格？"},
            ],
            "cap_stable_spec",
            ["为何 Cpk<Cp？", "与 Sixpack 共享？", "禁止句"],
            [{"wrong": "能力高=必须放行", "right": "指数是摘要；禁止过程合格话术。"}],
        ),
    }

    out["capability_sixpack"] = {
        "title": "过程能力 Sixpack",
        "used_for": "能力+诊断包装图。与 capability 同构共享 `cap_stable_spec`。",
        "not_for": "替代专项控制图深挖；失控集禁用。禁止过程合格。",
        "scenario": "同一稳定略偏心集。Sixpack 一眼对照稳定性与能力。",
        "related_ids": ["capability", "imr", "xbar_r"],
        "dialog_fill": {"variables": "厚度_um"},
        "click_steps": [
            "导入 `demo_cap_stable_spec`。",
            "菜单：质量工具 → 过程能力 Sixpack。",
            "变量=`厚度_um`；子组大小=1；LSL=95；USL=105；Target=100。",
            "对照：诊断侧应相对稳，能力侧 Cpk<Cp。禁止过程合格。",
        ],
        "dialog_fill_detail": [
            {"field": "变量 (`variables`)", "put": "厚度_um", "meaning": "Y。"},
            {"field": "子组大小 (`subgroup_size`)", "put": "1", "meaning": "单值。"},
            {"field": "LSL (`lsl`)", "put": "95", "meaning": "规格下限。"},
            {"field": "USL (`usl`)", "put": "105", "meaning": "规格上限。"},
            {"field": "Target (`target`)", "put": "100", "meaning": "目标。"},
        ],
        "glossary": [
            {"term": "Sixpack", "plain": "能力与若干诊断图的组合页。", "remember": "包装不是新公式。"},
            {"term": "同构共享", "plain": "与正态能力共享稳定略偏心集。", "remember": "白名单 cap_stable_spec。"},
            {"term": "Cpk", "plain": "含居中惩罚的能力。", "remember": "本课期望 Cpk<Cp。"},
        ],
        "buried_signals": [
            {
                "row": 1,
                "what": "稳定略偏心（同 capability）",
                "expect": "Sixpack 能力侧 Cpk<Cp；诊断侧无金标尖峰。禁止过程合格。",
            }
        ],
        "output_guide": [
            {"name": "Sixpack 页", "meaning": "对照稳与偏心；禁止已证明正态。"},
        ],
        "common_mistakes": _base_mistakes(["把 Sixpack 当放行章", "与 Box-Cox 偏态集共享"]),
        **_seven_plus(
            "用同构稳定集跑 Sixpack，说明与 capability 共享理由。",
            [
                {"q": "共享 dataset？", "good": "cap_stable_spec", "bad": "imr_spi_shift"},
                {"q": "期望？", "good": "Cpk<Cp", "bad": "过程合格"},
                {"q": "有尖峰吗？", "good": "无", "bad": "有片55"},
            ],
            [
                {"after": "导入", "prompt": "为何禁止与 I-MR 金标共享？"},
                {"after": "读 Sixpack", "prompt": "诊断稳能否写成必须停线？"},
            ],
            "cap_stable_spec",
            ["同构族？", "Cpk vs Cp", "禁止句"],
            [{"wrong": "Sixpack 绿=已证明正态", "right": "禁止已证明正态。"}],
        ),
    }

    out["between_within_capability"] = {
        "title": "组间/组内过程能力",
        "used_for": "分子组估计组间与组内变差的能力。",
        "not_for": "无子组标识的单值乱套；失控尖峰课。禁止过程合格。",
        "scenario": "子组 n=5。子组12起批均值上移。",
        "related_ids": ["capability", "xbar_r"],
        "dialog_fill": {"variables": "厚度_um", "subgroup": "子组"},
        "click_steps": [
            "导入 `demo_cap_between_within`。",
            "菜单：质量工具 → 组间/组内过程能力。",
            "测量值=`厚度_um`；子组=`子组`；LSL=95；USL=105；Target=100。",
            "对照子组12起组间台阶；禁止过程合格。",
        ],
        "dialog_fill_detail": [
            {"field": "测量值 (`variables`)", "put": "厚度_um", "meaning": "Y。"},
            {"field": "子组 (`subgroup`)", "put": "子组", "meaning": "组间/组内分解必需。"},
            {"field": "LSL (`lsl`)", "put": "95", "meaning": "规格下限。"},
            {"field": "USL (`usl`)", "put": "105", "meaning": "规格上限。"},
            {"field": "Target (`target`)", "put": "100", "meaning": "可选目标。"},
        ],
        "glossary": [
            {"term": "组内变差", "plain": "同一子组内的波动。", "remember": "本课组内相对稳。"},
            {"term": "组间变差", "plain": "子组均值之间的波动。", "remember": "子组12起抬高。"},
            {"term": "子组列", "plain": "标识每个子组的分类/编号列。", "remember": "本命令必选。"},
        ],
        "buried_signals": [
            {
                "row": 56,
                "what": "子组12起（约行56）批均值上移",
                "expect": "组间分量抬高；勿写过程合格。",
            }
        ],
        "output_guide": [
            {"name": "组间/组内能力表", "meaning": "对照子组12台阶。禁止必须停线。"},
        ],
        "common_mistakes": _base_mistakes(["漏选子组列", "用单值稳定集冒充组间结构"]),
        **_seven_plus(
            "用带组间台阶的子组集完成组间/组内能力。",
            [
                {"q": "子组列？", "good": "必选", "bad": "可省"},
                {"q": "子组12？", "good": "均值台阶", "bad": "尖峰废品"},
                {"q": "过程合格？", "good": "禁止写", "bad": "可以写"},
            ],
            [
                {"after": "选子组", "prompt": "没有子组列会发生什么？"},
                {"after": "读输出", "prompt": "组间抬高=必须停线吗？"},
            ],
            "cap_between_within",
            ["行56？", "组内vs组间", "禁止句"],
            [{"wrong": "组间大=过程合格失败已判决", "right": "是变差结构线索，禁止过程合格话术。"}],
        ),
    }

    out["binomial_capability"] = {
        "title": "二项过程能力",
        "used_for": "不合格品计数/比率的能力与 PPM 类摘要。",
        "not_for": "缺陷数（多缺陷/件）应泊松；计量型用正态能力。禁止过程合格。",
        "scenario": "批18起不合格率抬高。",
        "related_ids": ["poisson_capability", "p_chart"],
        "dialog_fill": {"defectives": "不合格品数", "inspected": "检验数"},
        "click_steps": [
            "导入 `demo_cap_binomial_lots`。",
            "菜单：质量工具 → 二项过程能力。",
            "不合格品数=`不合格品数`；检验数（列）=`检验数`；检验数常数留空；目标不合格品率可选 0.02。",
            "对照批18起抬高；禁止过程合格。",
        ],
        "dialog_fill_detail": [
            {"field": "不合格品数 (`defectives`)", "put": "不合格品数", "meaning": "不良件数。"},
            {"field": "检验数（列） (`inspected`)", "put": "检验数", "meaning": "可变 n；与常数二选一。"},
            {"field": "检验数（常数） (`inspected_constant`)", "put": "留空", "meaning": "已用列则留空。"},
            {"field": "目标不合格品率 (`target`)", "put": "0.02（可选）", "meaning": "目标比率。"},
        ],
        "glossary": [
            {"term": "不合格品", "plain": "整件判不良的计数。", "remember": "一件一票，不是缺陷数。"},
            {"term": "二项能力", "plain": "基于不合格品率的能力摘要。", "remember": "禁止写成过程合格。"},
            {"term": "检验数", "plain": "每批抽检件数。", "remember": "可变 n 用列。"},
        ],
        "buried_signals": [
            {
                "row": 18,
                "what": "批18起不合格率由约2%抬到约7%",
                "expect": "二项能力/PPM 线索变差；勿写过程合格。",
            }
        ],
        "output_guide": [
            {"name": "二项能力输出", "meaning": "对照批18后段。禁止必须停线。"},
        ],
        "common_mistakes": _base_mistakes(["把缺陷数当不合格品数", "漏检验数"]),
        **_seven_plus(
            "完成二项能力课，对照批18不合格率台阶。",
            [
                {"q": "Y 是？", "good": "不合格品数", "bad": "缺陷数"},
                {"q": "批18？", "good": "比率抬高", "bad": "过程合格"},
                {"q": "检验数？", "good": "用列", "bad": "可忽略"},
            ],
            [
                {"after": "选列", "prompt": "为何常数检验数要留空？"},
                {"after": "读输出", "prompt": "PPM 高=必须停线吗？"},
            ],
            "cap_binomial_lots",
            ["批18？", "二项vs泊松", "禁止句"],
            [{"wrong": "二项能力=已证明过程合格", "right": "禁止过程合格。"}],
        ),
    }

    out["poisson_capability"] = {
        "title": "泊松过程能力",
        "used_for": "缺陷计数/DPU 的泊松能力摘要。",
        "not_for": "不合格品件数用二项；计量型用正态能力。禁止过程合格。",
        "scenario": "批16起单位缺陷率抬高。",
        "related_ids": ["binomial_capability", "u_chart"],
        "dialog_fill": {"defects": "缺陷数", "units": "单位数"},
        "click_steps": [
            "导入 `demo_cap_poisson_counts`。",
            "菜单：质量工具 → 泊松过程能力。",
            "缺陷数=`缺陷数`；单位数（列）=`单位数`；单位数常数留空；目标 DPU 可选。",
            "对照批16起抬高；禁止过程合格。",
        ],
        "dialog_fill_detail": [
            {"field": "缺陷数 (`defects`)", "put": "缺陷数", "meaning": "缺陷计数 Y。"},
            {"field": "单位数（列） (`units`)", "put": "单位数", "meaning": "可变单位；与常数二选一。"},
            {"field": "单位数（常数） (`inspected_constant`)", "put": "留空", "meaning": "已用列则留空。"},
            {"field": "目标 DPU (`target`)", "put": "留空或 0.03", "meaning": "可选目标。"},
        ],
        "glossary": [
            {"term": "DPU", "plain": "单位缺陷数。", "remember": "泊松能力关注缺陷率。"},
            {"term": "缺陷 vs 不合格品", "plain": "一件可有多缺陷。", "remember": "选错命令会错读。"},
            {"term": "单位数", "plain": "面积/件数等暴露量。", "remember": "可变时用列。"},
        ],
        "buried_signals": [
            {
                "row": 16,
                "what": "批16起缺陷率抬高",
                "expect": "泊松能力/DPU 线索变差；勿写过程合格。",
            }
        ],
        "output_guide": [
            {"name": "泊松能力输出", "meaning": "对照批16。禁止已证明正态。"},
        ],
        "common_mistakes": _base_mistakes(["与二项命令互换", "漏单位数"]),
        **_seven_plus(
            "完成泊松能力课，对照批16 DPU 台阶。",
            [
                {"q": "Y？", "good": "缺陷数", "bad": "不合格品数"},
                {"q": "批16？", "good": "DPU 抬高", "bad": "量具通过"},
                {"q": "单位数？", "good": "用列", "bad": "可忽略"},
            ],
            [
                {"after": "选列", "prompt": "为何不用二项命令？"},
                {"after": "读输出", "prompt": "DPU 高能否写过程合格？"},
            ],
            "cap_poisson_counts",
            ["批16？", "缺陷vs不合格品", "禁止句"],
            [{"wrong": "泊松能力证明过程合格", "right": "禁止过程合格。"}],
        ),
    }

    out["box_cox"] = {
        "title": "Box-Cox 变换",
        "used_for": "为正值偏态数据寻找 λ 变换改善对称性。",
        "not_for": "已稳定正态能力课；含非正值。禁止已证明正态。",
        "scenario": "右偏厚度。约行45–48 右尾极大。",
        "related_ids": ["distribution_identification", "capability"],
        "dialog_fill": {"variables": "厚度_um"},
        "click_steps": [
            "导入 `demo_dist_skew_boxcox`。",
            "菜单：质量工具 → Box-Cox 变换。",
            "正值变量=`厚度_um`；Lambda 留空自动搜索；LSL/USL 留空。",
            "对照右偏与右尾；看推荐 λ。禁止已证明正态。",
        ],
        "dialog_fill_detail": [
            {"field": "正值变量 (`variables`)", "put": "厚度_um", "meaning": "必须为正。"},
            {"field": "Lambda（可选） (`lambda`)", "put": "留空", "meaning": "留空自动搜索 -5..5。"},
            {"field": "LSL（可选） (`lsl`)", "put": "留空", "meaning": "本课只看变换。"},
            {"field": "USL（可选） (`usl`)", "put": "留空", "meaning": "本课只看变换。"},
        ],
        "glossary": [
            {"term": "Box-Cox", "plain": "幂变换族，由 λ 参数化。", "remember": "λ=1 近似不变换。"},
            {"term": "右偏", "plain": "长右尾。", "remember": "本集故意右偏。"},
            {"term": "正值约束", "plain": "经典 Box-Cox 要求 y>0。", "remember": "零/负值需别法。"},
        ],
        "buried_signals": [
            {
                "row": 45,
                "what": "行45–48 右尾极大值拉长偏态",
                "expect": "推荐 λ 远离 1；勿写已证明正态。",
            }
        ],
        "output_guide": [
            {"name": "λ 搜索/对比", "meaning": "指着推荐 λ。禁止已证明正态。"},
        ],
        "common_mistakes": _base_mistakes(["与 cap_stable_spec 共享", "变换后宣称已证明正态"]),
        **_seven_plus(
            "在右偏集上跑 Box-Cox，解释为何不与稳定能力集共享。",
            [
                {"q": "能否共享 cap_stable_spec？", "good": "不能", "bad": "能"},
                {"q": "行45–48？", "good": "右尾极大", "bad": "过程合格"},
                {"q": "已证明正态？", "good": "禁止写", "bad": "可以写"},
            ],
            [
                {"after": "留空 λ", "prompt": "为何先自动搜索？"},
                {"after": "读 λ", "prompt": "λ 接近1意味着什么？"},
            ],
            "dist_skew_boxcox",
            ["右偏？", "与能力集", "禁止句"],
            [{"wrong": "变换后=已证明正态", "right": "禁止已证明正态。"}],
        ),
    }

    out["distribution_identification"] = {
        "title": "个体分布识别",
        "used_for": "比较候选分布拟合，为后续非正态能力等选分布。",
        "not_for": "证明唯一真分布；小样本过度自信。禁止已证明正态。",
        "scenario": "近似对数正态正值样本。",
        "related_ids": ["box_cox", "nonnormal_capability", "normality_test"],
        "dialog_fill": {"variables": "测量值"},
        "click_steps": [
            "导入 `demo_dist_id_candidates`。",
            "菜单：质量工具 → 个体分布识别。",
            "测量值=`测量值`。",
            "对照：对数正态/Weibull 等应优于正态；禁止已证明正态。",
        ],
        "dialog_fill_detail": [
            {"field": "测量值 (`variables`)", "put": "测量值", "meaning": "单列正值/连续 Y。"},
        ],
        "glossary": [
            {"term": "候选分布", "plain": "正态/对数正态/Weibull 等比较集。", "remember": "选相对更好，非唯一真理。"},
            {"term": "拟合优度线索", "plain": "图与统计量辅助比较。", "remember": "禁止已证明正态。"},
            {"term": "后续用途", "plain": "常为非正态能力选分布。", "remember": "本课 dataset 空挂非正态课。"},
        ],
        "buried_signals": [
            {
                "row": 1,
                "what": "全列近似对数正态生成",
                "expect": "识别结果中对数正态类优于正态；勿写已证明正态。",
            }
        ],
        "output_guide": [
            {"name": "分布比较表/图", "meaning": "相对排序；禁止已证明正态。"},
        ],
        "common_mistakes": _base_mistakes(["把第一名写成唯一真分布", "小 n 过度解读"]),
        **_seven_plus(
            "完成分布识别，说明相对拟合而非证明正态。",
            [
                {"q": "本集形状？", "good": "近对数正态", "bad": "标准正态尖峰"},
                {"q": "已证明正态？", "good": "禁止", "bad": "可以"},
                {"q": "角色？", "good": "variables=测量值", "bad": "随便"},
            ],
            [
                {"after": "跑识别", "prompt": "第一名能否当唯一真理？"},
                {"after": "对照正态", "prompt": "为何禁止已证明正态？"},
            ],
            "dist_id_candidates",
            ["形状？", "与 Box-Cox", "禁止句"],
            [{"wrong": "p 大=已证明正态", "right": "禁止已证明正态。"}],
        ),
    }
    return out


def _build_quality_graphs() -> dict[str, dict]:
    out: dict[str, dict] = {}
    out["pareto"] = {
        "title": "柏拉图",
        "used_for": "按缺陷类别频数抓主要问题。",
        "not_for": "计量控制图；因果机制证明。禁止过程合格。",
        "scenario": "虚焊/偏移占绝大多数。",
        "related_ids": ["cause_and_effect", "c_chart"],
        "dialog_fill": {"category": "缺陷类别"},
        "click_steps": [
            "导入 `demo_pareto_defect_tail`。",
            "菜单：质量工具 → 柏拉图。",
            "缺陷类别=`缺陷类别`；计数列留空（按行计频数）；Other 合并阈值可试 95。",
            "对照前两类累计约80%；禁止过程合格。",
        ],
        "dialog_fill_detail": [
            {"field": "缺陷类别 (`category`)", "put": "缺陷类别", "meaning": "分类标签。"},
            {"field": "计数列 (`counts`)", "put": "留空", "meaning": "留空则按行计数。"},
            {"field": "Other 合并阈值（可选 %） (`other_threshold`)", "put": "95（可选）", "meaning": "累计到阈值后合并尾部。"},
        ],
        "glossary": [
            {"term": "柏拉图", "plain": "按频数降序+累计百分比。", "remember": "抓少数主要类别。"},
            {"term": "80/20 线索", "plain": "少数类别占多数缺陷。", "remember": "是优先序，不是合格判决。"},
            {"term": "Other", "plain": "尾部合并。", "remember": "阈值可调。"},
        ],
        "buried_signals": [
            {
                "row": 1,
                "what": "配方保证虚焊+偏移约占前80%计数",
                "expect": "柏拉图左侧两根最高；勿写过程合格。",
            }
        ],
        "output_guide": [
            {"name": "柏拉图", "meaning": "指着前两类。禁止必须停线。"},
        ],
        "common_mistakes": _base_mistakes(["把累计80%写成法律阈值", "用计量列当类别"]),
        **_seven_plus(
            "用长尾缺陷集完成柏拉图，指出主要类别。",
            [
                {"q": "主类别？", "good": "虚焊/偏移", "bad": "其他"},
                {"q": "计数列？", "good": "可留空", "bad": "必须虚构"},
                {"q": "过程合格？", "good": "禁止写", "bad": "可以"},
            ],
            [
                {"after": "画图", "prompt": "为何先看累计百分比？"},
                {"after": "调 Other", "prompt": "阈值改变故事吗？"},
            ],
            "pareto_defect_tail",
            ["前两类？", "Other？", "禁止句"],
            [{"wrong": "柏拉图证明根因", "right": "只排序频数；根因要另做。"}],
        ),
    }

    out["cause_and_effect"] = {
        "title": "因果图（鱼骨）",
        "used_for": "整理头脑风暴的类别-原因结构。",
        "not_for": "统计显著性检验；自动证明根因。禁止过程合格。",
        "scenario": "焊点不良 5M1E 清单，含「钢网张力未校准」。",
        "related_ids": ["pareto", "multi_vari"],
        "dialog_fill": {"category": "类别", "cause": "原因"},
        "click_steps": [
            "导入 `demo_fishbone_solder_causes`。",
            "菜单：质量工具 → 因果图。",
            "类别列=`类别`；原因列=`原因`；效应标题=`焊点不良`。",
            "对照「方法/钢网张力未校准」等分枝；禁止过程合格。",
        ],
        "dialog_fill_detail": [
            {"field": "类别列 (`category`)", "put": "类别", "meaning": "鱼骨大枝。"},
            {"field": "原因列 (`cause`)", "put": "原因", "meaning": "具体原因短句。"},
            {"field": "效应标题 (`effect_title`)", "put": "焊点不良", "meaning": "鱼头效应名。"},
        ],
        "glossary": [
            {"term": "鱼骨图", "plain": "按类别展开可能原因。", "remember": "结构化讨论，不是检验。"},
            {"term": "5M1E", "plain": "人机料法环测等常用分类。", "remember": "分类可自定义。"},
            {"term": "效应", "plain": "要解释的问题结果。", "remember": "本课效应=焊点不良。"},
        ],
        "buried_signals": [
            {
                "row": 8,
                "what": "行含「方法 / 钢网张力未校准」（约行8）",
                "expect": "方法枝可见该原因；勿写已证明根因或过程合格。",
            }
        ],
        "output_guide": [
            {"name": "因果图", "meaning": "按类别分枝。禁止必须停线。"},
        ],
        "common_mistakes": _base_mistakes(["把鱼骨当统计证明", "效应标题留空却抱怨无标题"]),
        **_seven_plus(
            "完成鱼骨图，指出方法枝关键原因条目。",
            [
                {"q": "两列？", "good": "类别+原因", "bad": "只要测量值"},
                {"q": "鱼骨证明根因？", "good": "否", "bad": "是"},
                {"q": "效应标题？", "good": "焊点不良", "bad": "可随便空且抱怨"},
            ],
            [
                {"after": "填效应", "prompt": "效应标题作用？"},
                {"after": "读图", "prompt": "列出原因=已证明吗？"},
            ],
            "fishbone_solder_causes",
            ["钢网张力？", "5M1E", "禁止句"],
            [{"wrong": "鱼骨=根因已判决", "right": "只是结构化假设列表。"}],
        ),
    }

    out["multi_vari"] = {
        "title": "Multi-Vari 图",
        "used_for": "用 2–4 个因子分层看测量值模式。",
        "not_for": "正式 ANOVA 推断；1 个因子不够。禁止过程合格。",
        "scenario": "腔位×时段。晚班均值抬高。",
        "related_ids": ["variability_chart", "two_factor_anova"],
        "dialog_fill": {"measurement": "厚度_um"},
        "click_steps": [
            "导入 `demo_multi_vari_pos_time`。",
            "菜单：质量工具 → Multi-Vari 图。",
            "测量值=`厚度_um`；因子选 `腔位` 与 `时段`（共2列，满足2–4）。",
            "对照晚班抬高；禁止过程合格。",
        ],
        "dialog_fill_detail": [
            {"field": "测量值 (`measurement`)", "put": "厚度_um", "meaning": "Y。"},
            {"field": "因子（2～4 列） (`factors`)", "put": "腔位,时段", "meaning": "多选角色；必须 2–4 列。"},
        ],
        "glossary": [
            {"term": "Multi-Vari", "plain": "分层多变异可视化。", "remember": "探索主效应线索。"},
            {"term": "因子角色", "plain": "允许多列。", "remember": "本软件要求 2–4 个。"},
            {"term": "时段效应", "plain": "本课晚班抬高。", "remember": "不是停线指令。"},
        ],
        "buried_signals": [
            {
                "row": 25,
                "what": "约行25起进入晚班区块，均值上移",
                "expect": "Multi-Vari 显示时段差异大于腔位；勿写过程合格。",
            }
        ],
        "output_guide": [
            {"name": "Multi-Vari 图", "meaning": "指着晚班。禁止必须停线。"},
        ],
        "common_mistakes": _base_mistakes(["只选1个因子", "把图当 ANOVA p 值"]),
        **_seven_plus(
            "完成 Multi-Vari，指出晚班主效应线索。",
            [
                {"q": "因子个数？", "good": "2–4", "bad": "1"},
                {"q": "晚班？", "good": "均值抬高", "bad": "过程合格"},
                {"q": "可当 ANOVA？", "good": "否，探索图", "bad": "是正式检验"},
            ],
            [
                {"after": "选因子", "prompt": "为何至少两个因子？"},
                {"after": "读图", "prompt": "时段差=必须停线吗？"},
            ],
            "multi_vari_pos_time",
            ["行25？", "因子角色", "禁止句"],
            [{"wrong": "Multi-Vari 证明交互显著", "right": "探索图，不替代设计好的推断。"}],
        ),
    }

    out["run_chart"] = {
        "title": "运行图",
        "used_for": "按时间顺序看中位数、簇与趋势线索。",
        "not_for": "替代控制图控制限；规格符合性。禁止过程合格。",
        "scenario": "片28–40 相对中位数同侧偏高。",
        "related_ids": ["imr", "zone_chart"],
        "dialog_fill": {"variables": "厚度_um"},
        "click_steps": [
            "导入 `demo_run_chart_median_trend`。",
            "菜单：质量工具 → 运行图。",
            "数值观测=`厚度_um`。",
            "对照片28–40 同侧游程；中位数≠UCL。禁止过程合格。",
        ],
        "dialog_fill_detail": [
            {"field": "数值观测（一列） (`variables`)", "put": "厚度_um", "meaning": "单列时间序 Y。"},
        ],
        "glossary": [
            {"term": "运行图", "plain": "按序描点并对照中位数等。", "remember": "无 Shewhart 控制限。"},
            {"term": "中位数", "plain": "样本中位水平。", "remember": "中位数 ≠ UCL/USL。"},
            {"term": "同侧游程", "plain": "连续落在中位数同侧。", "remember": "本课片28–40。"},
        ],
        "buried_signals": [
            {
                "row": 28,
                "what": "片28–40 连续在中位数上方",
                "expect": "运行图标记簇/趋势线索；勿把中位数当 UCL，勿写过程合格。",
            }
        ],
        "output_guide": [
            {"name": "运行图", "meaning": "指着片28–40。中位数≠UCL。禁止必须停线。"},
        ],
        "common_mistakes": _base_mistakes(["把中位数当 UCL", "当成规格合格图"]),
        **_seven_plus(
            "完成运行图课，区分中位数与控制限。",
            [
                {"q": "中位数=UCL？", "good": "否", "bad": "是"},
                {"q": "片28–40？", "good": "同侧游程", "bad": "废品"},
                {"q": "过程合格？", "good": "禁止写", "bad": "可以"},
            ],
            [
                {"after": "画图", "prompt": "运行为何不是控制图？"},
                {"after": "读游程", "prompt": "同侧=必须停线吗？"},
            ],
            "run_chart_median_trend",
            ["中位数≠UCL", "片28", "禁止句"],
            [{"wrong": "点出中位数=超规格", "right": "中位数不是 USL/UCL。"}],
        ),
    }
    return out


def _build_formula_refs() -> dict[str, dict]:
    out: dict[str, dict] = {}

    out["acceptance_sampling"] = _formula_skeleton(
        cid="acceptance_sampling",
        title="属性一次抽样",
        menu="质量工具 → 属性一次抽样",
        used_for="给定 n/c（及可选 AQL/RQL）讨论一次抽样方案的接收特性。",
        not_for="过程能力指数；在线 SPC。本课无演示表。",
        scenario="来料一次抽样方案教学：在对话框填 n 与 c，不看工作表。",
        related=["binomial_capability", "p_chart"],
        glossary=[
            {"term": "n / c", "plain": "样本量与接收数。", "remember": "方案参数，不是测量列。"},
            {"term": "AQL / RQL", "plain": "可接收/拒收质量水平（可选输入）。", "remember": "比例在 [0,1]。"},
            {"term": "无需数据", "plain": "requires_data=false。", "remember": "dataset 空是诚实的。"},
        ],
        detail=[
            {"field": "样本量 n (`sample_size`)", "put": "例如 20", "meaning": "必须 ≥1。"},
            {"field": "接收数 c (`acceptance_number`)", "put": "例如 1", "meaning": "0…n。"},
            {"field": "AQL（可选） (`aql`)", "put": "留空或 0.01", "meaning": "[0,1]。"},
            {"field": "RQL（可选） (`rql`)", "put": "留空或 0.05", "meaning": "[0,1]。"},
            {"field": "批大小 N（可选） (`lot_size`)", "put": "留空或 1000", "meaning": "信息用。"},
        ],
        mission="能默写一次抽样对话框字段，并说明本课为何无 dataset。",
        prereq=[
            {"q": "需要导入工作表吗？", "good": "不需要", "bad": "必须"},
            {"q": "n 含义？", "good": "样本量", "bad": "规格上限"},
            {"q": "可写过程合格？", "good": "禁止", "bad": "可以"},
        ],
        self_explain=[
            {"after": "读字段", "prompt": "为何 AQL 是可选输入？"},
            {"after": "对照帮助", "prompt": "抽样方案≠能力指数，差在哪？"},
        ],
        retrieval=["n/c 是什么？", "本课 dataset？", "禁止句"],
        misc=[{"wrong": "抽样接收=过程合格", "right": "方案接收决策≠过程能力结论。"}],
    )

    out["attribute_agreement"] = _formula_skeleton(
        cid="attribute_agreement",
        title="属性一致性分析",
        menu="质量工具 → 属性一致性分析",
        used_for="评估员对属性评级的一致性（公式参考）。",
        not_for="计量 Gage R&R。菜单可能不可用；dataset 空。",
        scenario="若菜单可用，需评级/部件/评估者列；本波不提供演示表。",
        related=["gage_rr", "msa_type1"],
        glossary=[
            {"term": "属性一致性", "plain": "评级一致程度（κ 等）。", "remember": "不是 %GR&R。"},
            {"term": "评估者", "plain": "appraiser 角色。", "remember": "与操作员计量 MSA 不同。"},
            {"term": "公式参考", "plain": "implemented_status=formula_reference。", "remember": "步骤须诚实。"},
        ],
        detail=[
            {"field": "评级 (`rating`)", "put": "（无演示表）评级列", "meaning": "属性结果。"},
            {"field": "部件 (`part`)", "put": "部件列", "meaning": "样品标识。"},
            {"field": "评估者 (`appraiser`)", "put": "评估者列", "meaning": "人。"},
            {"field": "标准（可选） (`standard`)", "put": "可留空", "meaning": "金标准列可选。"},
            {"field": "有序评级 (`ordinal`)", "put": "false", "meaning": "true 时算 Kendall 等。"},
            {"field": "Kappa 权重 (`kappa_weight`)", "put": "目录默认", "meaning": "kappa_weight_method。"},
        ],
        mission="能列出属性一致性真实 roles/inputs，并说明本波空表原因。",
        prereq=[
            {"q": "与 Crossed Gage 相同？", "good": "否", "bad": "是"},
            {"q": "本波有表？", "good": "无", "bad": "有"},
            {"q": "量具通过？", "good": "禁止写", "bad": "可以"},
        ],
        self_explain=[
            {"after": "读 roles", "prompt": "为何有 appraiser 而不是 operator？"},
            {"after": "对照帮助", "prompt": "菜单不可用时学什么？"},
        ],
        retrieval=["四角色？", "ordinal？", "禁止句"],
        misc=[{"wrong": "κ 高=量具通过", "right": "禁止量具通过。"}],
        extra_mistakes=["为本课伪造演示宽表冒充已实现"],
    )

    out["batch_capability"] = _formula_skeleton(
        cid="batch_capability",
        title="批次过程能力",
        menu="质量工具 → 批次过程能力",
        used_for="按批次汇总的能力分析（公式参考）。",
        not_for="无批次列的普通能力。dataset 空。",
        scenario="需要测量值+批次列；本波不灌演示表。",
        related=["capability", "between_within_capability"],
        glossary=[
            {"term": "批次能力", "plain": "按 batch 分层的能力。", "remember": "要有批次列。"},
            {"term": "最小批次 N", "plain": "过小批次可剔除。", "remember": "默认≥2。"},
            {"term": "公式参考", "plain": "菜单可能不可用。", "remember": "对照帮助。"},
        ],
        detail=[
            {"field": "测量值 (`measurement`)", "put": "（无表）测量列", "meaning": "Y。"},
            {"field": "批次 (`batch`)", "put": "批次列", "meaning": "必选。"},
            {"field": "LSL (`lsl`)", "put": "规格下限", "meaning": "规格。"},
            {"field": "USL (`usl`)", "put": "规格上限", "meaning": "规格。"},
            {"field": "最小批次 N (`min_batch_size`)", "put": "2", "meaning": "过滤过小批。"},
        ],
        mission="能默写批次能力对话框字段，并拒绝过程合格话术。",
        prereq=[
            {"q": "需要批次列？", "good": "是", "bad": "否"},
            {"q": "本波 dataset？", "good": "空", "bad": "cap_stable_spec"},
            {"q": "过程合格？", "good": "禁止", "bad": "可以"},
        ],
        self_explain=[
            {"after": "读字段", "prompt": "与 between_within 的子组有何不同？"},
            {"after": "对照帮助", "prompt": "为何本波不硬塞表？"},
        ],
        retrieval=["roles？", "min_batch_size？", "禁止句"],
        misc=[{"wrong": "批次能力=过程合格章", "right": "禁止过程合格。"}],
    )

    out["expanded_gage_unbalanced"] = _formula_skeleton(
        cid="expanded_gage_unbalanced",
        title="不平衡 Expanded Gage R&R",
        menu="统计 → 不平衡 Expanded Gage R&R",
        used_for="不平衡扩展量具（公式参考；菜单在统计）。",
        not_for="平衡三因子用 expanded_gage_rr。dataset 空。",
        scenario="需要测量/Part/Operator；附加因子为输入开关。本波空表。",
        related=["expanded_gage_rr", "gage_rr"],
        glossary=[
            {"term": "不平衡", "plain": "各组合重复数不齐。", "remember": "与平衡课分开。"},
            {"term": "菜单路径", "plain": "在「统计」而非质量工具包。", "remember": "对照真实 menu_path。"},
            {"term": "附加因子开关", "plain": "input `additional` true/false。", "remember": "不是列角色。"},
        ],
        detail=[
            {"field": "测量 (`measurement`)", "put": "（无表）测量列", "meaning": "Y。"},
            {"field": "Part (`part`)", "put": "零件列", "meaning": "必选。"},
            {"field": "Operator (`operator`)", "put": "操作员列", "meaning": "必选。"},
            {"field": "附加因子 (`additional`)", "put": "false", "meaning": "input 开关，不是列。"},
        ],
        mission="能指出本命令在统计菜单，并区分平衡 Expanded 课。",
        prereq=[
            {"q": "菜单在？", "good": "统计", "bad": "质量工具"},
            {"q": "本波有表？", "good": "无", "bad": "msa_expanded_crossed"},
            {"q": "量具通过？", "good": "禁止", "bad": "可以"},
        ],
        self_explain=[
            {"after": "看 menu_path", "prompt": "为何不进质量工具波的共享表？"},
            {"after": "读 additional", "prompt": "它是角色还是 input？"},
        ],
        retrieval=["菜单？", "additional？", "禁止句"],
        misc=[{"wrong": "可与平衡表共享", "right": "锁表空 dataset；结构不同。"}],
        extra_mistakes=["偷偷挂 msa_expanded_crossed"],
    )

    out["nonnormal_capability"] = _formula_skeleton(
        cid="nonnormal_capability",
        title="非正态过程能力",
        menu="质量工具 → 非正态过程能力",
        used_for="选定非正态族后的能力（公式参考）。",
        not_for="稳定正态课；先识别分布。dataset 空。",
        scenario="需要变量+规格+分布枚举；本波不灌表。",
        related=["distribution_identification", "capability", "box_cox"],
        glossary=[
            {"term": "非正态能力", "plain": "按选定分布算能力。", "remember": "分布选择先于指数。"},
            {"term": "distribution 输入", "plain": "目录枚举。", "remember": "勿虚构。"},
            {"term": "公式参考", "plain": "菜单可能不可用。", "remember": "步骤诚实。"},
        ],
        detail=[
            {"field": "变量 (`variables`)", "put": "（无表）测量列", "meaning": "Y。"},
            {"field": "LSL (`lsl`)", "put": "规格下限", "meaning": "规格。"},
            {"field": "USL (`usl`)", "put": "规格上限", "meaning": "规格。"},
            {"field": "Target (`target`)", "put": "可选", "meaning": "目标。"},
            {"field": "分布 (`distribution`)", "put": "目录默认", "meaning": "nonnormal_capability_distribution。"},
        ],
        mission="能列出非正态能力字段，并指向分布识别课。",
        prereq=[
            {"q": "先做什么？", "good": "分布识别/变换讨论", "bad": "直接写过程合格"},
            {"q": "本波表？", "good": "空", "bad": "cap_stable_spec"},
            {"q": "已证明正态？", "good": "禁止", "bad": "可以"},
        ],
        self_explain=[
            {"after": "读分布输入", "prompt": "为何要目录枚举？"},
            {"after": "对照识别课", "prompt": "为何不共享偏态表到本课？"},
        ],
        retrieval=["字段？", "related_ids？", "禁止句"],
        misc=[{"wrong": "选了 Weibull=已证明", "right": "禁止已证明正态/合格。"}],
    )

    out["nonparametric_capability"] = _formula_skeleton(
        cid="nonparametric_capability",
        title="非参数过程能力",
        menu="质量工具 → 非参数过程能力",
        used_for="不依赖参数分布假设的能力摘要（公式参考）。",
        not_for="替代所有正态能力场景。dataset 空。",
        scenario="测量值+LSL+USL 必填；容差 K 默认6。",
        related=["capability", "nonnormal_capability"],
        glossary=[
            {"term": "非参数能力", "plain": "少依赖分布形状的能力。", "remember": "仍要规格。"},
            {"term": "容差 K", "plain": "尺度参数，默认6。", "remember": "不是 UCL。"},
            {"term": "公式参考", "plain": "本波空表。", "remember": "诚实步骤。"},
        ],
        detail=[
            {"field": "测量值 (`measurement`)", "put": "（无表）测量列", "meaning": "Y。"},
            {"field": "LSL (`lsl`)", "put": "必填", "meaning": "缺则报错。"},
            {"field": "USL (`usl`)", "put": "必填", "meaning": "缺则报错。"},
            {"field": "容差 K (`tolerance_k`)", "put": "6", "meaning": "默认6。"},
        ],
        mission="能说明非参数能力必填规格，并拒绝过程合格话术。",
        prereq=[
            {"q": "LSL/USL？", "good": "必填", "bad": "可空"},
            {"q": "本波表？", "good": "空", "bad": "有"},
            {"q": "过程合格？", "good": "禁止", "bad": "可以"},
        ],
        self_explain=[
            {"after": "读报错条件", "prompt": "为何规格必填？"},
            {"after": "对照正态课", "prompt": "何时才考虑非参数？"},
        ],
        retrieval=["tolerance_k？", "必填？", "禁止句"],
        misc=[{"wrong": "非参数=无需规格", "right": "本实现仍要 LSL/USL。"}],
    )

    out["tolerance_intervals"] = _formula_skeleton(
        cid="tolerance_intervals",
        title="容差区间",
        menu="质量工具 → 容差区间",
        used_for="覆盖总体比例的统计容差区间（公式参考）。",
        not_for="规格限本身；置信区间混淆。dataset 空。",
        scenario="测量值+方法/覆盖率/置信/方向。本波空表。",
        related=["capability", "one_sample_t"],
        glossary=[
            {"term": "容差区间", "plain": "以置信水平覆盖总体至少一定比例。", "remember": "≠规格限。"},
            {"term": "覆盖率", "plain": "要盖住的总体比例。", "remember": "常与置信水平一起设。"},
            {"term": "公式参考", "plain": "菜单可能不可用。", "remember": "对照帮助。"},
        ],
        detail=[
            {"field": "测量值 (`measurement`)", "put": "（无表）测量列", "meaning": "Y。"},
            {"field": "方法 (`method`)", "put": "目录默认", "meaning": "tolerance_method。"},
            {"field": "覆盖率 (%) (`coverage`)", "put": "95", "meaning": "覆盖比例。"},
            {"field": "置信水平 (%) (`confidence`)", "put": "95", "meaning": "置信。"},
            {"field": "区间方向 (`alternative`)", "put": "目录默认", "meaning": "tolerance_interval_alternative。"},
        ],
        mission="能区分容差区间与规格限，并列出真实 inputs。",
        prereq=[
            {"q": "容差区间=USL？", "good": "否", "bad": "是"},
            {"q": "本波表？", "good": "空", "bad": "有"},
            {"q": "过程合格？", "good": "禁止", "bad": "可以"},
        ],
        self_explain=[
            {"after": "读覆盖率", "prompt": "覆盖率与置信水平差在哪？"},
            {"after": "对照规格", "prompt": "为何不能写成过程合格？"},
        ],
        retrieval=["覆盖率？", "≠规格", "禁止句"],
        misc=[{"wrong": "容差区间内=过程合格", "right": "禁止过程合格。"}],
    )

    out["variability_chart"] = _formula_skeleton(
        cid="variability_chart",
        title="变异性图",
        menu="质量工具 → 变异性图",
        used_for="1–2 因子下的变异性可视化（公式参考）。",
        not_for="Multi-Vari（2–4 因子）直接替代。dataset 空。",
        scenario="测量值+factors(1–2)。本波空表；探索可用 multi_vari 课。",
        related=["multi_vari", "gage_rr"],
        glossary=[
            {"term": "变异性图", "plain": "展示因子水平下的散度。", "remember": "探索图。"},
            {"term": "因子数", "plain": "本命令 1–2 列。", "remember": "Multi-Vari 是 2–4。"},
            {"term": "公式参考", "plain": "本波空表。", "remember": "可 related 到 multi_vari。"},
        ],
        detail=[
            {"field": "测量值 (`measurement`)", "put": "（无表）测量列", "meaning": "Y。"},
            {"field": "因子（1～2 列） (`factors`)", "put": "1–2 个因子列", "meaning": "多选角色。"},
        ],
        mission="能区分变异性图与 Multi-Vari 的因子个数约束。",
        prereq=[
            {"q": "因子个数？", "good": "1–2", "bad": "必须4"},
            {"q": "本波表？", "good": "空", "bad": "multi_vari_pos_time"},
            {"q": "过程合格？", "good": "禁止", "bad": "可以"},
        ],
        self_explain=[
            {"after": "对照 multi_vari", "prompt": "因子个数要求差在哪？"},
            {"after": "读帮助", "prompt": "为何本波不共享 multi_vari 表？"},
        ],
        retrieval=["因子数？", "related？", "禁止句"],
        misc=[{"wrong": "变异性图=ANOVA 显著", "right": "探索图，不替代推断。"}],
        extra_mistakes=["偷挂 multi_vari_pos_time"],
    )

    return out


def write_overlays() -> None:
    from copy_depth import polish_overlay

    OVERLAY_DIR.mkdir(parents=True, exist_ok=True)
    overlays = build_overlays()
    assert len(overlays) == 24, sorted(overlays)
    for cid, payload in overlays.items():
        payload = polish_overlay(cid, payload)
        path = OVERLAY_DIR / f"{cid}.json"
        path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(overlays)} Wave-2 overlays to {OVERLAY_DIR}")


if __name__ == "__main__":
    write_overlays()
