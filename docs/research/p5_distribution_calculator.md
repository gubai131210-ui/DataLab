# P5：分布计算器（PDF/CDF/分位）

> 研究日期：2026-08-23 · 访问 2026-08-23（UTC+8）  
> Wave-5 W5-4；正态/t/χ²/F/Weibull；不改 GOF 数值核。

## 锁定

| 命令 | 交付 |
|---|---|
| `distribution_calculator` | 标量工具：PDF/CDF/quantile；Parameters + Result 表 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://www.itl.nist.gov/div898/handbook/dtoc.htm | 2026-08-23 |
| https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-23 |

## 公式（# source: formula_reference）

- 正态：复用 `normal_pdf` / `standard_normal_cdf` / `standard_normal_quantile`
- t：`student_t_cdf` / `student_t_quantile`
- χ² / F：本地 CDF/分位（不改 GOF TU）
- Weibull：`cdf_weibull3` / `percentile_life_weibull`

## 明确不做

- 改 GOF 数值路径、宣称「分布已正态」、golden
