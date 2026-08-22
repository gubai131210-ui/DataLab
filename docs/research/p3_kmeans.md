# P3：Cluster K-Means

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX.md`。

## 0. 本轮锁定与禁止偷懒

**做（竖切闭环）：**

| 命令 | 交付 |
|---|---|
| `kmeans` | 欧氏距离 K-Means；质心迭代；分配表 + 质心表 + 簇内平方和；可选标准化；`KMeansFacts` |

**禁止偷懒：**

- 禁止菜单占位无主计算  
- 禁止未写官方 URL  
- 禁止声称与 Minitab 数值逐点对齐（本仓 formula_reference）  
- 禁止一次塞满层次聚类 / Cluster Variables  
- 禁止解释写禁用子串  

---

## 1. 权威来源

| 来源 | URL | 访问 |
|---|---|---|
| Minitab：Cluster K-Means methods | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/cluster-k-means/methods-and-formulas/cluster-k-means/ | 2026-08-21 |
| Minitab：多元分析总览 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/supporting-topics/basics/multivariate-analyses-in-minitab/ | 2026-08-21 |

## 2. 产品锁定

- 命令 `kmeans`；≥2 数值列；complete-case；保留 `source_row`。  
- 距离：欧氏 \(d(x,c)=\sqrt{\sum_j(x_j-c_j)^2}\)。  
- 迭代（Lloyd）：整轮将每观测分配到最近质心后，再按簇成员均值更新全部质心；直到无再移动或达 `max_iterations`。  
- 初始质心：默认取前 `k` 个有效观测（诊断标明）；可选用户提供 `k`。  
- 可选列标准化（减均值除样本标准差）；常数列诊断并跳过。  
- 输出：簇分配表（原始行、簇号、到质心距离）；质心表；簇大小与簇内 SS；可选 PC1–PC2 散点着色（若变量≥2，用前两列作图轴，**不是** PCA）。  
- `KMeansFacts`：`k`、`n`、`variable_count`、`iterations`、`converged`、`total_within_ss`、`standardized`。  
- **不做：** 层次聚类、Cluster Variables、轮廓系数全套、自动选 k。

## 3. 接线

research → `kmeans.cpp` → `KMeansFacts` → `AnalysisService::kmeans` → `analysis_commands` → interpretation → serialization → `p3_batch1_kmeans_cart_adf_test` → help → backlog/roadmap/wiring。
