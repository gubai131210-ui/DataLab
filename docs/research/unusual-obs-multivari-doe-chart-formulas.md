# Unusual Observations / Multi-Vari / DOE 独立目标 / 图表属性

> 研究日期：2026-08-18  
> 访问日期：2026-08-18（UTC+8）  
> 本文只记录官方公式、Minitab 表形与本轮实现边界。`formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Unusual 表形 | [Fits and diagnostics table](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-regression-model/interpret-the-results/all-statistics-and-graphs/fits-and-diagnostics-table/) | 2026-08-18 |
| Unusual 规则 | [Unusual observations](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/supporting-topics/model-assumptions/unusual-observations/) | 2026-08-18 |
| 诊断公式 | [Diagnostic measures](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-regression-model/methods-and-formulas/diagnostic-measures/) | 2026-08-18 |
| NIST 诊断 | [Regression Diagnostics](https://www.itl.nist.gov/div898/software/dataplot/refman1/auxillar/regrdiag.htm) | 2026-08-18 |
| Multi-Vari 概述 | [Overview for Multi-Vari Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/multi-vari-chart/before-you-start/overview/) | 2026-08-18 |
| Multi-Vari 输入 | [Enter your data](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/multi-vari-chart/perform-the-analysis/enter-your-data/) | 2026-08-18 |
| Multi-Vari 数据要求 | [Data considerations](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/multi-vari-chart/before-you-start/data-considerations/) | 2026-08-18 |
| 个体/总体 D | [Individual and composite desirability](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/using-fitted-models/supporting-topics/response-optimization/what-are-individual-desirability-and-composite-desirability/) | 2026-08-18 |
| Desirability 权重 | [Determining the weight](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/using-fitted-models/supporting-topics/response-optimization/determining-the-weight/) | 2026-08-18 |
| Importance | [What is importance](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/using-fitted-models/supporting-topics/response-optimization/what-is-importance-in-response-optimization/) | 2026-08-18 |
| 合成 D | [Composite desirability](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/using-fitted-models/how-to/response-optimizer/methods-and-formulas/composite-desirability/) | 2026-08-18 |
| 图形编辑 | [Edit graphs](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-editing-basics/edit-graphs/) | 2026-08-18 |
| 图形属性 | [Graph attributes](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-editing-basics/edit-attributes-for-fills-fonts-and-lines/) | 2026-08-18 |

## 2. 线性回归 Unusual Observations

Minitab Fit Regression Model 在存在异常点时输出 **Fits and Diagnostics for Unusual Observations**，只列打标行，不列全部观测。示例列：Obs、响应、Fit、Resid、Std Resid，脚注 R / X。

### 2.1 杠杆与标准化残差

帽子矩阵对角线 \(h_i\)（Minitab Hi）。内部标准化残差：

```text
r_i = e_i / (s √(1 − h_i))
```

Cook 距离与 DFITS（与现有 `regression.cpp` 一致，NIST Dataplot 同形）：

```text
D_i = r_i² · h_i / [p (1 − h_i)]
DFITS_i = r_i · √(h_i / (1 − h_i))
```

其中 \(p\) 为模型系数个数（含截距），\(n\) 为 complete-case 观测数。

### 2.2 本轮表形打标（对标 Minitab 字母 + DataLab I）

| 标记 | 规则 | 来源 |
|---|---|---|
| R | `|内部标准化残差| > 2` | Minitab Unusual 表 |
| X | `h_i > min(3p/n, 0.99)` | Minitab Unusual 表 |
| I | `Cook's D > 4/n` 或 `|DFITS| > 2√(p/n)` | DataLab 现有影响点；Minitab 该表不标 I |

表「异常观测」仅含 R/X/I 任一为真的行。无打标行则不输出该表。列：观测、原始行、响应、拟合值、残差、标准化残差、杠杆、Cook、DFITS、标记。

### 2.3 不改动的现有契约

「拟合与诊断」全表与 `RegressionFacts` 计数语义保持：

- `is_outlier` / `outlier_count`：`|删除学生化残差| > 3`
- `is_high_leverage` / `high_leverage_count`：`h_i > 2p/n`
- `is_influential` / `influential_count`：Cook / DFITS 阈值同上 I

解释层只读 `RegressionFacts`，不自动删除观测，不写模型合格。

## 3. Multi-Vari Chart

对标 Stat > Quality Tools > Multi-Vari Chart。输入：测量列 + **2～4** 个因子列（complete-case；第 4 因子见 [`p1_multi_vari_fourth_factor.md`](p1_multi_vari_fourth_factor.md)）。

数据要求（官方）：

- 响应必须为数值。
- 每个因子至少 2 个水平。
- 至少覆盖全部因子水平组合的 60%；不足则不能画图，只诊断。

均值：单元内 complete-case 测量的算术平均。因子水平用原始单元格文本（不把分类改写成数值编码）。缺失、`*`、非法测量跳过并计数。

输出合同：

- 表「因子均值」：因子、水平、N、均值
- 表「单元均值」：各因子水平组合、N、均值
- 图：`PlotKind::scatter` + 个体点系列 + 按因子 2 连接的均值线；三因子用嵌套 x 位置，不新增 PlotKind
- `MultiVariFacts`：factor_count、valid_count、missing_count、combination_coverage
- 解释只陈述分层均值可见，不写过程合格

## 4. DOE 每响应独立目标

Minitab Response Optimizer 为每个响应指定 Goal（maximize / minimize / target）、Lower / Target / Upper、Weight（desirability 形状）与 Importance（合成 D 权重）。

个体 desirability（形状权重 = 1，本轮不改）：

```text
maximize: d = 0 (y≤L);  (y−L)/(U−L)  (L<y<U);  1 (y≥U)
minimize: d = 1 (y≤L);  (U−y)/(U−L)  (L<y<U);  0 (y≥U)
target:   在 [L,U] 内按目标分段线性，界外为 0
```

合成 D（DataLab `weight` = Minitab **Importance**）：

```text
D = exp( Σ w_i ln(d_i) / Σ w_i )
```

任一 \(d_i = 0\) 则 \(D = 0\)。

本轮：UI 为每个响应填独立 goal / lower / upper / target / weight，写入 `optimization_objectives`。单响应仍可用旧扁平字段并复制到 `optimization_objectives[0]`。服务层多响应表形与 `DoeFacts` 已完成，不重做。

## 5. 图表属性页

工作模型唯一为 `ChartModel`；确认后写回 `PlotSpec`。预览、PDF、PNG、复制共用 `ChartRenderer::render_to_pixmap`。

本轮：图元选中路径 `chart_interaction::element_path` 与属性页联动；系列列表显示 `ChartSeries.label`；编辑面板在图表右侧且不覆盖绘图区；对话框预览在 Tabs 右侧。

不做：注释、拖拽布局、多图拼版。

## 6. 本轮实现边界

| 模块 | 位置 |
|---|---|
| 回归诊断标记 | `src/domain/statistics/regression.*` |
| Unusual 表 | `AnalysisService::regression` |
| Multi-Vari | `src/domain/statistics/multi_vari.*` + `AnalysisService::multi_vari` |
| DOE 目标 UI | `analysis_commands` + `analysis_setup_dialog` |
| 图表 UX | `graph_properties_panel` / `dialog` / `analysis_chart_widget` |

未实现：desirability 形状 Weight ≠ 1；图表注释与 Layout。Multi-Vari 第 4 因子已接入。
