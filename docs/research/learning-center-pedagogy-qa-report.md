# 学习中心教学升级 — Agent6 Pedagogy QA 报告

> **岗**：Agent6 Pedagogy QA（**不写产品代码**）  
> **日期**：2026-09-03  
> **权威**：[`goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md`](goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md)  
> **Canvas 金标**：`canvases/learning-center-tutorial-example.canvas.tsx`  
> **调研 / 计划**：[`learning-center-pedagogy-upgrade-research.md`](learning-center-pedagogy-upgrade-research.md)、[`goal-learning-center-pedagogy-upgrade-wave-plan.md`](goal-learning-center-pedagogy-upgrade-wave-plan.md)  
> **抽检对象**：`tools/learning_data/tutorial_overlays/*.json`、`tools/build_learning_center_db.py` 生成器、`resources/help/learning_center.sqlite`、`src/ui/learning_center_page.cpp`（只读）

---

## 0. 总判（给 Orchestrator）

| 项 | 判定 |
|----|------|
| **金标 `imr` / `imr_spi_shift`** | **通过**（复检仍过） |
| 范围是否缩水（184 全集） | 未缩水（overlay 184、sqlite tutorials 184、datasets 93） |
| UI 是否「只入库不展示」 | **否**：0–6 / 7+ 分节折叠与练习控件已接线；先修反馈已按条目生成（P0 #2 复检过） |
| 旧 10 表是否残留于 live | **否**（sqlite `datasets` 无旧 id；仅 verify/banned 列表与历史文档提及） |
| **Goal complete** | **go** — P0 三项复检全过，金标仍过；可交 Agent5 / UpdateGoal complete |

---

## 1. 金标对照 Canvas（必须项）

对照 Canvas「I-MR + 专用集 + 关键词 + 7+ skill」与 live overlay / 生成器 / sqlite。

| Canvas / DoD 项 | 证据 | 结果 |
|-----------------|------|------|
| 专用数据 `imr_spi_shift` → 工作表 `demo_imr_spi_shift` | overlay `click_steps`；mapping；sqlite `dataset_id=imr_spi_shift`；**无** `demo_` 主键 | 过 |
| 列：片号 / 锡膏高度_um / 时段备注 | mapping + 生成器 CSV 列 | 过 |
| 埋点片 41 阶跃、片 55 尖峰 | `buried_signals`；生成器实测 r41≈123.9「钢网更换后（均值阶跃开始）」、r55=132.4「尖峰」；练习表 `imr_spi_spike_b` 尖峰在片 48 | 过 |
| §0 关键词（含 UCL≠USL） | glossary 11 条，含 UCL/USL/规格限/特殊原因 | 过 |
| §1–3 背景 / 专用数据 / 为何此工具 | `scenario` / `used_for` / `not_for` / `related_ids`→xbar_r | 过 |
| §4 参数表（真实字段，含 Nelson 等） | `dialog_fill_detail` **9** 项；`dialog_fill` 仅 `{"variables":"锡膏高度_um"}`（省略 stage） | 过 |
| §5 读输出对着埋点 | `output_guide` I/MR/诊断表与 41/55 对齐 | 过 |
| §6 误用禁止句 | 明确禁 UCL=USL、Cpk、停线、过程合格、已证明正态 | 过 |
| 7+ 五块非空且**不冲掉** 0–6 | prereq / self_explain / fade / retrieval / misconceptions + `skill_mission` 均在；结构仍是 0–6 正文 + 练习叠加 | 过 |
| fade 独立练习引用 `imr_spi_spike_b` | fade level 2 写明 `demo_imr_spi_spike_b` | 过 |
| catalog v2 | sqlite `meta.catalog_version=learning-center-v2`；C++ `kExpectedCatalogVersion` 同步 | 过 |

**金标判定：通过。**  
说明：Canvas 草稿参数表字段少于现网 `analysis_commands`；实现按手册 H8 **补全** Nelson / rule_policy / tests / 双历史限，属正确加严，不构成驳回。Canvas fade 草稿 4 档 vs 实现 0/1/2 三档，与 Wave-0 DoD（练习表独立练）一致，可接受。

---

## 2. 分 Wave 抽检（每波 ≥2）

抽检口径：术语首次释义、埋点↔读图、无「控制限=规格 / 过程合格 / 必须停线 / 已证明正态」**肯定句**（出现在禁止/误用/错念中算合规）。

### Wave-1（控制图）

| id | 术语 | 埋点↔读图 | 禁止句 | 备注 |
|----|------|-----------|--------|------|
| `xbar_r` | UCL/USL/特殊原因/Xbar/R 齐全 | 子组12 R 尖峰、子组20 Xbar 台阶与 output 对齐 | 仅禁止语境 | **过** |
| `p_chart` | p / 可变 n + UCL≠USL | 批22 台阶与限宽随 n | 仅禁止语境 | **过** |
| （附）`ewma` | EWMA/λ + UCL≠USL | 片31 小台阶 | 禁复用金标集 | **过** |
| （附）`special_cause_rules` | 规则术语 OK | 正文写「不导入」 | 禁止句 OK | **初检驳 → 复检过**（fade 已与「不导入」对齐，见 §9） |

### Wave-2（质量 / MSA / 能力）

| id | 术语 | 埋点↔读图 | 禁止句 | 备注 |
|----|------|-----------|--------|------|
| `capability` | Cp/Cpk/稳定前提 | 偏心 → Cpk&lt;Cp；禁 imr 失控集 | 禁过程合格/已证明正态 | **过** |
| `gage_rr` | %GR&R / 重复性再现性 / 交叉 | 操作员 B +2μm 偏倚 | 禁量具通过 | **过** |

### Wave-3（推断 / ANOVA）

| id | 术语 | 埋点↔读图 | 禁止句 | 备注 |
|----|------|-----------|--------|------|
| `two_sample_t` | glossary 含 H0/H1、p 值、置信区间（均值差）+ Welch | B 线偏高埋点与输出一致 | 仅禁止语境 | **初检弱通过 → 复检过** |
| `one_way_anova` | glossary 含 H0（ANOVA）、F/p、组间 vs 组内 | 腔3 行31–45 对齐 | 仅禁止语境 | **初检弱通过 → 复检过** |

### Wave-4（图形 / DOE / 其余）

| id | 术语 | 埋点↔读图 | 禁止句 | 备注 |
|----|------|-----------|--------|------|
| `histogram` | 直方图/右尾；「已证明正态」作禁止项可接受 | 行48–50 右尾肩部 | 合规 | **过** |
| `doe_factorial` | formula_reference 模板 | 空 dataset（锁表） | 合规 | **过（诚实空表）**；文案「菜单可能不可用」与 `id_metadata` 的 `formula_reference` 一致（help 状态与命令表并存属既有张力，非本 Goal 偷删菜单） |

### formula_reference 诚实性（抽 1+）

- **`weibayes`**：`used_for`/`click_steps`/`fade` 均标明无演示表、菜单可能仅为公式参考；禁止挂旧 10 表 → **诚实，过**。  
- **`doe_factorial`**：同上模板 + 锁表空 dataset → **过**（与 Wave 计划一致）。

---

## 3. 范围 / UI / 旧表

| 检查 | 结果 |
|------|------|
| tutorials = 184 | sqlite + overlays 均为 184 |
| datasets = 93（含练习表） | sqlite `dataset_count=93`；含 `imr_spi_spike_b` |
| 旧 10 id 不进 live | `live_banned=[]`；测试改为 **断言不存在** |
| UI 0–6 分节折叠 | `learning_center_page.cpp`：`CollapsibleSection` 0–6 默认展开 |
| UI 7+ 练习区 | 7A–7E 子折叠 + 外层「7+. 练习闭环」默认折；先修按钮 / 揭晓参考 **有交互** |
| 导航/导入链路 | 未见推倒重建迹象（本 QA 不要求回归测导入） |

**UI（初检缺陷，已定点修复）**：`prereq_block` 曾将答对/答错反馈硬编码为 UCL 文案；复检确认已按 `item.good`/`item.bad` 生成（见 §9）。

---

## 4. 金标判定（再声明）

### 金标：**通过**

禁止以金标未过为由整包回滚；后续只开 **定点返工单**。

---

## 5. 必改列表（定点返工单 → Agent3）

> **禁止**顺手重构无关模块；**禁止**删错念文案赶进度；**禁止**改 `algorithm_help` 公式语义。

### P0 — 完成 Goal 前必须改（**复检已关闭**）

| # | 定点文件 / 位置 | 问题 | 改法（最小） | 复检 |
|---|-----------------|------|--------------|------|
| 1 | `tools/learning_data/tutorial_overlays/special_cause_rules.json`（及对应 wave1 生成源若同源） | 正文/scenario/dialog 写「不导入」；`fade_levels` 1–2 却要求导入 `demo_imr_spi_shift` 并改规则字段 | 重写 fade：level0 对照帮助+金标课字段；level1 默写 rule_policy/tests；level2 写「灵敏度 vs 误报」三段。**不要**在本术语课强制导入数据 | **过** |
| 2 | `src/ui/learning_center_page.cpp` → `prereq_block` | good/bad 反馈写死 UCL 文案 | 按题目给出中性反馈，或按 `item.good`/`item.bad` 生成反馈；**禁止**全课套用 UCL | **过** |
| 3 | Wave-3 抽检代表：`two_sample_t.json`、`one_way_anova.json`（建议同批扫 `wave3_content` 中假设检验/ANOVA 族） | 缺 Canvas 要求的 H0/H1、p 值、置信区间等首次释义；glossary 出现「同构」「≠合格」等元词充数 | **只加深 glossary / scenario / output_guide 白话**，保留埋点与禁止句；勿删 misconceptions | **过** |

### P1 — 强烈建议（不挡金标，建议同一返工波次）

| # | 范围 | 问题 | 改法 |
|---|------|------|------|
| 4 | Wave-3/4 大量 implemented overlay（审计约 70+ 条 glossary 偏短/含「禁止句」元词） | 模板灌水：能过「≥3 条」门禁，但黑带课堂感弱 | 按菜单包分批：每课至少 3 条 **领域术语** 白话；「禁止句/空 dataset」仅留在 formula_reference |
| 5 | formula_reference 与 `analysis_commands` 并存（如 `doe_factorial`） | help 标 `formula_reference` 但命令表有菜单 | **本 Goal 不改 help 语义**；若返工触及文案，改为「以帮助状态为准；若你本机构建已出现菜单，可打开对照字段」，避免绝对「无菜单」 |

### 明确不改（防偷懒式扩大）

- 不推倒 `LearningCenterPage` 导航树 / 导入链路  
- 不把旧 10 表加回  
- 不因金标已过就删 7+ 或把错念清空  
- 不要求本轮重做全部 184 条至金标文风（P1 分批即可；P0 #3 至少修抽检失败代表）

---

## 6. 风险

1. **先修硬编码反馈**会在非 SPC 课制造「学对了却被 UCL 话术纠正」的信任损伤（P0 #2）。  
2. Wave-3/4 模板文风若不全量加深，学员仍会感觉「能导入、读不懂术语」（P1）。  
3. `help.implemented_status=formula_reference` 与菜单已实现并存，可能让用户困惑（既有数据张力）。  
4. 用户需本机 Qt Creator / `package_dist` 自测 UI 折叠与先修按钮；Agent 未跑 GUI。

---

## 7. go / no-go

| 问题 | 答案 |
|------|------|
| 金标过不过？ | **过**（初检 + 复检） |
| 能否现在 UpdateGoal **complete**？ | **可以（go）** |
| 下一步 | 交 **Agent5** 收口 / Orchestrator **UpdateGoal complete** |
| 复检焦点 | 见 §9；P0 三项全过，无新增必改 |

---

## 8. DoD 勾选（Agent6）

- [x] 对照 Canvas：金标含专用数据、关键词、参数表、读图、误用，且 7+ 未冲正文  
- [x] 金标 **通过/驳回** 已判定（**通过**）  
- [x] 每内容 Wave ≥2 抽检  
- [x] 抽检 ≥1 条 formula_reference 诚实性  
- [x] 范围 / UI 展示 / 旧表 live 残留已查  
- [x] 本报告路径：`docs/research/learning-center-pedagogy-qa-report.md`  
- [x] 若需返工：仅列定点单，未改 `src/` 功能赶工  
- [x] P0 定点返工后复检（§9）全过  

**是否可进 Goal complete：go（可交 Agent5）。**

---

## 9. 复检（Agent6 · P0 定点返工后）

> **日期**：2026-09-03（复检）  
> **范围**：只读核验 Orchestrator 已完成的 P0 #1–#3；不写功能代码。  
> **金标**：仍对照 Canvas / live `imr` overlay，未重开全量 Wave。

### 9.1 P0 三项

| # | 核验点 | 证据 | 复检 |
|---|--------|------|------|
| 1 | `special_cause_rules` fade 禁止强制导入 `demo_imr_spi_shift`；与「不导入」正文一致 | `scenario` / `dialog_fill_detail`「本课数据→不导入」；`fade_levels` 0–2 均为对照帮助/默写字段/写灵敏度三段，明确「不导入表 / 不导入演示表」；文件内 **无** `demo_imr_spi_shift` 字符串 | **过** |
| 2 | `prereq_block` 反馈按 `item.good` / `item.bad` 生成，不再写死 UCL | `learning_center_page.cpp`：`正确。本课期望理解：%1`（`good`）；`常见误解是「%1」；更稳妥的理解是「%2」`（`bad`,`good`）。反馈路径无 UCL 硬编码 | **过** |
| 3 | `two_sample_t` / `one_way_anova` glossary 含 H0/p/CI（或 ANOVA 等价） | `two_sample_t`：H0/H1、p 值、置信区间（均值差）；`one_way_anova`：H0（ANOVA）、F/p 值、组间 vs 组内（ANOVA 等价核心，非硬凑 CI） | **过** |

### 9.2 金标抽检（复检）

| 项 | 结果 |
|----|------|
| `imr` 专用表 `demo_imr_spi_shift`、埋点 41/55、UCL≠USL、fade→`demo_imr_spi_spike_b`、`skill_mission` / 7+ 结构 | **仍过**（overlay 关键字段未回退） |

### 9.3 复检结论

| 问题 | 答案 |
|------|------|
| P0 #1 / #2 / #3 | **全过** |
| 金标 | **仍过** |
| 新增必改 | **无** |
| Goal complete / Agent5 | **go** |

P1（Wave-3/4 glossary 文风加深、formula_reference 与菜单并存文案）仍为强烈建议，**不挡** Goal complete。
