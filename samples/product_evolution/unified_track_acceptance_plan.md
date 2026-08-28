# 产品演进 Track 统一验收计划（中间不停 · 末尾一次测）

> 策略：**G3–G8（及后续 Track）连续竖切交付，不在每个 Track 结束后强制停跑 Qt Creator。**  
> **统一验收门**：你准备好后，**只跑一轮** CMake + Build + 测试 + 手工清单。  
> 研究总览：`docs/research/product-evolution-market-ux-architecture-research.md` §7

日期：2026-08-22

---

## 1. 两阶段纪律

| 阶段 | 谁做 | 何时 | 通过条件 |
|------|------|------|----------|
| **A. Track 交付**（连续） | Agent / 开发 | 每个 Track 结束时 | research + 竖切代码 + help + 文档 + **脚本预检 OK**；**不要求**立即 Qt Creator 手工测 |
| **B. 统一验收**（一次） | 你（Qt Creator） | 多个 Track 交付后 **或** 发版前 | 本文 §3–§5 **全部勾选** |

**G1+G2** 已在 2026-08-22 **单独签收**（[`g1_g2_manual_acceptance.md`](g1_g2_manual_acceptance.md)）；后续 Track 并入下表，**统一测时一并回归 G1/G2**。

---

## 2. Track 签收表（交付 vs 统一测）

| Track | 交付主题 | 脚本/文档预检 | Track 交付 | 统一验收（§3–5） |
|-------|----------|---------------|------------|------------------|
| **G1** | 公式注册表 UI | ✅ | ✅ 2026-08-22 | ✅ 已签（可回归） |
| **G2** | 图表/表格复制 | ✅ | ✅ 2026-08-22 | ✅ 已签（可回归） |
| **G3** | Graph 受控 Builder | ⏳ 计划已备 | ⏳ | ⏳ |
| **G4** | 4-plot / Report Card | ⏳ | ⏳ | ⏳ |
| **G5** | AnalysisService 拆分 | ⏳ | ⏳ | ⏳ |
| **G6** | 命令 Wizard | ✅ | ✅ | ⏳ |
| **G7** | 离线监视摘要 | ⏳ | ⏳ | ⏳ |
| **G8** | Worksheet 编辑 | ⏳ | ⏳ | ⏳ |
| **G9** | 公式代入 / Show Your Work（框架） | ✅ | ✅ 2026-08-23 | ⏳ |
| **G9-D** | 验算轨迹深化（分步求值 · 能做尽做） | ✅ 2026-08-24 | ✅ 脚本预检 | ⏳ Qt Creator 手测 |

> G9：`python tools/verify_g9_formula_substitution_track.py`；覆盖矩阵 [`g9-formula-substitution-coverage-matrix.md`](../../docs/research/g9-formula-substitution-coverage-matrix.md)；DoD [`goal-wave-2026-08-23-g9-formula-substitution.md`](../../docs/research/goal-wave-2026-08-23-g9-formula-substitution.md)。  
> **G9-D（2026-08-24）：** 调研 [`g9-show-your-work-deepen-research-2026-08-24.md`](../../docs/research/g9-show-your-work-deepen-research-2026-08-24.md) · 深度矩阵 [`g9-show-your-work-depth-matrix.md`](../../docs/research/g9-show-your-work-depth-matrix.md) · 预检 `python tools/verify_g9_show_your_work_deepen_track.py` · QtTest `g9_show_your_work_deepen_track_test`。消灭「主公式」stub；Facts 真值 + Excel 风格分步求值；页3 独立步骤表。  

> G6 竖切（Tester）：脚本预检 ✅；Track 交付 ✅（引擎 + Wizard + 入口 + i18n + QtTest t01–t15）；统一验收 ⏳。python tools/verify_g6_command_wizard_track.py PASS。  
> DoD：[`docs/research/goal-wave-2026-08-23-g6-command-wizard.md`](../../docs/research/goal-wave-2026-08-23-g6-command-wizard.md)  
> **G3 下一 Goal（计划已备，待执行）：** [`goal-wave-2026-08-23-g3-graph-builder-plan-and-mega-prompt.md`](../../docs/research/goal-wave-2026-08-23-g3-graph-builder-plan-and-mega-prompt.md) · 调研 [`g3-graph-builder-ux-research-2026-08-23.md`](../../docs/research/g3-graph-builder-ux-research-2026-08-23.md) · DoD [`goal-wave-2026-08-23-g3-graph-builder.md`](../../docs/research/goal-wave-2026-08-23-g3-graph-builder.md)

### 算法竖切批次（2026-08-22）

| 项 | 交付主题 | 脚本/文档预检 | Track 交付 | 统一验收（§3–5） |
|----|----------|---------------|------------|------------------|
| **A1** | `best_subsets_regression` | ✅ | ✅ 2026-08-22 | ⏳ |
| **A2** | `logistic_regression` 深化 | ✅ | ✅ 2026-08-22 | ⏳ |
| **A3** | `batch_capability` | ✅ | ✅ 2026-08-22 | ⏳ |

### 算法 Wave-2（2026-08-22）

| 项 | 交付主题 | 脚本/文档预检 | Track 交付 | 统一验收（§3–5） |
|----|----------|---------------|------------|------------------|
| **W2-1** | `nominal_logistic` | ✅ | ✅ 2026-08-22 | ⏳ |
| **W2-2** | `nonparametric_capability` | ✅ | ✅ 2026-08-22 | ⏳ |
| **W2-3** | `stepwise_regression` AICc/BIC 深化 | ✅ | ✅ 2026-08-22 | ⏳ |
| **W2-4** | `accelerated_life` ALT 窄化 | ✅ | ✅ 2026-08-22 | ⏳ |

> 脚本：`python tools/verify_algorithm_wave2_track.py`

### 算法 Wave-2.5 + Wave-3（2026-08-22）

| 项 | 交付主题 | 脚本/文档预检 | Track 交付 | 统一验收（§3–5） |
|----|----------|---------------|------------|------------------|
| **Wave-2.5** | nominal IRLS；ALT Newton MLE；测试补强 | ✅ | ✅ 2026-08-22 | ⏳ |
| **W3-1** | `bootstrap_two_sample` | ✅ | ✅ 2026-08-22 | ⏳ |
| **W3-2** | KM/Log-rank K 组深化 | ✅ | ✅ 2026-08-22 | ⏳ |
| **W3-3** | `accelerated_life` 使用应力预测 + 图 | ✅ | ✅ 2026-08-22 | ⏳ |
| **W3-4** | `probit_reliability` | ✅ | ✅ 2026-08-22 | ⏳ |

> 脚本：`python tools/verify_algorithm_wave3_track.py`

### 算法 Wave-4（2026-08-22）

| 项 | 交付主题 | 脚本/文档预检 | Track 交付 | 统一验收（§3–5） |
|----|----------|---------------|------------|------------------|
| **W4-0** | 公式注册表 Wave-3 回填 + std::min 扫描 | ✅ | ✅ 2026-08-22 | ⏳ |
| **W4-1** | `nonparametric_capability` 深化（直方图 + PPM） | ✅ | ✅ 2026-08-22 | ⏳ |
| **W4-2** | `reliability` CIF 曲线 + Gray 检验窄化 | ✅ | ✅ 2026-08-22 | ⏳ |
| **W4-3** | `cox_regression` 固定协变量 PH | ✅ | ✅ 2026-08-22 | ⏳ |
| **W4-4** | `logistic_regression` 逐步 AIC/BIC | ✅ | ✅ 2026-08-22 | ⏳ |

> 脚本：`python tools/verify_algorithm_wave4_track.py`

### 算法 Wave-5（2026-08-23 · ✅）

| 项 | 交付主题 | 脚本/文档预检 | Track 交付 | 统一验收（§3–5） |
|----|----------|---------------|------------|------------------|
| **W5-1** | `random_forest` 窄化 | ✅ | ✅ | ⏳ |
| **W5-2** | `weibayes` 窄化 | ✅ | ✅ | ⏳ |
| **W5-3** | `taguchi_orthogonal_design` | ✅ | ✅ | ⏳ |
| **W5-4** | `distribution_calculator` | ✅ | ✅ | ⏳ |

> 计划：[`docs/research/goal-wave-2026-08-23-algorithm-wave5-plan-and-mega-prompt.md`](../../docs/research/goal-wave-2026-08-23-algorithm-wave5-plan-and-mega-prompt.md)  
> 调研：[`docs/research/algorithm-wave5-market-formula-research-2026-08-23.md`](../../docs/research/algorithm-wave5-market-formula-research-2026-08-23.md)  
> DoD：[`docs/research/goal-wave-2026-08-23-algorithm-wave5.md`](../../docs/research/goal-wave-2026-08-23-algorithm-wave5.md)  
> 门禁：`python tools/verify_algorithm_wave5_track.py`

### 算法 Wave-6（2026-08-25 · ✅）

| 项 | 交付主题 | 脚本/文档预检 | Track 交付 | 统一验收（§3–5） |
|----|----------|---------------|------------|------------------|
| **W6-1** | `taguchi_analyze` 静态 S/N | ✅ | ✅ | ⏳ |
| **W6-2** | `mixture_design` simplex-lattice | ✅ | ✅ | ⏳ |
| **W6-3** | `nhpp_repairable` Crow-AMSAA | ✅ | ✅ | ⏳ |
| **W6-4** | `reliability_test_plan` 演示型 | ✅ | ✅ | ⏳ |

> 计划：[`docs/research/goal-wave-2026-08-25-algorithm-wave6-plan-and-mega-prompt.md`](../../docs/research/goal-wave-2026-08-25-algorithm-wave6-plan-and-mega-prompt.md)  
> 调研：[`docs/research/algorithm-wave6-market-formula-research-2026-08-25.md`](../../docs/research/algorithm-wave6-market-formula-research-2026-08-25.md)  
> DoD：[`docs/research/goal-wave-2026-08-25-algorithm-wave6.md`](../../docs/research/goal-wave-2026-08-25-algorithm-wave6.md)  
> 门禁：`python tools/verify_algorithm_wave6_track.py`

### 算法 Wave-7（2026-08-28 · ✅）

| 项 | 交付主题 | 脚本/文档预检 | Track 交付 | 统一验收（§3–5） |
|----|----------|---------------|------------|------------------|
| **W7-1** | `mixture_analyze` Scheffé 分析 | ✅ | ✅ | ⏳ |
| **W7-2** | `glm_two_way` Type III GLM | ✅ | ✅ | ⏳ |
| **W7-3** | `analyze_variability` 分散效应 | ✅ | ✅ | ⏳ |
| **W7-4** | `factor_analysis` 主成分提取 | ✅ | ✅ | ⏳ |

> 计划：[`docs/research/goal-wave-2026-08-28-algorithm-wave7-plan-and-mega-prompt.md`](../../docs/research/goal-wave-2026-08-28-algorithm-wave7-plan-and-mega-prompt.md)  
> 调研：[`docs/research/algorithm-wave7-market-formula-research-2026-08-28.md`](../../docs/research/algorithm-wave7-market-formula-research-2026-08-28.md)  
> DoD：[`docs/research/goal-wave-2026-08-28-algorithm-wave7.md`](../../docs/research/goal-wave-2026-08-28-algorithm-wave7.md)  
> 门禁：`python tools/verify_algorithm_wave7_track.py`

### 算法 Wave-8（2026-08-28 · ✅）

| 项 | 交付主题 | 脚本/文档预检 | Track 交付 | 统一验收（§3–5） |
|----|----------|---------------|------------|------------------|
| **W8-1** | `binary_response_doe` Logit IRWLS | ✅ | ✅ | ⏳ |
| **W8-2** | `cluster_variables` 变量聚类 | ✅ | ✅ | ⏳ |
| **W8-3** | `glm_three_factor` Type III GLM | ✅ | ✅ | ⏳ |
| **W8-4** | `life_data_regression` Weibull 回归 | ✅ | ✅ | ⏳ |

> 计划：[`docs/research/goal-wave-2026-08-28-algorithm-wave8-plan-and-mega-prompt.md`](../../docs/research/goal-wave-2026-08-28-algorithm-wave8-plan-and-mega-prompt.md)  
> 调研：[`docs/research/algorithm-wave8-market-formula-research-2026-08-28.md`](../../docs/research/algorithm-wave8-market-formula-research-2026-08-28.md)  
> DoD：[`docs/research/goal-wave-2026-08-28-algorithm-wave8.md`](../../docs/research/goal-wave-2026-08-28-algorithm-wave8.md)  
> 门禁：`python tools/verify_algorithm_wave8_track.py`

### 报告产品 Phase（节选）

### UI 菜单 IA（2026-08-23）

| 项 | 交付主题 | 脚本/文档预检 | Track 交付 | 统一验收（§3–5） |
|----|----------|---------------|------------|------------------|
| **U1** | 声明式 `menu_path` + `menu_group`（137 命令） | ✅ | ✅ 2026-08-23 | ⏳ |
| **U2** | MainWindow 按字段渲染；删除硬编码白名单；深度≤1 | ✅ | ✅ 2026-08-23 | ⏳ |
| **U3** | help/wiring/acceptance + `verify_ui_menu_ia_track.py` + QtTest | ✅ | ✅ 2026-08-23 | ⏳ |

> 脚本：`python tools/verify_ui_menu_ia_track.py`（回归仍要求 wave4 verify PASS）  
> DoD：[`docs/research/goal-wave-2026-08-23-ui-menu-ia-layout.md`](../../docs/research/goal-wave-2026-08-23-ui-menu-ia-layout.md)

---

## 3. 统一验收 — 一次性构建（只做一次）

在 Qt Creator（建议 **Debug**，`build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug`）：

1. **Projects → Run CMake**（若曾 STALE：先 **Clear CMake Configuration**）  
   - Configure Output 应含各 Track 注册行（如 `Track G1+G2: 5 test targets registered`；后续 Track 会有对应 marker）
2. **Build → Build All**（或 Rebuild）
3. 确认 **DataLab.exe** 生成

诊断：

```powershell
python tools/check_g1g2_build_ready.py
# 后续 Track 会有对应 check 脚本；统一门前应全部为 OK
```

---

## 4. 统一验收 — 脚本预检（一轮）

```powershell
python tools/print_acceptance_status.py          # 期望 13/13 +
python tools/verify_g1_g2_track.py               # G1+G2（已交付 Track）
python tools/verify_algorithm_batch_2026_08.py   # 算法批 A1–A3（2026-08-22）
python tools/verify_algorithm_wave2_track.py       # 算法 Wave-2 W2-1–W2-4（2026-08-22）
python tools/verify_algorithm_wave3_track.py       # 算法 Wave-2.5 + Wave-3（2026-08-22）
python tools/verify_algorithm_wave4_track.py       # 算法 Wave-4 W4-0–W4-4（2026-08-22）
python tools/verify_algorithm_wave5_track.py       # 算法 Wave-5 W5-1–W5-4（2026-08-23）
python tools/verify_algorithm_wave6_track.py       # 算法 Wave-6 W6-1–W6-4（2026-08-25）
python tools/verify_algorithm_wave7_track.py       # 算法 Wave-7 W7-1–W7-4（2026-08-28）
python tools/verify_algorithm_wave8_track.py       # 算法 Wave-8 W8-1–W8-4（2026-08-28）
python tools/verify_ui_menu_ia_track.py            # UI 菜单 IA U1–U3（2026-08-23）
# 后续：python tools/verify_g6_command_wizard_track.py  # G6 命令 Wizard（Tester）
# 后续：python tools/verify_g3_track.py 等（随 Track 增加）
powershell -File tools/run_g1g2_tests.ps1        # 期望 5/5 PASS
```

Phase 3 / 算法大回归（与产品 Track **同一次** Qt Creator 会话内跑完，中间不停）：

```powershell
python tools/print_qt_creator_signoff_batches.py
# 按 runbook 跑 §3.1 / deepen / S1–S7（见 qt-creator-dual-line-acceptance-runbook.md）
```

---

## 5. 统一验收 — 手工清单（一轮勾选）

### 5.1 已交付 Track 回归

- [ ] **G1+G2**：按 [`g1_g2_manual_acceptance.md`](g1_g2_manual_acceptance.md) 快速回归（公式注册表 + 复制 PNG/TSV + Ctrl+C）

### 5.2 待交付 Track（交付后追加本节条目）

| Track | 手工要点 | 详细清单文件 |
|-------|----------|--------------|
| **UI Menu IA** | 统计/控制图/质量工具/图形下均为一级子菜单；Cox→可靠性；逐步回归→回归；无超长扁平「统计」叶列表 | [goal-wave-2026-08-23-ui-menu-ia-layout.md](../../docs/research/goal-wave-2026-08-23-ui-menu-ia-layout.md) 人手门 |
| G3 | Graph Builder 独立页；分面+geom | *待 G3 交付时创建 `g3_*_manual_acceptance.md`* |
| G4 | 4-plot / 假设面板 | *待创建* |
| G5 | 拆分后命令仍可用 | *待创建* |
| G6 | Wizard 推荐命令 | [goal-wave-2026-08-23-g6-command-wizard.md](../../docs/research/goal-wave-2026-08-23-g6-command-wizard.md) 人手门 |
| G7 | 监视摘要表 | *待创建* |
| G8 | 清除单元格 + 撤销 | *待创建* |

### 5.3 全局回归（每个统一验收门必勾）

- [ ] 导入 A→B 后旧输出/排除/undo 失效
- [ ] hidden / excluded 语义未破坏
- [ ] 无单页堆叠多层主流程

---

## 6. 统一签署（一次填表）

| 项 | 结果 | 日期 |
|----|------|------|
| Run CMake + Build All | PASS / FAIL | |
| 脚本预检 13/13 + Track 脚本 | PASS / FAIL | |
| G1+G2 回归 | PASS / FAIL | |
| G3–G8（已交付项） | PASS / FAIL | |
| Phase 3 Qt 测试批次 | PASS / FAIL | |
| 验收人 | | |
| **统一验收门** | PASS / FAIL | |

---

## 7. Agent / Goal 约定（中间不停）

每个 **Track /goal** 结束时：

- ✅ 必须：竖切代码、research、help、**脚本预检 PASS**、在本文 §2 表标记 **Track 交付 ✅**
- ❌ **不要**要求用户立即 Qt Creator 手工测才允许下一 Track
- ❌ **不要**因「未统一验收」阻塞下一 Track 开发

用户说 **「统一测」** 或发版前：只执行本文 **§3–§6 一次**。

### 7.1 多 Agent 协作（2026-08-22）

| 角色 | Cursor 实现 | Wave 内时机 |
|------|-------------|-------------|
| Orchestrator | 主 `/goal` 对话 | 锁 Wave、合并、文档、verify |
| Researcher | `Task` explore + WebSearch | Wave 启动 / 每项开工前 |
| Implementer | 主 agent | 连续竖切 3–6 项 |
| Reviewer | `Task` bugbot | 每项或 Wave 末 |
| Verifier | `verify_*_track.py` | Wave 交付门 |

权威说明：[`docs/research/goal-execution-framework.md`](../../docs/research/goal-execution-framework.md) §2–§3。

**禁止：** 子 agent 缩小 Goal；禁止只做 1 算法就 complete；禁止跳过 Primary URL 调研。

---

## 8. 相关文档

| 文档 | 用途 |
|------|------|
| [`goal-execution-framework.md`](../../docs/research/goal-execution-framework.md) | Wave 批量 · 多 Agent · Mega 提示词 §9 |
| [`goal-wave-template.md`](../../docs/research/goal-wave-template.md) | Wave 计划复制模板 |
| [`qt-creator-dual-line-acceptance-runbook.md`](../../docs/research/qt-creator-dual-line-acceptance-runbook.md) | Phase 3 / 算法 Qt 批次 |
| [`g1_g2_manual_acceptance.md`](g1_g2_manual_acceptance.md) | G1+G2 明细（已签） |
