# P1 DOE 等值线 / 曲面：因子切换与 hold

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab Contour Plot | [Create Contour Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/contour-plot/create-a-contour-plot/) | 2026-08-20 |
| Minitab Surface Plot | [Create Surface Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/surface-plot/create-a-surface-plot/) | 2026-08-20 |
| DataLab 现有网格 | `evaluate_coded_grid`；[`binomial-poisson-doe-anova-chart-formulas.md`](binomial-poisson-doe-anova-chart-formulas.md) | 2026-08-19 |

## 2. 合同

- 在编码空间 [−1,1] 上对已拟合的主效应+双因子交互模型求值。
- 用户选择两个不同因子作为 X/Y 轴；其余因子 **编码 hold = 0**。
- 缺省：前两个因子（与现网完全一致）。
- 非法/相同因子 → `invalid_contour_factors`，不出等值线/曲面。
- **不做**可旋转 3D；**不做**实际单位 hold 编辑器。
- 不改 Pareto / 立方图 / 残差 4 图 / 优化器 D。

## 3. 配置

`DoeConfiguration::contour_x_factor` / `contour_y_factor`：因子名字符串，空=默认前两因子。

## 4. Facts

`DoeFacts`：`contour_x_factor`、`contour_y_factor`、`held_factor_names`、`contour_plot_available`。
