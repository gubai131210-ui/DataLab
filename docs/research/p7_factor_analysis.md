# P7：因子分析（主成分提取窄化）

> 研究日期：2026-08-28 · 访问 2026-08-28（UTC+8）  
> Wave-7 W7-4；`factor_analysis`；与 `pca` Hotelling T² 区分；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `factor_analysis` | 相关阵特征分解；Loadings + % Var；Scree；可选 Varimax |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/factor-analysis/methods-and-formulas/methods-and-formulas/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/supporting-topics/principal-components-and-factor-analysis/differences-between-pca-and-factor-analysis/ | 2026-08-28 |

## 表形

- Factor Loadings
- Variance Explained / % Var
- Communalities
- Scree PlotSpec

## 公式（# source: formula_reference）

相关阵 \(R\) 特征对 \((\lambda_i, \mathbf{e}_i)\)。

未旋转载荷：

\[
L_{:,i} = \sqrt{\lambda_i}\,\mathbf{e}_i
\]

% Var：\(\lambda_i / \sum \lambda_j\)。

Varimax：正交旋转最大化载荷方差。

**不含** Hotelling T²（与 `pca` 命令区分）。

## UI 分页

1. 变量选择  
2. 提取/旋转  
3. 方法说明  
4. 预览
