# P11：Multiple Correspondence Analysis（窄化）

> 研究日期：2026-09-01 · 访问 2026-09-01（UTC+8）  
> Wave-11 W11-2；`multiple_correspondence`；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `multiple_correspondence` | 3～6 列分类原始数据；指示矩阵；Column Contributions |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/multiple-correspondence-analysis/before-you-start/overview/ | 2026-09-01 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/multiple-correspondence-analysis/methods-and-formulas/method/ | 2026-09-01 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/multiple-correspondence-analysis/perform-the-analysis/enter-your-data/ | 2026-09-01 |

## 表形

- Summary of Analysis
- Column Contributions（Qual, Mass, Inert, Coord×k, Contr×k）
- Column Plot

## 公式（# source: formula_reference）

\( m \) 个分类变量，水平数 \( c_1,\ldots,c_m \)。指示矩阵 \( Z \)（\( n \times \sum c_j \)）。

MCA 对 \( Z \) 做加权 PCA，Partition \( \chi^2 \) 惯性；维数上界 \( \sum (c_j - 1) \)。

## UI 分页

1. 选 3～6 分类列  
2. 组件数  
3. 输出表/图选项  
4. 预览
