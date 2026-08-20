# 泊松率检验 / 双因素 ANOVA 图 / Winters / Gage 图

> 研究日期：2026-08-19  
> 访问日期：2026-08-19（UTC+8）  
> 本文只记录官方公式、Minitab 表形与本轮实现边界。`formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| 1-Sample Poisson Rate 方法 | [Methods and formulas for 1-Sample Poisson Rate](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-sample-poisson-rate/methods-and-formulas/methods-and-formulas/) | 2026-08-19 |
| 2-Sample Poisson Rate 方法 | [Methods and formulas for 2-Sample Poisson Rate](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/2-sample-poisson-rate/methods-and-formulas/methods-and-formulas/) | 2026-08-19 |
| Garwood / χ² 与泊松 | [NIST 7.2.4.1 相关精确区间讨论](https://www.itl.nist.gov/div898/handbook/prc/section2/prc241.htm)；实现复用已有 `garwood_rate` | 2026-08-19 |
| 双因素残差图 | [Residual plots for Two-Way ANOVA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/two-way-anova/interpret-the-results/all-statistics-and-graphs/residual-plots/) | 2026-08-19 |
| 交互图 | [Interaction plot for Two-Way ANOVA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/two-way-anova/interpret-the-results/all-statistics-and-graphs/interaction-plot/) | 2026-08-19 |
| 残差图通则 | [Residual plots in Minitab](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/supporting-topics/residuals-and-residual-plots/residual-plots-in-minitab/) | 2026-08-19 |
| Winters 方法 | [Methods and formulas for Winters' Method](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/time-series/how-to/winters-method/methods-and-formulas/methods-and-formulas/) | 2026-08-19 |
| 交叉 Gage 图 | [Graphs for Crossed Gage R&R](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/crossed-gage-rr-study/interpret-the-results/all-statistics-and-graphs/graphs/) | 2026-08-19 |
| Xbar-R | 见 `docs/research/algorithm-chart-gap-matrix.md` 控制图口径 | 2026-08-18 |

主 Gage 图页在访问日返回 404；表形按 Minitab 交叉 Gage 常用「分量条 + 按操作者 Xbar-R」描述，不填写未导出数值。

## 2. 1-Sample / 2-Sample Poisson Rate

对标 Stat > Basic Statistics > 1-Sample / 2-Sample Poisson Rate。命令 `one_poisson_rate` / `two_poisson_rate`。

输入为缺陷计数 \(x\) 与观测长度 \(t\)（与泊松过程能力同一计数导入口径：`parse_numeric_cell`，`*` 计入 N*）。1-sample complete-case 多行求和 \(x=\sum D_i\)、\(t=\sum T_i\)（无长度列时每行用 `inspected_constant`）。2-sample 每组一行四列，多行不静默合并。

本轮 **不实现** 率比主输出、pooled-rate 选项、功效、Blaker/Wilson。

### 2.1 1-Sample 点估计

```text
λ̂ = x / t
H0: λ = λ0    λ0 > 0
备择：two_sided / less / greater
```

### 2.2 Exact：Garwood CI + 泊松尾

双侧 100(1−α)% Garwood（与泊松能力同一 `garwood_rate`）：

```text
λ_L = χ²_{α/2}(2x) / (2 t)          （x=0 时 λ_L = 0）
λ_U = χ²_{1−α/2}(2x+2) / (2 t)
```

精确检验（本轮用泊松尾，**不是** Minitab 双侧似然比 exact；`formula_reference ≠ golden`）：

```text
μ0 = λ0 · t
H1: λ > λ0  →  P(S ≥ x | S ~ Poisson(μ0))
H1: λ < λ0  →  P(S ≤ x | S ~ Poisson(μ0))
H1: λ ≠ λ0  →  min(1, 2 · min(下尾, 上尾))
```

单侧区间：把 Garwood 的 α 换成 2α 后只保留一侧（与单比例 Clopper–Pearson 同一策略）。

### 2.3 Normal：score 检验 + Wald CI

Minitab：正态近似在总发生数 > 10 时较稳妥；本轮 `x ≤ 10` 给警告。

```text
z = (λ̂ − λ0) / sqrt(λ0 / t)
Wald CI: λ̂ ± z_{1−α/2} · sqrt(λ̂ / t)
```

`λ̂ = 0` 时 Wald 标准误为 0，只诊断。

### 2.4 2-Sample 差值（H0: λ1 = λ2）

```text
λ̂1 = x1 / t1    λ̂2 = x2 / t2    Δ̂ = λ̂1 − λ̂2
```

Exact（条件二项，假设率相等）：

```text
S | W=x1+x2  ~  Binomial(W, p0)    p0 = t1 / (t1 + t2)
H1: Δ > 0  →  P(S ≥ x1 | W, p0)
H1: Δ < 0  →  P(S ≤ x1 | W, p0)
H1: Δ ≠ 0  →  min(1, 2 · min(下尾, 上尾))
```

Normal（Wald，未做 pooled-rate）：

```text
z = (λ̂1 − λ̂2) / sqrt(λ̂1/t1 + λ̂2/t2)
Δ̂ ± z_{1−α/2} · SE
```

### 2.5 表形

- 描述：事件数、观测长度、率、N* / 行数。
- 检验结果：方法、Z（normal）、P-Value、置信区间。
- 解释只陈述与 λ0 或两组率差的证据，不是规格判定。

## 3. 双因素 ANOVA 残差 4 图 + 交互均值图

对标 Stat > ANOVA > Two-Way ANOVA 的残差图与交互图。命令仍为 `two_factor_anova`。领域 RSS 差值、不可估计项不输出 F/P、无 Tukey——**本轮不改**。

残差已由单元格均值拟合：`e_i = y_i − ȳ_{A(i)B(i)}`。图顺序对齐 DOE 析因残差 4 图：

1. 残差 vs 拟合值（y=0 参考线）
2. 残差 vs 观测顺序（输入行序，x = 1..n）
3. 残差正态概率图
4. 残差直方图

每点 `source_rows` 为工作表原始行。交互图：x = 因子 A 水平序号，每个因子 B 一条均值连线；悬停用该单元格第一条观测的 `source_row`。解释不写「残差已正态」。

## 4. Winters / Holt–Winters 输出合同

对标 Stat > Time Series > Winters' Method。**不新开命令**，深化已有 `seasonal_forecasting`。计算核已存在（用户给定 α/β/γ，不是 Minitab 网格优化，也不是 Kalman / TSERIES）。

加法：

```text
L_t = α (Y_t − S_{t−p}) + (1−α)(L_{t−1} + T_{t−1})
T_t = γ (L_t − L_{t−1}) + (1−γ) T_{t−1}
S_t = δ (Y_t − L_t) + (1−δ) S_{t−p}
Ŷ_t = L_{t−1} + T_{t−1} + S_{t−p}
```

乘法：

```text
L_t = α (Y_t / S_{t−p}) + (1−α)(L_{t−1} + T_{t−1})
T_t = γ (L_t − L_{t−1}) + (1−γ) T_{t−1}
S_t = δ (Y_t / L_t) + (1−δ) S_{t−p}
Ŷ_t = (L_{t−1} + T_{t−1}) S_{t−p}
```

本轮补齐相对 SES/DES 的表形：

| 表 | Facts / metadata |
|---|---|
| 预测准确度 | `ForecastFacts.mape`（及已有 MASE / rolling） |
| 拟合与预测明细 | 含原始行 |
| 季节指数 | Phase / Seasonal Index（来自 `result.seasonal`） |

`method_metadata.estimation_method` = `holt_winters_additive` 或 `holt_winters_multiplicative`。`parameter_source=specified`（用户权重，不是 MLE）。SARIMA 候选表仍可出现，**不是** TSERIES golden。

## 5. 交叉 Gage 分量图 / 按零件 Xbar-R / Run Chart

对标质量工具 > Gage Study > Crossed Gage R&R 常用图。ANOVA 表、方差分量、ndc、负方差截断规则 **不改**。

- 分量条：`PlotKind::pareto`，类别 Repeatability / Reproducibility / Part-To-Part，值为 **%Contribution**（合计 100%）。不把 Total Gage R&R 再叠进同一 Pareto。
- **Gage Run Chart**：`PlotKind::control`；全部 complete-case 测量值按行序；中心线 = 全数据均值；每点 `source_rows`；不跑 SPC Test 1–8。
- **按零件 Xbar-R**（Crossed）：每个（操作者, 零件）单元格为一等量子组；`phase_labels` = 零件名；标题 `按零件 Xbar` / `按零件 R`。重复 <2 不画 R。
- **Nested** 仍用 **按操作者** Xbar-R（`append_operator_xbar_range_plots`）。

图序（Crossed）：%Contribution → %Study Var → Gage Run Chart → 按零件 Xbar/R → [gated] 按零件散点 → 操作者×零件交互。

解释仍只读 `MsaFacts`（ndc、%Study Var、截断）。控制图 OOC 不写成「量具不通过」。By Part 与 Operator×Part 交互图见 `docs/research/gage-by-part-interaction-formulas.md`。

### 5.1 %Study Var 分量条（Crossed + Nested 对称）

访问日期：2026-08-20（UTC+8）。Minitab 交叉 Gage 图页在访问日仍可能 404；表形按 Minitab 常用「分量条」输出描述，**不填写**未导出数值。`formula_reference ≠ golden`。

方差分量表已输出 `Study Var` 与 `%Study Var` 列（领域 `gage_rr.cpp` / `nested_gage_rr.cpp`）。本轮在 `%Contribution` Pareto **之后追加**独立 `%Study Var` Pareto，**两张图并存**。

```text
Study Var_i      = multiplier × StDev_i              （默认 multiplier = 6）
%Study Var_i     = StDev_i / sqrt(Σ VarComp) × 100
%Contribution_i  = VarComp_i / Σ VarComp × 100       ← 仅用于 %Contribution 图
```

| 图 | 合同 |
|---|---|
| 标题 | `方差分量 %Study Var` |
| 类型 | `PlotKind::pareto` |
| 类别 | Repeatability、Reproducibility、Part-To-Part（**不含** Total Gage R&R） |
| 柱高 | 各源 `percent_study_variation`（**禁止**用 `percent_contribution` 顶替） |
| 累积线 | 三条 `%Study Var` 的 running sum；**允许**累计不到 100%（与 %Contribution 不同，禁止强行归一化） |

Crossed 与 Nested **共用** `append_gage_study_var_pareto`。ANOVA、ndc、负方差截断、交互 pooled、By Part / 交互图语义 **不改**。解释不写量具通过。
