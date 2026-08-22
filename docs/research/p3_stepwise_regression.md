# P3：Stepwise 线性回归

> 研究日期：2026-08-21 · 访问 2026-08-22（UTC+8）  
> Track H；`formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `stepwise_regression` | α 逐步 + **Forward AICc/BIC**；步骤表含 R²/AICc/BIC；终模型系数；`StepwiseRegressionFacts` |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-regression-model/methods-and-formulas/stepwise/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-regression-model/perform-the-analysis/perform-stepwise-regression/ | 2026-08-22 |

## Minitab 表形（深化 · 非 golden）

| 表 | 列 |
|---|---|
| 逐步步骤详情 | Step · Action · Term · R² · Adj R² · SSE · **AICc** · **BIC**（信息准则路径） |
| 终模型系数 | Term · Coef · SE · T · P · CI |

## 产品

- 数值响应 + ≥2 候选预测；complete-case。  
- 方法：`stepwise` / `forward` / `backward`（α）；`forward_aicc` / `forward_bic`（信息准则）。  
- Forward 信息准则：每步加入 p 最小项，记录各步 AICc/BIC，终模型 = 准则最小步。  
- **不做：** Best subsets；k-fold 验证；Minitab golden。
