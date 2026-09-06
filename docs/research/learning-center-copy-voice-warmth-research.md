# 学习中心文案口吻升温（并肩版）— Agent1 调研报告

> **Goal 手册**：[`goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md`](goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md)  
> **口吻标尺（并肩版）**：`learning-center-copy-voice-target.canvas.tsx`  
> **金标深度硬点**：`learning-center-tutorial-example.canvas.tsx`  
> **访问 / 抽检日期**：2026-09-05（UTC+8）  
> **范围声明**：本报告只调研与归类；**未改** overlays / 生成器 / sqlite / C++ 产品代码。  
> **现网事实**：工作树里已有一轮「去电报/去禁令」的 `warmth_wave*.py` 产物；本 Goal **不以表面清黑名单为完成**，要对照 Canvas 并肩标尺继续升温到「同事带看」。

---

## 1. 网研结论（写法原则 → 本 Goal 映射）

读者是车间/质量新人、统计基础薄弱。可靠来源一致指向：**短句但细节够、先说读者要什么、警告带原因、邀请而非训诫、术语首次白话**。这与 Canvas「并肩版」同向；也明确反对把「短」做成电报断句（手册 §7.21），以及用「说教壳」代替真正并肩。

| # | Primary URL | 来源类型 | 对本 Goal 的可用原则 | 访问日期 |
|---|-------------|---------|----------------------|----------|
| 1 | https://developers.google.com/style/tone | Google 开发者文档 · Voice and tone | 像懂行的朋友；友好但不说教、不 pushy；忌「simply/easy」居高临下；忌电报式 choppy 断句 | 2026-09-05 |
| 2 | https://digital.gov/guides/plain-language/writing | Digital.gov · Plain language writing | 主动语态、现在时、短段；写清「谁做什么」；删虚词但保留决策所需细节 | 2026-09-05 |
| 3 | https://www.plainlanguage.gov/guidelines/ | 美国联邦 Plain Language Guidelines | 为特定受众写作；可测可读；清晰是义务不是风格偏好 | 2026-09-05 |
| 4 | https://www.archives.gov/federal-register/write/legal-docs/clear-writing.html | NARA · Principles of Clear Writing | 直接对读者；短句；动作动词；警告与动作同屏（对应「通常还要…」带原因） | 2026-09-05 |
| 5 | https://ap.osu.edu/technical-writing-style-guide | Ohio State · Technical writing style | 会话式友好但不随便；第二人称；约 15–20 词/句；避免「please/simple」假客气 | 2026-09-05 |
| 6 | https://learn.microsoft.com/en-us/windows/win32/uxguide/text-style-tone | Microsoft UX · Style and tone | 准确、鼓励、客观；像在读者肩后讲解；紧但不尖刻不友善 | 2026-09-05 |
| 7 | https://www.baklib.com/blog/manufacturing-work-instructions | 生产作业指导（MWI）实践 | 标题说清任务；背景一句；一步一动作；可视化定位；新人可独立执行 | 2026-09-05 |
| 8 | https://tsight.io/articles/6975716 | 车间作业指导 · 大白话原则 | 大白话、别拽词；术语要解释；目的是看懂不是炫技 | 2026-09-05 |
| 9 | https://www.telerik.com/documentation/style-guide/tone-and-voice | Telerik tone guide | 尊重、自然；不为讨好而油滑；短段 2–4 句；客观不 pedantic | 2026-09-05 |
| 10 | https://mintlify.com/docs/guides/style-and-tone | Mintlify style & tone | 语气匹配任务；删 filler；像同事解释，不像法律文书；新手页可更暖 | 2026-09-05 |

**原则压缩（给 Agent2/3 改稿用）**

| 要 | 不要 |
|----|------|
| 2～4 句说完一件事；能指列名/子组/行号/菜单 | `子组 n=5。子组12起批均值上移。` 式电报 |
| 「不妨 / 常见读法 / 多半 / 通常还要…」+ **本课现场细节** | 「你的任务是 / 你应该能 / 这一课只练 / 禁止过程合格。」 |
| 红线写原因：不等于放行样板；停线看规程 | 禁令牌收束；也**不要**用「目标不是背菜单」万能壳冒充并肩 |
| 字段 meaning 写本课列在现场是什么 | 全课复制「这一项决定图上或表上对应哪一列」；跨课复制同一无表壳 |
| 稳词：波动主要落在批内/批间 | 「抖主要…」歧义断句 |

---

## 2. 「补充语气、不推倒骨架」边界

| 保留（前两轮基线，禁止推倒） | 本轮只动 |
|------------------------------|----------|
| `learning-center-v2`；专用表 / 同构白名单；`dataset_id` 不以 `demo_` 开头 | 学员可见句的**语气与温度**（含已「半升温」但仍套壳的课） |
| 0–6 分节 + 折叠 UI；导入 → WorksheetRegistry | `used_for` / `not_for` / `scenario` / glossary / `dialog_fill_detail` / 埋点 / 读输出 / 误用 |
| 练习 JSON 形状：`why` / `hint` / `model_answer` | 7A / 7C / 7D / 7E 题干与揭晓并肩化 |
| 图形名实：`title`/`menu_path` = 菜单中文 | 金标 `imr`：**允许 0–6 明显加长**，片号 41/55、UCL≠USL、参数表硬点不删 |
| **7B UI 继续隐藏**；scrub「自解释」学员字样 | 生成器模板去掉「软壳并肩」与旧电报/说教，避免重建库再灌 |
| `LearningCenterPage` 导航树；`algorithm_help.json` 公式语义 | verify：长度门 + 黑名单 + **软壳短语门**（Agent4） |

**明确不改**：第二套帮助系统、升 v3、重做数据集、恢复 7B、为润色改公式或菜单 id。

---

## 3. 现网 overlays 抽检（对照并肩标尺 · 2026-09-05）

**抽检方法**：对照 Canvas 并肩版；手册 §2 点名课必纳；另跨控制图 / 能力 / MSA / 推断 / 图形 / ANOVA / 回归 / DOE / 可靠性。  
**全集粗扫**：184 课 `tools/learning_data/tutorial_overlays/*.json`。

### 3.0 与上一版调研的关键差异（不许装没看见）

| 旧调研（2026-09-03）所见 | **今日现网** |
|--------------------------|--------------|
| 大量电报：`子组 n=5。子组12起…` | 手册点名课（如 `between_within_capability`、`imr`、`xbar_r`）已接近 Canvas 并肩 |
| `not_for` 含「禁止过程合格」约 38 课 | **overlays 学员可见区黑名单 0 命中**（`你的任务是` / `这一课只练` / `禁止过程合格` / `抖主要` / `你应该能`） |
| `dialog_fill_detail` 套壳「这一项决定图上…」约 114 课 | **该旧壳句 0 命中**；但出现**跨课同句 meaning**（无练习表壳等） |
| 「不妨 / 常见读法」0 课 | 约 **111** 课有并肩邀请词；但多数仍是**可替换菜单名的软壳** |

**结论**：旧「电报/禁令」主战场已大半清掉；本 Goal 主战场升级为——**去掉 `warmth_*` 灌出的万能软壳，换成带本课现场细节的真正并肩文**。

| 信号（今日） | 粗计量 | 含义 |
|--------------|--------|------|
| 软壳 A「这条练习线是为…准备的」 | **46** 课 | 场景无产线细节，只换菜单名 |
| 软壳 B「目标不是背菜单」 | **50** 课 | 说教变体：仍在规定「本课目标」 |
| 软壳 C「更像一张对照纸条」 | **45** 课 | used_for 万能壳 |
| 软壳 D「不妨先当一次助手」 | **42** 课 | 邀请词有，现场细节无 |
| 软壳 E「还是那张本课练习表」 | **49** 课 | 半并肩开头，仍缺埋点故事 |
| 软壳 F「导入本课练习表后打开」 | **41** 课 | 流程壳，非车间场景 |
| 粗分桶：真并肩 / 软壳偏短 / 边界 | **~5 / ~133 / ~46** | 绝大多数课未达 Canvas 金样 |
| 学员可见黑名单（手册 §2） | **0** | 防回归仍须进 verify |
| 跨课完全相同 `meaning`（≥8 课） | **8** 组 | 新形态「字段壳」 |

### 3.1 反面教材 ≥15 条（路径 + 字段 + 摘录 + 类别）

| # | 路径 | 字段 | 摘录 | 类别 |
|---|------|------|------|------|
| 1 | `tutorial_overlays/arima.json` | `scenario` | 「这条练习线是为「ARIMA / Best ARIMA」准备的。埋点见第 5 节。不妨先运行一次…」 | **软壳**（换标题即复用） |
| 2 | 同上 | `used_for` | 「用「ARIMA…」把这一类问题摊开看一眼。结论先别写成放行样板。…先弄清菜单字段…」 | **软壳** |
| 3 | `tutorial_overlays/anom.json` | `used_for` | 「当学员第一次点「ANOM…」时，目标不是背菜单，而是认出练习表里的变化…」 | **软壳·说教变体** |
| 4 | `tutorial_overlays/acceptance_sampling.json` | `scenario` | 「不妨先当一次助手：打开「属性一次抽样」，按第 4 节填写，去找第 5 节写的线索…」 | **软壳**（有「不妨」仍无现场） |
| 5 | `tutorial_overlays/batch_capability.json` | `used_for`+`scenario` | 「目标不是背菜单…」/「这条练习线是为「批次过程能力」准备的…」 | **软壳** |
| 6 | `tutorial_overlays/analyze_variability.json` | `scenario` | 「这条练习线是为「Analyze Variability」准备的。埋点见第 5 节…」 | **软壳**（英文菜单硬塞） |
| 7 | `tutorial_overlays/best_subsets_regression.json` | `used_for` | 「「最优子集回归」更像一张对照纸条：告诉你该看哪一列…」 | **软壳** |
| 8 | `tutorial_overlays/cluster_variables.json` | `scenario` | 「还是那张本课练习表。菜单走「变量聚类」。输出里优先核对第 5 节…」 | **软壳** |
| 9 | `tutorial_overlays/adf_test.json` | `scenario` | 「不妨先当一次助手：打开「ADF 单位根检验」，按第 4 节填写…」 | **软壳** |
| 10 | `tutorial_overlays/cart_tree.json` | `scenario` | 「不妨先当一次助手：打开「CART 单树」…」 | **软壳** |
| 11 | `tutorial_overlays/binary_doe_probit.json` | `used_for` | 「当学员第一次点「二值 DOE Probit/Gompit」时，目标不是背菜单…」 | **软壳·说教变体** |
| 12 | `tutorial_overlays/area_plot.json` 等图形族 | `dialog_fill_detail` | 多课共用「面板上限。在对话框里把「6」填进「分面最大面板数」。」 | **跨课模板壳** |
| 13 | 无练习表课族（如 `analyze_definitive_screening`） | `dialog_fill_detail.meaning` | 「「响应」在软件对话框里有这一项。本课没有预填练习表…不要虚构没有的框。」（≥18 课同文） | **跨课模板壳** |
| 14 | `tools/learning_data/dataset_mapping.json` | `notes`（内部） | 多处仍写「禁止过程合格。」 | **内部残留**（勿回灌学员区；Agent3/5 清理或改写） |
| 15 | `tools/learning_data/id_metadata.json` | `interpretation_limits` | 「禁止过程合格、已证明因果。」 | **内部残留** |
| 16 | `tools/learning_data/warmth_wave234_overlays.py` | 生成逻辑 | 仍用软壳句式批量灌 `scenario`/`used_for` | **源头仍灌软壳** |
| 17 | `tools/learning_data/copy_depth.py` | `studentize_text` | 把旧禁令替换成并肩词——**必要但不够**；不能替代本课故事 | **半成品源头** |
| 18 | `tutorial_overlays/capability.json`（边界） | 全文 | 有部分并肩词，但场景/适用仍偏短、细节不如 Canvas 金样 | **边界·需加长** |
| 19 | 手册 §2 历史电报样（已修好，作防回归） | — | 「子组 n=5。子组12起批均值上移。」/「禁止过程合格。」 | **防回归教材**（verify 必须挡） |

### 3.2 正面样例 ≥3 条（已合格或接近 Canvas）

| # | 来源 | 字段 | 为何算正面 |
|---|------|------|------------|
| 1 | `tutorial_overlays/between_within_capability.json` | `scenario` / `used_for` | 与 Canvas `COLLAB` 几乎同文：镀膜线、子组 12、波动主要落在…、不妨先打开… |
| 2 | `tutorial_overlays/imr.json` | 0–6 全文 | 片 41/55、UCL≠USL、9 字段参数表仍可指认；语气已并肩加长 |
| 3 | `tutorial_overlays/xbar_r.json` | `scenario` / `used_for` | 子组 12 极差 + 子组 20 台阶；「波动主要落在…」；邀请先看 R 再看 Xbar |
| 4（加分） | Canvas `COLLAB.output` / `field_meaning` | 标尺 | 「常见读法是…通常还要对照规程」；字段写清厚度 vs 子组——批量改稿仍按这一列 |

**现网真并肩桶（粗分，仅约 5 课）**：`between_within_capability`、`imr`、`xbar_r`、`c_chart`、`ewma`。其余多数为软壳或边界，**不得**因黑名单已清而宣称 Goal 完成。

---

## 4. 生成器灌水风险结论

| 文件 | 仍在灌什么 | 证据 | Agent3 优先级 |
|------|------------|------|----------------|
| `tools/learning_data/warmth_wave234_overlays.py` | **软壳并肩**（练习线/对照纸条/当一次助手/目标不是背菜单） | 与 overlays 中 40–50 课命中同构 | **必改源头**（否则重建回灌） |
| `tools/learning_data/warmth_wave1_overlays.py` | 控制图包有真并肩样板，但仍含替换式 scrub | Wave1 部分课已达标（如 xbar_r）；需核对全包是否都达 §4.2 | **核对 + 补洞** |
| `tools/learning_data/copy_depth.py` | 旧壳→空/`studentize` 替换；`_role_aware_meaning` 半通用 | 无表课 meaning 跨课同文 | **加深本课专义**，勿只靠全局 replace |
| `tools/learning_data/wave*_content.py` | 已 `apply_warmth`；若 warmth 仍出软壳则连带污染 | wave2/3/4 import `apply_warmth` | 与 warmth 同源治理 |
| `glossary_bank.py` | 黑名单未命中 | Grep 干净 | 口吻升温时抽检白话；非主凶 |
| `dataset_mapping.json` / `id_metadata.json` | 内部仍「禁止过程合格」 | Grep 命中 | **勿写回 overlays**；可选 scrub 内部 notes |
| overlays 本身 | ~133 软壳课 | 今日粗分桶 | 改源头后按 Wave 重写；**禁止只手改 sqlite** |

**Grep / 扫描六词（学员 overlays）**

| 模式 | overlays | 生成器 / 元数据 |
|------|----------|-----------------|
| 你的任务是 | 无 | warmth/copy_depth 有**替换表**（防回归） |
| 这一课只练 / 本课只练 | 无 | 替换表仍在 |
| 禁止过程合格 | 无（学员区） | `dataset_mapping` / `id_metadata` 仍有 |
| 抖主要 | 无 | 替换表仍在 |
| 这一项决定图上或表上对应哪一列 | 无 | copy_depth 仍剥壳 |
| 软壳（练习线是为 / 目标不是背菜单 / 对照纸条 / 当一次助手） | **大量** | **warmth_wave234 主源** |

---

## 5. 对 Agent2 的直接建议（调研侧，非改码）

1. Wave-0：以现网 `imr` 为金标**验收样板**（已接近达标）——核对硬点清单可勾选；若有短板只定点加长，**禁止推倒**。  
2. 计划把 `warmth_wave234_overlays.py`（及 wave1 补洞）列为与 overlays **同级必改**；软壳短语必须进 verify 黑名单/频次门。  
3. verify 至少：旧黑名单 + `used_for`/`scenario`/`not_for` 汉字长度下限 + **软壳短语**（练习线是为 / 目标不是背菜单 / 更像一张对照纸条 / 不妨先当一次助手）+ 跨课 `dialog_fill_detail.meaning` 完全相同占比上限 + 金标硬点。  
4. 全集 184 不得缩成「黑名单已清」或「只改金标」；内部 Wave 只是队列。  
5. `dataset_mapping`/`id_metadata` 的「禁止过程合格」若会进入学员可见路径则必改；若纯内部 notes，计划里写明边界，仍建议改成并肩红线以免误用。

---

## 6. Agent1 DoD 勾选

- [x] research md 含 **Primary URL 表 + 访问日期（2026-09-05 UTC+8）**（≥5，本表 10 条）  
- [x] **反面教材 ≥15 条**（上表 19 条；含软壳新主战场 + 内部残留 + 防回归电报）  
- [x] **正面样例 ≥3 条**（现网 between_within / imr / xbar_r + Canvas）  
- [x] 写清「**补充语气、不推倒骨架**」边界  
- [x] 生成器灌水风险结论（`warmth_*` 软壳为主凶；旧电报/禁令 overlays 已清）  
- [x] **禁止**改产品代码 / overlays / py 生成器 / sqlite / C++（本 Agent 只更新本文件）

**风险（一行）**：若把「黑名单 0 命中」误当成 Goal 完成，会把约 130+ 课软壳万能文交付给学员；必须改 warmth 源头并加软壳 verify。

**go/no-go**：**GO** — 可进入 Agent2 详细计划（Wave 锁表 184 不漏 + 字段检查单 + verify 口吻/软壳断言 + 模板源头必改清单）。

---

## 7. 本阶段交付文件列表

| 文件 | 动作 |
|------|------|
| `docs/research/learning-center-copy-voice-warmth-research.md` | **重写**（对齐 2026-09-05 现网；旧 09-03 电报统计已过时） |

未改动任何产品代码或学习中心内容层文件。
