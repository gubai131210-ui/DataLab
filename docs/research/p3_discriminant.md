# P3：线性判别分析（LDA）

> 研究日期：2026-08-21 · 访问 2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `discriminant` | 线性判别（等协方差）；分类表；得分；`DiscriminantFacts` |

## 来源

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/supporting-topics/basics/multivariate-analyses-in-minitab/ | 2026-08-21 |

## 产品

- 类别响应 + ≥1 数值预测；≥2 类；complete-case。  
- 类均值、合并协方差、线性得分 \(\delta_c(\mathbf{x})=\mathbf{x}^\top\hat\Sigma^{-1}\boldsymbol\mu_c-\tfrac12\boldsymbol\mu_c^\top\hat\Sigma^{-1}\boldsymbol\mu_c+\log\hat\pi_c\)。  
- 输出：混淆矩阵、训练准确率、类均值表；≥2 预测时 LD1–LD2 投影散点（可选）。  
- **不做：** 二次判别 QDA；逐步判别。
