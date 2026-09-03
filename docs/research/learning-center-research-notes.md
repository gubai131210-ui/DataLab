# DataLab 学习中心 — Agent A 调研笔记

> 生成日期：2026-09-03  
> 覆盖 id 数量：**184**（`analysis_commands::all()` ∪ `algorithm_help.json` entries）  
> 权威计划（只读）：`docs/research/goal-learning-center-black-belt-plan.md`

## 清单审计

- 并集条目数：184
- help-only：`database_import`, `report_templates`, `special_cause_rules`
- command-only：`reliability_warranty`
- 每条 ≥1 权威来源；控制图/图形类含模式识别与「不替代假设检验」说明

## 共享数据集（Agent B 映射参考）

| dataset_id | 场景 |
|------------|------|
| smt_paste_height | SMT 锡膏高度 |
| two_line_thickness | 两产线膜厚 |
| paired_rework | 返工前后 |
| anova_cavity | 三模腔尺寸 |
| corr_temp_offset | 温度 vs 偏移 |
| attribute_defect | 班次不良 |
| gage_rr_balance | 量具 R&R |
| doe_factorial_demo | 析因/Taguchi/混料 |
| reliability_cycles | 寿命循环 |
| ts_weekly_yield | 周良率 |

---

## 图形

### area_plot — 面积图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 区域图
- **对话框角色**: `time`（顺序/时间变量）
- **algorithm_help purpose（对齐）**: 折线下方填色，强调累积或堆叠。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### bar_chart — 条形图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 条形图
- **对话框角色**: `category`（类别）
- **algorithm_help purpose（对齐）**: 分类计数条形；保持出现顺序。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### boxplot — 箱线图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 箱线图
- **对话框角色**: `variables`（变量（可多选））
- **algorithm_help purpose（对齐）**: 用五数概括展示位置与散布：Q1、中位数、Q3，须线到最后未超出围栏的点，围栏外画为离群点。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
光学膜：`膜厚_um`、`产线`（A/B 线）

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `two_line_thickness`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### bubble_plot — 气泡图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 气泡图
- **对话框角色**: `x_variable`（X 变量）
- **algorithm_help purpose（对齐）**: (x,y) 位置，气泡面积映射第三列。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### chi_square_mosaic_link — 卡方–马赛克联动

- **implemented_status**: `orchestration`
- **菜单路径（代码）**: **图形** → 卡方–马赛克联动
- **对话框角色**: `row_category`（行分类列）
- **algorithm_help purpose（对齐）**: 同一分类列组合输出卡方调整残差表/热图与马赛克图。
- **interpretation_limits**: 不写合格判定或因果结论。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### contour_plot — 等值线图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 等值线图
- **对话框角色**: `x_variable`（X 变量）
- **algorithm_help purpose（对齐）**: 在规则网格上对 z(x,y) 画等值线。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### correlation_plot — 相关图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 相关图
- **对话框角色**: `variables`（变量（可多选））
- **algorithm_help purpose（对齐）**: 多变量两两散点矩阵，辅助看线性或非线性形态。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### correlogram — Correlogram（相关热图）

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **图形** → Correlogram（相关热图）
- **对话框角色**: `variables`（数值变量（可多选））
- **algorithm_help purpose（对齐）**: 把多列两两相关矩阵画成热图并输出系数表。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
分析时序趋势、季节、自相关或短期预测。

**不能当什么用**
预测区间≠规格合格；结构突变需重拟合。

**典型样本量**: ≥50 点识别季节

**制造场景（列名示例）**
周度：`周次`、`良率_pct`

**常见误用**
- 非平稳直接回归。
- 忽略异常点。

**图形解读要点**: 看模式、离群与趋势；图形/控制图不替代假设检验。

**建议 dataset_id**: `ts_weekly_yield`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### density_plot — 密度图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 密度图
- **对话框角色**: `variable`（变量）
- **algorithm_help purpose（对齐）**: 高斯核密度估计曲线（Silverman 带宽）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### dotplot — 点图

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **图形** → 点图
- **对话框角色**: `y_variable`（Y 变量（可多选））
- **algorithm_help purpose（对齐）**: 一维分布点图，可选分组与 jitter。
- **interpretation_limits**: 解释层只陈述统计证据。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### ecdf_plot — 经验累积分布图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 经验累积分布图
- **对话框角色**: `variable`（变量）
- **algorithm_help purpose（对齐）**: F̂(x)=#{y≤x}/n，右端为 1。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### eda_4plot — EDA 四图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → EDA 四图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: NIST 单变量四图打包：run sequence、lag-1、histogram、normal probability。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### graph_gallery — 探索性图形画廊

- **implemented_status**: `graph_reference`
- **菜单路径（代码）**: **图形** → 探索性图形
- **对话框角色**: `x_variable`（X 变量）
- **algorithm_help purpose（对齐）**: 在相同列角色下切换 scatter/bar/box/histogram/dotplot 候选图型。
- **interpretation_limits**: 不写合格判定。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### heatmap_plot — 热图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 热图
- **对话框角色**: `variables`（相关变量）
- **algorithm_help purpose（对齐）**: 用颜色表示表格或分箱密度。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### hexbin_plot — Hexbin

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → Hexbin
- **对话框角色**: `x_variable`（X 变量）
- **algorithm_help purpose（对齐）**: 二维矩形分箱计数着色（Binned Scatter）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### histogram — 直方图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 直方图
- **对话框角色**: `variables`（变量（可多选））
- **algorithm_help purpose（对齐）**: 把数值分成若干区间，用矩形高度表示频数或密度。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### interval_plot — 区间图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 区间散点图
- **对话框角色**: `response`（响应变量）
- **algorithm_help purpose（对齐）**: 按组显示均值和置信区间。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### marginal_plot — 边际图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 边际图
- **对话框角色**: `x_variable`（X 变量）
- **algorithm_help purpose（对齐）**: 散点加上 x/y 边际直方图或箱线。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### matrix_plot — 矩阵图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 矩阵图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 所选数值列的两两散点。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### mosaic_plot — 马赛克图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 马赛克图
- **对话框角色**: `categories`（分类列（2～3，可多选））
- **algorithm_help purpose（对齐）**: 2～3 个分类列的组合频数/比例可视化。
- **interpretation_limits**: 不写合格判定。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### parallel_plot — 平行坐标图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 平行坐标图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 每个观测一条折线，轴为各变量（常做列内缩放）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### pie_plot — 饼图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 饼图
- **对话框角色**: `category`（分类变量）
- **algorithm_help purpose（对齐）**: 类别频数占合计的比例为扇区角。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### probability_plot — 正态概率图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 概率图
- **对话框角色**: `variable`（变量）
- **algorithm_help purpose（对齐）**: 排序观测对 Blom 正态分位。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### scatter_plot — 散点图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 散点图
- **对话框角色**: `x_variable`（X 变量）
- **algorithm_help purpose（对齐）**: 每个 complete-case 点画 (x,y)。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### simplex_design_plot — 混料三角图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 混料三角图
- **对话框角色**: `components`（分量列（3～4，可多选））
- **algorithm_help purpose（对齐）**: 在三角坐标系绘制混料设计点（proportions）。
- **interpretation_limits**: 不写合格判定。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### time_series_plot — 时间序列图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 时间序列图
- **对话框角色**: `time`（时间变量）
- **algorithm_help purpose（对齐）**: 按行序或时间列连接观测。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
周度：`周次`、`良率_pct`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `ts_weekly_yield`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### violin_plot — 小提琴图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 小提琴图
- **对话框角色**: `variables`（变量（可多选））
- **algorithm_help purpose（对齐）**: 分组镜像 KDE + 箱线五数。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

## 帮助

### special_cause_rules — 特殊原因规则 Catalog

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: 帮助 > 算法
- **对话框角色**: _（无 analysis_commands 条目；见 help id）_
- **algorithm_help purpose（对齐）**: Minitab Tests 1–8 与 DataLab 图种适用表、默认策略说明。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
理解 Western Electric/Nelson 规则如何标记可疑模式。

**不能当什么用**
触发≠必须停线；需工程调查。

**典型样本量**: 依附控制图点数≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 规则过多致假警报。
- 触发后不调查根因。

**图形解读要点**: 规则提示特殊原因线索，不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03

## 控制图

### c_chart — C 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → C 图
- **对话框角色**: `defects`（缺陷数）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：C 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
固定检验面积缺陷数监控。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### cusum — CUSUM 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → CUSUM 控制图
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：CUSUM 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
累积和检测小偏移。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### ewma — EWMA 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → EWMA 控制图
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：EWMA 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
对微小漂移敏感的 EWMA。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### g_chart — G 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → G 图
- **对话框角色**: `variables`（间隔列）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：G 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
几何分布间隔/良率事件监控。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### generalized_variance — 广义方差图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → 广义方差图
- **对话框角色**: `variables`（变量（多列））
- **algorithm_help purpose（对齐）**: 多元广义方差 |S| 子组控制图（Montgomery）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
多变量广义方差监控。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### hotelling_t2 — Hotelling T²

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → Hotelling T²
- **对话框角色**: `variables`（变量（多列））
- **algorithm_help purpose（对齐）**: 多元个体 Hotelling T² 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
多变量 T² 监控。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### imr — I-MR 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → I-MR 控制图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：I-MR 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
单件慢节拍个体值与移动极差监控。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### imr_rs — I-MR-R/S 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → I-MR-R/S 控制图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：I-MR-R/S 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
I-MR-R/S 组合监控。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### laney_p_chart — Laney P' 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → Laney P' 图
- **对话框角色**: `defectives`（不合格品数）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：Laney P' 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
过度离散时的 Laney P 图。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### laney_u_chart — Laney U' 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → Laney U' 图
- **对话框角色**: `defects`（缺陷数）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：Laney U' 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
过度离散时的 Laney U 图。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### mewma — MEWMA

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → MEWMA
- **对话框角色**: `variables`（变量（多列））
- **algorithm_help purpose（对齐）**: 多元 EWMA（MEWMA）控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
多变量 EWMA。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### moving_average — 移动平均控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → 移动平均控制图
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：移动平均控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
移动平均平滑监控。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
周度：`周次`、`良率_pct`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `ts_weekly_yield`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### np_chart — NP 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → NP 图
- **对话框角色**: `defectives`（不合格品数）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：NP 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
不合格品数监控（固定样本量）。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### p_chart — P 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → P 图
- **对话框角色**: `defectives`（不合格品数）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：P 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
不合格品率监控（变样本量）。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### t_chart — T 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → T 图
- **对话框角色**: `variables`（间隔列）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：T 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
时间间隔 T 图。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### u_chart — U 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → U 图
- **对话框角色**: `defects`（缺陷数）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：U 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
单位缺陷数监控（变检验单位）。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### xbar_r — Xbar-R 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → Xbar-R 控制图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：Xbar-R 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
子组 2–10 的均值与极差监控。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
注塑：`模腔尺寸_mm`、`模腔`（1/2/3 穴）

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `anova_cavity`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### xbar_s — Xbar-S 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → Xbar-S 控制图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：Xbar-S 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
子组较大时用标准差监控散布。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
注塑：`模腔尺寸_mm`、`模腔`（1/2/3 穴）

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `anova_cavity`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### z_mr — Z-MR 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → Z-MR 控制图
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：Z-MR 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
标准化个体与 MR 监控。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

### zone_chart — 区域图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → 区域图
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：区域图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
区域规则辅助识别模式。

**不能当什么用**
不能证明能力合格或产品可放行；不替代抽样验收。

**典型样本量**: ≥20 子组/点

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 未按时间排序。
- 控制限内就当合格。

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Wheeler — Understanding Variation](https://www.spcpress.com/understanding-variation/) — accessed 2026-09-03

## 数据

### database_import — 数据库导入

- **implemented_status**: `partial`
- **菜单路径（代码）**: 数据 > 数据库导入…
- **对话框角色**: _（无 analysis_commands 条目；见 help id）_
- **algorithm_help purpose（对齐）**: 通过 Provider 连接数据库、发现表/视图与列，按 ImportPlan 导入工作表。
- **实现说明**: orchestration；菜单可能不存在。

**常用来做什么**
外部数据库导入工作区的流程示意。

**不能当什么用**
当前版本可能无独立菜单。

**典型样本量**: N/A

**制造场景（列名示例）**
见共享数据集业务故事。

**常见误用**
- 把导入当分析完成。

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03

## 文件

### report_templates — 报告模板（客户/工程师/审计）

- **implemented_status**: `implemented`
- **菜单路径（代码）**: 文件 > 导出 PDF
- **对话框角色**: _（无 analysis_commands 条目；见 help id）_
- **algorithm_help purpose（对齐）**: 用同一份 OutputPage/Facts 生成客户版、工程师版或审计版报告；模板只改变展示与证据密度，不重算统计。
- **实现说明**: orchestration；菜单可能不存在。

**常用来做什么**
报告模板与输出页编排参考。

**不能当什么用**
当前版本可能无独立菜单。

**典型样本量**: N/A

**制造场景（列名示例）**
见共享数据集业务故事。

**常见误用**
- 把导入当分析完成。

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03

## 统计

### accelerated_life — 加速寿命分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 加速寿命分析
- **对话框角色**: `time`（寿命）
- **algorithm_help purpose（对齐）**: 用 Weibull+Arrhenius 拟合应力加速下的寿命数据。
- **interpretation_limits**: 禁止写成产品寿命已达标。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: 失效事件≥10

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 忽略删失。
- 混合失效模式。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Reliability](https://www.itl.nist.gov/div898/handbook/apr/apr.htm) — accessed 2026-09-03

### acf_pacf — ACF/PACF

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → ACF/PACF
- **对话框角色**: `variables`（序列）
- **algorithm_help purpose（对齐）**: 计算单列序列的自相关与偏自相关，并画带置信限的 ACF/PACF 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
分析时序趋势、季节、自相关或短期预测。

**不能当什么用**
预测区间≠规格合格；结构突变需重拟合。

**典型样本量**: ≥50 点识别季节

**制造场景（列名示例）**
周度：`周次`、`良率_pct`

**常见误用**
- 非平稳直接回归。
- 忽略异常点。

**建议 dataset_id**: `ts_weekly_yield`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### adf_test — ADF 单位根检验

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → ADF 单位根检验
- **对话框角色**: `variables`（序列）
- **algorithm_help purpose（对齐）**: 对单列序列做 Augmented Dickey–Fuller 检验，报告 τ 与临界值。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
分析时序趋势、季节、自相关或短期预测。

**不能当什么用**
预测区间≠规格合格；结构突变需重拟合。

**典型样本量**: ≥50 点识别季节

**制造场景（列名示例）**
周度：`周次`、`良率_pct`

**常见误用**
- 非平稳直接回归。
- 忽略异常点。

**建议 dataset_id**: `ts_weekly_yield`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### analyze_definitive_screening — 分析确定性筛选设计

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 分析确定性筛选设计
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: YME/Y2nd + AICc stepwise 分析 DSD。
- **interpretation_limits**: 解释层只陈述统计证据。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### analyze_variability — Analyze Variability

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Analyze Variability
- **对话框角色**: `factors`（因子列）
- **algorithm_help purpose（对齐）**: 2 水平 DOE：由重复列计算每运行标准差，拟合 ln(s) 分散模型并输出效应表。
- **interpretation_limits**: 禁止与 Taguchi 分析混读。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### anom — 均值分析 (ANOM)

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 均值分析 (ANOM)
- **对话框角色**: `response`（响应变量）
- **algorithm_help purpose（对齐）**: 在正态均值假定下，比较各组均值是否偏离总体均值（Analysis of Means）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
识别哪些组均值偏离总均值（ANOM）。

**不能当什么用**
需正态/方差假设；不等 n 谨慎。

**典型样本量**: 每组 n≥5

**制造场景（列名示例）**
注塑：`模腔尺寸_mm`、`模腔`（1/2/3 穴）

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `anova_cavity`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### anom_attribute — 属性 ANOM（二项/泊松）

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 属性 ANOM（二项/泊松）
- **对话框角色**: `response`（响应列）
- **algorithm_help purpose（对齐）**: 在二项或泊松假定下，比较各组比例/计数是否偏离总体中心（Minitab 属性 ANOM）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
属性数据 ANOM。

**不能当什么用**
小计数需合并。

**典型样本量**: 每组 n≥20

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### arima — ARIMA / Best ARIMA

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → ARIMA 基础预测
- **对话框角色**: `time`（时间列（可选））
- **algorithm_help purpose（对齐）**: 对单列时间顺序数据做ARIMA / Best ARIMA。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
分析时序趋势、季节、自相关或短期预测。

**不能当什么用**
预测区间≠规格合格；结构突变需重拟合。

**典型样本量**: ≥50 点识别季节

**制造场景（列名示例）**
周度：`周次`、`良率_pct`

**常见误用**
- 非平稳直接回归。
- 忽略异常点。

**建议 dataset_id**: `ts_weekly_yield`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### best_subsets_regression — Best Subsets 回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Best Subsets 回归
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 比较不同预测变量子集的线性回归拟合，按规模展示最佳模型摘要。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### binary_doe_probit — 二值 DOE Probit/Gompit

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 二值 DOE Probit
- **对话框角色**: `factors`（因子）
- **algorithm_help purpose（对齐）**: 析因二值 DOE；Probit 或 Gompit link；IRWLS；Events/Trials 或 0/1。
- **interpretation_limits**: 禁止过程已合格。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### binary_response_doe — 二值响应 DOE

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 二值响应 DOE
- **对话框角色**: `factors`（因子列）
- **algorithm_help purpose（对齐）**: 析因设计二值响应：Logit IRWLS；events/trials 或 0/1；输出系数、Odds Ratio 与拟合诊断。
- **interpretation_limits**: 禁止过程已合格 / 已优化。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### bootstrap_mean — Bootstrap 均值 CI

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Bootstrap 均值 CI
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 对单列样本均值做百分位或 BCa bootstrap 置信区间。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### bootstrap_two_sample — Bootstrap 双样本均值差 CI

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Bootstrap 双样本均值差 CI
- **对话框角色**: `first`（样本 1）
- **algorithm_help purpose（对齐）**: 对两独立样本均值差做百分位或 BCa bootstrap 置信区间。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### cart_tree — CART 单树

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → CART 单树
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 用自研二叉递归划分做分类或回归单树，输出结点表与变量重要性。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### ccf — 互相关（CCF）

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 互相关（CCF）
- **对话框角色**: `x`（序列 X）
- **algorithm_help purpose（对齐）**: 计算两列对齐序列在正负滞后上的互相关，并画置信带。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
分析时序趋势、季节、自相关或短期预测。

**不能当什么用**
预测区间≠规格合格；结构突变需重拟合。

**典型样本量**: ≥50 点识别季节

**制造场景（列名示例）**
周度：`周次`、`良率_pct`

**常见误用**
- 非平稳直接回归。
- 忽略异常点。

**建议 dataset_id**: `ts_weekly_yield`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### chi_square — 列联表卡方

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 列联表卡方
- **对话框角色**: `row_category`（行分类列）
- **algorithm_help purpose（对齐）**: 检验两个分类变量是否独立。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
检验分类变量独立性。

**不能当什么用**
期望频数过小需合并类别。

**典型样本量**: 总 n≥50

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### chi_square_gof — 卡方拟合优度

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 卡方拟合优度
- **对话框角色**: `category`（分类列）
- **algorithm_help purpose（对齐）**: 检验一个分类变量的观察频数是否符合指定或等比例。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
检验分类分布是否符合理论比例。

**不能当什么用**
不能证明数据生成机制。

**典型样本量**: 每类期望≥5

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### cluster_observations — 层次聚类（观测）

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 层次聚类（观测）
- **对话框角色**: `variables`（数值变量（可多选））
- **algorithm_help purpose（对齐）**: 对多维观测做 complete linkage 凝聚层次聚类，并按 k 切簇。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### cluster_variables — 变量聚类

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 变量聚类
- **对话框角色**: `variables`（变量列）
- **algorithm_help purpose（对齐）**: 变量层次聚类：Pearson 相关距离、连结合并、dendrogram 与 amalgamation 表。
- **interpretation_limits**: 禁止与观测量聚类混读。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### cochran_q — Cochran Q 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Cochran Q 检验
- **对话框角色**: `variables`（≥3 列配对二元）
- **algorithm_help purpose（对齐）**: 配对二元、k≥3 处理的边际阳性率检验。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
相关样本多组二分类比例。

**不能当什么用**
仅适用二元响应。

**典型样本量**: k≥3 处理

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### correlation — 相关分析

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 相关分析
- **对话框角色**: `variables`（变量（至少两列））
- **algorithm_help purpose（对齐）**: 度量两列及以上数值的线性（Pearson）或单调（Spearman）相关，并给出协方差矩阵与可选偏相关。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
量化两列或多列数值的线性/单调关联强度。

**不能当什么用**
相关≠因果；不能预测规格合格。

**典型样本量**: n≥30

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### cox_counting_process — Cox 计数过程

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Cox 计数过程
- **对话框角色**: `start`（Start）
- **algorithm_help purpose（对齐）**: 计数过程形式 Cox 比例风险：区间 (Start,End] + Case ID + Efron ties + 稳健 VC。
- **interpretation_limits**: 禁止过程合格、已证明因果。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: 失效事件≥10

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 忽略删失。
- 混合失效模式。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Reliability](https://www.itl.nist.gov/div898/handbook/apr/apr.htm) — accessed 2026-09-03

### cox_regression — Cox 回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Cox 回归
- **对话框角色**: `time`（时间）
- **algorithm_help purpose（对齐）**: 固定协变量 Cox 比例风险：估计相对风险 HR。
- **interpretation_limits**: 禁止过程合格、已证明因果、寿命达标。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: 失效事件≥10

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 忽略删失。
- 混合失效模式。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Reliability](https://www.itl.nist.gov/div898/handbook/apr/apr.htm) — accessed 2026-09-03

### cross_tabulation — 交叉表

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 交叉表
- **对话框角色**: `row_category`（行分类列）
- **algorithm_help purpose（对齐）**: 独立交叉表：观察频数与行%/列%/合计%，不做卡方检验。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
列联表汇总与比例展示。

**不能当什么用**
仅汇总不检验时需另做卡方。

**典型样本量**: 视列联表大小

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### definitive_screening_design — 确定性筛选设计

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 确定性筛选设计
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: 4～12 连续因子；会议矩阵 D=[C;-C;0]；3 水平编码。
- **interpretation_limits**: 禁止宣称最优设计。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### descriptive — 显示描述性统计

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 显示描述性统计
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 概括一列或多列数值的位置、散布和形状，并可按 By 变量分组。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
汇总测量值中心、散布与形状，可按产线/模腔分组。

**不能当什么用**
不能判断稳定性或规格符合性。

**典型样本量**: n≥30；分组每组≥5

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### discriminant — 线性判别分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 线性判别分析
- **对话框角色**: `response`（类别响应）
- **algorithm_help purpose（对齐）**: 用等协方差线性判别对类别响应分类并报告混淆矩阵。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### distribution_calculator — 分布计算器

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 分布计算器
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: 正态/t/χ²/F/Weibull 的 PDF、CDF、分位数工具。
- **interpretation_limits**: 禁止分布已正态 / 过程合格。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
质量工具：多变异、抽样 OC、功效或分布计算器。

**不能当什么用**
不能替代 SPC 或 MSA 证据。

**典型样本量**: 视工具；计算器无需数据

**制造场景（列名示例）**
见共享数据集业务故事。

**常见误用**
- 抽样计划与批量不匹配。
- 功效分析假设 σ 错误。

**权威来源**
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### doe_bbd — Box–Behnken 设计

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Box–Behnken 设计
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: 生成 BBD：边中点 + 中心点；明确避开所有因素同时极端的角点。
- **interpretation_limits**: 解释层只陈述设计结构。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### doe_ccd — 中心复合设计 CCD

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 中心复合设计 CCD
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: 生成 CCD（CCC/CCI/CCF）设计矩阵：立方点、星点、中心点；输出 coded/actual 与可行性诊断。
- **interpretation_limits**: 解释层只陈述设计结构，不写过程已优化。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### doe_d_optimal — D-最优设计

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → D-最优设计
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: 在 2^k 全因子候选集中用 Fedorov 交换选取 n 个运行点，最大化 det(X'X)。
- **interpretation_limits**: 解释层只陈述设计结构与 det 指标，不写预测质量已最优。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### doe_factorial — 2 水平析因设计生成

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 2 水平析因设计生成
- **对话框角色**: `response`（响应列（可选，选择后进行响应分析））
- **algorithm_help purpose（对齐）**: 生成 2^k 全因子或 2^(k-p) 部分析因设计矩阵，也可对已有设计表做响应分析。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### doe_plackett_burman — Plackett–Burman 设计生成

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Plackett–Burman 设计生成
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: 生成 Plackett–Burman 两水平筛选设计矩阵。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### doe_response — DOE 响应分析

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → DOE 响应分析
- **对话框角色**: `response`（响应列（可选，选择后进行响应分析））
- **algorithm_help purpose（对齐）**: 对已有 2 水平设计表拟合响应。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### expanded_gage_unbalanced — 不平衡 Expanded Gage R&R

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 不平衡 Expanded Gage R&R
- **对话框角色**: `measurement`（测量）
- **algorithm_help purpose（对齐）**: 不平衡 Part×Operator（+可选附加因子）GLM 方差分量：VarComp、%Contribution、%Study Var、NDC。
- **interpretation_limits**: 禁止测量系统合格 / 过程已合格。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
评估测量系统变异是否掩盖产品/过程差异。

**不能当什么用**
不能证明量具通过或产品合格。

**典型样本量**: AIAG 10×3×2/3

**制造场景（列名示例）**
量具 MSA：`零件号`、`操作员`、`测量值_mm`

**常见误用**
- 零件未覆盖过程范围。
- 操作员培训不一致。

**建议 dataset_id**: `gage_rr_balance`

**权威来源**
- [AIAG — Measurement Systems Analysis (MSA) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/msa) — accessed 2026-09-03
- [Minitab Support — Gage R&R Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-r-r-study/) — accessed 2026-09-03

### factor_analysis — 因子分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 因子分析
- **对话框角色**: `variables`（变量列）
- **algorithm_help purpose（对齐）**: 探索性因子分析：相关阵主成分提取、Loadings、% Var、Communalities 与 Scree 图；可选 Varimax。
- **interpretation_limits**: 禁止宣称与 Minitab golden 对齐。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### fine_gray_regression — Fine-Gray 竞争风险回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Fine-Gray 竞争风险回归
- **对话框角色**: `time`（时间）
- **algorithm_help purpose（对齐）**: IPCW 加权 Fine-Gray 子分布风险回归：包装 fine_gray.cpp（binary/continuous/multi）。
- **interpretation_limits**: 禁止过程合格、已证明因果。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: 失效事件≥10

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 忽略删失。
- 混合失效模式。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Reliability](https://www.itl.nist.gov/div898/handbook/apr/apr.htm) — accessed 2026-09-03

### fisher_exact — Fisher 精确检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Fisher 精确检验
- **对话框角色**: `variables`（两列分类（2×2））
- **algorithm_help purpose（对齐）**: 对恰好 2×2 的分类交叉表做 Fisher 双侧精确检验，并报告可选优势比。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
小样本 2×2 精确检验。

**不能当什么用**
大表计算慢。

**典型样本量**: 小计数

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### friedman — Friedman 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Friedman 检验
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 区组设计下比较多个处理的位置（非参数重复测量）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
重复测量/区组非参数。

**不能当什么用**
区组内配对完整。

**典型样本量**: 区组≥10

**制造场景（列名示例）**
注塑：`模腔尺寸_mm`、`模腔`（1/2/3 穴）

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `anova_cavity`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### general_manova — General MANOVA

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → General MANOVA
- **对话框角色**: `responses`（响应）
- **algorithm_help purpose（对齐）**: 2～4 响应 + 1～2 因子 + 可选协变量；Type III SSCP；Wilks/Pillai/LH/Roy 按效应。
- **interpretation_limits**: 禁止过程已合格 / 已证明差异。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### glm_three_factor — 三因子 GLM

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 三因子 GLM
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 不平衡三因子连续响应：Type III Adj SS、Fitted Means；可选 AB/AC/BC 两两交互。
- **interpretation_limits**: 禁止过程已合格。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### glm_two_way — 双因子 GLM

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 双因子 GLM
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 不平衡双因子连续响应：Type III Adj SS、Fitted Means（回归预测按水平平均）与残差诊断。
- **interpretation_limits**: 禁止过程已合格 / 已优化。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### isolation_forest — Isolation Forest

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Isolation Forest
- **对话框角色**: `variables`（数值变量（可多选））
- **algorithm_help purpose（对齐）**: 对多维数值观测计算孤立分数，标记相对孤立点。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### km_interval — 区间删失 Kaplan–Meier

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 区间删失 Kaplan–Meier
- **对话框角色**: `left`（区间左端 L）
- **algorithm_help purpose（对齐）**: 对左/区间/右删失寿命数据用 Turnbull NPMLE 估计生存函数。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: 失效事件≥10

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 忽略删失。
- 混合失效模式。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Reliability](https://www.itl.nist.gov/div898/handbook/apr/apr.htm) — accessed 2026-09-03

### kmeans — K-Means 聚类

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → K-Means 聚类
- **对话框角色**: `variables`（数值变量（可多选））
- **algorithm_help purpose（对齐）**: 把多维数值观测分成 k 个簇，并报告质心与簇内平方和。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### kruskal_wallis — Kruskal-Wallis 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Kruskal-Wallis 检验
- **对话框角色**: `response`（测量值）
- **algorithm_help purpose（对齐）**: 不假设正态，比较 k 组位置。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
多组非参数位置比较。

**不能当什么用**
显著后需事后非参数比较。

**典型样本量**: 每组 n≥5

**制造场景（列名示例）**
注塑：`模腔尺寸_mm`、`模腔`（1/2/3 穴）

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `anova_cavity`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### life_data_lognormal — 寿命数据 Lognormal

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 寿命 Lognormal
- **对话框角色**: `time`（时间）
- **algorithm_help purpose（对齐）**: Lognormal MLE；0～2 协变量；右删失；回归表 + 百分位。
- **interpretation_limits**: 禁止可靠性合格承诺。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: 失效事件≥10

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 忽略删失。
- 混合失效模式。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Reliability](https://www.itl.nist.gov/div898/handbook/apr/apr.htm) — accessed 2026-09-03

### life_data_regression — 寿命数据回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 寿命数据回归
- **对话框角色**: `time`（时间）
- **algorithm_help purpose（对齐）**: Weibull / Lognormal / Exponential 寿命协变量回归：右删失 MLE、回归表与可选 1%/5% 百分位。
- **interpretation_limits**: 禁止寿命已达标 / 已证明稳定。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: 失效事件≥10

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 忽略删失。
- 混合失效模式。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Reliability](https://www.itl.nist.gov/div898/handbook/apr/apr.htm) — accessed 2026-09-03

### logistic_regression — 二元 Logistic 回归

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 二元 Logistic 回归
- **对话框角色**: `response`（二元响应）
- **algorithm_help purpose（对齐）**: 用对数几率模型预测二元事件概率。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
建模二分类响应与因子关系。

**不能当什么用**
不能证明因果；需足够事件数。

**典型样本量**: 事件≥10

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### mann_whitney — Mann-Whitney 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Mann-Whitney 检验
- **对话框角色**: `variables`（两列独立样本）
- **algorithm_help purpose（对齐）**: 不假设正态，比较两独立样本的位置。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
两独立组非参数位置比较。

**不能当什么用**
检验分布位置而非仅中位数（一般解释）。

**典型样本量**: 每组 n≥10

**制造场景（列名示例）**
光学膜：`膜厚_um`、`产线`（A/B 线）

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `two_line_thickness`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### manova_one_way — 单因子 MANOVA

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 单因子 MANOVA
- **对话框角色**: `responses`（响应）
- **algorithm_help purpose（对齐）**: 2～4 连续响应 + 1 分类因子；Wilks/Pillai/LH/Roy；H/E SSCP 与特征值。
- **interpretation_limits**: 禁止过程已合格 / 已证明差异。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### mcnemar — McNemar 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → McNemar 检验
- **对话框角色**: `variables`（两列配对二元结果）
- **algorithm_help purpose（对齐）**: 配对二元结果的边际比例检验（2×2 不一致对）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
配对二分类变化检验。

**不能当什么用**
仅 2×2 配对表。

**典型样本量**: discordant≥10

**制造场景（列名示例）**
装配返工：`返工前扭矩_Nm`、`返工后扭矩_Nm`、`工件号`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `paired_rework`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### mixed_effects_reml — 混合效应 REML

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 混合效应 REML
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 1～2 随机因子 + 1～2 固定因子 + 可选协变量；REML 方差分量与固定效应 BLUE。
- **interpretation_limits**: 禁止测量系统合格判定。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### mixture_analyze — Mixture 分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Mixture 分析
- **对话框角色**: `components`（分量列）
- **algorithm_help purpose（对齐）**: 对混料设计数据拟合 Scheffé 线性（可选二次）模型，输出完整 Terms 表、ANOVA 与残差；可选过程变量与 PV×组分交叉。无常数项。
- **interpretation_limits**: 禁止配方已优化 / 过程已合格。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### mixture_design — Mixture 设计

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Mixture 设计
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: 生成 simplex-lattice {q,m=2} 混料设计矩阵（q=3～4）并写入工作表。
- **interpretation_limits**: 禁止宣称配方已优化。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### mixture_extreme_vertices_design — 极端顶点混料设计

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 极端顶点混料设计
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: XVERT 窄化极端顶点混料设计。
- **interpretation_limits**: 解释层只陈述统计证据。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### mixture_process_variable — Mixture + 过程变量

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Mixture + 过程变量
- **对话框角色**: `components`（组分）
- **algorithm_help purpose（对齐）**: 2～4 组分 + 1 过程变量；Scheffé 线性/二次 + 过程项 + 可选组分×过程交互。
- **interpretation_limits**: 禁止过程已合格。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### mood_median — Mood 中位数检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Mood 中位数检验
- **对话框角色**: `response`（测量值）
- **algorithm_help purpose（对齐）**: 不假设正态，比较 k 组中位数（基于总体中位数的 2×k 表）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
多组中位数差异检验。

**不能当什么用**
对离群较稳健。

**典型样本量**: 每组 n≥10

**制造场景（列名示例）**
注塑：`模腔尺寸_mm`、`模腔`（1/2/3 穴）

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `anova_cavity`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### multiple_correspondence — 多重对应分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 多重对应分析
- **对话框角色**: `variables`（分类变量（逗号列号））
- **algorithm_help purpose（对齐）**: 3～6 列分类；指示矩阵；Column Contributions。
- **interpretation_limits**: 禁止全量 MCA 宣称。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### nhpp_repairable — 可修复系统 NHPP

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 可修复系统 NHPP
- **对话框角色**: `time`（累积失效时间）
- **algorithm_help purpose（对齐）**: 对累积失效时间拟合 Crow–AMSAA 幂律 NHPP，估计 β、λ 并给出强度/累积均值表与可选 Duane 图。
- **interpretation_limits**: 禁止 ROCOF 合格 / 已证明稳定。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: 失效事件≥10

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 忽略删失。
- 混合失效模式。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Reliability](https://www.itl.nist.gov/div898/handbook/apr/apr.htm) — accessed 2026-09-03

### nominal_logistic — 名义 Logistic 回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 名义 Logistic 回归
- **对话框角色**: `response`（名义响应）
- **algorithm_help purpose（对齐）**: 对名义（无序）多水平响应拟合广义 logit 模型。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### nonlinear_regression — 非线性回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 非线性回归
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 单 Y + 单 X；内置模型；GN/LM；参数表 + Summary of Fit。
- **interpretation_limits**: 禁止外推无验证。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### normality_test — 正态性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 正态性检验
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 用 Anderson–Darling 或 Ryan–Joiner 检验一列数据是否与正态分布一致。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
评估数据与正态假设偏离，为参数/非参数选型提供证据。

**不能当什么用**
未拒绝≠证明正态；小样本功效低。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### one_poisson_rate — 单样本泊松率

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单样本泊松率
- **对话框角色**: `defects`（缺陷数）
- **algorithm_help purpose（对齐）**: 检验缺陷发生速率 λ 是否等于假设值。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
估计单位长度/面积缺陷发生率。

**不能当什么用**
需明确观测长度/面积。

**典型样本量**: 缺陷总数≥10

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### one_proportion — 单比例检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单比例检验
- **对话框角色**: `events`（事件数）
- **algorithm_help purpose（对齐）**: 检验事件比例是否等于假设 p₀。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
估计/检验单一事件比例。

**不能当什么用**
不能代表多阶段过程整体不良。

**典型样本量**: 事件数≥5

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### one_proportion_equivalence — 单比例等价性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单比例等价性检验
- **对话框角色**: `events`（事件数）
- **algorithm_help purpose（对齐）**: 检验样本比例相对目标比例的差是否落入等价界限（Wald z-TOST）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
证明效应落在事前等价区间内（生物等效/工艺窗口）。

**不能当什么用**
不能证明完全无差异；等价限需法规/工程协议。

**典型样本量**: 功效驱动，常 n≥20/组

**制造场景（列名示例）**
光学膜：`膜厚_um`、`产线`（A/B 线）

**常见误用**
- 等价限事后挑选。
- 忽略方差估计方法。

**建议 dataset_id**: `two_line_thickness`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### one_sample_equivalence — 单样本等价性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单样本等价性检验
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 检验均值是否落入规定等价界限内（TOST），而不是“是否等于目标”。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
证明效应落在事前等价区间内（生物等效/工艺窗口）。

**不能当什么用**
不能证明完全无差异；等价限需法规/工程协议。

**典型样本量**: 功效驱动，常 n≥20/组

**制造场景（列名示例）**
光学膜：`膜厚_um`、`产线`（A/B 线）

**常见误用**
- 等价限事后挑选。
- 忽略方差估计方法。

**建议 dataset_id**: `two_line_thickness`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### one_sample_t — 单样本 t 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单样本 t 检验
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 检验一列均值是否等于给定 μ₀。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
检验样本均值是否偏离假设目标值。

**不能当什么用**
不能证明过程长期受控。

**典型样本量**: n≥15

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### one_sample_z — 单样本 Z 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单样本 Z 检验
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 在已知总体标准差 σ 时，检验一列均值是否等于给定 μ₀。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
已知 σ 时检验均值；或大样本近似。

**不能当什么用**
σ 估计错误则结论无效。

**典型样本量**: n≥30

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### one_way_anova — 单因素 ANOVA

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单因素 ANOVA
- **对话框角色**: `response`（响应变量）
- **algorithm_help purpose（对齐）**: 检验多个组的均值是否全部相等。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
比较≥3 组均值是否全等。

**不能当什么用**
显著后需事后比较；残差需检查。

**典型样本量**: 每水平 n≥5

**制造场景（列名示例）**
注塑：`模腔尺寸_mm`、`模腔`（1/2/3 穴）

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `anova_cavity`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### ordinal_logistic — 有序 Logistic 回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 有序 Logistic 回归
- **对话框角色**: `response`（有序响应）
- **algorithm_help purpose（对齐）**: 对有序多水平响应拟合比例优势 logit 模型。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### orthogonal_regression — 正交回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 正交回归
- **对话框角色**: `x`（X 列）
- **algorithm_help purpose（对齐）**: 在 X 与 Y 均有测量误差时估计等方差正交回归斜率与可选截距。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### outlier_test — 异常值检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 异常值检验
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 检验单个最极端观测是否能被看作异常值（Grubbs 或 Dixon r10）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
识别可能来自特殊原因的极端观测。

**不能当什么用**
统计离群≠必须删除；需工程确认。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### paired_equivalence — 配对等价性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 配对等价性检验
- **对话框角色**: `variables`（两列配对样本）
- **algorithm_help purpose（对齐）**: 检验配对差值是否落入规定等价界限内（Paired TOST）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
证明效应落在事前等价区间内（生物等效/工艺窗口）。

**不能当什么用**
不能证明完全无差异；等价限需法规/工程协议。

**典型样本量**: 功效驱动，常 n≥20/组

**制造场景（列名示例）**
装配返工：`返工前扭矩_Nm`、`返工后扭矩_Nm`、`工件号`

**常见误用**
- 等价限事后挑选。
- 忽略方差估计方法。

**建议 dataset_id**: `paired_rework`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### paired_t — 配对 t 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 配对 t 检验
- **对话框角色**: `variables`（配对变量（两列））
- **algorithm_help purpose（对齐）**: 对成对观测的差值做单样本 t，检验平均差值是否为 0。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
比较同一单元处理前后均值差。

**不能当什么用**
配对必须一一对应。

**典型样本量**: 配对 n≥15

**制造场景（列名示例）**
装配返工：`返工前扭矩_Nm`、`返工后扭矩_Nm`、`工件号`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `paired_rework`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### pca — 主成分分析

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 主成分分析
- **对话框角色**: `variables`（数值变量（可多选））
- **algorithm_help purpose（对齐）**: 把相关数值列换成互不相关的主成分。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### pls_regression — 偏最小二乘回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 偏最小二乘回归
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 1～4 响应 + 多预测；单响应 NIPALS 或 PLS2 NIPALS；LOO CV；Model Selection + Coefficients。
- **interpretation_limits**: 禁止 Minitab golden 对齐声明。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### poisson_gof — 泊松拟合优度

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 泊松拟合优度
- **对话框角色**: `counts`（计数列（非负整数））
- **algorithm_help purpose（对齐）**: 检验一列非负整数计数是否服从泊松分布（估计 λ=样本均值）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
检验计数是否符合 Poisson。

**不能当什么用**
过度离散需负二项等。

**典型样本量**: 总计数≥20

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### poisson_regression — Poisson 回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Poisson 回归
- **对话框角色**: `response`（计数响应）
- **algorithm_help purpose（对齐）**: 对非负计数响应拟合 log 链 Poisson GLM，并报告系数与偏差。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### probit_reliability — Probit 可靠性

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Probit 可靠性
- **对话框角色**: `events`（事件数）
- **algorithm_help purpose（对齐）**: 用 logit 链接拟合二项失效比例与应力/剂量关系，并估计 LD50。
- **interpretation_limits**: 解释层只陈述统计证据，不写产品已满足可靠性目标。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: 失效事件≥10

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 忽略删失。
- 混合失效模式。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Reliability](https://www.itl.nist.gov/div898/handbook/apr/apr.htm) — accessed 2026-09-03

### random_forest — 随机森林

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 随机森林
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: Bagging CART 集成：多数表决/均值预测与平均不纯度下降重要性。
- **interpretation_limits**: 解释层只陈述统计证据；禁用过程合格。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### randomization_test — 随机化检验（两样本均值差）

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 随机化检验（两样本均值差）
- **对话框角色**: `first`（样本 1）
- **algorithm_help purpose（对齐）**: 对两独立样本均值差做标签置换检验，给出双侧 P 值。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### regression — 线性回归

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 线性回归
- **对话框角色**: `variables`（变量（第一列响应，其余为预测变量））
- **algorithm_help purpose（对齐）**: 用一个或多个预测变量拟合带截距的线性模型。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
建立预测因子与连续响应的关系。

**不能当什么用**
外推风险；残差诊断必备。

**典型样本量**: n≥10×参数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### reliability — 可靠性分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 可靠性分析（Kaplan-Meier / Weibull）
- **对话框角色**: `time`（寿命/时间）
- **algorithm_help purpose（对齐）**: 用寿命/删失数据估计生存函数或参数寿命分布。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: 失效事件≥10

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 忽略删失。
- 混合失效模式。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Reliability](https://www.itl.nist.gov/div898/handbook/apr/apr.htm) — accessed 2026-09-03

### reliability_test_plan — 可靠性试验计划

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 可靠性试验计划
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: Weibull 演示型试验计划：给定 β、R、CL、T0/tm 与允许失效数，计算样本量 n 与假设摘要。
- **interpretation_limits**: 禁止寿命已达标 / 过程已优化。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: 失效事件≥10

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 忽略删失。
- 混合失效模式。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Reliability](https://www.itl.nist.gov/div898/handbook/apr/apr.htm) — accessed 2026-09-03

### reliability_warranty — 保修摘要

- **implemented_status**: `command_only`
- **菜单路径（代码）**: **统计** → 保修摘要
- **对话框角色**: `exposure_col`（暴露量列（可选，列求和优先于标量））

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: 失效事件≥10

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 忽略删失。
- 混合失效模式。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Reliability](https://www.itl.nist.gov/div898/handbook/apr/apr.htm) — accessed 2026-09-03

### response_optimization — DOE 响应优化

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → DOE 响应优化
- **对话框角色**: `response`（响应列（可多选））
- **algorithm_help purpose（对齐）**: 在因子编码空间里找使一个或多个响应最接近目标的设置。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### rsm_response — 响应曲面分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 响应曲面分析
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 对已有连续因子设计表拟合二次响应曲面（线性+交互+纯二次）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### runs_test — 游程检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 游程检验
- **对话框角色**: `variables`（数值序列（一列））
- **algorithm_help purpose（对齐）**: 检验一列数值按行序相对比较准则 K 是否随机（Wald–Wolfowitz / Minitab Runs）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
检验序列随机性。

**不能当什么用**
不能定位特殊原因来源。

**典型样本量**: n≥25

**制造场景（列名示例）**
周度：`周次`、`良率_pct`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `ts_weekly_yield`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### seasonal_forecasting — 季节性预测（Winters）

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 季节性预测
- **对话框角色**: `value`（时间序列值）
- **algorithm_help purpose（对齐）**: 对单列时间顺序数据做季节性预测（Winters）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
分析时序趋势、季节、自相关或短期预测。

**不能当什么用**
预测区间≠规格合格；结构突变需重拟合。

**典型样本量**: ≥50 点识别季节

**制造场景（列名示例）**
周度：`周次`、`良率_pct`

**常见误用**
- 非平稳直接回归。
- 忽略异常点。

**建议 dataset_id**: `ts_weekly_yield`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### sign_test — 符号检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 符号检验
- **对话框角色**: `variables`（一列或两列配对）
- **algorithm_help purpose（对齐）**: 对单样本中位数或配对差做符号检验（二项精确），并报告中位数置信区间。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
配对符号检验，最稳健。

**不能当什么用**
功效低于 Wilcoxon。

**典型样本量**: n≥15

**制造场景（列名示例）**
装配返工：`返工前扭矩_Nm`、`返工后扭矩_Nm`、`工件号`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `paired_rework`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### simple_correspondence — 简单对应分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 简单对应分析
- **对话框角色**: `row_var`（行变量）
- **algorithm_help purpose（对齐）**: 2 列分类变量；列联表；惯性分解；行/列贡献；1～2 组件。
- **interpretation_limits**: 禁止因果结论。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### split_plot_analyze — 裂区析因分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 裂区析因分析
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 1 难改因子 + 1～2 易改因子；WP/SP 双误差 ANOVA；whole-plot residuals。
- **interpretation_limits**: 禁止设计已最优 / 过程已合格。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### split_plot_design — 2 水平裂区设计

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 2 水平裂区设计
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: 2～4 因子、1 HTC；设计矩阵 + Whole plot 列。
- **interpretation_limits**: 禁止宣称最优设计。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### stepwise_regression — 逐步回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 逐步回归
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 按 α 或 Forward AICc/BIC 对线性回归候选预测做逐步选择。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量降维、分类、聚类或重采样推断。

**不能当什么用**
黑箱/探索结果不能替代 DOE 因果验证。

**典型样本量**: 样本远大于特征数

**制造场景（列名示例）**
回流焊：`炉温_℃`、`焊点偏移_um`

**常见误用**
- 数据泄漏。
- 过拟合未交叉验证。

**建议 dataset_id**: `corr_temp_offset`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03

### t_power — t 功效与样本量

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → t 功效与样本量
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: 在给定效应、α 和功效目标下，估计样本量或可检测效应。mode 覆盖 t/ANOVA/比例/方差/泊松，以及等价 TOST、2 水平 DOE、正态容差样本量。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
质量工具：多变异、抽样 OC、功效或分布计算器。

**不能当什么用**
不能替代 SPC 或 MSA 证据。

**典型样本量**: 视工具；计算器无需数据

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 抽样计划与批量不匹配。
- 功效分析假设 σ 错误。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### taguchi_analyze — Taguchi 分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Taguchi 分析
- **对话框角色**: `factors`（因子）
- **algorithm_help purpose（对齐）**: 静态 Taguchi：对控制因子水平与外阵响应重复计算 Means/S/N 响应表（含 Delta/Rank）并绘制多水平主效应图。
- **interpretation_limits**: 禁止过程已优化 / 已合格 / 过程已稳定。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### taguchi_orthogonal_design — Taguchi 正交设计

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Taguchi 正交设计
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: 生成 Taguchi L8/L9/L12 正交设计矩阵并可写入工作表。
- **interpretation_limits**: 禁止宣称已优化工艺。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
结构化试验筛选/优化因子与响应关系。

**不能当什么用**
不能替代量产 SPC；外推需谨慎。

**典型样本量**: 2^k 或 RSM 3–5 水平

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 不随机化。
- 因子水平不现实。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [Montgomery — Design and Analysis of Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714611) — accessed 2026-09-03
- [Minitab Support — Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

### time_series_decomposition — 时间序列分解

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 时间序列分解
- **对话框角色**: `time`（时间列（可选））
- **algorithm_help purpose（对齐）**: 对单列时间顺序数据做时间序列分解。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
分析时序趋势、季节、自相关或短期预测。

**不能当什么用**
预测区间≠规格合格；结构突变需重拟合。

**典型样本量**: ≥50 点识别季节

**制造场景（列名示例）**
周度：`周次`、`良率_pct`

**常见误用**
- 非平稳直接回归。
- 忽略异常点。

**建议 dataset_id**: `ts_weekly_yield`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### time_series_smoothing — 指数平滑

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 时间序列平滑
- **对话框角色**: `variables`（时间序列）
- **algorithm_help purpose（对齐）**: 对单列时间顺序数据做指数平滑。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
分析时序趋势、季节、自相关或短期预测。

**不能当什么用**
预测区间≠规格合格；结构突变需重拟合。

**典型样本量**: ≥50 点识别季节

**制造场景（列名示例）**
周度：`周次`、`良率_pct`

**常见误用**
- 非平稳直接回归。
- 忽略异常点。

**建议 dataset_id**: `ts_weekly_yield`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### trend_analysis — 趋势分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 趋势分析
- **对话框角色**: `time`（时间列（可选））
- **algorithm_help purpose（对齐）**: 对时间顺序数值序列估计线性趋势，可选 Mann-Kendall 单调趋势检验。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格或规格已满足。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
分析时序趋势、季节、自相关或短期预测。

**不能当什么用**
预测区间≠规格合格；结构突变需重拟合。

**典型样本量**: ≥50 点识别季节

**制造场景（列名示例）**
周度：`周次`、`良率_pct`

**常见误用**
- 非平稳直接回归。
- 忽略异常点。

**建议 dataset_id**: `ts_weekly_yield`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### two_factor_anova — 双因素 ANOVA

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 双因素 ANOVA
- **对话框角色**: `response`（响应变量）
- **algorithm_help purpose（对齐）**: 把响应变异分解为因子 A、因子 B 和交互作用。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
评估两因子主效应与交互。

**不能当什么用**
不能外推未试验水平。

**典型样本量**: 每单元≥3 重复

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `doe_factorial_demo`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### two_poisson_rate — 双样本泊松率

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 双样本泊松率
- **对话框角色**: `first_events`（第一组缺陷数）
- **algorithm_help purpose（对齐）**: 比较两组泊松率（差值或率比）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
比较两组 Poisson 发生率。

**不能当什么用**
长度单位需一致。

**典型样本量**: 每组缺陷≥5

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### two_proportion_equivalence — 两比例等价性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 两比例等价性检验
- **对话框角色**: `first_events`（第一组事件数）
- **algorithm_help purpose（对齐）**: 检验两组比例差是否落入等价界限（Wald z-TOST）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
证明效应落在事前等价区间内（生物等效/工艺窗口）。

**不能当什么用**
不能证明完全无差异；等价限需法规/工程协议。

**典型样本量**: 功效驱动，常 n≥20/组

**制造场景（列名示例）**
光学膜：`膜厚_um`、`产线`（A/B 线）

**常见误用**
- 等价限事后挑选。
- 忽略方差估计方法。

**建议 dataset_id**: `two_line_thickness`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### two_proportions — 两比例检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 两比例检验
- **对话框角色**: `first_events`（第一组事件数）
- **algorithm_help purpose（对齐）**: 比较两组事件比例之差。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
比较两组比例差异。

**不能当什么用**
样本独立；小计数用精确法。

**典型样本量**: 每组 n≥30

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### two_sample_equivalence — 双样本等价性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 双样本等价性检验
- **对话框角色**: `variables`（两列独立样本）
- **algorithm_help purpose（对齐）**: 检验两组均值差是否落入等价界限。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
证明效应落在事前等价区间内（生物等效/工艺窗口）。

**不能当什么用**
不能证明完全无差异；等价限需法规/工程协议。

**典型样本量**: 功效驱动，常 n≥20/组

**制造场景（列名示例）**
光学膜：`膜厚_um`、`产线`（A/B 线）

**常见误用**
- 等价限事后挑选。
- 忽略方差估计方法。

**建议 dataset_id**: `two_line_thickness`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### two_sample_equivalence_ratio — 双样本均值比等价性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 双样本均值比等价性检验
- **对话框角色**: `variables`（检验列 + 参考列）
- **algorithm_help purpose（对齐）**: 检验两组均值比值 ρ=μ_test/μ_ref 是否落入比值等价界限（TOST）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
证明效应落在事前等价区间内（生物等效/工艺窗口）。

**不能当什么用**
不能证明完全无差异；等价限需法规/工程协议。

**典型样本量**: 功效驱动，常 n≥20/组

**制造场景（列名示例）**
光学膜：`膜厚_um`、`产线`（A/B 线）

**常见误用**
- 等价限事后挑选。
- 忽略方差估计方法。

**建议 dataset_id**: `two_line_thickness`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### two_sample_t — 双样本 t 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 双样本 t 检验
- **对话框角色**: `variables`（两列独立样本）
- **algorithm_help purpose（对齐）**: 比较两组均值差是否为 0。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
比较两独立组均值差异。

**不能当什么用**
组间非独立时不可用；不等方差选 Welch。

**典型样本量**: 每组 n≥15

**制造场景（列名示例）**
光学膜：`膜厚_um`、`产线`（A/B 线）

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `two_line_thickness`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### variance_test — 等方差检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 方差检验
- **对话框角色**: `first`（第一样本 / 测量列）
- **algorithm_help purpose（对齐）**: 检验两组或多组方差/标准差是否相等。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
比较两组或多样本方差是否齐性。

**不能当什么用**
对非正态敏感；小样本功效低。

**典型样本量**: 每组 n≥10

**制造场景（列名示例）**
光学膜：`膜厚_um`、`产线`（A/B 线）

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `two_line_thickness`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### weibayes — Weibayes

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Weibayes
- **对话框角色**: `time`（时间）
- **algorithm_help purpose（对齐）**: 固定形状先验 β 的 Weibull 特征寿命估计（少失效窄化）。
- **interpretation_limits**: 禁止寿命已达标 / 过程合格。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: 失效事件≥10

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- 忽略删失。
- 混合失效模式。

**建议 dataset_id**: `reliability_cycles`

**权威来源**
- [NIST/SEMATECH e-Handbook — Reliability](https://www.itl.nist.gov/div898/handbook/apr/apr.htm) — accessed 2026-09-03

### wilcoxon_signed_rank — Wilcoxon 符号秩检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Wilcoxon 符号秩检验
- **对话框角色**: `variables`（一列或两列配对）
- **algorithm_help purpose（对齐）**: 对配对差或单样本中位数做符号秩检验。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
配对非参数差值检验。

**不能当什么用**
差值分布需近似对称。

**典型样本量**: n≥15

**制造场景（列名示例）**
装配返工：`返工前扭矩_Nm`、`返工后扭矩_Nm`、`工件号`

**常见误用**
- 把 p 值当成工程决策唯一依据。
- 忽略测量系统噪声。

**建议 dataset_id**: `paired_rework`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

## 质量工具

### acceptance_sampling — 属性一次抽样

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **质量工具** → 属性一次抽样
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: 按样本量 n 与接收数 c 给出二项 OC 曲线与可选 AQL/RQL 风险点。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
质量工具：多变异、抽样 OC、功效或分布计算器。

**不能当什么用**
不能替代 SPC 或 MSA 证据。

**典型样本量**: 视工具；计算器无需数据

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 抽样计划与批量不匹配。
- 功效分析假设 σ 错误。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### attribute_agreement — 属性一致性分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **质量工具** → 属性一致性分析
- **对话框角色**: `rating`（评级）
- **algorithm_help purpose（对齐）**: 评估多个评价人对属性等级的一致性。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
评估测量系统变异是否掩盖产品/过程差异。

**不能当什么用**
不能证明量具通过或产品合格。

**典型样本量**: AIAG 10×3×2/3

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 零件未覆盖过程范围。
- 操作员培训不一致。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [AIAG — Measurement Systems Analysis (MSA) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/msa) — accessed 2026-09-03
- [Minitab Support — Gage R&R Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-r-r-study/) — accessed 2026-09-03

### batch_capability — 批次过程能力

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **质量工具** → 批次过程能力
- **对话框角色**: `measurement`（测量值）
- **algorithm_help purpose（对齐）**: 按批次列分组，逐批计算正态过程能力指标。
- **interpretation_limits**: 禁止批次合格/过程已稳定等结论性措辞。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
批次间能力评估。

**不能当什么用**
失控或未代表样本时指数无预测意义；不写过程合格。

**典型样本量**: ≥30 独立观测

**制造场景（列名示例）**
光学膜：`膜厚_um`、`产线`（A/B 线）

**常见误用**
- 未先验证受控。
- 规格限设错。

**建议 dataset_id**: `two_line_thickness`

**权威来源**
- [Minitab Support — Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03

### between_within_capability — 组间/组内过程能力

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → 组间/组内过程能力
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 同时估计组内与组间变异，并用 σ_BW 计算 Cp/Cpk。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
分解组间/组内变异的能力。

**不能当什么用**
失控或未代表样本时指数无预测意义；不写过程合格。

**典型样本量**: ≥30 独立观测

**制造场景（列名示例）**
注塑：`模腔尺寸_mm`、`模腔`（1/2/3 穴）

**常见误用**
- 未先验证受控。
- 规格限设错。

**建议 dataset_id**: `anova_cavity`

**权威来源**
- [Minitab Support — Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03

### binomial_capability — 二项过程能力

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → 二项过程能力
- **对话框角色**: `defectives`（不合格品数）
- **algorithm_help purpose（对齐）**: 用不合格品率和检验数评估属性过程（二项）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
二项不良率能力。

**不能当什么用**
失控或未代表样本时指数无预测意义；不写过程合格。

**典型样本量**: ≥30 独立观测

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 未先验证受控。
- 规格限设错。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [Minitab Support — Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03

### box_cox — Box-Cox 变换

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → Box-Cox 变换
- **对话框角色**: `variables`（正值变量）
- **algorithm_help purpose（对齐）**: 寻找使正值数据更接近常数方差/近似正态的幂变换。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
Box-Cox 变换辅助正态化。

**不能当什么用**
失控或未代表样本时指数无预测意义；不写过程合格。

**典型样本量**: ≥30 独立观测

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 未先验证受控。
- 规格限设错。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [Minitab Support — Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03

### capability — 正态过程能力

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → 正态过程能力
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 在正态与稳定假设未验证的前提下，用规格限计算 Cp/Cpk（组内 σ）和 Pp/Ppk（总体 s）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
估计 Cp/Cpk 等相对规格的能力指数。

**不能当什么用**
失控或未代表样本时指数无预测意义；不写过程合格。

**典型样本量**: ≥30 独立观测

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 未先验证受控。
- 规格限设错。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [Minitab Support — Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03

### capability_sixpack — 过程能力 Sixpack

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → 过程能力 Sixpack
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 在一页里同时看稳定性、正态性和能力。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
能力六合一图形诊断。

**不能当什么用**
失控或未代表样本时指数无预测意义；不写过程合格。

**典型样本量**: ≥30 独立观测

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 未先验证受控。
- 规格限设错。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [Minitab Support — Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03

### cause_and_effect — 因果图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → 因果图
- **对话框角色**: `category`（类别列）
- **algorithm_help purpose（对齐）**: 按类别汇总原因条目，输出结构摘要与类别计数图（鱼骨/因果头脑风暴）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### distribution_identification — 个体分布识别

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → 个体分布识别
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 比较正态、Weibull、对数正态、指数二参数的 Anderson–Darling，辅助选分布。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
拟合候选分布识别。

**不能当什么用**
失控或未代表样本时指数无预测意义；不写过程合格。

**典型样本量**: ≥30 独立观测

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 未先验证受控。
- 规格限设错。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [Minitab Support — Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03

### emp_crossed — EMP Crossed

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → EMP Crossed
- **对话框角色**: `measurement`（测量值）
- **algorithm_help purpose（对齐）**: Wheeler EMP Crossed：用交叉 Gage 方差分量做 ICC 分级。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
评估测量系统变异是否掩盖产品/过程差异。

**不能当什么用**
不能证明量具通过或产品合格。

**典型样本量**: AIAG 10×3×2/3

**制造场景（列名示例）**
量具 MSA：`零件号`、`操作员`、`测量值_mm`

**常见误用**
- 零件未覆盖过程范围。
- 操作员培训不一致。

**建议 dataset_id**: `gage_rr_balance`

**权威来源**
- [AIAG — Measurement Systems Analysis (MSA) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/msa) — accessed 2026-09-03
- [Minitab Support — Gage R&R Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-r-r-study/) — accessed 2026-09-03

### expanded_gage_rr — Expanded Gage R&R

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → Expanded Gage R&R
- **对话框角色**: `measurement`（测量值）
- **algorithm_help purpose（对齐）**: Expanded Gage：平衡 Part×Operator×附加因子三因子随机 ANOVA。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
评估测量系统变异是否掩盖产品/过程差异。

**不能当什么用**
不能证明量具通过或产品合格。

**典型样本量**: AIAG 10×3×2/3

**制造场景（列名示例）**
量具 MSA：`零件号`、`操作员`、`测量值_mm`

**常见误用**
- 零件未覆盖过程范围。
- 操作员培训不一致。

**建议 dataset_id**: `gage_rr_balance`

**权威来源**
- [AIAG — Measurement Systems Analysis (MSA) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/msa) — accessed 2026-09-03
- [Minitab Support — Gage R&R Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-r-r-study/) — accessed 2026-09-03

### gage_rr — Crossed Gage R&R

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → Crossed Gage R&R
- **对话框角色**: `measurement`（测量值）
- **algorithm_help purpose（对齐）**: 交叉设计下把测量变异分解为重复性、再现性与零件。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
评估测量系统变异是否掩盖产品/过程差异。

**不能当什么用**
不能证明量具通过或产品合格。

**典型样本量**: AIAG 10×3×2/3

**制造场景（列名示例）**
量具 MSA：`零件号`、`操作员`、`测量值_mm`

**常见误用**
- 零件未覆盖过程范围。
- 操作员培训不一致。

**建议 dataset_id**: `gage_rr_balance`

**权威来源**
- [AIAG — Measurement Systems Analysis (MSA) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/msa) — accessed 2026-09-03
- [Minitab Support — Gage R&R Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-r-r-study/) — accessed 2026-09-03

### msa_type1 — MSA Type 1 / Bias / Stability

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → MSA Type 1 / Bias / Stability
- **对话框角色**: `measurement`（测量值）
- **algorithm_help purpose（对齐）**: 对标准件评估偏倚、线性与 Type 1 Cg/Cgk；Stability 看重复测量是否漂移。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
评估测量系统变异是否掩盖产品/过程差异。

**不能当什么用**
不能证明量具通过或产品合格。

**典型样本量**: AIAG 10×3×2/3

**制造场景（列名示例）**
量具 MSA：`零件号`、`操作员`、`测量值_mm`

**常见误用**
- 零件未覆盖过程范围。
- 操作员培训不一致。

**建议 dataset_id**: `gage_rr_balance`

**权威来源**
- [AIAG — Measurement Systems Analysis (MSA) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/msa) — accessed 2026-09-03
- [Minitab Support — Gage R&R Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-r-r-study/) — accessed 2026-09-03

### multi_vari — Multi-Vari 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → Multi-Vari 图
- **对话框角色**: `measurement`（测量值）
- **algorithm_help purpose（对齐）**: 用 2～4 个因子展示测量值在各层的位置与散布。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
质量工具：多变异、抽样 OC、功效或分布计算器。

**不能当什么用**
不能替代 SPC 或 MSA 证据。

**典型样本量**: 视工具；计算器无需数据

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 抽样计划与批量不匹配。
- 功效分析假设 σ 错误。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### nested_gage_rr — Nested Gage R&R

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → Nested Gage R&R
- **对话框角色**: `measurement`（测量值）
- **algorithm_help purpose（对齐）**: 零件嵌套在操作者下的 Gage 研究。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
评估测量系统变异是否掩盖产品/过程差异。

**不能当什么用**
不能证明量具通过或产品合格。

**典型样本量**: AIAG 10×3×2/3

**制造场景（列名示例）**
量具 MSA：`零件号`、`操作员`、`测量值_mm`

**常见误用**
- 零件未覆盖过程范围。
- 操作员培训不一致。

**建议 dataset_id**: `gage_rr_balance`

**权威来源**
- [AIAG — Measurement Systems Analysis (MSA) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/msa) — accessed 2026-09-03
- [Minitab Support — Gage R&R Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-r-r-study/) — accessed 2026-09-03

### nonnormal_capability — 非正态过程能力

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **质量工具** → 非正态过程能力
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 在 Weibull 或对数正态假设下用 Z 分数计算 overall Pp/Ppk。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
非正态数据能力（如 Johnson/百分位）。

**不能当什么用**
失控或未代表样本时指数无预测意义；不写过程合格。

**典型样本量**: ≥30 独立观测

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 未先验证受控。
- 规格限设错。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [Minitab Support — Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03

### nonparametric_capability — 非参数过程能力

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **质量工具** → 非参数过程能力
- **对话框角色**: `measurement`（测量值）
- **algorithm_help purpose（对齐）**: 用经验分位数估计 Cnp/Cnpk，不假设特定分布。
- **interpretation_limits**: 禁止单独作为过程合格结论。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
非参数百分位能力。

**不能当什么用**
失控或未代表样本时指数无预测意义；不写过程合格。

**典型样本量**: ≥30 独立观测

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 未先验证受控。
- 规格限设错。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [Minitab Support — Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03

### pareto — 柏拉图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → 柏拉图
- **对话框角色**: `category`（缺陷类别）
- **algorithm_help purpose（对齐）**: 按类别频数从大到小排序，并画累积百分比。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### poisson_capability — 泊松过程能力

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → 泊松过程能力
- **对话框角色**: `defects`（缺陷数）
- **algorithm_help purpose（对齐）**: 用单位缺陷数评估泊松过程。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
Poisson 缺陷率能力。

**不能当什么用**
失控或未代表样本时指数无预测意义；不写过程合格。

**典型样本量**: ≥30 独立观测

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- 未先验证受控。
- 规格限设错。

**建议 dataset_id**: `attribute_defect`

**权威来源**
- [Minitab Support — Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03

### run_chart — 运行图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → 运行图
- **对话框角色**: `variables`（数值观测（一列））
- **algorithm_help purpose（对齐）**: 按行序画个体运行图（中位数中心线），并给出聚类/混合/趋势/振荡四模式近似 P。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
图形本身不提供显著性（需配套检验）。

**典型样本量**: n≥20

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 过度解读偶然模式。
- 坐标截断误导。

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [NIST/SEMATECH e-Handbook — EDA](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03

### tolerance_intervals — 容差区间

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **质量工具** → 容差区间
- **对话框角色**: `measurement`（测量值）
- **algorithm_help purpose（对齐）**: 给出以指定置信覆盖总体至少 P 比例的区间。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
估计覆盖比例的总体区间。

**不能当什么用**
失控或未代表样本时指数无预测意义；不写过程合格。

**典型样本量**: ≥30 独立观测

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 未先验证受控。
- 规格限设错。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [Minitab Support — Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03

### variability_chart — 变异性图

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **质量工具** → 变异性图
- **对话框角色**: `measurement`（测量值）
- **algorithm_help purpose（对齐）**: 按 1～2 个因子展示各单元均值（含极差）与标准差双面板。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
质量工具：多变异、抽样 OC、功效或分布计算器。

**不能当什么用**
不能替代 SPC 或 MSA 证据。

**典型样本量**: 视工具；计算器无需数据

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- 抽样计划与批量不匹配。
- 功效分析假设 σ 错误。

**建议 dataset_id**: `smt_paste_height`

**权威来源**
- [AIAG — Statistical Process Control (SPC) Reference Manual](https://www.aiag.org/quality/automotive-core-tools/spc) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control (7th ed.)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+7th+Edition-p-9781119146816) — accessed 2026-09-03
