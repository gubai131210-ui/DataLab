# Goal：学习中心文案口吻升温（并肩版 · 全集润色 · 不推倒骨架）

> **用途**：新开一场 Goal 对话时的**唯一权威操作手册**（本轮）。  
> **状态**：§0 已由用户拍板（2026-09-03 18:31 UTC+8），**决策锁定，禁止子 Agent 重问或改口**。  
> **前一轮已收口基线（必须保留，禁止推倒）**：[`goal-learning-center-copy-depth-and-graph-alignment-plan-and-mega-prompt.md`](goal-learning-center-copy-depth-and-graph-alignment-plan-and-mega-prompt.md)  
> **再前一轮教学骨架（必须保留）**：[`goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md`](goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md)  
> **母框架**：[`goal-execution-framework.md`](goal-execution-framework.md)  
> **口吻标尺 Canvas（并肩版）**：`~/.cursor/projects/d-QT-CppPrograms-DataLab/canvases/learning-center-copy-voice-target.canvas.tsx`  
> **金标深度 Canvas（0–6 硬点仍有效）**：`~/.cursor/projects/d-QT-CppPrograms-DataLab/canvases/learning-center-tutorial-example.canvas.tsx`  
> **Wave id 锁表可复用**：[`goal-learning-center-pedagogy-upgrade-wave-plan.md`](goal-learning-center-pedagogy-upgrade-wave-plan.md)、[`goal-learning-center-copy-depth-wave-plan.md`](goal-learning-center-copy-depth-wave-plan.md)

---

## §0 用户已拍板决策（2026-09-03）— 锁定

| # | 问题 | **用户决定（锁定）** |
|---|------|----------------------|
| Q1 | 范围 | **全集 184 课**学员可见文案都按「并肩版」过一遍。内部 Wave 只是施工队列，**不得**停在金标或「下一 Goal 再铺」 |
| Q2 | 改哪些字段 | **全部**：`used_for` / `not_for` / `scenario` / glossary / `dialog_fill_detail` / 埋点 / 读输出 / 误用 / **7A·7C·7D·7E**（先修、褪脚手架、检索、错念）的题干与揭晓。**7B 自解释 JSON 可保留但不对学生展示**（UI 继续隐藏） |
| Q3 | 口吻标尺 | 锁定 Canvas **「并肩版」**：少「你的任务/禁止/这一课只练/你应该能」；多用「不妨/常见读法/多半/通常还要…」。易断岔口语统一成稳写法，例如「**波动主要落在…**」，禁止「抖主要…」这类歧义断句 |
| Q4 | catalog / 结构 | **保持 `learning-center-v2`**。只改 overlays / 生成器 / verify（必要时极小解析兼容）。**不推倒** `LearningCenterPage` 导航与导入。**7B 继续隐藏** |
| Q5 | 金标 `imr` | **0–6 允许明显加长**（加温度、过渡、并肩语气），**仍不删**片号（如 41/55）、UCL≠USL、参数表等硬点。7+ 同步并肩化 |
| Q6 | 六 Agent | **Agent1 调研** → **Agent2 计划** → **Agent3 执行** → **Agent4 测试** → **Agent5 收尾** → **Agent6 教学检验**。职责：网研+现网反面教材 / 详细 plan / 执行 / verify / commit·push / QA。全程 **`model: "inherit"`，中途不换模型** |
| Q7 | 编译与打包 | Agent 跑 Python verify；**用户本机** Qt Creator / `package_dist` 自测。Goal 结束 **必须 git commit + push GitHub**。中文路径不强跑易失败的 cmake/package，除非用户本轮明确要求打包 |
| Q8 | 文档路径 | 本文件路径保持：`docs/research/goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md` |
| Q9 | 与上一 Goal 关系 | **只换语气/加温度/加深可读**，**禁止推倒重写骨架**：专用主集、同构白名单、0–6 分节、折叠 UI、导入链路、v2、图形名实对齐成果一律保留 |

**编排者纪律**：§0 已关闭。子 Agent **禁止**再问范围/是否推倒/是否升 v3/是否重开 7B；若与本表冲突，以本表为准。

### §0.1 现网硬约束（写进 Wave plan 文首）

| ID | 约束 |
|----|------|
| H1 | `catalog_version` **保持** `learning-center-v2`。禁止只改 sqlite、禁止偷偷升 v3 |
| H2 | `dataset_id` **不以 `demo_` 开头**。工作表仍是 `demo_{dataset_id}` |
| H3 | 文案必须进 `build_learning_center_db.py` / `tutorial_overlays` / `wave*_content.py` / `copy_depth.py` / `glossary_bank.py`；**禁止**只手改 sqlite |
| H4 | 同构共享仍以前一轮白名单为准；禁止回归旧 10 表 |
| H5 | `dialog_fill` 保持角色→列名 object；详解走 `dialog_fill_detail` |
| H6 | 禁止为术语课新增虚假 command_id |
| H7 | 禁止推倒 `LearningCenterPage` 导航树 / 导入 / `WorksheetRegistry` |
| H8 | 禁止改 `algorithm_help.json` 公式语义 |
| H9 | 金标 `imr`：**允许 0–6 加长并肩**，但片号、UCL≠USL、9 字段参数表等硬点**不得删成摘要** |
| H10 | 练习 JSON 形状以前一轮为准（`why`/`hint`/`model_answer`）；解析器缺字段降级为空，不得破坏旧库可读 |
| H11 | **7B「步间自解释」UI 保持隐藏**； scrub 成果保留；禁止把「自解释」字样再写回学员可见导语 |
| H12 | 学员可见区禁止开发黑话（`command_id` / `同构` / `WAVE` / `overlay` / 内部 id 当正确答案） |
| H13 | 图形名实对齐成果保留：`title`/`menu_path` = 菜单中文；禁止为润色把条形图改名成 Pareto |

---

## §1 本 Goal 要交付什么（产品语言）

上一轮已经做成：**练习题能读懂、揭晓有参考、图名大体对齐**。本轮要做成：**读起来像车间同事并肩带你看一眼，而不是电报体或考试官说教**。

1. **口吻换挡**：184 课全部学员可见句从「电报 / 禁令 / 说教」升到「并肩观察」——2～4 句把一件事说完，能指着列名/子组/行号/菜单，红线用「通常还不写到… / 不等于放行样板」而不是「禁止过程合格」。  
2. **字段全覆盖**：0–6 与 7A/7C/7D/7E 同步升温；金标允许明显加长，硬点不砍。  
3. **用词稳住**：波动/批内批间等说法统一清晰，避免「抖主要」这类易断岔口语。  
4. **衔接现网**：只改学习中心内容层与生成器/verify；不新建第二套帮助系统，不重做窗口。

**非目标（本 Goal 不做）**

- 重建学习中心窗口、改导入覆盖策略、改 Table 内核。  
- 改 `algorithm_help.json` 公式。  
- 重新做图形名实大审计（除非润色时撞上明显回归，定点修）。  
- 重新启用 7B 学员可见区。  
- 把 184 课缩成「只改 imr」或「只改 used_for」。  
- 用浏览器代替用户本机 `package_dist`（除非用户本轮再要求打包）。  
- 中途换模型。

---

## §2 已证实的问题（Agent 不许装没看见）

对话与 Canvas 已锁定的口吻缺陷（抽检时间 2026-09-03）：

### 2.1 电报体（省字到尖）

`between_within_capability` 一类：

- `used_for`：「分子组估计组间与组内变差的能力。」  
- `scenario`：「子组 n=5。子组12起批均值上移。」  
- `not_for`：「禁止过程合格。」

读者像被训斥，不知道「为什么」和「先看什么」。

### 2.2 说教体（有字但仍训人）

上一轮润色草稿里仍常见：

- 「你的任务不是…而是…」  
- 「你应该能指着说…」  
- 「练习把话说完整…」  
- 「这一课只练读信号」  
- 「先别说必须停线」

要改成邀请与观察：「不妨先…」「常见读法是…」「现场口语可以停在…」「通常还要对照规程」。

### 2.3 歧义口语

「抖主要出在批内」易被读成「抖主」——统一写成「**波动主要落在批内还是批间**」等稳句。

### 2.4 模板套话残留

`dialog_fill_detail.meaning` 若仍是「在对话框里把 X 填进 Y。这一项决定图上或表上对应哪一列。」——必须按本课列名与现场含义重写，禁止全课复制粘贴同一壳。

### 2.5 正确样例（并肩 · 场景）

> 还是那条镀膜线：厚度按子组采，每个子组 5 片。前半段批均值挺老实；走到大约子组 12，整批均值像抬了一级台阶。不妨先打开「组间/组内过程能力」，看看台阶主要落在组间还是组内——停不停线、放不放行，可以等信号看清楚再和现场规程对一下。

Agent1 须再抽 ≥15 课，列出「电报 / 说教 / 模板壳 / 已合格并肩」四类反面与正面教材（路径+字段）。

---

## §3 必须衔接的现有代码（禁止另起炉灶）

| 路径 | 本 Goal 允许 |
|------|----------------|
| `tools/learning_data/tutorial_overlays/*.json` | **主战场**：重写学员可见文案 |
| `tools/learning_data/wave*_content.py` / `copy_depth.py` / `glossary_bank.py` | 从源头改模板，避免再灌电报/说教 |
| `tools/build_learning_center_db.py` | 重建 sqlite；禁止只手改库 |
| `tools/verify_learning_center_copy_depth.py` 等 | 增加口吻/反说教/反电报断言（见 §6） |
| `tools/scrub_self_explain_student_copy.py` | 若重生成又带回「自解释」学员字样，须再 scrub；**不**恢复 7B UI |
| `src/ui/learning_center_page.*` | **原则上不改**；仅当发现学员可见硬编码说教导语时定点改字。禁止重做左树/导入；禁止重新加入 7B 区块 |
| `src/application/learning/*` | **原则上不改**；除非 JSON 增量键需要兼容 |
| `learning_dataset_store.*` / `worksheet_registry.*` / `MainWindow::import_learning_dataset` | 不改语义 |
| `analysis_commands.cpp` / `algorithm_help.json` | **只读**；禁止为润色改公式或菜单 id |
| `tests/learning_center_*_test.cpp` | 仅当文案相关断言需要对齐时改；金标硬点断言保留 |

架构保持：

```
帮助菜单 → 学习中心 LearningCenterPage
            左：搜索 + 命令树
            右：0–6 分节 + 7A/7C/7D/7E（无 7B）
            导入 → LearningDatasetStore → WorksheetRegistry
内容源： overlays / wave*_content / builder → learning-center.sqlite (v2)
```

---

## §4 内容规格（验收口径）

### 4.1 口吻铁律（并肩版）

| 要有 | 不要有 |
|------|--------|
| 2～4 句把一件事说完；允许「不妨 / 也就是说 / 常见读法」 | 电报：`子组 n=5。子组12起批均值上移。` |
| 能指着：列名、子组号、行号、菜单路径、图上哪一块 | 「你的任务是…」「你应该能…」「请你主动…」 |
| 红线带共享习惯：「不等于放行样板」「通常还要对照规程」 | 「禁止过程合格。」「这一课只练…」纯禁令收束 |
| 波动等稳词：「波动主要落在批内还是批间」 | 「抖主要…」及易断岔口语 |
| 字段含义写清本课列在现场是什么 | 全课复制「这一项决定图上或表上对应哪一列」 |

读者：非开发、统计基础薄弱的车间/质量学员。语气像**耐心同事**，不是考试官，也不是抒情散文。

### 4.2 字段最低质量（184 全覆盖）

| 块 | 最低要求 |
|----|----------|
| `used_for` / `not_for` | 各至少 2 句完整中文；说清「适合什么现场问题 / 什么情况先别硬套」；红线带原因 |
| `scenario` | 有产线/测量情境；有可指着的埋点位置；用「不妨…」邀请，不用「你的任务是」 |
| glossary | 术语 + 白话 + 怎么记；控制限课保留 UCL≠USL；非控制限课不为凑数考 UCL |
| `dialog_fill_detail` | 每字段 `meaning` 结合本课列名；禁止全课同一壳 |
| 埋点 / 读输出 | 指到行号或子组/图区；读出「看见什么」；放行/停线用「通常还要…」留白 |
| 误用 | 完整句子讲「容易漏什么、对调会怎样」 |
| 7A 先修 | 完整问句 + 完整选项；`why` 并肩解释，不训斥 |
| 7C 褪脚手架 | `student` 句可独立执行；语气邀请而非命令堆砌 |
| 7D 检索 | 完整问句 + `hint` 参考答；像助教笔记不是禁令牌 |
| 7E 错念 | `wrong`/`right` 完整；`right` 讲对在哪，不写「禁止已证明正态。」 |

**金标 `imr`**：0–6 **允许明显加长并肩**；片 41/55、UCL≠USL、参数表硬点必须仍可被 Agent6 指认。7+ 同步并肩化。

### 4.3 与上一 Goal 的边界

| 上一 Goal 已交付 | 本 Goal |
|------------------|---------|
| 练习可读、hint/why、图形名实、v2 | **保留** |
| 电报题/内部 id 当答案已清 | 若回归，定点修；主战场是口吻 |
| 7B UI 隐藏 | **保持隐藏** |
| 专用表 / 白名单 | **不重做数据集** |

---

## §5 六 Agent 流水线（一场 Goal；模型锁定 inherit）

```
Agent1 调研学习（网 + 口吻标尺 + 现网反面教材；禁止改产品代码）
  → Agent2 详细计划（Wave 锁表 + 文案规范 + verify 新断言；禁止改产品代码）
  → Agent3 执行（先金标 imr 0–6+7+ 并肩样板，再从模板源头铺 184）
  → Agent4 测试（verify 口吻/反电报/反说教断言 + 列出 C++ target）
  → Agent5 收尾（§8 勾选 + commit/push + 提示 package + 可开 review）
  → Agent6 教学检验（非开发读者；金标必须像同事带看，不是训话；出 QA 报告）
  → 驳回则定点返工，禁止顺手重构无关模块
```

### Agent 1 — 调研（Research）

**做什么**

1. 网上巩固「车间作业指导 / 白话统计帮助 / 非说教教学文案」：短句但细节够、主动邀请、警告带原因。Primary URL 写入 research md（访问日期 UTC+8）。  
2. 对照 Canvas 并肩版标尺，抽检现网 overlays：**电报 / 说教 / 模板壳 / 已合格** 四类，≥15 课真实路径+字段。  
3. 扫 `copy_depth.py` / `wave*_content.py` 是否仍在灌说教或电报模板。  
4. 产出：`docs/research/learning-center-copy-voice-warmth-research.md`

**DoD**

- [ ] research md 含 Primary URL 表 + 访问日期  
- [ ] 反面教材 ≥15 条；正面样例 ≥3 条（可引用 Canvas）  
- [ ] 写清「补充语气、不推倒骨架」边界  
- [ ] **禁止**改产品代码  

### Agent 2 — 计划（Plan）

产出：`docs/research/goal-learning-center-copy-voice-warmth-wave-plan.md`

必须含：Wave 锁表（复用前一轮 184 id，不得漏）、字段检查单、生成器改法、verify 新断言、§7 禁止偷懒全文、文件改动清单、「明确不改哪些文件」、金标加长但硬点清单。

**DoD**

- [ ] Wave-0 = `imr` 0–6+7+ 并肩金标样板（硬点不删）  
- [ ] Wave-1…n 铺完全集 184，不得「等等」  
- [ ] 模板源头文件列入必改  
- [ ] **禁止** Plan 阶段改产品代码  

### Agent 3 — 执行（Implement）

顺序强制：

1. 以 `imr` 为闸门：0–6 加长并肩 + 7+ 并肩；硬点可被指认  
2. 改 `copy_depth.py` / `wave*_content.py` / `glossary_bank.py` 等模板源头——去掉电报与说教壳  
3. 按 Wave 重写 overlays；builder 重建 sqlite  
4. 若学员可见又出现「自解释」字样 → 跑 scrub；**不**加回 7B UI  
5. 禁止只手改 sqlite；禁止碰无关 `analysis_service` 大文件  

**DoD（每 Wave）**

- [ ] 该 Wave 每课 `used_for`/`scenario` 不再是单行电报  
- [ ] 抽检无「你的任务是 / 这一课只练 / 禁止过程合格」等说教禁令壳（误用里讲清原因的完整句除外）  
- [ ] `dialog_fill_detail` 无全课同一壳  
- [ ] 金标硬点仍在  
- [ ] 禁止只改 sqlite  

### Agent 4 — 测试（Test）

跑：`tools/verify_learning_center_copy_depth.py`、`verify_learning_center_gate.py`、`verify_learning_center_db.py`（及本轮新增断言脚本）。

**口吻断言为硬门（不是建议）**，必须写入 verify 并 PASS：

- `used_for`/`scenario`/`not_for` 最小汉字长度（挡电报）  
- 禁止学员可见区匹配黑名单：`你的任务是`、`这一课只练`、`禁止过程合格`、`抖主要`  
- 禁止 `dialog_fill_detail.meaning` 大面积完全相同（模板壳）  
- 金标 imr：仍强制 glossary UCL/USL、buried 含片号硬点；长度下限提高但不得删硬点  
- 保持上一轮：`good` 不得像 `^[a-z][a-z0-9_]+$`；先修题干长度等  

列出用户需编的 C++ target（若本轮未改 C++ 可写「无强制」）：`learning_center_store_test` 等。

### Agent 5 — 收尾

对照 §7 禁止偷懒 + §8 完成定义；diff 不得误改无关大文件；**commit + push**；提示本机 `package_dist`；可开 bugbot / code-review 子 Agent 扫学习中心相关 diff。**禁止**收尾塞新功能。

### Agent 6 — 教学检验

角色：给车间新人上课的同事（**不是**代码 reviewer，也不是考试官）。

1. 金标 imr：0–6 读起来像并肩带看；硬点（片号、UCL≠USL）仍在；无说教收束。  
2. 每 Wave ≥3 课：有无电报、有无「你的任务是」、字段含义是否套壳、红线是否仍像训人。  
3. 抽 2 课练习区：揭晓是否像助教笔记。  
4. 产出：`docs/research/learning-center-copy-voice-warmth-qa-report.md`  
5. 金标口吻 + 硬点 **必须通过**才能 Goal complete。驳回只开定点返工单。

---

## §6 施工队列（内部 Wave；必须全部做完）

| Wave | 内容 | 出口 |
|------|------|------|
| Wave-0 | **imr** 0–6+7A/7C/7D/7E 并肩金标样板；模板源头先拆电报/说教壳 | Agent4+6：口吻通过 + 硬点仍在 |
| Wave-1 | 控制图包：0–6 + 7A/7C/7D/7E 全文并肩 | gate：该包每课 §4.2 全字段 |
| Wave-2 | 质量 / MSA / 能力：同上全字段 | gate：同上 |
| Wave-3 | 推断 / ANOVA / 回归：同上全字段 | gate：同上 |
| Wave-4 | 图形 / DOE / 可靠性 / 其余：同上全字段 | gate：同上 |
| Wave-5 | 黑名单 verify、scrub 残留、文档、重建 sqlite | Agent5+6 |

id 列表沿用前一轮 Wave plan，Agent2 不得漏 id。**184 必须全部做完。**

---

## §7 禁止偷懒（Plan 必须粘贴）

1. **禁止**推倒学习中心窗口/导入链路/专用表/v2/0–6 骨架。  
2. **禁止**只改金标或只改 `used_for` 就宣称 Goal 完成。  
3. **禁止**金标口吻未达标就批量灌水；也**禁止**金标过后借口下轮再铺 184。  
4. **禁止**电报体交付（单行断句堆事实）。  
5. **禁止**说教壳：`你的任务是` / `你应该能` / `这一课只练` / 纯 `禁止过程合格`。  
6. **禁止**歧义口语「抖主要」。  
7. **禁止** `dialog_fill_detail` 全课复制同一句壳。  
8. **禁止**为省事删掉金标片号 / UCL≠USL / 参数表硬点。  
9. **禁止**重新显示 7B 或把「自解释」写回学员导语。  
10. **禁止**学员可见区出现 `dataset_id` / `同构` / `WAVE` 当教学内容。  
11. **禁止**只手改 sqlite。  
12. **禁止**中途换模型；所有 Task **`model: "inherit"`**。  
13. **禁止** Agent6 没出 QA 报告就 UpdateGoal complete。  
14. **禁止**用浏览器代替用户本机 package（除非用户本轮再要求）。  
15. **禁止**升 catalog 到 v3。  
16. **禁止** `dataset_id` 以 `demo_` 开头。  
17. **禁止** `dialog_fill` 覆盖成数组。  
18. **禁止**新增虚假 command_id。  
19. **禁止**改 `algorithm_help.json` 公式语义。  
20. **禁止**并行两人改同一大文件；禁止收尾塞无关重构。  
21. **禁止**把「短句」理解成「省字到尖」——细节要够，态度要稳。  
22. **禁止**抒情散文或堆砌无关比喻；一句现场比喻足够。  

---

## §8 完成定义

- [ ] §0 已锁定（本文）。  
- [ ] 184 课 §4.2 字段全部并肩化；无电报/说教壳大面积残留。  
- [ ] 金标 `imr`：口吻通过 + 硬点仍在。  
- [ ] 7B UI 仍隐藏；学员可见无「自解释」导语回归。  
- [ ] catalog 仍为 `learning-center-v2`。  
- [ ] Python gate PASS（含口吻黑名单与长度断言）。  
- [ ] Agent6 QA 报告已写；金标通过。  
- [ ] git commit + push；提示用户本机 `package_dist` 抽查学习中心文案口吻。

---

## §9 每阶段输出格式

变更文件列表 + DoD 勾选 + 风险一行 + go/no-go

### 子 Agent 提示词头

```text
你是 DataLab「学习中心文案口吻升温（并肩版）」Goal 的 Agent{N}:{角色}。
权威手册：docs/research/goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md
口吻标尺 Canvas：learning-center-copy-voice-target.canvas.tsx（并肩版，不是说教版）。
前两轮基线必须保留：专用表、v2、0–6 分节、导入链路、练习 hint/why、图形名实、7B 隐藏。
本轮只换语气/加温度/加深可读，禁止推倒骨架。
模型：inherit，禁止建议换模型。
读者：非开发、基础薄弱。语气：耐心同事并肩看，不是考试官。
禁止偷懒：见该手册 §7。
交付：文件列表 + DoD 勾选 + 风险一行 + go/no-go。
```

---

## §10 新对话开场粘贴（Mega Prompt）

> 把下面整段贴进**新 Goal 对话**首条即可（§0 已填，勿再问）。建议同时挂上 `/goal` skill。

```text
/goal

【Goal 启动】学习中心文案口吻升温（并肩版 · 全集 184 · 补充不推倒）

权威手册（唯一决策源，先通读再动手）：
docs/research/goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md

请立即用 CreateGoal 建立长跑目标，objective 写清本 Goal 名称与手册路径。
母框架可参考 docs/research/goal-execution-framework.md。
前一轮已收口（必须保留）：docs/research/goal-learning-center-copy-depth-and-graph-alignment-plan-and-mega-prompt.md
再前一轮骨架（必须保留）：docs/research/goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md
口吻标尺 Canvas（并肩版）：~/.cursor/projects/d-QT-CppPrograms-DataLab/canvases/learning-center-copy-voice-target.canvas.tsx
金标深度 Canvas（硬点）：~/.cursor/projects/d-QT-CppPrograms-DataLab/canvases/learning-center-tutorial-example.canvas.tsx
Wave id 锁表可复用：docs/research/goal-learning-center-pedagogy-upgrade-wave-plan.md
以及：docs/research/goal-learning-center-copy-depth-wave-plan.md

【模型锁定 — 最重要】
- 全程只使用我当前对话所用模型。
- 所有 Task 子 Agent 必须 model: "inherit"。
- 中途禁止换模型，禁止建议换模型「省钱/加速」。

【§0 已拍板 — 禁止重问、禁止改口】
Q1 范围：全集 184 课学员可见文案都按并肩版过一遍；内部 Wave 只是施工队列，不得停在金标或推到下一 Goal
Q2 字段：全部 — used_for / not_for / scenario / glossary / dialog_fill_detail / 埋点 / 读输出 / 误用 / 7A·7C·7D·7E；7B JSON 可留但 UI 继续隐藏
Q3 口吻：并肩版（不妨/常见读法/通常还要…）；统一「波动主要落在…」；禁止「抖主要」歧义；少「你的任务/禁止/这一课只练」
Q4 结构：保持 learning-center-v2；不推倒导航/导入；7B 继续隐藏
Q5 金标 imr：0–6 允许明显加长并肩，仍不删片号/UCL≠USL/参数表硬点；7+ 同步并肩化
Q6 六 Agent：Agent1 调研 → Agent2 计划 → Agent3 执行 → Agent4 测试 → Agent5 收尾 → Agent6 教学检验；全程 inherit
Q7 编译/package：你跑 Python verify；我本机 Qt / package_dist 自测；结束必须 commit + push；中文路径不强跑易失败 cmake/package（除非我本轮明确要求打包）
Q8 文档路径：即本手册，不改名
Q9 态度：只换语气/加温度，禁止推倒专用表/白名单/0–6/折叠 UI/导入/v2/图形名实/练习 hint 成果

【读者与口吻铁律】
- 语气：车间同事并肩带看，不是考试官，不是电报，不是抒情散文
- 要有：2～4 句说完；能指着列名/子组/行号/菜单；红线写成「不等于放行样板 / 通常还要对照规程」
- 不要有：电报断句；「你的任务是」「你应该能」「这一课只练」；纯「禁止过程合格」；「抖主要」；dialog_fill_detail 全课同一壳
- 学员可见区禁止：command_id、同构、WAVE、内部 id 当正确答案
- 统计术语要用，首次 glossary 白话；非控制限课不为凑数考 UCL
- 金标硬点必须可指认：片号、UCL≠USL、参数表

【六 Agent 顺序（同一会话编排；每岗 DoD 未过不得进下一岗）】
Agent1 调研：网上巩固车间白话/非说教帮助写法；对照 Canvas 并肩标尺抽检 ≥15 课反面教材；扫生成器是否仍灌电报/说教；产出 docs/research/learning-center-copy-voice-warmth-research.md；禁止改产品代码
Agent2 详细计划：Wave 锁表（184 不漏）、字段检查单、verify 口吻断言、§7 禁止偷懒、文件清单；产出 docs/research/goal-learning-center-copy-voice-warmth-wave-plan.md；禁止 Plan 阶段改产品代码
Agent3 执行：Wave-0 = imr 0–6+7+ 并肩金标（硬点不删）→ 改 copy_depth/wave*_content/glossary 模板源头 → Wave-1…5 铺完 184 → builder 重建 sqlite；禁止只手改 sqlite；禁止恢复 7B UI
Agent4 测试：扩展并跑 tools/verify_learning_center_*.py（长度下限 + 说教/电报黑名单 + 金标硬点）；列出我需本机编译的 C++ target（若无 C++ 改动写明）
Agent5 收尾：对照手册 §7 禁止偷懒 + §8 完成定义；结构衔接检查；commit + push；提示我 package_dist；可开 bugbot/code-review 扫学习中心相关 diff；禁止收尾塞新功能
Agent6 教学检验：站在非开发新人角度；金标必须像同事带看且硬点仍在；每 Wave 抽检电报/说教/套壳；产出 docs/research/learning-center-copy-voice-warmth-qa-report.md；驳回则定点返工，禁止顺手重构无关模块

【Wave 施工队列（必须全部做完）】
Wave-0：imr 并肩金标 + 拆模板源头
Wave-1：控制图包
Wave-2：质量 / MSA / 能力
Wave-3：推断 / ANOVA / 回归
Wave-4：图形 / DOE / 可靠性 / 其余
Wave-5：verify 黑名单 + scrub 残留 + 文档 + 重建库

【衔接现网 — 禁止另起炉灶】
只扩展内容层，不平行复制：
- tools/learning_data/tutorial_overlays/*.json
- tools/learning_data/wave*_content.py、copy_depth.py、glossary_bank.py
- tools/build_learning_center_db.py
- tools/verify_learning_center_*.py
- tools/scrub_self_explain_student_copy.py（仅防回归）
- src/ui/learning_center_page.*（原则上不改；禁止加回 7B；仅定点改硬编码说教导语）
- src/application/learning/*（原则上不改）

禁止：重建第二套帮助系统；改 algorithm_help 公式；学习中心内嵌分析向导；导入覆盖当前表；丢掉专用表/v2/0–6/图形名实/练习 hint；升 v3；中途换模型。

【已知必须修的问题（手册 §2，不许装没看见）】
- 电报体：短到尖的断句（如「子组 n=5。子组12起批均值上移。」）
- 说教体：「你的任务是…」「这一课只练…」「禁止过程合格。」
- 歧义口语：「抖主要…」→ 统一「波动主要落在…」
- dialog_fill_detail 模板壳：「这一项决定图上或表上对应哪一列。」

【完成标准】
- 184 课达到手册 §4.2；金标口吻通过且硬点仍在
- 7B 仍隐藏；catalog 仍 v2
- Python learning_center gate PASS（含口吻断言）
- Agent6 QA 通过；无范围缩水
- git commit + push；提示我本机 package_dist 抽查口吻

【每阶段输出格式】
变更文件列表 + DoD 勾选 + 风险一行 + go/no-go

现在开始：先确认已读手册，CreateGoal，然后从 Agent1 调研开工。不要重问 §0。
```

---

## §11 修订记录

| 日期 | 说明 |
|------|------|
| 2026-09-03 | 初稿：用户确认电报/说教问题；拍板 Q1A 全集、Q2 全字段、Q3 并肩+「波动主要落在」、Q4 保 v2/隐 7B、Q5 金标可加长不砍硬点、Q6 六 Agent inherit、Q7 verify+自测+必 push、Q8 路径不变、Q9 补充不推倒 |
