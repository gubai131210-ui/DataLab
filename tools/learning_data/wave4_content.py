#!/usr/bin/env python3
"""Wave-4 graph / DOE / reliability / rest: datasets, generators, overlay writers.

83 command_ids. Shared whitelist family: graph_hist_prob (histogram + probability_plot).
Run: python tools/learning_data/wave4_content.py
"""
from __future__ import annotations

import json
import math
import random
from pathlib import Path

from copy_depth import RELATED_BY_ID, polish_overlay
from glossary_bank import glossary_for

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
OVERLAY_DIR = HERE / "tutorial_overlays"
INVENTORY = ROOT / "docs/research/_tmp_command_inventory.json"
META = HERE / "id_metadata.json"

# ---------------------------------------------------------------------------
# Datasets (wave=4)
# ---------------------------------------------------------------------------

WAVE4_DATASETS: dict[str, dict] = {
    "graph_hist_prob": {
        "wave": 4,
        "practice_only": False,
        "title": "近正态厚度（直方图/概率图同构）",
        "industry": "electronics",
        "story": "单变量厚度，近正态轻微右偏，无 SPC 特殊原因。服务 histogram+probability_plot。",
        "row_count": 60,
        "notes": "埋点：行48–50 轻微右尾偏高（约+3μm）；全列无片41/55 失控尖峰。期望直方图单峰、概率图大致贴线；禁止已证明正态/过程合格。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–60"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "基线/右尾"},
        ],
    },
    "graph_density_unimodal": {
        "wave": 4,
        "practice_only": False,
        "title": "单峰密度厚度",
        "industry": "electronics",
        "story": "稳定单峰厚度，专供密度图课（不与 hist/prob 共享）。",
        "row_count": 55,
        "notes": "埋点：行1–55 单峰集中约100μm；约行40–42 略抬高形成肩部。期望密度峰清晰；禁止已证明正态。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–55"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "单峰/肩部"},
        ],
    },
    "graph_ecdf_unimodal": {
        "wave": 4,
        "practice_only": False,
        "title": "经验分布厚度",
        "industry": "electronics",
        "story": "单峰厚度供 ECDF 课。",
        "row_count": 50,
        "notes": "埋点：行45–50 上分位抬高；ECDF 后段变陡。禁止过程合格。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–50"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "上分位"},
        ],
    },
    "graph_two_group_box": {
        "wave": 4,
        "practice_only": False,
        "title": "两线箱线对比",
        "industry": "electronics",
        "story": "A/B 两线厚度；B 线位置抬高。",
        "row_count": 40,
        "notes": "埋点：线别=B 的全部行（约行21–40）中位约高 +1.5μm。期望箱线图位置差可见。",
        "columns": [
            {"index": 0, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 1, "name": "线别", "role_hint": "group", "description": "A/B"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "B抬高"},
        ],
    },
    "graph_violin_groups": {
        "wave": 4,
        "practice_only": False,
        "title": "两组小提琴形状",
        "industry": "electronics",
        "story": "两组厚度形状对比；B 组更宽。",
        "row_count": 48,
        "notes": "埋点：线别=B 行（约行25–48）散度更大。期望小提琴宽度差；禁止过程合格。",
        "columns": [
            {"index": 0, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 1, "name": "线别", "role_hint": "group", "description": "A/B"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "B更宽"},
        ],
    },
    "graph_interval_groups": {
        "wave": 4,
        "practice_only": False,
        "title": "三组区间均值",
        "industry": "electronics",
        "story": "腔号三组；腔3 均值偏低。",
        "row_count": 45,
        "notes": "埋点：腔号=3 的行（约行31–45）均值约低 2μm。期望区间图位置差。",
        "columns": [
            {"index": 0, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "响应"},
            {"index": 1, "name": "腔号", "role_hint": "category", "description": "1/2/3"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "腔3偏低"},
        ],
    },
    "graph_bar_category": {
        "wave": 4,
        "practice_only": False,
        "title": "缺陷类别条形计数",
        "industry": "electronics",
        "story": "缺陷类别频次；虚焊主导。",
        "row_count": 80,
        "notes": "埋点：全表约80行配方中虚焊约占一半（对照行1起任意抽样可见最高频类）；期望条形最高。禁止过程合格。",
        "columns": [
            {"index": 0, "name": "缺陷类别", "role_hint": "category", "description": "类别"},
            {"index": 1, "name": "备注", "role_hint": "note", "description": "频次配方"},
        ],
    },
    "graph_pie_category": {
        "wave": 4,
        "practice_only": False,
        "title": "缺陷类别饼图",
        "industry": "electronics",
        "story": "类别份额；虚焊最大片。",
        "row_count": 70,
        "notes": "埋点：虚焊约 40%+；期望饼图最大扇区。禁止过程合格。",
        "columns": [
            {"index": 0, "name": "缺陷类别", "role_hint": "category", "description": "类别"},
            {"index": 1, "name": "备注", "role_hint": "note", "description": "份额"},
        ],
    },
    "graph_mosaic_two_cat": {
        "wave": 4,
        "practice_only": False,
        "title": "班次×缺陷马赛克",
        "industry": "electronics",
        "story": "两分类列；晚班虚焊偏多。",
        "row_count": 100,
        "notes": "埋点：晚班×虚焊格占比抬高（约100行配方）；期望马赛克格面积差。对照行号仅作顺序，关联在类别组合。",
        "columns": [
            {"index": 0, "name": "班次", "role_hint": "category", "description": "早/晚"},
            {"index": 1, "name": "缺陷类别", "role_hint": "category", "description": "类型"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "关联"},
        ],
    },
    "graph_scatter_xy": {
        "wave": 4,
        "practice_only": False,
        "title": "温度-偏移散点",
        "industry": "electronics",
        "story": "回流温度 vs 偏移；正相关 + 离群。",
        "row_count": 45,
        "notes": "埋点：行42 偏移尖峰离群；整体正斜率。期望散点趋势+离群可见。",
        "columns": [
            {"index": 0, "name": "温度_C", "role_hint": "x", "unit": "°C", "description": "X"},
            {"index": 1, "name": "偏移_um", "role_hint": "y", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "相关/离群"},
        ],
    },
    "graph_hexbin_xy": {
        "wave": 4,
        "practice_only": False,
        "title": "密集 XY Hexbin",
        "industry": "electronics",
        "story": "大量 XY 点云；中心密度高。",
        "row_count": 120,
        "notes": "埋点：约行1–100 集中椭圆核；行101–120 外缘稀疏散点。期望 hex 中心深色。",
        "columns": [
            {"index": 0, "name": "X坐标", "role_hint": "x", "description": "X"},
            {"index": 1, "name": "Y坐标", "role_hint": "y", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "核/外缘"},
        ],
    },
    "graph_bubble_xyz": {
        "wave": 4,
        "practice_only": False,
        "title": "气泡 XYZ",
        "industry": "electronics",
        "story": "X/Y 位置 + 尺寸；大泡集中一角。",
        "row_count": 36,
        "notes": "埋点：行30–36 尺寸列显著更大。期望大气泡聚集。",
        "columns": [
            {"index": 0, "name": "X坐标", "role_hint": "x", "description": "X"},
            {"index": 1, "name": "Y坐标", "role_hint": "y", "description": "Y"},
            {"index": 2, "name": "尺寸", "role_hint": "size", "description": "气泡大小"},
            {"index": 3, "name": "备注", "role_hint": "note", "description": "大气泡"},
        ],
    },
    "graph_marginal_xy": {
        "wave": 4,
        "practice_only": False,
        "title": "边际图 XY",
        "industry": "electronics",
        "story": "XY 相关 + 边际分布。",
        "row_count": 50,
        "notes": "埋点：行1–50 正相关；边缘约正态分布。期望边际直方图单峰。",
        "columns": [
            {"index": 0, "name": "X值", "role_hint": "x", "description": "X"},
            {"index": 1, "name": "Y值", "role_hint": "y", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "相关"},
        ],
    },
    "graph_contour_xy": {
        "wave": 4,
        "practice_only": False,
        "title": "响应等值面样点",
        "industry": "electronics",
        "story": "X/Y/Z 网格样点；Z 在中心高。",
        "row_count": 64,
        "notes": "埋点：X≈0,Y≈0 附近 Z 最高（约行28–36）。期望等值线中心峰。",
        "columns": [
            {"index": 0, "name": "X因子", "role_hint": "x", "description": "X"},
            {"index": 1, "name": "Y因子", "role_hint": "y", "description": "Y"},
            {"index": 2, "name": "响应Z", "role_hint": "z", "description": "Z"},
            {"index": 3, "name": "备注", "role_hint": "note", "description": "中心峰"},
        ],
    },
    "graph_corr_matrix": {
        "wave": 4,
        "practice_only": False,
        "title": "三变量相关矩阵",
        "industry": "electronics",
        "story": "A 与 B 强相关，C 较弱。",
        "row_count": 40,
        "notes": "埋点：全列 A–B 相关约 0.85；A–C 弱。期望相关图 A-B 深色。",
        "columns": [
            {"index": 0, "name": "变量A", "role_hint": "measurement", "description": "A"},
            {"index": 1, "name": "变量B", "role_hint": "measurement", "description": "B"},
            {"index": 2, "name": "变量C", "role_hint": "measurement", "description": "C"},
            {"index": 3, "name": "备注", "role_hint": "note", "description": "相关结构"},
        ],
    },
    "graph_heatmap_matrix": {
        "wave": 4,
        "practice_only": False,
        "title": "热图三变量",
        "industry": "electronics",
        "story": "与相关结构类似的热图课专用表。",
        "row_count": 36,
        "notes": "埋点：变量A–B 高相关块；行号仅顺序。期望热图对角外 A-B 亮。",
        "columns": [
            {"index": 0, "name": "变量A", "role_hint": "measurement", "description": "A"},
            {"index": 1, "name": "变量B", "role_hint": "measurement", "description": "B"},
            {"index": 2, "name": "变量C", "role_hint": "measurement", "description": "C"},
            {"index": 3, "name": "备注", "role_hint": "note", "description": "热图块"},
        ],
    },
    "graph_matrix_three": {
        "wave": 4,
        "practice_only": False,
        "title": "矩阵散点三列",
        "industry": "electronics",
        "story": "三连续变量矩阵图。",
        "row_count": 40,
        "notes": "埋点：列1–2 正斜率；列3 独立噪声。期望矩阵图对应面板相关。",
        "columns": [
            {"index": 0, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "V1"},
            {"index": 1, "name": "偏移_um", "role_hint": "measurement", "unit": "μm", "description": "V2"},
            {"index": 2, "name": "温度_C", "role_hint": "measurement", "unit": "°C", "description": "V3"},
            {"index": 3, "name": "备注", "role_hint": "note", "description": "面板相关"},
        ],
    },
    "graph_parallel_multi": {
        "wave": 4,
        "practice_only": False,
        "title": "平行坐标多变量",
        "industry": "electronics",
        "story": "四指标；差品在后两轴偏低。",
        "row_count": 30,
        "notes": "埋点：行21–30（备注=差品）在轴3/轴4 系统性偏低。期望平行坐标交叉簇。",
        "columns": [
            {"index": 0, "name": "轴1", "role_hint": "measurement", "description": "指标1"},
            {"index": 1, "name": "轴2", "role_hint": "measurement", "description": "指标2"},
            {"index": 2, "name": "轴3", "role_hint": "measurement", "description": "指标3"},
            {"index": 3, "name": "轴4", "role_hint": "measurement", "description": "指标4"},
            {"index": 4, "name": "备注", "role_hint": "note", "description": "良/差"},
        ],
    },
    "graph_area_time": {
        "wave": 4,
        "practice_only": False,
        "title": "按周面积序列",
        "industry": "electronics",
        "story": "周序产量面积图；后段抬升。",
        "row_count": 36,
        "notes": "埋点：周次≥25（行25起）产量台阶抬高。期望面积图后段抬升。",
        "columns": [
            {"index": 0, "name": "周次", "role_hint": "time", "description": "1–36"},
            {"index": 1, "name": "产量", "role_hint": "value", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "台阶"},
        ],
    },
    "graph_eda4_series": {
        "wave": 4,
        "practice_only": False,
        "title": "EDA 四图单列",
        "industry": "electronics",
        "story": "单列时间序；中段漂移供四图诊断。",
        "row_count": 48,
        "notes": "埋点：行25–36 均值抬高约 +1.2。期望 run/lag 等面板显示非随机。禁止已证明正态。",
        "columns": [
            {"index": 0, "name": "片号", "role_hint": "order", "description": "1–48"},
            {"index": 1, "name": "厚度_um", "role_hint": "measurement", "unit": "μm", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "中段漂移"},
        ],
    },
    "pca_three_var": {
        "wave": 4,
        "practice_only": False,
        "title": "三相关变量 PCA",
        "industry": "electronics",
        "story": "三指标共线结构；PC1 解释主变异。",
        "row_count": 40,
        "notes": "埋点：三列强共线；行35–40 在第三方向略离群。期望 PC1 方差占比高。",
        "columns": [
            {"index": 0, "name": "指标X", "role_hint": "measurement", "description": "X"},
            {"index": 1, "name": "指标Y", "role_hint": "measurement", "description": "Y"},
            {"index": 2, "name": "指标Z", "role_hint": "measurement", "description": "Z"},
            {"index": 3, "name": "备注", "role_hint": "note", "description": "共线/离群"},
        ],
    },
    "doe_factorial_y": {
        "wave": 4,
        "practice_only": False,
        "title": "2因子析因响应表",
        "industry": "electronics",
        "story": "温度×压力 编码 ±1；响应含主效应。",
        "row_count": 32,
        "notes": "埋点：温度=+1 的行响应系统性抬高；压力效应较弱。2×2×8 重复=32 行。期望效应图温度主效应明显。",
        "columns": [
            {"index": 0, "name": "温度", "role_hint": "factor", "description": "±1"},
            {"index": 1, "name": "压力", "role_hint": "factor", "description": "±1"},
            {"index": 2, "name": "响应Y", "role_hint": "response", "description": "Y"},
            {"index": 3, "name": "备注", "role_hint": "note", "description": "温度主效应"},
        ],
    },
    "doe_opt_two_resp": {
        "wave": 4,
        "practice_only": False,
        "title": "双响应优化样点",
        "industry": "electronics",
        "story": "两因子两响应；目标冲突弱。",
        "row_count": 32,
        "notes": "埋点：温度高时 Y1 升、Y2 略降；2×2×8 重复=32 行。期望优化折中区。禁止过程合格。",
        "columns": [
            {"index": 0, "name": "温度", "role_hint": "factor", "description": "±1"},
            {"index": 1, "name": "压力", "role_hint": "factor", "description": "±1"},
            {"index": 2, "name": "强度Y1", "role_hint": "response", "description": "Y1"},
            {"index": 3, "name": "翘曲Y2", "role_hint": "response", "description": "Y2"},
            {"index": 4, "name": "备注", "role_hint": "note", "description": "折中"},
        ],
    },
    "mix_simplex_3": {
        "wave": 4,
        "practice_only": False,
        "title": "三组分混料点",
        "industry": "electronics",
        "story": "三组分和为1；顶点与中心点。",
        "row_count": 30,
        "notes": "埋点：行1–3 为顶点（单组分≈1）；行4–9 为边中点；行10–30 为内部/扰动点（和≈1）。期望单纯形点落在三角形内。",
        "columns": [
            {"index": 0, "name": "组分A", "role_hint": "component", "description": "比例"},
            {"index": 1, "name": "组分B", "role_hint": "component", "description": "比例"},
            {"index": 2, "name": "组分C", "role_hint": "component", "description": "比例"},
            {"index": 3, "name": "备注", "role_hint": "note", "description": "顶点/中心"},
        ],
    },
    "rel_warranty_counts": {
        "wave": 4,
        "practice_only": False,
        "title": "保修暴露量汇总",
        "industry": "electronics",
        "story": "各批次暴露量；供保修摘要可选列。",
        "row_count": 30,
        "notes": "埋点：行1–30 暴露量列可求和；对话框仍需填保修窗口与 R(Tw)。期望暴露列优先于标量。",
        "columns": [
            {"index": 0, "name": "批次", "role_hint": "order", "description": "1–30"},
            {"index": 1, "name": "暴露量", "role_hint": "exposure", "description": "台时/件数"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "暴露"},
        ],
    },
    "ts_weekly_yield_series": {
        "wave": 4,
        "practice_only": False,
        "title": "周产量时间序列图",
        "industry": "electronics",
        "story": "周次+产量；后段抬升。专用 ts 图（不与分解/平滑共享）。",
        "row_count": 40,
        "notes": "埋点：周次≥30（行30起）产量抬高。期望时间序列图后段上移。",
        "columns": [
            {"index": 0, "name": "周次", "role_hint": "time", "description": "1–40"},
            {"index": 1, "name": "产量", "role_hint": "value", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "后段抬升"},
        ],
    },
    "ts_decomp_seasonal": {
        "wave": 4,
        "practice_only": False,
        "title": "季节分解周序列",
        "industry": "electronics",
        "story": "周期=4 的季节+趋势序列。",
        "row_count": 48,
        "notes": "埋点：季节周期4；行1–48 叠加缓慢上趋势。期望分解季节分量周期≈4。",
        "columns": [
            {"index": 0, "name": "周次", "role_hint": "time", "description": "1–48"},
            {"index": 1, "name": "产量", "role_hint": "value", "description": "Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "季节+趋势"},
        ],
    },
    "ts_smooth_weekly": {
        "wave": 4,
        "practice_only": False,
        "title": "平滑用周产量",
        "industry": "electronics",
        "story": "含噪声的周产量；平滑可见趋势。",
        "row_count": 40,
        "notes": "埋点：行1–40 含噪声上升趋势；期望指数平滑轨迹比原始更顺。禁止过程合格。",
        "columns": [
            {"index": 0, "name": "周次", "role_hint": "order", "description": "1–40"},
            {"index": 1, "name": "产量", "role_hint": "measurement", "description": "序列 Y"},
            {"index": 2, "name": "备注", "role_hint": "note", "description": "噪声趋势"},
        ],
    },
}

# Mapping role_map: keys must ⊆ id_metadata roles (often first role only).
ROLE_MAP_BY_DATASET: dict[str, dict[str, str]] = {
    "graph_hist_prob": {"variables": "厚度_um"},  # overridden per-command below
    "graph_density_unimodal": {"variable": "厚度_um"},
    "graph_ecdf_unimodal": {"variable": "厚度_um"},
    "graph_two_group_box": {"variables": "厚度_um"},
    "graph_violin_groups": {"variables": "厚度_um"},
    "graph_interval_groups": {"response": "厚度_um"},
    "graph_bar_category": {"category": "缺陷类别"},
    "graph_pie_category": {"category": "缺陷类别"},
    "graph_mosaic_two_cat": {"categories": "班次,缺陷类别"},
    "graph_scatter_xy": {"x_variable": "温度_C"},
    "graph_hexbin_xy": {"x_variable": "X坐标"},
    "graph_bubble_xyz": {"x_variable": "X坐标"},
    "graph_marginal_xy": {"x_variable": "X值"},
    "graph_contour_xy": {"x_variable": "X因子"},
    "graph_corr_matrix": {"variables": "变量A,变量B,变量C"},
    "graph_heatmap_matrix": {"variables": "变量A,变量B,变量C"},
    "graph_matrix_three": {"variables": "厚度_um,偏移_um,温度_C"},
    "graph_parallel_multi": {"variables": "轴1,轴2,轴3,轴4"},
    "graph_area_time": {"time": "周次"},
    "graph_eda4_series": {"variables": "厚度_um"},
    "pca_three_var": {"variables": "指标X,指标Y,指标Z"},
    "doe_factorial_y": {"response": "响应Y"},
    "doe_opt_two_resp": {"response": "强度Y1,翘曲Y2"},
    "mix_simplex_3": {"components": "组分A,组分B,组分C"},
    "rel_warranty_counts": {"exposure_col": "暴露量"},
    "ts_weekly_yield_series": {"time": "周次"},
    "ts_decomp_seasonal": {"time": "周次"},
    "ts_smooth_weekly": {"variables": "产量"},
}

# histogram vs probability_plot share dataset but different role ids
ROLE_MAP_BY_COMMAND: dict[str, dict[str, str]] = {
    "histogram": {"variables": "厚度_um"},
    "probability_plot": {"variable": "厚度_um"},
}

# ---------------------------------------------------------------------------
# Generators
# ---------------------------------------------------------------------------


def gen_graph_hist_prob(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(401)
    rows = []
    for i in range(1, 61):
        y = 100.0 + rng.gauss(0, 1.0)
        note = "基线"
        if 48 <= i <= 50:
            y += 3.0
            note = "右尾"
        rows.append([str(i), f"{y:.2f}", note])
    return rows


def gen_graph_density_unimodal(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(402)
    rows = []
    for i in range(1, 56):
        y = 100.0 + rng.gauss(0, 0.9)
        note = "单峰"
        if 40 <= i <= 42:
            y += 1.5
            note = "肩部"
        rows.append([str(i), f"{y:.2f}", note])
    return rows


def gen_graph_ecdf_unimodal(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(403)
    rows = []
    for i in range(1, 51):
        y = 100.0 + rng.gauss(0, 1.0)
        note = "基线"
        if i >= 45:
            y += 2.0
            note = "上分位"
        rows.append([str(i), f"{y:.2f}", note])
    return rows


def gen_graph_two_group_box(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(404)
    rows = []
    for _ in range(20):
        rows.append([f"{100.0 + rng.gauss(0, 1.0):.2f}", "A", "基线"])
    for _ in range(20):
        rows.append([f"{101.5 + rng.gauss(0, 1.0):.2f}", "B", "B抬高"])
    return rows


def gen_graph_violin_groups(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(405)
    rows = []
    for _ in range(24):
        rows.append([f"{100.0 + rng.gauss(0, 0.7):.2f}", "A", "窄"])
    for _ in range(24):
        rows.append([f"{100.0 + rng.gauss(0, 1.6):.2f}", "B", "B更宽"])
    return rows


def gen_graph_interval_groups(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(406)
    rows = []
    for cav in (1, 2, 3):
        mean = 98.0 if cav == 3 else 100.0
        note = "腔3偏低" if cav == 3 else "基线"
        for _ in range(15):
            rows.append([f"{mean + rng.gauss(0, 0.8):.2f}", str(cav), note])
    return rows


def gen_graph_bar_category(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(407)
    bag = ["虚焊"] * 40 + ["偏移"] * 20 + ["桥连"] * 12 + ["其他"] * 8
    rng.shuffle(bag)
    return [[c, "频次配方"] for c in bag]


def gen_graph_pie_category(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(408)
    bag = ["虚焊"] * 30 + ["偏移"] * 18 + ["桥连"] * 12 + ["其他"] * 10
    rng.shuffle(bag)
    return [[c, "份额"] for c in bag]


def gen_graph_mosaic_two_cat(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(409)
    rows = []
    for _ in range(50):
        rows.append(["早班", rng.choice(["虚焊", "偏移", "桥连", "偏移"]), "基线"])
    for _ in range(50):
        rows.append(["晚班", rng.choice(["虚焊", "虚焊", "虚焊", "偏移", "桥连"]), "晚班虚焊↑"])
    rng.shuffle(rows)
    return rows


def gen_graph_scatter_xy(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(410)
    rows = []
    for i in range(1, 46):
        x = 230.0 + rng.gauss(0, 8.0)
        y = 0.25 * (x - 230.0) + 5.0 + rng.gauss(0, 1.0)
        note = "正相关"
        if i == 42:
            y += 6.0
            note = "离群"
        rows.append([f"{x:.1f}", f"{y:.2f}", note])
    return rows


def gen_graph_hexbin_xy(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(411)
    rows = []
    for i in range(1, 121):
        if i <= 100:
            x, y = rng.gauss(0, 1.0), rng.gauss(0, 1.0)
            note = "核"
        else:
            x, y = rng.gauss(0, 2.5), rng.gauss(0, 2.5)
            note = "外缘"
        rows.append([f"{x:.2f}", f"{y:.2f}", note])
    return rows


def gen_graph_bubble_xyz(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(412)
    rows = []
    for i in range(1, 37):
        x, y = rng.uniform(0, 10), rng.uniform(0, 10)
        size = 2.0 + rng.random()
        note = "基线"
        if i >= 30:
            size = 8.0 + rng.random()
            note = "大气泡"
        rows.append([f"{x:.2f}", f"{y:.2f}", f"{size:.2f}", note])
    return rows


def gen_graph_marginal_xy(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(413)
    rows = []
    for _ in range(50):
        x = rng.gauss(0, 1.0)
        y = 0.7 * x + rng.gauss(0, 0.6)
        rows.append([f"{x:.2f}", f"{y:.2f}", "相关"])
    return rows


def gen_graph_contour_xy(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(414)
    rows = []
    xs = [-1.5, -0.5, 0.5, 1.5]
    ys = [-1.5, -0.5, 0.5, 1.5]
    idx = 0
    for x in xs:
        for y in ys:
            for _rep in range(4):
                idx += 1
                z = 10.0 - (x * x + y * y) + rng.gauss(0, 0.2)
                note = "中心峰" if abs(x) < 0.6 and abs(y) < 0.6 else "边缘"
                rows.append([f"{x:.1f}", f"{y:.1f}", f"{z:.2f}", note])
    return rows


def gen_graph_corr_matrix(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(415)
    rows = []
    for _ in range(40):
        a = rng.gauss(0, 1.0)
        b = 0.85 * a + rng.gauss(0, 0.4)
        c = rng.gauss(0, 1.0)
        rows.append([f"{a:.2f}", f"{b:.2f}", f"{c:.2f}", "A-B强"])
    return rows


def gen_graph_heatmap_matrix(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(416)
    rows = []
    for _ in range(36):
        a = rng.gauss(0, 1.0)
        b = 0.8 * a + rng.gauss(0, 0.45)
        c = 0.2 * a + rng.gauss(0, 1.0)
        rows.append([f"{a:.2f}", f"{b:.2f}", f"{c:.2f}", "热图块"])
    return rows


def gen_graph_matrix_three(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(417)
    rows = []
    for _ in range(40):
        t = 100.0 + rng.gauss(0, 1.0)
        o = 0.6 * (t - 100.0) + 5.0 + rng.gauss(0, 0.8)
        temp = 240.0 + rng.gauss(0, 5.0)
        rows.append([f"{t:.2f}", f"{o:.2f}", f"{temp:.1f}", "面板相关"])
    return rows


def gen_graph_parallel_multi(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(418)
    rows = []
    for i in range(1, 31):
        if i <= 20:
            vals = [rng.gauss(1, 0.2) for _ in range(4)]
            note = "良品"
        else:
            vals = [rng.gauss(1, 0.2), rng.gauss(1, 0.2), rng.gauss(0.3, 0.15), rng.gauss(0.3, 0.15)]
            note = "差品"
        rows.append([f"{v:.2f}" for v in vals] + [note])
    return rows


def gen_graph_area_time(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(419)
    rows = []
    for w in range(1, 37):
        y = 100.0 + rng.gauss(0, 3.0)
        note = "基线"
        if w >= 25:
            y += 15.0
            note = "台阶"
        rows.append([str(w), f"{y:.1f}", note])
    return rows


def gen_graph_eda4_series(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(420)
    rows = []
    for i in range(1, 49):
        y = 100.0 + rng.gauss(0, 0.6)
        note = "基线"
        if 25 <= i <= 36:
            y += 1.2
            note = "中段漂移"
        rows.append([str(i), f"{y:.2f}", note])
    return rows


def gen_pca_three_var(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(421)
    rows = []
    for i in range(1, 41):
        t = rng.gauss(0, 1.0)
        x = t + rng.gauss(0, 0.15)
        y = t + rng.gauss(0, 0.15)
        z = t + rng.gauss(0, 0.15)
        note = "共线"
        if i >= 35:
            z += 2.0
            note = "离群"
        rows.append([f"{x:.2f}", f"{y:.2f}", f"{z:.2f}", note])
    return rows


def gen_doe_factorial_y(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(422)
    rows = []
    for temp in (-1, 1):
        for press in (-1, 1):
            for _rep in range(8):
                y = 50.0 + 4.0 * temp + 1.0 * press + rng.gauss(0, 0.5)
                note = "温度主效应" if temp == 1 else "基线"
                rows.append([str(temp), str(press), f"{y:.2f}", note])
    return rows


def gen_doe_opt_two_resp(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(423)
    rows = []
    for temp in (-1, 1):
        for press in (-1, 1):
            for _rep in range(8):
                y1 = 80.0 + 5.0 * temp + rng.gauss(0, 0.6)
                y2 = 2.0 - 0.4 * temp + 0.2 * press + rng.gauss(0, 0.1)
                rows.append([str(temp), str(press), f"{y1:.2f}", f"{y2:.2f}", "折中"])
    return rows


def gen_mix_simplex_3(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(428)
    rows = [
        ["1.00", "0.00", "0.00", "顶点A"],
        ["0.00", "1.00", "0.00", "顶点B"],
        ["0.00", "0.00", "1.00", "顶点C"],
        ["0.50", "0.50", "0.00", "边AB"],
        ["0.50", "0.00", "0.50", "边AC"],
        ["0.00", "0.50", "0.50", "边BC"],
        ["0.70", "0.15", "0.15", "近A"],
        ["0.15", "0.70", "0.15", "近B"],
        ["0.15", "0.15", "0.70", "近C"],
        ["0.33", "0.33", "0.34", "中心"],
    ]
    while len(rows) < 30:
        a = rng.random()
        b = rng.random() * (1.0 - a)
        c = 1.0 - a - b
        rows.append([f"{a:.2f}", f"{b:.2f}", f"{c:.2f}", "内部扰动"])
    return rows


def gen_rel_warranty_counts(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(424)
    rows = []
    for i in range(1, 31):
        exp = 800 + rng.randint(0, 400)
        rows.append([str(i), str(exp), "暴露"])
    return rows


def gen_ts_weekly_yield_series(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(425)
    rows = []
    for w in range(1, 41):
        y = 95.0 + rng.gauss(0, 2.0)
        note = "基线"
        if w >= 30:
            y += 8.0
            note = "后段抬升"
        rows.append([str(w), f"{y:.1f}", note])
    return rows


def gen_ts_decomp_seasonal(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(426)
    rows = []
    seasonal = [2.0, -1.0, -2.0, 1.0]
    for w in range(1, 49):
        trend = 0.15 * w
        y = 100.0 + trend + seasonal[(w - 1) % 4] + rng.gauss(0, 0.8)
        rows.append([str(w), f"{y:.2f}", "季节+趋势"])
    return rows


def gen_ts_smooth_weekly(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(427)
    rows = []
    for w in range(1, 41):
        y = 90.0 + 0.25 * w + rng.gauss(0, 3.5)
        rows.append([str(w), f"{y:.2f}", "噪声趋势"])
    return rows


GENERATORS = {
    "graph_hist_prob": gen_graph_hist_prob,
    "graph_density_unimodal": gen_graph_density_unimodal,
    "graph_ecdf_unimodal": gen_graph_ecdf_unimodal,
    "graph_two_group_box": gen_graph_two_group_box,
    "graph_violin_groups": gen_graph_violin_groups,
    "graph_interval_groups": gen_graph_interval_groups,
    "graph_bar_category": gen_graph_bar_category,
    "graph_pie_category": gen_graph_pie_category,
    "graph_mosaic_two_cat": gen_graph_mosaic_two_cat,
    "graph_scatter_xy": gen_graph_scatter_xy,
    "graph_hexbin_xy": gen_graph_hexbin_xy,
    "graph_bubble_xyz": gen_graph_bubble_xyz,
    "graph_marginal_xy": gen_graph_marginal_xy,
    "graph_contour_xy": gen_graph_contour_xy,
    "graph_corr_matrix": gen_graph_corr_matrix,
    "graph_heatmap_matrix": gen_graph_heatmap_matrix,
    "graph_matrix_three": gen_graph_matrix_three,
    "graph_parallel_multi": gen_graph_parallel_multi,
    "graph_area_time": gen_graph_area_time,
    "graph_eda4_series": gen_graph_eda4_series,
    "pca_three_var": gen_pca_three_var,
    "doe_factorial_y": gen_doe_factorial_y,
    "doe_opt_two_resp": gen_doe_opt_two_resp,
    "mix_simplex_3": gen_mix_simplex_3,
    "rel_warranty_counts": gen_rel_warranty_counts,
    "ts_weekly_yield_series": gen_ts_weekly_yield_series,
    "ts_decomp_seasonal": gen_ts_decomp_seasonal,
    "ts_smooth_weekly": gen_ts_smooth_weekly,
}

WAVE4_COMMAND_IDS: list[str] = []  # filled in write_overlays from inventory


# ---------------------------------------------------------------------------
# Overlay helpers
# ---------------------------------------------------------------------------


def _seven_plus(mission, prereq, self_explain, fade_ds, retrieval, misc) -> dict:
    if fade_ds:
        fade = [
            {
                "level": 0,
                "student": "跟着 §4 填对话框、跟着 §5 对照埋点读输出。",
                "scaffold": "参数表 + 读图话术 + 埋点全部给出。",
            },
            {
                "level": 1,
                "student": f"仍导入 `demo_{fade_ds}`；最后「埋点是否出现、禁止句」自己写三句。",
                "scaffold": "对话框字段仍给出；不给标准答案。",
            },
            {
                "level": 2,
                "student": f"再导入 `demo_{fade_ds}`，自己改一个可选字段，不看埋点剧透写结论。",
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
                "student": "不看 detail，默写本命令真实 roles/inputs 名称。",
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
        + [{"wrong": "看完例题不做练习也会了", "right": "必须做褪脚手架和检索小测。"}],
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


def _data_overlay(
    *,
    title: str,
    used_for: str,
    not_for: str,
    scenario: str,
    related: list[str],
    dialog_fill: dict,
    click_steps: list[str],
    detail: list[dict],
    glossary: list[dict],
    buried: list[dict],
    output_guide: list[dict],
    mission: str,
    prereq: list,
    self_explain: list,
    fade_ds: str,
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
        "dialog_fill": dialog_fill,
        "click_steps": click_steps,
        "dialog_fill_detail": detail,
        "glossary": glossary,
        "buried_signals": buried,
        "output_guide": output_guide,
        "common_mistakes": _base_mistakes(extra_mistakes),
        **_seven_plus(mission, prereq, self_explain, fade_ds, retrieval, misc),
    }


def _g3(a, b, c) -> list[dict]:
    return [
        {"term": a[0], "plain": a[1], "remember": a[2]},
        {"term": b[0], "plain": b[1], "remember": b[2]},
        {"term": c[0], "plain": c[1], "remember": c[2]},
    ]


def build_data_overlays() -> dict[str, dict]:
    out: dict[str, dict] = {}

    out["histogram"] = _data_overlay(
        title="直方图",
        used_for="看单变量形状/峰度/偏态线索。与概率图同构共享 graph_hist_prob。",
        not_for="替代控制图；规格符合性。禁止已证明正态。禁止扩共享到 density/ecdf。",
        scenario="近正态厚度；行48–50 轻微右尾。导入 demo_graph_hist_prob。",
        related=["probability_plot", "density_plot", "capability"],
        dialog_fill={"variables": "厚度_um"},
        click_steps=[
            "导入 `demo_graph_hist_prob`。",
            "菜单：图形 → 直方图。",
            "变量=`厚度_um`；分组可留空；组数可留空自动。",
            "对照行48–50 右尾肩部；禁止已证明正态；禁止过程合格。",
        ],
        detail=[
            {"field": "变量（可多选） (`variables`)", "put": "厚度_um", "meaning": "连续 Y。"},
            {"field": "分组变量 (`by`)", "put": "留空", "meaning": "可选；本课不分面。"},
            {"field": "组数（可选） (`bins`)", "put": "留空", "meaning": "自动分箱。"},
        ],
        glossary=_g3(
            ("直方图", "频数按区间堆积。", "形状线索，不是控制限。"),
            ("右尾", "高值侧拉长。", "本课行48–50。"),
            ("已证明正态", "禁止句。", "图贴线≠证明。"),
        ),
        buried=[{"row": 48, "what": "行48–50 轻微右尾偏高", "expect": "直方图右侧肩部；勿写已证明正态/过程合格。"}],
        output_guide=[{"name": "直方图", "meaning": "指着右尾肩部。禁止已证明正态。"}],
        mission="能导入同构表画直方图并拒绝正态证明话术。",
        prereq=[
            {"q": "与概率图共享表？", "good": "graph_hist_prob", "bad": "各一张宽表"},
            {"q": "可写已证明正态？", "good": "禁止", "bad": "可以"},
            {"q": "UCL=柱高？", "good": "否", "bad": "是"},
        ],
        self_explain=[
            {"after": "画图", "prompt": "为何不把 density 挂到同表？"},
            {"after": "读右尾", "prompt": "右尾=必须停线吗？"},
        ],
        fade_ds="graph_hist_prob",
        retrieval=["共享族？", "行48？", "禁止句"],
        misc=[{"wrong": "直方图贴正态曲线=已证明正态", "right": "禁止已证明正态。"}],
    )

    out["probability_plot"] = _data_overlay(
        title="正态概率图",
        used_for="用概率图看点是否大致贴参考线。与直方图同构共享 graph_hist_prob。",
        not_for="替代正式正态性检验结论；禁止已证明正态。",
        scenario="同一张近正态表；角色 id 是 variable（不是 variables）。",
        related=["histogram", "normality_test"],
        dialog_fill={"variable": "厚度_um"},
        click_steps=[
            "导入 `demo_graph_hist_prob`（与直方图同构白名单）。",
            "菜单：图形 → 概率图。",
            "变量=`厚度_um`；分面留空。",
            "对照行48–50 右尾偏离；禁止已证明正态。",
        ],
        detail=[
            {"field": "变量 (`variable`)", "put": "厚度_um", "meaning": "单列 Y；注意 role id 与直方图不同。"},
            {"field": "分面变量 (`facet`)", "put": "留空", "meaning": "可选。"},
            {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "有分面时限制面板。"},
        ],
        glossary=_g3(
            ("概率图", "分位对照参考线。", "贴线≠证明正态。"),
            ("variable 角色", "本命令用 variable。", "直方图用 variables。"),
            ("同构共享", "白名单 graph_hist_prob。", "禁止扩到 density。"),
        ),
        buried=[{"row": 48, "what": "右尾三点", "expect": "概率图高端偏离参考线；禁止已证明正态。"}],
        output_guide=[{"name": "概率图", "meaning": "指着高端偏离。禁止已证明正态。"}],
        mission="能区分 variable vs variables，并拒绝正态证明话术。",
        prereq=[
            {"q": "角色 id？", "good": "variable", "bad": "variables"},
            {"q": "共享表？", "good": "graph_hist_prob", "bad": "imr_spi_shift"},
            {"q": "已证明正态？", "good": "禁止", "bad": "可以"},
        ],
        self_explain=[
            {"after": "填角色", "prompt": "为何不能抄直方图的 variables 键？"},
            {"after": "读图", "prompt": "贴线能否写已证明正态？"},
        ],
        fade_ds="graph_hist_prob",
        retrieval=["role id？", "行48？", "禁止句"],
        misc=[{"wrong": "概率图通过=已证明正态", "right": "禁止已证明正态。"}],
    )

    # Remaining data lessons — compact but complete
    specs = [
        (
            "density_plot",
            "密度图",
            "graph_density_unimodal",
            {"variable": "厚度_um"},
            [
                {"field": "变量 (`variable`)", "put": "厚度_um", "meaning": "连续 Y。"},
                {"field": "分面变量 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("密度图", "核密度估计曲线。", "探索形状。"), ("单峰", "一个主峰。", "本课目标。"), ("非共享", "不挂 graph_hist_prob。", "白名单外。")),
            [{"row": 40, "what": "行40–42 肩部", "expect": "密度峰右肩；禁止已证明正态。"}],
            "图形 → 密度图",
            "variable=`厚度_um`",
        ),
        (
            "ecdf_plot",
            "经验累积分布图",
            "graph_ecdf_unimodal",
            {"variable": "厚度_um"},
            [
                {"field": "变量 (`variable`)", "put": "厚度_um", "meaning": "连续 Y。"},
                {"field": "分面变量 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("ECDF", "经验累积分布。", "看分位。"), ("上分位", "高端抬高。", "行45–50。"), ("禁止句", "勿写过程合格。", "探索图。")),
            [{"row": 45, "what": "上分位抬高", "expect": "ECDF 后段变陡；禁止过程合格。"}],
            "图形 → 经验累积分布图",
            "variable=`厚度_um`",
        ),
        (
            "boxplot",
            "箱线图",
            "graph_two_group_box",
            {"variables": "厚度_um"},
            [
                {"field": "变量 (`variables`)", "put": "厚度_um", "meaning": "连续 Y。"},
                {"field": "分类变量 (`by`)", "put": "线别", "meaning": "分组对比。"},
            ],
            _g3(("箱线图", "五数概括。", "比位置/散度。"), ("中位数", "箱中线。", "≠UCL/USL。"), ("B线抬高", "行21–40。", "位置差。")),
            [{"row": 21, "what": "B线行中位抬高", "expect": "箱线位置差；中位数≠UCL。"}],
            "图形 → 箱线图",
            "变量=`厚度_um`；分类=`线别`",
        ),
        (
            "violin_plot",
            "小提琴图",
            "graph_violin_groups",
            {"variables": "厚度_um"},
            [
                {"field": "变量 (`variables`)", "put": "厚度_um", "meaning": "连续 Y。"},
                {"field": "分类变量 (`by`)", "put": "线别", "meaning": "分组。"},
                {"field": "分面变量 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("小提琴图", "密度+箱线信息。", "看形状宽度。"), ("宽度", "局部密度。", "B更宽。"), ("禁止句", "勿过程合格。", "探索。")),
            [{"row": 25, "what": "B组散度更大", "expect": "小提琴更宽；禁止过程合格。"}],
            "图形 → 小提琴图",
            "变量=`厚度_um`；分类=`线别`",
        ),
        (
            "interval_plot",
            "区间图",
            "graph_interval_groups",
            {"response": "厚度_um"},
            [
                {"field": "响应变量 (`response`)", "put": "厚度_um", "meaning": "Y。"},
                {"field": "类别 (`category`)", "put": "腔号", "meaning": "分组。"},
                {"field": "分面 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "置信水平 (`confidence`)", "put": "0.95", "meaning": "区间置信。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("区间图", "组均值±区间。", "比位置。"), ("置信区间", "不确定度。", "≠规格限。"), ("腔3", "行31–45偏低。", "埋点。")),
            [{"row": 31, "what": "腔3均值偏低", "expect": "区间位置差；勿写过程合格。"}],
            "图形 → 区间图",
            "响应=`厚度_um`；类别=`腔号`",
        ),
        (
            "bar_chart",
            "条形图",
            "graph_bar_category",
            {"category": "缺陷类别"},
            [
                {"field": "类别 (`category`)", "put": "缺陷类别", "meaning": "分类轴。"},
                {"field": "数值变量 (`value`)", "put": "留空", "meaning": "可空=计频。"},
                {"field": "权重/计数 (`weight`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("条形图", "类别频数/数值。", "探索帕累托前奏。"), ("虚焊", "最高条。", "配方埋点。"), ("禁止句", "勿过程合格。", "图≠放行。")),
            [{"row": 1, "what": "虚焊频次最高（全表配方）", "expect": "最高条为虚焊；禁止过程合格。"}],
            "图形 → 条形图",
            "类别=`缺陷类别`",
        ),
        (
            "pie_plot",
            "饼图",
            "graph_pie_category",
            {"category": "缺陷类别"},
            [
                {"field": "分类变量 (`category`)", "put": "缺陷类别", "meaning": "扇区。"},
                {"field": "权重 (`weight`)", "put": "留空", "meaning": "可空=计频。"},
                {"field": "Other 合并阈值 (`other_threshold`)", "put": "留空或95", "meaning": "可选合并。"},
            ],
            _g3(("饼图", "份额扇区。", "类别占比。"), ("最大扇区", "虚焊。", "埋点。"), ("禁止句", "勿过程合格。", "探索。")),
            [{"row": 1, "what": "虚焊份额最大", "expect": "最大扇区；禁止过程合格。"}],
            "图形 → 饼图",
            "分类=`缺陷类别`",
        ),
        (
            "mosaic_plot",
            "马赛克图",
            "graph_mosaic_two_cat",
            {"categories": "班次,缺陷类别"},
            [
                {"field": "分类列（2～3） (`categories`)", "put": "班次,缺陷类别", "meaning": "多选两列。"},
            ],
            _g3(("马赛克图", "列联面积。", "看关联。"), ("晚班虚焊", "格更大。", "埋点。"), ("≠卡方证明", "图是探索。", "推断另课。")),
            [{"row": 1, "what": "晚班×虚焊占比抬高", "expect": "对应格面积大；勿写过程合格。"}],
            "图形 → 马赛克图",
            "分类列多选 `班次` 与 `缺陷类别`",
        ),
        (
            "scatter_plot",
            "散点图",
            "graph_scatter_xy",
            {"x_variable": "温度_C"},
            [
                {"field": "X 变量 (`x_variable`)", "put": "温度_C", "meaning": "X。"},
                {"field": "Y 变量 (`y_variable`)", "put": "偏移_um", "meaning": "至少一个 Y。"},
                {"field": "分组 (`by`)", "put": "留空", "meaning": "可选着色。"},
                {"field": "分面 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "标签 (`label`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("散点图", "XY 关系。", "看趋势/离群。"), ("正相关", "温度↑偏移↑。", "主趋势。"), ("离群", "行42。", "埋点。")),
            [{"row": 42, "what": "偏移尖峰离群", "expect": "点远离主云；禁止过程合格。"}],
            "图形 → 散点图",
            "X=`温度_C`；Y=`偏移_um`",
        ),
        (
            "hexbin_plot",
            "Hexbin",
            "graph_hexbin_xy",
            {"x_variable": "X坐标"},
            [
                {"field": "X (`x_variable`)", "put": "X坐标", "meaning": "X。"},
                {"field": "Y (`y_variable`)", "put": "Y坐标", "meaning": "Y。"},
                {"field": "分面 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "分箱数 (`bins`)", "put": "0", "meaning": "0=自动。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("Hexbin", "二维分箱密度。", "大数据散点替代。"), ("中心核", "行1–100。", "深色。"), ("外缘", "行101–120。", "稀疏。")),
            [{"row": 1, "what": "中心密度核", "expect": "中心 hex 更深；禁止过程合格。"}],
            "图形 → Hexbin",
            "X=`X坐标`；Y=`Y坐标`",
        ),
        (
            "bubble_plot",
            "气泡图",
            "graph_bubble_xyz",
            {"x_variable": "X坐标"},
            [
                {"field": "X (`x_variable`)", "put": "X坐标", "meaning": "X。"},
                {"field": "Y (`y_variable`)", "put": "Y坐标", "meaning": "Y。"},
                {"field": "尺寸 (`size_variable`)", "put": "尺寸", "meaning": "气泡大小。"},
                {"field": "分组 (`by`)", "put": "留空", "meaning": "可选。"},
                {"field": "标签 (`label`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("气泡图", "第三维用尺寸。", "XYZ 编码。"), ("大气泡", "行30–36。", "埋点。"), ("禁止句", "勿过程合格。", "探索。")),
            [{"row": 30, "what": "尺寸显著更大", "expect": "大气泡可见；禁止过程合格。"}],
            "图形 → 气泡图",
            "X/Y/尺寸三列",
        ),
        (
            "marginal_plot",
            "边际图",
            "graph_marginal_xy",
            {"x_variable": "X值"},
            [
                {"field": "X (`x_variable`)", "put": "X值", "meaning": "X。"},
                {"field": "Y (`y_variable`)", "put": "Y值", "meaning": "Y。"},
                {"field": "分面 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("边际图", "散点+边缘分布。", "联合与边际。"), ("正相关", "主云斜向。", "埋点。"), ("禁止句", "勿已证明正态。", "边缘像≠证明。")),
            [{"row": 1, "what": "正相关云", "expect": "斜向点云+单峰边际；禁止已证明正态。"}],
            "图形 → 边际图",
            "X=`X值`；Y=`Y值`",
        ),
        (
            "contour_plot",
            "等值线图",
            "graph_contour_xy",
            {"x_variable": "X因子"},
            [
                {"field": "X (`x_variable`)", "put": "X因子", "meaning": "X。"},
                {"field": "Y (`y_variable`)", "put": "Y因子", "meaning": "Y。"},
                {"field": "Z (`z_variable`)", "put": "响应Z", "meaning": "高度。"},
                {"field": "分面 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "水平数 (`levels`)", "put": "目录默认/留空", "meaning": "等值线层数。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("等值线", "Z 的等高线。", "看峰谷。"), ("中心峰", "X≈0,Y≈0。", "埋点。"), ("禁止句", "勿过程合格。", "响应面探索。")),
            [{"row": 28, "what": "中心附近 Z 高", "expect": "中心等值峰；禁止过程合格。"}],
            "图形 → 等值线图",
            "X/Y/Z 三列",
        ),
        (
            "correlation_plot",
            "相关图",
            "graph_corr_matrix",
            {"variables": "变量A,变量B,变量C"},
            [
                {"field": "变量 (`variables`)", "put": "变量A,变量B,变量C", "meaning": "多选数值列。"},
                {"field": "分面 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "方法 (`method`)", "put": "目录默认", "meaning": "相关方法。"},
                {"field": "置信水平 (`confidence`)", "put": "0.95", "meaning": "可选。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("相关图", "矩阵色块。", "A-B 深。"), ("强相关", "A–B≈0.85。", "埋点。"), ("≠因果", "相关非因果。", "话术。")),
            [{"row": 1, "what": "A–B 强相关结构", "expect": "对应色块更深；禁止过程合格。"}],
            "图形 → 相关图",
            "多选三变量",
        ),
        (
            "heatmap_plot",
            "热图",
            "graph_heatmap_matrix",
            {"variables": "变量A,变量B,变量C"},
            [
                {"field": "相关变量 (`variables`)", "put": "变量A,变量B,变量C", "meaning": "多选。"},
                {"field": "X (`x_variable`)", "put": "留空", "meaning": "可选矩阵模式。"},
                {"field": "Y (`y_variable`)", "put": "留空", "meaning": "可选。"},
                {"field": "Z (`z_variable`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "方法 (`method`)", "put": "目录默认", "meaning": "聚合/相关。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("热图", "矩阵着色。", "看块结构。"), ("A-B块", "更亮/更深。", "埋点。"), ("禁止句", "勿过程合格。", "探索。")),
            [{"row": 1, "what": "A–B 高相关块", "expect": "热图对应块突出；禁止过程合格。"}],
            "图形 → 热图",
            "多选三变量",
        ),
        (
            "matrix_plot",
            "矩阵图",
            "graph_matrix_three",
            {"variables": "厚度_um,偏移_um,温度_C"},
            [
                {"field": "变量 (`variables`)", "put": "厚度_um,偏移_um,温度_C", "meaning": "≥2 列。"},
                {"field": "分组 (`by`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("矩阵图", "两两散点面板。", "看相关对。"), ("厚度-偏移", "正斜率。", "埋点。"), ("温度", "较独立。", "对照。")),
            [{"row": 1, "what": "厚度-偏移正斜率", "expect": "对应面板相关；禁止过程合格。"}],
            "图形 → 矩阵图",
            "多选三列",
        ),
        (
            "parallel_plot",
            "平行坐标图",
            "graph_parallel_multi",
            {"variables": "轴1,轴2,轴3,轴4"},
            [
                {"field": "变量 (`variables`)", "put": "轴1,轴2,轴3,轴4", "meaning": "多轴。"},
                {"field": "分组 (`by`)", "put": "留空", "meaning": "可选；也可目视备注。"},
                {"field": "分面 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("平行坐标", "多维折线。", "看簇。"), ("差品", "行21–30 后两轴低。", "埋点。"), ("禁止句", "勿过程合格。", "探索。")),
            [{"row": 21, "what": "差品后两轴偏低", "expect": "折线在轴3/4 下沉；禁止过程合格。"}],
            "图形 → 平行坐标图",
            "多选轴1–轴4",
        ),
        (
            "area_plot",
            "区域图",
            "graph_area_time",
            {"time": "周次"},
            [
                {"field": "顺序/时间 (`time`)", "put": "周次", "meaning": "横轴。"},
                {"field": "数值 (`value`)", "put": "产量", "meaning": "面积高度。"},
                {"field": "分面 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("区域图", "时间-数值填色。", "看台阶。"), ("后段台阶", "周≥25。", "行25起。"), ("禁止句", "勿过程合格。", "趋势探索。")),
            [{"row": 25, "what": "产量台阶抬高", "expect": "面积后段抬升；禁止过程合格。"}],
            "图形 → 区域图",
            "时间=`周次`；数值=`产量`",
        ),
        (
            "eda_4plot",
            "EDA 四图",
            "graph_eda4_series",
            {"variables": "厚度_um"},
            [
                {"field": "变量 (`variables`)", "put": "厚度_um", "meaning": "单列。"},
                {"field": "直方图组数 (`bins`)", "put": "留空", "meaning": "可选。"},
            ],
            _g3(("EDA 四图", "run/lag/hist/norm 面板。", "NIST 风格。"), ("中段漂移", "行25–36。", "埋点。"), ("禁止句", "勿已证明正态。", "诊断线索。")),
            [{"row": 25, "what": "中段均值抬高", "expect": "run/lag 非随机线索；禁止已证明正态。"}],
            "图形 → EDA 四图",
            "变量=`厚度_um`",
        ),
        (
            "pca",
            "主成分分析",
            "pca_three_var",
            {"variables": "指标X,指标Y,指标Z"},
            [
                {"field": "数值变量 (`variables`)", "put": "指标X,指标Y,指标Z", "meaning": "多选。"},
                {"field": "矩阵模式 (`mode`)", "put": "目录默认", "meaning": "pca_mode。"},
                {"field": "保留主成分数 (`components`)", "put": "0", "meaning": "0=全部。"},
            ],
            _g3(("PCA", "降维正交分量。", "PC1 主变异。"), ("共线", "三列同向。", "埋点。"), ("离群", "行35–40。", "得分图线索。")),
            [{"row": 35, "what": "第三方向离群", "expect": "得分/载荷异常线索；禁止过程合格。"}],
            "统计 → 主成分分析",
            "多选三指标；mode 用目录默认",
        ),
        (
            "doe_response",
            "DOE 响应分析",
            "doe_factorial_y",
            {"response": "响应Y"},
            [
                {"field": "响应列 (`response`)", "put": "响应Y", "meaning": "可选但本课必选以分析。"},
                {"field": "已导入因子列 (`factor_columns`)", "put": "温度,压力", "meaning": "多选因子列。"},
                {"field": "因子名 (`factors`)", "put": "温度,压力", "meaning": "与列对应。"},
                {"field": "低水平 (`low`)", "put": "-1,-1", "meaning": "编码低。"},
                {"field": "高水平 (`high`)", "put": "1,1", "meaning": "编码高。"},
                {"field": "部分析因 p (`fraction_p`)", "put": "0", "meaning": "全因子。"},
                {"field": "中心点 (`centers`)", "put": "0", "meaning": "本表无中心点。"},
                {"field": "区组 (`blocks`)", "put": "1", "meaning": "默认。"},
            ],
            _g3(("析因响应", "估主效应/交互。", "温度主效应。"), ("编码±1", "设计单位。", "本表。"), ("禁止句", "勿过程合格。", "效应≠放行。")),
            [{"row": 1, "what": "温度=+1 响应抬高（全表结构）", "expect": "温度主效应明显；禁止过程合格。"}],
            "统计 → DOE 响应分析",
            "响应=`响应Y`；因子列=`温度`,`压力`",
        ),
        (
            "response_optimization",
            "DOE 响应优化",
            "doe_opt_two_resp",
            {"response": "强度Y1,翘曲Y2"},
            [
                {"field": "响应列 (`response`)", "put": "强度Y1,翘曲Y2", "meaning": "多响应。"},
                {"field": "因子列 (`factor_columns`)", "put": "温度,压力", "meaning": "因子。"},
                {"field": "优化目标 (`goal`)", "put": "目录默认", "meaning": "doe_optimization_goal。"},
                {"field": "下限 (`lower`)", "put": "可空", "meaning": "默认观测最小。"},
                {"field": "上限 (`upper`)", "put": "可空", "meaning": "默认观测最大。"},
                {"field": "目标值 (`target`)", "put": "可空", "meaning": "目标型时填。"},
                {"field": "权重 (`weight`)", "put": "1", "meaning": "默认。"},
                {"field": "置信水平 (`confidence`)", "put": "0.95", "meaning": "置信。"},
            ],
            _g3(("响应优化", "多响应折中。", "期望面。"), ("冲突", "Y1↑时 Y2 略变。", "埋点。"), ("禁止句", "勿过程合格。", "优化≠放行。")),
            [{"row": 1, "what": "温度高 Y1 升、Y2 略降", "expect": "折中区；禁止过程合格。"}],
            "统计 → DOE 响应优化",
            "两响应+两因子",
        ),
        (
            "simplex_design_plot",
            "混料三角图",
            "mix_simplex_3",
            {"components": "组分A,组分B,组分C"},
            [
                {"field": "分量列（3～4） (`components`)", "put": "组分A,组分B,组分C", "meaning": "和≈1。"},
            ],
            _g3(("单纯形", "混料约束空间。", "三角形。"), ("顶点", "行1–3。", "单组分。"), ("中心", "行10。", "各≈1/3。")),
            [{"row": 1, "what": "顶点/边/内部共30行（行1–3顶点）", "expect": "点落在三角形；禁止过程合格。"}],
            "图形 → 混料三角图",
            "三组分列",
        ),
        (
            "reliability_warranty",
            "保修摘要",
            "rel_warranty_counts",
            {"exposure_col": "暴露量"},
            [
                {"field": "暴露量列 (`exposure_col`)", "put": "暴露量", "meaning": "列求和优先标量。"},
                {"field": "保修窗口 T_w (`warranty_time`)", "put": "1000", "meaning": "窗口。"},
                {"field": "时间单位 (`time_unit`)", "put": "hours", "meaning": "单位标签。"},
                {"field": "暴露量标量 (`exposure`)", "put": "有列时可次要", "meaning": "无列时用。"},
                {"field": "R(T_w) (`reliability`)", "put": "0.98", "meaning": "可靠度输入。"},
                {"field": "模型标签 (`model`)", "put": "weibull", "meaning": "必填标签。"},
                {"field": "观察失效数 (`observed_failures`)", "put": "例如 2", "meaning": "计数。"},
                {"field": "删失数 (`censored`)", "put": "0", "meaning": "计数。"},
                {"field": "有效观测数 (`valid`)", "put": "可对照行数", "meaning": "计数。"},
                {"field": "是否预测口径 (`prediction`)", "put": "false", "meaning": "true_false_flag。"},
            ],
            _g3(("保修摘要", "窗口内风险摘要。", "输入驱动。"), ("暴露列", "优先于标量。", "本课。"), ("禁止句", "勿过程合格。", "摘要≠放行。")),
            [{"row": 1, "what": "暴露量列可求和", "expect": "用列暴露而非只靠标量；禁止过程合格。"}],
            "统计 → 保修摘要",
            "暴露列=`暴露量`；填 T_w / R / model",
        ),
        (
            "time_series_plot",
            "时间序列图",
            "ts_weekly_yield_series",
            {"time": "周次"},
            [
                {"field": "时间变量 (`time`)", "put": "周次", "meaning": "横轴。"},
                {"field": "数值变量 (`value`)", "put": "产量", "meaning": "至少一个 Y。"},
                {"field": "分组 (`by`)", "put": "留空", "meaning": "可选。"},
                {"field": "分面 (`facet`)", "put": "留空", "meaning": "可选。"},
                {"field": "连接缺失间隔 (`connect_missing`)", "put": "目录默认", "meaning": "yes_no_choice。"},
                {"field": "分面最大面板数 (`facet_max_panels`)", "put": "6", "meaning": "面板上限。"},
            ],
            _g3(("时间序列图", "按时间描值。", "看台阶。"), ("后段抬升", "周≥30。", "行30。"), ("不共享", "与分解/平滑分表。", "锁表。")),
            [{"row": 30, "what": "产量后段抬高", "expect": "曲线后段上移；禁止过程合格。"}],
            "图形 → 时间序列图",
            "时间=`周次`；数值=`产量`",
        ),
        (
            "time_series_decomposition",
            "时间序列分解",
            "ts_decomp_seasonal",
            {"time": "周次"},
            [
                {"field": "时间列 (`time`)", "put": "周次", "meaning": "可选但本课填。"},
                {"field": "时间序列值 (`value`)", "put": "产量", "meaning": "必选 Y。"},
                {"field": "季节周期 (`period`)", "put": "4", "meaning": "本课季节=4。"},
                {"field": "模型 (`model`)", "put": "目录默认", "meaning": "decomposition_model。"},
                {"field": "预测期数 (`periods`)", "put": "4", "meaning": "预测。"},
            ],
            _g3(("分解", "趋势+季节+残差。", "period=4。"), ("季节", "周期分量。", "埋点。"), ("禁止句", "勿过程合格。", "分解≠放行。")),
            [{"row": 1, "what": "季节周期4+上趋势", "expect": "季节分量周期≈4；禁止过程合格。"}],
            "统计 → 时间序列分解",
            "时间=`周次`；值=`产量`；period=4",
        ),
        (
            "time_series_smoothing",
            "时间序列平滑",
            "ts_smooth_weekly",
            {"variables": "产量"},
            [
                {"field": "时间序列 (`variables`)", "put": "产量", "meaning": "单列序列。"},
                {"field": "方法 (`method`)", "put": "目录默认", "meaning": "smoothing_method。"},
                {"field": "Alpha (`alpha`)", "put": "0.2", "meaning": "平滑系数。"},
                {"field": "Gamma (`gamma`)", "put": "0.2", "meaning": "双指数用。"},
                {"field": "预测期数 (`periods`)", "put": "1", "meaning": "预测。"},
            ],
            _g3(("指数平滑", "加权近期。", "轨迹更顺。"), ("噪声", "原始抖动。", "本表。"), ("不共享", "独立 ts_smooth_weekly。", "锁表。")),
            [{"row": 1, "what": "噪声上升趋势", "expect": "平滑曲线更顺；禁止过程合格。"}],
            "统计 → 时间序列平滑",
            "序列=`产量`；alpha≈0.2",
        ),
    ]

    for cid, title, ds, fill, detail, gloss, buried, menu, fill_hint in specs:
        out[cid] = _data_overlay(
            title=title,
            used_for=f"用专用集 `{ds}` 练习「{title}」。",
            not_for="替代推断/放行结论；禁止过程合格 / 已证明正态 / 必须停线。",
            scenario=f"导入 `demo_{ds}`。{fill_hint}。",
            related=RELATED_BY_ID.get(cid, ["descriptive"]),
            dialog_fill=fill,
            click_steps=[
                f"导入 `demo_{ds}`。",
                f"菜单：{menu}。",
                f"按角色映射：{fill_hint}。",
                "对照埋点读图；禁止过程合格 / 已证明正态 / 必须停线。",
            ],
            detail=detail,
            glossary=gloss,
            buried=buried,
            output_guide=[{"name": title, "meaning": "对照埋点；禁止过程合格；禁止已证明正态；禁止必须停线。"}],
            mission=f"能独立导入 `demo_{ds}` 完成「{title}」并拒绝禁止句。",
            prereq=[
                {"q": "dataset？", "good": ds, "bad": "旧10表"},
                {"q": "过程合格？", "good": "禁止", "bad": "可以"},
                {"q": "菜单？", "good": menu.split("→")[0].strip(), "bad": "随便猜"},
            ],
            self_explain=[
                {"after": "导入", "prompt": "本课为何专用表？"},
                {"after": "读图", "prompt": "埋点对应哪一行？"},
            ],
            fade_ds=ds,
            retrieval=["dataset_id？", "埋点行？", "禁止句"],
            misc=[{"wrong": "图画完=过程合格", "right": "禁止过程合格。"}],
        )

    return out


def build_empty_overlays(wave4_ids: list[str], data_ids: set[str]) -> dict[str, dict]:
    meta = json.loads(META.read_text(encoding="utf-8"))
    by = {e["id"]: e for e in meta["entries"]}
    out: dict[str, dict] = {}
    for cid in wave4_ids:
        if cid in data_ids:
            continue
        e = by.get(cid, {})
        cmd = e.get("command") or {}
        help_info = e.get("help") or {}
        title = help_info.get("title") or cmd.get("menu_label") or cid
        menu = cmd.get("menu_path") or help_info.get("menu_path") or "（见帮助）"
        status = help_info.get("implemented_status") or "formula_reference"
        roles = cmd.get("roles") or []
        detail = []
        for r in roles:
            detail.append(
                {
                    "field": f"{r.get('label', r['id'])} (`{r['id']}`)",
                    "put": "（无演示表）对应列",
                    "meaning": "对照 analysis_commands；本波 dataset 空。",
                }
            )
        if not detail:
            detail.append(
                {
                    "field": "（无列角色 / 设计生成 / 帮助课）",
                    "put": "无需导入",
                    "meaning": "requires_data 可能为 false，或仅公式参考。",
                }
            )
        glossary = glossary_for(cid, {"title": title})
        related = []
        if "doe" in cid or "taguchi" in cid or "mixture" in cid or "split_plot" in cid:
            related = ["doe_response", "response_optimization"]
        elif "reliab" in cid or "cox" in cid or "weib" in cid or "life_" in cid or "nhpp" in cid:
            related = ["reliability_warranty"]
        elif "cluster" in cid or "forest" in cid or "cart" in cid or "discriminant" in cid:
            related = ["pca"]
        elif "time" in cid or "arima" in cid or "acf" in cid or "adf" in cid or "seasonal" in cid or "trend" in cid:
            related = ["time_series_plot", "time_series_decomposition"]
        elif cid in ("database_import", "report_templates", "graph_gallery", "distribution_calculator"):
            related = ["histogram", "descriptive"]
        else:
            related = ["histogram", "scatter_plot"]

        menu_full = f"{menu} → {title}" if menu and "→" not in menu else (menu or title)
        out[cid] = _formula_skeleton(
            title=title,
            menu=menu_full,
            used_for=f"了解「{title}」的适用边界与对话框字段（{status}）。",
            not_for="本波不提供演示表；勿把旧共享表硬挂过来。禁止过程合格。",
            scenario=f"锁表 dataset 空。若菜单可用走 {menu_full}；否则只读公式帮助。",
            related=related,
            glossary=glossary,
            detail=detail,
            mission=f"能默写「{title}」真实字段，并说明本课为何空 dataset。",
            prereq=[
                {"q": "本波有演示表？", "good": "无", "bad": "有"},
                {"q": "可挂旧10表？", "good": "禁止", "bad": "可以"},
                {"q": "过程合格？", "good": "禁止", "bad": "可以"},
            ],
            self_explain=[
                {"after": "读锁表", "prompt": "为何本课 dataset 空？"},
                {"after": "对照帮助", "prompt": "菜单不可用时学什么？"},
            ],
            retrieval=["dataset？", "menu_path？", "禁止句"],
            misc=[{"wrong": "空表=漏做", "right": "锁表诚实为空；公式/编排课允许。"}],
            extra_mistakes=["为本课伪造宽表冒充已实现"],
        )
    return out


def build_overlays() -> dict[str, dict]:
    inventory = json.loads(INVENTORY.read_text(encoding="utf-8"))
    wave4_ids = list(inventory["waves"]["4"])
    data = build_data_overlays()
    empty = build_empty_overlays(wave4_ids, set(data))
    out = {**empty, **data}
    missing = [cid for cid in wave4_ids if cid not in out]
    if missing:
        raise RuntimeError(f"Wave-4 overlay missing ids: {missing}")
    extra = sorted(set(out) - set(wave4_ids))
    if extra:
        raise RuntimeError(f"Wave-4 overlay extra ids: {extra}")
    return out


def write_overlays() -> None:
    OVERLAY_DIR.mkdir(parents=True, exist_ok=True)
    overlays = build_overlays()
    assert len(overlays) == 83, len(overlays)
    overlays = {cid: polish_overlay(cid, payload) for cid, payload in overlays.items()}
    for cid, payload in overlays.items():
        path = OVERLAY_DIR / f"{cid}.json"
        path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    # sanity: glossary>=3, dialog_fill object, seven+, skill_mission
    for cid, p in overlays.items():
        assert isinstance(p.get("dialog_fill"), dict), cid
        assert len(p.get("glossary") or []) >= 3, cid
        assert p.get("skill_mission"), cid
        for k in ("prereq_quiz", "self_explain", "fade_levels", "retrieval_quiz", "misconceptions"):
            assert p.get(k), (cid, k)
        if p.get("buried_signals"):
            assert isinstance(p["buried_signals"], list)
    print(f"Wrote {len(overlays)} Wave-4 overlays to {OVERLAY_DIR}")


if __name__ == "__main__":
    write_overlays()
