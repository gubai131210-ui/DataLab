# DataLab 产品演进调研：市场功能、呈现方式、架构与性能

> 研究日期：2026-08-22（UTC+8）  
> 访问日期：2026-08-22  
> 目的：为后续 **/goal 模式** 提供可执行队列——在保持**科学性/准确性**、**分层可持续**、**低耦合**、**运行快速**的前提下，把 DataLab 做成多功能且稳定的桌面统计质量平台。  
> **不做**：克隆 Minitab/JMP API、宣称 vendor oracle、嵌入 Python/R 运行时进 dist、假 PDF/A·UA 合规。

---

## §0 与现有文档的关系

| 文档 | 分工 |
|------|------|
| [`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) | 市场算法名 ↔ 命令 ↔ ✅/🟡/❌ |
| [`comprehensive-analytics-roadmap.md`](comprehensive-analytics-roadmap.md) | 综合轨道：规则目录、EDA、建模加宽、Track A–H |
| [`vertical-slice-algorithms-and-report-product-plan.md`](vertical-slice-algorithms-and-report-product-plan.md) | 双产品线：算法竖切 + 报告产品化 Phase 0–7 |
| [`architecture-review.md`](../architecture-review.md) | 上帝对象、分层走样、重构 ROI |
| [`reference-implementation-index.md`](reference-implementation-index.md) | Python 手算/reference，非 golden |
| **本文件** | **跨领域产品调研** + **呈现/UX 学习** + **架构/性能原则** + **/goal 队列草案** |

**DataLab 当前优势（应保留并放大）：**

- `domain` 纯 C++、Facts + EvidenceBundle + 三模板报告 + 双语 catalog（ADR 0006/0009）
- 竖切验收纪律：§3.1 61、deepen 37、interpretation 316/316、reference 脚本 5 项
- 诚实 gate（Box-Cox/Johnson/能力 pass-fail、PDF/A·UA not_validated）

---

## §1 市场格局与定位（Primary Sources）

| 产品 | 典型用户 | 强项 | 弱项/与 DataLab 差异 | 来源 |
|------|----------|------|----------------------|------|
| **Minitab Statistical** | 六西格玛、过程验证、PPAP | Assistant 决策树、Graph Builder 图库切换、能力/MSA/DOE 深度、Automated Capability（新） | 订阅贵；Real-Time SPC 为**云**产品；Predictive/ML 为独立模块 | [Features](https://www.minitab.com/en-us/products/minitab/features/) · [What's New](https://www.minitab.com/en-us/products/minitab/whats-new/) · [Assistant](https://www.minitab.com/en-us/products/minitab/assistant/) |
| **JMP** | 研发/工程探索、多变量 | Graph Builder 拖拽分区（X/Y/Group/Wrap/Color）、**动态链接 brushing**、Dashboard、Graph→Fit Y by X | 质量/SPC 工作流不如 Minitab「出厂即 PPAP」；许可模式不同 | [Graph Builder](https://www.jmp.com/support/help/en/19.1/jmp/graph-builder.shtml) · [Why JMP PDF](https://www.jmp.com/content/dam/jmp/documents/en/software/jmp/jmp14/why-jmp-software.pdf) |
| **QI Macros** | Excel 内快速 SPC | Chart/Stat **Wizard 自动选型**、模板填数即出图、低学习曲线 | 非独立统计平台；深度与可审计性有限 | [SPC in Excel](https://www.qimacros.com/spc-software-for-excel/statistical-control-software/) · [vs Minitab](https://www.qimacros.com/qi-macros/minitab-comparison/) |
| **Minitab Real-Time SPC** | 产线监控 | 工位 Dashboard、OOS/OOC/OoA 汇总、告警、导出 MWX/CSV 进桌面 Minitab | **实时+云**；与 DataLab 离线桌面定位不同，但**呈现与角色视图**可学 | [Real-Time SPC](https://www.minitab.com/en-us/products/real-time-spc/) · [Station dashboard](https://support.minitab.com/en-us/real-time-spc/reports-and-dashboards/station-dashboard/) |
| **NIST/SEMATECH EDA** | 方法学权威 | **4-plot** 假设检验、控制图 WECO 与误报讨论 | 非产品 UI | [4-Plot](https://www.itl.nist.gov/div898/handbook/eda/section3/eda3332.htm) · [WECO](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm) |

**定位建议（一句话）：** DataLab = **离线、可审计、汽车质量主路径** 的 Minitab 类深度 + **受控 Graph Builder 式探索** + **工程师/客户/审计三模板报告**；**不**做云 RTSPC 全栈，可做「导入 CSV/数据库 → 离线监控视图」的轻量变体。

---

## §2 值得学习的功能（按优先级）

### 2.1 高价值、与现有架构对齐

| 功能 | 市场参考 | DataLab 现状 | 建议 |
|------|----------|--------------|------|
| **Graph Builder 式探索** | Minitab/JMP：同一数据切换 geoms、分面、颜色 | Phase 7 分面 scatter/bar/density/hexbin + hidden/excluded 双口径 | 扩展 **Graph 角色页**（独立页，勿堆单页）：X/Y/Group/Facet/Color 槽 + geom 画廊；Facts 不变，仅改 `graph_assembly` |
| **Assistant 式「报告卡片」** | Summary / Diagnostic / Report Card（假设、样本量、功效） | Interpretation + EvidenceBundle + gate bullets | 新增 **Analysis Report Card** 块：假设检查列表 + 链到 `formula_reference` + 禁止 pass/fail 越权 |
| **4-plot / Sixpack 思维** | NIST：Run + Lag + Hist + NP；能力 Sixpack | 能力 Sixpack 部分有；无统一 4-plot 命令 | 命令 `assumption_panel` 或能力页固定四格；残差/原始数据皆可 |
| **规则百科 + 可选 Tests** | Minitab Tests 1–8 默认因图种而异 | `special_cause_rule_catalog` + 316 interpretation | UI：**规则多选页** + 帮助侧栏；输出 `rule_id` 稳定名（已有） |
| **Wizard 降低选型成本** | QI Macros Chart/Stat Wizard | 46 命令 + 角色对话框 | **轻量 Wizard**：根据列类型推荐命令（只推荐，不黑盒算） |
| **工位/指标 Dashboard（离线）** | RTSPC Current Performance Summary | 无 | 项目内 **监视列表**：多特性 Cpk/OOC 汇总表 + 跳转分析（只读 SQLite 快照） |
| **公式与文献页** | Minitab Help Methods；NIST 公式 | 分散 `docs/research/*-formulas.md` + `# source: formula_reference` | **应用内 Formula Registry 页**：id → 公式 LaTeX/图片 → 外链 NIST/Minitab support（用户已提需求） |
| **图表/表格复制** | Excel/JMP 右键复制 | 工作表交互已有；图表复制弱 | 输出区 **Copy chart / Copy table**（PNG + TSV）；与报告 PDF 裁剪规则一致 |
| **Automated Capability 思路** | Minitab 22 自动分布族筛选 | Box-Cox/Johnson gated + distribution_identification | 深化 **门控自动化**：先 AD/IDI → 再变换 → 再能力；全程 diagnostic，不自动判合格 |

### 2.2 中价值、分阶段

| 功能 | 说明 | 建议阶段 |
|------|------|----------|
| Post-analysis 工具栏（预测/换模型） | Minitab Predictive 模块 UX | P3+ 仅对已有回归/RSM 做「保存预测列」 |
| 交互 Pareto / Variability | Graph Builder 2024+ | Graph 加 Pareto geom + 分层保修已有基础 |
| 文本挖掘 / RegEx 列 | JMP Text Explorer | ⏸ 非汽车质量主路径 |
| What-If / 响应优化器 | JMP Profiler；Minitab Response Optimizer | 已有 `response_optimization` 域；需独立 **优化器页** |
| 本地数据筛选 → 统计 | JMP Local Data Filter + Fit Y by X | Graph 选区 → 临时子集 Facts（`row_visibility` 已有语义） |

### 2.3 明确不做（避免 scope 爆炸）

- Minitab Assistant **逐步向导克隆**（可借鉴卡片结构，不做决策树全量）
- Graph Builder **全拖拽**（先做受控槽位 + 画廊）
- Predictive Analytics / AutoML / TreeNet 进 dist
- Real-Time SPC 云、多租户、短信告警
- 可旋转 3D、宏语言、内嵌 R/Python 运行时（见 [`deferred-capability-agreement.md`](deferred-capability-agreement.md)）

---

## §3 呈现方式（UX）调研

### 3.1 JMP Graph Builder 模式（受控子集）

**核心交互：**

1. **Graph zones**：X、Y、Group X/Y、Wrap、Color、Overlay、Freq（[JMP 教学 PDF](https://www.jmp.com/content/dam/jmp/documents/en/academic/learning-library/03-graphical-displays-and-summaries/03-09-interactive-graphing-with-graph-builder.pdf)）
2. **Element 画廊**：仅显示当前变量类型**适用**的 geoms（不可用项置灰）
3. **Hidden vs Excluded**：隐藏只影响可见性；排除影响拟合/统计（[JMP Element Types](https://www.jmp.com/support/help/en/19.1/jmp/element-types-and-options.shtml)）——与 DataLab Phase 7 契约一致，应在 UI **显式两个开关**
4. **Brushing 动态链接**：选点同步多视图（JMP「magic」）——可先做：输出页 **linked selection**（同 worksheet 行高亮）

**DataLab 呈现原则：**

- 分析 setup / graph properties / report template **分页**（禁止单页堆控件）
- 默认 **中文 UI + 报告 locale 独立**（ADR 0006/0009）
- 客户模板：**少表少图**；工程师：**规则+诊断**；审计：**Evidence 附录**

### 3.2 Minitab Assistant 报告结构

| 报告块 | 内容 | DataLab 映射 |
|--------|------|--------------|
| Summary Report | 结论 + 上下文评论 | `InterpretationSection` lead bullets |
| Diagnostic Report | 离群、功效、额外细节 | `DiagnosticMessage` + param 表 |
| Report Card | 正态性、样本量、假设违反 | 新增 `AssumptionCheck` Facts 列表 |
| 一键 PowerPoint | 幻灯片导出 | ⏸；优先 **PDF+manifest** 已落地 |

### 3.3 科学呈现：NIST 4-plot

一次展示四图判断：固定位置、固定变异、随机性、分布（[解读](https://www.itl.nist.gov/div898/handbook/eda/section2/eda24.htm)）。

**验收：** 对残差或单变量输出 2×2 页；每张图带 **一句判读**（catalog id），不自动「过程合格」。

### 3.4 表格与工作表

| 能力 | 用户期望 | 建议 |
|------|----------|------|
| 复制图表 | 贴到 Word/PPT | `QClipboard` PNG；保留 Display N / hidden 脚注 |
| 清除单元格 | Excel 式编辑 | Worksheet 命令 + undo 栈；写回 SQLite 快照 |
| 百万行 | 不卡死 | 虚拟滚动已有方向；分析走 **列抽取 + 流式**（database keyset 已有） |

---

## §4 科学性与准确性

### 4.1 证据分层（保持）

```
domain 数值 → Facts → EvidenceBundle → ReportProfile 裁剪 → PDF/manifest/audit JSON
```

- **formula_reference** ≠ **vendor_oracle** ≠ **golden**（[`VALIDATION_MATRIX.md`](VALIDATION_MATRIX.md)）
- Gate bullet 优先保留（customer 截断也不丢 `:gate:*`）
- 解释层只读 Facts（[`interpretation_service`](../../src/application/interpretation_service.cpp)）

### 4.2 应对「Automated Capability / AI 摘要」潮流

Minitab 22 强调自动化与 AI 图表摘要（[新闻稿 2024-03](https://www.minitab.com/en-us/company/press-releases/minitab-statistical-software-with-enhanced-ai-capabilities/)）。

**DataLab 立场：**

- ✅ 自动化 **候选分布/变换筛选** + 完整 diagnostic 链
- ✅ 报告 **固定 catalog 句子**，可审计、可本地化
- ❌ LLM 自由生成结论写进 PDF
- 🟡 可选「工程师备注」字段（人工输入，非 AI）

### 4.3 数值验证策略

| 层级 | 工具 | 用途 |
|------|------|------|
| 单元/域 | Qt Test + `quality_statistics_test` 等 | 公式、边界、gate |
| Reference | `scripts/*_reference.py` | 手算再生 |
| 报告 | `ReportExportPhase2Test` 字节扫描 | 三模板/双语/裁剪 |
| 商业对齐 | ⏸ vendor oracle 冻结流程 | 无导出不得声称 |

新算法：**research md（含 Primary URL）→ domain → Facts → 竖切测试 → help catalog**（backlog 深度闭环规则）。

---

## §5 架构可持续性与耦合

依据 [`architecture-review.md`](../architecture-review.md) 与 ADR 0001：

### 5.1 目标分层（依赖单向）

```
ui → application → {infrastructure, reporting} → domain
```

**禁止：** infrastructure → ui（如 pdf writer include chart_adapter 需消除）。

### 5.2 上帝对象拆分（高 ROI）

| 对象 | 规模 | 拆分方向 |
|------|------|----------|
| `analysis_service.cpp` | ~15k 行 | 按 **命令族** 拆：`analysis_service_doe.cpp`、`..._reliability.cpp`、`..._capability.cpp` + 薄门面 |
| `report_localization.cpp` | ~11k 行 | 按 **diag / param / bullet** 拆文件；保留统一 `localize_*` 入口 |
| `report_text_catalog` | 2551 条 | 已拆 16 part；sync 脚本读 part |

### 5.3 深模块接口（便于测试与 AI 导航）

- Domain：`XxxOptions` + `XxxResult` + `std::vector<Diagnostic>`
- Application：命令 dispatch 表 + 无 Qt
- UI：仅绑定 Facts/参数，不算统计

### 5.4 构建与交付

- MinGW 大 TU：`-Wa,-mbig-obj` + ASCII `%TEMP%`（已入 CMake）
- Catalog 分 part（已做）
- 发布：`tools/package_dist.ps1` → `dist/`（windeployqt + MinGW runtime）

---

## §6 运行快速性

| 瓶颈 | 对策 |
|------|------|
| 超大 `.cpp` 编译 | 拆文件 + big-obj；避免单文件再超 5k 行 |
| 2551 条 catalog 静态初始化 | 16 part + 运行时 merge（已做）；远期可 JSON 懒加载 + 内存缓存 |
| 百万行 worksheet | 虚拟视图；分析前 **列统计摘要**；database **keyset 分页** |
| 报告 PDF | 表/plot 裁剪 + customer profile 限流（已有） |
| Graph 分面 | `max_plots` + 预聚合（hexbin/density） |

**性能验收：** 固定 fixture 上记录 **分析耗时 + PDF 字节数** 进 CI 日志（不要求 ms 级 oracle）。

---

## §7 建议 /goal 队列（供后续执行）

> 每项须：**独立竖切**、**不破坏导入 A→B 契约**、**新页而非堆控件**、**reference 可写则写**。

| Track | 目标 | 关键交付 | 参考 |
|-------|------|----------|------|
| **G1 公式注册表 UI** | 应用内 Formula Registry | 新页：搜索 id、显示公式、链到 research md / NIST | 用户诉求；`report_text_catalog` evidence 公式 id |
| **G2 图表/表格复制** | 输出可粘贴 | Copy PNG/TSV；尊重 hidden/excluded 脚注 | JMP/Minitab 导出 |
| **G3 Graph 受控 Builder** | 分面+geom 画廊 | 独立 `GraphBuilderPage`；对接 `graph_service` | §3.1；计划 [`goal-wave-2026-08-23-g3-graph-builder-plan-and-mega-prompt.md`](goal-wave-2026-08-23-g3-graph-builder-plan-and-mega-prompt.md) |
| **G4 4-plot / Report Card** | 假设可视化 | 命令或能力子页；Facts `AssumptionCheck` | NIST 4-plot |
| **G5 AnalysisService 拆分** | 降耦合 | 按命令族拆 cpp；dispatch 表 | architecture-review §3.1 |
| **G6 命令 Wizard** | 降学习曲线 | 列类型 → 推荐命令列表（不自动跑） | QI Macros Wizard |
| **G7 离线监视摘要** | 多特性一览 | 项目级 Cpk/OOC 表；跳转分析 | RTSPC Summary 离线版 |
| **G8 Worksheet 编辑** | 清除/撤销 | 清除单元格 + undo；快照一致 | Excel 交互 |

**与双产品线的关系：**

- 报告线：G1/G2/G4 直接增强 Phase 3–7
- 算法线：继续 backlog P3 + [`comprehensive-analytics-roadmap.md`](comprehensive-analytics-roadmap.md) Track B–H

### §7.1 验收节奏：**连续交付 · 末尾统一测**（2026-08-22 起）

> 权威清单：[`samples/product_evolution/unified_track_acceptance_plan.md`](../../samples/product_evolution/unified_track_acceptance_plan.md)  
> **大批量 Goal 操作手册**：[`goal-execution-framework.md`](goal-execution-framework.md)（Wave 粒度 · 多 Agent · 调研纪律）

| 阶段 | 规则 |
|------|------|
| **Track 交付**（G3–G8 默认） | 竖切 + 文档 + **脚本预检 OK** 即算该 Track 交付完成；**不要求**每个 Track 结束后立即 Qt Creator 手工测 |
| **Wave 交付**（算法/优化批） | 每 Wave **3–6 项**连续竖切；Wave 末 verify PASS；**禁止**只做 1 项就结束 Goal |
| **统一验收门** | 你准备好后 **只跑一轮**：Run CMake → Build All → 各 Track 测试脚本 → §5 手工清单 → §6 签署 |
| **G1+G2** | 已于 2026-08-22 单独签收；统一测时作 **回归** 勾选，不阻塞后续 Track 开发 |

**禁止：** 每完成一个 Track/Wave 就停住等 Qt Creator 全量签收才允许下一 Wave（除非用户显式要求单 Track 验收）。

---

## §8 给下一对话的提示词模板（可复制）

> **大批量 / 多 Wave / 多 Agent** 请用 [`goal-execution-framework.md`](goal-execution-framework.md) §9 的 **Mega 模板**（算法 Wave-2、G3–G8、混合 Wave 已预填候选）。  
> Wave 计划文件：复制 [`goal-wave-template.md`](goal-wave-template.md) → `docs/research/goal-wave-YYYY-MM-DD-*.md`

### 8.1 单 Track（小范围）

```markdown
/goal
目标：执行 product-evolution §7 的 Track G?（指定编号）。

约束：见 goal-execution-framework.md §6 禁止偷懒 + unified_track_acceptance_plan.md。
交付：竖切 + verify 脚本 PASS + 更新 acceptance §2。
不要 cmake/ctest/commit；不要每 Track 停 Qt Creator。
请先读：goal-execution-framework.md、unified_track_acceptance_plan.md、architecture-review.md（若 G5）。
```

### 8.2 算法 Wave（3–5 项 · 一次不停）

```markdown
/goal
## Wave 锁定（本 Goal 全部完成才 complete）
- Wave-1（3–5 项）：从 minitab-market-algorithm-backlog.md §12 + next-wave §2 选取 ❌/🟡
  - 示例：nominal_logistic、nonparametric_capability、stepwise 深化、…
- 每项完整竖切；网上 Primary URL → research md

## 多 Agent
- 启动：Task explore 扫 wiring/backlog
- 实现：主 agent 连续竖切（domain 多用 cpp-coding skill）
- Wave 末：Task bugbot + verify_*_track.py PASS

## 必读
1. docs/research/goal-execution-framework.md
2. samples/product_evolution/unified_track_acceptance_plan.md
3. docs/research/minitab-market-algorithm-backlog.md §12
4. docs/research/next-wave-algorithms-charts-ml-oss.md

## 禁止
- 禁止只做 1 个算法就 UpdateGoal complete
- 禁止跳过 Web 调研与 Primary URL
- 禁止每算法停 Qt Creator

## 交付
- goal-wave-YYYY-MM-DD-*.md + 各 research md + verify 脚本 + acceptance §2
```

### 8.3 产品 + 架构 Wave（G3 或 G5 独占）

```markdown
/goal
## Wave 锁定
- Wave-1：G3 Graph Builder（独占；新页 GraphBuilderPage）
- Wave-2（可选同 Goal）：G4 4-plot + G5 AnalysisService 拆分第一阶段

## 调研
- Minitab Graph Builder / JMP Graph Builder Primary URL → research md
- architecture-review.md §3.1 拆分边界

## 多 Agent / 验收
同 goal-execution-framework.md §2–§5

禁止偷懒：§9 全文 + 禁止单页堆控件。
```

---

## §9 禁止偷懒清单（/goal 执行时必须粘贴）

1. 禁止只做 UI 壳不算 domain/Facts  
2. 禁止跳过 interpretation 与 catalog 双语  
3. 禁止把 Minitab 数值当 golden 无导出  
4. 禁止单页堆叠超过一层主流程控件  
5. 禁止破坏 `row_visibility` hidden/excluded 语义  
6. 禁止 infrastructure 新增对 ui 的 include  
7. 禁止合并 customer/engineer/audit 报告裁剪为单模板  
8. 禁止省略 help catalog / algorithm_help.json  
9. 禁止大 catalog 单文件（>500 条/translation unit）  
10. 禁止宣称 PDF/A·UA 合规无验证器  
11. 禁止每 Track 强制停跑 Qt Creator 才允许下一 Track（默认 **连续交付 · 末尾统一测**，见 `unified_track_acceptance_plan.md`）  
12. 禁止 Goal 只完成 1 个算法/1 个小优化就 complete（见 `goal-execution-framework.md` §1）  
13. 禁止跳过网上 Primary URL 调研（WebSearch → research md 访问日期）  

---

## §10 来源索引（Primary / 高信任）

| # | 主题 | URL | 访问 |
|---|------|-----|------|
| 1 | Minitab 功能总览 | https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-22 |
| 2 | Minitab 新版本 | https://www.minitab.com/en-us/products/minitab/whats-new/ | 2026-08-22 |
| 3 | Minitab Assistant | https://www.minitab.com/en-us/products/minitab/assistant/ | 2026-08-22 |
| 4 | Minitab Graph Builder 说明 | https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/graph-builder/overview/ | 2026-08-22 |
| 5 | JMP Graph Builder | https://www.jmp.com/support/help/en/19.1/jmp/graph-builder.shtml | 2026-08-22 |
| 6 | JMP Graph Builder 教学 | https://www.jmp.com/content/dam/jmp/documents/en/academic/learning-library/03-graphical-displays-and-summaries/03-09-interactive-graphing-with-graph-builder.pdf | 2026-08-22 |
| 7 | JMP Hidden/Excluded | https://www.jmp.com/support/help/en/19.1/jmp/element-types-and-options.shtml | 2026-08-22 |
| 8 | QI Macros SPC | https://www.qimacros.com/spc-software-for-excel/statistical-control-software/ | 2026-08-22 |
| 9 | Minitab Real-Time SPC | https://www.minitab.com/en-us/products/real-time-spc/ | 2026-08-22 |
| 10 | RTSPC Station Dashboard | https://support.minitab.com/en-us/real-time-spc/reports-and-dashboards/station-dashboard/ | 2026-08-22 |
| 11 | NIST 4-Plot | https://www.itl.nist.gov/div898/handbook/eda/section3/eda3332.htm | 2026-08-22 |
| 12 | NIST WECO 规则 | https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm | 2026-08-22 |
| 14 | Multi-agent 协调 | https://www.augmentcode.com/guides/multi-agent-ai-software-development | 2026-08-22 |
| 15 | Goal 执行框架（本仓库） | docs/research/goal-execution-framework.md | 2026-08-22 |

---

**文档状态：** G1/G2 ✅；算法批 A1–A3 ✅（2026-08-22）。Goal 框架：[`goal-execution-framework.md`](goal-execution-framework.md)。验收：**连续交付 · 末尾统一测** → [`unified_track_acceptance_plan.md`](../../samples/product_evolution/unified_track_acceptance_plan.md)。
