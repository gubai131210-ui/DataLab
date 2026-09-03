# DataLab 学习中心 — Agent A 调研笔记

> 生成日期：2026-09-03  
> 覆盖 id 数量：**184**（`analysis_commands::all()` ∪ `algorithm_help.json` entries）  
> 权威计划（只读）：`docs/research/goal-learning-center-black-belt-plan.md`  
> **2026-09-03 教学升级 / Wave-5**：旧 mapping **已作废**。权威：[`goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md`](goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md)；锁表：[`goal-learning-center-pedagogy-upgrade-wave-plan.md`](goal-learning-center-pedagogy-upgrade-wave-plan.md)；现行 mapping：[`learning-center-dataset-mapping.md`](learning-center-dataset-mapping.md)。下文 **用途 / 不能做什么 / 误用** 仍可抽查；**「建议 dataset_id」已对齐 v2 锁表**（空 = 本课无导入表）。

## 清单审计

- 并集条目数：184
- help-only：`database_import`, `report_templates`, `special_cause_rules`
- command-only：`reliability_warranty`
- 每条 ≥1 权威来源；控制图/图形类含模式识别与「不替代假设检验」说明

## 共享数据集（**已淘汰** — 仅考古；勿再生成）

| dataset_id | 场景 | 本 Goal |
|------------|------|---------|
| smt_paste_height | SMT 锡膏高度 | 删除重建 |
| two_line_thickness | 两产线膜厚 | 删除重建 |
| paired_rework | 返工前后 | 删除重建 |
| anova_cavity | 三模腔尺寸 | 删除重建 |
| corr_temp_offset | 温度 vs 偏移 | 删除重建 |
| attribute_defect | 班次不良 | 删除重建 |
| gage_rr_balance | 量具 R&R | 删除重建 |
| doe_factorial_demo | 析因/Taguchi/混料 | 删除重建 |
| reliability_cycles | 寿命循环 | 删除重建 |
| ts_weekly_yield | 周良率 | 删除重建 |

## 专用数据设计模式（2026-09-03 增补）

教学理由：共享宽表把无关列塞进工作记忆（CLT），且同一张失控表无法同时教 I-MR 与 Cpk。默认 `command_id` → 专用 `dataset_id`（**不要** `demo_` 前缀）。同构共享仅白名单。完整表见总册 §6。

| 族 | 建议新 dataset_id（草稿） | 埋点 | 对话框不要填 |
|----|---------------------------|------|----------------|
| I-MR | `imr_spi_shift` | 行 41 阶跃；行 55 尖峰 | 阶段列、历史限、Nelson estimate=1 |
| 双样本 t | `ttest_two_lines` | 两独立列均值差 | 不要用配对列 |
| Gage 交叉 | `gage_balance_10x3x3` | 10×3×3；一操作员偏倚 | 嵌套布局 |
| 正态能力 | `cap_stable_spec` | 近似稳定 + 对话框 USL/LSL | 不要用 imr 失控集 |
| Pareto | `pareto_defect_codes` | 少数类别主导累计 | 不要当控制图 |
| P 图 | `pchart_lot_defects` | 可变 n + 不合格率台阶 | 缺陷计数字段 |

**权威源勘误（2026-09-03）**：NIST `pmc/section4/pmc4.htm` 现为时间序列，不是 Gage；Pareto `pri/section3/pareto.htm` 重定向失效；Minitab Gage 路径改为 `crossed-gage-r-r-study`；two-sample 应引 `prc/section3/prc31.htm` 而非 `prc22`（单样本）。

---

## 图形

### area_plot — 面积图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 区域图
- **对话框角色**: `time`（顺序/时间变量）
- **algorithm_help purpose（对齐）**: 折线下方填色，强调累积或堆叠。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
时间或类别轴上展示累积量、构成变化（描述性模式）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 堆叠面积难比较非底层系列。

**典型样本量**: 时间点≥12；类别不宜过多。

**制造场景（列名示例）**
各缺陷类型月度堆叠面积，看构成是否变化。

**常见误用**
- 堆叠面积比较中间系列
- 与折线趋势混淆
- 纵轴非从零误导

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_area_time`

**权威来源**
- [NIST Run/Time Plot](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc44.htm) — accessed 2026-09-03
- [Minitab Area Graph](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/area-graphs/) — accessed 2026-09-03

### bar_chart — 条形图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 条形图
- **对话框角色**: `category`（类别）
- **algorithm_help purpose（对齐）**: 分类计数条形；保持出现顺序。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
展示分类计数或汇总均值/合计的比较（描述性模式观察）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 截断纵轴可夸大差异；连续变量不宜强行分箱条图代替直方图。

**典型样本量**: 类别级：每类至少 5 次观测；汇总条图依赖底层 n。

**制造场景（列名示例）**
三班次不良数条形图（计数），或各线体平均周期条图。

**常见误用**
- 纵轴不从零误导
- 用条图表现时间序列趋势（应折线）
- 混淆条图与直方图

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_bar_category`

**权威来源**
- [NIST Bar Chart](https://www.itl.nist.gov/div898/handbook/eda/section3/barplot.htm) — accessed 2026-09-03
- [Minitab Bar Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/bar-charts/) — accessed 2026-09-03

### boxplot — 箱线图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 箱线图
- **对话框角色**: `variables`（变量（可多选））
- **algorithm_help purpose（对齐）**: 用五数概括展示位置与散布：Q1、中位数、Q3，须线到最后未超出围栏的点，围栏外画为离群点。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
比较多组位置、散布与离群点；快速看中位数差异与对称性（模式观察）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 围栏外点不等于必须删除。

**典型样本量**: 每组 n≥5 有意义；n≥20 更稳；极小样本箱线不稳定。

**制造场景（列名示例）**
三模腔注塑件关键尺寸箱线图，比较腔间散布与中位偏移。

**常见误用**
- 把须线外点当测量错误必删
- 组间样本量悬殊仍只比中位数
- 与正态假设混为一谈

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_two_group_box`

**权威来源**
- [NIST Boxplot](https://www.itl.nist.gov/div898/handbook/eda/section3/boxplot.htm) — accessed 2026-09-03
- [Minitab Boxplot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/boxplots/) — accessed 2026-09-03

### bubble_plot — 气泡图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 气泡图
- **对话框角色**: `x_variable`（X 变量）
- **algorithm_help purpose（对齐）**: (x,y) 位置，气泡面积映射第三列。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
散点加第三维大小编码，观察三维关系模式。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 气泡面积感知非线性易误判。

**典型样本量**: n≥30；标签过多会重叠。

**制造场景（列名示例）**
工站节拍(x)、良率(y)、批量大小(气泡) 多维比较。

**常见误用**
- 气泡半径与数值非线性映射
- 重叠气泡难读
- 第三变量量纲未标准化

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_bubble_xyz`

**权威来源**
- [Minitab Bubble Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/bubble-plots/) — accessed 2026-09-03
- [NIST Multivariate Graphs](https://www.itl.nist.gov/div898/handbook/pmc/section1/pmc11.htm) — accessed 2026-09-03

### chi_square_mosaic_link — 卡方–马赛克联动

- **implemented_status**: `orchestration`
- **菜单路径（代码）**: **图形** → 卡方–马赛克联动
- **对话框角色**: `row_category`（行分类列）
- **algorithm_help purpose（对齐）**: 同一分类列组合输出卡方调整残差表/热图与马赛克图。
- **interpretation_limits**: 不写合格判定或因果结论。

**常用来做什么**
马赛克图与卡方检验联动：看图找模式，用检验量化关联。

**不能当什么用**
不能单靠图断言显著；期望频数过小检验失效。

**典型样本量**: 总 n≥50；2×2 至少每格≥5。

**制造场景（列名示例）**
操作员×合格/不合格马赛克+卡方，验证是否独立。

**常见误用**
- 只看图不做检验
- p<α 就当因果
- 忽略分层或 Simpson 悖论

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Chi-Square](https://www.itl.nist.gov/div898/handbook/prc/section4/prc45.htm) — accessed 2026-09-03
- [Minitab Chi-Square](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/chi-square-test-for-association/) — accessed 2026-09-03

### contour_plot — 等值线图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 等值线图
- **对话框角色**: `x_variable`（X 变量）
- **algorithm_help purpose（对齐）**: 在规则网格上对 z(x,y) 画等值线。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
两连续因子响应面等高线，看最优区域与坡度（DOE/回归可视化）。

**不能当什么用**
不外推实验域；不替代回归显著性检验；需足够设计点支撑曲面。

**典型样本量**: 响应面设计至少 13 点（CCD）或等价；因子 2–3 个常见。

**制造场景（列名示例）**
注塑温度×压力对缩痕深度的等高线图。

**常见误用**
- 在未试验区域外推最优
- 忽略拟合失拟
- 混淆相关等高线与因果

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_contour_xy`

**权威来源**
- [NIST Contour Plot](https://www.itl.nist.gov/div898/handbook/eda/section3/contour.htm) — accessed 2026-09-03
- [Minitab Contour Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/contour-plots/) — accessed 2026-09-03

### correlation_plot — 相关图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 相关图
- **对话框角色**: `variables`（变量（可多选））
- **algorithm_help purpose（对齐）**: 多变量两两散点矩阵，辅助看线性或非线性形态。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
矩阵散点或相关视图，浏览多变量两两关系模式。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 多重比较未校正；相关≠因果。

**典型样本量**: 每个变量对 n≥30；变量数 p 不宜过大（≤10 探索）。

**制造场景（列名示例）**
五条尺寸链测量相关图，找共变强的工序。

**常见误用**
- p 个变量看 p(p-1)/2 对却不校正
- 非线性关系只看 Pearson
- 忽略时间序列自相关

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_corr_matrix`

**权威来源**
- [NIST Correlation](https://www.itl.nist.gov/div898/handbook/eda/section3/scatter.htm) — accessed 2026-09-03
- [Minitab Matrix Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/matrix-plots/) — accessed 2026-09-03

### correlogram — Correlogram（相关热图）

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **图形** → Correlogram（相关热图）
- **对话框角色**: `variables`（数值变量（可多选））
- **algorithm_help purpose（对齐）**: 把多列两两相关矩阵画成热图并输出系数表。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多变量相关矩阵热图，总览线性关系强度（模式观察）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。

**典型样本量**: n≥30；变量≤15 可读。

**制造场景（列名示例）**
产线 10 个 KPI 相关热图找冗余指标。

**常见误用**
- 不标显著性
- 非线性关系误判为无关
- 时间序列伪相关

**图形解读要点**: 看分布/关系/趋势模式与离群；图形探索不替代假设检验。

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Correlation Matrix](https://www.itl.nist.gov/div898/handbook/eda/section3/scatter.htm) — accessed 2026-09-03
- [Minitab Correlogram](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/correlogram/) — accessed 2026-09-03

### density_plot — 密度图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 密度图
- **对话框角色**: `variable`（变量）
- **algorithm_help purpose（对齐）**: 高斯核密度估计曲线（Silverman 带宽）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
平滑展示连续变量概率密度形状，比较分布轮廓（模式观察）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 带宽选择影响形状。

**典型样本量**: n≥30；n≥100 密度估计更可靠。

**制造场景（列名示例）**
两供应商垫片厚度核密度对比，看尾部差异。

**常见误用**
- 带宽过小见假峰
- 多组样本量差大仍直接叠加
- 把密度峰值当过程目标

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_density_unimodal`

**权威来源**
- [NIST Density](https://www.itl.nist.gov/div898/handbook/eda/section3/density.htm) — accessed 2026-09-03
- [Minitab Density](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/density-plots/) — accessed 2026-09-03

### dotplot — 点图

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **图形** → 点图
- **对话框角色**: `y_variable`（Y 变量（可多选））
- **algorithm_help purpose（对齐）**: 一维分布点图，可选分组与 jitter。
- **interpretation_limits**: 解释层只陈述统计证据。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
小样本下一维分布点排列，看清每个测量值位置与重复（模式观察）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。

**典型样本量**: n=5–50 最理想；大样本点过密需 jitter 或改直方图。

**制造场景（列名示例）**
首件 10 点膜厚点图，确认是否集中在规格附近。

**常见误用**
- 大样本不 jitter 叠点误判
- 与统计意义上的点图（dot chart）混淆

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Dot Plot](https://www.itl.nist.gov/div898/handbook/eda/section3/dotplot.htm) — accessed 2026-09-03
- [ASQ Dot Plot](https://asq.org/quality-resources/dot-plot) — accessed 2026-09-03

### ecdf_plot — 经验累积分布图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 经验累积分布图
- **对话框角色**: `variable`（变量）
- **algorithm_help purpose（对齐）**: F̂(x)=#{y≤x}/n，右端为 1。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
经验累积分布，比较样本 CDF 或分位数位置（模式观察）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 阶梯线在小 n 下跳跃大。

**典型样本量**: n≥30；两组比较各≥30。

**制造场景（列名示例）**
两供应商垫片厚度 ECDF，看 99th 百分位谁更靠规格。

**常见误用**
- 小样本外推分位数
- 不与规格限对照就下结论
- 两组样本量差大误读交叉

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_ecdf_unimodal`

**权威来源**
- [NIST EDA CDF](https://www.itl.nist.gov/div898/handbook/eda/section3/eda33.htm) — accessed 2026-09-03
- [Minitab ECDF](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/empirical-cdf-plots/) — accessed 2026-09-03

### eda_4plot — EDA 四图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → EDA 四图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: NIST 单变量四图打包：run sequence、lag-1、histogram、normal probability。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
NIST 四图：运行序、lag-1、直方图、正态概率——单变量综合模式筛查。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 四图是筛查包，不替代完整控制图或检验。

**典型样本量**: n≥25；lag-1 需有序列顺序。

**制造场景（列名示例）**
量具重复性测量序列 EDA 四图，查独立性与正态性线索。

**常见误用**
- 忽略运行序图上的漂移
- 概率图略弯就删数据
- 未记录采样顺序

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_eda4_series`

**权威来源**
- [NIST 4-Plot](https://www.itl.nist.gov/div898/handbook/eda/section3/eda33.htm) — accessed 2026-09-03
- [NIST Lag Plot](https://www.itl.nist.gov/div898/handbook/eda/section3/lagplot.htm) — accessed 2026-09-03

### graph_gallery — 探索性图形画廊

- **implemented_status**: `graph_reference`
- **菜单路径（代码）**: **图形** → 探索性图形
- **对话框角色**: `x_variable`（X 变量）
- **algorithm_help purpose（对齐）**: 在相同列角色下切换 scatter/bar/box/histogram/dotplot 候选图型。
- **interpretation_limits**: 不写合格判定。

**常用来做什么**
可视化分布、关系或结构，支持 EDA 与沟通。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 不自动选最优图型。

**典型样本量**: 依所选图型；一般 n≥20 起。

**制造场景（列名示例）**
导入膜厚与温度两列，在画廊中切换散点与箱线做初探。

**常见误用**
- 只换图不查数据质量
- 多列角色未对齐
- 把预览当最终报告图

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST EDA Graphical](https://www.itl.nist.gov/div898/handbook/eda/section3/eda33.htm) — accessed 2026-09-03
- [Minitab Graph Gallery](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/graph-gallery/) — accessed 2026-09-03

### heatmap_plot — 热图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 热图
- **对话框角色**: `variables`（相关变量）
- **algorithm_help purpose（对齐）**: 用颜色表示表格或分箱密度。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
矩阵或网格上色展示相关、频次或响应强度模式。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 颜色尺度选择影响解读。

**典型样本量**: 相关阵：n≥30；格子频数表：每格足够计数。

**制造场景（列名示例）**
工位×缺陷类型发生次数热图。

**常见误用**
- 未标数值只靠颜色
- Rainbow 色图误导
- 时间轴排序错误

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_heatmap_matrix`

**权威来源**
- [NIST Heatmap/Contour](https://www.itl.nist.gov/div898/handbook/eda/section3/contour.htm) — accessed 2026-09-03
- [Minitab Heatmap](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/heatmaps/) — accessed 2026-09-03

### hexbin_plot — Hexbin

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → Hexbin
- **对话框角色**: `x_variable`（X 变量）
- **algorithm_help purpose（对齐）**: 二维矩形分箱计数着色（Binned Scatter）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
大样本二维关系密度热力分箱，看聚集区与稀疏区（模式观察）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 丢失单点细节；不适合小样本。

**典型样本量**: n≥500 才有意义；上万点常见。

**制造场景（列名示例）**
上万点 SPI 体积-面积散点 hexbin，定位焊膏过量聚集区。

**常见误用**
- 小样本用 hexbin 过度平滑
- 不检查坐标轴量纲
- 把颜色深浅当因果

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_hexbin_xy`

**权威来源**
- [Matplotlib Hexbin](https://matplotlib.org/stable/api/_as_gen/matplotlib.pyplot.hexbin.html) — accessed 2026-09-03
- [NIST Scatter](https://www.itl.nist.gov/div898/handbook/eda/section3/scatter.htm) — accessed 2026-09-03

### histogram — 直方图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 直方图
- **对话框角色**: `variables`（变量（可多选））
- **algorithm_help purpose（对齐）**: 把数值分成若干区间，用矩形高度表示频数或密度。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
探索性观察单变量分布形态、偏度、多峰与潜在离群；模式查看第一步。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 叠加正态曲线不是正态性检验。

**典型样本量**: n≥30 可看大致形状；n≥100 分箱更稳定；小样本仅作线索。

**制造场景（列名示例）**
SMT 锡膏印刷高度（μm）按班次直方图，初判是否右偏或双峰。

**常见误用**
- 分箱宽度随意改后过度解读峰形
- 把直方图当能力合格证明
- 多组叠加不分面导致误读

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_hist_prob`

**权威来源**
- [NIST EDA 直方图](https://www.itl.nist.gov/div898/handbook/eda/section3/histogm.htm) — accessed 2026-09-03
- [Minitab Histogram](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/histograms/) — accessed 2026-09-03

### interval_plot — 区间图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 区间散点图
- **对话框角色**: `response`（响应变量）
- **algorithm_help purpose（对齐）**: 按组显示均值和置信区间。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
展示各组均值及置信区间，比较位置差异线索（可视化，非完整推断报告）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 区间重叠不等于无差异（需正式检验）。

**典型样本量**: 每组 n≥10；n≥30 区间更窄更稳。

**制造场景（列名示例）**
两固化炉平均硬度及 95% CI 区间图，辅助是否做双样本 t。

**常见误用**
- CI 重叠就断言无差异
- 不等方差仍用默认 CI
- 混淆预测区间与置信区间

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_interval_groups`

**权威来源**
- [Minitab Interval Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/interval-plots/) — accessed 2026-09-03
- [NIST CI](https://www.itl.nist.gov/div898/handbook/prc/section2/prc241.htm) — accessed 2026-09-03

### marginal_plot — 边际图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 边际图
- **对话框角色**: `x_variable`（X 变量）
- **algorithm_help purpose（对齐）**: 散点加上 x/y 边际直方图或箱线。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
散点图边缘叠加边际直方/密度，同时看联合与边缘分布。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。

**典型样本量**: n≥50 边际密度较稳。

**制造场景（列名示例）**
X-Y 尺寸配合散点+边缘分布，查边缘是否偏规格。

**常见误用**
- 只读中心散点忽略边缘多峰
- 分组边际未分色

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_marginal_xy`

**权威来源**
- [Minitab Marginal Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/marginal-plots/) — accessed 2026-09-03
- [NIST Bivariate EDA](https://www.itl.nist.gov/div898/handbook/eda/section3/eda33.htm) — accessed 2026-09-03

### matrix_plot — 矩阵图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 矩阵图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 所选数值列的两两散点。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
多变量散点矩阵，系统浏览两两关系与离群。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 变量过多图不可读。

**典型样本量**: n≥30；变量数≤8 为宜。

**制造场景（列名示例）**
注塑压力、温度、周期、重量四变量矩阵图。

**常见误用**
- 不查对角线分布就回归
- 多重比较无计划
- 时间序列未按序看

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_matrix_three`

**权威来源**
- [Minitab Matrix Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/matrix-plots/) — accessed 2026-09-03
- [NIST Scatterplot Matrix](https://www.itl.nist.gov/div898/handbook/eda/section3/scatter.htm) — accessed 2026-09-03

### mosaic_plot — 马赛克图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 马赛克图
- **对话框角色**: `categories`（分类列（2～3，可多选））
- **algorithm_help purpose（对齐）**: 2～3 个分类列的组合频数/比例可视化。
- **interpretation_limits**: 不写合格判定。

**常用来做什么**
两（多）分类变量列联表比例可视化，看关联模式。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 小期望频数格子不稳定；需卡方或 Fisher 确认。

**典型样本量**: 总 n≥50；每格期望≥5 较稳。

**制造场景（列名示例）**
缺陷类型×班次马赛克图，看某班次是否某缺陷偏高。

**常见误用**
- 稀疏表过度解读格子比例
- 把面积当因果
- 类别合并随意改变结论

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_mosaic_two_cat`

**权威来源**
- [Minitab Mosaic](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/mosaic-plots/) — accessed 2026-09-03
- [NIST Contingency](https://www.itl.nist.gov/div898/handbook/prc/section4/prc45.htm) — accessed 2026-09-03

### parallel_plot — 平行坐标图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 平行坐标图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 每个观测一条折线，轴为各变量（常做列内缩放）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
高维数值多变量平行坐标，看模式与簇（探索性）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 轴顺序影响观感；不适合报告因果。

**典型样本量**: n=50–500；维数 4–12 常见。

**制造场景（列名示例）**
多传感器过程参数平行坐标，找异常批次轨迹。

**常见误用**
- 维度过高线条糊成一团
- 未标准化不同量纲
- 把交叉当交互效应

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_parallel_multi`

**权威来源**
- [NIST Parallel Coordinates](https://www.itl.nist.gov/div898/handbook/eda/section3/parallel.htm) — accessed 2026-09-03
- [Minitab Parallel Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/parallel-plots/) — accessed 2026-09-03

### pie_plot — 饼图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 饼图
- **对话框角色**: `category`（分类变量）
- **algorithm_help purpose（对齐）**: 类别频数占合计的比例为扇区角。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
展示整体中各类占比（部分-整体，类别少时）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 类别>5 难读；不适合精确比较；时间趋势用折线更好。

**典型样本量**: 各类有足够计数；总 n≥30。

**制造场景（列名示例）**
不良四类占比饼图（类别≤4）。

**常见误用**
- 切片过多
- 3D 饼图失真
- 用饼图比较两组（用条图）

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_pie_category`

**权威来源**
- [NIST Pie Chart caution](https://www.itl.nist.gov/div898/handbook/eda/section3/piechart.htm) — accessed 2026-09-03
- [ASQ Pie Chart](https://asq.org/quality-resources/pie-chart) — accessed 2026-09-03

### probability_plot — 正态概率图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 概率图
- **对话框角色**: `variable`（变量）
- **algorithm_help purpose（对齐）**: 排序观测对 Blom 正态分位。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
正态（或其它分布）概率图，目视判断分布拟合与离群（模式观察）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 Anderson-Darling 等检验更正式；概率图不是能力指数。

**典型样本量**: n≥20 可看；n≥50 尾部更可靠。

**制造场景（列名示例）**
轴径正态概率图，决定用正态还是 Johnson 能力分析。

**常见误用**
- 点略弯就断言非正态
- 混用不同分布未说明
- 删点直到变直

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_hist_prob`

**权威来源**
- [NIST Normal Probability Plot](https://www.itl.nist.gov/div898/handbook/eda/section3/normprpl.htm) — accessed 2026-09-03
- [Minitab Probability Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/probability-plots/) — accessed 2026-09-03

### scatter_plot — 散点图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 散点图
- **对话框角色**: `x_variable`（X 变量）
- **algorithm_help purpose（对齐）**: 每个 complete-case 点画 (x,y)。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
观察两连续变量线性/非线性关系、离群与杠杆点（模式观察）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 相关≠因果；外推需谨慎。

**典型样本量**: n≥25 可见趋势；回归诊断建议 n≥30。

**制造场景（列名示例）**
回流炉区温度 vs 元件偏移量散点，筛查设定温度影响。

**常见误用**
- 把相关当因果
- 忽略异方差或非线性
- 离群点未追查就删除

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_scatter_xy`

**权威来源**
- [NIST Scatter](https://www.itl.nist.gov/div898/handbook/eda/section3/scatter.htm) — accessed 2026-09-03
- [Minitab Scatterplot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/scatterplots/) — accessed 2026-09-03

### simplex_design_plot — 混料三角图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 混料三角图
- **对话框角色**: `components`（分量列（3～4，可多选））
- **algorithm_help purpose（对齐）**: 在三角坐标系绘制混料设计点（proportions）。
- **interpretation_limits**: 不写合格判定。

**常用来做什么**
三元混料（如树脂/填料/助剂比例）在单纯形上的设计点与响应等高线（DOE 可视化）。

**不能当什么用**
非混料问题勿用；不替代混料回归分析；约束外推无效。

**典型样本量**: 混料设计 q 成分至少 q 个顶点+中心点；响应面需更多点。

**制造场景（列名示例）**
环氧封装胶 A/B/C 三组分配方单纯形图，标出抗拉强度等高线。

**常见误用**
- 成分和不为 1 仍用三角图
- 在可行域外预测
- 忽略工艺约束导致不可制造配方

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `mix_simplex_3`

**权威来源**
- [NIST Mixture Designs](https://www.itl.nist.gov/div898/handbook/pri/section5/pri532.htm) — accessed 2026-09-03
- [Cornell Mixture Experiments](https://www.wiley.com/en-us/Experiments+with+Mixtures%3A+Designs%2C+Models%2C+and+the+Analysis+of+Mixture+Data%2C+4th+Edition-p-9781118915172) — accessed 2026-09-03

### time_series_plot — 时间序列图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 时间序列图
- **对话框角色**: `time`（时间变量）
- **algorithm_help purpose（对齐）**: 按行序或时间列连接观测。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
按时间顺序看水平、趋势、季节与异常点（模式观察）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 不自动给出预测区间；非平稳序列直接回归有风险。

**典型样本量**: ≥24 点见季节；≥50 点趋势更可靠。

**制造场景（列名示例）**
周良率时间序列，查季节性低谷与趋势。

**常见误用**
- 忽略抽样间隔不等
- 不查自相关就做回归
- 把单次尖峰当永久改型

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `ts_weekly_yield_series`

**权威来源**
- [NIST Time Series](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc44.htm) — accessed 2026-09-03
- [Minitab Time Series Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/time-series-plots/) — accessed 2026-09-03

### violin_plot — 小提琴图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **图形** → 小提琴图
- **对话框角色**: `variables`（变量（可多选））
- **algorithm_help purpose（对齐）**: 分组镜像 KDE + 箱线五数。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
同时看分布形状与四分位，比较多组密度（模式观察）。

**不能当什么用**
不替代正式假设检验、过程能力判定或控制图判稳；图形证据需结合专门分析确认。 小样本密度尾部不可靠。

**典型样本量**: 每组 n≥20 较稳；n<10 慎用。

**制造场景（列名示例）**
四工位扭矩分布小提琴图，比较形状而不只看均值。

**常见误用**
- 样本极小仍解读尾部形状
- 与箱线图刻度不一致误比
- 忽略组内相关性

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `graph_violin_groups`

**权威来源**
- [Minitab Violin](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/violin-plots/) — accessed 2026-09-03
- [NIST EDA](https://www.itl.nist.gov/div898/handbook/eda/section3/eda33.htm) — accessed 2026-09-03

## 帮助

### special_cause_rules — 特殊原因规则 Catalog

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: 帮助 > 算法
- **对话框角色**: _（无 analysis_commands 条目；见 help id）_
- **algorithm_help purpose（对齐）**: Minitab Tests 1–8 与 DataLab 图种适用表、默认策略说明。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
用于在 Shewhart 控制图上系统应用 Western Electric（WE）/Nelson 等判异规则，识别越 3σ、同侧运行、Zone A/B 聚集、趋势、交替、分层/混合等非随机模式；应明确所选规则集与误报 ARL 权衡，用于触发调查而非直接判“不合格”，且不能替代正式假设检验来确认偏移显著性。

**不能当什么用**
不能用于无控制限的 run chart 随意解读、不能把所有 8 条 Nelson 规则默认全开而不接受误报率从约 1/371 升至约 1/92（NIST）、不能证明根因或措施有效。

**典型样本量**: 依附于底层控制图（I、X̄、属性图等）

**制造场景（列名示例）**
SMT 回流焊对“峰值温度_℃”I 图启用 Test 1+2+5，当“温区7设定值”漂移时出现 9 点同侧运行触发“特殊原因调查单”，但改进效果仍用前后 t 检验验证。

**常见误用**
- 规则过多导致“狼来了”式误报，现场不再响应
- 混用 WE-4（8 点）与 Nelson-2（9 点）却不文档化

**图形解读要点**: 规则提示特殊原因线索，不替代假设检验。

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook — WECO Rules on Variables Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm) — accessed 2026-09-03
- [Minitab — Tests for Special Causes](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/tests-for-special-causes/) — accessed 2026-09-03
- [AIAG & VDA SPC Manual — Decision Rules](https://www.dqsglobal.com/en/explore/blog/release-of-aiag-vda-spc-manual-1st-edition) — accessed 2026-09-03

## 控制图

### c_chart — C 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → C 图
- **对话框角色**: `defects`（缺陷数）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：C 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于泊松型缺陷计数、固定“检验单位/面积/样本量”下监视总缺陷数 c；应关注缺陷数突增、同侧偏高及限不对称（常无 LCL）等模式，用于识别系统性缺陷源，但不替代泊松率检验或缺陷率置信区间推断。

**不能当什么用**
不能用于检验单位大小变化（应选 u 图）、不能用于合格/不合格判定（应选 p/np 图）、不能区分缺陷严重度权重。

**典型样本量**: 固定检验单位（如每批 100 件或每卷 50 m²）；平均缺陷数 c̄ 通常 ≥5

**制造场景（列名示例）**
PCB  AOI 每面板固定检查“焊点总数”，记录“虚焊缺陷数”“漏焊缺陷数”合计为 c，按“面次编号”“产线号”绘 c 图，发现“钢网编号”变更后缺陷计数上升。

**常见误用**
- 批量大小时变仍用 c 图
- 把不合格品数当缺陷数

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `c_chart_defect_step`

**权威来源**
- [NIST e-Handbook — Counts Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc331.htm) — accessed 2026-09-03
- [Minitab — Overview for C Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/attributes-charts/c-chart/before-you-start/overview/) — accessed 2026-09-03
- [Minitab Workspace — C Chart](https://support.minitab.com/en-us/workspace/help-and-how-to/forms/types-of-forms/statistical-analysis/control-chart/) — accessed 2026-09-03

### cusum — CUSUM 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → CUSUM 控制图
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：CUSUM 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于检测小幅单向或双向均值偏移，通过累积偏差和（CUSUM）对持续偏离目标/均值的点快速累加信号；应关注 CUSUM 单向爬升、超决策区间 h 及重置后再次累积等模式，用于比 3σ 限更早发现漂移，但不替代序贯概率比检验的正式推断框架。

**不能当什么用**
不能用于大幅随机跳动过程（误报风险）、不能用于多变量相关特性（应选 MCUSUM/MEWMA）、不能回答偏移幅度是否超过规格要求。

**典型样本量**: 个体或子组均值序列；设计需设定 k（参考值）与 h（决策限）

**制造场景（列名示例）**
药片压片重量按“压片机号”“批次号”逐片称重，CUSUM 监视相对“目标重量_mg”的累积偏差，用于发现“冲模编号”磨损导致的缓慢减重。

**常见误用**
- 目标值选错导致 CUSUM 系统性单向累积
- 未设定 V-mask/决策限就主观读图

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `spc_small_drift`

**权威来源**
- [Minitab — Overview for CUSUM Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/time-weighted-charts/cusum-chart/before-you-start/overview/) — accessed 2026-09-03
- [Montgomery — Statistical Quality Control Ch.9 CUSUM](https://www.oreilly.com/library/view/statistical-quality-control/9781118146811/Chapter09.html) — accessed 2026-09-03
- [NIST e-Handbook — Variables Control Charts ARL](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc321.htm) — accessed 2026-09-03

### ewma — EWMA 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → EWMA 控制图
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：EWMA 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于需要比 Shewhart 图更早发现小幅、持续均值漂移的计量或子组均值数据，通过指数加权递推平滑对近期点赋更高权重；应关注 EWMA 曲线缓慢爬升/下降、越界及与 I/X̄ 图不一致等模式，用于早期预警，但不替代 CUSUM 的正式变点检验或功效已设计的假设检验。

**不能当什么用**
不能用于大幅瞬时偏移的快速检测（Shewhart 更敏感）、不能用于属性计数（应选 p/c 等或专门方案）、参数 λ 选择不当会导致误报或迟钝。

**典型样本量**: 个体序列或子组均值序列；λ 常取 0.2–0.4

**制造场景（列名示例）**
精密车削对“主轴转速_rpm”“零件直径_mm”逐件监控，EWMA 监视直径均值，用于在公差带内发现“刀具磨损”导致的缓慢漂移。

**常见误用**
- 未与 Shewhart 图联用，单独解读 EWMA
- λ 过大使 EWMA 退化为普通均值图，失去小偏移灵敏度

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `spc_small_drift`

**权威来源**
- [Minitab — Overview for EWMA Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/time-weighted-charts/ewma-chart/before-you-start/overview/) — accessed 2026-09-03
- [Montgomery — Statistical Quality Control Ch.9 EWMA](https://www.oreilly.com/library/view/statistical-quality-control/9781118146811/Chapter09.html) — accessed 2026-09-03
- [NIST e-Handbook — ARL for Shewhart vs CUSUM](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc321.htm) — accessed 2026-09-03

### g_chart — G 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → G 图
- **对话框角色**: `variables`（间隔列）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：G 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于稀有事件监控，绘制两次事件之间的机会数或天数（几何分布），在样本量不足以支撑 p/u 图时仍能建立有意义控制限；应关注点低于 LCL（事件过于频繁/间隔缩短）或异常长间隔等模式，用于安全/质量稀有事件预警，但不替代生存分析或 Poisson 过程正式建模。

**不能当什么用**
不能用于精确时间戳间隔（应选 T 图）、不能用于每日机会数剧烈变化（日机会数应相对稳定）、不能用于高频率缺陷常规监控。

**典型样本量**: 每次事件一条记录；事件间隔机会数或天数

**制造场景（列名示例）**
总装线记录“重大返工事件日期”，按“生产天数间隔”绘 G 图，监视“重大返工”是否因某“工艺变更单号”而异常频繁发生。

**常见误用**
- 日产量/检验量波动大仍用“天数间隔”
- 把 G 图点高于 UCL 误读为“变差”（通常表示间隔延长、事件变少，属改善方向）

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `g_chart_gap_days`

**权威来源**
- [Minitab — Overview for G Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/rare-event-charts/g-chart/before-you-start/overview/) — accessed 2026-09-03
- [Minitab — Data Considerations for G Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/rare-event-charts/g-chart/before-you-start/data-considerations/) — accessed 2026-09-03
- [Minitab Blog — Monitoring Rare Events with G Charts](https://blog.minitab.com/en/blog/monitoring-rare-events-with-g-charts) — accessed 2026-09-03

### generalized_variance — 广义方差图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → 广义方差图
- **对话框角色**: `variables`（变量（多列））
- **algorithm_help purpose（对齐）**: 多元广义方差 |S| 子组控制图（Montgomery）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于同时监控两个及以上相关计量变量过程变异（协方差结构）是否稳定，|S| 广义方差图是多元 R/S 图的对应物；应关注 |S| 突增、持续偏高及与 T² 图联动报警等模式，用于发现多变量离散度增大，但不替代 Bartlett 协方差齐性检验或 NIST 指出的“尚无简单稳健多元变异性图”之严格推断。

**不能当什么用**
不能用于变量不相关（分别用 R/S 图）、NIST 警告现有广义方差法存在统计局限、不能定位具体哪一变量方差增大（需分解或单变量 S 图）。

**典型样本量**: 子组 n>p+1；Phase I 需足够子组估计协方差

**制造场景（列名示例）**
双组分灌装机同时监视“流量_A_L/min”“流量_B_L/min”的协方差，广义方差图发现“配比波动”增大，配合单变量 R 图定位 B 路跳动加剧。

**常见误用**
- 样本协方差矩阵奇异仍强行计算 |S|
- 仅看 T² 均值图忽略 |S| 方差增大

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `genvar_two_var`

**权威来源**
- [Minitab — Overview for Generalized Variance Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/multivariate-charts/generalized-variance-chart/before-you-start/overview/) — accessed 2026-09-03
- [NIST e-Handbook — Multivariate Variability Charts](https://www.itl.nist.gov/div898/handbook/pmc/section5/pmc5435.htm) — accessed 2026-09-03
- [NIST e-Handbook — Multivariate Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc34.htm) — accessed 2026-09-03

### hotelling_t2 — Hotelling T²

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → Hotelling T²
- **对话框角色**: `variables`（变量（多列））
- **algorithm_help purpose（对齐）**: 多元个体 Hotelling T² 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于同时监控两个及以上相关计量变量的均值向量是否偏离历史受控中心，T² 统计量综合均值偏移与协方差结构；应关注 T² 突增、持续偏高及需配合 T² 分解/单变量图定位变量等模式，用于多变量均值监控，但不能替代 Hotelling T² 假设检验的 p 值推断，且报警时不直接指明哪一变量失控。

**不能当什么用**
不能用于变量独立且无相关时（分别做单变量图更简单）、不能监控协方差矩阵本身（应配合广义方差或 |S| 图）、样本量需 p<n-1。

**典型样本量**: 子组 n>p+1（个体观测时 Phase I 样本量需充分估计 Σ）

**制造场景（列名示例）**
注塑同时测“产品长度_mm”“产品宽度_mm”“翘曲_mm”，Hotelling T² 监视三变量均值向量，发现“模温_℃”异常时 T² 报警，再分解定位“翘曲_mm”贡献最大。

**常见误用**
- T² 报警后未做分解就随意调整所有参数
- 忽略变量间相关性仍分别设宽限导致误报/漏报

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `t2_two_var_shift`

**权威来源**
- [NIST e-Handbook — Hotelling Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc341.htm) — accessed 2026-09-03
- [NIST e-Handbook — Multivariate Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc34.htm) — accessed 2026-09-03
- [Tracy et al. — Decomposition of T²](https://doi.org/10.1080/00224065.1995.11979573) — accessed 2026-09-03

### imr — I-MR 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → I-MR 控制图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：I-MR 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于计量型、逐件采集且无法形成合理子组的场合，以单值图（I）监视过程位置、以移动极差图（MR）监视相邻两点间短期波动；应关注越界点、同侧连续运行、阶梯漂移及 MR 突增等模式，用于判断是否存在特殊原因，但不能替代假设检验来“证明”均值是否等于目标。

**不能当什么用**
不能回答规格是否满足（控制限≠规格限）、不能在有合理子组时替代 X̄-R/S 以分离组内/组间变异、不能证明正态性，也不能单独判定长期过程能力或显著性差异。

**典型样本量**: n=1（每时间点一个测量值）；建立 Phase I 控制限通常建议至少 20–25 个连续点

**制造场景（列名示例）**
注塑车间对“产品编号”“班次”“射胶压力_MPa”逐模测量，每模只取一次“尺寸_mm”，按时间顺序绘制 I-MR，用于发现换模、料温漂移或模具磨损导致的突发偏移。

**常见误用**
- 将规格限当作控制限，或把“在规格内”误判为过程受控
- 对自相关或批次混合数据直接套用 I-MR，导致伪信号或漏检

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `imr_spi_shift`

**建议埋点（教学升级）**
- 60 行单值；片 1–40 基线；**片 41 起均值阶跃**；**片 55 尖峰**冲向/越过 I 图 UCL；尖峰处 MR 变大
- 列只要：片号、锡膏高度_um、时段备注（备注不进对话框）
- 阶段列/历史限留空；`use_nelson_estimate=0`；`tests` 留空走 `rule_policy`

**权威来源**
- [NIST e-Handbook — Individuals Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc322.htm) — accessed 2026-09-03
- [Minitab — Overview for I-MR Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/i-mr-chart/before-you-start/overview/) — accessed 2026-09-03
- [Minitab — Tests for special causes](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/basics/using-tests-for-special-causes/) — accessed 2026-09-03
- [Wheeler — Short Run SPC (SPC Press)](https://www.spcpress.com/pdf/DJW359.pdf) — accessed 2026-09-03

### imr_rs — I-MR-R/S 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → I-MR-R/S 控制图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：I-MR-R/S 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于同一设备/产线加工多种零件或批次、每个子组代表不同批次/零件的混合生产场景，同时用 I 图监视跨批次的过程位置、MR 图监视相邻个体短期波动、R/S 图监视批次内离散度；应关注三图联动报警（尤其 R/S 先失控），用于定位批次内与批次间变异来源，但不替代嵌套方差分析或混合模型检验。

**不能当什么用**
不能用于单一稳定产品且子组可合理划分的常规 X̄-R/S 场景、不能回答不同零件间均值差异的显著性、不能替代量具重复性/再现性研究。

**典型样本量**: 跨批次逐件 I 序列 + 每批次内 n=2–10 的子组测量

**制造场景（列名示例）**
柔性产线按“零件图号”“批次号”混流生产，每批次首件及过程件测“关键尺寸_mm”，跨批次绘 I-MR、批次内绘 R，用于发现“夹具编号”切换后某图号批次内跳动异常。

**常见误用**
- 未按“批次/零件”正确分层就把所有数据混在一个 I 图上
- 忽略 R/S 图仅看 I 图，漏检某批次内部变异异常

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `imr_rs_subgroup_shift`

**权威来源**
- [Minitab Real-Time SPC — I-MR-R/S Chart](https://support.minitab.com/en-us/real-time-spc/quality-analyses/control-charts/control-chart-settings-for-each-measure/) — accessed 2026-09-03
- [NIST e-Handbook — Variables Control Charts Overview](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm) — accessed 2026-09-03
- [Montgomery — Short Run and Multiple Stream Processes Ch.10](https://www.oreilly.com/library/view/statistical-quality-control/9781118146811/Chapter10.html) — accessed 2026-09-03

### laney_p_chart — Laney P' 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → Laney P' 图
- **对话框角色**: `defectives`（不合格品数）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：Laney P' 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于 p 图场景但数据存在过度离散或欠离散（大样本、缺陷率时变、子组间相关）时，通过 Sigma Z 调整控制限以更准确区分共同原因与特殊原因；应关注调整后仍出现的持续偏移或阶梯变化，用于避免传统 p 图限过窄导致的误报，但不替代 Laney 原始论文中的模型诊断或正式超/欠离散检验。

**不能当什么用**
不能用于小样本常规二项过程（标准 p 图即可）、不能处理缺陷计数（应选 Laney U'）、不能证明改进措施显著有效。

**典型样本量**: 大子组（数百至数千）或长期属性监控

**制造场景（列名示例）**
电子厂终检每批“抽检数量”2000+，记录“功能不良数”，传统 p 图频繁误报，改用 Laney P' 监视“功能不良率”，结合“测试程序版本”调查真实特殊原因。

**常见误用**
- 未做过度/欠离散诊断就直接用传统 p 图
- 把 Sigma Z>1 的修正误解为“过程变差更大所以放行”

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `laney_p_overdispersed`

**权威来源**
- [Laney (2002) — Improved Control Charts for Attributes](https://doi.org/10.1081/qen-120003555) — accessed 2026-09-03
- [Minitab — Overdispersion and Underdispersion](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/understanding-attributes-control-charts/overdispersion-and-underdispersion/) — accessed 2026-09-03
- [Mohammed et al. (2013) — Large Sample Attribute SPC Review](https://doi.org/10.1136/bmjqs-2012-001373) — accessed 2026-09-03

### laney_u_chart — Laney U' 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → Laney U' 图
- **对话框角色**: `defects`（缺陷数）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：Laney U' 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于 u 图场景但存在过度/欠离散时，以 Sigma Z 修正单位缺陷率控制限，减少大样本或时变缺陷率下的误报/漏报；应关注修正后仍持续的 u 偏移或周期性抬升，用于属性过程稳定性监控，但不替代 Poisson/负二项模型的正式拟合优度检验。

**不能当什么用**
不能用于固定小单位常规泊松过程、不能用于二项不合格比例、不能单独量化各缺陷类型的贡献度。

**典型样本量**: 检验单位数 n 大且可变；长期缺陷率监控

**制造场景（列名示例）**
半导体封测按“检验_die数”可变记录“外观缺陷数”，传统 u 图因 n 很大而限过窄，Laney U' 监视“每_die缺陷率”，发现“封装材料批次”导致的真实率上升。

**常见误用**
- 标准 u 图大量误报后未检查 Sigma Z 就频繁停线
- 混淆缺陷率改善与控制限变宽

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `laney_u_overdispersed`

**权威来源**
- [Laney (2002) — Improved Control Charts for Attributes](https://doi.org/10.1081/qen-120003555) — accessed 2026-09-03
- [Minitab — All Statistics for Laney U' Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/attributes-charts/laney-u-chart/interpret-the-results/all-statistics-and-graphs/) — accessed 2026-09-03
- [Minitab Assistant — Attribute Control Charts PDF](https://support.minitab.com/en-us/minitab/media/pdfs/translate/Assistant_Attribute_Control_Charts.pdf) — accessed 2026-09-03

### mewma — MEWMA

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → MEWMA
- **对话框角色**: `variables`（变量（多列））
- **algorithm_help purpose（对齐）**: 多元 EWMA（MEWMA）控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于多变量均值的小幅持续漂移检测，对均值向量做指数加权递推并以 T² 型统计量判异，比多元 Shewhart T² 对小偏移更敏感；应关注 MEWMA 统计量渐进爬升、越 UCL 及 λ 过小导致滞后等模式，用于化学/制药等多响应过程早期预警，但不替代多元 CUSUM 的正式比较或 MANOVA 显著性检验。

**不能当什么用**
不能用于单变量（应选 EWMA）、不能监控协方差结构变化、高维共线数据需降维（PCA/PLS）后再监控。

**典型样本量**: 多变量时间序列；λ 常取 0.1–0.3

**制造场景（列名示例）**
反应釜同时记录“温度_℃”“压力_MPa”“pH值”，MEWMA 监视三变量均值向量缓慢漂移，早于单变量图发现“催化剂批次”导致的综合偏移。

**常见误用**
- λ 与 UCL 未按 ARL 设计导致误报过高
- 忽略变量尺度/量纲未标准化

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `mewma_two_var_drift`

**权威来源**
- [NIST e-Handbook — Multivariate EWMA Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc333.htm) — accessed 2026-09-03
- [Lowry et al. (1992) — Multivariate EWMA](https://doi.org/10.1080/00401706.1992.10485232) — accessed 2026-09-03
- [TIBCO — MEWMA Charts Overview](https://docs.tibco.com/data-science/GUID-B18E88EF-EEAD-4AD1-BF58-59785F836021.html) — accessed 2026-09-03

### moving_average — 移动平均控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → 移动平均控制图
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：移动平均控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于对最近 w 个观测做简单（等权）滑动平均以平滑噪声并检测小幅均值变化；应关注滑动均值缓慢偏离中心线、连续同侧及窗口边界效应等模式，用于过程趋势监视，但不替代 EWMA（Minitab 指出 EWMA 通常更优）或时间序列 ARIMA 检验。

**不能当什么用**
不能用于大幅阶跃偏移的快速报警、不能用于属性数据、窗口宽度 w 选择无理论最优时易迟钝或误报。

**典型样本量**: 个体或子组序列；窗口 w 常取 3–10

**制造场景（列名示例）**
挤出机对“熔体温度_℃”每分钟记录，3 点移动平均图平滑噪声后监视“加热区编号”调整后的温度趋势是否稳定。

**常见误用**
- 窗口过大导致对小偏移不敏感
- 与 EWMA 混淆而未考虑指数衰减权重

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `ma_small_drift`

**权威来源**
- [Minitab — Overview for Moving Average Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/time-weighted-charts/moving-average-chart/before-you-start/overview/) — accessed 2026-09-03
- [Montgomery — Statistical Quality Control Ch.9.3 Moving Average](https://www.oreilly.com/library/view/statistical-quality-control/9781118146811/Chapter09.html) — accessed 2026-09-03
- [NIST e-Handbook — Glossary MA/EWMA](https://www.itl.nist.gov/div898/handbook/glossary.htm) — accessed 2026-09-03

### np_chart — NP 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → NP 图
- **对话框角色**: `defectives`（不合格品数）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：NP 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于二项型属性数据、每子组检验数量固定时，直接监视“不合格品个数 np”；应关注 np 突增、连续高于中心线及接近 UCL 的聚集模式，用于发现批量性失效，但不替代二项精确检验或 Fisher 精确检验。

**不能当什么用**
不能用于子组样本量变化（应选 p 图）、不能用于同一单位多缺陷计数（应选 c/u 图）、不能估计过程能力指数。

**典型样本量**: 固定 n（常见 50–200），每点为一个子组的不合格品计数

**制造场景（列名示例）**
装配线每班固定抽检 100 台，记录“功能失效台数”，按“生产日期”“线体编号”绘制 np 图，发现某“工装版本”导致失效台数连续偏高。

**常见误用**
- 订单量变化仍固定用 np 图
- 与 p 图混用刻度（比例 vs 计数）导致解读错误

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `np_chart_const_n_step`

**权威来源**
- [NIST e-Handbook — Proportions Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc332.htm) — accessed 2026-09-03
- [Minitab — Overview for NP Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/attributes-charts/np-chart/before-you-start/overview/) — accessed 2026-09-03
- [Minitab — Attributes Control Charts in Minitab](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/understanding-attributes-control-charts/attributes-control-charts-in-minitab/) — accessed 2026-09-03

### p_chart — P 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → P 图
- **对话框角色**: `defectives`（不合格品数）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：P 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于二项型属性数据、按“不合格品/检验数”计算比例且子组样本量可变的场合，监视不合格率 p 是否稳定；应关注比例突增、同侧运行及限随 n 变化而变窄/变宽等模式，用于触发特殊原因调查，但不能替代比例 z 检验或卡方检验来正式比较两阶段不合格率。

**不能当什么用**
不能用于计数“缺陷数”（应选 c/u 图）、不能用于固定小子组且需整数不合格数的场合（可考虑 np 图）、不能回答单个产品是否合格。

**典型样本量**: 每子组检验数 n 可变（常见 50–500）；np̄ 应足够大以满足二项近似

**制造场景（列名示例）**
终检站按“检验批次号”“抽检数量”“外观不良数”记录，因各批订单量不同用 p 图监视“外观不良率”，发现某“供应商批次”不良率抬升。

**常见误用**
- 把缺陷数当不合格品数，误用 p 图
- 子组过小或 p̄ 极低时仍用正态近似 3σ 限，导致限失真

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `p_chart_variable_n_step`

**建议埋点（教学升级）**
- 可变检验数 n；中段某批不合格率台阶；限随 n 变宽窄
- 列：不合格品数 + 检验数；不要埋缺陷计数（那是 c/u）

**权威来源**
- [NIST e-Handbook — Proportions Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc332.htm) — accessed 2026-09-03
- [Minitab — Overview for P Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/attributes-charts/p-chart/before-you-start/overview/) — accessed 2026-09-03
- [OSU Extension — Attributes Control Charts](https://extension.oregonstate.edu/catalog/em-9110-statistical-process-control-part-8-attributes-control-charts) — accessed 2026-09-03

### t_chart — T 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → T 图
- **对话框角色**: `variables`（间隔列）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：T 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于稀有事件且间隔为连续时间（日期/时间戳或精确时长）的场合，经 Weibull 变换后类似 I 图监视事件间隔稳定性；应关注间隔异常缩短、变换后越界及零间隔（同时发生）等模式，用于比 G 图更精细的时间间隔监控，但不替代 Cox 比例风险或参数生存模型的正式检验。

**不能当什么用**
不能用于仅整数机会计数（应选 G 图）、不能用于非独立间隔（如批量共因失效）、不能证明根因与事件因果关联。

**典型样本量**: 每次事件一条时间戳或间隔记录

**制造场景（列名示例）**
压力容器产线记录“泄漏事件时间戳”，T 图监视“事件间隔_小时”是否因某“焊接参数版本”变更而异常缩短。

**常见误用**
- 仅有日期无精确时间仍强行用 T 图
- 忽略零间隔事件导致参数估计偏差

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `t_chart_time_interval`

**权威来源**
- [Minitab — Methods and Formulas for T Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/rare-event-charts/t-chart/methods-and-formulas/methods-and-formulas/) — accessed 2026-09-03
- [Minitab Blog — G and T Charts for Rare Events](https://blog.minitab.com/en/blog/monitoring-rare-events-with-g-charts) — accessed 2026-09-03
- [SESUG 2024 — G-charts and T-charts](https://www.lexjansen.com/sesug/2024/42_Final_PDF.pdf) — accessed 2026-09-03

### u_chart — U 图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → U 图
- **对话框角色**: `defects`（缺陷数）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：U 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于泊松型缺陷计数、检验单位大小（面积、长度、件数）随子组变化时，监视单位缺陷率 u；应关注 u 突增、限随 n 变化及过度离散导致的频繁误报等模式，用于发现单位缺陷强度变化，但不替代 Poisson 回归或率比检验。

**不能当什么用**
不能用于固定检验单位（应选 c 图）、不能用于二项不合格品比例（应选 p 图）、在严重过度离散时标准 u 图可能失效（考虑 Laney U'）。

**典型样本量**: 每子组检验单位数 n 可变；ū 通常 ≥5

**制造场景（列名示例）**
涂装线按“班次”“喷涂面积_m²”记录“颗粒缺陷数”“流挂缺陷数”，因面积不同用 u 图监视“每平米缺陷数”，发现“喷枪编号”堵塞导致单位缺陷率升高。

**常见误用**
- 固定样本量仍用 u 图
- 忽略过度离散，把大量假失控当真实特殊原因

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `u_chart_variable_unit_step`

**权威来源**
- [NIST e-Handbook — Counts Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc331.htm) — accessed 2026-09-03
- [Minitab — Overview for U Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/attributes-charts/u-chart/before-you-start/overview/) — accessed 2026-09-03
- [Minitab — Attributes Control Charts in Minitab](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/understanding-attributes-control-charts/attributes-control-charts-in-minitab/) — accessed 2026-09-03

### xbar_r — Xbar-R 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → Xbar-R 控制图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：Xbar-R 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于计量型、可在同一条件下一次取多个测量值形成子组的场合，X̄ 图监视子组均值偏移，R 图监视组内离散度；应同时检查 X̄ 与 R 的越界、同侧运行及 R 图先报警等模式，用于识别特殊原因，但不替代 t 检验/F 检验来判定组间差异是否显著。

**不能当什么用**
不能用于子组过大（通常 n>10 应改 X̄-S）、不能用于仅有个别测量值的慢速/破坏性检验、不能回答产品是否合格或 Cpk 是否达标。

**典型样本量**: 子组 n=2–10（常见 n=4 或 5）；Phase I 常取 m≥25 个子组

**制造场景（列名示例）**
机加产线每班对“工单号”“工序号”“轴径_mm”按“每批5件”取样，绘制 X̄-R，用于发现“刀具编号”更换后均值上移或组内跳动加剧。

**常见误用**
- 子组划分不合理（混合不同批次/夹具/操作员），使组内方差被人为放大
- 只看 X̄ 图忽略 R 图，漏检组内变异增大

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `xbar_r_n5_range_spike`

**权威来源**
- [NIST e-Handbook — X-bar and R Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc321.htm) — accessed 2026-09-03
- [Montgomery — Statistical Quality Control Ch.6–7](https://www.oreilly.com/library/view/statistical-quality-control/9781118146811/Chapter06.html) — accessed 2026-09-03
- [AIAG & VDA SPC Manual Overview](https://www.dqsglobal.com/en/explore/blog/release-of-aiag-vda-spc-manual-1st-edition) — accessed 2026-09-03

### xbar_s — Xbar-S 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → Xbar-S 控制图
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：Xbar-S 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于计量型、子组较大（通常 n>10）的场合，X̄ 图监视均值，S 图监视子组标准差；应关注 S 图先失控、X̄ 同侧漂移及方差阶梯变化等模式，用于判断变异结构是否改变，但不能替代 Levene/Bartlett 检验来正式检验方差齐性。

**不能当什么用**
不能用于小子组（n≤10 时 R 图更高效）、不能处理非独立批次数据、不能判定缺陷率或二项/泊松计数过程。

**典型样本量**: 子组 n>10（常见 n=10–25）；Phase I 建议 m≥25 个子组

**制造场景（列名示例）**
化工灌装线每小时对“罐号”“净重_g”连续称量 15 瓶为一子组，用 X̄-S 监视“灌装均值”与“瓶间重量波动”，发现泵头磨损导致的方差增大。

**常见误用**
- 在 n=4–5 的小子组仍用 S 图，估计效率低于 R 图
- 未先确认 S 图受控就解读 X̄ 图

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `xbar_s_n8_sd_shift`

**权威来源**
- [NIST e-Handbook — X-bar and S Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc321.htm) — accessed 2026-09-03
- [Minitab — Xbar-S Chart guidance](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-subgroups/xbar-s-chart/before-you-start/overview/) — accessed 2026-09-03
- [Montgomery — Statistical Quality Control Ch.7](https://www.oreilly.com/library/view/statistical-quality-control/9781118146811/Chapter07.html) — accessed 2026-09-03

### z_mr — Z-MR 控制图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → Z-MR 控制图
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：Z-MR 控制图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于短制程/多品种混线生产，将不同产品的测量值按各产品名义值与 Sigma(X)（基于移动极差，非全局标准差）标准化为 z 分数，再统一绘 z 图与 W（移动极差）图；应关注 z 越 ±3、W 越 3.686 及跨产品混排时的同侧运行，用于混流监控，但不替代各产品独立能力研究或 ANOVA 比较品种差异。

**不能当什么用**
不能用于单一产品长期稳定过程（直接用 I-MR）、不能用全局标准差做 Wheeler 禁止的“传统 z 变换”、不能用于属性数据。

**典型样本量**: 每产品仅少量点亦可；需为每个产品定义名义值与 Sigma(X)

**制造场景（列名示例）**
多品种 CNC 线按“产品图号”“名义尺寸_mm”“实测尺寸_mm”混流生产，z-MR 统一监视偏离名义的标准化距离，发现某“程序版本号”下某图号持续偏高。

**常见误用**
- 用产品总体标准差代替移动极差估计 Sigma(X)，掩盖信号
- 产品间变异差异大仍用差值图而非 z 图

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `z_mr_short_run`

**权威来源**
- [Wheeler — Short Run SPC Part 3 (SPC Press)](https://www.spcpress.com/pdf/DJW359.pdf) — accessed 2026-09-03
- [Wheeler — ZED Charts (SPC Press)](https://www.spcpress.com/pdf/DJW357.pdf) — accessed 2026-09-03
- [Quality Digest — Short Run SPC Part 3](https://www.qualitydigest.com/inside/statistics-column/short-run-spc-part-3-020220.html) — accessed 2026-09-03

### zone_chart — 区域图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **控制图** → 区域图
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 用控制图监视过程：区域图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
用于以 σ 分区（Zone 1–4）和累积得分替代多条 Western Electric 规则来检测非随机模式，适合需要统一自动化判异逻辑的子组或个体数据；应关注同侧累积得分≥8、越 Zone 4 及跨中心线重置等模式，用于结构化判异，但不替代假设检验或 Nelson 规则全集的灵敏度分析。

**不能当什么用**
不能用于属性计数数据、不能检测 EWMA/CUSUM 类小漂移（除非配合时间加权图）、权重方案不当会降低特定模式检出率。

**典型样本量**: 与子组 X̄ 或个体 I 图相同

**制造场景（列名示例）**
轴承磨削对“外径_mm”每班 10 件子组，Zone 图累积判异，当“砂轮编号”磨损时出现连续 Zone 2–3 同侧得分触发调查。

**常见误用**
- 自定义权重后未验证 ARL/误报率
- 信号后未重置累积得分继续误判

**图形解读要点**: 看超出控制限、趋势、周期、游程；模式识别不替代假设检验。

**建议 dataset_id**: `zone_chart_runs`

**权威来源**
- [Minitab — Overview for Zone Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-subgroups/zone-chart/before-you-start/overview/) — accessed 2026-09-03
- [Minitab — Interpret Zone Chart Key Results](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-subgroups/zone-chart/interpret-the-results/key-results/) — accessed 2026-09-03
- [NIST e-Handbook — WECO Rules Background](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm) — accessed 2026-09-03

## 数据

### database_import — 数据库导入

- **implemented_status**: `partial`
- **菜单路径（代码）**: 数据 > 数据库导入…
- **对话框角色**: _（无 analysis_commands 条目；见 help id）_
- **algorithm_help purpose（对齐）**: 通过 Provider 连接数据库、发现表/视图与列，按 ImportPlan 导入工作表。
- **实现说明**: orchestration；菜单可能不存在。

**常用来做什么**
从 SQLite/ODBC 等数据源只读导入表到工作区，衔接 MES/历史库。

**不能当什么用**
不替代数据清洗；大表需分页；写权限默认关闭。

**典型样本量**: 导入行数建议分批；学习中心演示 30–200 行。

**制造场景（列名示例）**
从 MES SQLite 导入上周 SPI 检测结果到新工作表。

**常见误用**
- 编码/时区列类型错
- 全表一次载入内存
- 连接名泄漏未关闭

**图形解读要点**: 看分布/关系/趋势模式与离群；图形探索不替代假设检验。

**权威来源**
- [Qt SQL Programming](https://doc.qt.io/qt-6/qsqldatabase.html) — accessed 2026-09-03
- [SQLite Documentation](https://www.sqlite.org/docs.html) — accessed 2026-09-03

## 文件

### report_templates — 报告模板（客户/工程师/审计）

- **implemented_status**: `implemented`
- **菜单路径（代码）**: 文件 > 导出 PDF
- **对话框角色**: _（无 analysis_commands 条目；见 help id）_
- **algorithm_help purpose（对齐）**: 用同一份 OutputPage/Facts 生成客户版、工程师版或审计版报告；模板只改变展示与证据密度，不重算统计。
- **实现说明**: orchestration；菜单可能不存在。

**常用来做什么**
同一分析输出按客户/工程师/审计模板导出 PDF，控制证据密度与表述。

**不能当什么用**
不重算统计；模板不改正文外的分析结论；不能替代原始数据审查。

**典型样本量**: 不适用。

**制造场景（列名示例）**
能力分析完成后导出审计版 PDF 附 Cpk 表与图。

**常见误用**
- 以为换模板会改 p 值
- 审计版缺原始追溯
- 工程师版过度删诊断

**权威来源**
- [ASQ Quality Reports](https://asq.org/quality-resources) — accessed 2026-09-03
- [AIAG PPAP](https://www.aiag.org/quality/ppap/) — accessed 2026-09-03

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

**典型样本量**: Often 20–40+ units across 2–3 stress levels; NIST: ≥10 failures total preferred.

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- Wrong life-stress model
- not checking activation energy
- zero failures at low stress
- ignoring censoring.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Accelerated Life Testing](https://www.itl.nist.gov/div898/handbook/apr/section5/apr51.htm) — accessed 2026-09-03
- [Minitab: Accelerated Life Test](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/how-to/accelerated-life-test/) — accessed 2026-09-03

### acf_pacf — ACF/PACF

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → ACF/PACF
- **对话框角色**: `variables`（序列）
- **algorithm_help purpose（对齐）**: 计算单列序列的自相关与偏自相关，并画带置信限的 ACF/PACF 图。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
识别 AR/MA 阶数、季节周期与自相关结构（诊断图）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 图形解读主观；不替代 Ljung-Box 检验；非平稳序列需先差分。

**典型样本量**: ≥50；季节识别需≥2 季节。

**制造场景（列名示例）**
ACF/PACF 判断日尺寸偏差的 MA/AR 阶。

**常见误用**
- 非平稳直接看 ACF
- 滞后截断误判阶数
- 小样本过度解读

**图形解读要点**: 看分布/关系/趋势模式与离群；图形探索不替代假设检验。

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST ACF](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc45.htm) — accessed 2026-09-03
- [Minitab ACF/PACF](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/time-series/autocorrelation-and-partial-autocorrelation/) — accessed 2026-09-03

### adf_test — ADF 单位根检验

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → ADF 单位根检验
- **对话框角色**: `variables`（序列）
- **algorithm_help purpose（对齐）**: 对单列序列做 Augmented Dickey–Fuller 检验，报告 τ 与临界值。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过、规格已满足或必须删点。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
检验序列单位根/非平稳性，为差分或 ARIMA 定阶提供证据。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 低功效；临界值依赖规格；不能单独定模型阶。

**典型样本量**: ≥50 较稳；短序列功效低。

**制造场景（列名示例）**
ADF 检验周良率是否需一阶差分再建模。

**常见误用**
- p>0.05 就当平稳
- 结构突变未处理
- 忽略季节单位根

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Dickey-Fuller](https://doi.org/10.2307/1912352) — accessed 2026-09-03
- [Minitab ADF](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/time-series/unit-root-test/) — accessed 2026-09-03

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

**典型样本量**: Inherits DSD run count; recommend +4 fake-factor runs.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Overfitting with many active effects
- not using specialized analysis
- ignoring heredity in model selection.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Jones & Nachtsheim: DSD Analysis Methods](https://www.jmp.com/en/statistics-knowledge-portal/design-of-experiments/screening-designs/definitive-screening-designs) — accessed 2026-09-03
- [Minitab: Analyze Definitive Screening Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/definitive-screening/analyze-definitive-screening-design/) — accessed 2026-09-03

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

**典型样本量**: Factorial structure with ≥2 replicates per cell for variance estimates; often 16–32+ runs.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Few replicates making variance unstable
- ignoring lognormal nature of SD
- confounding mean and variance effects.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Analysis of Variance of Dispersion](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Analyze Variability](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/factorial/analyze-variability-for-factorial-design/) — accessed 2026-09-03

### anom — 均值分析 (ANOM)

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 均值分析 (ANOM)
- **对话框角色**: `response`（响应变量）
- **algorithm_help purpose（对齐）**: 在正态均值假定下，比较各组均值是否偏离总体均值（Analysis of Means）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
在单因素多水平比较中，将各组均值与总均值对比，识别哪些组均值显著偏离整体水平（图形化多重比较，平衡设计常用）。

**不能当什么用**
不能替代方差分析整体检验；对非正态/不等方差敏感；不能用于属性数据（用属性 ANOM）；未控制多重比较风险时需结合实验目的。

**典型样本量**: 每组 n=5–15 常见；总组数 k 不宜过大；与 ANOVA 共用同一数据集。

**制造场景（列名示例）**
三模腔尺寸 ANOM：列「尺寸_mm」「模腔」「机台号」，识别哪个模腔均值系统性偏离三台合计平均水平。

**常见误用**
- 不做整体 F 检验直接看 ANOM
- 把落在控制限内当成「完全相同」
- 组间方差差异大仍解读
- 与个体控制图混淆。

**图形解读要点**: 看分布/关系/趋势模式与离群；图形探索不替代假设检验。

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST/SEMATECH e-Handbook — Analysis of Means (ANOM)](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm) — accessed 2026-09-03
- [AIAG — SPC Manual, ANOM for Means](https://www.aiag.org/store/quality/) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 3](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03

### anom_attribute — 属性 ANOM（二项/泊松）

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 属性 ANOM（二项/泊松）
- **对话框角色**: `response`（响应列）
- **algorithm_help purpose（对齐）**: 在二项或泊松假定下，比较各组比例/计数是否偏离总体中心（Minitab 属性 ANOM）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多组二项不良率或泊松缺陷率的 ANOM 比较，识别哪几组率值偏离整体水平（属性版均值分析）。

**不能当什么用**
不能用于连续测量；小样本率不稳定；需满足二项/泊松近似条件；不能替代属性控制图的过程监控。

**典型样本量**: 二项：各组 n≥50 且 np≥5；泊松：各组暴露量足够使 λ̂ 稳定；组数适中。

**制造场景（列名示例）**
四班次不良率 ANOM：列「班次」「抽检数」「不良数」，识别哪个班次不良率偏离四班合计水平。

**常见误用**
- 组样本量差异极大仍直接比率
- 低不良率小样本误判
- 与 p 图/u 图监控混淆
- 未统一检验单位。

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST/SEMATECH e-Handbook — ANOM for Attributes](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc33.htm) — accessed 2026-09-03
- [AIAG — SPC Manual, Attribute ANOM](https://www.aiag.org/store/quality/) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control, Ch. 7](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+8th+Edition-p-9781119726764) — accessed 2026-09-03

### arima — ARIMA / Best ARIMA

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → ARIMA 基础预测
- **对话框角色**: `time`（时间列（可选））
- **algorithm_help purpose（对齐）**: 对单列时间顺序数据做ARIMA / Best ARIMA。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
平稳/差分后序列的 ARIMA 建模与预测，捕捉自回归移动平均结构。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 短序列定阶不稳；外生变量用 ARIMAX；不保证优于简单法。

**典型样本量**: ≥50；定阶需 ACF/PACF 与信息准则。

**制造场景（列名示例）**
日产量 ARIMA 预测下两周产出。

**常见误用**
- 非平稳未差分
- 过拟合高阶
- 不验证残差白噪声

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST ARIMA](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc45.htm) — accessed 2026-09-03
- [Minitab ARIMA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/time-series/arima/) — accessed 2026-09-03

### best_subsets_regression — Best Subsets 回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Best Subsets 回归
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 比较不同预测变量子集的线性回归拟合，按规模展示最佳模型摘要。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
枚举（或限规模）比较不同子集模型的 R²、Cp、BIC 等，选简约模型。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 候选>15 不可穷举；仍有过拟合风险。

**典型样本量**: n≥40；候选变量≤15。

**制造场景（列名示例）**
8 个模具参数 best subsets 选 3 因子解释翘曲。

**常见误用**
- 只看 R² 不看 Mallows Cp
- 样本小仍多变量
- 不做残差诊断

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab Best Subsets](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/regression/best-subsets-regression/) — accessed 2026-09-03
- [NIST Model Selection](https://www.itl.nist.gov/div898/handbook/pmd/section4/pmd44.htm) — accessed 2026-09-03

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

**典型样本量**: Similar to binary DOE: 50–100+ with multiple trials per factor setting preferred.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Too few events per cell
- extrapolating probit beyond data
- ignoring overdispersion.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: Applied Statistics — Probit Analysis](https://www.wiley.com/en-us/Applied+Statistics+and+Probability+for+Engineers%2C+7th+Edition-p-9781119409530) — accessed 2026-09-03
- [NIST e-Handbook: Probit Analysis](https://www.itl.nist.gov/div898/handbook/pri/section7/pri7.htm) — accessed 2026-09-03

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

**典型样本量**: Power-dependent: often 40–100+ runs; replicate at corner/center for stability.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Analyzing as continuous
- separation problems (all pass at a setting)
- inadequate replicates per factor combo.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Factorial with Binary Response](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Analyze Binary Response DOE](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/factorial/analyze-binary-response-for-factorial-design/) — accessed 2026-09-03

### bootstrap_mean — Bootstrap 均值 CI

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Bootstrap 均值 CI
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 对单列样本均值做百分位或 BCa bootstrap 置信区间。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
重抽样估计均值置信区间，弱分布假设下推断。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 自相关/分层数据需专门 bootstrap；不能修复偏倚设计。

**典型样本量**: 原始 n≥20；重抽样 B≥1000。

**制造场景（列名示例）**
偏态镀层厚度 bootstrap 均值 95% CI。

**常见误用**
- 有放回当无放回混淆
- 时间序列独立 bootstrap 失真
- B 太小区间不稳

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Bootstrap](https://www.itl.nist.gov/div898/handbook/pmc/section2/pmc22.htm) — accessed 2026-09-03
- [Efron Bootstrap](https://doi.org/10.1214/aos/1176345638) — accessed 2026-09-03

### bootstrap_two_sample — Bootstrap 双样本均值差 CI

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Bootstrap 双样本均值差 CI
- **对话框角色**: `first`（样本 1）
- **algorithm_help purpose（对齐）**: 对两独立样本均值差做百分位或 BCa bootstrap 置信区间。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
两独立样本均值差 bootstrap CI，非正态或方差不齐时的补充。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 不替代随机化/配对设计；小样本功效仍低。

**典型样本量**: 每组 n≥15；B≥2000。

**制造场景（列名示例）**
两供应商强度差 bootstrap CI 辅助等价评估。

**常见误用**
- 配对数据当独立
- 不等 n 解释不当
- 与置换检验混淆

**图形解读要点**: 看分布/关系/趋势模式与离群；图形探索不替代假设检验。

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Bootstrap Two Sample](https://www.itl.nist.gov/div898/handbook/pmc/section2/pmc22.htm) — accessed 2026-09-03
- [Minitab Bootstrapping](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/bootstrapping/) — accessed 2026-09-03

### cart_tree — CART 单树

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → CART 单树
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 用自研二叉递归划分做分类或回归单树，输出结点表与变量重要性。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过、规格已满足或必须删点。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
可解释的单树分类/回归，规则分段与变量重要性（探索/基线模型）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 单树易过拟合；不稳定；不能外推连续关系。

**典型样本量**: n≥100；叶最小样本≥5。

**制造场景（列名示例）**
决策树判断哪道量测最先把不良分开。

**常见误用**
- 深度过大过拟合
- 不做剪枝或 CV
- 把训练精度当泛化

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Breiman CART](https://doi.org/10.1201/9781315139470) — accessed 2026-09-03
- [Minitab CART](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/multivariate/classification-and-regression-trees/) — accessed 2026-09-03

### ccf — 互相关（CCF）

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 互相关（CCF）
- **对话框角色**: `x`（序列 X）
- **algorithm_help purpose（对齐）**: 计算两列对齐序列在正负滞后上的互相关，并画置信带。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
两序列互相关，找领先/滞后关系与同步性。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 伪相关风险；需平稳或去趋势；不能证明因果。

**典型样本量**: ≥50 对齐点；预白化有时必要。

**制造场景（列名示例）**
CCF 查环境温度领先于尺寸偏移多少小时。

**常见误用**
- 非平稳伪相关
- 不对齐时间戳
- 多重滞后未校正

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Cross-Correlation](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc44.htm) — accessed 2026-09-03
- [Minitab Cross Correlation](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/time-series/cross-correlation/) — accessed 2026-09-03

### chi_square — 列联表卡方

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 列联表卡方
- **对话框角色**: `row_category`（行分类列）
- **algorithm_help purpose（对齐）**: 检验两个分类变量是否独立。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
检验两个分类变量是否独立，或比较多个组的分类比例是否相同（列联表分析）。

**不能当什么用**
期望频数过小需 Fisher 精确或合并类别；不能用于连续测量；配对二元数据用 McNemar；不能推断因果。

**典型样本量**: 期望频数≥5 的格子占多数；总样本常≥50；2×2 小样本用精确法。

**制造场景（列名示例）**
班次与缺陷类型：列「班次」「缺陷类型」「计数」，检验缺陷类型分布是否因班次而异。

**常见误用**
- 期望过小仍用 Pearson 卡方
- 把关联当成因果
- 未区分独立与配对
- 合并类别不当。

**建议 dataset_id**: `cat_shift_line`

**权威来源**
- [NIST/SEMATECH e-Handbook — Chi-Square Test for Independence](https://www.itl.nist.gov/div898/handbook/prc/section4/prc44.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 3](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [ASQ — Chi-Square Test](https://asq.org/quality-resources/chi-square) — accessed 2026-09-03

### chi_square_gof — 卡方拟合优度

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 卡方拟合优度
- **对话框角色**: `category`（分类列）
- **algorithm_help purpose（对齐）**: 检验一个分类变量的观察频数是否符合指定或等比例。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
检验观测分类频数是否符合指定理论比例（如缺陷类型是否符合历史分布、班次产量分类是否符合计划比例）。

**不能当什么用**
不能用于连续数据；参数需从数据估计时损失自由度；期望频数过小需合并类别；不能验证复杂分布族。

**典型样本量**: 各类期望≥5；总样本≥50 较常见；类别数不宜过多。

**制造场景（列名示例）**
四类外观缺陷：列「缺陷类型」「计数」，检验当前周缺陷构成是否与「标准构成比例」一致。

**常见误用**
- 用样本比例当理论比例未校正自由度
- 类别划分过细
- 期望过小
- 与独立性卡方混淆。

**建议 dataset_id**: `gof_category_bias`

**权威来源**
- [NIST/SEMATECH e-Handbook — Chi-Square Goodness-of-Fit](https://www.itl.nist.gov/div898/handbook/prc/section4/prc43.htm) — accessed 2026-09-03
- [Montgomery — Statistical Quality Control, Ch. 7](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+8th+Edition-p-9781119726764) — accessed 2026-09-03
- [ASQ — Goodness of Fit](https://asq.org/quality-resources/chi-square) — accessed 2026-09-03

### cluster_observations — 层次聚类（观测）

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 层次聚类（观测）
- **对话框角色**: `variables`（数值变量（可多选））
- **algorithm_help purpose（对齐）**: 对多维观测做 complete linkage 凝聚层次聚类，并按 k 切簇。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
无监督地把相似观测分群，生成树状图与簇标签（探索分组）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 簇数主观；不能证明工艺类别；结果对距离/链接敏感。

**典型样本量**: n=30–500；维数不宜过高。

**制造场景（列名示例）**
多传感器波形层次聚类，发现异常批次簇。

**常见误用**
- k 随意选
- 未标准化
- 把聚类当分类真值

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Cluster](https://www.itl.nist.gov/div898/handbook/eda/section3/hclus.htm) — accessed 2026-09-03
- [Minitab Cluster Observations](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/multivariate/cluster-observations/) — accessed 2026-09-03

### cluster_variables — 变量聚类

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 变量聚类
- **对话框角色**: `variables`（变量列）
- **algorithm_help purpose（对齐）**: 变量层次聚类：Pearson 相关距离、连结合并、dendrogram 与 amalgamation 表。
- **interpretation_limits**: 禁止与观测量聚类混读。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
对变量按相关结构聚类，简化多变量集或选代表变量。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 不能替代领域知识删变量。

**典型样本量**: n≥50；变量数 5–30。

**制造场景（列名示例）**
20 个尺寸变量聚类选代表尺寸做 SPC。

**常见误用**
- 样本小相关不稳
- 混用 Pearson/Spearman
- 代表变量选择无验证

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab Cluster Variables](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/multivariate/cluster-variables/) — accessed 2026-09-03
- [NIST Variable Clustering](https://www.itl.nist.gov/div898/handbook/eda/section3/hclus.htm) — accessed 2026-09-03

### cochran_q — Cochran Q 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Cochran Q 检验
- **对话框角色**: `variables`（≥3 列配对二元）
- **algorithm_help purpose（对齐）**: 配对二元、k≥3 处理的边际阳性率检验。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
随机区组设计中，多个相关二元处理（成功/失败）的比例是否相同，如多名评价者对多种方案的是/否判定。

**不能当什么用**
仅适用于二元响应；不能用于连续测量；区组内处理数≥3；不能替代重复测量二项模型。

**典型样本量**: 区组数 b≥10、处理数 k≥3 较常见；每格期望成功/失败不宜过小。

**制造场景（列名示例）**
三名检验员对四种外观标准：列「检验员」「标准方案」「判定_合格」，比较四种标准下合格率是否一致。

**常见误用**
- 把独立样本当区组
- 用于非二元数据
- 区组数过少
- 与卡方独立性检验混淆。

**建议 dataset_id**: `cochran_three_repeat`

**权威来源**
- [NIST/SEMATECH e-Handbook — Cochran's Q Test](https://www.itl.nist.gov/div898/handbook/prc/section4/prc48.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 4](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [ASQ — Cochran Q Test](https://asq.org/quality-resources/hypothesis-testing) — accessed 2026-09-03

### correlation — 相关分析

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 相关分析
- **对话框角色**: `variables`（变量（至少两列））
- **algorithm_help purpose（对齐）**: 度量两列及以上数值的线性（Pearson）或单调（Spearman）相关，并给出协方差矩阵与可选偏相关。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
量化两个连续变量之间的线性关联强度与方向（如温度与尺寸偏移、压力与焊点强度），用于探索性分析与回归建模前筛查。

**不能当什么用**
不能推断因果关系；不能替代实验设计验证因子效应；对非线性关系线性相关系数可能接近零；不能替代测量系统分析。

**典型样本量**: 探索性分析 n≥25–30 较稳定；报告相关时常用 n=30–100；极端值会强烈影响 Pearson r。

**制造场景（列名示例）**
回流焊工艺：列「炉温_℃」「元件偏移_um」「产品型号」「轨道号」，分析炉温与偏移的线性关联，为后续回归或 DOE 提供线索。

**常见误用**
- 把显著相关当成因果
- 忽略散点图直接看 r
- 未检查线性假设
- 把不同批次/工况混合后得出虚假相关。

**建议 dataset_id**: `corr_temp_offset_y`

**权威来源**
- [NIST/SEMATECH e-Handbook — Correlation](https://www.itl.nist.gov/div898/handbook/eda/section3/eda35c.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 2](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [ASQ — Correlation Analysis](https://asq.org/quality-resources/correlation) — accessed 2026-09-03

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

**典型样本量**: Multiple events across units: often 20+ units with 50+ total recurrent events.

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- Treating recurrent events as independent
- ignoring within-unit correlation
- wrong risk set.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Recurrent Events](https://www.itl.nist.gov/div898/handbook/apr/section5/apr57.htm) — accessed 2026-09-03
- [Minitab: Cox Regression (Recurrent)](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/how-to/cox-regression/) — accessed 2026-09-03

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

**典型样本量**: Rule of thumb: ≥10 events per covariate; often 50–100+ total with censoring.

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- Ignoring PH assumption
- including covariates with perfect separation
- tied event handling.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Cox Proportional Hazards](https://www.itl.nist.gov/div898/handbook/apr/section5/apr56.htm) — accessed 2026-09-03
- [Minitab: Cox Regression](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/how-to/cox-regression/) — accessed 2026-09-03

### cross_tabulation — 交叉表

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 交叉表
- **对话框角色**: `row_category`（行分类列）
- **algorithm_help purpose（对齐）**: 独立交叉表：观察频数与行%/列%/合计%，不做卡方检验。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
汇总两个或多个分类变量的频数与百分比，形成列联表，为卡方检验、帕累托筛选提供数据结构。

**不能当什么用**
本身不做显著性检验；不能处理连续变量（需先分箱）；稀疏表解释需谨慎；不能替代抽样设计。

**典型样本量**: 视单元格数而定；每格期望不宜过小；报告时同时给出边际合计。

**制造场景（列名示例）**
不良原因×产线：列「产线」「不良原因」「件数」，生成交叉表查看各线主要不良模式。

**常见误用**
- 把描述性交叉表当成已检验显著
- 百分比基数选错（行% vs 列%）
- 类别划分不一致
- 忽略缺失类别。

**建议 dataset_id**: `cat_shift_line`

**权威来源**
- [NIST/SEMATECH e-Handbook — Contingency Tables](https://www.itl.nist.gov/div898/handbook/prc/section4/prc44.htm) — accessed 2026-09-03
- [AIAG — SPC Manual, Attribute Data Tabulation](https://www.aiag.org/store/quality/) — accessed 2026-09-03
- [ASQ — Contingency Tables](https://asq.org/quality-resources/chi-square) — accessed 2026-09-03

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

**典型样本量**: Minimum 2k+1 (even k) or 2k+3 (odd k); k≤4 uses 13 runs; +4 extra runs recommended.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Using for confirmatory RSM alone
- skipping extra runs
- too many active effects for run size.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Jones & Nachtsheim (2011): Definitive Screening Designs](https://www.jmp.com/en/statistics-knowledge-portal/design-of-experiments/screening-designs/definitive-screening-designs) — accessed 2026-09-03
- [Minitab: Create Definitive Screening Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/definitive-screening/create-definitive-screening-design/) — accessed 2026-09-03

### descriptive — 显示描述性统计

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 显示描述性统计
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 概括一列或多列数值的位置、散布和形状，并可按 By 变量分组。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
概括数值列的位置、散布与形状（均值、标准差、四分位、偏度、峰度），可按班次、产线、模腔等分组对比数据概况。

**不能当什么用**
不能替代假设检验、能力指数或规格判定；不能仅凭偏度/峰度下分布结论；不能判断过程是否受控。

**典型样本量**: 探索阶段每组 n≥5 即有参考价值；报告中心趋势与散布时常用 n=20–100；分组对比时各组宜尽量均衡。

**制造场景（列名示例）**
SMT 锡膏印刷后测高：列「锡膏高度_um」「班次」「产线」「钢网编号」，先按班次分组查看均值与标准差，了解各班散布差异。

**常见误用**
- 把描述统计当成合格判定
- 忽略缺失值个数 N*
- 在严重偏态数据上过度解读均值
- 未分组就混合不同工况的数据。

**建议 dataset_id**: `desc_unimodal_stable`

**权威来源**
- [NIST/SEMATECH e-Handbook — Exploratory Data Analysis](https://www.itl.nist.gov/div898/handbook/eda/section2/eda29.htm) — accessed 2026-09-03
- [ASQ — Descriptive Statistics](https://asq.org/quality-resources/descriptive-statistics) — accessed 2026-09-03
- [AIAG — Statistical Process Control (SPC) Manual, 2nd ed.](https://www.aiag.org/store/quality/) — accessed 2026-09-03

### discriminant — 线性判别分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 线性判别分析
- **对话框角色**: `response`（类别响应）
- **algorithm_help purpose（对齐）**: 用等协方差线性判别对类别响应分类并报告混淆矩阵。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
已知类别标签下，找线性组合区分类别并评估误分类率。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 需多元正态与同协方差近似；不能用于发现新类。

**典型样本量**: 每类 n≥20–30；总 n≥60。

**制造场景（列名示例）**
合格/返工品多尺寸 LDA 找区分边界。

**常见误用**
- 训练集上评估过乐观
- 类不平衡忽略
- 高维小样本过拟合

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab Discriminant](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/multivariate/discriminant-analysis/) — accessed 2026-09-03
- [NIST Discriminant](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc43.htm) — accessed 2026-09-03

### distribution_calculator — 分布计算器

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 分布计算器
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: 正态/t/χ²/F/Weibull 的 PDF、CDF、分位数工具。
- **interpretation_limits**: 禁止分布已正态 / 过程合格。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
查 PDF/CDF/分位数、概率换算，教学与规格限概率计算。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 不能代替数据拟合分布；参数需明确来源。

**典型样本量**: 不适用（理论计算）。

**制造场景（列名示例）**
正态 N(10,0.2) 下 P(X>10.5) 用于规格裕量估算。

**常见误用**
- 分布族选错
- 把理论分位当过程证据
- 单位换算错误

**权威来源**
- [NIST Distributions](https://www.itl.nist.gov/div898/handbook/eda/section3/eda366.htm) — accessed 2026-09-03
- [Minitab Probability Distributions](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/probability-distributions/) — accessed 2026-09-03

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

**典型样本量**: k=3: 15 runs; k=4: 27; k=5: 46; add center replicates.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Using BBD outside design region
- insufficient center points
- assuming rotatability.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Box-Behnken Designs](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Box-Behnken Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/response-surface/create-response-surface-design/) — accessed 2026-09-03

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

**典型样本量**: 2^k + 2k + cp: e.g., k=3 → 8+6+5≈20 runs; rotatable or face-centered choice affects α.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- α too large causing run failures
- too few center points for pure error
- blocking ignored when needed.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Central Composite Designs](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Create Response Surface Design (CCD)](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/response-surface/create-response-surface-design/) — accessed 2026-09-03

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

**典型样本量**: At least number of model terms + 3–5 for lack-of-fit; often 1.5–2× parameter count.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Under-specified model
- too few runs for chosen order
- not verifying |X'X| condition
- ignoring practical constraints in algorithm.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Optimal Designs](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Create Custom Design (D-Optimal)](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/factorial/create-custom-design/) — accessed 2026-09-03

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

**典型样本量**: 2^k runs: 8 for k=3, 16 for k=4; add 3–5 center points; replicate for error estimate.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Confounding active interactions
- not randomizing
- changing factors between runs improperly
- ignoring significant curvature.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: Design and Analysis of Experiments — Factorial Designs](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [NIST e-Handbook: Factorial Designs](https://www.itl.nist.gov/div898/handbook/pri/section3/pri3.htm) — accessed 2026-09-03
- [Minitab: Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/factorial/create-factorial-design/) — accessed 2026-09-03

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

**典型样本量**: Standard: 12-run PB for up to 11 factors; 20-run for up to 19; 8-run for up to 7.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Assuming interactions are negligible when they exist
- not folding over to de-alias
- too many factors for run size.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Fractional Factorial & Plackett-Burman](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Create Plackett-Burman Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/plackett-burman/create-plackett-burman-design/) — accessed 2026-09-03

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

**典型样本量**: Inherits from design: minimum 2^k; power analysis may require replication.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Declaring significance without checking residuals
- ignoring aliasing in fractional designs
- not confirming with verification runs.

**建议 dataset_id**: `doe_factorial_y`

**权威来源**
- [Montgomery: DOE — Analysis of Factorial Designs](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Analyze Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/factorial/analyze-factorial-design/) — accessed 2026-09-03

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

**典型样本量**: Strive for ≥10 parts per operator where possible; total measurements ≥60–90.

**制造场景（列名示例）**
量具 MSA：`零件号`、`操作员`、`测量值_mm`

**常见误用**
- Extreme imbalance causing non-estimable components
- treating missing as zero
- not using REML/appropriate unbalanced ANOVA.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [AIAG MSA Manual (4th ed.)](https://www.aiag.org/quality/msa) — accessed 2026-09-03
- [Minitab: Expanded Gage R&R (Unbalanced)](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/) — accessed 2026-09-03

### factor_analysis — 因子分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 因子分析
- **对话框角色**: `variables`（变量列）
- **algorithm_help purpose（对齐）**: 探索性因子分析：相关阵主成分提取、Loadings、% Var、Communalities 与 Scree 图；可选 Varimax。
- **interpretation_limits**: 禁止宣称与 Minitab golden 对齐。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
探索潜在因子结构、载荷与共同度（问卷/多传感器结构假设）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 不能证明潜变量存在；旋转解不唯一。

**典型样本量**: n≥100 或 n≥5×变量数；KMO 需检查。

**制造场景（列名示例）**
多道外观评分探索潜在「表面质量」因子。

**常见误用**
- 样本过小
- 不做旋转与残差检查
- 与 PCA 混用目的

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab Factor Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/multivariate/factor-analysis/) — accessed 2026-09-03
- [NIST Factor](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc42.htm) — accessed 2026-09-03

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

**典型样本量**: Adequate events per cause: often 30+ per primary cause of interest.

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- Confusing subdistribution with cause-specific hazard
- censoring competing events incorrectly.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Competing Risks](https://www.itl.nist.gov/div898/handbook/apr/section5/apr58.htm) — accessed 2026-09-03
- [Minitab: Fine-Gray Competing Risks Regression](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/how-to/fine-gray-competing-risks-regression/) — accessed 2026-09-03

### fisher_exact — Fisher 精确检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Fisher 精确检验
- **对话框角色**: `variables`（两列分类（2×2））
- **algorithm_help purpose（对齐）**: 对恰好 2×2 的分类交叉表做 Fisher 双侧精确检验，并报告可选优势比。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
2×2 列联表小样本或期望频数不足时，精确检验两分类变量是否独立（或两比例差异）。

**不能当什么用**
大样本时计算量大且与卡方结论相近；不能用于大于 2×2 表（用 Freeman-Halton 等）；不能证明因果。

**典型样本量**: 总频数<40 或期望<5 时常用；单侧/双侧需事先指定。

**制造场景（列名示例）**
小批量失效模式：列「失效类型」「供应商」，共 24 件样本，精确检验失效类型与供应商是否关联。

**常见误用**
- 大样本仍用精确检验浪费计算
- 与卡方结论不一致时不检查期望频数
- 独立/配对设计混淆。

**建议 dataset_id**: `fisher_small_counts`

**权威来源**
- [NIST/SEMATECH e-Handbook — Fisher's Exact Test](https://www.itl.nist.gov/div898/handbook/prc/section4/prc50.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 3](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [ASQ — Fisher Exact Test](https://asq.org/quality-resources/hypothesis-testing) — accessed 2026-09-03

### friedman — Friedman 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Friedman 检验
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 区组设计下比较多个处理的位置（非参数重复测量）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
随机区组（重复测量）设计中，多个处理水平的非参数比较，每个区组接受全部处理。

**不能当什么用**
不能用于完全独立样本；不能评估交互；区组效应需合理；不能替代双因素方差分析（有重复时）。

**典型样本量**: 处理数 k≥3、区组数 b≥10 较常见；与重复测量 ANOVA 对应。

**制造场景（列名示例）**
三台检具交叉：列「测量值」「检具编号」「零件编号」，同一零件在三台检具上测量，比较检具间系统性差异。

**常见误用**
- 把独立组数据当区组设计
- 区组不可比
- 忽略处理间事后比较
- 与 Kruskal-Wallis 混淆。

**建议 dataset_id**: `friedman_three_treat`

**权威来源**
- [NIST/SEMATECH e-Handbook — Friedman Test](https://www.itl.nist.gov/div898/handbook/prc/section3/prc38.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 4](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [ASQ — Friedman Test](https://asq.org/quality-resources/nonparametric-tests) — accessed 2026-09-03

### general_manova — General MANOVA

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → General MANOVA
- **对话框角色**: `responses`（响应）
- **algorithm_help purpose（对齐）**: 2～4 响应 + 1～2 因子 + 可选协变量；Type III SSCP；Wilks/Pillai/LH/Roy 按效应。
- **interpretation_limits**: 禁止过程已合格 / 已证明差异。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
多因子/协变量下多响应 Type III MANOVA。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 设计不平衡时解释复杂；不能自动选模型。

**典型样本量**: 总 n≥60；每单元格足够。

**制造场景（列名示例）**
温度×湿度对多尺寸响应的 MANOVA。

**常见误用**
- 交互项乱加
- 不平衡设计误读主效应
- 不做假设诊断

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab General MANOVA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/anova/multivariate-analysis-of-variance/) — accessed 2026-09-03
- [NIST GLM Multivariate](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc43.htm) — accessed 2026-09-03

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

**典型样本量**: 2×2×2 minimum 8 cells; with replication n≥16–24+ for error estimation.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Interpreting 3-way interaction without plot
- too few replicates
- not simplifying model after insignificance.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Factorial Extensions](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: General Linear Model](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/anova/how-to/general-linear-model/) — accessed 2026-09-03

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

**典型样本量**: Balanced: ≥2 replicates per cell; unbalanced: enough per cell for stable MSE (often 3–5+).

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Pooling interactions without testing
- using GLM for split-plot
- not checking normality/equal variance.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Two-Factor Factorial](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: GLM Two-Way ANOVA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/anova/how-to/general-linear-model/general-linear-model/) — accessed 2026-09-03

### isolation_forest — Isolation Forest

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Isolation Forest
- **对话框角色**: `variables`（数值变量（可多选））
- **algorithm_help purpose（对齐）**: 对多维数值观测计算孤立分数，标记相对孤立点。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
无监督多维异常检测，标记孤立点（探索性筛查）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 不能自动定根因；阈值需业务校准；与单变量 outlier 检验互补。

**典型样本量**: n≥100；异常比例不宜过高。

**制造场景（列名示例）**
多通道过程数据孤立森林标可疑批次。

**常见误用**
- 把异常分数当删除指令
- 未追查测量错误
- 高维诅咒未降维

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Liu Isolation Forest](https://doi.org/10.1109/ICDM.2008.17) — accessed 2026-09-03
- [scikit-learn IsolationForest](https://scikit-learn.org/stable/modules/generated/sklearn.ensemble.IsolationForest.html) — accessed 2026-09-03

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

**典型样本量**: ≥20–30 with sufficient events; more for stable CI at high percentiles.

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- Ignoring censoring
- assuming independence
- interpreting KM beyond last event time.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Kaplan-Meier](https://www.itl.nist.gov/div898/handbook/apr/section2/apr215.htm) — accessed 2026-09-03
- [Minitab: Nonparametric Distribution Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/how-to/nonparametric-distribution-analysis-right-censoring/) — accessed 2026-09-03

### kmeans — K-Means 聚类

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → K-Means 聚类
- **对话框角色**: `variables`（数值变量（可多选））
- **algorithm_help purpose（对齐）**: 把多维数值观测分成 k 个簇，并报告质心与簇内平方和。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过、规格已满足或必须删点。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
指定 k 的划分式聚类，快速分群（探索）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 对初值敏感；球形簇假设；不能给层次结构。

**典型样本量**: n≥30；k<√(n/2) 经验。

**制造场景（列名示例）**
k=3 对焊接能量曲线聚类分正常/边缘/异常。

**常见误用**
- k 拍脑袋
- 不同 seed 结果不稳
- 未标准化

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST k-means](https://www.itl.nist.gov/div898/handbook/eda/section3/kmeans.htm) — accessed 2026-09-03
- [Minitab Cluster k-means](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/multivariate/cluster-observations/) — accessed 2026-09-03

### kruskal_wallis — Kruskal-Wallis 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Kruskal-Wallis 检验
- **对话框角色**: `response`（测量值）
- **algorithm_help purpose（对齐）**: 不假设正态，比较 k 组位置。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
三个及以上独立组的非参数位置比较，正态或等方差不满足时替代单因素 ANOVA。

**不能当什么用**
不能定位具体组对差异（需事后非参数比较）；假定各组分布形状相似；不能处理因子交互；不等散布时解释受限。

**典型样本量**: 每组 n≥5–8；总样本越大渐近越好；与 ANOVA 一样需事后比较。

**制造场景（列名示例）**
四操作员量测：列「测量值_mm」「操作员」「零件编号」，非参数比较四名操作员测量分布位置。

**常见误用**
- 显著后不做事后比较
- 组间方差/形状差异大仍解读为位置差
- 与单因素 ANOVA 结论矛盾时不检查假设。

**建议 dataset_id**: `kw_three_cavity`

**权威来源**
- [NIST/SEMATECH e-Handbook — Kruskal-Wallis Test](https://www.itl.nist.gov/div898/handbook/prc/section3/prc37.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 3](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [ASQ — Kruskal-Wallis](https://asq.org/quality-resources/nonparametric-tests) — accessed 2026-09-03

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

**典型样本量**: ≥10 failures (NIST); 20–30+ for regression extensions.

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- Using lognormal by default
- not checking log-scale normality
- ignoring competing risks.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Lognormal Distribution](https://www.itl.nist.gov/div898/handbook/apr/section3/apr312.htm) — accessed 2026-09-03
- [Minitab: Distribution ID / Lognormal](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/how-to/distribution-id-right-censoring/) — accessed 2026-09-03

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

**典型样本量**: ≥30–50 observations with adequate events across covariate levels.

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- Ignoring censoring
- assuming same shape across groups
- multicollinearity in covariates.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Regression with Life Data](https://www.itl.nist.gov/div898/handbook/apr/section5/apr55.htm) — accessed 2026-09-03
- [Minitab: Regression with Life Data](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/how-to/regression-with-life-data/) — accessed 2026-09-03

### logistic_regression — 二元 Logistic 回归

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 二元 Logistic 回归
- **对话框角色**: `response`（二元响应）
- **algorithm_help purpose（对齐）**: 用对数几率模型预测二元事件概率。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
二元合格/不合格、通过/失败与预测变量的 logit 关系建模与 odds 比。

**不能当什么用**
不能用于连续响应；分离完全时估计不稳定；因果需实验设计支持。

**典型样本量**: 每系数至少 10–15 个事件；总 n≥50。

**制造场景（列名示例）**
预测锡膏类型+炉温组合下虚焊发生概率。

**常见误用**
- 稀有事件样本不足
- 把概率当计数
- 忽略类不平衡

**建议 dataset_id**: `logit_pass_fail`

**权威来源**
- [NIST Logistic](https://www.itl.nist.gov/div898/handbook/pmd/section6/pmd63.htm) — accessed 2026-09-03
- [Minitab Binary Logistic](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/regression/binary-logistic-regression/) — accessed 2026-09-03

### mann_whitney — Mann-Whitney 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Mann-Whitney 检验
- **对话框角色**: `variables`（两列独立样本）
- **algorithm_help purpose（对齐）**: 不假设正态，比较两独立样本的位置。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
两个独立组连续或有序数据的非参数位置比较（不要求正态），比较两分布是否位置不同。

**不能当什么用**
不能检验方差差异；假定观测独立；对形状差异敏感时解释需谨慎；不能用于配对数据。

**典型样本量**: 每组 n≥8–10 较常见；大样本时渐近正态；检验功效通常低于 t 检验（正态时）。

**制造场景（列名示例）**
两种清洗剂效果：列「残留_mg」「清洗剂类型」「零件号」，比较两种清洗剂去除残留的效果分布。

**常见误用**
- 配对数据误用
- 默认检验的是中位数（实际检验分布位置）
- 忽略两组形状差异
- 与双样本 t 结论矛盾时不调查原因。

**建议 dataset_id**: `infer_two_sample_location`

**权威来源**
- [NIST/SEMATECH e-Handbook — Mann-Whitney Test](https://www.itl.nist.gov/div898/handbook/prc/section3/prc35.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 3 (Nonparametric)](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [ASQ — Nonparametric Tests](https://asq.org/quality-resources/nonparametric-tests) — accessed 2026-09-03

### manova_one_way — 单因子 MANOVA

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 单因子 MANOVA
- **对话框角色**: `responses`（响应）
- **algorithm_help purpose（对齐）**: 2～4 连续响应 + 1 分类因子；Wilks/Pillai/LH/Roy；H/E SSCP 与特征值。
- **interpretation_limits**: 禁止过程已合格 / 已证明差异。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
单因子对多个连续响应同时检验组间差异（整体 Wilks 等）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 需多元正态与同协方差；不能替代逐响应 ANOVA 的规划。

**典型样本量**: 每组 n≥20；响应数 2–4 常见。

**制造场景（列名示例）**
三供应商同时比较长度、宽度、高度三组均值。

**常见误用**
- 不做多变量正态检查
- 显著后不查 univariate
- 响应高度相关仍多检验

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST MANOVA](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc43.htm) — accessed 2026-09-03
- [Minitab MANOVA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/anova/multivariate-analysis-of-variance/) — accessed 2026-09-03

### mcnemar — McNemar 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → McNemar 检验
- **对话框角色**: `variables`（两列配对二元结果）
- **algorithm_help purpose（对齐）**: 配对二元结果的边际比例检验（2×2 不一致对）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
配对二元数据，比较同一批对象在两种条件下的不一致比例是否对称（如两种检测方法、改机前后合格状态）。

**不能当什么用**
不能用于独立两比例比较；仅两个相关分类；期望不一致数过小时用精确检验；多于两水平需扩展方法。

**典型样本量**: 不一致对数 b+c≥10 较稳；小样本用精确二项；配对对象需一一对应。

**制造场景（列名示例）**
AOI 与人工复检：列「AOI判定」「人工判定」「板号」，评估两种检测方法对不良判定是否一致。

**常见误用**
- 独立样本误用两比例检验
- 忽略 discordant pairs 过少
- 把 McNemar 当成普通 2×2 卡方。

**建议 dataset_id**: `mcnemar_paired_binary`

**权威来源**
- [NIST/SEMATECH e-Handbook — McNemar Test](https://www.itl.nist.gov/div898/handbook/prc/section4/prc49.htm) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control, Ch. 10](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+8th+Edition-p-9781119726764) — accessed 2026-09-03
- [ASQ — McNemar Test](https://asq.org/quality-resources/hypothesis-testing) — accessed 2026-09-03

### mixed_effects_reml — 混合效应 REML

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 混合效应 REML
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 1～2 随机因子 + 1～2 固定因子 + 可选协变量；REML 方差分量与固定效应 BLUE。
- **interpretation_limits**: 禁止测量系统合格判定。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
固定效应+随机效应（批次、操作员）方差分量与 BLUE 估计。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 随机水平需代表总体；小样本方差分量不稳。

**典型样本量**: 随机水平≥5；总 n≥50。

**制造场景（列名示例）**
操作员固定、批次随机对尺寸的混合模型。

**常见误用**
- 随机当固定
- 嵌套/交叉搞错
- REML 与 ML 混用比较

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab Mixed Models](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/anova/general-linear-model/) — accessed 2026-09-03
- [NIST Variance Components](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc41.htm) — accessed 2026-09-03

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

**典型样本量**: Per design points; need ≥ df in model + 3 for error estimation.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Using standard polynomial without Scheffé form
- misinterpreting trace plots
- not validating on holdout blends.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Mixture Model Analysis](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Analyze Mixture Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/mixture/analyze-mixture-design/) — accessed 2026-09-03

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

**典型样本量**: Simplex lattice degree m: C(q+m-1, m) points; q=3,m=2 → 6 vertices + center; add replicates.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Treating components as independent
- not re-scaling when totals differ
- ignoring pseudo-components for narrow ranges.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Mixture Experiments](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Create Mixture Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/mixture/create-mixture-design/) — accessed 2026-09-03

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

**典型样本量**: Vertices + edge centroids + overall centroid; typically 10–30 runs for q=3–4 with constraints.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Missing feasible vertices
- not checking constraint consistency
- extrapolating outside polytope.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Constrained Mixture Designs](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Extreme Vertices Mixture Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/mixture/create-mixture-design/) — accessed 2026-09-03

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

**典型样本量**: Mixture points × process levels: e.g., 7 mixture × 3 temp = 21+ runs.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Not separating mixture vs process effects
- insufficient replicates for combined model
- ignoring collinearity.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Mixtures with Process Variables](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Mixture Design with Process Variables](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/mixture/) — accessed 2026-09-03

### mood_median — Mood 中位数检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Mood 中位数检验
- **对话框角色**: `response`（测量值）
- **algorithm_help purpose（对齐）**: 不假设正态，比较 k 组中位数（基于总体中位数的 2×k 表）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
比较三个及以上独立组的中位数是否全部相等，对异常值比均值检验更稳健。

**不能当什么用**
功效常低于 Kruskal-Wallis；仍假定组间分布形状相似；不能用于配对数据；显著后需事后比较。

**典型样本量**: 每组 n≥10 较常见；小样本渐近近似不稳。

**制造场景（列名示例）**
五供应商硬度：列「硬度_HV」「供应商」「炉次」，比较各供应商批次硬度的中位水平。

**常见误用**
- 与「检验均值」混淆
- 形状差异大时误判
- 不做事后两两比较
- 样本量过小。

**建议 dataset_id**: `mood_two_group`

**权威来源**
- [NIST/SEMATECH e-Handbook — Mood's Median Test](https://www.itl.nist.gov/div898/handbook/prc/section3/prc39.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 3](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [ASQ — Median Tests](https://asq.org/quality-resources/nonparametric-tests) — accessed 2026-09-03

### multiple_correspondence — 多重对应分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 多重对应分析
- **对话框角色**: `variables`（分类变量（逗号列号））
- **algorithm_help purpose（对齐）**: 3～6 列分类；指示矩阵；Column Contributions。
- **interpretation_limits**: 禁止全量 MCA 宣称。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
≥3 分类变量 MCA，探索多属性共现（问卷、检查项）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 缺失与稀有类别敏感；解释需谨慎。

**典型样本量**: n≥150；变量 3–6 常见。

**制造场景（列名示例）**
多项外观检查项 MCA 找常见缺陷组合。

**常见误用**
- 类别未合并导致稀疏
- 与 PCA 混淆
- 不做惯性贡献解读

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab MCA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/multivariate/multiple-correspondence-analysis/) — accessed 2026-09-03
- [Greenacre MCA](https://doi.org/10.1201/9781315139470-14) — accessed 2026-09-03

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

**典型样本量**: Multiple systems × event times: e.g., 10 machines tracked over 2 years with 50+ events total.

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- Treating repair times as failures
- ignoring system age
- assuming homogeneous Poisson when trend exists.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Repairable Systems](https://www.itl.nist.gov/div898/handbook/apr/section5/apr57.htm) — accessed 2026-09-03
- [Minitab: Parametric Growth Curve](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/how-to/parametric-growth-curve/) — accessed 2026-09-03

### nominal_logistic — 名义 Logistic 回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 名义 Logistic 回归
- **对话框角色**: `response`（名义响应）
- **algorithm_help purpose（对齐）**: 对名义（无序）多水平响应拟合广义 logit 模型。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
无序多类缺陷类型/故障模式与预测变量的广义 logit 建模。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 类别过多导致稀疏；不能自动选根因。

**典型样本量**: 每类至少 20–30；总 n≥100。

**制造场景（列名示例）**
三类失效模式（开路/短路/外观）与工艺参数名义 logit。

**常见误用**
- 有序等级误用名义模型
- 参照水平选择影响 OR 解释
- 样本极不均衡

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab Nominal Logistic](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/regression/nominal-logistic-regression/) — accessed 2026-09-03
- [NIST GLM](https://www.itl.nist.gov/div898/handbook/pmd/section6/pmd63.htm) — accessed 2026-09-03

### nonlinear_regression — 非线性回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 非线性回归
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 单 Y + 单 X；内置模型；GN/LM；参数表 + Summary of Fit。
- **interpretation_limits**: 禁止外推无验证。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
机理已知但曲线非线性的单 Y–X 关系（如饱和、指数衰减）参数估计。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 初值敏感；外推危险；不能替代 DOE 找最优。

**典型样本量**: 参数个数×5–10 点；全因子范围覆盖。

**制造场景（列名示例）**
胶水固化粘度随时间非线性衰减拟合。

**常见误用**
- 模型结构错设
- 初值不当不收敛
- 局部最优当全局

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Nonlinear](https://www.itl.nist.gov/div898/handbook/pmd/section6/pmd63n.htm) — accessed 2026-09-03
- [Minitab Nonlinear Regression](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/regression/nonlinear-regression/) — accessed 2026-09-03

### normality_test — 正态性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 正态性检验
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 用 Anderson–Darling 或 Ryan–Joiner 检验一列数据是否与正态分布一致。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
评估连续测量数据与正态分布的偏离程度，为选择参数检验（t、ANOVA、能力分析）或考虑非参数/变换提供参考。

**不能当什么用**
不能因 p>α 就认定数据服从正态；小样本检验功效低；不能替代残差图、概率图等图形检查；不能单独作为删点或停线依据。

**典型样本量**: Anderson-Darling 等检验在 n=20–50 较常见；n<10 时检验功效很低；大样本时轻微偏离也可能显著。

**制造场景（列名示例）**
注塑件关键尺寸：列「尺寸_mm」「模腔」「测量时间」，对各模腔分别做正态性评估，再决定是否用参数能力分析或变换。

**常见误用**
- 把不拒绝 H₀ 说成「已确认正态」
- 只看 p 值不看直方图/正态概率图
- 对多组数据只做一次合并检验
- 未考虑测量分辨率造成的离散化。

**图形解读要点**: 看分布/关系/趋势模式与离群；图形探索不替代假设检验。

**建议 dataset_id**: `norm_mild_skew`

**权威来源**
- [NIST/SEMATECH e-Handbook — Anderson-Darling Test](https://www.itl.nist.gov/div898/handbook/prc/section1/prc11.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 3](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [ASQ — Normality Testing](https://asq.org/quality-resources/normality) — accessed 2026-09-03

### one_poisson_rate — 单样本泊松率

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单样本泊松率
- **对话框角色**: `defects`（缺陷数）
- **algorithm_help purpose（对齐）**: 检验缺陷发生速率 λ 是否等于假设值。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
检验单位面积/时间/件数上的缺陷发生强度 λ 是否等于目标值（如每百件划痕数、每小时停机次数）。

**不能当什么用**
不能用于「有/无」二元比例（那是二项）；过度离散数据可能不适合泊松；不能忽略观测窗口/面积差异而不标准化。

**典型样本量**: 合计缺陷计数≥20–30 时渐近推断较稳；比较率时需统一「检验单位」如每千件。

**制造场景（列名示例）**
外观缺陷计数：列「缺陷数」「检验面积_dm2」「产线」「检验员」，检验单位面积缺陷率是否超过「目标缺陷率」。

**常见误用**
- 未统一检验单位就比较
- 零膨胀或聚集缺陷仍用泊松
- 小计数用近似检验
- 忽略暴露量不同。

**建议 dataset_id**: `pois_one_count`

**权威来源**
- [NIST/SEMATECH e-Handbook — Poisson Process](https://www.itl.nist.gov/div898/handbook/prc/section4/prc47.htm) — accessed 2026-09-03
- [Montgomery — Statistical Quality Control, Ch. 7 (u-chart)](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+8th+Edition-p-9781119726764) — accessed 2026-09-03
- [AIAG — SPC Manual, u-Chart and Poisson Assumptions](https://www.aiag.org/store/quality/) — accessed 2026-09-03

### one_proportion — 单比例检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单比例检验
- **对话框角色**: `events`（事件数）
- **algorithm_help purpose（对齐）**: 检验事件比例是否等于假设 p₀。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
检验单个总体的缺陷率/合格率是否等于指定值（如目标不良率、历史平均水平）。

**不能当什么用**
不能用于连续测量；小样本或极端比例时正态近似不可靠；不能用于同一单元多次试验的二元相关数据。

**典型样本量**: np≥5 且 n(1−p)≥5 时正态近似较稳；计数型控制图常 n=50–200；精确二项检验可用于小 n。

**制造场景（列名示例）**
贴片不良率：列「抽检数」「不良数」「班次」「产线」，检验当前班次不良率是否高于「目标不良率」。

**常见误用**
- 样本量过小仍用正态近似
- 检验目标比例选错
- 忽略抽样框与随机性
- 把时间聚集的不良当成独立。

**建议 dataset_id**: `prop_one_lot`

**权威来源**
- [NIST/SEMATECH e-Handbook — One Proportion Test](https://www.itl.nist.gov/div898/handbook/prc/section4/prc45.htm) — accessed 2026-09-03
- [AIAG — SPC Manual, Attribute Control Charts](https://www.aiag.org/store/quality/) — accessed 2026-09-03
- [Montgomery — Statistical Quality Control, Ch. 7](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+8th+Edition-p-9781119726764) — accessed 2026-09-03

### one_proportion_equivalence — 单比例等价性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单比例等价性检验
- **对话框角色**: `events`（事件数）
- **algorithm_help purpose（对齐）**: 检验样本比例相对目标比例的差是否落入等价界限（Wald z-TOST）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
单组比例落在目标比例 ±等价限内（如不良率接近目标）。

**不能当什么用**
稀有事件 n 需足够；近似失效时用精确法。

**典型样本量**: 取决于 p 与限宽；常 n≥100。

**制造场景（列名示例）**
新线不良率与基准 2% ±1% 等价。

**常见误用**
- 小 n 正态近似
- 等价限无业务定义
- 与置信区间混淆

**建议 dataset_id**: `equiv_prop_one`

**权威来源**
- [Minitab 1 Proportion Equivalence](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/equivalence-test/) — accessed 2026-09-03
- [NIST Proportions](https://www.itl.nist.gov/div898/handbook/prc/section2/prc22.htm) — accessed 2026-09-03

### one_sample_equivalence — 单样本等价性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单样本等价性检验
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 检验均值是否落入规定等价界限内（TOST），而不是“是否等于目标”。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
证明单组均值落在目标值 ±等价限内（TOST 思路），而非仅「不等于」。

**不能当什么用**
不能用于大差异筛查；等价限需监管/工程先验；非正态需稳健或变换。

**典型样本量**: 常 n=20–40/组，取决于等价限与 σ 估计。

**制造场景（列名示例）**
新锡膏平均厚度与标准膏 ±2μm 等价验证。

**常见误用**
- 等价限过宽
- 用差异检验代替等价
- p>α 当「未证明不等」

**建议 dataset_id**: `equiv_one_near_target`

**权威来源**
- [Schuirmann TOST](https://doi.org/10.1080/03610918708829567) — accessed 2026-09-03
- [Minitab Equivalence](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/equivalence-test/) — accessed 2026-09-03

### one_sample_t — 单样本 t 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单样本 t 检验
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 检验一列均值是否等于给定 μ₀。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
检验单个连续总体的均值是否等于给定目标值（如名义尺寸、历史中心线、工艺设定值），总体方差未知时用 t 分布。

**不能当什么用**
不能检验方差或比例；严重非正态且小样本时结论不可靠；不能替代规格符合性判定；不能用于未独立同分布的序列数据而不考虑自相关。

**典型样本量**: 功效分析常用 n=10–30；AIAG 能力研究常取 n≥25–30；单侧检验需事先确定方向。

**制造场景（列名示例）**
键合拉力抽检：列「拉力_N」「批次号」「检验员」，检验本批均值是否偏离目标「标准拉力_N」。

**常见误用**
- 未检查正态性或样本量就使用
- 把目标值与规格限混淆
- 忽略数据非独立（时间序列）
- 双侧/单侧假设与业务问题不一致。

**建议 dataset_id**: `infer_one_sample_mean`

**权威来源**
- [NIST/SEMATECH e-Handbook — One-Sample t-Test](https://www.itl.nist.gov/div898/handbook/prc/section2/prc31.htm) — accessed 2026-09-03
- [Montgomery — Statistical Quality Control, Ch. 9](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+8th+Edition-p-9781119726764) — accessed 2026-09-03
- [ASQ — Hypothesis Testing](https://asq.org/quality-resources/hypothesis-testing) — accessed 2026-09-03

### one_sample_z — 单样本 Z 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单样本 Z 检验
- **对话框角色**: `variables`（测量值）
- **algorithm_help purpose（对齐）**: 在已知总体标准差 σ 时，检验一列均值是否等于给定 μ₀。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
当总体标准差 σ 已知（或样本量极大、用已知历史 σ）时，检验均值是否等于目标值；亦用于大样本近似。

**不能当什么用**
σ 未知且样本不大时不应替代 t 检验；不能用于方差未知的小样本；不能假设 σ 长期稳定而不验证。

**典型样本量**: 教科书示例常 n≥30 作 t 近似 Z；已知 σ 时 n 可较小但仍需考虑正态性。

**制造场景（列名示例）**
标准化计量室复检：列「尺寸_mm」「量具编号」，使用历史长期 σ 评估今日抽检均值是否偏离「名义尺寸_mm」。

**常见误用**
- 在 σ 实际未知时仍用 Z 检验
- 用过时的过程 σ 而未重新评估
- 忽略数据非正态。

**建议 dataset_id**: `infer_one_sample_mean`

**权威来源**
- [NIST/SEMATECH e-Handbook — Tests for Location (Z-test)](https://www.itl.nist.gov/div898/handbook/prc/section2/prc12.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 2](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [AIAG — SPC Manual, Process Capability Context](https://www.aiag.org/store/quality/) — accessed 2026-09-03

### one_way_anova — 单因素 ANOVA

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 单因素 ANOVA
- **对话框角色**: `response`（响应变量）
- **algorithm_help purpose（对齐）**: 检验多个组的均值是否全部相等。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
检验三个及以上组的连续测量均值是否全部相等（如多模腔、多机台、多操作员、多材料水平）。

**不能当什么用**
不能指出具体哪两组不同（需事后多重比较）；不能处理因子交互；严重违反正态/等方差假设时考虑 Kruskal-Wallis 或变换；不能替代随机化。

**典型样本量**: 每组 n=5–15 常见；总样本 N≥30 较稳；平衡设计功效更高；事后比较需考虑多重性。

**制造场景（列名示例）**
三模腔注塑：列「尺寸_mm」「模腔」「材料批次」「机台号」，检验三模腔均值是否存在系统性差异。

**常见误用**
- ANOVA 显著后不做事后比较
- 把「不显著」当成「各组相同」
- 忽略残差诊断
- 不等方差仍用标准 ANOVA。

**建议 dataset_id**: `anova_one_cavity`

**权威来源**
- [NIST/SEMATECH e-Handbook — One-Way ANOVA](https://www.itl.nist.gov/div898/handbook/prc/section2/prc32.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 3](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [ASQ — ANOVA](https://asq.org/quality-resources/anova) — accessed 2026-09-03

### ordinal_logistic — 有序 Logistic 回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 有序 Logistic 回归
- **对话框角色**: `response`（有序响应）
- **algorithm_help purpose（对齐）**: 对有序多水平响应拟合比例优势 logit 模型。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
有序等级响应（如外观 1–5 级）与预测变量的比例优势模型。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 比例优势假设违背时结论受限；不宜当名义分类。

**典型样本量**: 每预测变量每等级至少 10 例；总 n≥100。

**制造场景（列名示例）**
划痕严重度等级（轻/中/重）与抛光参数有序 logit。

**常见误用**
- 有序当名义或连续乱用
- 不检验比例优势
- 等级间距不等仍当等距

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab Ordinal Logistic](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/regression/ordinal-logistic-regression/) — accessed 2026-09-03
- [Agresti Categorical Data](https://www.wiley.com/en-us/Categorical+Data+Analysis%2C+3rd+Edition-p-9780470463635) — accessed 2026-09-03

### orthogonal_regression — 正交回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 正交回归
- **对话框角色**: `x`（X 列）
- **algorithm_help purpose（对齐）**: 在 X 与 Y 均有测量误差时估计等方差正交回归斜率与可选截距。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
X、Y 均有测量误差时估计线性关系（Deming/正交回归）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 需误差比 λ；X 可控无误差时用普通 OLS。

**典型样本量**: n≥30；λ 需方法学或重复性估计。

**制造场景（列名示例）**
两坐标仪互测同一批工件，X/Y 都有量测误差时拟合关系。

**常见误用**
- X 无误差仍用正交回归
- λ 随意设为 1
- 与 Passing-Bablok 混淆场景

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Orthogonal/Deming](https://www.itl.nist.gov/div898/handbook/pmd/section1/pmd14.htm) — accessed 2026-09-03
- [Minitab Orthogonal Regression](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/regression/orthogonal-regression/) — accessed 2026-09-03

### outlier_test — 异常值检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 异常值检验
- **对话框角色**: `variables`（变量）
- **algorithm_help purpose（对齐）**: 检验单个最极端观测是否能被看作异常值（Grubbs 或 Dixon r10）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
在假定数据来自单峰、近似对称总体时，识别可能远离主体分布的极端观测（如录入错误、特殊原因事件）。

**不能当什么用**
不能自动判定测量错误；不能替代控制图上的特殊原因分析；对多峰或严重偏态数据不适用；检出异常不等于必须删除。

**典型样本量**: Grubbs 等单峰检验通常 n≥7；AIAG SPC 建议子组 n=4–5 时配合控制图；大样本中统计检验过于敏感。

**制造场景（列名示例）**
光测膜厚抽检：列「膜厚_um」「抽检序号」「机台号」，对单机台数据做异常值筛查，结合「备注」列追溯是否换刀或参数调整。

**常见误用**
- 未调查原因就删点
- 对非正态或混合总体滥用单峰异常值检验
- 重复检验同一数据集而不校正 α
- 把规格超限点与统计异常值混为一谈。

**建议 dataset_id**: `outlier_one_spike`

**权威来源**
- [NIST/SEMATECH e-Handbook — Grubbs' Test for Outliers](https://www.itl.nist.gov/div898/handbook/prc/section1/prc16.htm) — accessed 2026-09-03
- [AIAG — SPC Manual, Outlier and Special Cause Guidance](https://www.aiag.org/store/quality/) — accessed 2026-09-03
- [Montgomery — Introduction to Statistical Quality Control, Ch. 5](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+8th+Edition-p-9781119726764) — accessed 2026-09-03

### paired_equivalence — 配对等价性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 配对等价性检验
- **对话框角色**: `variables`（两列配对样本）
- **algorithm_help purpose（对齐）**: 检验配对差值是否落入规定等价界限内（Paired TOST）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
配对差值在 ±δ 内等价（同一工件前后、左右对照）。

**不能当什么用**
配对差非正态且 n 小需谨慎；不能忽略周期/学习效应。

**典型样本量**: 配对 n≥16–30。

**制造场景（列名示例）**
抛光前后表面粗糙度差在等价限内的配对等价。

**常见误用**
- 独立样本方法误用
- 配对顺序效应未随机
- δ 过宽

**建议 dataset_id**: `equiv_paired_near`

**权威来源**
- [Schuirmann TOST paired](https://doi.org/10.1080/03610918708829567) — accessed 2026-09-03
- [Minitab Paired Equivalence](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/equivalence-test/) — accessed 2026-09-03

### paired_t — 配对 t 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 配对 t 检验
- **对话框角色**: `variables`（配对变量（两列））
- **algorithm_help purpose（对齐）**: 对成对观测的差值做单样本 t，检验平均差值是否为 0。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
比较同一单元在两种处理下的均值差（如改机前后、返工前后、左右配对件、同一零件两次测量），分析差值是否为零。

**不能当什么用**
不能用于独立两组；配对关系必须明确且一一对应；不能用于未配对的重复测量（需混合模型）；差值严重非正态时考虑符号秩检验。

**典型样本量**: 配对数 n=10–30 常见；n<10 时检验功效低；需保证配对可比性。

**制造场景（列名示例）**
返工前后尺寸：列「返工前_mm」「返工后_mm」「工单号」「缺陷类型」，检验返工是否系统性改变尺寸。

**常见误用**
- 把独立样本当配对
- 配对顺序搞错
- 忽略差值的正态性检查
- 未报告差值均值及置信区间。

**建议 dataset_id**: `infer_paired_shift`

**权威来源**
- [NIST/SEMATECH e-Handbook — Paired t-Test](https://www.itl.nist.gov/div898/handbook/prc/section2/prc23.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 2](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [AIAG — SPC Manual, Before/After Comparison](https://www.aiag.org/store/quality/) — accessed 2026-09-03

### pca — 主成分分析

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 主成分分析
- **对话框角色**: `variables`（数值变量（可多选））
- **algorithm_help purpose（对齐）**: 把相关数值列换成互不相关的主成分。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
降维、去相关、找主变异方向（过程监控、探索性）。

**不能当什么用**
主成分不一定有物理意义；不能替代原始规格判定；样本相关阵需足够 n。

**典型样本量**: n≥5×变量数；n≥100 更稳。

**制造场景（列名示例）**
多尺寸测量 PCA 找共变模式，服务 T² 监控。

**常见误用**
- 未标准化不同量纲
- 成分数凭感觉
- 把 PC 当独立因子因果解释

**建议 dataset_id**: `pca_three_var`

**权威来源**
- [NIST PCA](https://www.itl.nist.gov/div898/handbook/pmc/section1/pmc11.htm) — accessed 2026-09-03
- [Minitab PCA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/multivariate/principal-components-analysis/) — accessed 2026-09-03

### pls_regression — 偏最小二乘回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 偏最小二乘回归
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 1～4 响应 + 多预测；单响应 NIPALS 或 PLS2 NIPALS；LOO CV；Model Selection + Coefficients。
- **interpretation_limits**: 禁止 Minitab golden 对齐声明。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
预测变量多、共线严重时的偏最小二乘降维回归（光谱、多传感器）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 解释性弱于 OLS；需交叉验证选成分数。

**典型样本量**: n≥50；高维时 n 相对 p 仍要足够。

**制造场景（列名示例）**
近红外 200 波长预测涂层厚度 PLS。

**常见误用**
- 不 CV 选成分
- 未标准化变量
- 把载荷当因果权重

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab PLS](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/regression/partial-least-squares-regression/) — accessed 2026-09-03
- [NIPALS Wold](https://doi.org/10.1016/0022-5193(66)90014-5) — accessed 2026-09-03

### poisson_gof — 泊松拟合优度

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 泊松拟合优度
- **对话框角色**: `counts`（计数列（非负整数））
- **algorithm_help purpose（对齐）**: 检验一列非负整数计数是否服从泊松分布（估计 λ=样本均值）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
检验计数数据是否服从泊松分布（如单位面积缺陷数、单位时间事件数），为 u 图等属性控制图假设提供参考。

**不能当什么用**
不能用于二元比例；过度离散/零膨胀数据不适合；不能单独作为控制图可用性最终判定；小计数需合并区间。

**典型样本量**: 合计计数≥30–50；分组后各组期望不宜过小；需统一检验单位。

**制造场景（列名示例）**
涂装颗粒缺陷：列「缺陷数」「检验面积_dm2」「批次」，检验单位面积缺陷是否符合泊松假设以支持 u 图。

**常见误用**
- 未统一暴露量
- 缺陷聚集仍假设泊松
- 与二项拟合优度混淆
- 类别合并随意。

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST/SEMATECH e-Handbook — Poisson Distribution Tests](https://www.itl.nist.gov/div898/handbook/prc/section4/prc47.htm) — accessed 2026-09-03
- [AIAG — SPC Manual, u-Chart Assumptions](https://www.aiag.org/store/quality/) — accessed 2026-09-03
- [Montgomery — Statistical Quality Control, Ch. 7](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+8th+Edition-p-9781119726764) — accessed 2026-09-03

### poisson_regression — Poisson 回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → Poisson 回归
- **对话框角色**: `response`（计数响应）
- **algorithm_help purpose（对齐）**: 对非负计数响应拟合 log 链 Poisson GLM，并报告系数与偏差。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
计数型缺陷数/事件数随协变量变化的 Poisson GLM（率建模）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 过度离散需负二项；零膨胀另建模。

**典型样本量**: 总事件≥50；每系数有足够事件数。

**制造场景（列名示例）**
每千件划痕数与线速、湿度的 Poisson 回归。

**常见误用**
- 方差≈均值假设不成立
- 暴露量不同未用 offset
- 把比率当正态回归

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Poisson Regression](https://www.itl.nist.gov/div898/handbook/pmd/section6/pmd63.htm) — accessed 2026-09-03
- [Minitab Poisson Regression](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/regression/poisson-regression/) — accessed 2026-09-03

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

**典型样本量**: Multiple stress levels with ≥20–50 units per level; need failures and survivals at each level.

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- Too few levels to fit line
- extrapolating beyond tested stress
- ignoring overdispersion.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Probit Analysis](https://www.itl.nist.gov/div898/handbook/pri/section7/pri7.htm) — accessed 2026-09-03
- [Minitab: Probit Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/how-to/probit-analysis/) — accessed 2026-09-03

### random_forest — 随机森林

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 随机森林
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: Bagging CART 集成：多数表决/均值预测与平均不纯度下降重要性。
- **interpretation_limits**: 解释层只陈述统计证据；禁用过程合格。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
集成多树提高预测与变量重要性，非线性关系探索。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 黑箱性较单树强；外推差；小样本优势不明显。

**典型样本量**: n≥200 更稳；类别平衡需注意。

**制造场景（列名示例）**
RF 预测良率并排序关键工艺参数。

**常见误用**
- 不调树数/深度
- 泄漏未来信息
- 重要性偏类别变量

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Breiman Random Forests](https://doi.org/10.1023/A:1010933404324) — accessed 2026-09-03
- [Minitab Random Forests](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/multivariate/random-forests/) — accessed 2026-09-03

### randomization_test — 随机化检验（两样本均值差）

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 随机化检验（两样本均值差）
- **对话框角色**: `first`（样本 1）
- **algorithm_help purpose（对齐）**: 对两独立样本均值差做标签置换检验，给出双侧 P 值。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
置换/随机化检验两样本均值差，弱分布假设下的 p 值。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 自相关数据需块置换；不能补救糟糕抽样框。

**典型样本量**: 总 n≥20；置换次数≥5000。

**制造场景（列名示例）**
A/B 夹具均值差随机化检验（小样本）。

**常见误用**
- 配对当独立置换
- 观测非交换仍置换
- 只看 p 不看效应量

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Randomization Tests](https://www.itl.nist.gov/div898/handbook/prc/section2/prc22.htm) — accessed 2026-09-03
- [Minitab Randomization Test](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/randomization-test/) — accessed 2026-09-03

### regression — 线性回归

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 线性回归
- **对话框角色**: `variables`（变量（第一列响应，其余为预测变量））
- **algorithm_help purpose（对齐）**: 用一个或多个预测变量拟合带截距的线性模型。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
量化一个或多个预测变量对连续响应的线性关系，预测与筛选因子。

**不能当什么用**
不能证明因果；外推超出数据域危险；残差非独立/异方差时 OLS 推断失真。

**典型样本量**: n≥10×预测变量数；预测 n≥30 更稳。

**制造场景（列名示例）**
回流温度、链速对焊点高度的多元线性回归，优化设定窗口。

**常见误用**
- R² 高就当因果
- 忽略共线性
- 残差有模式仍用线性
- 离群点未诊断

**建议 dataset_id**: `regr_temp_strength`

**权威来源**
- [NIST Regression](https://www.itl.nist.gov/div898/handbook/pmd/section1/pmd14.htm) — accessed 2026-09-03
- [Minitab Regression](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/regression/) — accessed 2026-09-03

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

**典型样本量**: NIST: ≥10 failures for reasonable Weibull/lognormal estimates; ≥5 marginal; plan with censoring.

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- Ignoring censoring
- wrong distribution
- mixing failure modes
- insufficient follow-up time.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Reliability](https://www.itl.nist.gov/div898/handbook/apr/section1/apr1.htm) — accessed 2026-09-03
- [Minitab: Distribution Analysis (Right Censoring)](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/how-to/distribution-analysis-right-censoring/) — accessed 2026-09-03

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

**典型样本量**: Zero-failure plans: e.g., 22 units for 95%/90% with Weibull β assumption; depends on required MTBF.

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- Stopping at arbitrary time
- not pre-specifying β
- ignoring multiple failure modes
- mixing redesigns.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Test Plans](https://www.itl.nist.gov/div898/handbook/apr/section3/apr31.htm) — accessed 2026-09-03
- [Minitab: Demonstration Test Plans](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/how-to/demonstration-test-plans/) — accessed 2026-09-03

### reliability_warranty — 保修摘要

- **implemented_status**: `command_only`
- **菜单路径（代码）**: **统计** → 保修摘要
- **对话框角色**: `exposure_col`（暴露量列（可选，列求和优先于标量））

**常用来做什么**
基于失效/删失数据分析寿命与应力效应。

**不能当什么用**
不能写成产品已达标；外推需工程论证。

**典型样本量**: Combine field data (often 100s–1000s censored) with lab data; Bayesian updating helpful.

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- Assuming all customers use product equally
- ignoring reporting delay
- mixing redesign cohorts.

**建议 dataset_id**: `rel_warranty_counts`

**权威来源**
- [NIST e-Handbook: Reliability Case Studies](https://www.itl.nist.gov/div898/handbook/apr/section7/apr7.htm) — accessed 2026-09-03
- [Minitab: Warranty Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/how-to/warranty-analysis/) — accessed 2026-09-03

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

**典型样本量**: Based on underlying RSM design; confirm with 3–5 verification runs.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Optimizing on noisy model
- ignoring process constraints
- not validating optimum on production equipment.

**建议 dataset_id**: `doe_opt_two_resp`

**权威来源**
- [Montgomery: DOE — Response Surface Optimization](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Response Optimizer](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/response-surface/response-optimizer/) — accessed 2026-09-03

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

**典型样本量**: CCD: ~2^k + 2k + cp (e.g., 20 for k=3); BBD: 15 for k=3; add replicates at center.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Extrapolating beyond design region
- ignoring lack-of-fit
- active axial points too extreme causing failure runs.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Response Surface Methods](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Analyze Response Surface Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/response-surface/analyze-response-surface-design/) — accessed 2026-09-03

### runs_test — 游程检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 游程检验
- **对话框角色**: `variables`（数值序列（一列））
- **algorithm_help purpose（对齐）**: 检验一列数值按行序相对比较准则 K 是否随机（Wald–Wolfowitz / Minitab Runs）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
检验序列是否随机（游程过多或过少），用于评估时间顺序数据、抽检顺序或二元序列的随机性。

**不能当什么用**
不能检测具体分布形态；对趋势/周期不敏感（需自相关/控制图）；不能用于无顺序的数据；样本量很小时功效低。

**典型样本量**: n≥20–25 较常见；二元序列以中位数或给定阈值二分；过长序列需考虑自相关。

**制造场景（列名示例）**
抽检顺序随机性：列「测量值」「抽检序号」「机台号」，按时间顺序检验测量值高低游程是否提示非随机抽样。

**常见误用**
- 无时间/顺序意义仍做游程检验
- 用错误阈值二分
- 忽略明显趋势
- 把随机性当成独立性。

**建议 dataset_id**: `runs_clustered`

**权威来源**
- [NIST/SEMATECH e-Handbook — Runs Test for Randomness](https://www.itl.nist.gov/div898/handbook/eda/section3/eda35d.htm) — accessed 2026-09-03
- [Montgomery — Statistical Quality Control, Ch. 5](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+8th+Edition-p-9781119726764) — accessed 2026-09-03
- [AIAG — SPC Manual, Randomness and Sampling](https://www.aiag.org/store/quality/) — accessed 2026-09-03

### seasonal_forecasting — 季节性预测（Winters）

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 季节性预测
- **对话框角色**: `value`（时间序列值）
- **algorithm_help purpose（对齐）**: 对单列时间顺序数据做季节性预测（Winters）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
带季节性的 Holt-Winters 类预测。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 结构突变时预测失效；需足够季节历史。

**典型样本量**: ≥2–3 完整季节周期；每季≥3 点。

**制造场景（列名示例）**
周需求季节性 Winters 预测备料。

**常见误用**
- 季节周期设错
- 单季节数据硬套
- 不更新模型

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Winters](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc43.htm) — accessed 2026-09-03
- [Minitab Winters](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/time-series/winters-method/) — accessed 2026-09-03

### sign_test — 符号检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 符号检验
- **对话框角色**: `variables`（一列或两列配对）
- **algorithm_help purpose（对齐）**: 对单样本中位数或配对差做符号检验（二项精确），并报告中位数置信区间。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
配对数据最基础的非参数检验：差值正负是否平衡，适用于仅知方向不知大小的有序或连续配对。

**不能当什么用**
功效低于符号秩检验；不能用于独立样本；对差值幅度不敏感；不能评估效应大小。

**典型样本量**: n≥10 较有意义；小样本可用精确二项分布；零差值需处理。

**制造场景（列名示例）**
抛光前后表面粗糙度：列「抛光前_Ra」「抛光后_Ra」「工件序号」，仅方向性评估抛光是否倾向于降低粗糙度。

**常见误用**
- 有数值信息却只用符号检验浪费功效
- 零差处理不一致
- 与配对设计搞混。

**建议 dataset_id**: `infer_paired_shift`

**权威来源**
- [NIST/SEMATECH e-Handbook — Sign Test](https://www.itl.nist.gov/div898/handbook/prc/section3/prc34.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 3](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [ASQ — Sign Test](https://asq.org/quality-resources/nonparametric-tests) — accessed 2026-09-03

### simple_correspondence — 简单对应分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 简单对应分析
- **对话框角色**: `row_var`（行变量）
- **algorithm_help purpose（对齐）**: 2 列分类变量；列联表；惯性分解；行/列贡献；1～2 组件。
- **interpretation_limits**: 禁止因果结论。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
两分类变量对应分析，低维展示关联结构（行/列剖面）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 不能因果；稀疏表不稳定。

**典型样本量**: 总 n≥100；行列类别各≥3。

**制造场景（列名示例）**
缺陷位置×失效模式对应图找共现模式。

**常见误用**
- 类别过多维数难解释
- 与卡方结论不一致不追查
- 过度解读距离

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab Correspondence](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/multivariate/simple-correspondence-analysis/) — accessed 2026-09-03
- [Greenacre Correspondence](https://doi.org/10.1201/9781315139470-14) — accessed 2026-09-03

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

**典型样本量**: Inherits from design; need ≥2 whole plots per whole-plot factor level.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Using sub-plot error for whole-plot effects
- ignoring restricted randomization
- mixed model mis-specification.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Split-Plot Analysis](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Analyze Split-Plot Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/split-plot/analyze-split-plot-design/) — accessed 2026-09-03

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

**典型样本量**: Whole plots × sub-plots: e.g., 8 ovens × 4 positions = 32 runs minimum; more for power.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Analyzing as completely randomized
- too few whole plots
- confounding whole-plot effects with blocks improperly.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Split-Plot Designs](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Create Split-Plot Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/split-plot/create-split-plot-design/) — accessed 2026-09-03

### stepwise_regression — 逐步回归

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 逐步回归
- **对话框角色**: `response`（响应）
- **algorithm_help purpose（对齐）**: 按 α 或 Forward AICc/BIC 对线性回归候选预测做逐步选择。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格、量具通过或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
从多个候选预测变量中自动筛选进入线性模型的子集（探索性变量选择）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 数据驱动选择膨胀 I 类错误；不能替代先验假设；预测性能需验证集。

**典型样本量**: n≥10–15×候选变量数；n≥50 起较稳。

**制造场景（列名示例）**
从 12 个过程参数中筛选影响圆度的关键因子。

**常见误用**
- 逐步法结果当确认性
- 多重共线性下不稳定
- 不做交叉验证

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab Stepwise](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/regression/stepwise-regression/) — accessed 2026-09-03
- [Harrell Regression Modeling](https://hbiostat.org/doc/rms.pdf) — accessed 2026-09-03

### t_power — t 功效与样本量

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → t 功效与样本量
- **对话框角色**: _无需列角色（计算器/设计生成类）_
- **algorithm_help purpose（对齐）**: 在给定效应、α 和功效目标下，估计样本量或可检测效应。mode 覆盖 t/ANOVA/比例/方差/泊松，以及等价 TOST、2 水平 DOE、正态容差样本量。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
规划 t 检验样本量或评估功效（α、β、效应量）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 效应量需先验或试点；不能弥补错误设计。

**典型样本量**: 功效计算本身；常目标功效 0.8–0.9。

**制造场景（列名示例）**
比较两线膜厚差 0.5μm 需各组多少 n（功效 0.9）。

**常见误用**
- 效应量设过小样本过大浪费
- 单侧双侧混淆
- 事后功效无意义

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Power](https://www.itl.nist.gov/div898/handbook/prc/section2/prc22.htm) — accessed 2026-09-03
- [Minitab Power for t](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/power-and-sample-size/2-sample-t/) — accessed 2026-09-03

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

**典型样本量**: Inherits from orthogonal array; replicate noise conditions in outer array.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Wrong S/N ratio type
- pooling interactions into error
- not confirming with confirmation runs.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Robust Design Analysis](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [Minitab: Analyze Taguchi Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/doe/how-to/taguchi/analyze-taguchi-design/) — accessed 2026-09-03

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

**典型样本量**: Standard arrays: L8 (7 factors @2 levels), L9 (4@3), L18 (8@2 + 1@2), L27; depends on array.

**制造场景（列名示例）**
DOE：`温度_℃`、`压力_MPa`、`响应_良率`

**常见误用**
- Ignoring interactions assigned to dummy columns
- not including outer array for noise factors
- mis-assigning factors to columns.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Montgomery: DOE — Robust Parameter Design](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119716837) — accessed 2026-09-03
- [NIST e-Handbook: Taguchi Methods](https://www.itl.nist.gov/div898/handbook/pri/section5/pri5.htm) — accessed 2026-09-03

### time_series_decomposition — 时间序列分解

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 时间序列分解
- **对话框角色**: `time`（时间列（可选））
- **algorithm_help purpose（对齐）**: 对单列时间顺序数据做时间序列分解。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
分解趋势、季节、残差成分，理解周期与异常（探索）。

**不能当什么用**
不自动预测；分解假设可加性/乘性需判断；短序列季节不稳。

**典型样本量**: ≥24（月）；≥2 季节周期长度×2。

**制造场景（列名示例）**
月度能耗分解看季节与趋势。

**常见误用**
- 不足一季就分季节
- 结构突变未分段
- 残差不查自相关

**建议 dataset_id**: `ts_decomp_seasonal`

**权威来源**
- [NIST Decomposition](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc44.htm) — accessed 2026-09-03
- [Minitab Decomposition](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/time-series/decomposition/) — accessed 2026-09-03

### time_series_smoothing — 指数平滑

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 时间序列平滑
- **对话框角色**: `variables`（时间序列）
- **algorithm_help purpose（对齐）**: 对单列时间顺序数据做指数平滑。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
单序列平滑去噪，短期预测（简单指数平滑等）。

**不能当什么用**
长期结构变化大时失效；不替代控制图判稳；需检查残差自相关。

**典型样本量**: ≥20 点；季节模型需≥2 完整季节。

**制造场景（列名示例）**
周产量单指数平滑做下月粗预测。

**常见误用**
- 非平稳直接平滑
- α 不调
- 把平滑当永久趋势

**图形解读要点**: 看分布/关系/趋势模式与离群；图形探索不替代假设检验。

**建议 dataset_id**: `ts_smooth_weekly`

**权威来源**
- [NIST Smoothing](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc43.htm) — accessed 2026-09-03
- [Minitab Single Exp Smoothing](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/time-series/single-exponential-smoothing/) — accessed 2026-09-03

### trend_analysis — 趋势分析

- **implemented_status**: `formula_reference`
- **菜单路径（代码）**: **统计** → 趋势分析
- **对话框角色**: `time`（时间列（可选））
- **algorithm_help purpose（对齐）**: 对时间顺序数值序列估计线性趋势，可选 Mann-Kendall 单调趋势检验。
- **interpretation_limits**: 解释层只陈述统计证据，不写过程合格或规格已满足。
- **实现说明**: formula_reference / 编排：当前版本菜单可能没有此项；公式见帮助对话框，数据仅供对照学习。

**常用来做什么**
检验/估计时间序列单调趋势（Mann-Kendall 类或线性趋势）。

**不能当什么用**
当前版本菜单中可能没有此项或仅公式参考；导入演示数据仅供学习对照。 自相关存在时 p 值失真；不能证明因果。

**典型样本量**: ≥20；有自相关需调整方法。

**制造场景（列名示例）**
季度 ppm 趋势检验，判断是否在改善。

**常见误用**
- 忽略自相关
- 把趋势当永远持续
- 混用不同时间粒度

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST Trend](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc44.htm) — accessed 2026-09-03
- [Minitab Trend Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/time-series/trend-analysis/) — accessed 2026-09-03

### two_factor_anova — 双因素 ANOVA

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 双因素 ANOVA
- **对话框角色**: `response`（响应变量）
- **algorithm_help purpose（对齐）**: 把响应变异分解为因子 A、因子 B 和交互作用。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
同时评估两个因子主效应及交互效应对连续响应的影响（如温度×压力、操作员×量具、材料×机台）。

**不能当什么用**
不能用于未重复实验估计交互（无重复时需其他结构）；不能替代规范的析因设计；随机/固定效应混淆时结论受限；不能外推超出设计范围。

**典型样本量**: 完全析因每单元至少 n=2–3 次重复；2^k 设计每组合至少 1–2 重复以估交互与误差；总运行数随水平数快速增长。

**制造场景（列名示例）**
焊接工艺：列「抗拉强度_MPa」「温度水平」「压力水平」「试片编号」，评估温度、压力及其交互对强度的影响。

**常见误用**
- 无重复却声称检验交互
- 把观测研究当析因
- 忽略交互显著仍只看主效应
- 未做残差检查。

**建议 dataset_id**: `anova_two_factor`

**权威来源**
- [NIST/SEMATECH e-Handbook — Two-Way ANOVA](https://www.itl.nist.gov/div898/handbook/prc/section2/prc33.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 5](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [AIAG — SPC Manual, Multi-Vari Analysis Context](https://www.aiag.org/store/quality/) — accessed 2026-09-03

### two_poisson_rate — 双样本泊松率

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 双样本泊松率
- **对话框角色**: `first_events`（第一组缺陷数）
- **algorithm_help purpose（对齐）**: 比较两组泊松率（差值或率比）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
比较两个独立泊松总体的发生率是否不同（如两产线单位时间缺陷数、两区域单位面积瑕疵数）。

**不能当什么用**
不能用于二项「合格/不合格」比例；暴露量（时间、面积、件数）不等时必须纳入；过度离散需其他模型。

**典型样本量**: 两组合计计数各≥10–20 较常见；检出小差异需更长观测窗口或更大面积。

**制造场景（列名示例）**
两装配线停机：列「停机次数」「观测小时」「产线」「周次」，比较两条线每小时停机强度。

**常见误用**
- 未按暴露量标准化
- 把独立区域数据当成一个泊松总体
- 忽略缺陷聚集
- 与小样本比例检验混淆。

**建议 dataset_id**: `pois_two_count`

**权威来源**
- [NIST/SEMATECH e-Handbook — Poisson Rate Comparison](https://www.itl.nist.gov/div898/handbook/prc/section4/prc47.htm) — accessed 2026-09-03
- [Montgomery — Statistical Quality Control, Ch. 7](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+8th+Edition-p-9781119726764) — accessed 2026-09-03
- [AIAG — SPC Manual, u-Chart Comparison](https://www.aiag.org/store/quality/) — accessed 2026-09-03

### two_proportion_equivalence — 两比例等价性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 两比例等价性检验
- **对话框角色**: `first_events`（第一组事件数）
- **algorithm_help purpose（对齐）**: 检验两组比例差是否落入等价界限（Wald z-TOST）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
两组比例差在 ±δ 内等价。

**不能当什么用**
稀疏表需 Fisher/精确；不能证明优效。

**典型样本量**: 每组事件≥10；总 n 常≥200。

**制造场景（列名示例）**
两班次不良率差在 ±0.5% 等价。

**常见误用**
- δ 随意
- 忽略聚类/批次
- 与 RR/OR 等价混淆

**建议 dataset_id**: `equiv_prop_two`

**权威来源**
- [Minitab 2 Proportion Equivalence](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/equivalence-test/) — accessed 2026-09-03
- [NIST Two Proportions](https://www.itl.nist.gov/div898/handbook/prc/section2/prc22.htm) — accessed 2026-09-03

### two_proportions — 两比例检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 两比例检验
- **对话框角色**: `first_events`（第一组事件数）
- **algorithm_help purpose（对齐）**: 比较两组事件比例之差。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
比较两个独立总体的比例是否不同（如两供应商、两班次、两种夹具的不良率差异）。

**不能当什么用**
不能用于配对二元数据（用 McNemar）；小样本或期望频数过小需精确方法；不能替代规范的抽样方案设计。

**典型样本量**: 每组 n≥30 且期望不良数不太小；比较 1% 级别不良率需更大样本；功效分析按可检出比例差设计。

**制造场景（列名示例）**
两供应商来料：列「供应商」「抽检数」「不良数」「物料编码」，比较供应商甲与乙的不良率。

**常见误用**
- 期望频数不足仍用卡方近似
- 未区分独立与配对设计
- 忽略不同组抽检量差异的解读
- 只比较比例不看置信区间。

**建议 dataset_id**: `prop_two_line`

**权威来源**
- [NIST/SEMATECH e-Handbook — Two Proportions Test](https://www.itl.nist.gov/div898/handbook/prc/section4/prc46.htm) — accessed 2026-09-03
- [AIAG — SPC Manual, Attribute Data Analysis](https://www.aiag.org/store/quality/) — accessed 2026-09-03
- [ASQ — Proportion Tests](https://asq.org/quality-resources/hypothesis-testing) — accessed 2026-09-03

### two_sample_equivalence — 双样本等价性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 双样本等价性检验
- **对话框角色**: `variables`（两列独立样本）
- **algorithm_help purpose（对齐）**: 检验两组均值差是否落入等价界限。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
两独立组均值差在 ±δ 内等价的 TOST 检验。

**不能当什么用**
不能证明优于对照；方差不齐需方法调整；等价限需预先定义。

**典型样本量**: 常每组 20–50；功效分析必备。

**制造场景（列名示例）**
新夹具与旧夹具均值差在 ±0.01mm 内等价。

**常见误用**
- δ 事后挑选
- 与 superiority 检验混淆
- 忽略多重比较

**建议 dataset_id**: `equiv_two_near_equal`

**权威来源**
- [FDA Statistical Guidance](https://www.fda.gov/regulatory-information/search-fda-guidance-documents/statistical-guidance-reporting-results-studies-evaluating-diagnostic-tests) — accessed 2026-09-03
- [Minitab 2-Sample Equivalence](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/equivalence-test/) — accessed 2026-09-03

### two_sample_equivalence_ratio — 双样本均值比等价性检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 双样本均值比等价性检验
- **对话框角色**: `variables`（检验列 + 参考列）
- **algorithm_help purpose（对齐）**: 检验两组均值比值 ρ=μ_test/μ_ref 是否落入比值等价界限（TOST）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
两独立组均值比在 [θL, θU] 内等价（生物等效/比例型规格）。

**不能当什么用**
对数正态假设需检查；不能用于绝对差规格场景。

**典型样本量**: 每组 24–36 常见（比例限紧时更大）。

**制造场景（列名示例）**
两种焊膏体积比在新/旧配方 0.9–1.1 等价。

**常见误用**
- 算术均值当比例
- 比值限无监管依据
- 离群未调查

**建议 dataset_id**: `equiv_ratio_near_one`

**权威来源**
- [ICH E9](https://www.ich.org/page/efficacy-guidelines) — accessed 2026-09-03
- [Minitab Equivalence Ratio](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/equivalence-test/) — accessed 2026-09-03

### two_sample_t — 双样本 t 检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 双样本 t 检验
- **对话框角色**: `variables`（两列独立样本）
- **algorithm_help purpose（对齐）**: 比较两组均值差是否为 0。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
比较两个独立组的连续测量均值是否不同（如两条产线、两种材料、改机前后独立抽样）。

**不能当什么用**
不能用于配对数据；假定两组独立且（合并或分别）方差结构需事先选定；不能替代随机化实验的因果推断；不能比较两个以上组。

**典型样本量**: 每组 n=10–30 常见；功效分析按期望差值与 σ 确定；不等样本量可行但降低功效。

**制造场景（列名示例）**
两产线膜厚对比：列「膜厚_um」「产线」「抽检日期」，比较产线 A 与产线 B 的均值差异。

**常见误用**
- 配对数据误用独立双样本 t
- 未做等方差检验或选对合并/非合并方差版本
- 忽略组间独立性与随机化
- 只报告 p 值不报告差值置信区间。

**建议 dataset_id**: `infer_two_sample_location`

**建议埋点（教学升级）**
- 两列独立样本，均值差约 0.8–1.5σ；默认对话框 `variance=welch`
- 不要埋配对结构（那是 `paired_t`）

**权威来源**
- [NIST/SEMATECH e-Handbook — Two processes same mean](https://www.itl.nist.gov/div898/handbook/prc/section3/prc31.htm) — accessed 2026-09-03（**勘误**：`prc22.htm` 是单样本均值，不是双样本）
- [NIST — One-sample mean (prc22)](https://www.itl.nist.gov/div898/handbook/prc/section2/prc22.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 2](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03

### variance_test — 等方差检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → 方差检验
- **对话框角色**: `first`（第一样本 / 测量列）
- **algorithm_help purpose（对齐）**: 检验两组或多组方差/标准差是否相等。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
比较两个独立正态总体的方差是否相等，为选择合并方差 t 检验、ANOVA 或能力比较提供依据。

**不能当什么用**
对非正态数据结论不可靠；不能单独作为过程稳定性判定；不能比较两个以上方差（用 Bartlett/Levene）；对方差极其敏感。

**典型样本量**: 每组 n≥10 较常见；方差检验功效通常低于均值检验；AIAG 建议先确认测量系统足够。

**制造场景（列名示例）**
两产线膜厚散布：列「膜厚_um」「产线」「抽检批次」，比较两线膜厚方差是否一致，再选合适的均值检验。

**常见误用**
- 未检查正态性就做 F 检验
- 显著不等方差仍用合并方差 t
- 把标准差比当成能力比
- 重复检验不校正。

**建议 dataset_id**: `var_two_line_unequal`

**权威来源**
- [NIST/SEMATECH e-Handbook — F-Test for Equality of Variances](https://www.itl.nist.gov/div898/handbook/prc/section2/prc21.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 3](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [ASQ — Variance Tests](https://asq.org/quality-resources/hypothesis-testing) — accessed 2026-09-03

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

**典型样本量**: Often 5–20 failures plus prior; combines with censored data.

**制造场景（列名示例）**
可靠性：`循环次数`、`失效状态`、`应力_V`

**常见误用**
- Prior dominates with little data
- wrong prior specification
- not performing prior sensitivity.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Bayesian Reliability](https://www.itl.nist.gov/div898/handbook/apr/section7/apr73.htm) — accessed 2026-09-03
- [Minitab: Parametric Distribution Analysis (Bayesian)](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/) — accessed 2026-09-03

### wilcoxon_signed_rank — Wilcoxon 符号秩检验

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **统计** → Wilcoxon 符号秩检验
- **对话框角色**: `variables`（一列或两列配对）
- **algorithm_help purpose（对齐）**: 对配对差或单样本中位数做符号秩检验。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
配对连续数据的非参数检验，评估差值分布是否对称于零（比符号检验功效高，但仍不要求正态）。

**不能当什么用**
不能用于独立两组；差值分布严重偏斜时需谨慎；不能检测特定方向的系统性偏差以外的效应；零差值需剔除。

**典型样本量**: 配对 n≥10–15 较常见；n<6 时考虑精确符号检验。

**制造场景（列名示例）**
治具调整前后：列「调整前_um」「调整后_um」「夹具编号」，非参数评估调整是否改变测量偏移。

**常见误用**
- 独立样本误用
- 未剔除零差
- 与配对 t 混用条件
- 忽略差值对称性假设。

**建议 dataset_id**: `infer_paired_shift`

**权威来源**
- [NIST/SEMATECH e-Handbook — Wilcoxon Signed Rank Test](https://www.itl.nist.gov/div898/handbook/prc/section3/prc36.htm) — accessed 2026-09-03
- [Montgomery — Design and Analysis of Experiments, Ch. 3](https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648) — accessed 2026-09-03
- [ASQ — Wilcoxon Test](https://asq.org/quality-resources/nonparametric-tests) — accessed 2026-09-03

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

**典型样本量**: ANSI/ASQ Z1.4 tables: n depends on lot size and inspection level (e.g., n=32–125 for common lots).

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- Switching rules ignored
- non-random sampling
- applying wrong plan for lot size
- confusing AQL with process capability.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Acceptance Sampling](https://www.itl.nist.gov/div898/handbook/pmc/section2/pmc26.htm) — accessed 2026-09-03
- [Montgomery: Introduction to Statistical Quality Control — Acceptance Sampling](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+8th+Edition-p-9781119726836) — accessed 2026-09-03

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

**典型样本量**: Minitab minimum: 50 samples × 3 appraisers × 2 replicates; AIAG attribute study often 30–50 parts with known standards.

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- Samples not spanning good/bad borderline
- appraisers see previous ratings
- too few borderline parts
- ignoring low kappa despite high % agreement.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [AIAG MSA Manual (4th ed.) — Attribute MSA](https://www.aiag.org/quality/msa) — accessed 2026-09-03
- [Minitab: Attribute Agreement Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/attribute-agreement-analysis/) — accessed 2026-09-03

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

**典型样本量**: ≥20–30 batches with 3–10 measurements per batch; total n often 100–300 depending on batch-to-batch variation.

**制造场景（列名示例）**
光学膜：`膜厚_um`、`产线`（A/B 线）

**常见误用**
- Treating batch means as replicates
- ignoring batch size imbalance
- not verifying batch stability
- using normal capability on batch means only.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab: Between/Within Capability (batch context)](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-sixpack/between-within-capability-sixpack/) — accessed 2026-09-03
- [Montgomery: Introduction to Statistical Quality Control](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+8th+Edition-p-9781119726836) — accessed 2026-09-03

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

**典型样本量**: ≥25 subgroups with 3–5 units each (≥75–125 total); subgroups must represent intentional process structure (batch, roll, cavity).

**制造场景（列名示例）**
注塑：`模腔尺寸_mm`、`模腔`（1/2/3 穴）

**常见误用**
- Wrong subgroup definition
- pooling between-batch variation into within sigma
- interpreting high Cpk when Ppk is poor
- insufficient subgroups across batches.

**建议 dataset_id**: `cap_between_within`

**权威来源**
- [Minitab: Between/Within Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/between-within-capability-analysis/) — accessed 2026-09-03
- [NIST e-Handbook: Capability Indices](https://www.itl.nist.gov/div898/handbook/pmc/section1/pmc16.htm) — accessed 2026-09-03

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

**典型样本量**: Enough trials to observe expected defects: rule of thumb n≥5/p for estimating p; often 200–1000+ inspections for low p.

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- Pooling heterogeneous lots
- ignoring inspection ambiguity
- using normal approximation for rare events
- not checking stability of defect rate over time.

**建议 dataset_id**: `cap_binomial_lots`

**权威来源**
- [NIST e-Handbook: Attribute Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc35.htm) — accessed 2026-09-03
- [Montgomery: Introduction to Statistical Quality Control — Attribute Data](https://www.wiley.com/en-us/Introduction+to+Statistical+Quality+Control%2C+8th+Edition-p-9781119726836) — accessed 2026-09-03

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

**典型样本量**: ≥30; preferably ≥50 for stable λ estimation.

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- Applying λ from one dataset to another without re-checking
- back-transforming capability incorrectly
- ignoring that λ is estimated (extra uncertainty).

**建议 dataset_id**: `dist_skew_boxcox`

**权威来源**
- [NIST e-Handbook: Box-Cox Transformation](https://www.itl.nist.gov/div898/handbook/pmc/section6/pmc63.htm) — accessed 2026-09-03
- [Minitab: Box-Cox Transformation](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/anova/how-to/box-cox-transformation/) — accessed 2026-09-03

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

**典型样本量**: Minimum 30 individual values; NIST recommends ~50 for valid indices and n≥100 for reliable Cpk confidence intervals; PPAP often expects 100+ measurements over production variation.

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- Computing Cpk on out-of-control data
- using within-subgroup sigma for overall performance
- ignoring normality
- too-small samples
- mixing batches/shifts
- treating specification target as required for Cpk.

**建议 dataset_id**: `cap_stable_spec`

**建议埋点（教学升级）**
- 近似受控、近正态；轻微偏心使 Cpk < Cp；USL/LSL 只在对话框，不进表
- 不要埋片 41/55 类特殊原因

**权威来源**
- [NIST/SEMATECH e-Handbook: Process Capability](https://www.itl.nist.gov/div898/handbook/pmc/section1/pmc16.htm) — accessed 2026-09-03
- [Minitab: Normal Capability Analysis Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/before-you-start/overview/) — accessed 2026-09-03
- [AIAG PPAP Manual (4th ed.) — Process Capability Studies](https://www.aiag.org/quality/ppap) — accessed 2026-09-03

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

**典型样本量**: Same as underlying capability analysis: typically ≥30, preferably ≥50–100 subgrouped or individual values.

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- Reading only Cpk without checking control chart
- ignoring probability plot failures
- wrong Sixpack type (normal vs nonnormal vs between/within).

**图形解读要点**: 看分布/关系/趋势模式与离群；图形探索不替代假设检验。

**建议 dataset_id**: `cap_stable_spec`

**权威来源**
- [Minitab: Capability Sixpack Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-sixpack/) — accessed 2026-09-03
- [NIST e-Handbook: Process Capability](https://www.itl.nist.gov/div898/handbook/pmc/section1/pmc16.htm) — accessed 2026-09-03

### cause_and_effect — 因果图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → 因果图
- **对话框角色**: `category`（类别列）
- **algorithm_help purpose（对齐）**: 按类别汇总原因条目，输出结构摘要与类别计数图（鱼骨/因果头脑风暴）。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
头脑风暴阶段整理人/机/料/法/环/测等潜在原因（结构化思考，非统计检验）。

**不能当什么用**
不能证明哪条原因成立；不替代 DOE 或回归验证；非数据驱动排序。

**典型样本量**: 不适用；依赖团队知识与现场证据。

**制造场景（列名示例）**
焊点虚焊因果图，列出锡膏、炉温、钢网、操作等候选因素。

**常见误用**
- 鱼骨图当根因结论
- 原因条目过细或过粗无法验证
- 缺少数据验证闭环

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `fishbone_solder_causes`

**权威来源**
- [ASQ Fishbone](https://asq.org/quality-resources/fishbone) — accessed 2026-09-03
- [AIAG 质量工具](https://www.aiag.org/quality/) — accessed 2026-09-03

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

**典型样本量**: ≥50; 100+ preferred for distinguishing similar distributions.

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- Choosing by lowest AD/p-value alone
- ignoring physical plausibility
- not holding out validation data
- mixing censored and uncensored incorrectly.

**建议 dataset_id**: `dist_id_candidates`

**权威来源**
- [Minitab: Individual Distribution Identification](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability-survival/how-to/distribution-id-right-censoring/) — accessed 2026-09-03
- [NIST e-Handbook: Probability Distributions](https://www.itl.nist.gov/div898/handbook/eda/section3/eda366.htm) — accessed 2026-09-03

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

**典型样本量**: Same as crossed Gage R&R: 10×3×3 = 90; unbalanced designs need more parts for stable variance components.

**制造场景（列名示例）**
量具 MSA：`零件号`、`操作员`、`测量值_mm`

**常见误用**
- Ignoring significant interaction
- mis-specifying random vs fixed factors
- insufficient parts for interaction df.

**建议 dataset_id**: `msa_crossed_aiag`

**权威来源**
- [AIAG MSA Manual (4th ed.) — ANOVA Method](https://www.aiag.org/quality/msa) — accessed 2026-09-03
- [Minitab: Expanded Gage R&R](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/expanded-gage-r-r-study-crossed/) — accessed 2026-09-03

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

**典型样本量**: 10 parts × 3 operators × 3 trials minimum; more parts if estimating interaction.

**制造场景（列名示例）**
量具 MSA：`零件号`、`操作员`、`测量值_mm`

**常见误用**
- Over-interpreting variance components with small df
- not documenting ANOVA assumptions
- mixing fixed and random effects incorrectly.

**建议 dataset_id**: `msa_expanded_crossed`

**权威来源**
- [AIAG MSA Manual (4th ed.)](https://www.aiag.org/quality/msa) — accessed 2026-09-03
- [Montgomery & Runger: Applied Statistics — Measurement Systems](https://www.wiley.com/en-us/Applied+Statistics+and+Probability+for+Engineers%2C+7th+Edition-p-9781119409530) — accessed 2026-09-03

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

**典型样本量**: AIAG standard: 10 parts × 3 operators × 3 trials = 90 measurements; minimum ~40 total for coarse estimate.

**制造场景（列名示例）**
量具 MSA：`零件号`、`操作员`、`测量值_mm`

**常见误用**
- Parts not covering full tolerance range
- operators know part identity
- non-randomized order
- accepting %GRR<30% without checking NDC≥5
- using wrong method for nested layouts.

**建议 dataset_id**: `msa_crossed_aiag`

**建议埋点（教学升级）**
- 10 parts × 3 operators × 3 trials；零件覆盖过程范围；一名操作员系统偏倚
- 不写「量具通过」；NDC / %GR&R 只作统计描述

**权威来源**
- [Minitab: Crossed Gage R&R Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/crossed-gage-r-r-study/before-you-start/overview/) — accessed 2026-09-03（旧 `gage-r-r-study-crossed` 路径 404）
- [NIST e-Handbook: Gauge R&R variability](https://www.itl.nist.gov/div898/handbook/mpc/section4/mpc44.htm) — accessed 2026-09-03（**勘误**：`pmc/section4/pmc4.htm` 现为时间序列，不是 Gage）
- [Minitab: Is my measurement system acceptable?](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/supporting-topics/gage-r-r-and-wheeler-s-emp-studies/is-my-measurement-system-acceptable/) — accessed 2026-09-03

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

**典型样本量**: AIAG: 50 measurements of one part by one operator on one gage in short time window.

**制造场景（列名示例）**
量具 MSA：`零件号`、`操作员`、`测量值_mm`

**常见误用**
- Using multiple parts
- long elapsed time introducing drift
- not comparing bias to tolerance
- ignoring gage resolution.

**建议 dataset_id**: `msa_type1_ref`

**权威来源**
- [AIAG MSA Manual (4th ed.) — Type 1 Study](https://www.aiag.org/quality/msa) — accessed 2026-09-03
- [Minitab: Type 1 Gage Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/type-1-gage-study/) — accessed 2026-09-03

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

**典型样本量**: Structured grid: e.g., 5 parts × 4 positions × 3 operators = 60 measurements minimum.

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- Wrong nesting/crossing structure
- too few levels per factor
- stopping at chart without confirmation experiment.

**建议 dataset_id**: `multi_vari_pos_time`

**权威来源**
- [NIST e-Handbook: Multi-Vari Studies](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc45.htm) — accessed 2026-09-03
- [Minitab: Multi-Vari Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/multi-vari-chart/) — accessed 2026-09-03

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

**典型样本量**: Minimum 10 parts per operator × 3 operators × 2–3 trials; often 30+ parts total per operator group.

**制造场景（列名示例）**
量具 MSA：`零件号`、`操作员`、`测量值_mm`

**常见误用**
- Using crossed analysis on nested data
- parts not matched for difficulty across operators
- too few parts per operator.

**建议 dataset_id**: `msa_nested_operator`

**权威来源**
- [AIAG MSA Manual (4th ed.) — Nested Designs](https://www.aiag.org/quality/msa) — accessed 2026-09-03
- [Minitab: Gage R&R Study (Nested)](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-r-r-study-nested/) — accessed 2026-09-03

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

**典型样本量**: ≥50 observations; often 100+ for reliable tail-probability and percentile estimates near specification limits.

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- Forcing a poor distribution fit
- using normal Cpk on skewed data
- selecting distribution by best p-value alone without engineering rationale
- mixing special causes.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [Minitab: Nonnormal Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/nonnormal-capability-analysis/) — accessed 2026-09-03
- [NIST e-Handbook: Probability Distributions](https://www.itl.nist.gov/div898/handbook/pmc/section6/pmc6.htm) — accessed 2026-09-03

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

**典型样本量**: ≥100 for stable percentile estimates; more for extreme tails (e.g., 99.99%).

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- Using nonparametric methods with n<50
- misinterpreting empirical percentiles as long-term performance
- ignoring time/order structure.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Nonparametric Methods](https://www.itl.nist.gov/div898/handbook/pmc/section6/pmc62.htm) — accessed 2026-09-03
- [Minitab: Nonparametric Capability](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/) — accessed 2026-09-03

### pareto — 柏拉图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → 柏拉图
- **对话框角色**: `category`（缺陷类别）
- **algorithm_help purpose（对齐）**: 按类别频数从大到小排序，并画累积百分比。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
按频次/成本排序不良或缺陷类型，识别少数关键项（80/20 模式观察）。

**不能当什么用**
不证明因果；不替代统计检验判断组间差异；分类定义变更会改变排序。

**典型样本量**: 累计至少 20–50 个事件；类别不宜过多（≤15）。

**制造场景（列名示例）**
装配线一周不良代码柏拉图，聚焦前 3 项返工原因。

**常见误用**
- 类别划分不一致导致排名跳变
- 把柏拉图当控制图用
- 忽略测量系统对缺陷分类的影响

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `pareto_defect_tail`

**建议埋点（教学升级）**
- 少数缺陷代码占累计大部分 + 长尾；可选计数列
- 不证明因果；不与控制图共用失控时间序

**权威来源**
- [Minitab: Pareto Chart Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/pareto-chart/before-you-start/overview/) — accessed 2026-09-03
- [NIST Pareto `pri/section3/pareto.htm`](https://www.itl.nist.gov/div898/handbook/pri/section3/pareto.htm) — accessed 2026-09-03 **失效**（重定向 ITL 首页）

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

**典型样本量**: ≥25–50 units; more when mean defects per unit is low (<1).

**制造场景（列名示例）**
装配：`班次`、`不良数`、`检验数`

**常见误用**
- Varying inspection area without normalization
- overdispersion ignored
- using Poisson when defects cluster
- unstable inspection standards.

**建议 dataset_id**: `cap_poisson_counts`

**权威来源**
- [NIST e-Handbook: Poisson Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc36.htm) — accessed 2026-09-03
- [Minitab: Capability for Poisson Data](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/) — accessed 2026-09-03

### run_chart — 运行图

- **implemented_status**: `implemented`
- **菜单路径（代码）**: **质量工具** → 运行图
- **对话框角色**: `variables`（数值观测（一列））
- **algorithm_help purpose（对齐）**: 按行序画个体运行图（中位数中心线），并给出聚类/混合/趋势/振荡四模式近似 P。
- **interpretation_limits**: 解释层只陈述统计证据、假设状态和不可计算原因，不写过程合格、量具通过、分布已证明、规格已满足或必须删点。

**常用来做什么**
按时间/序号看过程中心是否漂移、跳变或循环（模式观察，控制图前奏）。

**不能当什么用**
无控制限，不能判特殊原因或稳态；不替代 I-MR/Xbar 控制图。

**典型样本量**: ≥20 点可看趋势；50+ 更易见模式。

**制造场景（列名示例）**
每日线体 OEE 或尺寸均值运行图，查换型后是否台阶变化。

**常见误用**
- 无控制限却宣称过程失控
- 忽略抽样间隔变化
- 混用不同子组定义

**图形解读要点**: 看分布形状、离群、关联模式；不替代假设检验。

**建议 dataset_id**: `run_chart_median_trend`

**权威来源**
- [NIST Run Chart](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm) — accessed 2026-09-03
- [Minitab Run Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/run-chart/) — accessed 2026-09-03

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

**典型样本量**: Depends on coverage/confidence: 95%/95% often needs n≥30–90; 99%/99% needs n≥100–500+.

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- Using normal tolerance factors on non-normal data
- wrong sided vs two-sided
- interpreting as spec compliance without stability.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Tolerance Intervals](https://www.itl.nist.gov/div898/handbook/prc/section2/prc25.htm) — accessed 2026-09-03
- [Minitab: Tolerance Intervals (Normal)](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/tolerance-intervals-normal-data/) — accessed 2026-09-03

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

**典型样本量**: Same as multi-vari: enough points per factor level to see pattern (≥3–5 per cell).

**制造场景（列名示例）**
SMT 印刷工站：`锡膏高度_um`、`产线`、`检测时间`

**常见误用**
- Over-interpreting noise as signal
- too few levels
- not labeling hierarchy correctly.

**建议 dataset_id**: （空 — 本课无专用导入表；旧共享表建议作废）

**权威来源**
- [NIST e-Handbook: Multi-Vari Chart](https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc45.htm) — accessed 2026-09-03
- [Minitab: Multi-Vari Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/multi-vari-chart/) — accessed 2026-09-03
