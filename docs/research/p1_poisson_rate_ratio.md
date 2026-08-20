# P1 双样本泊松率比

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 2-Sample Poisson Rate | [2-Sample Poisson Rate methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/2-sample-poisson-rate/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| 条件二项 / 率比 | NIST / 标准泊松比较；与现有 DataLab exact 差值同源 | 2026-08-20 |

## 2. 公式

```text
λ̂1 = x1/t1,  λ̂2 = x2/t2
ρ = λ̂1 / λ̂2
```

**normal（log-Wald）：**

```text
log ρ̂ = log(λ̂1) − log(λ̂2)
SE = sqrt(1/x1 + 1/x2)          # 仅当 x1>0 且 x2>0
CI(ρ) = exp( log ρ̂ ± z · SE )
双侧 z = (log ρ̂) / SE 相对 H0: ρ=1
```

**exact：** 给定 `x1+x2`，`x1 ~ Binomial(x1+x2, π)`，`π = t1/(t1+t2)`。  
率比与 `π` 的单调关系用于条件 CI（本轮：用条件二项分位变换到 ρ；若实现复杂则对 ρ 用同一条件检验的 P 值，CI 用 log-Wald 回退并诊断）。

**实现锁定（务实）：**

- `comparison=difference`：现有行为不变。
- `comparison=ratio`：主输出 ρ 与率比 CI；exact 时若 x1,x2>0，优先用 log-Wald 区间并标注 `ci_method=log_wald`（与 normal 同）；检验 P 对 exact 仍用条件二项相对 ρ=1（等价于相对 π=t1/(t1+t2)）。零事件：log-Wald 不可用 → 诊断 `zero_events_for_rate_ratio`，CI=`*`。

## 3. 配置

`InferenceConfiguration::rate_comparison`：`difference`（默认）| `ratio`。

## 4. Facts

`PoissonRateFacts`：`comparison`、`ratio`、`ratio_ci_lower`、`ratio_ci_upper`。

## 5. 不做

Bonett / Bartlett；Blaker；泊松功效；改 2-sample 每组一行契约。
