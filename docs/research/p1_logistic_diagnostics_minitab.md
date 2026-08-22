# P1 Logistic 诊断增强（Residual / Leverage / VIF）

- 访问日期：2026-08-20
- 目标：诊断表与解释统一输出 residual、杠杆、VIF

## 公式与定义

- Pearson 残差：`r_i = (y_i-\hat{p}_i) / sqrt(\hat{p}_i(1-\hat{p}_i))`
- Deviance 残差：`sign(y_i-\hat{p}_i) * sqrt(2 * contribution_i)`
- 杠杆值：`h_i = w_i * x_i^T (X^T W X)^{-1} x_i`
- 高杠杆阈值（Minitab 常见）：`min(3p/n, 0.99)`，其中 `p` 为参数数（含截距）
- VIF：`VIF_j = 1 / (1-R_j^2)`（第 j 个预测变量对其余预测变量回归）

## 解释规则

- 高杠杆与高 VIF 作为“诊断信号”，不是自动删点/删变量指令。
- 与 complete-case 行映射一起输出，便于回查原始行。

## §2 关联与分类（批次 A2 · 2026-08-22）

- **配对一致率**：对响应不同的观测对 (i,j)，比较事件顺序与预测概率顺序；报告 Concordant / Discordant / Tied 与 Pairs Concordance (%)。  
- **分类表（阈值 0.5）**：2×2 表（TP/TN/FP/FN）；`formula_reference`，非 Minitab golden。

### Primary URL

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-binary-logistic-model/interpret-the-results/all-statistics-and-graphs/association-statistics/ | 2026-08-22 |

## Minitab 口径

- Fits and Diagnostics 展示 residual 与 leverage（Hi）。
- 建议联合诊断图与模型稳定性对比解释影响点。

## 来源

- https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-binary-logistic-model/interpret-the-results/all-statistics-and-graphs/fits-and-diagnostics/
- https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/supporting-topics/logistic-regression/diagnostics-and-residual-analysis/
