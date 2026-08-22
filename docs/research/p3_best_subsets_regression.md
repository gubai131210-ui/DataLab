# P3：Best Subsets 回归

> 研究日期：2026-08-22 · 访问 2026-08-22（UTC+8）  
> Track H 延伸；`formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `best_subsets_regression` | 枚举候选预测子集；每规模保留最佳模型；R² / Adj R² / Mallows Cp / S；`BestSubsetsRegressionFacts` |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/best-subsets-regression/before-you-start/overview/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/best-subsets-regression/methods-and-formulas/methods-and-formulas/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/best-subsets-regression/interpret-the-results/all-statistics/ | 2026-08-22 |

## Minitab 表形参考（非 golden）

- **模型摘要表**：Vars（预测变量个数）+ 各候选列 X 标记 + R-sq + R-sq(adj) + Mallows Cp + S。  
- 默认每规模 1 个最佳模型（最高 R²）；可配置每规模最多 5 个。  
- 含截距；连续响应 + 连续预测。

## 产品边界

- complete-case；`source_row` 保留。  
- 候选预测 ≤15（超出报错，防 2^m 爆炸）；非 Hamiltonian Walk 实现，直接子集枚举 + `fit_linear_regression`。  
- **不做：** 强制项、AICc/BIC/PRESS 扩展表、分类预测、与 Fit Regression Model 联动。  
- 解释层：描述相对拟合证据，禁止「已证明最优模型」类措辞。

## 公式（# source: formula_reference）

- R² = 1 − SSE/SST  
- Adj R² = 1 − [(n−1)/(n−p−1)](1−R²)，p 为预测变量数（不含截距）  
- S = √(MSE)  
- Mallows Cp = SSE_p / s²_full − (n − 2(p+1))，s²_full 为全模型 MSE  
