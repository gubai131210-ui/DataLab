# DataLab 算法扩展会话简报

> 给**下一对话**用。算法工作从本文件开始，不要从 `docs/session-handoff.md` 第 5 节续重构。
>
> 新对话第一条消息建议：
> 「读取 `docs/algorithm-session-brief.md`，按其中步骤做本轮算法扩展。中文路径下不要自行 cmake/ctest；改完列出手工验收项。」

---

## 1. 任务

面向汽车质量工程师、对标 Minitab 的桌面工具。本轮：**深化已有缺口 + 增加有杠杆的新算法 + 图表属性页 UX**。完成判据：四项范围都有公式文档、领域结果、服务页、命令接线（新算法）、测试和验收条目；解释只读 Facts；导入 A→B 契约未破坏。

## 2. 步骤（按序，完成一项再下一项）

1. **读真相源**（完成：能复述本轮四项、禁区和接线，且不把 handoff 第 5 节当任务）。
   - 已完成项与延后清单：`docs/quality-algorithms-acceptance.md`
   - 方法状态与导入契约：`docs/research/algorithm-chart-gap-matrix.md`
   - 延后项细节：`docs/research/deferred-capability-agreement.md`
   - 计算口径：`docs/statistical-methodology.md`
   - 词汇：`CONTEXT.md`
   - 架构：`docs/adr/0001-core-architecture.md`、`0003-structured-interpretation-facts.md`、`0004-grouped-analysis-configuration.md`
   - UI：`docs/ui-guidelines.md`
   - 分层与踩坑（只读第 4、6 节）：`docs/session-handoff.md`
2. **研究公式**（完成：`docs/research/` 有本轮主题文档，含 Minitab/NIST 链接与访问日期；`formula_reference ≠ golden`）。加载 **research** skill。对照 Minitab **输出表形**（表名、列、诊断），不填写未导出数值。
3. **计划后竖切实现**（完成：每个切片 domain → Facts → AnalysisService → `analysis_commands` → 解释 → 序列化/测试 → CMake）。加载 **tdd** 与 **cpp-coding** skill。
4. **收尾**（完成：更新本文件「本轮范围」状态、`quality-algorithms-acceptance.md`、`statistical-methodology.md`；列出用户在 Qt Creator 中的手工验收项）。

Skill 路径：`C:\Users\孤白赟悫\.codex\skills\research\SKILL.md`、`tdd\SKILL.md`；`C:\Users\孤白赟悫\.agents\skills\cpp-coding\SKILL.md`。

## 3. 接线框架

统计核心在 C++ `src/domain/statistics/`（零 Qt）。Python 只做 Excel 导入桥。

```
ui → application / infrastructure / reporting → domain
```

新增或深化一个分析：

| 层 | 文件 | 职责 |
|---|---|---|
| 领域 | `src/domain/statistics/*` | 计算 + 诊断码 |
| 配置/事实 | `src/domain/quality_types.h` | 嵌套配置 + `*Facts` |
| 编排 | `src/application/analysis_service.cpp` | 装配 `OutputPage`（表/图/diagnostics/facts） |
| 菜单 | `src/ui/analysis_commands.cpp` | id / 角色 / 输入 / apply / run 单一来源 |
| 解释 | `src/application/interpretation_service.cpp` | 只读 Facts |
| 持久化 | `src/infrastructure/output_serialization.cpp` | JSON round-trip |
| 构建 | `CMakeLists.txt` + `tools/check_layering.ps1` | 新源文件进对应 target |

数据衔接：`column_assembly` complete-case **行主序**（`aligned[i][j]` = 第 i 个观测第 j 列）；`parse_numeric_cell`；保留 `RowId`。导入契约见缺口矩阵第 3 节。`tests/import_state_reset_test.cpp` 覆盖重导入文件 B 后排除行、输出页、undo、行选择失效。

测试目标以 `CMakeLists.txt` 的 `add_datalab_test` / `add_test` 为准（约 26 个，不是 handoff 里的 12）。

## 4. 本轮范围（2026-08-18 已完成）

按优先级。Stretch 未做。

1. **深化 Logistic** ✅
   - 独立「拟合优度」表（HL 卡方/DF/组数/P/状态）；「拟合与残差」含影响点列；`LogisticFacts` 扩展；解释只读 Facts。
   - 公式：`docs/research/logistic-idi-between-within-chart-formulas.md`

2. **个体分布识别** ✅
   - 命令 `distribution_identification`（质量工具）；四族二参数 AD 排序 + 概率图；不改 `capability_method`。
   - 领域：`distribution_identification.*`、`anderson_darling.*`（从 `normality_test` 抽出公共核）。

3. **组间/组内过程能力** ✅
   - 命令 `between_within_capability`；必填子组列；`ProcessCapability::calculate_between_within`；Cp/Cpk 用 σ_BW。
   - 默认「正态过程能力」仍为 `capability_method=normal`。

4. **图表属性页 UX** ✅
   - 预览右侧；仅控制图显示「参考线」Tab；系列色列 `ResizeToContents`；测试 `graph_properties_dialog_test`。

**Stretch（未做）：** Multi-Vari 图；回归 Unusual Observations 表补列。

已有菜单入口在 `src/ui/analysis_commands.cpp`（描述统计到 PCA、控制图、能力、MSA、可靠性、DOE、图形命令）。不要重复注册已有 `chart_type`。

## 5. 硬约束

- 解释层只陈述证据与假设状态，不写过程合格、量具通过、分布已证明、已证明一致。
- 未从 Minitab 导出的结果不得写入 `tests/fixtures/minitab/VALIDATION_MATRIX.md`。公式参考测试标注 `# source: formula_reference`。
- 电脑是中文路径：改完说明让用户在 Qt Creator 自行测试；agent 不在易损坏环境下跑 cmake/ctest。
- 新文件加入对应 CMake target；跨层 include 过 `tools/check_layering.ps1`。
- `align_complete_rows` 输出行主序。应用层新头文件里的 domain 类型写 `domain::X`。
- 源码 UTF-8 无 BOM。

本轮不做（延后项，见 `deferred-capability-agreement.md`）：Weighted Kappa；无界似然 bias-correction 数值对齐；Kalman / TSERIES 对齐；Bonett / Bartlett / Jackson–Mudholkar 解析限；图表注释、拖拽布局、多图拼版；重构阶段 5/6（PlotSpec 合一、CI、i18n），除非挡住本轮接线。

## 6. 可贴给新对话的短提示词

```
读取 docs/algorithm-session-brief.md，按其中步骤做本轮算法扩展。
先 research 公式再编码；用 tdd + cpp-coding。
范围：Logistic 拟合优度表形深化；个体分布识别；组间/组内能力；graph_properties_dialog 预览右侧化。
不要做重构阶段 5/6，不要填假 Minitab golden。
中文路径下不要自行 cmake/ctest，改完列出我要手工验收的项。
```
