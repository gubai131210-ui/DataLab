# P9：单因子 MANOVA（窄化）

> 研究日期：2026-08-31 · 访问 2026-08-31（UTC+8）  
> Wave-9 W9-4；`manova_one_way`；四 multivariate 检验；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `manova_one_way` | 2～4 连续响应；1 分类因子；Wilks / Pillai / LH / Roy；H/E SSCP |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/supporting-topics/basics/understanding-manova/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/general-manova/methods-and-formulas/manova-tests/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/general-manova/interpret-the-results/all-statistics-and-graphs/manova-test-table/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/general-manova/interpret-the-results/all-statistics-and-graphs/eigen-analysis/ | 2026-08-31 |

## 表形

- MANOVA Test Table（Wilks / Pillai / LH / Roy：Value / F / DF / P）
- Group Mean Vectors
- Eigen analysis（\( \mathbf{E}^{-1}\mathbf{H} \) 特征值）
- Univariate ANOVA 附表（每响应一行，可选）

## 公式（# source: formula_reference）

SSCP：

\[
\mathbf{H} = \sum_g n_g (\bar{\mathbf{y}}_g - \bar{\mathbf{y}})(\bar{\mathbf{y}}_g - \bar{\mathbf{y}})'
\]

\[
\mathbf{E} = \sum_g \sum_{j \in g} (\mathbf{y}_{gj} - \bar{\mathbf{y}}_g)(\mathbf{y}_{gj} - \bar{\mathbf{y}}_g)'
\]

特征值 \( \lambda_1 \ge \cdots \ge \lambda_p \) 为 \( \mathbf{E}^{-1}\mathbf{H} \) 的特征值。

\[
\text{Wilks } \Lambda = \prod_i \frac{1}{1+\lambda_i}, \quad
\text{Pillai} = \sum_i \frac{\lambda_i}{1+\lambda_i}, \quad
\text{LH} = \sum_i \lambda_i, \quad
\text{Roy} = \lambda_1
\]

F 近似：\( s=\min(p,q) \)；\( s=1,2 \) 时精确，否则标注 approximate。

## UI 分页

1. 多响应 + 因子  
2. 检验选项（四检验默认全开）  
3. SSCP / MANOVA 方法  
4. 预览
