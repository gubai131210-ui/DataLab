# SPC Test 7 / 正态能力直方图 / 卡方关联 / Grubbs 异常值检验

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> 本文只记录官方公式、Minitab 表形/图名与本轮实现边界。`formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 控制图检验 | [Tests for special causes](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-subgroups/xbar-r-chart/interpret-the-results/all-statistics-and-graphs/) | 2026-08-20 |
| NIST SPC 通则 | [NIST 6.3.2 What are Control Charts?](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm) | 2026-08-20 |
| 正态过程能力 | [Normal Capability methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/methods-and-formulas/methods/) | 2026-08-20 |
| 能力直方图图 | [All statistics and graphs for Normal Capability](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/interpret-the-results/all-statistics-and-graphs/) | 2026-08-20 |
| 卡方关联 | [Methods and formulas for Chi-Square Test for Association](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/how-to/chi-square-test-for-association/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| 列联表输出 | [All statistics and graphs for Chi-Square Test for Association](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/how-to/chi-square-test-for-association/interpret-the-results/all-statistics-and-graphs/) | 2026-08-20 |
| NIST 卡方独立性 | [NIST 1.3.5.15 Chi-Square Test](https://www.itl.nist.gov/div898/handbook/eda/section3/eda35f.htm) | 2026-08-20 |
| Minitab Outlier Test | [Methods and formulas for Outlier Test](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/outlier-test/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| Minitab Outlier 解读 | [Interpret all statistics and graphs for Outlier Test](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/outlier-test/interpret-the-results/all-statistics-and-graphs/) | 2026-08-20 |
| NIST Grubbs | [NIST 1.3.5.17.1 Grubbs' Test](https://www.itl.nist.gov/div898/handbook/eda/section3/eda35h1.htm) | 2026-08-20 |

本轮 **不改** Test 1–6/8 判定、Cp/Cpk/Pp/Ppk 公式、Pearson/似然比卡方核、Sixpack 仅正态。不做 Dixon、功效与样本量、Mosaic 图。对照表形，不填写未导出对照数值。

## 2. I-MR / Xbar Test 7 边界

Nelson / Minitab Test 7：连续 15 点位于中心线两侧的 1σ **内**。NIST/缺口矩阵口径：

```text
点 i 计入“1σ 内”  ⇔  σ_i > 0 且  |y_i − CL_i| < σ_i
恰好 |y − CL| = σ  不计入“内”，该 15 点窗口不触发 Test 7
```

σ_i 优先用 `point_sigma`；缺省时用 `(UCL − CL) / 3`（Xbar 即 A2·R̄ / 3）。  
I 图与 Xbar 图适用 Test 1–8；R/S/MR 仍仅 Test 1–4。

| 输出 | 合同 |
|---|---|
| 失败点集 | `special_cause_points[6]` 为 Test 7 窗口下标 |
| 服务层图 | I-MR 的 I 图 / Xbar-R 的 Xbar 图 `triggered_tests` 含 7 当且仅当窗口全严格 `<σ` |
| `source_row` | 控制图点对应原始行；子组图为子组首行 |

解释不把 Test 7 写成过程合格或控制图通过。

## 3. 正态能力直方图参考线

对标 Capability Analysis (Normal) 直方图：规格虚线 + Within/Overall 正态曲线。指数公式不改：

```text
Cp  = (USL − LSL) / (6 σ_within)
Cpk = min((μ − LSL)/(3 σ_within), (USL − μ)/(3 σ_within))
Pp  = (USL − LSL) / (6 σ_overall)
Ppk = min((μ − LSL)/(3 σ_overall), (USL − μ)/(3 σ_overall))
```

| 输出 | 合同 |
|---|---|
| LSL / USL / Target | 仅配置了的规格画虚线并标注；单侧缺席的一侧不画 |
| Within 曲线 | N(μ, σ_within)；组间/组内能力用 σ_BW（与 Potential/BW Cp 同口径） |
| Overall 曲线 | N(μ, σ_overall) |
| 图例 | Within（灰虚线）、Overall（红实线） |
| Sixpack | 复用同一直方图 `PlotSpec`；Sixpack 仍强制 `capability_method=normal` |
| 悬停 | `values` 与 `source_rows` 等长 |

密度缩放保持现有 `φ(x; μ, σ) × n × 组距` 口径，本轮只补标签与图例。解释不写合格 / Cpk 通过。Johnson / 非正态直方图不在本轮改合同。

## 4. 卡方关联输出合同

对标 Stat > Tables > Chi-Square Test for Association。领域公式不改：

```text
E_ij = R_i C_j / N
Pearson χ² = Σ (O_ij − E_ij)² / E_ij
G² = 2 Σ O_ij ln(O_ij / E_ij)     （O_ij > 0）
df = (r − 1)(c − 1)
```

E_ij < 1 时不显示 P 值（诊断 `expected_count_below_one`）。

| 输出 | 合同 |
|---|---|
| 观察频数 | 行×列表 + 行/列合计 |
| 卡方检验 | Pearson χ²、DF、P；Likelihood Ratio χ²、P |
| 单元格统计 | Observed / Expected / Raw / Standardized / Adjusted Residual / Contribution |
| N / N* | 两侧分类均非缺失的行计入 N；`*`/空计入 N*，不进列联表 |
| Facts | `ChiSquareFacts`；解释只陈述与独立性假设的一致程度，**不写因果** |
| 热图 | 观察频数 `PlotKind::heatmap`（行×列观察计数，无合计）；`ChiSquareFacts.plot_available`；**不是** Mosaic |

## 5. Grubbs 异常值检验

对标 Stat > Basic Statistics > Outlier Test（**只实现 Grubbs**，不做 Dixon）。假设：单变量近似正态、至多一个异常值。

双侧统计量（NIST / Minitab）：

```text
G = max_i |y_i − ȳ| / s
s = √( Σ(y_i − ȳ)² / (n − 1) )
```

单侧：检验最小值 `G = (ȳ − y_min)/s`；检验最大值 `G = (y_max − ȳ)/s`。

临界值（双侧，显著性 α）：

```text
t* = t_{α/(2n), n−2}
G_crit = ((n−1)/√n) √( t*² / (n−2 + t*²) )
```

单侧把 α/(2n) 换成 α/n。P 值由 G 反解 t，再乘样本点数（Minitab；上界近似，钳制到 [0, 1]）：

```text
t = G √( n(n−2) / ((n−1)² − n G²) )     （分母 ≤ 0 时 G 达上界，P → 0）
P_one  = n (1 − F_{t_{n−2}}(t))
P_two  = min(1, 2 P_one)
```

| 输出 | 合同 |
|---|---|
| 检验表 | N、N*、Mean、StDev、G、P、嫌疑值、方向（largest/smallest）、source_row |
| 个体值图 | `PlotKind::scatter`；x=观测序，y=测量；嫌疑点可打标；`source_rows` 等长 |
| 边界 | n < 3 或 s = 0 → `not_computed` 诊断，不伪造 P |
| 导入 | `extract_numeric_column` complete-case；`*` 计入 N* |
| Facts | `OutlierTestFacts`；`assumption_status=not_verified` |

解释：P ≤ α 只陈述拒绝“无异常值”假设并提示调查；**不写必须删除 / 已确认异常 / 已证明正态**。未拒绝不得写成数据中没有异常值。
