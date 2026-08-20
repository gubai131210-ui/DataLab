# P1 功效族扩展：方差样本量 / 功效

- 访问日期：2026-08-20
- 目标：在现有 `t_power` 家族中补齐 `one_variance_*` / `two_variance_*`
- 口径：优先贴近 Minitab Power and Sample Size for 1 Variance / 2 Variances；本轮先落 **χ² / F 检验** 路径，不做 Levene / Bonett

## 选定范围

- `one_variance_power`
- `one_variance_sample_size`
- `two_variance_power`
- `two_variance_sample_size`

当前把 `effect_size` 解释为：

- 单方差：`σ / σ0` 的标准差比
- 双方差：`σ1 / σ2` 的标准差比

这是无量纲输入，因此不需要再额外输入绝对 σ 值。

## 单方差（1 Variance）

设：

- `n`：样本量
- `df = n - 1`
- `r = σ / σ0`

H0 下统计量：

`T = (n-1) S^2 / σ0^2 ~ χ²(df)`

真值为 `σ = r σ0` 时：

`T = r^2 * X`, 其中 `X ~ χ²(df)`

因此功效可由 H0 的临界值与缩放后的 χ² 分布得到：

- 双侧：`P(T < c_L or T > c_U | r)`
- 上侧：`P(T > c_U | r)`
- 下侧：`P(T < c_L | r)`

Minitab 文档说明其单方差功效基于 χ² 分布，并在样本量/ratio 反求时使用迭代算法。

## 双方差（2 Variances）

设两组每组样本量相同为 `n`，并定义：

- `df1 = df2 = n - 1`
- `r = σ1 / σ2`

H0 下统计量：

`F = S1^2 / S2^2 ~ F(df1, df2)`

真值为 `σ1 / σ2 = r` 时：

`F = r^2 * F0`, 其中 `F0 ~ F(df1, df2)`

因此功效为：

- 双侧：`P(F < c_L or F > c_U | r)`
- 上侧：`P(F > c_U | r)`
- 下侧：`P(F < c_L | r)`

Minitab 2 Variances 支持 F-test、Levene、Bonett；当前 DataLab 只实现 F-test 路径。

## 反求样本量

与现有 `t_power` 一致：

- 给定 `target_power`
- 对整数样本量二分搜索
- 输出最小满足目标功效的样本量
- `Actual Power` 为该整数样本量对应的真实功效

## UI / mode 约定

在 `t_power` 命令中新增 mode 前缀：

- `one_variance_power`
- `one_variance_sample_size`
- `two_variance_power`
- `two_variance_sample_size`

输入 `alternative` 复用：

- `two_sided`
- `greater`
- `less`

## 边界与限制

- 当前双方差按**等组样本量**处理
- 当前只做标准差比输入，不直接输入方差比；若用户给方差比，需自行开平方后再填
- 不做 Levene / Bonett
- 不把输出解释为“样本量足够”或“设计已充分”，只给统计功效关系

## Minitab 参考

1. Methods and formulas for Power and Sample Size for 1 Variance  
   https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/power-and-sample-size/how-to/hypothesis-tests/power-and-sample-size-for-1-variance/methods-and-formulas/methods-and-formulas/  
   访问日期：2026-08-20

2. Overview for Power and Sample Size for 2 Variances  
   https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/power-and-sample-size/how-to/hypothesis-tests/power-and-sample-size-for-2-variances/before-you-start/overview/  
   访问日期：2026-08-20

3. Methods and formulas for Power and Sample Size for 2 Variances  
   https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/power-and-sample-size/how-to/hypothesis-tests/power-and-sample-size-for-2-variances/methods-and-formulas/methods-and-formulas/  
   访问日期：2026-08-20

## 实现备注

- domain：`src/domain/statistics/t_power.*`
- service：`AnalysisService::t_power()` 的 `compute` 与曲线循环两处一起扩展
- help：`t_power` 帮助正文补全新 mode 说明
- tests：继续使用 `formula_reference`，禁止冒充 Minitab 数值 golden
