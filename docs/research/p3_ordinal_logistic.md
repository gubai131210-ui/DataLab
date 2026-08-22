# P3：有序 Logistic（比例优势）

> 研究日期：2026-08-21 · 访问 2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `ordinal_logistic` | 比例优势 logit；阈值 θ + 共享 β；系数表；`OrdinalLogisticFacts` |

**禁止：** 名义 Logistic 冒充有序；菜单占位；未写官方 URL。

## 来源

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/ordinal-logistic-regression/methods-and-formulas/methods-and-formulas/ | 2026-08-21 |

## 产品

- 有序响应（≥3 水平，按标签序或数值序）+ ≥1 数值预测；complete-case。  
- \(\mathrm{logit}\,P(Y\le k)=\theta_k+\mathbf{x}^\top\boldsymbol\beta\)（logit 链）。  
- Newton/IRLS 风格 MLE；输出 θ、β、SE/Z/P、对数似然、AIC。  
- **不做：** normit/gompit；逐步；名义 Logistic。
