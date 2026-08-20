# P1 两比例 Agresti–Coull 差值置信区间

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Agresti–Coull 单比例 | Agresti & Coull (1998)；[NIST e-Handbook 7.2.4.1](https://www.itl.nist.gov/div898/handbook/prc/section2/prc241.htm) | 2026-08-20 |
| 两比例差值 AC 型区间 | 各比例先 AC 调整再对 \(\tilde p_1-\tilde p_2\) 做 Wald 型区间（教材/实务常用） | 2026-08-20 |
| Minitab 2 Proportions 表形 | [2 Proportions methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/2-proportions/methods-and-formulas/methods-and-formulas/)（只对照表形） | 2026-08-20 |
| DataLab Newcombe / 单比例 AC | [`p1_two_proportion_newcombe_wilson.md`](p1_two_proportion_newcombe_wilson.md)、[`p1_agresti_coull_proportion_ci.md`](p1_agresti_coull_proportion_ci.md) | 2026-08-20 |

## 2. 产品选型

在 `two_proportions` 扩展 `method`：

| method | 检验 Z/P | CI | `ci_method` |
|---|---|---|---|
| `normal`（默认） | unpooled Wald | Wald | `wald` |
| `wilson` | **同一** unpooled Wald | Newcombe–Wilson | `newcombe_wilson` |
| `agresti_coull` | **同一** unpooled Wald | AC 差值 | `agresti_coull_diff` |

不改 Fisher；不做 Blaker。

## 3. 公式

设 \(z=z_{1-\alpha/2}\)（双侧；单侧用对应临界值）。对组 \(i=1,2\)：

\[
\tilde n_i=n_i+z^2,\quad
\tilde x_i=x_i+\tfrac{z^2}{2},\quad
\tilde p_i=\tilde x_i/\tilde n_i
\]

\[
\mathrm{CI}=\bigl(\tilde p_1-\tilde p_2\bigr)\pm
z\sqrt{\dfrac{\tilde p_1(1-\tilde p_1)}{\tilde n_1}+\dfrac{\tilde p_2(1-\tilde p_2)}{\tilde n_2}}
\]

检验仍：

\[
\mathrm{SE}_\mathrm{sep}=\sqrt{\hat p_1(1-\hat p_1)/n_1+\hat p_2(1-\hat p_2)/n_2},\quad
Z=(\hat p_1-\hat p_2)/\mathrm{SE}_\mathrm{sep}
\]

## 4. 表形

现有描述 / 检验 / 差值区间图；Facts：`method=agresti_coull`，`ci_method=agresti_coull_diff`。

## 5. 明确不做

Blaker；Wilson CC；改默认 Wald 数值合同；单比例路径重做。

## 6. 测试

同计数下 `normal`/`wilson` 回归；`agresti_coull` 时 Z 不变、CI 按上式；`# source: formula_reference`。
