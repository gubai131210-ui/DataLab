# P0 等效性检验（TOST）输出统一

- 访问日期：2026-08-20
- 目标：统一 1-sample / 2-sample 的双单侧 P 值、CI、等价区间判定

## 公式

- 差值：`d = \hat{\mu}_T - \hat{\mu}_R`（单样本时 `\hat{\mu}-\mu_0`）
- 下侧检验统计量：`t_L = (d-LEL)/SE`
- 上侧检验统计量：`t_U = (d-UEL)/SE`
- 双单侧 p 值：
  - `p_lower = 1 - F_t(t_L, df)`
  - `p_upper = F_t(t_U, df)`
- 判定：`p_lower <= α && p_upper <= α`
- 区间：当前实现输出 `CI(1-α)` 并与等价区间同表展示。

## 变量定义

- `LEL/UEL`: 等价区间下/上限
- `SE`: 差值标准误
- `df`: 自由度（单样本 `n-1`；双样本按 Welch/pooled）
- `α = 1-confidence_level`

## 适用条件

- 近似正态、独立样本、完整行（complete-case）输入。
- 配置可选择 Welch 或 pooled 方差口径（双样本）。

## Minitab 口径

- 核心解释是“CI 是否完整落入等价区间”。
- Minitab 支持默认 equivalence CI，与 `(1-2α)` 标准 CI 的替代展示。

## 来源

- https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/how-to/2-sample-equivalence-test/interpret-the-results/key-results/
- https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/supporting-topics/confidence-intervals-in-equivalence-testing/
