# P8：变量聚类（层次法窄化）

> 研究日期：2026-08-28 · 访问 2026-08-28（UTC+8）  
> Wave-8 W8-2；`cluster_variables`；相关距离；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `cluster_variables` | Pearson 相关距离；层次连结；dendrogram + amalgamation |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/cluster-variables/methods-and-formulas/distance-measures/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/cluster-variables/methods-and-formulas/linkage-methods/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/cluster-variables/methods-and-formulas/similarity/ | 2026-08-28 |

## 表形

- Amalgamation Steps（Step / Left / Right / Height / Similarity）
- Distance Matrix（可选摘要）
- Dendrogram（PlotSpec）

## 公式（# source: formula_reference）

Pearson：\(\rho_{ij} = \frac{\sum (x_i-\bar x_i)(x_j-\bar x_j)}{\sqrt{\sum(x_i-\bar x_i)^2\sum(x_j-\bar x_j)^2}}\)

距离：\(d_{ij} = 1 - |\rho_{ij}|\)

相似度：\(s_{ij} = 100\left(1 - \frac{d_{ij}}{d_{\max}}\right)\)

变量数 \(p\) 时合并步数 = \(p-1\)。

## UI 分页

1. 变量选择  
2. 距离 / 连结  
3. 方法说明  
4. 预览
