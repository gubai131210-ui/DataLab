# P8：寿命数据回归（Weibull + 协变量窄化）

> 研究日期：2026-08-28 · 访问 2026-08-28（UTC+8）  
> Wave-8 W8-4；`life_data_regression`；右删失 MLE；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `life_data_regression` | Weibull MLE；1～2 协变量；右删失；回归表 + 百分位 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/regression-with-life-data/methods-and-formulas/equations/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/regression-with-life-data/interpret-the-results/regression-table/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/regression-with-life-data/perform-the-analysis/select-the-analysis-options/ | 2026-08-28 |

## 表形

- Regression Table（Coef / SE / Z / P）
- Distribution / Shape
- Percentile Table（1%、5% 可选）
- Observation Summary

## 公式（# source: formula_reference）

Weibull 回归：

\[
\log Y_p = \beta_0 + \sum_k \beta_k x_k + \sigma \Phi^{-1}(p)
\]

其中 \(\sigma = 1/\text{shape}\)，\(\Phi\) 为标准极值分位函数。

右删失似然：失败项 \(\log f(t)\)，删失项 \(-F(t)\)；Newton-Raphson MLE。

## UI 分页

1. 时间 + 删失 + 协变量  
2. 分布 / 百分位选项  
3. 方法说明  
4. 预览
