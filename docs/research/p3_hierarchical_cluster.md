# P3：层次聚类（观测，Complete linkage）

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。

## 0. 锁定与禁止偷懒

| 命令 | 交付 |
|---|---|
| `cluster_observations` | 凝聚层次；**complete** 连锁；合并历程表；切 k 簇分配；`HierarchicalClusterFacts` |

**禁止：** 只出距离矩阵无合并；菜单占位；把 K-Means 冒充层次。

## 1. 权威来源

| 来源 | URL | 访问 |
|---|---|---|
| Minitab：多元分析总览（Cluster Observations） | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/supporting-topics/basics/multivariate-analyses-in-minitab/ | 2026-08-21 |
| Minitab Feature List | https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-21 |

## 2. 产品锁定

- ≥2 数值列；complete-case；可选标准化。  
- 距离：欧氏；连锁：**complete**（簇间最大两两距离）。  
- 输出：合并步骤表（step、merged_a、merged_b、height、new_id）；按用户 `k` 切簇的分配表；前两列散点着色。  
- **不做：** single/average/Ward 本轮不实现（诊断可提示）；Cluster Variables；树形 PlotKind 可后续。

## 3. 接线

`hierarchical_cluster.cpp` → Facts → Service → 命令/解释/序列化/测试/help。
