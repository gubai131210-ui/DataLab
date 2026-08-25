# P5：随机森林（Bagging CART 窄化）

> 研究日期：2026-08-23 · 访问 2026-08-23（UTC+8）  
> Wave-5 W5-1；新增 `random_forest`；`formula_reference ≠ golden`；**披露非 TreeNet / Minitab RF 对齐**。

## 锁定

| 命令 | 交付 |
|---|---|
| `random_forest` | Bagging CART：bootstrap 上调用 `fit_cart_tree`；默认分类；n_trees≈50；MDI 重要性；多数表决/均值；可选 OOB |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/predictive-analytics/how-to/random-forests-regression/methods-and-formulas/methods/ | 2026-08-23 |
| https://scikit-learn.org/stable/modules/generated/sklearn.ensemble.RandomForestClassifier.html | 2026-08-23 |

## Minitab 表形参考（非 golden）

### Model Summary

| 列 | 含义 |
|---|---|
| N / Trees / Train metric / OOB | 样本量、树数、训练指标、可选 OOB |

### Variable Importance

| 列 | 含义 |
|---|---|
| Variable / Mean Impurity Decrease | 平均不纯度下降 |

### Confusion Matrix（分类）

| 列 | 含义 |
|---|---|
| Actual \\ Predicted | 训练集混淆计数 |

## DataLab 交付范围

- domain：`random_forest.{h,cpp}`；Facts / Configuration / Service / commands / interp / help
- 行循环同 `cart_tree`：仅 `excluded_rows`；complete-case；保留 `source_rows`
- 披露：非 TreeNet / Minitab RF 对齐

## 公式（# source: formula_reference）

Bootstrap 样本上拟合 CART；重要性 = 各树不纯度下降均值；分类多数表决，回归均值。

## 明确不做

- TreeNet® / AutoML / 嵌 Python·sklearn 运行时 / Minitab golden
