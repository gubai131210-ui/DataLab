# P1 单样本 Wilcoxon（相对 η0）

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。加深现有命令；不改配对 Wilcoxon 本体。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 方法 | [1-Sample Wilcoxon methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/1-sample-wilcoxon/methods-and-formulas/methods-and-formulas/) | 2026-08-21 |
| Minitab 统计量 | [Interpret all statistics](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/1-sample-wilcoxon/interpret-the-results/all-statistics/) | 2026-08-21 |
| Walsh / CI 叙述 | [Estimated median and CI](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/supporting-topics/calculate-the-estimated-median-and-confidence-interval-for-the-1-sample-wilcoxon-test/) | 2026-08-21 |
| NIST | [NIST e-Handbook 7.4](https://www.itl.nist.gov/div898/handbook/prd/section4/prd4.htm) | 2026-08-21 |

## 2. 产品选型

- **加深** 命令 `wilcoxon_signed_rank`：1 列 + η0 **或** 2 列配对
- 新 domain API：`wilcoxon_signed_rank_one_sample(values, eta0, alternative)`  
  实现：对 \(x_i-\eta_0\) 调用与配对相同的符号秩核（**不**伪造第二列常量 0 绕过 API）
- Facts：`method = "wilcoxon_one_sample"`（配对仍为 `wilcoxon_signed_rank`）
- 表形：W+/W−、Z、P + **Walsh 估计中位数** + **正态近似 CI**（公式参考；不作 golden）
- 图：箱线 + 个体值；单样本无配对散点

解释禁止「已证明中位数等于 η0」。不做 Sign CI。

## 3. 公式（锁定）

### 3.1 检验

丢弃 \(x_i=\eta_0\)；对非零差分 \(d_i=x_i-\eta_0\) 的绝对值做 midrank 符号秩；结修正方差 \(\mathrm{Var}=n(n+1)(2n+1)/24-\sum(t^3-t)/48\)；连续性校正 Z 与现有配对路径一致。

### 3.2 Walsh 点估计

对非结观测 \(Y_1,\ldots,Y_n\)（已去掉 \(=\eta_0\) 者；若全结则对有限原值）构造 Walsh 平均 \((Y_i+Y_j)/2\)（\(i\le j\)），取其中位数为估计 \(\hat\eta\)。

### 3.3 置信区间（公式参考）

\(M=n(n+1)/2\)；目标水平 \(1-\alpha\)；

\[
d=\frac{n(n+1)}{4}-0.5-z_{1-\alpha/2}\sqrt{\frac{n(n+1)(2n+1)}{24}}
\]

取 \(d^*=\lfloor d\rfloor\)（夹到合法秩）；下限 \(W_{(d^*+1)}\)，上限 \(W_{(M-d^*)}\)（1-based 有序 Walsh）。报告 achieved 水平说明诊断即可。配对路径**不**强制出 CI。

## 4. 表形

| 表 | 合同 |
|---|---|
| 符号秩检验 | N、η0、W+、W−、Z、P、结修正 |
| 位置估计 | 估计中位数（Walsh）；可选 CI 下限/上限 |

## 5. 明确不做

改配对 Wilcoxon 秩公式；Sign CI；假 Minitab golden；Ryan–Joiner。

## 6. 测试

对称绕 η0：P 大；单侧偏：P 小；配对两列回归不变；单列 η0=中位数附近；`# source: formula_reference`。
