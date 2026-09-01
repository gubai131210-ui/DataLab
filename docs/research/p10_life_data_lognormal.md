# P10：寿命数据 Lognormal 回归（窄化）

> 研究日期：2026-08-31 · 访问 2026-08-31（UTC+8）  
> Wave-10 W10-4；`life_data_lognormal`；MLE + 删失；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `life_data_lognormal` | Lognormal 分布；1～2 协变量；右删失/精确；≠ `life_data_regression` |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/regression-with-life-data/before-you-start/overview/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/regression-with-life-data/methods-and-formulas/equations/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/regression-with-life-data/interpret-the-results/regression-table/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/regression-with-life-data/perform-the-analysis/select-the-analysis-options/ | 2026-08-31 |

## 表形

- Regression Table（log scale Coef/SE）
- Distribution Summary（μ, σ on log scale）
- Percentiles（可选百分位）
- Fits / Residuals（含 source_row）

## 公式（# source: formula_reference）

Lognormal：\( Y_p = \log(T_p) \)

\[
\log(Y_p) = \beta_0 + \sum_k \beta_k x_k + \sigma \Phi^{-1}(p)
\]

MLE with right censoring（与 W8 Weibull 删失契约一致）。

## UI 分页

1. 时间 + 删失 + 协变量  
2. Lognormal 分布选项  
3. MLE 方法  
4. 预览
