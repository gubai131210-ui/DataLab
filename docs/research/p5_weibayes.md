# P5：Weibayes（固定形状先验窄化）

> 研究日期：2026-08-23 · 访问 2026-08-23（UTC+8）  
> Wave-5 W5-2；新增 `weibayes`；NIST APR + 表形；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `weibayes` | 固定 β；η=(Σ t_i^β / r)^{1/β}（r≥1）；r=0 诚实边界；B10/B50/B90；右删失主路径 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://www.itl.nist.gov/div898/handbook/apr/section4/apr4.htm | 2026-08-23 |
| https://www.itl.nist.gov/div898/handbook/apr/apr.htm | 2026-08-23 |
| https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-23 |

## 表形

- Parameter Estimates（β prior、η）
- Percentiles（B10/B50/B90）
- Censoring Summary（N、r、右删失、zero-failure bound）

## 公式（# source: formula_reference）

\[
\hat\eta = \left(\frac{\sum_i t_i^{\beta}}{r}\right)^{1/\beta},\quad r\ge 1
\]

分位寿命复用 `percentile_life_weibull`。r=0 不做 η 点估计。

## 明确不做

- 全寿命回归全家桶、宣称「寿命已达标」、Minitab golden
