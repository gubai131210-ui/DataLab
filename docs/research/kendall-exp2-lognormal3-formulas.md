# Kendall W/τ / 两参数指数 / 三参数对数正态 / 数据区框选缩放

> 研究日期：2026-08-18
> 访问日期：2026-08-18（UTC+8）
> 本文只记录官方公式与本轮实现边界。公式参考测试不是 Minitab 导出，不得写成数值对齐。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Kendall 系数 | [Kendall's coefficients methods](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/attribute-agreement-analysis/attribute-agreement-analysis/methods-and-formulas/kendall-s-coefficients/) | 2026-08-18 |
| Kappa 与 Kendall 选用 | [Kappa statistics and Kendall's coefficients](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/supporting-topics/attribute-agreement-analysis/kappa-statistics-and-kendall-s-coefficients/) | 2026-08-18 |
| 阈值参数 | [Distributions with threshold parameters](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/supporting-topics/distribution-models/distributions-with-threshold-parameters/) | 2026-08-18 |
| 参数字段名 | [Parameter estimates (right censoring)](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/parametric-distribution-analysis-right-censoring/methods-and-formulas/parameter-estimates/) | 2026-08-18 |
| 分布 PDF/CDF 字段 | [Individual Distribution Identification distributions](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/individual-distribution-identification/methods-and-formulas/distributions/) | 2026-08-18 |
| 无界似然 | [Estimation methods (right censoring)](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/parametric-distribution-analysis-right-censoring/methods-and-formulas/estimation-methods/) | 2026-08-18 |
| 图形区域 | [Graph regions](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-framework-and-scale/graph-regions/) | 2026-08-18 |
| 注释（本轮不做） | [Annotation shapes, text, and markers](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/labels-lines-and-annotations/annotation-shapes-text-and-markers/) | 2026-08-18 |

## 2. Kendall W 与 Kendall τ_b

Minitab Attribute Agreement Analysis 在**有序**评级（≥3 个等级）时，除 Kappa 外给出 Kendall。名义分类只用 Kappa。官方**没有** linear/quadratic Weighted Kappa。

字段名对齐官方表：W 为 Coef / Chi-Sq / DF / P；与标准的相关为 Coef / SE Coef / Z / P。官方 methods 页主体是图片。本实现用 midrank 和谐系数与 Agresti (1984) τ_b，测试为 `# source: formula_reference`，**不把 Z/P 登记为 Minitab golden**。

无已知标准（评估者间 / 评估者内重复）：

```text
对每个评估者（或每次试验）用 midrank 给 N 个样品排秩
R_i = 样品 i 的秩和
S   = Σ_i (R_i − R̄)²
T_j = Σ_g (t_g³ − t_g)     （第 j 个评估者的结组）
W   = 12 S / [k²(N³ − N) − k Σ_j T_j]
χ²  = k (N − 1) W  ~  χ²_{N−1}
```

有已知标准：每个评估者与标准组成对，τ_b 为

```text
n0 = n++(n++ − 1)/2
TX = 0.5 Σ_i n_i+(n_i+ − 1)
TY = 0.5 Σ_j n_+j(n_+j − 1)
C, D = 协调 / 不协调对数
τ_b = (C − D) / sqrt((n0 − TX)(n0 − TY))
```

全体 vs 标准的 Tc 为各评估者 τ_b 的平均。SE 使用 H0 下标准 Kendall 近似 `sqrt(2(2N+5)/(9N(N−1)))`，供输出列完整；不对齐 Minitab 未公开的精确 SE 图片。

DataLab：`ratings_are_ordinal` 默认 false。为 true 且互异数值等级 ≥ 3 才计算。非数值评级 → `ordinal_ratings_unranked`。拒绝 W=0 或 τ=0 不得写成「已证明有序一致」。

## 3. 两参数指数

字段：Scale θ、Threshold λ。

```text
R(t) = exp(−(t − λ)/θ)     t > λ
t_p  = λ − θ ln(1 − p)
AIC 的 k = 2
```

实现：剖面 λ < min(t)，把 t′ = t − λ 交给现有一参数右删失 MLE（rate = 失效数 / 暴露）。无界或失效不足只诊断 `exponential2_likelihood_unbounded`，不伪造 θ=1。不是 Minitab bias-correction。菜单 `model=exponential2`；默认 `exponential` 仍为一参数。

## 4. 三参数对数正态

字段：Location μ、Scale σ、Threshold λ。

```text
ln(T − λ) ~ N(μ, σ)        t > λ
t_p = λ + exp(μ + σ Φ⁻¹(p))
AIC 的 k = 3
```

实现：同样剖面后复用 `fit_lognormal`。无界只诊断 `lognormal3_likelihood_unbounded`。菜单 `model=lognormal3`；默认 `lognormal` 仍为二参数。分布比较表仍是二参数三列。

## 5. 数据区框选缩放

工作模型仍是 `ChartModel`。可选 `x_min` / `x_max`（与已有 `y_min` / `y_max` 一样缺省自动）。Shift+拖拽把框写入数据刻度；「适合窗口」清除自定义刻度并复位 view zoom。左键框选点不变。

不实现：图元拖拽布局、注释、多图 Layout Tool。

## 6. 本轮实现边界

| 模块 | 位置 |
|---|---|
| Kendall W/τ | `attribute_agreement.cpp` |
| 两参数指数 | `fit_exponential2` |
| 三参数对数正态 | `fit_lognormal3` |
| 数据区 X 刻度 | `ChartModel` / `ChartRenderer` / `graph_properties_dialog` |

明确不做：Weighted Kappa、Minitab 无界似然 bias-correction、Kalman/TSERIES、图表注释与区域拖拽布局。公式参考 ≠ Minitab golden。
