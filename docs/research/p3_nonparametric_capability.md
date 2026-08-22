# P3：Nonparametric Capability（非参数过程能力）

> 研究日期：2026-08-22 · 访问 2026-08-22（UTC+8）  
> `formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `nonparametric_capability` | Cnp / Cnpl / Cnpu / Cnpk；经验分位数；能力直方图；`NonparametricCapabilityFacts` |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/nonparametric-capability-analysis/methods-and-formulas/overall-capability/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/nonparametric-capability-analysis/interpret-the-results/key-results/ | 2026-08-22 |

## Minitab 表形（非 golden）

| 统计 | 含义 |
|---|---|
| Cnp | (USL−LSL) / (Xpu−Xpl)；仅 spread |
| Cnpl | (η−LSL) / (η−Xpl) |
| Cnpu | (USL−η) / (Xpu−η) |
| Cnpk | min{Cnpl, Cnpu} |
| η | 过程中位数 |
| Xpl / Xpu | 容差 K×σ 对应经验分位数（默认 K=6 → 0.135% / 99.865%） |

## 产品边界

- 双侧规格 LSL+USL；N≥10；complete-case。  
- 容差 K 默认 6（可配置）；经验分位数线性插值（Minitab 公式）。  
- **不做：** Automated capability；正态/Johnson 路径；「批次合格」结论。
