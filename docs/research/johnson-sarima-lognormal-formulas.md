# Johnson / 非正态能力 / Lognormal / SARIMA 公式笔记

> 研究日期：2026-08-18  
> 访问日期：2026-08-18（UTC+8）  
> 本文只记录官方公式与本轮实现边界。公式参考测试不是 Minitab 导出，不得写成数值对齐。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Johnson 变换后的能力指数 | [Johnson transformed data](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/methods-and-formulas/johnson-transformed-data/) | 2026-08-18 |
| Johnson 族与选模 | Chou, Y., Polansky, A.M., and Mason, R.L. (1998). Transforming nonnormal data to normality in statistical process control. *Journal of Quality Technology*, 30(2), 133–141. Minitab 用 Anderson–Darling 代替原文 Shapiro–Wilk。 | 2026-08-18 |
| 非正态 overall 能力 | [Z-score method](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/supporting-topics/capability-metrics/z-score-method-for-nonnormal-capability/) | 2026-08-18 |
| 右删失参数估计 | [Parametric Distribution Analysis estimation methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/parametric-distribution-analysis-right-censoring/methods-and-formulas/estimation-methods/) | 2026-08-18 |
| 对数正态参数 | [Parameter estimates (right censoring)](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/parametric-distribution-analysis-right-censoring/methods-and-formulas/parameter-estimates/) | 2026-08-18 |
| ARIMA | [ARIMA methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/time-series/how-to/arima/methods-and-formulas/methods-and-formulas/) | 2026-08-18 |

## 2. Johnson 变换

族与定义域：

| 族 | 变换 | 定义域 |
|---|---|---|
| SB | `γ + η ln[(x−ε)/(λ+ε−x)]` | `η,λ>0`，`ε < x < ε+λ` |
| SL | `γ + η ln(x−ε)` | `η>0`，`x > ε` |
| SU | `γ + η sinh⁻¹[(x−ε)/λ]` | `η,λ>0`，x 无界 |

参数估计使用 Chou et al. (1998) 的分位匹配（Slifker–Shapiro 型 `m,n,p`）。DataLab 在候选 `z` 网格上估计、变换后计算 Anderson–Darling p 值，选择 **p 最大且 p > 0.10** 的变换。找不到合格变换时输出 `johnson_transform_not_found`，不填写伪造 Pp。

规格限落在变换定义域内时，对变换后的数据按正态 **overall** 公式计算 Pp/Ppk/PPL/PPU。Johnson 路径不报告 within Cp/Cpk（诊断 `within_not_applicable_after_johnson`）。规格限越出定义域时，能按官方百分位回推则计算，否则 `johnson_spec_outside_support`。

本轮数值是公式参考，不是 Minitab Individual Distribution Identification 导出。

## 3. 非正态能力（Z-score）

对选定分布的 CDF `F`：

```text
Z.LSL = Φ⁻¹(F(LSL))
Z.USL = Φ⁻¹(F(USL))
Pp    = (Z.USL − Z.LSL) / 6     （需双侧规格）
PPL   = −Z.LSL / 3
PPU   = Z.USL / 3
Ppk   = min(PPL, PPU)
期望 PPM < LSL = F(LSL) × 1e6
期望 PPM > USL = (1 − F(USL)) × 1e6
```

本轮分布：Weibull、Lognormal。不报告 Cp/Cpk。不做 Gamma / 三参数分布。

## 4. 两参数对数正态（右删失）

`ln(T) ~ N(μ, σ)`，`σ > 0`。MLE（Newton–Raphson，与 Minitab 口径同类，数值不必相同）：

```text
ℓ = Σ_δ=1 [ −ln σ − ½ ln(2π) − (ln t − μ)² / (2σ²) ]
  + Σ_δ=0 ln(1 − Φ((ln t − μ)/σ))
t_p = exp(μ + σ Φ⁻¹(p))
AIC = 2k − 2ℓ,  k = 2
```

全删失、失效少于 2、非正寿命不识别参数。三参数对数正态本轮不实现。

## 5. ARIMA / SARIMA

Minitab ARIMA 使用迭代最小二乘与 back forecast（Meeker TSERIES / Box–Jenkins），**不是** Kalman 状态空间 MLE。Best ARIMA 的信息准则对数似然见 Brockwell & Davis (1991) §8.6。

DataLab 本轮：

- 非季节：条件最小二乘（CSS）+ 高斯似然近似 AICc。
- 季节：乘法多项式 `φ(B) Φ(B^s) ∇^d ∇_s^D Y_t = θ(B) Θ(B^s) ε_t` 的 CSS，不是 Minitab 无条件最小二乘。

诊断码 `arima_css_approximation` / `sarima_css_approximation`。与 Minitab 最优阶或预测值的差异须等用户按 `EXPORT_GUIDE.md` 覆盖 golden 后再评估。

## 6. 本轮实现边界

| 模块 | 位置 |
|---|---|
| Johnson | `src/domain/statistics/johnson_transform.*` |
| 非正态 Z-score | `ProcessCapability::calculate_johnson` / `calculate_nonnormal` |
| Lognormal | `fit_lognormal` / `percentile_life_lognormal` |
| SARIMA CSS | `fit_best_sarima_candidates` |
| 图表主题 | `ChartRenderer` `theme_colors` |

明确不做：Weighted Kappa、Kendall W/τ、三参数对数正态、Kalman SARIMA、图表拖拽注释。三参数 Weibull / Fleiss 见 [`weibull3-fleiss-chart-formulas.md`](weibull3-fleiss-chart-formulas.md)。公式参考 ≠ Minitab golden。
