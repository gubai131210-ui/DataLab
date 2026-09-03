#!/usr/bin/env python3
"""Generate learning-center-dataset-mapping.md and dataset_mapping.json (Agent B)."""
from __future__ import annotations

import json
from collections import defaultdict
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

DATASETS: dict[str, dict] = {
    "smt_paste_height": {
        "title": "SMT 锡膏印刷高度",
        "industry": "electronics",
        "story": "某 SMT 线体印刷后 3D 锡膏检测，按时间顺序记录高度，用于描述统计、正态性、I-MR、能力、直方图等。",
        "row_count": 80,
        "columns": [
            {"index": 0, "name": "锡膏高度_um", "role_hint": "measurement", "unit": "μm", "description": "3D 锡膏高度"},
            {"index": 1, "name": "产线", "role_hint": "factor", "description": "产线 A/B"},
            {"index": 2, "name": "班次", "role_hint": "factor", "description": "早/中/晚班"},
            {"index": 3, "name": "检测时间", "role_hint": "time", "description": "ISO 时间戳"},
            {"index": 4, "name": "钢网编号", "role_hint": "factor", "description": "钢网 ID"},
        ],
    },
    "two_line_thickness": {
        "title": "两产线光学膜厚",
        "industry": "electronics",
        "story": "两条镀膜产线抽检膜厚，比较均值与方差、箱线图、双样本 t。",
        "row_count": 60,
        "columns": [
            {"index": 0, "name": "膜厚_um", "role_hint": "measurement", "unit": "μm"},
            {"index": 1, "name": "产线", "role_hint": "factor", "description": "A 线 / B 线"},
            {"index": 2, "name": "抽检批次", "role_hint": "factor"},
            {"index": 3, "name": "检测时间", "role_hint": "time"},
        ],
    },
    "paired_rework": {
        "title": "装配返工前后扭矩",
        "industry": "assembly",
        "story": "返工前后同一工件扭矩配对测量，用于 paired t / Wilcoxon / 符号检验。",
        "row_count": 40,
        "columns": [
            {"index": 0, "name": "返工前扭矩_Nm", "role_hint": "measurement"},
            {"index": 1, "name": "返工后扭矩_Nm", "role_hint": "measurement"},
            {"index": 2, "name": "工件号", "role_hint": "id"},
            {"index": 3, "name": "缺陷类型", "role_hint": "factor"},
        ],
    },
    "anova_cavity": {
        "title": "注塑三模腔尺寸",
        "industry": "molding",
        "story": "三穴模具尺寸抽检，单因素 ANOVA、ANOM、Kruskal-Wallis、箱线图。",
        "row_count": 90,
        "columns": [
            {"index": 0, "name": "模腔尺寸_mm", "role_hint": "measurement", "unit": "mm"},
            {"index": 1, "name": "模腔", "role_hint": "factor", "description": "穴 1/2/3"},
            {"index": 2, "name": "材料批次", "role_hint": "factor"},
            {"index": 3, "name": "机台号", "role_hint": "factor"},
        ],
    },
    "corr_temp_offset": {
        "title": "回流温度与焊点偏移",
        "industry": "electronics",
        "story": "回流焊炉温与 AOI 焊点偏移，用于相关、回归、散点图、多变量图。",
        "row_count": 55,
        "columns": [
            {"index": 0, "name": "炉温_℃", "role_hint": "measurement"},
            {"index": 1, "name": "焊点偏移_um", "role_hint": "measurement"},
            {"index": 2, "name": "链速_mm_min", "role_hint": "measurement"},
            {"index": 3, "name": "产品型号", "role_hint": "factor"},
            {"index": 4, "name": "轨道号", "role_hint": "factor"},
        ],
    },
    "attribute_defect": {
        "title": "装配班次不良计数",
        "industry": "assembly",
        "story": "各班次抽检不良与缺陷分类，用于 p/np/c/u 图、卡方、帕累托、比例检验。",
        "row_count": 48,
        "columns": [
            {"index": 0, "name": "班次", "role_hint": "factor"},
            {"index": 1, "name": "检验数", "role_hint": "trials"},
            {"index": 2, "name": "不良数", "role_hint": "events"},
            {"index": 3, "name": "缺陷数", "role_hint": "defects"},
            {"index": 4, "name": "缺陷类型", "role_hint": "category"},
            {"index": 5, "name": "检验面积_dm2", "role_hint": "length"},
            {"index": 6, "name": "产线", "role_hint": "factor"},
        ],
    },
    "gage_rr_balance": {
        "title": "三座标 MSA 交叉研究",
        "industry": "msa",
        "story": "10 零件 × 3 操作员 × 3 次重复测量，Gage R&R 系列教程。",
        "row_count": 90,
        "columns": [
            {"index": 0, "name": "零件号", "role_hint": "part"},
            {"index": 1, "name": "操作员", "role_hint": "operator"},
            {"index": 2, "name": "重复序号", "role_hint": "replicate"},
            {"index": 3, "name": "测量值_mm", "role_hint": "measurement", "unit": "mm"},
        ],
    },
    "doe_factorial_demo": {
        "title": "回流焊 2³ 析因试验",
        "industry": "electronics",
        "story": "温度×链速×氮气流量对焊点强度的全因子 DOE，含响应面与优化类命令。",
        "row_count": 24,
        "columns": [
            {"index": 0, "name": "温度_℃", "role_hint": "factor"},
            {"index": 1, "name": "链速_mm_min", "role_hint": "factor"},
            {"index": 2, "name": "氮气流量_L_min", "role_hint": "factor"},
            {"index": 3, "name": "响应_强度_MPa", "role_hint": "response"},
            {"index": 4, "name": "响应_虚焊率", "role_hint": "response"},
            {"index": 5, "name": "运行序号", "role_hint": "order"},
            {"index": 6, "name": "成分_A_pct", "role_hint": "mixture"},
            {"index": 7, "name": "成分_B_pct", "role_hint": "mixture"},
            {"index": 8, "name": "成分_C_pct", "role_hint": "mixture"},
        ],
    },
    "reliability_cycles": {
        "title": "电源模块寿命循环",
        "industry": "reliability",
        "story": "加速应力下循环至失效/删失，KM、Weibull、Cox、ALT 等可靠性教程。",
        "row_count": 45,
        "columns": [
            {"index": 0, "name": "循环次数", "role_hint": "time"},
            {"index": 1, "name": "失效状态", "role_hint": "event", "description": "0=删失 1=失效"},
            {"index": 2, "name": "应力_V", "role_hint": "stress"},
            {"index": 3, "name": "单元号", "role_hint": "id"},
            {"index": 4, "name": "失效模式", "role_hint": "category"},
            {"index": 5, "name": "事件时间_小时", "role_hint": "time"},
        ],
    },
    "ts_weekly_yield": {
        "title": "周度装配良率",
        "industry": "assembly",
        "story": "52 周良率序列，用于时序图、平滑、分解、ARIMA、ADF、ACF。",
        "row_count": 52,
        "columns": [
            {"index": 0, "name": "周次", "role_hint": "time"},
            {"index": 1, "name": "良率_pct", "role_hint": "measurement"},
            {"index": 2, "name": "产量_件", "role_hint": "measurement"},
            {"index": 3, "name": "年份", "role_hint": "factor"},
        ],
    },
}

# Per-command overrides: dataset_id (None = no import) and optional role_map overrides
COMMAND_OVERRIDES: dict[str, dict] = {}

ATTRIBUTE_CHARTS = {
    "p_chart", "np_chart", "c_chart", "u_chart", "laney_p_chart", "laney_u_chart",
    "g_chart", "t_chart", "binomial_capability", "poisson_capability",
    "anom_attribute", "one_proportion", "two_proportions", "one_proportion_equivalence",
    "two_proportion_equivalence", "chi_square", "cross_tabulation", "chi_square_gof",
    "fisher_exact", "mcnemar", "cochran_q", "attribute_agreement", "pareto",
    "bar_chart", "mosaic_plot", "chi_square_mosaic_link",
}

GAGE_IDS = {
    "gage_rr", "emp_crossed", "expanded_gage_rr", "nested_gage_rr", "msa_type1",
    "expanded_gage_unbalanced", "attribute_agreement",
}

DOE_ANALYZE = {
    "doe_response", "rsm_response", "response_optimization", "taguchi_analyze",
    "mixture_analyze", "mixture_process_variable", "split_plot_analyze",
    "analyze_definitive_screening", "analyze_variability", "glm_two_way", "glm_three_factor",
    "binary_response_doe", "binary_doe_probit",
}

DOE_DESIGN_NO_DATA = {
    "doe_factorial", "doe_plackett_burman", "doe_ccd", "doe_bbd", "doe_d_optimal",
    "taguchi_orthogonal_design", "mixture_design", "mixture_extreme_vertices_design",
    "split_plot_design", "definitive_screening_design",
}

RELIABILITY_IDS = {
    "reliability", "accelerated_life", "reliability_warranty", "weibayes",
    "nhpp_repairable", "probit_reliability", "life_data_regression", "life_data_lognormal",
    "km_interval", "cox_regression", "cox_counting_process", "fine_gray_regression",
}

TS_IDS = {
    "acf_pacf", "time_series_smoothing", "time_series_decomposition", "trend_analysis",
    "seasonal_forecasting", "arima", "adf_test", "ccf", "correlogram", "time_series_plot",
    "area_plot", "runs_test", "moving_average",
}

PAIRED_IDS = {
    "paired_t", "wilcoxon_signed_rank", "sign_test", "paired_equivalence", "mcnemar",
}

TWO_SAMPLE_IDS = {
    "two_sample_t", "mann_whitney", "variance_test", "two_sample_equivalence",
    "two_sample_equivalence_ratio", "bootstrap_two_sample", "randomization_test",
}

ANOVA_IDS = {
    "one_way_anova", "anom", "kruskal_wallis", "mood_median", "friedman",
    "manova_one_way", "general_manova",
}

CONTROL_CHART_NUMERIC = {
    "imr", "xbar_r", "xbar_s", "imr_rs", "ewma", "cusum", "zone_chart", "z_mr",
    "moving_average", "hotelling_t2", "mewma", "generalized_variance",
}

ROLE_COLUMN_POOL: dict[str, dict[str, str]] = {
    "smt_paste_height": {
        "variables": "锡膏高度_um", "measurement": "锡膏高度_um", "by": "班次",
        "time": "检测时间", "factor": "产线", "category": "钢网编号",
    },
    "two_line_thickness": {
        "variables": "膜厚_um", "measurement": "膜厚_um", "by": "产线", "factor": "产线",
        "time": "检测时间",
    },
    "paired_rework": {
        "variables": "返工前扭矩_Nm", "first": "返工前扭矩_Nm", "second": "返工后扭矩_Nm",
        "paired": "返工前扭矩_Nm", "paired_second": "返工后扭矩_Nm",
    },
    "anova_cavity": {
        "variables": "模腔尺寸_mm", "response": "模腔尺寸_mm", "factor": "模腔",
        "factor_columns": "模腔", "by": "模腔",
    },
    "corr_temp_offset": {
        "variables": "炉温_℃", "variable": "炉温_℃", "response": "焊点偏移_um",
        "predictors": "炉温_℃", "x": "炉温_℃", "y": "焊点偏移_um",
    },
    "attribute_defect": {
        "events": "不良数", "trials": "检验数", "defects": "缺陷数", "length": "检验面积_dm2",
        "category": "缺陷类型", "by": "班次", "factor": "班次", "variables": "不良数",
        "first_events": "不良数", "first_trials": "检验数",
        "second_events": "缺陷数", "second_trials": "检验面积_dm2",
    },
    "gage_rr_balance": {
        "parts": "零件号", "part": "零件号", "operators": "操作员", "operator": "操作员",
        "measurement": "测量值_mm", "variables": "测量值_mm", "replicate": "重复序号",
    },
    "doe_factorial_demo": {
        "response": "响应_强度_MPa", "variables": "响应_强度_MPa", "factor_columns": "温度_℃",
        "factors": "温度_℃", "mixture": "成分_A_pct",
    },
    "reliability_cycles": {
        "time": "循环次数", "failure": "失效状态", "event": "失效状态", "stress": "应力_V",
        "variables": "循环次数", "response": "循环次数",
    },
    "ts_weekly_yield": {
        "variables": "良率_pct", "time": "周次", "sequence": "周次", "response": "良率_pct",
    },
}

NO_DATA_IDS = DOE_DESIGN_NO_DATA | {
    "distribution_calculator", "t_power", "acceptance_sampling", "reliability_test_plan",
    "special_cause_rules", "database_import", "report_templates", "cause_and_effect",
}


def col_by_role(dataset_id: str, role_id: str, role_label: str = "") -> str | None:
    pool = ROLE_COLUMN_POOL.get(dataset_id, {})
    if role_id in pool:
        return pool[role_id]
    # heuristics
    hints = {c["name"]: c for c in DATASETS[dataset_id]["columns"]}
    rh = {c["role_hint"]: c["name"] for c in DATASETS[dataset_id]["columns"]}
    if role_id in ("variables", "measurement") and "measurement" in rh:
        return rh["measurement"]
    if role_id == "by" and "factor" in rh:
        return rh["factor"]
    if role_id == "time" and "time" in rh:
        return rh["time"]
    if role_id == "response" and "response" in rh:
        return rh["response"]
    if role_id in rh:
        return rh[role_id]
    # multi variables: return first measurement
    if role_id == "variables":
        for c in DATASETS[dataset_id]["columns"]:
            if c["role_hint"] == "measurement":
                return c["name"]
    return None


def pick_dataset(entry: dict) -> str | None:
    cid = entry["id"]
    if cid in COMMAND_OVERRIDES:
        ds = COMMAND_OVERRIDES[cid].get("dataset_id")
        if ds is not None or COMMAND_OVERRIDES[cid].get("no_data"):
            return ds
    if cid in NO_DATA_IDS:
        return None
    research = entry.get("_research", {})
    if research.get("dataset_hint"):
        return research["dataset_hint"]
    if cid in ATTRIBUTE_CHARTS:
        return "attribute_defect"
    if cid in GAGE_IDS:
        return "gage_rr_balance"
    if cid in DOE_ANALYZE:
        return "doe_factorial_demo"
    if cid in RELIABILITY_IDS:
        return "reliability_cycles"
    if cid in TS_IDS:
        return "ts_weekly_yield"
    if cid in PAIRED_IDS:
        return "paired_rework"
    if cid in TWO_SAMPLE_IDS:
        return "two_line_thickness"
    if cid in ANOVA_IDS:
        return "anova_cavity"
    if cid in CONTROL_CHART_NUMERIC:
        return "smt_paste_height"
    cmd = entry.get("command") or {}
    mp = cmd.get("menu_path", "")
    cat = (entry.get("help") or {}).get("category", "")
    if mp == "控制图":
        return "attribute_defect" if cid in {"p_chart", "np_chart", "c_chart", "u_chart"} else "smt_paste_height"
    if mp == "图形":
        if cid in {"scatter_plot", "correlation_plot", "bubble_plot", "regression", "contour_plot"}:
            return "corr_temp_offset"
        if cid in {"boxplot", "dotplot"}:
            return "two_line_thickness"
        if cid in {"time_series_plot", "run_chart"}:
            return "ts_weekly_yield"
        if cid in {"simplex_design_plot"}:
            return "doe_factorial_demo"
        return "smt_paste_height"
    if mp == "质量工具":
        if "能力" in cat or cid.startswith("capability") or "capability" in cid:
            return "smt_paste_height"
        if cid in {"multi_vari", "variability_chart"}:
            return "anova_cavity"
        return "smt_paste_height"
    if "回归" in cat or cid in {"regression", "logistic_regression", "stepwise_regression",
                                  "best_subsets_regression", "poisson_regression", "pls_regression",
                                  "nonlinear_regression", "orthogonal_regression"}:
        return "corr_temp_offset"
    if cid in {"pca", "factor_analysis", "cluster_observations", "cluster_variables",
               "discriminant", "kmeans", "cart_tree", "random_forest", "isolation_forest",
               "matrix_plot", "parallel_plot", "heatmap_plot", "correlogram"}:
        return "corr_temp_offset"
    if cid in {"descriptive", "normality_test", "outlier_test", "one_sample_t", "one_sample_z",
               "capability", "capability_sixpack", "box_cox", "distribution_identification",
               "tolerance_intervals", "nonnormal_capability", "between_within_capability",
               "batch_capability", "nonparametric_capability", "histogram", "probability_plot",
               "ecdf_plot", "density_plot", "eda_4plot"}:
        return "smt_paste_height"
    if cid in {"correlation", "one_way_anova", "two_factor_anova"}:
        return "anova_cavity" if "anova" in cid else "corr_temp_offset"
    if cid in {"one_poisson_rate", "two_poisson_rate", "poisson_gof", "poisson_regression"}:
        return "attribute_defect"
    if cid in {"equivalence" }:
        return "paired_rework"
    if "equivalence" in cid:
        return "paired_rework" if "paired" in cid else "two_line_thickness"
    if cid in {"bootstrap_mean", "randomization_test"}:
        return "smt_paste_height"
    if cid in {"mixed_effects_reml"}:
        return "gage_rr_balance"
    if cid in {"simple_correspondence", "multiple_correspondence"}:
        return "attribute_defect"
    # help-only orchestration
    if cid in {"database_import", "report_templates", "special_cause_rules"}:
        return None
    if cid == "reliability_warranty":
        return "reliability_cycles"
    return "smt_paste_height"


def build_role_map(entry: dict, dataset_id: str | None) -> dict[str, str]:
    if not dataset_id:
        return {}
    override = COMMAND_OVERRIDES.get(entry["id"], {}).get("role_map", {})
    if override:
        return dict(override)
    cmd = entry.get("command") or {}
    roles = cmd.get("roles") or []
    role_map: dict[str, str] = {}
    for r in roles:
        rid = r["id"]
        col = col_by_role(dataset_id, rid, r.get("label", ""))
        if col:
            role_map[rid] = col
    # paired two-column commands
    if entry["id"] in PAIRED_IDS and "variables" in [r["id"] for r in roles]:
        role_map["variables"] = "返工前扭矩_Nm"
        if dataset_id == "paired_rework":
            role_map["variables_second"] = "返工后扭矩_Nm"
    if entry["id"] == "two_sample_t" and dataset_id == "two_line_thickness":
        role_map["variables"] = "膜厚_um"
        role_map["by"] = "产线"
    return role_map


def main() -> None:
    meta = json.loads((ROOT / "tools/learning_data/id_metadata.json").read_text(encoding="utf-8"))
    research = json.loads((ROOT / "tools/learning_data/research_by_id.json").read_text(encoding="utf-8"))
    mappings = []
    serves: dict[str, list[str]] = defaultdict(list)
    for e in meta["entries"]:
        e = dict(e)
        e["_research"] = research.get(e["id"], {})
        ds = pick_dataset(e)
        role_map = build_role_map(e, ds)
        cmd = e.get("command") or {}
        help_info = e.get("help") or {}
        item = {
            "command_id": e["id"],
            "title": help_info.get("title") or cmd.get("menu_label") or e["id"],
            "menu_path": cmd.get("menu_path") or help_info.get("menu_path", ""),
            "implemented_status": help_info.get("implemented_status", "command_only"),
            "requires_data": cmd.get("requires_data", True) if cmd else False,
            "dataset_id": ds,
            "role_map": role_map,
            "import_worksheet_name": f"demo_{ds}" if ds else None,
            "no_import_reason": None if ds else (
                "无需导入：计算器/设计生成/编排参考，直接打开菜单"
                if e["id"] in NO_DATA_IDS or not cmd
                else "教程标注为公式参考；可选导入对照数据"
            ),
        }
        if ds:
            serves[ds].append(e["id"])
        mappings.append(item)

    out_json = {
        "generated_at": date.today().isoformat(),
        "catalog_version": "learning-center-mapping-v1",
        "datasets": DATASETS,
        "mappings": mappings,
    }
    for ds_id, cmds in serves.items():
        DATASETS[ds_id]["serves_commands"] = sorted(cmds)

    json_path = ROOT / "tools/learning_data/dataset_mapping.json"
    json_path.write_text(json.dumps(out_json, ensure_ascii=False, indent=2), encoding="utf-8")

    lines = [
        "# DataLab 学习中心 — Agent B 数据集映射表",
        "",
        f"> 生成日期：{date.today().isoformat()}  ",
        f"> 覆盖 command/help 并集：**{len(mappings)}** 条  ",
        "> 权威计划（只读）：`docs/research/goal-learning-center-black-belt-plan.md`",
        "",
        "## 多工作表接入方案（Agent C 必读）",
        "",
        "### 现状（代码证据）",
        "",
        "- `MainWindow` 仅持有一个 `table_` + `WorksheetModel`（`mainwindow.h`），**无** `QMap<工作表名, DataTable>`。",
        "- `ProjectNavigator::add_worksheet()` 只在树节点追加名称，**不切换**底层数据。",
        "- MES/数据库导入（`import_database` / `open_mes_tools`）会 `clear_contents()` 并 **替换** `table_`，且清空输出页 — **与学习中心的「不覆盖、不清输出」冲突**。",
        "",
        "### 最小接缝（Agent C 实现，禁止静默丢数）",
        "",
        "1. 在 `MainWindow` 增加 `std::map<std::string, datalab::domain::DataTable> worksheets_` 与 `std::string active_worksheet_`。",
        "2. `import_learning_dataset(dataset_id)`：",
        "   - 若 `table_` 非空且不在 `worksheets_` 中，先以当前名（或 `工作表_1`）存入 `worksheets_`。",
        "   - 从 `LearningDatasetStore` 物化 `demo_<short>` 表，写入 `worksheets_`，设为 `active_worksheet_`，`display_table()`。",
        "   - `navigator_->add_worksheet(demo名)`；**不**调用 `output_workspace_->clear_pages()`。",
        "3. 导航器点击工作表（需补信号 `worksheet_activated(QString)`）：从 `worksheets_` 加载到 `table_` + model。",
        "4. 若首版来不及做切换：至少 **导入前把当前表存入 map**，新表为活动表；旧表仍在 map 中可恢复（比单表覆盖安全）。",
        "",
        "### SQLite / qrc 嵌入（Agent C）",
        "",
        "- 库：`resources/help/learning_center.sqlite`",
        "- 生成：`tools/build_learning_center_db.py` ← `tools/learning_data/*.csv` + `dataset_mapping.json`",
        "- qrc：并入 `resources/help/learning_center.qrc` 或 `algorithm_help_resources`",
        "- 连接名：`learning_center_<uuid>`，用完 `close()` + `removeDatabase()`",
        "",
        "---",
        "",
        "## 共享数据集定义",
        "",
    ]
    for ds_id, ds in DATASETS.items():
        cmds = ds.get("serves_commands", [])
        lines += [
            f"### `{ds_id}` — {ds['title']}",
            "",
            f"- **行业**: {ds['industry']}",
            f"- **故事**: {ds['story']}",
            f"- **行数**: {ds['row_count']}",
            f"- **服务命令数**: {len(cmds)}",
            "",
            "| 列序 | 列名 | role_hint | 说明 |",
            "|------|------|-----------|------|",
        ]
        for c in ds["columns"]:
            desc = c.get("description", c.get("unit", ""))
            lines.append(f"| {c['index']} | `{c['name']}` | {c['role_hint']} | {desc} |")
        lines.append("")
        lines.append("<details><summary>服务的 command_id 列表</summary>")
        lines.append("")
        lines.append(", ".join(f"`{x}`" for x in cmds))
        lines.append("")
        lines.append("</details>")
        lines.append("")

    lines += ["---", "", "## command_id → dataset_id → 角色映射（全集）", ""]
    lines.append("| command_id | dataset_id | 角色→列 | 工作表名 | 备注 |")
    lines.append("|------------|------------|---------|----------|------|")
    for m in sorted(mappings, key=lambda x: (x["menu_path"], x["command_id"])):
        roles = "; ".join(f"`{k}`→`{v}`" for k, v in m["role_map"].items()) or "—"
        note = m["no_import_reason"] or ""
        ws = m["import_worksheet_name"] or "—"
        ds = m["dataset_id"] or "—"
        lines.append(f"| `{m['command_id']}` | `{ds}` | {roles} | `{ws}` | {note} |")

  # audit
    missing_ds = [m for m in mappings if m["requires_data"] and m["dataset_id"] is None
                  and m["implemented_status"] == "implemented"
                  and m["command_id"] not in NO_DATA_IDS]
    lines += ["", "## 审计", ""]
    lines.append(f"- 映射条目：{len(mappings)}")
    lines.append(f"- 数据集数量：{len(DATASETS)}")
    lines.append(f"- 无 dataset 的 implemented 需数据命令：{len(missing_ds)}")
    if missing_ds:
        lines.append("  - " + ", ".join(m["command_id"] for m in missing_ds))

    md_path = ROOT / "docs/research/learning-center-dataset-mapping.md"
    md_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {md_path} ({len(mappings)} mappings, {len(DATASETS)} datasets)")
    if missing_ds:
        print("WARN missing dataset for:", [m["command_id"] for m in missing_ds])


if __name__ == "__main__":
    main()
