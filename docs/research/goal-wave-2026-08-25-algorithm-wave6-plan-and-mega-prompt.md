# Wave：算法 Wave-6（DOE/可靠性补齐）计划与 Mega `/goal` 提示词（2026-08-25）

> 访问日期：2026-08-25（UTC+8）  
> 调研正文：[`algorithm-wave6-market-formula-research-2026-08-25.md`](algorithm-wave6-market-formula-research-2026-08-25.md)  
> 执行框架：[`goal-execution-framework.md`](goal-execution-framework.md)  
> 状态权威：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) §12 / §15  
> 候选池：[`next-wave-algorithms-charts-ml-oss.md`](next-wave-algorithms-charts-ml-oss.md) Track H / DOE / Reliability  
> DoD 文件：[`goal-wave-2026-08-25-algorithm-wave6.md`](goal-wave-2026-08-25-algorithm-wave6.md)  
> **不做本 Goal：** G3 Graph Builder 全量、TreeNet/AutoML、嵌 R/Python、Minitab golden、GLM Mixed/MANOVA 全量、Automated Capability、「一次做完 Feature List 全部 ❌」

---

## §0 给 Orchestrator / 人类的一页摘要

| 维度 | 内容 |
|------|------|
| **Goal 名称** | 算法 Wave-6：4 项竖切（`taguchi_analyze` / `mixture_design` / `nhpp_repairable` / `reliability_test_plan`） |
| **为何这 4 项** | Wave-5 已补 Taguchi **设计** + Weibayes + 分布工具；本波补 Taguchi **分析**、Mixture **设计**、可修复 NHPP、可靠性 **试验计划**——对齐 Minitab DOE+Reliability 最大剩余缝，且汽车质量杠杆高 |
| **Wave 数** | **1 个 Wave；4 项全部 ✅ 才 complete** |
| **交付门** | `python tools/verify_algorithm_wave6_track.py` PASS |
| **人手门** | 用户 Qt Creator Rebuild；agent **禁止**强跑 cmake/ctest（中文路径） |
| **四角色** | Planner → Implementer → Tester → Checker（串行门禁、互相监督） |
| **导入** | 每项证明不破坏 complete-case / `source_row` / A→B |
| **「全部算法」口径** | 本 Goal **清空锁定 4 项**；全产品 ❌ 由 Wave-6/7/… 滚动，见 research §4 |

### 用户诉求映射（来自备忘）

| 诉求 | 本计划如何落实 |
|------|----------------|
| 算法扩展 + 深化 | 3 新增缝 + 1 工具链（Test plan）；Taguchi 是「设计→分析」深化 |
| 一次多做 | 锁定 **4 项**同 Goal |
| 网上公式 + Minitab 输出 | research + 每项 `p6_*.md` + Primary URL |
| 导入衔接 | §3 + Tester 强制回归 |
| 四角色互相监督 | §4 串行门禁 |
| 足够测试 | domain/service + Wave verify + formula_reference + 回归 wave5/menuIA |
| UI 不堆控件 | §5 UI 硬规则；每命令多页 |
| 执行 agent 很懒 | §5 禁止偷懒 14–28 条；Checker 对照 DoD |

---

## §1 框架结构（团队 Agent）

```
Orchestrator（/goal 主对话）
  CreateGoal → TodoWrite(W6-1..4 + 四角色)
       │
       ▼
  Planner (Task explore) ──门禁──► 交「4 项映射表+UI线框+测例+导入影响」
       │                              禁止写 domain
       ▼
  Implementer (Task generalPurpose) ──按 id 顺序竖切；加载 cpp-coding
       │  每项：p6_*.md → domain → Facts → service → commands(含 Menu IA)
       │       → interpretation → serialization → tests → help → wiring
       ▼
  Tester (Task shell) ──写/跑 verify_algorithm_wave6_track.py
       │                 + 回归 wave5 + menuIA + g9 deepen（若存在）
       ▼
  Checker (bugbot/自查) ──Diff vs DoD；Critical→回 Implementer
       │
       ▼
  UpdateGoal complete（仅全绿）
```

**子 Agent 统一结尾模板：**

```text
文件列表 | DoD [x/ ] | UI页数确认 | 风险一行 | 是否破坏导入A→B / wave5 verify
```

---

## §2 锁定项摘要

| ID | command_id | DoD 要点 |
|----|------------|----------|
| W6-1 | `taguchi_analyze` | 静态 Taguchi；≥2 种 S/N；响应表+Delta/Rank；主效应图；完整竖切 |
| W6-2 | `mixture_design` | simplex-lattice 或 centroid；q=3～4；写入工作表；预览页 |
| W6-3 | `nhpp_repairable` | 幂律 NHPP；β/λ 估计；强度或累积均值表；可选 Duane 图 |
| W6-4 | `reliability_test_plan` | Weibull 演示型样本量/允许失效；计划表+假设摘要 |

详情与公式 URL：调研文档 §2。

---

## §3 优先阅读清单（严格顺序）

| # | 路径 | 用途 |
|---|------|------|
| 1 | **本文件** + [`algorithm-wave6-market-formula-research-2026-08-25.md`](algorithm-wave6-market-formula-research-2026-08-25.md) | 锁定与公式入口 |
| 2 | [`goal-execution-framework.md`](goal-execution-framework.md) | Wave / 禁止偷懒 |
| 3 | [`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) | 状态登记；§13 延后勿做 |
| 4 | [`next-wave-algorithms-charts-ml-oss.md`](next-wave-algorithms-charts-ml-oss.md) | Track H / DOE / Reliability |
| 5 | [`deferred-capability-agreement.md`](deferred-capability-agreement.md) | 勿做清单 |
| 6 | Wave-5 范例：`docs/research/p5_*.md` + `goal-wave-2026-08-23-algorithm-wave5-plan-and-mega-prompt.md` | 竖切写法 |
| 7 | 邻域 domain：`taguchi_orthogonal.*`、`doe_*`、`reliability*`、`weibayes.*`、`accelerated_life.*`、`t_power` | 代码邻域 |
| 8 | `src/application/analysis_service.h/.cpp` | 接线 |
| 9 | `src/ui/analysis_commands.h/.cpp` | 命令元数据 + Menu IA |
| 10 | 导入：`infrastructure/data_import*`、worksheet / `row_visibility` | A→B 契约 |
| 11 | `docs/algorithm-wiring-index.md`、`samples/product_evolution/unified_track_acceptance_plan.md` | 登记 |
| 12 | 回归：`tools/verify_algorithm_wave5_track.py`、`verify_ui_menu_ia_track.py` | 不得破坏 |

Implementer：**必载** `.agents/skills/cpp-coding/SKILL.md`  
**禁止误读：** `待修改.md`、`build/`、把 G9/UI IA 大改当成本 Goal 主交付

---

## §4 四角色门禁（互相监督）

| 顺序 | 角色 | subagent | 必须产出 | 未过禁止进入下一角色 |
|------|------|----------|----------|----------------------|
| 1 | **Planner** | explore | 每项：id 冲突检查、domain 文件归属、**UI 分页线框**、表形清单、≥3 测例/项、导入影响、Primary URL 钉死；终稿 4 项确认 | 无表禁止改 domain |
| 2 | **Implementer** | generalPurpose | 4 项完整竖切 + CMake 测试目标；每命令对话框 **≥2 页** | 禁止跳过 interpretation/help；禁止 golden；禁止单页堆控件 |
| 3 | **Tester** | shell | `tests/algorithm_wave6_track_test.cpp` + `verify_algorithm_wave6_track.py` PASS；回归 wave5 + menuIA | verify 未 PASS 禁止 Checker |
| 4 | **Checker** | bugbot/自查 | Diff vs DoD；无 Critical；无嵌 Python；无破坏 Menu IA；UI 无「单页堆积」 | Critical → Implementer |

---

## §5 禁止偷懒

### 5.1 通用

粘贴并遵守 `goal-execution-framework.md` **§6（1–13）** 全文。

### 5.2 Wave-6 增补（防笨防懒）

14. 禁止只做 1～2 个算法就 `UpdateGoal complete`  
15. 禁止跳过网上 Primary URL / `p6_*.md`  
16. 禁止把 Minitab/JMP **数值**当 golden 写入 `VALIDATION_MATRIX.md`  
17. 禁止破坏 complete-case / `source_row` / A→B  
18. 禁止解释层「过程已失控 / 已证明稳定 / 批次合格 / 分布已正态」  
19. 禁止把 sklearn/R/Python 运行时打进 dist  
20. 禁止宣称等同 Minitab Taguchi/Mixture/Reliability Module 数值对齐  
21. 禁止混入 G3 Graph Builder / G5 大拆 / Menu IA 推翻重做  
22. 禁止大 catalog 单 TU 无拆分（interpretation 过大须 part）  
23. 禁止无 `menu_path`/`menu_group` 的新命令  
24. **禁止把「实现全部 Feature List」偷换成「只改文档/只加菜单壳」**  
25. **UI：禁止在一个对话框单页堆积大量控件**；选项 / 方法 / 预览 / 写入确认 **按层次拆页**；层次不一致的内容不得同页硬塞  
26. **UI：禁止「设计生成 + 分析 + 优化」三个流程塞进同一页**；该新建命令/对话框就新建  
27. 禁止 Taguchi 分析复用设计生成对话框「加几个勾选」交差  
28. 禁止 NHPP / Test plan 只有一个数字、无表无形、无诊断、无 formula_reference 测试  

---

## §6 测试与 verify

### 6.1 每项最低

- domain 或 service 测：主路径 + 门禁失败诊断  
- `# source: formula_reference` ≥1  
- `find(command_id)` 非空；Menu IA 四顶层合法  
- 设计写入类：至少 1 测「写入列数/水平合法」  

### 6.2 Wave 脚本门

```powershell
python tools/verify_algorithm_wave6_track.py
python tools/verify_algorithm_wave5_track.py
python tools/verify_ui_menu_ia_track.py
```

`verify_algorithm_wave6_track.py` 至少检查：

1. 本 research + DoD md 存在且 W6-1～4 有 `[x]`  
2. 四命令 id 出现在 `analysis_commands.cpp`  
3. 各 `p6_*.md` 存在且含 Primary URL / 访问日期  
4. CMake 含 `algorithm_wave6_track_test`  
5. wiring-index + acceptance 含 Wave-6 / 各 id  
6. 测试文件含四个 id 标记  

---

## §7 交付物

| 产物 | 路径 |
|------|------|
| 调研 | `docs/research/algorithm-wave6-market-formula-research-2026-08-25.md` |
| 本计划 + Mega 提示词 | 本文件 |
| DoD | `docs/research/goal-wave-2026-08-25-algorithm-wave6.md` |
| 每项 research | `docs/research/p6_taguchi_analyze.md` 等 |
| 测试 | `tests/algorithm_wave6_track_test.cpp` |
| Verify | `tools/verify_algorithm_wave6_track.py` |
| 登记 | backlog、wiring-index、acceptance §2 |

---

## §8 如何继续开发（给人类）

1. **新开对话**，整段粘贴下方 **§9 Mega `/goal` 提示词**。  
2. 脚本门绿后，你在 Qt Creator Rebuild，按四命令手测。  
3. Wave-6 完成后，下一波从 research §4 候补队列开 **Wave-7**（Mixture 分析 / GLM 窄化等）。  
4. 图表类（parallel/bubble）建议另开 Track G Goal，勿与本算法 Goal 混会话。  

---

## §9 Mega `/goal` 提示词（复制到新对话整段粘贴）

````markdown
/goal

## 身份与总目标
你是 DataLab Orchestrator。用 `/goal` 模式一次做完 **算法 Wave-6**。
**W6-1 + W6-2 + W6-3 + W6-4 全部完整竖切且脚本门 PASS 才允许 UpdateGoal complete。**
禁止缩小范围、禁止只做 1 项、禁止中途换模型、禁止用「文档更新」冒充算法完成。

工作区：DataLab（Qt/C++ 汽车质量桌面工具）
人手门：中文路径 — **禁止** agent 强跑 cmake/ctest；完成后告知用户 Qt Creator Rebuild 手测点。
**不要** commit/push，除非用户明确要求。

「实现所有算法」在本产品的正确含义：按 backlog 产品范围内 ❌ 滚动清空。
**本 Goal 只锁定下面 4 项，但必须 4 项都做完**；其余登记到 Wave-7+，禁止假装 Feature List 已 100% 克隆。

---

## 本 Goal 做什么（锁定 4 项）

权威调研：`docs/research/algorithm-wave6-market-formula-research-2026-08-25.md`  
权威计划：`docs/research/goal-wave-2026-08-25-algorithm-wave6-plan-and-mega-prompt.md`  
DoD：`docs/research/goal-wave-2026-08-25-algorithm-wave6.md`

| ID | command_id | 交付 |
|----|------------|------|
| **W6-1** | `taguchi_analyze` | 静态 Taguchi 分析：≥2 种 S/N；Means/S/N 响应表+Delta/Rank；主效应图；完整竖切 |
| **W6-2** | `mixture_design` | Mixture 设计生成（simplex-lattice 或 centroid，q=3～4）；写入工作表；矩阵预览页 |
| **W6-3** | `nhpp_repairable` | 可修复系统幂律 NHPP（Crow–AMSAA 类）；参数表+强度/累积均值；可选 Duane 图 |
| **W6-4** | `reliability_test_plan` | Weibull 演示型可靠性试验计划（样本量/允许失效）；计划表+假设摘要 |

每项竖切模板（缺一不可）：
`p6_*.md`（含 Primary URL + 访问日期 + 公式）
→ domain（纯 C++）
→ `*Facts`
→ AnalysisService
→ analysis_commands（**必须** menu_path + menu_group）
→ interpretation（只读 Facts）
→ serialization
→ `# source: formula_reference` 测试
→ help catalog（禁止正文「见 md」）
→ wiring-index + backlog 状态 + acceptance §2

备选替换仅当 Planner 证明原项不可行（见 research §2 备选表）；**不得减项**。

Verify：`tools/verify_algorithm_wave6_track.py`  
测试：`tests/algorithm_wave6_track_test.cpp`

---

## 明确不做
- G3 Graph Builder 全量拖拽、可旋转 3D、G5 架构大拆
- TreeNet / AutoML / 商标级 Random Forests 对齐
- 嵌 R/Python；sklearn 进 dist
- Minitab 数值 golden / VALIDATION_MATRIX 伪造
- GLM Mixed / MANOVA 全量、DSD、Split-plot、D-opt、extreme-vertices Mixture
- Automated Capability / Assistant
- 破坏导入契约；重做 Wave-2～5 已 ✅ 算法

## 已完成水位（勿重做）
P0–P2 主项；Wave-2～5（含 `taguchi_orthogonal_design`/`weibayes`/`random_forest`/`distribution_calculator`）；
G1/G2/G6/G9（公式代入/分步求值已有——本 Goal 不为 G9 主交付，但新命令有公式时应可挂 trace）。

---

## 必读（严格按序）
1. `docs/research/goal-wave-2026-08-25-algorithm-wave6-plan-and-mega-prompt.md`
2. `docs/research/algorithm-wave6-market-formula-research-2026-08-25.md`
3. `docs/research/goal-execution-framework.md`（尤其 §6 禁止偷懒）
4. `docs/research/minitab-market-algorithm-backlog.md`
5. `docs/research/next-wave-algorithms-charts-ml-oss.md`
6. `docs/research/deferred-capability-agreement.md`
7. Wave-5 范例 `docs/research/p5_*.md`
8. 邻域：`src/domain/statistics/taguchi_orthogonal.*`、`doe_*`、`reliability*`、`weibayes.*`、`accelerated_life.*`
9. `src/application/analysis_service.*`、`src/ui/analysis_commands.*`
10. 导入与 `row_visibility`（complete-case、source_row、A→B）
11. `docs/algorithm-wiring-index.md`、`samples/product_evolution/unified_track_acceptance_plan.md` §2

Implementer 必载：`.agents/skills/cpp-coding/SKILL.md`  
禁止误读：`待修改.md`、`build/`

执行前：**再次用 WebSearch/WebFetch 核对**每项 Primary URL（NIST / Minitab Methods and Formulas），把最终公式写入对应 `p6_*.md`，访问日期写 **当天**。

---

## 架构约束
```
ui/analysis_commands → application/AnalysisService → domain/statistics（纯 C++）
interpretation 只读 Facts
禁止：domain→Qt；infrastructure→ui
禁止：破坏 complete-case / source_row / A→B / hidden≠excluded
新命令必须带 menu_path + menu_group（Menu IA 四顶层）
```

### UI 硬规则（执行 agent 特别容易偷懒 — 强制）
1. **该新建页面就新建页面**：选项页 ≠ 方法说明页 ≠ 设计矩阵预览页 ≠ 写入确认。
2. **禁止单页堆积大量控件**；禁止把层次不一致的内容（设计参数 vs 分析结果 vs 优化目标）塞同一页。
3. Taguchi：**分析**必须独立命令/对话框，禁止在 `taguchi_orthogonal_design` 对话框「加勾选」交差。
4. Mixture：设计生成与未来「分析」分离；本 Goal 只做设计生成+预览+写表。
5. 新对话框参考现有多页分析对话框模式；控件分组清晰；默认值合理。

---

## 四角色团队（互相监督 · 串行门禁）

| 顺序 | 角色 | subagent | 必须产出 | 门禁 |
|------|------|----------|----------|------|
| 1 | **Planner** | Task explore | 4 项映射表（文件归属+**UI分页线框**+表形+≥3测例+导入影响+Primary URL）；确认无 id 冲突 | 无表禁止写 domain |
| 2 | **Implementer** | Task generalPurpose | 按 W6-1→4 顺序完整竖切；加载 cpp-coding | 禁止跳过 help/interp/测试；禁止单页堆 UI |
| 3 | **Tester** | Task shell | wave6 测试 + verify PASS；回归 wave5 + menuIA | verify 未 PASS 禁止 Checker |
| 4 | **Checker** | bugbot 或主 agent | Diff vs DoD；无 Critical；抽查 UI 分页 | Critical→Implementer |

子 Agent 结尾：`文件列表 | DoD [x/ ] | UI页数 | 风险一行 | 是否破坏导入A→B/wave5 verify`

Orchestrator：CreateGoal 一次；立刻开干；连续到 4 项全完；仅 Checker APPROVE + verify PASS 才 complete。

---

## 禁止偷懒
`goal-execution-framework.md` §6（1–13）+ 本计划 §5.2（14–28）。
重点：禁止单算法 complete；禁止 golden；禁止嵌 Python；禁止破坏导入；禁止无 Primary URL；**禁止单页堆控件**；禁止只做菜单壳。

---

## 如何验收

### 脚本门
```powershell
python tools/verify_algorithm_wave6_track.py
python tools/verify_algorithm_wave5_track.py
python tools/verify_ui_menu_ia_track.py
```

### 人手门（告知用户，勿代跑 Qt）
Rebuild 后分别打开：
1. Taguchi 分析 — 选因子/响应，切 S/N 类型，看响应表与主效应图  
2. Mixture 设计 — 生成矩阵预览 → 写入工作表 → 换文件确认排除不串  
3. NHPP — 累积失效时间列，看参数与图/表  
4. 可靠性试验计划 — 改 β/R/置信度，看 n 与假设摘要  

---

## 如何记录
1. `goal-wave-2026-08-25-algorithm-wave6.md` DoD 全 `[x]`
2. backlog 对应行 ✅/⚪；§15 增 Wave-6 锁定表
3. wiring-index + acceptance §2 增 Wave-6 行
4. 每项 `p6_*.md` 含访问日期与 Primary URL 与公式
5. 结束告知用户文件清单与 Rebuild 手测点；**不**擅自 commit/push

---

## 启动顺序（本回合立刻执行）
1. CreateGoal（写清 W6-1～4 + verify）
2. TodoWrite（含四角色）
3. Planner explore（无计划表禁止改代码）
4. 网上再搜一遍公式，写/补 `p6_*.md`
5. Implementer → Tester → Checker
6. 全绿 UpdateGoal complete

中间不要换模型。开始执行。
````

---

**文档状态：** 2026-08-25 首版；结构对齐 Wave-5 mega-prompt，供新对话直接粘贴 §9。
