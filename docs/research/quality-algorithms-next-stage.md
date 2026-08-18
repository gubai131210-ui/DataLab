# DataLab 下一阶段质量算法研究口径

> 研究范围：SPC 特殊原因 Test 1–8、正态过程能力 Cp/Cpk/Pp/Ppk/PPM、
> Anderson-Darling 正态性检验、线性回归诊断。  
> 研究日期：2026-08-17  
> 访问日期：2026-08-17（UTC+8）

本文只记录下一阶段实现前应确认的算法契约，不修改计划文件或生产代码。公式中的
`μ`/`x̄` 表示过程均值，`σw` 表示组内（within）标准差，`σo` 表示总体/长期
（overall）标准差，`Φ` 表示标准正态 CDF，`LSL`/`USL` 表示规格下限/上限。

## 1. 主要结论

1. Minitab 将 Test 1–8 作为特殊原因识别规则；默认只启用 Test 1，增加规则会提高
   对小偏移的敏感度，也会提高误报风险。NIST/SEMATECH 对应的 WECO 规则主要是
   Test 1、5、6 和 2（8 点同侧），并不等同于 Minitab 的完整 Nelson 1–8 集合。
2. Cp/Cpk 使用组内短期波动，Pp/Ppk 使用全部数据的总体波动；能力分析前应先用
   控制图确认过程稳定。能力指标是正态分布假设下的估计量，NIST 提醒通常需要约
   50 个独立观测才有较可靠的估计。
3. Anderson-Darling 是尾部加权的拟合检验；应同时报告 `A²`、调整后的 `A²*`、
   p 值、显著性水平和样本量，不能把“不拒绝正态”写成“证明正态”。
4. 回归诊断应把残差图作为主诊断：正态概率图、残差-拟合值、残差-顺序、残差-预测
   变量，并辅以 DW、杠杆值、Cook 距离、DFITS、删除学生化残差。单一的 `R²` 或
   正态性 p 值不能证明模型充分。

## 2. SPC Test 1–8

### 2.1 统一定义

对每个绘图点 `y_i`，中心线为 `CL_i`，局部标准差为
`σ_i = (UCL_i - LCL_i)/6`。若控制限为固定值，则这些量不随 `i` 变化；P/U/Laney
图可以因子组分母不同而随点变化。区间边界的建议约定如下：

- 超过（`>`）2σ、1σ：严格超出边界才计数；
- 在 1σ 内：使用 `|y_i-CL_i| < σ_i`，等于边界不计入“内”；
- 中心线上点既不属于上侧也不属于下侧，并重置同侧运行；
- 每个测试单独保留失败点集；绘图若只能显示一个编号，显示最小测试编号。

Minitab 的标准数量和含义如下：

| 测试 | 规则 | 主要信号 |
|---|---|---|
| Test 1 | 1 点超过中心线同侧 3σ（即出控制限） | 异常点 |
| Test 2 | 连续 9 点在中心线同一侧 | 均值偏移 |
| Test 3 | 连续 6 点全部递增或全部递减 | 趋势 |
| Test 4 | 连续 14 点上下交替 | 系统性/周期性变化 |
| Test 5 | 连续 3 点中至少 2 点超过同侧 2σ | 小偏移 |
| Test 6 | 连续 5 点中至少 4 点超过同侧 1σ | 小偏移 |
| Test 7 | 连续 15 点位于中心线两侧的 1σ 内 | 分层或控制限过宽 |
| Test 8 | 连续 8 点均超过 1σ，点可在中心线任一侧 | 混合/双群模式 |

来源：[Minitab：Using tests for special causes](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/basics/using-tests-for-special-causes/)
（访问 2026-08-17）；[Minitab：Individuals Chart tests](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/individuals-chart/perform-the-analysis/i-chart-options/select-tests-for-special-causes/)
（访问 2026-08-17）。

### 2.2 推荐输出字段

每个点至少输出：

`source_row`、`point_index`、`plotted_value`、`center_line`、`sigma`、
`LCL`、`UCL`、`zone`（C/B/A/outside）、`test1_failed` … `test8_failed`、
`failed_tests`、`phase`、`parameter_source`（estimated/historical）。

分析级输出至少包括：启用的测试列表、每个测试的失败点集、中心线和 sigma 估计
方法、估计样本量、排除行、阶段边界和诊断消息。必须区分“未启用”“数据不足”
和“未触发”。

### 2.3 DataLab 当前口径与差异

当前 `docs/statistical-methodology.md` 已记录上述 1–8 的数量，但当前源码
`src/domain/statistics/control_charts.cpp` 存在需在下一阶段明确的差异：

- Test 1–7 基本与 Minitab 数量一致；Test 2 对中心线严格同侧，中心线点重置运行，
  与本研究建议一致。
- 当前 Test 8 已不要求交替：连续 8 点均超过 1σ 即可，无论同侧或两侧。
- Test 7 使用 `|y-CL| ≤ σ`。
- Xbar-R 要求各组样本量相等；不等子组返回 `unbalanced_design`，不会用最后一组的 d2/A2。
- 对 R、S、MR 图，Minitab 页面说明只提供 Test 1–4；不要默认把 5–8 套到这些
  统计量上，除非界面显式声明并有统计依据。

NIST/SEMATECH 的变量控制图页面给出 WECO 规则：1 点超过 3σ、最近 3 点 2 点
超过 2σ、最近 5 点 4 点超过 1σ、8 点同侧，并提醒增加规则会增加误报：
[NIST/SEMATECH 6.3.2 Variables Control Charts](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm)
（访问 2026-08-17）。因此输出应避免把任何 Test 失败直接等同于已确认的根因。

## 3. Capability Cp/Cpk/Pp/Ppk/PPM

### 3.1 公式

双侧规格时：

```text
Cp  = (USL - LSL) / (6σw)
CPL = (μ - LSL) / (3σw)
CPU = (USL - μ)  / (3σw)
Cpk = min(CPL, CPU)

Pp  = (USL - LSL) / (6σo)
PPL = (μ - LSL) / (3σo)
PPU = (USL - μ)  / (3σo)
Ppk = min(PPL, PPU)
```

正态假设下，期望 PPM：

```text
Expected PPM below = 1,000,000 × Φ((LSL - μ)/σ)
Expected PPM above = 1,000,000 × [1 - Φ((USL - μ)/σ)]
Expected PPM total = below + above
```

观测 PPM 不依赖正态假设：

```text
Observed PPM below = 1,000,000 × count(x < LSL) / N
Observed PPM above = 1,000,000 × count(x > USL) / N
Observed PPM total = below + above
```

单侧规格只计算对应的 CPU/CPL 或 PPU/PPL；双侧 Cp/Pp 与另一侧缺失时应为空，
而不是填 0。目标值 `T` 若引入 Cpm，公式为：

```text
Cpm = (USL - LSL) / [6 × sqrt(σo² + (μ - T)²)]
```

来源：[Minitab：potential and overall capability](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/supporting-topics/capability-metrics/potential-and-overall-capability/)
、[Minitab：overall capability methods](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/methods-and-formulas/overall-capability/)
（访问 2026-08-17）；[NIST/SEMATECH：What is Process Capability?](https://www.itl.nist.gov/div898/handbook/pmc/section1/pmc16.htm)
（访问 2026-08-17）。

### 3.2 假设、边界和输出字段

能力输出至少应包含：

- 输入：`N`、缺失数、均值、LSL、USL、Target（如有）；
- 波动：`StDev(Within)`、`StDev(Overall)`、各自的估计方法及子组定义；
- 指标：`Cp`、`CPL`、`CPU`、`Cpk`、`Pp`、`PPL`、`PPU`、`Ppk`，可选 `Cpm`；
- 性能：Observed/Expected PPM 的 below、above、total；
- 诊断：正态性结果、稳定性提醒、规格合法性、sigma 无效、样本量不足；
- 可选置信区间：指标、置信水平、自由度和区间方法。

必须满足或明确处理：

1. 至少一个规格限；双侧时 `LSL < USL`，所有输入为有限数；
2. `σw > 0`、`σo > 0`；sigma 为零时不输出无限能力值；
3. Cp/Cpk 依赖合理的组内子组和稳定过程；Pp/Ppk 描述观测期间的总体表现；
4. 正态能力指标要求近似正态。非正态数据应转化或采用非正态能力方法，不能只
   用正态公式掩盖尾部偏差；
5. PPM 的规格边界使用严格超出（`< LSL`、`> USL`）；等于规格限不算超规；
6. 样本少、子组不合理、过程不稳定时仍可计算描述性结果，但必须显示警告，不应
   把数值解释成已验证能力。NIST 指出约 50 个独立观测通常才是“足够大”的量级。

### 3.3 DataLab 当前口径与差异

当前实现 `process_capability.cpp` 已采用 `σw` 计算 Cp/Cpk、`σo`（描述统计中的
样本标准差）计算 Pp/Ppk，并同时输出 observed 与 expected PPM；规格边界和
sigma 正值检查也已存在。当前需要在后续设计中补齐或明确：

- `Target`/`Cpm` 尚未进入 `ProcessCapabilityResult`，研究口径建议预留但不要在无
  Target 时伪造；
- 实现没有在能力计算入口强制正态性或控制图稳定性，仅由上层提示；输出应回显
  “assumption not verified” 而不是暗示验证通过；
- 由观测向量计算 PPM 时分母使用输入向量长度，后续应确认该向量已经完成缺失值
  过滤，并输出有效 `N`；
- 当前 expected PPM 以正态 CDF 计算，未提供非正态能力路径；
- 文档已有 `Cpk/Ppk = 1.33` 提示阈值，但这是项目解释阈值，不是 Minitab/NIST 的
  数学边界，报告中应标注“benchmark/提示”。

## 4. Anderson-Darling

### 4.1 公式和检验

给定排序观测 `Y_(1) ≤ … ≤ Y_(n)`，指定分布 CDF 为 `F`：

```text
A² = -n - (1/n) Σ(i=1..n) (2i - 1)
     × [ ln(F(Y_(i))) + ln(1 - F(Y_(n+1-i))) ]
```

`A²` 越小表示拟合越好，尾部权重高于 Kolmogorov-Smirnov。原假设为
`H0: 数据来自指定分布`，备择为不来自该分布；拒绝方向是 `A²` 大于对应分布和
参数估计方法的临界值。临界值不能脱离“分布、参数是否估计、样本量和修正常数”
单独复用。

Minitab 正态性 p 值采用调整统计量 `A²*` 的 Stephens 分段近似：

```text
A²* = A² × (1 + 0.75/n + 2.25/n²)

0.600 < A²* < 13:
  p = exp(1.2937 - 5.709A²* + 0.0186(A²*)²)
0.340 < A²* ≤ 0.600:
  p = exp(0.9177 - 4.279A²* - 1.38(A²*)²)
0.200 < A²* ≤ 0.340:
  p = 1 - exp(-8.318 + 42.796A²* - 59.938(A²*)²)
A²* ≤ 0.200:
  p = 1 - exp(-13.436 + 101.14A²* - 223.73(A²*)²)
```

来源：[Minitab：Methods and formulas for Normality Test](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/normality-test/methods-and-formulas/methods-and-formulas/)
、[Minitab：Anderson-Darling statistic](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/supporting-topics/normality/the-anderson-darling-statistic/)
、[NIST：Anderson-Darling Test](https://itl.nist.gov/div898/handbook/eda/section3/eda35e.htm)
（均访问 2026-08-17）。

### 4.2 推荐输出、边界和差异

输出字段：`N`、`N*`、均值、样本标准差、`AD`/`A²`、`AD_adjusted`、p 值、
alpha、判定（reject/fail-to-reject）、分布名、参数估计方式、概率图点和诊断。
判定建议写成“在 alpha 下拒绝/未拒绝 H0”，不得写成绝对的“正态/非正态证明”。

数值边界：排序输入必须为有限数；`s > 0`；CDF 进入 `log` 前要裁剪到
`[ε, 1-ε]`；极端 p 值须限制到 `[0,1]`；样本过小时返回诊断而非伪造 p 值。

当前 DataLab 的 `normality_test.cpp` 已实现上述 `A²`、Stephens 修正和分段 p
值，并对 CDF 做 `1e-12` 裁剪。但存在一个文档/实现差异：`docs/statistical-methodology.md`
写为 `n < 8` 时返回诊断，而源码实际只在 `n < 3` 时拒绝。因此下一阶段必须选定
一个契约并同步文档、字段和测试；在未完成选择前，报告应至少把小样本警告与 p 值
分开。Minitab 还指出某些情形 p 值在数学上不存在，此时应输出缺失 p 值和原因。

## 5. 回归诊断

### 5.1 模型和基础字段

当前 DataLab 使用带截距的 OLS：

```text
y = Xβ + ε
β̂ = argminβ ||y - Xβ||²
e_i = y_i - ŷ_i
SSE = Σe_i²
MSE = SSE / (n - p - 1)
R² = 1 - SSE/SST
R²adj = 1 - [SSE/(n-p-1)] / [SST/(n-1)]
```

基础输出：观测数 `N`、预测变量数 `p`、误差 DF、系数、SE、t、p、置信区间、
S（残差标准差）、SSE/SSR/SST、R²、调整 R²、预测 R²、PRESS、模型 ANOVA
（DF、SS、MS、F、p）。

### 5.2 诊断字段和解释

逐观测输出：`source_row`、`fitted`、`residual`、`standardized_residual`、
`studentized_residual`、`deleted_studentized_residual`、`leverage`、`Cook_D`、
`DFITS`、`outlier`、`high_leverage`、`influential`、诊断标志。

推荐公式：

```text
h_i = x_i'(X'X)^(-1)x_i
DW = Σ(i=2..n)(e_i-e_(i-1))² / Σ(i=1..n)e_i²
VIF_j = 1 / (1 - R²_j)
PRESS = Σ(e_i/(1-h_i))²
```

推荐相对阈值（仅作标记，不作自动删行）：

```text
high leverage: h_i > 2(p+1)/n
Cook influential: D_i > 4/n
DFITS influential: |DFITS_i| > 2√((p+1)/n)
outlier: |deleted studentized residual| > 3
```

图形和假设：

- 正态概率图/AD：残差近似正态；明显弯曲或离群点需调查；
- 残差 vs 拟合值：两侧随机、无漏斗，支持线性和等方差；
- 残差 vs 预测变量：无曲线或分层结构，支持函数形式；
- 残差 vs 顺序：无趋势、周期或成串，支持独立性；
- 杠杆/Cook/DFITS：识别会改变系数或拟合的观测；不得机械删除。

来源：[Minitab：Validate model assumptions](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/supporting-topics/model-assumptions/validate-model-assumptions/)
、[Minitab：Residual plots](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/supporting-topics/residuals-and-residual-plots/residual-plots-in-minitab/)
、[Minitab：Fit Regression residual plots](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-regression-model/interpret-the-results/all-statistics-and-graphs/residual-plots/)
、[NIST：How can I tell if a model fits my data?](https://www.itl.nist.gov/div898/handbook/pmd/section4/pmd44.htm)
（访问 2026-08-17）。

### 5.3 DataLab 当前口径与差异

当前 `regression.cpp` 已有 complete-case 输入、QR 求解、秩亏拒绝、系数推断、
杠杆、Cook、DFITS、PRESS、DW、残差 AD 和相对阈值。下一阶段应注意：

- 源码把 `studentized_residual` 直接设为 `standardized_residual`；这不是严格意义
  的外部删除学生化残差。字段应改名为 internally standardized，或实现真正的
  deleted/external studentization；
- 当前残差正态性只返回 AD 结果，尚未把四类残差图都作为结构化诊断字段/图层契约；
- DW 使用输入行顺序，这一点必须在输出中回显；如果用户按时间列排序，应先明确
  排序策略，不可静默重排；
- 当前设计矩阵秩亏阈值为 `1e-10`，残差和杠杆计算有 `1e-15` 防零保护。这些是
  数值保护，不是统计显著性阈值，建议集中为可测试的数值容差常量；
- 回归至少需要 `N ≥ p+2` 才有正误差自由度，当前还要求至少 3 个观测；报告应
  输出实际 `error_df = N-p-1`，并在不足时不给出 F/t/p；
- 当前 VIF 通过 `(X'X)^(-1)` 的对角元素与交叉积对角元素计算，数学上与辅助回归
  定义等价于满秩设计，但遇到尺度差异时应保留数值稳定性诊断；
- Minitab 明确指出图形诊断通常比单个数值检验更能揭示模型问题；DataLab 的
  解释服务不应仅凭 AD p 值或 R² 给出“模型合格”结论。

## 6. 建议的下一阶段验收清单

- [ ] SPC 每个 Test 的窗口、边界、中心线点和失败点集有独立测试。
- [ ] 专门覆盖 Test 8“同侧但均在 1σ 外”的回归测试，防止当前交替条件漏报。
- [ ] 能力分析覆盖双侧、单侧、LSL=USL、sigma=0、Target 缺失、非正态/不稳定警告、
      缺失值和等于规格限。
- [ ] AD 覆盖 `n=2`、`n=3`、`n=7`、`n=8`、常量样本、尾部极端值和 p 值边界。
- [ ] 回归覆盖正误差自由度、秩亏、常量预测变量、强尺度差异、顺序相关、杠杆点、
      Cook/DFITS/删除学生化残差字段语义。
- [ ] 报告固定回显：方法、数据行数、缺失处理、规格限、sigma 来源、alpha、容差、
      警告和“未验证假设”。

