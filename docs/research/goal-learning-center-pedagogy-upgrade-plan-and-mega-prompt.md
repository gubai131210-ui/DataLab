# Goal：学习中心教学升级（专用数据集 + 黑带文案 + Agent 教学闭环）

> **用途**：新开一场 Goal 对话时的**唯一权威操作手册**（本轮教学升级）。  
> **状态**：框架与文案模板已由用户在 Canvas 确认（2026-09-03）；**若干执行参数仍待用户拍板**（见 §0）。  
> **Canvas 标杆**：`~/.cursor/projects/d-QT-CppPrograms-DataLab/canvases/learning-center-tutorial-example.canvas.tsx`  
> **母框架**：[`goal-execution-framework.md`](goal-execution-framework.md)  
> **已落地基线（勿推倒重来）**：[`goal-learning-center-black-belt-plan.md`](goal-learning-center-black-belt-plan.md) — 学习中心页面 / SQLite / 导入 / 对齐测试 **已存在**；本 Goal 是**内容与 schema 升级**，不是重建学习中心。  
> **研究笔记（旧）**：[`learning-center-research-notes.md`](learning-center-research-notes.md)、[`learning-center-dataset-mapping.md`](learning-center-dataset-mapping.md)

---

## §0 开场前必须问用户（有一项未答就不要开工）

| # | 问题 | 推荐默认（用户可改） |
|---|------|----------------------|
| Q1 | **全程锁定哪一个模型？**（六 Agent 禁止中途换模型） | 由用户在新对话首条消息写死模型名；子 Agent 全部 `model: "inherit"` |
| Q2 | **范围节奏**：先做 `imr` 标杆验收，再按菜单分 Wave；还是一次铺开全部 ~184？ | **先 imr 金标 → 再按包 Wave（每 Wave 8–15 条）** |
| Q3 | **UI**：Wave-0 是否**必须**改 `LearningCenterPage` 最小渲染（glossary / dialog 详解可见）？折叠披露与练习勾选是否推迟到本 Goal 末段「UI 增强切片」（**不要叫 Wave-2**，以免与 §7 内容包重名）？ | **Wave-0 必做最小渲染**（否则「页面能读到关键词」无法验收）；折叠/练习交互 = 可选「UI 增强切片」 |
| Q4 | **「一算法一专用集」严格度**：真 · 1:1，还是同构极小共享？ | **默认 1 主命令 1 表**；例外仅当 `roles`/`inputs`+埋点完全一致（例：若未来拆出并行 id；**现网只有 `imr` / `imr_rs`，没有单独 Individuals id**） |
| Q5 | **旧共享表**（如 `smt_paste_height`）：删除、保留「综合练习」、还是迁移别名？ | **库内可保留**；教程默认改链新专用 id；重跑 mapping 脚本不得把主链冲回共享宽表 |
| Q6 | **中文路径编译**：是否仍由用户本机 Qt Creator / `package_dist`，Agent 只跑 Python verify？ | **是** |

**编排者纪律**：用户未回复 §0 前，只允许读代码与写调研草稿，**禁止**改 schema / 大批量生成数据 / 宣称 Goal 完成。

### §0.1 现网硬约束（不必再问用户；写进 Wave plan 文首）

| # | 约束 | 为何 |
|---|------|------|
| H1 | `catalog_version` 现为字符串 **`learning-center-v1`**。升级时必须**同时**改 `tools/build_learning_center_db.py` 的 `META_VERSION` 与 `LearningDatasetStore::kExpectedCatalogVersion`（建议 `learning-center-v2`）。只改 sqlite 会导致页面报版本不匹配并禁用导入。 |
| H2 | 工作表名公式是 `demo_{dataset_id}`（`LearningCenterPage` + store）。金标 **`dataset_id` 用 `imr_spi_shift`**，导入后显示 `demo_imr_spi_shift`。**禁止**再把 id 写成 `demo_imr_spi_shift`（会变成 `demo_demo_…`）。Canvas 里的 `demo_imr_spi_shift` 指**工作表显示名**，不是 sqlite 主键。 |
| H3 | **没有**独立 `tutorials.json` 主源。现网是 `build_learning_center_db.py` 的 `GENERATORS` + `build_tutorial_row()` **每次重生**步骤文案。金标/0–6/7+ 必须做成**生成器覆盖层**并接入 builder；**禁止**只手改 sqlite 却不改生成脚本（下次 build 会冲掉）。 |
| H4 | `tools/build_learning_dataset_mapping.py` 仍可能默认挂 `smt_paste_height`。Agent2 必须规定：mapping 生成与校验改为「专用主集」；重跑不得把 1:1 冲回共享。 |
| H5 | 测试/gate 现状敏感：`listsTenDatasets`（恰好 10 张）、教程约 184、`used_for`/`not_for`/`scenario`/`click_steps≥2` 等。加专用表后必须**改测试期望**，不能假装仍是 10。DDL 优先 `ADD COLUMN`，保留 `research_sources` 等旧列。 |
| H6 | glossary「基础统计术语」**禁止**为了术语课新增第 185 个假 command id；用 `related_ids` 指向已有命令或课内嵌 glossary。 |
| H7 | `dialog_fill` 保持「角色→列名」object，供现网解析；详解走**并列** `dialog_fill_detail`。禁止用详解数组覆盖 `dialog_fill` 导致 `parse_dialog_fill` 变空。 |
| H8 | 金标对话框字段以 `analysis_commands` 的 `imr` 为准（含 Nelson / 特殊原因等），Canvas 可少写字段但实现不得删真实 inputs。 |
| H9 | 页面已缓存教程 `entries_`（184 条元数据）是允许的；§8「禁止整库 dataset 常驻」指**禁止**把所有单元格 load 进巨大 `QVector`，不要误拆 entries 缓存。 |
| H10 | 旧 [`goal-learning-center-black-belt-plan.md`](goal-learning-center-black-belt-plan.md) 仍写「强制共享」——**本手册 §2 已覆盖**；Agent2 plan 文首必须写「数据集策略以本手册为准」。 |

---

## §1 本 Goal 要交付什么（产品语言）

把学习中心从「能打开、能导入、摘要级说明」升级为：

1. **一算法一（主）专用演示集**：少列、故意埋入本工具该看见的模式；禁止再用一张宽表糊弄几十个菜单。  
2. **黑带课堂文案骨架（0–6）**：关键词 → 背景 → 专用数据 → 为何此工具 → 逐步+参数表 → 读输出 → 误用。  
3. **Agent 教学 skill 叠加（7+）**：先修自测 → 步间自解释 → 褪脚手架层级 → 检索小测 → 错念纠正（可与 §6 误用互补）。  
4. **与现程序衔接**：不换 Table 内核、不换帮助对话框、不新建第二套分析向导；只扩展 learning 层 schema / 生成脚本 / 目录解析 /（可选）页面渲染。

**非目标（本 Goal 不做）**

- 重做 `AlgorithmHelpDialog` / 改 `algorithm_help.json` 公式语义。  
- 在学习中心内嵌分析设置对话框。  
- 用浏览器代替用户本机 `package_dist`。  
- 把旧 black-belt plan 的「强制共享大表」当作仍有效的决策（**已被本文件 §2 覆盖**）。

---

## §2 决策覆盖（相对旧 plan 的变更）

| 项 | 旧 black-belt plan | **本 Goal（以用户 Canvas 确认为准）** |
|----|--------------------|----------------------------------------|
| 数据集策略 | 强制共享（约 10 张表挂几十命令） | **一算法一专用主集**；同构共享仅例外 |
| 教程深度 | 用途/场景/步骤/输出/误用 | **+ 关键词 glossary**；参数字段「填什么/代表什么」；对着埋点读图 |
| 练习闭环 | 无 | **+ 先修 / 自解释 / 褪脚手架 / 检索 / 错念**（JSON 可存，UI 可渐进） |
| 页面 | 已建 `LearningCenterPage` | **扩展渲染**，禁止推倒重做导航/导入链路 |
| 覆盖范围 | 全集 id | **仍要求最终全集**；交付节奏按 Wave（见 Q2） |

---

## §3 必须衔接的现有代码（禁止另起炉灶）

### 3.1 已存在模块（只扩展，不平行复制）

| 路径 | 职责 | 本 Goal 允许的改动 |
|------|------|-------------------|
| `src/application/learning/learning_types.h` | `LearningTutorialEntry` 等 | **扩展字段**（glossary、dialog 详解、练习 JSON…）；保持向后兼容解析 |
| `src/application/learning/learning_tutorial_catalog.*` | 读 tutorials | 解析新列 / JSON；旧库缺列时降级为空 |
| `src/application/learning/learning_dataset_store.*` | sqlite → `domain::Table` | 支持更多 dataset；连接清理逻辑不动坏 |
| `src/application/learning/worksheet_registry.*` | 多工作表 | **禁止回退单表覆盖导入** |
| `src/ui/learning_center_page.*` | 独立窗口 UI | `build_entry_html` 增加分节；可选折叠 |
| `MainWindow::import_learning_dataset` | 新建 worksheet | 签名与「不清输出页」行为保持 |
| `resources/help/learning_center.sqlite` + qrc | 嵌入库 | 由生成脚本重建；`meta.catalog_version` 递增 |
| `tools/build_learning_center_db.py` | 建库 | 主改点之一 |
| `tools/learning_data/*` | csv / mapping / tutorials | 主改点之一 |
| `tools/verify_learning_center_*.py` | 门禁 | 扩展断言（1:1、glossary 非空、埋点注释等） |
| `tests/learning_center_*_test.cpp` | C++ 测试 | 对齐新 schema；连接名不泄漏 |

### 3.2 外部权威源（只读对齐）

- 菜单/角色：`src/ui/analysis_commands.cpp` 的 `roles` / `inputs` / `menu_path` / `menu_label`  
- 公式边界：`resources/help/algorithm_help.json`（不改公式语义）  
- 解释纪律：既有 `interpretation_limits` — 教程禁止「过程合格 / 必须停线 / 已证明正态」  
- 打包：`tools/package_dist.ps1` 已拷 `help/learning_center.sqlite` — 升级后用户再跑一次验收

### 3.3 架构（保持，勿画第二套）

```
帮助菜单
 ├─ 算法、公式与参考资料 → AlgorithmHelpDialog（不动语义）
 └─ 学习中心             → LearningCenterPage
                              ├ 左：搜索 + 与命令表同步的树
                              ├ 右：教程 HTML（0–6 + 可选 7+）
                              └ 导入 → LearningDatasetStore → WorksheetRegistry / MainWindow
```

---

## §4 内容与数据规格（验收口径）

### 4.1 教程正文骨架（每条 implemented 必填）

| 节 | 内容 | 来源 |
|----|------|------|
| 0 关键词 | `glossary[]`：term / plain / remember；控制限课必含 UCL≠USL | Canvas + teach skill |
| 1 背景 | 问题、Y、DMAIC 阶段、本课只回答什么 | 黑带课堂 |
| 2 专用数据 | dataset_id、列角色、**埋入信号**（行号写进 notes） | 用户要求 |
| 3 为何此工具 | 适用边界（如 I-MR vs Xbar-R） | 黑带课堂 |
| 4 逐步+参数 | click_steps + **dialog 详解**（字段/填什么/代表什么） | Canvas |
| 5 读输出 | 对着埋点；指图话术带术语 | Canvas |
| 6 误用 | 禁止句 | 既有纪律 |
| 7+ 练习 | prereq / self_explain / fade / retrieval / misconceptions | edu-agent / fading skills |

**标杆命令**：`imr` + dataset_id **`imr_spi_shift`**（工作表名 `demo_imr_spi_shift`；片 41 阶跃、片 55 尖峰）。金标未过，禁止大规模批量灌水。

### 4.2 专用数据集规则

- 列尽量少（通常 2–6 列）；行 30–200（控制图可到 ~60–100 点）。  
- `datasets.notes` 或生成脚本注释必须写：**埋了什么、在哪一行、期望图上看到什么**。  
- 列名业务化（`锡膏高度_um`），禁止 `x1,x2`。  
- 无需数据的命令：`dataset_id` 空，步骤写清。  
- `formula_reference`：场景+术语+限制说明；步骤标明菜单可能不可用。

### 4.3 Schema 扩展建议（Agent B 定稿，C 实现）

在现有 `tutorials` / `datasets` 上**增量**，优先 JSON 文本列（与现 `dialog_fill`/`output_guide` 风格一致），避免破坏性大迁移：

```text
tutorials 新增（建议）：
  glossary            TEXT  -- JSON [{term,plain,remember}]
  dialog_fill_detail  TEXT  -- JSON [{field,put,meaning}]  （可与 dialog_fill 并存）
  buried_signals      TEXT  -- JSON [{row,what,expect}]
  prereq_quiz         TEXT  -- JSON [{q,good,bad}]
  self_explain        TEXT  -- JSON [{after,prompt}]
  fade_levels         TEXT  -- JSON [{level,student,scaffold}]
  retrieval_quiz      TEXT  -- JSON [string]
  misconceptions      TEXT  -- JSON [{wrong,right}]
  skill_mission       TEXT  -- 一句话本课技能目标

datasets 强化：
  notes 必须含埋点；industry/story 保留
```

`LearningTutorialEntry` 同步扩字段；`build_entry_html` 渲染 0–6；7+ 可先折叠或附录。

`meta.catalog_version`：改为 **`learning-center-v2`**（字符串，不是整数 +1）；Python `META_VERSION` 与 C++ `kExpectedCatalogVersion` **双写同步**。

---

## §5 六 Agent 流水线（一场 Goal 内顺序；模型锁定）

**总编排**：主对话 = Orchestrator。六个角色用 `Task` 子 Agent 或主 Agent 分阶段扮演均可，但 **职责、DoD、禁止项** 必须按下列执行。  
**模型**：全程同一个模型；子 Agent **一律 `model: "inherit"`**；禁止「调研用大模型、执行用小模型」之类切换。

```
用户答完 §0
  → Agent1 调研学习（网 + 代码）
  → Agent2 详细计划（切片 + mapping + schema）
  → 用户可选确认计划中的风险点
  → Agent3 执行（先金标 imr，再 Wave）
  → Agent4 测试（Python gate + C++ tests 清单）
  → Agent5 收尾检查（结构衔接 + 禁止偷懒 + git）
  → Agent6 教学检验（对照 Canvas / 黑带合理性）
  → 未过则回 Agent3（定点修），禁止直接 UpdateGoal complete
```

### Agent 1 — 网络调研与学习（Research）

**输入**：本文件 §4、Canvas、现有 `learning-center-research-notes.md`、`analysis_commands` 相关 id。

**做什么**

1. 网上巩固教学 skill：worked example + fading、self-explanation、CLT/Renkl、主动回忆、错念分析（记 URL + accessed 日期）。  
2. 按菜单包抽查权威统计来源（NIST / Minitab Help / AIAG / Montgomery 等），更新/增补 research notes：**用途 / 不能做什么 / 典型误用 / 建议埋点模式**。  
3. 对照代码：每个待改 id 的真实 `roles`/`inputs`，列出「对话框字段清单」草稿。  
4. 产出：`docs/research/learning-center-pedagogy-upgrade-research.md`（本轮调研总册）+ 必要时更新 notes。

**完成标准**

- [ ] 金标 `imr`：字段清单与 Canvas 参数表对齐，无虚构菜单。  
- [ ] 至少 1 份「专用数据设计模式」表（控制图 / 假设检验 / MSA / 能力 / Pareto…）。  
- [ ] 写明旧「共享大表」策略为何被覆盖（教学理由，非情绪）。  
- [ ] **禁止**改产品代码。

### Agent 2 — 详细计划（Plan）

**输入**：Agent1 产出 + 本文件。

**做什么**

1. 写执行切片：`docs/research/goal-learning-center-pedagogy-upgrade-wave-plan.md`（本文件的可勾选 Wave 表）。  
2. 定 schema 增量 DDL/JSON 形状与兼容策略。  
3. 定 mapping：`command_id → demo_* dataset_id`（例外共享白名单）。  
4. 定文件改动清单（精确到路径）与 verify 脚本新增断言。  
5. 列出 **禁止偷懒**（粘贴 §8）与风险（库体积、文案工作量、UI 范围）。

**完成标准**

- [ ] Wave-0 = 仅 `imr` 金标；后续 Wave 按菜单包切分，每包有 id 列表。  
- [ ] 每个 Wave 有 DoD 勾选表。  
- [ ] 明确「不改哪些文件」。  
- [ ] **禁止**在 Plan 阶段改产品代码（可改 plan md）。  
- [ ] 若 §0 Q2/Q3 与推荐默认不同，以用户答复为准写进 plan 文首。

### Agent 3 — 执行（Implement）

**顺序强制**

1. Schema + `build_learning_center_db.py` + types/catalog 解析（兼容旧字段）。  
2. 在 **生成脚本** 中落地金标：`imr_spi_shift` 生成器 + 教程覆盖层（完整 0–6 + 7+ JSON），再 build sqlite。  
3. Wave-0：`LearningCenterPage` **最小渲染**（glossary、dialog 详解、埋点、误用）；折叠/练习交互仅当用户在 Q3 要求「UI 增强切片」时做。  
4. 同步改 `listsTenDatasets` 等测试期望；扩展 verify 断言。  
5. 金标门禁 PASS 后，按内容 Wave 批量：专用生成器 + mapping，禁止只改 sqlite。  
6. 更新 mapping md / research；`META_VERSION` + `kExpectedCatalogVersion` → `learning-center-v2`。

**完成标准（每 Wave）**

- [ ] 该 Wave 全部 id：glossary 非空（至少 3 个术语或写明「沿用基础统计术语课」并 link）。  
- [ ] implemented：专用 dataset 存在且 `dialog_fill`/`detail` 对照真实 inputs。  
- [ ] 埋点 notes 可检索。  
- [ ] 不破坏导入新建工作表、不泄漏 QSql 连接。  
- [ ] **禁止**只改十几条好看的就声称全集完成。  
- [ ] **禁止**重新引入「十表打天下」作为默认 mapping。

### Agent 4 — 测试（Test）

**做什么**

- 跑并必要时扩展：  
  - `tools/verify_learning_center_gate.py`  
  - `tools/verify_learning_center_db.py`  
  - `tools/verify_learning_dataset_mapping.py`  
  - `tools/verify_learning_research_notes.py`  
- 断言建议：  
  - tutorial.command_id ⊆ commands ∪ help  
  - dataset 外键无悬空  
  - **主路径**：implemented 且需要数据 → `dataset_id` 不以共享宽表为主（对 `smt_paste_height` 等旧 id：若仍被大量引用则 FAIL 或 WARN 按 plan）  
  - glossary / buried_signals 对金标强制；对其余 Wave 按 plan 阈值  
- C++：`learning_center_store_test` / `worksheet_registry_test` / analysis sample — **列出用户需在 Qt 编译的 target**，Agent 因中文路径不强跑 cmake。

**完成标准**

- [ ] Python gate 本机可 PASS（Agent 跑）。  
- [ ] 给出「请用户编译的 test target 列表」。  
- [ ] 金标 imr：有一份「导入后预期」检查单（片 55 / UCL 话术）。  
- [ ] **禁止**用「我看了一眼 HTML」代替 gate。

### Agent 5 — 收尾检查（Closeout）

**做什么**

- 对照 §8 禁止偷懒全文勾选。  
- 结构衔接：diff 是否误改 `analysis_service` 大文件、是否动公式 JSON 语义、导入路径是否仍走 registry。  
- 资源：sqlite 体积、qrc、package_dist 说明。  
- Git：commit（用户规则）+ push；说明用户自行 `package_dist`。  
- 启动独立 **code-review / bugbot** 子 Agent 扫本分支学习中心相关 diff（用户规则）。

**完成标准**

- [ ] 检查清单全勾或缺口显式写入 commit/PR。  
- [ ] 未代用户宣称 package_dist 已验。  
- [ ] **禁止**在收尾阶段塞新功能。

### Agent 6 — 教学检验（Pedagogy QA）

**角色定位**：不写功能代码；像黑带教练 + 教学设计师抽检。

**做什么**

1. 对照 Canvas：金标课是否仍含专用数据、关键词、参数表、读图、误用，**且** skill 叠加未冲掉正文。  
2. 抽检每 Wave ≥2 条：术语是否首次释义、埋点与读图是否对得上、有无「控制限=规格」话术。  
3. 抽检 1 条 `formula_reference`：是否诚实标注未实现。  
4. 输出：`docs/research/learning-center-pedagogy-qa-report.md`（通过/驳回 + 必改列表）。

**完成标准**

- [ ] 金标 **必须通过** 才能 Goal complete。  
- [ ] 若驳回：只开「定点返工单」给 Agent3，禁止顺手重构无关模块。  
- [ ] **禁止**为赶进度把错念文案删成空白。

---

## §6 多 Agent 协作纪律（与母框架对齐）

| 规则 | 要求 |
|------|------|
| 单一模型 | 用户指定模型后，Orchestrator 与所有 Task **inherit**，中途不换 |
| 单一会话 | 同一 Goal 对话内跑完六岗；不要每 Agent 新开无上下文会话（除非用户要求） |
| 文件分片 | 并行改代码时禁止两人同时改同一 cpp；生成数据可用并行 Python |
| 中文路径 | Agent 不强跑易损坏的 cmake/package；提示用户自测 |
| 决策锁定 | §0 答完后，禁止子 Agent 擅自改回「共享大表默认」 |
| 输出格式 | 每岗结束：`变更文件列表` + `DoD 勾选` + `风险一行` + `是否可进下一岗` |

### 子 Agent 提示词头（粘贴）

```text
你是 DataLab「学习中心教学升级」Goal 的 Agent{N}:{角色}。
权威手册：docs/research/goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md
模型：与主对话相同（inherit），禁止建议换模型。
必须衔接现有 learning_* / LearningCenterPage / WorksheetRegistry / build_learning_center_db.py。
禁止缩小 Goal；禁止重建第二套帮助系统；禁止改 algorithm_help 公式语义。
禁止偷懒：见该手册 §8。
交付：文件列表 + DoD 勾选 + 风险一行 + go/no-go。
```

---

## §7 Wave 建议切分（Agent2 可微调，不可删金标）

| Wave | 内容 | 出口 |
|------|------|------|
| Wave-0 | Schema + catalog + **页面最小渲染** + **imr / imr_spi_shift 金标** | Agent4+6 过金标 |
| Wave-1 | 控制图包（`imr` 族、Xbar、P/U…）专用集 | gate |
| Wave-2 | 质量工具 / MSA / 能力 | gate |
| Wave-3 | 统计推断 / ANOVA / 回归相关 | gate |
| Wave-4 | 图形 + DOE + 可靠性 + 其余 | gate |
| Wave-5 | 旧共享表策略落地 + 文档 + package 说明 | Agent5+6 |
| （可选）UI 增强切片 | 折叠披露 / 练习勾选 — **名称勿与 Wave-2 混淆** | 仅当 Q3 要求 |

（具体 id 列表由 Agent2 从 `analysis_commands::all()` ∪ help 生成锁表。）

---

## §8 禁止偷懒（执行 Agent 必读；Plan 必须粘贴）

1. **禁止**推倒重做学习中心窗口/导入链路「为了干净」。  
2. **禁止**恢复默认「10 张共享表挂几十命令」而不写例外白名单。  
3. **禁止**金标未过就批量生成上百条水文案。  
4. **禁止**教程步骤不写真实 `menu_path` / 角色名。  
5. **禁止** glossary 为空却满篇 UCL/Cpk/%GR&R。  
6. **禁止**专用集无埋点注释（「随机正态」充数）。  
7. **禁止** dialog 只写列名不写「字段含义/为何留空」。  
8. **禁止**输出解读不对照埋点（通用套话）。  
9. **禁止**写成「过程合格 / 必须停线 / 已证明正态 / 点出 UCL=废品」。  
10. **禁止**改 `algorithm_help.json` 公式语义或删 interpretation 边界。  
11. **禁止**导入覆盖当前表、跳过 `WorksheetRegistry`。  
12. **禁止** QSql 默认连接残留。  
13. **禁止**整库载入常驻 `QVector`。  
14. **禁止**学习中心再做分析向导。  
15. **禁止**跳过 id 对齐 / mapping verify。  
16. **禁止**中途换模型或让子 Agent 用另一模型「省钱」。  
17. **禁止** Agent6 没出 QA 报告就 UpdateGoal complete。  
18. **禁止**用浏览器代替用户本机 `package_dist`。  
19. **禁止**把 Canvas 里的 skill 叠加块删掉只留摘要。  
20. **禁止** UI 把 0–6 与 7+ 糊成一堵墙且无分节（若本 Goal 含 UI）。  
21. **禁止**只改 sqlite / 不改 `build_learning_center_db.py` 生成器（下次 build 会冲掉）。  
22. **禁止** `dataset_id` 以 `demo_` 开头导致工作表变成 `demo_demo_…`。  
23. **禁止**只改 `META_VERSION` 或只改 `kExpectedCatalogVersion`（必须双写）。  
24. **禁止**用详解 JSON 覆盖原 `dialog_fill` 对象格式。  
25. **禁止**为术语课新增虚假 command_id 撑到 185+。

---

## §9 完成定义（整场 Goal）

- [ ] §0 用户答复已记入 plan 文首。  
- [ ] Wave-0 金标 `imr` + `imr_spi_shift`：生成脚本可复现；文案 0–6 + 7+；**页面最小渲染**能读到关键词与参数释义（与 Q3 默认一致）。  
- [ ] 约定范围内全部 Wave 的 mapping / tutorials / datasets 按「专用主集」策略落地。  
- [ ] Python learning_center gate PASS。  
- [ ] Agent5 收尾 + git push。  
- [ ] Agent6 QA 报告：**金标通过**；抽检问题已关闭或登记为已知限制。  
- [ ] 提示用户本机：`cmake --build build-mingw --target package_dist`（或其惯用包）并抽查学习中心导入。

---

## §10 新对话开场粘贴（Mega Prompt）

把下面整段贴进**新 Goal 对话**首条（并补上模型名与 §0 答案）：

```text
按 docs/research/goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md 执行学习中心教学升级 Goal。

【模型锁定】全程只使用：________（填写）。所有 Task 子 Agent 必须 model: "inherit"。中途禁止换模型。

【§0 答复】
Q2 范围节奏：________
Q3 UI 范围：________
Q4 1:1 严格度：________
Q5 旧共享表：________
Q6 编译/package 仍由我本机执行：是

【硬约束】
- 这是升级，不是重建：衔接 learning_types / catalog / dataset_store / LearningCenterPage / WorksheetRegistry / build_learning_center_db.py。
- 数据集默认「一算法一专用主集」；覆盖旧 black-belt plan 的强制共享策略。
- 文案以 Canvas learning-center-tutorial-example 为金标：保留 0–6，叠加 7+ skill，禁止删掉上一版深度内容只留骨架。
- 六 Agent 顺序：调研 → 计划 → 执行 → 测试 → 收尾检查 → 教学检验；每岗 DoD 未过不得进入下一岗。
- 先 Wave-0（imr + dataset_id=imr_spi_shift → 工作表 demo_imr_spi_shift）金标，再分 Wave；金标未过禁止大批量灌文案。
- 遵守该手册 §0.1 硬约束与 §8 禁止偷懒全文。
- catalog_version 双写 learning-center-v2；内容必须进 build_learning_center_db.py 生成器，禁止只手改 sqlite。
- 中文路径：不要强跑易失败的 cmake/package；完成后 commit+push，并提示我本机 package_dist 自测。
- 每阶段结束输出：文件列表 + DoD + 风险 + go/no-go。
```

---

## §11 参考技能与文献线索（Agent1 须核实 URL 与日期）

| 主题 | 用途 |
|------|------|
| 本地 `~/.agents/skills/teach/SKILL.md` | 一课一事、术语表、ZPD、检索 |
| edu-agent-skills（主动回忆 / 错念 / 先修） | 练习闭环字段设计 |
| Worked example + backward fading（Renkl / Atkinson / CLT） | 完整例 → 完成题 → 独立练 |
| agentskills.io progressive disclosure | 右侧面板勿一屏塞满 |
| DataLab `goal-execution-framework.md` | Wave、verify、多 Agent 边界 |

---

## §12 修订记录

| 日期 | 说明 |
|------|------|
| 2026-09-03 | 初稿：六 Agent、专用集覆盖共享策略、衔接现网学习中心、§0 待问项、mega prompt |
| 2026-09-03 | 子 Agent 审查后补 §0.1 硬约束：catalog 双写、dataset_id 命名、生成器管线、测试 10 表、dialog_fill 并列等 |
