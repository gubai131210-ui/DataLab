# 单比例 / TOST 等价性 / DOE 残差 4 图 / I-MR-R/S

> 研究日期：2026-08-19  
> 访问日期：2026-08-19（UTC+8）  
> 本文只记录官方公式、Minitab 表形与本轮实现边界。`formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| 1 Proportion 方法 | [Methods and formulas for 1 Proportion](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-proportion/methods-and-formulas/methods-and-formulas/) | 2026-08-19 |
| 1 Proportion 概览 | [Overview for 1 Proportion](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-proportion/before-you-start/overview/) | 2026-08-19 |
| 1 Proportion 选项 | [Select the analysis options for 1 Proportion](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-proportion/perform-the-analysis/select-the-analysis-options/) | 2026-08-19 |
| Clopper–Pearson / 二项 | [NIST 7.2.4](https://www.itl.nist.gov/div898/handbook/prc/section2/prc24.htm) | 2026-08-19 |
| 1-Sample 等价检验 | [Test for 1-Sample Equivalence Test](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/how-to/1-sample-equivalence-test/interpret-the-results/all-statistics-and-graphs/test/) | 2026-08-19 |
| 2-Sample 等价假设 | [Hypotheses for 2-Sample Equivalence Test](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/how-to/2-sample-equivalence-test/before-you-start/hypotheses/) | 2026-08-19 |
| 2-Sample 差值公式 | [Methods and formulas for Test mean - reference mean](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/how-to/2-sample-equivalence-test/methods-and-formulas/test-mean---reference-mean/) | 2026-08-19 |
| DOE 析因残差图 | [Residual plots for Analyze Factorial Design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-factorial-design/interpret-the-results/all-statistics-and-graphs/residual-plots/) | 2026-08-19 |
| 残差图通则 | [Residual plots in Minitab](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/supporting-topics/residuals-and-residual-plots/residual-plots-in-minitab/) | 2026-08-19 |
| I-MR-R/S 概览 | [Overview for I-MR-R/S Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-subgroups/i-mr-r-s-chart/before-you-start/overview/) | 2026-08-19 |
| I-MR-R/S 解读 | [Interpret the key results for I-MR-R/S Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-subgroups/i-mr-r-s-chart/interpret-the-results/key-results/) | 2026-08-19 |
| I-MR-R/S 标准差 | [Standard deviations in I-MR-R/S Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-subgroups/i-mr-r-s-chart/methods-and-formulas/standard-deviations/) | 2026-08-19 |
| I-MR-R/S 估计选项 | [Specify how to estimate the parameters](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-subgroups/i-mr-r-s-chart/perform-the-analysis/i-mr-r-s-options/specify-estimation-options/) | 2026-08-19 |
| 组间/组内能力 | 见 `docs/research/logistic-idi-between-within-chart-formulas.md` | 2026-08-18 |

## 2. 单比例检验（1 Proportion）

对标 Stat > Basic Statistics > 1 Proportion。菜单命令 `one_proportion`。

输入为事件数 \(x\) 与试验数 \(n\)（两列 complete-case 求和 \(x=\sum D_i\)、\(n=\sum N_i\)），对齐二项过程能力的 Dtot/Ntot。两比例检验仍要求每组一行汇总；单比例允许多行求和，并诊断 `summarized_from_multiple_rows`。

本轮 **不实现** Minitab 当前默认 Adjusted Blaker，也不做 Wilson-score / Agresti–Coull。默认 **exact = Clopper–Pearson**，与已有二项能力同一 `clopper_pearson_interval`。

### 2.1 点估计

```text
p̂ = x / n
```

假设比例 \(p_0 \in (0,1)\)。备择：`two_sided` / `less` / `greater`。

### 2.2 Exact：Clopper–Pearson CI + 二项精确 P

双侧 100(1−α)% CI（Minitab / NIST F 形式，与二项能力相同）：

```text
若 x = 0：p_L = 0
否则：p_L = 1 / (1 + (n − x + 1)/x · F_{1−α/2}(2(n−x+1), 2x))
若 x = n：p_U = 1
否则：p_U = 1 / (1 + (n − x)/((x + 1) · F_{1−α/2}(2(x+1), 2(n−x))))
```

对应精确检验（Minitab “Test that corresponds to the Clopper-Pearson exact confidence interval”）：

```text
H1: p > p0  →  P(X ≥ x | n, p0)
H1: p < p0  →  P(X ≤ x | n, p0)
H1: p ≠ p0  →  min(1, 2 · min(下尾, 上尾))   加倍尾；不是 Blaker
```

### 2.3 Normal：Wald CI + score 检验

对齐已有 two_proportions 的 Wald 区间；检验用 score（Minitab Score test，SE 用 \(p_0\)）：

```text
z_score = (p̂ − p0) / sqrt(p0(1−p0)/n)
Wald CI: p̂ ± z_{1−α/2} · sqrt(p̂(1−p̂)/n)
```

`p̂ ∈ {0,1}` 或 `p0 ∈ {0,1}` 只诊断。`n p0 < 5` 或 `n(1−p0) < 5` 警告。

### 2.4 表形

- 描述：事件数、试验数、比例。
- 检验结果：方法、Z 或 Exact、P-Value、置信区间。
- 不输出 Cp/Cpk。解释不写过程合格。

## 3. 等价性检验 TOST

对标 Equivalence Tests > 1-Sample / 2-Sample。命令 `one_sample_equivalence` / `two_sample_equivalence`。本轮只做均值差，不做比值、对数变换、配对。

```text
单样本 Δ = mean − target
双样本 Δ = mean1 − mean2     （默认 Welch；可选 pooled）
H0: Δ ≤ δ1  与  H0: Δ ≥ δ2
H1: δ1 < Δ < δ2

t1 = (Δ − δ1) / SE    p1 = P(T_ν > t1)
t2 = (Δ − δ2) / SE    p2 = P(T_ν < t2)
100(1−2α)% CI = [Δ − t_{1−α,ν} SE, Δ + t_{1−α,ν} SE]
within_limits ⇔ p1 ≤ α 且 p2 ≤ α
```

`α = 1 − confidence_level`（UI 填 95% → α=0.05）。上述 CI 完全落在 [δ1, δ2] 与双侧单侧检验决策等价。

Minitab 默认展示的等价区间为 `[min(C, Dl), max(C, Du)]`（100(1−α)% 形态），与 100(1−2α)% CI **数值不必一致**；本轮不宣称 golden。

解释只陈述是否落在等价界限内，不写「过程合格」或「已证明等价」。

### 3.1 表/图合同

- 描述统计（N、Mean、StDev）。
- 等价性检验：下限 t/P、上限 t/P、α、CI、δ1/δ2、结论 `within_limits` / `not_within_limits`。
- 区间图：一条 CI，参考线为等价界限。

## 4. DOE 析因残差 4 图

对标 Analyze Factorial Design > Residual plots。残差 \(e_i = y_i - \hat{y}_i\) 已由现有 `fit_response_analysis` 计算。本轮只在响应页补图，**不改** 优化器 D、Pareto、立方、等值线/静态曲面。

顺序：

1. 残差 vs 拟合值（y=0 参考线；检查等方差）
2. 残差 vs 观测顺序（x = run_order+1；检查独立性）
3. 残差正态概率图（检查正态；直方图不得代替此图）
4. 残差直方图（看偏态/离群；Minitab：不要用直方图评估正态）

`source_rows` 用导入运行的工作表行（`standard_order`）。解释：残差图供调查；不写「残差已正态 / 模型合格」。

## 5. I-MR-R/S（组间/组内）

对标 Variables Charts for Subgroups > I-MR-R/S (Between/Within)。与已有 `between_within_capability` 同一套 σ。

```text
σ_within = R̄ / d2(n)          n≤8 画 R 图；n≥9 画 S 图（S̄ / c4）
σ_x̄     = MR̄(子组均值) / d2(2)
σ_B²     = max(0, σ_x̄² − σ_w² / n)
σ_BW     = sqrt(σ_B² + σ_w²)
```

三张图：

- I 图：子组均值，限 \( \bar{\bar{x}} \pm 3\sigma_{\bar{x}} \)（画均值，不是 σ_B）；Test 1–8
- MR 图：子组均值的移动极差；Test 1–4
- R 或 S：组内；Test 1–4

等长子组、n≥2、至少 2 个子组。点 `source_row` = 子组第一观测行。解释陈述超限点数与组间/组内 σ，不写过程合格。无子组只诊断，不静默退化成 I-MR。
