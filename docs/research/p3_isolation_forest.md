# P3：Isolation Forest（多元异常）

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。与单变量 `outlier_test` **分流**。

## 0. 锁定与禁止偷懒

| 命令 | 交付 |
|---|---|
| `isolation_forest` | 自研 Isolation Forest；异常分数表+图；`IsolationForestFacts` |

**禁止：** 声称 TreeNet/RF；嵌入 sklearn；无分数黑盒；菜单占位。

## 1. 权威来源

| 来源 | URL | 访问 |
|---|---|---|
| sklearn IsolationForest（算法说明，次级） | https://scikit-learn.org/stable/modules/generated/sklearn.ensemble.IsolationForest.html | 2026-08-21 |
| Minitab Predictive 模型类型（边界披露） | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/predictive-analytics/types-of-predictive-analytics-models-in-minitab-statistical-software/ | 2026-08-21 |

## 2. 产品锁定

- ≥2 数值列；complete-case；`source_row`。  
- 参数：`n_trees`（默认 100）、`max_samples`（默认 min(256,n)）、`seed`（可复现）。  
- 路径长度期望用 \(c(n)=2H(n-1)-2(n-1)/n\)；分数 \(s=2^{-E(h)/c}\)。  
- 异常标记：分数 ≥ 经验分位阈值（默认 0.90）或用户 `contamination` 上分位。  
- 输出：分数表、异常标记、分数序列图；诊断写明非单变量 Grubbs/Dixon。  
- **不做：** 扩展隔离森林变体、自动特征选择。

## 3. 接线

`isolation_forest.cpp` → Facts → Service → 命令/解释/序列化/测试/help。
