# P1 配对 Wilcoxon Walsh / HL 置信区间

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。加深配对 `wilcoxon_signed_rank`；**不改**单样本路径与秩/P。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| 单样本 Walsh/CI（已实现） | [`p1_wilcoxon_one_sample.md`](p1_wilcoxon_one_sample.md) | 2026-08-21 |
| Minitab 1-Sample Wilcoxon | [Methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/1-sample-wilcoxon/methods-and-formulas/methods-and-formulas/) | 2026-08-21 |
| 配对差分模型 | 标准教材：对 \(d_i=x_i-y_i\) 做符号秩与 Walsh | 2026-08-21 |

## 2. 产品选型

- `wilcoxon_signed_rank(first, second, alternative, confidence_level)`：对差分启用 `compute_location=true`
- 复用现有 Walsh 中位数 + 正态近似 CI 核（与单样本相同）
- 服务层配对页增加「位置估计（Walsh）」表；命令已有 `confidence` 配对亦消费
- Facts：配对 `method=wilcoxon_signed_rank` 时填 `location_estimate` / `ci_*`

## 3. 公式

与 [`p1_wilcoxon_one_sample.md`](p1_wilcoxon_one_sample.md) §3.2–3.3 相同，输入为 complete-case 差分（η0=0）。秩和检验公式不变。

## 4. 表形

| 表 | 合同 |
|---|---|
| 符号秩检验 | 配对列保持（无 η0 列或 η0=0 标注） |
| 位置估计（Walsh） | 估计中位数、CI 下限/上限（与单样本同形） |

## 5. 明确不做

改结修正/连续性校正/P；Sign CI；假 golden。

## 6. 测试

配对 Walsh/CI 与「差分列 + 单样本 Wilcoxon」一致；W+/W−/P 回归；`# source: formula_reference`。
