# PCA / 非参数 / 等方差 / 图表刻度 Auto

> 研究日期：2026-08-18
> 访问日期：2026-08-18（UTC+8）
> 本文只记录官方公式与本轮实现边界。公式参考测试不是 Minitab 导出，不得写成数值对齐。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| PCA 系数/得分/特征值/Mahalanobis | [Principal Components Analysis methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/principal-components/methods-and-formulas/methods-and-formulas/) | 2026-08-18 |
| PCA 解读 | [Interpret PCA](https://support.minitab.com/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/principal-components/interpret-the-results/all-statistics-and-graphs/) | 2026-08-18 |
| Mann-Whitney | [Mann-Whitney methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/mann-whitney-test/methods-and-formulas/methods-and-formulas/) | 2026-08-18 |
| Mann-Whitney W 与 p | [Calculating Mann-Whitney statistics](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/supporting-topics/calculating-mann-whitney-statistics/) | 2026-08-18 |
| Kruskal-Wallis | [Kruskal-Wallis methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/kruskal-wallis-test/methods-and-formulas/methods-and-formulas/) | 2026-08-18 |
| 1-Sample Wilcoxon | [1-Sample Wilcoxon methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/1-sample-wilcoxon/methods-and-formulas/methods-and-formulas/) | 2026-08-18 |
| 等方差 / Levene | [Test for Equal Variances methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/test-for-equal-variances/methods-and-formulas/methods-and-formulas/) | 2026-08-18 |
| Levene 计算演示 | [Calculate Levene's test](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/supporting-topics/anova-calculations/calculate-levene-s-test/) | 2026-08-18 |
| 连续刻度 | [Continuous scale](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-framework-and-scale/continuous-scale/) | 2026-08-18 |
| 图形编辑 | [Edit graphs](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-editing-basics/edit-graphs/) | 2026-08-18 |

`formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 2. PCA

官方系数是协方差/相关矩阵的特征向量 V（载荷图用系数，约 [-1, 1]）。得分 `Z = YV`：相关矩阵方法中 Y 为标准化数据，协方差方法中 Y 为中心化原始数据。

```text
R = V D V'                         （或 S = V D V'）
Z = Y V
Proportion_k = λ_k / Σ_{i=1}^p λ_i
Cumulative_k = Σ_{j=1}^k Proportion_j
```

解释率必须用**全部** p 个特征值作分母。先截断再求和会把保留成分抬到 100%，与官方口径不符。

DataLab 另输出相关载荷 `Loading_j = v_j √λ_j`，表题不得冒充官方「系数」。

T²/Q **不是** Minitab PCA 标准表（官方离群图用 Mahalanobis）。本轮 Jackson 风格诊断：

```text
T_i² = Σ_{a=1}^k t_ia² / λ_a
Q_i  = ||x_i||² − Σ_{a=1}^k t_ia²
限   = 样本 T²/Q 的经验分位（anomaly_quantile，默认 0.99）
```

诊断码 `empirical_anomaly_quantile`。不实现 Jackson–Mudholkar θ 解析限，也不伪造 T² 控制图 UCL。全成分且 S 可逆时 T² = Mahalanobis。不收敛、完整行不足、常量列只诊断，不填 T²=0。

解释：T²/Q 超限 ≠ 过程合格/失控。

## 3. 非参数

并列值一律平均秩。正态/χ² 近似；精确表不做。

### Mann-Whitney

```text
W = 第一组秩和
E(W) = n1(n1+n2+1)/2
Var(W) = n1 n2 (N+1)/12
Var_ties = n1 n2 / 12 · [N+1 − Σ(t³−t)/(N(N−1))]
Z = (W − E(W) − c) / √Var        c = ±0.5 连续性修正
```

有 ties 时同时给出调整 P 与未调整 P。`n1` 或 `n2` < 10 时小样本警告。McKean–Ryan CI 不实现。位置差为 pairwise 差的中位数（Hodges–Lehmann），不是 Minitab golden。

### Wilcoxon 符号秩（配对差）

官方 1-sample 用 Walsh 平均数显示 W。本轮保持 W+/W-（与 Walsh 计数等价检验），不改写成 Walsh 显示以免假对齐。零差剔除；结用 midrank；方差减去 `Σ(t³−t)/48`；连续性修正 0.5；非零差 n<10 警告。

### Kruskal-Wallis

```text
H = 12/(N(N+1)) Σ_j R_j²/n_j − 3(N+1)
H(adj) = H / [1 − Σ(t³−t)/(N³−N)]
Z_j = (R̄_j − (N+1)/2) / sqrt( (N+1)(N−n_j)/(12 n_j) )
```

有 ties 时 Z 的分母乘以与 H(adj) 相同的结修正平方根。P 用 χ²(k−1)。任一 n_j<5 警告。未拒绝 ≠ 已证明各组分布相同。

## 4. 等方差 / Levene

Minitab「Levene」是 Brown–Forsythe：组中位数绝对偏差再做单因素 ANOVA。

```text
Z_ij = |Y_ij − median(group_j)|
F = MS_between(Z) / MS_within(Z)
```

DataLab：`levene` → 中位数；`levene_mean` 保留 1960 均值版；缺 JSON 字段仍默认 `f`。k 组用测量列 + 分组列。一方差仍为 χ²。两列无分组时输出 F + Levene。

不做 Bonett、多重比较区间、Bartlett。组内偏差全 0 只诊断，不伪造 F=1。未拒绝 ≠ 已证明方差相等。

## 5. 图表刻度 Auto

Minitab Continuous scale：Min / Max 可分别取消 Auto。`ChartModel` 已有独立可选 `y_min`/`y_max`/`x_min`/`x_max`。对话框四个 Auto 勾选；「清除 Y/X 范围」把该轴 optionals 清空。两侧均手动时要求 min < max。不做注释、图元拖拽、多图 Layout Tool。
