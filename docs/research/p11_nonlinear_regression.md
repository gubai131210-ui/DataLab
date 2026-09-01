# P11：Nonlinear Regression（窄化）

> 研究日期：2026-09-01 · 访问 2026-09-01（UTC+8）  
> Wave-11 W11-3；`nonlinear_regression`；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `nonlinear_regression` | 单 Y + 单 X；内置模型；GN/LM；Parameter + Summary of Fit |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/nonlinear-regression/before-you-start/overview/ | 2026-09-01 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/nonlinear-regression/methods-and-formulas/methods/ | 2026-09-01 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/nonlinear-regression/perform-the-analysis/select-the-results-to-display/ | 2026-09-01 |

## 内置模型（窄化）

| id | 形式 |
|---|---|
| `growth` | \( y = a - b e^{-cx} \) |
| `decay` | \( y = a e^{-bx} \) |
| `logistic_saturation` | \( y = a / (1 + e^{-b(x-c)}) \) |
| `michaelis_menten` | \( y = V x / (K + x) \) |
| `power` | \( y = a x^b \) |

## 表形

- Method（算法、迭代、收敛）
- Starting Values
- Parameter Estimates（Estimate, SE, CI）
- Summary of Fit（SSE, DF, MSE, S）

## 公式（# source: formula_reference）

GN：\( \delta = (J'J)^{-1} J' r \)，\( \theta_{k+1} = \theta_k + \delta \)。

LM：\( (J'J + \lambda D)\delta = J'r \)。

相对 offset 收敛：\( \max_i | \delta_i / \theta_i | < \text{tol} \)。

## UI 分页

1. 响应 + 预测列  
2. 模型选择 + 初值表  
3. 算法（GN/LM）+ 迭代/容差  
4. 预览
