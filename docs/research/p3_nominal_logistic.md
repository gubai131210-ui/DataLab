# P3：名义 Logistic（Nominal Logistic）

> 研究日期：2026-08-22 · 访问 2026-08-22（UTC+8）  
> `formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `nominal_logistic` | 基线类别 logit（K−1 条）；系数/SE/Z/P/OR 表；Log-Likelihood；G 检验；`NominalLogisticFacts` |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/nominal-logistic-regression/methods-and-formulas/methods-and-formulas/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/nominal-logistic-regression/interpret-the-results/all-statistics/ | 2026-08-22 |

## Minitab 表形（非 golden）

| 表/统计 | 列 |
|---|---|
| Logistic Regression Table | Predictor · Coef · SE Coef · Z · P · Odds Ratio · 95% CI Lower/Upper；按 Logit k（相对参考事件）分块 |
| Log-Likelihood | 标量 |
| Test of All Slopes Equal to Zero | DF · G · P-Value |
| Goodness-of-Fit（可选） | Pearson / Deviance χ² |

## 估计方法（Wave-2.5 · IRLS）

基线类别（末水平）multinomial logit；参数 \(\beta_{k,j}\) 为水平 \(k\) 相对参考的对数优势。

- **IRLS**：解析 score + 信息矩阵（负 Hessian）；Newton 步 + 线搜索；收敛容差 \(10^{-6}\)。  
- **SE**：信息矩阵逆的对角元；Z 与双侧正态 P；OR = \(\exp(\hat\beta)\)。  
- **G 检验**：相对仅截距零模型；DF = \((K-1)\times p\)。  
- 诊断 `nominal_method` = 「基线类别 multinomial logit IRLS（解析得分/信息矩阵）」。

## 产品边界

- 名义响应 ≥3 水平（无自然顺序）+ ≥1 数值预测；complete-case；保留 `source_row`。  
- 参考事件 = 末水平（可配置标签序）；generalized logit，**非**比例优势。  
- **不做：** 有序 Logistic 冒充名义；HL 诊断全量；逐步选模；Minitab golden。
