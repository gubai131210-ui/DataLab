# 品质工作站统计方法规范

本文档定义 DataLab 品质工作站的计算口径。实现和测试必须以本文档为准；界面显示的小数位不影响内部精度。

对照来源：

- [I-MR Estimate options](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/i-mr-chart/perform-the-analysis/i-mr-options/specify-estimation-options/)
- [Xbar-R methods](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-subgroups/xbar-r-chart/methods-and-formulas/r-chart/)
- [Normal Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/before-you-start/overview/)
- [Display Descriptive Statistics](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/display-descriptive-statistics/before-you-start/overview/)
- [Laney P' methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/attributes-charts/laney-p-chart/methods-and-formulas/methods-and-formulas/)
- [Laney U' methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/attributes-charts/laney-u-chart/methods-and-formulas/methods-and-formulas/)
- [Overdispersion and underdispersion](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/understanding-attributes-control-charts/overdispersion-and-underdispersion/)
- [Tests for special causes](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/basics/using-tests-for-special-causes/)
- [Analyze Factorial Design ANOVA methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-factorial-design/methods-and-formulas/analysis-of-variance/)
- [Nested Gage R&R ANOVA methods](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/nested-gage-r-r-study/methods-and-formulas/anova-table/)
- [Attribute Agreement Analysis overview](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/supporting-topics/attribute-agreement-analysis/what-is-an-attribute-agreement-analysis-also-called-attribute-gage-r-r-study/)
- [Principal Components Analysis methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/principal-components/methods-and-formulas/methods-and-formulas/)

## 缺失值

空单元格、`*`、`NA`、`N/A`、`NaN` 视为缺失。描述统计中：

- `N`：非缺失数值个数
- `N*`：缺失个数
- 分析默认跳过缺失，不从原始数据删除

## 描述统计

对于有效观测 `x_1 ... x_n`：

- Mean = `sum(x_i) / n`
- StDev = 样本标准差 `s = sqrt(sum((x_i-mean)^2) / (n-1))`，`n >= 2`
- SE Mean = `s / sqrt(n)`
- 四分位使用顺序统计量的线性插值（位置 `p * (n-1)`）

By 变量：按分类水平分别计算上表。

补充输出：

- Variance = `s²`
- IQR = `Q3 - Q1`
- Range = `Maximum - Minimum`
- Sum = `Σx_i`
- Skewness 和 Excess Kurtosis 在 `n` 足够且样本方差大于零时使用有限样本修正；
  `n` 不足或常量列显示为不可用，不填充 0。
- 输出表同时回显 `N`、`N*` 和缺失处理口径。

## 相关分析

相关分析默认使用 Pearson，也可以选择 Spearman。每一对变量采用 pairwise
有效观测；缺失值不改变工作表，并保留有效配对的原始行映射。

### Pearson 相关

```text
r = Σ[(x_i-x̄)(y_i-ȳ)] / sqrt(Σ(x_i-x̄)² * Σ(y_i-ȳ)²)
t = r * sqrt((n-2)/(1-r²))
DF = n - 2
```

相关性检验为 `H0: ρ = 0`，默认双侧 p 值为 t 分布尾概率。
置信区间使用 Fisher z 变换：

```text
z = atanh(r)
SE(z) = 1 / sqrt(n-3)
z_L/U = z ± z_(1-alpha/2) * SE(z)
r_L/U = tanh(z_L/U)
```

当有效配对数不足、任意变量为常量或置信水平非法时返回诊断。

### Spearman 秩相关

先将每列转换为秩；并列值使用平均秩，然后对秩执行 Pearson 公式。
这使 Spearman 能够检测单调关系，而不要求关系是线性的。

## t 检验

默认置信水平为 95%，默认备择假设为双侧。输出 Mean、StDev、SE Mean、
t、DF、P-Value 和均值差置信区间。

### 单样本 t

```text
d = x̄ - μ₀
SE = s / sqrt(n)
t = d / SE
DF = n - 1
```

双侧置信区间为 `d ± t_(1-alpha/2, DF) * SE`。单侧检验使用对应方向的
t 分布尾概率和单侧置信界限。样本标准差为 0 且差值不为 0 时不生成虚假
无限 t 值，而是返回明确诊断。

### 双样本 t

Welch 方法默认不假设两组方差相等：

```text
d = x̄₁ - x̄₂
SE = sqrt(s₁²/n₁ + s₂²/n₂)
DF = (s₁²/n₁ + s₂²/n₂)²
     / [(s₁²/n₁)²/(n₁-1) + (s₂²/n₂)²/(n₂-1)]
t = d / SE
```

选择 pooled 时：

```text
s_p² = [(n₁-1)s₁² + (n₂-1)s₂²] / (n₁+n₂-2)
SE = s_p * sqrt(1/n₁ + 1/n₂)
DF = n₁ + n₂ - 2
```

两种方法均使用 `H0: μ₁-μ₂ = 0`，并输出差值的置信区间。

## 单因素 ANOVA

设有 `k` 个组，第 `j` 组样本量为 `n_j`、均值为 `ȳ_j`，总样本量为 `N`，
总均值为 `ȳ`：

```text
SS_Between = Σ_j n_j(ȳ_j-ȳ)²
SS_Error   = Σ_j Σ_i (y_ij-ȳ_j)²
SS_Total   = SS_Between + SS_Error

DF_Between = k - 1
DF_Error   = N - k
DF_Total   = N - 1
MS_Between = SS_Between / DF_Between
MS_Error   = SS_Error / DF_Error
F          = MS_Between / MS_Error
```

总体检验为 `H0: 所有组均值相等`，p 值为 F 分布右尾概率。
输出包括组均值表和 ANOVA 表。当前批次只做总体 F 检验，不自动声称具体
哪两组不同；后续可增加 Tukey 多重比较，并同时控制家族错误率。

## I-MR

默认移动极差长度 `w = 2`。

- 单值中心线：`X̄`
- 移动极差：窗口 `w` 内 `max - min`（`w=2` 时即 `|x_i - x_{i-1}|`）
- `MR̄`：用于估计的移动极差平均值（可选中位数）
- `σ_within = MR̄ / d2(w)`，`d2(2) = 1.128`
- I 图控制限：`X̄ ± 3 σ_within`
- MR 图中心线：`MR̄`；UCL = `D4(w) * MR̄`；LCL = `D3(w) * MR̄`
- Test 1：点落在控制限外

## Xbar-R

子组大小 `n` 使用 ASTM/AIAG 常数表（2–25）：

- `σ_within = R̄ / d2(n)`
- Xbar：`X̿ ± A2(n) * R̄`，其中 `A2 = 3 / (d2 * sqrt(n))`
- R 图：中心线 `R̄`，LCL = `D3 * R̄`，UCL = `D4 * R̄`
- `D3 = max(0, 1 - 3 d3/d2)`，`D4 = 1 + 3 d3/d2`

数据可按固定子组大小切分，或按子组标识列聚合。

## P 图

- `p̄ = 总不合格品数 / 总检验数`
- 第 i 个子组：`UCL/LCL = p̄ ± 3 sqrt(p̄(1-p̄)/n_i)`，并截断到 `[0, 1]`
- Test 1：点出限

## Xbar-S

对每个等大小子组计算均值 `x̄_i` 和样本标准差 `s_i`：

- `S̄ = mean(s_i)`
- `c4(n) = sqrt(2/(n-1)) * Γ(n/2) / Γ((n-1)/2)`
- `σ_within = S̄ / c4(n)`
- Xbar 控制限：`X̿ ± A3(n) * S̄`，`A3 = 3/(c4*sqrt(n))`
- S 图控制限：`B3*S̄`、`B4*S̄`
- `B3 = max(0, 1 - 3*sqrt(1-c4²)/c4)`，`B4 = 1 + 3*sqrt(1-c4²)/c4`

Xbar-R 适合子组大小不超过 8 的数据；更大的子组使用 Xbar-S。DataLab
支持固定子组大小和子组标识列两种输入方式。子组控制图采用严格输入：
测量列存在缺失/非法值、固定子组存在尾部不完整子组、标签子组大小不一致时，
分析返回诊断而不自动拼接或补齐数据。

## 正态性检验与正态概率图

正态性检验使用 Anderson-Darling。设有效样本量为 `n`，样本均值为 `x̄`，
样本标准差为 `s`，排序后观测为 `x_(1) ≤ ... ≤ x_(n)`：

```text
z_i = (x_(i) - x̄) / s
F_i = Φ(z_i)
A²  = -n - (1/n) Σ_{i=1..n} [(2i-1) ln(F_i) + (2n+1-2i) ln(1-F_i)]
```

其中 `Φ` 为标准正态 CDF。计算前对 `F_i` 做极小值裁剪以避免 `ln(0)`。
DataLab 同时输出原始 `A²` 与 Stephens 修正 `A²*`，并用分段近似求 p 值。
判定为 `reject` / `fail_to_reject` / `not_computed`：`p < alpha` 时拒绝正态假设；
`p ≥ alpha` 时输出“未拒绝正态假设”，不得写成“数据服从正态分布”。
契约：`n < 3` 或 `s = 0` 不计算 AD；`3 ≤ n < 8` 仍计算但发出 `sample_size_warning` /
`assumption_not_verified`。默认 `alpha = 0.05`。p 值限制在 `[0,1]`。

正态概率图默认使用 Blom 作图位置（代码中 `index` 从 0 起算，等价于 1-based 的
`i - 0.375`）：

```text
p_i = (i + 0.625) / (n + 0.25)     （i = 0,1,...,n-1）
y_i = Φ⁻¹(p_i)
```

Minitab 概率图位置方法可配置为 median rank，手工对照时必须保持位置方法一致。

## NP、C、U 图

- NP 图：中心线 `n_i * p̄`，控制限 `n_i*p̄ ± 3*sqrt(n_i*p̄*(1-p̄))`，LCL 截断到 0
- C 图：中心线 `c̄ = mean(c_i)`，控制限 `c̄ ± 3*sqrt(c̄)`，LCL 截断到 0
- U 图：`u_i = c_i/n_i`，中心线 `ū = Σc_i/Σn_i`，控制限 `ū ± 3*sqrt(ū/n_i)`，LCL 截断到 0

P、NP 图把观测对象分为合格/不合格两类，遵循二项分布；C、U 图统计一个
单位中可以重复出现的缺陷数，遵循泊松分布。P 图显示不合格品率，NP 图显示
不合格品数；U 图显示单位缺陷数，C 图显示每个固定大小样本的缺陷数。

四种图均默认使用 Test 1：当绘制点低于 LCL 或高于 UCL 时标记为超限点。
P、U 图的控制限可以随子组分母变化；NP 图的中心线和控制限也随子组大小变化；
C 图要求各子组单位数相同。输出页除控制图外，还提供逐子组表格，包含原始计数、
分母、绘制值、中心线、LCL、UCL 和 Test 1 状态。

严格输入：计数与分母必须是非负整数；分母不能为 0；P/NP 要求
`defectives ≤ inspected`。非法输入返回诊断，不静默修正。

## Laney P' / U' 图

Laney 图在传统 P/U 图基础上，用标准化点的相邻移动极差估计额外离散
（overdispersion / underdispersion）。无偏常数 `d2(2) = 1.128`。

### Laney P'

设第 `i` 个子组不合格品数为 `d_i`、检验数为 `n_i`：

```text
p_i = d_i / n_i
p̄   = Σd_i / Σn_i          （或由历史中心线指定）
Z_i = (p_i - p̄) / sqrt(p̄(1-p̄)/n_i)
MR_i = |Z_i - Z_(i-1)|     （i ≥ 2）
MR̄  = mean(MR_i)
Sigma Z = MR̄ / 1.128       （或由历史 Sigma Z 指定）

LCL_i = max(0, p̄ - 3 * SigmaZ * sqrt(p̄(1-p̄)/n_i))
UCL_i = min(1, p̄ + 3 * SigmaZ * sqrt(p̄(1-p̄)/n_i))
```

### Laney U'

设第 `i` 个子组缺陷数为 `c_i`、单位数为 `n_i`：

```text
u_i = c_i / n_i
ū   = Σc_i / Σn_i          （或由历史中心线指定）
Z_i = (u_i - ū) / sqrt(ū/n_i)
MR_i = |Z_i - Z_(i-1)|
MR̄  = mean(MR_i)
Sigma Z = MR̄ / 1.128       （或由历史 Sigma Z 指定）

LCL_i = max(0, ū - 3 * SigmaZ * sqrt(ū/n_i))
UCL_i = ū + 3 * SigmaZ * sqrt(ū/n_i)
```

### 离散解释

- `Sigma Z ≈ 1`：Laney 控制限与传统 P/U 图接近。
- `Sigma Z > 1`：过度离散，传统控制限偏窄，易产生过多误报。
- `Sigma Z < 1`：欠离散，传统控制限偏宽。
- 若无法估计移动极差（有效子组不足或全部 Z 相同导致 `MR̄ = 0` 且未提供历史
  Sigma Z），返回明确诊断。

### 输出口径

Laney 输出包含：`p̄/ū`、`Sigma Z`、`MR̄(Z)`、逐子组 `p_i/u_i`、`Z_i`、`MR_i`、
中心线、LCL、UCL、阶段标签、Test 1–4 失败状态，以及“历史参数 / 估计”方法回显。

## 特殊原因测试（Test 1–4）

默认启用 Test 1；Test 2–4 由分析配置显式启用。结果模型分别记录每个测试失败
的点集，而不是只保留合并后的异常点。

| 测试 | 规则 |
|------|------|
| Test 1 | 一点超出控制限：`y_i < LCL_i` 或 `y_i > UCL_i` |
| Test 2 | 连续 9 点在中心线同侧（严格大于或严格小于中心线） |
| Test 3 | 连续 6 点持续上升或持续下降 |
| Test 4 | 连续 14 点上下交替 |

图面显示策略与 Minitab 一致：同一点触发多个测试时，点标签使用**最小测试编号**；
悬停与统计表列出完整失败测试集合。

## 阶段与历史参数

- **阶段列**：可选文本/分类列。阶段标签缺失时报告原始行号并中止分析。
- **阶段切换**：图上可在阶段边界显示分隔线；参数表按阶段回显中心线/控制限来源。
- **历史中心线** / **历史 Sigma Z**：只替代估计，不改写原始计数；未提供时自动估计。
- **行筛选**：`included_rows` 与 `excluded_rows` 互斥；排除只影响估计与绘图，不修改工作表。
- **保留空位**：`leave_gaps_for_excluded = true` 时图上保留空位，源行号不重排。

## 正态过程能力

规格限 LSL/USL/Target 由工程输入，不从样本推导。每个存在的限必须为有限数。

- 双侧规格要求 `LSL < USL`；`LSL = USL` 返回 `invalid_specification`，不输出指数。
- 仅 LSL 时只计算 CPL/PPL；仅 USL 时只计算 CPU/PPU；另一侧保持空值。
- Within/Overall sigma 必须有限且 `> 0`，禁止输出无限 Cp/Cpk/Pp/Ppk。
- 观测先筛选有限值；观测 PPM 分母为有效 N；恰好等于规格限的值不计入超规。
- Target 缺失不计算 Cpm；Target 非有限返回 `invalid_target`，不能静默跳过。
- 领域层不假设过程已经稳定或正态，结果带 `assumption_status = not_verified`。
- 诊断码包括 `invalid_specification`、`invalid_target`、`invalid_within_sigma`、
  `invalid_overall_sigma`、`assumption_not_verified`。
- 双侧指数置信区间：Cp/Pp 为 χ² 尺度
  `C·√(χ²_{α/2,ν}/ν)` 至 `C·√(χ²_{1-α/2,ν}/ν)`；Cpk/Ppk 为 Bissell 近似
  `C ± z_{1-α/2} √(1/(9N)+C²/(2ν))`。缺规格侧保持空值。不是 Kushler–Hurwitz 全套。
  见 [`docs/adr/0008-capability-index-confidence-intervals.md`](adr/0008-capability-index-confidence-intervals.md)。

## Johnson 变换过程能力

Johnson 族与定义域见 [`docs/research/johnson-sarima-lognormal-formulas.md`](research/johnson-sarima-lognormal-formulas.md)。DataLab 用 Chou et al. (1998) 分位匹配，并在变换后用 Anderson–Darling p 值选择 **p 最大且 p > 0.10** 的 SB/SL/SU 变换。找不到合格变换时输出 `johnson_transform_not_found`，不填写伪造 Pp。规格限可变换时，对变换后数据按正态 overall 公式计算 Pp/Ppk；不报告 within Cp/Cpk（`within_not_applicable_after_johnson`）。规格限越出定义域时输出 `johnson_spec_outside_support`。数值是公式参考，不是 Minitab 导出。解释层不得写成「已正态」或「过程合格」。

## 非正态过程能力（Z-score）

对 Weibull 或 Lognormal 的 CDF `F`：

```text
Z.LSL = Φ⁻¹(F(LSL))
Z.USL = Φ⁻¹(F(USL))
Pp    = (Z.USL − Z.LSL) / 6
PPL   = −Z.LSL / 3
PPU   = Z.USL / 3
Ppk   = min(PPL, PPU)
```

不报告 Cp/Cpk。期望 PPM 由 `F(LSL)` 与 `1−F(USL)` 得到。不做 Gamma 或三参数分布。拟合未拒绝假设不等于已证明过程服从该分布。

## 过程能力 Sixpack

Sixpack 对个体数据同时展示 I 图、MR 图、能力直方图、正态概率图、最近 25 个观测趋势图和能力指标表。

- 正态概率图按排序后的观测值 `x_(i)` 与 Blom 作图位置
  `p_i = (i + 0.625) / (n + 0.25)` 计算。
- 理论分位数为标准正态分布分位数 `z_i = Φ⁻¹(p_i)`。
- 图中相关系数为 `corr(z_i, x_(i))`，用于辅助判断正态关系，不能替代正式正态性检验。
- I 图的 `σ_within` 必须与能力分析的 Within StDev 使用同一 MR-bar/d2 估计。
- 结论卡默认以 `Cpk/Ppk = 1.33` 作为调查提示阈值，不是已验证的过程合格结论。

子组大小 = 1 时，`σ_within` 必须与同一数据的 I-MR 默认估计一致（`MR̄/d2`）。
子组大小 > 1 时，`σ_within = R̄/d2`。
`σ_overall` 使用样本标准差 `s`。

```text
CPL = (mean - LSL) / (3 * sigma_within)
CPU = (USL - mean) / (3 * sigma_within)
Cpk = min(CPL, CPU)
Cp  = (USL - LSL) / (6 * sigma_within)

PPL/PPU/Ppk/Pp 将 sigma_within 换成 sigma_overall。
```

PPM（正态假设）：

- 期望低于 LSL：`1e6 * Φ((LSL-mean)/sigma)`
- 期望高于 USL：`1e6 * (1-Φ((USL-mean)/sigma))`
- 观测 PPM：有效样本中严格低于 LSL 或严格高于 USL 的比例 × 1e6。等于规格限不计超规。

`Φ` 使用 `0.5 * (1 + erf(z / sqrt(2)))`。

## 配对 t 检验、Tukey 和比例推断

配对 t 检验先对完整配对计算 `d_i = x_{1i} - x_{2i}`，再对差值执行单样本
t 检验：

```text
d̄ = Σd_i / n
s_d = sqrt(Σ(d_i-d̄)²/(n-1))
SE = s_d / sqrt(n)
t = d̄ / SE
DF = n - 1
```

两比例检验默认报告 separate-estimates 的正态近似；当样本计数适用时同时报告
Fisher 精确检验。列联表卡方输出 Pearson χ²、似然比 χ²、期望频数、原始残差、
标准化残差、调整残差和单元贡献。期望频数小于 1 时不显示近似 P 值。

ANOVA 后续比较输出每一对组均值差、标准误、同时置信区间、q 统计量和调整后
P 值，并复用总体 ANOVA 的误差均方和误差自由度。当前实现是
`conservative_sidak_t_studentized_range_approximation`，不是精确 Studentized Range。
区间包含 0 时不得标记为显著差异。输出回显族置信水平、个体置信水平、alpha、
误差 DF 和 MSE。不平衡样本或误差 DF 不足时返回结构化诊断。

## 线性回归

DataLab 的线性回归模型包含截距：

```text
y = Xβ + ε
β̂ = argminβ ||y-Xβ||²
SSE = Σ(y_i-ŷ_i)²
MSE = SSE / (n-p-1)
R² = 1 - SSE/SST
R²(adj) = 1 - [SSE/(n-p-1)]/[SST/(n-1)]
```

实现使用 QR 分解求解系数并拒绝秩亏设计矩阵。`error_df = N-p-1`；当
`error_df ≤ 0` 时仍可给出系数点估计，但不输出 t/F/P。内部标准化残差为
`e_i / (s√(1-h_ii))`；学生化残差为 `e_i / (s_(i)√(1-h_ii))`，其中 `s_(i)` 为
删除第 i 个观测后的残差标准差；删除学生化残差为 `(e_i/(1-h_ii)) / s_(i)`。
Durbin-Watson 只按用户输入顺序计算，不静默按时间重排。α=0.05 近似 dL/dU（15≤n≤100，1≤k'≤5）给出判定区；范围外标 `not_computed`。VIF>5 只作为共线性调查提示，不自动删列。
解释层不得只看 R² 或残差 AD p 值判定模型合格。

## Box-Cox 变换

Box-Cox 仅接受严格正值。令 `G` 为几何均值，标准化变换为：

```text
W = G ln(Y)                         λ = 0
W = (Y^λ - 1)/(λ G^(λ-1))           λ ≠ 0
```

DataLab 在 `[-5, 5]` 网格搜索使变换后样本标准差最小的 λ，并可将结果圆整到
常用的 `-2、-1、-0.5、0、0.5、1、2`。输出保留 lambda 搜索序列和变换后数据，
零值或负值返回诊断而不自动修正。

## 控制图特殊原因 Test 5–8

除 Test 1–4 外，控制图可按 Minitab 规则启用：

- Test 5：连续 3 点中至少 2 点位于同侧 2σ 外。
- Test 6：连续 5 点中至少 4 点位于同侧 1σ 外。
- Test 7：连续 15 点均位于中心线 1σ **内**，使用 `|y-CL| < σ`，等于 1σ 不计入。
- Test 8：连续 8 点均在中心线任一侧的 1σ 外（不要求上下交替）。

每个测试独立保留失败点集；同一点触发多个测试时，图面仍显示最小测试编号，
表格和悬停信息保留完整测试集合。

## 柏拉图图

类别按缺陷计数从高到低排序。设类别 `i` 的计数为 `Count_i`，总计数为
`Total = ΣCount_i`：

```text
Percent_i = 100 * Count_i / Total
CumPct_k  = 100 * Σ(i=1..k) Count_i / Total
```

累计百分比线用于判断前几个类别贡献了多少问题；它不是新的缺陷计数。
可选的 `Other` 阈值会**保留**累计百分比首次超过阈值的那一类，并将其后剩余类别合并为
`Other`，且 `Other` 固定显示在最后。

## 数据治理

- 原始数据永不修改。
- 排除行只作用于分析配置，工作表用底色标记。
- 报告必须回显方法、列、规格限和 σ 估计方法。

## 非参数检验

Mann–Whitney 检验将两组观测合并排序，并列值使用平均秩。第一组秩和为 `W`：

```text
E(W) = n1(n1+n2+1)/2
Var(W) = n1*n2(n1+n2+1)/12
Var_ties = n1 n2 / 12 · [N+1 − Σ(t³−t)/(N(N−1))]
Z = (W − E(W) − c) / √Var     c = ±0.5 连续性修正
```

存在 ties 时同时输出调整 P 与未调整 P。Wilcoxon signed-rank 对非零配对差值的绝对值排序并分别累计正负秩；结用 midrank，方差减去 `Σ(t³−t)/48`。统计量保持 W+/W-，不改写成 Walsh 显示。Kruskal–Wallis 对所有组联合排序，输出 `H` 和 ties 修正后的 `H(adj)`，组 Z 为

```text
Z_j = (R̄_j − (N+1)/2) / sqrt( (N+1)(N−n_j)/(12 n_j) )
```

Mann–Whitney / Wilcoxon / Kruskal–Wallis / Friedman / Sign / McNemar 服务层输出相应表与（适用时）箱线/个体值图，`source_row` 可追溯。Friedman 可选 `posthoc=nemenyi`（默认无后比较）。Sign 主 P 为二项精确。McNemar 使用 Edwards 连续性校正，且不改列联表卡方命令。

## EWMA 与 CUSUM

EWMA 递推为：

```text
Z_t = λx_t + (1-λ)Z_(t-1)
SE(Z_t) = σ sqrt[ λ/(2-λ) * (1-(1-λ)^(2t)) ]
UCL/LCL = μ ± k SE(Z_t)
```

Tabular CUSUM 使用：

```text
C+_t = max(0, C+_(t-1) + x_t - μ0 - K)
C-_t = max(0, C-_(t-1) + μ0 - x_t - K)
```

当 `C+ > hσ` 或 `C- > hσ` 时分别报告上侧或下侧信号。

## Crossed Gage R&R

Crossed Gage R&R 要求每个零件×操作员组合具有相同重复次数。ANOVA 方差分量
包含 Part、Operator、Part×Operator 和 Repeatability：

```text
Total Gage R&R = Repeatability + Reproducibility
Total Variation = Total Gage R&R + Part-To-Part
%Contribution = VarComp / Total Variation * 100
Study Var = 6 * SD
%Study Var = SD_component / SD_total * 100
%Tolerance = Study Var / (USL - LSL) * 100
ndc = floor(1.41 * SD_part / SD_gage_rr)
```

`ndc < 1` 时报告为 1；`ndc < 5` 只作为调查提示。负方差分量截断为 0 时保存
截断前值并发出 `negative_variance_component`。公差为 NaN/无穷/负数返回
`invalid_tolerance`；公差为 0 时不计算 %Tolerance。

非平衡设计、组合缺失、重复次数不足或总变异为 0 时返回诊断。

## 时间序列平滑

单指数平滑使用：

```text
S_t = αY_t + (1-α)S_(t-1)
```

双指数平滑使用 level/trend：

```text
L_t = αY_t + (1-α)(L_(t-1)+T_(t-1))
T_t = γ(L_t-L_(t-1)) + (1-γ)T_(t-1)
F_(t+m) = L_t + mT_t
```

输出 MAD、MSD、MAPE 和基于 `1.96 * 1.25 * MAD` 的预测区间。

## 双因素/因子 ANOVA

因子模型使用设计矩阵 `X` 表示主效应和交互效应：

```text
y = Xβ + ε
β̂ = argminβ ||y - Xβ||²
SS_Total = Σ(y_i - ȳ)²
SS_Error = Σ(y_i - ŷ_i)²
MS_term = SS_term / DF_term
F_term = MS_term / MS_Error
```

顺序平方和（Seq SS）按模型项加入顺序计算；调整平方和（Adj SS）在控制其余
模型项后计算。线性回归多预测变量时，每项输出 Seq SS 与 Adj SS；
秩亏或共线项不输出伪造 F/P。交互项由两个因子的编码列逐列相乘得到。设计矩阵秩亏、因子
水平不足或组合缺失时不强行产生 F 检验，并在输出中标记诊断。

Minitab 参考：[Regression ANOVA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-regression-model/methods-and-formulas/analysis-of-variance/)

## 回归残差诊断

回归残差定义为：

```text
e_i = y_i - ŷ_i
DW = Σ(i=2..n)(e_i - e_(i-1))² / Σ(i=1..n)e_i²
VIF_j = 1 / (1 - R²_j)
```

残差图按 Minitab 语义分别检查：残差与拟合值的随机性和等方差性、残差与
观测顺序的独立性、残差正态概率图的线性，以及残差与预测变量的非随机模式。
Cook's D、DFITS、杠杆值和删除学生化残差用于标记可能影响模型的观测。

## ARIMA 基础预测

首版非季节 ARIMA 使用差分后的序列：

```text
W_t = (1-B)^d Y_t
AR(1): W_t = c + φW_(t-1) + ε_t
MA(1): W_t = c + ε_t + θ ε_(t-1)
AIC = -2 log(L) + 2k
AICc = AIC + 2k(k+1)/(n-k-1)
BIC = -2 log(L) + k log(n)
```

候选模型按 AIC、AICc 或 BIC 选择；预测值使用最终参数递推计算，预测表同时
输出周期、Forecast、Lower 和 Upper。时间列乱序、重复时间点和缺失时间点不
自动修复，改为返回输入诊断。

Best ARIMA 网格为 p/q≤3、d≤max(`arima_differencing`,2) 的 AR/MA 候选（不含混合
ARMA 阶）。当前使用条件最小二乘与差分尺度高斯似然近似 AICc。Minitab ARIMA
使用迭代最小二乘与 back forecast（Meeker TSERIES），**不是** Kalman 状态空间
MLE；DataLab CSS 的最优阶/数值可能不同。对照测试见
`tests/fixtures/minitab/expected/arima_trend_golden.tsv`，最终 golden 须从
Minitab 导出。回归 ANOVA 的 Seq SS / Adj SS 与 Minitab Type I/III 对照见
`tests/fixtures/minitab/expected/regression_golden.tsv`（容差：系数/SS 相对 ≤1e-4，
AICc 绝对 ≤0.01，Forecast 绝对 ≤0.05）。

## 二元 Logistic 回归

二元响应使用 logit link：

```text
p_i = 1 / (1 + exp(-x_i'β))
logit(p_i) = log[p_i/(1-p_i)] = x_i'β
W_i = p_i(1-p_i)
β_new = β_old + (X'WX)^(-1)X'(y-p)
OddsRatio_j = exp(β_j)
```

模型使用 IRLS 迭代最大化二项分布似然。输出 Coef、SE Coef、Z、P、Odds Ratio
及其置信区间，并报告 Log-Likelihood、Deviance、AIC 和 BIC。完全分离、
准完全分离、概率溢出、设计矩阵秩亏或不收敛时返回诊断，不将发散系数解释为
稳定结果。

## 一方差与两方差检验

一方差检验的正态假设方法使用：

```text
χ² = (n-1)s² / σ0²
DF = n-1
```

两方差 F-test 使用：

```text
F = s1² / s2²
DF1 = n1-1
DF2 = n2-1
```

Levene/Brown–Forsythe 方法将每个观测转换为相对于组中位数的绝对偏差：
`Z_ij = |Y_ij - median(group_j)|`，再对 Z 做单因素方差分析。菜单方法 `levene`
使用该中位数口径；`levene_mean` 保留 1960 均值版。缺 JSON 字段仍默认 `f`。
F-test 只在近似正态时作为主要结论；Levene 用于更稳健的非正态比较。
k 组等方差使用测量列加分组列。Levene（中位数）为稳健默认。两样本可选 Bonett（Banga–Fox 修正峰度，SD 比区间；公式参考）。k≥2 可选 Bartlett（正态敏感 χ²）。单因素 Tukey 输出下限/上限列、差值区间图与 Grouping Information 字母（近似算法不变）。
未拒绝原假设不得写成已证明方差相等。零组内偏差只诊断，不伪造 F=1。

## 时间序列分解

固定季节周期 `m` 时，分解模型为：

```text
Additive: Y_t = Trend_t + Seasonal_t + Error_t
Multiplicative: Y_t = Trend_t * Seasonal_t * Error_t
Trend_t = b0 + b1*t
```

先用长度为 `m` 的中心移动平均提取趋势，再对加法模型计算
`Y_t - MovingAverage_t`，对乘法模型计算 `Y_t / MovingAverage_t`。按季节位置
取 raw seasonal 的中位数，并分别归一化到均值 0 或 1。预测值由趋势项和季节
指数重新组合得到。乘法模型要求响应值严格为正，周期不足两个完整季节时返回诊断。

## 2 水平全因子设计

每个因子使用编码水平 `-1` 和 `+1`。`k` 个因子的完整设计包含：

```text
N = 2^k
```

主效应使用高、低水平响应均值之差：

```text
Effect(A) = Mean(Y | A=+1) - Mean(Y | A=-1)
Coefficient(A) = Effect(A) / 2
```

交互项使用编码列乘积，例如 `AB = A * B`，并按同样方式计算效应。
随机运行顺序使用可复现种子；区组只改变运行分配，不改变标准设计顺序。
中心点不参与二水平效应估计，用于检查曲率和提供重复中心响应。

## Nested Gage R&R

嵌套设计要求每个部件只属于一个操作者，且各操作者下的部件数和重复数一致。
ANOVA 方差分量按嵌套模型的均方差估计，负的抽样方差分量截断为零，并在诊断中
报告。输出包括：

```text
%Contribution = 100 * VarianceComponent / TotalVariance
StudyVariation = 6 * sqrt(VarianceComponent)
%StudyVariation = 100 * StudyVariation / TotalStudyVariation
ndc = floor(1.41 * PartVariation / GageRRVariation)
```

公差百分比仅在输入 tolerance 大于零时计算。非平衡嵌套数据不强行套用平衡
ANOVA 公式。

## 属性一致性分析

评级数据按部件和评估者对齐。观察一致率为完全相同评级的比例；Kappa 使用：

```text
Kappa = (P_observed - P_expected) / (1 - P_expected)
SE(Kappa) 由渐近方差估计
CI = Kappa ± z_(1-alpha/2) * SE(Kappa)
```

报告评估者内一致性、评估者间两两 Cohen Kappa、与标准的一致性，以及在评估者不少于 3 人时的
Fleiss overall Kappa。空评级不进入分母，但缺失数量必须在诊断中显示。`P_expected = 1` 时
Kappa 不可识别，不计算无限标准误。重复次数不一致返回 `unbalanced_replicates`，不静默截断。
`kappa_weight_scheme` 默认 `none`（未加权 Cohen/Fleiss）。为 `linear` / `quadratic` 时两两
Cohen 与 vs-standard 使用 Cohen (1968) 加权 Kappa（`cohen_linear` / `cohen_quadratic`）；
Fleiss overall **保持未加权**并诊断 `fleiss_remains_unweighted`。评级须可映射为有序数值
等级，否则诊断 `ordinal_ratings_unranked` 并回退。这是 DataLab 可选加权 Cohen，**不是**
Minitab Attribute Agreement 默认输出（有序路径仍走 Kendall）。`ratings_are_ordinal` 默认
false；为 true 且互异数值等级不少于 3 时，另输出 Kendall W（无标准）和 Kendall τ_b（有标准）。
两水平返回 `kendall_requires_three_ordinal_levels`。拒绝 Kappa=0 或 Kendall 系数=0 不得写成
「已证明评估者一致」。临界值使用标准正态分位数。公式参考见
[`docs/research/p1_weighted_kappa_cohen.md`](research/p1_weighted_kappa_cohen.md)、
[`docs/research/kendall-exp2-lognormal3-formulas.md`](research/kendall-exp2-lognormal3-formulas.md)。

## Holt-Winters 季节性预测

加法误差模型使用：

```text
l_t = α(Y_t - s_(t-m)) + (1-α)(l_(t-1)+b_(t-1))
b_t = β(l_t-l_(t-1)) + (1-β)b_(t-1)
s_t = γ(Y_t-l_t) + (1-γ)s_(t-m)
```

乘法模型把减法项改为除法项。预测区间基于残差尺度和置信水平；滚动起点验证
只使用预测起点之前的数据，避免未来信息泄漏。准确度指标包括 MAD、MSD、MAPE、
RMSE 和相对朴素基线的 MASE。季节预测的 SARIMA 候选使用乘法多项式
`φ(B) Φ(B^s) ∇^d ∇_s^D Y_t = θ(B) Θ(B^s) ε_t` 的条件最小二乘（CSS），允许混合
p/q 与 P/Q（网格 p/q≤2、P/Q≤1）。这不是 Minitab 的迭代最小二乘 + back forecast。
固定参数 SARIMA 接口在参数不可容纳时必须返回诊断，禁止输出伪造预测。

## PCA 主成分分析

协方差 PCA 使用中心化矩阵 `S`，标准化 PCA 先按变量标准差缩放：

```text
S v_j = λ_j v_j
Coefficient_j = v_j
Loading_j = v_j * sqrt(λ_j)
Score = X_centered * v_j
Proportion_k = λ_k / Σ_{i=1}^p λ_i
```

Minitab 系数是特征向量 V；相关载荷 `v√λ` 另表输出，不得冒充官方系数。解释率分母为全部 p 个特征值，不得在截断后再求和。特征值按降序排列，得分图使用前两个保留主成分。Hotelling T² 和 Q 残差用于辅助识别多变量异常，阈值为本样本经验分位（诊断 `empirical_anomaly_quantile`），不是 Minitab T² 控制图 UCL，也不替代工艺专家确认。T²/Q 超限不得写成过程合格或失控。常量列、完整行不足和特征分解不收敛必须作为明确诊断输出。

## DOE 响应分析

二水平因子使用编码值 `x_j ∈ {-1,+1}`。含主效应和二因子交互项的模型为：

```text
Y_i = β0 + Σ β_j x_ij + Σ β_jk x_ij x_ik + ε_i
β̂ = (X'X)^-1 X'Y
```

残差为 `e_i = Y_i - Ŷ_i`，误差平方和为 `SSE = Σe_i²`，模型平方和为
`SSR = Σ(Ŷ_i - Ȳ)²`，总平方和为 `SST = Σ(Y_i - Ȳ)²`。ANOVA 使用
`MS = SS / df` 和 `F = MS_term / MS_error`。主效应按
`Mean(Y|x_j=+1)-Mean(Y|x_j=-1)` 输出，交互作用图使用各组合水平的均值。
缺失运行、重复编码运行和 `rank(X) < columns(X)` 必须作为诊断输出。

## Type 1 Gage、偏倚/线性与稳定性

Type 1 Gage 以参考值 `μ0` 为基准：

```text
Bias = ȳ - μ0
Repeatability = s
t = Bias / (s / sqrt(n))
```

偏倚置信区间为 `Bias ± t_(1-α/2,n-1) s/sqrt(n)`。线性研究把不同参考值
`r_j` 的估计偏倚拟合为 `Bias_j = a + b r_j + ε_j`，并报告截距、斜率及其
置信区间。稳定性按时间顺序绘制观测值、中心线和控制限，并保留异常点位置。

## Kaplan-Meier 与寿命分布

Kaplan-Meier 生存函数处理右删失指示 `δ_i`：

```text
Ŝ(t) = Π_(t_i≤t) (1 - d_i / n_i)
Var(Ŝ(t)) = Ŝ(t)^2 Σ d_i / (n_i (n_i-d_i))
```

其中 `d_i` 是时点 `t_i` 的失效数，`n_i` 是风险集数量。Weibull 分布使用形状
参数 `β`、尺度参数 `η`：

```text
R(t) = exp(-(t/η)^β)
t_p = η[-ln(1-p)]^(1/β)
```

指数分布是 `β=1` 的特例，`R(t)=exp(-t/η)`。删失观测只进入风险集，不计入
失效数。寿命必须为有限正数。事件列只接受明确的失效/删失编码（如 `1/0`、
`fail/censored`）；`yes`、`unknown`、`2` 等未知值进入 `invalid_event_value`
并关联原始 RowId，不得静默成为删失。同一时间多个失效一次性处理。全删失、
无失效或最大观测为删失时给出不可估计原因。拟合结果回显 `converged`、
迭代次数和参数边界命中。
Log-rank 检验按每个失效时点比较观测失效数与风险集期望失效数：

```text
U = Σ(O_1i - E_1i)
V = Σ[d_i n_1i n_2i (n_i-d_i) / (n_i^2(n_i-1))]
χ² = U² / V,    p = P(χ²_1 ≥ χ²)
```

右删失 Weibull 的对数似然为
`ℓ(β,η)=Σδ_i[lnβ-βlnη+(β-1)ln t_i]-Σ(t_i/η)^β`；
参数通过数值求解似然方程得到，AIC=`2k-2ℓ`，BIC=`k ln(n)-2ℓ`。

三参数 Weibull 增加阈值 λ，`R(t)=exp(-((t-λ)/α)^β)`（t>λ），
`t_p=λ+α[-ln(1-p)]^(1/β)`，AIC 的 k=3。实现用剖面似然（固定 λ 后复用二参数 MLE），
只接受形状 β>1 的内点。似然无界时输出 `weibull3_likelihood_unbounded`，不伪造参数。
这不是 Minitab 的 Lockhart–Stephens bias-correction。菜单 `model=weibull3`；
默认 `weibull` 仍为二参数。分布比较表不把三参数混进 AIC。
数值是公式参考，不是 Minitab 导出。解释层不得写成「寿命服从三参数 Weibull」。

两参数对数正态 `ln(T)~N(μ,σ)` 的右删失对数似然为

```text
ℓ = Σ_δ=1 [−ln σ − ½ ln(2π) − (ln t − μ)²/(2σ²)]
  + Σ_δ=0 ln(1 − Φ((ln t − μ)/σ))
t_p = exp(μ + σ Φ⁻¹(p))
```

全删失或失效少于 2 时不识别参数。三参数对数正态用剖面似然估计阈值 λ，
`t_p = λ + exp(μ + σ Φ⁻¹(p))`，AIC 的 k=3，菜单 `model=lognormal3`；似然无界时
`lognormal3_likelihood_unbounded`。默认 `lognormal` 仍为二参数。两参数指数
`R(t)=exp(-(t-λ)/θ)`，`t_p=λ-θ ln(1-p)`，菜单 `model=exponential2`；无界时
`exponential2_likelihood_unbounded`。默认 `exponential` 仍为一参数。分布比较表输出
Weibull、指数与对数正态的 AIC/BIC，不把阈值模型混进 AIC。数值是公式参考，不是 Minitab 导出。
解释层不得写成「寿命服从该分布」。

## 二元 Logistic 拟合优度

Hosmer–Lemeshow：按拟合概率排序分约 10 组等计数，χ² = Σ(O−E)²/[n_k π̄_k(1−π̄_k)]，DF = g−2。
n&lt;20、未收敛、完全分离、有效组&lt;6 或组方差过小 → `not_computed`。
影响点：杠杆 h_ii &gt; 2(p+1)/n。解释只能写「在 α 下拒绝/未拒绝拟合不足」，未拒绝 ≠ 模型已充分。

## 个体分布识别

Anderson-Darling 公共核 A² = −n − (1/n)Σ[(2i−1)ln Z_(i)+(2n+1−2i)ln(1−Z_(i))]。
本轮四族二参数：Normal（Stephens p）、Lognormal（ln(x) 复用正态 p）、Weibull/Exponential（A² 排序，p 可为 not_computed）。
数据含非正值时后三族 not_computed。排序最优 ≠ 已证明服从该分布；不自动改写 `capability_method`。

## 组间/组内过程能力

σ_within = R̄/d2(n)；σ_X̄ = MR̄(子组均值)/d2(2)；σ²_B = max(0, σ²_X̄ − σ²_within/n)；σ_BW = sqrt(σ²_B + σ²_within)。
Cp/Cpk 用 σ_BW；Pp/Ppk 用样本标准差。需要子组标识列；σ²_B 截断为 0 时诊断 `between_variance_truncated`。
formula_reference，非 Minitab golden。

## t 检验功效与样本量

单样本 t 检验的非中心参数为 `δ = |μ1-μ0|/(σ/√n)`；双样本等方差设计使用
`δ = |μ1-μ2|/(σ√(1/n1+1/n2))`。给定显著性水平 `α` 和效应量后，使用
非中心 t 分布计算功效；样本量取满足目标功效的最小整数。标准差、效应量和
目标功效必须为正且目标功效小于 1。

One-Way ANOVA 使用非中心 F 分布，`df1=k-1`、`df2=N-k`，非中心参数
`λ=N f²`，其中 `f` 为 Cohen 方差效应量；功效为超过临界值
`F_(1-α;df1,df2)` 的非中心 F 尾概率。单比例和双比例分别使用原假设方差
与备择方差计算标准误，明确单侧/双侧方向以及 pooled/unpooled 选项；样本量
通过逐个整数搜索返回达到 Target Power 的最小样本量，并同时回显 Actual Power。
服务层输出「功效与样本量」表与功效曲线，并填充 `PowerFacts`。解释只陈述假设功效与实际功效，不写样本量足够。

## 卡方拟合优度

Pearson `χ²=Σ(O−E)²/E`，`E_i=p_i N`，`DF=k−1`。默认等比例。类别按首次出现顺序。
命令 `chi_square_gof` 与列联表独立性检验分离。

## G 图与 T 图

G 图：输入为非负数值间隔，`p̂=1/(x̄+1)`，控制限为几何分布分位减 1。T 图：间隔全为正时 Weibull MLE；含 0 间隔则排除后 log-log 回归并诊断 `zero_interval_regression_used`。默认仅 Test 1。不解析事件日期。
