# Logistic 拟合优度 / 个体分布识别 / 组间组内能力 / 图表属性

> 研究日期：2026-08-18  
> 访问日期：2026-08-18（UTC+8）  
> 本文只记录官方公式与本轮实现边界。`formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Logistic HL / Pearson / Deviance | [Goodness-of-fit statistics](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-binary-logistic-model/methods-and-formulas/goodness-of-fit-statistics/) | 2026-08-18 |
| Logistic 拟合优度表形 | [Goodness-of-fit tests](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-binary-logistic-model/interpret-the-results/all-statistics-and-graphs/goodness-of-fit-tests/) | 2026-08-18 |
| IDI 方法与 AD | [IDI methods](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/individual-distribution-identification/methods-and-formulas/methods/) | 2026-08-18 |
| IDI 分布族 | [IDI distributions](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/individual-distribution-identification/methods-and-formulas/distributions/) | 2026-08-18 |
| 正态 AD | [Normality Test methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/normality-test/methods-and-formulas/methods-and-formulas/) | 2026-08-18 |
| 组间/组内 σ | [Between/Within methods](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/between-within-capability-analysis/methods-and-formulas/methods/) | 2026-08-18 |
| B/W 能力指数 | [Between/Within capability statistics](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-sixpack/between-within-capability-sixpack/interpret-the-results/all-statistics-and-graphs/capability-statistics/) | 2026-08-18 |
| 图形编辑 | [Edit graphs](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-editing-basics/edit-graphs/) | 2026-08-18 |
| 图形属性 | [Graph attributes](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-editing-basics/edit-attributes-for-fills-fonts-and-lines/) | 2026-08-18 |

## 2. Logistic Hosmer–Lemeshow

Minitab 按估计概率排序，尽量分成 10 组等计数。对第 k 组：

```text
E_k = Σ π_i          （组内拟合概率之和）
O_k = Σ y_i          （组内事件数）
n_k = 组内观测数
π̄_k = E_k / n_k
χ²_HL = Σ_k (O_k − E_k)² / [n_k π̄_k (1 − π̄_k)]
DF = g − 2           （g = 有效组数）
```

P 为 χ²(g−2) 右尾概率。DataLab 本轮只输出独立 **拟合优度** 表（检验 / 卡方 / DF / 组数 / P / 状态），不实现 Pearson/Deviance 整体 GOF 表。

**不计算条件**（`hosmer_lemeshow_status = not_computed`）：

- n < 20
- IRLS 未收敛
- 完全分离
- 任一组 n_k π̄_k (1−π̄_k) ≤ 1e−12
- 有效组数 g < 6

**影响点**：杠杆 h_ii > 2(p+1)/n 标为影响点（p = 预测变量数，不含截距）。不另造 Cook 距离表。

**解释契约**（ADR 0003）：HL P 只能写「在 α 下拒绝/未拒绝拟合不足」；未拒绝 ≠ 模型已充分；不写过程合格。

## 3. 个体分布识别（四族二参数）

本轮仅 Normal / Weibull / Lognormal / Exponential。**不**自动改写 `capability_method` / `nonnormal_distribution`。

### Anderson-Darling 公共核

设排序后 CDF 值为 Z_(1) < … < Z_(n)：

```text
A² = −n − (1/n) Σ_i [(2i−1) ln Z_(i) + (2n+1−2i) ln(1−Z_(i))]
```

计算前对 Z 做极小值裁剪避免 ln(0)。**禁止**为各分布另写一套求和循环；从 `normality_test` 抽出公共函数。

| 分布 | 参数估计 | CDF F(x) | p 值 |
|---|---|---|---|
| Normal | 无偏 x̄, s | Φ((x−μ)/σ) | Stephens 正态 A²* 分段式（现有 `normality_test`） |
| Lognormal | 对 ln(x) 用无偏 μ, σ | Φ((ln x−μ)/σ) | **复用**正态 Stephens p（官方：正态/对数正态用无偏估计） |
| Exponential | θ = x̄（全失效 MLE） | 1−exp(−x/θ), x>0 | D'Agostino–Stephens 指数族修正；无闭合式则 p=`not_computed` |
| Weibull | MLE（`fit_weibull`） | 1−exp(−(x/α)^β), x>0 | 同上 Weibull 族修正；无闭合式则 p=`not_computed` |

**非正值**：Lognormal / Weibull / Exponential 要求 x>0；含 ≤0 时该行 `not_computed` 并诊断。

**排序**：按 A² 升序输出拟合优度表；最小 AD 为表内最优，**不是**已证明服从该分布。

**概率图**：各候选一张 `PlotKind::probability`；正态用 Blom 位置 + Φ⁻¹；其余用各自 F 的反函数。

本轮不做三参数族、LRT、Gamma/极值/Logistic 等 Minitab 扩展族。

## 4. 组间/组内过程能力

需要**子组标识列**；无子组或 `build_strict_subgroups` 失败只诊断，不伪造 σ_between。

### 标准差估计（本轮默认）

```text
σ_within = R̄ / d2(n)                    （等长子组，n = 子组大小）
X̄_i      = 第 i 子组均值
MR_i     = |X̄_i − X̄_(i−1)|
σ_X̄      = MR̄ / d2(2)
σ²_B     = max(0, σ²_X̄ − σ²_within / n)
σ_BW     = sqrt(σ²_B + σ²_within)
σ_overall = 样本标准差（全观测）
```

**说明**：Minitab 默认 within 可用 pooled Sp；DataLab 本轮用 R̄/d2 与现有正态能力子组路径一致，文档标注为 formula_reference，非 Minitab golden。

σ²_B 被截断为 0 时诊断 `between_variance_truncated`。

### 能力指数

```text
Cp  = (USL − LSL) / (6 σ_BW)
Cpk = min((μ−LSL)/(3σ_BW), (USL−μ)/(3σ_BW))
Pp  = (USL − LSL) / (6 σ_overall)
Ppk = min((μ−LSL)/(3σ_overall), (USL−μ)/(3σ_overall))
```

Cp/Cpk 表题为 **Between/Within Capability**；Pp/Ppk 仍为 Overall。解释不写过程合格。

新命令 `between_within_capability`；现有 `capability` 默认 `capability_method=normal` 不变。

## 5. 图表属性页 UX

参考 Minitab「当前图属性可编辑、区域属性与默认分离」。本轮：

- 预览置于 Tab **右侧**（非底栏）
- 仅 `ChartKind::Control` 显示「参考线」Tab
- 系列色已在表内 `QPushButton` 点选；颜色列 `ResizeToContents`
- 工作模型仍为 `ChartModel`；确认后写回 `PlotSpec`

不做注释、拖拽布局、多图拼版。
