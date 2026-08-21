# P1 Mood 中位数检验

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。对照 Minitab 表形（N≤ / N>、χ²），不填未导出数值。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 概述 | [Overview for Mood’s Median Test](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/mood-s-median-test/before-you-start/overview/) | 2026-08-21 |
| Minitab 计算方法 | [Calculation method](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/mood-s-median-test/methods-and-formulas/calculation-method/) | 2026-08-21 |
| Minitab 统计量释义 | [Interpret all statistics](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/mood-s-median-test/interpret-the-results/all-statistics/) | 2026-08-21 |
| NIST / 非参数入口 | [NIST e-Handbook 7.4](https://www.itl.nist.gov/div898/handbook/prd/section4/prd4.htm) | 2026-08-21 |

## 2. 产品选型

- **新命令** `mood_median`（独立于 `kruskal_wallis`）
- 输入：测量列 + 分组列；complete-case；`source_row`
- 仅纳入观测数 ≥2 的因子水平（对齐 Minitab）
- Facts：`NonparametricFacts.method = "mood_median"`
- 图：箱线 + 个体值（复用 KW helper）

解释禁止「已证明各组中位数相同」。

## 3. 公式（锁定）

1. 对所有 complete-case 观测求总体中位数 \(M\)（偶数个取两中位平均）。
2. 对每个保留组 \(j\) 计数：
   - \(N_{\le,j}\)：\(y \le M\)
   - \(N_{>,j}\)：\(y > M\)
   （等于 \(M\) 计入 \(N_\le\)，对齐 Minitab；**不做**「结稀少时把结并入 Above」的可选分支。）
3. 对 \(2\times k\) 表做 Pearson \(\chi^2\)：

\[
\chi^2=\sum_{i,j}\frac{(O_{ij}-E_{ij})^2}{E_{ij}},\quad
E_{ij}=\frac{R_i\,C_j}{N},\quad
\mathrm{DF}=k-1
\]

4. \(p=P(\chi^2_{\mathrm{DF}}\ge\chi^2)\)。任一对 \(E_{ij}<5\) 时诊断 `expected_count_warning`。

期望分母为 0 或 \(k<2\) → 诊断、不出 P。

## 4. 表形

| 表 | 合同 |
|---|---|
| 各组摘要 | 水平、N、组中位数、N≤、N> |
| Mood 中位数检验 | 总体中位数 \(M\)、χ²、DF、P |
| 诊断 | 小组剔除、小期望、缺失 |

本轮**不做**各组 Sign CI（Minitab 可有；延后）。

## 5. 明确不做

Kruskal Dunn/SD；Friedman/Nemenyi；Mood 后比较；假 Minitab golden；改 chi_square 命令。

## 6. 测试

对称同中位数组：P 大；明显分离中位数：χ² 与手算一致；单水平/全结诊断；`# source: formula_reference`。
