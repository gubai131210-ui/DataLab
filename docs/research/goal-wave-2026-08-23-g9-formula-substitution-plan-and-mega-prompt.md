# Wave：G9 公式代入 / 计算过程（全算法覆盖）计划与 Mega `/goal` 提示词（2026-08-23）

> 访问日期：2026-08-23（UTC+8）  
> 调研正文：[`formula-substitution-show-your-work-research-2026-08-23.md`](formula-substitution-show-your-work-research-2026-08-23.md)  
> 执行框架：[`goal-execution-framework.md`](goal-execution-framework.md)  
> DoD 骨架：[`goal-wave-2026-08-23-g9-formula-substitution.md`](goal-wave-2026-08-23-g9-formula-substitution.md)  
> G1（静态，勿合并）：[`g1-g2-formula-registry-chart-copy.md`](g1-g2-formula-registry-chart-copy.md)  
> **不做本 Goal：** G3、G4 全量、G5 大拆、嵌 R/Python、Minitab golden、TreeNet/AutoML、Cassini AGPL 并入

---

## §0 给 Orchestrator 的一页摘要

| 维度 | 内容 |
|------|------|
| **本 Goal 名称** | G9：公式代入（Show Your Work）— **全算法覆盖** |
| **Complete 条件** | FS-A～FS-J **全部**完成；覆盖矩阵 **0 缺口**；`verify_g9_formula_substitution_track.py` PASS |
| **禁止缩小** | 禁止只做框架+2 个试点就 UpdateGoal complete；禁止「先做能力族其余以后再说」 |
| **人手门** | 禁止 agent 强跑 cmake/ctest；完成后告知用户 Qt Creator Rebuild |
| **四角色** | Planner → Implementer → Tester → Checker（串行门禁） |
| **UI** | **必须新建多页**；禁止单页堆控件（见 §5 / 禁止偷懒 14–23） |

### 用户诉求映射

| 诉求 | 落实 |
|------|------|
| 跑完看公式变量取值 | `OutputPage.computation_traces` + 公式代入 UI |
| 网上总结 | research §1 Primary Sources |
| 所有算法 | 覆盖矩阵 + FS-B～FS-I 分族填满 + FS-J 门 |
| 四角色监督 | §4 |
| 足够测试 | §6 |
| UI 不堆页 | §5 四页结构；该新建就新建 |

---

## §1 框架结构（团队 Agent）

```
Orchestrator（/goal 主对话）
  CreateGoal → TodoWrite(FS-A..J + 四角色)
       │
       ▼
  Planner (explore) ──门禁──► 无「覆盖矩阵初稿 + 文件归属 + UI 分页线框」禁止写 domain
       │
       ▼
  Implementer (generalPurpose) ──按 FS-A→J 顺序；加载 cpp-coding
       │  框架 → 试点 → 分族全量 → 覆盖门
       ▼
  Tester (shell) ── verify_g9 + 回归 wave4/wave5/menuIA/g1g2
       │
       ▼
  Checker (bugbot/主 agent) ──Diff vs DoD；UI 是否堆页；覆盖 0 缺口
       │
       ▼
  UpdateGoal complete（仅全绿）
```

**子 Agent 统一结尾：**

```text
文件列表 | DoD [x/ ] | 风险一行 | 是否破坏导入A→B / 既有 verify | 覆盖缺口数
```

---

## §2 锁定交付（FS-A～FS-J）

详见 research §2。摘要：

| ID | 交付 | Complete 证据 |
|----|------|----------------|
| **FS-A** | `ComputationTrace` 类型 + 序列化 + **四页 UI** + 试点 ≥3 命令 | 测例 + 手测入口 |
| **FS-B** | 能力/质量工具族 **全部**命令有实质/最小 trace | 覆盖矩阵勾选 |
| **FS-C** | 控制图族全部 | 同上 |
| **FS-D** | 基础统计/检验/非参数全部 | 同上 |
| **FS-E** | 回归/多变量/ML 全部 | 同上 |
| **FS-F** | 可靠性/寿命全部 | 同上 |
| **FS-G** | DOE/RSM/Taguchi 全部 | 同上 |
| **FS-H** | MSA 全部 | 同上 |
| **FS-I** | 图形+工具全部（图形允许 `display_summary`） | 同上 |
| **FS-J** | `g9-formula-substitution-coverage-matrix.md` + verify **0 缺口** | 脚本 PASS |

**豁免（仅 E 类）：** `tests`、`rule_policy` 等元命令 — Planner 必须写入豁免表并说明理由；**不得**把统计命令塞进豁免。

---

## §3 优先阅读清单（严格顺序）

| # | 路径 | 用途 |
|---|------|------|
| 1 | 本文件 + research | 锁定与市场 |
| 2 | `goal-execution-framework.md` | Wave / 禁止偷懒 1–13 |
| 3 | `g1-g2-formula-registry-chart-copy.md` | **勿合并** G1；可跳转 |
| 4 | `product-evolution-market-ux-architecture-research.md` §2.1 / §7 | G Track 语境 |
| 5 | `deferred-capability-agreement.md` | 勿做清单 |
| 6 | `resources/help/algorithm_help.json` | `formula_blocks` / `symbol_definitions` |
| 7 | `src/ui/formula_registry_dialog.*` | G1 UI 模式参考（新页另建） |
| 8 | `src/ui/output_workspace.*` / `page_renderer.*` | 输出页工具条入口 |
| 9 | `src/domain/quality_types.h`（`OutputPage`） | 挂 `computation_traces` |
| 10 | `src/application/analysis_service.*` | 各族填充 trace |
| 11 | `src/infrastructure/output_serialization.*` | round-trip |
| 12 | `src/ui/analysis_commands.cpp` + `tools/_list_command_ids.py` | **全命令清单权威源** |
| 13 | 导入 / `row_visibility` | A→B 契约 |
| 14 | `docs/algorithm-wiring-index.md`、`unified_track_acceptance_plan.md` §2 | 登记 |

Implementer：**必载** `.agents/skills/cpp-coding/SKILL.md`  
禁止误读：`待修改.md`、`build/`

---

## §4 四角色门禁

| 顺序 | 角色 | subagent | 必须产出 | 未过禁止进入下一角色 |
|------|------|----------|----------|----------------------|
| 1 | **Planner** | explore | ① 全命令分类表（A–E）② 覆盖矩阵初稿路径 ③ domain/UI 文件归属 ④ **UI 四页线框**（禁止单页）⑤ 每族 ≥2 测例计划 ⑥ 豁免名单 | 无表禁止改 domain |
| 2 | **Implementer** | generalPurpose | FS-A→J **全部**；cpp-coding；每族真实 bindings；四页 UI | 禁止跳过序列化/测试/覆盖矩阵；禁止只试点 |
| 3 | **Tester** | shell | `verify_g9_formula_substitution_track.py` PASS；回归 wave4+wave5+menuIA+`verify_g1_g2_track.py`（若存在） | verify 未 PASS 禁止 Checker |
| 4 | **Checker** | bugbot/主 agent | Diff vs DoD；**UI 堆页 = Critical**；覆盖缺口 = Critical；无嵌 Python | Critical → Implementer |

---

## §5 UI 设计铁律（给笨懒 agent）

### 5.1 页面划分（必须新建）

| 屏 | 名称建议 | 只做一件事 |
|----|----------|------------|
| 0 | 输出工具条 | 「公式代入」按钮 |
| 1 | 公式列表页 | 列出本次 traces |
| 2 | 变量取值页 | 一张绑定表 |
| 3 | 代入预览页 | 代入文本 + 结果（+ 可选 steps） |
| 4 | 出处页 | URL / 打开 G1 / evidence_type |

### 5.2 禁止（UI）

1. 禁止把 1–4 屏内容塞进**同一个**无分页 `QWidget`  
2. 禁止在输出主结果区嵌入完整变量表  
3. 禁止与 G1 注册表、G4 Report Card、命令设置对话框混页  
4. 禁止左侧树 + 右侧公式 + 下方绑定 + 侧栏帮助 **四层同屏**  
5. 该用 `QStackedWidget` / 向导 / 多 Dialog 就用；**层次不同 → 不同页**

---

## §6 测试与 verify

### 6.1 代码测试

- `tests/g9_formula_substitution_track_test.cpp`（或分族多个 test target，但 verify 须全部登记）  
- 试点命令：trace 非空、关键符号存在、result≈Facts  
- serialize round-trip  
- UI smoke：四页可切换（若 UI 测可行）  
- `# source: formula_reference`

### 6.2 脚本门

```powershell
python tools/verify_g9_formula_substitution_track.py
python tools/verify_algorithm_wave5_track.py
python tools/verify_algorithm_wave4_track.py
python tools/verify_ui_menu_ia_track.py
python tools/verify_g1_g2_track.py
```

`verify_g9_formula_substitution_track.py` **至少**检查：

1. research + plan + DoD md 存在；DoD FS-A～J 均为 `[x]`  
2. `OutputPage` / 序列化含 `computation_traces`  
3. UI 文件存在且出现「公式列表/变量/代入/出处」分页标记（防单页堆）  
4. `g9-formula-substitution-coverage-matrix.md` 存在  
5. 对 `analysis_commands` 每个 id：矩阵行状态 ∈ {实质绑定, display_summary, 豁免} 且豁免仅允许 E 类  
6. 试点 + 每族抽检命令的 service/domain 标记存在  
7. CMake 含 g9 track test  

---

## §7 交付物

| 产物 | 路径 |
|------|------|
| 调研 | `docs/research/formula-substitution-show-your-work-research-2026-08-23.md` |
| 本计划 + Mega 提示词 | 本文件 |
| DoD | `docs/research/goal-wave-2026-08-23-g9-formula-substitution.md` |
| 覆盖矩阵 | `docs/research/g9-formula-substitution-coverage-matrix.md`（执行时创建） |
| 类型/序列化 | `quality_types.h`、`output_serialization.*` |
| UI | `src/ui/formula_substitution_*.h/.cpp`（新建；命名可微调） |
| 测试 | `tests/g9_formula_substitution_track_test.cpp`（及必要分测） |
| Verify | `tools/verify_g9_formula_substitution_track.py` |
| 登记 | wiring-index、acceptance §2、backlog/product 队列 |

---

## §8 如何继续开发（给人类）

1. 新对话整段粘贴 §9 `/goal`。  
2. Goal complete 后，再开 **G4 Report Card**（可链 G9），勿混。  
3. 人手：Rebuild → 跑任一算法 → 点「公式代入」→ 走完四页；抽测每族 1 个命令；换文件确认 A→B。  

---

## §5b 禁止偷懒

**通用：** goal-execution-framework.md §6（1–13）。

**G9 增补：**

14. **禁止**只做框架 + ≤3 试点命令就 UpdateGoal complete  
15. **禁止**覆盖矩阵用「TODO/稍后」冒充完成  
16. **禁止**把统计命令写入豁免表  
17. **禁止**单页堆叠公式列表+变量表+代入+出处  
18. **禁止**合并/改造 G1 注册表为运行时代入页（只允许跳转）  
19. **禁止**混入 G3/G4/G5  
20. **禁止**嵌 R/Python/KaTeX 运行时；可用纯文本或既有 formula nodes  
21. **禁止** Minitab/JMP 数值 golden  
22. **禁止**破坏 complete-case / `source_row` / A→B / hidden≠excluded  
23. **禁止**解释层「过程合格 / 已证明稳定 / 分布已正态 / 寿命已达标」  
24. **禁止**空 `computation_traces` 却勾 DoD  
25. **禁止**向量/矩阵全量倾倒到 UI（只摘要）  
26. **禁止**跳过 serialization round-trip 测试  
27. **禁止** verify 不检查「全 command 覆盖」  

---

## §9 Mega `/goal` 提示词（复制到新对话整段粘贴）

````markdown
/goal

## 身份与总目标
你是 DataLab Orchestrator。用 `/goal` 模式一次做完 **G9 公式代入 / 计算过程（Show Your Work）**。
**FS-A + FS-B + FS-C + FS-D + FS-E + FS-F + FS-G + FS-H + FS-I + FS-J 全部完成且覆盖矩阵 0 缺口 + verify PASS 才允许 UpdateGoal complete。**
**「所有算法」= analysis_commands 中每个适用命令都有 ComputationTrace（或合法 E 类豁免）。**
禁止缩小范围、禁止只做试点、禁止中途换模型、禁止单页堆 UI。

工作区：DataLab（Qt/C++）
人手门：中文路径 — **禁止** agent 强跑 cmake/ctest；完成后告知用户 Qt Creator Rebuild。
**不要** commit/push，除非用户明确要求。

---

## 本 Goal 做什么（锁定）

权威调研：`docs/research/formula-substitution-show-your-work-research-2026-08-23.md`  
权威计划：`docs/research/goal-wave-2026-08-23-g9-formula-substitution-plan-and-mega-prompt.md`

| ID | 交付 |
|----|------|
| **FS-A** | ComputationTrace 领域模型 + 序列化 + **四页 UI**（列表/变量/代入/出处）+ 输出页入口按钮 + ≥3 试点命令 |
| **FS-B** | 能力/质量工具族 **全部**命令绑定 |
| **FS-C** | 控制图族全部 |
| **FS-D** | 基础统计/检验/非参数全部 |
| **FS-E** | 回归/多变量/ML 全部 |
| **FS-F** | 可靠性/寿命全部 |
| **FS-G** | DOE/RSM/Taguchi 全部 |
| **FS-H** | MSA 全部 |
| **FS-I** | 图形（允许 display_summary）+ 工具命令全部 |
| **FS-J** | `g9-formula-substitution-coverage-matrix.md` + verify **0 缺口** |

权威命令清单：`python tools/_list_command_ids.py`（以 `analysis_commands.cpp` 为准，约 140+）。

DoD：`docs/research/goal-wave-2026-08-23-g9-formula-substitution.md`（逐项 `[x]`）  
Verify：`tools/verify_g9_formula_substitution_track.py`  
测试：`tests/g9_formula_substitution_track_test.cpp`（可另加分族测，但须登记）

---

## UI 铁律（笨懒 agent 必读）
- **必须新建页面**：页1 公式列表 · 页2 变量取值 · 页3 代入预览 · 页4 出处；输出页只留按钮。
- **禁止**单页堆控件；禁止与 G1/G4/设置对话框混页；层次不同必须分页。
- 可参考但勿合并：`formula_registry_dialog`（G1 静态）。

---

## 明确不做
G3 Graph Builder、G4 Report Card 全量、G5 大拆、嵌 R/Python、Minitab golden、TreeNet/AutoML、Cassini AGPL 并入、破坏导入契约、把 G1 改成运行时页。

## 已完成水位（勿重做）
G1/G2、G6、Menu IA、算法 Wave-2～5。本 Goal 只加「运行时公式代入」能力。

---

## 必读（严格按序）
1. `docs/research/goal-wave-2026-08-23-g9-formula-substitution-plan-and-mega-prompt.md`
2. `docs/research/formula-substitution-show-your-work-research-2026-08-23.md`
3. `docs/research/goal-execution-framework.md`
4. `docs/research/g1-g2-formula-registry-chart-copy.md`
5. `docs/research/product-evolution-market-ux-architecture-research.md`
6. `docs/research/deferred-capability-agreement.md`
7. `resources/help/algorithm_help.json`（formula_blocks / symbol_definitions）
8. `src/ui/formula_registry_dialog.*`、`output_workspace.*`、`page_renderer.*`
9. `src/domain/quality_types.h`（OutputPage）
10. `src/application/analysis_service.*`、`src/infrastructure/output_serialization.*`
11. `src/ui/analysis_commands.cpp`、`tools/_list_command_ids.py`
12. 导入与 row_visibility（complete-case、source_row、A→B）
13. `docs/algorithm-wiring-index.md`、`samples/product_evolution/unified_track_acceptance_plan.md` §2

Implementer 必载：`.agents/skills/cpp-coding/SKILL.md`  
禁止误读：`待修改.md`、`build/`

---

## 架构约束
```
ui（输出按钮 + FormulaSubstitution 多页）
  → 只读 OutputPage.computation_traces
AnalysisService / domain 填充 traces（纯 C++）
interpretation 只读；禁止合格越权
禁止：domain→Qt；infrastructure→ui
禁止：破坏 complete-case / source_row / A→B / hidden≠excluded
```

建议结构：
```
ComputationTrace { formula_id, plain_formula, bindings[{symbol,label,value,role}], steps?, result_*, evidence_type, primary_url }
OutputPage.computation_traces
```
权威值 = Facts；代入串仅展示；测试强制 result≈Facts。

---

## 四角色团队（互相监督 · 串行门禁）

| 顺序 | 角色 | subagent | 必须产出 | 门禁 |
|------|------|----------|----------|------|
| 1 | **Planner** | Task explore | 全命令 A–E 分类；覆盖矩阵初稿；UI 四页线框；文件归属；豁免名单；测例计划 | 无表禁止写 domain |
| 2 | **Implementer** | Task generalPurpose | FS-A→J 全做；cpp-coding；每命令真实 bindings（图形可 display_summary） | 禁止只试点；禁止跳过 UI 分页/序列化/矩阵 |
| 3 | **Tester** | Task shell | verify_g9 PASS；回归 wave4+wave5+menuIA+g1g2 | 未 PASS 禁止 Checker |
| 4 | **Checker** | bugbot 或主 agent | Diff vs DoD；UI 堆页=Critical；覆盖缺口=Critical | Critical→Implementer |

子 Agent 结尾：`文件列表 | DoD [x/ ] | 风险一行 | 是否破坏导入A→B/既有verify | 覆盖缺口数`

Orchestrator：CreateGoal 一次；立刻开干；连续到 FS-J；仅 Checker APPROVE + verify PASS 才 complete。

---

## 禁止偷懒
goal-execution-framework.md §6（1–13）+ 本计划 §5b（14–27）。
重点：禁止单页堆 UI；禁止只试点 complete；禁止统计命令豁免；禁止空 trace 勾 DoD；禁止嵌 Python；禁止破坏导入；禁止无覆盖门。

---

## 如何验收

### 脚本门
```powershell
python tools/verify_g9_formula_substitution_track.py
python tools/verify_algorithm_wave5_track.py
python tools/verify_algorithm_wave4_track.py
python tools/verify_ui_menu_ia_track.py
python tools/verify_g1_g2_track.py
```

### 人手门（告知用户）
Rebuild → 任选算法跑通 → 点「公式代入」走完四页 → 每族至少抽 1 个命令 → Taguchi/能力等换文件确认排除不串。

---

## 如何记录
1. DoD 全 `[x]`
2. `g9-formula-substitution-coverage-matrix.md` 0 缺口
3. wiring-index + acceptance §2 增 G9 行
4. research 含访问日期与 Primary URL（已有则核对）
5. 结束告知用户文件清单与 Rebuild 手测点

---

## 启动顺序（本回合立刻执行）
1. CreateGoal（写清 FS-A～J + 全算法覆盖 + verify）
2. TodoWrite
3. Planner explore（无覆盖矩阵+四页线框禁止改代码）
4. Implementer → Tester → Checker
5. 全绿 UpdateGoal complete

中间不要换模型。开始执行。
````

---

**文档状态：** 2026-08-23 首版；§9 可整段粘贴执行。
