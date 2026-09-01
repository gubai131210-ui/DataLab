# P10：析因二值响应 Probit/Gompit（窄化）

> 研究日期：2026-08-31 · 访问 2026-08-31（UTC+8）  
> Wave-10 W10-3；`binary_doe_probit`；normit/gompit link；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `binary_doe_probit` | 析因二值 DOE；probit 或 gompit；IRWLS；≠ `binary_response_doe` |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-binary-logistic-model/methods-and-formulas/methods/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-binary-response/before-you-start/overview/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-factorial-design/before-you-start/overview/ | 2026-08-31 |

## 表形

- Coefficients（Term, Coef, SE, z, P）
- ANOVA（Adj SS / DF / Chi-square / P）
- Fits and Diagnostics
- Link-specific 说明（probit 边际效应 vs logit OR）

## 公式（# source: formula_reference）

Probit（normit）：

\[
\Phi^{-1}(\mu_i) = \mathbf{x}_i'\boldsymbol{\beta}
\]

Gompit（complementary log-log）：

\[
\log(-\log(1-\mu_i)) = \mathbf{x}_i'\boldsymbol{\beta}
\]

IRWLS 权重依 link 导数（Minitab Methods 页）。

## UI 分页

1. 因子 + Events/Trials 或 0/1 响应  
2. Link 选择（probit / gompit）  
3. IRWLS 方法  
4. 预览
