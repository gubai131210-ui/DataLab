# Goal：G-Trust — 高频命令参考实现 Golden（可信度 · 衔接现网 · 不声称 Minitab 商业对齐）

> **用途**：新开一场 Goal 对话时的**唯一权威操作手册**（本轮）。  
> **状态**：§0 已由用户拍板（2026-09-05 13:45 UTC+8），**决策锁定，禁止子 Agent 重问或改口**。  
> **方向来源**：[`datalab-next-direction-research-2026-09-05.md`](datalab-next-direction-research-2026-09-05.md) Track A / G-Trust  
> **母框架**：[`goal-execution-framework.md`](goal-execution-framework.md)  
> **证据权威**：[`VALIDATION_MATRIX.md`](VALIDATION_MATRIX.md)、[`deferred-capability-agreement.md`](deferred-capability-agreement.md)、[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md)  
> **现网 fixture 骨架（必须衔接，禁止另起炉灶）**：`tests/fixtures/minitab/`、`golden_loader.*`、`minitab_*_golden_test.cpp`  
> **并排方向 Canvas（可选对照）**：`~/.cursor/projects/d-QT-CppPrograms-DataLab/canvases/datalab-next-direction-analysis.canvas.tsx`

---

## §0 用户已拍板决策（2026-09-05）— 锁定

| # | 问题 | **用户决定（锁定）** |
|---|------|----------------------|
| Q1 | Golden 证据级别 | **B：`reference_implementation` 冻结为回归 golden**。可复用官方公开数据集 CSV 作输入；**禁止**在 UI/帮助/对外文档声称「与 Minitab 数值完全对齐 / vendor_oracle」。真·Minitab 导出留待后续 Goal |
| Q2 | 命令范围 | **A：默认高频包（10 课）**，见 §0.2；内部 Wave 只是施工队列，**不得**缩成「只做 imr」或推到下一 Goal |
| Q3 | 对不上时 | **A：允许改 domain 算法**（竖切修到容差内）+ 更新 wiring-index / backlog / VALIDATION_MATRIX；禁止 silently 改公式却不登记 |
| Q4 | 五 Agent | **Agent1 调研 → Agent2 计划 → Agent3 执行 → Agent4 测试 → Agent5 收尾（含代码 review）**。全程 **`model: "inherit"`，中途禁止换模型、禁止建议换模型** |
| Q5 | 顺带范围 | **无**。不做学习中心文案、Assistant、Graph Builder、LicenseAdmin moc、升 catalog v3、新开偏门算法 |
| Q6 | 容差与完成定义 | **按手册默认**（§4）：每命令 ≥1 条冻结主输出 golden；默认容差见 §4.2 |
| Q7 | 编译 / Git / 文档 | Agent 跑 **Python verify**；若环境已有 gtest/ctest 可跑相关 target，**中文路径不强跑易失败 cmake/package**（除非用户本轮明确要求）。Goal 结束 **必须 git commit + push**。文档路径即本文件，不改名。用户本机 Qt Creator / `package_dist` 抽查 |

**编排者纪律**：§0 已关闭。子 Agent **禁止**再问「要不要真 Minitab 导出 / 是否缩范围 / 是否换模型 / 是否另建验证框架」；若冲突，以本表为准。

### §0.1 现网硬约束（写进 Wave plan 文首）

| ID | 约束 |
|----|------|
| H1 | **必须衔接** `docs/research/VALIDATION_MATRIX.md` 证据类型定义：本 Goal 冻结项标 **`golden` ← `reference_implementation`**，**不是** `vendor_oracle` |
| H2 | **必须复用** `tests/fixtures/minitab/`（`converted/`、`expected/`、`golden_loader.*`、`SOURCE.md`、`EXPORT_GUIDE.md`）。禁止平行再建 `tests/fixtures/g_trust_v2/` 之类第二套 |
| H3 | 输入数据优先复用已有公开转换集（如 `PistonRingDiameter.csv`、`CrankshaftMovement.csv`、`UnansweredCalls.csv`、`PinLength.csv` 等）；新建 CSV 须登记 `SOURCE.md` |
| H4 | 期望值文件优先 TSV（`# section:` / `# config:`），与现有 `regression_golden.tsv` 同形；由 **Python 参考脚本生成**，禁止手抄无来源数字 |
| H5 | 测试入口衔接现有：`minitab_numerical_golden_test` / `minitab_formula_golden_test` / `minitab_fixture_test`；可扩展同一 target 或新增 `g_trust_*_test` **但须挂进现有 CMake 测试列表**，禁止孤儿 exe |
| H6 | 参考脚本放 `scripts/` 或 `tools/`，钉死依赖版本（注释写明）；脚本须可独立复跑生成 expected |
| H7 | 修改 domain 算法时：**竖切** domain → Facts/解释（若文案变）→ tests → 必要时 `algorithm_help` **仅当语义真变**；禁止改无关大文件（如乱改 `analysis_service.cpp` 上帝对象） |
| H8 | 学习中心 / 导入 / v2 catalog / 7B 隐藏 / 图形名实：**本 Goal 不碰** |
| H9 | UI/帮助文案禁止写「已与 Minitab 数值对齐」；可写「参考实现回归 / formula_reference / 待 vendor_oracle」 |
| H10 | backlog ⚪「待 golden」行：本 Goal 完成后改为可区分标记（例如「ref-golden 已冻」或在 VALIDATION_MATRIX 登记 ✅），**不得**把 ⚪ 直接改成暗示 vendor 对齐的 ✅ 而不改证据类型 |

### §0.2 命令锁表（Q2A 默认包，n=10，不得漏）

| # | command_id | 优先输入 fixture（可调整但须登记） | 主输出（至少冻一项） |
|---|------------|-----------------------------------|----------------------|
| 1 | `imr` | 学习中心/合成单值流或现有 SPI 教学集；或自建小 N 参考脚本输入 | CL / UCL / LCL（I 图）；可选 MR 限 |
| 2 | `xbar_r` | `converted/CrankshaftMovement.csv` 或 `CamshaftLength.csv` | Xbar/R 的 CL/UCL/LCL 或等价中心与限 |
| 3 | `capability` | `converted/PistonRingDiameter.csv`（LSL/USL/Target 见 SOURCE.md） | Cp / Cpk（及文档约定的 Pp/Ppk 若实现路径输出） |
| 4 | `capability_sixpack` | 同上或同源配置 | 六合一中与能力指数相关的主数字 + 诚实列出图组件契约 |
| 5 | `gage_rr` | 现有 MSA 交叉样例（学习数据或 samples）；须在 plan 钉死路径 | %GR&R 或 %Study Var / 区分类别数（ndc）中本实现已输出者 |
| 6 | `two_sample_t` | 自建或 samples 两列独立样本 | 均值差、CI、t、df、p（按现网 Facts 字段） |
| 7 | `normality_test` | 能力同源或正态/轻偏样本 | 选定方法的统计量 + p（方法名钉死） |
| 8 | `one_way_anova` | 自建或 samples 单因子 | SS/DF/MS/F/p 主表 |
| 9 | `p_chart` | `converted/UnansweredCalls.csv` | 中心线与变限（或平均 p、平均 n）主数字 |
| 10 | `between_within_capability` | 学习中心 `cap_between_within` 同源或等价 CSV | 组间/组内分量或对应能力指数（按现网输出） |

**计数**：10。Agent2 锁表不得删减；Wave 只是施工顺序。

---

## §1 本 Goal 要交付什么（产品语言）

当前状态：命令很多，但多数仍是 `formula_reference` 或「待 golden」。客户/审核问「数字能不能信」时，缺少**可复跑的参考实现冻结基线**。

本 Goal 做成：

1. **十条高频命令**各有至少一条 **可复跑** 的 `reference_implementation` → 冻结 `golden` fixture + C++/gtest 断言。  
2. **矩阵诚实**：`VALIDATION_MATRIX.md` 登记类型与状态；**不**声称 Minitab vendor 对齐。  
3. **对不上就修到容差内**（Q3A），并留下登记，禁止暗改。  
4. **衔接现网** loader / TSV / fixtures / CMake 测试，不另起验证宇宙。

**非目标**

- 真·Minitab `vendor_oracle` 导出对齐（留给后续 Goal）。  
- 新开偏门算法、学习中心润色、Assistant、Graph Builder、LicenseAdmin、升 v3。  
- 用浏览器代替用户本机 `package_dist`。  
- 中途换模型。

---

## §2 必须衔接的现有代码（禁止另起炉灶）

| 路径 | 本 Goal 允许 |
|------|----------------|
| `docs/research/VALIDATION_MATRIX.md` | **登记**本轮冻结行；更新证据类型/状态 |
| `docs/research/minitab-market-algorithm-backlog.md` | 对应命令行备注「ref-golden」；禁止伪造成 vendor ✅ |
| `docs/research/deferred-capability-agreement.md` | 只读或追加「本 Goal 已用 ref 冻结」指针；不推翻延后协议 |
| `tests/fixtures/minitab/**` | 扩展 converted/expected/SOURCE/EXPORT_GUIDE |
| `tests/fixtures/minitab/golden_loader.*` | 复用或小幅增强（容差/section） |
| `tests/minitab_numerical_golden_test.cpp` 等 | 扩展断言或新增同目录测试并挂 CMake |
| `scripts/*_reference.py` / `tools/*_reference.py` | 新增本轮参考脚本（可复跑生成 expected） |
| `src/domain/statistics/*` | **仅当** golden 对不上且属实现错误/容差内可修（Q3A） |
| `src/application/*` | 原则上不改；Facts 字段映射测试需要时定点 |
| `CMakeLists.txt` | 仅挂测试 target / 源文件，禁止大重构 |
| `tools/verify_*.py` | 新增或扩展 `verify_g_trust_golden_gate.py`（检查 expected 存在、SOURCE 登记、矩阵行、禁止 vendor 宣称） |

**禁止新建**：第二套 `fixtures/oracle_v2`、并行 golden_loader、学习中心第二帮助系统。

架构保持：

```
输入 CSV（fixtures/minitab/converted 或 samples）
  → Python reference script（钉版本）→ expected/*.tsv
  → C++ test + golden_loader → 调 domain API
  → VALIDATION_MATRIX 登记 golden←reference_implementation
```

---

## §3 五 Agent 流水线（一场 Goal；模型锁定 inherit）

```
Agent1 调研（网 + 现网 fixture/矩阵/测试学习；禁止改产品代码）
  → Agent2 详细计划（Wave 锁表 + 每命令证据清单 + §7；禁止改产品代码）
  → Agent3 执行（脚本 → expected → 测试 → 必要时修 domain）
  → Agent4 测试（verify gate + 列出用户 C++ target；能跑则跑 gtest）
  → Agent5 收尾（§8 勾选 + code review + commit/push + 提示 package）
```

### Agent 1 — 调研（Research）

**做什么**

1. 网上巩固：过程能力 / Gage R&R / 控制限 / 两样本 t / 单因子 ANOVA / P 图 / 正态性检验的**标准公式口径**（NIST、AIAG、经典教材 Primary URL；访问日期 UTC+8）。  
2. 通读现网：`VALIDATION_MATRIX.md`、`SOURCE.md`、`EXPORT_GUIDE.md`、`golden_loader`、`minitab_*_test.cpp`、相关 domain 头文件输出字段。  
3. 盘点每条锁表命令：已有 fixture？已有 formula 测试？缺口是输入、期望、还是 API？  
4. 产出：`docs/research/g-trust-reference-golden-research.md`

**DoD**

- [ ] Primary URL 表 ≥8 条 + 访问日期  
- [ ] 10 命令缺口表（输入/期望/API/风险）  
- [ ] 写清「reference_implementation ≠ vendor_oracle」边界  
- [ ] **禁止**改产品代码  

### Agent 2 — 计划（Plan）

产出：`docs/research/goal-g-trust-reference-golden-wave-plan.md`

必须含：文首 H1–H10、10 命令锁表、Wave 划分、每命令参考脚本路径与主输出字段、容差表、CMake/测试挂载点、§7 禁止偷懒全文、明确不改文件清单。

**建议 Wave（可微调，不得漏命令）**

| Wave | 内容 | 出口 |
|------|------|------|
| Wave-0 | 矩阵/目录约定、loader 小幅增强（若需要）、verify 骨架、EXPORT_GUIDE 增补本轮文件名 | 约定可执行 |
| Wave-1 | `imr` + `xbar_r` + `p_chart` | 3 条 ref-golden 测试 PASS |
| Wave-2 | `capability` + `capability_sixpack` + `between_within_capability` | 3 条 PASS |
| Wave-3 | `gage_rr` + `two_sample_t` + `normality_test` + `one_way_anova` | 4 条 PASS |
| Wave-4 | verify gate 全集、VALIDATION_MATRIX/backlog 登记、文档 | Agent4+5 |

**DoD**

- [ ] 10 id 全锁、不漏  
- [ ] 每命令写明：输入路径、脚本、expected、断言字段、容差  
- [ ] **禁止** Plan 阶段改产品代码  

### Agent 3 — 执行（Implement）

顺序强制：Wave-0 → 1 → 2 → 3 → 4。

每命令最小闭环：

1. 钉输入 CSV（hash 或行数/列名写入脚本头注释）  
2. 写/跑 reference 脚本 → 写入 `expected/*_ref_golden.tsv`（或扩展现有命名）  
3. C++ 测试：加载输入 → 调 domain → `compare_double`  
4. 对不上：按 Q3A 修 domain，再复跑脚本与测试  
5. 更新 SOURCE/EXPORT_GUIDE/VALIDATION_MATRIX 行  

**DoD（每 Wave）**

- [ ] 该 Wave 每命令 expected 可复跑生成  
- [ ] 测试非 `QSKIP` 空过（缺文件应 FAIL 或显式登记延后——本 Goal **禁止**用 QSKIP 冒充完成）  
- [ ] 无「已与 Minitab 对齐」文案  
- [ ] 禁止只手改 expected 却删掉生成脚本  

### Agent 4 — 测试（Test）

1. 落地并跑：`tools/verify_g_trust_golden_gate.py`（或并入现有 verify）硬门：  
   - 10 命令各有 expected + SOURCE/EXPORT 登记  
   - VALIDATION_MATRIX 含对应行且类型为 `golden`←`reference_implementation`  
   - 仓库内禁止新增「与 Minitab 数值对齐」学生/帮助可见宣称（grep 门）  
2. 若本机已有构建目录且 gtest 可跑：跑 `minitab_*_golden_test` / 本轮新 target；否则列出用户 Qt Creator 要编的 target 清单。  
3. **不强跑**中文路径易失败的全量 cmake/package。  

**DoD**

- [ ] Python gate PASS  
- [ ] C++ target 清单已写给用户（有跑则贴结果）  

### Agent 5 — 收尾（含代码 review）

1. 对照 §7 + §8；结构衔接检查（无第二套 fixture）。  
2. **代码 review**（可用 Task `bugbot` 或 `generalPurpose` + `model: inherit`）：扫本 Goal diff——容差是否过松、是否误改无关大文件、是否伪 vendor 宣称、脚本是否可复跑。  
3. review 驳回则定点返工，禁止顺手重构。  
4. **commit + push**；提示用户本机编测试 + 可选 `package_dist`。  
5. **禁止**收尾塞新功能。  

---

## §4 内容规格（验收口径）

### 4.1 每命令最低交付

| 项 | 要求 |
|----|------|
| 输入 | 路径稳定；列名与命令对话框角色一致；登记 SOURCE |
| 参考脚本 | 可复跑；打印/写出与 expected 一致；注释含公式口径 URL |
| expected | TSV 或现有 loader 支持格式；含 `# config` |
| C++ 测试 | 至少覆盖 §0.2「主输出」列；失败信息可读 |
| 矩阵行 | VALIDATION_MATRIX 有行；状态可达「已冻结」；类型正确 |

### 4.2 默认容差（Q6）

| 量类型 | 默认容差 | 说明 |
|--------|----------|------|
| 中心线 / 均值 / 系数 | `rel_tol=1e-4` 或 `abs_tol=1e-6`（取较宽但仍严） | 与 `GoldenTolerance` 默认同量级 |
| 控制限 UCL/LCL | `rel_tol=1e-4`（相对 \|expected\|） | 极近 0 时改用 abs |
| Cp/Cpk/Pp/Ppk | `abs_tol=1e-3` | 能力指数 |
| %GR&R / %Study Var | `abs_tol=0.05`（百分数点）或实现一致的比例尺 | plan 钉死单位 |
| t / F / 正态统计量 | `rel_tol=1e-4` | |
| p 值 | `abs_tol=1e-4`（或对极小 p 用相对） | 禁止只比「显著/不显著」布尔完事 |
| 整数 DF | 必须精确相等 | |

Agent2 可按命令收紧，**禁止**无说明放宽到「几乎必过」。

### 4.3 「变绿」定义（完成）

- 锁表 **10/10** 命令均有非跳过的自动化断言 PASS。  
- Python g-trust gate PASS。  
- VALIDATION_MATRIX 10 行已登记为 ref-golden 冻结。  
- Agent5 code review 无阻断项。  
- git commit + push 完成。  

---

## §5 施工队列（内部 Wave；必须全部做完）

见 Agent2 建议表。**10 命令必须全部做完。** 禁止「Wave-1 过了就算 Goal 完成」。

---

## §6 与 vendor_oracle 的边界（写进所有子 Agent 提示）

| 可以 | 不可以 |
|------|--------|
| 使用 Minitab **官方公开数据集**页面的 CSV 转换作**输入** | 把公开数据输入 + 自写脚本输出叫做「Minitab 对齐」 |
| 冻结 `reference_implementation` golden | 在帮助/README 写「数值已与 Minitab 一致」 |
| 后续 Goal 再接真导出 TSV | 本 Goal 要求用户必须导出 Minitab（Q1 已否） |

现有 `expected/regression_golden.tsv` 等若标注来自真导出，**保持**其证据类型，本 Goal 不降级也不冒充统一成 vendor。

---

## §7 禁止偷懒（Plan 必须粘贴）

1. **禁止**另起第二套 fixture/loader/验证宇宙。  
2. **禁止**只做 1～2 个命令就宣称 Goal 完成。  
3. **禁止**用 `QSKIP`/缺文件跳过冒充 PASS。  
4. **禁止**手写 expected 却无生成脚本。  
5. **禁止**声称 Minitab vendor 数值对齐。  
6. **禁止**对不上时只改测试容差到「一定过」而不修实现或登记。  
7. **禁止**暗改 domain 公式不更新矩阵/wiring/backlog。  
8. **禁止**改学习中心文案/v2/7B/导入/Graph Builder/Assistant。  
9. **禁止**中途换模型；所有 Task **`model: "inherit"`**。  
10. **禁止** Agent5 无 code review、无 commit/push 就 UpdateGoal complete。  
11. **禁止**强跑易失败中文路径全量 cmake/package（除非用户本轮要求）。  
12. **禁止**并行两人改同一大文件；禁止收尾塞无关重构。  
13. **禁止**把 `formula_reference` 测试改头换面假充本 Goal 交付。  
14. **禁止**新增虚假 command_id。  
15. **禁止**改 `algorithm_help.json` 公式语义，除非 domain 真变且登记。  

---

## §8 完成定义

- [ ] §0 已锁定（本文）。  
- [ ] 10 命令 ref-golden 全绿（非 QSKIP）。  
- [ ] VALIDATION_MATRIX / SOURCE / EXPORT_GUIDE 已更新且诚实。  
- [ ] Python g-trust gate PASS。  
- [ ] Agent5 code review 通过（无阻断）。  
- [ ] 无 vendor 对齐虚假宣称。  
- [ ] git commit + push；提示用户本机编 `minitab_*_golden_test`（及本轮新 target）并可选 `package_dist`。  

---

## §9 每阶段输出格式

变更文件列表 + DoD 勾选 + 风险一行 + go/no-go

### 子 Agent 提示词头

```text
你是 DataLab「G-Trust 参考实现 Golden」Goal 的 Agent{N}:{角色}。
权威手册：docs/research/goal-g-trust-minitab-golden-plan-and-mega-prompt.md
证据规则：VALIDATION_MATRIX.md；本轮只做 golden←reference_implementation，禁止声称 Minitab vendor 对齐。
必须衔接：tests/fixtures/minitab/、golden_loader、现有 minitab_*_test。
命令锁表 10 个：imr, xbar_r, capability, capability_sixpack, gage_rr, two_sample_t, normality_test, one_way_anova, p_chart, between_within_capability。
对不上允许修 domain（Q3A）但必须登记。
模型：inherit，禁止建议换模型。
禁止偷懒：见该手册 §7。
交付：文件列表 + DoD 勾选 + 风险一行 + go/no-go。
```

---

## §10 新对话开场粘贴（Mega Prompt）

> 把下面整段贴进**新 Goal 对话**首条（§0 已填，勿再问）。建议同时挂上 `/goal` skill。

```text
/goal

【Goal 启动】G-Trust — 高频命令参考实现 Golden（可信度）

权威手册（唯一决策源，先通读再动手）：
docs/research/goal-g-trust-minitab-golden-plan-and-mega-prompt.md

请立即用 CreateGoal 建立长跑目标，objective 写清本 Goal 名称与手册路径。
母框架：docs/research/goal-execution-framework.md
方向研究：docs/research/datalab-next-direction-research-2026-09-05.md
证据权威：docs/research/VALIDATION_MATRIX.md
现网骨架：tests/fixtures/minitab/ 、golden_loader、minitab_*_golden_test.cpp

【模型锁定 — 最重要】
- 全程只使用我当前对话所用模型。
- 所有 Task 子 Agent 必须 model: "inherit"。
- 中途禁止换模型，禁止建议换模型「省钱/加速」。

【§0 已拍板 — 禁止重问、禁止改口】
Q1 证据：reference_implementation 冻结为回归 golden；禁止声称 Minitab vendor 数值对齐
Q2 范围：默认 10 命令 — imr, xbar_r, capability, capability_sixpack, gage_rr, two_sample_t, normality_test, one_way_anova, p_chart, between_within_capability；Wave 只是队列，不得缩水
Q3 对不上：允许改 domain 修到容差内，并登记 wiring/backlog/VALIDATION_MATRIX
Q4 五 Agent：调研 → 计划 → 执行 → 测试 → 收尾（收尾必须含代码 review）；全程 inherit
Q5 不做：学习中心文案、Assistant、Graph Builder、LicenseAdmin、升 v3、新开偏门算法
Q6 容差与完成定义：按手册 §4；每命令 ≥1 条主输出冻结；禁止 QSKIP 冒充完成
Q7 编译/Git：Agent 跑 Python verify；若可能跑已有 gtest；中文路径不强跑易失败 cmake/package；结束必须 commit + push；文档即本手册不改名

【衔接现网 — 禁止另起炉灶】
- 扩展 tests/fixtures/minitab/（converted/expected/SOURCE/EXPORT_GUIDE）
- 复用 golden_loader；扩展 minitab_*_test 或同目录新测试并挂 CMake
- 参考脚本可复跑生成 expected
- 更新 VALIDATION_MATRIX 为 golden←reference_implementation

【五 Agent 顺序（每岗 DoD 未过不得进下一岗）】
Agent1 调研：网上公式口径 + 现网 fixture/矩阵盘点 → docs/research/g-trust-reference-golden-research.md；禁止改产品代码
Agent2 计划：Wave 锁表（10 不漏）+ 每命令脚本/字段/容差 + §7 → docs/research/goal-g-trust-reference-golden-wave-plan.md；禁止改产品代码
Agent3 执行：Wave-0…4 脚本→expected→测试→必要时修 domain
Agent4 测试：verify_g_trust_golden_gate（或等价）PASS；列出我需本机编译的 C++ target
Agent5 收尾：§7+§8 + 代码 review（bugbot 或 inherit 子 Agent）+ commit/push + 提示我 package_dist/编测试；禁止收尾塞新功能

【完成标准】
- 10/10 ref-golden 自动化 PASS
- 矩阵诚实；无 vendor 对齐虚假宣称
- Python gate PASS；Agent5 review 通过
- git commit + push

【每阶段输出格式】
变更文件列表 + DoD 勾选 + 风险一行 + go/no-go

现在开始：先确认已读手册，CreateGoal，然后从 Agent1 调研开工。不要重问 §0。
```

---

## §11 修订记录

| 日期 | 说明 |
|------|------|
| 2026-09-05 | 初稿：用户拍板 Q1B、Q2A、Q3A、Q4 五 Agent+收尾 code review、Q5 无顺带、Q6 手册默认容差、Q7 Python verify/可选 gtest/不强跑 cmake；commit+push 按项目惯例锁定 |
