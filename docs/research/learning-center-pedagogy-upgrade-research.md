# 学习中心教学升级 — Agent1 调研总册

> **岗**：Agent1 Research（**禁止改产品代码**）  
> **日期**：2026-09-03  
> **权威手册**：[`goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md`](goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md)  
> **模型**：inherit（与主对话相同；禁止建议换模型）  
> **衔接**：`learning_types` / `learning_tutorial_catalog` / `learning_dataset_store` / `LearningCenterPage` / `WorksheetRegistry` / `tools/build_learning_center_db.py`  
> **金标 Canvas**：`C:\Users\孤白赟悫\.cursor\projects\d-QT-CppPrograms-DataLab\canvases\learning-center-tutorial-example.canvas.tsx`

本文件是本 Goal 的**调研总册**。旧 [`learning-center-research-notes.md`](learning-center-research-notes.md) 的用途/误用仍可抽查；**`建议 dataset_id` 与 10 张共享表 mapping 已作废**（见 §3）。Agent2 以本文件 + 手册 §0/§4/§8 写 Wave plan。

---

## 0. §0 决策已锁定摘要（勿重问）

用户 2026-09-03 10:16 UTC+8 拍板，子 Agent **禁止**再问范围/UI/旧表策略。

| # | 锁定 |
|---|------|
| Q1 | 全程 **inherit 当前对话模型**；禁止换模型 |
| Q2 | **本 Goal 一次铺开全集**（`analysis_commands::all()` ∪ `algorithm_help.json`）；Wave 只是施工队列 |
| Q3 | **UI 一次做完**：0–6 分节折叠 + 7+ 练习区可交互展示 |
| Q4 | 默认 **一主命令一专用表**；同构工具可共享**极小族**（白名单） |
| Q5 | **删除旧 10 共享表并重建**（`smt_paste_height` 等及旧 mapping 主链淘汰） |
| Q6 | Agent 跑 Python verify；用户本机 Qt / `package_dist` 自测 |

### 0.1 硬约束（写入 Wave plan 文首）

| ID | 约束 |
|----|------|
| H1 | `catalog_version` **双写** `learning-center-v2`：`tools/build_learning_center_db.py` 的 `META_VERSION` **与** `LearningDatasetStore::kExpectedCatalogVersion`（现均为 `learning-center-v1`） |
| H2 | 工作表公式是 `demo_{dataset_id}`。金标 **`dataset_id` = `imr_spi_shift`** → 显示名 `demo_imr_spi_shift`。**禁止** id 以 `demo_` 开头 |
| H3 | 无独立 `tutorials.json`；文案必须进 `build_learning_center_db.py` 生成器覆盖层 |
| H4 | 旧 10 表生成器/CSV/**默认挂 smt_paste_height** 删除或整段替换 |
| H5 | 测试「恰好 10 张表」断言必须改；DDL 优先 `ADD COLUMN`，保留 `research_sources` 等旧列名 |
| H6 | glossary「基础统计术语」**禁止**新增虚假第 185 个 `command_id`；用 `related_ids` 或课内嵌 glossary |
| H7 | `dialog_fill` 保持角色→列名 **object**；详解走并列 `dialog_fill_detail`。`parse_dialog_fill` 只认 JSON object 的 string 值 |
| H8 | 金标对话框以代码 `imr` 为准（含 Nelson / 特殊原因）；Canvas 可少写，实现不得删真实 inputs |
| H9 | `LearningCenterPage::entries_` 元数据缓存允许；禁止整库 cell 常驻巨大 `QVector` |
| H10 | 旧 black-belt plan「强制共享」**已被本手册 §0/§2 覆盖** |
| H11 | 扩展 `LearningCenterPage` 渲染；**禁止**推倒导航树/导入按钮链路 |

---

## 1. 教学 skill 核实（accessed 2026-09-03）

本地 `~/.agents/skills/teach/SKILL.md`（手册 §11）：**一课一事**、术语表、ZPD、检索练习、storage strength > fluency illusion。落到本产品：每条 implemented 教程只教一个菜单 id；§0 glossary 先扫盲；7+ 用检索而不是再读一遍。

### 1.1 Worked example + backward fading（Renkl / Atkinson）

| 主张 | 落到学习中心 |
|------|----------------|
| 新手先看**完整例题**（每步为何），再逐步把最后几步变成学员自己填 | §4–5 是完整例；`fade_levels` 由「参数表全给」→「只给清单」→「独立换表」 |
| Backward fading：先省略**最后一步**（Renkl 等现场实验用此法） | 金标：完成题 1 = 自己判断片 55 是否越 UCL；对话框仍给出 |
| 相对「例题–习题对」的突然切换，fade 降低错误、促进近迁移 | **禁止**讲完摘要就丢独立题（手册 §8.19） |

**URL（本机核实 2026-09-03）**

- Renkl, Atkinson, Maier, Staley (2002). *From Example Study to Problem Solving: Smooth Transitions Help Learning*. *J. Exp. Educ.* 70(4):293–315. https://doi.org/10.1080/00220970209599510 — DOI 页可解析；出版社全文 fetch 超时，以 DOI + 摘要索引为准  
- Renkl & Atkinson (2003). *Structuring the Transition From Example Study to Problem Solving…* *Educ. Psychol.* 38(1). https://doi.org/10.1207/s15326985ep3801_3  
- Renkl, Atkinson, Maier (2000) eScholarship 记录：https://escholarship.org/uc/item/81b9j9hs — 本机 WebFetch 返回 CloudFront 403；保留 DOI/记录 URL，不把 403 页当正文源  
- 综述：Atkinson, Derry, Renkl, Wortham (2000). *Learning from Examples*. *Rev. Educ. Res.* 70(2):181–214. https://doi.org/10.3102/00346543070002181

### 1.2 Self-explanation（Chi 1989）

好学生在例题上**生成解释、把步骤连到原理**；差学生复制步骤。提示自解释可提高理解。落到：`self_explain[]` 挂在导入/选菜单/留空阶段列/读片 55 等步后，**不替代** §4 参数表。

**URL（2026-09-03）**

- Chi, Bassok, Lewis, Reimann, Glaser (1989). *Self-Explanations…* *Cognitive Science* 13(2):145–182. https://doi.org/10.1207/s15516709cog1302_1  
- Wiley 记录：https://onlinelibrary.wiley.com/doi/10.1207/s15516709cog1302_1 — fetch 超时；DOI 有效  
- ERIC 技术报告 PDF（可拉取）：https://files.eric.ed.gov/fulltext/ED296291.pdf

### 1.3 CLT / Sweller–van Merriënboer–Paas

工作记忆有限；新手用 worked example / completion problem 降低外来负荷，把资源留给图式建构。落到：专用少列表（降低无关列分裂注意）；右侧 **0–6 与 7+ 分节折叠**（progressive disclosure）；禁止一屏塞满 glossary+练习。

**URL（2026-09-03）**

- Sweller, van Merriënboer, Paas (1998). *Cognitive Architecture and Instructional Design*. *Educ. Psychol. Rev.* 10:251–296. https://doi.org/10.1023/A:1022193728205 — Springer 页已打开（摘要+参考文献列表）  
- 镜像 PDF 索引：https://link.springer.com/article/10.1023/A:1022193728205

### 1.4 主动回忆 / testing effect

测验本身增强长期保持，优于等时重读（Roediger & Karpicke 2006）。落到：`prereq_quiz`（先修，错则回 glossary）+ `retrieval_quiz`（合上教程）。先修用「正确理解 vs 常见错选」对抗 UCL=USL 等错念；检索题偏自由回忆（「用一句话区分 UCL 与 USL」）。

**URL（2026-09-03）**

- Roediger & Karpicke (2006). *Test-Enhanced Learning*. *Psychological Science* 17:249–255. https://doi.org/10.1111/j.1467-9280.2006.01693.x  
- Sage 记录：https://journals.sagepub.com/doi/10.1111/j.1467-9280.2006.01693.x — fetch 超时  
- PubMed：https://pubmed.ncbi.nlm.nih.gov/16507066/  
- 作者 PDF（Washington Univ. 镜像，已索引）：http://psychnet.wustl.edu/memory/wp-content/uploads/2018/04/Roediger-Karpicke-2006_PsychSci-1.pdf

### 1.5 错念分析

显式「错念 → 纠正」比只列禁止句更容易检索。与 §6 `common_mistakes` **互补**：§6 是纪律（禁止写「过程合格」）；`misconceptions` 是诊断卡片。edu-agent 原则：先修检查、主动回忆、显式纠错念、控认知负荷。

**URL（2026-09-03）**

- https://github.com/enigmaicon-eng/edu-agent-skills — 仓库页可打开（主动回忆 / Socratic / misconception-detector）  
- https://github.com/GarethManning/education-agent-skills — 仓库页可打开（retrieval-practice-generator、cognitive load、worked example 域）  
- 检索练习 skill 路径：https://github.com/GarethManning/education-agent-skills/blob/main/skills/memory-learning-science/retrieval-practice-generator/SKILL.md

### 1.6 Progressive disclosure（UI，不是第二套帮助）

**URL（2026-09-03）**：https://agentskills.io/specification — 已打开。「元数据 → 正文 → 按需资源」对应：左侧树只显示条目；右侧默认展开 0–6；**7+ 默认折叠**（Agent2 可改默认展开策略，但必须有折叠，手册 §8.20）。

---

## 2. 7+ skill 字段 → schema 映射

现网 `tutorials` 列（`build_learning_center_db.py`）：`command_id, title, category, menu_path, implemented_status, used_for, not_for, scenario, dataset_id, click_steps, dialog_fill, output_guide, common_mistakes, related_ids, research_sources`。

`LearningTutorialEntry` 尚无 glossary / 练习字段。**增量 JSON 文本列**（优先 `ADD COLUMN`，旧库缺列解析为空）。

| 教学块（Canvas / 手册 §4） | 建议列名 | JSON 形状 | 现网衔接 |
|---------------------------|----------|-----------|----------|
| 0 关键词 | `glossary` | `[{term,plain,remember}]` | 新；控制限课必含 UCL≠USL |
| 4 参数详解 | `dialog_fill_detail` | `[{field,put,meaning}]` | **并列**；不得覆盖 `dialog_fill` |
| 2/5 埋点 | `buried_signals` | `[{row,what,expect}]` | 与 `datasets.notes` 同行号 |
| 7A 先修 | `prereq_quiz` | `[{q,good,bad}]` | Canvas `PREREQ_QUIZ` |
| 7B 自解释 | `self_explain` | `[{after,prompt}]` | Canvas `SELF_EXPLAIN` |
| 7C 褪脚手架 | `fade_levels` | `[{level,student,scaffold}]` | Canvas `FADING` |
| 7D 检索 | `retrieval_quiz` | `[string]` | Canvas `RETRIEVAL` |
| 7E 错念 | `misconceptions` | `[{wrong,right}]` | 与 `common_mistakes` 互补 |
| 技能句 | `skill_mission` | 纯文本 | 一句话本课目标 |

**`dialog_fill`（H7）**

- 类型：JSON **object**，键 = `RoleSpec.id`（及需要预填的 input id 仅当现网解析器已支持；**现网 `parse_dialog_fill` 只把 object 的 string 值读进 `QMap`**，学习中心导入映射按**角色**用）。  
- 金标建议 object **只含角色列映射**：`{"variables":"锡膏高度_um"}`。阶段列留空 = **不要写 stage 键**（或空串，Agent2 定一种并在 verify 里锁）。  
- 所有字段的「填什么/代表什么/为何留空」放进 `dialog_fill_detail`，键名用对话框 **label 或 input id**，与代码一致。

**catalog_version**：字符串 `learning-center-v2`，Python + C++ **双写**。

**术语课**：不新增 command；基础术语用课内 `glossary` + `related_ids` 指向已有 id（如 `imr`, `capability`）。

---

## 3. 旧「共享大表」为何被 §0/§2 覆盖（教学理由）

不是情绪、不是「想重做窗口」。

| 机制 | 共享 10 表（现 mapping：`smt_paste_height` 挂 42 个 id） | 专用主集 |
|------|----------------------------------------------------------|----------|
| 注意 / CLT | 宽表列多（产线/班次/钢网…），学员工作记忆被**本课用不到的角色**占满 | 2–6 列，只服务本菜单角色 |
| Worked example | 同一张表既要教 I-MR 阶跃又要教 Cpk，埋点互相污染（失控集算能力是明确误用） | 一课一个故意信号；能力课必须近似稳定 |
| 自解释 | 「为什么选这列」无法回答：因为大家都挂这张表 | 每列有角色与「不进对话框」的备注列 |
| 检索 | 读图话术只能写套话 | 对着行号（片 41/55）检索 |
| 错念 | 强化「一张表跑完全部算法」 | 金标 misconceptions 第二条直接打这条 |

代码证据：`docs/research/learning-center-dataset-mapping.md` 中 `imr`、`capability`、`histogram`、`one_sample_t` 均映射 `smt_paste_height`。I-MR 课若用含阶跃+尖峰的失控序列去算 Cpk，正好踩手册 §8.9。  
**同构极小共享仍允许**（Q4）：仅当 `roles`/`inputs`/埋点教学信号一致，且白名单写清服务哪些 `command_id`。默认仍一主命令一表。

---

## 4. Canvas 结构（0–6 + 7+）与现网缺口

Canvas 组件顺序（实现应对齐，而不是再发明骨架）：

| 节 | Canvas | 现网 `build_entry_html` |
|----|--------|-------------------------|
| 0 | glossary 表 + UCL≠USL callout | 无 |
| 1 | 背景（问题、Y、DMAIC、本课只回答什么） | `used_for`/`scenario` 摘要级 |
| 2 | 专用数据 + 列角色 + 样例行 + **dataset 显示名** | `dataset_id` + 导入；无埋点行号 UI |
| 3 | 为何此工具（I-MR vs Xbar-R） | `not_for` 部分覆盖 |
| 4 | click_steps + 参数表 field/put/meaning | `click_steps` + `dialog_fill` 仅列名 |
| 5 | 对着埋点读 I/MR/诊断表 | `output_guide` 从 help 切句，不对照行号 |
| 6 | 误用禁止句 | `common_mistakes` |
| 7+ | prereq / self_explain / fade / retrieval / misconceptions | **无**；UI 无折叠 |

Canvas Stat 写 `demo_imr_spi_shift` = **工作表显示名**（H2），不是 sqlite 主键。

---

## 5. 金标 `imr`：代码字段 vs Canvas 参数表（无虚构菜单）

**菜单（代码）**：`控制图` → `I-MR 控制图`（`menu_path`=`控制图`，`menu_group`=`计量图`，`id`=`imr`）。  
**数据**：`dataset_id`=`imr_spi_shift`；导入后工作表 `demo_imr_spi_shift`。列：`片号`（序，不进对话框）、`锡膏高度_um`（Y）、`时段备注`（读图，不进对话框）。埋点：片 41 阶跃、片 55 尖峰。

### 5.1 真实 roles / inputs（`analysis_commands.cpp` 约 5921–5939）

**Roles**

| id | 对话框 label | multi | optional |
|----|----------------|-------|----------|
| `variables` | 变量 | false | false |
| `stage` | 阶段列 | false | true |

**Inputs（实现不得删）**

| id | 对话框 label | 来源 | 默认 / 选项 |
|----|----------------|------|-------------|
| `mr_length` | 移动极差长度 | 整数，placeholder `2` | 金标填 `2` |
| `sigma_method` | σ 方法 | catalog `imr_sigma_method` | 默认 `average_moving_range`（平均移动极差）；另有 `median_moving_range`、`mssd` |
| `use_nelson_estimate` | Nelson estimate | catalog `zero_one_flag` | 默认 `0`；`1`=剔除过大 MR 后重估 σ（仅平均 MR 路径） |
| `rule_policy` | 规则默认策略 | catalog `special_cause_rule_policy` | 默认 `all_applicable`（个体图适用规则 1–8）；`minitab_like`=仅「单点超出 3σ」 |
| `tests` | 特殊原因测试 | `InputKind::special_cause_tests`，placeholder=`individuals` | 空 = 走 `rule_policy`；非空则 policy 变 `explicit` |
| `historical_center` | 历史均值 | 可选 | 金标**留空** |
| `historical_sigma` | 历史 Sigma | 可选 | 金标**留空** |

个体图适用规则 1–8（`special_cause_rule_catalog`，与 Minitab Tests 1–8 同族，**不是另做菜单**）：  
1 单点超出 3σ；2 连续 9 点同侧；3 连续 6 点趋势；4 连续 14 点交替；5 3 点中 2 点超 2σ；6 5 点中 4 点超 1σ；7 连续 15 点在 1σ 内；8 连续 8 点在 1σ 外同侧。

空 `tests` + `all_applicable`：I 图启用 1–8（比 Minitab 默认「仅 Test 1」更灵敏、误报更高——对话框 help 已写）。尖峰课的**主教学信号仍是规则 1**；阶跃可能触发规则 2。教程必须写清，禁止假装没有 `rule_policy`/`tests`/`Nelson`。

### 5.2 对齐表（Canvas 可少写；落地不得少字段）

| Canvas 字段 | 填什么 | 代码对应 | 落地 |
|-------------|--------|----------|------|
| 变量 | `锡膏高度_um` | role `variables` | `dialog_fill` + detail |
| 阶段列 | 留空 | role `stage` optional | detail 写「为何留空」；**不要**把「钢网更换」做成阶段列（会吃掉阶跃） |
| 移动极差长度 | `2` | `mr_length` | detail |
| σ 方法 | 平均移动极差 | `sigma_method`=`average_moving_range` | detail；禁止写成不存在的菜单名 |
| 特殊原因规则 | 「默认个体图规则」 | **拆成** `rule_policy` + `tests` | 金标建议：`rule_policy` 默认（`all_applicable`）或显式该值；`tests` 留空。若要更接近 Minitab 只报越界，用 `minitab_like` 并在课文说明灵敏度差异 |
| 历史均值 / Sigma | 留空 | **两字段** `historical_center`、`historical_sigma` | 各一条 detail |
| （Canvas 未写） | 0（否） | `use_nelson_estimate` | **必须有 detail**：本课关闭，以免剔除尖峰 MR 后把教学用 UCL 拉窄/拉乱 |

**禁止虚构**：没有「Nelson 规则」独立菜单项；Nelson **estimate** 是 σ 估计开关，Nelson/WE **tests** 是 `tests` 多选。没有 `tutorials.json` 主源。

**建议金标 `dialog_fill`**

```json
{"variables": "锡膏高度_um"}
```

**建议金标 `dialog_fill_detail`（字段用对话框 label，id 可并列注明）**

| field | put | meaning |
|-------|-----|---------|
| 变量 (`variables`) | 锡膏高度_um | 画进 I 图的 Y |
| 阶段列 (`stage`) | 留空 | 同一阶段里看见片 41 阶跃 |
| 移动极差长度 (`mr_length`) | 2 | MR=\|当前−前一点\| |
| σ 方法 (`sigma_method`) | 平均移动极差 | 用平均 MR 估 σ 定 UCL/LCL |
| Nelson estimate (`use_nelson_estimate`) | 0（否） | 本课不剔除过大 MR；打开会改教学限 |
| 规则默认策略 (`rule_policy`) | 全部适用规则 | 空 tests 时启用个体图 1–8；误报高于仅 3σ |
| 特殊原因测试 (`tests`) | 留空（用策略） | 非空会变成 explicit 列表 |
| 历史均值 (`historical_center`) | 留空 | 用本集估 CL |
| 历史 Sigma (`historical_sigma`) | 留空 | 用本集估控制限 |

### 5.3 读输出（对着埋点；解释纪律）

禁止：「过程合格 / 必须停线 / 已证明正态 / 点出 UCL=废品」。  
允许：片 55 相对近期波动不寻常（规则 1 线索）；UCL≠USL；MR 越界≠超规格。

---

## 6. 专用数据设计模式（控制图 / 假设检验 / MSA / 能力 / Pareto / DOE）

生成器 `notes`/`buried_signals` 必须写：**埋了什么、哪一行、图上期望什么**。列名业务化。行数 30–200（控制图约 60–100）。`dataset_id` 不以 `demo_` 开头。

| 族 | 代表 id | 列（少） | 故意埋点 | 期望看见 | 明确不要埋 |
|----|---------|----------|----------|----------|------------|
| I-MR | `imr` | 序, Y, 备注 | 前段稳定；**行 41 均值阶跃**；**行 55 尖峰** | I：后段上移；55 冲/越 UCL。MR：尖峰处变大 | 规格限列、多产线、子组列 |
| Xbar-R | `xbar_r` | 测量, 子组, （可选阶段） | 固定 n=5；某一子组内极差尖峰 **或** 连续子组均值台阶 | Xbar 与 R **分工**：R 先乱则不读 Xbar 限 | 单值流（那是 I-MR 课） |
| P 图 | `p_chart` | 不合格品数, 检验数, （阶段） | 可变 n；中段某批不合格率台阶 | p 限随 n 变宽窄；台阶触发特殊原因 | 缺陷**计数**（应 c/u） |
| 双样本 t | `two_sample_t` | 两列独立样本 **或** 一列+分组（本软件是**两列**） | 两线均值差约 0.8–1.5σ；方差接近或故意不等（对应 Welch） | p 与 CI 同向；**不写因果** | 配对前后（那是 `paired_t`） |
| Gage R&R 交叉 | `gage_rr` | 测量, 零件, 操作员 | AIAG 型 10×3×3 平衡；零件覆盖过程范围；一名操作员系统偏倚 | %GR&R / NDC；**不写量具通过** | 嵌套结构（`nested_gage_rr`） |
| 正态能力 | `capability` | Y（稳定）、规格在对话框 | **先稳定**（无 41/55 那种失控教学信号）；轻微偏心使 Cpk&lt;Cp | Cp/Cpk vs USL/LSL；变换默认正态 | 失控 I-MR 金标表 |
| Pareto | `pareto` | 缺陷类别, （可选计数） | 少数类别占累计大部分；一组长尾 | 排序条+累计线；**不证明因果** | 时间序控制图列 |
| 2 水平析因 | `doe_factorial` | 生成器可不需表；响应分析要 Y+因子列 | 一显著主效应 + 一交互；其余近噪声 | 效应/Pareto of effects；不宣称最优已证明 | 把析因表当控制图 |

**同构极小共享（仅候选，Agent2 白名单锁定；禁止扩成宽表）**

| 候选族 | 可共享条件 | 不要共享 |
|--------|------------|----------|
| `gage_rr` 与 `emp_crossed` | 同为 crossed 三列；**仅当**埋点仍服务两课术语 | 能力规格、嵌套、属性一致性 |
| `xbar_r` / `xbar_s` | 仅当子组大小教学相同且埋点对两图都成立 | 默认分开（R vs S 的 n 惯例不同） |
| `p_chart` / `np_chart` | 同批不合格品+检验数，课文分别解释 p vs np | 与 u/c 混用 |
| 图形类直方图/概率图 | 仅当「同一分布形状课」且无互相干扰的失控点 | 与 I-MR 金标表 |

---

## 7. 抽样 command_id 的 roles/inputs 草稿

来源：`src/ui/analysis_commands.cpp` + `resources/ui/input_option_catalog.json`。Agent2/3 以此为对话框清单，**禁止编菜单**。

| command_id | 菜单 | roles (id / label / optional) | inputs (id / label / 默认或选项集) |
|------------|------|-------------------------------|-------------------------------------|
| **imr** | 控制图 / I-MR 控制图 | `variables` 变量；`stage` 阶段列 opt | 见 §5.1 |
| `xbar_r` | 控制图 / Xbar-R | `variables` 变量；`subgroup` 子组列 opt；`stage` opt | `subgroup_size` 默认 5；`rule_policy`；`tests` kind=`xbar` |
| `p_chart` | 控制图 / P 图 | `defectives` 不合格品数；`inspected` 检验数（列）opt；`stage` opt | `inspected_constant`；`rule_policy`；`tests` kind=`attribute` |
| `u_chart` | 控制图 / U 图 | `defects` 缺陷数；`units` 单位数列；`stage` opt | `rule_policy`；`tests` kind=`attribute` |
| `ewma` | 控制图 / EWMA | `variables` 测量值 | `lambda` 0.2；`limit` 3；`historical_mean`；`historical_sigma`；`rule_policy`；`tests` kind=`ewma`（仅规则 1） |
| `gage_rr` | 质量工具 / Crossed Gage R&R | `measurement`；`part`；`operator` 皆必填 | `lsl` 可选；`usl` 可选 |
| `nested_gage_rr` | 质量工具 / Nested Gage R&R | `measurement`；`part` 部件；`operator` 操作者 | `tolerance` 可选 |
| `emp_crossed` | 质量工具 / EMP Crossed | 同 gage_rr 三角色 | **无**规格输入 |
| `capability` | 质量工具 / 正态过程能力 | `variables` 变量 | `subgroup_size` 默认 1；`lsl`；`usl`；`target` 可选；`transform`=`capability_transform` 默认 `normal` |
| `pareto` | 质量工具 / 柏拉图 | `category` 缺陷类别；`counts` 计数列 opt | `other_threshold` 可选 % |
| `one_sample_t` | 统计 / 单样本 t | `variables` 测量值 | `hypothesis_mean`；`confidence` 95；`alternative`=`hypothesis_alternative` 默认 `two_sided` |
| `two_sample_t` | 统计 / 双样本 t | `variables` **两列独立样本** multi 必填恰好 2 | `variance`=`two_sample_t_method` 默认 `welch`；`confidence` 95；`alternative` |
| `paired_t` | 统计 / 配对 t | `variables` 配对两列 | `confidence`；`alternative` |
| `one_way_anova` | 统计 / 单因素 ANOVA | `response`；`factor` | **无 inputs** |
| `chi_square` | 统计 / 列联表卡方 | `row_category`；`column_category` | 无 |
| `histogram` | 图形 / 直方图 | `variables` 可多选；`by` 分组 opt | `bins` 可选 |
| `doe_factorial` | 统计 / 2 水平析因设计生成 | `response` opt；`factor_columns` 多选 opt | `factors,low,high,fraction_p,generators,centers,blocks,seed,x_factor,y_factor,hold`；**`requires_data=false`** |

`c_chart` 注意：代码把 `stage` 放在 **inputs** 而不是 roles（与 p/u 不一致）。教程按代码字段走，不要发明「阶段列角色」。

`formula_reference` / 无数据命令：`dataset_id` 空；步骤标明菜单可能不可用。

---

## 8. 按菜单包抽查权威源（用途 / 不能做什么 / 误用 / 埋点）

下列 URL 均于 **2026-09-03** 用 WebSearch/WebFetch 核对。修正旧 notes 中已失效路径。

### 8.1 控制图

| 用途 | 不能做什么 | 典型误用 | 建议埋点 |
|------|------------|----------|----------|
| 监视过程是否可预测；揪特殊原因（NIST I 图：n=1 用 MR；UCL= x̄±3MR̄/1.128） | 不回答是否符合规格；有合理子组时不应替代 Xbar-R/S（Minitab I-MR overview） | UCL=USL；打乱时间序；填历史限盖住教学阶跃；全规则开启却当成「已证明失控」 | I-MR：阶跃+尖峰分行；P：可变 n + 比例台阶 |

**URL**

- NIST Individuals：https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc322.htm — **已打开**（公式与 flowrate 例）  
- Minitab I-MR Overview：https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/i-mr-chart/before-you-start/overview/ — **已打开**（无子组用 I-MR；有子组改 Xbar）  
- Minitab 特殊原因 tests：https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/basics/using-tests-for-special-causes/ — **已打开**（Tests 1–8；多规则↑灵敏度也↑误报）  
- NIST 比例图：https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc332.htm（旧 notes 已引；本轮未再全文打开，不替代 pmc322）

### 8.2 假设检验

| 用途 | 不能做什么 | 典型误用 | 建议埋点 |
|------|------------|----------|----------|
| 双样本 t：两独立过程均值差（NIST 给出合并方差与 Welch） | 不能当配对；不能推因果；不能替代 ANOVA（≥3 组） | 配对当独立；只报 p 不报 CI；把显著写成「产线必须停」 | 两列均值差；方差结构对应 Welch/pooled 选项 |

**URL**

- NIST 两过程均值：https://www.itl.nist.gov/div898/handbook/prc/section3/prc31.htm — **已打开**（含 Welch-Satterthwaite）  
- 旧 notes 把 two-sample 写成 `prc22.htm`：**那是单样本均值**。单样本应引 https://www.itl.nist.gov/div898/handbook/prc/section2/prc22.htm — **已打开**  
- Montgomery DAE：https://www.wiley.com/en-us/Design+and+Analysis+of+Experiments%2C+10th+Edition-p-9781119714648（书目页；非公式源）

### 8.3 MSA

| 用途 | 不能做什么 | 典型误用 | 建议埋点 |
|------|------------|----------|----------|
| 交叉：每操作员测每个零件，分解重复性/再现性（Minitab：10×3×3 例） | 嵌套结构应 Nested；不能写「量具通过/产品合格」 | 零件未覆盖过程范围；非随机顺序；%GR&R 阈值当法律 | 平衡交叉 + 一名操作员偏倚；零件间差异可见 |

**URL**

- Minitab Crossed Gage R&R Overview：https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/crossed-gage-r-r-study/before-you-start/overview/ — **已打开**（旧路径 `gage-r-r-study-crossed` **404**，已换 `crossed-gage-r-r-study`）  
- NIST 量具变异（嵌套时间分量）：https://www.itl.nist.gov/div898/handbook/mpc/section4/mpc44.htm — **已打开**  
- 旧 notes `pmc/section4/pmc4.htm` 现为 **时间序列导论**，**不可再当 Gage 源**  
- AIAG MSA 4th：https://www.aiag.org/quality/msa — 本机 **404**；以印刷手册 + Minitab「AIAG 准则」页为准：https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/supporting-topics/gage-r-r-and-wheeler-s-emp-studies/is-my-measurement-system-acceptable/  
- EMP vs AIAG 分类不同：教程禁止把 %GR&R&lt;10% 写成「已通过」

### 8.4 过程能力

| 用途 | 不能做什么 | 典型误用 | 建议埋点 |
|------|------------|----------|----------|
| 稳定过程相对 USL/LSL 的 Cp/Cpk（NIST：约 50 点起；Cpk 区间宜更大 n） | 失控数据指数无预测意义；不写「过程合格」 | 用 I-MR 教学失控集算 Cpk；把控制限当规格 | 稳定 + 轻微偏心；规格只在对话框 |

**URL**

- NIST Process Capability：https://www.itl.nist.gov/div898/handbook/pmc/section1/pmc16.htm — **已打开**（Cp/Cpk 定义；稳定+正态假设）  
- Minitab Normal Capability Overview：https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/before-you-start/overview/ — **已打开**（先 Sixpack 查稳定/正态）

### 8.5 Pareto / 质量图

| 用途 | 不能做什么 | 典型误用 | 建议埋点 |
|------|------------|----------|----------|
| 按频次排序，聚焦少数类别 | 不证明因果；分类一变排序就变 | 当控制图用；类别定义漂移 | 少数主导 + 长尾 |

**URL**

- Minitab Pareto Overview：https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/pareto-chart/before-you-start/overview/ — **已打开**  
- 旧 notes `handbook/pri/section3/pareto.htm` 本轮 **重定向到 ITL 首页**，**失效**。DOE 效应 Pareto 可用 Dataplot 说明，质量柏拉图以 Minitab 为准  
- ASQ Pareto：https://asq.org/quality-resources/pareto — Cloudflare **拦截**，本轮不引用为可打开源

### 8.6 DOE

| 用途 | 不能做什么 | 典型误用 | 建议埋点 |
|------|------------|----------|----------|
| 2^k 全因子：每因子两水平所有组合（k≥5 时 NIST 不推荐全因子） | 生成设计 ≠ 已证明最优；`doe_factorial` 可无表 | 把随机噪声交互当真实效应 | 一主效应 + 一交互 |

**URL**

- NIST 全因子：https://itl.nist.gov/div898/handbook/pri/section3/pri333.htm — **已打开**  
- NIST 选设计总目：https://itl.nist.gov/div898/handbook/pri/section3/pri3.htm

---

## 9. 现网衔接（给 Agent2；本岗未改代码）

| 模块 | 现状 | 本 Goal 扩什么 |
|------|------|----------------|
| `learning_types.h` | 无 glossary/7+ | 扩字段 |
| `learning_tutorial_catalog.cpp` | `parse_dialog_fill` = object→QMap | 新列缺省为空；**禁止**把 detail 数组塞进 `dialog_fill` |
| `learning_dataset_store` | `kExpectedCatalogVersion = learning-center-v1`；表名 `demo_{id}` | 双写 v2 |
| `LearningCenterPage` | `build_entry_html`；`entries_`；工作表 `demo_%1` | 分节折叠 + 7+ 展示；**保留导入信号** |
| `build_learning_center_db.py` | `META_VERSION = learning-center-v1`；`GENERATORS` + `build_tutorial_row()` | 金标覆盖层 + 新列；删除 10 共享生成器 |
| 测试 | `listsTenDatasets` 等 | 张数 = 新专用集数量 |

**非目标**：不改 `algorithm_help.json` 公式语义；不建第二套 `AlgorithmHelpDialog`；不在学习中心内嵌分析向导。

---

## 10. 金标 7+ 草稿（与 Canvas 一致，供生成器）

`skill_mission`：用 I-MR 在单值时间序上找出阶跃与尖峰，并口头区分 UCL 与 USL。

`prereq_quiz` / `self_explain` / `fade_levels` / `retrieval_quiz` / `misconceptions`：直接采用 Canvas 常量 `PREREQ_QUIZ`、`SELF_EXPLAIN`、`FADING`、`RETRIEVAL`、`MISCONCEPTIONS`（实现时补上 Nelson 字段的一条 self_explain：「为什么本课关闭 Nelson estimate？」）。

独立练习表：同构另一 `dataset_id`（例如 `imr_spi_spike_b`，尖峰行号不同），**仍不以 `demo_` 开头**。

---

## 11. Agent2 必须锁的事项（本岗不重问 §0）

1. Wave-0 = schema + 教学 UI + **仅** `imr`/`imr_spi_shift` 金标；随后按手册 §7 连续 Wave-1…5。  
2. `dialog_fill` 只保留角色 object；detail 覆盖 §5.2 全部 9 个对话框字段。  
3. 同构白名单写进 mapping；默认专用。  
4. 旧 `smt_paste_height` 等 10 id：删除重建，测试期望全改。  
5. glossary 课不新增 command_id。

---

## 12. Agent1 DoD / 风险 / go-no-go

**DoD（手册 Agent1）**

- [x] 金标 `imr`：字段清单与 Canvas 参数表对齐，无虚构菜单（Canvas 少写的 Nelson / 策略 / 双历史限已补进落地表）  
- [x] 至少 1 份专用数据设计模式表（§6：控制图 / 假设检验 / MSA / 能力 / Pareto / DOE）  
- [x] 写明旧共享大表被覆盖的**教学理由**（§3）  
- [x] **未改**产品代码（`src/`、`tools/*.py` 除 docs、CMake、sqlite、JSON 资源均未动）

**风险（一行）**：全集 184 条的 `dialog_fill_detail`/专用集工作量远大于金标；若 Agent3 在 Wave-0 后截断，将直接违反 Q2——计划必须把 Wave 锁成可勾选 id 表而不能停在 imr。

**go/no-go**：**GO** — 可进 Agent2（详细计划）。金标字段与 Canvas 缺口已闭合；权威 URL 已标注 accessed 2026-09-03 与失效更正；schema/H1–H11 足够写 Wave plan。不阻塞项：AIAG 官网 MSA 深链 404、部分出版社全文超时（已有 DOI + 已打开摘要页）。
