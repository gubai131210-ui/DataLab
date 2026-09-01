# P10：General MANOVA（窄化）

> 研究日期：2026-08-31 · 访问 2026-08-31（UTC+8）  
> Wave-10 W10-1；`general_manova`；多因子/协变量；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `general_manova` | 2～4 响应；1～2 固定因子；可选 1 协变量；Wilks/Pillai/LH/Roy |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/general-manova/before-you-start/overview/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/general-manova/methods-and-formulas/manova-tests/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/general-manova/before-you-start/data-considerations/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/supporting-topics/basics/understanding-manova/ | 2026-08-31 |

## 表形

- MANOVA Test Table（四检验）
- Group Mean Vectors（按因子组合）
- Eigen analysis
- Univariate ANOVA 附表

## 公式（# source: formula_reference）

\[
\mathbf{H} = \sum_g n_g (\bar{\mathbf{y}}_g - \bar{\mathbf{y}})(\bar{\mathbf{y}}_g - \bar{\mathbf{y}})'
\]

多因子 Type III：按效应分区 \( \mathbf{H}_\text{effect} \)；\( \mathbf{E} \) 为残差 SSCP。

特征值 \( \lambda_i \) of \( \mathbf{E}^{-1}\mathbf{H} \)：

\[
\Lambda = \prod_i \frac{1}{1+\lambda_i}, \quad
\text{Pillai} = \sum_i \frac{\lambda_i}{1+\lambda_i}
\]

## UI 分页

1. 多响应 + 因子（1～2）  
2. 协变量 + 模型（主效应/交互）  
3. 四检验 + SSCP 方法  
4. 预览
