# Wave：算法 Wave-9（不平衡 Gage + 裂区 + Mixture 过程变量 + MANOVA）计划与 Mega `/goal` 提示词（2026-08-28）

> 访问日期：2026-08-31（UTC+8）  
> 调研正文：[`algorithm-wave9-market-formula-research-2026-08-28.md`](algorithm-wave9-market-formula-research-2026-08-28.md)  
> 执行框架：[`goal-execution-framework.md`](goal-execution-framework.md)  
> 状态权威：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) §7 / §8 / §10 / §15  
> DoD：[`goal-wave-2026-08-28-algorithm-wave9.md`](goal-wave-2026-08-28-algorithm-wave9.md)  
> **不做本 Goal：** Mixed REML/MANOVA 全量、General MANOVA 协变量、TreeNet/AutoML、嵌 R/Python、Minitab golden、Mixture extreme-vertices 设计、Graph Builder 全量、Binary DOE probit 全量、寿命多分布全量、「一次做完 Feature List 全部 ❌」

---

## §0 给 Orchestrator / 人类的一页摘要

| 维度 | 内容 |
|------|------|
| **Goal 名称** | 算法 Wave-9：4 项竖切（`expanded_gage_unbalanced` / `split_plot_analyze` / `mixture_process_variable` / `manova_one_way`） |
| **为何这 4 项** | W8 补了 DOE/多元/ANOVA/可靠性缝；本波补 **MSA 不平衡** + **裂区 DOE** + **Mixture 过程变量** + **单因子 MANOVA**——对齐 backlog 最大剩余 MSA/DOE/多元缝 |
| **Wave 数** | **1 个 Wave；4 项全部 ✅ 才 complete** |
| **交付门** | `python tools/verify_algorithm_wave9_track.py` PASS |
| **人手门** | 用户 Qt Creator Rebuild；agent **禁止**强跑 cmake/ctest（中文路径） |
| **四角色** | Planner → Implementer → Tester → Checker（串行门禁、互相监督） |
| **「全部算法」口径** | 本 Goal **清空锁定 4 项**；全产品 ❌ 由 Wave-10+ 滚动 |

### 用户诉求映射

| 诉求 | 本计划如何落实 |
|------|----------------|
| 算法扩展 + 深化 | Expanded Gage 平衡→不平衡；Mixture 纯组分→过程变量；补裂区/MANOVA |
| 一次多做 | 锁定 **4 项**同 Goal |
| 网上公式 + Minitab 输出 | research + 每项 `p9_*.md` + Primary URL（执行前再搜） |
| 导入衔接 | §3 + Tester 强制回归 wave8 |
| 四角色互相监督 | §4 串行门禁 |
| 足够测试 | domain/service + Wave verify + formula_reference + 回归 |
| **UI 不堆控件** | §5 UI 硬规则；每命令 **≥4 页**，层次分离 |
| 执行 agent 很笨还很懒 | §5 禁止偷懒 14–36；Checker 对照 DoD |
| 公式与来源页 | help catalog + G9 trace（有公式则挂） |

---

## §1 框架结构（团队 Agent）

```
Orchestrator（/goal 主对话）
  CreateGoal → TodoWrite(W9-1..4 + 四角色)
       │
       ▼
  Planner (Task explore) ──门禁──► 4 项映射表 + UI 线框 + 测例 + Primary URL
       │                              禁止写 domain
       ▼
  Implementer (Task generalPurpose) ── W9-1→4 顺序竖切；加载 cpp-coding
       │  每项：p9_*.md → domain → Facts → service → commands(Menu IA)
       │       → **专用多页 QDialog** → interpretation → serialization → tests → help
       ▼
  Tester (Task shell) ── verify_algorithm_wave9_track.py PASS
       │                 + 回归 wave8 + wave7 + menuIA
       ▼
  Checker (bugbot/自查) ── Diff vs DoD；UI 分页抽查；Critical→Implementer
       │
       ▼
  UpdateGoal complete（仅全绿）
```

**子 Agent 统一结尾模板：**

```text
文件列表 | DoD [x/ ] | UI页数确认 | 风险一行 | 是否破坏导入A→B / wave8 verify
```

### 四角色互相监督（硬门禁）

| 角色 | 监督谁 | 未过则 |
|------|--------|--------|
| Planner | Orchestrator | 无映射表 → Implementer **禁止**写 domain |
| Implementer | Planner + DoD | 跳过 help/interp/测试 → Tester **拒绝**验收 |
| Tester | Implementer | verify 未 PASS → Checker **禁止**放行 |
| Checker | 全员 | 发现 Critical → **退回** Implementer，不得 complete |

---

## §2 锁定项摘要

| ID | command_id | DoD 要点 |
|----|------------|----------|
| W9-1 | `expanded_gage_unbalanced` | 不平衡 GLM 方差分量；VarComp+%GRR；≠ `expanded_gage_rr` 对话框 |
| W9-2 | `split_plot_analyze` | 难改/易改因子；WP/SP 双误差；ANOVA；≠ `doe_factorial` 单页 |
| W9-3 | `mixture_process_variable` | Scheffé + 1 过程变量 + 可选交互；≠ `mixture_analyze` 对话框 |
| W9-4 | `manova_one_way` | 2～4 响应；Wilks/Pillai/LH/Roy；H/E SSCP；单因子 |

详情与公式 URL：调研文档 §2；每项 `p9_*.md` 已预写公式入口（执行前须再 WebFetch 核对）。

---

## §2.5 Planner 映射表（Implementer 开工前必须核对/补全）

| ID | 新建 domain | 新建 dialog | 修改共享 | menu_path / menu_group | 邻域复用 |
|----|-------------|-------------|----------|------------------------|----------|
| W9-1 | `expanded_gage_unbalanced.*` | `expanded_gage_unbalanced_dialog.*` | service/commands/mainwindow/serialization/part20/help | 统计 / **MSA** | `expanded_gage_rr`、`glm_three_factor` |
| W9-2 | `split_plot_analyze.*` | `split_plot_analyze_dialog.*` | 同上 | 统计 / **DOE** | `doe_factorial`、`glm_two_way`、`analyze_variability` |
| W9-3 | `mixture_process_variable.*` | `mixture_process_variable_dialog.*` | 同上 | 统计 / **DOE** | `mixture_analyze`、`mixture_design` |
| W9-4 | `manova_one_way.*` | `manova_one_way_dialog.*` | 同上 | 统计 / **ANOVA** | `glm_three_factor`、`one_way_anova`、`hotelling_t2` |

**共享必改：** `CMakeLists.txt`、`quality_types.h`、`report_text_catalog_part20.cpp`（+ parts.h/cpp）、`algorithm-wiring-index.md`、`backlog §15.5`、`unified_track_acceptance_plan.md §2`

**UI 线框（Checker 硬查 ≥4 页）：**

| 命令 | P1 | P2 | P3 | P4 |
|------|----|----|----|-----|
| expanded_gage_unbalanced | 测量+Part/Op/附加 | 随机/固定/嵌套 | GLM/VarComp 方法 | 预览 |
| split_plot_analyze | 响应+难改/易改+WP | 模型交互 | WP/SP 双误差方法 | 预览 |
| mixture_process_variable | 组分+响应+过程 | 模型阶/交互 | Scheffé 方法 | 预览 |
| manova_one_way | 多响应+因子 | 四检验选项 | SSCP 方法 | 预览 |

**测例（每项 ≥3）：**

- W9-1：不平衡 toy VarComp；水平不足 error；Facts 序列化  
- W9-2：平衡裂区 F 分母 WP/SP；WP residual；source_row  
- W9-3：组分和≈1 门禁；A×X 交互可估；≠ mixture_analyze dialog  
- W9-4：2 响应 3 组 Wilks；单组 error；特征值排序  

---

## §3 优先阅读清单（严格顺序）

| # | 路径 | 用途 |
|---|------|------|
| 1 | **本文件** + [`algorithm-wave9-market-formula-research-2026-08-28.md`](algorithm-wave9-market-formula-research-2026-08-28.md) | 锁定与公式 |
| 2 | [`goal-execution-framework.md`](goal-execution-framework.md) §6 | 禁止偷懒 |
| 3 | [`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) | §7 MSA / §8 DOE / §10 多元 |
| 4 | [`next-wave-algorithms-charts-ml-oss.md`](next-wave-algorithms-charts-ml-oss.md) | Track H 候补 |
| 5 | [`deferred-capability-agreement.md`](deferred-capability-agreement.md) | 勿做 |
| 6 | Wave-8 范例：`docs/research/p8_*.md` + `goal-wave-2026-08-28-algorithm-wave8-plan-and-mega-prompt.md` | 竖切与 UI 分页 |
| 7 | Wave-8 邻域：`binary_response_doe.*`、`glm_three_factor.*`、`cluster_variables.*`、`life_data_regression.*` | 最新竖切范例 |
| 8 | 本波邻域：`expanded_gage_rr.*`、`mixture_analyze.*`、`mixture_design.*`、`doe_factorial.*`、`glm_three_factor.*`、`one_way_anova.*`、`hotelling_t2.*`、`discriminant.*` | 复用模式 |
| 9 | UI 范例：`binary_response_doe_dialog.*`、`glm_three_factor_dialog.*`、`mixture_analyze_dialog.*` | QStackedWidget 多页 |
| 10 | `src/application/analysis_service.*`、`src/ui/analysis_commands.*`、`mainwindow.cpp` 命令拦截 | 接线 |
| 11 | 导入与 `row_visibility` | complete-case、source_row、A→B |
| 12 | `docs/algorithm-wiring-index.md`、`unified_track_acceptance_plan.md` §2 | 登记 |
| 13 | 回归：`tools/verify_algorithm_wave8_track.py`、wave7、menuIA | 不得破坏 |

Implementer：**必载** `.agents/skills/cpp-coding/SKILL.md`  
**禁止误读：** `待修改.md`、`build/`

执行前：**WebSearch/WebFetch 核对**每项 Primary URL，公式写入 `p9_*.md`，访问日期 **当天**。

---

## §4 四角色门禁（互相监督）

| 顺序 | 角色 | subagent | 必须产出 | 未过禁止进入下一角色 |
|------|------|----------|----------|----------------------|
| 1 | **Planner** | explore | 4 项映射表：**UI 分页线框**（每页控件清单）、表形、≥3 测例/项、导入影响、Primary URL、id 无冲突 | 无表禁止写 domain |
| 2 | **Implementer** | generalPurpose | 4 项完整竖切；**每命令专用 QDialog ≥4 页**；禁止 AnalysisSetupDialog 单页堆控件 | 禁止跳过 help/interp/测试 |
| 3 | **Tester** | shell | `algorithm_wave9_track_test.cpp` + `verify_algorithm_wave9_track.py` PASS；回归 wave8/wave7/menuIA | verify 未 PASS 禁止 Checker |
| 4 | **Checker** | bugbot/自查 | Diff vs DoD；无 Critical；**UI 无单页堆积**；命令未错误合并对话框 | Critical → Implementer |

---

## §5 禁止偷懒

### 5.1 通用

粘贴并遵守 `goal-execution-framework.md` **§6（1–13）** 全文。

### 5.2 Wave-9 增补（防笨防懒）

14. 禁止只做 1～2 个算法就 `UpdateGoal complete`  
15. 禁止跳过 Primary URL / `p9_*.md`  
16. 禁止 Minitab/JMP **数值**当 golden  
17. 禁止破坏 complete-case / `source_row` / A→B  
18. 禁止解释层「过程已失控 / 已证明稳定 / 批次合格 / 分布已正态 / 测量系统合格」  
19. 禁止 sklearn/R/Python 打进 dist  
20. 禁止宣称数值等同 Minitab Expanded Gage/Split-plot/Mixture/MANOVA  
21. 禁止混入 G3 Graph Builder / G5 大拆  
22. 禁止无 `menu_path`/`menu_group` 的新命令  
23. **禁止把不平衡 Gage 塞进 `expanded_gage_rr` 对话框「加勾选不平衡」**  
24. **禁止把 Mixture 过程变量塞进 `mixture_analyze` 对话框「加过程变量勾选」**  
25. **禁止把裂区分析塞进 `doe_factorial` 或 `glm_three_factor` 对话框**  
26. **禁止单页堆：列选择 + 模型阶 + 误差项 + 结果表 + 图**  
27. **禁止层次不一致内容同页**（例：Part/Operator 选择 vs VarComp 表 vs 优化建议）  
28. Expanded Gage：**因子角色（随机/固定）**与**测量列布局**必须分页  
29. Split-plot：**难改/易改因子**与**WP 指示**分页；双误差说明独立页  
30. Mixture 过程变量：**组分选择**与**过程变量**分页；模型交互开关独立页  
31. MANOVA：**多响应列选择**与**检验选项**分页；SSCP 方法说明独立页  
32. 禁止只有菜单壳、无 domain/Facts/测试  
33. 禁止用「文档更新」冒充算法完成  
34. 禁止 General MANOVA 全量冒充 `manova_one_way` 窄化完成  
35. 禁止 extreme-vertices 设计生成冒充 Mixture 分析完成  
36. Planner 映射表未产出时 Implementer **不得**写第一行 domain 代码  

### 5.3 UI 分页最低要求（Checker 硬查）

| 命令 | 最少页数 | 分页线框 |
|------|----------|----------|
| `expanded_gage_unbalanced` | **4** | 测量+Part/Operator/附加因子 \| 随机/固定/嵌套 \| GLM/方差分量方法 \| 预览 |
| `split_plot_analyze` | **4** | 响应+难改/易改+WP \| 模型(主效应/交互) \| 双误差项方法 \| 预览 |
| `mixture_process_variable` | **4** | 组分+响应+过程变量 \| 模型(组分阶/过程交互) \| Scheffé 方法 \| 预览 |
| `manova_one_way` | **4** | 多响应+因子 \| 检验选项(Wilks等) \| SSCP/MANOVA 方法 \| 预览 |

实现模式：`QDialog` + `QStackedWidget` + Back/Next/Run；参考 Wave-8 对话框；`MainWindow::run_from_spec` 拦截四 id（同 Wave-7/8）。

---

## §6 测试与 verify

### 6.1 每项最低

- domain 或 service 测：主路径 + 门禁失败诊断  
- `# source: formula_reference` ≥1  
- `find(command_id)` 非空；Menu IA 四顶层合法  
- Expanded Gage：不平衡单元格仍可估 VarComp；NDC 可算  
- Split-plot：难改因子 F 用 WP 误差；易改用 SP 误差  
- Mixture 过程变量：组分和≈1 门禁；过程交互项可估  
- MANOVA：2 响应 3 组玩具数据 Wilks/Pillai 可算  

### 6.2 Wave 脚本门

```powershell
python tools/verify_algorithm_wave9_track.py
python tools/verify_algorithm_wave8_track.py
python tools/verify_algorithm_wave7_track.py
python tools/verify_ui_menu_ia_track.py
```

`verify_algorithm_wave9_track.py` 至少检查：

1. research + DoD md 存在且 W9-1～4 实现项有 `[x]`  
2. 四 command id 在 `analysis_commands.cpp`  
3. 各 `p9_*.md` 含 Primary URL / 访问日期  
4. CMake 含 `algorithm_wave9_track_test`  
5. wiring-index + acceptance 含 Wave-9  
6. 测试文件含四 id + formula_reference 标记  
7. 四专用 dialog 源文件存在且 `QStackedWidget` 页数 ≥ 最低  
8. Expanded Gage / Mixture 过程变量 **独立** dialog（非合并到既有对话框）

---

## §7 交付物

| 产物 | 路径 |
|------|------|
| 调研 | `docs/research/algorithm-wave9-market-formula-research-2026-08-28.md` |
| 本计划 + Mega 提示词 | 本文件 |
| DoD | `docs/research/goal-wave-2026-08-28-algorithm-wave9.md` |
| 每项 research | `docs/research/p9_expanded_gage_unbalanced.md` 等 |
| 测试 | `tests/algorithm_wave9_track_test.cpp` |
| Verify | `tools/verify_algorithm_wave9_track.py` |
| 登记 | backlog §15.5、wiring-index、acceptance §2、taxonomy map |

---

## §8 如何继续开发（给人类）

1. **新开对话**，整段粘贴下方 **§9 Mega `/goal` 提示词**。  
2. 脚本门绿后，Qt Creator Rebuild，按四命令手测。  
3. Wave-9 完成后开 **Wave-10**（见 research §4：General MANOVA、Mixed REML、Binary DOE probit…）。  
4. Parallel/Bubble 图建议 **另开 Track G Goal**，勿与算法 Wave 混会话。  

---

## §9 Mega `/goal` 提示词（复制到新对话整段粘贴）

````markdown
/goal

## 身份与总目标
你是 DataLab Orchestrator。用 `/goal` 模式一次做完 **算法 Wave-9**。
**W9-1 + W9-2 + W9-3 + W9-4 全部完整竖切且脚本门 PASS 才允许 UpdateGoal complete。**
禁止缩小范围、禁止只做 1 项、禁止中途换模型、禁止用「文档更新」冒充算法完成。

工作区：DataLab（Qt/C++ 汽车质量桌面工具）
人手门：中文路径 — **禁止** agent 强跑 cmake/ctest；完成后告知用户 Qt Creator Rebuild 手测点。
**不要** commit/push，除非用户明确要求。

「实现所有算法」在本产品的正确含义：按 backlog 产品范围内 ❌ 滚动清空。
**本 Goal 只锁定下面 4 项，但必须 4 项都做完**；其余登记到 Wave-10+，禁止假装 Feature List 已 100% 克隆。

---

## 本 Goal 做什么（锁定 4 项）

权威调研：`docs/research/algorithm-wave9-market-formula-research-2026-08-28.md`  
权威计划：`docs/research/goal-wave-2026-08-28-algorithm-wave9-plan-and-mega-prompt.md`  
DoD：`docs/research/goal-wave-2026-08-28-algorithm-wave9.md`

| ID | command_id | 交付 |
|----|------------|------|
| **W9-1** | `expanded_gage_unbalanced` | 不平衡 Expanded Gage GLM 方差分量；VarComp+%GRR+NDC；**独立**多页对话框 |
| **W9-2** | `split_plot_analyze` | 裂区析因；难改/易改因子；WP/SP 双误差 ANOVA；**独立**对话框 |
| **W9-3** | `mixture_process_variable` | Scheffé + 1 过程变量 + 可选组分×过程交互；**独立**对话框 |
| **W9-4** | `manova_one_way` | 单因子 MANOVA；2～4 响应；Wilks/Pillai/LH/Roy |

每项竖切模板（缺一不可）：
`p9_*.md`（含 Primary URL + 访问日期 + 公式）
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

Verify：`tools/verify_algorithm_wave9_track.py`  
测试：`tests/algorithm_wave9_track_test.cpp`

---

## 明确不做
- G3 Graph Builder 全量、可旋转 3D、G5 架构大拆
- Mixed REML / General MANOVA 全量 / MANOVA 协变量
- Mixture extreme-vertices 设计生成
- TreeNet / AutoML / 商标级 RF 数值对齐
- 嵌 R/Python；sklearn 进 dist
- Minitab 数值 golden / VALIDATION_MATRIX 伪造
- Automated Capability / Assistant
- 破坏导入契约；重做 Wave-2～8 已 ✅ 算法
- **禁止**把不平衡 Gage 并入 `expanded_gage_rr` 对话框
- **禁止**把 Mixture 过程变量并入 `mixture_analyze` 对话框
- **禁止**把裂区分析并入 `doe_factorial` 或 `glm_three_factor`

## 已完成水位（勿重做）
P0–P2 主项；Wave-2～8（含 `binary_response_doe`/`cluster_variables`/`glm_three_factor`/`life_data_regression`/`mixture_analyze`/`glm_two_way`/`expanded_gage_rr`/`random_forest` 等）；
G1/G2/G6/G9（公式代入/分步求值——新命令有公式时应可挂 trace）。

---

## 必读（严格按序）
1. `docs/research/goal-wave-2026-08-28-algorithm-wave9-plan-and-mega-prompt.md`
2. `docs/research/algorithm-wave9-market-formula-research-2026-08-28.md`
3. `docs/research/goal-execution-framework.md`（尤其 §6 禁止偷懒）
4. `docs/research/minitab-market-algorithm-backlog.md`
5. `docs/research/next-wave-algorithms-charts-ml-oss.md`
6. `docs/research/deferred-capability-agreement.md`
7. Wave-8 范例 `docs/research/p8_*.md` + 对话框 `binary_response_doe_dialog` / `glm_three_factor_dialog`
8. 邻域：`expanded_gage_rr.*`、`mixture_analyze.*`、`doe_factorial.*`、`glm_three_factor.*`、`one_way_anova.*`、`hotelling_t2.*`
9. `src/application/analysis_service.*`、`src/ui/analysis_commands.*`、`mainwindow.cpp` 拦截模式
10. 导入与 `row_visibility`（complete-case、source_row、A→B）
11. `docs/algorithm-wiring-index.md`、`samples/product_evolution/unified_track_acceptance_plan.md` §2

Implementer 必载：`.agents/skills/cpp-coding/SKILL.md`  
禁止误读：`待修改.md`、`build/`

执行前：**再次用 WebSearch/WebFetch 核对**每项 Primary URL（Minitab Methods and Formulas / NIST），把最终公式写入对应 `p9_*.md`，访问日期写 **当天**。

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
3. Expanded Gage：**Part/Operator/因子角色**必须独立页；禁止与 VarComp 结果表同页输入。
4. Split-plot：**难改/易改因子**与 **WP 指示**分页；双误差说明独立页。
5. Mixture 过程变量：**组分**与**过程变量**分页；禁止与 `mixture_analyze` 混对话框。
6. MANOVA：**多响应列**与**检验选项**分页；SSCP 方法说明独立页。
7. 新对话框参考 Wave-8 多页模式；控件分组清晰；默认值合理。

---

## 四角色团队（互相监督 · 串行门禁）

| 顺序 | 角色 | subagent | 必须产出 | 门禁 |
|------|------|----------|----------|------|
| 1 | **Planner** | Task explore | 4 项映射表（文件归属+**UI分页线框**+表形+≥3测例+导入影响+Primary URL）；确认无 id 冲突 | 无表禁止写 domain |
| 2 | **Implementer** | Task generalPurpose | 按 W9-1→4 顺序完整竖切；加载 cpp-coding | 禁止跳过 help/interp/测试；禁止单页堆 UI |
| 3 | **Tester** | Task shell | wave9 测试 + verify PASS；回归 wave8 + wave7 + menuIA | verify 未 PASS 禁止 Checker |
| 4 | **Checker** | bugbot 或主 agent | Diff vs DoD；无 Critical；抽查 UI 分页 | Critical→Implementer |

子 Agent 结尾：`文件列表 | DoD [x/ ] | UI页数 | 风险一行 | 是否破坏导入A→B / wave8 verify`

Orchestrator：CreateGoal 一次；立刻开干；连续到 4 项全完；仅 Checker APPROVE + verify PASS 才 complete。

---

## 禁止偷懒
`goal-execution-framework.md` §6（1–13）+ 本计划 §5.2（14–36）。
重点：禁止单算法 complete；禁止 golden；禁止嵌 Python；禁止破坏导入；禁止无 Primary URL；**禁止单页堆控件**；禁止 Gage/Mixture/Split-plot 混对话框；禁止只做菜单壳。

---

## 如何验收

### 脚本门
```powershell
python tools/verify_algorithm_wave9_track.py
python tools/verify_algorithm_wave8_track.py
python tools/verify_algorithm_wave7_track.py
python tools/verify_ui_menu_ia_track.py
```

### 人手门（告知用户，勿代跑 Qt）
Rebuild 后分别打开：
1. Expanded Gage Unbalanced — 不等重复，看 VarComp + %GRR  
2. Split-plot Analyze — 难改/易改因子，看 WP/SP 双误差 ANOVA  
3. Mixture Process Variable — 组分+温度，看交互项与 ANOVA  
4. MANOVA One-Way — 2 响应 3 组，看 Wilks/Pillai 表  

---

## 如何记录
1. `goal-wave-2026-08-28-algorithm-wave9.md` DoD 全 `[x]`
2. backlog 对应行 ✅/⚪；§15.5 增 Wave-9 锁定表
3. wiring-index + acceptance §2 增 Wave-9 行
4. 每项 `p9_*.md` 含访问日期与 Primary URL 与公式
5. 结束告知用户文件清单与 Rebuild 手测点；**不**擅自 commit/push

---

## 启动顺序（本回合立刻执行）
1. CreateGoal（写清 W9-1～4 + verify）
2. TodoWrite（含四角色）
3. Planner explore（无计划表禁止改代码）
4. 网上再搜一遍公式，写/补 `p9_*.md`
5. Implementer → Tester → Checker
6. 全绿 UpdateGoal complete

中间不要换模型。开始执行。
````

---

**文档状态：** 2026-08-31 刷新（§2.5 Planner 映射表 + p9_*.md）；供 `/goal` 整段复制 §9。

