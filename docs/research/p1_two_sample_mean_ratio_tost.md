# P1 双样本均值比 TOST（非对数）

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab Test mean / reference mean | [Methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/how-to/2-sample-equivalence-test/methods-and-formulas/test-mean-reference-mean/) | 2026-08-20 |
| Minitab 假设（比值） | [Hypotheses](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/how-to/2-sample-equivalence-test/before-you-start/hypotheses/) | 2026-08-20 |
| TOST 一般程序 | Schuirmann (1987)；教材 TOST 与 100(1−2α)% CI 合同 | 2026-08-20 |
| DataLab 现有差值 TOST | [`p0_equivalence_tost_minitab_alignment.md`](p0_equivalence_tost_minitab_alignment.md)、`equivalence_test` | 2026-08-20 |

## 2. 产品选型

- **新命令** `two_sample_equivalence_ratio`（不改 `two_sample_equivalence` 差值合同）。
- 角色：检验列（test）+ 参考列（reference）；界限为比值尺度（如 0.80 / 1.25）。
- `EquivalenceFacts.kind=two_sample_ratio`；**`difference` 字段存 ρ̂ = ȳ_test / ȳ_ref`**；`ci_method=tost_ratio_1_minus_alpha`。
- `within_limits ⇔ p_lower ≤ α ∧ p_upper ≤ α`（与差值 TOST 同一合同）。
- **不做**对数变换路径；不做配对/单样本比值。

## 3. 公式（双侧等价，非对数）

设检验样本均值/SD/n 为 \(\bar x_1,s_1,n_1\)，参考为 \(\bar x_2,s_2,n_2\)；界限 \(\delta_1<\delta_2\)；\(\alpha=1-\mathrm{CL}\)。

点估计：\(\hat\rho=\bar x_1/\bar x_2\)（要求 \(\bar x_2>0\)；\(\delta_1>0\)）。

### 3.1 不等方差（默认 Welch 口径）

对候选比值 δ：

```text
SE(δ) = sqrt(s1²/n1 + δ²·s2²/n2)
t(δ)  = (ȳ1 − δ·ȳ2) / SE(δ)
```

- \(t_1=t(\delta_1)\)，\(p_\mathrm{lower}=1-F_t(t_1;\nu)\)（上侧，对应 H0: ρ≤δ1）
- \(t_2=t(\delta_2)\)，\(p_\mathrm{upper}=F_t(t_2;\nu)\)（下侧，对应 H0: ρ≥δ2）

自由度 ν：对 \(\hat\rho\) 处 SE 用 Welch–Satterthwaite：

```text
a = s1²/n1
b = ρ̂²·s2²/n2
ν = (a+b)² / (a²/(n1−1) + b²/(n2−1))
```

### 3.2 等方差（pooled）

```text
Sp² = ((n1−1)s1²+(n2−1)s2²)/(n1+n2−2)
SE(δ) = Sp · sqrt(1/n1 + δ²/n2)
ν = n1+n2−2
```

### 3.3 100(1−2α)% 比值 CI（Fieller / Minitab 同型）

令 \(t^*=t_{1-\alpha,\nu}\)。在 \(\bar x_2^2 - t^{*2}s_2^2/n_2 > 0\)（或 pooled 对应条件）时，

不等方差：

```text
A = ȳ2² − t*²·s2²/n2
B = ȳ1·ȳ2
C = ȳ1² − t*²·s1²/n1
disc = B² − A·C
ρL, ρU = (B ± sqrt(disc)) / A   （有序使 ρL≤ρU）
```

pooled：将 \(s_i^2/n_i\) 换为 \(S_p^2/n_i\)（参考侧乘 δ 时用 \(S_p^2·\delta^2/n_2\) 进 SE；CI 用 \(S_p^2/n_1\)、\(S_p^2/n_2\)）。

本产品默认输出 **100(1−2α)%** 区间（与差值 TOST 一致），不用 Minitab 可选的 100(1−α)% 包装形式。

## 4. Minitab 表形（产品合同）

| 表/图 | 合同 |
|---|---|
| 描述统计 | 两组 N、均值、标准差 |
| 等价性检验 | 比值估计、下限/上限、t/P 两侧、CI |
| 区间图 | 横轴「比值」；须为 CI；竖线为界限 |
| Facts | `kind=two_sample_ratio`；`difference`=ρ̂ |

## 5. 明确不做

- 对数变换 TOST  
- 配对 / 比例 z-TOST 重做  
- 假 Minitab golden  

## 6. 测试策略

`# source: formula_reference`：手工小样例核对 ρ̂、两侧 t、CI 端点；参考均值≤0 / 界限非法 → 诊断。
