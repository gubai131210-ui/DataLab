# Nested Gage 图 / ARIMA 明细 / 属性一致图 / 单因素残差直方图

> 研究日期：2026-08-19  
> 访问日期：2026-08-19（UTC+8）  
> 本文只记录官方公式、Minitab 表形/图名与本轮实现边界。`formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Nested Gage ANOVA | [ANOVA table for Nested Gage R&R](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/nested-gage-r-r-study/methods-and-formulas/anova-table/) | 2026-08-19 |
| Nested Gage 方差分量 | [Variance components for Nested Gage R&R](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/nested-gage-r-r-study/methods-and-formulas/gage-r-r-table/) | 2026-08-19 |
| Nested Gage 图 | [Graphs for Nested Gage R&R](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/nested-gage-r-r-study/interpret-the-results/all-statistics-and-graphs/graphs/) | 2026-08-19 |
| 交叉 Gage 图口径 | 见 [`poisson-rate-anova-winters-gage-formulas.md`](poisson-rate-anova-winters-gage-formulas.md) §5 | 2026-08-19 |
| ARIMA | [ARIMA methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/time-series/how-to/arima/methods-and-formulas/methods-and-formulas/) | 2026-08-19 |
| Best ARIMA | [Forecast with Best ARIMA model](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/time-series/how-to/forecast-with-best-arima-model/methods-and-formulas/methods/) | 2026-08-19 |
| 属性一致性图 | [Graphs for Attribute Agreement Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/attribute-agreement-analysis/attribute-agreement-analysis/interpret-the-results/all-statistics-and-graphs/graphs/) | 2026-08-19 |
| Kappa | [Kappa statistics](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/attribute-agreement-analysis/attribute-agreement-analysis/methods-and-formulas/kappa-statistics/) | 2026-08-19 |
| 单因素残差图 | [Residual plots for One-Way ANOVA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/one-way-anova/interpret-the-results/all-statistics-and-graphs/residual-plots/) | 2026-08-19 |
| NIST 残差图通则 | [NIST e-Handbook process modeling / residuals](https://www.itl.nist.gov/div898/handbook/pmd/section4/pmd44.htm) | 2026-08-19 |

主 Nested Gage 图页在部分文档树可能 404；表形按 Minitab Nested Gage 常用「分量条 + 按操作者 Xbar-R」描述，与交叉 Gage 本轮已实现口径对齐，不填写未导出数值。

## 2. Nested Gage R&R 图

对标质量工具 > Gage Study > Nested Gage R&R。领域 ANOVA / 方差分量 / `ndc = floor(1.41 × σ_Part / σ_Gage)` / 负方差截断 **不改**。

嵌套设计：每个零件只属于一个操作者。方差分量来源名为 `Repeatability`、`Reproducibility`（操作者）、`Total Gage R&R`、`Part-To-Part`、`Total Variation`。

本轮图合同（对齐交叉 Gage）：

| 图 | 合同 |
|---|---|
| 方差分量 %Contribution | `PlotKind::pareto`；类别仅 Repeatability / Reproducibility / Part-To-Part；值为 `%Contribution`。不把 Total Gage R&R 叠进同一条图。 |
| 按操作者 Xbar / R | 每个（操作者, 零件）单元格的重复测量为一等量子组，调用已有 `xbar_range_dual`；`phase_labels` 为操作者；`source_rows` 为子组首行。重复 <2 不画 R。 |
| 按零件 By Part | 与交叉 Gage 相同合同，见 [`gage-by-part-interaction-formulas.md`](gage-by-part-interaction-formulas.md) §2。Minitab 来源：[Graphs for Nested Gage R&R](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/nested-gage-r-r-study/interpret-the-results/all-statistics-and-graphs/graphs/)。平衡且 `replicate_count >= 2` 才出图；否则只诊断。 |

导入：测量 + 零件 + 操作者 complete-case；`*` / 空标签跳过并诊断；`source_row` 可追溯。不平衡设计只诊断不画控制图与 By Part。

明确不做：Operator×Part 交互图（嵌套设计中零件不跨全部操作者，语义不成立）、改 ndc/截断。

## 3. ARIMA 拟合-预测明细

对标 Stat > Time Series > ARIMA。估计仍为候选 CSS（`arima_candidate_css`），**不是** Minitab TSERIES 迭代最小二乘 + back forecast，**不是** Kalman 状态空间 MLE。

Minitab 输出常见表形：模型信息、参数、残差 ACF、预测。本轮只补与 SES/Winters 对齐的明细表形，不宣称数值等于 TSERIES。

| 表 | 合同 |
|---|---|
| 候选模型比较 | 已有：模型 / SSE / AIC / AICc / BIC |
| 模型摘要与预测 | 已有：最优模型、Forecast / Lower / Upper |
| 拟合与预测明细 | `序号, 原始行, Observed, Fitted, Residual, Forecast, Lower, Upper`。历史行填 Observed/Fitted/Residual；预测行只填 Forecast 与区间 |

`ForecastFacts.mape` / `mase` 只读。`parameter_source=estimated`。缺失单元格计入 `missing_count`；时间列乱序/重复仍报错。

## 4. 属性一致性图

对标 Attribute Agreement Analysis 图。Cohen（两两）与 Fleiss（≥3）**不改**。Weighted Kappa 仍不实现。

评估者 × 零件一致率：

```text
对单元格 (评估者 a, 零件 p)：
  有效评级 = 该格非空评级（空评级不进分母）
  参照 = 若该零件有标准则用标准；
         否则用该零件全体有效评级的众数；
         众数平票 → 该格 NaN，诊断 ambiguous_part_mode
  一致率 = 100 × (与参照相同的有效评级数) / (有效评级数)
```

| 图 | 合同 |
|---|---|
| 评估者×零件热图 | `PlotKind::heatmap`；行=评估者，列=零件；颜色 0–100 |
| 评估者一致率条 | `PlotKind::pareto`（无累计线）；类别=评估者，值为已有 within `Agreement %` |

解释只陈述观察一致率与 Kappa；不写「已证明一致」。

## 5. 单因素 ANOVA 残差直方图

对标 One-Way ANOVA residual plots。组均值个体 CI（pooled MSE）与 Tukey **不改**。

残差 `e_i = y_i − ȳ_{g(i)}`，顺序为输入行序（complete-case）。四图：残差-拟合（y=0）、残差-顺序（y=0）、正态概率、直方图（Sturges，`histogram(..., 0)`）。解释不写残差已正态。
