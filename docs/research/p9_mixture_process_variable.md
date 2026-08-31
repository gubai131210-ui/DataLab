# P9：混合物 + 过程变量分析（Scheffé 窄化）

> 研究日期：2026-08-31 · 访问 2026-08-31（UTC+8）  
> Wave-9 W9-3；`mixture_process_variable`；组分×过程交互；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `mixture_process_variable` | 2～4 组分 + 1 过程变量；Scheffé 线性/二次 + 过程项 + 可选 A×X 交互 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/mixtures/analyze-mixture-design/before-you-start/overview/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/mixtures/analyze-mixture-design/methods-and-formulas/coefficients/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/mixtures/analyze-mixture-design/methods-and-formulas/analysis-of-variance/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/supporting-topics/mixture-designs/models-terms-and-blending/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/mixtures/analyze-mixture-design/before-you-start/example/ | 2026-08-31 |

## 表形

- Coefficients（无截距 Scheffé）
- ANOVA（Adj SS / DF / MS / F / P）
- R² / Adj R²
- Fits / Residuals

## 公式（# source: formula_reference）

约束 \( \sum x_i = 1 \)。线性 Scheffé：

\[
Y = \sum_i b_i x_i + \sum_{i<j} b_{ij} x_i x_j + \gamma X_1 + \sum_i \delta_i x_i X_1
\]

OLS：\( \hat{\mathbf{b}} = (\mathbf{X}'\mathbf{X})^{-1}\mathbf{X}'\mathbf{y} \)（无截距）。

组分和在容差 \(|\sum x_i - 1| \le \tau\)（默认 0.05）。

## UI 分页

1. 组分 + 响应 + 过程变量  
2. 模型（组分阶 / 过程交互开关）  
3. Scheffé 方法说明  
4. 预览
