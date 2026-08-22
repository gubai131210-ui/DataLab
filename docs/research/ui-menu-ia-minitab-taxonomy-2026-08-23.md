# DataLab 菜单信息架构（IA）调研：算法分类与级联菜单

> 研究日期：2026-08-23 · 访问日期：2026-08-23（UTC+8）  
> 用途：下一阶段 **UI 菜单分类整理** 的权威依据；`formula_reference ≠ golden`（本 Wave 无数值 golden）。  
> 配套 Goal 计划：[`goal-wave-2026-08-23-ui-menu-ia-layout-plan-and-mega-prompt.md`](goal-wave-2026-08-23-ui-menu-ia-layout-plan-and-mega-prompt.md)

---

## §0 问题陈述（仓库现状）

| 现象 | 证据 |
|------|------|
| 「统计」下命令过多、一屏很长 | `analysis_commands.cpp` 中约 **67** 条 `menu_path == "统计"` |
| 二级分组不完整 | `mainwindow.cpp` 的 `analysis_menu_group()` **硬编码白名单**；大量 Wave-2/3/4 命令（如 `stepwise_regression`、`nominal_logistic`、`kmeans`、`accelerated_life`、`bootstrap_two_sample`…）**未列入** → `return {}` → **直接挂在顶层菜单** |
| 声明字段未用 | `AnalysisCommand` 已有 `menu_group`（`analysis_commands.h`），但建菜单时几乎不读该字段，改走硬编码函数 |
| 顶层与 Minitab 不完全对齐 | 「控制图」独立顶层；Minitab 多为 `Stat > Control Charts`；「质量工具」与「统计」并存，需锁定本产品语义 |

**本 Wave 目标：** 把同一类别的算法收进对应子菜单（按钮/级联项），避免点开「统计」出现超长扁平目录；顺带统一 `menu_path` / `menu_group` / help `menu_path` / wiring 文档。

---

## §1 Primary Sources（网上调研）

| 主题 | URL | 访问 | 对本产品的采纳 |
|------|-----|------|----------------|
| Minitab Feature List（功能族划分） | https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-23 | **Stat / Graph / Quality Tools / Reliability / DOE / Multivariate…** 作为子菜单命名母版 |
| Minitab Quality Tools 路径 | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/supporting-topics/quality-tools-in-minitab/ | 2026-08-23 | `Stat > Quality Tools > …`；DataLab 可保留顶层「质量工具」或迁入统计下二级（见 §3 锁定） |
| Minitab Regression 菜单路径 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/supporting-topics/basics/regression-analyses-in-minitab/ | 2026-08-23 | `Stat > Regression > …`；Binary/Ordinal/Nominal/Best Subsets 同族 |
| Minitab Getting Started（菜单角色） | https://www.minitab.com/content/dam/www/en/uploadedfiles/documents/getting-started/MinitabGettingStarted_EN.pdf | 2026-08-23 | Graph 与 Stat 分工；分析内嵌图不替代 Graph 菜单 |
| Graph Builder 概览（**本 Wave 不做**） | https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/graph-builder/overview/ | 2026-08-23 | 登记延后 G3；本 Wave 只整理 Graph 现有命令分组 |
| JMP Menu Preferences（按兴趣隐藏） | https://www.jmp.com/support/help/en/19.1/jmp/menu-preferences.shtml | 2026-08-23 | **延后**：用户可隐藏整族菜单；本 Wave 只做默认分类，不做偏好面板 |
| UX：级联菜单深度 | https://ux.stackexchange.com/questions/111047/what-is-this-menu-with-multiple-levels-called-and-should-i-use-it | 2026-08-23 | **最多 1 级子菜单**（顶层 → 分组 → 叶命令）；禁止三层悬停走廊 |
| jamovi UI（分组/折叠） | https://dev.jamovi.org/ui/basic-design/ | 2026-08-23 | 对话框内「高级选项」折叠；菜单层用分组而非更深嵌套 |
| 命令树重组研究（遥测） | https://www.spinellis.gr/pubs/conf/2018-ICSE-SEIP-cmd-optimize/html/cmd-optimize.pdf | 2026-08-23 | 无遥测时用 **领域启发式 + Minitab 对齐**；禁止拍脑袋乱序 |

---

## §2 行业菜单模式摘要（可执行原则）

1. **按分析意图分族，不按实现文件分族**  
   Minitab：Basic Statistics / Regression / ANOVA / Control Charts / Quality Tools / DOE / Reliability / Multivariate / Time Series / Power…  
2. **顶层少、二级稳、叶命令短**  
   顶层建议 ≤6 个分析相关菜单；每个二级分组叶命令目标 **≤12～15**（超出再拆子族或加分隔）。  
3. **级联深度硬上限 = 1**  
   `顶层菜单 → 子菜单(group) → 命令`。禁止 `统计 → 回归 → 二元 Logistic → …` 三层悬停。  
4. **「控制图」可独立顶层**（DataLab 已有）或 `统计 → 控制图`；本产品 **保留独立顶层「控制图」**，与现有用户习惯一致。  
5. **质量工具顶层保留**  
   能力 / MSA / Pareto / Run Chart 等走「质量工具」；与 Minitab「Stat 下挂 Quality Tools」略不同，但减少「统计」膨胀。  
6. **声明式配置 > 硬编码白名单**  
   每个 `AnalysisCommand` 必须自带 `menu_path` + `menu_group`；MainWindow **只渲染**，禁止再维护巨型 `if (id == …)` 列表。  
7. **帮助 / wiring 与菜单同源**  
   `algorithm_help.json` 的 `menu_path` 与 UI 一致（或可机读映射）；避免「菜单在 A、帮助写 B」。

---

## §3 DataLab 目标分类树（锁定 · 2026-08-23）

> 下表是 **目标 IA**；执行 Goal 时按命令 id 全量填入，不得遗漏 Wave-4 新命令。

### 3.1 顶层

| 顶层 `menu_path` | 职责 |
|------------------|------|
| 文件 / 编辑 / 数据 / 查看 / 帮助 | 非分析（本 Wave 不改结构，仅勿破坏） |
| **统计** | 推断、建模、多元、时序、功效、可靠性（非质量工具族） |
| **控制图** | 全部控制图命令 |
| **质量工具** | 能力、MSA、质量图、容差、抽样、变换/分布识别 |
| **图形** | 探索性/通用图形（非控制图） |

### 3.2 「统计」二级 `menu_group`

| 二级分组 | 放入的命令族（示例 id） | Minitab 对标 |
|----------|-------------------------|--------------|
| 基础统计 | `descriptive`, `normality_test`, `outlier_test`, `correlation`, t/Z/比例/泊松率/等价性… | Stat > Basic Statistics |
| 假设检验 | `two_proportions`, `chi_square*`, `fisher_exact`, `variance_test`, 非参数秩检验… | 部分 Basic / Nonparametrics |
| ANOVA | `one_way_anova`, `two_factor_anova`, `anom` | Stat > ANOVA |
| 回归 | `regression`, `stepwise_regression`, `best_subsets_regression`, `logistic_regression`, `ordinal_logistic`, `nominal_logistic`, `poisson_regression` | Stat > Regression |
| 多变量 | `pca`, `discriminant`, `kmeans`, `cluster_observations`, `cart_tree`, `isolation_forest` | Stat > Multivariate (+ Predictive 窄化) |
| 时间序列 | `time_series_*`, `seasonal_forecasting`, `arima`, `adf`, `ccf`, `autocorrelation`（若独立命令） | Stat > Time Series |
| 可靠性 | `reliability`, `accelerated_life`, `probit_reliability`, `cox_regression`, `km_interval` | Stat > Reliability/Survival |
| DOE | `doe_factorial`, `doe_response`, `response_optimization`, `doe_plackett_burman`, RSM 设计生成相关 | Stat > DOE |
| 功效与样本量 | `t_power` 及扩展 | Stat > Power and Sample Size |
| 表格 | `cross_tabulation` | Stat > Tables |
| 推断 / 仿真 | `bootstrap_mean`, `bootstrap_two_sample` | Simulations / resampling（窄化） |

### 3.3 「控制图」二级（建议再拆，避免 20 条扁平）

| 二级分组 | 示例 |
|----------|------|
| 计量图 | `imr`, `xbar_r`, `xbar_s`, `imr_rs`, `z_mr`, `zone_chart` |
| 计数图 | `p_chart`, `np_chart`, `c_chart`, `u_chart`, `laney_*` |
| 时间加权 | `ewma`, `cusum`, `moving_average` |
| 多元 / 稀有事件 | `hotelling_t2`, `mewma`, `generalized_variance`, `g_chart`, `t_chart` |

### 3.4 「质量工具」二级

| 二级分组 | 示例 |
|----------|------|
| 过程能力 | `capability`, `nonnormal_capability`, `capability_sixpack`, `batch_capability`, `nonparametric_capability`, `binomial_capability`, `poisson_capability`, `between_within_capability` |
| 变换 / 分布 | `box_cox`, `distribution_identification`（及 Johnson 相关若独立） |
| MSA | `gage_rr`, `nested_gage_rr`, `expanded_gage_rr`, `msa_type1`, `attribute_agreement`, `emp_crossed` |
| 质量图 / 规划 | `pareto`, `run_chart`, `cause_and_effect`, `multi_vari`, `variability_chart` |
| 容差 / 抽样 | `tolerance_intervals`, `acceptance_sampling` |

### 3.5 「图形」二级

| 二级分组 | 示例 |
|----------|------|
| 分布与单变量 | `histogram`, `boxplot`, `density_plot`, `violin_plot`, `ecdf_plot`, `probability_plot`, `dotplot`（若有） |
| 关系与多元 | `scatter_plot`, `matrix_plot`, `bubble_plot`, `hexbin_plot`, `correlation_plot`, `correlogram`, `parallel_plot`, `heatmap_plot` |
| 时间 / 条形 / 其他 | `time_series_plot`, `bar_chart`, `area_plot`, `pie_plot`, `interval_plot`, `marginal_plot`, `eda_4plot` |

---

## §4 实现约束（给执行 agent）

| # | 约束 |
|---|------|
| 1 | **数据驱动**：每个命令在 `analysis_commands.cpp` 设置 `menu_path` + `menu_group`；MainWindow 只按字段建 `QMenu` |
| 2 | **删除/收缩** `analysis_menu_group()` / `primary_analysis_menu()` 特例，改为读命令字段（允许极少数 override 表，须 ≤10 行并单测） |
| 3 | **深度 ≤1** 级子菜单；过长分组用 `separator_before` 分段，不第三层 |
| 4 | **双语**：菜单顶层/分组/叶标签走现有 `ui_tr`；新增分组键写入 Linguist / `ui_menu_strings.json`（若仓库已有流水线） |
| 5 | **同步**：`algorithm_help.json` `menu_path`、`docs/algorithm-wiring-index.md`、acceptance 手工项 |
| 6 | **不改算法数值路径**：本 Wave **禁止**改 domain 统计公式；只动 UI 分类与相关测试/文档 |
| 7 | **不做**：Graph Builder、JMP 式菜单偏好隐藏、Ribbon 大改版、单页堆控件 |

---

## §5 验收要点（人手 + 脚本）

| 门 | 内容 |
|----|------|
| 脚本 | `python tools/verify_ui_menu_ia_track.py`：每个 `AnalysisCommand` 有非空 `menu_group`（允许白名单「顶层直接叶」极少）；统计顶层叶子数 ≤ 阈值；无「孤儿」id |
| 测试 | QtTest：菜单树构建后，「统计」下只有子菜单项（或极少直叶）；抽样命令落在预期 group |
| 人手 | Qt Creator 打开：统计 / 控制图 / 质量工具 / 图形 — 各级目录短、可扫读；Wave-4 命令（Cox、CIF 仍在可靠性、逐步 Logistic 在回归）位置正确 |

---

## §6 明确不做

- G3 Graph Builder 拖拽全量  
- Assistant / 命令 Wizard（G6）本 Wave 可只预留分组，不做推荐引擎  
- 按使用频率遥测自动重排  
- 三层以上级联或横向 mega-menu 大重做  

---

**文档状态：** 2026-08-23 首版；供 UI Menu IA Goal 直接引用。
