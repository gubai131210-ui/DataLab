# 学习中心文案口吻升温（并肩版）— Agent1 调研报告

> **Goal 手册**：[`goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md`](goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md)  
> **口吻标尺（并肩版）**：`learning-center-copy-voice-target.canvas.tsx`  
> **金标深度硬点**：`learning-center-tutorial-example.canvas.tsx`  
> **访问 / 抽检日期**：2026-09-03（UTC+8）  
> **范围声明**：本报告只调研与归类；**未改** overlays / 生成器 / sqlite / C++ 产品代码。

---

## 1. 网研结论（写法原则 → 本 Goal 映射）

读者是车间/质量新人、统计基础薄弱。可靠来源一致指向：**短句但细节够、先说读者要什么、警告带原因、邀请而非训诫、术语首次白话**。这与 Canvas「并肩版」完全同向；也明确反对把「短」做成电报断句（手册 §7.21）。

| # | Primary URL | 来源类型 | 对本 Goal 的可用原则 | 访问日期 |
|---|-------------|---------|----------------------|----------|
| 1 | https://analysisfunction.civilservice.gov.uk/policy-store/writing-about-data/ | 英政府分析职能 · 写数据 | 假定读者无技术背景；句长约 ≤25 词；一段一事；**透明写清局限/不确定**（对应「通常还要对照规程」而非纯禁令） | 2026-09-03 |
| 2 | https://analysisfunction.civilservice.gov.uk/policy-store/writing-about-statistics-2/ | 同上 · 写统计 | 白话 + 术语首次解释；一致用词；复杂概念用 plain English（对应 glossary 白话 + 稳词「波动主要落在…」） | 2026-09-03 |
| 3 | https://www.cdc.gov/nceh/clearwriting/docs/clear-writing-guide-508.pdf | CDC Clear Writing Guide | 会话式、可用 you/we；主动语态；约 ≤20 词/句；**写给读者需要知道的，不是作者想炫技的** | 2026-09-03 |
| 4 | https://www.cdc.gov/health-literacy/php/develop-materials/plain-language.html | CDC Plain Language | 最重要信息先行；术语当场解释；删多余词但保留读者决策所需细节 | 2026-09-03 |
| 5 | https://www.plainlanguage.gov/guidelines/（Digital.gov 现转载） | 美国联邦 Plain Language | 为特定受众写作；可测可读；内容清晰是义务不是风格偏好 | 2026-09-03 |
| 6 | https://datafield.dev/technical-writing/part-04/chapter-22/ | 技术写作 · 指令与程序 | **警告放在动作前**；写出危害是什么、为什么、该怎么做——禁止只贴「禁止…」标签 | 2026-09-03 |
| 7 | https://gethaiku.ai/blog/clear-work-instructions | 车间作业指导实践 | 一步一事、动词可观察、决策 if/then 显式；埋藏警告=缺陷 | 2026-09-03 |
| 8 | https://pressbooks.bccampus.ca/technicalwriting2ed/chapter/readercentred/ | Reader-centred writing | **成人对成人**：不居高临下训人；合作语气；少用纯否定堆砌 | 2026-09-03 |
| 9 | https://hop-online.net/how-to-write-with-empathy-for-educational-content/ | 教学文案同理心 | 鼓励/邀请取代命令；纠错中性；专业温度（非抒情散文） | 2026-09-03 |
| 10 | https://journals.sagepub.com/doi/10.1177/0963721420922183 | Paas & van Merriënboer · CLT | 新手靠 worked example；细节要够才减外在负荷——**短≠省到尖** | 2026-09-03 |

**原则压缩（给 Agent2/3 改稿用）**

| 要 | 不要 |
|----|------|
| 2～4 句说完一件事；能指列名/子组/行号/菜单 | `子组 n=5。子组12起批均值上移。` 式电报 |
| 「不妨 / 常见读法 / 多半 / 通常还要…」 | 「你的任务是 / 你应该能 / 这一课只练 / 禁止过程合格。」 |
| 红线写原因：不等于放行样板；停线看规程 | 禁令牌收束、无理由「禁止…」 |
| 字段 meaning 写本课现场含义 | 全课复制「这一项决定图上或表上对应哪一列」 |
| 稳词：波动主要落在批内/批间 | 「抖主要…」歧义断句 |

---

## 2. 「补充语气、不推倒骨架」边界

| 保留（前两轮基线，禁止推倒） | 本轮只动 |
|------------------------------|----------|
| `learning-center-v2`；专用表 / 同构白名单；`dataset_id` 不以 `demo_` 开头 | 学员可见句的**语气与温度** |
| 0–6 分节 + 折叠 UI；导入 → WorksheetRegistry | `used_for` / `not_for` / `scenario` / glossary / `dialog_fill_detail` / 埋点 / 读输出 / 误用 |
| 练习 JSON 形状：`why` / `hint` / `model_answer` | 7A / 7C / 7D / 7E 题干与揭晓并肩化 |
| 图形名实：`title`/`menu_path` = 菜单中文 | 金标 `imr`：**允许 0–6 明显加长**，片号 41/55、UCL≠USL、参数表硬点不删 |
| **7B UI 继续隐藏**；scrub「自解释」学员字样 | 生成器模板去电报/说教壳，避免重建库再灌回去 |
| `LearningCenterPage` 导航树；`algorithm_help.json` 公式语义 | verify 增加口吻黑名单与长度门（Agent4） |

**明确不改**：第二套帮助系统、升 v3、重做数据集、恢复 7B、为润色改公式或菜单 id。

---

## 3. 现网 overlays 抽检（对照并肩标尺）

**抽检方法**：对照 Canvas 并肩版；手册 §2 点名课必纳；另跨控制图 / 能力 / MSA / 推断 / 图形 / ANOVA / 回归。  
**全集粗扫（2026-09-03）**：184 课 overlays。

| 信号 | 粗计量 | 含义 |
|------|--------|------|
| `used_for`/`scenario` 汉字 ≤28（电报候选） | **167** 条字段命中（绝大多数课） | 事实堆叠、缺邀请与「为什么」 |
| `not_for` 含「禁止过程合格」 | **38** 课 | 说教禁令壳仍在红线位 |
| `dialog_fill_detail` 含模板壳句 | **114** 课 | 字段含义套壳 |
| 「本课只练」 | **2** 课（`imr`、`c_chart`） | 说教收束 |
| 「不妨 / 常见读法 / 通常还要」 | **0** 课 | 并肩邀请语尚未落地 |
| 「你的任务是 / 你应该能 / 抖主要」 | overlays 内 **未检出**（好消息；Canvas 说教版勿写回） | 上一轮禁令清理有效，但电报/壳仍大 |

### 3.1 反面教材 ≥15 条（路径 + 字段 + 摘录 + 类别）

| # | 路径 | 字段 | 摘录 | 类别 |
|---|------|------|------|------|
| 1 | `tutorial_overlays/between_within_capability.json` | `used_for` | 「分子组估计组间与组内变差的能力。」 | **电报**（手册 §2.1 点名） |
| 2 | 同上 | `scenario` | 「子组 n=5。子组12起批均值上移。」 | **电报** |
| 3 | 同上 | `not_for` | 「…禁止过程合格。」 | **说教**（禁令无原因） |
| 4 | 同上 | `dialog_fill_detail[*].meaning` | 「…这一项决定图上或表上对应哪一列。」（多字段同壳） | **模板壳** |
| 5 | `tutorial_overlays/xbar_r.json` | `used_for` | 「固定子组 n=5：Xbar 看位置，R 看组内极差。」 | **电报** |
| 6 | 同上 | `scenario` | 「子组12极差尖峰；子组20均值台阶。」 | **电报** |
| 7 | `tutorial_overlays/capability.json` | `scenario` + `not_for` | 「稳定厚度略偏高。LSL=95…」/「…禁止过程合格。」 | **电报 + 说教** |
| 8 | `tutorial_overlays/p_chart.json` | `used_for` / `scenario` | 「可变检验数下的不合格品率。限宽随 n 变化。」/「…批22起不合格率台阶。」 | **电报** |
| 9 | `tutorial_overlays/c_chart.json` | `used_for` | 「…本课只练「固定单位 + 缺陷台阶」读特殊原因线索。」 | **说教**（道德收束） |
| 10 | `tutorial_overlays/imr.json` | `used_for` | 「…本课只练「对着埋点读特殊原因线索」…」 | **说教**（深度够，口吻仍偏考试官；金标硬点须保留） |
| 11 | `tutorial_overlays/two_sample_t.json` | `used_for` / `scenario` | 「两独立列位置差（本软件两列）。」/「A/B 线厚度。导入 demo_…」 | **电报** |
| 12 | `tutorial_overlays/pareto.json` | `not_for` / click | 「…禁止过程合格。」 | **说教** |
| 13 | `tutorial_overlays/boxplot.json` | `used_for` / `not_for` | 「练习菜单「箱线图」：导入…不要把它当成放行…」/「…禁止过程合格 / 已证明正态 / 必须停线。」 | **模板壳 + 说教**（图形波通用壳） |
| 14 | `tutorial_overlays/one_way_anova.json` | `used_for` / `scenario` | 「单因子多组均值。」/「三腔位。导入 demo_…」 | **电报** |
| 15 | `tutorial_overlays/normality_test.json` | `not_for` | 「禁止写成已证明正态。」 | **说教**（应改「不等于已经证明正态」类共享习惯） |
| 16 | `tutorial_overlays/laney_p_chart.json` | `used_for` / `scenario` | 「过离散时加宽 P 限…」/「批间额外波动。对照普通 P…」 | **电报** |
| 17 | `tutorial_overlays/regression.json` | `used_for` / `scenario` | 「第一列响应、其余预测。」/「强度~温度。导入 demo_…」 | **电报** |
| 18 | `tutorial_overlays/gage_rr.json` | `dialog_fill_detail` + click | meaning 仍套「这一项决定…」；「禁止写成量具通过。」 | **模板壳 + 说教残留**（scenario 本身相对有现场感，可作半成品） |
| 19 | `tutorial_overlays/violin_plot.json`（及同族图形课） | `not_for` / `expect` / `hint` | 「禁止过程合格」贯穿 expect/why/hint | **说教**（生成器灌水典型） |

> 归类说明：**尚无整课达到 Canvas「已合格并肩」**（邀请语 0 命中）。局部片段（如 `between_within` glossary 白话、`imr` 的字段专义非套壳）可作「半成品」参考，但不能标为整课合格。

### 3.2 正面样例 ≥3 条（并肩正确向）

以下摘自口吻标尺 Canvas `COLLAB`（并肩版目标文），作为 Agent3 改稿金样——**不是**现网 overlays 已达标文案。

| # | 来源 | 字段角色 | 样例（并肩） |
|---|------|----------|--------------|
| 1 | Canvas 并肩版 | 场景 `scenario` | 「还是那条镀膜线：厚度按子组采，每个子组 5 片。前半段批均值挺老实；走到大约子组 12，整批均值像抬了一级台阶。不妨先打开「组间/组内过程能力」，看看台阶主要落在组间还是组内——停不停线、放不放行，可以等信号看清楚再和现场规程对一下。」 |
| 2 | Canvas 并肩版 | 适用 `used_for` | 「很多现场数据是「一批好几片」：…组间/组内能力就是把这两层拆开看一眼，再和规格放在一起聊——它更像在问「波动主要落在批内，还是批间」，而不是直接回答「这批货能不能放行」。」 |
| 3 | Canvas 并肩版 | 读输出 / 红线 | 「…常见读法是：抬高的分量更多落在组间。现场口语可以停在「批均值抬高了」；至于要不要停线、客户是否接受，通常还要对照规程和规格——软件这一步只是把信号摊开给你看。」 |
| 4（加分） | Canvas 并肩版 | 字段 meaning | 「「厚度_um」是量出来的厚度（微米）。对话框里把它放进测量值…旁边的子组编号只是告诉软件「哪几片算一批」，本身不是测量结果。」——对照现网「这一项决定图上或表上对应哪一列」壳 |

**现网可保留的「半成品」片段（非整课合格）**

- `between_within_capability.json` → glossary「组内/组间变差」白话已接近并肩，改稿时**加长场景/适用即可，勿推倒术语表骨架**。  
- `imr.json` → `dialog_fill_detail` 已是本课专义（无套壳句）；0–6 **加温度时不得删**片 41/55、UCL≠USL、参数表硬点。只需去掉「本课只练」类收束并邀请化。

---

## 4. 生成器灌水风险结论

| 文件 | 仍在灌什么 | 证据 | Agent3 优先级 |
|------|------------|------|----------------|
| `tools/learning_data/copy_depth.py` | **模板壳** | `expand_meaning()` 约 L228：`这一项决定图上或表上对应哪一列。` → 直接喂进 114+ 课 overlays | **必改源头**（否则重建库回灌） |
| `tools/learning_data/wave2_content.py` | **电报 + 说教禁令** | `scenario`:「子组 n=5。子组12起批均值上移。」；大量 `禁止过程合格`；`meaning`:「只陈述定义与边界；禁止过程合格；…」 | **必改**（能力/质量包主源头） |
| `tools/learning_data/wave4_content.py` | **说教禁令灌图形/DOE/时序** | `not_for`/`expect`/`output_guide`/`misc` 模板复用「禁止过程合格 / 已证明正态 / 必须停线」；空课壳 `pad` 同文 | **必改** |
| `tools/learning_data/wave1_content.py` | **电报标题/故事 +「本课只练」** | `used_for` 含「本课只练「固定单位 + 缺陷台阶」…」；`story`「每子组 n=5。…」；`used_for`「固定子组 n=5：…」 | **必改**（控制图包） |
| `tools/learning_data/glossary_bank.py` | 本轮黑名单字符串 **未命中** | Grep 无「你的任务是/禁止过程合格/抖主要/套壳句」 | 口吻升温时仍须**逐条检查白话是否并肩**；非本轮电报主凶 |
| overlays 本身 | 历史产物 | 167 电报字段 + 114 壳 + 38 禁令 `not_for` | 改源头后按 Wave **重写/重生**；禁止只手改 sqlite |

**Grep 六词结论**

| 模式 | overlays | `copy_depth.py` | `wave*_content.py` | `glossary_bank.py` |
|------|----------|-----------------|--------------------|--------------------|
| 你的任务是 | 无 | 无 | 无 | 无 |
| 这一课只练 / 本课只练 | 有（imr、c_chart） | 无 | wave1 有 | 无 |
| 禁止过程合格 | 大量 | 无（间接经 studentize） | wave2/4 **大量** | 无 |
| 抖主要 | 无 | 无 | 无 | 无 |
| 这一项决定图上或表上对应哪一列 | **114 课** | **硬编码生成** | — | 无 |
| 子组 n= | between_within 等 | — | wave1/2 story/scenario | 无 |

---

## 5. 对 Agent2 的直接建议（调研侧，非改码）

1. Wave-0 必须以 `imr` 做出**并肩加长**样板：去「本课只练」，加「不妨/常见读法/通常还要」，**硬点清单可勾选**。  
2. 计划里把 `copy_depth.expand_meaning`、`wave1/2/4` 禁令模板列为**与 overlays 同级必改**，否则 Agent3 白写。  
3. verify 黑名单至少：`你的任务是`、`这一课只练`/`本课只练`、`禁止过程合格`、`抖主要`、套壳句；加 `used_for`/`scenario` 最小汉字长度挡电报。  
4. 全集 184 不得缩成「只改金标」；内部 Wave 只是队列。

---

## 6. Agent1 DoD 勾选

- [x] research md 含 **Primary URL 表 + 访问日期（2026-09-03 UTC+8）**（≥5，本表 10 条）  
- [x] **反面教材 ≥15 条**（上表 19 条；含手册点名 `between_within_capability`）  
- [x] **正面样例 ≥3 条**（Canvas 并肩版 3+1）  
- [x] 写清「**补充语气、不推倒骨架**」边界  
- [x] 生成器灌水风险结论（`copy_depth` / `wave1/2/4` 仍灌电报或说教）  
- [x] **禁止**改产品代码 / overlays / py 生成器 / sqlite / C++（本 Agent 只新建本文件）

**风险（一行）**：若不先改 `copy_depth.py` 与 `wave*_content.py` 的电报/禁令/套壳模板，Agent3 即使手改 overlays，重建 sqlite 仍会大面积回灌。

**go/no-go**：**GO** — 可进入 Agent2 详细计划（Wave 锁表 + 字段检查单 + verify 口吻断言 + 模板源头必改清单）。

---

## 7. 本阶段交付文件列表

| 文件 | 动作 |
|------|------|
| `docs/research/learning-center-copy-voice-warmth-research.md` | **新建**（本报告） |

未改动任何产品代码或学习中心内容层文件。
