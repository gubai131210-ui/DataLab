# Goal：G-Chart+Teach — Track C 图形窄化 + Track E 岗位教学路径

> **用途**：新开一场 Goal 对话时的**唯一权威操作手册**（本轮）。  
> **状态**：§0 为编排者**推荐锁定**（2026-09-05）；用户若改口须先改本表再开工。  
> **前序已完成**：G-Trust（`2118ad2`）— 10 命令 `golden`←`reference_implementation`；**本 Goal 不重做 golden / 不碰 vendor_oracle**。  
> **方向来源**：[`datalab-next-direction-research-2026-09-05.md`](datalab-next-direction-research-2026-09-05.md) Track C / Track E  
> **母框架**：[`goal-execution-framework.md`](goal-execution-framework.md)  
> **图形现网盘点**：[`graph-chart-interaction-audit-2026-09.md`](graph-chart-interaction-audit-2026-09.md)（行联动已收口；注释/自由拼版仍弱）  
> **图属性现网**：`GraphPropertiesDialog` 已有 CL/LCL/UCL **样式** Tab；`ChartModel` **无**自由文本注释、**无**用户自定义水平/垂直参考线、**无**双图拼版模型  
> **教学现网**：184 课口吻/深度已收口；树按命令分类；**无**岗位路径 /「跟做报告」端到端编排

---

## §0 推荐锁定决策（2026-09-05）— 用户确认前视为默认

| # | 问题 | **推荐决定（默认锁定）** |
|---|------|--------------------------|
| Q1 | Goal 形态 | **A：一场 Goal 串两条 Track**：先 Track C（图形窄化），后 Track E（岗位路径）。内部 Wave 不得缩成「只做注释」或「只写三篇 md」 |
| Q2 | Track C 范围 | **窄化三件套**：(1) 自由文本注释（锚点 + 文案，可进屏与 PDF）；(2) 用户自定义水平/垂直参考线（数值 + 标签 + 样式）；(3) **受控双图拼版导出**（一次选两张已产出图 → 左右或上下进同一 PDF 页）。**禁止** Graph Builder 拖拽全量、自由像素仪表盘、可旋转 3D |
| Q3 | Track C 试点图种 | **Control（含 I-MR 子图）+ Scatter + Histogram** 三图种必须竖切打通注释/参考线；拼版不限图种但须能选两张 `ChartModel`。其它图种：能共用 `ChartModel` 字段则跟进，不能则登记 backlog，禁止 silently 假装支持 |
| Q4 | Track E 范围 | **三条岗位路径**（进料检验 / 制程监控 / PPAP 立项）+ 每路径 **「跟做报告」清单**（有序 `command_id` + 期望产出物：导入→分析→导出 PDF 检查点）。复用现有 184 课与专用数据集；**禁止**重写全库文案、禁止升 catalog v3、禁止新开偏门算法 |
| Q5 | 五 Agent | **Agent1 调研 → Agent2 计划 → Agent3 执行 → Agent4 测试 → Agent5 收尾（必须含代码 review）**。全程 **`model: "inherit"`**，禁止建议换模型 |
| Q6 | 顺带范围 | **无**。不做 G-Workflow（`.dlab` 记忆）、G-MES-Lite、LicenseAdmin moc、Assistant、Word 全兼容、真·Minitab vendor 对齐 |
| Q7 | 编译 / Git / 文档 | Agent 跑 **Python verify**；若环境已有相关 gtest 可跑；**中文路径不强跑易失败 cmake/package**。Goal 结束 **必须 git commit + push**。文档路径即本文件。用户本机 Qt / `package_dist` 自测 |

**编排者纪律**：子 Agent **禁止**再问「要不要 Graph Builder / 要不要重写 184 课 / 是否换模型」；冲突以本表为准。若用户在开聊前改口，先改 §0 再 CreateGoal。

### §0.1 现网硬约束

| ID | 约束 |
|----|------|
| H1 | **必须衔接** `ChartModel` / `AnalysisChartWidget` / `GraphPropertiesDialog` / `pdf_report_writer`；禁止平行再建第二套图表模型 |
| H2 | 注释与自定义参考线进 `ChartModel`（或明确挂载的子结构），序列化进现有 output / PDF 路径；禁止「只 UI 画、导出丢」 |
| H3 | 拼版 = **受控两图合成 PDF 页**，不是自由像素仪表盘；文案禁止写成「Graph Builder」 |
| H4 | 行联动主路径（[`graph-chart-interaction-audit-2026-09.md`](graph-chart-interaction-audit-2026-09.md)）**不得回退** |
| H5 | Track E 路径数据进学习中心管线（sqlite / overlay / 生成脚本），UI 有独立入口或清晰分区；**禁止**只写 md 无产品面 |
| H6 | 路径步骤只引用已实现 `command_id`；缺课登记 backlog，禁止伪造「已实现」 |
| H7 | 口吻门：`tools/verify_learning_center_copy_depth.py` 等既有门 **不得误伤**；新增路径文案须过同等或专用 gate |
| H8 | 学习中心既有 184 课深度/口吻 **本 Goal 不重写**；只加路径层与跟做清单 |
| H9 | domain 算法层 **原则上不改**；若注释/参考线仅 reporting/ui，禁止顺手改 statistics |
| H10 | backlog / wiring：Track C 更新「图属性注释/拼版」行；Track E 登记路径 id；禁止伪造成 vendor/算法 ✅ |

### §0.2 Track C 交付锁表（不得漏）

| # | 能力 | 验收抓手 |
|---|------|----------|
| C1 | 自由文本注释 | 属性对话框可增删改；图上可见；导出 PDF 仍可见；gtest 或等价断言模型往返 |
| C2 | 自定义水平参考线 | 数值 + 标签 + 可见/色/线型；至少试点三图种绘制 |
| C3 | 自定义垂直参考线 | 同上（适用 X 轴数值/类目约定写进 plan） |
| C4 | 受控双图拼版 PDF | UI：从输出区选两图 → 左右/上下 → 一页 PDF；至少 1 条自动化或可脚本的结构断言 + 用户本机目视 |
| C5 | 诚实边界 | UI/帮助写清「窄化编辑 / 非 Graph Builder」；审计 md 更新 |

### §0.3 Track E 交付锁表（不得漏）

| # | path_id | 产品名（可微调） | 最少步骤（command_id 有序） | 跟做报告检查点 |
|---|---------|------------------|------------------------------|----------------|
| E1 | `path_incoming_iqc` | 进料检验 | ≥5：如 `gage_rr`（或适用 MSA）→ `normality_test` → `capability` → 相关图/表 → 导出 PDF | 每步：导入专用集 → 对话框要点 → 看什么 → PDF 是否含关键数字/图 |
| E2 | `path_process_spc` | 制程监控 | ≥5：如 `imr` 或 `xbar_r` → 失控解读 → `capability` →（可选）`p_chart` → 导出 PDF | 同上 |
| E3 | `path_ppap_capability` | PPAP 立项 | ≥5：正态/稳定性 → `capability` / `capability_sixpack` →（可选）`between_within_capability` → 导出 PDF | 同上；文案诚实：教学路径 ≠ AIAG 正式 PPAP 包 |

**计数**：C 表 5 + E 表 3 路径。Agent2 可微调步骤 id，**不得删路径或删 C1–C4**。

---

## §1 本 Goal 要交付什么（产品语言）

**Track C**：工程师能在常用图上加**注释与参考线**，并把**两张图拼进一页 PDF**交给审核——不必等 Graph Builder。

**Track E**：新人按**岗位路径**跟做「进料 / 制程 / PPAP」最短闭环，每步连到现有课与数据集，最后能导出一份可读 PDF 检查清单——不是再堆 184 篇散文。

**非目标**

- Graph Builder 拖拽全量、自由仪表盘、3D  
- `.dlab` 工程记忆、MES 映射、LicenseAdmin、Assistant、Word  
- 重写学习中心全库 / 升 v3 / 新算法  
- Minitab `vendor_oracle`  
- 中途换模型  

---

## §2 必须衔接的现有代码（禁止另起炉灶）

| 路径 | 本 Goal 允许 |
|------|----------------|
| `src/reporting/chart_model.h` (+ 序列化) | 扩展注释 / 自定义参考线字段；`schema_version` 递增并兼容读旧 |
| `src/ui/analysis_chart_widget.*` | 绘制注释与自定义参考线 |
| `src/ui/graph_properties_dialog.*` | **新建或拆页**：注释页、自定义参考线页（禁止把控件再堆回已挤的轴页） |
| `src/infrastructure/pdf_report_writer.*` / `report_export_service.*` | 注释进 PDF；双图拼版页 |
| `src/ui/output_workspace.*` / `mainwindow` 导出路径 | 拼版入口（独立小对话框优先，禁止塞进主窗一屏堆控件） |
| `tests/chart_*` / `graph_properties_dialog_test.cpp` | 扩展断言 |
| `src/ui/learning_center_page.*` / `learning_*` / `tools/learning_data/**` | 路径导航 + 跟做清单；重建 sqlite |
| `tools/verify_learning_center_*.py` | 扩展或新增 `verify_teach_path_gate.py` |
| `docs/research/graph-chart-interaction-audit-2026-09.md` | 追加 C 能力状态行 |
| `docs/research/minitab-market-algorithm-backlog.md` | 图属性注释/拼版行更新为窄化 ✅ 或 🟡 |

**禁止新建**：第二套 Chart 引擎、第二套学习中心 DB、并行 PDF 管线。

架构保持：`ui → application → domain`；图表绘制/导出属 reporting/ui；domain 统计不因本 Goal 大改。

---

## §3 五 Agent 流水线（模型锁定 inherit）

```
Agent1 调研 → Agent2 计划 → Agent3 执行 → Agent4 测试 → Agent5 收尾（含 code review）
```

### Agent1 — 调研

产出：`docs/research/g-chart-edit-and-teach-path-research.md`

1. 网上：图表注释/参考线/双图报告惯例（Minitab Graph Editing、JMP 标注、常见审核 PDF 拼图）Primary URL ≥6。  
2. 网上：进料 / 制程 SPC / PPAP 能力研究**最短教学路径**（AIAG/NIST 公开材料；诚实边界）。  
3. 现网：`ChartModel`、属性对话框、PDF writer、学习中心 schema/生成器。  
4. **禁止**改产品代码。

### Agent2 — 计划

产出：`docs/research/goal-g-chart-edit-and-teach-path-wave-plan.md`

必须含：H1–H10、C/E 锁表、字段设计、UI 分页面（禁止一页堆满）、Wave 出口、§7 禁止偷懒全文。

**建议 Wave**

| Wave | 内容 | 出口 |
|------|------|------|
| Wave-0 | 模型字段约定、`schema_version`、审计/backlog 诚实化、verify 骨架 | 约定可执行 |
| Wave-1 | C1+C2+C3：注释 + 水平/垂直参考线；试点三图种绘制 + 属性新页 | 模型往返测试 PASS；绘制不回退行联动 |
| Wave-2 | C4：双图拼版 PDF + 入口对话框（独立页） | 结构断言 + 文档步骤给用户目视 |
| Wave-3 | E1–E3：三路径数据 + 学习中心 UI 入口 + 跟做清单 | sqlite 重建；路径可点开 |
| Wave-4 | gate 全集、审计 md、backlog、口吻/路径 verify | Agent4+5 |

### Agent3 — 执行

按 Wave-0…4 竖切；UI **该新建对话框/Tab 就新建**，禁止把注释+拼版+路径全塞进一个页面。

### Agent4 — 测试

- `tools/verify_g_chart_teach_gate.py`（或拆两个 gate）PASS  
- 列出用户需本机编译的 C++ target  
- 能跑则跑相关 gtest；不强跑全量 cmake/package  

### Agent5 — 收尾

§7+§8 勾选 + **代码 review**（bugbot 或 inherit 子 Agent）+ commit/push + 提示用户本机编测与 `package_dist`；禁止收尾塞新功能。

---

## §4 内容规格（验收口径）

### 4.1 注释

- 字段建议：`id`、`text`、`x`/`y`（数据坐标或归一化约定须在 plan 钉死）、可选 `anchor`（数据点 index）  
- 上限：单图合理上限（如 ≤20），超限明确报错  
- PDF：与屏上同文案，允许字号略缩，禁止丢注释  

### 4.2 自定义参考线

- 与控制限 `center/lower/upper` **并存不互相覆盖语义**；自定义线单独容器  
- 水平：Y 值；垂直：X 值（时间图/控制图用序号约定写清）  

### 4.3 双图拼版

- 布局：左右 | 上下 二选一（本 Goal 不做 2×2）  
- 来源：当前输出工作区已有图；禁止要求用户从磁盘选任意 PNG 拼贴冒充产品能力  

### 4.4 岗位路径

- 每路径：标题、受众一句话、有序步骤（`command_id` + 课内跳转 + dataset 提示 + 报告检查点）  
- 文案口吻：延续「并肩版」；PPAP 路径必须声明**非正式提交包**  

---

## §5 禁止偷懒（Plan 必须粘贴）

1. **禁止**只做属性对话框 UI 壳、PDF/模型不接线。  
2. **禁止**把注释/参考线/拼版/路径控件堆在同一页；该新建 Tab/对话框就新建。  
3. **禁止** Graph Builder / 自由仪表盘 / 3D 范围偷渡。  
4. **禁止**拼版用「截屏两张 PNG 手动说明」代替产品内合成。  
5. **禁止** Track E 只写三篇 markdown 无学习中心入口。  
6. **禁止**重写 184 课正文「顺便优化口吻」。  
7. **禁止**路径引用未实现命令却标已完成。  
8. **禁止**回退行联动 / 弄坏现有 CL 样式 Tab。  
9. **禁止** QSKIP / 空测冒充 C1–C4 或三路径完成。  
10. **禁止** Agent5 不做 code review。  
11. **禁止**中文路径强跑易挂 cmake/package（除非用户本轮要求）。  
12. **禁止**收尾不 commit/push。  
13. **禁止**声称「已与 Minitab Graph Builder / AIAG PPAP 官方包对齐」。  
14. **禁止**中途换模型或 `model` 非 `inherit`。  
15. **禁止** Wave-1 过了就宣告 Goal 完成（C+E 锁表必须收口）。  

---

## §6 完成定义

- [ ] C1–C4 自动化或门禁 PASS；C5 文档诚实  
- [ ] E1–E3 三路径可在学习中心打开；跟做清单完整；gate PASS  
- [ ] 行联动无回退；既有学习中心口吻门无误伤  
- [ ] Agent5 code review 通过  
- [ ] git commit + push  
- [ ] 用户拿到本机编测 / 目视 PDF / `package_dist` 提示  

---

## §7 每阶段输出格式

```text
变更文件列表
DoD 勾选
风险一行
go / no-go
```

---

## §8 新对话开场粘贴（Mega Prompt）

> 把下面整段贴进**新 Goal 对话**首条。建议挂上 `/goal` skill。若你要改 §0，先改手册再开聊。

```text
/goal

【Goal 启动】G-Chart+Teach — Track C 图形窄化 + Track E 岗位教学路径

权威手册（唯一决策源，先通读再动手）：
docs/research/goal-g-chart-edit-and-teach-path-plan-and-mega-prompt.md

请立即用 CreateGoal 建立长跑目标，objective 写清本 Goal 名称与手册路径。
母框架：docs/research/goal-execution-framework.md
方向研究：docs/research/datalab-next-direction-research-2026-09-05.md
图形审计：docs/research/graph-chart-interaction-audit-2026-09.md
前序：G-Trust 已完成（ref-golden）；本 Goal 不重做 golden、不做 vendor_oracle。

【模型锁定 — 最重要】
- 全程只使用我当前对话所用模型。
- 所有 Task 子 Agent 必须 model: "inherit"。
- 中途禁止换模型，禁止建议换模型。

【§0 已拍板 — 禁止重问、禁止改口】
Q1 一场 Goal：先 Track C 后 Track E；Wave 不得缩水
Q2 Track C：自由注释 + 自定义水平/垂直参考线 + 受控双图拼版 PDF；禁止 Graph Builder/自由仪表盘/3D
Q3 试点图种：Control + Scatter + Histogram 必须打通注释/参考线
Q4 Track E：三路径 path_incoming_iqc / path_process_spc / path_ppap_capability + 跟做报告清单；复用 184 课；不重写全库、不升 v3
Q5 五 Agent：调研→计划→执行→测试→收尾（收尾必须含 code review）；全程 inherit
Q6 不做：G-Workflow、MES-Lite、LicenseAdmin、Assistant、Word、vendor 对齐
Q7 Python verify + 可选 gtest；中文路径不强跑易失败 cmake/package；结束必须 commit+push

【衔接现网 — 禁止另起炉灶】
- 扩展 ChartModel / AnalysisChartWidget / GraphPropertiesDialog（新 Tab）/ PDF writer
- 拼版用独立小对话框；学习中心加路径入口
- 更新 graph-chart-interaction-audit 与 backlog 诚实行

【五 Agent 顺序】
Agent1 → docs/research/g-chart-edit-and-teach-path-research.md（禁止改产品代码）
Agent2 → docs/research/goal-g-chart-edit-and-teach-path-wave-plan.md（含 §5 禁止偷懒；禁止改产品代码）
Agent3 → Wave-0…4 执行
Agent4 → verify_g_chart_teach_gate（或等价）PASS；列出我需本机编译的 target
Agent5 → review + commit/push + 提示我本机编测/package_dist；禁止塞新功能

【完成标准】
- C1–C4 + E1–E3 收口；门禁 PASS；review 通过；git push

现在开始：先确认已读手册，CreateGoal，然后从 Agent1 调研开工。不要重问 §0。
```

---

## §9 修订记录

| 日期 | 说明 |
|------|------|
| 2026-09-05 | 初稿：G-Trust 完成后衔接 Track C+E；§0 推荐锁定 |
