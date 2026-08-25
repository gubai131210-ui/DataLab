# G9：公式代入 / 计算过程（Show Your Work）市场调研与产品边界（2026-08-23）

> 研究日期 / 访问日期：2026-08-23（UTC+8）  
> 用途：给 `/goal` **产品 Track G9 — 全算法公式变量取值页** 的调研正文  
> 配套计划：[`goal-wave-2026-08-23-g9-formula-substitution-plan-and-mega-prompt.md`](goal-wave-2026-08-23-g9-formula-substitution-plan-and-mega-prompt.md)  
> 执行框架：[`goal-execution-framework.md`](goal-execution-framework.md)  
> 关联：[`g1-g2-formula-registry-chart-copy.md`](g1-g2-formula-registry-chart-copy.md)（G1 **静态**公式注册表）· [`product-evolution-market-ux-architecture-research.md`](product-evolution-market-ux-architecture-research.md) §2 / §7  
> **不做本 Goal：** G3 Graph Builder、G4 Report Card 全量、G5 AnalysisService 大拆、嵌 R/Python、Minitab golden、TreeNet/AutoML

---

## §0 用户诉求（锁定）

| 诉求 | 产品翻译 |
|------|----------|
| 算法跑完后看到公式里**每个变量的本次取值** | 运行时 **ComputationTrace / FormulaBinding**，非静态帮助 |
| 合理 UI，勿单页堆控件 | **新建独立对话框 + 多页分层**（公式列表 / 变量表 / 代入预览 / 出处） |
| **所有算法都要实现** | `analysis_commands` 中**每一个统计/质量/控制图/可靠性/DOE/MSA/ML 命令**均须产出 trace；纯元命令豁免须登记 |
| 四角色互相监督 + 足够测试 | Planner → Implementer → Tester → Checker；verify + QtTest 覆盖契约 |

**与 G1 的边界（硬）：**

| | G1 公式注册表 | G9 公式代入 |
|--|---------------|-------------|
| 数据 | `algorithm_help.json` 静态 | **本次** `OutputPage` 绑定 |
| 问题 | 「一般用什么公式？」 | 「这次 n=？x̄=？Cpk=？」 |
| UI | 帮助 → 公式注册表 | 输出页 → **公式代入**（新页） |

---

## §1 Primary Sources（网上调研 · 2026-08-23）

### 1.1 工业 SPC / 审计透明（最贴合）

| 主题 | URL | 访问 | 采纳到 DataLab |
|------|-----|------|----------------|
| Cassini Show Your Work | https://saturnis.io/cassini | 2026-08-23 | **点结果 → 公式 + 输入 + 中间步 + 标准引用**；显示值=解释值契约测试 |
| Cassini GitHub 能力表 | https://github.com/saturnis-io/cassini | 2026-08-23 | 学交互与契约；**禁止 AGPL 整站并入** |
| Cassini 首页叙事 | https://saturnis.io/ | 2026-08-23 | 「每个数都能 show its work」→ 输出页入口 |

**采纳要点：**

1. Trace 从**结果数字**出发，而不是强迫用户先读帮助。  
2. 面板内容固定分层：**公式模板 · 输入绑定 · 中间量 · 出处**（分到不同页面）。  
3. **契约测试**：UI 展示的结果值必须与 Facts/表中权威值一致（容差内）。

### 1.2 Minitab / JMP（表形与「代入叙事」）

| 主题 | URL | 访问 | 采纳 |
|------|-----|------|------|
| Minitab Methods & Formulas（描述统计） | https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/display-descriptive-statistics/methods-and-formulas/methods-and-formulas/ | 2026-08-23 | **符号定义表形**；帮助是静态，**不是**本 run 代入 |
| Minitab Support 总览 | https://support.minitab.com/en-us/minitab/ | 2026-08-23 | Methods and Formulas 入口模式；复用 G1 Primary URL |
| JMP Show Prediction Expression | https://www.jmp.com/support/help/en/19.1/jmp/show-prediction-expression.shtml | 2026-08-23 | 报告内展示带**系数已代入**的预测式（例：−2.696−1.185+0.987(10)=5.99） |
| JMP Model Fit Options | https://www.jmp.com/support/help/en/19.1/jmp/model-fit-options.shtml | 2026-08-23 | Prediction Expression / Save Formula；学「表达式层」勿抄 JSL |

**结论：** Minitab 解决「公式是什么」；JMP 局部解决「系数写进式子」；**Cassini 解决「点开看本次计算」**。G9 主对标 Cassini，公式文案/符号对齐 G1 + Minitab Methods 表形。

### 1.3 办公 / 教学：逐步求值 UX

| 主题 | URL | 访问 | 采纳 |
|------|-----|------|------|
| Excel Evaluate Formula | https://support.microsoft.com/en-us/excel/evaluate-a-nested-formula-one-step-at-a-time | 2026-08-23 | **逐步求值对话框**（独立窗，不堆主表） |
| Excel Trace Precedents | https://support.microsoft.com/en-us/excel/display-the-relationships-between-formulas-and-cells | 2026-08-23 | 依赖关系可视；G9 用「绑定表」代替箭头 |
| UX.SE 中间结果可视化 | https://ux.stackexchange.com/questions/37496/visualizing-structure-and-intermediate-results-of-a-computation | 2026-08-23 | 默认隐藏、按需展开；避免信息过载 |
| UAHDataScienceSF learn=TRUE | https://cran.r-project.org/web/packages/UAHDataScienceSF/vignettes/UAHDataScienceSF.html | 2026-08-23 | 公式 → 列数据摘要 → 代入 → 得数；**禁嵌 R** |
| JASP Show R Syntax | https://jasp-stats.org/2023/05/30/jasp-0-17-2-blog/ | 2026-08-23 | 结果旁「透明层」；学入口位置，**不**做 R 语法页 |

### 1.4 DataLab 已有资产（复用，勿重做）

| 资产 | 路径 | G9 用法 |
|------|------|---------|
| 静态公式块 / 符号定义 | `resources/help/algorithm_help.json`（`formula_blocks`、`symbol_definitions`） | 公式模板 + 符号含义默认文案 |
| G1 注册表 Dialog | `src/ui/formula_registry_dialog.*` | 入口「在注册表查看静态公式」；**不合并页** |
| Facts 标量 | `quality_types.h` 各 `*Facts` | 绑定值的权威来源 |
| Interpretation | `interpretation_service.cpp` | 只读；禁止越权合格句 |
| 证据类型 | `VALIDATION_MATRIX.md` | 一律 `formula_reference`，非 golden |

---

## §2 命令盘点（全覆盖义务）

> 盘点脚本：`tools/_list_command_ids.py`（2026-08-23：`analysis_commands` **143** 项；help entries **143**；缺 help 核对：`tests`、`rule_policy`、`reliability_warranty`）。

### 2.1 覆盖策略（「所有算法」的操作定义）

| 类别 | 定义 | G9 义务 |
|------|------|---------|
| **A. 统计算法** | 推断、回归、多变量、可靠性、DOE 分析、MSA、能力、控制图限值等 | **必须**至少 1 条主公式 + 完整绑定表 + 代入预览 |
| **B. 设计生成** | `doe_*` 设计、`taguchi_orthogonal_design` 等 | 必须：阵列/点数/水平公式或生成规则绑定 |
| **C. 分布/功效工具** | `distribution_calculator`、`t_power` 等 | 必须：所选分布/功效公式绑定 |
| **D. 纯图形命令** | `histogram`、`scatter_plot`、`boxplot`… | **最小 trace**：有效 N、complete-case 规则、可选带宽/箱线五数等；无推断公式则登记 `trace_kind=display_summary` |
| **E. 元命令豁免** | 仅 UI 策略、非一次分析输出 | `tests`、`rule_policy` 等 → **豁免表**登记，verify 允许 |

**Complete 条件：** 类别 A/B/C **100%** 有实质绑定；类别 D **100%** 有最小 trace；类别 E 全部登记豁免。  
**禁止：** 用空 stub / 「尚未实现」糊弄过 verify（除 E）。

### 2.2 分族批次（执行时按族填满，禁止只做 2～3 个命令就 complete）

| 批次 | 族 | 示例 command_id（非穷尽，以 commands 全表为准） |
|------|-----|--------------------------------------------------|
| **FS-A** | 框架 + UI 多页 + 序列化 + 试点 3 命令 | `capability`、`one_sample_t`、`weibayes`（或 `descriptive`） |
| **FS-B** | 能力 / 质量工具 | `capability*`、`box_cox`、`tolerance_intervals`、`acceptance_sampling`、`distribution_identification`… |
| **FS-C** | 控制图 | `imr`、`xbar_*`、`p/np/c/u_chart`、`laney_*`、`ewma`、`cusum`、`zone_chart`、`z_mr`、`moving_average`、`g/t_chart`、`hotelling_t2`、`mewma`… |
| **FS-D** | 基础统计 / 假设检验 / 非参数 | `descriptive`、`*_t`、`*_z`、`anova`、`chi_square*`、`proportion*`、`poisson*`、`mann_whitney`、`kruskal_wallis`、`friedman`… |
| **FS-E** | 回归 / 多变量 / ML | `regression`、`*logistic*`、`poisson_regression`、`stepwise_*`、`best_subsets_*`、`pca`、`kmeans`、`cart_tree`、`random_forest`、`isolation_forest`… |
| **FS-F** | 可靠性 / 寿命 | `reliability`、`accelerated_life`、`cox_regression`、`weibayes`、`probit_reliability`、`km_interval`… |
| **FS-G** | DOE / RSM / Taguchi | `doe_*`、`taguchi_orthogonal_design`、`rsm_response`、`response_optimization`… |
| **FS-H** | MSA | `gage_rr`、`emp_crossed`、`expanded_gage_rr`、`msa_type1`、`nested_gage_rr`、`attribute_agreement`… |
| **FS-I** | 图形 + 工具 | 全部 `*_plot` + `histogram`/`pareto`/`run_chart`/… + `distribution_calculator` + `t_power` + `bootstrap_*`… |
| **FS-J** | 覆盖门 | verify：commands 全表 vs trace/豁免矩阵 **0 缺口** |

权威全表：以 `src/ui/analysis_commands.cpp` + `python tools/_list_command_ids.py` 为准；执行时写入 `docs/research/g9-formula-substitution-coverage-matrix.md`。

---

## §3 领域模型（建议）

```text
ComputationTrace {
  command_id
  formula_id          // 对齐 help formula_blocks 或稳定 id
  title               // 如 "Cpk"
  plain_formula       // 模板：Cpk = min( (USL-μ̂)/(3σ̂), (μ̂-LSL)/(3σ̂) )
  bindings[] { symbol, label_zh, label_en, value, unit?, fact_path?, role=input|intermediate|result }
  steps[] { order, description, expression_after }  // 可选；Excel Evaluate 风格
  result_symbol
  result_value
  evidence_type = "formula_reference"
  primary_url
  notes               // 边界诚实
}

OutputPage.computation_traces : vector<ComputationTrace>
```

**权威数值：** 仍以 domain 已算 Facts/表为准；代入字符串仅展示。  
**契约：** `|display_result − facts_value| ≤ ε`（测试强制）。

---

## §4 UI 分页（防笨懒 · 强制）

### 禁止

- 禁止把变量表、公式、出处、步骤、G1 搜索、Report Card **堆在同一页**  
- 禁止在输出主页加「一大坨」展开面板占半屏  
- 禁止单 Dialog 内多层 Tab + 左侧树 + 右侧三栏无层次乱堆

### 必须新建的页面结构

| 页面 | 职责 | 控件预算 |
|------|------|----------|
| **输出页工具条** | 仅按钮「公式代入」(+ 可选状态徽标：已绑定 N 条) | ≤2 控件 |
| **页 1 · 公式列表** | 本次 OutputPage 的 trace 列表 | 列表 + 打开 |
| **页 2 · 变量取值** | 选中公式的绑定表（符号/含义/值） | 一表为主 |
| **页 3 · 代入预览** | plain 代入一行 + 结果；可选 steps | 文本 + 可选表 |
| **页 4 · 出处** | Primary URL、research 路径、打开 G1 | 链接按钮 |

实现可用：`QStackedWidget` / 向导式「上一步·下一步」**或** 多个 `QDialog` 串联；**每一屏只做一件事**。

---

## §5 架构约束

```
OutputWorkspace 按钮
  → FormulaSubstitution* UI（只读 OutputPage）
  → 数据来自 page.computation_traces（由 AnalysisService 填充）
  → domain/service 在计算关键标量时 append bindings
interpretation 只读 Facts/trace；禁止合格越权句
禁止：domain→Qt；infrastructure→ui；嵌 R/Python
禁止：破坏 complete-case / source_row / A→B / hidden≠excluded
```

---

## §6 明确不做

- G3 Graph Builder、G4 Report Card 全量、G5 大拆  
- 嵌 R/Python、TreeNet/AutoML、Minitab golden  
- AGPL 并入 Cassini  
- 把 G1 注册表改造成运行时页（可跳转，不合并）  
- 向量/矩阵全量打印 — 只摘要维度/范数/前 k 项  

---

## §7 测试与契约

| 层 | 要求 |
|----|------|
| Domain/Service | 每族至少抽检；试点命令强制完整 |
| Serialize | `computation_traces` round-trip |
| UI | 四页导航；按钮；空态诚实 |
| Coverage | verify：全 command_id ∈（有 trace ∪ 豁免） |
| 回归 | wave4 / wave5 / menuIA / g1g2 不破 |

---

## §8 建议后续

| 下一刀 | 内容 |
|--------|------|
| G4 | Report Card — **分会话**；可链到 G9 binding |
| 深化 | 表单元格点击 → 打开对应 formula_id（Cassini 点数字） |

---

**文档状态：** 2026-08-23 首版；供 G9 `/goal` 直接引用。
