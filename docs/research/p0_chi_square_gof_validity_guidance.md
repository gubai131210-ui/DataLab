# P0 Chi-Square GOF 低期望频数有效性提示

- 访问日期：2026-08-20
- 目标：低期望频数场景输出“有效性等级 + 建议动作”，避免硬判定

## 公式

- 期望频数：`E_i = p_i * N`
- 统计量：`χ² = Σ (O_i-E_i)^2 / E_i`
- 自由度：`df = k-1`
- P 值：`P(Χ > χ² | df)`

## 变量定义

- `O_i`: 第 `i` 类观察频数
- `E_i`: 第 `i` 类期望频数
- `p_i`: 假设比例（给定或均匀）
- `N`: 总计数

## 有效性规则（Minitab 助手口径）

- 规则 A：`min(E_i) >= 1.25` 且 `<5` 的类别比例不超过 `50%`
- 规则 B：`min(E_i) >= 2.5`
- 实现建议状态：
  - `ok`: 满足规则 A 或 B
  - `caution`: 未满足 A/B 但最小期望频数仍不极端
  - `poor`: 最小期望频数过低

## 建议动作

- 优先合并相邻类别后复算。
- 输出 recommendation 字段，解释层给出“谨慎解释”而非直接 invalid。

## 来源

- https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/supporting-topics/chi-square/are-the-results-of-my-chi-square-test-invalid/
- https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/how-to/chi-square-goodness-of-fit-test/methods-and-formulas/methods-and-formulas/
