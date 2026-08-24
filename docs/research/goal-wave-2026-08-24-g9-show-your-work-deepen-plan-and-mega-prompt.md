# Wave：G9-D 验算轨迹深化（分步颗粒度 · 能做尽做）计划与 Mega `/goal` 提示词（2026-08-24）

> 访问日期：2026-08-24（UTC+8）  
> 调研正文：[`g9-show-your-work-deepen-research-2026-08-24.md`](g9-show-your-work-deepen-research-2026-08-24.md)  
> 执行框架：[`goal-execution-framework.md`](goal-execution-framework.md)  
> DoD 骨架：[`goal-wave-2026-08-24-g9-show-your-work-deepen.md`](goal-wave-2026-08-24-g9-show-your-work-deepen.md)  
> 前序框架（勿回退）：[`goal-wave-2026-08-23-g9-formula-substitution.md`](goal-wave-2026-08-23-g9-formula-substitution.md)  
> G1（静态，勿合并）：[`g1-g2-formula-registry-chart-copy.md`](g1-g2-formula-registry-chart-copy.md)  
> **不做本 Goal：** G3、G4 全量、G5 大拆、嵌 R/Python、Minitab golden、TreeNet/AutoML、Cassini AGPL、算法 Wave-6 新命令批量

---

## §0 给 Orchestrator 的一页摘要

| 维度 | 内容 |
|------|------|
| **本 Goal 名称** | G9-D：验算轨迹深化（Show Your Work Depth）— **分步颗粒度 · A/B/C 能做尽做** |
| **Complete 条件** | SYW-A～SYW-J **全部**完成；深度矩阵 **A 类几乎全 L3**（例外≤15% 且登记）；新 verify **PASS**；旧 g9/wave4/5/menuIA/g1g2 回归 PASS |
| **禁止缩小** | 禁止只深化 3～10 个试点就 complete；禁止「其余以后再说」；禁止继续用 `"xxx 主公式"` 冒充实质绑定 |
| **人手门** | 禁止 agent 强跑 cmake/ctest；完成后告知用户 Qt Creator Rebuild 自测 |
| **四角色** | Planner → Implementer → Tester → Checker（串行门禁） |
| **UI** | 保留四页；页3 升级为分步求值表；**禁止单页堆控件** |

### 用户诉求映射

| 诉求 | 落实 |
|------|------|
| 每个变量计算过程中的值 | Facts 优先填 bindings；禁「见结果表」主路径 |
| 分步颗粒度 | `ComputationStep` 含 order/expression_before/after/value；页3 步骤表 |
| 能做尽做 | A 类尽量 L3；B/C 最低 L2；D=L1；E=L0 |
| 网上总结 | research §1 |
| 四角色 + 足够测试 | §4 / §6 |
| UI 不堆 | §5 |

---

## §1 框架结构（团队 Agent）

```
Orchestrator（/goal 主对话）
  CreateGoal → TodoWrite(SYW-A..J + 四角色)
       │
       ▼
  Planner (explore) ──门禁──► 无「深度矩阵初稿 + L2 例外清单 + 文件归属 + 页3线框」禁止写 domain
       │
       ▼
  Implementer (generalPurpose) ──按 SYW-A→J；加载 cpp-coding
       │  模型扩展 → 试点契约 → 分族深度替换 stub → UI 页3 → verify 加严
       ▼
  Tester (shell) ── 新 deepen verify + 旧 g9 + wave4/5 + menuIA + g1g2
       │
       ▼
  Checker (bugbot/主 agent) ──Diff vs DoD；抽检 stub 残留；UI 是否堆页
       │
       ▼
  UpdateGoal complete（仅全绿）
```

**子 Agent 统一结尾：**

```text
文件列表 | DoD [x/ ] | 风险一行 | stub残留数 | L3覆盖率 | 是否破坏导入A→B
```

---

## §2 锁定交付（SYW-A～SYW-J）

| ID | 交付 | Complete 证据 |
|----|------|----------------|
| **SYW-A** | 扩展 `ComputationStep`（order/expression_*/value）+ 序列化 + 页3「分步求值」表 + 试点≥5 命令 L3 契约 | 测例 + UI |
| **SYW-B** | 能力/质量工具族：**消灭「主公式」stub**，尽量 L3 | 深度矩阵 |
| **SYW-C** | 控制图族：限值公式 L3（UCL/LCL/中心线等） | 同上 |
| **SYW-D** | 基础统计/检验/非参数：尽量 L3 | 同上 |
| **SYW-E** | 回归/多变量/ML：回归/GLM 类 L3；树/森林允许 L2+诚实 notes（路径摘要，非全树） | 同上 |
| **SYW-F** | 可靠性/寿命：尽量 L3 | 同上 |
| **SYW-G** | DOE/RSM/Taguchi 分析：L3；纯设计生成：L2/L3 规则绑定 | 同上 |
| **SYW-H** | MSA：尽量 L3 | 同上 |
| **SYW-I** | 图形 L1 真实 N；工具命令（分布计算器/功效）L2/L3 | 同上 |
| **SYW-J** | `g9-show-your-work-depth-matrix.md` + verify 加严 **0 非法 stub** | 脚本 PASS |

**豁免（仅 E）：** `tests`、`rule_policy`。  
**L2 例外：** Planner 书面清单，A 类 ≤15%，每条理由（迭代法/Bootstrap/全树路径等）。

---

## §3 优先阅读清单（严格顺序）

| # | 路径 | 用途 |
|---|------|------|
| 1 | 本文件 + `g9-show-your-work-deepen-research-2026-08-24.md` | 锁定 |
| 2 | `goal-execution-framework.md` | Wave / 禁止偷懒 |
| 3 | `formula-substitution-show-your-work-research-2026-08-23.md` | 前序边界 |
| 4 | `g9-formula-substitution-coverage-matrix.md` | 旧覆盖（须升级 depth 列） |
| 5 | `src/domain/quality_types.h`（`ComputationTrace`/`ComputationStep`） | 扩展字段 |
| 6 | `src/application/computation_trace_attach.cpp` | **重写质量主战场** |
| 7 | `tools/_gen_g9_computation_trace_attach.py` | **禁止再用它灌 stub**；可改生成器产出 L3 骨架但须手填 Facts |
| 8 | `src/ui/formula_substitution_dialog.*` | 页3 升级 |
| 9 | `src/infrastructure/output_serialization.cpp` | 新字段 round-trip |
| 10 | 各族 `*Facts` + `analysis_service` 填充点 | Facts 权威源 |
| 11 | `resources/help/algorithm_help.json` | plain_formula 文案 |
| 12 | `.agents/skills/cpp-coding/SKILL.md` | 改 C++ 时加载 |
| 13 | `deferred-capability-agreement.md` | 勿做清单 |

---

## §4 四角色门禁

### Planner（explore）

交付物缺一不可，否则 Implementer **不得开工**：

1. 全命令 **depth 矩阵初稿**（L3/L2/L1/L0）  
2. A 类 **L2 例外清单**（≤15%）  
3. 页3 线框（步骤表列）  
4. 文件归属（谁改 attach / Facts / UI / verify）  
5. 试点 5 命令名单（含已有 3 + 至少 2 新：建议 `imr`、`regression` 或 `two_sample_t`、`gage_rr`）

### Implementer

- 按 SYW-A→J；每完成一族更新矩阵 depth  
- **优先 Facts → fmt_num → bindings/steps**；表刮取仅次级  
- 加载 cpp-coding  
- 禁止缩小范围；禁止只改 UI  

### Tester

```text
python tools/verify_g9_show_your_work_deepen_track.py
python tools/verify_g9_formula_substitution_track.py
python tools/verify_algorithm_wave5_track.py
python tools/verify_algorithm_wave4_track.py
python tools/verify_ui_menu_ia_track.py
python tools/verify_g1_g2_track.py
```

### Checker

- Diff vs DoD；抽检 ≥20 个原 stub 命令已无 `"主公式"`  
- UI 四页未堆叠  
- 无 Critical → 才允许 UpdateGoal complete  

---

## §5 UI 分页（强制）

见 research §3。要点：

- 页3 标题改为 **「分步求值」**（或保留「代入预览」但主控件必须是步骤表）  
- 列：序 | 说明 | 代入前表达式 | 代入后 | 得数  
- 页底显示 `结果 symbol = value`  
- **禁止**与页2/页4合并  

---

## §6 测试与契约

| 层 | 要求 |
|----|------|
| 模型 | `ComputationStep` 新字段序列化 round-trip |
| Domain/attach | 每族 ≥1 L3 QtTest；试点 ≥5 契约 |
| Verify 新脚本 | 拒 `"主公式"`（A/B/C）；拒 A-L3 bindings 含「见结果表」；拒 L3 而 steps&lt;2 |
| 覆盖 | depth 矩阵每命令有 depth |
| 回归 | 旧脚本全 PASS |
| 证据 | `# source: formula_reference`；非 golden |

---

## §7 禁止偷懒（本 Goal 必粘贴 · 在框架 1–13 之上）

14. 禁止只深化试点就 UpdateGoal complete  
15. 禁止 `"xxx 主公式"` / 仅 n·stat·p 冒充实质绑定  
16. 禁止 A 类主路径「见结果表」  
17. 禁止 verify 只查「有没有 attach 字符串」不查深度  
18. 禁止再用旧生成器无脑灌 stub 后勾 DoD  
19. 禁止单页堆变量表+步骤+出处+G1 搜索  
20. 禁止输出主页半屏展开面板  
21. 禁止把 D 类降级政策套到 A 类  
22. 禁止跳过 Facts、只刮表  
23. 禁止破坏 A→B / complete-case / source_row / hidden≠excluded  
24. 禁止 domain→Qt；infrastructure→ui  
25. 禁止嵌 R/Python、Minitab golden、Cassini AGPL 并入  
26. 禁止本 Goal 夹带 G3/G4/G5/算法 Wave-6 大批新命令  
27. 禁止 L3 步骤只有空 `description` 而无 expression/value  
28. 禁止矩阵写「实质绑定」却 depth 空白  
29. 禁止 agent 强跑 cmake/ctest（中文路径；用户 Qt Creator 自测）  
30. 禁止 Checker 未抽检 stub 残留就 complete  

---

## §8 Mega `/goal` 提示词（复制到新对话）

````text
你是 DataLab Orchestrator。用 `/goal` 模式一次做完 **G9-D 验算轨迹深化（Show Your Work Depth）**。

## 用户锁定（不可改）
1. 要能验算：公式里每个变量在**本次计算过程中的真实取值**。
2. **分步颗粒度**：Excel Evaluate 风格 — 每步有代入前表达式、代入后表达式、得数。
3. **深度：能做的尽量都做** — A/B/C 类尽量 L3 分步验算；不得只做几个试点。
4. UI：该新建/强化页面就新建；**禁止单页堆控件**；保留四页分层，页3=分步求值表。
5. 四角色串行：Planner → Implementer → Tester → Checker；互相监督。
6. 中文路径：禁止强跑 cmake/ctest；完成后告诉用户自己 Qt Creator Rebuild 测试。
7. 不要 commit/push，除非用户另说（若用户规则要求 push 文档则可只推 md）。

## 必读（严格顺序）
1. docs/research/goal-wave-2026-08-24-g9-show-your-work-deepen-plan-and-mega-prompt.md
2. docs/research/g9-show-your-work-deepen-research-2026-08-24.md
3. docs/research/goal-execution-framework.md
4. docs/research/formula-substitution-show-your-work-research-2026-08-23.md
5. docs/research/g9-formula-substitution-coverage-matrix.md（须升级 depth 列或另建深度矩阵）
6. src/domain/quality_types.h（ComputationTrace / ComputationStep）
7. src/application/computation_trace_attach.cpp（现状：~79 个「主公式」stub，必须消灭）
8. tools/_gen_g9_computation_trace_attach.py（禁止再用它无脑灌 stub）
9. src/ui/formula_substitution_dialog.*
10. src/infrastructure/output_serialization.cpp
11. src/application/analysis_service.*（各族 *Facts 填充点 = 真值权威源）
12. resources/help/algorithm_help.json（formula_blocks）
13. docs/research/deferred-capability-agreement.md
14. samples/product_evolution/unified_track_acceptance_plan.md（§2 G9-D 行）
15. .agents/skills/cpp-coding/SKILL.md（改 C++ 时）

## CreateGoal
objective: G9-D 验算轨迹深化：分步求值 + Facts绑定 + A/B/C能做尽做L3 + verify加严拒stub

## TodoWrite
SYW-A..SYW-J + planner/implementer/tester/checker

## 交付定义（摘要）
- 扩展 ComputationStep：order, description, expression_before, expression_after, value
- 优先从 *Facts fmt_num 填 bindings/steps；禁止 A 类主路径「见结果表」
- 消灭 attach 中「xxx 主公式」模板对 A/B/C 的使用；Checker 前 grep 残留须为 0（A/B/C）
- UI 页3：分步求值表（序|说明|代入前|代入后|得数）；禁止与变量/出处同屏堆叠
- 深度矩阵 docs/research/g9-show-your-work-depth-matrix.md：每命令 depth∈{L3,L2,L1,L0}
- A 类 L2 例外 ≤15% 且书面理由
- 新 verify：tools/verify_g9_show_your_work_deepen_track.py PASS
- 新 QtTest：tests/g9_show_your_work_deepen_track_test.cpp（每族≥1 + 试点契约）并挂 CMake
- 回归：verify_g9_formula_substitution + wave4 + wave5 + menuIA + g1g2 PASS
- 更新 docs/algorithm-wiring-index.md §8.1、samples/product_evolution/unified_track_acceptance_plan.md §2 G9-D、DoD 全 [x]

## 分族顺序
SYW-A 模型+UI+≥5试点契约
→ B 能力 → C 控制图限值 → D 基础统计 → E 回归/ML → F 可靠性 → G DOE → H MSA → I 图形/工具 → J 门禁

## 网上调研
执行前对照 research md Primary URL；若缺公式则 WebSearch Minitab Methods & Formulas / NIST，写入 research，访问日期 UTC+8。禁止无公式瞎编。

## 禁止偷懒（全文粘贴 · 必须遵守）
框架（goal-execution-framework §6）：
1. 禁止只做 UI 壳不算 domain/Facts
2. 禁止跳过 interpretation 与 catalog 双语（若本 Goal 触及文案）
3. 禁止把 Minitab 数值当 golden
4. 禁止单页堆叠超过一层主流程控件
5. 禁止破坏 row_visibility hidden/excluded 语义
6. 禁止 infrastructure 新增对 ui 的 include
7. 禁止合并 customer/engineer/audit 为单模板
8. 禁止省略 help / algorithm_help 必要更新（若触及）
9. 禁止大 catalog 单 TU（>500 条）
10. 禁止宣称 PDF/A·UA 合规无验证器
11. 禁止每 Wave 强制停 Qt Creator 才允许下一 Wave
12. 禁止 Goal 在只完成少数命令后标记 complete
13. 禁止跳过网上 Primary URL 调研
本 Goal 加严：
14. 禁止只深化试点就 UpdateGoal complete
15. 禁止「xxx 主公式」/ 仅 n·stat·p 冒充实质绑定
16. 禁止 A 类主路径「见结果表」
17. 禁止 verify 只查「有没有 attach 字符串」不查深度
18. 禁止再用旧生成器无脑灌 stub 后勾 DoD
19. 禁止单页堆变量表+步骤+出处+G1 搜索
20. 禁止输出主页半屏展开面板
21. 禁止把 D 类降级政策套到 A 类
22. 禁止跳过 Facts、只刮表
23. 禁止破坏 A→B / complete-case / source_row / hidden≠excluded
24. 禁止 domain→Qt；infrastructure→ui
25. 禁止嵌 R/Python、Minitab golden、Cassini AGPL 并入
26. 禁止本 Goal 夹带 G3/G4/G5/算法 Wave-6 大批新命令
27. 禁止 L3 步骤只有空 description 而无 expression/value
28. 禁止矩阵写「实质绑定」却 depth 空白
29. 禁止 agent 强跑 cmake/ctest（中文路径；用户 Qt Creator 自测）
30. 禁止 Checker 未 grep 确认 A/B/C「主公式」残留=0 就 complete

## 角色输出格式
每个子 agent 结尾：文件列表 | DoD勾选 | 风险一行 | stub残留数 | L3覆盖率 | A→B是否完好

## Complete 条件
仅当 SYW-A..J 全 [x]、深度矩阵合法、新 verify PASS、QtTest 已挂 CMake、回归 PASS、Checker 确认 stub 残留=0 且无 Critical。
然后 UpdateGoal complete，并提示用户：请在 Qt Creator Rebuild 后手测「公式代入」四页（尤其分步求值），中文路径勿用命令行强编。

中间不要换模型。开始执行。
````

---

## §9 验收话术（完成后给用户）

```text
G9-D 已脚本预检通过。请你本地：
1) Qt Creator Rebuild
2) 跑 capability / one_sample_t / imr / regression / gage_rr
3) 点「公式代入」→ 看页2真数值、页3分步表
4) 换含排除行的文件确认 A→B / complete-case 未坏
不要用命令行强编（中文路径）。
```

---

**文档状态：** 2026-08-24 首版；用户确认「分步颗粒度 + 能做尽做」后锁定。
