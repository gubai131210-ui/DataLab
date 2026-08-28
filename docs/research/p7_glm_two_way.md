# P7：双因子 GLM（不平衡窄化）

> 研究日期：2026-08-28 · 访问 2026-08-28（UTC+8）  
> Wave-7 W7-2；`glm_two_way`；Type III Adj SS；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `glm_two_way` | 不平衡双因子；主效应 + 交互；Type III；Fitted Means（LS 风格） |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/supporting-topics/anova-models/balanced-and-unbalanced-designs/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/fit-general-linear-model/methods-and-formulas/methods/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/fit-general-linear-model/interpret-the-results/all-statistics-and-graphs/analysis-of-variance-table/ | 2026-08-28 |

## 表形

- ANOVA（Adj SS / DF / MS / F / P）
- Coefficients
- Fitted Means（按因子水平平均回归预测）
- 残差诊断

## 公式（# source: formula_reference）

\[
Y = \mathbf{X}\boldsymbol{\beta} + \boldsymbol{\varepsilon}, \quad
\hat{\boldsymbol{\beta}} = (\mathbf{X}'\mathbf{X})^{-1}\mathbf{X}'\mathbf{y}
\]

Type III：项在「其余项已在模型」下的 Adj SS。

Fitted Mean（因子 A 水平 \(a\)）：

\[
\bar{\hat{Y}}_a = \frac{1}{n_a}\sum_{i: A_i=a} \hat{y}_i
\]

## UI 分页

1. 列选择  
2. 模型（主效应/交互）  
3. 方法  
4. 预览
