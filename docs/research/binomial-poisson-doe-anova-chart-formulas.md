# 二项/泊松过程能力 / DOE 等值线曲面 / ANOVA 区间残差 / 系列线型点型

> 研究日期：2026-08-19  
> 访问日期：2026-08-19（UTC+8）  
> 本文只记录官方公式、Minitab 表形与本轮实现边界。`formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| 二项能力统计 | [Capability statistics (Binomial)](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/binomial-capability-analysis/methods-and-formulas/capability-statistics/) | 2026-08-19 |
| 二项 CI | [Confidence intervals (Binomial)](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/binomial-capability-analysis/methods-and-formulas/confidence-intervals-and-bounds/) | 2026-08-19 |
| 二项图 | [Graphs (Binomial)](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/binomial-capability-analysis/methods-and-formulas/graphs/) | 2026-08-19 |
| 泊松能力统计 | [Capability statistics (Poisson)](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/poisson-capability-analysis/methods-and-formulas/capability-statistics/) | 2026-08-19 |
| 泊松 CI | [Confidence intervals (Poisson)](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/poisson-capability-analysis/methods-and-formulas/confidence-intervals-and-bounds/) | 2026-08-19 |
| 泊松图 | [Graphs (Poisson)](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/poisson-capability-analysis/methods-and-formulas/graphs/) | 2026-08-19 |
| Clopper–Pearson / 泊松 Garwood | [NIST 7.2.4](https://www.itl.nist.gov/div898/handbook/prc/section2/prc24.htm) | 2026-08-19 |
| DOE Contour | [Overview for Contour Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/using-fitted-models/how-to/contour-plot/before-you-start/overview/) | 2026-08-19 |
| DOE Contour 设置 | [Specify the settings for Contour Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/using-fitted-models/how-to/contour-plot/perform-the-analysis/specify-settings/) | 2026-08-19 |
| DOE Surface | [Overview for Surface Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/using-fitted-models/how-to/surface-plot/before-you-start/overview/) | 2026-08-19 |
| ANOVA 区间图 | [Data plots for One-Way ANOVA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/one-way-anova/interpret-the-results/all-statistics-and-graphs/data-plots/) | 2026-08-19 |
| ANOVA 残差图 | [Residual plots for One-Way ANOVA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/one-way-anova/interpret-the-results/all-statistics-and-graphs/residual-plots/) | 2026-08-19 |
| 图形属性 | [Graph attributes](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-editing-basics/edit-attributes-for-fills-fonts-and-lines/) | 2026-08-19 |

## 2. 二项过程能力

对标 Stat > Quality Tools > Capability Analysis > Binomial。数据为各子组不合格品数 \(D_i\) 与检验数 \(N_i\)。Sixpack **仍只正态**，本分析不改 Sixpack。

### 2.1 点估计（Minitab capability statistics）

```text
p̄ = Dtot / Ntot
%Defective = 100 · p̄
PPM Defective = 10^6 · p̄
Process Z = Φ⁻¹(1 − p̄)     仅当 0 < p̄ < 1，否则 not_computed
累计 %Defective_j = 100 · (Σ_{i=1..j} D_i) / (Σ_{i=1..j} N_i)
```

其中 `Dtot = Σ D_i`，`Ntot = Σ N_i`。

### 2.2 Average P 的 95% CI（Clopper–Pearson，Minitab v1–v4）

Minitab 记号：`v1 = 2 Dtot`，`v2 = 2(Ntot − Dtot + 1)`，`v3 = 2(Dtot + 1)`，`v4 = 2(Ntot − Dtot)`。等价 F 形式（NIST / Clopper–Pearson）：

```text
D = Dtot, N = Ntot, α = 0.05（默认）
若 D = 0：p_L = 0
否则：p_L = 1 / (1 + (N − D + 1)/D · F_{1−α/2}(v2, v1))
若 D = N：p_U = 1
否则：p_U = 1 / (1 + (N − D)/((D + 1) · F_{1−α/2}(v3, v4)))
```

`%Defective` CI = `100 · p` CI；PPM CI = `10^6 · p` CI。  
Process Z 的 CI 对 p 上下限取 `Φ⁻¹(1 − p)`（方向对调：p 越大 Z 越小）。

### 2.3 表/图合同

- 过程数据：子组数、Dtot、Ntot、N*、Average P。
- 能力表：%Defective / PPM / Process Z 及 95% CI。不输出 Cp/Cpk。
- 累计 %Defective 图：点为累计率；参考线为 %Defective 与 CI 上下限。
- P 图：复用现有 `ControlCharts::p_chart`。

Target：`specifications.target` 钉为不合格品**比例** \((0,1]\)。输入落在 `(1,100]` 时按百分数除以 100，并诊断 `target_interpreted_as_percent`。`≤0` 或 `>100` 诊断越界，不把目标写成达标。

边界：无有效行、负数、非整数、\(D_i > N_i\)、检验数 = 0 → 只诊断，不造指数。complete-case 行主序；`*`/空计入 N*。

## 3. 泊松过程能力

对标 Capability Analysis > Poisson。数据为各子组缺陷数 \(D_i\) 与单位/机会数 \(N_i\)。

```text
Mean DPU = Dtot / Ntot
Mean Defective = Dtot / N          （N = 子组数）
Min/Max DPU = min/max(D_i / N_i)
累计 DPU_j = (Σ_{i=1..j} D_i) / (Σ_{i=1..j} N_i)
```

均值 CI 用 Garwood χ²（NIST 7.2.4；Dtot = 0 时下限 0）：

```text
Mean DPU 下限 = χ²_{α/2, 2 Dtot} / (2 Ntot)
Mean DPU 上限 = χ²_{1−α/2, 2(Dtot+1)} / (2 Ntot)
Mean Defective 用 N 替换 Ntot
```

表：过程数据 + Mean DPU / Mean Defective / Min / Max + CI。图：累计 DPU + U 图。Target 为非负 DPU。解释不写过程合格。

## 4. DOE 等值线 / 曲面图

对标 Contour Plot / Surface Plot（需已拟合模型）。本轮用已有二水平主效应 + 两因子交互系数，**不**改 `predict_response` 的 ±1 门，**不**重做响应优化几何平均 D。

编码空间（其余因子 hold = 0，对标 Minitab 连续因子取均值）：

```text
ŷ(x) = b0 + Σ b_i x_i + Σ b_ij x_i x_j    x ∈ [-1, 1]
```

默认 25×25 规则网格。≥3 因子：前两个因子作轴，其余 hold 0 并诊断。1 因子：不画等值线。二水平无平方项：诊断 `factorial_contour_no_quadratic`（双线性面，不能表示曲率）。区组项在 hold=0 处取参考区组。

等值线复用 `PlotKind::contour` 填色格子（不新做真等值线描边）。曲面为同一网格的静态等轴测线框（`PlotKind::surface`），不可旋转、不拖拽。

## 5. 单因素 ANOVA 区间图 + 残差图

Minitab One-Way 区间图是**各组均值的个体 CI**，使用 ANOVA 的 pooled \(S=\sqrt{\mathrm{MSE}}\)，不是图形菜单各组自己的 s，也不是 Tukey 同时区间。

```text
CI_i = ȳ_i ± t_{1−α/2, ν_e} · √(MSE / n_i)
```

`ν_e = 0` 或 MSE 不可用：不画区间、不填伪 CI。Tukey 表头、同时置信、含 0 判定不改。

残差图：残差-拟合、残差-顺序、残差正态概率；散点加 y=0。顺序图 X 为 complete-case **输入行序**（不是按组标签排序拼接）。直方图本轮不做。

## 6. 侧栏系列线型 / 点样式

Minitab 图属性可改系列线型与点样式。颜色与线宽已写回 `series[].style`。本轮侧栏补 `line_style` / `point_style`，文案与对话框一致（实线/虚线/点线/点划线；无/圆/方/三角/十字）。`point_style != None` 时 `show_points = true`。不做注释、拖拽、拼版。
