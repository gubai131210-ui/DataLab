# DataLab 算法扩展会话简报

> 给**下一对话**用。算法工作从本文件开始，不要从 `docs/session-handoff.md` 第 5 节续重构。
>
> 新对话第一条消息建议直接粘贴第 7 节短提示词。

---

## 1. 任务

面向汽车质量工程师、对标 Minitab 的桌面工具。本轮：**深化已有缺口 + 增加有杠杆的新算法 + 图表属性页 UX**。完成判据：范围内每项都有公式文档、领域结果、服务页、命令接线（新算法）、测试和验收条目；解释只读 Facts；导入 A→B 契约未破坏。

## 2. 步骤（按序，完成一项再下一项）

1. **读真相源**（完成：能复述本轮四项、禁区和接线，且不把 handoff 第 5 节当任务）。
   - 已完成项与延后清单：`docs/quality-algorithms-acceptance.md`
   - 方法状态与导入契约：`docs/research/algorithm-chart-gap-matrix.md`
   - 延后项细节：`docs/research/deferred-capability-agreement.md`
   - 计算口径：`docs/statistical-methodology.md`
   - 词汇：`CONTEXT.md`
   - 架构：`docs/adr/0001-core-architecture.md`、`0003-structured-interpretation-facts.md`、`0004-grouped-analysis-configuration.md`
   - UI：`docs/ui-guidelines.md`
   - 分层与踩坑（只读第 4、6 节）：`docs/session-handoff.md`
2. **研究公式**（完成：`docs/research/` 有本轮主题文档，含 Minitab/NIST 链接与访问日期；`formula_reference ≠ golden`）。加载 **research** skill。对照 Minitab **输出表形**（表名、列、诊断），不填写未导出数值。
3. **计划后竖切实现**（完成：每个切片 domain → Facts → AnalysisService → `analysis_commands` → 解释 → 序列化/测试 → CMake）。加载 **tdd** 与 **cpp-coding** skill。
4. **收尾**（完成：更新本文件「本轮范围」状态、`quality-algorithms-acceptance.md`、`algorithm-chart-gap-matrix.md`；列出用户在 Qt Creator 中的手工验收项）。

Skill 路径：`C:\Users\孤白赟悫\.codex\skills\research\SKILL.md`、`tdd\SKILL.md`；`C:\Users\孤白赟悫\.agents\skills\cpp-coding\SKILL.md`。

## 3. 接线框架

统计核心在 C++ `src/domain/statistics/`（零 Qt）。Excel 导入已由 `ExcelTableImporter`（infrastructure，Qt+zlib）原生实现；应用运行时不需要 Python。

```
ui → application / infrastructure / reporting → domain
```

新增或深化一个分析：

| 层 | 文件 | 职责 |
|---|---|---|
| 领域 | `src/domain/statistics/*` | 计算 + 诊断码 |
| 配置/事实 | `src/domain/quality_types.h` | 嵌套配置 + `*Facts` |
| 编排 | `src/application/analysis_service.cpp` | 装配 `OutputPage`（表/图/diagnostics/facts） |
| 菜单 | `src/ui/analysis_commands.cpp` | id / 角色 / 输入 / apply / run 单一来源 |
| 解释 | `src/application/interpretation_service.cpp` | 只读 Facts |
| 持久化 | `src/infrastructure/output_serialization.cpp` | JSON round-trip |
| 构建 | `CMakeLists.txt` + `tools/check_layering.ps1` | 新源文件进对应 target |

数据衔接（本轮特别注意，用户强调导入不要断）：

- `column_assembly` complete-case **行主序**（`aligned[i][j]` = 第 i 个观测第 j 列）。
- `parse_numeric_cell`；保留 `RowId` / `source_row`；图上每个点可追溯原始行。
- 导入契约见缺口矩阵第 3 节。`tests/import_state_reset_test.cpp` 覆盖重导入文件 B 后排除行、输出页、undo、行选择失效。
- 多列角色（响应 + 因子）必须与工作表列类型、缺失单元格、排除行一致；不要另写一套解析。

测试目标以 `CMakeLists.txt` 的 `add_datalab_test` / `add_test` 为准（约 26 个，不是 handoff 里的 12）。

## 4. 已完成（不要重做）

### 2026-08-18 上午（上一算法轮）

1. Logistic 拟合优度表 + `LogisticFacts`
2. 个体分布识别 `distribution_identification`
3. 组间/组内能力 `between_within_capability`
4. 图表属性页预览右侧化

### 2026-08-18 下午（本会话 A + B）

1. **时间序列分解 / 指数平滑输出契约** ✅
   - 分解：预测准确度 / 拟合与预测明细 / 季节指数；`ForecastFacts`；`classical_decomposition_cma_trend`
   - 平滑：单/双指数拟合与预测明细；`holt_linear_des` / `single_exponential_ses`
   - 解释走 `ForecastFacts`；公式：`docs/research/time-series-decomposition-smoothing-formulas.md`
2. **DOE 多响应优化 + DoeFacts** ✅
   - `response_columns` + `optimization_objectives`；单响应表形保持兼容
   - 多响应：「响应目标」、各响应预测/D、总体 D（几何平均）
   - 解释只读 `DoeFacts`（response_count、best_overall_desirability、区间可用性）

### 2026-08-18 晚（本轮四项竖切）✅

1. **线性回归 Unusual Observations**
   - 领域 `unusual_r` / `unusual_x` 与现有 outlier/leverage/influential 并存
   - 服务在「拟合与诊断」后插入「异常观测」表（仅 R/X/I 打标行；无打标则缺席）
   - 解释只读 `RegressionFacts`，提示调查、不删点
2. **Multi-Vari Chart**
   - 命令 `multi_vari`；测量 + 2～3 因子；complete-case；缺失/`*` 诊断
   - `MultiVariFacts`；覆盖率 <60% 只诊断不画图
3. **DOE 每响应独立 goal/权重**
   - `InputKind::response_objectives` + `intent.inputs["objectives"]` JSON
   - 单响应无 JSON 时旧共享字段仍写入 `optimization_objectives[0]`
4. **图表属性页 UX**
   - 右键进入非覆盖编辑：图表左侧、`GraphPropertiesPanel` 右侧
   - `element_selected` → `set_selected_path`；系列列表用 `series.label`（空则「系列 n」）
   - 对话框预览横向右侧；复制仍走 `ChartRenderer::render_to_pixmap`

公式：`docs/research/unusual-obs-multivari-doe-chart-formulas.md`

已有菜单入口在 `src/ui/analysis_commands.cpp`。不要重复注册已有 `chart_type`。缺口矩阵里「DOE 响应优化 = 领域层已有但未接入」已过时。

### 2026-08-18 夜（回归带 / DOE 析因图 / 容差区间 / 系列样式）✅

1. **回归 Fitted Line 带 + 残差 y=0**
   - 单预测变量在残差 4 图前插入「拟合线图」（`actual` / `fitted` / CI、PI `confidence_band`）
   - 残差散点含 y=0；多预测变量不画 2D Fitted Line
   - 不改 Unusual R/X/I 与 `RegressionFacts` 计数
2. **DOE 析因图**
   - 标准化效应 Pareto（df>0 用 `|t|`，饱和走 Lenth PSE）；2/3 因子立方图；主效应系列
   - 不改响应优化与 `DoeFacts` 的 D/多响应字段
3. **正态容差区间**
   - 命令 `tolerance_intervals`；Howe 双侧 / Natrella 单侧；`ToleranceFacts`
   - 解释只陈述区间与 `not_verified`，不写合格或规格已覆盖
4. **侧栏系列色/线宽**
   - 选中「数据系列 / …」写回 `ChartModel.series[i].style`；未选中仍写全局 `model.line_width`

公式：`docs/research/regression-doe-tolerance-chart-formulas.md`

### 2026-08-19（二项/泊松能力 / DOE 等值线曲面 / ANOVA 图 / 侧栏线型点型）✅

1. **二项 / 泊松过程能力**
   - 命令 `binomial_capability` / `poisson_capability`；Clopper–Pearson / Garwood CI
   - 过程数据 + 能力表 + 累计图 + P/U 图；`CapabilityFacts.method`；Sixpack 仍只正态
2. **DOE 等值线 / 曲面图**
   - 编码网格求值（其余因子 hold 0）；`PlotKind::contour` + 静态等轴测 `PlotKind::surface`
   - 不改响应优化 ±1 门与几何平均 D
3. **单因素 ANOVA 区间图 + 残差图**
   - 组均值个体 CI 用 pooled MSE；残差-拟合/顺序/概率图（输入行序）；Tukey 语义不变
4. **侧栏系列线型 / 点样式**
   - 选中系列写回 `style.line_style` / `point_style`；`show_points` 随点型

公式：`docs/research/binomial-poisson-doe-anova-chart-formulas.md`

### 2026-08-19 下午（单比例 / TOST / DOE 残差 4 图 / I-MR-R/S）✅

1. **单比例检验**
   - 命令 `one_proportion`；事件+试验 complete-case 多行求和（相对 two_proportions 一行汇总）
   - exact = Clopper–Pearson（与二项能力同一函数）；normal = Wald CI + score z
   - `ProportionFacts`；不实现 Blaker/Wilson/Agresti–Coull
2. **单/双样本 TOST**
   - 命令 `one_sample_equivalence` / `two_sample_equivalence`；复用 t 检验均值/SE/df
   - 100(1−2α)% CI；`within_limits ⇔ p1≤α 且 p2≤α`；不做配对/比值/对数
   - `EquivalenceFacts`；解释只陈述是否落入界限
3. **DOE 析因残差 4 图**
   - 响应页追加 vs 拟合（y=0）/ 观测顺序 / 正态概率 / 直方图
   - `DoeFacts.residual_count`；不改 Pareto/立方/等值线/曲面/优化器 D
4. **I-MR-R/S**
   - 命令 `imr_rs`；子组均值 I-MR + R（n≤8）或 S（n≥9）
   - σ 与 `calculate_between_within` 同口径（n=5 用 R̄/d2）；无子组只诊断

公式：`docs/research/proportion-tost-doe-residual-imr-rs-formulas.md`

### 2026-08-19 夜（泊松率 / 双因素图 / Winters / Gage 图）✅

1. **1-Sample / 2-Sample Poisson Rate**
   - 命令 `one_poisson_rate` / `two_poisson_rate`；1-sample complete-case 多行求和，2-sample 每组一行
   - exact = Garwood CI + 泊松尾；2-sample exact = 条件二项；normal = score/Wald
   - `PoissonRateFacts`；不做率比主输出、Blaker/Wilson、功效
2. **双因素 ANOVA 残差 4 图 + 交互均值图**
   - vs 拟合（y=0）/ 观测顺序 / 正态概率 / 直方图；交互图按因子 B 连线
   - 不改 Tukey（无）与不可估计项 F/P=`*`
3. **Winters / Holt–Winters 输出合同**
   - 季节指数表；`holt_winters_additive` / `holt_winters_multiplicative`；`parameter_source=specified`
   - 加法与乘法都填 `ForecastFacts.mape`；不把 SARIMA/Kalman 当 TSERIES golden
4. **交叉 Gage 分量条 + 按操作者 Xbar-R**
   - %Contribution Pareto（Repeatability / Reproducibility / Part-To-Part，不含 Total Gage R&R）
   - 按操作者等量子组 Xbar 与 R；不改 ndc / 负方差截断 / 交互不自动 pooled

公式：`docs/research/poisson-rate-anova-winters-gage-formulas.md`

### 2026-08-19 晚（Nested Gage 图 / ARIMA 明细 / 属性一致图 / ANOVA 直方图）✅

1. **Nested Gage R&R 图**
   - %Contribution 条（Repeatability / Reproducibility / Part-To-Part）+ 按操作者 Xbar-R
   - complete-case 跳过缺失零件/操作者；不改 nested ndc / 截断
2. **ARIMA 拟合-预测明细**
   - 「拟合与预测明细」对齐 SES（原始行 + 预测行）；保留候选表；`arima_candidate_css`
   - 不把 TSERIES/Kalman 当 golden
3. **属性一致性图**
   - 评估者×零件一致率热图 + 评估者一致率条；不做 Weighted Kappa
4. **单因素 ANOVA 残差直方图**
   - 与双因素/DOE 4 图对齐；不改 Tukey / 组均值 CI

公式：`docs/research/nested-gage-arima-kappa-anova-formulas.md`

### 2026-08-19（Type 1 图 / Bias 图合同 / 两比例 / Box-Cox）✅

1. **Type 1 Gage 图**
   - 直方图（Ref + 规格）+ Run Chart 保留 `source_row`；不改 Cg/Cgk / 零重复性
2. **Bias/Linearity 图合同**
   - complete-case 散点 + 拟合线 + 均值 CI 带；原始 `source_row`；不改斜率公式
3. **两比例检验输出合同**
   - 每组独立 complete-case 多行求和；`ProportionFacts.kind=two_sample`；差值 Wald 区间图；不做 Blaker
4. **Box-Cox 输出合同**
   - λ–SD 诊断图 + 变换前/后概率图；`BoxCoxFacts`；解释不写已正态/合格

公式：`docs/research/type1-bias-twoprop-boxcox-formulas.md`

### 2026-08-19（t 区间图 / 描述图 / 正态输出 / 相关矩阵散点）✅

1. **单/双/配对 t 区间图**
   - 单样本中心为 ȳ、须为 μ0+差值 CI；双样本组均值个体 CI（Welch vs pooled 仅展示）；配对散点 `source_row` + 差值区间
   - `TTestFacts`；不改 Welch/pooled/配对公式
2. **描述统计箱线 + 个体值图**
   - 缺失/`*` 诊断；解释不写过程合格
3. **正态性检验输出合同**
   - 概率图+直方图悬停原始行；`NormalityFacts`；未拒绝不得写成已正态
4. **相关矩阵散点**
   - complete-case 行主序；两列散点 + `PlotKind::matrix`；`CorrelationFacts`；不改 Pearson/Spearman

公式：`docs/research/ttest-descriptive-normality-correlation-formulas.md`

### 2026-08-20（Test 2–8 边界 / 能力表形 / Johnson·非正态）✅

1. **SPC Test 2–6、Test 8 边界**
   - 每 Test 独立 synthetic 负例 + I-MR/Xbar-R 服务夹具；Test 7 `<σ` 语义不变
2. **正态能力 / Sixpack 表形**
   - Process Data 含 Anderson-Darling A²/A²*/P/判定；Performance (PPM) 三列；单侧 `*`
   - Sixpack 六图顺序与标题（I/Xbar → 直方图 → MR/R → 正态概率 → 最后 25 → 能力图）
3. **Johnson / 非正态能力输出合同**
   - Johnson 变换表 + Overall Capability；非正态分布参数表；PPM 无 Within 期望列
   - `CapabilityFacts` 扩展 normality/transform/nonnormal 字段；解释不写合格
4. **组间/组内与 Sixpack 标签**
   - Sixpack 子图标题与正态能力 I/MR 图一致；组间/组内直方图 Between/Within 标签保持

公式：`docs/research/spc-tests-capability-johnson-formulas.md`

### 2026-08-20（Test 7 边界 / 能力直方图 / 卡方合同 / Grubbs）✅

1. **I-MR / Xbar Test 7 边界**
   - 领域层 `|y-CL| < σ` 已确认；补 Xbar synthetic、I-MR 历史 σ、I-MR/Xbar-R 服务层夹具
   - 不改 Test 1–6/8
2. **正态能力直方图参考线合同**
   - LSL/USL/Target 虚线标签；Within/Overall 曲线图例；组间/组内曲线用 σ_BW
   - Sixpack 复用同 spec；解释不写合格
3. **卡方关联输出合同**
   - 观察频数表（含合计）+ 卡方检验 + 单元格统计；`ChiSquareFacts` 扩展 N/N* 与 LR 字段
   - 解释只读 Facts，不写因果
4. **Grubbs 异常值检验**
   - 命令 `outlier_test`；complete-case + `source_row`；个体值图；`OutlierTestFacts`
   - 不做 Dixon / 功效样本量

公式：`docs/research/spc-capability-chi-grubbs-formulas.md`

### 2026-08-20 后（Nested Gage By Part + MsaFacts 图元数据）✅

1. **Nested Gage By Part**
   - 复用 `append_gage_by_part_plot`；门控 `replicate_count >= 2 && design_balanced`
   - complete-case `source_row`；不做 Operator×Part 交互；ANOVA/ndc/截断不变
2. **MsaFacts 图元数据**
   - `by_part_plot_available` / `interaction_plot_available` / `plot_point_count`
   - crossed + nested 服务层填充；JSON round-trip；解释只读不写量具通过

公式：`docs/research/gage-by-part-interaction-formulas.md`、`docs/research/nested-gage-arima-kappa-anova-formulas.md` §2

### 2026-08-20 后（Gage 图 / SPC 逐点 / Johnson 图 / 可靠性图 / σ 口径）✅

1. **交叉 Gage By Part / Operator×Part**
   - 表不变；complete-case 按零件散点+均值连线；操作者×零件单元格均值交互图；`source_row`
   - 不改 ndc / 截断
2. **SPC I-MR / I-MR-R/S 逐点表 + 阶段 + 历史 μ/σ**
   - I-MR「逐点统计」含触发测试集与原始行；历史参数标注；阶段列打断 Test/MR 窗口
   - I-MR-R/S 子组逐点表；Xbar-R/S 阶段列接入
3. **Johnson 变换后正态概率图**
   - 变换成功才出；失败只诊断；不改 Pp/Ppk
4. **可靠性 weibull3 / exponential2 / lognormal3 生存曲线与概率图**
   - 可识别且收敛才出图；比较表仍二参数；无 golden
5. **I-MR-R/S 与组间/组内 σ 口径**
   - n≤8 R̄/d2、n≥9 S̄/c4 共用 `estimate_within_subgroup_sigma`；不改 Cp 公式
6. **`docs/algorithm-wiring-index.md`** 命令/Facts/公式索引
7. **RegressionFacts / PcaFacts** 残差 AD 与图计数字段 round-trip
8. **回归多预测变量残差 vs 预测变量**（此前已实现，本轮只回文档）

公式：`docs/research/gage-by-part-interaction-formulas.md`、`spc-tests-capability-johnson-formulas.md`、`kendall-exp2-lognormal3-formulas.md`

### 2026-08-20 后（Laney/P-U 阶段 / 属性图阶段 / DOE 立方图诊断）✅

1. **Laney P'/U' 逐点表契约**
   - `LaneyChartOptions.phase_labels` 在 domain 层打断 Test 2–8 窗口；逐子组表含 Test 1–8、`source_row`、阶段列
   - 历史中心线 / Sigma Z 参数表标注；`SpcFacts.sigma_z` / `out_of_control_count`
2. **P/NP/C/U 逐子组表 + 可选阶段列**
   - 菜单可选阶段列；`attribute_chart_table` 含阶段；`phase_labels` 重算特殊原因；`SpcFacts`
3. **DOE ≥4 因子立方图诊断**
   - 明确 info 文案（含因子数）；`DoeFacts.factor_count` / `cube_plot_available`；解读 limitations；`doe_factorial_output_test`

公式：`docs/research/spc-control-charts.md` §5.1、[`regression-doe-tolerance-chart-formulas.md`](research/regression-doe-tolerance-chart-formulas.md) §3.2

### 2026-08-20 后（Xbar-R/S / EWMA/CUSUM 输出合同）✅

1. **Xbar-R/S 逐子组表对齐 I-MR/Laney**
   - `subgroup_dual_point_table` 合并 Xbar 与 R/S 触发测试；阶段列对双图重算 Test 1–8 / 1–4
   - `SpcFacts.sigma_within` / `out_of_control_count`（Test 1 并集）；`finalize_page`
   - I-MR 顺带显式填充 `SpcFacts`
2. **EWMA 逐点表 + 数值参数表**
   - 「EWMA 参数」+「EWMA 逐点统计」（观测值、EWMA、σ、CL/LCL/UCL、Test 1）
   - 菜单/服务接入 `historical_sigma`；`SpcFacts`
3. **CUSUM 逐点表 + 全部信号表**
   - 「CUSUM 参数」+「CUSUM 逐点统计」+「CUSUM 信号」列出全部信号点
   - `SpcFacts.out_of_control_count` = 信号点去重计数

公式：`docs/research/spc-control-charts.md` §9.1–9.3

### 2026-08-20 后（非参数图 / 卡方热图 / McKean–Ryan CI）✅

1. **非参数伴随图**
   - Mann-Whitney / Wilcoxon / Kruskal-Wallis：箱线图 + 个体值图；Wilcoxon 另含配对散点
   - complete-case / `source_row`；缺失/`*` 只诊断不进图；`NonparametricFacts` 扩展 group/plot/missing 计数
2. **卡方关联观察频数热图**
   - 三表不变；`PlotKind::heatmap`；`ChiSquareFacts.plot_available`；解释不写因果
3. **Mann-Whitney McKean–Ryan 置信区间**
   - Hodges–Lehmann 点估计 + 序统计 CI 列；`location_estimate` / `ci_lower` / `ci_upper` round-trip
   - `# source: formula_reference`；无 Minitab golden

公式：[`pca-nonparametric-variance-chart-formulas.md`](research/pca-nonparametric-variance-chart-formulas.md)、[`spc-capability-chi-grubbs-formulas.md`](research/spc-capability-chi-grubbs-formulas.md)

### 2026-08-20 后（Gage %Study Var 条 / Bias %Linearity）✅

1. **Crossed + Nested Gage %Study Var 分量条**
   - 表不变；`append_gage_study_var_pareto` 用 `percent_study_variation`（非 `%Contribution`）
   - 与 `%Contribution` Pareto 并存；累积允许不到 100%；ANOVA/ndc/截断不变
2. **Bias %Linearity（可选过程变差）**
   - `MsaConfiguration::process_variation`（6×σ）；缺省只诊断 `process_variation_not_provided`
   - Linearity / %Linearity / 各级 %Bias 表；`MsaFacts` linearity 字段 round-trip
   - OLS 斜率/均值 CI 带不变；解释不写量具通过

公式：[`poisson-rate-anova-winters-gage-formulas.md`](research/poisson-rate-anova-winters-gage-formulas.md) §5.1、[`type1-bias-twoprop-boxcox-formulas.md`](research/type1-bias-twoprop-boxcox-formulas.md) §3

### 2026-08-20 后（Gage Linearity 全表 / Crossed Run+按零件 Xbar-R / DOE 图序）✅

1. **Gage Linearity Minitab 全表形**
   - Coef / S and R-Sq / Gage Linearity / Gage Bias（各级 Bias/%Bias/t/P + Average）；无 PV 仍出 Gage Bias
   - `intercept_p_value` / `average_bias_p` / `residual_s`；OLS 与均值 CI 带不变
2. **Crossed Gage Run Chart + 按零件 Xbar-R**
   - 替换按操作者 Xbar-R；全数据 Run Chart（中心=均值）；表/ANOVA/ndc/截断不变
   - Nested 仍按操作者 Xbar-R
3. **DOE 2^k 图序合同**
   - `regression-doe-tolerance-chart-formulas.md` §3.3；`doe_factorial_output_test` index 断言

公式：[`type1-bias-twoprop-boxcox-formulas.md`](research/type1-bias-twoprop-boxcox-formulas.md) §3.1、[`poisson-rate-anova-winters-gage-formulas.md`](research/poisson-rate-anova-winters-gage-formulas.md) §5、[`regression-doe-tolerance-chart-formulas.md`](research/regression-doe-tolerance-chart-formulas.md) §3.3

## 5. 2026-08-20 后建议本轮范围（已实现）✅

1. **正态能力 Cp/Cpk/Pp/Ppk 置信区间** ✅  
   Potential/Overall 表为估计/下限/上限；Bissell + χ² 尺度；单侧 `*`；`CapabilityFacts` CI 字段
2. **卡方拟合优度** ✅  
   命令 `chi_square_gof`；列联表 `chi_square` 热图未改
3. **G 图 / T 图** ✅  
   数值间隔列；Test 1；逐点表 `source_row`；`SpcFacts`
4. **功效接线** ✅  
   `PowerFacts`；Actual Power 表；功效曲线；解释不写「足够」
5. **图表侧栏 Auto/字号 + 复制/清除** ✅  
   侧栏写回 `ChartModel`；`copy_to_clipboard` 回退尺寸与 PNG；空串 Display 为空白；`clear_selection` 可用 currentIndex

公式：[`docs/research/capability-ci-gof-rare-event-power-formulas.md`](docs/research/capability-ci-gof-rare-event-power-formulas.md)、ADR 0008

## 5a. 2026-08-20 本轮收尾（已实现）✅

1. **P0 回归修复**
   - 图表复制：surface 聚焦、Ctrl+C 走图表、剪贴板统一 `image + image/png`
   - 清除单元格：`push_table_change` 比较内容；清除只入一条 undo
2. **P1 配对等价 / 方差功效 / DOE 精确 PI**
   - `paired_equivalence`、`one_variance_*` / `two_variance_*`、响应优化精确协方差区间
   - 缺协方差时 CI/PI=`*`，并通过 `DoeFacts.prediction_interval_available` 传给解释层
3. **P1 Gage `%Tolerance` / Bias 表形**
   - crossed / nested 条件式 `%Tolerance` Pareto
   - nested `%Tolerance` 不可用时输出 `*`；Bias 表补 `N / SE Bias`
4. **P2 图表编辑收口**
   - 聚焦图表 Ctrl+Z / Ctrl+Y 撤销重做
   - 复制与 PNG 导出复用同一 pixmap 渲染路径；侧栏非法 `Min >= Max` 自动纠正

## 5c. 2026-08-20 算法深化补齐（已实现）✅

1. **比例等价 z-TOST**
   - 命令 `one_proportion_equivalence` / `two_proportion_equivalence`
   - Wald z；`ci_method=wald_z_tost`；复用 `EquivalenceFacts`（`kind=one_proportion/two_proportion`）
   - 输入对齐单/两比例 complete-case 求和；解释不写「已证明等价」
2. **Weighted Kappa（Cohen linear/quadratic）**
   - `kappa_weight_scheme` 接入领域；去掉 `weighted_kappa_not_implemented`
   - 配置与 Facts 序列化；Fleiss overall 仍未加权 + `fleiss_remains_unweighted`
   - 产品口径：DataLab 有序加权 Cohen，**不是** Minitab AAA（有序仍走 Kendall）
3. **DOE 等值线/曲面 X/Y 因子切换**
   - `contour_x_factor` / `contour_y_factor`；其余编码 hold 0
   - 2 因子默认行为不变；不做可旋转 3D
4. **双样本泊松率比**
   - `two_poisson_rate` + `comparison=ratio`；默认 `difference` 兼容
   - log-Wald CI；零事件诊断；`PoissonRateFacts.ratio*`

公式：[`docs/research/p1_proportion_equivalence_z_tost.md`](research/p1_proportion_equivalence_z_tost.md)、[`docs/research/p1_weighted_kappa_cohen.md`](research/p1_weighted_kappa_cohen.md)、[`docs/research/p1_doe_contour_factor_hold.md`](research/p1_doe_contour_factor_hold.md)、[`docs/research/p1_poisson_rate_ratio.md`](research/p1_poisson_rate_ratio.md)

## 5d. 2026-08-20 算法深化（本轮四项竖切）✅

1. **单比例 Wilson score CI**
   - `one_proportion` 方法 `wilson`；CI=`wilson_score`，检验仍用 score z under p0
   - `ProportionFacts.ci_method`；不做 Agresti–Coull / Blaker / Wilson CC
2. **Bonett 等方差**
   - `variance_test_method=bonett`；两样本 SD 比 Z/CI（Banga–Fox）；k>2 诊断
   - Levene 中位数默认不变；不做 Bartlett
3. **泊松率功效**
   - `t_power` mode `one_poisson_*` / `two_poisson_*`；正态近似；`observation_length`
   - 不重做方差/比例功效；不做泊松 Blaker
4. **ANOVA Tukey 区间表形**
   - 「Tukey 同时比较」拆下限/上限；「Tukey 差值同时区间」图；近似算法不变

公式：[`p1_wilson_proportion_ci.md`](research/p1_wilson_proportion_ci.md)、[`p1_bonett_equal_variance.md`](research/p1_bonett_equal_variance.md)、[`p1_poisson_rate_power.md`](research/p1_poisson_rate_power.md)、[`p1_anova_tukey_interval_table.md`](research/p1_anova_tukey_interval_table.md)

## 5e. 2026-08-20 算法深化（本轮四项竖切）✅

1. **单比例 Agresti–Coull CI**
   - `one_proportion` 方法 `agresti_coull`；CI=`agresti_coull`；检验仍用 score z under p0
   - 不做 Blaker / Wilson CC / 两比例 AC
2. **Bartlett 等方差**
   - `variance_test_method=bartlett`；k≥2 χ² + 校正；与 F/Levene/Bonett 并列
   - 解释不写「已证明等方差」；不做 Bartlett 功效
3. **DOE 实际单位 hold**
   - `contour_hold_actual` + UI `hold=名=值;…`；实际→编码；越界 clamp
   - 空 hold 仍编码 0；不做可旋转 3D
4. **ANOVA Tukey Grouping 字母**
   - 「Grouping Information」CLD；只消费现有 `significant`；不改近似算法
   - 不做精确 Studentized Range；不做 TOST 均值比

公式：[`p1_agresti_coull_proportion_ci.md`](research/p1_agresti_coull_proportion_ci.md)、[`p1_bartlett_equal_variance.md`](research/p1_bartlett_equal_variance.md)、[`p1_doe_actual_unit_hold.md`](research/p1_doe_actual_unit_hold.md)、[`p1_anova_tukey_grouping_letters.md`](research/p1_anova_tukey_grouping_letters.md)

## 5f. 2026-08-20 算法深化（本轮四项竖切）✅

1. **双样本均值比 TOST**
   - 命令 `two_sample_equivalence_ratio`；非对数；Fieller 100(1−2α)%
   - `EquivalenceFacts.kind=two_sample_ratio`（`difference` 存 ρ̂）
   - 不做对数变换；不改差值/配对/比例 TOST
2. **两比例 Newcombe–Wilson CI**
   - `two_proportions` method=`wilson`；`ci_method=newcombe_wilson`
   - 检验 Z 仍 unpooled Wald；默认 `normal`/Wald 不变
   - 不做两比例 Agresti–Coull / Blaker
3. **Kruskal Dunn 多重比较**
   - Dunn–Bonferroni 成对表 + Grouping Information (Dunn)
   - 复用 `tukey_grouping_letters`；`NonparametricFacts.dunn_*`
   - 不做 Steel–Dwass
4. **Multi-Vari 第 4 因子**
   - 因子上限 2～4；complete-case / `source_row`；覆盖率门不变

公式：[`p1_two_sample_mean_ratio_tost.md`](research/p1_two_sample_mean_ratio_tost.md)、[`p1_two_proportion_newcombe_wilson.md`](research/p1_two_proportion_newcombe_wilson.md)、[`p1_kruskal_dunn_posthoc.md`](research/p1_kruskal_dunn_posthoc.md)、[`p1_multi_vari_fourth_factor.md`](research/p1_multi_vari_fourth_factor.md)

## 5g. 2026-08-20 算法深化（本轮四项竖切）✅

1. **均值比 TOST 对数变换**
   - `two_sample_equivalence_ratio` + `transform=log`；几何均值比；`ci_method=tost_ratio_log_1_minus_alpha`
   - 默认 `none` 非对数 Fieller 不变；全正值门控
2. **两比例 Agresti–Coull 差值 CI**
   - `two_proportions` method=`agresti_coull`；`ci_method=agresti_coull_diff`
   - 检验 Z 仍 unpooled Wald；默认/wilson 不变；不做 Blaker
3. **Kruskal 后 Steel–Dwass（近似）**
   - `posthoc=steel_dwass`；成对 Wilcoxon + 渐近 TK 临界；Grouping (Steel-Dwass)
   - 默认 `dunn` 不变
4. **Friedman 检验**
   - 命令 `friedman`；响应+处理+区组；complete-case / `source_row`；平衡设计；结修正 χ²
   - 不做后比较

公式：[`p1_tost_ratio_log_transform.md`](research/p1_tost_ratio_log_transform.md)、[`p1_two_proportion_agresti_coull_ci.md`](research/p1_two_proportion_agresti_coull_ci.md)、[`p1_kruskal_steel_dwass.md`](research/p1_kruskal_steel_dwass.md)、[`p1_friedman_test.md`](research/p1_friedman_test.md)

## 5b. 以后再计划的项（不要从本节当成本轮任务）

不要重做 §4、§5、§5a、§5c、§5d、§5e、§5f、§5g 已完成项。帮助中心已有，不要重做。延后项见 `docs/research/deferred-capability-agreement.md`（Blaker、Kalman/TSERIES、可旋转 3D、Nemenyi 独立命令、Jackson–Mudholkar 解析限、重构阶段 5/6）。

## 6. 硬约束

- 解释层只陈述证据与假设状态，不写过程合格、量具通过、分布已证明、已证明一致。
- 未从 Minitab 导出的结果不得写入 `tests/fixtures/minitab/VALIDATION_MATRIX.md`。公式参考测试标注 `# source: formula_reference`。
- 电脑是中文路径：改完说明让用户在 Qt Creator 自行测试；agent **不**在易损坏环境下跑 cmake/ctest。
- 新文件加入对应 CMake target；跨层 include 过 `tools/check_layering.ps1`（由用户本地跑）。
- `align_complete_rows` 输出行主序。应用层新头文件里的 domain 类型写 `domain::X`。
- 源码 UTF-8 无 BOM。
- 回复格式：切入点理解、任务、影响文件、测试策略、实现说明；改完列 Qt Creator 手工验收项。

本轮不做（见 `deferred-capability-agreement.md`）：Blaker；无界似然 bias-correction 数值对齐；Kalman / TSERIES 对齐；Jackson–Mudholkar 解析限；图表注释、拖拽布局、多图拼版；可旋转 3D；Nemenyi 独立命令 / Friedman 后比较；精确 studentized-range；重构阶段 5/6（PlotSpec 合一、CI、i18n），除非挡住本轮接线。不要重做已完成的配对 TOST / 方差功效 / DOE 精确 PI / Gage %Tol+Bias / 图表复制清除 / Wilson / Bonett / 泊松功效 / Tukey 表形 / Agresti–Coull 单比例 / Bartlett / DOE 实际 hold / Tukey Grouping / 均值比 TOST（含对数） / 两比例 Newcombe–Wilson / 两比例 Agresti–Coull / Kruskal Dunn / Steel–Dwass / Multi-Vari 第 4 因子 / Friedman。

## 7. 可贴给新对话的短提示词

见仓库外用户粘贴稿；仓库内权威范围以本节之上第 5、6 节为准。新对话应先 SwitchMode 到 **plan**，研究完成后再 agent 实现。
