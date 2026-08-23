# Wave：算法 Wave-5（扩展 + 深化）计划与 Mega `/goal` 提示词（2026-08-23）

> 访问日期：2026-08-23（UTC+8）  
> 调研正文：[`algorithm-wave5-market-formula-research-2026-08-23.md`](algorithm-wave5-market-formula-research-2026-08-23.md)  
> 执行框架：[`goal-execution-framework.md`](goal-execution-framework.md)  
> 状态权威：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) §12 / §15  
> 候选池：[`next-wave-algorithms-charts-ml-oss.md`](next-wave-algorithms-charts-ml-oss.md) Track E/F/H  
> **不做本 Goal：** G3/G4/G5 产品 Track、TreeNet/AutoML、嵌 R/Python、Minitab golden

---

## §0 给 Orchestrator 的一页摘要

| 维度 | 内容 |
|------|------|
| **本 Goal 名称** | 算法 Wave-5：4 项竖切（`random_forest` / `weibayes` / `taguchi_orthogonal_design` / `distribution_calculator`） |
| **Wave 数** | **1 个 Wave；4 项全部 ✅ 才 complete**（禁止只做 1 项就结束） |
| **交付门** | `python tools/verify_algorithm_wave5_track.py` PASS |
| **人手门** | 用户 Qt Creator Rebuild；agent **禁止**强跑 cmake/ctest |
| **四角色** | Planner → Implementer → Tester → Checker |
| **导入** | 每项必须证明不破坏 complete-case / `source_row` / A→B |

### 用户诉求映射（来自备忘）

| 诉求 | 本计划如何落实 |
|------|----------------|
| 算法扩展 + 深化 | 3 新增 + 1 工具；深化落在各 research 表形对齐 |
| 一次多做 | 锁定 **4 项**，同 Goal 连续竖切 |
| 网上公式 + Minitab 输出 | 每项独立 `p5_*.md` + Primary URL |
| 导入衔接 | §3 + Tester 强制回归标记 |
| 四角色互相监督 | §4 串行门禁 |
| 足够测试 | 每项 domain/service 测 + Wave verify + formula_reference |

---

## §1 框架结构（团队 Agent）

```
Orchestrator（/goal 主对话）
  CreateGoal → TodoWrite(W5-1..4 + 四角色)
       │
       ▼
  Planner (explore) ──门禁──► 无「4 项映射表+测试清单」禁止写 domain
       │
       ▼
  Implementer (generalPurpose) ──按 id 顺序竖切；加载 cpp-coding
       │  每项：research→domain→Facts→service→commands→interp→tests→help→wiring
       ▼
  Tester (shell) ──写/跑 verify_algorithm_wave5_track.py + 回归 wave4
       │
       ▼
  Checker (bugbot/自查) ──Diff vs DoD；Critical→回 Implementer
       │
       ▼
  UpdateGoal complete（仅全绿）
```

**子 Agent 统一结尾：**

```text
文件列表 | DoD [x/ ] | 风险一行 | 是否破坏导入A→B/wave4 verify
```

---

## §2 锁定项（W5-1～W5-4）

详见 research §2。摘要：

| ID | command_id | DoD 要点 |
|----|------------|----------|
| W5-1 | `random_forest` | 窄化森林；重要性表；非 TreeNet 披露；完整竖切 |
| W5-2 | `weibayes` | 少失效 Weibull 窄化；右删失；NIST/Minitab 表形 |
| W5-3 | `taguchi_orthogonal_design` | L8/L9/L12 之一设计矩阵 → 可写入工作表 |
| W5-4 | `distribution_calculator` | 正态/t/χ²/F/Weibull PDF·CDF·分位 |

**备选替换规则：** 见 research §2；替换须 Planner 文档化且仍满 4 项。

---

## §3 优先阅读清单（严格顺序）

| # | 路径 | 用途 |
|---|------|------|
| 1 | 本文件 + [`algorithm-wave5-market-formula-research-2026-08-23.md`](algorithm-wave5-market-formula-research-2026-08-23.md) | 锁定与公式入口 |
| 2 | [`goal-execution-framework.md`](goal-execution-framework.md) | Wave / 禁止偷懒 |
| 3 | [`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) | 状态登记 |
| 4 | [`next-wave-algorithms-charts-ml-oss.md`](next-wave-algorithms-charts-ml-oss.md) | Track E/F/H |
| 5 | [`deferred-capability-agreement.md`](deferred-capability-agreement.md) | 勿做清单 |
| 6 | 最近竖切范例：`docs/research/p4_*.md` 或 Wave-4 plan | 表形写法 |
| 7 | `src/domain/statistics/` 同类（`cart_tree`、`reliability`、`doe_*`） | 代码邻域 |
| 8 | `src/application/analysis_service.h/.cpp` | 接线 |
| 9 | `src/ui/analysis_commands.h/.cpp` | 命令元数据 + Menu IA 字段 |
| 10 | 导入：`infrastructure/data_import*`、worksheet / row_visibility | A→B 契约 |
| 11 | `docs/algorithm-wiring-index.md`、`unified_track_acceptance_plan.md` §2 | 登记 |
| 12 | 回归脚本：`verify_algorithm_wave4_track.py` | 不得破坏 |

Implementer：**必载** `.agents/skills/cpp-coding/SKILL.md`  
禁止误读：`待修改.md`、`build/`

---

## §4 四角色门禁

| 顺序 | 角色 | subagent | 必须产出 | 未过禁止进入下一角色 |
|------|------|----------|----------|----------------------|
| 1 | **Planner** | explore | 每项：已有 id 冲突检查、domain 文件归属、表形清单、≥3 测例/项、导入影响；终稿 4 项确认 | 无表禁止改 domain |
| 2 | **Implementer** | generalPurpose | 4 项完整竖切 + CMake 测试目标 | 禁止跳过 interpretation/help；禁止 golden |
| 3 | **Tester** | shell | `tests/algorithm_wave5_track_test.cpp` + `verify_algorithm_wave5_track.py` PASS；回归 wave4 | verify 未 PASS 禁止 Checker |
| 4 | **Checker** | bugbot/自查 | Diff vs DoD；无 Critical；无嵌 Python；无破坏 Menu IA 四顶层 | Critical → Implementer |

---

## §5 禁止偷懒

**通用：** goal-execution-framework.md §6（1–13）。

**Wave-5 增补：**

14. 禁止只做 1 个算法就 UpdateGoal complete  
15. 禁止跳过网上 Primary URL / `p5_*.md`  
16. 禁止 Minitab/JMP 数值当 golden  
17. 禁止破坏 complete-case / `source_row` / A→B  
18. 禁止解释层「过程已失控 / 已证明稳定 / 批次合格 / 分布已正态」  
19. 禁止把 sklearn/R 运行时打进 dist  
20. 禁止宣称等同 TreeNet® / Minitab Random Forests®  
21. 禁止混入 G3 Graph Builder / G5 大拆  
22. 禁止大 catalog 单 TU 无拆分（interpretation >500 条须 part）  
23. 禁止无 `menu_path`/`menu_group` 的新命令（破坏 Menu IA）  

---

## §6 测试与 verify

### 6.1 每项最低

- domain 单元或 service 测：主路径 + 门禁失败诊断  
- `# source: formula_reference` 至少 1 处  
- 命令 `find(id)` 非空；Menu IA 四顶层合法  

### 6.2 Wave 脚本门

```powershell
python tools/verify_algorithm_wave5_track.py
python tools/verify_algorithm_wave4_track.py
python tools/verify_ui_menu_ia_track.py
```

`verify_algorithm_wave5_track.py` 至少检查：

1. 本 research + goal DoD md 存在且 W5-1～4 有 `[x]`  
2. 四命令 id 出现在 `analysis_commands.cpp`  
3. 各 `p5_*.md` 或合并 research 存在且含 Primary URL / 访问日期  
4. CMake 含 `algorithm_wave5_track_test`  
5. wiring-index + acceptance 含 Wave-5 / 各 id  
6. 测试文件含四个 id 标记  

---

## §7 交付物

| 产物 | 路径 |
|------|------|
| 调研 | `docs/research/algorithm-wave5-market-formula-research-2026-08-23.md` |
| 本计划 + Mega 提示词 | 本文件 |
| DoD | `docs/research/goal-wave-2026-08-23-algorithm-wave5.md` |
| 每项 research | `docs/research/p5_random_forest.md` 等 |
| 测试 | `tests/algorithm_wave5_track_test.cpp` |
| Verify | `tools/verify_algorithm_wave5_track.py` |
| 登记 | backlog、wiring-index、acceptance §2 |

---

## §8 如何继续开发（给人类）

1. 本 Goal 用新对话粘贴 §9 执行。  
2. Wave-5 脚本门绿后，再开 **G3** 或 **Wave-6**（平行不混会话为佳）。  
3. 统一验收仍按 `unified_track_acceptance_plan.md` 末尾一次 Qt 测。  

---

## §9 Mega `/goal` 提示词（复制到新对话整段粘贴）

````markdown
/goal

## 身份与总目标
你是 DataLab Orchestrator。用 `/goal` 模式一次做完 **算法 Wave-5**。
**W5-1 + W5-2 + W5-3 + W5-4 全部完整竖切且脚本门 PASS 才允许 UpdateGoal complete。**
禁止缩小范围、禁止只做 1 项、禁止中途换模型。

工作区：DataLab（Qt/C++）
人手门：中文路径 — **禁止** agent 强跑 cmake/ctest；完成后告知用户 Qt Creator Rebuild。
**不要** commit/push，除非用户明确要求。

---

## 本 Goal 做什么（锁定 4 项）

权威调研：`docs/research/algorithm-wave5-market-formula-research-2026-08-23.md` §2  
权威计划：`docs/research/goal-wave-2026-08-23-algorithm-wave5-plan-and-mega-prompt.md`

| ID | command_id | 交付 |
|----|------------|------|
| **W5-1** | `random_forest` | 窄化随机森林；重要性；披露非 TreeNet/Minitab RF 对齐；完整竖切 |
| **W5-2** | `weibayes` | 少失效 Weibull/Weibayes 窄化；右删失主路径；完整竖切 |
| **W5-3** | `taguchi_orthogonal_design` | Taguchi 正交表设计生成（L8/L9/L12 子集起步）；可导出/写入工作表 |
| **W5-4** | `distribution_calculator` | 正态/t/χ²/F/Weibull：PDF/CDF/分位数工具 |

每项竖切模板：
`p5_*.md` → domain → Facts → AnalysisService → analysis_commands（含 menu_path/menu_group）→ interpretation → serialization → formula_reference 测试 → help → wiring + backlog ✅

备选替换仅当 Planner 证明原项不可行（见 research §2）；**不得减项**。

DoD 文件：`docs/research/goal-wave-2026-08-23-algorithm-wave5.md`（逐项 `[x]`）  
Verify：`tools/verify_algorithm_wave5_track.py`  
测试：`tests/algorithm_wave5_track_test.cpp`

---

## 明确不做
G3 Graph Builder、G4 Report Card、G5 大拆、TreeNet/AutoML、嵌 R/Python、Minitab golden、Automated Capability 向导、GLM Mixed/MANOVA 全量、破坏导入契约。

## 已完成水位（勿重做算法）
Wave-2～4、P0–P2 主项、G1/G2/G6、Menu IA。参考 backlog 与 `verify_algorithm_wave4_track.py`。

---

## 必读（严格按序）
1. `docs/research/goal-wave-2026-08-23-algorithm-wave5-plan-and-mega-prompt.md`
2. `docs/research/algorithm-wave5-market-formula-research-2026-08-23.md`
3. `docs/research/goal-execution-framework.md`
4. `docs/research/minitab-market-algorithm-backlog.md`
5. `docs/research/next-wave-algorithms-charts-ml-oss.md`
6. `docs/research/deferred-capability-agreement.md`
7. 同类 domain：`cart_tree` / `reliability` / `doe_*` / `accelerated_life`
8. `src/application/analysis_service.*`、`src/ui/analysis_commands.*`
9. 导入与 `row_visibility`（complete-case、source_row、A→B）
10. `docs/algorithm-wiring-index.md`、`samples/product_evolution/unified_track_acceptance_plan.md` §2

Implementer 必载：`.agents/skills/cpp-coding/SKILL.md`  
禁止误读：`待修改.md`、`build/`

---

## 架构约束
```
ui/analysis_commands → application/AnalysisService → domain/statistics（纯 C++）
interpretation 只读 Facts
禁止：domain→Qt；infrastructure→ui
禁止：破坏 complete-case / source_row / A→B / hidden≠excluded
新命令必须带 menu_path + menu_group（Menu IA）
```

---

## 四角色团队（互相监督 · 串行门禁）

| 顺序 | 角色 | subagent | 必须产出 | 门禁 |
|------|------|----------|----------|------|
| 1 | **Planner** | Task explore | 4 项映射表（文件归属+表形+测例+导入影响）；确认无 id 冲突 | 无表禁止写 domain |
| 2 | **Implementer** | Task generalPurpose | 按 W5-1→4 顺序完整竖切；cpp-coding | 禁止跳过 help/interp/测试 |
| 3 | **Tester** | Task shell | wave5 测试 + verify PASS；回归 wave4 + menuIA | verify 未 PASS 禁止 Checker |
| 4 | **Checker** | bugbot 或主 agent | Diff vs DoD；无 Critical | Critical→Implementer |

子 Agent 结尾：`文件列表 | DoD [x/ ] | 风险一行 | 是否破坏导入A→B/wave4 verify`

Orchestrator：CreateGoal 一次；立刻开干；连续到 4 项全完；仅 Checker APPROVE + verify PASS 才 complete。

---

## 禁止偷懒
goal-execution-framework.md §6（1–13）+ 本计划 §5（14–23）。
重点：禁止单算法 complete；禁止 golden；禁止嵌 Python；禁止破坏导入；禁止无 Primary URL。

---

## 如何验收

### 脚本门
```powershell
python tools/verify_algorithm_wave5_track.py
python tools/verify_algorithm_wave4_track.py
python tools/verify_ui_menu_ia_track.py
```

### 人手门（告知用户）
Rebuild → 分别打开四命令：随机森林 / Weibayes / Taguchi 设计生成 / 分布计算器 → 看表+图+诊断；Taguchi 生成矩阵后换文件确认排除不串。

---

## 如何记录
1. `goal-wave-2026-08-23-algorithm-wave5.md` DoD 全 `[x]`
2. backlog 对应行 ✅/⚪
3. wiring-index + acceptance §2 增 Wave-5 行
4. 每项 `p5_*.md` 含访问日期与 Primary URL
5. 结束告知用户文件清单与 Rebuild 手测点

---

## 启动顺序（本回合立刻执行）
1. CreateGoal（写清 W5-1～4 + verify）
2. TodoWrite
3. Planner explore（无计划表禁止改代码）
4. Implementer → Tester → Checker
5. 全绿 UpdateGoal complete

中间不要换模型。开始执行。
````

---

**文档状态：** 2026-08-23 首版；与 G6/G3 计划同构，供算法扩展会话直接粘贴 §9。
