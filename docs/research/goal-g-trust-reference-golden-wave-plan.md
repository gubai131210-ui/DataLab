# Goal：G-Trust 参考实现 Golden — Wave 施工计划（Agent2）

> **角色**：Agent2 计划（只产本文；**禁止**改产品代码）  
> **日期**：2026-09-05（UTC+8）  
> **权威手册**：[`goal-g-trust-minitab-golden-plan-and-mega-prompt.md`](goal-g-trust-minitab-golden-plan-and-mega-prompt.md)（§0 已锁定）  
> **调研结论**：[`g-trust-reference-golden-research.md`](g-trust-reference-golden-research.md)  
> **证据权威**：[`VALIDATION_MATRIX.md`](VALIDATION_MATRIX.md)  
> **现网骨架**：`tests/fixtures/minitab/`、`golden_loader.*`、`minitab_*_golden_test.cpp`  
> **证据级别（强制）**：本 Goal 仅冻结 **`golden` ← `reference_implementation`**；**禁止**声称 Minitab `vendor_oracle` 数值对齐。  
> **模型**：全程 `inherit`；禁止建议换模型。

---

## §0 文首硬约束（手册 §0.1 H1–H10 全文粘贴）

| ID | 约束 |
|----|------|
| H1 | **必须衔接** `docs/research/VALIDATION_MATRIX.md` 证据类型定义：本 Goal 冻结项标 **`golden` ← `reference_implementation`**，**不是** `vendor_oracle` |
| H2 | **必须复用** `tests/fixtures/minitab/`（`converted/`、`expected/`、`golden_loader.*`、`SOURCE.md`、`EXPORT_GUIDE.md`）。禁止平行再建 `tests/fixtures/g_trust_v2/` 之类第二套 |
| H3 | 输入数据优先复用已有公开转换集（如 `PistonRingDiameter.csv`、`CrankshaftMovement.csv`、`UnansweredCalls.csv`、`PinLength.csv` 等）；新建 CSV 须登记 `SOURCE.md` |
| H4 | 期望值文件优先 TSV（`# section:` / `# config:`），与现有 `regression_golden.tsv` 同形；由 **Python 参考脚本生成**，禁止手抄无来源数字 |
| H5 | 测试入口衔接现有：`minitab_numerical_golden_test` / `minitab_formula_golden_test` / `minitab_fixture_test`；可扩展同一 target 或新增 `g_trust_*_test` **但须挂进现有 CMake 测试列表**，禁止孤儿 exe |
| H6 | 参考脚本放 `scripts/` 或 `tools/`，钉死依赖版本（注释写明）；脚本须可独立复跑生成 expected |
| H7 | 修改 domain 算法时：**竖切** domain → Facts/解释（若文案变）→ tests → 必要时 `algorithm_help` **仅当语义真变**；禁止改无关大文件（如乱改 `analysis_service.cpp` 上帝对象） |
| H8 | 学习中心 / 导入 / v2 catalog / 7B 隐藏 / 图形名实：**本 Goal 不碰** |
| H9 | UI/帮助文案禁止写「已与 Minitab 数值对齐」；可写「参考实现回归 / formula_reference / 待 vendor_oracle」 |
| H10 | backlog ⚪「待 golden」行：本 Goal 完成后改为可区分标记（例如「ref-golden 已冻」或在 VALIDATION_MATRIX 登记 ✅），**不得**把 ⚪ 直接改成暗示 vendor 对齐的 ✅ 而不改证据类型 |

---

## §1 命令锁表（Q2A，n=10，不得漏、不得缩）

| # | command_id | 优先输入 fixture（可调整但须登记） | 主输出（至少冻一项） |
|---|------------|-----------------------------------|----------------------|
| 1 | `imr` | 学习中心/合成单值流或现有 SPI 教学集；或自建小 N 参考脚本输入 | CL / UCL / LCL（I 图）；可选 MR 限 |
| 2 | `xbar_r` | `converted/CrankshaftMovement.csv` 或 `CamshaftLength.csv` | Xbar/R 的 CL/UCL/LCL 或等价中心与限 |
| 3 | `capability` | `converted/PistonRingDiameter.csv`（LSL/USL/Target 见 SOURCE.md） | Cp / Cpk（及文档约定的 Pp/Ppk 若实现路径输出） |
| 4 | `capability_sixpack` | 同上或同源配置 | 六合一中与能力指数相关的主数字 + 诚实列出图组件契约 |
| 5 | `gage_rr` | 现有 MSA 交叉样例（学习数据或 samples）；须在 plan 钉死路径 | %GR&R 或 %Study Var / 区分类别数（ndc）中本实现已输出者 |
| 6 | `two_sample_t` | 自建或 samples 两列独立样本 | 均值差、CI、t、df、p（按现网 Facts 字段） |
| 7 | `normality_test` | 能力同源或正态/轻偏样本 | 选定方法的统计量 + p（方法名钉死） |
| 8 | `one_way_anova` | 自建或 samples 单因子 | SS/DF/MS/F/p 主表 |
| 9 | `p_chart` | `converted/UnansweredCalls.csv` | 中心线与变限（或平均 p、平均 n）主数字 |
| 10 | `between_within_capability` | 学习中心 `cap_between_within` 同源或等价 CSV | 组间/组内分量或对应能力指数（按现网输出） |

**计数**：10。Wave 只是施工顺序，**不得**删减命令或推到下一 Goal。

---

## §2 证据边界与完成定义

| 可以 | 不可以 |
|------|--------|
| 用 Minitab **官方公开数据集** CSV 作输入（已在 `SOURCE.md`） | 公开输入 + 自写脚本输出 ⇒「已与 Minitab 数值对齐」 |
| 冻结 `reference_implementation` → `golden` | UI/帮助写 vendor 对齐 |
| Q3A：对不上改 domain 到容差内并登记 | 暗改公式 / 只放宽容差「一定过」 |
| 保留既有 `regression_golden.tsv` / `arima_trend_golden.tsv` 的真实来源标签 | 本 Goal 把它们冒充成 vendor，或降级锁表交付 |

**本 Goal「变绿」**：10/10 非 QSKIP 自动化断言 PASS + Python `verify_g_trust_golden_gate.py` PASS + VALIDATION_MATRIX 10 行 `golden`←`reference_implementation` + Agent5 review + commit/push。

---

## §3 Wave-0…4 划分与出口

| Wave | 内容 | 出口（硬门槛） |
|------|------|----------------|
| **Wave-0** | 约定、文案诚实化、loader/verify 骨架、EXPORT/SOURCE 命名模板 | 约定可执行；**文案已与 Q1B 一致**（见 §3.1、§9） |
| **Wave-1** | `imr` + `xbar_r` + `p_chart` | 3 条 ref-golden 测试 **PASS**（非 QSKIP） |
| **Wave-2** | `capability` + `capability_sixpack` + `between_within_capability` | 3 条 PASS |
| **Wave-3** | `gage_rr` + `two_sample_t` + `normality_test` + `one_way_anova` | 4 条 PASS |
| **Wave-4** | verify 全集、VALIDATION_MATRIX / backlog 登记、reference-index、文档 | Agent4+5 可收口 |

**强制顺序**：Wave-0 → 1 → 2 → 3 → 4。禁止 Wave-1 过了就算 Goal 完成。

### §3.1 Wave-0 明细（必须含 Q1B 文案改写）

| # | 工作项 | 说明 |
|---|--------|------|
| W0-1 | **文案诚实化（Q1B）** | 把 fixture 内「等 Minitab export / QSKIP 算完成」叙事改为：本轮以 `reference_implementation` 冻结 golden；真·Minitab 导出属后续 Goal；缺 ref-golden 文件对锁表命令必须 **FAIL**，不得用 QSKIP 冒充完成。目标文件见 **§9** |
| W0-2 | expected 命名约定 | `tests/fixtures/minitab/expected/<command_id>_ref_golden.tsv`（与既有 `*_golden.tsv` 并存；既有 regression/arima **不改证据类型**） |
| W0-3 | TSV 形 | `# source: reference_implementation …`；`# config: …`；`# section: …`；Tab 分隔；与 `golden_loader` 兼容 |
| W0-4 | loader | **优先零改动**复用 `GoldenTolerance` / `compare_double` / `table_cell_as_double`；仅当整数 DF 或按命令容差需辅助时小幅增强；禁止新 loader 宇宙 |
| W0-5 | `tools/verify_g_trust_golden_gate.py` | 骨架可先检查路径存在 + 禁止 vendor 文案 grep；Wave-4 收满清单（§7） |
| W0-6 | SOURCE / EXPORT_GUIDE | 预留 10 命令登记表头；Wave 落地时填齐 |
| W0-7 | CMake/测试挂载决策钉死 | 见 §6：**扩展** `minitab_numerical_golden_test` 为主路径（推荐） |

---

## §4 默认容差（手册 §4.2；可收紧，禁止无说明放宽）

| 量类型 | 默认容差 | 说明 |
|--------|----------|------|
| 中心线 / 均值 / 系数 | `rel_tol=1e-4` 或 `abs_tol=1e-6`（取较宽但仍严） | 与 `GoldenTolerance` 默认同量级 |
| 控制限 UCL/LCL | `rel_tol=1e-4`（相对 \|expected\|） | 极近 0 时改用 abs |
| Cp/Cpk/Pp/Ppk | `abs_tol=1e-3` | 能力指数 |
| %GR&R / %Study Var | `abs_tol=0.05`（百分数点）或实现一致的比例尺 | plan 钉死单位（见各命令） |
| t / F / 正态统计量 | `rel_tol=1e-4` | |
| p 值 | `abs_tol=1e-4`（或对极小 p 用相对） | 禁止只比「显著/不显著」布尔完事 |
| 整数 DF | 必须精确相等 | |

**本计划收紧处**（相对默认，均有说明）：

- `imr` / `xbar_r` / `p_chart` 中心线与限：优先 `abs_tol=1e-6` **且** `rel_tol=1e-4`（`compare_double` 取较宽者；脚本用 float64 生成，避免手抄）。
- `two_sample_t` / `one_way_anova` 的 DF：整数精确；`degrees_of_freedom` 若为 Welch 浮点 df，用 `abs_tol=1e-6`（钉死在 `# config`）。
- `gage_rr`：`percent_study_variation` 用 **百分数点** `abs_tol=0.05`；`ndc` 用 `abs_tol=0.05`（非整数强制相等，因现网为 `double`）。

---

## §5 每命令钉死规格（输入 / 脚本 / expected / 字段 / 容差）

> 通用：`# source` 必须含 `reference_implementation` + Primary URL（见调研 A 表）+ 脚本路径 + 依赖版本注释。  
> Domain 入口以头文件为准；测试优先调 **domain API**（避免绑死 `analysis_service.cpp` 大对象），sixpack 图契约可走 `AnalysisService::capability_sixpack`。

### 5.1 `imr`

| 项 | 钉死值 |
|----|--------|
| **输入** | `tests/fixtures/minitab/converted/imr_ref_golden_input.csv`（由现有 `expected/imr_synthetic.csv` 的数值序列 **正规化** 为单列观测 CSV，列名 `Value`；或脚本内嵌与 synthetic 同源的小 N 向量并写出 converted）。**禁止**另起 `fixtures/g_trust_*`。登记 `SOURCE.md` |
| **参考脚本** | `scripts/g_trust_imr_reference.py` |
| **公式口径** | NIST PMC 3.2.2；σ̂=MR̄/d₂(2)，d₂=1.128；默认 `SigmaEstimateMethod::average_moving_range`，`moving_range_length=2`，`use_nelson_estimate=false`，无 omit |
| **expected** | `tests/fixtures/minitab/expected/imr_ref_golden.tsv` |
| **`# config`** | `command_id=imr`；`method=average_moving_range`；`mr_length=2`；`nelson=0` |
| **断言字段（domain）** | `ControlCharts::individuals_moving_range_dual` → `DualControlChartResult`：`primary.center_line[0]`（I CL）、`primary.upper_control_limit[0]`（I UCL）、`primary.lower_control_limit[0]`（I LCL）、`sigma`、`average_moving_range`；可选 `secondary.upper_control_limit[0]`（MR UCL≈D₄·MR̄） |
| **TSV sections** | `metrics`：Key/Value 行含 `i_cl`,`i_ucl`,`i_lcl`,`sigma`,`mr_bar`,`mr_ucl` |
| **容差** | 中心/限/σ/MR̄：`rel_tol=1e-4`，近零用 `abs_tol=1e-6` |
| **测试槽** | `MinitabNumericalGoldenTest::imrMatchesRefGolden`（或同 target 新 slot） |
| **Q3A 风险** | σ 方法 / Nelson omit 与默认不一致即对不上 → 修实现或钉死 options 并登记 |

### 5.2 `xbar_r`

| 项 | 钉死值 |
|----|--------|
| **输入** | `tests/fixtures/minitab/converted/CrankshaftMovement.csv`（列 `A to B Distance`、`Date`；按 `Date` 建子组） |
| **备选** | `CamshaftLength.csv`（`Subgroup ID`）；若换用须改 EXPORT/SOURCE 并更新本表 |
| **参考脚本** | `scripts/g_trust_xbar_r_reference.py` |
| **公式口径** | NIST PMC 3.2.1；σ̂=R̄/d₂；X̄：±A₂R̄；R：D₃/D₄；子组大小取各组实际 n（本集每日 5 点，以脚本读出为准） |
| **expected** | `tests/fixtures/minitab/expected/xbar_r_ref_golden.tsv` |
| **`# config`** | `command_id=xbar_r`；`value_col=A to B Distance`；`subgroup_col=Date`；`special_causes=default`（与 `xbar_range_dual` 默认一致） |
| **断言字段** | `ControlCharts::xbar_range_dual`：`primary`（X̄）与 `secondary`（R）各自 `center_line[0]`、`upper_control_limit[0]`、`lower_control_limit[0]`（常数限时取 `[0]`；若实现为变限则断言全序列或 `# section: limits` 按子组索引） |
| **TSV sections** | `xbar_limits`：`cl,ucl,lcl`；`r_limits`：`cl,ucl,lcl`；可选 `summary`：`r_bar`,`a2`,`d3`,`d4`,`subgroup_size` |
| **容差** | 限与中心：`rel_tol=1e-4` / `abs_tol=1e-6` |
| **测试槽** | `…::xbarRMatchesRefGolden` |
| **Q3A 风险** | 子组键解析（Date 浮点）、不等 n、常数表与 n 不一致 |

### 5.3 `capability`

| 项 | 钉死值 |
|----|--------|
| **输入** | `tests/fixtures/minitab/converted/PistonRingDiameter.csv`（列 `Diameter`） |
| **规格** | LSL=73.95，USL=74.05，Target=74.00（`SOURCE.md`）；子组约定：每 5 行一组（与 SOURCE 一致） |
| **参考脚本** | `scripts/g_trust_capability_reference.py` |
| **公式口径** | NIST PMC 1.6；within σ 与现网 `estimate_within_subgroup_sigma` / I-MR 路径一致（脚本必须复现**同一** within 方法字符串写入 `# config`） |
| **expected** | `tests/fixtures/minitab/expected/capability_ref_golden.tsv` |
| **`# config`** | `command_id=capability`；`lsl=73.95`；`usl=74.05`；`target=74.00`；`subgroup_size=5`；`within_sigma_method=<钉死字符串>` |
| **断言字段** | `ProcessCapability::calculate`（观测 + within σ + specs）或服务同路径结果：`cp`,`cpk`；若路径输出则加 `pp`,`ppk`（手册允许） |
| **TSV sections** | `indices`：Metric/Value 含 `Cp`,`Cpk`,`Pp`,`Ppk`（未输出则 section 注明 N/A 且测试不断言） |
| **容差** | Cp/Cpk/Pp/Ppk：`abs_tol=1e-3` |
| **测试槽** | `…::capabilityMatchesRefGolden` |
| **Q3A 风险** | within σ（n=1 I-MR vs 子组 R̄/d₂）漂移；稳定性门禁不挡指数但仍须配置一致 |

### 5.4 `capability_sixpack`

| 项 | 钉死值 |
|----|--------|
| **输入** | 同 `capability`：`PistonRingDiameter.csv` + 同规格/子组 |
| **参考脚本** | `scripts/g_trust_capability_sixpack_reference.py`（可 `import` capability 脚本再生指数；图契约不由 Python 画图，只在 TSV `# config`/`# section: contract` 写死期望） |
| **expected** | `tests/fixtures/minitab/expected/capability_sixpack_ref_golden.tsv` |
| **`# config`** | 同 capability + `command_id=capability_sixpack` |
| **断言字段（数值）** | 与 capability 同源：`cp`,`cpk`（+ 若有 `pp`,`ppk`）——优先从 domain 能力结果取，避免只比 UI 字符串 |
| **断言字段（契约，诚实）** | `AnalysisService::capability_sixpack`：`page.plots.size() >= 5`（现网 `buildsCapabilitySixpack`）；可选钉死 `== 6` 若当前实现稳定为 6（`quality_statistics_test` 有 `== 6` 用例）。**禁止**把「有图」当成 vendor 数值对齐 |
| **TSV sections** | `indices`（同 capability）；`contract`：`min_plots`,`expected_plots` |
| **容差** | 指数同 capability；图数量整数精确 |
| **测试槽** | `…::capabilitySixpackMatchesRefGolden` |
| **Q3A 风险** | 与 capability 共享 σ 口径；改一处须两边复跑 |

### 5.5 `gage_rr`

| 项 | 钉死值 |
|----|--------|
| **输入** | 复制登记：`samples/measurement_system/gage_rr_crossed.csv` → `tests/fixtures/minitab/converted/gage_rr_crossed.csv`（列 `Part`,`Operator`,`Measurement`）；`SOURCE.md` 注明来自 samples，**非** Minitab 官方库 |
| **参考脚本** | `scripts/g_trust_gage_rr_reference.py` |
| **公式口径** | AIAG 交叉 ANOVA 形（公开 ASQ 术语 + 现网实现）；`tolerance=0` 时主冻 `%Study Var` 与 `ndc` |
| **expected** | `tests/fixtures/minitab/expected/gage_rr_ref_golden.tsv` |
| **`# config`** | `command_id=gage_rr`；`part_col=Part`；`operator_col=Operator`；`meas_col=Measurement`；`tolerance=0`；`study_var_multiplier=6` |
| **断言字段** | `crossed_gage_rr` → `GageRrResult`：在 `variance_components` 中 `source == "Total Gage R&R"` 的 `percent_study_variation`；以及 `ndc`（`ndc_available` 为真时） |
| **TSV sections** | `summary`：`percent_study_variation_total_gage_rr`,`ndc`；可选 `components` 表 |
| **容差** | `%Study Var`：`abs_tol=0.05`（百分数点）；`ndc`：`abs_tol=0.05` |
| **单位钉死** | `percent_study_variation` 为 **0–100 百分数点**（与现网 `GageVarianceComponent` 一致），不是 0–1 比例 |
| **测试槽** | `…::gageRrMatchesRefGolden` |
| **Q3A 风险** | 交互保留阈值、负方差截断、×6 study var；**禁止**标成 AIAG/Minitab vendor 数 |

### 5.6 `two_sample_t`

| 项 | 钉死值 |
|----|--------|
| **输入** | 新建 `tests/fixtures/minitab/converted/two_sample_t_ref_golden_input.csv`（两列 `Sample1`,`Sample2` 或长表 `Group,Value`——**选一钉死**：推荐宽表两列独立样本）；登记 SOURCE（合成；注明生成种子） |
| **参考脚本** | `scripts/g_trust_two_sample_t_reference.py` |
| **公式口径** | NIST EDA 3.5.3；**默认 Welch**（与 `VarianceMethod::welch` 一致） |
| **expected** | `tests/fixtures/minitab/expected/two_sample_t_ref_golden.tsv` |
| **`# config`** | `command_id=two_sample_t`；`variance_method=welch`；`alternative=two_sided`；`confidence=0.95` |
| **断言字段** | `two_sample_t_test` → `TwoSampleTTestResult`：`mean_difference`,`t_statistic`,`degrees_of_freedom`,`p_value`,`confidence_lower`,`confidence_upper` |
| **TSV sections** | `summary`：上述 Key/Value |
| **容差** | mean_diff / t / CI：`rel_tol=1e-4`；p：`abs_tol=1e-4`；Welch df：`abs_tol=1e-6` |
| **测试槽** | `…::twoSampleTMatchesRefGolden` |
| **Q3A 风险** | 误用 pooled；单侧；缺测 complete-case |

### 5.7 `normality_test`

| 项 | 钉死值 |
|----|--------|
| **输入** | 优先复用 `PistonRingDiameter.csv` 的 `Diameter`；或新建轻偏合成列并登记。**方法钉死**：`anderson_darling`（默认） |
| **参考脚本** | `scripts/g_trust_normality_test_reference.py` |
| **公式口径** | NIST EDA 3.5.e；Stephens 调整 A² 与现网 `adjusted_anderson_darling` / `p_value` 一致 |
| **expected** | `tests/fixtures/minitab/expected/normality_test_ref_golden.tsv` |
| **`# config`** | `command_id=normality_test`；`method=anderson_darling`；`alpha=0.05` |
| **断言字段** | `normality_test(..., "anderson_darling")` → `method` 字符串精确；`adjusted_anderson_darling`（或 `anderson_darling` 若调整为空则钉死用哪个）；`p_value` |
| **TSV sections** | `summary`：`method`,`A2` 或 `A2_star`,`p_value` |
| **容差** | 统计量 `rel_tol=1e-4`；p `abs_tol=1e-4` |
| **测试槽** | `…::normalityTestMatchesRefGolden` |
| **禁止** | 本 Goal 不混冻 Ryan-Joiner；若日后加 RJ 须另 expected |

### 5.8 `one_way_anova`

| 项 | 钉死值 |
|----|--------|
| **输入** | 新建 `tests/fixtures/minitab/converted/one_way_anova_ref_golden_input.csv`（列 `Factor`,`Response`）；登记 SOURCE（合成种子） |
| **参考脚本** | `scripts/g_trust_one_way_anova_reference.py` |
| **公式口径** | NIST PRC 4.3；平衡或轻度不平衡均可，脚本与 domain 同分组 |
| **expected** | `tests/fixtures/minitab/expected/one_way_anova_ref_golden.tsv` |
| **`# config`** | `command_id=one_way_anova`；`factor_col=Factor`；`response_col=Response` |
| **断言字段** | `one_way_anova` → `AnovaResult`：`between_sum_of_squares`,`error_sum_of_squares`,`total_sum_of_squares`；`between_degrees_of_freedom`,`error_degrees_of_freedom`,`total_degrees_of_freedom`（**精确**）；`between_mean_square`,`error_mean_square`；`f_statistic`；`p_value` |
| **TSV sections** | `anova`：Source/SS/DF/MS/F/P 至少 Factor / Error / Total 三行 |
| **容差** | SS/MS/F：`rel_tol=1e-4`；p：`abs_tol=1e-4`；DF：精确相等 |
| **测试槽** | `…::oneWayAnovaMatchesRefGolden` |
| **非强制** | Tukey 字母 / 事后多重比较 **不** 作为本 Goal 主输出 |

### 5.9 `p_chart`

| 项 | 钉死值 |
|----|--------|
| **输入** | `tests/fixtures/minitab/converted/UnansweredCalls.csv`（`Unanswered Calls` = 不合格数，`Total Calls` = n） |
| **参考脚本** | `scripts/g_trust_p_chart_reference.py` |
| **公式口径** | NIST PMC 3.3.2；p̄=ΣD/Σn；限=p̄±3√(p̄(1−p̄)/nᵢ)；LCL 下截 0 |
| **expected** | `tests/fixtures/minitab/expected/p_chart_ref_golden.tsv` |
| **`# config`** | `command_id=p_chart`；`defectives_col=Unanswered Calls`；`inspected_col=Total Calls` |
| **断言字段** | `ControlCharts::p_chart`：中心 `center_line`（常数则 `[0]`，记为 `p_bar`）；对每个子组或代表性索引断言 `upper_control_limit[i]`/`lower_control_limit[i]`；至少冻：`p_bar` + 第 0 点 UCL/LCL + 最大 n 与最小 n 各一点（变限覆盖） |
| **TSV sections** | `summary`：`p_bar`,`total_defectives`,`total_inspected`；`limits`：Index/UCL/LCL（全 21 行优先） |
| **容差** | `rel_tol=1e-4` / 近零 `abs_tol=1e-6` |
| **测试槽** | `…::pChartMatchesRefGolden` |
| **Q3A 风险** | 列反接；LCL 截断 |

### 5.10 `between_within_capability`

| 项 | 钉死值 |
|----|--------|
| **输入** | 复制登记：`tools/learning_data/csv/cap_between_within.csv` → `tests/fixtures/minitab/converted/cap_between_within.csv`（列 `子组`,`厚度_um`）；SOURCE 注明学习数据同源，**非**官方 Minitab 库。规格：在脚本/`# config` 钉死（建议 LSL/USL 与学习课一致；若学习库无规格则 plan 执行时从学习元数据读出并写入 SOURCE——**禁止 silent 默认**） |
| **参考脚本** | `scripts/g_trust_between_within_capability_reference.py` |
| **公式口径** | 组间/组内分解与 `ProcessCapability::calculate_between_within` 一致（调研：I-MR-RS / Xbar-R 交叉风险须在 `# config` 写 `between_sigma_method` / `within_sigma_method`） |
| **expected** | `tests/fixtures/minitab/expected/between_within_capability_ref_golden.tsv` |
| **`# config`** | `command_id=between_within_capability`；`subgroup_col=子组`；`value_col=厚度_um`；`lsl=…`；`usl=…`；方法字符串钉死 |
| **断言字段** | `ProcessCapabilityResult`：`within_standard_deviation` 或 `subgroup_within_standard_deviation`；`between_standard_deviation`；`between_within_standard_deviation`；主指数至少 `cp` 或 `cpk`（按路径实际输出钉死两者中已有者，优先两者都冻） |
| **TSV sections** | `sigma`：`sigma_within`,`sigma_between`,`sigma_bw`；`indices`：`Cp`,`Cpk`… |
| **容差** | σ：`rel_tol=1e-4`；指数：`abs_tol=1e-3` |
| **测试槽** | `…::betweenWithinCapabilityMatchesRefGolden` |
| **Q3A 风险** | 与普通 capability / `imr_rs` 混用配置 |

---

## §6 CMake / 测试挂载点

### 6.1 推荐方案（默认）

**扩展现有 target** `minitab_numerical_golden_test`：

| 项 | 路径 / 动作 |
|----|-------------|
| 源 | `tests/minitab_numerical_golden_test.cpp` 增加 10 个 private slots（上表测试槽名） |
| loader | 已链入 `tests/fixtures/minitab/golden_loader.cpp`（CMakeLists.txt ≈1038–1059） |
| CMake | **通常无需新 target**；若新增辅助 `.cpp`，加入同一 `qt_add_executable(minitab_numerical_golden_test …)` |
| ctest | 已有 `add_test(NAME minitab_numerical_golden_test …)` |

### 6.2 备选方案

若文件过大，可新增 `tests/g_trust_ref_golden_test.cpp` + CMake：

```cmake
qt_add_executable(g_trust_ref_golden_test
    tests/g_trust_ref_golden_test.cpp
    tests/fixtures/minitab/golden_loader.cpp
)
# link datalab_application / infrastructure / Qt6::Test
# DATALAB_SOURCE_DIR
add_test(NAME g_trust_ref_golden_test COMMAND g_trust_ref_golden_test)
```

**禁止**：未 `add_test` / 未挂进 `CMakeLists.txt` 的孤儿 exe。

### 6.3 与既有测试关系

| Target | 本 Goal 角色 |
|--------|----------------|
| `minitab_fixture_test` | 保持形状测试；新增 converted CSV 可扩行数断言（可选） |
| `minitab_formula_golden_test` | **保留**为 `formula_reference`；**禁止**改头换面假充本 Goal 交付 |
| `minitab_numerical_golden_test` | **主挂载**：既有 regression/arima **保留**；新增 10× ref-golden |
| `quality_statistics_test` 等 | 不删；不视为本 Goal 完成证据 |

### 6.4 QSKIP 策略（钉死）

| 场景 | 行为 |
|------|------|
| 锁表 10 命令的 `*_ref_golden.tsv` 缺失 | **FAIL**（`QVERIFY`/`QFAIL`），**禁止** QSKIP 冒充完成 |
| 既有 `regression_golden.tsv` / `arima_trend_golden.tsv` 缺失 | 可维持历史 QSKIP（属既有路径）；Wave-0 文案须说明「历史行为 ≠ 本 Goal 完成定义」 |
| 输入 CSV 缺失 | 锁表命令 **FAIL** |

---

## §7 `tools/verify_g_trust_golden_gate.py` 检查项清单

脚本路径：`tools/verify_g_trust_golden_gate.py`（新建；可参考 `verify_formula_reference_gate.py` 风格，但**独立**门禁）。

| # | 检查项 | 失败条件 |
|---|--------|----------|
| G1 | 锁表 10 `command_id` 常量完整 | 漏 id |
| G2 | 每命令存在 `expected/<id>_ref_golden.tsv` | 缺文件 |
| G3 | 每 TSV 含 `# source:` 且含 `reference_implementation` | 缺失或写成 `vendor_oracle` 冒充 |
| G4 | 每 TSV 含 `# config: command_id=<id>` 与命令一致 | 不一致 |
| G5 | 每命令存在可复跑脚本 `scripts/g_trust_<id>_reference.py`（sixpack/capability 允许共享 helper，但须有入口脚本） | 缺脚本 |
| G6 | `SOURCE.md` 登记本轮输入（含复制自 samples/learning 的说明） | 未登记 |
| G7 | `expected/EXPORT_GUIDE.md` 列出 10 个 expected 文件名与生成方式 | 未列 |
| G8 | `docs/research/VALIDATION_MATRIX.md` 含 10 行且类型为 `golden`←`reference_implementation` | 缺行 / 错类型 / 暗示 vendor ✅ |
| G9 | `tests/fixtures/minitab/VALIDATION_MATRIX.md` 与 Q1B 叙事一致（不再写「只有 Minitab 导出才算最终 golden」作为本 Goal 完成条件） | 旧叙事残留 |
| G10 | grep 门：本 Goal 新增/改动的学生可见路径禁止「与 Minitab 数值对齐」「已与 Minitab 一致」等（范围：`docs/`、`tests/fixtures/minitab/`、本轮脚本头注释；**不**误伤历史「待 Minitab 导出」诚实句，但诚实句不得写成「已对齐」） | 命中禁止宣称 |
| G11 | 禁止第二套 fixture 目录名（如 `fixtures/g_trust_v2`、`fixtures/oracle_v2`）出现 | 检出平行宇宙 |
| G12 | （可选）若 `build-mingw/minitab_numerical_golden_test.exe` 存在则运行；否则打印用户需在 Qt Creator 编译的 target 清单后 **不** 因缺 exe 失败（与手册 Q7 一致） | — |
| G13 | backlog：对应命令备注含 `ref-golden` 且未伪造成 vendor ✅（Wave-4） | 伪 vendor |

**退出码**：任一项失败 → non-zero。

---

## §8 禁止偷懒（手册 §7 全文粘贴）

1. **禁止**另起第二套 fixture/loader/验证宇宙。  
2. **禁止**只做 1～2 个命令就宣称 Goal 完成。  
3. **禁止**用 `QSKIP`/缺文件跳过冒充 PASS。  
4. **禁止**手写 expected 却无生成脚本。  
5. **禁止**声称 Minitab vendor 数值对齐。  
6. **禁止**对不上时只改测试容差到「一定过」而不修实现或登记。  
7. **禁止**暗改 domain 公式不更新矩阵/wiring/backlog。  
8. **禁止**改学习中心文案/v2/7B/导入/Graph Builder/Assistant。  
9. **禁止**中途换模型；所有 Task **`model: "inherit"`**。  
10. **禁止** Agent5 无 code review、无 commit/push 就 UpdateGoal complete。  
11. **禁止**强跑易失败中文路径全量 cmake/package（除非用户本轮要求）。  
12. **禁止**并行两人改同一大文件；禁止收尾塞无关重构。  
13. **禁止**把 `formula_reference` 测试改头换面假充本 Goal 交付。  
14. **禁止**新增虚假 command_id。  
15. **禁止**改 `algorithm_help.json` 公式语义，除非 domain 真变且登记。  

**本计划额外禁止（执行 Agent 必须遵守）**：

- 禁止只写笼统 Wave 不填每命令脚本/字段（本文已填；执行时禁止再缩水）。  
- 禁止删减命令。  
- 禁止 Plan 阶段已过去后仍改产品范围到 Q5 禁区。  
- 禁止另起第二套 fixture。  
- 禁止建议换模型。

---

## §9 Wave-0 文案改写目标文件列表

| 文件 | 改写目标（与 Q1B 一致） |
|------|------------------------|
| `tests/fixtures/minitab/expected/EXPORT_GUIDE.md` | 标题/导语改为区分：(A) 历史「待 vendor 导出」文件；(B) 本 Goal **`*_ref_golden.tsv`** = `reference_implementation` 冻结。删除或改写「测试在文件缺失时 QSKIP…只有实际从 Minitab 导出的结果才应作为最终 golden」——对 **锁表 ref-golden** 改为缺失即失败；并登记 10 文件生成命令 |
| `tests/fixtures/minitab/VALIDATION_MATRIX.md` | 「第二轮等用户 Minitab 导出才补数值」→ 改为：公开数据集输入可用；**本轮**数值基线来自 reference 脚本；vendor 导出仍为后续。Golden 表增加 10× ref-golden 行；改写「自动化测试 QSKIP…只有 Minitab 导出才算最终基准」为诚实双轨 |
| `tests/minitab_numerical_golden_test.cpp` | 注释/失败信息：锁表用例禁止 QSKIP；函数名可保留 `MatchesMinitabGolden` 历史名但注释必须写明 regression/arima 现为公式镜像/非 vendor；新 slot 命名用 `MatchesRefGolden` |
| `docs/research/VALIDATION_MATRIX.md` | Wave-4 增 10 行 `golden`←`reference_implementation`（可先在 Wave-0 留模板小节「G-Trust 锁表」） |
| `tests/fixtures/minitab/SOURCE.md` | 增补本轮新建/复制 CSV 行（imr 输入、gage_rr_crossed、cap_between_within、two_sample_t、one_way_anova） |
| （可选）`tools/dump_minitab_golden_reference.py` | 注释标明仅 regression/arima；**不要**假装覆盖锁表 10；锁表用 `scripts/g_trust_*_reference.py` |

---

## §10 本 Goal 不改文件清单（明确）

| 类别 | 路径 / 范围 |
|------|-------------|
| 学习中心 | `src/ui/learning_*`、`tools/learning_data/**` 内容润色（允许 **只读复制** CSV 到 `fixtures/minitab/converted`） |
| 导入 / catalog | 导入管线、v2/v3 catalog、7B 隐藏 |
| Graph Builder / Assistant | 全部 |
| LicenseAdmin | `tools/license_admin/**` |
| 上帝对象乱改 | 禁止为「顺手」重构 `src/application/analysis_service.cpp`；仅当 sixpack 契约断言必需且无 domain 替代时 **定点** 调用，不改无关逻辑 |
| algorithm_help | `algorithm_help.json` 除非 domain 语义真变且登记（H7/§7.15） |
| 偏门新算法 | 不新开 command_id |
| 第二套验证 | 任何 `tests/fixtures/g_trust_v2`、并行 `golden_loader2` 等 |
| 模型/编排 | 不建议换模型；不改手册文件名 |

**允许改（执行阶段）**：`tests/fixtures/minitab/**`、`golden_loader.*`（小幅）、`minitab_numerical_golden_test.cpp`（或新挂载 test）、`scripts/g_trust_*_reference.py`、`tools/verify_g_trust_golden_gate.py`、`docs/research/VALIDATION_MATRIX.md`、`docs/research/minitab-market-algorithm-backlog.md`（ref-golden 备注）、`docs/research/reference-implementation-index.md`（登记脚本）、`CMakeLists.txt`（仅挂测试）、以及 Q3A 时相关 `src/domain/statistics/*`（竖切 + 登记）。

---

## §11 脚本依赖钉死（执行时写进各脚本头注释）

| 项 | 要求 |
|----|------|
| 语言 | Python 3.10+（与仓库现有 verify 一致） |
| 依赖 | **优先仅 stdlib**（`csv`,`math`,`statistics`,`pathlib`）；若需 SciPy，注释钉死版本且 `verify` 可检测 ImportError 给出说明 |
| 复跑 | `python scripts/g_trust_<id>_reference.py` → 覆写对应 `expected/*_ref_golden.tsv` |
| 禁止 | 无脚本手改 expected；禁止从 C++ 打印抄回当「参考」而不写独立公式 |

---

## §12 Domain Q3A 登记模板（对不上时强制）

若修改 domain，必须在同一 PR/commit 说明中留下：

```text
Q3A 登记
- command_id:
- 文件:
- 旧行为 / 新行为:
- 公式口径 URL:
- 更新: VALIDATION_MATRIX / backlog / wiring-index（若有）
- 复跑脚本: scripts/g_trust_*_reference.py
```

禁止 silently 改公式。

---

## §13 用户本机验证清单（Agent 不强跑全量 cmake）

1. Qt Creator 编译并运行：`minitab_numerical_golden_test`（及若新建的 `g_trust_ref_golden_test`）。  
2. PowerShell：`python tools/verify_g_trust_golden_gate.py`。  
3. 可选：`package_dist` 抽查（用户执行）。  

---

## §14 Agent2 DoD / 风险 / 裁决

### 变更文件列表（本阶段）

- `docs/research/goal-g-trust-reference-golden-wave-plan.md`（本文；Agent2 产出）

### DoD 勾选

- [x] 文首粘贴 H1–H10 全文  
- [x] 10 命令锁表完整（不漏不缩）  
- [x] Wave-0…4 划分与出口；Wave-0 含 Q1B 文案改写  
- [x] 每命令钉死：输入、脚本、expected、断言字段、容差  
- [x] CMake/测试挂载点  
- [x] `verify_g_trust_golden_gate.py` 检查项清单  
- [x] §7 禁止偷懒全文粘贴  
- [x] 「本 Goal 不改」文件清单  
- [x] Wave-0 文案改写目标文件列表  
- [x] **未**改产品代码；**未**建议换模型；**未**另起第二套 fixture  

### 风险（一行）

**风险**：10 命令 API 齐备但脚本×expected×非 QSKIP 测试全缺；口径陷阱集中在 within-σ、Gage 交互截断、AD 调整、Welch df；文案若不改干净易把 QSKIP/「等 Minitab」误认为完成——Wave-0 必须先诚实化。

### go/no-go

**GO** → Agent3 按 Wave-0→4 执行（`model: inherit`）。
