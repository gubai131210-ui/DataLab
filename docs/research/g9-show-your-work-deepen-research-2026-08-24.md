# G9 深化：验算轨迹 / 分步求值（Show Your Work Depth）（2026-08-24）

> 研究日期 / 访问日期：2026-08-24（UTC+8）  
> 用途：给 `/goal` **G9-D — 公式代入深化（分步颗粒度 · 能做尽做）** 的调研正文  
> 配套计划：[`goal-wave-2026-08-24-g9-show-your-work-deepen-plan-and-mega-prompt.md`](goal-wave-2026-08-24-g9-show-your-work-deepen-plan-and-mega-prompt.md)  
> 前序（框架已交付、深度不足）：[`formula-substitution-show-your-work-research-2026-08-23.md`](formula-substitution-show-your-work-research-2026-08-23.md) · [`goal-wave-2026-08-23-g9-formula-substitution-plan-and-mega-prompt.md`](goal-wave-2026-08-23-g9-formula-substitution-plan-and-mega-prompt.md)  
> 执行框架：[`goal-execution-framework.md`](goal-execution-framework.md)  
> **不做本 Goal：** G3 Graph Builder、G4 Report Card 全量、G5 大拆、嵌 R/Python、Minitab golden、TreeNet/AutoML、AGPL 并入 Cassini

---

## §0 用户锁定（相对 G9 首轮的纠偏）

| 用户原话 | 产品翻译 |
|----------|----------|
| 公式里**每个变量在计算过程中的值** | bindings 必须是**本次 run 真值**；禁止主路径「见结果表」 |
| **分步颗粒度** | Excel Evaluate 风格：每步 `expression_before` → 代入 → `expression_after` → `value` |
| **深度：能做的尽量都做** | A/B/C 类命令尽量 **深度验算**；D 类保留 `display_summary` 但须有真实 N/规则；禁止再用「xxx 主公式」冒充实质绑定 |
| UI 不堆页 | 沿用四页；第 3 页升级为「分步求值」；禁止单页堆控件 |

### 现状诊断（仓库实测 · 2026-08-24）

| 指标 | 数量 | 含义 |
|------|------|------|
| attach 分支 | ~141 | 覆盖广 |
| `"… 主公式"` stub | ~79 | **假「实质绑定」** |
| 真公式 + Facts 代入 | **3** | 仅 `capability` / `one_sample_t` / `weibayes` |
| `见结果表` 字面 | ~433 | 表刮取失败就糊弄 |
| `ComputationStep` 字段 | 仅 `description` | **不够分步求值** |
| `verify_g9` | 只查接线/矩阵 | **不拒 stub** |

**结论：** 首轮 G9 完成了「有入口、有类型、全命令挂上」；**未完成「可验算」**。本 Goal 专门修深度。

---

## §1 Primary Sources（网上调研 · 2026-08-24）

### 1.1 分步求值 UX（用户要的颗粒度）

| 主题 | URL | 访问 | 采纳到 DataLab |
|------|-----|------|----------------|
| Excel Evaluate Formula | https://support.microsoft.com/en-us/excel/evaluate-a-nested-formula-one-step-at-a-time | 2026-08-24 | **逐步求值对话框**：先显示完整式 → 每次求一个下划线子表达式 → 中间结果替换进去 → 直到得数 |
| Excel 公式审计（同主题补充） | https://support.microsoft.com/en-us/excel/detect-formula-errors-in-excel | 2026-08-24 | Evaluate / Step In / Step Out 叙事；G9 用**静态步骤列表**模拟（不必做交互点击求值引擎） |
| UX 中间结果可视化 | https://ux.stackexchange.com/questions/37496/visualizing-structure-and-intermediate-results-of-a-computation | 2026-08-24 | 默认分层、按需展开；避免主输出页信息过载 → **独立第 3 页** |

**Excel 示例（产品对标形态）：**

```text
=IF(AVERAGE(F2:F5)>50, SUM(G2:G5), 0)
=IF(40>50, SUM(G2:G5), 0)     ← AVERAGE 已求值
=IF(False, SUM(G2:G5), 0)     ← 比较已求值
0                               ← 最终
```

DataLab 对应：

```text
t = (x̄ − μ₀) / (s / √n)
√n = √20 = 4.47214
s/√n = 1.12 / 4.47214 = 0.25044
x̄ − μ₀ = 10.24 − 10 = 0.24
t = 0.24 / 0.25044 = 0.9583
```

### 1.2 工业 SPC「Show Your Work」（审计透明）

| 主题 | URL | 访问 | 采纳 |
|------|-----|------|------|
| Cassini Show Your Work | https://saturnis.io/cassini | 2026-08-24 | 点结果 → **公式 + 全部输入 + 每一步中间计算 + 标准引用**；显示值=解释值契约测试 |
| Cassini GitHub 能力 | https://github.com/saturnis-io/cassini | 2026-08-24 | 学契约与分层；**禁止 AGPL 整站并入** |
| Saturnis 首页 | https://saturnis.io/ | 2026-08-24 | 「每个数都能 show its work」→ 输出页入口已有，深化内容 |

### 1.3 商业统计软件「代入叙事」

| 主题 | URL | 访问 | 采纳 |
|------|-----|------|------|
| JMP Show Prediction Expression | https://www.jmp.com/support/help/en/19.1/jmp/show-prediction-expression.shtml | 2026-08-24 | 系数已写入式子：`−2.696−1.185+0.987(10)=5.99` |
| JMP Prediction Formula | https://www.jmp.com/support/help/en/19.1/jmp/prediction-formula.shtml | 2026-08-24 | 学「表达式层」；不做 JSL / 嵌脚本 |
| Minitab Methods & Formulas（描述统计） | https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/display-descriptive-statistics/methods-and-formulas/methods-and-formulas/ | 2026-08-24 | **符号定义表形**；帮助是静态，**不是**本 run 分步 |

**边界：** Minitab =「公式是什么」；JMP =「系数写进式」；Excel/Cassini =「逐步算出数」。本 Goal 主对标 **Excel + Cassini**。

### 1.4 公式文本来源（复用，勿重做 G1）

| 资产 | 路径 | 用法 |
|------|------|------|
| 静态公式块 | `resources/help/algorithm_help.json` | `plain_formula` / 符号含义默认文案 |
| G1 注册表 | `src/ui/formula_registry_dialog.*` | 出处页跳转；**不合并** |
| Facts | `quality_types.h` 各 `*Facts` | **权威数值来源**（优先于表刮取） |
| 现有 attach | `src/application/computation_trace_attach.cpp` | **重写质量**，禁止再扩 stub 生成器糊弄 |

---

## §2 「深度」操作定义（能做尽做）

### 2.1 深度等级

| 等级 | 名称 | 必须包含 | 适用 |
|------|------|----------|------|
| **L3** | 分步验算 | 真 `plain_formula` + 全符号 bindings（真值）+ **≥2 步** `ComputationStep`（含 expression/value）+ `substituted_text` + 结果与 Facts/表一致 | **所有 A 类**（推断/能力/控制限/回归/MSA/可靠性/DOE 分析等）能做尽做 |
| **L2** | 代入验算 | 真公式 + 全符号 bindings + 一行代入 + 结果一致；steps 可 1 步摘要 | B/C 类（设计生成规则、分布/功效）优先 L3，最低 L2 |
| **L1** | 显示摘要 | N、complete-case、关键显示参数；`evidence_type=display_summary` | **仅 D 类图形** |
| **L0** | 豁免 | 元命令 | **仅** `tests`、`rule_policy`（已登记） |

**禁止：** 用 L0/L1 冒充 A 类；用 `"xxx 主公式"` + n/stat/p 冒充 L2/L3。

### 2.2 分步颗粒度（强制字段）

扩展 `ComputationStep`（相对现状仅 `description`）：

```text
ComputationStep {
  order                 // 1..n
  description           // 人话：求标准误
  expression_before     // s / √n
  expression_after      // 1.12 / 4.47214
  value                 // 0.25044
}
```

UI 第 3 页（「分步求值」）以**步骤表**为主：序 | 说明 | 代入前 | 代入后 | 得数。  
禁止把步骤、变量表、出处堆在同一屏。

### 2.3 数值权威链路（硬）

```
domain 计算 → *Facts / 表单元格（同源）
           → attach 时优先读 Facts（fmt_num）
           → bindings / steps / result_value
           → UI 只读展示
契约测试：|display_result − facts_or_table| ≤ ε
```

**禁止：** 主路径依赖 `table_value` 失败后写「见结果表」。  
允许：表刮取作 **Facts 缺失时的次级回退**，但 A 类 L3 命令若回退到「见结果表」→ **verify FAIL**。

### 2.4 矩阵与全量义务

- 覆盖矩阵增加列：`depth` ∈ {L3, L2, L1, L0}  
- A 类目标：**尽量全部 L3**；个别不可稳定展开的数值法（迭代/Bootstrap/树路径全集）允许登记 **L2 + notes**（须写诚实边界，禁止空壳）  
- B/C：**尽量 L3，最低 L2**  
- D：L1  
- E：L0  

「能做的尽量都做」= 不为了省事把 A 类批量降为 L2；Planner 须列出**允许 L2 的例外清单**（≤15% A 类，且每条有理由）。

---

## §3 UI（防笨懒）

| 页面 | 职责 | 本 Goal 变更 |
|------|------|--------------|
| 输出工具条 | 「公式代入」按钮 | 可加徽标「已验算 N 条」；**不加**大面板 |
| 页1 公式列表 | 本次 traces | 可显示 depth 徽标（L3/L2/L1） |
| 页2 变量取值 | 符号/含义/取值/角色 | 取值列禁止大量「见结果表」 |
| 页3 **分步求值**（原「代入预览」可改名） | 步骤表 + 最终结果 | **新建/强化本页**；主控件=步骤表 |
| 页4 出处 | URL / G1 跳转 | 不变 |

**禁止：** 单页堆变量+步骤+出处；禁止输出主页嵌半屏展开。

---

## §4 明确不做

- G3 / G4 全量 / G5 大拆  
- 交互式「点 Evaluate 按钮逐步高亮」（静态步骤表足够）  
- KaTeX / 嵌 Web 引擎渲染公式  
- 点单元格打开 trace（可作后续；本 Goal 不强制）  
- 向量/矩阵全量打印（摘要维度/前 k）  
- Minitab golden、嵌 R/Python、Cassini AGPL 并入  
- 破坏 A→B / complete-case / `source_row` / hidden≠excluded  

---

## §5 测试与门禁（相对旧 verify 的加严）

| 门 | 要求 |
|----|------|
| `verify_g9_show_your_work_deepen_track.py`（新）或扩展 `verify_g9_*` | A 类禁止 `"主公式"` 字面；A 类 L3 禁止 bindings 含「见结果表」；L3 要求 `steps`≥2 且含 `expression_after`/`value` |
| QtTest | 每族 ≥1 深测；试点命令契约 `|t−facts|≤ε` |
| 回归 | 旧 g9 接线 + wave4/5 + menuIA + g1g2 PASS |
| 人手 | Qt Creator Rebuild；用户自测（中文路径禁止 agent 强跑 cmake） |

---

## §6 与算法 Wave 的关系

本 Goal **不是**算法 Wave-6。  
若同会话误开「再加 4 个新算法」→ **缩小为只做 G9-D**。  
算法深化可在本 Goal 完成后另开。

---

**文档状态：** 2026-08-24 首版；供 G9-D `/goal` 直接引用。
