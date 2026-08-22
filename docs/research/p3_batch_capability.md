# P3：Batch Capability（按批次能力）

> 研究日期：2026-08-22 · 访问 2026-08-22（UTC+8）  
> Track B 能力族；`formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `batch_capability` | 按批次列分组；每批独立正态能力（样本 σ）；批次摘要表；`BatchCapabilityFacts` |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/supporting-topics/basics/capability-analyses-in-minitab/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/methods-and-formulas/methods-and-formulas/ | 2026-08-22 |

## Minitab 表形参考（非 golden）

- 按 **Batch / 子组** 分列展示：N、均值、σ、Cp、Cpk、Pp、Ppk（若有规格）。  
- 与 **Between/Within** 分流：本命令为**逐批独立**能力，不做组间/组内分解。

## 产品边界

- complete-case；保留 `source_row`。  
- 批次内 n≥2 才计算；过小批次跳过并诊断。  
- 正态 + 样本标准差；**不做** Automated / 非正态 / Johnson。  
- 解释层：只陈述指标与样本证据，禁止「批次合格」。
