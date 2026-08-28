# P8：析因二值响应 DOE（Logit 窄化）

> 研究日期：2026-08-28 · 访问 2026-08-28（UTC+8）  
> Wave-8 W8-1；`binary_response_doe`；Logit IRWLS；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `binary_response_doe` | 2～4 因子；events/trials 或 0/1；Logit IRWLS；Coef + OR + 拟合 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-binary-response/methods-and-formulas/methods/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-binary-response/methods-and-formulas/estimated-equation/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-binary-response/before-you-start/data-considerations/ | 2026-08-28 |

## 表形

- Method（Link = logit）
- Coefficients（Coef / SE / Z / P）
- Odds Ratios
- Goodness-of-Fit（Deviance / AIC）

## 公式（# source: formula_reference）

\[
\eta_i = X_i\beta, \quad p_i = \text{logit}^{-1}(\eta_i) = \frac{e^{\eta_i}}{1+e^{\eta_i}}
\]

IRWLS：\(\hat\beta\) 迭代至偏差变化 \(< 10^{-8}\)。

Odds Ratio：\(\text{OR}_j = \exp(\beta_j)\)。

events/trials：行权重展开或等价二项似然；须验证 \(0 \le \text{events} \le \text{trials}\)。

## UI 分页

1. 因子 + 响应布局（events/trials 或 0/1）  
2. 模型（主效应 / AB 交互）  
3. 方法（logit / IRWLS）  
4. 预览
