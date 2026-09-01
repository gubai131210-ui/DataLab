# P10：混合效应 REML（窄化）

> 研究日期：2026-08-31 · 访问 2026-08-31（UTC+8）  
> Wave-10 W10-2；`mixed_effects_reml`；单随机项 REML；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `mixed_effects_reml` | 1 随机因子 + 1～2 固定因子 + 可选协变量；REML 方差分量 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/mixed-effects-model/before-you-start/overview/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/mixed-effects-model/methods-and-formulas/methods/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/mixed-effects-model/methods-and-formulas/variance-components/ | 2026-08-31 |
| https://blog.minitab.com/en/blog/understanding-statistics/see-the-new-features-and-enhancements-in-minitab-18-statistical-software | 2026-08-31 |

## 表形

- Variance Components（来源, VarComp, %Contribution）
- Fixed Effects（Term, Coef, SE, t, P）
- ANOVA Type III（固定项）
- Random BLUP（可选）

## 公式（# source: formula_reference）

\[
\mathbf{y} = \mathbf{X}\boldsymbol{\beta} + \mathbf{Z}\boldsymbol{\mu} + \boldsymbol{\varepsilon}
\]

\[
\mathbf{V} = \sigma^2 \mathbf{I} + \sigma_u^2 \mathbf{Z}\mathbf{Z}'
\]

REML：最大化 restricted log-likelihood（Minitab 默认；Newton + MINQUE 初值）。

## UI 分页

1. 响应 + 随机因子  
2. 固定因子 + 协变量  
3. REML 方法说明  
4. 预览
