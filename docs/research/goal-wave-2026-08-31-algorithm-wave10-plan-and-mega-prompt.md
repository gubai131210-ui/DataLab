# Wave：算法 Wave-10（General MANOVA + Mixed REML + Binary Probit + Life Lognormal）计划与 Mega `/goal` 提示词（2026-08-31）

> 访问日期：2026-08-31（UTC+8）  
> 调研正文：[`algorithm-wave10-market-formula-research-2026-08-31.md`](algorithm-wave10-market-formula-research-2026-08-31.md)  
> 执行框架：[`goal-execution-framework.md`](goal-execution-framework.md)  
> 状态权威：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) §4 / §10 / §15  
> DoD：[`goal-wave-2026-08-31-algorithm-wave10.md`](goal-wave-2026-08-31-algorithm-wave10.md)  
> **不做本 Goal：** Mixed REML 全量、General MANOVA 全量预测链、TreeNet/AutoML、嵌 R/Python、Minitab golden、Correspondence 全量、Mixture extreme-vertices、Graph Builder 全量、「一次做完 Feature List 全部 ❌」

---

## §0 给 Orchestrator / 人类的一页摘要

| 维度 | 内容 |
|------|------|
| **Goal 名称** | 算法 Wave-10：4 项竖切（`general_manova` / `mixed_effects_reml` / `binary_doe_probit` / `life_data_lognormal`） |
| **为何这 4 项** | W9 补单因子 MANOVA；W8 仅 Logit 二值 DOE + Weibull 寿命回归。本波补 **General MANOVA 窄化** + **Mixed REML** + **Probit/Gompit DOE** + **Lognormal 寿命回归**——对齐 backlog §4 ANOVA/DOE/可靠性最大剩余缝 |
| **Wave 数** | **1 个 Wave；4 项全部 ✅ 才 complete** |
| **交付门** | `python tools/verify_algorithm_wave10_track.py` PASS |
| **人手门** | 用户 Qt Creator Rebuild；agent **禁止**强跑 cmake/ctest（中文路径） |
| **四角色** | Planner → Implementer → Tester → Checker（串行门禁、互相监督） |
| **「全部算法」口径** | 本 Goal **清空锁定 4 项**；全产品 ❌ 由 Wave-11+ 滚动 |

### 用户诉求映射

| 诉求 | 本计划如何落实 |
|------|----------------|
| 算法扩展 + 深化 | MANOVA 单因子→多因子；Mixed 从 0→REML 窄化；二值 DOE link 扩展；寿命 Weibull→Lognormal |
| 一次多做 | 锁定 **4 项**同 Goal |
| 网上公式 + Minitab 输出 | research + 每项 `p10_*.md` + Primary URL（执行前再搜） |
| 导入衔接 | §3 + Tester 强制回归 wave9 |
| 四角色互相监督 | §4 串行门禁 |
| 足够测试 | domain/service + Wave verify + formula_reference + 回归 |
| **UI 不堆控件** | §5 UI 硬规则；每命令 **≥4 页**，层次分离 |
| 执行 agent 很笨还很懒 | §5 禁止偷懒 14–40；Checker 对照 DoD |
| 公式与来源页 | help catalog + G9 trace（有公式则挂） |

---

## §1 框架结构（团队 Agent）

```
Orchestrator（/goal 主对话）
  CreateGoal → TodoWrite(W10-1..4 + 四角色)
       │
       ▼
  Planner (Task explore) ──门禁──► 4 项映射表 + UI 线框 + 测例 + Primary URL
       │                              禁止写 domain
       ▼
  Implementer (Task generalPurpose) ── W10-1→4 顺序竖切；加载 cpp-coding
       │  每项：p10_*.md → domain → Facts → service → commands(Menu IA)
       │       → **专用多页 QDialog** → interpretation → serialization → tests → help
       ▼
  Tester (Task shell) ── verify_algorithm_wave10_track.py PASS
       │                 + 回归 wave9 + wave8 + menuIA
       ▼
  Checker (bugbot/自查) ── Diff vs DoD；UI 分页抽查；Critical→Implementer
       │
       ▼
  UpdateGoal complete（仅全绿）
```

**子 Agent 统一结尾模板：**

```text
文件列表 | DoD [x/ ] | UI页数确认 | 风险一行 | 是否破坏导入A→B / wave9 verify
```

### 四角色互相监督（硬门禁）

| 角色 | subagent | 监督谁 | 必须产出 | 未过则 |
|------|----------|--------|----------|--------|
| **Planner** | `Task explore` | Orchestrator | 4 项映射表：**UI 分页线框**（每页控件清单）、表形、≥3 测例/项、导入影响、Primary URL、id 无冲突 | 无表 → Implementer **禁止**写 domain |
| **Implementer** | `Task generalPurpose` | Planner + DoD | 4 项完整竖切；**每命令专用 QDialog ≥4 页** | 跳过 help/interp/测试 → Tester **拒绝** |
| **Tester** | `Task shell` | Implementer | wave10 测试 + verify PASS；回归 wave9/wave8/menuIA | verify 未 PASS → Checker **禁止**放行 |
| **Checker** | `bugbot` 或主 agent | 全员 | Diff vs DoD；无 Critical；**UI 无单页堆积** | Critical → **退回** Implementer |

---

## §2 锁定项摘要

| ID | command_id | DoD 要点 |
|----|------------|----------|
| W10-1 | `general_manova` | 2～4 响应；1～2 因子；可选协变量；四检验；≠ `manova_one_way` 对话框 |
| W10-2 | `mixed_effects_reml` | 1 随机 + 1～2 固定；REML VarComp；≠ 单页 GLM 壳 |
| W10-3 | `binary_doe_probit` | Probit/Gompit IRWLS；≠ `binary_response_doe` 对话框 |
| W10-4 | `life_data_lognormal` | Lognormal MLE + 删失；≠ `life_data_regression` 对话框 |

详情与公式 URL：调研文档 §2；每项 `p10_*.md` 已预写公式入口（执行前须再 WebFetch 核对）。

---

## §2.5 Planner 映射表（Implementer 开工前必须核对/补全）

| ID | 新建 domain | 新建 dialog | 修改共享 | menu_path / menu_group | 邻域复用 |
|----|-------------|-------------|----------|------------------------|----------|
| W10-1 | `general_manova.*` | `general_manova_dialog.*` | service/commands/mainwindow/serialization/part21/help | 统计 / **ANOVA** | `manova_one_way`、`glm_three_factor` |
| W10-2 | `mixed_effects_reml.*` | `mixed_effects_reml_dialog.*` | 同上 | 统计 / **ANOVA** | `glm_three_factor`、`glm_two_way` |
| W10-3 | `binary_doe_probit.*` | `binary_doe_probit_dialog.*` | 同上 | 统计 / **DOE** | `binary_response_doe`、`doe_factorial` |
| W10-4 | `life_data_lognormal.*` | `life_data_lognormal_dialog.*` | 同上 | 统计 / **可靠性** | `life_data_regression`、`accelerated_life` |

**共享必改：** `CMakeLists.txt`、`quality_types.h`、`report_text_catalog_part21.cpp`（+ parts.h/cpp）、`algorithm-wiring-index.md`、`backlog §15.6`、`unified_track_acceptance_plan.md §2`

**UI 线框（Checker 硬查 ≥4 页）：**

| 命令 | P1 | P2 | P3 | P4 |
|------|----|----|----|-----|
| general_manova | 多响应+因子(1～2) | 协变量+模型交互 | 四检验+SSCP方法 | 预览 |
| mixed_effects_reml | 响应+随机因子 | 固定因子+协变量 | REML方法说明 | 预览 |
| binary_doe_probit | 因子+Events/Trials | Link(probit/gompit) | IRWLS方法 | 预览 |
| life_data_lognormal | 时间+删失+协变量 | Lognormal选项 | MLE方法 | 预览 |

**测例（每项 ≥3）：**

- W10-1：2 响应 × 2 因子玩具 Wilks；单水平 error；Facts 序列化  
- W10-2：1 随机区组 REML VarComp；收敛失败 diagnostic；source_row  
- W10-3：2×2 probit F 可算；gompit link 切换；≠ binary_response_doe dialog  
- W10-4：Lognormal 右删失 MLE；时间≤0 error；≠ life_data_regression dialog  

---

## §3 优先阅读清单（严格顺序）

| # | 路径 | 用途 |
|---|------|------|
| 1 | **本文件** + [`algorithm-wave10-market-formula-research-2026-08-31.md`](algorithm-wave10-market-formula-research-2026-08-31.md) | 锁定与公式 |
| 2 | [`goal-execution-framework.md`](goal-execution-framework.md) §6 | 禁止偷懒 |
| 3 | [`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) | §4 ANOVA / §10 多元 / 可靠性 |
| 4 | [`next-wave-algorithms-charts-ml-oss.md`](next-wave-algorithms-charts-ml-oss.md) | Track H 候补 |
| 5 | [`deferred-capability-agreement.md`](deferred-capability-agreement.md) | 勿做 |
| 6 | Wave-9 范例：`p9_*.md` + `goal-wave-2026-08-28-algorithm-wave9-plan-and-mega-prompt.md` | 最新竖切与 UI 分页 |
| 7 | Wave-9 邻域：`manova_one_way.*`、`expanded_gage_unbalanced.*`、`split_plot_analyze.*` | W9 模式 |
| 8 | Wave-8 邻域：`binary_response_doe.*`、`life_data_regression.*`、`glm_three_factor.*` | link/寿命/GLM |
| 9 | UI 范例：`manova_one_way_dialog.*`、`binary_response_doe_dialog.*`、`life_data_regression_dialog.*` | QStackedWidget 多页 |
| 10 | `src/application/analysis_service.*`、`src/ui/analysis_commands.*`、`mainwindow.cpp` | 接线 |
| 11 | 导入与 `row_visibility` | complete-case、source_row、A→B |
| 12 | `docs/algorithm-wiring-index.md`、`unified_track_acceptance_plan.md` §2 | 登记 |
| 13 | 回归：`tools/verify_algorithm_wave9_track.py`、wave8、menuIA | 不得破坏 |

Implementer：**必载** `.agents/skills/cpp-coding/SKILL.md`  
**禁止误读：** `待修改.md`、`build/`

执行前：**WebSearch/WebFetch 核对**每项 Primary URL，公式写入 `p10_*.md`，访问日期 **当天**。

---

## §4 四角色门禁（互相监督）

| 顺序 | 角色 | subagent | 必须产出 | 未过禁止进入下一角色 |
|------|------|----------|----------|----------------------|
| 1 | **Planner** | explore | 4 项映射表：**UI 分页线框**、表形、≥3 测例/项、导入影响、Primary URL、id 无冲突 | 无表禁止写 domain |
| 2 | **Implementer** | generalPurpose | 4 项完整竖切；**每命令专用 QDialog ≥4 页** | 禁止跳过 help/interp/测试 |
| 3 | **Tester** | shell | `algorithm_wave10_track_test.cpp` + verify PASS；回归 wave9/wave8/menuIA | verify 未 PASS 禁止 Checker |
| 4 | **Checker** | bugbot/自查 | Diff vs DoD；无 Critical；**UI 无单页堆积** | Critical → Implementer |

---

## §5 禁止偷懒

### 5.1 通用

粘贴并遵守 `goal-execution-framework.md` **§6（1–13）** 全文。

### 5.2 Wave-10 增补（防笨防懒）

14. 禁止只做 1～2 个算法就 `UpdateGoal complete`  
15. 禁止跳过 Primary URL / `p10_*.md`  
16. 禁止 Minitab/JMP **数值**当 golden  
17. 禁止破坏 complete-case / `source_row` / A→B  
18. 禁止解释层「过程已失控 / 已证明稳定 / 批次合格 / 测量系统合格」  
19. 禁止 sklearn/R/Python 打进 dist  
20. 禁止宣称数值等同 Minitab General MANOVA / Mixed REML / Probit / Lognormal  
21. 禁止混入 G3 Graph Builder / G5 大拆  
22. 禁止无 `menu_path`/`menu_group` 的新命令  
23. **禁止把 General MANOVA 塞进 `manova_one_way` 对话框「加第二因子勾选」**  
24. **禁止把 Probit DOE 塞进 `binary_response_doe` 对话框「加 link 下拉」**  
25. **禁止把 Lognormal 塞进 `life_data_regression` 对话框「加分布下拉」**  
26. **禁止 Mixed REML 做成无随机项的 GLM 壳**  
27. **禁止单页堆：列选择 + 模型阶 + 误差项 + 结果表 + 图**  
28. **禁止层次不一致内容同页**（例：随机因子选择 vs VarComp 表 vs 优化建议）  
29. General MANOVA：**多响应**与**因子/协变量**必须分页  
30. Mixed REML：**随机因子**与**固定因子**必须分页  
31. Binary Probit：**因子布局**与 **link 选择**分页  
32. Life Lognormal：**删失列**与**分布/MLE 方法**分页  
33. 禁止只有菜单壳、无 domain/Facts/测试  
34. 禁止用「文档更新」冒充算法完成  
35. 禁止 Mixed REML 全量（多随机项）冒充窄化完成  
36. 禁止 General MANOVA stored model 预测链冒充完成  
37. Planner 映射表未产出时 Implementer **不得**写第一行 domain 代码  
38. 禁止破坏 Wave-9 verify 回归  
39. 禁止 correspondence / nonlinear 等 Wave-11 项偷换锁定 4 项  
40. 每项必须 `page.analysis_command_id = "<command_id>"`  

### 5.3 UI 分页最低要求（Checker 硬查）

| 命令 | 最少页数 | 分页线框 |
|------|----------|----------|
| `general_manova` | **4** | 多响应+因子 \| 协变量+模型 \| 检验+SSCP \| 预览 |
| `mixed_effects_reml` | **4** | 响应+随机 \| 固定+协变量 \| REML方法 \| 预览 |
| `binary_doe_probit` | **4** | 因子+响应 \| Link \| IRWLS \| 预览 |
| `life_data_lognormal` | **4** | 时间+删失+协变量 \| 分布选项 \| MLE方法 \| 预览 |

实现模式：`QDialog` + `QStackedWidget` + Back/Next/Run；参考 Wave-9 对话框；`MainWindow::run_from_spec` 拦截四 id。

---

## §6 测试与 verify

### 6.1 每项最低

- domain 或 service 测：主路径 + 门禁失败诊断  
- `# source: formula_reference` ≥1  
- `find(command_id)` 非空；Menu IA 四顶层合法  
- General MANOVA：2 因子交互可估；四检验可算  
- Mixed REML：VarComp 可估；固定效应表非空  
- Binary Probit：probit 与 gompit 至少各 1 测例路径  
- Life Lognormal：删失 MLE 收敛或明确 diagnostic  

### 6.2 Wave 脚本门

```powershell
python tools/verify_algorithm_wave10_track.py
python tools/verify_algorithm_wave9_track.py
python tools/verify_algorithm_wave8_track.py
python tools/verify_ui_menu_ia_track.py
```

`verify_algorithm_wave10_track.py` 至少检查：

1. research + DoD md 存在且 W10-1～4 实现项有 `[x]`  
2. 四 command id 在 `analysis_commands.cpp`  
3. 各 `p10_*.md` 含 Primary URL / 访问日期  
4. CMake 含 `algorithm_wave10_track_test`  
5. wiring-index + acceptance 含 Wave-10  
6. 测试文件含四 id + formula_reference 标记  
7. 四专用 dialog 源文件存在且 `QStackedWidget` 页数 ≥ 最低  
8. **独立** dialog（非合并到 W8/W9 既有对话框）

---

## §7 交付物

| 产物 | 路径 |
|------|------|
| 调研 | `docs/research/algorithm-wave10-market-formula-research-2026-08-31.md` |
| 本计划 + Mega 提示词 | 本文件 |
| DoD | `docs/research/goal-wave-2026-08-31-algorithm-wave10.md` |
| 每项 research | `docs/research/p10_*.md` |
| 测试 | `tests/algorithm_wave10_track_test.cpp` |
| Verify | `tools/verify_algorithm_wave10_track.py` |
| 登记 | backlog §15.6、wiring-index、acceptance §2、taxonomy map |

---

## §8 如何继续开发（给人类）

1. **新开对话**，整段粘贴下方 **§9 Mega `/goal` 提示词**。  
2. 脚本门绿后，Qt Creator Rebuild，按四命令手测。  
3. Wave-10 完成后开 **Wave-11**（见 research §4：Correspondence、Nonlinear、Mixture extreme-vertices…）。  
4. Parallel/Bubble 图建议 **另开 Track G Goal**，勿与算法 Wave 混会话。  

---

## §9 Mega `/goal` 提示词（复制到新对话整段粘贴）

````markdown
/goal

## 身份与总目标
你是 DataLab Orchestrator。用 `/goal` 模式一次做完 **算法 Wave-10**。
**W10-1 + W10-2 + W10-3 + W10-4 全部完整竖切且脚本门 PASS 才允许 UpdateGoal complete。**
禁止缩小范围、禁止只做 1 项、禁止中途换模型、禁止用「文档更新」冒充算法完成。

工作区：DataLab（Qt/C++ 汽车质量桌面工具）
人手门：中文路径 — **禁止** agent 强跑 cmake/ctest；完成后告知用户 Qt Creator Rebuild 手测点。
**不要** commit/push，除非用户明确要求。

「实现所有算法」在本产品的正确含义：按 backlog 产品范围内 ❌ 滚动清空。
**本 Goal 只锁定下面 4 项，但必须 4 项都做完**；其余登记到 Wave-11+，禁止假装 Feature List 已 100% 克隆。

---

## 本 Goal 做什么（锁定 4 项）

权威调研：`docs/research/algorithm-wave10-market-formula-research-2026-08-31.md`  
权威计划：`docs/research/goal-wave-2026-08-31-algorithm-wave10-plan-and-mega-prompt.md`  
DoD：`docs/research/goal-wave-2026-08-31-algorithm-wave10.md`

| ID | command_id | 交付 |
|----|------------|------|
| **W10-1** | `general_manova` | General MANOVA 窄化；2～4 响应；1～2 因子；可选协变量；Wilks/Pillai/LH/Roy；**独立**多页对话框 |
| **W10-2** | `mixed_effects_reml` | 1 随机 + 1～2 固定；REML 方差分量；**独立**对话框 |
| **W10-3** | `binary_doe_probit` | 析因二值 Probit/Gompit IRWLS；**独立**对话框 |
| **W10-4** | `life_data_lognormal` | Lognormal 寿命回归 MLE + 删失；**独立**对话框 |

每项竖切模板（缺一不可）：
`p10_*.md`（含 Primary URL + 访问日期 + 公式）
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

Verify：`tools/verify_algorithm_wave10_track.py`  
测试：`tests/algorithm_wave10_track_test.cpp`

---

## 明确不做
- G3 Graph Builder 全量、可旋转 3D、G5 架构大拆
- Mixed REML **全量**（多随机项/嵌套/交叉）
- General MANOVA **全量**（stored model、任意高阶）
- Correspondence / Nonlinear / PLS 全量（Wave-11+）
- TreeNet / AutoML / 商标级 RF 数值对齐
- 嵌 R/Python；sklearn 进 dist
- Minitab 数值 golden / VALIDATION_MATRIX 伪造
- Automated Capability / Assistant
- 破坏导入契约；重做 Wave-2～9 已 ✅ 算法
- **禁止** General MANOVA 并入 `manova_one_way` 对话框
- **禁止** Probit DOE 并入 `binary_response_doe` 对话框
- **禁止** Lognormal 并入 `life_data_regression` 对话框

## 已完成水位（勿重做）
P0–P2 主项；Wave-2～9（含 W9 四项 + W8 四项 + W7 mixture/glm 等）；
G1/G2/G6/G9（公式 trace——新命令有公式时应可挂 trace）。

---

## 必读（严格按序）
1. `docs/research/goal-wave-2026-08-31-algorithm-wave10-plan-and-mega-prompt.md`
2. `docs/research/algorithm-wave10-market-formula-research-2026-08-31.md`
3. `docs/research/goal-execution-framework.md`（尤其 §6 禁止偷懒）
4. `docs/research/minitab-market-algorithm-backlog.md`
5. `docs/research/next-wave-algorithms-charts-ml-oss.md`
6. `docs/research/deferred-capability-agreement.md`
7. Wave-9 范例 `docs/research/p9_*.md` + 对话框 `manova_one_way_dialog` / `expanded_gage_unbalanced_dialog`
8. Wave-8 邻域：`binary_response_doe.*`、`life_data_regression.*`、`glm_three_factor.*`
9. `src/application/analysis_service.*`、`src/ui/analysis_commands.*`、`mainwindow.cpp` 拦截模式
10. 导入与 `row_visibility`（complete-case、source_row、A→B）
11. `docs/algorithm-wiring-index.md`、`samples/product_evolution/unified_track_acceptance_plan.md` §2

Implementer 必载：`.agents/skills/cpp-coding/SKILL.md`  
禁止误读：`待修改.md`、`build/`

执行前：**再次用 WebSearch/WebFetch 核对**每项 Primary URL（Minitab Methods and Formulas / NIST），把最终公式写入对应 `p10_*.md`，访问日期写 **当天**。

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
3. General MANOVA：**多响应**与**因子/协变量**必须独立页；禁止与 SSCP 结果表同页输入。
4. Mixed REML：**随机因子**与**固定因子**分页；REML 方法说明独立页。
5. Binary Probit：**因子布局**与 **link 选择**分页；禁止与 `binary_response_doe` 混对话框。
6. Life Lognormal：**删失/时间**与**分布/MLE 方法**分页；禁止与 `life_data_regression` 混对话框。
7. 新对话框参考 Wave-9 多页模式；控件分组清晰；默认值合理。

---

## 四角色团队（互相监督 · 串行门禁）

| 顺序 | 角色 | subagent | 必须产出 | 门禁 |
|------|------|----------|----------|------|
| 1 | **Planner** | Task explore | 4 项映射表（文件归属+**UI分页线框**+表形+≥3测例+导入影响+Primary URL）；确认无 id 冲突 | 无表禁止写 domain |
| 2 | **Implementer** | Task generalPurpose | 按 W10-1→4 顺序完整竖切；加载 cpp-coding | 禁止跳过 help/interp/测试；禁止单页堆 UI |
| 3 | **Tester** | Task shell | wave10 测试 + verify PASS；回归 wave9 + wave8 + menuIA | verify 未 PASS 禁止 Checker |
| 4 | **Checker** | bugbot 或主 agent | Diff vs DoD；无 Critical；抽查 UI 分页 | Critical→Implementer |

子 Agent 结尾：`文件列表 | DoD [x/ ] | UI页数 | 风险一行 | 是否破坏导入A→B / wave9 verify`

Orchestrator：CreateGoal 一次；立刻开干；连续到 4 项全完；仅 Checker APPROVE + verify PASS 才 complete。

---

## 禁止偷懒
`goal-execution-framework.md` §6（1–13）+ 本计划 §5.2（14–40）。
重点：禁止单算法 complete；禁止 golden；禁止嵌 Python；禁止破坏导入；禁止无 Primary URL；**禁止单页堆控件**；禁止 W8/W9 混对话框；禁止只做菜单壳。

---

## 如何验收

### 脚本门
```powershell
python tools/verify_algorithm_wave10_track.py
python tools/verify_algorithm_wave9_track.py
python tools/verify_algorithm_wave8_track.py
python tools/verify_ui_menu_ia_track.py
```

### 人手门（告知用户，勿代跑 Qt）
Rebuild 后分别打开：
1. General MANOVA — 2 响应 2 因子，看 Wilks/Pillai 与组均值向量  
2. Mixed REML — 1 随机区组 + 1 固定因子，看 VarComp + 固定效应  
3. Binary DOE Probit — 2×2 析因，probit link，看系数与拟合  
4. Life Lognormal — 右删失 + 1 协变量，看 lognormal 回归表  

---

## 如何记录
1. `goal-wave-2026-08-31-algorithm-wave10.md` DoD 全 `[x]`
2. backlog 对应行 ✅/⚪；§15.6 增 Wave-10 锁定表
3. wiring-index + acceptance §2 增 Wave-10 行
4. 每项 `p10_*.md` 含访问日期与 Primary URL 与公式
5. 结束告知用户文件清单与 Rebuild 手测点；**不**擅自 commit/push

---

## 启动顺序（本回合立刻执行）
1. CreateGoal（写清 W10-1～4 + verify）
2. TodoWrite（含四角色）
3. Planner explore（无计划表禁止改代码）
4. 网上再搜一遍公式，写/补 `p10_*.md`
5. Implementer → Tester → Checker
6. 全绿 UpdateGoal complete

中间不要换模型。开始执行。
````

---

**文档状态：** 2026-08-31 初稿；供 `/goal` 整段复制 §9。
