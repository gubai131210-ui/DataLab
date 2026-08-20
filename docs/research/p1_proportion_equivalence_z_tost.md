# P1 比例等价性（1/2-sample z-TOST）

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 等价检验 TOST 口径 | [Confidence intervals in equivalence testing](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/supporting-topics/confidence-intervals-in-equivalence-testing/) | 2026-08-20 |
| Minitab 1-Sample Equivalence（均值 TOST 表形参考） | [1-Sample Equivalence Test methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/how-to/1-sample-equivalence-test/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| Minitab 单比例方法 | [1 Proportion methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-proportion/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| Minitab 两比例方法 | [2 Proportions methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/2-proportions/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| NIST 二项比例 | [NIST e-Handbook: Confidence intervals for a proportion](https://www.itl.nist.gov/div898/handbook/prc/section2/prc241.htm) | 2026-08-20 |
| TOST 比例大样本 z | NCSS/PASS One Proportion Equivalence（Schuirmann TOST + Wald z） | 2026-08-20 |

Minitab 桌面菜单以**均值**等价检验为主；本产品比例等价采用与现有均值 TOST 同构的 **Wald z-TOST + 100(1−2α)% CI**，帮助写明非 Wilson/Blaker。

## 2. 公式

设置信水平为 `1−α`（默认 0.95 ⇒ α=0.05）。等价界限 `[LEL, UEL]`，要求 `LEL < UEL`。

### 2.1 单比例

```text
p̂ = x / n
d = p̂ − p0
SE = sqrt(p̂(1−p̂)/n)                    # Wald；用样本比例
z_L = (d − LEL) / SE
z_U = (d − UEL) / SE
p_L = 1 − Φ(z_L)
p_U = Φ(z_U)
within_limits ⇔ p_L ≤ α 且 p_U ≤ α
CI(1−2α) = d ± z_(1−α) · SE              # 标准正态分位；ci_method=wald_z_tost
```

### 2.2 两比例

```text
p̂1 = x1/n1,  p̂2 = x2/n2
d = p̂1 − p̂2
SE = sqrt(p̂1(1−p̂1)/n1 + p̂2(1−p̂2)/n2)   # 未合并双方差 Wald
其余同 2.1
```

## 3. 变量与边界

| 符号 | 含义 |
|---|---|
| x, n | 事件数、试验数（完整行求和） |
| p0 | 单样本目标比例 |
| LEL, UEL | 等价下/上限（比例差尺度） |
| Φ | 标准正态 CDF |

边界：

- `n=0`、`events > trials`、`LEL ≥ UEL`、置信水平非法 → 错误诊断，不伪造 P。
- `p̂∈{0,1}` 或两组导致 `SE=0` → `zero_variance`，CI=`*`。
- 不做 Blaker / Wilson / Agresti–Coull；不做比值/对数 TOST。

## 4. Minitab 表形对齐（产品合同）

| 表/图 | 合同 |
|---|---|
| 描述统计 | 组 / 事件 / 试验 / 比例（单样本一行；双样本两行） |
| 等价性检验 | 差值、下限 z、下限 P、上限 z、上限 P、α、CI、CI 方法、界限、结论 |
| 区间图 | 差值点 + 100(1−2α)% CI；等价上下界参考线 |
| Facts | `EquivalenceFacts.kind=one_proportion\|two_proportion`；`ci_method=wald_z_tost` |

解释只陈述 CI 是否落入界限与双单侧 P，**不写已证明等价**。

## 5. 导入契约

与 `one_proportion` / `two_proportions` 相同：complete-case 多行求和；`*`/`NA` 计 N*；一组缺失不污染另一组；`source_row` 不进比例求和分母以外的伪造。

## 6. 命令

- `one_proportion_equivalence`
- `two_proportion_equivalence`
