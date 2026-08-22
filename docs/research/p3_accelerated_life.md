# P3：Accelerated Life Testing（ALT · 窄化）

> 研究日期：2026-08-22 · 访问 2026-08-22（UTC+8）  
> `formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `accelerated_life` | Weibull + Arrhenius；Newton-Raphson MLE；回归表；B10/B50/B90 分位寿命；Life-Stress 图；`AcceleratedLifeFacts` |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/accelerated-life-testing/methods-and-formulas/equations/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/accelerated-life-testing/interpret-the-results/regression-table/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/supporting-topics/basics/choose-model-for-accelerated-life-testing/ | 2026-08-22 |

## Minitab 表形（非 golden）

| 表 | 列 |
|---|---|
| Regression Table | Predictor · Coef · SE Coef · Z · P · 95% CI Lower/Upper |
| Shape | 估计 · SE · 95% CI |
| Percentiles for Accelerated Levels | Stress · Percentile · Life |
| Percentiles at Design Level | Use Stress · Percentile · Life |
| 模型方程 | log(Yp) = β0 + β1·x + (1/shape)·Φ⁻¹(p)；x = 11604.83/(T+273.16) |

## MLE（Newton-Raphson）

### 参数

| 符号 | 代码字段 | 含义 |
|---|---|---|
| β₀ | `intercept` | Arrhenius 回归截距（log 尺度） |
| β₁ | `slope` | Arrhenius 回归斜率 |
| log(β) | `log_shape` | Weibull 形状参数对数 |

应力变换：`x = 11604.83 / (T + 273.16)`（Arrhenius，T 为摄氏温度）。

各观测 i 的尺度：`α_i = exp(β₀ + β₁ x_i)`。

### 右删失 Weibull 对数似然

失效（δᵢ=1）：

```
ℓ_i = log(β/α_i) + (β-1) log t_i - β log α_i - (t_i/α_i)^β
```

右删失（δᵢ=0）：

```
ℓ_i = -(t_i/α_i)^β
```

### 估计流程

1. 失效子集 OLS 初始化 β₀、β₁；失效 log 时间方差启发式初始化 log(β)。
2. Newton-Raphson 最大化 ℓ；梯度为解析式，观测信息矩阵为梯度数值 Hessian 的负值。
3. 在 MLE 处反演观测信息矩阵得协方差；`SE Coef = sqrt(cov_ii)`。
4. 形状 SE：`SE_shape = shape × SE_log_shape`（delta 法）。

诊断码：`alt_newton_mle`、`alt_observed_information`。

## 分位寿命预测

使用应力 T（°C）：

```
scale = exp(intercept + slope × 11604.83/(T+273.16))
life_p = scale × [-ln(1 - p/100)]^(1/shape)
```

| 输出 | 说明 |
|---|---|
| `percentiles_at_stress_levels` | 各加速应力水平的 B10/B50/B90 |
| `percentiles_at_use_stress` | 配置 `use_stress_celsius` 处 B10/B50/B90 |
| `life_stress_curve` | 应力范围内 B10/B50/B90 曲线点（Life-Stress 图） |

导出函数：`accelerated_life_percentile_at_stress(intercept, slope, shape, stress_celsius, percentile)`。

## 产品边界

- 寿命列 + 失效指示 + **单**应力列（摄氏温度）；右删失 Weibull MLE。  
- 仅 Arrhenius 变换；**不做** Eyring/Log-power/多应力；试验计划；Probit。  
- 解释：参数为拟合证据，非「产品已达标寿命」。
