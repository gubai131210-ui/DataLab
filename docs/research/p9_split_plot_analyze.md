# P9：裂区析因分析（Split-plot 窄化）

> 研究日期：2026-08-31 · 访问 2026-08-31（UTC+8）  
> Wave-9 W9-2；`split_plot_analyze`；WP/SP 双误差；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `split_plot_analyze` | 1 难改因子 + 1～2 易改因子；WP/SP 双误差 ANOVA；whole-plot residuals |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-factorial-design/methods-and-formulas/analysis-of-variance/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-factorial-design/methods-and-formulas/model-information/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-factorial-design/methods-and-formulas/fits-and-residuals/ | 2026-08-31 |
| https://blog.minitab.com/en/blog/applying-statistics-in-quality-projects/interpreting-results-from-a-split-plot-design | 2026-08-31 |

## 表形

- ANOVA（含 WP Error / SP Error 行）
- Effects / Coefficients
- Fits / Residuals / Whole plot residuals
- 可选 WP R² 与 SP R²

## 公式（# source: formula_reference）

平衡裂区：

\[
F_\text{HTC} = \frac{\text{MS}_\text{term}}{\text{MS}_\text{WP Error}}, \quad
F_\text{ETC} = \frac{\text{MS}_\text{term}}{\text{MS}_\text{SP Error}}
\]

WP 指示矩阵 \(Z\)（n×w）；子区数 \(m\)。

Whole plot residual：

\[
\text{WP Resid}_i = \hat{y}_i^\text{full} - \hat{y}_i^\text{fixed only}
\]

不平衡（窄化）：期望均方线性组合构造 F 分母（与 W8 Type III 邻域一致，分 WP/SP 层）。

## UI 分页

1. 响应 + 难改 / 易改因子 + WP 列  
2. 模型（主效应 / 交互）  
3. 双误差项方法说明  
4. 预览
