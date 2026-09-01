# Wave：算法 Wave-11（Simple CA + Multiple CA + Nonlinear + Split-Plot Design）计划与 Mega `/goal` 提示词（2026-09-01）

> 访问日期：2026-09-01（UTC+8）  
> 调研正文：[`algorithm-wave11-market-formula-research-2026-09-01.md`](algorithm-wave11-market-formula-research-2026-09-01.md)  
> 执行框架：[`goal-execution-framework.md`](goal-execution-framework.md)  
> 状态权威：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) §4 / §12 / §15  
> DoD：[`goal-wave-2026-09-01-algorithm-wave11.md`](goal-wave-2026-09-01-algorithm-wave11.md)  
> **不做本 Goal：** PLS 全量、Nonlinear 任意表达式、MCA 指示矩阵手工模式、Split-plot 7 因子全表、Mixed REML 多随机、Graph Builder、TreeNet/AutoML、嵌 R/Python、Minitab golden、「一次做完 Feature List 全部 ❌」

---

## §0 给 Orchestrator / 人类的一页摘要

| 维度 | 内容 |
|------|------|
| **Goal 名称** | 算法 Wave-11：4 项竖切（`simple_correspondence` / `multiple_correspondence` / `nonlinear_regression` / `split_plot_design`） |
| **为何这 4 项** | Wave-10 后 backlog 最大剩余缝：多元对应分析 0→窄化；非线性回归 ❌；W9 仅有裂区**分析**缺**设计生成** |
| **Wave 数** | **1 个 Wave；4 项全部 ✅ 才 complete** |
| **交付门** | `python tools/verify_algorithm_wave11_track.py` PASS |
| **人手门** | 用户 Qt Creator Rebuild；agent **禁止**强跑 cmake/ctest（中文路径） |
| **四角色** | Planner → Implementer → Tester → Checker（串行门禁、互相监督） |
| **「全部算法」口径** | 本 Goal **清空锁定 4 项**；全产品 ❌ 由 Wave-12+ 滚动（见 research §4） |

### 用户诉求映射

| 诉求 | 本计划如何落实 |
|------|----------------|
| 算法扩展 + 网上公式 | research §1 + 每项 `p11_*.md` + 执行前 WebSearch/WebFetch 再核对 |
| 一次多做 | 锁定 **4 项**同 Goal |
| 四角色互相监督 | §4 串行门禁 + 未过则退回 |
| 足够测试 | domain/service + wave11 track + verify + 回归 wave10/9/menuIA |
| **UI 不堆控件** | §5；每命令 **≥4 独立 QStackedWidget 页** |
| 执行 agent 很笨还很懒 | §5 禁止偷懒 14–45；Checker 对照 DoD 与 UI 线框 |
| 导入衔接 | complete-case / `source_row` / A→B 必测 |

---

## §1 框架结构（必读 · 优先看哪些程序）

### 1.1 分层架构

```
DataLab.exe
  └── main.cpp
  └── src/ui/          datalab_ui        ← 对话框、菜单、MainWindow::run_from_spec
        └── src/application/  datalab_application  ← AnalysisService（编排入口）
              └── src/domain/       datalab_domain       ← 纯 C++ 算法（禁止 Qt）
              └── src/infrastructure/
              └── src/reporting/

tests/*_test.exe
  └── 链接同一批 static lib 子集 + Qt6::Test（无 UI、无 main.cpp）
  └── 直接调 AnalysisService::xxx(DataTable, AnalysisConfiguration)
```

**依赖方向（硬约束）：** `ui → application → domain`；**domain 不得 include Qt**；infrastructure **不得** include ui。

### 1.2 新算法竖切路径（每项必走）

```
docs/research/p11_*.md（Primary URL + 公式 + 访问日期）
  → src/domain/statistics/<algo>.{h,cpp}
  → src/domain/quality_types.h（*Facts 结构）
  → src/application/analysis_service.cpp（AnalysisService::<cmd>）
  → src/ui/analysis_commands.cpp（command 注册 + menu_path/menu_group）
  → src/ui/<cmd>_dialog.{h,cpp}（QDialog + QStackedWidget ≥4 页）
  → src/ui/mainwindow.cpp（run_from_spec 拦截 command_id）
  → interpretation_service + report_text_catalog_part*.cpp
  → resources/algorithm_help.json + docs/algorithm-wiring-index.md
  → tests/algorithm_wave11_track_test.cpp + CMakeLists.txt add_datalab_test
  → tools/verify_algorithm_wave11_track.py
  → backlog §15.7 + unified_track_acceptance_plan.md §2
```

### 1.3 优先阅读文件（Planner 第一小时）

| 优先级 | 路径 | 目的 |
|--------|------|------|
| P0 | 本文件 + `algorithm-wave11-market-formula-research-2026-09-01.md` | 锁定范围 |
| P0 | `docs/research/goal-execution-framework.md` §2–§6 | 四角色 + 禁止偷懒 |
| P0 | `docs/research/minitab-market-algorithm-backlog.md` §12–§15 | 登记与勿重做 |
| P1 | `src/ui/analysis_commands.cpp` | 已有 command_id；Menu IA |
| P1 | `src/application/analysis_service.cpp` | 邻域命令实现模式 |
| P1 | `tests/algorithm_wave10_track_test.cpp` | 上一 Wave 测试模板 |
| P1 | `tools/verify_algorithm_wave10_track.py` | verify 脚本模板 |
| P2 | `src/ui/split_plot_analyze_dialog.cpp`（或 W9 任一多页 dialog） | UI 分页参考 |
| P2 | `src/domain/statistics/split_plot_analyze.cpp` | 裂区分析邻域 |
| P2 | `src/domain/statistics/doe_factorial*.cpp` | DOE 设计生成邻域 |
| P2 | `samples/product_evolution/unified_track_acceptance_plan.md` | 统一验收 |

### 1.4 团队 Agent 流程

```
Orchestrator（/goal 主对话）
  CreateGoal → TodoWrite(W11-1..4 + 四角色)
       │
       ▼
  Planner (Task explore) ──门禁──► 4 项映射表 + UI 线框 + 测例 + Primary URL
       │                              禁止写 domain
       ▼
  Implementer (Task generalPurpose) ── W11-1→4 顺序竖切；加载 cpp-coding skill
       │  每项：p11_*.md → domain → Facts → service → commands
       │       → **专用多页 QDialog** → interpretation → serialization → tests → help
       ▼
  Tester (Task shell) ── verify_algorithm_wave11_track.py PASS
       │                 + 回归 wave10 + wave9 + menuIA
       ▼
  Checker (Task bugbot 或 generalPurpose) ── Diff vs DoD；UI 分页抽查
       │  Critical → 退回 Implementer；通过后 Orchestrator 才可 UpdateGoal complete
       ▼
  UpdateGoal complete（仅全绿）
```

**子 Agent 统一结尾模板：**

```text
文件列表 | DoD [x/ ] | UI页数确认 | 风险一行 | 是否破坏导入A→B / wave10 verify
```

### 1.5 四角色互相监督（硬门禁）

| 角色 | subagent | 监督谁 | 必须产出 | 未过则 |
|------|----------|--------|----------|--------|
| **Planner** | `Task explore` | Orchestrator | 4 项映射表：**UI 分页线框**（每页控件清单）、表形、≥3 测例/项、导入影响、Primary URL、id 无冲突 | 无表 → Implementer **禁止**写 domain |
| **Implementer** | `Task generalPurpose` | Planner + DoD | 4 项完整竖切；**每命令专用 QDialog ≥4 页** | 跳过 help/interp/测试 → Tester **拒绝** |
| **Tester** | `Task shell` | Implementer | wave11 测试 + verify PASS；回归 wave10/wave9/menuIA | verify 未 PASS → Checker **禁止**放行 |
| **Checker** | `bugbot` 或 `Task generalPurpose` | 全员 | Diff vs DoD；无 Critical；**UI 无单页堆积** | Critical → **退回** Implementer |

**互相监督规则：**

1. Planner 产出映射表前，Implementer **不得**创建任何 `src/domain/statistics/*` 新文件。  
2. Tester 在 verify 未 PASS 时，Checker **不得**标记 Wave 完成。  
3. Checker 发现 UI 页数不足或层次混页，Implementer **必须**拆页后 Tester 重跑。  
4. 任一子 agent 试图缩小为「只做 1 项」，Orchestrator **拒绝**并引用本文件 §0。

---

## §2 锁定项摘要

| ID | command_id | DoD 要点 |
|----|------------|----------|
| W11-1 | `simple_correspondence` | 2 列分类；列联；惯性+贡献表；≠ MCA 对话框 |
| W11-2 | `multiple_correspondence` | 3～6 列分类；Column Contributions；≠ SCA 对话框 |
| W11-3 | `nonlinear_regression` | 内置模型 + GN/LM；≠ linear 对话框 |
| W11-4 | `split_plot_design` | 2～4 因子 1 HTC；设计矩阵；≠ split_plot_analyze 对话框 |

详情与公式 URL：调研文档 §2；每项 `p11_*.md`（执行前须 WebFetch 核对 Primary URL）。

---

## §2.5 Planner 映射表（Implementer 开工前必须核对/补全）

| ID | 新建 domain | 新建 dialog | 修改共享 | menu_path / menu_group | 邻域复用 |
|----|-------------|-------------|----------|------------------------|----------|
| W11-1 | `simple_correspondence.*` | `simple_correspondence_dialog.*` | service/commands/mainwindow/serialization/catalog/help | 统计 / **多变量** | `factor_analysis`、`discriminant` |
| W11-2 | `multiple_correspondence.*` | `multiple_correspondence_dialog.*` | 同上 | 统计 / **多变量** | W11-1、`cross_tabulation` |
| W11-3 | `nonlinear_regression.*` | `nonlinear_regression_dialog.*` | 同上 | 统计 / **回归** | `linear_regression`、`stepwise_regression` |
| W11-4 | `split_plot_design.*` | `split_plot_design_dialog.*` | 同上 | 统计 / **DOE** | `split_plot_analyze`、`doe_factorial` |

**共享必改：** `CMakeLists.txt`、`quality_types.h`、`report_text_catalog_part22.cpp`（+ `report_text_catalog_parts.h/cpp`）、`algorithm-wiring-index.md`、`docs/research/ui-menu-ia-command-taxonomy-map-2026-08-23.md`、`backlog §15.7`、`unified_track_acceptance_plan.md §2`

**UI 线框（Checker 硬查 ≥4 页 · 禁止同页堆不同层次）：**

| 命令 | P1（数据层） | P2（模型/设计层） | P3（方法/输出层） | P4（预览） |
|------|-------------|------------------|------------------|-----------|
| simple_correspondence | 行变量 + 列变量 | 组件数 + 图选项 | 贡献表/惯性选项 | 预览 |
| multiple_correspondence | 3～6 分类列 | 组件数 | 输出表/Column plot | 预览 |
| nonlinear_regression | 响应 + 预测 | 模型 + 初值表 | GN/LM + 收敛 | 预览 |
| split_plot_design | 因子数 + 名称 | HTC + 设计表 | 复制 + 随机化 | 预览矩阵 |

**测例（每项 ≥3）：**

- W11-1：2×3 列联玩具；惯性求和；空表 error；Facts 序列化  
- W11-2：4 变量指示展开；Qual/Mass 非空；组件数门禁  
- W11-3：Growth 模型收敛；LM 切换；初值非法 diagnostic  
- W11-4：4 因子 1 HTC 32 run 玩具；Whole plot 列单调；可接 split_plot_analyze 说明测  

---

## §3 优先处理什么 / 如何继续开发

### 3.1 本 Wave 处理范围

| 做 | 不做 |
|----|------|
| 4 命令完整竖切（domain→UI→测试→help） | PLS、Cox 全量、Graph Builder |
| 每项独立多页 QDialog | 把 SCA+MCA 合并成一个「加模式勾选」对话框 |
| verify + 回归 wave10/9/menuIA | 破坏 Wave-2～10 已 ✅ 命令 |
| backlog 登记 §15.7 | 用 Minitab 数值当 golden |

### 3.2 实施顺序（Implementer 不得并行跳项）

1. **W11-1** `simple_correspondence`（domain 最简单，建立 Facts/图表模式）  
2. **W11-4** `split_plot_design`（与 W9 分析闭环；可复用 doe 基础设施）  
3. **W11-3** `nonlinear_regression`（迭代数值；独立回归菜单）  
4. **W11-2** `multiple_correspondence`（指示矩阵略重；放最后便于复用 SCA 线性代数）

### 3.3 Wave-11 完成后下一 Wave 候选（Wave-12+）

见 `algorithm-wave11-market-formula-research-2026-09-01.md` §4：

- `pls_regression`（NIPALS + PRESS）  
- Mixture extreme-vertices 设计  
- Mixed REML 多随机项  
- Track G：`parallel_plot` / `bubble_plot`（**独占 Goal，勿与算法混**）  
- G3 Graph Builder（**独占 Goal**）

### 3.4 人类手测（agent 完成后必告知）

1. Qt Creator → Rebuild Project（**不要**对单 target 用 `--clean-first`）  
2. 手测四菜单：多元×2、回归×1、DOE×1  
3. 每命令走完整向导 4 页 → Run → 检查 Session 表形  
4. Split design 生成矩阵 → 填响应 → `split_plot_analyze` 联测  

---

## §4 四角色执行细则

| 阶段 | 角色 | subagent | 输入 | 输出 |
|------|------|----------|------|------|
| 1 | **Planner** | `explore` thorough | backlog + analysis_commands + wiring-index | 映射表 + UI ASCII 线框 + ≥12 测例要点 + Primary URL 列表 |
| 2 | **Implementer** | `generalPurpose` | 映射表 + p11_*.md + cpp-coding skill | 4 项代码 + help + catalog |
| 3 | **Tester** | `shell` | 实现完成 | verify PASS 日志 + 回归 PASS |
| 4 | **Checker** | `bugbot` 或 `generalPurpose` | diff + DoD md | APPROVE / Critical 列表 |

---

## §5 禁止偷懒（Goal 必粘贴 · 防笨防懒）

### 5.1 通用

粘贴并遵守 `goal-execution-framework.md` **§6（1–13）** 全文。

### 5.2 Wave-11 增补

14. 禁止只做 1～2 个算法就 `UpdateGoal complete`  
15. 禁止跳过 Primary URL / `p11_*.md` / WebSearch  
16. 禁止 Minitab/JMP **数值**当 golden  
17. 禁止破坏 complete-case / `source_row` / A→B  
18. 禁止解释层「已证明关联显著 / 模型最优 / 设计最优」等决策性结论  
19. 禁止 sklearn/R/Python 打进 dist  
20. 禁止宣称数值等同 Minitab SCA/MCA/Nonlinear/Split-plot  
21. 禁止混入 G3 Graph Builder / G5 大拆  
22. 禁止无 `menu_path`/`menu_group` 的新命令  
23. **禁止把 SCA 和 MCA 塞进同一对话框「模式切换」**  
24. **禁止把 Nonlinear 塞进 `linear_regression` 对话框「加非线性勾选」**  
25. **禁止把 Split design 塞进 `split_plot_analyze` 对话框**  
26. **禁止 Split design 无 Whole plot 列**  
27. **禁止单页堆：列选择 + 模型 + 方法 + 结果表 + 图**  
28. **禁止层次不一致内容同页**（例：因子命名 vs 设计分辨率表 vs 随机化选项）  
29. SCA：**行/列变量选择** 与 **组件/图** 必须分页  
30. MCA：**变量多选** 与 **组件数** 必须分页  
31. Nonlinear：**模型/初值** 与 **算法迭代** 必须分页  
32. Split design：**因子定义** 与 **HTC/设计表** 必须分页  
33. 禁止只有菜单壳、无 domain/Facts/测试  
34. 禁止用「文档更新」冒充算法完成  
35. 禁止 MCA 全量（指示矩阵手工输入 + 任意维数导出）冒充窄化完成  
36. 禁止 Nonlinear 任意用户表达式 parser 冒充窄化完成  
37. Planner 映射表未产出时 Implementer **不得**写第一行 domain 代码  
38. 禁止破坏 Wave-10 verify 回归  
39. 禁止 PLS / correspondence 全量 / mosaic 偷换锁定 4 项  
40. 每项必须 `page.analysis_command_id = "<command_id>"`  
41. **禁止在一个 QStackedWidget 页内同时放「数据选择 QListWidget + QTableWidget 结果预览 + QChart」**  
42. **禁止用 QTabWidget 代替分页向导来堆 6+ 个 tab**（每命令仍用 Back/Next/Run 向导）  
43. **禁止 Checker 未读 dialog 源文件就 APPROVE**  
44. **禁止 Tester 只跑 wave11 不回归 wave10**  
45. **禁止 Implementer 跳过 `algorithm_wave11_track_test.cpp` 中任一 command_id 测试**

### 5.3 UI 分页最低要求（Checker 硬查）

| 命令 | 最少页数 | 分页线框 |
|------|----------|----------|
| `simple_correspondence` | **4** | 行列变量 \| 组件+图 \| 表选项 \| 预览 |
| `multiple_correspondence` | **4** | 分类列 \| 组件 \| 输出 \| 预览 |
| `nonlinear_regression` | **4** | Y+X \| 模型+初值 \| 算法 \| 预览 |
| `split_plot_design` | **4** | 因子 \| HTC+设计 \| 复制 \| 预览 |

实现模式：`QDialog` + `QStackedWidget` + Back/Next/Run；参考 `mixed_effects_reml_dialog` / `split_plot_analyze_dialog`；`MainWindow::run_from_spec` 拦截四 id。

---

## §6 测试与 verify

### 6.1 每项最低

- domain 或 service 测：主路径 + 门禁失败 diagnostic  
- `# source: formula_reference` ≥1  
- `find(command_id)` 非空；Menu IA 四顶层合法  
- SCA：χ² 惯性 > 0 玩具；MCA：Column Contr 非空  
- Nonlinear：至少 2 个内置模型各 1 路径  
- Split design：Whole plot 列 + Run Order 非空  

### 6.2 Wave 脚本门

```powershell
python tools/verify_algorithm_wave11_track.py
python tools/verify_algorithm_wave10_track.py
python tools/verify_algorithm_wave9_track.py
python tools/verify_ui_menu_ia_track.py
```

`verify_algorithm_wave11_track.py` 至少检查：

1. research + DoD md 存在且 W11-1～4 实现项有 `[x]`  
2. 四 command id 在 `analysis_commands.cpp`  
3. 各 `p11_*.md` 含 Primary URL / 访问日期  
4. CMake 含 `algorithm_wave11_track_test`  
5. wiring-index + acceptance 含 Wave-11  
6. 测试文件含四 id + formula_reference 标记  
7. 四专用 dialog 源文件存在且 `QStackedWidget` 页数 ≥ 最低  
8. **独立** dialog（非合并到邻域命令）

### 6.3 可选深度回归（时间允许）

- `quality_statistics_test` 若因邻域改动失败须修复  
- `minitab_formula_golden_test` / `minitab_numerical_golden_test` 不得新增失败  

---

## §7 交付物

| 产物 | 路径 |
|------|------|
| 调研 | `docs/research/algorithm-wave11-market-formula-research-2026-09-01.md` |
| 本计划 + Mega 提示词 | 本文件 |
| DoD | `docs/research/goal-wave-2026-09-01-algorithm-wave11.md` |
| 每项 research | `docs/research/p11_*.md` |
| 测试 | `tests/algorithm_wave11_track_test.cpp` |
| Verify | `tools/verify_algorithm_wave11_track.py` |
| 登记 | backlog §15.7、wiring-index、acceptance §2 |

---

## §8 如何继续开发（给人类）

1. **新开对话**，整段粘贴下方 **§9 Mega `/goal` 提示词**。  
2. 脚本门绿后，Qt Creator Rebuild，按四命令手测。  
3. Wave-11 完成后开 **Wave-12**（PLS、Mixture extreme-vertices…）。  
4. Parallel/Bubble / Graph Builder 建议 **另开 Track G / G3 Goal**，勿与算法 Wave 混会话。  

---

## §9 Mega `/goal` 提示词（复制到新对话整段粘贴）

````markdown
/goal

## 身份与总目标
你是 DataLab Orchestrator。用 `/goal` 模式一次做完 **算法 Wave-11**。
**W11-1 + W11-2 + W11-3 + W11-4 全部完整竖切且脚本门 PASS 才允许 UpdateGoal complete。**
禁止缩小范围、禁止只做 1 项、禁止中途换模型、禁止用「文档更新」冒充算法完成。

工作区：DataLab（Qt/C++ 汽车质量桌面工具）
构建目录：**`D:\QT_CppPrograms\DataLab\build-mingw`**（Qt 6.11.1 MinGW）
**允许且要求** agent 在 Goal 模式下编译并运行测试（见下方「测试与编译」）。
完成后仍告知用户 Qt Creator 手测四命令 UI。
**不要** commit/push，除非用户明确要求。

「实现所有算法」在本产品的正确含义：按 backlog 产品范围内 ❌ 滚动清空。
**本 Goal 只锁定下面 4 项，但必须 4 项都做完**；其余登记到 Wave-12+，禁止假装 Feature List 已 100% 克隆。

---

## 本 Goal 做什么（锁定 4 项）

权威调研：`docs/research/algorithm-wave11-market-formula-research-2026-09-01.md`
权威计划：`docs/research/goal-wave-2026-09-01-algorithm-wave11-plan-and-mega-prompt.md`
DoD：`docs/research/goal-wave-2026-09-01-algorithm-wave11.md`

| ID | command_id | 交付 |
|----|------------|------|
| **W11-1** | `simple_correspondence` | 2 列分类 SCA；惯性+贡献表；**独立**≥4 页对话框 |
| **W11-2** | `multiple_correspondence` | 3～6 列 MCA；Column Contributions；**独立**对话框 |
| **W11-3** | `nonlinear_regression` | 内置模型 GN/LM；**独立**对话框 |
| **W11-4** | `split_plot_design` | 2 水平裂区设计生成；Whole plot 列；**独立**对话框 |

每项竖切模板（缺一不可）：
`p11_*.md`（含 Primary URL + 访问日期 + 公式）
→ domain（纯 C++）
→ `*Facts`
→ `AnalysisService::*`
→ `analysis_commands`（menu_path/menu_group）
→ **专用 QDialog + QStackedWidget ≥4 页**（Back/Next/Run）
→ `interpretation_service` + catalog 双语
→ serialization + `algorithm_help.json`
→ `tests/algorithm_wave11_track_test.cpp`
→ `tools/verify_algorithm_wave11_track.py`

---

## 四角色团队（串行门禁 · 互相监督）

### TodoWrite 初始项
- [ ] Planner：映射表 + UI 线框 + Primary URL + ≥3 测例/项
- [ ] Implementer：W11-1 → W11-4 → W11-3 → W11-2 竖切
- [ ] Tester：verify wave11 PASS + 回归 wave10/wave9/menuIA
- [ ] Checker：DoD + UI 分页 + 无 Critical

### 1. Planner（Task explore, thorough）
**监督：** 无映射表则 Implementer 禁止写 domain。
**必读：** backlog §12–15、analysis_commands.cpp、wiring-index、wave10 track test。
**产出：**
- 4 项映射表（domain/dialog/共享/menu/邻域）
- 每命令 **4 页 UI ASCII 线框**（每页控件清单 ≤8 个主控件）
- ≥3 测例/项（含失败路径）
- WebSearch + WebFetch Primary URL → 更新 `p11_*.md` 访问日期
**禁止：** 写代码；缩小为 1 项；建议合并 SCA+MCA 对话框。

### 2. Implementer（Task generalPurpose）
**监督：** Planner 表 + DoD；违反 §5 则 Tester 拒收。
**顺序：** W11-1 → W11-4 → W11-3 → W11-2（见计划 §3.2）
**加载：** `.agents/skills/cpp-coding/SKILL.md`（domain/service 改动）
**UI 硬规则：**
- 每命令 **新建** `*_dialog.cpp`，`QStackedWidget` ≥4 页
- **禁止**单页：数据+模型+方法+结果
- **禁止** QTabWidget 堆 6+ tab 代替向导
- `MainWindow::run_from_spec` 注册四 command_id
**禁止：** 跳过 help/interp/测试；SCA 并入 MCA；Nonlinear 并入 linear；design 并入 split_plot_analyze。

### 3. Tester（Task shell）
**监督：** Implementer 交付完整性。
**必须执行（Goal 模式下允许跑测试）：**

```powershell
# 1) 编译 wave11 测试目标（禁止对单 target 使用 --clean-first，会 wipe 全部 exe）
cd D:\QT_CppPrograms\DataLab\build-mingw
cmake --build . --target algorithm_wave11_track_test -j 8

# 2) 运行 wave11 测试
.\tests\algorithm_wave11_track_test.exe

# 3) verify 静态门 + 回归
cd D:\QT_CppPrograms\DataLab
python tools/verify_algorithm_wave11_track.py
python tools/verify_algorithm_wave10_track.py
python tools/verify_algorithm_wave9_track.py
python tools/verify_ui_menu_ia_track.py
```

**若 wave11 测试 exe 不存在：** 先 `cmake .. -G "MinGW Makefiles"`（仅 build 目录未配置时），再 build target。
**若链接失败（exe 被锁）：** 重试一次；仍失败则记录并继续 verify，在风险行注明。
**邻域改动后可选回归：**
```powershell
cmake --build D:\QT_CppPrograms\DataLab\build-mingw --target quality_statistics_test -j 8
D:\QT_CppPrograms\DataLab\build-mingw\tests\quality_statistics_test.exe
```
**未 PASS → Checker 禁止放行。**

### 4. Checker（Task bugbot 或 generalPurpose）
**监督：** 全员。
**检查：**
- Diff vs `goal-wave-2026-09-01-algorithm-wave11.md` 每项 `[x]`
- 四 dialog 源文件页数 ≥4
- 无 Critical（domain 依赖 Qt、单页堆积、缺 formula_reference）
**Critical → 退回 Implementer → Tester 重跑。**

---

## 架构速记

```
ui → application → domain（无 Qt）
tests 直调 AnalysisService，不经 UI
```

优先参考：`algorithm_wave10_track_test.cpp`、`verify_algorithm_wave10_track.py`、`mixed_effects_reml_dialog.cpp`、`split_plot_analyze.cpp`、`doe_factorial` 族。

---

## 禁止偷懒（粘贴 goal-execution-framework.md §6 全文 + 下列增补）

14–45 见计划 `goal-wave-2026-09-01-algorithm-wave11-plan-and-mega-prompt.md` §5.2。

**UI 核心：** 层次不同 → 不同页；该新建 dialog 就新建；**禁止**在一个页面堆积很多控件或混放「选列 + 设计表 + 方法 + 结果预览」。

---

## 必读文件

- `docs/research/goal-execution-framework.md`
- `docs/research/minitab-market-algorithm-backlog.md` §12–§15
- `docs/research/next-wave-algorithms-charts-ml-oss.md` §Track H
- `samples/product_evolution/unified_track_acceptance_plan.md`
- `docs/research/deferred-capability-agreement.md` §5

---

## 完成条件（全部满足才 UpdateGoal complete）

1. W11-1～4 DoD 全 `[x]`
2. `verify_algorithm_wave11_track.py` PASS
3. 回归 wave10 + wave9 + menuIA PASS
4. Checker APPROVE（无 Critical）
5. 告知用户 Qt Creator 手测四命令 UI；Split design → split_plot_analyze 联测说明

## 交付物清单

- `docs/research/algorithm-wave11-market-formula-research-2026-09-01.md`（已存在，可增补）
- `docs/research/p11_*.md` ×4
- domain ×4 + dialog ×4 + service + commands
- `tests/algorithm_wave11_track_test.cpp`
- `tools/verify_algorithm_wave11_track.py`
- backlog §15.7 + wiring-index + acceptance §2 更新
````

---

**文档状态：** 2026-09-01 初稿；Wave-10 ✅ 后续 Wave。
