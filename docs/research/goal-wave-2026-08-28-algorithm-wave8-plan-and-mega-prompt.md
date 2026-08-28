# Wave：算法 Wave-8（DOE 二值 + 多元聚类 + GLM 扩展 + 寿命回归）计划与 Mega `/goal` 提示词（2026-08-28）

> 访问日期：2026-08-28（UTC+8）  
> 调研正文：[`algorithm-wave8-market-formula-research-2026-08-28.md`](algorithm-wave8-market-formula-research-2026-08-28.md)  
> 执行框架：[`goal-execution-framework.md`](goal-execution-framework.md)  
> 状态权威：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) §12 / §15  
> DoD：[`goal-wave-2026-08-28-algorithm-wave8.md`](goal-wave-2026-08-28-algorithm-wave8.md)  
> **不做本 Goal：** Mixed REML/MANOVA 全量、Expanded Gage 不平衡全量、TreeNet/AutoML、嵌 R/Python、Minitab golden、Mixture 过程变量、Graph Builder 全量、ABC 三阶交互全量、「一次做完 Feature List 全部 ❌」

---

## §0 给 Orchestrator / 人类的一页摘要

| 维度 | 内容 |
|------|------|
| **Goal 名称** | 算法 Wave-8：4 项竖切（`binary_response_doe` / `cluster_variables` / `glm_three_factor` / `life_data_regression`） |
| **为何这 4 项** | W7 补了双因子 GLM/分散/FA；本波补 **二值 DOE** + **变量聚类** + **三因子 GLM** + **寿命协变量回归**——对齐 backlog 最大剩余 DOE/多元/ANOVA/可靠性缝 |
| **Wave 数** | **1 个 Wave；4 项全部 ✅ 才 complete** |
| **交付门** | `python tools/verify_algorithm_wave8_track.py` PASS |
| **人手门** | 用户 Qt Creator Rebuild；agent **禁止**强跑 cmake/ctest（中文路径） |
| **四角色** | Planner → Implementer → Tester → Checker（串行门禁、互相监督） |
| **「全部算法」口径** | 本 Goal **清空锁定 4 项**；全产品 ❌ 由 Wave-9+ 滚动 |

### 用户诉求映射（来自 `待修改.md`）

| 诉求 | 本计划如何落实 |
|------|----------------|
| 算法扩展 + 深化 | W7 GLM 双因子→W8 三因子；补 Binary DOE / 变量聚类 / 寿命回归 |
| 一次多做 | 锁定 **4 项**同 Goal |
| 网上公式 + Minitab 输出 | research + 每项 `p8_*.md` + Primary URL |
| 导入衔接 | §3 + Tester 强制回归 wave7 |
| 四角色互相监督 | §4 串行门禁 |
| 足够测试 | domain/service + Wave verify + formula_reference + 回归 |
| **UI 不堆控件** | §5 UI 硬规则；每命令 **≥4 页**，层次分离 |
| 执行 agent 很懒 | §5 禁止偷懒 14–32；Checker 对照 DoD |
| 公式与来源页 | help catalog + G9 trace（有公式则挂） |

---

## §1 框架结构（团队 Agent）

```
Orchestrator（/goal 主对话）
  CreateGoal → TodoWrite(W8-1..4 + 四角色)
       │
       ▼
  Planner (Task explore) ──门禁──► 4 项映射表 + UI 线框 + 测例 + Primary URL
       │                              禁止写 domain
       ▼
  Implementer (Task generalPurpose) ── W8-1→4 顺序竖切；加载 cpp-coding
       │  每项：p8_*.md → domain → Facts → service → commands(Menu IA)
       │       → **专用多页 QDialog** → interpretation → serialization → tests → help
       ▼
  Tester (Task shell) ── verify_algorithm_wave8_track.py PASS
       │                 + 回归 wave7 + wave6 + menuIA
       ▼
  Checker (bugbot/自查) ── Diff vs DoD；UI 分页抽查；Critical→Implementer
       │
       ▼
  UpdateGoal complete（仅全绿）
```

**子 Agent 统一结尾模板：**

```text
文件列表 | DoD [x/ ] | UI页数确认 | 风险一行 | 是否破坏导入A→B / wave7 verify
```

---

## §2 锁定项摘要

| ID | command_id | DoD 要点 |
|----|------------|----------|
| W8-1 | `binary_response_doe` | Logit IRWLS；events/trials 或 0/1；Coef+OR+拟合；≠ logistic_regression 对话框 |
| W8-2 | `cluster_variables` | 相关距离；层次连结；dendrogram + amalgamation；≠ cluster_observations |
| W8-3 | `glm_three_factor` | 三因子不平衡；Type III；Fitted Means；无 ABC 三阶；≠ glm_two_way 对话框 |
| W8-4 | `life_data_regression` | Weibull MLE；1～2 协变量；右删失；回归+百分位；≠ accelerated_life 对话框 |

详情与公式 URL：调研文档 §2。

---

## §3 优先阅读清单（严格顺序）

| # | 路径 | 用途 |
|---|------|------|
| 1 | **本文件** + [`algorithm-wave8-market-formula-research-2026-08-28.md`](algorithm-wave8-market-formula-research-2026-08-28.md) | 锁定与公式 |
| 2 | [`goal-execution-framework.md`](goal-execution-framework.md) §6 | 禁止偷懒 |
| 3 | [`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) | §8 DOE / §4 GLM / §9 可靠性 / §10 多元 |
| 4 | [`next-wave-algorithms-charts-ml-oss.md`](next-wave-algorithms-charts-ml-oss.md) | Track H 候补 |
| 5 | [`deferred-capability-agreement.md`](deferred-capability-agreement.md) | 勿做 |
| 6 | Wave-7 范例：`docs/research/p7_*.md` + `goal-wave-2026-08-28-algorithm-wave7-plan-and-mega-prompt.md` | 竖切与 UI 分页 |
| 7 | Wave-7 邻域：`glm_two_way.*`、`factor_analysis.*`、`mixture_analyze.*`、`analyze_variability.*` | 代码邻域 |
| 8 | 本波邻域：`logistic_regression.*`、`doe_factorial.*`、`cluster_observations.*`、`hierarchical_cluster.*`、`reliability.*`、`accelerated_life.*` | 复用模式 |
| 9 | UI 范例：`glm_two_way_dialog.*`、`mixture_analyze_dialog.*`、`taguchi_analyze_dialog.*` | QStackedWidget 多页 |
| 10 | `src/application/analysis_service.*`、`src/ui/analysis_commands.*`、`mainwindow.cpp` 命令拦截 | 接线 |
| 11 | 导入与 `row_visibility` | complete-case、source_row、A→B |
| 12 | `docs/algorithm-wiring-index.md`、`unified_track_acceptance_plan.md` §2 | 登记 |
| 13 | 回归：`tools/verify_algorithm_wave7_track.py`、wave6、menuIA | 不得破坏 |

Implementer：**必载** `.agents/skills/cpp-coding/SKILL.md`  
**禁止误读：** `待修改.md`、`build/`

执行前：**WebSearch/WebFetch 核对**每项 Primary URL，公式写入 `p8_*.md`，访问日期 **当天**。

---

## §4 四角色门禁（互相监督）

| 顺序 | 角色 | subagent | 必须产出 | 未过禁止进入下一角色 |
|------|------|----------|----------|----------------------|
| 1 | **Planner** | explore | 4 项映射表：**UI 分页线框**（每页控件清单）、表形、≥3 测例/项、导入影响、Primary URL、id 无冲突 | 无表禁止写 domain |
| 2 | **Implementer** | generalPurpose | 4 项完整竖切；**每命令专用 QDialog ≥4 页**；禁止 AnalysisSetupDialog 单页堆控件 | 禁止跳过 help/interp/测试 |
| 3 | **Tester** | shell | `algorithm_wave8_track_test.cpp` + `verify_algorithm_wave8_track.py` PASS；回归 wave7/wave6/menuIA | verify 未 PASS 禁止 Checker |
| 4 | **Checker** | bugbot/自查 | Diff vs DoD；无 Critical；**UI 无单页堆积**；命令未错误合并对话框 | Critical → Implementer |

---

## §5 禁止偷懒

### 5.1 通用

粘贴并遵守 `goal-execution-framework.md` **§6（1–13）** 全文。

### 5.2 Wave-8 增补（防笨防懒）

14. 禁止只做 1～2 个算法就 `UpdateGoal complete`  
15. 禁止跳过 Primary URL / `p8_*.md`  
16. 禁止 Minitab/JMP **数值**当 golden  
17. 禁止破坏 complete-case / `source_row` / A→B  
18. 禁止解释层「过程已失控 / 已证明稳定 / 批次合格 / 分布已正态 / 寿命已达标」  
19. 禁止 sklearn/R/Python 打进 dist  
20. 禁止宣称数值等同 Minitab Binary DOE/GLM/Life Regression  
21. 禁止混入 G3 Graph Builder / G5 大拆  
22. 禁止无 `menu_path`/`menu_group` 的新命令  
23. **禁止把 Binary DOE 塞进 `logistic_regression` 对话框「加勾选」**  
24. **禁止把三因子 GLM 塞进 `glm_two_way` 对话框「加因子」**  
25. **禁止单页堆：列选择 + 模型阶 + 连结 + 结果表 + 图**  
26. **禁止层次不一致内容同页**（例：因子列选择 vs OR 表 vs 优化建议）  
27. Binary DOE：**响应布局（event/trial）**与**模型项**必须分页  
28. Cluster Variables：**变量选择**与**距离/连结**分页；dendrogram 在输出区  
29. GLM 三因子：**三因子列选择**与**两两交互开关**分页  
30. Life regression：**时间/删失**与**协变量**分页；分布选项独立页  
31. 禁止只有菜单壳、无 domain/Facts/测试  
32. 禁止用「文档更新」冒充算法完成  

### 5.3 UI 分页最低要求（Checker 硬查）

| 命令 | 最少页数 | 分页线框 |
|------|----------|----------|
| `binary_response_doe` | **4** | 数据布局(因子+event/trial) \| 模型(主效应/交互) \| 方法(logit/IRWLS) \| 预览 |
| `cluster_variables` | **4** | 变量选择 \| 距离/连结 \| 方法说明 \| 预览 |
| `glm_three_factor` | **4** | 列选择(三因子+响应) \| 模型(主效应/两两交互) \| 方法(不平衡/拟合均值) \| 预览 |
| `life_data_regression` | **4** | 时间+删失+协变量 \| 分布/百分位 \| 方法说明 \| 预览 |

实现模式：`QDialog` + `QStackedWidget` + Back/Next/Run；参考 `glm_two_way_dialog` / `mixture_analyze_dialog`；`MainWindow::run_from_spec` 拦截四 id（同 Wave-7）。

---

## §6 测试与 verify

### 6.1 每项最低

- domain 或 service 测：主路径 + 门禁失败诊断  
- `# source: formula_reference` ≥1  
- `find(command_id)` 非空；Menu IA 四顶层合法  
- Binary DOE：events≤trials；简单 2×2 logit 可收敛  
- Cluster Variables：ρ=1 → d=0；amalgamation 步数 = p-1  
- GLM 三因子：不平衡单元格仍可算 Type III  
- Life regression：Weibull 玩具数据 MLE 可收敛；删失行计入  

### 6.2 Wave 脚本门

```powershell
python tools/verify_algorithm_wave8_track.py
python tools/verify_algorithm_wave7_track.py
python tools/verify_algorithm_wave6_track.py
python tools/verify_ui_menu_ia_track.py
```

`verify_algorithm_wave8_track.py` 至少检查：

1. research + DoD md 存在且 W8-1～4 实现项有 `[x]`  
2. 四 command id 在 `analysis_commands.cpp`  
3. 各 `p8_*.md` 含 Primary URL / 访问日期  
4. CMake 含 `algorithm_wave8_track_test`  
5. wiring-index + acceptance 含 Wave-8  
6. 测试文件含四 id + formula_reference 标记  
7. 四专用 dialog 源文件存在且 `QStackedWidget` 页数 ≥ 最低  
8. Binary DOE / GLM 三因子 **独立** dialog（非合并到 W7 对话框）

---

## §7 交付物

| 产物 | 路径 |
|------|------|
| 调研 | `docs/research/algorithm-wave8-market-formula-research-2026-08-28.md` |
| 本计划 + Mega 提示词 | 本文件 |
| DoD | `docs/research/goal-wave-2026-08-28-algorithm-wave8.md` |
| 每项 research | `docs/research/p8_binary_response_doe.md` 等 |
| 测试 | `tests/algorithm_wave8_track_test.cpp` |
| Verify | `tools/verify_algorithm_wave8_track.py` |
| 登记 | backlog §15.4、wiring-index、acceptance §2、taxonomy map |

---

## §8 如何继续开发（给人类）

1. **新开对话**，整段粘贴下方 **§9 Mega `/goal` 提示词**。  
2. 脚本门绿后，Qt Creator Rebuild，按四命令手测。  
3. Wave-8 完成后开 **Wave-9**（见 research §4：Expanded Gage、Mixture 过程变量、Split-plot…）。  
4. Parallel/Bubble 图建议 **另开 Track G Goal**，勿与算法 Wave 混会话。  

---

## §9 Mega `/goal` 提示词（复制到新对话整段粘贴）

````markdown
/goal

## 身份与总目标
你是 DataLab Orchestrator。用 `/goal` 模式一次做完 **算法 Wave-8**。
**W8-1 + W8-2 + W8-3 + W8-4 全部完整竖切且脚本门 PASS 才允许 UpdateGoal complete。**
禁止缩小范围、禁止只做 1 项、禁止中途换模型、禁止用「文档更新」冒充算法完成。

工作区：DataLab（Qt/C++ 汽车质量桌面工具）
人手门：中文路径 — **禁止** agent 强跑 cmake/ctest；完成后告知用户 Qt Creator Rebuild 手测点。
**不要** commit/push，除非用户明确要求。

「实现所有算法」在本产品的正确含义：按 backlog 产品范围内 ❌ 滚动清空。
**本 Goal 只锁定下面 4 项，但必须 4 项都做完**；其余登记到 Wave-9+，禁止假装 Feature List 已 100% 克隆。

---

## 本 Goal 做什么（锁定 4 项）

权威调研：`docs/research/algorithm-wave8-market-formula-research-2026-08-28.md`  
权威计划：`docs/research/goal-wave-2026-08-28-algorithm-wave8-plan-and-mega-prompt.md`  
DoD：`docs/research/goal-wave-2026-08-28-algorithm-wave8.md`

| ID | command_id | 交付 |
|----|------------|------|
| **W8-1** | `binary_response_doe` | 析因二值响应；Logit IRWLS；events/trials 或 0/1；Coef+OR+拟合；**独立**多页对话框 |
| **W8-2** | `cluster_variables` | 变量层次聚类；相关距离；dendrogram + amalgamation；**独立**对话框 |
| **W8-3** | `glm_three_factor` | 三因子不平衡 GLM；Type III Adj SS；Fitted Means；无 ABC 三阶交互 |
| **W8-4** | `life_data_regression` | Weibull MLE；1～2 协变量；右删失；回归表+可选百分位 |

每项竖切模板（缺一不可）：
`p8_*.md`（含 Primary URL + 访问日期 + 公式）
→ domain（纯 C++）
→ `*Facts`
→ AnalysisService
→ analysis_commands（**必须** menu_path + menu_group）
→ **专用 QDialog（QStackedWidget ≥4 页）**
→ interpretation（只读 Facts）
→ serialization
→ `# source: formula_reference` 测试
→ help catalog（禁止正文「见 md」）
→ wiring-index + backlog 状态 + acceptance §2

备选替换仅当 Planner 证明原项不可行（见 research §2 备选表）；**不得减项**。

Verify：`tools/verify_algorithm_wave8_track.py`  
测试：`tests/algorithm_wave8_track_test.cpp`

---

## 明确不做
- G3 Graph Builder 全量、可旋转 3D、G5 架构大拆
- Mixed REML / MANOVA 全量、Expanded Gage 不平衡全量、GLM 协变量全量
- Mixture 过程变量/extreme-vertices；binary DOE probit/gompit 全量
- TreeNet / AutoML / 商标级 RF 数值对齐
- 嵌 R/Python；sklearn 进 dist
- Minitab 数值 golden / VALIDATION_MATRIX 伪造
- Automated Capability / Assistant
- 破坏导入契约；重做 Wave-2～7 已 ✅ 算法
- **禁止**把 Binary DOE 并入 `logistic_regression` 对话框
- **禁止**把三因子 GLM 并入 `glm_two_way` 对话框「加第三因子」

## 已完成水位（勿重做）
P0–P2 主项；Wave-2～7（含 `mixture_analyze`/`glm_two_way`/`analyze_variability`/`factor_analysis`/`taguchi_analyze`/`mixture_design`/`random_forest`/`weibayes` 等）；
G1/G2/G6/G9（公式代入/分步求值——新命令有公式时应可挂 trace）。

---

## 必读（严格按序）
1. `docs/research/goal-wave-2026-08-28-algorithm-wave8-plan-and-mega-prompt.md`
2. `docs/research/algorithm-wave8-market-formula-research-2026-08-28.md`
3. `docs/research/goal-execution-framework.md`（尤其 §6 禁止偷懒）
4. `docs/research/minitab-market-algorithm-backlog.md`
5. `docs/research/next-wave-algorithms-charts-ml-oss.md`
6. `docs/research/deferred-capability-agreement.md`
7. Wave-7 范例 `docs/research/p7_*.md` + 对话框 `glm_two_way_dialog` / `mixture_analyze_dialog`
8. 邻域：`logistic_regression.*`、`doe_factorial.*`、`glm_two_way.*`、`cluster_observations.*`、`reliability.*`、`accelerated_life.*`
9. `src/application/analysis_service.*`、`src/ui/analysis_commands.*`、`mainwindow.cpp` 拦截模式
10. 导入与 `row_visibility`（complete-case、source_row、A→B）
11. `docs/algorithm-wiring-index.md`、`samples/product_evolution/unified_track_acceptance_plan.md` §2

Implementer 必载：`.agents/skills/cpp-coding/SKILL.md`  
禁止误读：`待修改.md`、`build/`

执行前：**再次用 WebSearch/WebFetch 核对**每项 Primary URL（Minitab Methods and Formulas / NIST），把最终公式写入对应 `p8_*.md`，访问日期写 **当天**。

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
1. **该新建页面就新建页面**：列选择 ≠ 模型选项 ≠ 方法说明 ≠ 预览确认 ≠ 结果解读。
2. **禁止单页堆积大量控件**；禁止把层次不一致的内容塞同一页。
3. Binary DOE：**event/trial 布局**必须独立页；禁止与 OR 结果表同页输入。
4. Cluster Variables：**变量选择**与**距离/连结**分页；dendrogram 在输出区。
5. GLM 三因子：**三因子列选择**与**两两交互开关**分页；禁止与 `glm_two_way` 混对话框。
6. Life regression：**时间/删失**与**协变量**分页；分布选项独立页。
7. 新对话框参考 Wave-7 多页模式；控件分组清晰；默认值合理。

---

## 四角色团队（互相监督 · 串行门禁）

| 顺序 | 角色 | subagent | 必须产出 | 门禁 |
|------|------|----------|----------|------|
| 1 | **Planner** | Task explore | 4 项映射表（文件归属+**UI分页线框**+表形+≥3测例+导入影响+Primary URL）；确认无 id 冲突 | 无表禁止写 domain |
| 2 | **Implementer** | Task generalPurpose | 按 W8-1→4 顺序完整竖切；加载 cpp-coding | 禁止跳过 help/interp/测试；禁止单页堆 UI |
| 3 | **Tester** | Task shell | wave8 测试 + verify PASS；回归 wave7 + wave6 + menuIA | verify 未 PASS 禁止 Checker |
| 4 | **Checker** | bugbot 或主 agent | Diff vs DoD；无 Critical；抽查 UI 分页 | Critical→Implementer |

子 Agent 结尾：`文件列表 | DoD [x/ ] | UI页数 | 风险一行 | 是否破坏导入A→B / wave7 verify`

Orchestrator：CreateGoal 一次；立刻开干；连续到 4 项全完；仅 Checker APPROVE + verify PASS 才 complete。

---

## 禁止偷懒
`goal-execution-framework.md` §6（1–13）+ 本计划 §5.2（14–32）。
重点：禁止单算法 complete；禁止 golden；禁止嵌 Python；禁止破坏导入；禁止无 Primary URL；**禁止单页堆控件**；禁止 Binary DOE/三因子 GLM 混对话框；禁止只做菜单壳。

---

## 如何验收

### 脚本门
```powershell
python tools/verify_algorithm_wave8_track.py
python tools/verify_algorithm_wave7_track.py
python tools/verify_algorithm_wave6_track.py
python tools/verify_ui_menu_ia_track.py
```

### 人手门（告知用户，勿代跑 Qt）
Rebuild 后分别打开：
1. Binary Response DOE — 2×2 因子 pass/fail，看 Coef + Odds Ratio  
2. Cluster Variables — 多变量，看 dendrogram + amalgamation  
3. 三因子 GLM — 不平衡数据，看 Type III 与 Fitted Means  
4. Life Data Regression — 时间+删失+协变量，看回归表与百分位  

---

## 如何记录
1. `goal-wave-2026-08-28-algorithm-wave8.md` DoD 全 `[x]`
2. backlog 对应行 ✅/⚪；§15.4 增 Wave-8 锁定表
3. wiring-index + acceptance §2 增 Wave-8 行
4. 每项 `p8_*.md` 含访问日期与 Primary URL 与公式
5. 结束告知用户文件清单与 Rebuild 手测点；**不**擅自 commit/push

---

## 启动顺序（本回合立刻执行）
1. CreateGoal（写清 W8-1～4 + verify）
2. TodoWrite（含四角色）
3. Planner explore（无计划表禁止改代码）
4. 网上再搜一遍公式，写/补 `p8_*.md`
5. Implementer → Tester → Checker
6. 全绿 UpdateGoal complete

中间不要换模型。开始执行。
````

---

**文档状态：** 2026-08-28 首版；供下一阶段 `/goal` 整段复制。
