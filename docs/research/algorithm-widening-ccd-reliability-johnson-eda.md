# DataLab 算法加宽调研：CCD/BBD、可靠性/保修、非正态能力与 Graph Builder EDA

研究日期：2026-08-21（UTC+8）  
研究范围：只读调研算法边界；不修改 C++、CMake 或测试代码。研究笔记写入 `docs/research/`，便于后续竖切实现。

## 1. 执行摘要

建议按以下顺序加宽：

1. **CCD/BBD 设计生成**：先形成纯函数设计矩阵、编码/实际单位映射、随机化与几何属性校验。
2. **可靠性基线**：先支持 exact/right-censored 数据、Kaplan–Meier、Weibull MLE 和 B-life/保修期失效率；再扩展区间删失、竞争风险、加速模型。
3. **非正态能力**：先实现“稳定性前置 + 正态能力 + 明确的非正态分支”，再实现 Johnson SB/SL/SU/SN 拟合和反变换规格限；不要把“变换成功”直接等同于过程能力成立。
4. **Graph Builder 式 EDA 最小边界**：提供少量可组合的 X/Y/分组/颜色/分页槽位和基础图元；把交互选择、隐藏/排除、联动作为第二阶段能力。

核心原则：输出必须携带**算法方法、数据清洗/删失规则、参数、假设、诊断、版本和可复现随机种子**。单一指标或一张图不能替代稳定性、分布适配和数据质量诊断。

## 2. 证据等级与来源边界

- NIST/DOE Handbook 用于定义设计、可靠性、删失、MLE、Kaplan–Meier 和能力分析的统计契约。
- CRAN/R 官方手册用于对照可执行 API、参数命名和成熟实现边界。
- JMP 官方公开帮助只用于界定 Graph Builder 的交互形态，不把其商业实现当作 DataLab 的算法标准。
- ASQ 的公开页面用于确认能力分析的工程语境；ISO 22514 系列的完整标准内容受版权/付费访问限制，不能从公开商品页推导具体公式。  
  来源：https://asq.org/quality-press/display-item?item=T1610E

## 3. CCD/BBD 设计生成

### 3.1 算法契约

**CCD（Central Composite Design）**

- 输入：因子数 `k`；因子名；每个因子的中心值、低/高实际边界；嵌入的全因子或分数因子结构；中心点重复数；星点距离 `alpha`；随机化策略/种子；可选分块信息。
- 生成：立方体（factorial/fractional factorial）点 + `2k` 个轴向星点 + 中心点。星点对每个因子分别取 `+alpha/-alpha`，其余因子为 0。
- 类型：CCC 的星点可能超出立方体；CCI 将因子边界放在星点上；CCF 使用面中心点，三种设计的空间和旋转性质不同。
- 输出：设计行表、编码坐标、实际单位坐标、点类型（cube/star/center）、block、run order、标准随机种子、设计属性和诊断。

NIST 明确指出 CCD 由嵌入的全因子/分数因子设计、中心点和用于估计曲率的星点组成；`k` 个因子对应 `2k` 个星点，并区分 CCC、CCI、CCF。  
来源：https://itl.nist.gov/div898/handbook/pri/section3/pri3361.htm

**BBD（Box–Behnken Design）**

- 输入：因子数 `k`（第一阶段建议 3–7）；因子名；每个因子的中心/低/高实际值；中心点数；随机化策略/种子。
- 生成：每次选取两个因子放在 `-1/+1`，其余因子置 0，并加入中心点；不包含所有因子的极端组合。
- 输出：设计行表、编码/实际单位映射、中心点和块信息、run order、设计属性。
- 约束：BBD 是独立的二次设计，点位在过程空间边的中点和中心，要求每个因子三个水平；正交分块能力相对 CCD 有限。

NIST 对 BBD 的定义、三水平、边中点/中心点几何和近似旋转性有明确说明。  
来源：https://itl.nist.gov/div898/handbook/pri/section3/pri3362.htm

R 的成熟 `rsm` 文档和实现可作为 `reference_implementation` 对照：`ccd()` 支持随机化、分数/别名和分块参数；`bbd()` 支持 3–7 因子，并说明其三水平及可能少于 CCD 的运行次数。只有固定版本、输入 hash、参数、输出和容差并经项目 review 冻结后，才可生成项目 `golden`。  
来源：https://cran.r-project.org/web/packages/rsm/refman/rsm.html

### 3.2 假设与禁止隐含行为

- 因子必须是可量化连续变量；分类因子不应静默编码成数值后送入 CCD/BBD。
- 低/高边界必须有序且有限；中心值默认 `(low+high)/2`，若用户显式给出中心值则必须在边界内并记录。
- 编码尺度必须固定：`x_coded = (x_actual - center) / half_range`；反变换必须可逆并保留单位。
- `alpha` 不能默认“永远旋转”：旋转性取决于设计类型、因子数和参数选择；应输出设计属性而非宣称。
- 随机化只改变执行顺序，不改变设计点集合；同一种子必须可复现。
- BBD 不应被用于研究所有因子同时处于极端水平的响应；这不是实现缺陷，而是设计空间边界。

NIST 指出响应面设计主要服务于二次模型，工业场景中三阶模型通常不常见，但应通过失拟/残差证据判断是否需要更高阶模型。  
来源：https://itl.nist.gov/div898/handbook/pri/section3/pri336.htm

### 3.3 可验证 golden

1. `k=2`、全因子 CCD、中心点 1：应有 4 个 cube 点、4 个 star 点、1 个 center 点；若重复中心点为 `n0`，总行数为 `2^k + 2k + n0`。
2. `k=3` BBD、中心点 1：应有 12 个边中点点 + 1 个中心点；不得出现 `(±1,±1,±1)`。
3. 对任一设计行做编码→实际单位→编码，数值误差应在明确容差内。
4. 相同输入/种子生成的 `run order` 完全一致；不同种子只允许改变顺序，不允许改变点集合。
5. 设计矩阵二次项列应可满秩；若用户选择分数 CCD、过少中心点或非法参数导致不可估计，必须返回结构化错误。
6. 与 CRAN `rsm::ccd()`/`bbd()` 在相同编码、中心点、alpha 和随机化约束下对比点集合；允许行顺序差异，但不允许点集合差异。

## 4. 可靠性与保修分析

### 4.1 最小可行算法契约

**输入模式**

- 必需：单位 ID、观察时间；事件标志（failure/event 或 right-censored）。
- 第一阶段支持 exact failure + right censoring；保留 `censoring_type` 字段，为 left/interval/counting/competing-risk 扩展。
- 可选：产品批次、应力/环境、失效模式、保修开始/结束、频数。

R 官方 `survival::Surv()` 明确了 right、left、counting、interval 等数据类型及事件/删失编码；right-censored 的常见编码是 0=删失、1=事件。  
来源：https://stat.ethz.ch/R-manual/R-devel/library/survival/html/Surv.html

**输出**

- KM：时间点、风险集、事件数、删失数、`R(t)`、`F(t)`、置信区间。
- Weibull：shape `beta/gamma`、scale `eta/alpha`、对数似然、协方差/置信区间、拟合诊断、`R(t)`、hazard、B10/B50/B90、MTTF。
- Lognormal：`log(time)` 域的 location/scale、原始时间域的可靠度、B-life/分位点、置信区间、删失计数和反变换诊断；不能把 log 域均值/标准差直接当作原始时间参数。
- 保修：给定保修时间 `T_w` 的 `F(T_w)=1-R(T_w)`、每 N 件预期索赔数、区间估计、输入数据覆盖范围和外推警告。
- 任何模型都要输出：删失类型、事件数、总样本量、是否存在全删失/零事件、时间单位、模型选择理由和警告。

NIST 将 KM 描述为不依赖特定分布的经验方法，要求精确失效时间，并通过风险集递推可靠度；删失单位只计入其最后观测前的风险集。  
来源：https://www.itl.nist.gov/div898/handbook/apr/section2/apr215.htm

NIST 给出 Weibull 的可靠度、失效率、均值等公式；二参数 Weibull 的可靠度为 `R(t)=exp(-(t/alpha)^gamma)`。  
来源：https://www.itl.nist.gov/div898/handbook/apr/section1/apr162.htm

### 4.2 MLE、删失与保修假设

- MLE 的似然必须按观测类型组合：精确失效使用 `f(t)`，区间删失使用 `F(right)-F(left)`，右删失使用 `1-F(t)`。
- 第一阶段不应把删失时间当成失败时间；这会系统性低估寿命。
- KM 适合描述观测范围内的经验可靠度；保修期超出最后事件/风险集时，应明确“不可识别/外推”。
- Weibull 的分布选择应同时考虑失效机制、历史经验和拟合诊断；仅凭样本量小的图形直线不能决定模型。
- NIST 建议在失败不极稀疏时优先 MLE；图形估计容易快速监控，但不能提供合法的置信区间。

来源：  
https://itl.nist.gov/div898/handbook/apr/section4/apr41.htm  
https://itl.nist.gov/div898/handbook/apr/section4/apr412.htm  
https://www.itl.nist.gov/div898/handbook/apr/section2/apr21.htm

DOE 的公开可靠性材料把 Weibull 分析用于寿命概率、保修预测和测试计划，并强调历史服务/现场/保修数据是可靠性预测的输入。  
来源：https://www.energy.gov/sites/default/files/2021-07/Module_3E.pdf

### 4.3 可验证 golden

1. 无删失样本：KM 在每个独立失败时间的递推结果应与手算乘积一致。
2. 一个右删失样本：删失只减少后续风险集，不在删失时刻产生 failure step。
3. 全部删失或零事件：KM/Weibull 不得返回看似正常的 B-life；必须返回“无法估计/需要更多事件”的结构化状态。
4. Weibull 合成数据：固定 `beta`、`eta` 和种子，MLE 估计应在预先声明的误差/覆盖标准内；同时验证 `R(eta)=exp(-1)`。
5. 保修 golden：给定拟合参数和 `T_w`，`claims_per_1000 = 1000*(1-R(T_w))`；输出必须带时间单位。
6. 与 R `survival::survfit(Surv(time,status)~1)` 对比 KM 曲线和风险集；与 NIST Weibull 公式对比可靠度/失效率；Lognormal 则使用固定版本的独立参考实现分别对比 log 域参数、原始尺度可靠度和分位点。

### 4.4 风险

- 竞争失效模式不能简单当普通删失后解释成“总体可靠度”；第一阶段应明确只做 cause-specific 或暂不支持。
- 加速寿命需要应力模型、物理依据和试验覆盖；不能把不同应力单元直接合并。
- 保修数据经常有暴露量、退货偏差、重复维修和产品装机量分母；只拿索赔计数拟合寿命会混淆风险集。
- 小样本、全删失、同一时间大量 ties、非独立样本和批次混合都必须触发诊断。

## 5. 非正态能力与 Johnson

### 5.1 算法契约

输入至少包含：稳定过程样本、LSL/USL、目标值（如有）、规格方向、分组/阶段字段和缺失规则。输出应分层：

1. 正态基线：`Cp/Cpk/Cpm`、均值、标准差、规格外概率及置信区间。
2. 非正态分支：分布/变换类型、参数、变换前后诊断、变换后的规格限、能力指标、反变换后的缺陷概率。
3. 非参数兜底：基于分位数的 `Cnp/Cnpk/Cnpm`，并报告分位数估计的不确定性。
4. 可解释诊断：稳定性未通过、分布拟合不佳、规格限落在变换域外、样本量不足、尾部超出数据支持范围。

NIST 明确说明传统 Cp/Cpk/Cpm 依赖正态性；非正态数据可先变换至近似正态，或使用非正态/非参数能力指标。NIST 还给出以 0.135% 和 99.865% 分位数模拟正态 ±3σ 覆盖的 Cnp 定义，并提醒不同资料可能使用 0.5%/99.5% 覆盖。  
来源：https://www.itl.nist.gov/div898/handbook/pmc/section1/pmc16.htm  
来源：https://itl.nist.gov/div898/software/dataplot/refman2/auxillar/cnp.htm

ASQ 公开能力资源强调：能力估计依赖稳定系统，Cp/Cpk 对正态假设敏感；稳定但非正态时可考虑变换或 Weibull/lognormal 等替代分布。  
来源：https://asq.org/quality-resources/process-capability

### 5.2 Johnson 变换边界

- Johnson 系统至少包含 `SN`、`SL`、`SB`、`SU` 家族；成熟 R `SuppDists` 文档提供 `JohnsonFit()`、密度/CDF/分位数/随机数函数。
- 拟合输出必须保存 `gamma、delta、xi、lambda、type`；不能只保存变换后的列。
- 规格限变换要尊重家族支持域：例如有界 SB 不能把超出支持域的值静默截断。
- “更接近正态”必须由变换后诊断和拟合质量支持；不能由 Anderson–Darling 单个 p 值自动决定能力结论。
- 第一阶段允许实现“拟合 + 诊断 + 可逆变换”的研究结果，但只有在固定参考版本、输入 hash、参数化约定、尾部比对、失败回退和至少一个项目 `golden` fixture 齐备后，才开放能力计算；不实现自动穷举所有分布和复杂置信区间。

来源：https://search.r-project.org/CRAN/refmans/SuppDists/html/Johnson.html

### 5.3 可验证 golden 与风险

1. 对固定样本和固定 Johnson 参数，`forward(inverse(z))` 与 `inverse(forward(x))` 在支持域内应满足数值可逆。
2. `SN`/`SU` 的宽域样本、`SL` 的正值样本、`SB` 的有界样本分别验证支持域检查和尾部行为。
3. 用固定版本的 R `SuppDists::JohnsonFit()` 生成 `reference_implementation`；固定版本、输入 hash、参数化约定和容差后，再经 review 冻结为项目 `golden`；必须比较拟合家族、CDF/分位数和变换后诊断。
4. 规格限在支持域外时必须是错误或明确 warning，不能输出普通 Cpk。
5. 同一数据在不稳定分组混合前后能力结果应能被诊断出差异；稳定性应在能力计算前验证。

主要风险是把“变换使图形像正态”误写成“过程已满足能力分析假设”，以及在尾部用样本外推得到没有证据支持的缺陷率。

## 6. Graph Builder 式 EDA 的最小可行边界

### 6.1 必须支持的最小契约

建议将其定义成 `EdaPlotSpec`，而不是复制一个大而全的“万能图表窗口”：

- 数据源：工作表/列 ID、筛选表达式、缺失处理、是否排除行。
- 槽位：X、Y、Color/Overlay、Group X/Group Y、Wrap/Page、Freq/Size。
- 图元：点图/散点、直方图、箱线图、条形图、折线图、基本拟合线。
- 输出：图元类型、轴、分组、统计摘要、选中行 ID、诊断和可重现 spec。
- 交互：悬停显示行信息；选择后保留原始行映射；隐藏与排除必须区分。

JMP 官方文档把 Graph Builder 定义为交互式探索工具，核心操作是将列拖到 X/Y/分组等 zones，再选择图元。其公开 graph zones 分为数据槽和分组槽，并明确 X/Y、Freq、Color、Size、Interval、Group、Wrap、Overlay、Page 的语义。  
来源：https://www.jmp.com/support/help/en/19.1/jmp/graph-builder.shtml  
来源：https://www.jmp.com/support/help/en/19.1/jmp/graph-zones.shtml

官方材料还明确支持点、线、样条、箱线图、条形图、直方图、马赛克图等多类图元，以及跨图联动/行状态的交互方向。  
来源：https://www.jmp.com/content/dam/jmp/documents/en/academic/learning-library/03-graphical-displays-and-summaries/03-09-interactive-graphing-with-graph-builder.pdf  
来源：https://www.jmp.com/support/help/en/19.1/jmp/element-types-and-options.shtml

### 6.2 明确不纳入第一刀

- 不做完整 JMP 图元集合、地图、脚本系统、仪表板编排和商业级自动排版。
- 不把 EDA 图上的趋势线当作正式回归/因果结论；正式建模走现有分析入口。
- 不把所有控件堆在主分析页；建议“图规格/数据选择”与“图画布/属性”分栏或独立页面。
- 不默认把排除行当作隐藏行：官方语义中 hidden 影响可见性，excluded 还会影响分析结果。

### 6.3 可验证 golden

1. 给定 `x,y,group,color` 和同一个 `EdaPlotSpec`，重复渲染的点集合、分组数量、摘要统计和选中行 ID 一致。
2. X/Y 连续变量生成散点；单连续变量生成直方图/箱线图；分类 X + 数值 Y 生成条形/箱线图；非法槽位组合返回诊断而不是崩溃。
3. 选择图点后能反查原始 `row_id`；过滤/隐藏不应改变分析统计，排除才改变统计，并在输出中记录。
4. Page/Wrap 的组数超过上限时给出分页/上限警告，不静默丢组。
5. 与 JMP 官方 zone 语义对照：同一 spec 至少能解释 X/Y、Group、Color、Overlay、Page 的差异。

## 7. 建议竖切顺序

### Slice 1：设计矩阵核心

- `DesignSpec -> DesignTable` 纯函数端口。
- CCD：全因子 cube、star、center、CCC/CCF 基线。
- BBD：3–7 因子、中心点、三水平。
- 编码/反编码、随机化种子、点集合/秩/重复诊断。
- Golden 先对齐 NIST 几何和 R `rsm`。

### Slice 2：可靠性描述与 Weibull

- 统一 `LifeObservation` 和删失枚举。
- KM + 风险集表 + 曲线区间。
- 二参数 Weibull MLE + B-life/保修时间评估。
- 全删失、零事件、少事件、超范围外推诊断。

### Slice 3：非正态能力

- 稳定性/分组前置门禁。
- 正态能力基线和非参数 Cnp/Cnpk。
- Johnson 拟合、支持域、规格限变换、可逆性和诊断。
- Golden 对齐 NIST、ASQ 公开边界和 `SuppDists`。

### Slice 4：EDA 组合器

- `EdaPlotSpec`、列类型适配、点/直方图/箱线图/条形图/散点。
- X/Y/Group/Color/Overlay/Page 槽位。
- 原始行联动、悬停标签、hidden/excluded 语义。
- 最后再扩展拟合线、密度、马赛克图和跨图联动。

## 8. 总体风险清单

- **契约风险**：把实际单位、编码单位、规格单位和时间单位混在一起。
- **统计风险**：忽略稳定性、删失机制、竞争风险、分布支持域或尾部不确定性。
- **复现风险**：随机化无种子、模型输出不保存参数、图表只保存图片不保存 spec。
- **产品风险**：用一个“算法加宽”页面堆积 DOE、可靠性、能力和 EDA 控件，造成输入/假设不可见。
- **对标风险**：Graph Builder 的交互体验可以借鉴，但其完整行为不是公开统计规范；应以最小可验证契约为准。
- **标准风险**：ISO 22514 的具体条文和公式需要合法访问标准原文后再作为合规依据，公开 ASQ 商品页只能证明标准存在和主题范围。
