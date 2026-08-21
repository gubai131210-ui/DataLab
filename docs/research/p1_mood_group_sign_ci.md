# P1 Mood 各组中位数 Sign CI

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。加深 `mood_median`；**不改** Mood Pearson χ² 本体。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Mood 主检验（已实现） | [`p1_mood_median_test.md`](p1_mood_median_test.md) | 2026-08-21 |
| Sign 中位数 CI | [`p1_sign_confidence_interval.md`](p1_sign_confidence_interval.md)；[Minitab 1-Sample Sign](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/1-sample-sign/methods-and-formulas/methods-and-formulas/) | 2026-08-21 |
| Minitab Mood 概述 | [Mood’s Median Test](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/moods-median-test/before-you-start/overview/)（表形对照；不填未导出数） | 2026-08-21 |

## 2. 产品选型

- 加深 `mood_median_test(..., confidence_level)`：对**每个保留组**调用同一 `sign_median_ci`
- 组汇总表增加置信下限 / 上限 / 达到水平（可选列）
- Mood 总体中位数 M、N≤/N>、χ²、P **完全不变**
- 命令加 `confidence`（默认 95%）
- 组 N 过小无法出 CI → 该组 CI 空 + 诊断；不丢组（组保留规则仍 N≥2）

解释禁止「已证明各组中位数不同/相同」。

## 3. 公式

与 [`p1_sign_confidence_interval.md`](p1_sign_confidence_interval.md) 相同；输入为该组有限观测。不跨组池化。

## 4. 表形

| 表 | 合同 |
|---|---|
| 各组汇总 | 原列 + CI 下限 / CI 上限（无可算时 `*`） |
| Mood 中位数检验 | χ² 表不变 |
| 图 | 箱线 / 个体值不变 |

## 5. 明确不做

Mood 后比较；改 χ² / 结计入 N≤；假 golden；独立新命令。

## 6. 测试

χ²/P 与加深前一致；各组 CI 与对该组单独 `sign_median_ci` 一致；`# source: formula_reference`。
