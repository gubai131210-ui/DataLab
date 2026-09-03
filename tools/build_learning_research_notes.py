#!/usr/bin/env python3
"""
Generate docs/research/learning-center-research-notes.md from id_metadata.json
and curated research templates. Agent A deliverable.
"""
from __future__ import annotations

import json
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ACCESSED = "2026-09-03"

# Shared authoritative sources (reused across entries; each entry cites >=1)
SOURCES = {
    "nist_spc": {
        "label": "NIST/SEMATECH e-Handbook — Control Charts",
        "url": "https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm",
        "accessed": ACCESSED,
    },
    "nist_eda": {
        "label": "NIST/SEMATECH e-Handbook — EDA",
        "url": "https://www.itl.nist.gov/div898/handbook/eda/eda.htm",
        "accessed": ACCESSED,
    },
    "nist_anova": {
        "label": "NIST/SEMATECH e-Handbook — ANOVA",
        "url": "https://www.itl.nist.gov/div898/handbook/prc/section2/prc231.htm",
        "accessed": ACCESSED,
    },
    "montgomery_spc": {
        "label": "Montgomery — Introduction to Statistical Quality Control (7th ed.)",
        "url": "https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816",
        "accessed": ACCESSED,
    },
    "aiag_spc": {
        "label": "AIAG — Statistical Process Control (SPC) Reference Manual",
        "url": "https://www.aiag.org/quality/automotive-core-tools/spc",
        "accessed": ACCESSED,
    },
    "aiag_msa": {
        "label": "AIAG — Measurement Systems Analysis (MSA) Reference Manual",
        "url": "https://www.aiag.org/quality/automotive-core-tools/msa",
        "accessed": ACCESSED,
    },
    "wheeler": {
        "label": "Wheeler — Understanding Variation",
        "url": "https://www.spcpress.com/understanding-variation/",
        "accessed": ACCESSED,
    },
    "minitab_imr": {
        "label": "Minitab Support — Individuals Chart",
        "url": "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/individuals-chart/",
        "accessed": ACCESSED,
    },
    "minitab_cap": {
        "label": "Minitab Support — Capability Analysis",
        "url": "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/",
        "accessed": ACCESSED,
    },
    "minitab_gage": {
        "label": "Minitab Support — Gage R&R Study",
        "url": "https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-r-r-study/",
        "accessed": ACCESSED,
    },
    "minitab_doe": {
        "label": "Minitab Support — Factorial Design",
        "url": "https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/",
        "accessed": ACCESSED,
    },
    "asq_spc": {
        "label": "ASQ — Control Chart",
        "url": "https://asq.org/quality-resources/control-chart",
        "accessed": ACCESSED,
    },
    "iso_7870": {
        "label": "ISO 7870 — Control charts (overview)",
        "url": "https://www.iso.org/standard/78702.html",
        "accessed": ACCESSED,
    },
    "nist_reliability": {
        "label": "NIST/SEMATECH e-Handbook — Reliability",
        "url": "https://www.itl.nist.gov/div898/handbook/apr/apr.htm",
        "accessed": ACCESSED,
    },
    "montgomery_doe": {
        "label": "Montgomery — Design and Analysis of Experiments",
        "url": "https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611",
        "accessed": ACCESSED,
    },
}

# Per-id curated research (keys not listed fall back to category template)
CURATED: dict[str, dict] = {
    "descriptive": {
        "used_for": "快速摸清一列或多列测量值的中心位置、散布和分布形状，并按产线/班次分组对比。",
        "not_for": "不能判断过程是否稳定、是否满足规格，也不能替代假设检验或控制图。",
        "typical_sample_size": "探索阶段 n≥30；分组对比每组建议 n≥5。",
        "common_mistakes": [
            "把均值接近规格中点当成“过程良好”。",
            "忽略偏度/峰度就套用正态能力分析。",
        ],
        "manufacturing_scenario": "SMT 线体导入 `锡膏高度_um`，按 `产线` 分组看均值与标准差是否一致。",
        "sources": ["nist_eda", "montgomery_spc"],
        "dataset_hint": None,
    },
    "normality_test": {
        "used_for": "检查测量数据与正态假设的偏离程度，为后续参数/非参数方法选型提供证据。",
        "not_for": "p>α 不能证明正态；p≤α 也不能单独证明“必须换方法”而不看图形与工程背景。",
        "typical_sample_size": "n≥20 较稳定；n<10 检验功效很低。",
        "common_mistakes": [
            "把未拒绝正态当成“已证明正态”。",
            "大样本下微小偏离也会显著，却忽略实际影响大小。",
        ],
        "manufacturing_scenario": "注塑件 `模腔尺寸_mm` 在换模后先做正态性，再决定是否用 Cp/Cpk。",
        "sources": ["nist_eda", "montgomery_spc"],
        "dataset_hint": None,
    },
    "imr": {
        "used_for": "监控单件/慢节拍过程的个体值与移动极差，识别特殊原因引起的突变、漂移或周期。",
        "not_for": "不能证明过程能力合格；不能替代抽样检验放行；不等同于能力指数报告。",
        "typical_sample_size": "建议≥20 个点；子组不可合并时优先 I-MR。",
        "common_mistakes": [
            "把控制限内当成“质量合格”。",
            "未按时间顺序排列就画 I-MR。",
        ],
        "manufacturing_scenario": "光测站逐件记录 `焊点偏移_um` 与 `检测时间`，用 I-MR 看是否出现漂移。",
        "sources": ["nist_spc", "aiag_spc", "wheeler", "minitab_imr"],
        "chart_note": "看点是否随机分布在中心线附近、是否超出控制限、是否存在趋势/周期；不替代假设检验。",
        "dataset_hint": None,
    },
    "xbar_r": {
        "used_for": "子组大小 2–10 时监控过程均值与组内变异，判断均值或波动是否受特殊原因影响。",
        "not_for": "不能代替量具研究；不能证明长期 Cpk；子组内非独立时结论可能失真。",
        "typical_sample_size": "≥20–25 个子组，子组大小 n=4–5 常用。",
        "common_mistakes": [
            "子组划分随意（跨班次/跨夹具混在同一子组）。",
            "R 图失控却只看 Xbar 图。",
        ],
        "manufacturing_scenario": "注塑 `模腔` 每模 4 穴取 `尺寸_mm`，按模次为子组画 Xbar-R。",
        "sources": ["nist_spc", "aiag_spc", "montgomery_spc"],
        "chart_note": "Xbar 看均值偏移，R 看组内散布突变；模式识别不替代显著性检验。",
        "dataset_hint": None,
    },
    "capability": {
        "used_for": "在过程受控且规格明确时，估计变异相对规格宽度的能力指数（Cp/Cpk 等）。",
        "not_for": "不能证明过程“合格”或“可放行”；失控过程上的 Cpk 没有预测意义。",
        "typical_sample_size": "≥30 个独立观测；子组结构需先确认稳定。",
        "common_mistakes": [
            "未先确认受控就算能力。",
            "规格限设错或混用短期/长期 σ。",
        ],
        "manufacturing_scenario": "稳定后评估 `锡膏高度_um` 相对 USL/LSL 的 Cpk。",
        "sources": ["minitab_cap", "aiag_spc", "montgomery_spc"],
        "dataset_hint": None,
    },
    "gage_rr": {
        "used_for": "量化测量系统重复性与再现性占比，判断测量噪声是否掩盖过程/产品差异。",
        "not_for": "不能证明量具“通过”或产品合格；不能替代仪器校准记录。",
        "typical_sample_size": "交叉设计常用 10 件×3 操作员×2–3 次重复。",
        "common_mistakes": [
            "零件未覆盖过程变异范围。",
            "操作员知道目标值造成再现性偏低假象。",
        ],
        "manufacturing_scenario": "三座标测 `外壳厚度_mm`：零件×操作员×重复，评估 %GRR。",
        "sources": ["aiag_msa", "minitab_gage"],
        "dataset_hint": None,
    },
    "doe_factorial": {
        "used_for": "筛选多个因子对响应的主效应与交互，用结构化试验代替盲目调参。",
        "not_for": "不能外推到未试验的极端条件；不能替代长期 SPC 监控。",
        "typical_sample_size": "2^k 全因子或 2^(k-p) 部分因子；每单元≥1 次重复更佳。",
        "common_mistakes": [
            "因子水平设置不现实导致结论不可用。",
            "不随机化顺序引入时间混杂。",
        ],
        "manufacturing_scenario": "回流焊 `温度_℃`、`链速_mm_min`、`氮气流量` 对 `虚焊率` 的 2^3 设计。",
        "sources": ["montgomery_doe", "minitab_doe"],
        "dataset_hint": None,
    },
    "histogram": {
        "used_for": "直观看测量值的分布形状、多峰与离群，辅助理解过程变异结构。",
        "not_for": "不能检验正态性显著性；不能判断时间上的稳定性（需控制图/时序图）。",
        "typical_sample_size": "n≥30 图形较稳定；分组对比需每组足够点。",
        "common_mistakes": [
            "箱数过少掩盖多峰结构。",
            "把分布形状直接等同于过程能力结论。",
        ],
        "manufacturing_scenario": "对比两产线 `膜厚_um` 的分布是否重叠。",
        "sources": ["nist_eda", "montgomery_spc"],
        "chart_note": "看对称性、拖尾、多峰与离群；图形探索不替代假设检验。",
        "dataset_hint": None,
    },
    "special_cause_rules": {
        "used_for": "理解 Western Electric / Nelson 等规则如何标记控制图上的可疑模式，辅助识别特殊原因。",
        "not_for": "规则触发不等于必须停线或删点；不能替代工程调查与 SOP。",
        "typical_sample_size": "依附于控制图子组/点数，通常≥20 点。",
        "common_mistakes": [
            "规则过多导致假警报泛滥。",
            "触发后未区分常见原因与特殊原因。",
        ],
        "manufacturing_scenario": "I-MR 监控 `锡膏高度_um` 时勾选适用规则，记录哪条规则触发。",
        "sources": ["nist_spc", "aiag_spc", "wheeler"],
        "chart_note": "规则用于模式提示；最终判断需结合工艺知识，不替代假设检验。",
        "dataset_hint": None,
    },
    "database_import": {
        "used_for": "（编排参考）从外部数据库导入数据到工作区的概念说明，便于与 MES/ERP 数据衔接。",
        "not_for": "当前版本可能无独立菜单；不能替代数据清洗与列角色配置。",
        "typical_sample_size": "视业务表规模；教程演示建议≤200 行。",
        "common_mistakes": ["把导入当分析完成，不做列类型与缺失检查。"],
        "manufacturing_scenario": "从质检库导入 `批次`、`测量值`、`检测时间` 列供后续 SPC。",
        "sources": ["nist_eda"],
        "implemented_note": "help 条目为 orchestration；菜单可能不存在，教程仅作流程示意。",
    },
    "report_templates": {
        "used_for": "（编排参考）报告模板与输出页编排的概念，帮助理解如何固化分析交付物。",
        "not_for": "不能替代实际统计分析；当前版本可能无独立菜单。",
        "typical_sample_size": "N/A",
        "common_mistakes": ["模板美化掩盖数据质量问题。"],
        "manufacturing_scenario": "将 `Cp/Cpk` 与 `控制图` 输出页编入周质量报告。",
        "sources": ["aiag_spc"],
        "implemented_note": "orchestration 条目；步骤写「公式见帮助对话框」。",
    },
    "reliability_warranty": {
        "used_for": "基于寿命数据评估保修/失效风险，支持可靠性预测与备件策略。",
        "not_for": "不能证明产品已满足寿命目标；外推需假设失效机制不变。",
        "typical_sample_size": "失效数≥10 较稳；截尾数据需声明删失类型。",
        "common_mistakes": ["忽略右删失把未失效当成失效。", "混合不同失效模式。"],
        "manufacturing_scenario": "电源模块 `循环次数` 与 `是否失效` 预测保修期失效率。",
        "sources": ["nist_reliability", "montgomery_spc"],
        "dataset_hint": None,
        "implemented_note": "命令表有、help 暂无独立条目；教程对齐 menu_path。",
    },
}

# Category fallback templates when no curated entry
CATEGORY_TEMPLATES: dict[str, dict] = {
    "基础统计": {
        "used_for": "对测量数据做基础汇总、检验或比较，为工艺决策提供统计证据。",
        "not_for": "不能单独证明过程长期合格或因果成立。",
        "typical_sample_size": "n≥30 探索；检验功效依赖效应大小。",
        "common_mistakes": ["混淆统计显著与工程显著。", "忽略缺失值与测量单位。"],
        "sources": ["nist_eda", "montgomery_spc"],
    },
    "假设检验": {
        "used_for": "在明确假设下检验均值/比例/分布差异是否超出随机波动。",
        "not_for": "p 值不能证明原假设为真；不能替代控制图监控。",
        "typical_sample_size": "功效分析建议 n≥20/组；配对数据成对完整。",
        "common_mistakes": ["多重比较不校正。", "非独立样本用独立检验。"],
        "sources": ["nist_anova", "montgomery_spc"],
    },
    "方差分析": {
        "used_for": "比较三个及以上水平均值是否一致，或评估因子主效应。",
        "not_for": "显著不等于因果；残差正态/方差齐性需结合图形检查。",
        "typical_sample_size": "每水平 n≥5，总体≥30 更稳。",
        "common_mistakes": ["不做事后比较就断定哪组最好。", "忽略交互作用。"],
        "sources": ["nist_anova", "montgomery_doe"],
        "dataset_hint": None,
    },
    "回归": {
        "used_for": "量化预测因子与响应的关系，用于解释或预测。",
        "not_for": "相关/回归不能证明因果；外推超出观测范围风险大。",
        "typical_sample_size": "n ≥ 10×参数个数；残差诊断必备。",
        "common_mistakes": ["忽略多重共线性。", "把 R² 当成因果强度。"],
        "sources": ["nist_eda", "montgomery_doe"],
        "dataset_hint": None,
    },
    "控制图": {
        "used_for": "按时间顺序监控过程，区分常见原因与特殊原因变异。",
        "not_for": "不能证明能力合格；不替代抽样验收。",
        "typical_sample_size": "≥20 子组/点；属性图需足够缺陷计数。",
        "common_mistakes": ["未按时间排序。", "控制限内就当合格。"],
        "sources": ["nist_spc", "aiag_spc", "asq_spc"],
        "chart_note": "识别趋势、周期、突变与超出控制限；模式识别不替代假设检验。",
        "dataset_hint": None,
    },
    "能力": {
        "used_for": "评估过程变异相对规格的位置与宽度。",
        "not_for": "失控过程或无代表样本时指数无预测意义。",
        "typical_sample_size": "≥30 独立观测。",
        "common_mistakes": ["未验证受控。", "规格限错误。"],
        "sources": ["minitab_cap", "aiag_spc"],
        "dataset_hint": None,
    },
    "MSA": {
        "used_for": "评估测量系统变异是否可接受，避免误判产品/过程。",
        "not_for": "不能替代校准合格证；不能证明产品合格。",
        "typical_sample_size": "AIAG 交叉法 10×3×2/3。",
        "common_mistakes": ["零件范围不足。", "操作员培训不一致。"],
        "sources": ["aiag_msa", "minitab_gage"],
        "dataset_hint": None,
    },
    "DOE": {
        "used_for": "系统筛选/优化因子，估计主效应与交互。",
        "not_for": "不能替代量产 SPC；外推需谨慎。",
        "typical_sample_size": "2^k 或响应曲面 3–5 水平。",
        "common_mistakes": ["不随机化。", "因子水平不现实。"],
        "sources": ["montgomery_doe", "minitab_doe"],
        "dataset_hint": None,
    },
    "图形": {
        "used_for": "可视化分布、关系与结构，辅助 EDA 与沟通。",
        "not_for": "图形本身不提供显著性检验（除非配套检验）。",
        "typical_sample_size": "n≥20 可见结构；时序图按时间顺序。",
        "common_mistakes": ["过度解读偶然模式。", "坐标轴截断误导。"],
        "sources": ["nist_eda", "montgomery_spc"],
        "chart_note": "看模式与离群；不替代假设检验。",
    },
    "时间序列": {
        "used_for": "分析按时间采集的数据：趋势、季节、自相关与短期预测。",
        "not_for": "预测区间不等于规格合格；结构突变时需重新建模。",
        "typical_sample_size": "≥50 点识别季节；ARIMA 需足够历史。",
        "common_mistakes": ["非平稳序列直接回归。", "忽略异常点影响。"],
        "sources": ["nist_eda", "montgomery_spc"],
        "dataset_hint": None,
    },
    "可靠性": {
        "used_for": "基于失效/删失数据分析寿命分布与加速应力效应。",
        "not_for": "不能写成产品已达标；外推需工程论证。",
        "typical_sample_size": "失效事件≥10；加速试验需应力模型假设。",
        "common_mistakes": ["混用失效模式。", "忽略删失。"],
        "sources": ["nist_reliability"],
        "dataset_hint": None,
    },
    "质量工具": {
        "used_for": "质量改进常用工具（帕累托、抽样、多变异等）支持问题定义与验证。",
        "not_for": "不能替代统计检验或 SPC 证据。",
        "typical_sample_size": "视工具而定；帕累托需完整缺陷分类。",
        "common_mistakes": ["类别划分不一致导致帕累托失真。"],
        "sources": ["aiag_spc", "montgomery_spc"],
        "dataset_hint": None,
    },
    "机器学习": {
        "used_for": "探索性分类/聚类/异常检测，辅助发现复杂模式。",
        "not_for": "黑箱模型不能替代因果 DOE；需验证过拟合与可解释性。",
        "typical_sample_size": "训练样本远大于特征数；交叉验证必备。",
        "common_mistakes": ["数据泄漏。", "未划分训练/验证集。"],
        "sources": ["nist_eda"],
    },
}

MENU_PATH_DATASET = {
    "控制图": None,
    "图形": None,
    "统计": None,
}


def load_research_by_id() -> dict:
    path = ROOT / "tools/learning_data/research_by_id.json"
    if path.exists():
        return json.loads(path.read_text(encoding="utf-8"))
    return {}


def load_mapping_hints() -> dict[str, str]:
    mapping_path = ROOT / "tools/learning_data/dataset_mapping.json"
    if not mapping_path.exists():
        return {}
    mapping = json.loads(mapping_path.read_text(encoding="utf-8"))
    return {
        m["command_id"]: (m.get("dataset_id") or "")
        for m in mapping.get("mappings") or []
    }

def pick_template(entry: dict, research_map: dict) -> dict:
    cid = entry["id"]
    base: dict = {}
    if cid in research_map:
        base = dict(research_map[cid])
        # resolve source keys to full source objects later
    elif cid in CURATED:
        base = dict(CURATED[cid])
    else:
        help_info = entry.get("help") or {}
        cat = help_info.get("category", "")
        menu_path = (entry.get("command") or {}).get("menu_path") or help_info.get("menu_path", "")
        if menu_path == "控制图" or cat == "控制图":
            base = dict(CATEGORY_TEMPLATES["控制图"])
        elif menu_path == "图形" or cat in ("图形", "探索性数据分析"):
            base = dict(CATEGORY_TEMPLATES["图形"])
        elif cat in CATEGORY_TEMPLATES:
            base = dict(CATEGORY_TEMPLATES[cat])
        else:
            base = dict(CATEGORY_TEMPLATES["基础统计"])
    help_info = entry.get("help") or {}
    if "等价" in (help_info.get("title") or "") and "used_for" not in base:
        base.update({
            "used_for": "证明效应落在等价区间内，而非简单“有无差异”。",
            "not_for": "不能证明工艺完全等同；边界需事前协议。",
        })
    if help_info.get("implemented_status") == "formula_reference" and "implemented_note" not in base:
        base["implemented_note"] = (
            "formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。"
        )
    menu_path = (entry.get("command") or {}).get("menu_path") or ""
    if menu_path in ("控制图", "图形") and "chart_note" not in base:
        base["chart_note"] = "看模式、离群与趋势；图形/控制图不替代假设检验。"
    return base

def format_roles(entry: dict) -> str:
    cmd = entry.get("command")
    if not cmd:
        return "_（无 analysis_commands 条目；见 help id）_"
    roles = cmd.get("roles") or []
    if not roles:
        return "_无需列角色（计算器/设计生成类）_"
    parts = [f"`{r['id']}`（{r['label']}）" for r in roles]
    return "、".join(parts)

def format_menu(entry: dict) -> str:
    cmd = entry.get("command")
    help_info = entry.get("help") or {}
    if cmd:
        group = ""
        # menu_group not in extracted metadata; use help menu_path
        mp = cmd["menu_path"]
        label = cmd["menu_label"]
        return f"**{mp}** → {label}"
    return help_info.get("menu_path", "—")

def render_entry(entry: dict, research_map: dict) -> str:
    cid = entry["id"]
    tpl = pick_template(entry, research_map)
    help_info = entry.get("help") or {}
    title = (help_info.get("title") or (entry.get("command") or {}).get("menu_label") or cid)
    status = help_info.get("implemented_status", "command_only")
    purpose = help_info.get("purpose", "")
    limits = help_info.get("interpretation_limits", "")
    lines = [
        f"### {cid} — {title}",
        "",
        f"- **implemented_status**: `{status}`",
        f"- **菜单路径（代码）**: {format_menu(entry)}",
        f"- **对话框角色**: {format_roles(entry)}",
    ]
    if purpose:
        lines.append(f"- **algorithm_help purpose（对齐）**: {purpose}")
    if limits:
        lines.append(f"- **interpretation_limits**: {limits}")
    if tpl.get("implemented_note"):
        lines.append(f"- **实现说明**: {tpl['implemented_note']}")
    lines += [
        "",
        "**常用来做什么**",
        tpl["used_for"],
        "",
        "**不能当什么用**",
        tpl["not_for"],
        "",
        f"**典型样本量**: {tpl['typical_sample_size']}",
        "",
        "**制造场景（列名示例）**",
        tpl.get("manufacturing_scenario", "见共享数据集业务故事。"),
        "",
        "**常见误用**",
    ]
    for m in tpl.get("common_mistakes", []):
        lines.append(f"- {m}")
    if tpl.get("chart_note"):
        lines.append("")
        lines.append(f"**图形解读要点**: {tpl['chart_note']}")
    # Wave-5: prefer v2 lock-table mapping; never recommend retired shared tables.
    hints = getattr(render_entry, "_mapping_hints", None)
    if isinstance(hints, dict) and entry["id"] in hints:
        ds = hints[entry["id"]]
    else:
        ds = tpl.get("dataset_hint") or MENU_PATH_DATASET.get(
            (entry.get("command") or {}).get("menu_path", "")
        )
    if ds:
        lines += ["", f"**建议 dataset_id**: `{ds}`"]
    else:
        lines += ["", "**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）"]
    lines += ["", "**权威来源**"]
    src_keys = tpl.get("sources", ["nist_eda"])
    for key in src_keys:
        if isinstance(key, dict):
            lines.append(f"- [{key['label']}]({key['url']}) — accessed {key.get('accessed', ACCESSED)}")
        elif key in SOURCES:
            s = SOURCES[key]
            lines.append(f"- [{s['label']}]({s['url']}) — accessed {s['accessed']}")
        else:
            lines.append(f"- {key} — accessed {ACCESSED}")
    lines.append("")
    return "\n".join(lines)

def main() -> None:
    meta_path = ROOT / "tools/learning_data/id_metadata.json"
    meta = json.loads(meta_path.read_text(encoding="utf-8"))
    research_map = load_research_by_id()
    render_entry._mapping_hints = load_mapping_hints()  # type: ignore[attr-defined]
    entries = meta["entries"]
    out_lines = [
        "# DataLab 学习中心 — Agent A 调研笔记",
        "",
        f"> 生成日期：{date.today().isoformat()}  ",
        f"> 覆盖 id 数量：**{len(entries)}**（`analysis_commands::all()` ∪ `algorithm_help.json` entries）  ",
        "> 权威：[`goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md`]"
        "(goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md)；"
        "锁表 mapping：[`learning-center-dataset-mapping.md`](learning-center-dataset-mapping.md)。",
        "",
        "## 清单审计",
        "",
        f"- 并集条目数：{len(entries)}",
        "- help-only：`database_import`, `report_templates`, `special_cause_rules`",
        "- command-only：`reliability_warranty`",
        "- 每条 ≥1 权威来源；控制图/图形类含模式识别与「不替代假设检验」说明",
        "",
        "## 数据集策略（Wave-5）",
        "",
        "- 旧 10 张共享宽表 **已作废**，禁止再当「建议 dataset_id」。",
        "- 现行专用主集 + §3 同构白名单见 [`learning-center-dataset-mapping.md`]"
        "(learning-center-dataset-mapping.md)。",
        "- 下文「建议 dataset_id」对齐 `tools/learning_data/dataset_mapping.json`。",
        "",
        "---",
        "",
    ]
    # Group by menu_path
    groups: dict[str, list] = {}
    for e in entries:
        cmd = e.get("command")
        help_info = e.get("help") or {}
        key = (cmd or {}).get("menu_path") or help_info.get("menu_path", "其他").split(">")[0].strip()
        groups.setdefault(key, []).append(e)
    for group_name in sorted(groups.keys(), key=lambda x: (x == "其他", x)):
        out_lines.append(f"## {group_name}")
        out_lines.append("")
        for e in sorted(groups[group_name], key=lambda x: x["id"]):
            out_lines.append(render_entry(e, research_map))
    out_path = ROOT / "docs/research/learning-center-research-notes.md"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(out_lines), encoding="utf-8")
    print(f"Wrote {out_path} ({len(entries)} entries)")

if __name__ == "__main__":
    main()
