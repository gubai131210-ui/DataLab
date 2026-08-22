# P3：Kaplan–Meier / Log-rank K 组深化

> 研究日期：2026-08-22 · 访问 2026-08-22（UTC+8）  
> Wave-3 W2；配既有 `kaplan_meier` 与两组 `log_rank_test`；`formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `reliability` (model=km + group column) | K≥2 组 Log-rank（df=K−1）；分组 KM 曲线；Minitab 形表 |

## Primary Source

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability/how-to/nonparametric-distribution-analysis-right-censoring/interpret-the-results/all-statistics-and-graphs/survival-plots/ | 2026-08-22 |

## 公式（# source: formula_reference）

在每个 distinct 失效时刻 \(t_j\)：

- \(n_j\)：总风险集；\(d_j\)：总失效数  
- \(n_{ij}\)、\(d_{ij}\)：第 \(i\) 组在 \(t_j\) 的风险集与失效数  
- 期望失效：\(e_{ij} = d_j \cdot n_{ij}/n_j\)

累计 \(U_i = \sum_j (d_{ij} - e_{ij})\)（前 K−1 组），协方差矩阵

\[
V_{ik} = \sum_j \frac{d_j\, n_{ij}\, n_{kj}}{n_j^2\,(n_j-1)} \big(n_j\,\delta_{ik} - n_{kj}\big)
\]

整体检验（df = K−1）：

\[
\chi^2 = U^\top V^{-1} U,\quad P = P(\chi^2_{K-1} \ge \chi^2)
\]

K=2 时退化为既有两组 Mantel–Cox 实现（`log_rank_test` 保持 API 不变）。

## Minitab 表形参考（非 golden）

- **Test for Equality of Survival Dist**：Chi-Square、DF、P  
- **各组**：At Risk (N)、Events、Censored  
- **Survival Plot**：每组一条 KM 阶梯曲线（含 CI）

## 产品边界

- 右删失 + 事件列 0/1（或删失类型列）；分组列文本水平映射为 0…K−1  
- 每组至少 1 条记录；方差矩阵不可逆时报错，不伪造 P 值  
- `ReliabilityFacts.log_rank_*` 供解释层与序列化  
- **不做：** Gehan–Wilcoxon、趋势检验、分层 Cox、左/区间删失 Log-rank
