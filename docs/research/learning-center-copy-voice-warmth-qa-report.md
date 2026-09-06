# 学习中心文案口吻升温（并肩版）— Agent6 教学检验 QA 报告

> **岗**：Agent6 教学检验（车间同事视角 · 并肩版 COLLAB）  
> **复检日期**：2026-09-06  
> **权威手册**：[`goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md`](goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md)  
> **口吻标尺**：Canvas 并肩版（COLLAB）  
> **调研基线**：[`learning-center-copy-voice-warmth-research.md`](learning-center-copy-voice-warmth-research.md)  
> **读文对象**：`tools/learning_data/tutorial_overlays/*.json`（184）  
> **本岗**：只更新本报告；禁止改产品代码；model inherit  

---

## 总结论

| 项 | 结果 |
|----|------|
| **金标 `imr` 口吻 + 硬点** | **PASS** |
| **go / no-go** | **GO** |
| 旧黑名单（你的任务是 / 这一课只练 / 禁止过程合格 / 抖主要…） | **184 课 0 命中**（亲读 + 全集扫描） |
| 调研点名软壳 A–F（练习线是为 / 目标不是背菜单 / 对照纸条 / 当一次助手 / 还是那张本课练习表 / 导入本课练习表后打开） | **0 命中** |
| `dialog_fill_detail` 旧 meaning 壳 | **0 命中** |
| catalog | **仍为 `learning-center-v2`**（`build_learning_center_db.py` META + `verify_learning_center_db.py` PASS） |
| Python verify | **PASS**（db / copy-depth+voice warmth / gate） |
| 定点返工单 | **有（非阻断）**：见 §5 R19；口吻闸门仍放行 |
| 建议润色 | 见 §6（批量变体壳、7D 问句拗口等）— **不否决 GO** |

**一句话**：金标像同事带看且片 41/55、UCL≠USL、9 字段参数表仍在；控制图母版与组间/组内真并肩；Wave-2/3/4 已去掉调研软壳 A–F，但批量课仍有「对着列名…摊开看一眼」变体 + `not_for` 首句机械重复——记入返工建议，**本轮教学检验放行（GO）**。

---

## 1. 金标 `imr` 判定：**PASS**

| 检查项 | 结果 | 读感（同事视角） |
|--------|------|------------------|
| 0–6 并肩带看 | **PASS** | SMT 锡膏线、不妨打开 I-MR、红线停在「看见信号 / 通常还要对照规程」——像并肩站在屏幕前，不是考试官 |
| 片 **41** / **55** | **PASS** | `scenario`、`buried_signals`、`output_guide`、练习区均可指认 |
| **UCL ≠ USL** | **PASS** | glossary remember、埋点 expect、7A why、7D hint、7E right 多处明示 |
| `dialog_fill_detail` ≥9 | **PASS** | **9** 字段；每条 meaning 绑本课列名/片号，非万能壳 |
| 禁说教串 | **PASS** | 无「你的任务是 / 这一课只练 / 禁止过程合格 / 抖主要」 |
| 软壳 A–F | **PASS** | 金标全文 0 命中 |
| 7A/7C/7D/7E | **PASS** | 像助教笔记：why/hint/right 讲清「为什么」，不训人 |

**金标摘句（验收指认）**

- `scenario`：…大约片 **41** 起有没有整段抬高，片 **55** 附近有没有…尖峰…  
- glossary / 埋点：`UCL ≠ USL`  
- 参数表：变量 / 阶段列 / 移动极差长度 / σ 方法 / Nelson estimate / 规则默认策略 / 特殊原因测试 / 历史均值 / 历史 Sigma → **9**

---

## 2. 每 Wave ≥3 课抽检表

判定口径：电报？说教？meaning 套壳？红线是否仍训人？软壳 A–F？

### Wave-1 控制图（抽 `xbar_r` / `ewma` / `c_chart` / `p_chart`）

| 课 | 电报 | 说教 | meaning 套壳 | 红线 | 总评 |
|----|------|------|--------------|------|------|
| `xbar_r` | 否 | 否 | 否（6 条互异） | 「练习台阶≠放行样板；通常还要对照规程」 | **真并肩**（波动主要落在批均值还是批内） |
| `ewma` | 否 | 否 | 否 | 不等于放行样板；通常还要对照规程 | **真并肩**（片 31 小台阶） |
| `c_chart` | 否 | 否 | 否 | 同上 + UCL≠规格 | **真并肩**（批 26 缺陷台阶） |
| `p_chart` | 否 | 否 | 否 | UCL≠USL；通常还要对照规程 | **合格并肩**（略短于金标，可读） |

### Wave-2 质量 / 能力 / MSA（抽 `between_within_capability` / `gage_rr` / `capability` / `msa_type1` / `pareto`）

| 课 | 电报 | 说教 | meaning 套壳 | 红线 | 总评 |
|----|------|------|--------------|------|------|
| `between_within_capability` | 否 | 否 | 否 | 「不是过程已合格样板；通常还要对照规程」 | **金样级并肩**（子组 12 台阶） |
| `gage_rr` | 否 | 否 | 否 | 有「通常还要…」；**首句重复一遍** | 有列名+埋点；**变体壳** + R19 |
| `capability` | 否 | 否 | 否 | 同上 | 变体壳可读；**不训人** |
| `msa_type1` | 否 | 否 | 否 | 同上 | 同上 |
| `pareto` | 否 | 否 | 否 | 探索≠根因判决 | 同上 |

### Wave-3 推断（抽 `two_sample_t` / `one_way_anova` / `regression` / `normality_test`）

| 课 | 电报 | 说教 | meaning 套壳 | 红线 | 总评 |
|----|------|------|--------------|------|------|
| `two_sample_t` | 否 | 否 | 否 | p 值≠放行；通常还要对照规程 | 变体壳；偶发双「不妨」毛刺 |
| `one_way_anova` | 否 | 否 | 否 | 组间差≠必须停线 | 同上 |
| `regression` | 否 | 否 | 否 | 拟合≠因果/放行 | 同上 |
| `normality_test` | 否 | 否 | 否 | 同上 | 同上 |

### Wave-4 图形 / DOE（抽 `histogram` / `boxplot` / `scatter_plot` / `doe_response` / `response_optimization`）

| 课 | 电报 | 说教 | meaning 套壳 | 红线 | 总评 |
|----|------|------|--------------|------|------|
| `histogram` | 否 | 否 | 否 | 读图≠证明正态/放行 | 变体壳 + 可指行 48–50 |
| `boxplot` | 否 | 否 | 否 | 同上 | 同上（双「不妨」毛刺） |
| `scatter_plot` | 否 | 否 | 否 | 同上 | 同上 |
| `doe_response` | 否 | 否 | 否 | 显著效应≠最优配方已放行 | 变体壳；有「温度=+1」埋点 |
| `response_optimization` | 否 | 否 | 否 | 同上 | 同上（强度Y1/翘曲Y2） |

**Wave 抽检结论**：无电报堆、无旧说教禁令、无 meaning 旧壳、红线已改成「不等于… / 通常还要…」。批量课深度不如金标/Wave-1 母版，属**分套变体**，不构成「大面积软壳 A–F 回归」。

---

## 3. 练习区抽检（7A / 7C / 7D / 7E）— 2 课

| 课 | 揭晓读感 | 判定 |
|----|----------|------|
| `xbar_r` | 7A why / 7D hint / 7E right 能讲清 UCL≠USL、Xbar vs R；7C「不妨先导入…」邀请语气 | **像助教笔记** PASS |
| `between_within_capability` | why/hint/right 停在「组间抬高了 / 通常还要对照规程」，不写「禁止过程合格」 | **像助教笔记** PASS |

附带：批量课 7D 问句模板「看完为什么不能写成能不能直接写成放行？」拗口（见 §6）；DOE 课 hint 偶有句段拼接感——**建议润色，不挡 GO**。

---

## 4. catalog / verify（亲读 + 脚本）

| 项 | 证据 |
|----|------|
| catalog | `tools/build_learning_center_db.py`：`META_VERSION = "learning-center-v2"` |
| `verify_learning_center_db.py` | `PASS: 184 tutorials… catalog learning-center-v2…` |
| `verify_learning_center_copy_depth.py` | `PASS: 184 overlays meet copy-depth readability + voice warmth`（含软壳短语门） |
| `verify_learning_center_gate.py` | `PASS: learning_center_gate …` |

本人已读金标全文与各 Wave 抽检课原文；**不以 verify 代替读文**，verify 作交叉确认。

---

## 5. 定点返工单（非阻断口吻闸门）

| ID | 严重度 | 范围 | 现象 | 建议修法 | 是否否决本轮 GO |
|----|--------|------|------|----------|-----------------|
| **R19** | 中（机械重复） | 约 **162** 课 `not_for` | 首句完整复制两遍，再接「先别硬套无关场景…」 | 生成器去重：同一句只保留一次；顺手收紧万能尾 | **否**（非说教/非电报/非软壳 A–F；红线语义仍在） |

**无**「致命说教 / 电报 / 软壳 A–F 大面积回潮」类返工 → **不开 NO-GO 阻断单**。

---

## 6. 建议润色（零星 / 批量变体 · 不否决）

1. Wave-2/3/4 约 80 课共享「对着「列名」这类现场列时，菜单「…」多半用来把大约第 N 行附近「…」摊开看一眼」——比金标浅，但已带列名+埋点；下轮可按菜单包换 2～3 套开场，勿再灌调研软壳。  
2. 「先别硬套无关场景」高频（与 R19 同包出现）——可改为本课具体误用一句。  
3. 7D 问句「为什么不能写成能不能直接写成放行」→ 改成「为什么还不能写成已经放行」。  
4. 个别 `scenario` 双「不妨」叠句（如 `two_sample_t` / `boxplot`）— 定点删一句即可。  
5. 无练习表 DOE（如 `taguchi_orthogonal_design`）文案诚实说明「可能没有练习表」——可接受；有表课优先加深现场故事。

---

## 7. DoD 勾选

- [x] 金标 `imr` 像并肩带看  
- [x] 硬点：片 41/55、UCL≠USL、`dialog_fill_detail`≥9 仍可指认  
- [x] 无「你的任务是 / 这一课只练 / 禁止过程合格 / 抖主要」  
- [x] 无调研软壳 A–F 终态  
- [x] 每 Wave ≥3 课抽检（控制图 / 质量能力 / 推断 / 图形·DOE）  
- [x] 抽 2 课练习区揭晓像助教笔记  
- [x] catalog 仍为 learning-center-v2（亲读 + verify PASS）  
- [x] QA 报告已覆盖写入本路径  
- [x] 无致命说教/电报 → **GO**；R19 记返工建议  

---

## 8. 风险（一行）

**风险**：若只认「黑名单/软壳 A–F = 0」却长期保留 `not_for` 首句双写与 Wave-2/3/4 变体开场，学员会感到「换菜单名的同一段」——不挡本轮口吻收口，但建议下轮定点清 R19 并加深 1～2 套批量开场。

---

## 9. go / no-go

### **GO**

| 闸门 | 本轮 |
|------|------|
| 金标口吻 + 硬点 | **PASS → GO** |
| 大面积软壳 A–F / 说教 / 电报 | **无 → GO** |
| 零星/批量变体瑕疵 + R19 | 建议润色 / 非阻断返工 |
| **教学检验总判** | **GO** |

请用户本机用 Qt Creator / `package_dist` 抽查：金标 I-MR（片 41/55 + UCL≠USL）+ 任一能力课（`between_within_capability`）+ 任一推断/图形课口吻。中文路径下 Agent 不强跑打包。

---

## 10. 本岗变更

| 文件 | 动作 |
|------|------|
| `docs/research/learning-center-copy-voice-warmth-qa-report.md` | **覆盖写**（2026-09-06 Agent6 · **GO**） |

未改 overlays / 生成器 / sqlite / C++ 产品代码。
