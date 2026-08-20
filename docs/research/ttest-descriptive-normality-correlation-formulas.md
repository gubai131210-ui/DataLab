# 单/双/配对 t 区间图 / 描述箱线与个体值 / 正态性输出 / 相关矩阵散点

> 研究日期：2026-08-19  
> 访问日期：2026-08-19（UTC+8）  
> 本文只记录官方公式、Minitab 表形/图名与本轮实现边界。`formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| 单样本 t | [Methods and formulas for 1-Sample t](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-sample-t/methods-and-formulas/methods-and-formulas/) | 2026-08-19 |
| 双样本 t | [Methods and formulas for 2-Sample t](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/2-sample-t/methods-and-formulas/methods-and-formulas/) | 2026-08-19 |
| 配对 t | [Methods and formulas for Paired t](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/paired-t/methods-and-formulas/methods-and-formulas/) | 2026-08-19 |
| 区间图 | [All statistics and graphs for Interval Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/interval-plot/how-to/interval-plot/interpret-the-results/all-statistics-and-graphs/) | 2026-08-19 |
| 描述统计图 | [All statistics and graphs for Display Descriptive Statistics](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/display-descriptive-statistics/interpret-the-results/all-statistics-and-graphs/) | 2026-08-19 |
| 箱线图 | [All statistics and graphs for Boxplot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/boxplot/how-to/boxplot/interpret-the-results/all-statistics-and-graphs/) | 2026-08-19 |
| 个体值图 | [All statistics and graphs for Individual Value Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/individual-value-plot/how-to/individual-value-plot/interpret-the-results/all-statistics-and-graphs/) | 2026-08-19 |
| 正态性检验 | [Methods and formulas for Normality Test](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/normality-test/methods-and-formulas/methods-and-formulas/) | 2026-08-19 |
| NIST 正态性 | [NIST 1.3.5.14 Normality Tests](https://www.itl.nist.gov/div898/handbook/eda/section3/eda35a.htm) | 2026-08-19 |
| NIST AD | [NIST 1.3.5.14 Anderson-Darling](https://www.itl.nist.gov/div898/handbook/eda/section3/eda35e.htm) | 2026-08-19 |
| 相关 | [Methods and formulas for Correlation](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/correlation/methods-and-formulas/methods-and-formulas/) | 2026-08-19 |
| 矩阵图 | [All statistics and graphs for Matrix Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/matrix-plot/how-to/matrix-plot/interpret-the-results/all-statistics-and-graphs/) | 2026-08-19 |
| NIST 相关通则 | [NIST 7.2.1 Two-sample t-test for means](https://www.itl.nist.gov/div898/handbook/prc/section2/prc21.htm) | 2026-08-19 |

本轮 **不改** 领域 t / AD / Pearson / Spearman / 箱线五数公式。对照表形与图名，不填写未导出对照数值。不做功效与样本量。

## 2. 单样本 / 双样本 / 配对 t 区间图

对标 Stat > Basic Statistics > 1-Sample t / 2-Sample t / Paired t。检验统计量保持现有实现：

```text
单样本:  t = (ȳ − μ0) / (s/√n)，df = n−1
         现有 CI 中心是差值 δ = ȳ − μ0，不是 ȳ
双样本 Welch:  SE = √(s1²/n1 + s2²/n2)，Welch–Satterthwaite df
双样本 pooled: s_p² = ((n1−1)s1² + (n2−1)s2²)/(n1+n2−2)
               SE = s_p √(1/n1+1/n2)，df = n1+n2−2
配对:  d_i = y1,i − y2,i；t = d̄ / (s_d/√n)
```

| 输出 | 合同 |
|---|---|
| 单样本区间图 | `PlotKind::interval`；中心 **ȳ**；须 = `μ0 + 差值 CI`；双侧两端都有才画；单侧只诊断 |
| 双样本区间图 | 两组均值个体 CI（Welch：`ȳ_i ± t_{n_i−1} s_i/√n_i`；pooled：`ȳ_i ± t_{n1+n2−2} s_p/√n_i`）。**不改**差值 t/df/CI |
| 配对 | complete-case 散点补 `source_rows`；差值均值区间用已有配对 CI |
| Facts | `TTestFacts`；解释不是规格判定 |

导入：单样本/双样本按列独立 `extract_numeric_column`；配对 `align_complete_rows_with_source`。`*` 计入 missing。区间点 `source_rows` 取该组首个源行。

## 3. 描述统计箱线图与个体值图

对标 Display Descriptive Statistics 的 Boxplot / Individual Value Plot。五数与 Tukey 须仍用 `box_plot_summary`（1.5×IQR）。

| 输出 | 合同 |
|---|---|
| 描述表 | 已有 N/N*/Mean/StDev/分位等，不改 |
| 箱线图 | `PlotKind::boxplot`；多变量或 By 分组各一箱 |
| 个体值图 | `PlotKind::scatter`；x=组下标，y=观测，`source_rows` |
| 缺失 | `*`/空/非法不入图，诊断 `missing_values` |

解释只读 `DescriptiveFacts`，不写过程合格。

## 4. 正态性检验输出合同

对标 Stat > Basic Statistics > Normality Test（Anderson-Darling）。A² / A²* / p / `reject|fail_to_reject|not_computed` **不改**。

概率图理论分位（已有）：

```text
p_i = (i − 0.375) / (n + 0.25)     i = 1..n  （实现为 (index+0.625)/(n+0.25)）
x_i = Φ⁻¹(p_i)
```

排序后 `source_rows` 随观测走。

| 输出 | 合同 |
|---|---|
| 检验表 | 变量、N、N*、Mean、StDev、AD、A²*、Alpha、P、判定（拒绝/未拒绝/无法计算） |
| 概率图 | `PlotKind::probability`；悬停原始行 |
| 直方图 | 可选；`values`+`source_rows` 等长，悬停观测行 |
| Facts | `NormalityFacts`；`fail_to_reject` 不得写成已正态 |

## 5. 相关矩阵散点

对标 Correlation + Matrix Plot。Pearson / Spearman 系数、P、Fisher-z CI **不改**。本轮修接线：complete-case 行主序（`aligned[i][j]`），禁止各列独立抽取后按索引 zip。

| 输出 | 合同 |
|---|---|
| 系数矩阵 / 明细表 | 已有表形 |
| 两列散点 | `source_rows` |
| ≥2 列 | `PlotKind::matrix`；单元格内就近点映射原始行 |
| Facts | `CorrelationFacts`；相关≠因果；未拒绝零相关不是已证明无关 |

listwise N 与旧的错位 zip 可能不同；测试锁定 complete-case。不做功效。
