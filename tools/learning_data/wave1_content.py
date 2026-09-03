#!/usr/bin/env python3
"""Wave-1 control-chart datasets, role maps, generators, and overlay writers.

Import from build_learning_dataset_mapping / build_learning_center_db.
Run `python tools/learning_data/wave1_content.py` to (re)write overlay JSON files.
"""
from __future__ import annotations

import json
import math
import random
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OVERLAY_DIR = Path(__file__).resolve().parent / "tutorial_overlays"

# ---------------------------------------------------------------------------
# Dataset catalog (mapping builder)
# ---------------------------------------------------------------------------

WAVE1_DATASETS: dict[str, dict] = {
    "c_chart_defect_step": {
        "wave": 1,
        "practice_only": False,
        "title": "回流焊焊点缺陷计数（固定单位台阶）",
        "industry": "electronics",
        "story": "每炉固定检验 1 个标准托盘。前段缺陷数稳定，批26起均值台阶抬高。",
        "row_count": 40,
        "notes": "埋点：批26（行26）起缺陷数由基线约3抬到约8；期望 C 图后段上移或越 UCL。固定单位，不要当 u 图。UCL≠USL。",
        "columns": [
            {"index": 0, "name": "批号", "role_hint": "order", "description": "1–40 炉次"},
            {"index": 1, "name": "焊点缺陷数", "role_hint": "defects", "description": "固定托盘内缺陷计数"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "基线/台阶；不进对话框"},
        ],
    },
    "spc_small_drift": {
        "wave": 1,
        "practice_only": False,
        "title": "贴片厚度微小持续漂移（EWMA/CUSUM 同构）",
        "industry": "electronics",
        "story": "单值厚度序列。前段稳定，片31起约 +0.8μm 小台阶（约 2σ），不是尖峰。服务 ewma+cusum。",
        "row_count": 60,
        "notes": "埋点：片31（行31）起均值由约100μm 抬到约100.8μm（小漂移）；期望 EWMA/CUSUM 比 Shewhart 更早爬升越界。禁止复用 imr_spi_shift。moving_average 不进本族。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–60"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "基线/小漂移"},
        ],
    },
    "g_chart_gap_days": {
        "wave": 1,
        "practice_only": False,
        "title": "客户投诉间隔天数（稀有事件 G 图）",
        "industry": "electronics",
        "story": "相邻重大投诉间隔（天）。前段间隔长，事件28起间隔变短。",
        "row_count": 36,
        "notes": "埋点：事件28（行28）起间隔天数由基线约10–14天缩短到约2–4天；期望 G 图后段点下移（间隔变短=事件更密）。",
        "columns": [
            {"index": 0, "name": "事件序号", "role_hint": "order", "description": "1–36"},
            {"index": 1, "name": "间隔天数", "role_hint": "measurement", "unit": "天", "description": "距上次投诉天数"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "基线/变密"},
        ],
    },
    "genvar_two_var": {
        "wave": 1,
        "practice_only": False,
        "title": "焊盘高宽联合波动（广义方差）",
        "industry": "electronics",
        "story": "每子组 n=5 测高度与宽度。子组18起联合协方差膨胀。",
        "row_count": 125,
        "notes": "埋点：子组18起（约行86–90 起）高度与宽度联合方差放大；期望 |S| 图后段抬高。子组大小 n=5 > p=2。行按子组连续堆叠。",
        "columns": [
            {"index": 0, "name": "子组", "role_hint": "subgroup", "description": "1–25，每组5行"},
            {"index": 1, "name": "高度_um", "role_hint": "measurement", "unit": "μm", "description": "变量1"},
            {"index": 2, "name": "宽度_um", "role_hint": "measurement", "unit": "μm", "description": "变量2"},
        ],
    },
    "t2_two_var_shift": {
        "wave": 1,
        "practice_only": False,
        "title": "双尺寸联合均值偏移（Hotelling T²）",
        "industry": "electronics",
        "story": "每片测长度与宽度。片36起长度均值台阶，宽度仍稳。",
        "row_count": 50,
        "notes": "埋点：片36（行36）起长度_mm 均值上移约0.15；期望 T² 后段抬高。宽度列几乎无台阶，用来对比一元图可能漏检联合偏移。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–50"},
            {"index": 1, "name": "长度_mm", "role_hint": "measurement", "unit": "mm", "description": "变量1"},
            {"index": 2, "name": "宽度_mm", "role_hint": "measurement", "unit": "mm", "description": "变量2"},
        ],
    },
    "imr_rs_subgroup_shift": {
        "wave": 1,
        "practice_only": False,
        "title": "子组均值台阶（I-MR-R/S）",
        "industry": "electronics",
        "story": "每批抽5件测厚度。子组16起批均值上移，组内极差仍稳。",
        "row_count": 125,
        "notes": "埋点：子组16（约行76起）批均值由约100抬到约102；期望 Xbar/I 侧后段上移，R/S 侧不明显乱。子组列必选。",
        "columns": [
            {"index": 0, "name": "子组", "role_hint": "subgroup", "description": "1–25，每组5行"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "基线/均值台阶"},
        ],
    },
    "laney_p_overdispersed": {
        "wave": 1,
        "practice_only": False,
        "title": "过离散批次不合格率（Laney P'）",
        "industry": "electronics",
        "story": "可变检验数下的不合格品。额外批间波动使普通 P 限过窄。",
        "row_count": 40,
        "notes": "埋点：批间额外波动（过离散）；约批22、31不合格率相对普通二项限会假性越界。期望 Laney Sigma Z>1、P' 限更宽。不要与普通 p_chart 共享。",
        "columns": [
            {"index": 0, "name": "批号", "role_hint": "order", "description": "1–40"},
            {"index": 1, "name": "不合格品数", "role_hint": "defectives", "description": "不良件数"},
            {"index": 2, "name": "检验数", "role_hint": "inspected", "description": "可变 n≈80–200"},
        ],
    },
    "laney_u_overdispersed": {
        "wave": 1,
        "practice_only": False,
        "title": "过离散缺陷率（Laney U'）",
        "industry": "electronics",
        "story": "可变单位数下的缺陷计数，批间额外波动。",
        "row_count": 40,
        "notes": "埋点：过离散缺陷率；约批20、29相对普通 u 限易假性报警。期望 Laney U' 限更宽。不要与普通 u_chart 共享。",
        "columns": [
            {"index": 0, "name": "批号", "role_hint": "order", "description": "1–40"},
            {"index": 1, "name": "缺陷数", "role_hint": "defects", "description": "缺陷计数"},
            {"index": 2, "name": "单位数", "role_hint": "units", "description": "可变面积/件数"},
        ],
    },
    "mewma_two_var_drift": {
        "wave": 1,
        "practice_only": False,
        "title": "双变量微小联合漂移（MEWMA）",
        "industry": "electronics",
        "story": "长度与宽度同时缓慢漂移。片28起各约 +0.4σ。",
        "row_count": 50,
        "notes": "埋点：片28（行28）起长度与宽度同时小幅上移；期望 MEWMA 统计量后段爬升。勿与一元 ewma 表混用。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–50"},
            {"index": 1, "name": "长度_mm", "role_hint": "measurement", "unit": "mm", "description": "变量1"},
            {"index": 2, "name": "宽度_mm", "role_hint": "measurement", "unit": "mm", "description": "变量2"},
        ],
    },
    "ma_small_drift": {
        "wave": 1,
        "practice_only": False,
        "title": "贴片厚度小漂移（移动平均专用）",
        "industry": "electronics",
        "story": "与 EWMA 同构信号但独立表：片30起小台阶。不进 spc_small_drift 族。",
        "row_count": 55,
        "notes": "埋点：片30（行30）起厚度由约50抬到约50.6；期望移动平均曲线后段上移。禁止挂到 spc_small_drift。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–55"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "基线/小漂移"},
        ],
    },
    "np_chart_const_n_step": {
        "wave": 1,
        "practice_only": False,
        "title": "恒定检验数不合格品（NP 图）",
        "industry": "electronics",
        "story": "每批固定检验100件。批21起不合格品数台阶。",
        "row_count": 35,
        "notes": "埋点：批21（行21）起不合格品数由约3抬到约9（n=100恒定）；期望 NP 图后段上移。禁止与可变 n 的 p_chart 共享。",
        "columns": [
            {"index": 0, "name": "批号", "role_hint": "order", "description": "1–35"},
            {"index": 1, "name": "不合格品数", "role_hint": "defectives", "description": "不良件数"},
            {"index": 2, "name": "检验数", "role_hint": "inspected", "description": "恒定100"},
        ],
    },
    "p_chart_variable_n_step": {
        "wave": 1,
        "practice_only": False,
        "title": "可变检验数不合格率台阶（P 图）",
        "industry": "electronics",
        "story": "检验数随批变化。批22起不合格率台阶；限宽随 n 变。",
        "row_count": 36,
        "notes": "埋点：批22（行22）起不合格率由约3%抬到约8%；检验数在50–180间变化，期望 p 限随 n 宽窄变化且后段比例上移。禁止与 np 共享。",
        "columns": [
            {"index": 0, "name": "批号", "role_hint": "order", "description": "1–36"},
            {"index": 1, "name": "不合格品数", "role_hint": "defectives", "description": "不良件数"},
            {"index": 2, "name": "检验数", "role_hint": "inspected", "description": "可变 n"},
        ],
    },
    "t_chart_time_interval": {
        "wave": 1,
        "practice_only": False,
        "title": "设备宕机间隔小时（T 图）",
        "industry": "electronics",
        "story": "相邻宕机间隔（小时）。事件25起间隔变短。",
        "row_count": 32,
        "notes": "埋点：事件25（行25）起间隔由基线约40–60小时缩短到约8–15小时；期望 T 图后段下移。",
        "columns": [
            {"index": 0, "name": "事件序号", "role_hint": "order", "description": "1–32"},
            {"index": 1, "name": "间隔小时", "role_hint": "measurement", "unit": "h", "description": "距上次宕机"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "基线/变密"},
        ],
    },
    "u_chart_variable_unit_step": {
        "wave": 1,
        "practice_only": False,
        "title": "可变单位缺陷率台阶（U 图）",
        "industry": "electronics",
        "story": "单位面积/件数变化。批20起单位缺陷率台阶。",
        "row_count": 35,
        "notes": "埋点：批20（行20）起缺陷率由约0.04抬到约0.12；单位数可变；期望 u 限随单位数变化且后段上移。禁止与 laney_u 共享。",
        "columns": [
            {"index": 0, "name": "批号", "role_hint": "order", "description": "1–35"},
            {"index": 1, "name": "缺陷数", "role_hint": "defects", "description": "缺陷计数"},
            {"index": 2, "name": "单位数", "role_hint": "units", "description": "可变单位"},
        ],
    },
    "xbar_r_n5_range_spike": {
        "wave": 1,
        "practice_only": False,
        "title": "子组 n=5 极差尖峰（Xbar-R）",
        "industry": "electronics",
        "story": "每批抽5件。子组12组内极差尖峰；子组20起均值台阶。",
        "row_count": 125,
        "notes": "埋点：子组12（约行56–60）组内极差尖峰，期望 R 图报警、先勿读该段 Xbar 限；子组20（约行96起）均值台阶，期望 Xbar 后段上移。禁止与 xbar_s 共享。",
        "columns": [
            {"index": 0, "name": "子组", "role_hint": "subgroup", "description": "1–25，每组5行"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "基线/极差尖峰/均值台阶"},
        ],
    },
    "xbar_s_n8_sd_shift": {
        "wave": 1,
        "practice_only": False,
        "title": "子组 n=8 标准差台阶（Xbar-S）",
        "industry": "electronics",
        "story": "每批抽8件。子组14起组内标准差放大。",
        "row_count": 160,
        "notes": "埋点：子组14（约行105起）组内σ放大；期望 S 图后段上移。子组大小=8。禁止与 xbar_r（n=5/R）共享。",
        "columns": [
            {"index": 0, "name": "子组", "role_hint": "subgroup", "description": "1–20，每组8行"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "基线/σ台阶"},
        ],
    },
    "z_mr_short_run": {
        "wave": 1,
        "practice_only": False,
        "title": "短跑多型号标准化（Z-MR）",
        "industry": "electronics",
        "story": "三种产品短跑混排。分组后标准化；型号B后段有台阶。",
        "row_count": 48,
        "notes": "埋点：行33–40 为型号B且均值相对该型号目标上移；期望 Z 图上对应点抬高。分组列=产品型号。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–48"},
            {"index": 1, "name": "尺寸_mm", "role_hint": "measurement", "unit": "mm", "description": "Y"},
            {"index": 2, "name": "产品型号", "role_hint": "group", "description": "A/B/C 短跑"},
        ],
    },
    "zone_chart_runs": {
        "wave": 1,
        "practice_only": False,
        "title": "同侧游程积分（区域图）",
        "industry": "electronics",
        "story": "单值序列。片24–33 连续落在中心线同侧 Zone C/B，积分配分。",
        "row_count": 50,
        "notes": "埋点：片24–33（行24–33）连续同侧偏高；期望区域图累计分抬高触发警戒。不是尖峰课。UCL≠USL。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–50"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "基线/同侧游程"},
        ],
    },
}

# Keys allowed in mapping.role_map must ⊆ id_metadata roles (often only required).
ROLE_MAP_BY_DATASET: dict[str, dict[str, str]] = {
    "c_chart_defect_step": {"defects": "焊点缺陷数"},
    "spc_small_drift": {"variables": "厚度_um"},
    "g_chart_gap_days": {"variables": "间隔天数"},
    "genvar_two_var": {"variables": "高度_um,宽度_um"},
    "t2_two_var_shift": {"variables": "长度_mm,宽度_mm"},
    "imr_rs_subgroup_shift": {"variables": "厚度_um"},
    "laney_p_overdispersed": {"defectives": "不合格品数"},
    "laney_u_overdispersed": {"defects": "缺陷数"},
    "mewma_two_var_drift": {"variables": "长度_mm,宽度_mm"},
    "ma_small_drift": {"variables": "厚度_um"},
    "np_chart_const_n_step": {"defectives": "不合格品数"},
    "p_chart_variable_n_step": {"defectives": "不合格品数"},
    "t_chart_time_interval": {"variables": "间隔小时"},
    "u_chart_variable_unit_step": {"defects": "缺陷数"},
    "xbar_r_n5_range_spike": {"variables": "厚度_um"},
    "xbar_s_n8_sd_shift": {"variables": "厚度_um"},
    "z_mr_short_run": {"variables": "尺寸_mm"},
    "zone_chart_runs": {"variables": "厚度_um"},
}

# ---------------------------------------------------------------------------
# Generators
# ---------------------------------------------------------------------------


def _poisson(rng: random.Random, lam: float) -> int:
    # Knuth
    L = math.exp(-lam)
    k = 0
    p = 1.0
    while p > L:
        k += 1
        p *= rng.random()
    return max(0, k - 1)


def gen_c_chart_defect_step(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(101)
    rows = []
    for i in range(1, 41):
        if i < 26:
            c = _poisson(rng, 3.0)
            note = "基线"
        else:
            c = _poisson(rng, 8.0)
            note = "缺陷台阶"
        rows.append([str(i), str(c), note])
    return rows


def gen_spc_small_drift(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(102)
    rows = []
    for i in range(1, 61):
        if i < 31:
            y = 100.0 + rng.gauss(0, 0.4)
            note = "基线"
        else:
            y = 100.8 + rng.gauss(0, 0.4)
            note = "小漂移"
        rows.append([str(i), f"{y:.2f}", note])
    return rows


def gen_g_chart_gap_days(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(103)
    rows = []
    for i in range(1, 37):
        if i < 28:
            gap = max(1, int(rng.expovariate(1 / 12)))
            note = "基线"
        else:
            gap = max(1, int(rng.expovariate(1 / 3)))
            note = "间隔变短"
        rows.append([str(i), str(gap), note])
    return rows


def gen_genvar_two_var(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(104)
    rows = []
    for sg in range(1, 26):
        scale = 2.5 if sg >= 18 else 1.0
        for _ in range(5):
            h = 120.0 + rng.gauss(0, 0.6 * scale)
            w = 80.0 + rng.gauss(0, 0.5 * scale)
            if scale > 1:
                # inflate covariance
                shock = rng.gauss(0, 0.8)
                h += shock
                w += shock * 0.7
            rows.append([str(sg), f"{h:.2f}", f"{w:.2f}"])
    return rows


def gen_t2_two_var_shift(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(105)
    rows = []
    for i in range(1, 51):
        length = 10.0 + rng.gauss(0, 0.05)
        width = 5.0 + rng.gauss(0, 0.04)
        if i >= 36:
            length += 0.15
        rows.append([str(i), f"{length:.3f}", f"{width:.3f}"])
    return rows


def gen_imr_rs_subgroup_shift(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(106)
    rows = []
    for sg in range(1, 26):
        mean = 102.0 if sg >= 16 else 100.0
        note = "均值台阶" if sg >= 16 else "基线"
        for _ in range(5):
            y = mean + rng.gauss(0, 0.7)
            rows.append([str(sg), f"{y:.2f}", note])
    return rows


def gen_laney_p_overdispersed(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(107)
    rows = []
    for i in range(1, 41):
        n = rng.randint(80, 200)
        # beta-binomial-ish: p fluctuates
        p = min(0.25, max(0.005, rng.betavariate(2, 40)))
        if i in (22, 31):
            p = min(0.35, p + 0.08)
        d = sum(1 for _ in range(n) if rng.random() < p)
        rows.append([str(i), str(d), str(n)])
    return rows


def gen_laney_u_overdispersed(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(108)
    rows = []
    for i in range(1, 41):
        units = rng.randint(40, 120)
        rate = max(0.01, rng.gammavariate(2.0, 0.03))
        if i in (20, 29):
            rate *= 2.2
        defects = _poisson(rng, rate * units)
        rows.append([str(i), str(defects), str(units)])
    return rows


def gen_mewma_two_var_drift(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(109)
    rows = []
    for i in range(1, 51):
        l = 10.0 + rng.gauss(0, 0.05)
        w = 5.0 + rng.gauss(0, 0.04)
        if i >= 28:
            l += 0.02
            w += 0.016
        rows.append([str(i), f"{l:.3f}", f"{w:.3f}"])
    return rows


def gen_ma_small_drift(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(110)
    rows = []
    for i in range(1, 56):
        if i < 30:
            y = 50.0 + rng.gauss(0, 0.3)
            note = "基线"
        else:
            y = 50.6 + rng.gauss(0, 0.3)
            note = "小漂移"
        rows.append([str(i), f"{y:.2f}", note])
    return rows


def gen_np_chart_const_n_step(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(111)
    rows = []
    for i in range(1, 36):
        n = 100
        p = 0.09 if i >= 21 else 0.03
        d = sum(1 for _ in range(n) if rng.random() < p)
        rows.append([str(i), str(d), str(n)])
    return rows


def gen_p_chart_variable_n_step(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(112)
    rows = []
    for i in range(1, 37):
        n = rng.choice([50, 80, 100, 120, 150, 180])
        p = 0.08 if i >= 22 else 0.03
        d = sum(1 for _ in range(n) if rng.random() < p)
        rows.append([str(i), str(d), str(n)])
    return rows


def gen_t_chart_time_interval(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(113)
    rows = []
    for i in range(1, 33):
        if i < 25:
            gap = max(1.0, rng.expovariate(1 / 48))
            note = "基线"
        else:
            gap = max(1.0, rng.expovariate(1 / 12))
            note = "间隔变短"
        rows.append([str(i), f"{gap:.1f}", note])
    return rows


def gen_u_chart_variable_unit_step(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(114)
    rows = []
    for i in range(1, 36):
        units = rng.randint(20, 80)
        rate = 0.12 if i >= 20 else 0.04
        defects = _poisson(rng, rate * units)
        rows.append([str(i), str(defects), str(units)])
    return rows


def gen_xbar_r_n5_range_spike(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(115)
    rows = []
    for sg in range(1, 26):
        mean = 102.0 if sg >= 20 else 100.0
        if sg == 12:
            note = "极差尖峰"
            vals = [100.0 + rng.gauss(0, 0.5) for _ in range(4)]
            vals.append(100.0 + 6.0)  # within-group spike
        elif sg >= 20:
            note = "均值台阶"
            vals = [mean + rng.gauss(0, 0.6) for _ in range(5)]
        else:
            note = "基线"
            vals = [mean + rng.gauss(0, 0.6) for _ in range(5)]
        for y in vals:
            rows.append([str(sg), f"{y:.2f}", note])
    return rows


def gen_xbar_s_n8_sd_shift(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(116)
    rows = []
    for sg in range(1, 21):
        sd = 1.8 if sg >= 14 else 0.6
        note = "σ台阶" if sg >= 14 else "基线"
        for _ in range(8):
            y = 100.0 + rng.gauss(0, sd)
            rows.append([str(sg), f"{y:.2f}", note])
    return rows


def gen_z_mr_short_run(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(117)
    # short runs: AAAA BBBB CCCC repeating-ish
    schedule = (
        ["A"] * 8 + ["B"] * 8 + ["C"] * 8 + ["A"] * 8 + ["B"] * 8 + ["C"] * 8
    )
    targets = {"A": 10.0, "B": 12.0, "C": 8.0}
    rows = []
    for i, g in enumerate(schedule, start=1):
        y = targets[g] + rng.gauss(0, 0.15)
        if g == "B" and i >= 33:
            y += 0.45
        rows.append([str(i), f"{y:.3f}", g])
    return rows


def gen_zone_chart_runs(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(118)
    rows = []
    for i in range(1, 51):
        if 24 <= i <= 33:
            # mild positive bias same side of CL
            y = 100.4 + abs(rng.gauss(0, 0.25))
            note = "同侧游程"
        else:
            y = 100.0 + rng.gauss(0, 0.45)
            note = "基线"
        rows.append([str(i), f"{y:.2f}", note])
    return rows


GENERATORS = {
    "c_chart_defect_step": gen_c_chart_defect_step,
    "spc_small_drift": gen_spc_small_drift,
    "g_chart_gap_days": gen_g_chart_gap_days,
    "genvar_two_var": gen_genvar_two_var,
    "t2_two_var_shift": gen_t2_two_var_shift,
    "imr_rs_subgroup_shift": gen_imr_rs_subgroup_shift,
    "laney_p_overdispersed": gen_laney_p_overdispersed,
    "laney_u_overdispersed": gen_laney_u_overdispersed,
    "mewma_two_var_drift": gen_mewma_two_var_drift,
    "ma_small_drift": gen_ma_small_drift,
    "np_chart_const_n_step": gen_np_chart_const_n_step,
    "p_chart_variable_n_step": gen_p_chart_variable_n_step,
    "t_chart_time_interval": gen_t_chart_time_interval,
    "u_chart_variable_unit_step": gen_u_chart_variable_unit_step,
    "xbar_r_n5_range_spike": gen_xbar_r_n5_range_spike,
    "xbar_s_n8_sd_shift": gen_xbar_s_n8_sd_shift,
    "z_mr_short_run": gen_z_mr_short_run,
    "zone_chart_runs": gen_zone_chart_runs,
}

# ---------------------------------------------------------------------------
# Overlay helpers
# ---------------------------------------------------------------------------

UCL_USL_GLOSSARY = [
    {
        "term": "UCL（上控制限）",
        "plain": "按过程自身波动算出的上警戒线，不是客户规格。",
        "remember": "UCL ≠ USL。点越 UCL = 特殊原因线索，不是自动判废。",
    },
    {
        "term": "USL（规格上限）",
        "plain": "客户或图纸规定的合格上限。",
        "remember": "控制限与规格限是两套尺子；本课不回答是否符合规格。",
    },
    {
        "term": "特殊原因",
        "plain": "可指认的异常扰动（换料、参数漂移、测错），相对普通原因日常抖动。",
        "remember": "控制图任务是揪特殊原因线索，不是证明过程合格或必须停线。",
    },
]


def _policy_detail(kind: str) -> list[dict]:
    return [
        {
            "field": "规则默认策略 (`rule_policy`)",
            "put": "全部适用规则",
            "meaning": f"空 tests 时按策略启用适用规则（kind≈{kind}）。规则越多越灵敏也越易误报。",
        },
        {
            "field": "特殊原因测试 (`tests`)",
            "put": "留空（用策略）",
            "meaning": "留空走 rule_policy；非空变成 explicit 列表。本课主信号仍对照埋点读图。",
        },
    ]


def _seven_plus(mission: str, prereq: list, self_explain: list, fade_ds: str, retrieval: list, misc: list) -> dict:
    return {
        "skill_mission": mission,
        "prereq_quiz": prereq,
        "self_explain": self_explain,
        "fade_levels": [
            {
                "level": 0,
                "student": "跟着 §4 填对话框、跟着 §5 对照埋点读图。",
                "scaffold": "参数表 + 读图话术 + 埋点行号全部给出。",
            },
            {
                "level": 1,
                "student": f"仍导入 `demo_{fade_ds}`；最后一步「埋点是否出现、UCL≠USL」自己写三句。",
                "scaffold": "对话框字段仍给出；不给读图标准答案。",
            },
            {
                "level": 2,
                "student": f"再导入 `demo_{fade_ds}`，自己改一个输入（如规则策略或历史限留空与否），不看埋点剧透写结论。",
                "scaffold": "仅术语表 + 误用禁止句。",
            },
        ],
        "retrieval_quiz": retrieval,
        "misconceptions": misc
        + [
            {
                "wrong": "点出 UCL = 产品超规格废品",
                "right": "UCL 是过程警戒线；超规格看 USL/LSL 或能力课。UCL ≠ USL。",
            },
            {
                "wrong": "看完例题不做练习也会了",
                "right": "必须做褪脚手架和检索小测。",
            },
        ],
    }


def _base_mistakes(extra: list[str] | None = None) -> list[str]:
    base = [
        "把 UCL/LCL 当成 USL/LSL，或把点出 UCL 写成超规格废品。",
        "写成过程合格、必须停线或已证明正态。",
        "用本教学失控/漂移集去算 Cpk 当合格证据。",
    ]
    return base + (extra or [])


def build_overlays() -> dict[str, dict]:
    """Return command_id -> overlay dict for all Wave-1 ids."""
    out: dict[str, dict] = {}

    # ---- c_chart ----
    out["c_chart"] = {
        "title": "C 图",
        "used_for": "固定检验单位下监视缺陷计数 c。本课只练「固定单位 + 缺陷台阶」读特殊原因线索。",
        "not_for": "单位大小变化应选 u 图；合格/不合格件数应选 p/np。不能把 UCL 当 USL。",
        "scenario": "回流焊每炉固定检 1 托盘。批26起焊点缺陷数抬高。只用 C 图看是否出现特殊原因线索。",
        "related_ids": ["u_chart", "np_chart", "imr"],
        "dialog_fill": {"defects": "焊点缺陷数"},
        "click_steps": [
            "打开「帮助」→「学习中心」，选择本教程「C 图」。",
            "导入测试数据，工作表 `demo_c_chart_defect_step`。",
            "菜单：控制图 → C 图。",
            "缺陷数=`焊点缺陷数`；每个子组单位数=1；阶段列输入留空；规则默认；tests 留空。",
            "对照埋点：批26起缺陷数抬高；口头区分 UCL ≠ USL。",
        ],
        "dialog_fill_detail": [
            {"field": "缺陷数 (`defects`)", "put": "焊点缺陷数", "meaning": "固定托盘内缺陷计数 Y。"},
            {"field": "每个子组单位数 (`units`)", "put": "1", "meaning": "本集固定单位；代码里 units 是 input 不是角色。"},
            {"field": "阶段列 (`stage`)", "put": "留空", "meaning": "本软件把 stage 放在 inputs；本课不分阶段，以免吃掉台阶。"},
            *_policy_detail("attribute"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "c（缺陷数）", "plain": "固定检验单位内的缺陷计数。", "remember": "单位一变就该改 u 图。"},
        ],
        "buried_signals": [
            {
                "row": 26,
                "what": "批26起缺陷数由基线约3抬到约8",
                "expect": "C 图后段上移或越 UCL。不要写成过程合格或必须停线。",
            }
        ],
        "output_guide": [
            {"name": "C 图", "meaning": "指着 CL/UCL：批26后上移。UCL ≠ USL。"},
            {"name": "逐点表", "meaning": "核对触发行号与备注列「缺陷台阶」。"},
        ],
        "common_mistakes": _base_mistakes(["批量大小时变仍用 c 图", "把不合格品数当缺陷数"]),
        **_seven_plus(
            "用 C 图在固定单位缺陷计数上找出批26台阶，并区分 UCL 与 USL。",
            [
                {"q": "C 图适合什么数据？", "good": "固定检验单位的缺陷计数", "bad": "检验单位大小随批变化"},
                {"q": "UCL 是规格上限吗？", "good": "否，UCL 是过程警戒线", "bad": "是，点出 UCL 就是废品"},
                {"q": "单位大小变化应改用？", "good": "U 图", "bad": "继续用 C 图硬画"},
            ],
            [
                {"after": "导入", "prompt": "为什么本课单位数固定为1？"},
                {"after": "读批26", "prompt": "点越 UCL 能写成超规格吗？"},
            ],
            "c_chart_defect_step",
            ["UCL 与 USL 差一句？", "批26期望看见什么？", "何时改用 u 图？"],
            [{"wrong": "缺陷数=不合格品数", "right": "一件可有多缺陷；不合格品是件计数，应 p/np。"}],
        ),
    }

    # ---- ewma / cusum share spc_small_drift ----
    out["ewma"] = {
        "title": "EWMA 控制图",
        "used_for": "对小幅、持续均值漂移比 Shewhart 更敏感。本课埋微小台阶，不埋尖峰。",
        "not_for": "大幅瞬时偏移优先 Shewhart；属性计数不用 EWMA。禁止复用 imr_spi_shift。",
        "scenario": "贴片厚度投诉「慢慢偏厚」。片31起约 +0.8μm 小台阶。用 EWMA 看是否比 I 图更早爬升。",
        "related_ids": ["cusum", "imr", "moving_average"],
        "dialog_fill": {"variables": "厚度_um"},
        "click_steps": [
            "学习中心导入 `demo_spc_small_drift`。",
            "菜单：控制图 → EWMA 控制图。",
            "测量值=`厚度_um`；Lambda=0.2；控制限倍数=3；历史均值/Sigma 留空；规则默认；tests 留空。",
            "对照片31起：EWMA 曲线应缓慢爬升；口头 UCL ≠ USL。",
        ],
        "dialog_fill_detail": [
            {"field": "测量值 (`variables`)", "put": "厚度_um", "meaning": "单值 Y。"},
            {"field": "Lambda (`lambda`)", "put": "0.2", "meaning": "近期权重；过大则接近普通均值图。"},
            {"field": "控制限倍数 (`limit`)", "put": "3", "meaning": "EWMA 限宽度倍数。"},
            {"field": "历史均值 (`historical_mean`)", "put": "留空", "meaning": "用本集估中心，避免盖住小漂移。"},
            {"field": "历史 Sigma (`historical_sigma`)", "put": "留空", "meaning": "用本集估 σ。"},
            *_policy_detail("ewma"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "EWMA", "plain": "指数加权移动平均：越近的点权重越大。", "remember": "擅长小而持续的漂移。"},
            {"term": "Lambda (λ)", "plain": "平滑系数，常用 0.2–0.4。", "remember": "λ 大→更像 Shewhart；λ 小→更平滑更慢。"},
        ],
        "buried_signals": [
            {
                "row": 31,
                "what": "片31起均值约 +0.8μm 小台阶",
                "expect": "EWMA 后段爬升并可能越 UCL；不要写成过程合格。",
            }
        ],
        "output_guide": [
            {"name": "EWMA 图", "meaning": "片31后曲线上移。UCL ≠ USL。"},
            {"name": "参数表", "meaning": "核对 λ 与限倍数是否为教学默认。"},
        ],
        "common_mistakes": _base_mistakes(["λ 过大失去小偏移灵敏度", "单独解读不与 Shewhart 对照"]),
        **_seven_plus(
            "用 EWMA 检出片31小漂移，并说明为何不用 I-MR 金标失控表。",
            [
                {"q": "EWMA 相对 I 图更擅长？", "good": "小幅持续漂移", "bad": "单点大尖峰"},
                {"q": "本课能否复用 imr_spi_shift？", "good": "不能，那是阶跃+尖峰课", "bad": "可以，反正都是单值"},
                {"q": "UCL=USL吗？", "good": "否", "bad": "是"},
            ],
            [
                {"after": "选 λ=0.2", "prompt": "若 λ=1，图会更像什么？"},
                {"after": "读片31", "prompt": "为什么历史均值要留空？"},
            ],
            "spc_small_drift",
            ["片31埋了什么？", "UCL≠USL 一句", "为何不与 moving_average 共享本表？"],
            [{"wrong": "EWMA 可证明过程正态", "right": "控制图不证明分布；禁止「已证明正态」。"}],
        ),
    }

    out["cusum"] = {
        "title": "CUSUM 控制图",
        "used_for": "累积偏差以更快检出小幅持续偏移。与 EWMA 同构共享 `spc_small_drift`。",
        "not_for": "不能当假设检验 p 值；大尖峰仍看 Shewhart。禁止挂 imr_spi_shift。",
        "scenario": "同一厚度小漂移集。用 CUSUM（目标≈100，σ≈0.4）看累积和是否在片31后偏离。",
        "related_ids": ["ewma", "imr"],
        "dialog_fill": {"variables": "厚度_um"},
        "click_steps": [
            "导入 `demo_spc_small_drift`。",
            "菜单：控制图 → CUSUM 控制图。",
            "测量值=`厚度_um`；目标值=100；过程 Sigma=0.4；k=0.5；h=4；规则默认；tests 留空。",
            "对照片31：CUSUM 应累积偏离；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "测量值 (`variables`)", "put": "厚度_um", "meaning": "单值 Y。"},
            {"field": "目标值 (`target`)", "put": "100", "meaning": "CUSUM 相对目标累积；对本集基线均值。"},
            {"field": "过程 Sigma (`sigma`)", "put": "0.4", "meaning": "标准化尺度；勿填成规格公差。"},
            {"field": "参考值 k (`k`)", "put": "0.5", "meaning": "允许松弛（以 σ 计）。"},
            {"field": "决策间隔 h (`h`)", "put": "4", "meaning": "越界阈值。"},
            *_policy_detail("cusum"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "CUSUM", "plain": "累积和：把相对目标的偏差累加起来。", "remember": "小偏移会慢慢堆高。"},
            {"term": "k / h", "plain": "参考值与决策间隔，决定灵敏度。", "remember": "本课用默认 0.5 / 4 作教学起点。"},
        ],
        "buried_signals": [
            {
                "row": 31,
                "what": "片31起小台阶",
                "expect": "CUSUM 统计量后段偏离决策间隔；勿写必须停线。",
            }
        ],
        "output_guide": [
            {"name": "CUSUM 图", "meaning": "片31后累积偏离。UCL/决策限 ≠ USL。"},
        ],
        "common_mistakes": _base_mistakes(["把 sigma 填成规格半宽", "把 CUSUM 当 p 值检验"]),
        **_seven_plus(
            "用 CUSUM 在同构小漂移集上检出片31偏移，并区分决策限与规格限。",
            [
                {"q": "CUSUM 与 EWMA 本课能否共享表？", "good": "可以，白名单 spc_small_drift", "bad": "必须各一张宽表"},
                {"q": "目标值应填？", "good": "过程目标/基线均值约100", "bad": "客户 USL"},
                {"q": "UCL≠USL？", "good": "是，必须区分", "bad": "控制限就是规格"},
            ],
            [
                {"after": "填 sigma=0.4", "prompt": "若误填成规格公差会发生什么？"},
                {"after": "读图", "prompt": "累积偏离等于必须停线吗？"},
            ],
            "spc_small_drift",
            ["本族服务哪些命令？", "片31期望？", "UCL≠USL"],
            [{"wrong": "CUSUM 越界=已证明均值差显著", "right": "是监视线索，不替代设计好的假设检验。"}],
        ),
    }

    # ---- remaining commands: continue in write_overlays via second builder ----
    out.update(_build_overlays_part2())
    return out


def _build_overlays_part2() -> dict[str, dict]:
    out: dict[str, dict] = {}

    out["g_chart"] = {
        "title": "G 图",
        "used_for": "稀有事件的间隔（天数等几何型）。间隔变短=事件变密。",
        "not_for": "高频计量单值应 I-MR；缺陷计数应 c/u。",
        "scenario": "重大客户投诉间隔。事件28起间隔变短。",
        "related_ids": ["t_chart", "c_chart"],
        "dialog_fill": {"variables": "间隔天数"},
        "click_steps": [
            "导入 `demo_g_chart_gap_days`。",
            "菜单：控制图 → G 图。",
            "间隔列=`间隔天数`；规则默认；tests 留空。",
            "对照事件28：间隔下移；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "间隔列 (`variables`)", "put": "间隔天数", "meaning": "相邻稀有事件间隔。"},
            *_policy_detail("g"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [{"term": "G 图", "plain": "基于几何分布的间隔图。", "remember": "点低=间隔短=事件更密。"}],
        "buried_signals": [
            {"row": 28, "what": "事件28起间隔缩短", "expect": "G 图后段下移；勿写过程合格。"}
        ],
        "output_guide": [{"name": "G 图", "meaning": "事件28后点更低。UCL≠USL。"}],
        "common_mistakes": _base_mistakes(["把间隔当缺陷计数画 c 图"]),
        **_seven_plus(
            "用 G 图识别事件28起间隔变短。",
            [
                {"q": "G 图 Y 是？", "good": "事件间隔", "bad": "每批缺陷数"},
                {"q": "间隔变短意味着？", "good": "稀有事件更频繁", "bad": "过程一定合格"},
                {"q": "UCL=USL？", "good": "否", "bad": "是"},
            ],
            [{"after": "读图", "prompt": "点下移为何可能是坏消息？"}],
            "g_chart_gap_days",
            ["行28期望？", "G 与 T 图差别一句", "UCL≠USL"],
            [{"wrong": "间隔变短=质量变好", "right": "对投诉/宕机而言变短通常更糟。"}],
        ),
    }

    out["t_chart"] = {
        "title": "T 图",
        "used_for": "稀有事件连续时间间隔（小时等）。",
        "not_for": "不要与计量 I-MR 混用场景。",
        "scenario": "设备宕机间隔小时。事件25起变短。",
        "related_ids": ["g_chart"],
        "dialog_fill": {"variables": "间隔小时"},
        "click_steps": [
            "导入 `demo_t_chart_time_interval`。",
            "菜单：控制图 → T 图。",
            "间隔列=`间隔小时`；规则默认；tests 留空。",
            "对照事件25；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "间隔列 (`variables`)", "put": "间隔小时", "meaning": "连续时间间隔。"},
            *_policy_detail("t"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [{"term": "T 图", "plain": "稀有事件时间间隔控制图。", "remember": "与 G 图同类，尺度常为连续时间。"}],
        "buried_signals": [
            {"row": 25, "what": "事件25起间隔缩短", "expect": "T 图后段下移。"}
        ],
        "output_guide": [{"name": "T 图", "meaning": "事件25后间隔变短。UCL≠USL。"}],
        "common_mistakes": _base_mistakes(),
        **_seven_plus(
            "用 T 图识别宕机间隔在事件25后变短。",
            [
                {"q": "T 图监视什么？", "good": "事件间隔时间", "bad": "子组均值"},
                {"q": "UCL≠USL？", "good": "必须区分", "bad": "相同"},
                {"q": "本课埋点行？", "good": "25", "bad": "41"},
            ],
            [{"after": "导入", "prompt": "为何不用 I-MR 画间隔？"}],
            "t_chart_time_interval",
            ["行25期望", "与 G 图关系", "UCL≠USL"],
            [{"wrong": "必须停线因为一点越界", "right": "越界是调查线索，教程禁止「必须停线」。"}],
        ),
    }

    out["generalized_variance"] = {
        "title": "广义方差图",
        "used_for": "监视多变量子组协方差行列式 |S|。",
        "not_for": "不能替代单变量 R/S；n 必须 > p。",
        "scenario": "焊盘高度+宽度每子组5点。子组18起联合波动放大。",
        "related_ids": ["hotelling_t2", "xbar_s"],
        "dialog_fill": {"variables": "高度_um,宽度_um"},
        "click_steps": [
            "导入 `demo_genvar_two_var`。",
            "菜单：控制图 → 广义方差图。",
            "变量多选 `高度_um` 与 `宽度_um`；子组大小 n=5。",
            "对照子组18起 |S| 抬高；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "变量（多列）(`variables`)", "put": "高度_um,宽度_um", "meaning": "至少两列数值；按子组连续堆叠。"},
            {"field": "子组大小 n (`subgroup_size`)", "put": "5", "meaning": "必须 n>p；本课 p=2。"},
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "|S| 广义方差", "plain": "样本协方差矩阵行列式，概括联合波动体积。", "remember": "|S| 大=联合更散。"},
            {"term": "n > p", "plain": "子组大小必须大于变量个数。", "remember": "否则协方差不可估。"},
        ],
        "buried_signals": [
            {"row": 86, "what": "子组18起（约行86）联合方差放大", "expect": "|S| 图后段抬高。"}
        ],
        "output_guide": [{"name": "|S| 图", "meaning": "子组18后上移。UCL≠USL。"}],
        "common_mistakes": _base_mistakes(["n≤p 仍硬跑", "把 |S| 当成合格判定"]),
        **_seven_plus(
            "用广义方差图检出子组18起联合波动放大。",
            [
                {"q": "n 与 p 关系？", "good": "n 必须大于 p", "bad": "n 可以等于 p"},
                {"q": "本课几列变量？", "good": "2", "bad": "1"},
                {"q": "UCL=USL？", "good": "否", "bad": "是"},
            ],
            [{"after": "设 n=5", "prompt": "若误设 n=2 会怎样？"}],
            "genvar_two_var",
            ["埋点子组？", "n>p 为何？", "UCL≠USL"],
            [{"wrong": "|S| 低=产品合格", "right": "只说明联合波动相对历史，不回答规格。"}],
        ),
    }

    out["hotelling_t2"] = {
        "title": "Hotelling T²",
        "used_for": "多变量联合均值偏移监视。",
        "not_for": "不能替代各变量单图诊断根因；本课至少两列。",
        "scenario": "长度+宽度。片36起长度均值台阶。",
        "related_ids": ["mewma", "generalized_variance"],
        "dialog_fill": {"variables": "长度_mm,宽度_mm"},
        "click_steps": [
            "导入 `demo_t2_two_var_shift`。",
            "菜单：控制图 → Hotelling T²。",
            "变量多选两列；控制阶段=Phase I；α=0.05。",
            "对照片36；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "变量（多列）(`variables`)", "put": "长度_mm,宽度_mm", "meaning": "至少两列。"},
            {"field": "控制阶段 (`phase`)", "put": "Phase I", "meaning": "目录值 phase1；本课用 Phase I 估限。"},
            {"field": "α (`alpha`)", "put": "0.05", "meaning": "显著性/限对应水平。"},
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "Hotelling T²", "plain": "多变量到中心的马氏距离型统计量。", "remember": "联合偏移可能单图不明显。"},
            {"term": "Phase I / II", "plain": "I=用当前数据建限；II=用历史限监视。", "remember": "本课 Phase I。"},
        ],
        "buried_signals": [
            {"row": 36, "what": "片36起长度均值上移", "expect": "T² 后段抬高。"}
        ],
        "output_guide": [{"name": "T² 图", "meaning": "片36后上移。UCL≠USL。"}],
        "common_mistakes": _base_mistakes(["只选一列", "把 T² 当因果证明"]),
        **_seven_plus(
            "用 T² 检出片36联合均值偏移。",
            [
                {"q": "至少几列？", "good": "2", "bad": "1"},
                {"q": "Phase I 做什么？", "good": "用当前数据估限", "bad": "只用历史限"},
                {"q": "UCL≠USL？", "good": "是", "bad": "否"},
            ],
            [{"after": "读片36", "prompt": "为何宽度几乎无台阶仍可能 T² 报警？"}],
            "t2_two_var_shift",
            ["行36", "Phase I 含义", "UCL≠USL"],
            [{"wrong": "T² 越界必须停线", "right": "禁止「必须停线」；应调查贡献变量。"}],
        ),
    }

    out["mewma"] = {
        "title": "MEWMA",
        "used_for": "多变量小漂移的指数加权监视。",
        "not_for": "不要与一元 ewma 表混用；至少两列。",
        "scenario": "长度+宽度同时小漂移，片28起。",
        "related_ids": ["ewma", "hotelling_t2"],
        "dialog_fill": {"variables": "长度_mm,宽度_mm"},
        "click_steps": [
            "导入 `demo_mewma_two_var_drift`。",
            "菜单：控制图 → MEWMA。",
            "两列变量；Lambda=0.1；UCL 可选留空。",
            "对照片28；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "变量（多列）(`variables`)", "put": "长度_mm,宽度_mm", "meaning": "至少两列。"},
            {"field": "Lambda (`lambda`)", "put": "0.1", "meaning": "多元平滑系数，默认更小。"},
            {"field": "UCL（可选）(`ucl`)", "put": "留空", "meaning": "本课用软件默认计算限。"},
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "MEWMA", "plain": "多元 EWMA。", "remember": "抓多变量同时小漂移。"},
        ],
        "buried_signals": [
            {"row": 28, "what": "片28起双变量小漂移", "expect": "MEWMA 后段爬升。"}
        ],
        "output_guide": [{"name": "MEWMA", "meaning": "片28后抬高。UCL≠USL。"}],
        "common_mistakes": _base_mistakes(["与一元 ewma 共用错误表"]),
        **_seven_plus(
            "用 MEWMA 检出片28双变量小漂移。",
            [
                {"q": "MEWMA 至少几列？", "good": "2", "bad": "1"},
                {"q": "与 ewma 共享表？", "good": "不，本课专用 mewma_two_var_drift", "bad": "共享 spc_small_drift"},
                {"q": "UCL≠USL？", "good": "是", "bad": "否"},
            ],
            [{"after": "设 λ=0.1", "prompt": "为何多元默认 λ 比一元更小？"}],
            "mewma_two_var_drift",
            ["行28", "为何不进 spc_small_drift 族", "UCL≠USL"],
            [{"wrong": "MEWMA 已证明正态", "right": "禁止「已证明正态」。"}],
        ),
    }

    out["imr_rs"] = {
        "title": "I-MR-R/S 控制图",
        "used_for": "同时看个体/移动极差与子组 R 或 S 的组合诊断。",
        "not_for": "不要当成单纯 I-MR 金标课；需要子组结构。",
        "scenario": "每批5件。子组16起批均值上移。",
        "related_ids": ["imr", "xbar_r"],
        "dialog_fill": {"variables": "厚度_um", "subgroup": "子组"},
        "click_steps": [
            "导入 `demo_imr_rs_subgroup_shift`。",
            "菜单：控制图 → I-MR-R/S 控制图。",
            "变量=`厚度_um`；子组列=`子组`；子组大小=5；阶段列留空；规则默认；tests 留空。",
            "对照子组16；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "变量 (`variables`)", "put": "厚度_um", "meaning": "测量 Y。"},
            {"field": "子组列 (`subgroup`)", "put": "子组", "meaning": "合理子组编号。"},
            {"field": "阶段列 (`stage`)", "put": "留空", "meaning": "本课同一阶段看均值台阶。"},
            {"field": "子组大小 (`subgroup_size`)", "put": "5", "meaning": "与数据每组5行一致。"},
            *_policy_detail("individuals"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "合理子组", "plain": "组内尽量只含普通原因波动。", "remember": "子组乱则限无意义。"},
            {"term": "I-MR-R/S", "plain": "个体图与子组离散图组合。", "remember": "先看离散侧是否乱。"},
        ],
        "buried_signals": [
            {"row": 76, "what": "子组16起（约行76）均值台阶", "expect": "位置侧后段上移；R/S 侧相对稳定。"}
        ],
        "output_guide": [
            {"name": "组合图", "meaning": "子组16后位置上移。UCL≠USL。"},
        ],
        "common_mistakes": _base_mistakes(["不选子组列", "与金标 imr_spi_shift 混用"]),
        **_seven_plus(
            "用 I-MR-R/S 在子组结构下检出子组16均值台阶。",
            [
                {"q": "子组列本课填？", "good": "子组", "bad": "留空也可随便"},
                {"q": "能否用 imr_spi_shift？", "good": "不能", "bad": "能"},
                {"q": "UCL≠USL？", "good": "是", "bad": "否"},
            ],
            [{"after": "选子组列", "prompt": "若不选子组列会损失什么信息？"}],
            "imr_rs_subgroup_shift",
            ["子组16期望", "与纯 I-MR 差别", "UCL≠USL"],
            [{"wrong": "R 图安静就可以宣称合格", "right": "离散稳定≠符合规格。"}],
        ),
    }

    out["moving_average"] = {
        "title": "移动平均控制图",
        "used_for": "固定窗宽平滑后监视小漂移。独立表 `ma_small_drift`。",
        "not_for": "禁止挂入 spc_small_drift 族；尖峰课用 Shewhart。",
        "scenario": "厚度片30起小台阶。窗宽 w=3。",
        "related_ids": ["ewma", "imr"],
        "dialog_fill": {"variables": "厚度_um"},
        "click_steps": [
            "导入 `demo_ma_small_drift`。",
            "菜单：控制图 → 移动平均控制图。",
            "测量值=`厚度_um`；窗宽=3；MR长度=2；限倍数=3；历史限留空；规则默认；tests 留空。",
            "对照片30；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "测量值 (`variables`)", "put": "厚度_um", "meaning": "单值 Y。"},
            {"field": "窗宽 w (`ma_window`)", "put": "3", "meaning": "移动平均长度。"},
            {"field": "移动极差长度 (`mr_length`)", "put": "2", "meaning": "估短期 σ。"},
            {"field": "控制限倍数 (`limit`)", "put": "3", "meaning": "限宽。"},
            {"field": "历史均值 (`historical_center`)", "put": "留空", "meaning": "避免盖住漂移。"},
            {"field": "历史 Sigma (`historical_sigma`)", "put": "留空", "meaning": "用本集估。"},
            *_policy_detail("moving_average"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "移动平均窗宽", "plain": "最近 w 点的平均。", "remember": "w 大更平滑更迟钝。"},
        ],
        "buried_signals": [
            {"row": 30, "what": "片30起小台阶", "expect": "MA 曲线后段上移。"}
        ],
        "output_guide": [{"name": "MA 图", "meaning": "片30后上移。UCL≠USL。"}],
        "common_mistakes": _base_mistakes(["把本表并进 spc_small_drift"]),
        **_seven_plus(
            "用移动平均检出片30小漂移，并说明为何不进 ewma/cusum 同构族。",
            [
                {"q": "本课 dataset？", "good": "ma_small_drift", "bad": "spc_small_drift"},
                {"q": "窗宽默认？", "good": "3", "bad": "30"},
                {"q": "UCL≠USL？", "good": "是", "bad": "否"},
            ],
            [{"after": "导入", "prompt": "计划为何禁止 MA 进 spc_small_drift？"}],
            "ma_small_drift",
            ["行30", "与 EWMA 差别一句", "UCL≠USL"],
            [{"wrong": "平滑后就能证明正态", "right": "禁止「已证明正态」。"}],
        ),
    }

    out.update(_build_overlays_part3())
    return out


def _build_overlays_part3() -> dict[str, dict]:
    out: dict[str, dict] = {}

    out["p_chart"] = {
        "title": "P 图",
        "used_for": "可变检验数下的不合格品率。限宽随 n 变化。",
        "not_for": "恒定 n 可考虑 np；缺陷计数用 c/u。禁止与 np 共享表。",
        "scenario": "检验数50–180变化。批22起不合格率台阶。",
        "related_ids": ["np_chart", "laney_p_chart"],
        "dialog_fill": {"defectives": "不合格品数", "inspected": "检验数"},
        "click_steps": [
            "导入 `demo_p_chart_variable_n_step`。",
            "菜单：控制图 → P 图。",
            "不合格品数=`不合格品数`；检验数（列）=`检验数`；检验数常数留空；阶段列留空；规则默认；tests 留空。",
            "对照批22与限宽随 n 变化；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "不合格品数 (`defectives`)", "put": "不合格品数", "meaning": "不良件数。"},
            {"field": "检验数（列）(`inspected`)", "put": "检验数", "meaning": "可变 n；本课必选列。"},
            {"field": "阶段列 (`stage`)", "put": "留空", "meaning": "同一阶段看比例台阶。"},
            {"field": "检验数（常数）(`inspected_constant`)", "put": "留空", "meaning": "已有检验数列就不要填常数。"},
            *_policy_detail("attribute"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "p（不合格品率）", "plain": "不良件数/检验数。", "remember": "n 小则限更宽。"},
            {"term": "可变 n", "plain": "各批检验数不同。", "remember": "P 图限会一窄一宽。"},
        ],
        "buried_signals": [
            {"row": 22, "what": "批22起不合格率台阶", "expect": "p 后段上移；限宽随检验数变化。"}
        ],
        "output_guide": [{"name": "P 图", "meaning": "批22后比例上移。UCL≠USL。"}],
        "common_mistakes": _base_mistakes(["与 np 共用一张表", "把缺陷计数当不合格品"]),
        **_seven_plus(
            "用 P 图在可变 n 下检出批22比例台阶。",
            [
                {"q": "为何不与 np 共享？", "good": "np 要近似恒定 n", "bad": "完全一样"},
                {"q": "检验数列本课？", "good": "必选检验数", "bad": "只填常数"},
                {"q": "UCL≠USL？", "good": "是", "bad": "否"},
            ],
            [{"after": "读限宽", "prompt": "n 变大时期望限怎样变？"}],
            "p_chart_variable_n_step",
            ["行22", "可变 n 教学点", "UCL≠USL"],
            [{"wrong": "p 越界=批次必须报废", "right": "是特殊原因线索，禁止「必须停线/报废」话术。"}],
        ),
    }

    out["np_chart"] = {
        "title": "NP 图",
        "used_for": "近似恒定检验数下的不合格品数。",
        "not_for": "可变 n 应 p 图。禁止与 p 共享。",
        "scenario": "每批固定100件。批21起不合格品数台阶。",
        "related_ids": ["p_chart"],
        "dialog_fill": {"defectives": "不合格品数", "inspected": "检验数"},
        "click_steps": [
            "导入 `demo_np_chart_const_n_step`。",
            "菜单：控制图 → NP 图。",
            "不合格品数=`不合格品数`；检验数列=`检验数`（或常数100）；阶段留空；规则默认；tests 留空。",
            "对照批21；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "不合格品数 (`defectives`)", "put": "不合格品数", "meaning": "不良件数。"},
            {"field": "检验数列 (`inspected`)", "put": "检验数", "meaning": "本集恒定100。"},
            {"field": "阶段列 (`stage`)", "put": "留空", "meaning": "不分阶段。"},
            {"field": "检验数常数 (`inspected_constant`)", "put": "可留空或100", "meaning": "与列二选一；本课用列即可。"},
            *_policy_detail("attribute"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "np", "plain": "不合格品数（n 近似固定）。", "remember": "n 变来变去就改 p 图。"},
        ],
        "buried_signals": [
            {"row": 21, "what": "批21起不合格品数台阶", "expect": "NP 图后段上移。"}
        ],
        "output_guide": [{"name": "NP 图", "meaning": "批21后上移。UCL≠USL。"}],
        "common_mistakes": _base_mistakes(["可变 n 仍用 np"]),
        **_seven_plus(
            "用 NP 图在恒定 n 下检出批21台阶。",
            [
                {"q": "本课 n？", "good": "恒定100", "bad": "随意变化"},
                {"q": "与 p 共享表？", "good": "禁止", "bad": "可以"},
                {"q": "UCL≠USL？", "good": "是", "bad": "否"},
            ],
            [{"after": "导入", "prompt": "为何计划拒绝 p∪np 共享？"}],
            "np_chart_const_n_step",
            ["行21", "恒定 n 为何重要", "UCL≠USL"],
            [{"wrong": "np 越界证明供应商作弊", "right": "只是统计线索，需工程调查。"}],
        ),
    }

    out["u_chart"] = {
        "title": "U 图",
        "used_for": "可变单位数下的单位缺陷率。",
        "not_for": "固定单位可用 c；不合格品用 p/np。禁止与 laney_u 共享。",
        "scenario": "单位数变化。批20起缺陷率台阶。",
        "related_ids": ["c_chart", "laney_u_chart"],
        "dialog_fill": {"defects": "缺陷数", "units": "单位数"},
        "click_steps": [
            "导入 `demo_u_chart_variable_unit_step`。",
            "菜单：控制图 → U 图。",
            "缺陷数=`缺陷数`；单位数列=`单位数`；阶段留空；规则默认；tests 留空。",
            "对照批20；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "缺陷数 (`defects`)", "put": "缺陷数", "meaning": "缺陷计数。"},
            {"field": "单位数列 (`units`)", "put": "单位数", "meaning": "可变单位；本课必选。"},
            {"field": "阶段列 (`stage`)", "put": "留空", "meaning": "同一阶段看率台阶。"},
            *_policy_detail("attribute"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "u（单位缺陷率）", "plain": "缺陷数/单位数。", "remember": "单位变→限宽变。"},
        ],
        "buried_signals": [
            {"row": 20, "what": "批20起缺陷率台阶", "expect": "u 后段上移；限随单位数变化。"}
        ],
        "output_guide": [{"name": "U 图", "meaning": "批20后上移。UCL≠USL。"}],
        "common_mistakes": _base_mistakes(["与 laney_u 共享", "漏选单位列"]),
        **_seven_plus(
            "用 U 图在可变单位下检出批20率台阶。",
            [
                {"q": "单位列能否省略？", "good": "不能，本软件必选", "bad": "能"},
                {"q": "与 c 图差别？", "good": "单位可变用 u", "bad": "完全一样"},
                {"q": "UCL≠USL？", "good": "是", "bad": "否"},
            ],
            [{"after": "选单位列", "prompt": "若单位全相同，为何仍可能选 u？"}],
            "u_chart_variable_unit_step",
            ["行20", "为何不与 laney_u 共享", "UCL≠USL"],
            [{"wrong": "u 越界=客户规格超限", "right": "UCL≠USL。"}],
        ),
    }

    out["laney_p_chart"] = {
        "title": "Laney P' 图",
        "used_for": "过离散时加宽 P 限，减少假报警。",
        "not_for": "普通二项波动用 p 即可。禁止与 p_chart 共享。",
        "scenario": "批间额外波动。对照普通 P 可能假性越界。",
        "related_ids": ["p_chart"],
        "dialog_fill": {"defectives": "不合格品数", "inspected": "检验数"},
        "click_steps": [
            "导入 `demo_laney_p_overdispersed`。",
            "菜单：控制图 → Laney P' 图。",
            "不合格品数+检验数列；常数与历史限留空；规则默认；tests 留空。",
            "关注 Sigma Z>1 与更宽的限；对照批22/31；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "不合格品数 (`defectives`)", "put": "不合格品数", "meaning": "不良件数。"},
            {"field": "检验数列 (`inspected`)", "put": "检验数", "meaning": "可变 n。"},
            {"field": "阶段列 (`stage`)", "put": "留空", "meaning": "本课看过离散。"},
            {"field": "检验数常数 (`inspected_constant`)", "put": "留空", "meaning": "已有列。"},
            {"field": "历史中心线 (`historical_center`)", "put": "留空", "meaning": "用本集。"},
            {"field": "历史 Sigma Z (`historical_sigma_z`)", "put": "留空", "meaning": "让软件估过离散因子。"},
            *_policy_detail("laney"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "过离散", "plain": "实际波动大于纯二项假设。", "remember": "普通 P 限过窄→假报警。"},
            {"term": "Sigma Z", "plain": "Laney 过离散校正因子。", "remember": ">1 表示需加宽。"},
        ],
        "buried_signals": [
            {"row": 22, "what": "过离散下批22相对高比例", "expect": "对照普通 P 更易假警；Laney 限更宽。"},
            {"row": 31, "what": "批31另一高比例点", "expect": "同样用于体会过离散。"},
        ],
        "output_guide": [
            {"name": "Laney P'", "meaning": "看 Sigma Z 与加宽限。UCL≠USL。"},
        ],
        "common_mistakes": _base_mistakes(["与普通 p 共用表", "把 Sigma Z 当 Cpk"]),
        **_seven_plus(
            "用 Laney P' 理解过离散与加宽限。",
            [
                {"q": "为何不用普通 p 表？", "good": "Laney 要过离散信号", "bad": "随便共用"},
                {"q": "Sigma Z>1 意味？", "good": "过离散，限应更宽", "bad": "过程合格"},
                {"q": "UCL≠USL？", "good": "是", "bad": "否"},
            ],
            [{"after": "看 Sigma Z", "prompt": "若强行用普通 P 限会怎样？"}],
            "laney_p_overdispersed",
            ["批22/31", "过离散一句", "UCL≠USL"],
            [{"wrong": "Laney 证明已消除特殊原因", "right": "只校正限宽，不证明无特殊原因。"}],
        ),
    }

    out["laney_u_chart"] = {
        "title": "Laney U' 图",
        "used_for": "过离散单位缺陷率。",
        "not_for": "普通泊松波动用 u。禁止与 u 共享。",
        "scenario": "可变单位+过离散。批20/29。",
        "related_ids": ["u_chart"],
        "dialog_fill": {"defects": "缺陷数", "units": "单位数"},
        "click_steps": [
            "导入 `demo_laney_u_overdispersed`。",
            "菜单：控制图 → Laney U' 图。",
            "缺陷数+单位数；历史限留空；规则默认；tests 留空。",
            "对照批20/29与加宽限；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "缺陷数 (`defects`)", "put": "缺陷数", "meaning": "缺陷计数。"},
            {"field": "单位数列 (`units`)", "put": "单位数", "meaning": "可变单位。"},
            {"field": "阶段列 (`stage`)", "put": "留空", "meaning": "看过离散。"},
            {"field": "历史中心线 (`historical_center`)", "put": "留空", "meaning": "用本集。"},
            {"field": "历史 Sigma Z (`historical_sigma_z`)", "put": "留空", "meaning": "估过离散。"},
            *_policy_detail("laney"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "Laney U'", "plain": "对 u 图做过离散校正。", "remember": "与普通 u 数据需求不同。"},
        ],
        "buried_signals": [
            {"row": 20, "what": "批20过离散高点", "expect": "体会加宽限。"},
            {"row": 29, "what": "批29过离散高点", "expect": "同上。"},
        ],
        "output_guide": [{"name": "Laney U'", "meaning": "Sigma Z 与加宽限。UCL≠USL。"}],
        "common_mistakes": _base_mistakes(["与 u_chart 共享"]),
        **_seven_plus(
            "用 Laney U' 理解过离散缺陷率。",
            [
                {"q": "与 u 共享？", "good": "禁止", "bad": "可以"},
                {"q": "历史 Sigma Z 本课？", "good": "留空让软件估", "bad": "随便填 1"},
                {"q": "UCL≠USL？", "good": "是", "bad": "否"},
            ],
            [{"after": "读批20", "prompt": "加宽限后仍越界说明什么？"}],
            "laney_u_overdispersed",
            ["批20/29", "为何独立于 u", "UCL≠USL"],
            [{"wrong": "加宽限后过程就合格", "right": "限宽校正≠规格合格。"}],
        ),
    }

    out["xbar_r"] = {
        "title": "Xbar-R 控制图",
        "used_for": "固定子组 n=5：Xbar 看位置，R 看组内极差。",
        "not_for": "单值流用 I-MR；n 较大看 S。禁止与 xbar_s 共享。",
        "scenario": "子组12极差尖峰；子组20均值台阶。",
        "related_ids": ["xbar_s", "imr"],
        "dialog_fill": {"variables": "厚度_um", "subgroup": "子组"},
        "click_steps": [
            "导入 `demo_xbar_r_n5_range_spike`。",
            "菜单：控制图 → Xbar-R 控制图。",
            "变量=`厚度_um`；子组列=`子组`；子组大小=5；阶段留空；规则默认；tests 留空。",
            "先看 R：子组12；再看 Xbar：子组20；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "变量 (`variables`)", "put": "厚度_um", "meaning": "测量 Y。"},
            {"field": "子组列 (`subgroup`)", "put": "子组", "meaning": "合理子组。"},
            {"field": "阶段列 (`stage`)", "put": "留空", "meaning": "同一阶段。"},
            {"field": "子组大小 (`subgroup_size`)", "put": "5", "meaning": "与数据一致。"},
            *_policy_detail("xbar"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "Xbar 图", "plain": "子组均值图。", "remember": "R 先乱则慎读 Xbar 限。"},
            {"term": "R 图", "plain": "子组极差图。", "remember": "先看组内是否失控。"},
        ],
        "buried_signals": [
            {"row": 56, "what": "子组12（约行56–60）极差尖峰", "expect": "R 图报警；先勿盲目读该段 Xbar。"},
            {"row": 96, "what": "子组20（约行96起）均值台阶", "expect": "Xbar 后段上移。"},
        ],
        "output_guide": [
            {"name": "R 图", "meaning": "子组12极差尖峰。"},
            {"name": "Xbar 图", "meaning": "子组20后上移。UCL≠USL。"},
        ],
        "common_mistakes": _base_mistakes(["只看 Xbar 忽略 R", "与 xbar_s 合并一张表"]),
        **_seven_plus(
            "用 Xbar-R 先读 R 尖峰再读 Xbar 台阶。",
            [
                {"q": "R 乱时先做什么？", "good": "先调查组内，慎读 Xbar 限", "bad": "只看 Xbar 下结论"},
                {"q": "与 xbar_s 共享？", "good": "禁止", "bad": "可以"},
                {"q": "UCL≠USL？", "good": "是", "bad": "否"},
            ],
            [{"after": "看子组12", "prompt": "为何教程强调先读 R？"}],
            "xbar_r_n5_range_spike",
            ["子组12与20", "n=5 为何", "UCL≠USL"],
            [{"wrong": "Xbar 在限内=过程合格", "right": "稳定≠符合规格；UCL≠USL。"}],
        ),
    }

    out["xbar_s"] = {
        "title": "Xbar-S 控制图",
        "used_for": "较大子组（本课 n=8）用 S 代替 R。",
        "not_for": "n=5 极差课用 xbar_r。禁止共享。",
        "scenario": "子组14起组内σ放大。",
        "related_ids": ["xbar_r"],
        "dialog_fill": {"variables": "厚度_um", "subgroup": "子组"},
        "click_steps": [
            "导入 `demo_xbar_s_n8_sd_shift`。",
            "菜单：控制图 → Xbar-S 控制图。",
            "变量=`厚度_um`；子组列=`子组`；子组大小=8；阶段留空；规则默认；tests 留空。",
            "对照子组14；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "变量 (`variables`)", "put": "厚度_um", "meaning": "测量 Y。"},
            {"field": "子组列 (`subgroup`)", "put": "子组", "meaning": "合理子组。"},
            {"field": "阶段列 (`stage`)", "put": "留空", "meaning": "同一阶段。"},
            {"field": "子组大小 (`subgroup_size`)", "put": "8", "meaning": "本课 n=8，不要留默认5。"},
            *_policy_detail("xbar"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "S 图", "plain": "子组标准差图。", "remember": "n 较大时比 R 更合适。"},
        ],
        "buried_signals": [
            {"row": 105, "what": "子组14（约行105起）σ放大", "expect": "S 图后段上移。"}
        ],
        "output_guide": [{"name": "S / Xbar", "meaning": "先看 S：子组14后上移。UCL≠USL。"}],
        "common_mistakes": _base_mistakes(["subgroup_size 仍留5", "与 xbar_r 共享"]),
        **_seven_plus(
            "用 Xbar-S 在 n=8 下检出子组14 σ 台阶。",
            [
                {"q": "本课 n？", "good": "8", "bad": "5"},
                {"q": "与 xbar_r 共享？", "good": "禁止", "bad": "可以"},
                {"q": "UCL≠USL？", "good": "是", "bad": "否"},
            ],
            [{"after": "设 n=8", "prompt": "若误留默认5会发生什么？"}],
            "xbar_s_n8_sd_shift",
            ["子组14", "为何不用 R", "UCL≠USL"],
            [{"wrong": "S 变大=必须停线", "right": "禁止「必须停线」；应查组内原因。"}],
        ),
    }

    out["z_mr"] = {
        "title": "Z-MR 控制图",
        "used_for": "短跑多产品：按组标准化后再画 I/MR。",
        "not_for": "单一稳定长跑优先普通 I-MR。",
        "scenario": "型号 A/B/C 短跑。型号B后段相对目标上移。",
        "related_ids": ["imr"],
        "dialog_fill": {"variables": "尺寸_mm", "group": "产品型号"},
        "click_steps": [
            "导入 `demo_z_mr_short_run`。",
            "菜单：控制图 → Z-MR 控制图。",
            "测量值=`尺寸_mm`；分组列=`产品型号`；MR长度=2；规则默认；tests 留空。",
            "对照行33–40 型号B；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "测量值 (`variables`)", "put": "尺寸_mm", "meaning": "Y。"},
            {"field": "分组列（可选）(`group`)", "put": "产品型号", "meaning": "短跑产品；本课应选。"},
            {"field": "移动极差长度 (`mr_length`)", "put": "2", "meaning": "估短期波动。"},
            *_policy_detail("z_mr"),
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "Z 标准化", "plain": "按组均值/σ 把不同产品拉到同一尺度。", "remember": "短跑混线常用。"},
        ],
        "buried_signals": [
            {"row": 33, "what": "行33起型号B相对其上移", "expect": "Z 图对应点抬高。"}
        ],
        "output_guide": [{"name": "Z-MR", "meaning": "型号B后段抬高。UCL≠USL。"}],
        "common_mistakes": _base_mistakes(["不选分组列", "与金标 imr 表混用"]),
        **_seven_plus(
            "用 Z-MR 在短跑混线中检出型号B偏移。",
            [
                {"q": "分组列本课？", "good": "产品型号", "bad": "必须留空"},
                {"q": "为何标准化？", "good": "不同产品目标不同", "bad": "为了证明正态"},
                {"q": "UCL≠USL？", "good": "是", "bad": "否"},
            ],
            [{"after": "选分组", "prompt": "若不分组直接画 I 图会怎样？"}],
            "z_mr_short_run",
            ["行33", "短跑含义", "UCL≠USL"],
            [{"wrong": "Z=0 就是规格中心", "right": "Z 相对组内统计中心，不是 USL/LSL。"}],
        ),
    }

    out["zone_chart"] = {
        "title": "区域图",
        "used_for": "按 Zone 积分配分识别同侧游程等模式。",
        "not_for": "不要当成能力分析；本课不是尖峰课。",
        "scenario": "片24–33 连续同侧偏高。",
        "related_ids": ["imr", "special_cause_rules"],
        "dialog_fill": {"variables": "厚度_um"},
        "click_steps": [
            "导入 `demo_zone_chart_runs`。",
            "菜单：控制图 → 区域图。",
            "测量值=`厚度_um`；MR长度=2；历史限留空。",
            "对照片24–33 积分抬高；UCL≠USL。",
        ],
        "dialog_fill_detail": [
            {"field": "测量值 (`variables`)", "put": "厚度_um", "meaning": "单值 Y。"},
            {"field": "移动极差长度 (`mr_length`)", "put": "2", "meaning": "分区尺度来自短期 σ。"},
            {"field": "历史均值 (`historical_center`)", "put": "留空", "meaning": "用本集。"},
            {"field": "历史 Sigma (`historical_sigma`)", "put": "留空", "meaning": "用本集。"},
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {"term": "Zone（区域）", "plain": "把控制限内分成 A/B/C 等带。", "remember": "同侧游程会积分配分。"},
            {"term": "区域积分", "plain": "按落点区域累加分数触发警戒。", "remember": "不是规格扣分。"},
        ],
        "buried_signals": [
            {"row": 24, "what": "片24–33 同侧游程", "expect": "区域累计分抬高。"}
        ],
        "output_guide": [{"name": "区域图", "meaning": "片24–33 积分抬高。UCL≠USL。"}],
        "common_mistakes": _base_mistakes(["把区域分当缺陷扣分"]),
        **_seven_plus(
            "用区域图识别片24–33 同侧游程积分。",
            [
                {"q": "本课主信号？", "good": "同侧游程", "bad": "单点大尖峰"},
                {"q": "区域分=规格扣分？", "good": "否", "bad": "是"},
                {"q": "UCL≠USL？", "good": "是", "bad": "否"},
            ],
            [{"after": "读游程", "prompt": "为何不是片55尖峰课？"}],
            "zone_chart_runs",
            ["片24–33", "Zone 含义", "UCL≠USL"],
            [{"wrong": "积分高必须停线", "right": "禁止「必须停线」；积分是调查触发器。"}],
        ),
    }

    out["special_cause_rules"] = {
        "title": "特殊原因判异规则（术语课）",
        "used_for": "理解 Nelson/Western Electric 等判异规则：灵敏度与误报权衡。公式/帮助参考。",
        "not_for": "不是独立菜单分析；不要为术语新增假 command。菜单可能仅作帮助条目。",
        "scenario": "学员在 I-MR / Xbar 课已见规则策略。本课只澄清规则含义与 UCL≠USL，不导入数据。",
        "related_ids": ["imr", "xbar_r", "zone_chart"],
        "dialog_fill": {},
        "click_steps": [
            "打开「帮助」→「算法、公式与参考资料」，查找特殊原因 / 控制图规则说明（本条目可能无独立菜单）。",
            "回到学习中心 I-MR 金标课，对照 rule_policy 与 tests 字段。",
            "口头复述：规则越多越灵敏也越易误报；UCL ≠ USL；禁止「过程合格/必须停线」。",
        ],
        "dialog_fill_detail": [
            {
                "field": "规则默认策略 (`rule_policy`)",
                "put": "（在具体控制图对话框中设置）",
                "meaning": "all_applicable=全部适用规则；minitab_like≈仅3σ。本术语课无独立对话框。",
            },
            {
                "field": "特殊原因测试 (`tests`)",
                "put": "留空或显式列表（在具体图中）",
                "meaning": "空=走策略；非空=explicit。",
            },
            {
                "field": "本课数据",
                "put": "不导入",
                "meaning": "formula_reference / 帮助条目；关联 imr 金标练习。",
            },
        ],
        "glossary": UCL_USL_GLOSSARY
        + [
            {
                "term": "Nelson / WE 规则",
                "plain": "除单点越3σ外，还有连续同侧、趋势等模式测试。",
                "remember": "规则↑灵敏度↑误报↑。",
            },
            {
                "term": "术语见关联课",
                "plain": "基础控制限术语以 I-MR 金标课 glossary 为准。",
                "remember": "related_ids → imr；本课不新增第185个假 id。",
            },
        ],
        "buried_signals": [],
        "output_guide": [
            {
                "name": "帮助/公式页",
                "meaning": "只陈述规则定义与误报权衡；禁止过程合格；禁止必须停线；禁止已证明正态。",
            }
        ],
        "common_mistakes": _base_mistakes(
            ["为本术语课伪造菜单命令", "把规则触发写成法律结论"]
        ),
        "skill_mission": "能解释判异规则灵敏度/误报权衡，并指向 I-MR 金标课区分 UCL 与 USL。",
        "prereq_quiz": [
            {"q": "规则开得越多？", "good": "更灵敏也更易误报", "bad": "一定更正确"},
            {"q": "本课是否新建假 command？", "good": "否，用 related_ids", "bad": "是，再加一个 id"},
            {"q": "UCL≠USL？", "good": "是", "bad": "否"},
        ],
        "self_explain": [
            {"after": "读规则列表", "prompt": "为何金标课要写清 rule_policy？"},
            {"after": "对照 imr", "prompt": "tests 留空时发生了什么？"},
        ],
        # 本课 formula_reference：不导入数据；fade 禁止强制导入 demo_imr_spi_shift
        "fade_levels": [
            {
                "level": 0,
                "student": "对照帮助页规则定义 + 金标 I-MR 课的 rule_policy/tests 字段说明。",
                "scaffold": "本课不导入表；参数含义表 + related_ids→imr 全部给出。",
            },
            {
                "level": 1,
                "student": "合上本页，默写 rule_policy 与 tests 各一句（空 tests 时走策略；非空=explicit）。",
                "scaffold": "只给字段名清单，不给标准答案。",
            },
            {
                "level": 2,
                "student": "写三段：①规则↑如何影响灵敏度；②误报如何上升；③为何不能写成「必须停线」。不导入演示表。",
                "scaffold": "仅术语表 + 误用禁止句；需要动手画图时另开 I-MR 金标课。",
            },
        ],
        "retrieval_quiz": ["规则与误报", "related_ids 指向谁", "UCL≠USL"],
        "misconceptions": [
            {
                "wrong": "触发任一规则=已证明失控必须停线",
                "right": "规则是统计线索；禁止「必须停线/已证明」。",
            },
            {
                "wrong": "点出 UCL = 产品超规格废品",
                "right": "UCL 是过程警戒线；超规格看 USL/LSL 或能力课。UCL ≠ USL。",
            },
            {
                "wrong": "为本术语课再加一个假 command_id 最干净",
                "right": "用 related_ids 指向已有控制图课；禁止撑到第 185 个假 id。",
            },
        ],
    }

    return out


def write_overlays() -> None:
    from copy_depth import polish_overlay

    OVERLAY_DIR.mkdir(parents=True, exist_ok=True)
    overlays = build_overlays()
    for cid, payload in overlays.items():
        payload = polish_overlay(cid, payload)
        path = OVERLAY_DIR / f"{cid}.json"
        path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(overlays)} Wave-1 overlays to {OVERLAY_DIR}")


if __name__ == "__main__":
    write_overlays()
