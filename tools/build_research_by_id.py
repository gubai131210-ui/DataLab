#!/usr/bin/env python3
"""Build per-id research payloads for all learning center entries."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ACCESSED = "2026-09-03"

DATASET_SCENARIOS = {
    "smt_paste_height": "SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`",
    "two_line_thickness": "光学膜：`膜厚_um`、`产线`（A/B 线）",
    "paired_rework": "装配返工：`返工前扭矩_Nm`、`返工后扭矩_Nm`、`工件号`",
    "anova_cavity": "注塑：`模腔尺寸_mm`、`模腔`（1/2/3 穴）",
    "corr_temp_offset": "回流焊：`炉温_℃`、`焊点偏移_um`",
    "attribute_defect": "装配：`班次`、`不良数`、`检验数`",
    "gage_rr_balance": "量具 MSA：`零件号`、`操作员`、`测量值_mm`",
    "doe_factorial_demo": "DOE：`温度_℃`、`压力_MPa`、`响应_良率`",
    "reliability_cycles": "可靠性：`循环次数`、`失效状态`、`应力_V`",
    "ts_weekly_yield": "周度：`周次`、`良率_pct`",
}

# Per-id tailoring: used_for, not_for, typical_sample_size, common_mistakes, dataset_hint, sources keys
ID_RESEARCH: dict[str, dict] = {}

def add(cid: str, **kwargs) -> None:
    ID_RESEARCH[cid] = kwargs

# --- 统计 / 基础 ---
for cid, used, notf, n, ds in [
    ("descriptive", "汇总测量值中心、散布与形状，可按产线/模腔分组。", "不能判断稳定性或规格符合性。", "n≥30；分组每组≥5", "smt_paste_height"),
    ("normality_test", "评估数据与正态假设偏离，为参数/非参数选型提供证据。", "未拒绝≠证明正态；小样本功效低。", "n≥20", "smt_paste_height"),
    ("outlier_test", "识别可能来自特殊原因的极端观测。", "统计离群≠必须删除；需工程确认。", "n≥20", "smt_paste_height"),
    ("correlation", "量化两列或多列数值的线性/单调关联强度。", "相关≠因果；不能预测规格合格。", "n≥30", "corr_temp_offset"),
    ("one_sample_t", "检验样本均值是否偏离假设目标值。", "不能证明过程长期受控。", "n≥15", "smt_paste_height"),
    ("one_sample_z", "已知 σ 时检验均值；或大样本近似。", "σ 估计错误则结论无效。", "n≥30", "smt_paste_height"),
    ("two_sample_t", "比较两独立组均值差异。", "组间非独立时不可用；不等方差选 Welch。", "每组 n≥15", "two_line_thickness"),
    ("paired_t", "比较同一单元处理前后均值差。", "配对必须一一对应。", "配对 n≥15", "paired_rework"),
    ("one_proportion", "估计/检验单一事件比例。", "不能代表多阶段过程整体不良。", "事件数≥5", "attribute_defect"),
    ("two_proportions", "比较两组比例差异。", "样本独立；小计数用精确法。", "每组 n≥30", "attribute_defect"),
    ("one_poisson_rate", "估计单位长度/面积缺陷发生率。", "需明确观测长度/面积。", "缺陷总数≥10", "attribute_defect"),
    ("two_poisson_rate", "比较两组 Poisson 发生率。", "长度单位需一致。", "每组缺陷≥5", "attribute_defect"),
    ("variance_test", "比较两组或多样本方差是否齐性。", "对非正态敏感；小样本功效低。", "每组 n≥10", "two_line_thickness"),
    ("one_way_anova", "比较≥3 组均值是否全等。", "显著后需事后比较；残差需检查。", "每水平 n≥5", "anova_cavity"),
    ("two_factor_anova", "评估两因子主效应与交互。", "不能外推未试验水平。", "每单元≥3 重复", "doe_factorial_demo"),
    ("regression", "建立预测因子与连续响应的关系。", "外推风险；残差诊断必备。", "n≥10×参数", "corr_temp_offset"),
    ("logistic_regression", "建模二分类响应与因子关系。", "不能证明因果；需足够事件数。", "事件≥10", "attribute_defect"),
    ("chi_square", "检验分类变量独立性。", "期望频数过小需合并类别。", "总 n≥50", "attribute_defect"),
    ("cross_tabulation", "列联表汇总与比例展示。", "仅汇总不检验时需另做卡方。", "视列联表大小", "attribute_defect"),
    ("chi_square_gof", "检验分类分布是否符合理论比例。", "不能证明数据生成机制。", "每类期望≥5", "attribute_defect"),
    ("mann_whitney", "两独立组非参数位置比较。", "检验分布位置而非仅中位数（一般解释）。", "每组 n≥10", "two_line_thickness"),
    ("wilcoxon_signed_rank", "配对非参数差值检验。", "差值分布需近似对称。", "n≥15", "paired_rework"),
    ("sign_test", "配对符号检验，最稳健。", "功效低于 Wilcoxon。", "n≥15", "paired_rework"),
    ("kruskal_wallis", "多组非参数位置比较。", "显著后需事后非参数比较。", "每组 n≥5", "anova_cavity"),
    ("friedman", "重复测量/区组非参数。", "区组内配对完整。", "区组≥10", "anova_cavity"),
    ("mood_median", "多组中位数差异检验。", "对离群较稳健。", "每组 n≥10", "anova_cavity"),
    ("cochran_q", "相关样本多组二分类比例。", "仅适用二元响应。", "k≥3 处理", "attribute_defect"),
    ("mcnemar", "配对二分类变化检验。", "仅 2×2 配对表。", "discordant≥10", "paired_rework"),
    ("fisher_exact", "小样本 2×2 精确检验。", "大表计算慢。", "小计数", "attribute_defect"),
    ("runs_test", "检验序列随机性。", "不能定位特殊原因来源。", "n≥25", "ts_weekly_yield"),
    ("anom", "识别哪些组均值偏离总均值（ANOM）。", "需正态/方差假设；不等 n 谨慎。", "每组 n≥5", "anova_cavity"),
    ("anom_attribute", "属性数据 ANOM。", "小计数需合并。", "每组 n≥20", "attribute_defect"),
    ("poisson_gof", "检验计数是否符合 Poisson。", "过度离散需负二项等。", "总计数≥20", "attribute_defect"),
]:
    add(cid, used_for=used, not_for=notf, typical_sample_size=n, dataset_hint=ds,
        common_mistakes=["把 p 值当成工程决策唯一依据。", "忽略测量系统噪声。"],
        sources=["nist_eda", "montgomery_spc"])

# Equivalence family
for cid in ["one_sample_equivalence", "two_sample_equivalence", "two_sample_equivalence_ratio",
            "paired_equivalence", "one_proportion_equivalence", "two_proportion_equivalence"]:
    add(cid,
        used_for="证明效应落在事前等价区间内（生物等效/工艺窗口）。",
        not_for="不能证明完全无差异；等价限需法规/工程协议。",
        typical_sample_size="功效驱动，常 n≥20/组",
        dataset_hint="paired_rework" if "paired" in cid else "two_line_thickness",
        common_mistakes=["等价限事后挑选。", "忽略方差估计方法。"],
        sources=["nist_eda", "montgomery_spc"])

# Control charts
CC = [
    ("imr", "单件慢节拍个体值与移动极差监控。", "smt_paste_height"),
    ("xbar_r", "子组 2–10 的均值与极差监控。", "anova_cavity"),
    ("xbar_s", "子组较大时用标准差监控散布。", "anova_cavity"),
    ("imr_rs", "I-MR-R/S 组合监控。", "smt_paste_height"),
    ("p_chart", "不合格品率监控（变样本量）。", "attribute_defect"),
    ("np_chart", "不合格品数监控（固定样本量）。", "attribute_defect"),
    ("c_chart", "固定检验面积缺陷数监控。", "attribute_defect"),
    ("u_chart", "单位缺陷数监控（变检验单位）。", "attribute_defect"),
    ("laney_p_chart", "过度离散时的 Laney P 图。", "attribute_defect"),
    ("laney_u_chart", "过度离散时的 Laney U 图。", "attribute_defect"),
    ("ewma", "对微小漂移敏感的 EWMA。", "smt_paste_height"),
    ("cusum", "累积和检测小偏移。", "smt_paste_height"),
    ("zone_chart", "区域规则辅助识别模式。", "smt_paste_height"),
    ("z_mr", "标准化个体与 MR 监控。", "smt_paste_height"),
    ("moving_average", "移动平均平滑监控。", "ts_weekly_yield"),
    ("g_chart", "几何分布间隔/良率事件监控。", "attribute_defect"),
    ("t_chart", "时间间隔 T 图。", "reliability_cycles"),
    ("hotelling_t2", "多变量 T² 监控。", "corr_temp_offset"),
    ("mewma", "多变量 EWMA。", "corr_temp_offset"),
    ("generalized_variance", "多变量广义方差监控。", "corr_temp_offset"),
]
for cid, used, ds in CC:
    add(cid,
        used_for=used,
        not_for="不能证明能力合格或产品可放行；不替代抽样验收。",
        typical_sample_size="≥20 子组/点",
        dataset_hint=ds,
        chart_note="看超出控制限、趋势、周期、游程；模式识别不替代假设检验。",
        common_mistakes=["未按时间排序。", "控制限内就当合格。"],
        sources=["nist_spc", "aiag_spc", "wheeler"])

add("special_cause_rules",
    used_for="理解 Western Electric/Nelson 规则如何标记可疑模式。",
    not_for="触发≠必须停线；需工程调查。",
    typical_sample_size="依附控制图点数≥20",
    dataset_hint="smt_paste_height",
    chart_note="规则提示特殊原因线索，不替代假设检验。",
    common_mistakes=["规则过多致假警报。", "触发后不调查根因。"],
    sources=["nist_spc", "aiag_spc"])

# Capability / MSA
CAP = [
    ("capability", "估计 Cp/Cpk 等相对规格的能力指数。", "smt_paste_height"),
    ("nonnormal_capability", "非正态数据能力（如 Johnson/百分位）。", "smt_paste_height"),
    ("between_within_capability", "分解组间/组内变异的能力。", "anova_cavity"),
    ("batch_capability", "批次间能力评估。", "two_line_thickness"),
    ("nonparametric_capability", "非参数百分位能力。", "smt_paste_height"),
    ("binomial_capability", "二项不良率能力。", "attribute_defect"),
    ("poisson_capability", "Poisson 缺陷率能力。", "attribute_defect"),
    ("capability_sixpack", "能力六合一图形诊断。", "smt_paste_height"),
    ("box_cox", "Box-Cox 变换辅助正态化。", "smt_paste_height"),
    ("distribution_identification", "拟合候选分布识别。", "smt_paste_height"),
    ("tolerance_intervals", "估计覆盖比例的总体区间。", "smt_paste_height"),
]
for cid, used, ds in CAP:
    add(cid,
        used_for=used,
        not_for="失控或未代表样本时指数无预测意义；不写过程合格。",
        typical_sample_size="≥30 独立观测",
        dataset_hint=ds,
        common_mistakes=["未先验证受控。", "规格限设错。"],
        sources=["minitab_cap", "aiag_spc"])

MSA = [
    ("gage_rr", "交叉量具 R&R。", "gage_rr_balance"),
    ("emp_crossed", "展开交叉 Gage R&R。", "gage_rr_balance"),
    ("expanded_gage_rr", "展开型 Gage R&R。", "gage_rr_balance"),
    ("expanded_gage_unbalanced", "非平衡展开 Gage。", "gage_rr_balance"),
    ("nested_gage_rr", "嵌套 Gage R&R。", "gage_rr_balance"),
    ("msa_type1", "Type 1 偏倚与重复性。", "gage_rr_balance"),
    ("attribute_agreement", "属性一致性/agreement。", "attribute_defect"),
]
for cid, _, ds in MSA:
    add(cid,
        used_for="评估测量系统变异是否掩盖产品/过程差异。",
        not_for="不能证明量具通过或产品合格。",
        typical_sample_size="AIAG 10×3×2/3",
        dataset_hint=ds,
        common_mistakes=["零件未覆盖过程范围。", "操作员培训不一致。"],
        sources=["aiag_msa", "minitab_gage"])

# DOE family
DOE_IDS = [
    "doe_factorial", "doe_response", "rsm_response", "response_optimization",
    "doe_plackett_burman", "doe_ccd", "doe_bbd", "doe_d_optimal",
    "taguchi_orthogonal_design", "taguchi_analyze", "mixture_design",
    "mixture_extreme_vertices_design", "mixture_analyze", "mixture_process_variable",
    "binary_response_doe", "binary_doe_probit", "split_plot_design", "split_plot_analyze",
    "definitive_screening_design", "analyze_definitive_screening",
    "glm_two_way", "glm_three_factor", "analyze_variability",
]
for cid in DOE_IDS:
    add(cid,
        used_for="结构化试验筛选/优化因子与响应关系。",
        not_for="不能替代量产 SPC；外推需谨慎。",
        typical_sample_size="2^k 或 RSM 3–5 水平",
        dataset_hint="doe_factorial_demo",
        common_mistakes=["不随机化。", "因子水平不现实。"],
        sources=["montgomery_doe", "minitab_doe"])

# Graphics
GRAPH_IDS = [
    "histogram", "boxplot", "dotplot", "pareto", "run_chart", "cause_and_effect",
    "density_plot", "hexbin_plot", "violin_plot", "bar_chart", "scatter_plot",
    "graph_gallery", "interval_plot", "correlation_plot", "bubble_plot",
    "simplex_design_plot", "mosaic_plot", "chi_square_mosaic_link",
    "probability_plot", "ecdf_plot", "matrix_plot", "marginal_plot",
    "parallel_plot", "heatmap_plot", "time_series_plot", "area_plot",
    "contour_plot", "pie_plot", "eda_4plot",
]
GRAPH_DS = {
    "histogram": "smt_paste_height", "boxplot": "two_line_thickness", "dotplot": "smt_paste_height",
    "pareto": "attribute_defect", "scatter_plot": "corr_temp_offset", "time_series_plot": "ts_weekly_yield",
    "contour_plot": "doe_factorial_demo", "simplex_design_plot": "doe_factorial_demo",
}
for cid in GRAPH_IDS:
    add(cid,
        used_for="可视化分布、关系或结构，支持 EDA 与沟通。",
        not_for="图形本身不提供显著性（需配套检验）。",
        typical_sample_size="n≥20",
        dataset_hint=GRAPH_DS.get(cid, "smt_paste_height"),
        chart_note="看分布形状、离群、关联模式；不替代假设检验。",
        common_mistakes=["过度解读偶然模式。", "坐标截断误导。"],
        sources=["nist_eda", "montgomery_spc"])

# Time series
TS = ["acf_pacf", "time_series_smoothing", "time_series_decomposition", "trend_analysis",
      "seasonal_forecasting", "arima", "adf_test", "ccf", "correlogram"]
for cid in TS:
    add(cid,
        used_for="分析时序趋势、季节、自相关或短期预测。",
        not_for="预测区间≠规格合格；结构突变需重拟合。",
        typical_sample_size="≥50 点识别季节",
        dataset_hint="ts_weekly_yield",
        common_mistakes=["非平稳直接回归。", "忽略异常点。"],
        sources=["nist_eda", "montgomery_spc"])

# Reliability
REL = ["reliability", "accelerated_life", "reliability_warranty", "reliability_test_plan",
       "weibayes", "nhpp_repairable", "probit_reliability", "life_data_regression",
       "life_data_lognormal", "km_interval", "cox_regression", "cox_counting_process",
       "fine_gray_regression"]
for cid in REL:
    add(cid,
        used_for="基于失效/删失数据分析寿命与应力效应。",
        not_for="不能写成产品已达标；外推需工程论证。",
        typical_sample_size="失效事件≥10",
        dataset_hint="reliability_cycles",
        common_mistakes=["忽略删失。", "混合失效模式。"],
        sources=["nist_reliability"])

# ML / advanced
ML = ["pca", "factor_analysis", "cluster_observations", "cluster_variables", "discriminant",
      "kmeans", "cart_tree", "random_forest", "isolation_forest", "ordinal_logistic",
      "nominal_logistic", "stepwise_regression", "best_subsets_regression",
      "orthogonal_regression", "nonlinear_regression", "pls_regression", "poisson_regression",
      "bootstrap_mean", "bootstrap_two_sample", "randomization_test",
      "simple_correspondence", "multiple_correspondence", "manova_one_way", "general_manova",
      "mixed_effects_reml"]
for cid in ML:
    add(cid,
        used_for="多变量降维、分类、聚类或重采样推断。",
        not_for="黑箱/探索结果不能替代 DOE 因果验证。",
        typical_sample_size="样本远大于特征数",
        dataset_hint="corr_temp_offset",
        common_mistakes=["数据泄漏。", "过拟合未交叉验证。"],
        sources=["nist_eda", "montgomery_doe"])

# Quality tools
QT = ["multi_vari", "variability_chart", "acceptance_sampling", "t_power", "distribution_calculator"]
for cid in QT:
    ds = "attribute_defect" if cid == "acceptance_sampling" else "smt_paste_height"
    add(cid,
        used_for="质量工具：多变异、抽样 OC、功效或分布计算器。",
        not_for="不能替代 SPC 或 MSA 证据。",
        typical_sample_size="视工具；计算器无需数据",
        dataset_hint=None if cid == "distribution_calculator" else ds,
        common_mistakes=["抽样计划与批量不匹配。", "功效分析假设 σ 错误。"],
        sources=["aiag_spc", "montgomery_spc"])

# Orchestration / help-only
for cid, used in [
    ("database_import", "外部数据库导入工作区的流程示意。"),
    ("report_templates", "报告模板与输出页编排参考。"),
]:
    add(cid,
        used_for=used,
        not_for="当前版本可能无独立菜单。",
        typical_sample_size="N/A",
        implemented_note="orchestration；菜单可能不存在。",
        common_mistakes=["把导入当分析完成。"],
        sources=["nist_eda"])

def main() -> None:
    meta = json.loads((ROOT / "tools/learning_data/id_metadata.json").read_text(encoding="utf-8"))
    out: dict[str, dict] = {}
    missing = []
    for e in meta["entries"]:
        cid = e["id"]
        if cid in ID_RESEARCH:
            r = dict(ID_RESEARCH[cid])
        else:
            missing.append(cid)
            r = {
                "used_for": "见 category 模板。",
                "not_for": "不能过度解读统计结果。",
                "typical_sample_size": "n≥30",
                "common_mistakes": ["忽略假设条件。"],
                "sources": ["nist_eda"],
            }
        ds = r.get("dataset_hint")
        if ds and ds in DATASET_SCENARIOS:
            r["manufacturing_scenario"] = DATASET_SCENARIOS[ds]
        out[cid] = r
    path = ROOT / "tools/learning_data/research_by_id.json"
    path.write_text(json.dumps(out, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Wrote {len(out)} entries, missing curated: {missing}")

if __name__ == "__main__":
    main()
