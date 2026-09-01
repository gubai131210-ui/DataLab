# P11：Simple Correspondence Analysis（窄化）

> 研究日期：2026-09-01 · 访问 2026-09-01（UTC+8）  
> Wave-11 W11-1；`simple_correspondence`；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `simple_correspondence` | 2 列分类 → 列联表；1～2 维；行/列贡献 + 双标图 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/simple-correspondence-analysis/before-you-start/overview/ | 2026-09-01 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/simple-correspondence-analysis/methods-and-formulas/method/ | 2026-09-01 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/simple-correspondence-analysis/before-you-start/example/ | 2026-09-01 |

## 表形

- Summary of Analysis（惯性、χ²、维数）
- Row Contributions（Qual, Mass, Inert, Coord, Contr）
- Column Contributions（同上）
- Row Plot / Column Plot（窄化 2D）

## 公式（# source: formula_reference）

列联表 \( N = (n_{ij}) \)，行/列边际 \( r_i, c_j \)，总 \( n \)。

惯性：\( I = \chi^2 / n \)，其中 \( \chi^2 = \sum_{ij} \frac{(n_{ij} - e_{ij})^2}{e_{ij}} \)，\( e_{ij} = r_i c_j / n \)。

标准化矩阵 \( S = D_r^{-1/2} (N/n - r c'/n) D_c^{-1/2} \) 的 SVD 给出主坐标。

## UI 分页

1. 行变量 + 列变量  
2. 组件数 + 图选项  
3. 输出表选项  
4. 预览
