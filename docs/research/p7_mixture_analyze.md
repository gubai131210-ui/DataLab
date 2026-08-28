# P7：Mixture 分析（Scheffé 线性/二次）

> 研究日期：2026-08-28 · 访问 2026-08-28（UTC+8）  
> Wave-7 W7-1；`mixture_analyze`；独立于 `mixture_design`；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `mixture_analyze` | Scheffé 线性（+可选二次）；无常数项；Coefficients + ANOVA + 残差 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/supporting-topics/mixture-designs/models-terms-and-blending/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/mixtures/analyze-mixture-design/methods-and-formulas/coefficients/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/mixtures/analyze-mixture-design/methods-and-formulas/analysis-of-variance/ | 2026-08-28 |

## 表形

- Coefficients（Coef / SE / T / P）
- ANOVA（Seq SS / Adj SS / DF / MS / F / P）
- Fits and Residuals
- Design Info（q、模型阶）

## 公式（# source: formula_reference）

无截距 Scheffé 模型：

\[
Y = \sum_{i=1}^{q} b_i x_i + \sum_{i<j} b_{ij} x_i x_j
\]

OLS：

\[
\hat{\mathbf{b}} = (\mathbf{X}'\mathbf{X})^{-1}\mathbf{X}'\mathbf{y}
\]

分量约束 \(\sum x_i = 1\)；偏离 1 报诊断，不静默归一化。

## UI 分页

1. 列选择（分量 + 响应）  
2. 模型阶（linear / quadratic）  
3. 方法说明  
4. 预览确认
