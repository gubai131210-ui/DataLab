# DataLab 综合分析与图表路线图

> 研究日期：2026-08-21（UTC+8）  
> 访问日期：2026-08-21  
> 范围：在 [`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) 质量主路径之外，补齐**判断规则可文档化**、**通用 EDA/图表**、**建模加宽**的路线图；**不**克隆 Minitab/JMP API，**不**填写未导出的 golden 数值。

---

## 主要来源（Primary Sources）

| 主题 | URL | 访问日期 |
|---|---|---|
| Minitab 功能列表（Graphics / Quality / DOE / Time Series 等） | https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-21 |
| Minitab 控制图特殊原因测试（Nelson 1–8 口径） | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/basics/using-tests-for-special-causes/ | 2026-08-21 |
| Minitab I-MR 可选 Tests（默认仅 Test 1） | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/i-mr-chart/perform-the-analysis/i-mr-options/select-tests-for-special-causes/ | 2026-08-21 |
| Minitab 可视化资产列表（含 Graph Builder 候选图型） | https://support.minitab.com/en-us/minitab-solution-center/dashboard/asset-library/data-visualizations/ | 2026-08-21 |
| Minitab Graph Builder 概述 | https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/graph-builder/overview/ | 2026-08-21 |
| Minitab 默认测试选项（R/S/MR/属性仅 1–4；EWMA/MA 仅 1；Zone 不支持 Tests） | https://support.minitab.com/en-us/minitab/help-and-how-to/minitab-environment/settings-and-defaults/control-charts-and-quality-tools/tests/ | 2026-08-21 |
| Minitab I 图估计选项（MSSD / 中位 MR / Nelson estimate） | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/individuals-chart/perform-the-analysis/i-chart-options/specify-estimation-options/ | 2026-08-21 |
| Minitab Run Chart 方法 | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/run-chart/methods-and-formulas/methods-and-formulas/ | 2026-08-21 |
| NIST 工程统计手册 — 过程监控（含 WECO 与多元控制图目录） | https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm | 2026-08-21 |
| NIST — 变量控制图与 WECO 四规则及误报代价 | https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm | 2026-08-21 |
| NIST — 探索性数据分析（EDA）总章 | https://www.itl.nist.gov/div898/handbook/eda/eda.htm | 2026-08-21 |
| NIST — EDA 图形输出与 4-plot 解读 | https://www.itl.nist.gov/div898/handbook/eda/section4/eda4222.htm | 2026-08-21 |
| JMP Graph Builder 概述（交互式拖拽探索，**市场参考非 API 克隆**） | https://www.jmp.com/support/help/en/19.1/jmp/graph-builder.shtml | 2026-08-21 |
| ggplot2 常见 geoms（histogram / density / violin 等，**市场参考**） | https://ggplot2.tidyverse.org/reference/index.html | 2026-08-21 |

**口径说明：** Minitab 文档中的 Test 1–8 与 Nelson (1984) 规则集合一致；NIST 手册常称 **Western Electric (WECO) 四规则**（3σ、2/3 超 2σ、4/5 超 1σ、8 点同侧），与 Nelson 8 测试**部分重叠、编号不同**。DataLab 实现与文档对齐 **Nelson/Minitab Test 1–8**，并在 Zone 图使用 **Jaehn 区域计分**（非 Tests 1–8 替代品）。用户可见报告已改为稳定 `rule_id` + 具体规则名称（见 `special_cause_rule_catalog`）；内部仍可用 1–8 编号配置。

**2026-08-21 进度：** 报告规则 ✅；SQLite 导入+复合 keyset ✅；PG/MySQL/ODBC 驱动门控竖切 ✅（无驱动 ❌）；工作表/报告表交互 ✅；百万行压测仍 ⏸。  
**报告产品化 Phase 0：** 契约/验证矩阵/基准数据集 ✅。  
**报告产品化 Phase 1：** 三模板装配 + JSON + 模板 UI 🟡。  
**报告产品化 Phase 2：** manifest/原子导出/PDF 元数据 🟡；PDF/A·UA 未验证 ⏸。  
**报告产品化 Phase 3：** 报告双语 catalog + 独立 locale 🟡（ADR 0009）；§3.1 **61** 项 + F′/E′/E″ **37** 项 deepen + 场景预筛 **84** 项 + `tools/phase3_preflight.ps1` + **5** 项 `reference_implementation` 脚本；**Qt Creator 61+37 全绿 + §6 签署待用户**；全量 UI i18n 仍 ⏸。

---

## §0 目的与文档关系

### 目的

1. 为 DataLab 从「汽车质量 / Minitab 对齐」扩展到「更全面的桌面统计 + 图表」提供**可执行的 /goal 队列**与验收边界。  
2. 把**判断规则**（控制图 Tests、Run Chart 模式、ANOM、能力解读约束）从散落 research md 汇总为**可选/可文档化目录**。  
3. 对**通用 EDA 图表**做诚实缺口矩阵（基于 2026-08-21 代码审计 + 渲染器能力），避免把分析内嵌图误报为独立图形平台。

### 与 `minitab-market-algorithm-backlog.md` 的分工

| 文档 | 职责 |
|---|---|
| [`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) | **质量主路径**：市场算法名 ↔ DataLab 命令 ↔ ✅/🟡/❌/⏸；§12 优先队列（P0→P3） |
| [`product-evolution-market-ux-architecture-research.md`](product-evolution-market-ux-architecture-research.md) | **产品演进调研**（2026-08-22）：市场/UX/架构/性能 + Track G1–G8；**统一验收** → [`unified_track_acceptance_plan.md`](../../samples/product_evolution/unified_track_acceptance_plan.md) |
| **本文件** | **综合轨道**：判断规则深化、通用图表/EDA、建模加宽、四条 /goal Track；显式「不做」清单 |
| [`algorithm-chart-gap-matrix.md`](algorithm-chart-gap-matrix.md) | 单算法接线、Facts、导入契约 |
| [`deferred-capability-agreement.md`](deferred-capability-agreement.md) | golden 延后、Blaker、Kalman、3D 旋转等 |
| [`next-wave-algorithms-charts-ml-oss.md`](next-wave-algorithms-charts-ml-oss.md) | **P3+ / 经典 ML / 开源灵感** 候选池与 Track E–H |

**原则：** 质量 P2/P3 仍登记在 backlog §12；本文件不重复改 ✅ 行，只引用并扩展「综合视角」的下一步。

---

## §1 进度快照

### §12 P0 + P1 — 已清空（2026-08-21）

权威依据：[`algorithm-session-brief.md`](../algorithm-session-brief.md) §5b、`minitab-market-algorithm-backlog.md` §12。

已闭环（节选，**禁止重做**）：`runs_test`、`fisher_exact`、`outlier_test` Dixon r10、`run_chart`、`cause_and_effect`、`one_sample_z`、`variability_chart`、容差 `tolerance_method`、`acceptance_sampling`、`anom`、`poisson_gof`、协方差/偏相关、`zone_chart`、`z_mr`、`moving_average`。

### 剩余质量队列（backlog §12）

| 优先级 | 项 | backlog 状态 | 类型 |
|---|---|---|---|
| **P2** | DOE 设计生成（全/部分析因、PB、DSD…） | ✅ / ⏸ | 2^k+2^(k-p) ✅；PB ✅；DSD ⏸ |
| **P2** | RSM 分析（已有设计表时） | ✅ | 命令 `rsm_response` |
| **P2** | Expanded Gage / EMP | ✅/⚪ | EMP + 平衡三因子 Expanded；不平衡 GLM ⚪ |
| **P2** | 多元控制图 T² / GV / MEWMA | ✅ | T²+GV+MEWMA |
| **P2** | ACF / PACF；等价/DOE/容差 **功效** | ✅ | `acf_pacf`；`t_power` 扩展 |
| **P3** | 加速寿命；保修/可修复系统 | ✅/🟡 | ALT Newton MLE + 使用应力百分位 ✅；`probit_reliability` 窄化 ✅ |
| **P3** | 有序 Logistic / Discriminant / CCF / Correlogram | ✅ | 批次 3；见 next-wave §7 |
| **P3** | Stepwise / 区间删失 KM / Plackett–Burman | ✅ | Track H；`stepwise_regression` / `km_interval` / `doe_plackett_burman` |
| **P3** | 名义 Logistic；Best subsets；CCD/BBD 生成 | ✅ | 名义 IRLS ✅（Wave-2.5）；Best subsets ✅；CCD/BBD ✅ |

### 并行存在的 🟡（非 §12 独占，但综合轨道需跟进）

来自 backlog 正文：二元 Logistic 深化、KM/Log-log 删失、ARIMA 候选、PCA 解析 T²/Q 限、Batch/非参数能力、Johnson/非正态/可靠性族 **⚪ 待 golden**、不平衡 Expanded GLM ⚪ 等。

---

## §2 算法判断规则目录

> 目标：用户能**选择**规则集合、在帮助/输出中看到**与 Minitab/NIST 可对照的定义**，解释层遵守「证据 ≠ 合格」契约。

### 2.1 Western Electric / Nelson / Minitab Tests 1–8（控制图）

| 规则 | Minitab/Nelson 定义（摘要） | DataLab 现状 | 可选/文档化 |
|---|---|---|---|
| Test 1 | 1 点超 3σ | ✅ 已实现 | UI `tests` 可选；默认空=该图种**全部适用**测试（与 Minitab 默认仅 Test 1 **不同**，需在帮助中写明） |
| Test 2 | 9 点同侧 | ✅ | 同上 |
| Test 3 | 6 点严格单调 | ✅ | 同上 |
| Test 4 | 14 点交替 | ✅ | 同上 |
| Test 5 | 3 点中 2 点同侧超 2σ | ✅ | 同上 |
| Test 6 | 5 点中 4 点同侧超 1σ | ✅ | 同上 |
| Test 7 | 15 点在 ±1σ **内**（严格 `<σ`） | ✅ | 同上；边界已按 gap-matrix 核对 |
| Test 8 | 8 点在 ±1σ **外** | ✅ | 同上 |
| 图种适用性 | R/S/MR/属性 1–4；EWMA/MA/G/T 仅 1；CUSUM 无 Tests；Zone 无 Tests | ✅ 与 Minitab 一致 | `applicable_special_cause_tests()` 已编码 |
| 阶段/缺失打断窗口 | 跨阶段不串窗 | ✅ | 已写入 `control_charts.cpp` |
| Nelson estimate σ | 剔除过大 MR 后重估（I 图） | ✅ `use_nelson_estimate` | Track B3 |
| MSSD / 中位 MR 估计 | Minitab Estimate 页选项 | ✅ `mssd` + `median_moving_range` | Track B3 |
| Sixpack/二项·泊松能力上的 Tests | Minitab 默认页联动 | 🟡 能力 Sixpack 有控制图，Tests 选择是否全入口一致需审计 | Track B |
| 帮助 catalog 八条规则全文 | 应可读、带 K 值 | 🟡 控制图命令有；未统一成「规则百科」页 | Track B |

**NIST 对照：** [WECO 四规则](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm) 强调误报率上升；解释层应保留「多规则 ≠ 自动判失控/合格」——与现有 `interpretation_service` 方向一致。

### 2.2 Zone / Jaehn 区域计分

| 项 | 定义 | DataLab | 缺口 |
|---|---|---|---|
| Jaehn 1/2/4 同侧累计 ≥8 | 区域图专用；**不是** Tests 1–8 | ✅ `zone_chart` | 帮助与解读已声明非替代品 |
| 自定义权重/阈值 UI | Minitab Zone 亦有限 | ❌ 刻意不做本轮 | 见 `p1_zone_zmr_ma_charts.md` |
| Western Electric 作为 Zone 别名 | 市场无此等价 | ❌ 不应做 | — |

### 2.3 Run Chart 模式检验

| 模式 | Minitab Run Chart（聚类/混合/趋势/振荡） | DataLab | 缺口 |
|---|---|---|---|
| 中心线 | 中位数 | ✅ `run_chart` | — |
| 四类近似 P | 公式参考 | ✅ `RunChartFacts` | ⚪ 无 Minitab golden |
| 与 Runs test 合并 | 产品入口分离 | ✅ 已分离 | — |
| 控制限 / Tests 1–8 | Run Chart **不做** | ✅ 未做 | — |
| Gage 内嵌 Run Chart | 稳定性/交叉 Gage | ✅ | 与独立 `run_chart` 并存 |

### 2.4 ANOM（均值分析）

| 项 | DataLab | 缺口 |
|---|---|---|
| 正态均值 + 分组 | ✅ `anom` | — |
| 决策限 | Nelson 正态/多重比较**近似** | 帮助已标注近似 |
| 二项/泊松 ANOM | ❌ 本轮不做 | 诊断引导其他工具 |
| 与 ANOVA 后比较关系 | 解释禁止「已证明同均值」 | ✅ |

### 2.5 能力分析解读约束

| 约束 | DataLab 现状 | 待深化 |
|---|---|---|
| 禁止「过程合格 / 已证明稳定」 | ✅ 解释层广泛存在 | 统一 Facts 字段名 |
| Cpk 与 1.33 提示基准 | ✅ 仅作**项目提示**，非合格判定 | 可配置门槛仍 ❌ |
| Cp 用 σ_within、Pp 用 σ_overall | ✅ | — |
| 非正态 / Johnson 无 Within Cp | ✅ 诊断 | ⚪ golden 待导出 |
| Sixpack Tests 与能力指数同页 | 🟡 六图有；Tests 默认策略见 §2.1 | Track B |

---

## §3 剩余质量算法（❌ / 🟡 摘要）

> 完整表见 [`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md)。此处按**综合轨道**归并。

### ❌ 未实现（P2–P3 核心）

| 类别 | 算法/能力 | 备注 |
|---|---|---|
| DOE | 设计生成（全/部分析因、PB、DSD、RSM、Mixture、Taguchi…） | 现有偏「分析已有设计」 |
| DOE | 变异性分析、二项响应 DOE | — |
| MSA | Gage R&R Expanded、EMP Crossed | EMP ✅；平衡三因子 Expanded ✅；不平衡 GLM ⚪ |
| SPC | Multivariate T²、GV、MEWMA | T²+GV+MEWMA ✅ |
| 时序 | ACF、PACF、CCF、ADF；Trend 独立平台 | ACF/PACF/CCF/ADF ✅；分解/Winters/ARIMA 候选部分覆盖 |
| 功效 | 等价、DOE、容差 **样本量** | t/ANOVA/比例/方差/泊松 ✅ |
| 可靠性 | ALT、寿命回归、Probit、Weibayes、试验计划 | ALT ✅ Newton MLE；Probit 窄化 ✅；Weibayes/试验计划 ❌ |
| 建模 | GLM、Mixed、MANOVA、有序/名义 Logistic、Stepwise、PLS、Cox… | 名义 Logistic IRLS ✅；有序/Poisson ✅；Stepwise AICc/BIC ✅；PLS/Cox ❌ |
| 能力 | Batch、非参数能力、自动化能力向导 | — |
| 表格 | 独立 Tally / 交叉表工具 | ✅ `cross_tabulation` |
| 其他 | 2×2 crossover TOST、Stability studies | 低优先 / 制药向 |

### 🟡 已接入待深化（质量内）

| 项 | 说明 |
|---|---|
| `chi_square` 关联 | ✅ 百分比表 + 调整残差热图 + Facts；mosaic ⏸ |
| `binary` Logistic | HL/VIF/影响点有；逐步/验证集无 |
| KM / Log-rank | 右删失 KM ✅；`km_interval` ✅；K 组 Log-rank ✅（2026-08-22）；多失效模式无 |
| Bootstrap | 单样本 / 两样本 | `bootstrap_mean` ✅；`bootstrap_two_sample` ✅ |
| ARIMA | CSS 候选；Kalman/TSERIES 对齐 ⏸ |
| PCA | 经验 T²/Q；Jackson–Mudholkar ⏸ |
| Historical / shift-in-process | I-MR 历史参数 + 分阶段估计表 ✅（B4） |
| 方法选项可切换 / 小样本警告 | 多处有；文案与 Facts 统一 🟡 |
| 表+图同页 | 多数有；Variability 等待手工验收闭环 |

---

## §4 通用图表与 EDA（非仅质量）

> 状态基于 2026-08-21 对 `analysis_commands.cpp`（图形菜单）、`GraphService`、`ChartRenderer` 的审计。  
> 图例：✅ 独立菜单或分析内稳定产出；🟡 部分有/渲染或交互浅/需代码审计；❌ 无。

| 图表 / EDA | 典型用途（NIST EDA / Minitab Graphics / ggplot 参照） | DataLab | 说明 |
|---|---|---|---|
| 直方图 | 单变量分布 | ✅ | `histogram` 命令 + 描述/正态/能力/Sixpack 内嵌 |
| 密度曲线 | 平滑分布（`geom_density`） | ✅ | 命令 `density_plot`；高斯 KDE + Silverman |
| 箱线图 | 分组比较 | ✅ | `boxplot` + 描述统计 |
| 小提琴图 | 密度+箱线（`geom_violin`） | ✅ | 命令 `violin_plot` |
| 个体值图 | 单值序列 | ✅ | 描述统计、异常值、能力等 |
| 散点图 | 两变量关系 | ✅ | `scatter_plot` |
| 矩阵散点 / 相关图 | 多变量 EDA | ✅ | `matrix_plot`、`correlation_plot` |
| 边际图 | 散点+边缘分布 | ✅ | `marginal_plot` |
| Hexbin / 二维分箱 | 大 n 散点 | ✅ | 命令 `hexbin_plot`；矩形格（Binned Scatter） |
| 气泡图 | 三变量 | ✅ | `bubble_plot` |
| 区间图 / 森林式区间 | 组均值 CI、meta 区间 | 🟡 | `interval_plot`、ANOVA/t 区间 ✅；**无**命名 `forest_plot` / 多研究森林图 |
| 等值线 | 三变量曲面投影 | ✅ | `contour_plot` + DOE 响应 |
| 曲面 3D | RSM 曲面（Minitab rotating 3D） | 🟡 | DOE 有 **静态** `PlotKind::surface`；**无**可旋转 3D（⏸） |
| 热图 | 相关/列联 | ✅ | `heatmap_plot`、卡方热图 |
| 平行坐标 | 高维 EDA | ✅ | `parallel_plot` |
| Mosaic / spine | 分类列联 | ❌ | JMP/ggplot `geom_mosaic` 类无 |
| 条形图 | 分类计数 | ✅ | 命令 `bar_chart`；与柏拉图分流 |
| 饼图 / 圆环 | 部分/占比（**慎用**） | 🟡 | `pie_plot` ✅；**无** donut；解读需防误导 |
| 时间序列线 | 趋势 | ✅ | `time_series_plot` |
| 面积图 | 累积/组成 | ✅ | `area_plot` |
| 日历热图 | 日粒度模式 | ❌ | — |
| 正态 Q-Q / P-P | 分布假设 | 🟡 | 概率图 ✅（`probability_plot` + 分析内嵌）；**非**全分布 Q-Q 套件 |
| 经验 CDF | 分布对比 | ✅ | `ecdf_plot` |
| 残差四图 | 回归/ANOVA 诊断 | ✅ | 回归/ANOVA/DOE 内嵌（vs vs order vs histogram） |
| NIST EDA 四图 | run / lag / hist / NPP | ✅ | 命令 `eda_4plot`；探索用 |
| 自相关图 ACF/PACF | 时序诊断 | ✅ | 命令 `acf_pacf`；默认 NIST 白噪声固定带宽 |
| 控制图 | SPC | ✅ | 质量菜单（非「图形」） |
| 柏拉图 | 缺陷 Pareto | ✅ | 质量+图形 |
| 多图拼版 / Graph Builder | JMP 拖拽探索 | ❌ | ⏸ 见 §6 |
| 图属性编辑 | 色/线/点/主题 | 🟡 | 侧栏有；注释/拖拽布局 ⏸ |

**市场对照（不克隆 API）：**

- **Minitab Graphics / Graph Builder 候选图型**（[Data Visualizations](https://support.minitab.com/en-us/minitab-solution-center/dashboard/asset-library/data-visualizations/)，访问 2026-08-21）：Histogram、Probability、Boxplot、Interval、Individual Value、Variability、Line、Pareto、Bar、Pie、Scatter、Binned Scatter、Bubble、Matrix、Correlogram、Parallel Coordinates、Heatmap、Wafer、Tabulated Statistics、Time Series、Stacked Area、KPI、Table。DataLab **已有**多数基础型（直方图/箱线/散点/矩阵/平行/热图/气泡/区间/概率/ECDF/时序/面积/饼/等值线/密度/Hexbin/Violin/通用条形等）及 **Correlogram**（`correlogram`）；**仍缺或仅部分**：Wafer、KPI 看板、Graph Builder 画廊本身。  
- **JMP Graph Builder** 强调拖拽 zones、即时改 geom、Local Data Filter（[Graph Builder Help](https://www.jmp.com/support/help/en/19.1/jmp/graph-builder.shtml)）；DataLab 采用「命令 + 角色列 + 侧栏属性」，不做画布拖拽。  
- **ggplot2** 的 `geom_histogram` / `geom_density` / `geom_violin` / `geom_hex` 等（[Package index](https://ggplot2.tidyverse.org/reference/index.html)）可作为**geom 清单**对照缺口，而非 API 设计。

**NIST EDA 启示：** [EDA 章](https://www.itl.nist.gov/div898/handbook/eda/eda.htm) 强调 4-plot（run sequence、histogram、normal probability、lag）与假设检验分离；DataLab 已以命令 `eda_4plot` 打包同页四图（探索用，非受控/正态证明）。

---

## §5 建议 /goal 队列

四条 Track 可并行，但每轮 **锁定 3～4 项竖切闭环**（research md → domain → Facts → service → commands → interpretation → tests `# source: formula_reference` → help → backlog/本文件更新）。

### Track A — 质量 P2（默认下一批）

**优先级：** 高（与 backlog §12 一致）

| 顺序建议 | 目标 | 交付证据 |
|---|---|---|
| A1 | DOE 设计生成（至少 2^k 全因子 + 部分析因） | ✅ 命令 `doe_factorial`；设计矩阵+生成器+别名；与 `doe_response` 衔接 |
| A2 | RSM 分析（中心复合/已有设计表） | ✅ 命令 `rsm_response`；系数/等值线·曲面/残差；`RsmFacts` |
| A3 | ACF/PACF + 时序诊断图 | ✅ 命令 `acf_pacf`；白噪声固定带宽说明 |
| A4 | 等价/DOE/容差功效 | ✅ `t_power` mode 扩展 + PowerFacts/曲线 |

**禁止偷懒：**

- 禁止只做菜单占位无设计矩阵输出  
- 禁止 RSM 无残差诊断图  
- 禁止 ACF 图无默认置信带说明  
- 禁止功效页无 `PowerFacts` 与曲线  
- 禁止未更新 backlog §12 就标 ✅  

### Track B — 判断规则深化

**优先级：** 中高（质量体验杠杆）

| 目标 | 说明 | 状态 |
|---|---|---|
| B1 | 统一 **Special Cause Rules Catalog** 进 help JSON（Tests 1–8 + 图种适用表 + 默认策略差异说明） | ✅ help `special_cause_rules` |
| B2 | UI 默认策略选项：`minitab_like`（仅 Test 1）vs `all_applicable`（现状） | ✅ `rule_policy` 可序列化 + 诊断 |
| B3 | I 图 **Nelson estimate** + MSSD/中位 MR（对照 Minitab Estimate 页） | ✅ Nelson + MSSD；中位 MR 已有 |
| B4 | Historical 参数页（μ/σ 按阶段/子组导入）与 Tests 联动文档 | ✅ 窄化：I-MR「历史参数与分阶段估计」表 |
| B5 | Run Chart / ANOM / Zone 规则在「公式与来源」页的交叉链接 | ✅ 窄化：`special_cause_rules` 正文交叉链接 |

**禁止偷懒：**

- 禁止把 Zone Jaehn 写成 Tests 1–8 而不写诊断  
- 禁止改默认规则却不序列化/不解释误报风险  
- 禁止 Nelson estimate 只改 CL 不改诊断表  
- 禁止帮助正文写「见 md」  

### Track C — 综合图表（EDA 加宽）

**优先级：** 中（产品差异化）

| 顺序建议 | 目标 |
|---|---|
| C1 | 独立 **密度图 / KDE**（或直方图+密度层选项） | ✅ `density_plot`；Silverman；`EdaPlotFacts` |
| C2 | **Hexbin** 大样本散点 | ✅ `hexbin_plot`；矩形二维分箱 |
| C3 | **Violin**（或箱线+密度组合） | ✅ `violin_plot`；镜像 KDE + 箱线 |
| C4 | 通用 **条形图**（与柏拉图分流） | ✅ `bar_chart`；无排序/Cum% |
| C5 | **EDA 四图** 打包命令（可选，单变量） | ✅ `eda_4plot`；NIST 四图同页 |
| C6 | 日历热图（若有时间列需求） |

**禁止偷懒：**

- 禁止只加 ChartKind 不接线 GraphService + renderer + 手工验收项  
- 禁止 mosaic/spine 用错误卡方图糊弄  
- 禁止饼图无「慎用」解读  
- 禁止把 DOE 静态曲面冒充可旋转 Graph Builder  
- 禁止不做 `source_row` / complete-case 契约  

### Track D — 建模加宽

**优先级：** 中低（非汽车质量主路径，按需启动）

| 目标 | 说明 |
|---|---|
| D1 | 有序 Logistic（至少比例 odds + 拟合表） | ✅ `ordinal_logistic` |
| D2 | 交叉表 / Tally 独立工具 | ✅ `cross_tabulation`；频数+百分比；不做卡方 |
| D3 | 卡方关联深化（标准化残差、可选 mosaic） | ✅ 百分比表 + 调整残差热图 + Facts；mosaic 仍 ⏸ |
| D4 | GLM 入口（Poisson/计数响应） | ✅ `poisson_regression` |
| D5 | 可靠性：左/区间删失 KM | ✅ `km_interval` |

**禁止偷懒：**

- 禁止 Logistic 无 HL/影响点诊断  
- 禁止 GLM 无残差/偏离度表  
- 禁止交叉表破坏现有关联分析命令  
- 禁止 chi_square 边界改动除非本轮明确包含  

---

## §6 明确不做（保持延后）

与 [`deferred-capability-agreement.md`](deferred-capability-agreement.md) §5 及 backlog §13 一致；**清空 backlog ❌ 不算失败**。

| 类别 | 不做项 |
|---|---|
| 区间/精确 P | Blaker / Adjusted Blaker；泊松 Blaker |
| 控制图数值 | Kalman / TSERIES 与 Minitab 对齐；Jackson–Mudholkar T²/Q **解析**限 |
| 图表产品 | **Graph Builder 全量拖拽**；图表注释/自由拼版；**可旋转 3D** |
| 统计模块 | **Predictive Analytics** 全模块；Assistant 向导；宏/Python/R 集成 |
| 其他 | Nemenyi **独立**命令；精确 studentized-range；重构阶段 5/6（PlotSpec 合一、CI、i18n）除非挡路 |
| golden | 任何未从 Minitab 导出的数值写入 `VALIDATION_MATRIX.md` |

---

## §7 维护规则（未来 agent 必读）

1. **每竖切闭环一项**：更新 [`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) 对应行、[`quality-algorithms-acceptance.md`](../quality-algorithms-acceptance.md)、[`algorithm-chart-gap-matrix.md`](algorithm-chart-gap-matrix.md)；若属综合图表/规则，**同步改本文件 §2 / §4 / §5 状态列**。  
2. **新发现的市场项或图表型**：先写入 backlog 或本文件 §4，再写代码；禁止只做实现不登记。  
3. **研究 md 先行**：P2+ 仍遵循 `docs/research/p1_*.md` 或新 `p2_*.md` 模式；URL + 访问日期；`formula_reference ≠ golden`。  
4. **/goal 终态（质量）**：backlog §12 产品范围内 ❌/🟡 清到可接受水位 + §6 书面延后；**不是** Minitab Feature List 100% 克隆。  
5. **/goal 终态（综合）**：Track C 至少补齐 §4 中标记 ❌ 的**高杠杆**项（密度/hexbin/violin/通用条形/EDA 四图）中的商定子集；Track B 完成规则 catalog + 默认策略文档化。  
6. **访问日期变更**：更新文首来源表与所引 Minitab/NIST 链接日期。  
7. **本文件与 backlog 冲突时**：算法名与 ✅/❌ 以 **backlog 为准**；图表与 Track 优先级以 **本文件为准**。

---

## §8 下一波 Track E–H（指针）

P3+、经典可审计 ML、图表加宽、开源功能灵感的一次性调研与 `/goal` 批次建议：

→ [`next-wave-algorithms-charts-ml-oss.md`](next-wave-algorithms-charts-ml-oss.md)

| Track | 状态（2026-08-21） |
|---|---|
| E1/E3、F1、G1/G2 | ✅ |
| H：kmeans/adf/poisson/hclust/ordinal/lda/ccf/stepwise/km_interval/PB | ✅ |
| **Wave-4（2026-08-22）**：nonparametric PPM/hist；reliability CIF+Gray；`cox_regression`；logistic stepwise | ✅ |
| E2 RF / F2–F3 / Best subsets / CCD·BBD / TreeNet | ❌ / ⏸ |

该文档定义 Track **E（经典 ML）/ F（Bootstrap·分布）/ G（图表）/ H（P3 建模·多元·可靠性）**；**不**复活 §6 明确不做项。

---

## 附录：命令 ID 速查（图形 + 质量相关）

| 命令 ID | 菜单 |
|---|---|
| `histogram`, `boxplot`, `scatter_plot`, `matrix_plot`, `marginal_plot`, `parallel_plot`, `heatmap_plot`, `contour_plot`, `time_series_plot`, `area_plot`, `pie_plot`, `probability_plot`, `ecdf_plot`, `interval_plot`, `correlation_plot`, `bubble_plot` | 图形 |
| `imr`, `xbar_r`, `xbar_s`, `p_chart`, `np_chart`, `c_chart`, `u_chart`, `ewma`, `cusum`, `zone_chart`, `z_mr`, `moving_average`, `run_chart`, `pareto`, `cause_and_effect`, `anom`, … | 质量/控制图 |

（完整 wiring 见 [`algorithm-wiring-index.md`](../algorithm-wiring-index.md）。）
