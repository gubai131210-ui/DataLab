# P1 符号检验（Sign test）

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。去结后二项精确双侧 P；大样本可附正态近似说明。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| 符号检验 / 二项 | 标准非参数教材：对 η₀ 的符号计数 \(S\sim\mathrm{Bin}(n,1/2)\) | 2026-08-21 |
| Minitab 1-Sample Sign | [Methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/1-sample-sign/methods-and-formulas/methods-and-formulas/)（表形对照；Minitab 大样本阈值与本产品锁定可不同，不作 golden） | 2026-08-21 |
| Minitab 概述 | [Overview for 1-Sample Sign](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/1-sample-sign/before-you-start/overview/) | 2026-08-21 |
| NIST 非参数入口 | [NIST e-Handbook 7.4](https://www.itl.nist.gov/div898/handbook/prd/section4/prd4.htm) | 2026-08-21 |
| DataLab 配对缝 | 现有 `wilcoxon_signed_rank`（**本轮不改**）；`align_complete_rows` | 2026-08-21 |

## 2. 产品选型

- **新命令** `sign_test`
- **单样本**：一列数值 + 假设中位数 \(\eta_0\)（**默认 0**）
- **配对**：两列数值；对差分 \(d_i=x_i-y_i\) 相对 \(\eta_0=0\) 做符号检验（complete-case）
- 丢弃等于 \(\eta_0\) 的观测（结）；有效样本量 \(n=\) 非结个数
- Facts：可挂 `NonparametricFacts` 或轻量 `SignTestFacts`（方法标记 `sign_test` / `sign_test_paired`）

**禁止**修改 `wilcoxon_signed_rank` 本体、秩与结修正。

解释禁止「已证明中位数等于/不等于 η₀」。

## 3. 公式

令 \(n_+\) = 大于 \(\eta_0\) 的个数，\(n_-\) = 小于 \(\eta_0\) 的个数，\(n=n_++n_-\)（结已丢）。在 \(H_0:\eta=\eta_0\) 下 \(n_+\sim\mathrm{Bin}(n,1/2)\)。

**双侧精确 P（产品锁定）：**

\[
p=2\cdot\min\bigl\{
  P(X\le n_+),\ 
  P(X\ge n_+)
\bigr\}
\quad\text{再截断到 }[0,1]
\]

其中 \(X\sim\mathrm{Bin}(n,1/2)\)；即较小单侧尾再加倍（标准 exact binomial two-sided；当 \(n_+=n/2\) 时 \(p=1\)）。

报告：\(n\)、\(n_+\)、\(n_-\)、结数、样本中位数、\(\eta_0\)、精确 \(p\)。

**大样本说明（可选，非改 P）：** 当 \(n\ge 25\)，可在诊断/注释中提示亦可参照正态近似

\[
Z=\frac{n_+-n/2}{\sqrt{n/4}}
\]

（是否带连续性校正仅作说明；**主 P 仍用精确二项**）。Minitab Help 对 \(n>50\) 才切正态——本产品不跟其切换阈值，避免假对齐。

\(n=0\)（全结或无有效对）→ 诊断、不出 P。

## 4. 表形

| 表 | 合同 |
|---|---|
| 摘要 | N（有效）、结数、中位数、η₀、n₊、n₋ |
| 符号检验 | 精确双侧 P；方法=二项 |
| （可选）注释 | n≥25 正态近似提示 |

配对模式标题标明配对差分。

## 5. 明确不做

改 `wilcoxon_signed_rank`；把主 P 改为仅正态近似；假 Minitab golden CI 三区间（本轮可不实现 Sign CI）；Mood 中位数检验。

## 6. 测试

对称绕 η₀：P 大；明显单侧偏：P 小且与手算二项一致；全结诊断；配对差分与单列差分命令结果一致；Wilcoxon 回归不变；`# source: formula_reference`。
