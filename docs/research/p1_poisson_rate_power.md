# P1 泊松率功效与样本量（接 t_power）

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 1-Sample Poisson Rate 功效 | [Power and Sample Size for 1-Sample Poisson Rate — methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/power-and-sample-size/how-to/hypothesis-tests/power-and-sample-size-for-1-sample-poisson-rate/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| Minitab 概览 | [Overview: 1-Sample Poisson Rate power](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/power-and-sample-size/how-to/hypothesis-tests/power-and-sample-size-for-1-sample-poisson-rate/before-you-start/overview/) | 2026-08-20 |
| 2-Sample Poisson Rate 功效 | [Power and Sample Size analyses in Minitab](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/power-and-sample-size/supporting-topics/power-and-sample-size-analyses-in-minitab/) | 2026-08-20 |
| 正态近似功效（教材） | 泊松均值/率大样本 z 检验功效 | 2026-08-20 |

## 2. 锁定口径

采用 **正态近似**（与 Minitab 文档 Φ / \(z_\alpha\) / \(\sigma_0\) / \(\sigma_A\) 一致），**不做**精确泊松功效、不做 Blaker。

### 字段映射（复用 PowerConfiguration，语义写死）

| 配置字段 | 单样本泊松 | 双样本泊松 |
|---|---|---|
| `null_proportion` | \(\lambda_0\)（假设率） | \(\lambda_1\)（第一组率） |
| `second_proportion` | \(\lambda_1\)（比较率） | \(\lambda_2\)（第二组率） |
| `observation_length`（新增，默认 1） | 观测长度 \(L\) | 每组观测长度 \(L\)（等长） |
| `sample_size` | \(n\) | 每组 \(n\) |
| `effect_size` | 输出用 \(\lambda_1-\lambda_0\) | 输出用 \(\lambda_1-\lambda_2\) |

### mode 前缀

- `one_poisson_power` / `one_poisson_sample_size`
- `two_poisson_power` / `two_poisson_sample_size`

## 3. 公式

### 3.1 单样本

总暴露 \(E = n \cdot L\)。率估计 \(\hat\lambda\) 的渐近方差 \(\lambda/E\)。

```text
σ0 = sqrt(λ0 / E)
σA = sqrt(λ1 / E)
δ  = λ1 − λ0
```

单侧 \(\lambda > \lambda_0\)：

```text
power = Φ( (δ − z_α · σ0) / σA )
```

单侧 \(\lambda < \lambda_0\)：

```text
power = Φ( (−δ − z_α · σ0) / σA )   # 等价于对 −δ 的上侧形式
```

双侧：

```text
z = z_{α/2}
power = Φ( (−z·σ0 − δ) / σA ) + 1 − Φ( (z·σ0 − δ) / σA )
```

样本量：对整数 \(n\) 二分搜索，使 power ≥ 目标；`Actual Power` = 该整数下真实功效。

### 3.2 双样本（等组 n，等长 L）

比较 \(\delta = \lambda_1 - \lambda_2\)（差值主路径，与现有 `two_poisson_rate` 默认一致）。

```text
E = n · L
σ0 = sqrt( (λ̄ + λ̄) / E )  或 unpooled：sqrt(λ1/E + λ2/E)
     本产品锁定 unpooled：σA = σ0_alt = sqrt(λ1/E + λ2/E)
     H0 用合并或不合并：采用与比例 unpooled 类似的
     null_se = sqrt(λ1/E + λ2/E) 在 δ=0 规划下用两率均值？
```

**锁定（公式参考）**：双侧/单侧功效用

```text
null_se = sqrt( (λ1+λ2)/E )           # 保守规划 SE under H0-ish
alt_se  = sqrt( λ1/E + λ2/E )
```

再复用现有 `proportion_power(δ, null_se, alt_se, …)` 同构逻辑。

## 4. 表形

与现有 `t_power`：参数摘要 + Actual Power 表 + 功效曲线；`PowerFacts.mode` 带 `one_poisson_*` / `two_poisson_*`。解释 **不写「样本量足够」**。

## 5. 明确不做

- 重做 t / ANOVA / 比例 / 方差功效公式  
- 泊松 Blaker  
- 率比功效（ratio）  
- 假 Minitab golden  

## 6. 测试策略

`# source: formula_reference`：固定 \(\lambda_0,\lambda_1,n,L\) 手算 Φ 功效；sample_size 反求 actual_power ≥ target；非法率/长度诊断。
