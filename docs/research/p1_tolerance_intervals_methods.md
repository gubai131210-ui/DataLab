# P1 Tolerance Intervals（Normal + Nonparametric）

- 访问日期：2026-08-20
- 目标：输出统一 `method/method_family/achieved_confidence`

## 正态法（normal）

- 双侧（Howe 近似）：
  - `k = z_{(1+p)/2} * sqrt((ν*(1+1/n))/χ²_{1-α,ν})`
  - 区间：`[x̄-ks, x̄+ks]`
- 单侧（Natrella 近似）：
  - 按 Natrella 近似计算 `k`，输出单侧上下限。

## 非参数法（order statistic）

- 连续分布假设下，使用顺序统计量边界。
- 输出 `achieved_confidence`（样本给出的实际置信度）。
- 若 `achieved_confidence` 显著低于目标置信水平，给出样本量不足警告。

## 变量定义

- `p`: 覆盖率（coverage）
- `1-α`: 目标置信水平
- `x̄, s`: 样本均值与样本标准差
- `n, ν=n-1`: 样本量与自由度

## Minitab 口径

- 同时展示 normal 与 nonparametric 方法。
- 强调 nonparametric 需要较大样本，且需报告 achieved confidence。

## 来源

- https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/tolerance-intervals-nonnormal-distribution/interpret-the-results/all-statistics-and-graphs/tolerance-intervals/
- https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/supporting-topics/tolerance-interval-basics/
