# G-Trust 参考实现 Golden 调研（Agent1）

> 调研日期 / 访问日期：**2026-09-05（UTC+8）**  
> 角色：Agent1 调研（只读；**未改**任何产品/测试代码）  
> 权威手册：`docs/research/goal-g-trust-minitab-golden-plan-and-mega-prompt.md` §0（已锁定，不重问）  
> 证据权威：`docs/research/VALIDATION_MATRIX.md`  
> 现网骨架：`tests/fixtures/minitab/`、`golden_loader.*`、`minitab_*_golden_test.cpp`  
> **结论先行**：锁表 10 命令 **均有** domain/API + 多为 `formula_reference`/契约测试；**无一**具备本 Goal 要求的 `golden ← reference_implementation` 冻结 expected TSV + 非 QSKIP 断言。**禁止**把现有 fixture / formula 测试说成「已与 Minitab 数值对齐」。

---

## A. Primary URL 表（≥8 条）

| # | 主题 / 对应命令 | Primary URL | 口径要点 | 访问日期 |
|---|---------------|-------------|----------|----------|
| 1 | I-MR 控制限（`imr`） | https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc322.htm | MR=\|xᵢ−xᵢ₋₁\|；σ̂=MR̄/d₂(2)，d₂(2)=1.128；I：CL=x̄，UCL/LCL=x̄±3·MR̄/1.128 | 2026-09-05 UTC+8 |
| 2 | X̄-R 控制限（`xbar_r`） | https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc321.htm | σ̂=R̄/d₂；X̄：CL=x̿，UCL/LCL=x̿±A₂R̄；R：CL=R̄，UCL=D₄R̄，LCL=D₃R̄ | 2026-09-05 UTC+8 |
| 3 | 正态过程能力 Cp/Cpk（`capability` / sixpack） | https://www.itl.nist.gov/div898/handbook/pmc/section1/pmc16.htm | Cp=(USL−LSL)/(6σ)；Cpk=min((USL−μ)/(3σ),(μ−LSL)/(3σ))；组内/总体 σ 分途 | 2026-09-05 UTC+8 |
| 4 | P 图（`p_chart`） | https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc332.htm | p̄=ΣDᵢ/Σnᵢ；UCL/LCLᵢ=p̄±3√(p̄(1−p̄)/nᵢ)；LCL 下截为 0 | 2026-09-05 UTC+8 |
| 5 | 双样本 t（`two_sample_t`） | https://www.itl.nist.gov/div898/handbook/eda/section3/eda353.htm | Welch / 合并方差两式；Welch–Satterthwaite df；默认现网偏 Welch | 2026-09-05 UTC+8 |
| 6 | Anderson–Darling 正态性（`normality_test` 默认） | https://www.itl.nist.gov/div898/handbook/eda/section3/eda35e.htm | A²=−N−S；参数估计时需 Stephens 调整常数与临界；**非** vendor 导出 | 2026-09-05 UTC+8 |
| 7 | 单因子 ANOVA（`one_way_anova`） | https://www.itl.nist.gov/div898/handbook/prc/section4/prc43.htm | H₀: 水平均值相等；分解 SS/DF/MS → F → p（正态+等方差假设语境） | 2026-09-05 UTC+8 |
| 8 | Gage R&R / MSA 术语与判读（`gage_rr`） | https://asq.org/quality-resources/gage-repeatability | EV/AV/R&R；%Study / %Tol；ndc 概念；AIAG 手册为商业出版物（本 Goal 用公开 ASQ+现网 AIAG 表形文档，**非**抄 vendor 数） | 2026-09-05 UTC+8 |
| 9 | （补充）Shewhart 变量图通则 | https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm | 3σ 模型、控制限≠规格限、WECO 规则代价 | 2026-09-05 UTC+8 |
| 10 | （补充）能力评估叙事 | https://www.itl.nist.gov/div898/handbook/ppc/section4/ppc46.htm | Cp vs Cpk；居中/偏移 | 2026-09-05 UTC+8 |

**仓库内既有公式研究（可复用，非本轮 Primary 替代）**：  
`docs/research/spc-control-charts.md`、`spc-capability-chi-grubbs-formulas.md`、`ttest-descriptive-normality-correlation-formulas.md`、`p1_gage_percent_tolerance_bias_aiag.md`、`p1_ryan_joiner_normality.md`。

---

## B. 10 命令缺口表（输入 / 期望 / API / 风险）

> 列含义：**输入**=稳定 CSV+SOURCE 登记；**期望**=可复跑 `expected/*_ref_golden.tsv`；**API**=domain/Facts 是否够冻主输出；**风险**=竖切/口径陷阱。  
> 状态缩写：✅有 · 🟡部分 · ❌无。**无一**标为 vendor 对齐。

| # | command_id | 输入 | 期望 | API | 风险 |
|---|------------|------|------|-----|------|
| 1 | `imr` | 🟡 `expected/imr_synthetic.csv`（手算小 N，**未**挂 golden_loader）；学习集 `imr_spi_shift`（教学，非 fixtures/minitab）；`PistonRingDiameter` 仅 smoke | ❌ 无 `*_ref_golden.tsv`；`minitab_formula_golden_test` 为硬编码手算（formula_reference），非 loader 冻结 | ✅ `ControlCharts::individuals_moving_range_dual`；`DualControlChartResult`（CL/UCL/LCL、MR̄、σ、MR UCL≈D₄·MR̄）；`SpcFacts` | σ 方法（avg MR / median / MSSD）、Nelson omit、阶段标签会改限；与能力 within-σ 耦合 |
| 2 | `xbar_r` | 🟡 `converted/CrankshaftMovement.csv`（Date 子组）、`CamshaftLength.csv`（Subgroup ID）；仅 `minitab_fixture_test` 行数/列名 | ❌ 无 expected TSV；`quality_statistics_test` / `xbar_output_test` 为 formula/契约 | ✅ `xbar_range` / `xbar_range_dual`；`ControlChartResult` + dual；命令→`AnalysisService::xbar_range` | 子组列角色（Date 序列 vs ID）；不等子组；A₂/D₃/D₄ 表与 n 钉死；特殊原因规则默认集 |
| 3 | `capability` | 🟡 `PistonRingDiameter.csv`（SOURCE：LSL=73.95,USL=74.05,Target=74.00；每 5 行子组约定）；`PinLength`/`CableWires` 备选 | ❌ 无 Cp/Cpk/Pp/Ppk ref-golden TSV；能力表形在 `quality_statistics_test` formula/contract | ✅ `ProcessCapability::calculate*`；`ProcessCapabilityResult` + `CapabilityFacts`（cp/cpk/pp/ppk…） | 组内 σ：n=1 走 I-MR vs 子组 R̄/d₂；overall=s；稳定性门禁不挡指数计算但影响产品语义 |
| 4 | `capability_sixpack` | 🟡 无专用 converted；测试用合成 30 点；可同源 PistonRing | ❌ 无 expected；仅 `buildsCapabilitySixpack`（plots≥5、有表） | ✅ `AnalysisService::capability_sixpack`；能力数字复用 capability 路径 | **图组件契约**须诚实列出（几图/何图），勿把「有图」当数值 golden；与 `capability` 共享 σ 口径漂移风险 |
| 5 | `gage_rr` | 🟡 学习库 `gage_rr_balance`（sqlite）；`samples/measurement_system/gage_rr_crossed.csv`（Part/Operator/Measurement）**未**进 `fixtures/minitab/converted` | ❌ 无 ref-golden；`crossed_gage_rr` 玩具断言 + `gage_rr_output_test` 契约 | ✅ `crossed_gage_rr` → `GageRrResult`（方差分量、`percent_study_variation`、`ndc`）；`MsaFacts` | 交互保留阈值、负方差截断、study_var×6、%Tol 单位；**禁止**把输出标成 AIAG/Minitab vendor 数 |
| 6 | `two_sample_t` | ❌ 无 minitab converted；仅测试内嵌向量 | ❌ 无 expected TSV | ✅ `two_sample_t_test` → `TwoSampleTTestResult`（mean_diff、t、df、p、CI）；`TTestFacts`；默认 Welch，可选 pooled | 方法开关必须写入 `# config`；单侧 vs 双侧；缺测行 complete-case |
| 7 | `normality_test` | 🟡 可复用能力同源列或自建；无专用 fixture | ❌ 无 AD/RJ 冻结 TSV | ✅ `normality_test(..., method)`；`NormalityTestResult` / `NormalityFacts`（默认 `anderson_darling`；可选 `ryan_joiner`） | **必须钉死方法名**；AD 调整 A² 与 p 近似族差异大；RJ≠AD |
| 8 | `one_way_anova` | ❌ 无官方 converted；需自建/samples | ❌ 无 SS/DF/MS/F/p expected；现有为 formula 小样本 | ✅ `one_way_anova` → `AnovaResult`；`AnovaFacts`；服务层 Tukey 等为加深项，主冻 ANOVA 表即可 | 因子列角色；不平衡；与 two_factor 混淆；Tukey 字母**非**本 Goal 主输出强制项 |
| 9 | `p_chart` | 🟡 `UnansweredCalls.csv`（Unanswered Calls / Total Calls，21 行）；fixture 形状已测 | ❌ 无 p̄/变限 expected TSV；`calculatesPChart` 玩具公式 | ✅ `ControlCharts::p_chart`；变限序列在 `ControlChartResult` | 变 n 楼梯限；LCL 截断；列角色（缺陷数 vs 总数）反接即错 |
| 10 | `between_within_capability` | 🟡 学习 `tools/learning_data/csv/cap_between_within.csv`（子组,厚度）；**未**复制到 fixtures/minitab | ❌ 无组间/组内 σ 与指数 expected | ✅ `ProcessCapability::calculate_between_within`；`between_within_capability` 服务；输出 StDev(Between/Within/BW)+Cp 等 | I-MR-RS / Xbar-R 分解与 `imr_rs` 口径交叉；子组列必填；与普通 capability 混用配置 |

---

## C. 现网骨架盘点

### C.1 目录与文件清单（现状）

| 路径 | 现状 |
|------|------|
| `tests/fixtures/minitab/SOURCE.md` | 6 个官方公开数据集转换说明（下载 2026-08-13）；**输入可用，≠ 数值 oracle** |
| `tests/fixtures/minitab/VALIDATION_MATRIX.md` | 完整性/字段轮；golden 仍指向「待 Minitab 导出」叙事——与本 Goal Q1B（ref-golden）需在后续登记时**改写诚实** |
| `tests/fixtures/minitab/converted/` | `CrankshaftMovement.csv`(125×2)、`CamshaftLength.csv`(100×4)、`PistonRingDiameter.csv`(125×1)、`PinLength.csv`(100×2)、`UnansweredCalls.csv`(21×2)、`CableWires.csv`(100×2)、`regression.csv`、`arima_trend.csv` |
| `tests/fixtures/minitab/expected/EXPORT_GUIDE.md` | 仅登记 **regression / arima** 导出格式；**未**含锁表 10 命令 |
| `tests/fixtures/minitab/expected/regression_golden.tsv` | `# source: OLS formulas…` — **formula 镜像**，注释写明待 Minitab 导出替换 |
| `tests/fixtures/minitab/expected/arima_trend_golden.tsv` | 镜像 domain ARIMA 网格 — **非** vendor_oracle |
| `tests/fixtures/minitab/expected/imr_synthetic.csv` | 手算 I-MR 指标 CSV；**未被** `minitab_*_golden_test` / loader 消费 |
| `tests/fixtures/minitab/golden_loader.h/.cpp` | TSV：`# section:` / `# config:`；`compare_double`（默认 abs/rel 1e-4）；`table_cell_as_double` |
| `tools/dump_minitab_golden_reference.py` | 仅再生 regression/arima 公式参考 — **无** SPC/MSA/推断锁表脚本 |

### C.2 测试入口（CMake 已挂）

| Target | 文件 | 与锁表关系 |
|--------|------|------------|
| `minitab_fixture_test` | `tests/minitab_fixture_test.cpp` | 6 CSV 行/列完整性；**无数值** |
| `minitab_formula_golden_test` | `tests/minitab_formula_golden_test.cpp` | 小 N I-MR 手算 + capability within-σ 与 I-MR 一致 + PistonRing smoke；**formula_reference** |
| `minitab_numerical_golden_test` | `tests/minitab_numerical_golden_test.cpp` + `golden_loader.cpp` | **仅** regression / ARIMA；缺文件 `QSKIP`；**不覆盖**锁表 10 |
| `quality_statistics_test` 等 | `tests/quality_statistics_test.cpp` 等 | 10 命令大多有 **formula_reference / 契约**；**不等于**本 Goal ref-golden |

`docs/research/VALIDATION_MATRIX.md`：**无**锁表 10 命令的 `golden←reference_implementation` 行（DOE/可靠性等另册已有 ref-golden 先例可仿）。

### C.3 Domain / Facts 主输出字段（冻结候选）

| command_id | Domain 入口 | 建议冻字段（与手册 §0.2 对齐） |
|------------|-------------|--------------------------------|
| `imr` | `individuals_moving_range_dual` | I: CL/UCL/LCL；σ；MR̄；可选 MR UCL |
| `xbar_r` | `xbar_range_dual` | X̄/R 的 CL/UCL/LCL（或等价中心+限） |
| `capability` | `ProcessCapability::calculate*` | Cp、Cpk（+ 若路径输出 Pp/Ppk） |
| `capability_sixpack` | `capability_sixpack` | 同上主指数 + 图数量/标题契约（诚实） |
| `gage_rr` | `crossed_gage_rr` | `%Study Var`（`percent_study_variation`）和/或 `ndc` |
| `two_sample_t` | `two_sample_t_test` | mean_difference、t、df、p、CI |
| `normality_test` | `normality_test` | method + 统计量（AD 或 RJ-r）+ p |
| `one_way_anova` | `one_way_anova` | SS/DF/MS/F/p 主表 |
| `p_chart` | `p_chart` | p̄（中心）+ 代表性 UCL/LCL 或全序列摘要 |
| `between_within_capability` | `calculate_between_within` | σ_within / σ_between / σ_BW + Cp 或约定指数 |

### C.4 旁路输入（可登记进 SOURCE，勿另起炉灶）

- `samples/measurement_system/gage_rr_crossed.csv`  
- `tools/learning_data/csv/cap_between_within.csv`  
- 学习 sqlite：`imr_spi_shift`、`gage_rr_balance`（导出 CSV → `converted/` 后登记）

---

## D. `reference_implementation` ≠ `vendor_oracle` 边界

| 维度 | `reference_implementation` → `golden`（**本 Goal**） | `vendor_oracle` → `golden`（**后续 Goal**） |
|------|------------------------------------------------------|---------------------------------------------|
| 输入 | 可用 Minitab **官方公开数据集** CSV（`SOURCE.md` 已列） | 同上或自有数据 |
| 期望值来源 | **钉版本** Python/R 参考脚本，按 NIST/AIAG/**公开**公式生成 | **真实** Minitab（或商业软件）导出 TSV，含版本/菜单路径/选项 |
| `VALIDATION_MATRIX` | `golden`←`reference_implementation`；状态「ref-golden 已冻」 | `golden`←`vendor_oracle`；方可谈「商业对齐候选」 |
| UI/帮助可写 | 「参考实现回归 / formula_reference / 待 vendor_oracle」 | 仅冻结流程完成后才可写对齐类表述 |
| **禁止** | 公开数据输入 + 自写脚本输出 ⇒「已与 Minitab 数值对齐」 | — |
| 现网 regression/arima expected | 保持其真实来源标签（公式镜像）；**本 Goal 不降级也不冒充 vendor** | 若日后真导出可替换 `# source` |

权威定义见 `docs/research/VALIDATION_MATRIX.md` 文首表；手册 §6 / H1 / H9 / H10。

**盘点声明（强制）**：当前仓库对锁表 10 命令 **不存在** vendor_oracle 对齐证据；现有 minitab numerical golden **也不**覆盖这 10 条。

---

## E. Wave 建议映射（仅建议，**不改**锁表）

与手册 Agent2 建议表一致；Wave 是施工队列，**10/10 必须做完**。

| Wave | 命令 | 建议输入锚点 | 出口 |
|------|------|--------------|------|
| **Wave-0** | （基建） | 扩展 EXPORT_GUIDE/SOURCE 命名；loader 容差若需；`verify_g_trust_golden_gate.py` 骨架；矩阵行模板；**改写 fixture 内「等 Minitab export / QSKIP 算完成」说法与 Q1B 一致** | 约定可执行 |
| **Wave-1** | `imr`, `xbar_r`, `p_chart` | 小 N 合成或 `imr_synthetic` 正规化为 TSV；`CrankshaftMovement`；`UnansweredCalls` | 3× ref-golden PASS |
| **Wave-2** | `capability`, `capability_sixpack`, `between_within_capability` | `PistonRingDiameter`+SOURCE 规格；六合一同源；`cap_between_within`→converted | 3× PASS |
| **Wave-3** | `gage_rr`, `two_sample_t`, `normality_test`, `one_way_anova` | `gage_rr_crossed`→converted；自建两列 t / 单因子 ANOVA；正态钉 AD | 4× PASS |
| **Wave-4** | 登记与门禁 | VALIDATION_MATRIX / backlog「ref-golden」；verify 全集；禁止 vendor 文案 grep | Agent4+5 |

容差默认跟手册 §4.2（能力 abs 1e-3；控制限 rel 1e-4；p abs 1e-4；DF 精确）。

---

## F. 风险与依赖（一行汇总）

**风险/依赖**：10 命令 API 齐备但 **expected×脚本×矩阵×非 QSKIP 测试** 全缺；最大口径风险在 **within-σ / Gage 交互截断 / AD 调整 / Welch 默认**；必须复用 `fixtures/minitab`+`golden_loader`；公开 Minitab 数据集≠ vendor 数；Agent 侧 Python verify，用户本机编 `minitab_*_golden_test`（及新 target）。

---

## Agent1 DoD 勾选

- [x] Primary URL 表 ≥8 + 访问日期 2026-09-05 UTC+8  
- [x] 10 命令缺口表（输入/期望/API/风险）无遗漏  
- [x] 现网骨架盘点（loader / 测试 / fixture）  
- [x] `reference_implementation ≠ vendor_oracle` 边界  
- [x] Wave 建议映射（不改锁表）  
- [x] **未**改产品/测试代码；**未**声称 vendor 对齐；**未**建议换模型  

**go/no-go（给编排者）**：**GO** → Agent2 写 `goal-g-trust-reference-golden-wave-plan.md`（粘贴 H1–H10 + §7）。
