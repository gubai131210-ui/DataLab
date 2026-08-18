# DataLab 其他质量算法深度规则化研究

研究日期：2026-08-17（UTC+8）  
范围：过程能力、正态性、回归/ANOVA、MSA、可靠性；SPC 作为已完成的统一参考实现。

本文记录官方资料、当前源码能力和下一阶段实现边界，不修改 SPC 计划文件。

## 1. 总体结论

SPC 已经形成了“规则元数据 → 适用性 → 算法结果 → 逐点证据 → 输出/序列化”的契约。
其他算法也应采用同样的深度逻辑，但规则类型不同：

- 过程能力：输入/规格合法性、sigma 来源、稳定性与正态性假设、能力指标和 PPM 的
  证据链。
- 正态性：样本量、有限值、常量样本、AD 修正统计量、p 值存在性和“拒绝/未拒绝”
  语义。
- 回归/ANOVA：误差自由度、秩亏、残差假设、影响点、方差齐性和多重比较家族错误率。
- MSA：实验设计平衡性、ANOVA 方差分量、重复性/再现性/零件间变异、%Contribution、
  %Study Var、%Tolerance 和 ndc。
- 可靠性：失效/删失语义、时间合法性、风险集、Kaplan-Meier 乘积极限、参数分布拟合
  与置信区间。

所有告警必须表达“证据显示需要调查”，不能把单个 p 值、阈值或能力指数直接写成
过程已经合格或根因已经确认。

## 2. 官方来源与冻结口径

### 2.1 过程能力

- Minitab：[Normal Capability Analysis methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/methods-and-formulas/methods/)
- Minitab：[Between/Within Capability Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/between-within-capability-analysis/methods-and-formulas/methods/)
- NIST：[Assessing Process Capability](https://www.itl.nist.gov/div898/handbook/ppc/section4/ppc46.htm)
- NIST：[What is Process Capability?](https://itl.nist.gov/div898/handbook/pmc/section1/pmc16.htm)

冻结规则：

- Cp/Cpk 使用 within sigma；Pp/Ppk 使用 overall sigma。
- 双侧规格要求 `LSL < USL`；单侧能力不应伪造缺失的另一侧指标。
- sigma 非有限或不大于零时，不输出无限能力值。
- PPM 分为 observed 与 expected；等于规格限不算超规。
- Target 存在时才计算 Cpm，不从样本猜测 Target。
- 结果必须回显 sigma 估计方法、子组定义、有效 N、缺失处理和稳定性/正态性是否已验证。
- `Cpk/Ppk = 1.33` 只能作为项目提示基准，不是数学上的通过边界。

### 2.2 正态性与 Anderson-Darling

- Minitab：[Normality Test methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/normality-test/methods-and-formulas/methods-and-formulas/)
- Minitab：[Anderson-Darling statistic](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/supporting-topics/normality/the-anderson-darling-statistic/)
- NIST：[Anderson-Darling Test](https://itl.nist.gov/div898/handbook/eda/section3/eda35e.htm)

冻结规则：

- 输出 `N`、`A²`、调整后的 `A²*`、alpha、p 值、判定和分布/参数估计口径。
- p 值判定只能写“在 alpha 下拒绝/未拒绝 H0”，不能写成证明正态。
- 非有限值、常量样本、样本量不足必须返回结构化诊断；不产生伪 p 值。
- CDF 进入对数前要裁剪，p 值限制在 `[0, 1]`；修正公式与最小样本契约必须统一。

### 2.3 回归与 ANOVA

- Minitab：[Validate model assumptions](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/supporting-topics/model-assumptions/validate-model-assumptions/)
- Minitab：[Diagnostic measures](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-regression-model/methods-and-formulas/diagnostic-measures/)
- Minitab：[One-way ANOVA model](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/one-way-anova/methods-and-formulas/model/)
- Minitab：[Multiple comparisons](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/one-way-anova/methods-and-formulas/multiple-comparisons/)

冻结规则：

- 回归至少要求 `error_df = N - p - 1 > 0`；秩亏设计直接拒绝拟合。
- 残差诊断至少覆盖正态性、残差-拟合值、残差-预测变量、残差-顺序，以及
  leverage、Cook、DFITS、VIF、Durbin-Watson。
- 当前字段若把 internally standardized residual 命名为 deleted/studentized，
  必须改名或实现真正的外部删除学生化残差。
- VIF 大于 5 作为共线性调查提示，不自动删除变量。
- ANOVA 要求独立、近似正态、方差相同；应把残差图和方差检验作为证据。
- Tukey 等多重比较必须回显 simultaneous confidence level、调整后 p 值和误差 DF，
  不可把逐比较 alpha 当成家族错误率。

### 2.4 MSA / Gage R&R

- Minitab：[Crossed Gage R&R table formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/crossed-gage-r-r-study/methods-and-formulas/gage-r-r-table/)
- Minitab：[Crossed Gage R&R method of analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/crossed-gage-r-r-study/methods-and-formulas/method-of-analysis/)
- Minitab：[Variance components](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/supporting-topics/gage-r-r-and-wheeler-s-emp-studies/variance-components/)
- Minitab：[Number of distinct categories](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/supporting-topics/gage-r-r-and-wheeler-s-emp-studies/using-the-number-of-distinct-categories-in-a-gage-r-r-study/)

冻结规则：

- 交叉设计优先检查 Part × Operator 交互；显著性和是否缩减模型必须回显。
- ANOVA 方差分量：Repeatability 来自误差，Reproducibility 包含 Operator 及必要的
  Operator × Part，Part-to-Part 单独报告。
- `Total Gage R&R = Repeatability + Reproducibility`，Total Variation 还包含
  Part-to-Part。
- `Study Var = multiplier × StDev`，默认 multiplier 6；%Contribution 与 %Study Var
  不可混淆。
- `ndc = truncate(1.41 × PartStDev / GageStDev)`，小于 1 时显示 1；ndc 小于 5
  是调查提示，不应被包装成绝对判定。
- 需要检查设计平衡、缺失单元、零方差、负方差分量截断策略和规格容差是否存在。

### 2.5 可靠性

- Minitab：[Kaplan-Meier estimation](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/distribution-overview-plot-right-censoring/methods-and-formulas/nonparametric-methods-and-formulas/kaplan-meier-estimation/)
- Minitab：[Right-censoring estimation methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/nonparametric-distribution-analysis-right-censoring/methods-and-formulas/estimation-methods/)
- Minitab：[Parametric estimates with right censoring](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/parametric-distribution-analysis-right-censoring/methods-and-formulas/parameter-estimates/)

冻结规则：

- 时间必须有限且非负；删失标志只能取明确定义的 failure/suspension 语义。
- Kaplan-Meier 使用风险集和乘积极限；同一时刻多个失效应作为同一时间点处理，
  同时报告 at-risk、failures、censored、survival 和标准误。
- 最大观测删失时，均值/尾部指标可能不存在，必须显示“不可估计”及原因。
- Weibull 至少回显 shape、scale、参数估计方法、删失似然和置信区间；三参数模型
  还要回显 threshold，不能在未识别时默默使用。
- Log-rank 比较必须报告删失处理、组别有效样本、事件数和检验自由度。

## 3. 当前代码缺口与实现优先级

当前主要入口位于：

- `src/domain/statistics/process_capability.*`
- `src/domain/statistics/normality_test.*`
- `src/domain/statistics/regression.*`
- `src/domain/statistics/hypothesis_tests.*`
- `src/domain/statistics/inference_extensions.*`
- `src/domain/statistics/gage_rr.*`、`nested_gage_rr.*`、`msa_type1.*`
- `src/domain/statistics/reliability.*`

建议按以下顺序实现：

1. 过程能力与正态性：统一有效样本、假设状态、规格/sigma 边界、PPM 和版本化输出。
2. 回归与 ANOVA：修正残差字段语义，补齐误差 DF、残差图模型、方差齐性/多重比较
   规则和影响点证据。
3. MSA：补齐方差分量契约、ndc、%Tolerance、平衡设计和交互模型输出。
4. 可靠性：补齐删失模型、风险集、KM 同时事件和 Weibull 参数/置信区间。
5. 时间序列、DOE、PCA、Logistic、非参数检验和图形算法：复用同一诊断/序列化框架，
   逐模块冻结公式后再实现，不把所有模块一次性混合重构。

每个阶段都应有：领域结果模型、规则/适用性元数据、结构化诊断、逐点或逐组证据、
输出表/图形/悬停一致性、JSON round-trip、边界测试和中文路径手工验收。

## 4. 代码库补充扫描

对现有统计模块的只读扫描还发现两项不应被后续遗忘的横向缺口：

- 时间序列的 Holt-Winters、滚动起点验证和基础 ARIMA 已存在，但候选 ARIMA 范围较窄；
  SARIMA 在状态空间验证和参数可容纳性完成前不应宣称已经完整实现。后续时间序列阶段
  应增加残差自相关、结构变化、时间乱序/重复时间点和多步预测稳定性诊断。
- `InterpretationFacts` 已覆盖 SPC、能力、DOE、MSA、可靠性和预测，但描述统计、相关、
  回归、方差检验和非参数检验仍可能由 `method_name`、表头文字反解析输出。解释层应
  改为领域结果直接提供结构化事实，避免 UI 表头变更导致解释静默失效。

其他已有模块的输入边界总体较完整：DOE 已检查缺失/重复运行和秩亏设计，可靠性已有
全删失、无失效和非法时间测试，Logistic 已有分离/不收敛诊断，非参数检验已有 ties
和近似方法提示，PCA 已保留有效行与原始行映射。它们应在各自阶段重点补“假设验证、
解释事实和小样本稳健性”，而不是重复建设浅层输入校验。

## 5. 优先缺陷清单

实现前应先把以下问题纳入测试基线：

1. 双因素 ANOVA 的 Sequential/Adjusted SS 应来自嵌套模型 RSS 差值；秩亏、不可估计
   effect 或无误差自由度时，不应继续输出伪造的 F/P。
2. 可靠性事件列必须拒绝未知编码；不能把任意非空文本静默解释为删失，并且排序后的
   Kaplan-Meier、失效和删失记录要保留原始 RowId。
3. 属性一致性 Kappa 要处理任意 alpha、`P_expected = 1`、重复评级不平衡和加权 Kappa
   的适用性，输出方差/区间方法。
4. 能力分析对每一个存在的规格限、Target 和观测值单独检查有限性，PPM 分母固定使用
   有效观测数；MSA 要报告负方差分量的原始值与截断值。
5. Type 1 Gage 的零重复性、回归的外部学生化残差/原始行映射、MSA 稳定性的逐点规则
   结果，都应明确不可识别或来源，而不是返回看似有效的默认数值。
6. SPC 现有文档和源码还需统一 Test 4、Test 7 的边界语义，以及 Xbar-R 不等子组和
   NaN 输入的拒绝规则；这些应作为跨算法契约的回归测试。
