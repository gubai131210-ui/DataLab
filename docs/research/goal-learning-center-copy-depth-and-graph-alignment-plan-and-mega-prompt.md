# Goal：学习中心文案加深 + 练习闭环可读性 + 图形名实对齐

> **用途**：文案加深 + 练习闭环 + 图形对齐 Goal 的权威手册；**该 Goal 内容层已收口**。  
> **后续 Goal（口吻升温 · 并肩版）**：见 [`goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md`](goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md)。禁止用本文件当下一场「口吻」对话的唯一决策源。  
> **状态**：§0 已由用户拍板（2026-09-03 15:05 UTC+8），**决策锁定，禁止子 Agent 重问或改口**。  
> **前一轮已收口基线（必须保留，禁止推倒）**：[`goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md`](goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md)  
> **母框架**：[`goal-execution-framework.md`](goal-execution-framework.md)  
> **金标 Canvas（0–6 深度标杆，仍有效）**：`~/.cursor/projects/d-QT-CppPrograms-DataLab/canvases/learning-center-tutorial-example.canvas.tsx`  
> **调研/锁表/QA（只读参考）**：[`learning-center-pedagogy-upgrade-research.md`](learning-center-pedagogy-upgrade-research.md)、[`goal-learning-center-pedagogy-upgrade-wave-plan.md`](goal-learning-center-pedagogy-upgrade-wave-plan.md)、[`learning-center-pedagogy-qa-report.md`](learning-center-pedagogy-qa-report.md)

---

## §0 用户已拍板决策（2026-09-03）— 锁定

| # | 问题 | **用户决定（锁定）** |
|---|------|----------------------|
| Q1 | 全程模型 | **使用用户当前对话所用模型**；全程不换。所有 Task 子 Agent **`model: "inherit"`**。禁止建议换模型「省钱/加速」 |
| Q2 | 范围 | **全集 184 课**都加深练习闭环与术语详解。内部 Wave 只是施工队列，不得停在金标或「下一 Goal 再铺」 |
| Q3 | 对前一轮的态度 | **补充，不是推倒**。保留专用主集、同构白名单、0–6 分节、折叠 UI、导入链路、`learning-center-v2`、金标 `imr`/`imr_spi_shift` |
| Q4 | 读者 | **非开发人员、统计基础薄弱**的车间/质量学员。禁止开发黑话、禁止「列数？/共享族？/dataset？」这类没头没脑的题 |
| Q5 | catalog | **保持 `learning-center-v2`**。不改 META_VERSION / `kExpectedCatalogVersion`。JSON 形状可在现有列内增量（见 §4.3） |
| Q6 | 练习揭晓 UI | **内容必须写完整题干 + 白话参考答**；揭晓只显示**本课 JSON 里的参考**，禁止再套全课通用 UCL 话术 |
| Q7 | 图形 | Agent1/3 **必须审计「图名 ↔ 菜单中文名 ↔ 实际出图 ↔ 演示列」**；对不上的定点修，禁止假装没看见 |
| Q8 | 编译/package | Agent 跑 Python verify；用户本机 Qt Creator / `package_dist` 自测。中文路径不强跑易失败的 cmake/package，除非用户本轮明确要求打包 |

**编排者纪律**：§0 已关闭。子 Agent **禁止**再问范围/是否推倒/是否升 v3；若与本表冲突，以本表为准。

### §0.1 现网硬约束（写进 Wave plan 文首）

| ID | 约束 |
|----|------|
| H1 | `catalog_version` **保持** `learning-center-v2`。禁止只改 sqlite、禁止偷偷升 v3 |
| H2 | `dataset_id` **不以 `demo_` 开头**。工作表仍是 `demo_{dataset_id}` |
| H3 | 文案必须进 `build_learning_center_db.py` / `tutorial_overlays` / `wave*_content.py`；**禁止**只手改 sqlite |
| H4 | 同构共享仍以 wave-plan §3 白名单为准；禁止回归旧 10 表 |
| H5 | `dialog_fill` 保持角色→列名 object；详解走 `dialog_fill_detail` |
| H6 | 禁止为术语课新增虚假 command_id |
| H7 | 禁止推倒 `LearningCenterPage` 导航树 / 导入 / `WorksheetRegistry` |
| H8 | 禁止改 `algorithm_help.json` 公式语义 |
| H9 | 金标 `imr` 的 0–6 深度（片 41/55、UCL≠USL、9 字段参数表）**不得删成摘要**；7+ 按本 Goal 重写可读性 |
| H10 | 练习 JSON 可**增量字段**（`why`/`hint`/`model_answer`），解析器缺字段时降级为空，不得破坏旧库可读 |
| H11 | 图形课 `title`/`menu_path`/`click_steps` 必须等于 `analysis_commands` 的中文菜单名；`related_ids` 必须是**真相关工具**，禁止模板一律挂 `histogram`+`scatter_plot`+`graph_gallery` |

---

## §1 本 Goal 要交付什么（产品语言）

前一轮已经做成：**能导入、有专用表、有 0–6 分节、有 7+ 槽位**。本轮要做成：**车间新人能独立读懂、独立做练习、揭晓后知道「为什么这样」**。

1. **读者换挡**：所有面向学员的句子（glossary / 步骤 / 先修 / 自解释 / 检索 / 错念 / 揭晓）用完整中文，术语首次出现必须白话释义。禁止对内口径（`command_id`、`role_map`、`同构白名单`、`WAVE`）出现在学员可见区。  
2. **练习闭环补全**：每课 7+ 从「电报式填空」升级为「有情境的完整问题 + 可理解的对/错选项 + 揭晓参考答（含为什么）」。  
3. **0–6 只补过短句，不换骨架**：金标与已写清楚的背景/埋点/参数表保留；把「连续 Y。」「可空=计频。」这类半句话写成学员能跟做的说明。  
4. **图形名实对齐**：菜单叫什么、教程标题叫什么、导入后对话框填什么、图上画的是什么，四者一致；审计图标借用造成的「看起来像另一张图」。  
5. **衔接现网**：只扩展 overlay、生成器、catalog 解析、练习区揭晓文案；不新建第二套帮助系统。

**非目标（本 Goal 不做）**

- 重建学习中心窗口、改导入覆盖策略、改 Table 内核。  
- 改 `algorithm_help.json` 公式。  
- 把专用集再合成 10 张大宽表。  
- 把 184 课缩成「只改图形」或「只改 imr」。  
- 用浏览器代替用户本机 `package_dist`（除非用户本轮再要求打包）。

---

## §2 已证实的问题（Agent 不许装没看见）

抽检时间 2026-09-03。这些是**现网真实缺陷**，本 Goal 要修，不是再调研「要不要修」。

### 2.1 练习题没头没脑（学员可见）

`histogram.json` 先修示例：

- 「与概率图共享表？」正确项写成 `graph_hist_prob`（内部 id，学员看不到也不该背）  
- 「UCL=柱高？」把控制图术语硬塞进直方图课  
- 检索题：`共享族？` / `行48？` / `禁止句` — 不像问题

`two_sample_t.json` 先修示例：

- 「列数？」好=「2」坏=「1」  
- 「共享？」好=`infer_two_sample_location`  
- 检索：`两列？` / `同构？` / `禁止句`

**合格改写方向（同一课，保留教学意图）**

- 先修：「画直方图之前，数据应该是什么样？」好：「一个连续测量值（如厚度），按区间数一数有多少点」坏：「把控制图的 UCL 当成柱子高度」  
- 检索：「用自己的话说明：直方图右侧稍微拉长，能不能写成『已经证明数据服从正态分布』？为什么？」揭晓必须是完整句子，不是「禁止句」三个字。

### 2.2 揭晓参考怪

现网 `LearningCenterPage`：

- 先修：点对/错后，反馈几乎只是把 `good`/`bad` 原文再念一遍（选项若是 `2` / `graph_hist_prob`，参考就更怪）。  
- 自解释：默认揭晓是「对照本题提示『…』」，等于把问题读一遍，没有参考答。Nelson 特例仍写 I-MR 话术，非 SPC 课不能走这条。  
- 检索：揭晓仍带「控制限课请再核对 UCL≠USL」，直方图/ANOVA 课会误导。

**本 Goal UI 最小改动（Q6）**：揭晓读取 JSON 新增的 `hint` / `why` / `model_answer`；缺省时写中性句「请回到上面第 0 节关键词和第 5 节读图」，**禁止**无条件提 UCL。

### 2.3 图形和名称对不上 — 已知成因（Agent1 要逐条核实，Agent3 定点修）

| 现象 | 现网原因 | 修法 |
|------|----------|------|
| 图形课 `related_ids` 几乎都是 histogram + scatter_plot + graph_gallery | Wave-4 `_data_overlay` 模板写死 | 改为真相关：条形图→Pareto/饼图；区域图→时间序列图/折线；箱线→多变异/能力… |
| `bar_chart` 在 `analysis_commands` 的**图标键**是 `"pareto"` | 命令表第四个 QString 常是图标/别名，不是菜单名 | 教程**标题必须用「条形图」**；步骤里写清「不要和 Pareto 图当成同一个菜单」；**不要**把教程改名为 Pareto |
| `eda_4plot` 图标键是 `"histogram"` | 同上 | 标题保持「EDA 四图」；说明四宫格含游程/直方图/概率图/滞后，不是「再画一张直方图」 |
| `area_plot` 菜单中文是「区域图」，`dialog_fill` 只填了 `time`，`value` 写在 detail | 导入预填可能只带一列 | `dialog_fill` 应含真实必填角色：`time`+`value`（以 commands 为准） |
| 学员把 `demo_graph_*` / `dataset_id` 当成图的名字 | 文案把内部 id 写进 scenario | 学员可见区只写菜单中文 + 工作表显示名 |
| 直方图与概率图共享表是对的，但文案用「同构」讲解 | 给开发看的词 | 改成「同一张厚度表可以画直方图，也可以画概率图；本课只教直方图」 |

Agent1 必须再出一张**全图形 command 对照表**（id / 菜单中文 / 图标键 / overlay 标题 / dataset / 填列 / 实际 `chart_type`），标红不一致项。禁止只抽 2 条就声称对齐。

### 2.4 0–6 过短句（补充，不删节）

保留分节。把 detail 的 `meaning` 从「连续 Y。」改成「填哪一列测量值、这一列在图上是什么」。`used_for` 不要只写「用专用集练习某某图」。金标 `imr.json` 的 0–6 **以保留为主**，只改 7+ 可读性，除非发现与代码字段不一致。

---

## §3 必须衔接的现有代码（禁止另起炉灶）

| 路径 | 本 Goal 允许 |
|------|----------------|
| `src/application/learning/learning_types.h` | 给 `LearningSelfExplain` 增加可选 `hint`；检索改为可解析 string **或** `{q,hint}` 的结构（向后兼容） |
| `learning_tutorial_catalog.cpp` | 解析新 JSON 键；旧 overlay 缺键不崩 |
| `learning_dataset_store.*` / `worksheet_registry.*` | **原则上不改**；除非审计发现导入列映射错误 |
| `src/ui/learning_center_page.*` | **只改练习揭晓文案来源**（用本课 hint）；禁止重做左树/导入按钮 |
| `MainWindow::import_learning_dataset` | 不改语义 |
| `tools/build_learning_center_db.py` + `tools/learning_data/wave*_content.py` + `tutorial_overlays/*.json` | **主战场**：重写学员可见文案 |
| `tools/verify_learning_center_*.py` | 加「练习题可读性」断言（见 §6） |
| `tests/learning_center_*_test.cpp` | 仅当解析器形状变化时对齐；金标断言保留 |
| `analysis_commands.cpp` / `algorithm_help.json` | **只读对齐**；禁止为了图名去改公式 |

架构保持：

```
帮助菜单 → 学习中心 LearningCenterPage
            左：搜索 + 命令树
            右：0–6 分节（保留）+ 7+ 练习（文案+揭晓加深）
            导入 → LearningDatasetStore → WorksheetRegistry
```

---

## §4 内容规格（验收口径）

### 4.1 读者与用词

| 禁止出现在学员可见区 | 应写成 |
|----------------------|--------|
| `command_id` / `dataset_id` / `role` / `overlay` / `WAVE` / `同构` / `白名单` | 菜单中文名；「演示工作表」；「这一列在对话框里叫『变量』」 |
| `graph_hist_prob` | 「演示工作表：厚度直方图练习」（显示名 `demo_graph_hist_prob` 可在步骤里出现一次，并解释这是导入后左上角表名） |
| 「列数？」「共享族？」「禁止句」 | 完整问句，主语宾语都在 |
| 「连续 Y。」「可空=计频。」 | 至少一两句完整说明 |
| 开发者禁止项当先修题 | 先修问概念；禁止句放 §6 误用和错念 |

术语规则：

- 统计名词（直方图、中位数、p 值、控制限）**要用**，不要用更怪的缩写代替。  
- 每个术语在该课 glossary **第一次**用「术语 + 白话 + 怎么记」。  
- 控制限课必须保留 UCL ≠ USL；**非控制限课禁止为凑数考 UCL**。

### 4.2 7+ 每课最低质量（184 全覆盖）

| 块 | 最低要求 |
|----|----------|
| 先修 `prereq_quiz` | ≥3 题。`q` 是完整问句（≥12 个汉字）。`good`/`bad` 是完整选项（≥8 个汉字），字数尽量接近以免猜长短。增加可选 `why`：选完后显示的一句话原因 |
| 自解释 `self_explain` | ≥2 条。`after` 用学员动作（「导入演示表之后」）不是「看差」。`prompt` 完整。必填 `hint`：3–6 句参考答，解释为什么，不要复读题目 |
| 褪脚手架 `fade_levels` | 仍 0/1/2。学员任务写成「你要做什么」，不要「跟着 §4」。§ 编号可留在脚手架给自己看，但学员句要能独立执行 |
| 检索 `retrieval_quiz` | ≥3 题，完整问句。每题有 `hint`（参考答要点）。允许 JSON 从纯字符串升级为对象，解析器双形态 |
| 错念 `misconceptions` | ≥2 条。`wrong`/`right` 都是完整句子；`right` 要讲对在哪，不能只写「禁止已证明正态。」 |

**金标 `imr`**：0–6 保留 Canvas 深度；7+ 按上表重写（可保留教学意图：UCL≠USL、片 55、阶段列留空）。

### 4.3 JSON 形状（不升 catalog 版本）

在现有 TEXT 列内扩展，**不新增 sqlite 列也可以**（推荐，符合 Q5）：

```text
prereq_quiz:     [{q, good, bad, why?}]
self_explain:    [{after, prompt, hint}]          -- hint 本 Goal 必填
fade_levels:     [{level, student, scaffold}]     -- 文案加长即可
retrieval_quiz:  ["旧字符串"] 或 [{q, hint}]     -- 解析器两者都收；新内容必须用对象
misconceptions:  [{wrong, right}]
```

C++：`LearningSelfExplain::hint`；检索用小结构或 `QVector` 新类型 + 兼容 parse。旧字符串检索：`hint` 空，UI 用中性句。

### 4.4 图形对齐清单（Agent2 锁进 plan）

对每个「图形」菜单 id：

1. `analysis_commands`：`id`、中文 `menu_label`、图标键、`roles`、`inputs`、`chart_type`  
2. overlay：`title` 必须 = 中文菜单名（允许副标题用破折号补充，主名必须可检索）  
3. `click_steps` 菜单路径 = 真实 `menu_path`  
4. `dialog_fill` 键 ⊆ 真实 role id，且必填角色都有列名（可空角色写进 detail 并解释为何留空）  
5. 演示列名与生成器 CSV 表头一致  
6. `related_ids` 每条能一句话说清为何相关；禁止模板三件套  
7. 若图标键 ≠ 本图种类：在「为何此工具」用一句人话说明「软件里图标可能借用相近图，菜单名称仍以左侧树为准」

---

## §5 六 Agent 流水线（一场 Goal；模型锁定 inherit）

```
Agent1 调研学习（网 + 现网对照；禁止改产品代码）
  → Agent2 详细计划（Wave 锁表 + 文案规范 + 图名红表；禁止改产品代码）
  → Agent3 执行（先金标 imr 的 7+ 与揭晓 UI，再按菜单包铺 184）
  → Agent4 测试（verify 新断言 + 列出 C++ target）
  → Agent5 收尾（§8 勾选 + commit/push + 提示 package）
  → Agent6 教学检验（非开发读者抽检；金标 7+ 必须像课而不是像 API）
  → 驳回则定点返工，禁止顺手重构无关模块
```

### Agent 1 — 调研（Research）

**做什么**

1. 巩固「给新手写练习题」：完整题干、选项等长、参考答含原理（teach skill + 前一轮 pedagogy research，核实 URL）。  
2. **图形名实全表**：`analysis_commands` 图形包 ∪ 教程 overlay ∪ CSV 表头 ∪ 图标键。  
3. 抽查每菜单包 ≥3 课的 7+，记录「电报题 / 内部 id / 揭晓复读题目」清单。  
4. 产出：`docs/research/learning-center-copy-depth-and-graph-alignment-research.md`

**DoD**

- [ ] 图形对照表覆盖全部图形 command_id（不是抽样冒充全集）  
- [ ] 练习题反面教材 ≥10 条（真实文件路径+字段）  
- [ ] 写清「补充而非推倒」的边界  
- [ ] **禁止**改产品代码  

### Agent 2 — 计划（Plan）

产出：`docs/research/goal-learning-center-copy-depth-wave-plan.md`

必须含：Wave 锁表（可复用前一轮 184 id 分组）、每课文案检查单、JSON 解析兼容策略、verify 新断言、§8 禁止偷懒全文、文件改动清单、「明确不改哪些文件」。

**DoD**

- [ ] Wave-0 = 揭晓 UI + `imr` 7+ 金标加深（0–6 不砍）  
- [ ] Wave-1…n 铺完全集 184，不得「等等」  
- [ ] 图形红表进入施工队列  
- [ ] **禁止** Plan 阶段改产品代码  

### Agent 3 — 执行（Implement）

顺序强制：

1. catalog 解析兼容 + `LearningCenterPage` 揭晓改读 `hint`/`why`  
2. 重写 `imr` 的 7+（金标闸门）  
3. 改掉 wave 生成器里的电报模板（`_seven_plus` / Wave-4 模板 `related_ids`）——从源头修，避免再灌水  
4. 按 Wave 重生成 overlays + builder 重建 sqlite  
5. 图形 dialog_fill 补全必填角色；related_ids 去模板化  

**DoD（每 Wave）**

- [ ] 该 Wave 每课先修/检索不再出现纯内部 id 当「正确答案」  
- [ ] 每课自解释均有 `hint`  
- [ ] 图形课 title=菜单中文  
- [ ] 金标 0–6 未被删成摘要  
- [ ] 禁止只改 sqlite  

### Agent 4 — 测试（Test）

跑：`LEARNING_CENTER_WAVE=5`（或本轮约定的最终档）`tools/verify_learning_center_*.py`

新增断言建议：

- 先修 `q` 长度下限；`good`/`bad` 不得仅为 `是/否/2/1` 除非 why 写满（优先直接禁止过短 good）  
- `good` 不得匹配 `^[a-z][a-z0-9_]+$`（拦住 dataset_id 当答案）  
- 检索若为对象则 `hint` 非空；图形 overlay `title` ∈ 命令中文名  
- 金标 imr 仍强制 glossary UCL/USL、buried 41/55  

列出用户需编的 C++ target：`learning_center_store_test` / `learning_center_worksheet_registry_test` / `learning_center_analysis_sample_test`

### Agent 5 — 收尾

对照 §8；diff 不得误改 `analysis_service` 大文件；commit+push（用户规则）；提示本机 `package_dist`；可开 code-review 子 Agent 扫学习中心 diff。**禁止**收尾塞新功能。

### Agent 6 — 教学检验

角色：黑带教练 + **给新人上课的老师**（不是代码 reviewer）。

1. 金标 imr：0–6 仍在；7+ 读起来像课堂提问，揭晓像助教答案。  
2. 每 Wave ≥2 课：先修是否没头没脑、揭晓是否复读题目、有无内部 id。  
3. 图形 ≥3 课：菜单名 vs 标题 vs 填列 vs「会不会看成另一张图」。  
4. 产出：`docs/research/learning-center-copy-depth-qa-report.md`  
5. 金标 7+ **必须通过**才能 Goal complete。驳回只开定点返工单。

---

## §6 施工队列（内部 Wave；必须全部做完）

| Wave | 内容 | 出口 |
|------|------|------|
| Wave-0 | 解析器兼容 + 练习揭晓 UI + **imr 7+ 金标加深**（0–6 保留） | Agent4+6 过金标练习可读性 |
| Wave-1 | 控制图包 7+ 与过短 0–6 补句 | gate |
| Wave-2 | 质量 / MSA / 能力 | gate |
| Wave-3 | 推断 / ANOVA / 回归 | gate |
| Wave-4 | **图形名实对齐** + DOE / 可靠性 / 其余 | gate |
| Wave-5 | 模板源头清干净、verify 新断言、文档 | Agent5+6 |

id 列表沿用 [`goal-learning-center-pedagogy-upgrade-wave-plan.md`](goal-learning-center-pedagogy-upgrade-wave-plan.md) §2，Agent2 不得漏 id。

---

## §7 禁止偷懒（Plan 必须粘贴）

1. **禁止**推倒学习中心窗口/导入链路。  
2. **禁止**丢掉前一轮 0–6 骨架、专用表、白名单、v2 catalog。  
3. **禁止**金标 7+ 未达标就批量灌水；也**禁止**金标过后借口下轮再铺 184。  
4. **禁止**学员可见区出现 `dataset_id` / `同构` / `WAVE` 当教学内容。  
5. **禁止**先修/检索用「列数？」「共享族？」「禁止句」当题干。  
6. **禁止**揭晓只把题目或选项复读一遍。  
7. **禁止**非控制图课无条件考 UCL。  
8. **禁止**图形 `related_ids` 再写死 histogram+scatter+gallery。  
9. **禁止**为对齐图名去改 `algorithm_help` 公式或乱改菜单 id。  
10. **禁止**把条形图教程改名为 Pareto（图标借用 ≠ 菜单名）。  
11. **禁止**只手改 sqlite。  
12. **禁止**中途换模型。  
13. **禁止** Agent6 没出 QA 报告就 UpdateGoal complete。  
14. **禁止**用浏览器代替用户本机 package（除非用户本轮再要求）。  
15. **禁止**把练习 JSON 删空赶进度。  
16. **禁止**新增虚假 command_id。  
17. **禁止** `dialog_fill` 覆盖成数组。  
18. **禁止** `dataset_id` 以 `demo_` 开头。  
19. **禁止**升 catalog 到 v3（本 Goal 已锁定保持 v2）。  
20. **禁止**并行两人改同一 cpp。  

---

## §8 完成定义

- [ ] §0 已锁定（本文）。  
- [ ] 184 课 7+ 达到 §4.2；过短 0–6 已补句，金标 0–6 未砍。  
- [ ] 图形名实对照表无红项（或红项已修并在 QA 注明）。  
- [ ] 练习揭晓使用本课 `hint`/`why`，无全课 UCL 套话。  
- [ ] Python gate PASS（含新可读性断言）。  
- [ ] Agent6：金标 7+ 通过；抽检无「没头没脑题」。  
- [ ] git commit + push；提示用户本机 package_dist 抽查学习中心练习区。

---

## §9 每阶段输出格式

变更文件列表 + DoD 勾选 + 风险一行 + go/no-go

### 子 Agent 提示词头

```text
你是 DataLab「学习中心文案加深 + 图形名实对齐」Goal 的 Agent{N}:{角色}。
权威手册：docs/research/goal-learning-center-copy-depth-and-graph-alignment-plan-and-mega-prompt.md
前一轮基线必须保留：专用表、v2、0–6 分节、导入链路。本轮是补充。
模型：inherit，禁止建议换模型。
读者：非开发、基础薄弱。禁止电报题和内部 id 当答案。
禁止偷懒：见该手册 §7。
交付：文件列表 + DoD 勾选 + 风险一行 + go/no-go。
```

---

## §10 新对话开场粘贴（Mega Prompt）

> 把下面整段贴进**新 Goal 对话**首条即可（§0 已填，勿再问）。建议同时挂上 `/goal` skill。

```text
/goal

【Goal 启动】学习中心文案加深 + 练习闭环可读性 + 图形名实对齐（补充，不推倒）

权威手册（唯一决策源，先通读再动手）：
docs/research/goal-learning-center-copy-depth-and-graph-alignment-plan-and-mega-prompt.md

请立即用 CreateGoal 建立长跑目标，objective 写清本 Goal 名称与手册路径。
母框架可参考 docs/research/goal-execution-framework.md。
前一轮已收口基线（必须保留）：docs/research/goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md
金标 Canvas（0–6 仍有效）：~/.cursor/projects/d-QT-CppPrograms-DataLab/canvases/learning-center-tutorial-example.canvas.tsx
前一轮锁表可复用：docs/research/goal-learning-center-pedagogy-upgrade-wave-plan.md

【模型锁定 — 最重要】
- 全程只使用我当前对话所用模型。
- 所有 Task 子 Agent 必须 model: "inherit"。
- 中途禁止换模型，禁止建议换模型「省钱/加速」。

【§0 已拍板 — 禁止重问、禁止改口】
Q1 模型：inherit 当前对话模型，不换
Q2 范围：全集 184 课加深 7+ 与过短句；内部 Wave 只是施工队列，不得停在金标或推到下一 Goal
Q3 态度：补充不推倒——保留专用主集、同构白名单、0–6 分节折叠、导入链路、learning-center-v2、金标 imr / imr_spi_shift 的 0–6 深度
Q4 读者：非开发人员、统计基础薄弱。禁止「列数？/共享族？/dataset_id 当正确答案」；要用完整中文题干 + 白话参考答（含为什么）
Q5 catalog：保持 learning-center-v2，禁止升 v3；JSON 可在现有列内增量 hint/why
Q6 UI：内容必须写完整参考；揭晓只显示本课 JSON 的 hint/why，禁止全课套 UCL 话术；禁止推倒导航/导入
Q7 图形：必须审计「菜单中文名 ↔ overlay 标题 ↔ 演示列 ↔ 实际出图/图标借用」，对不上定点修
Q8 编译/package：你跑 Python verify；我本机 Qt Creator / package_dist 自测；因中文路径不要强跑易失败的 cmake/package（除非我本轮明确要求打包）

【读者与文案铁律】
- 学员可见区禁止出现：command_id、dataset_id（可用一次并解释为工作表显示名 demo_*）、同构、白名单、WAVE、role_map 等开发口径
- 先修/检索必须是完整问句；选项 good/bad 必须是完整句子，不能是「2」「是」「graph_hist_prob」
- 自解释每条必须有 hint（3–6 句参考答，解释为什么，禁止复读题目）
- 非控制图课禁止无条件考 UCL
- 统计术语要用，但首次出现必须 glossary 白话释义；不要用莫名其妙的电报缩写代替正经术语
- 0–6：只把过短句补完整，禁止删成摘要壳；金标 imr 的片41/55、UCL≠USL、参数表不得砍掉

【六 Agent 顺序（同一会话编排；每岗 DoD 未过不得进下一岗）】
Agent1 调研：网上巩固「给新手写练习题」；对照 analysis_commands 做图形名实全表；抽检电报题反面教材；产出 docs/research/learning-center-copy-depth-and-graph-alignment-research.md；禁止改产品代码
Agent2 详细计划：Wave 锁表（复用前一轮 184 id）、JSON 兼容策略、verify 新断言、文件清单、§7 禁止偷懒；禁止 Plan 阶段改产品代码
Agent3 执行：Wave-0 = 解析兼容 + 揭晓 UI 读本课 hint + imr 7+ 金标加深；然后从生成器模板源头改掉电报题，连续 Wave-1…5 铺完 184；内容进 overlays/wave*_content/builder，禁止只手改 sqlite
Agent4 测试：扩展并跑 tools/verify_learning_center_*.py（可读性断言：禁止过短题干、禁止 good 像内部 id）；列出我需本机编译的 C++ test target
Agent5 收尾：对照手册 §7；结构衔接检查；commit + push；提示我 package_dist；可开 bugbot/code-review 扫学习中心相关 diff
Agent6 教学检验：站在非开发新人角度抽检；金标 7+ 必须像课堂提问而不是 API；图形名实抽检；产出 docs/research/learning-center-copy-depth-qa-report.md；驳回则定点返工，禁止顺手重构无关模块

【Wave 施工队列（必须全部做完）】
Wave-0：揭晓 UI + imr 7+ 金标加深（0–6 保留）
Wave-1：控制图包
Wave-2：质量工具 / MSA / 能力
Wave-3：统计推断 / ANOVA / 回归相关
Wave-4：图形名实对齐 + DOE + 可靠性 + 其余
Wave-5：清电报模板残留 + verify/文档

【衔接现网 — 禁止另起炉灶】
只扩展、不平行复制：
- src/application/learning/learning_types.h（可选 hint 字段）
- learning_tutorial_catalog.*（兼容旧 JSON）
- src/ui/learning_center_page.*（只改练习揭晓来源，禁止推倒左树/导入）
- MainWindow 导入新建工作表链路
- tools/build_learning_center_db.py、tools/learning_data/wave*_content.py、tutorial_overlays/*.json
- tools/verify_learning_center_*.py、tests/learning_center_*_test.cpp

禁止：重建第二套帮助系统；改 algorithm_help.json 公式语义；学习中心内嵌分析向导；导入覆盖当前表；QSql 连接泄漏；丢掉前一轮专用表/v2/0–6。

【已知必须修的问题（手册 §2，不许装没看见）】
- 先修/检索电报题（如「列数？」「共享族？」「禁止句」；答案写成 graph_hist_prob）
- 揭晓复读题目或全课套 UCL
- 图形 related_ids 模板写死 histogram+scatter+gallery
- bar_chart 图标键借用 pareto ≠ 菜单名「条形图」（教程标题不要改成 Pareto）
- area_plot 等 dialog_fill 可能缺必填角色

【完成标准】
- 184 课 7+ 达到手册 §4.2；过短 0–6 已补；金标 0–6 未砍
- 图形名实对照表无未修红项
- 揭晓用本课 hint/why
- Python learning_center gate PASS（含可读性断言）
- Agent6 QA：金标 7+ 通过；无没头没脑题 / 无范围缩水
- git commit + push；提示我本机 package_dist 抽查练习区

【每阶段输出格式】
变更文件列表 + DoD 勾选 + 风险一行 + go/no-go

现在开始：先确认已读手册，CreateGoal，然后从 Agent1 调研开工。不要重问 §0。
```

---

## §11 修订记录

| 日期 | 说明 |
|------|------|
| 2026-09-03 | 初稿：用户反馈练习没头没脑、揭晓怪、图名可能对不上、读者非开发；§0 拍板全集 + 保持 v2 + 揭晓读本课 hint |
