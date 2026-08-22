# P3：Poisson 回归（缺陷计数）

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。

## 0. 锁定与禁止偷懒

| 命令 | 交付 |
|---|---|
| `poisson_regression` | log 链 Poisson GLM（IRLS）；系数/SE/Z/P；偏差/AIC；Pearson 残差；`PoissonRegressionFacts` |

**禁止：** 菜单占位；未写官方 URL；把负二项/零膨胀冒充本轮；解释禁用子串。

## 1. 权威来源

| 来源 | URL | 访问 |
|---|---|---|
| Minitab：Fit Poisson Model methods | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-poisson-model/methods-and-formulas/methods/ | 2026-08-21 |
| Minitab：estimated equation | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-poisson-model/methods-and-formulas/estimated-equation/ | 2026-08-21 |

## 2. 产品锁定

- 响应：非负整数计数（允许 0）；预测：≥1 数值列；complete-case。  
- 默认 **natural log** 链：\(\log\mu_i=\mathbf{x}_i^\top\boldsymbol\beta\)（含截距）。  
- IRLS：权重 \(w_i=\mu_i\)，工作响应 \(z_i=\eta_i+(y_i-\mu_i)/\mu_i\)（\(\mu\) 下限 \(10^{-12}\)）。  
- 偏差：\(D=2\sum[y\log(y/\mu)-(y-\mu)]\)（\(y=0\) 时 \(y\log\) 项取 0）。  
- 输出：系数表、模型摘要（N、迭代、收敛、LL、D、AIC）、拟合/Pearson 残差表；可选残差散点。  
- **不做：** 负二项、零膨胀、逐步、偏移 exposure 列（本轮可诊断提示未实现）。

## 3. 接线

`poisson_regression.cpp` → Facts → `AnalysisService::poisson_regression` → 命令/解释/序列化/`p3_batch2_*_test` → help → 文档。
