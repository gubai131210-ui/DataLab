# P1 两比例 Newcombe–Wilson 差值 CI

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Newcombe Method 10 | Newcombe R.G. (1998), *Stat Med* — Wilson score intervals for difference of proportions | 2026-08-20 |
| NIST 单比例 Wilson | [NIST e-Handbook 7.2.4.1](https://www.itl.nist.gov/div898/handbook/prc/section2/prc241.htm) | 2026-08-20 |
| Minitab 2 Proportions | [2 Proportions methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/2-proportions/methods-and-formulas/methods-and-formulas/)（表形对照，不填未导出数） | 2026-08-20 |
| DataLab 单比例 Wilson | [`p1_wilson_proportion_ci.md`](p1_wilson_proportion_ci.md) | 2026-08-20 |

## 2. 产品选型

在 `two_proportions` 增加 `method`：

| method | 检验 Z/P | CI | `ci_method` |
|---|---|---|---|
| `normal`（默认） | 现有 unpooled Wald | unpooled Wald | `wald` |
| `wilson` | **同一 unpooled Wald**（只换 CI） | Newcombe–Wilson | `newcombe_wilson` |

不做 Agresti–Coull、Blaker；不改 Fisher。

## 3. 公式

设 \(\hat p_i=x_i/n_i\)，\(z=z_{1-\alpha/2}\)。单比例 Wilson（无连续性校正）：

```text
denom_i = 1 + z²/n_i
l_i = (p̂_i + z²/(2n_i) − z·sqrt(p̂_i(1−p̂_i)/n_i + z²/(4n_i²))) / denom_i
u_i = (p̂_i + z²/(2n_i) + z·sqrt(...)) / denom_i
```

边界：\(x_i=0\Rightarrow l_i=0\)；\(x_i=n_i\Rightarrow u_i=1\)；再 clamp 到 [0,1]。

差值 Δ = p̂1 − p̂2 的 Newcombe–Wilson：

```text
CI_lower = (p̂1 − p̂2) − sqrt( (p̂1−l1)² + (u2−p̂2)² )
CI_upper = (p̂1 − p̂2) + sqrt( (u1−p̂1)² + (p̂2−l2)² )
```

检验仍：

```text
SE_sep = sqrt(p̂1(1−p̂1)/n1 + p̂2(1−p̂2)/n2)
Z = Δ / SE_sep
```

## 4. 表形

现有描述 / 检验 / 差值区间图；Facts：`method=wilson`，`ci_method=newcombe_wilson`。

## 5. 明确不做

两比例 Agresti–Coull；Blaker；改默认 Wald 数值合同。

## 6. 测试

`# source: formula_reference`：同一计数下 `normal` 与改前 Wald 一致；`wilson` 时 Z 不变、CI 端点按上式。
