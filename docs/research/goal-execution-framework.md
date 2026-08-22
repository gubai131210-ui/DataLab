# DataLab Goal 执行框架（大批量 · 多 Agent · 可持续）

> 研究日期：2026-08-22 · 访问日期：2026-08-22（UTC+8）  
> 用途：**长时 `/goal` 的权威操作手册**——一次 goal 可连续交付多算法 / 多 Track / 多优化，**不在单项完成后停住**。  
> 验收节奏：[`samples/product_evolution/unified_track_acceptance_plan.md`](../../samples/product_evolution/unified_track_acceptance_plan.md)  
> 产品队列：[`product-evolution-market-ux-architecture-research.md`](product-evolution-market-ux-architecture-research.md) §7  
> 算法队列：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) §12 + [`next-wave-algorithms-charts-ml-oss.md`](next-wave-algorithms-charts-ml-oss.md)

---

## §0 设计原则（可持续）

| 原则 | 含义 |
|------|------|
| **Wave 不碎步** | 一个 `/goal` 锁定 **1–3 个 Wave**（每 Wave **3–6 项**），Wave 内连续竖切，**Wave 结束才停** |
| **Research 先行** | 每项开工前：网上查 **Primary URL** → 写/更新 `docs/research/*.md`（含访问日期） |
| **竖切不变** | research → domain → Facts → service → commands → interpretation → tests → help → wiring |
| **脚本门、人手门分离** | Wave 交付 = **脚本预检 PASS**；Qt Creator = **统一验收门**（你准备好后一次跑） |
| **登记即可持续** | 每闭环一项：backlog ✅、wiring-index、acceptance §2、verify 脚本——避免「只有代码没有地图」 |
| **formula_reference ≠ golden** | 参考 Minitab/JMP **表形/输出结构**；测试标 `# source: formula_reference` |

### 外部参考（多 Agent 协作 · 2026 实践）

| 主题 | URL | 访问 | 采纳到本框架 |
|------|-----|------|--------------|
| Multi-agent 协调与验证边界 | https://www.augmentcode.com/guides/multi-agent-ai-software-development | 2026-08-22 | 编排者不写大段代码；子任务有 DoD；Review 独立 |
| Git worktree 并行隔离 | https://www.augmentcode.com/guides/git-worktrees-parallel-ai-agent-execution | 2026-08-22 | 并行 explore/review 用 Task；主 agent 合并 |
| 编排者委派模式 | https://levelup.gitconnected.com/what-371-git-worktrees-taught-me-about-multi-agent-ai-36d4d61acfb5 | 2026-08-22 | 小任务 + 精确 DoD + 脚本预检代替口头「完成」 |

---

## §1 Wave 与 Goal 粒度

### 1.1 推荐批量

| Goal 类型 | 单 Wave 项数 | 单 Goal Wave 数 | 示例 |
|-----------|-------------|----------------|------|
| **算法竖切** | 3–5 command id | 1–2 Wave | Wave-1: nominal_logistic + nonparametric_capability + … |
| **算法深化** | 4–6 command id | 1 Wave | 多个 🟡 → ✅（表形对齐，非 golden） |
| **产品 Track** | 1–2 Track（G3 可独占） | 1–2 Wave | Wave-1: G3；Wave-2: G4+G5 文档拆分 |
| **架构优化** | 1 大项 + 2–3 小项 | 1 Wave | G5 拆分 + 2 个命令迁移回归 |
| **混合** | 2 算法 + 1 产品 | 1 Wave | 仅当共享 domain/报告层 |

**禁止：** 一个 `/goal` 只交付 1 个算法就 `UpdateGoal complete`（除非用户显式「只做这一项」）。

### 1.2 Wave 生命周期

```
锁定 Wave 清单（plan md）
  → 网上调研 Primary URL（每项至少 1 条）
  → 并行 explore 子 agent（可选，见 §3）
  → 顺序/并行竖切实现（主 agent 或 worker）
  → 扩展 verify_*_track.py + CMake 测试
  → 更新 backlog / wiring / acceptance §2
  → 脚本预检 PASS → Wave 完成
  → 若 Goal 还有下一 Wave → 立即开始，不等人手 Qt
  → 全部 Wave 完成 → Goal complete
```

---

## §2 多 Agent 协作（Cursor / DataLab 映射）

### 2.1 角色

| 角色 | 谁 | 职责 | 禁止 |
|------|-----|------|------|
| **Orchestrator** | 主 `/goal` 对话 | 锁 Wave、写 plan、合并 diff、更新文档、跑 verify | 跳过 research；只做 UI 壳 |
| **Researcher** | `Task` + `explore` / `generalPurpose` | 查 backlog、读 wiring、对标 Minitab 表形 | 改 domain 代码 |
| **Implementer** | 主 agent 或 `Task` + `generalPurpose` | 竖切 domain/service/commands | 跳过 interpretation/help |
| **Reviewer** | `Task` + `bugbot` | 分支 diff 对照 DoD 与 §9 禁止偷懒 | 替代脚本预检 |
| **Verifier** | `Task` + `shell` 或主 agent | 跑 `verify_*_track.py`、grep 门禁 | 代替 Qt Creator 统一门 |

### 2.2 何时并行子 Agent

| 场景 | 并行策略 |
|------|----------|
| Wave 启动前 | 1× `explore`：扫 backlog §12 + `analysis_commands.cpp` 已有 id |
| 多项无依赖 | 2× `generalPurpose`：**禁止**同时改同一文件；按 domain 文件分片 |
| Wave 结束前 | 1× `bugbot`（额度允许时）+ 主 agent 跑 verify |
| 大目录 research | 1× `explore` + WebSearch Primary URL → 主 agent 写 research md |

**纪律：**

- 子 agent **不得**缩小 Goal 范围（「只做第一项」除非 orchestrator 改 plan）。  
- 子 agent 输出：**文件路径 + DoD 勾选 + 未决风险**；主 agent 合并进 Wave plan。  
- **不要**每算法开新 Cursor 会话；同一 `/goal` 跨 turn 延续至 Wave/Goal 完成。  
- 用户中文路径：**不**在 agent shell 强跑 cmake/ctest；编译由用户在 Qt Creator 统一做。

### 2.3 子 Agent 提示词片段（粘贴到 Task description）

```text
你是 DataLab Wave {N} 的 {Researcher|Implementer|Reviewer}。
Goal 文件：docs/research/goal-wave-{date}-{name}.md
禁止缩小范围；禁止 Minitab golden；禁止 domain 依赖 Qt。
交付：变更文件列表 + 每项 DoD 勾选 + 风险一行。
若改 domain/service 较多：遵循 .agents/skills/cpp-coding/SKILL.md
```

---

## §3 调研纪律（每项必做）

1. **Primary URL**：Minitab support / NIST / 官方文档优先（写进 research md 表格）。  
2. **访问日期**：UTC+8，与 research md 文首一致。  
3. **表形清单**：列出 Minitab 输出表/图 **名称与列**（不抄数值）。  
4. **产品边界**：明确「不做」项（与 `deferred-capability-agreement.md` 一致）。  
5. **登记**：新 command id 先写入 `minitab-market-algorithm-backlog.md` 或 roadmap，再写代码。

模板见 [`docs/research/p3_best_subsets_regression.md`](p3_best_subsets_regression.md)。

---

## §4 竖切 DoD（单项）

- [ ] `docs/research/p*_*.md`（Primary URL + 日期）  
- [ ] `src/domain/**` 纯 C++  
- [ ] `*Facts` + `AnalysisService::*` + `analysis_commands`  
- [ ] `interpretation_service` + catalog 双语（part 拆分 >500 条）  
- [ ] `algorithm_help.json` + `docs/algorithm-wiring-index.md`  
- [ ] CMake 测试 + `# source: formula_reference`  
- [ ] complete-case / `source_row` / A→B 失效（测试或既有回归覆盖）  
- [ ] backlog 行 ✅ + acceptance §2 行更新  

---

## §5 脚本与验收

| 脚本 | 用途 |
|------|------|
| `tools/verify_*_track.py` | Wave 交付门（每个 Wave 一个，可递增 wave 号） |
| `tools/print_acceptance_status.py` | 仓库级静态门 |
| `samples/product_evolution/unified_track_acceptance_plan.md` | 统一 Qt Creator 门（§3–§6） |

**Wave 完成条件：** 对应 `verify_*` **PASS** + acceptance §2 交付列 ✅ + plan md 该项 `[x]`。

---

## §6 禁止偷懒（Goal 必粘贴）

1. 禁止只做 UI 壳不算 domain/Facts  
2. 禁止跳过 interpretation 与 catalog 双语  
3. 禁止把 Minitab 数值当 golden  
4. 禁止单页堆叠超过一层主流程控件  
5. 禁止破坏 `row_visibility` hidden/excluded 语义  
6. 禁止 infrastructure 新增对 ui 的 include  
7. 禁止合并 customer/engineer/audit 为单模板  
8. 禁止省略 help catalog / `algorithm_help.json`  
9. 禁止大 catalog 单 TU（>500 条）  
10. 禁止宣称 PDF/A·UA 合规无验证器  
11. 禁止每 Wave 强制停 Qt Creator 才允许下一 Wave  
12. **禁止 Goal 在只完成 1 个算法/1 个小优化后标记 complete**（除非用户显式限定）  
13. **禁止跳过网上 Primary URL 调研**（至少 WebSearch + 写入 research md）  

---

## §7 下一 Wave 候选（2026-08-22 快照）

> 开 Goal 前以 **当前** backlog §12 为准；下表仅为建议池。

### 算法 Wave-2（P3 缺口）

| 优先级 | command / 主题 | 类型 |
|--------|----------------|------|
| 高 | `nominal_logistic` | 新增 |
| 高 | `nonparametric_capability` | 新增 |
| 中 | 可靠性 ALT / 保修（P3） | 新增（窄化） |
| 中 | `stepwise_regression` 深化（AICc 表形） | 深化 |
| 低 | GLM / Mixed（登记后分 Wave） | 大 gap，勿与上表同 Wave |

### 产品 Wave（G Track）

| Track | 说明 |
|-------|------|
| G3 | Graph 受控 Builder（**独占 1 Wave**） |
| G4 | 4-plot / Report Card |
| G5 | AnalysisService 拆分 |
| G6–G8 | Wizard / 监视 / Worksheet |

### 优化 Wave（可与 G5 搭配）

- AnalysisService 按命令族拆 cpp（architecture-review §3.1）  
- catalog JSON 懒加载（§6 性能）  
- 报告导出裁剪 profile 统一  

---

## §8 文档维护

| 事件 | 更新 |
|------|------|
| 新 Wave 启动 | 新建 `docs/research/goal-wave-YYYY-MM-DD-{name}.md` |
| Wave 完成 | plan 勾选 + acceptance §2 + verify 脚本 |
| Goal complete | backlog §12 + 本文件 §7 快照（可选） |
| 策略变更 | 本文件 + `unified_track_acceptance_plan.md` §7 |

---

**文档状态：** 2026-08-22 首版；算法批 A1–A3 已交付，见 `algorithm-batch-2026-08-22-plan.md`。

---

## §9 Mega `/goal` 提示词（直接复制）

### 9.1 算法 Wave-2（推荐 · 4 项一批）

```markdown
/goal
## 本 Goal 范围（全部 Wave 完成才 complete — 禁止做 1 项就停）
**Wave-1（4 项算法竖切）** — 从 backlog §12 ❌/🟡 锁定：
1. `nominal_logistic`（新增）
2. `nonparametric_capability`（新增）
3. `stepwise_regression` 深化（AICc/BIC 表形，非 golden）
4. 可靠性窄化一项（如 accelerated life 或 warranty，按 research 评估选 1）

每项：WebSearch Primary URL → research md → domain → Facts → service → commands → interpretation → test → help。

## 多 Agent 协作
- 启动：Task explore(thorough) — backlog + analysis_commands + wiring-index，输出重复 id 风险
- 调研：主 agent WebSearch → 每项 research md（Minitab 表形）
- 实现：主 agent 连续竖切；domain/service 多用 `.agents/skills/cpp-coding/SKILL.md`
- Wave 末：Task bugbot（diff: branch changes）+ `python tools/verify_algorithm_wave2_track.py` PASS

## 架构 / 验收 / 禁止偷懒
- 分层 ui→application→domain；新页不堆控件；ImportPlan/source_row/A→B 失效必测
- 验收：连续交付 · 末尾统一 Qt Creator（unified_track_acceptance_plan.md §3–§6）
- 本 Wave 交付：verify PASS + acceptance §2 + backlog ✅
- 粘贴 goal-execution-framework.md §6 全文
- 不要 cmake/ctest/commit/push；改完告诉我本地 Qt Creator 测

## 必读
docs/research/goal-execution-framework.md
docs/research/minitab-market-algorithm-backlog.md §12
docs/research/next-wave-algorithms-charts-ml-oss.md
samples/product_evolution/unified_track_acceptance_plan.md
docs/research/comprehensive-analytics-roadmap.md

## 交付物
docs/research/goal-wave-2026-08-22-algorithm-wave2.md
各 p*_research.md + verify 脚本 + wiring + acceptance §2
```

### 9.2 产品 Wave（G3 独占 + G4 可选第二 Wave）

```markdown
/goal
## Wave-1（必须完成）
**G3 Graph 受控 Builder** — 独立 `GraphBuilderPage`；分面+geom；对接 graph_service
- 调研 Primary URL：Minitab Graph Builder + JMP Graph Builder（写 research md）

## Wave-2（同一 Goal，Wave-1 verify PASS 后立即开始）
**G4 4-plot / Report Card** — AssumptionCheck Facts + 假设面板

## 多 Agent
- explore：graph_service / analysis_chart_widget 现有能力
- bugbot：Wave 末 UI 是否单页堆控件

## 禁止 / 验收 / 必读
同 goal-execution-framework.md §2–§6 + product-evolution §7 §9
不要每 Track 停 Qt Creator
```

### 9.3 架构优化 Wave（G5 + 回归）

```markdown
/goal
## Wave-1
1. **G5 AnalysisService 拆分** — 按命令族拆 cpp（architecture-review §3.1），dispatch 表不变
2. **回归**：已有 command id 脚本预检仍 PASS
3. research md：拆分边界 + 文件 ownership 表

## 多 Agent
- explore：analysis_service.cpp 行数/命令族分组建议
- 主 agent：拆分 + 最小 diff
- bugbot：无行为变更声明 vs 测试覆盖

禁止：拆分同时改算法逻辑；禁止单 PR 混 G3 UI
```

### 9.4 混合 Wave（仅当共享 domain）

```markdown
/goal
## Wave-1（2 算法 + 1 小优化）
- 算法 A、B：完整竖切
- 优化：report 导出 footnote 统一 / catalog part 拆分（择一）

同一 verify 脚本覆盖；Goal complete = Wave 全 DoD + verify PASS
```
