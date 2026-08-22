# G6 命令 Wizard：选型 UX 调研与推荐规则草案（2026-08-23）

> 研究日期 / 访问日期：2026-08-23（UTC+8）  
> 用途：产品 Track **G6 命令 Wizard** 的权威调研；配套 Goal 计划见  
> [`goal-wave-2026-08-23-g6-command-wizard-plan-and-mega-prompt.md`](goal-wave-2026-08-23-g6-command-wizard-plan-and-mega-prompt.md)  
> 产品队列：[`product-evolution-market-ux-architecture-research.md`](product-evolution-market-ux-architecture-research.md) §2.1 / §7  
> 依赖水位：UI 菜单 IA ✅（[`ui-menu-ia-command-taxonomy-map-2026-08-23.md`](ui-menu-ia-command-taxonomy-map-2026-08-23.md)）

---

## §0 问题陈述

菜单已按 Minitab 风格分组，但新用户仍面对 **137 条命令**：「我有这些列，该点哪个分析？」  
市场解法分两派：

| 派系 | 代表 | 行为 | DataLab 采纳 |
|------|------|------|--------------|
| **决策树引导** | [Minitab Assistant](https://www.minitab.com/en-us/products/minitab/assistant/) | 问答树 → 选工具 → 再跑分析 | 借鉴「先选型」；**不做**全量 Assistant 决策树克隆 |
| **数据驱动一键选型** | [QI Macros Chart/Stat Wizard](https://www.qimacros.com/qi-macros/wizards/) | 选列 → 内置规则 → **直接跑**多种图/检验 | 借鉴「列数/类型 → 候选」；**禁止**一键黑盒跑全套 |
| **角色平台自动分支** | [JMP Fit Y by X](https://www.jmp.com/content/dam/jmp/documents/en/white-papers/minitab-to-jmp.pdf) | 指定 Y/X 后按建模类型自动选 ANOVA/回归/列联/Logistic | 远期可参考；本 Wave **不做**自动执行平台 |

**本 Wave 锁定产品句：**  
**选列 → 看推荐列表（含理由 + 菜单路径）→ 用户点一项 → 打开既有 AnalysisSetupDialog**。  
Wizard **不**调用 `AnalysisService` 跑统计；**不**写 PDF 结论。

---

## §1 Primary Sources（网上调研）

| 主题 | URL | 访问 | 对本产品的采纳 |
|------|-----|------|----------------|
| QI Macros Wizards 总览 | https://www.qimacros.com/qi-macros/wizards/ | 2026-08-23 | 「内置规则降低选型成本」；明确 Decision Tree ≠ Wizard |
| Chart Wizard 规则表（列数×类型） | https://www.qimacros.com/quality-tools/chart-wizard/ | 2026-08-23 | 1 列 / 2 列 / 3–9 / 10+ 启发式；本 Wave 只映射到 **命令 id 候选**，不自动出图 |
| Stat Wizard 检验选型 | https://www.qimacros.com/hypothesis-testing/statistics-wizard-excel/ | 2026-08-23 | 按列数推荐 t / ANOVA / 方差 / χ²；本 Wave 输出推荐列表 |
| Control Chart Wizard | https://www.qimacros.com/control-chart/control-chart-wizard/ | 2026-08-23 | 计量/计数图选型启发；推荐 `imr`/`xbar_r`/`p_chart` 等，**不自动跑** |
| Minitab Assistant | https://www.minitab.com/en-us/products/minitab/assistant/ | 2026-08-23 | 交互决策树 + 简化对话框；本 Wave 用 **短问答（意图）+ 列类型**，不做 MSA/DOE 全树 |
| Minitab Assistant 选图 | https://blog.minitab.com/en/blog/statistics-and-quality-improvement/use-the-minitab-assistant-to-choose-a-graph | 2026-08-23 | 「Help me choose」流程图；UI 可放「为何推荐」折叠说明 |
| JMP Fit Y by X 过渡指南 | https://www.jmp.com/content/dam/jmp/documents/en/white-papers/minitab-to-jmp.pdf | 2026-08-23 | 列建模类型驱动分析分支——登记为 **G6.5/远期**；本 Wave 不自动跑 |
| jamovi UI 设计（高级折叠） | https://dev.jamovi.org/ui/basic-design/ | 2026-08-23 | Wizard 页内「意图 / 高级」折叠；禁止单页堆控件 |

---

## §2 行业模式摘要（可执行原则）

1. **选型与执行分离**  
   Assistant/Wizard 的价值在「选对工具」；执行仍走既有对话框与服务竖切。  
2. **规则可解释**  
   每条推荐附带 1 句理由（catalog / 固定中文源串 + 英文），禁止「AI 说该跑这个」。  
3. **列类型是主特征**  
   DataLab 已有 `domain::ColumnType`：`numeric` / `categorical` / `time` / `unknown`。  
4. **列数是次特征**  
   1 列、2 列、≥3 列改变假设检验/ANOVA/回归候选。  
5. **意图是可选过滤**  
   用户可选：描述 / 比较均值 / 关联 / 控制图 / 能力 / 可靠性 / 图形探索（短列表，非 Minitab 全树）。  
6. **推荐有上限**  
   默认展示 **Top 5～8**；超出用「更多」展开；禁止一次刷 30 条。  
7. **菜单路径同源**  
   推荐项展示 `menu_path > menu_group`（来自 Menu IA 声明式字段），点开即进既有命令。  
8. **诚实边界**  
   unknown 列多 → 降级推荐 + 提示先核对列类型；不得假装「已证明正态」。

---

## §3 DataLab 推荐规则草案（锁定 · 实现可微调数值阈值）

> 实现时落在纯函数（建议 `application` 或 `ui` 旁路无 Qt 的推荐引擎），**单测覆盖**；规则表可 JSON/静态表。

### 3.1 输入

| 输入 | 来源 |
|------|------|
| 选中列索引 | Worksheet 选择 / Wizard 列清单勾选 |
| 每列 `ColumnType` | `DataTable.column_types` |
| 可选意图 `intent` | Wizard 单选：`describe` / `compare` / `associate` / `control_chart` / `capability` / `reliability` / `graph` / `any` |
| 命令目录 | `analysis_commands::all()` 的 id + menu_path + menu_group + menu_label |

### 3.2 输出

```text
Recommendation {
  command_id,
  score,           // 排序用，不展示给用户亦可
  reason_zh,       // 固定文案，可走 ui_tr / catalog
  menu_path_display  // "统计 > 基础统计"
}
```

### 3.3 启发式（示例 · 须写成可测表）

| 条件 | 优先推荐 command_id（示例） | 理由要点 |
|------|------------------------------|----------|
| 1× numeric，intent=any/describe | `descriptive`, `histogram`, `normality_test`, `boxplot` | 单变量描述与分布 |
| 1× numeric，intent=control_chart | `imr` | 个体计量图 |
| 1× numeric，intent=capability | `capability`, `nonnormal_capability` | 过程能力（须规格在对话框再填） |
| 2× numeric，intent=compare | `two_sample_t`, `mann_whitney`, `variance_test` | 两组比较 |
| 2× numeric，intent=associate | `correlation`, `regression`, `scatter_plot` | 相关/回归/散点 |
| 1× numeric + 1× categorical | `one_way_anova`, `boxplot`, `kruskal_wallis` | 因子分组 |
| ≥3× numeric，intent=associate | `regression`, `pca`, `correlation` | 多变量 |
| 1× categorical 计数感（实现可先粗判） | `pareto`, `chi_square` | 属性/帕累托 |
| time + event 列名启发或用户标为可靠性意图 | `reliability`, `cox_regression` | 生存/可靠性（弱启发，须标注「请核对删失编码」） |
| intent=graph | `histogram`/`scatter_plot`/`boxplot`/`time_series_plot` 按列型 | 探索图 |

**禁止：** Wizard 根据「看起来正态」自动勾选能力合格路径；能力推荐只打开对话框。

### 3.4 与 Menu IA 的衔接

推荐项旁显示 taxonomy 路径；「在菜单中定位」可选（高亮菜单或仅文案）。  
命令 id 必须存在于 `analysis_commands::find`；孤儿 id **FAIL** 测试。

---

## §4 明确不做（本 Wave）

| 项 | 原因 |
|----|------|
| 一键跑完 QI Macros 式「所有可能图」 | 破坏可审计竖切；用户不知算了什么 |
| Minitab Assistant 全决策树 + Report Card 全量 | 属 G4；本 Wave 只选型 |
| JMP Fit Y by X 自动执行 | 远期 |
| LLM / 自然语言问数 | 与 formula_reference / catalog 诚实策略冲突 |
| Graph Builder（G3） | 独占 Goal |
| 改 domain 统计公式 | 产品 UI Track |
| Weibayes / TreeNet | deferred |

---

## §5 验收要点（人手 + 脚本）

| 门 | 内容 |
|----|------|
| 脚本 | `verify_g6_command_wizard_track.py`：推荐引擎纯函数测例、UI 文件存在、CMake 注册、不调用 AnalysisService |
| QtTest | 列型组合 → 期望 id 集合；Top-N 上限；未知 id 为 0 |
| 人手 | 选 1 列数值 → 见描述/正态/直方图；点推荐 → 打开既有设置对话框（不直接出结果页） |

---

**文档状态：** 2026-08-23 首版；供 G6 Goal 直接引用。
