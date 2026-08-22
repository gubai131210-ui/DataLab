# P3：Correlogram（相关矩阵热图）

> 研究日期：2026-08-21 · 访问 2026-08-21（UTC+8）  
> Track G2；与 `correlation_plot` 分流为**独立相关热图命令**。

## 锁定

| 命令 | 交付 |
|---|---|
| `correlogram` | 多列相关矩阵表 + 热图；`CorrelogramFacts` |

## 来源

| URL | 访问 |
|---|---|
| https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-21 |

## 产品

- ≥2 数值列；Pearson/Spearman；复用 `correlation_matrix`。  
- 输出：系数矩阵表；`PlotKind::heatmap`；可选显示 p 值表。  
- **不做：** Graph Builder；可旋转 3D。
