# 回归残差图 / Fitted Line 带 / DOE 析因图 / 正态容差区间 / 系列样式

> 研究日期：2026-08-18  
> 访问日期：2026-08-18（UTC+8）  
> 本文只记录官方公式、Minitab 表形与本轮实现边界。`formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| 残差图选项 | [Select graphs for Fit Regression Model](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-regression-model/perform-the-analysis/select-the-graphs-to-display/) | 2026-08-18 |
| 残差图解读 | [Residual plots for Fit Regression Model](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-regression-model/interpret-the-results/all-statistics-and-graphs/residual-plots/) | 2026-08-18 |
| 残差图总览 | [Residual plots in Minitab](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/supporting-topics/residuals-and-residual-plots/residual-plots-in-minitab/) | 2026-08-18 |
| 拟合与 SE Fit / CI | [Fits and residuals methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-regression-model/methods-and-formulas/fits-and-residuals/) | 2026-08-18 |
| Predict CI / PI | [Methods and formulas for Predict](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/using-fitted-models/how-to/predict/methods-and-formulas/methods-and-formulas/) | 2026-08-18 |
| Fitted Line 图 | [Fitted line plot](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fitted-line-plot/interpret-the-results/all-statistics-and-graphs/fitted-line-plot/) | 2026-08-18 |
| Fitted Line 选项 | [Fitted Line Plot options](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fitted-line-plot/perform-the-analysis/select-the-analysis-options/) | 2026-08-18 |
| NIST 均值区间 | [NIST 4.5.1.1 confidence interval](https://www.itl.nist.gov/div898/handbook/pmd/section5/pmd511.htm) | 2026-08-18 |
| NIST 预测区间 | [NIST 4.5.1.2 prediction interval](https://www.itl.nist.gov/div898/handbook/pmd/section5/pmd512.htm) | 2026-08-18 |
| DOE 效应图公式 | [Effects plots methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-factorial-design/methods-and-formulas/effects-plots/) | 2026-08-18 |
| DOE 效应图解读 | [Effects plots for Analyze Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-factorial-design/interpret-the-results/all-statistics-and-graphs/effects-plots/) | 2026-08-18 |
| Cube Plot | [Overview for Cube Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/using-fitted-models/how-to/cube-plot/before-you-start/overview/) | 2026-08-18 |
| Cube Plot 解读 | [Interpret Cube Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/using-fitted-models/how-to/cube-plot/interpret-the-results/key-results/) | 2026-08-18 |
| 正态容差区间公式 | [Tolerance Intervals (Normal) methods](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/tolerance-intervals-normal-distribution/methods-and-formulas/methods-and-formulas/) | 2026-08-18 |
| NIST 容差区间 | [NIST 7.2.6.3](https://www.itl.nist.gov/div898/handbook/prc/section2/prc263.htm) | 2026-08-18 |
| 图形属性 | [Graph attributes](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-editing-basics/edit-attributes-for-fills-fonts-and-lines/) | 2026-08-18 |

## 2. 线性回归残差 4 图与 Fitted Line 带

Minitab Fit Regression Model 的残差图契约：残差-拟合值、残差-顺序、残差-预测变量、正态概率图。残差对拟合/顺序/预测变量图以 0 为中心线。顺序图的 X 为数据行顺序（complete-case 后的输入顺序）。本轮残差图使用普通残差 `e_i = y_i − ŷ_i`，不改 Unusual 表 R/X/I，不改 `RegressionFacts` 的 outlier / high_leverage / influential 计数语义。

### 2.1 Fitted Line 置信带与预测带

Minitab Fitted Line Plot 仅适用于**一个**连续预测变量。图含观测点、回归线，可选 95% CI 与 PI（虚线带）。PI 始终宽于对应 CI。

单预测变量 SE Fit（与 Minitab / 多元通式等价）：

```text
SE_fit(x0) = s · √( x0′ (X′X)⁻¹ x0 )
           = s · √( 1/n + (x0 − x̄)² / Σ(x_i − x̄)² )
SE_pred(x0) = s · √( 1 + x0′ (X′X)⁻¹ x0 )
```

其中 `x0 = [1, x]`（含截距），`s² = MSE`，`ν = n − p`，`p` 含截距。

NIST / Minitab：

```text
CI:  ŷ(x0) ± t_{1−α/2, ν} · SE_fit(x0)
PI:  ŷ(x0) ± t_{1−α/2, ν} · SE_pred(x0)
```

`error_df ≤ 0` 或秩亏：不输出带，只诊断。多预测变量不画 2D Fitted Line（残差-预测变量图已覆盖）。

### 2.2 本轮表/图合同

- 残差 4 图标题保持：「残差与拟合值」「残差与观测顺序」「残差与预测变量 - {名}」「残差正态概率图」。
- 单预测变量追加「拟合线图」：`actual` 点保留 `source_row`；`fitted` 线；两条 `confidence_band`（标签区分 CI / PI）。
- 残差散点加 y=0 参考系列。

## 3. DOE 析因图

### 3.1 Pareto of standardized effects

Minitab Analyze Factorial Design 效应图：

- 误差 df ≥ 1：条高为 `|t|`（标准化效应），参考线为 `t_{1−α/2, ν_error}`。图名 Pareto Chart of the Standardized Effects。
- 误差 df = 0：无法算标准化效应，条高为 `|effect|`，参考线用 Lenth PSE 的边际误差 `ME = t*PSE`。`t` 为 `t_{1−α/2, m/3}`，`m` = 效应个数（不含截距）。图名 Pareto Chart of the Effects。诊断码 `lenth_pse_unreplicated`。

Lenth PSE（Minitab 四步）：

```text
1. 取 |effect_i|（不含截距）
2. S = 1.5 · median(|effect|)
3. 对 |effect| < 2.5 S 的效应再取中位数
4. PSE = 1.5 · 该中位数
ME = t_{1−α/2, m/3} · PSE
```

2 水平编码下 `effect = 2 · coefficient`，故 `|t_effect| = |t_coefficient|`。Pareto 排除截距，区组项可进入。默认 α = 0.05。

本轮不把质量柏拉图的累积百分比套到效应条上：效应 Pareto 不填 `cumulative_percent`；渲染器在累积为空时不画右轴与累积线；`y_axis_title` 非空时替代「计数」。

解释只陈述最大 |t| 或 |effect| 与参考线方法，不写「因子显著即过程合格」。

### 3.2 Cube / 主效应

Cube Plot：2 因子为方形，3 因子为立方（Minitab Cube Plot overview，访问日期 2026-08-20）。
本轮用 **data means**（该编码组合 complete-case 响应的算术平均），2/3 因子画图；
≥4 因子只输出 info 诊断 `cube_plot_requires_2_or_3_factors`，文案含当前因子数并提示
改用主效应/交互/等值线/曲面；不画假立方图。`DoeFacts.factor_count` 与
`DoeFacts.cube_plot_available` 供解读层只读。3 因子用二维等距投影，不新增 `ChartKind`。
角点 `point_labels` 为均值文本，棱为 `PlotSeries`。

主效应图：低/高水平均值连线；X 轴使用因子 `low_level` / `high_level` 文本；`PlotSeries` + `show_points`。不重做交互图逻辑。

### 3.3 2^k 响应页输出顺序合同（2026-08-20 后）

访问日期：2026-08-20（UTC+8）。`formula_reference ≠ golden`。`doe_pages.cpp` 装配顺序：

**表（push 序）**：系数与效应 → DOE ANOVA → [模型项与区组] → [纯误差与失拟] → [中心点与曲率] → 残差诊断。

**图（2 因子响应页，11 块）**：

| Index | 块 |
|---|---|
| 0 | 标准化效应 Pareto（或 Lenth PSE） |
| 1 | 立方图 |
| 2..F+1 | 各因子主效应图（因子索引序） |
| F+2.. | 交互作用图（first<second 序） |
| … | 等值线图、响应曲面图（≥2 因子且网格 OK） |
| 末 4 | 残差与拟合值 → 残差与观测顺序 → 残差正态概率图 → 残差直方图 |

2 因子 8 运行典型：`plots[0]` Pareto … `plots[7..10]` 残差 4 图。≥4 因子无立方图 + `cube_plot_requires_2_or_3_factors` 诊断。

## 4. 正态容差区间

对标 Stat > Quality Tools > Tolerance Intervals（Normal Distribution）。本轮只做正态方法，不做非参数、二项、泊松。

表形：

- 过程数据：N、N*、Mean、StDev
- 正态容差区间：方法、覆盖率 P、置信水平 1−α、下限、上限

区间：`L = ȳ − k s`，`U = ȳ + k s`（单侧只输出对应一侧）。

### 4.1 双侧（Howe 1969，NIST 7.2.6.3）

Minitab 官方双侧用 Krishnamoorthy–Mathew 精确积分；本轮采用 NIST 发表的 Howe 近似，测试标 `# source: formula_reference`，**不是** Minitab golden。诊断 `two_sided_howe_approximation`。

NIST 将 α 记为**置信水平**。k₂：

```text
k₂ = z_{(1+p)/2} · √[ ν (1 + 1/N) / χ²_{α_lower, ν} ]
```

其中 `ν = N−1`，`χ²_{α_lower, ν}` 是 χ²(ν) 的下侧 `1−confidence` 分位。NIST 硅片例：N=43，p=0.90，置信 0.99 → z_{0.95}=1.645，χ²_{0.01,42}=23.650，**k₂=2.217**。

可选 Guenther (1977) 校正 `w`；本轮采用 Howe 主项以便与 NIST 例数字对齐。

### 4.2 单侧（Natrella 1963，NIST 7.2.6.3）

Minitab 单侧精确因子为非中心 t：`k₁ = t'_{1−α}(ν, z_p √N) / √N`。本轮用 NIST 发表的 Natrella 近似，诊断 `one_sided_natrella_approximation`：

```text
a = 1 − z_α² / [2(N−1)]
b = z_p² − z_α² / N
k₁ = (z_p + √(z_p² − a b)) / a
```

NIST 同例上侧：k₁=1.8752。解释层陈述区间与 `assumption_status = not_verified`，不写过程合格或规格已覆盖。

边界：n<2、s=0、P 或置信越界 → 只诊断，不填区间。

## 5. 图表侧栏系列色 / 线宽

Minitab 图属性可改系列颜色与线宽。本轮：侧栏选中「数据系列 / …」时写入 `ChartModel.series[i].style.color` 与 `line_width`；未选中系列时全局默认线宽仍写 `model.line_width`。主题不覆盖用户系列色。预览 / 复制 / PDF 仍走 `ChartRenderer::render_to_pixmap`。不做注释、拖拽、多图拼版。
