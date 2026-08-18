# 三参数 Weibull / Fleiss Kappa / 图表编辑公式笔记

> 研究日期：2026-08-18  
> 访问日期：2026-08-18（UTC+8）  
> 本文只记录官方公式与本轮实现边界。公式参考测试不是 Minitab 导出，不得写成数值对齐。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| 阈值参数 | [Distributions with threshold parameters](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/supporting-topics/distribution-models/distributions-with-threshold-parameters/) | 2026-08-18 |
| 右删失估计 | [Estimation methods (right censoring)](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/parametric-distribution-analysis-right-censoring/methods-and-formulas/estimation-methods/) | 2026-08-18 |
| 参数估计字段 | [Parameter estimates (right censoring)](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/parametric-distribution-analysis-right-censoring/methods-and-formulas/parameter-estimates/) | 2026-08-18 |
| Kappa | [Kappa statistics for Attribute Agreement Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/attribute-agreement-analysis/attribute-agreement-analysis/methods-and-formulas/kappa-statistics/) | 2026-08-18 |
| Kappa 与 Kendall | [Kappa statistics and Kendall's coefficients](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/supporting-topics/attribute-agreement-analysis/kappa-statistics-and-kendall-s-coefficients/) | 2026-08-18 |
| Kendall（本轮不做） | [Kendall's coefficients methods](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/attribute-agreement-analysis/attribute-agreement-analysis/methods-and-formulas/kendall-s-coefficients/) | 2026-08-18 |
| 图形编辑 | [Edit graphs](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-editing-basics/edit-graphs/) | 2026-08-18 |
| 图形区域 | [Graph regions](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-framework-and-scale/graph-regions/) | 2026-08-18 |

## 2. 三参数 Weibull（右删失）

字段名对齐 Minitab：`Shape β`、`Scale α = exp(μ)`、`Threshold λ`。

```text
R(t) = exp(-((t-λ)/α)^β)     t > λ
t_p  = λ + α [-ln(1-p)]^(1/β)
ℓ = Σ_δ=1 [ln β − ln α + (β−1) ln((t−λ)/α) − ((t−λ)/α)^β]
  + Σ_δ=0 [ −((t−λ)/α)^β ]
约束 λ < min(t_i)；AIC 的 k = 3
```

实现：对候选 λ 做剖面，把 `t' = t − λ` 交给现有二参数右删失 MLE。只接受 **β > 1** 的内点。β ≤ 1 时似然常随 λ → min(failure) 无界；Minitab 会固定阈值并做 bias-correction（Lockhart & Stephens 1994，且有未公开改进）。**本轮不实现该修正、不对齐其数值。** 输出诊断 `weibull3_likelihood_unbounded`，不伪造 Shape/Scale/Threshold。

全删失、失效少于 3、非正寿命、λ 越界只诊断。菜单 `model=weibull3`；默认 `weibull` 仍为二参数。比较表仍是二参数 Weibull / Exponential / Lognormal。

数值是公式参考，不是 Minitab Parametric Distribution Analysis 导出。

## 3. Fleiss Kappa（替代 Weighted Kappa）

Minitab Attribute Agreement Analysis **没有** linear/quadratic Weighted Kappa。默认多评估者为 Fleiss；恰好两评估者/两试验为 Cohen。有序评级官方另给 Kendall W/τ（本轮不做）。

未知标准、m 次试验、n 个样品、k 类：

```text
x_ij = 样品 i 被分入类 j 的次数
p_j  = (Σ_i x_ij) / (n m)
P_e  = Σ_j p_j²
P_i  = [Σ_j x_ij(x_ij − 1)] / [m(m − 1)]
P̄    = (Σ_i P_i) / n
κ    = (P̄ − P_e) / (1 − P_e)
```

`P_e = 1` 时 Kappa 不可识别。两两评估者仍用未加权 Cohen。`kappa_weight_scheme != none` 继续 `weighted_kappa_not_implemented`。拒绝 κ=0 不得写成「已证明一致」。

## 4. 图表编辑

工作模型仍是 `ChartModel`。本轮：

- Windows 剪贴板同时放 PNG 与 BMP/`QImage`，预览/PDF 仍走 `ChartRenderer::render_to_pixmap`。
- 可选 `y_min` / `y_max`（缺省自动刻度）与 `data_region_fill`（缺省透明）。
- 不实现拖拽布局、注释、区域拖拽缩放。

## 5. 本轮实现边界

| 模块 | 位置 |
|---|---|
| 三参数 Weibull | `fit_weibull3`（`reliability.cpp`） |
| Fleiss overall | `attribute_agreement.cpp` |
| 图表刻度/填色 | `ChartModel` / `ChartRenderer` / `graph_properties_dialog` |

明确不做：Weighted Kappa、Kendall W/τ、三参数对数正态/两参数指数、Minitab 无界似然 bias-correction 数值对齐、Kalman/TSERIES、图表拖拽注释。公式参考 ≠ Minitab golden。
