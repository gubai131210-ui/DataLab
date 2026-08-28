# P8：三因子 GLM（不平衡窄化）

> 研究日期：2026-08-28 · 访问 2026-08-28（UTC+8）  
> Wave-8 W8-3；`glm_three_factor`；Type III；无 ABC 三阶交互。

## 锁定

| 命令 | 交付 |
|---|---|
| `glm_three_factor` | 三因子不平衡；Type III Adj SS；Fitted Means；AB/AC/BC 可选 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/supporting-topics/anova-models/balanced-and-unbalanced-designs/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/fit-general-linear-model/methods-and-formulas/methods/ | 2026-08-28 |
| https://blog.minitab.com/en/blog/marilyn-wheatleys-blog/anova-data-means-and-fitted-means-balanced-and-unbalanced-designs/ | 2026-08-28 |

## 表形

- ANOVA Type III
- Coefficients
- Fitted Means（A/B/C 边际）
- Residual Diagnostics

## 公式（# source: formula_reference）

\[
Y = X\beta + \varepsilon, \quad \hat\beta = (X'X)^{-1}X'y
\]

Type III：项在其余项已在模型下的 Adj SS。

Fitted Mean（因子水平 \(a\)）：\(\bar{\hat Y}_a = \frac{1}{n_a}\sum_{i:A_i=a}\hat y_i\)

**不做** ABC 三阶交互。

## UI 分页

1. 列选择（三因子 + 响应）  
2. 模型（主效应 / AB·AC·BC）  
3. 方法（不平衡 / 拟合均值）  
4. 预览
