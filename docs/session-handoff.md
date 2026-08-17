# DataLab 重构会话交接文档

> 本文件记录截至当前会话的完整工作状态，供**新对话**无缝续接。新对话第一条消息建议：
> 「读取 `docs/session-handoff.md` 与 `docs/refactor-plan.md`，继续 DataLab 重构，从**阶段 5 剩余**（PlotSpec/ChartModel 合一 或 InterpretationService 类型化）或**阶段 6 工程化**开始。」

---

## 1. 项目与环境

| 项 | 值 |
|---|---|
| 项目路径 | `D:\QT_CppPrograms\DataLab` |
| 产品 | 模仿 Minitab 的汽车质量分析工具（46 种分析） |
| 技术栈 | Qt 6.11.1 / MinGW 13.1 / C++17 / CMake / SQLite；Python(pandas) 仅用于 Excel 导入桥 |
| 规模 | 约 20,500 行 C++（src+tests），12 个测试目标 |
| 版本控制 | 本地 git 仓库（初始提交 `26ad743` 后 14 个重构提交）；无远端 |

### 构建与测试（每次改动后必须全绿）

```powershell
$env:Path = "D:\QT\6.11.1\mingw_64\bin;D:\QT\Tools\mingw1310_64\bin;$env:Path"
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug -j 8
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug --output-on-failure
powershell -NoProfile -ExecutionPolicy Bypass -File tools/check_layering.ps1
```

验证协议：**构建全绿 + ctest 12/12 + layering 检查通过**，每步一个可回滚提交。

---

## 2. 当前 git 历史（工作区干净）

```
a9d893b fix(ui): 图表框选在缩放/平移下不错位 + 删 thread_local 死代码
6c5789b docs: 阶段 3.4 完成 + 阶段 2.4 重新定界（等待 i18n 决策）
c031836 feat(ui): 行排除与清除排除纳入 undo 栈
bde8a03 refactor(ui): output_workspace 与 report_preview_dialog 共享页面渲染器
1cc6bbd fix(application): align_complete_rows 行主序修正 + 补 2.3 服务层测试
30e14ed refactor(application): capability 家族与 logistic_regression 薄壳化
74e9a55 refactor(application): complete-case 行对齐下沉 column_assembly
57d92c0 refactor(application): doe_factorial 薄壳化（doe_pages 页面装配模块）
47fd2c8 refactor(ui): 阶段 3.1 命令化（AnalysisCatalog 数据驱动化）
d79a1b1 docs: 新增会话交接文档（session-handoff.md）
094b5af refactor(infrastructure): DataImportService 门面与 ProjectRepository 健壮性修复
83729ba refactor(ui): 死代码清理（AnalysisDialog/AnalysisOutputView/menu_path/AnalysisResult/旧 PDF 重载）
c3419cb refactor(application): 控制图族方法薄壳化（chart_pages 共享页面构建器）
13978dc refactor(application): 抽取 AnalysisService 共享构建器与列装配层
b5ee761 refactor(build): 按层拆分 CMake 目标并消除 infrastructure→ui 反向依赖
bce2426 docs: 补三套数值解析并存与 interpretation 覆盖不一致至重构计划
fc00cfb docs: 补 PythonTableImporter 测试覆盖缺口至重构计划
e44552a docs: 补充图表子系统 thread_local 气味至已知缺陷清单
26ad743 chore: initial commit（286 文件，含 samples 的 Minitab 对照数据）
```

---

## 3. 已完成工作（按重构计划阶段）

### 阶段 1：构建系统分层（`b5ee761`）
- 图表渲染四件套（`chart_model`/`chart_renderer`/`chart_adapter`/`chart_coordinate_mapper`）从 `src/ui/` 下沉到 **`src/reporting/`**（纯 QPainter，无 QWidget），12 处 include 消费方改前缀
- **消除 infrastructure→ui 反向依赖**（原 `pdf_report_writer.cpp` include `ui/`）
- mainwindow.cpp/h/ui 从根目录迁入 `src/ui/`；main.cpp 改 `#include "ui/mainwindow.h"`
- CMake 拆 5 个库目标 + DataLab 可执行目标（见第 4 节）；`DATALAB_SOURCE_DIR` 移至 infrastructure
- 分层约束：链接依赖图（反向引用链接失败）+ `tools/check_layering.ps1`（include 前缀白名单）
- 新增 `docs/adr/0002-layer-targets.md`

### 阶段 2 首轮：拆 AnalysisService 管线（`13978dc`）
- 新增 `src/application/output_builder.{h,cpp}`（280 行）：数字格式、错误页、配置解析、通用表格/控制图构建器（14 个函数）
- 新增 `src/application/column_assembly.{h,cpp}`（78 行）：`build_strict_subgroups`、`first_variable`、`SubgroupInput`
- 统一数值解析：pca 裸 `std::stod` → `parse_numeric_cell`；`capability()` 新增 `capability_indices` out 参数，sixpack 不再反解析自己产出的表格
- 删死代码：`to_legacy_result`、`count_column_values`
- `analysis_service.cpp`：4013 → 3625 行

### 阶段 2 二轮：控制图族薄壳化（`c3419cb`）
- 新增 `src/application/chart_pages.{h,cpp}`（190 行）三个共享页面构建器：
  - `subgroup_dual_chart_page`（Xbar-R/Xbar-S：266/262 行 → 26/24 行）
  - `attribute_chart_page`（P/NP/C/U：~55 行 → ~40 行规格）
  - `laney_chart_page`（Laney P'/U'：~90 行 → ~45 行）
- 模式：**规格结构体 + `assemble`/`compute` lambda**；行为逐字保留（含 xbar_s 与 xbar_range 的 parameter_summary 措辞差异、np 常数-列覆盖顺序、p/u 的 min 截断）
- `analysis_service.cpp`：3625 → 3486 行

### 阶段 2.3 大方法薄壳化（`57d92c0`/`74e9a55`/`30e14ed`/`1cc6bbd`）
- **`doe_factorial` 292 → 98 行**：新增 `src/application/doe_pages.{h,cpp}`——`doe_response_page`（系数与效应/DOE ANOVA/模型项与区组/纯误差与失拟/残差诊断五表 + 主效应/交互作用图）与 `doe_design_page`（设计矩阵）；3 处同构 ANOVA 表收敛为 `append_anova_rows`（区组行 "Block: " 前缀保留）
- **complete-case 行对齐下沉** `column_assembly::align_complete_rows`（**行主序**：`aligned[i][j]` = 第 i 个对齐观测第 j 列值），paired_t/regression 共用。⚠️ 初版列主序导致 regression 预测变量结构被拍平（fit 报 invalid_regression_shape）——当时无服务层测试未发现，`1cc6bbd` 补测试后修正
- **`capability` 143 → 65 行**：四表（Process Data/Performance PPM/Potential Within/Overall）+ 直方图下沉 `build_capability_content`；**`capability_sixpack` 138 → 89 行**：概率图（含参考线）/最后 25 点图下沉 `probability_plot_spec`/`last_points_plot`；**`logistic_regression` 125 → 88 行**：complete-case 导入下沉 `logistic_import_rows`（LogisticImport 结构）
- **补 4 条服务层测试**（quality_statistics_test）：`buildsDoeFactorialServiceOutput`（设计/响应两分支）、`buildsRegressionServiceOutput`、`buildsLogisticServiceOutput`、`buildsPairedTServiceOutput`——覆盖此前无兜底的抽取路径。测试数据教训：完全可分数据致 IRLS 秩亏（rank_deficient_design）、4 运行=4 参数致零误差自由度（拟合拒绝），均需规避
- `analysis_service.cpp` 3486 → 3313 行；构建全绿 + ctest 12/12 + layering 通过

### 阶段 3.4 剩余 + 阶段 5 已知 bug（`bde8a03`/`c031836`/`a9d893b`）
- **共享页面渲染器**：新增 `src/ui/page_renderer.{h,cpp}` 统一标题/方法卡/解释/表格/诊断/图渲染（`PageRenderOptions` 参数化：interactive_charts 交互、include_method 方法卡）；`output_workspace.cpp` 301 → 132 行、`report_preview_dialog.cpp` 199 → 52 行；`analysis_icon_resource` 收敛为 `page_renderer::icon_resource`（两处原完全重复）
- **行排除 undo**：新增 `CleaningChangeCommand`（前后 `cleaning_operations_` 快照 + 应用器 lambda，命令不依赖 MainWindow 类型）；`MainWindow::restore_cleaning_operations` 统一重放；排除/清除可 Ctrl+Z/Ctrl+Y
- **bug 修复**：① 框选 `to_index` 原忽略 zoom/pan 线性映射，缩放/平移后选中错误区间——改为复用 `hit_test` 同款 `ChartCoordinateMapper`（数据范围 + zoom + pan）求数据 X 再钳制取整；② 删除无调用方的 `selected_rows()`（`static thread_local` 引用返回）——信号路径本就按值发送；③ **t_power ensure_data 语义确认无问题**：`AnalysisService::t_power` 不使用数据表（未命名参数），`requires_data=false` 正确
- 构建全绿 + ctest 12/12 + layering 通过

### 阶段 3.1 命令化（`47fd2c8`）
- 新增 `src/ui/analysis_commands.{h,cpp}`（1498 行）：**46 项 `AnalysisCommand` 数据驱动表**——id/菜单文字/对话框标题/menu_path/图标/角色与输入规格/`apply`（配置构建）/`run`（`AnalysisService::xxx`）；doe_factorial 与 doe_response 共用 `doe_apply`/`doe_run`；t_power `requires_data=false`（不调 `ensure_data()`）
- **删 MainWindow 的 94 行 `run_analysis` if/else + 45 个 `run_*`（约 1600 行）**，只留通用 `run_from_spec(id)`：ensure_data（按需）→ 建对话框（命令表提供角色/输入/图标）→ apply 填配置（校验失败返回 `AnalysisApplyResult`，error_title 非空弹框、为空静默中止，忠实保留原行为差异）→ publish_page
- 菜单按 `menu_path` 数据化生成（表顺序即菜单顺序，`separator_before` 重现分隔线）；"测量系统分析/试验设计"占位项保留（msa 仍只注册不进菜单——原样保留）
- **图标映射表合并**：命令表 `icon_file` 为单一来源，删 mainwindow 45 项硬编码表 + `analysis_setup_dialog.cpp` 标题规则表（构造函数新增 `icon_resource` 参数）；reliability→report、t_power→one_sample_t、doe_response→doe_factorial 三个图标随合并生效（原为 data-table 回退）
- 删除闲置 `AnalysisCatalog`（application 层，无调用方）；CMake 同步增删源文件
- `mainwindow.cpp` 2502 → 842 行；`mainwindow.h` 107 → 62 行；构建全绿 + ctest 12/12 + layering 通过

### 阶段 3.4 首批：死代码清理（`83729ba`）
- 删 `AnalysisDialog`、`AnalysisOutputView`（无调用方，含 CMake 源项）
- 删 `AnalysisCatalog::menu_path` 死字段（46 处描述符第三项）
- 删 `domain::AnalysisResult` 结构体 + `PdfReportWriter` 单分析版 `write` 重载（生产只用多页重载）；空 pages 内联"暂无分析结果。"
- `pdf_layout_test` 图表用例改写为走生产多页重载（保留 PDF+图表冒烟）

### 阶段 3.3 + 4.3 部分（`094b5af`）
- 新增 `src/infrastructure/data_import_service.{h,cpp}`：扩展名分派下沉，mainwindow 不再直接引用具体导入器；新增 2 个测试用例
- `project_repository` save()：失败路径补 `QSqlDatabase::removeDatabase`；4 条 DELETE 返回值逐一检查、失败回滚
- **决策**：`ProjectService`/`ReportService` 暂缓（当前是一行调用直通，按"删除测试"原则不造浅模块）

---

## 4. 当前架构

```
DataLab(exe: main.cpp+资源) ── PRIVATE → datalab_ui
datalab_ui（src/ui/** + mainwindow）── PUBLIC: domain, Widgets；PRIVATE: application/infrastructure/reporting
datalab_infrastructure（csv_importer, data_import_service, project_repository, output_serialization,
  pdf_report_writer, report_layout_cursor, report_painter, python_table_importer）
  ── PUBLIC: domain, Core, Gui；PRIVATE: reporting, Sql；PUBLIC 定义 DATALAB_SOURCE_DIR
datalab_reporting（src/reporting/**：chart_model/renderer/adapter/coordinate_mapper）── PUBLIC: domain, Gui
datalab_application（analysis_service, analysis_catalog, interpretation_service,
  output_builder, column_assembly, chart_pages）── PUBLIC: domain, Core
datalab_domain（src/domain/**）── 纯 C++，零 Qt 依赖
```

include 根统一为 `src/`（保留 `domain/...`、`ui/...` 自描述前缀），分层由链接图 + lint 脚本约束。

---

## 5. 剩余工作（按优先级）

### 🔴 最高价值：阶段 5 剩余 或 阶段 6 工程化（下一轮主攻）
- **PlotSpec / ChartModel 合一**：4 个视图状态字段（selected/hovered/zoom/pan）收进 `ChartViewState`，明确"数据 vs 视图"边界
- **InterpretationService 类型化**：消灭按表头字符串抓取的隐藏契约（`number_after`/`number_in_column` 仍用 `std::stod`）；核对覆盖不一致（descriptive/chi_square/regression 只落到兜底文案）
- **阶段 6 工程化**：i18n 决策（见 2.4）、CMake 测试目标 function() 化、CI（GitHub Actions + Qt 6.11 + MinGW + ctest）、ADR 补齐、test-results.xml 方差差异核对

### 阶段 2 剩余
- 2.3/3.1/3.4 已完成 ✅（见 §3）；剩余方法为 bespoke 内容不再强行抽取

### 阶段 2.4 文案分离（重新定界 ⏸，等待阶段 6 i18n 决策）
- 证据：`error_page` 44 处、`analysis_name` 39 处全部内容唯一——集中化零去重且调用点变长；i18n 形态（tr() 原位 vs 单语声明）未定，两种都可能推翻 messages.h 大迁移
- 触发条件：阶段 6 i18n 决策确定后按所选形态一次执行（详见 refactor-plan 2.4）

### 阶段 4：Python 桥与序列化
- Python 桥：脚本入 Qt 资源/随 install（删 `DATALAB_SOURCE_DIR` 依赖）、QProcess 异步化、协议版本化、瘦 venv/PyInstaller、补 `PythonTableImporter` 测试（4 类用例）
- 序列化：150 字段声明式映射（键+成员+默认值各一次）+ 全字段 round-trip 测试
- 存储：抽 `ProjectStore` 端口（ADR #5）；`raw_rows.values_json` 改 JSON（注意旧 .dlab 兼容）；schema `user_version` 迁移

### 阶段 5：模型与解释类型化
- `PlotSpec`/`ChartModel` 视图状态收进 `ChartViewState`（或合一）
- `InterpretationService` 类型化（消灭字符串抓取表头的隐藏契约）；核对覆盖不一致（descriptive/chi_square/regression 只落到兜底文案）
- 修 bug：框选 zoom/pan 错位（analysis_chart_widget.cpp:235-241 vs 431-435）、`selected_rows` 的 static thread_local、`run_t_power` ensure_data 语义

### 阶段 6：工程化收尾
- i18n 决策（全量 tr() 或正式声明单语）；CMake 测试目标 function() 化、图标清单单源化；CI（GitHub Actions + Qt 6.11 + MinGW + ctest）；ADR 补齐；确认 test-results.xml 快照中的方差差异（9.58 vs 10.0）是否已修复

---

## 6. 关键经验与约束（新对话必读）

1. **golden 测试是安全网**：`quality_statistics_test`、`minitab_formula_golden_test` 直接断言 AnalysisService 输出；`output_serialization_test` 做 round-trip——重构 AnalysisService 时输出必须逐字不变
2. **命名空间限定**：application 层新头文件里的 domain 类型必须写 `domain::X`（`StatisticTable` 等），否则编译失败（教训：output_builder.h 首版踩过）
3. **git 提交消息**：含中文引号（如 `"暂无分析结果。"`）时 PowerShell 内联 `-m` 会拆参数——用 `[System.IO.File]::WriteAllText(path, $msg, (New-Object System.Text.UTF8Encoding($false)))` 写消息文件 + `git commit -F`（避免 BOM）
4. **编码**：项目源码 UTF-8 无 BOM；pwsh 读写文件用 `-Encoding UTF8` + `UTF8Encoding($false)`，不要用 PS5.1 的 `Set-Content -Encoding utf8`（带 BOM）
5. **大块文本删除**：edit 工具 old_string 必须逐字匹配（含组合字符 R̄/X̄ 等）；不匹配就改用 pwsh 行范围删除（先 read 确认行号）
6. **分层检查**：新文件加入后跑 `tools/check_layering.ps1`；跨层依赖必须显式写 target_link_libraries
7. **新增源文件要加进 CMakeLists 对应目标**，否则链接失败
8. `analysis_service.cpp` 的匿名命名空间 using 块（datalab::domain::X）只在该文件可见；新模块要自己 using 或限定
9. 控制图族模式参考：`chart_pages.h` 的规格结构体 + lambda 设计是薄壳化的样板
10. **抽取前先确认测试覆盖**：`doe_factorial`/`regression`/`logistic`/`paired_t` 的服务层路径原无测试兜底，`74e9a55` 的 `align_complete_rows` 列主序缺陷（regression 预测变量结构被拍平）即因无覆盖而漏网——`1cc6bbd` 补 4 条服务层测试后才发现并修正；抽取行为等价性必须靠测试，不能只靠"逐字搬运"
11. **行主序 vs 列主序**：`align_complete_rows` 输出行主序（`aligned[i][j]` = 第 i 个观测第 j 列），regression 直接切片为观测行、paired_t 需转置——写共享数据装配函数前先想清下游消费方的布局
12. **测试数据退化情形**：logistic 完全可分数据致 IRLS 秩亏（`rank_deficient_design`）；DOE 4 运行=4 参数致零误差自由度（内部回归拒绝拟合）——构造服务层测试数据时规避，或显式断言错误诊断

---

## 7. 关键文件速查

| 文件 | 作用 |
|---|---|
| `docs/refactor-plan.md` | 分阶段重构计划（各阶段状态已标注 ✅/未完成） |
| `docs/architecture-review.md` | 初始架构评审报告（证据+建议） |
| `docs/adr/0001-core-architecture.md` / `0002-layer-targets.md` | 架构决策记录 |
| `src/application/analysis_service.cpp` | 3313 行的编排类（剩余方法为 bespoke 内容，不再强行抽取） |
| `src/application/chart_pages.{h,cpp}` | 控制图族共享页面构建器（薄壳化样板） |
| `src/application/doe_pages.{h,cpp}` | DOE 响应分析页/设计矩阵页组装（2.3 产物） |
| `src/application/output_builder.{h,cpp}` | 数字格式/错误页/通用表格构建器 |
| `src/application/column_assembly.{h,cpp}` | 子组构建/列选择/complete-case 行对齐（`align_complete_rows`，行主序） |
| `src/ui/analysis_commands.{h,cpp}` | **46 项分析命令数据表**（3.1 产物：菜单/对话框/图标/apply/run 单一来源） |
| `src/ui/page_renderer.{h,cpp}` | 分析页共享渲染器（3.4 产物：workspace/预览共用，选项参数化交互） |
| `src/infrastructure/data_import_service.{h,cpp}` | 导入分派门面 |
| `src/ui/mainwindow.cpp` | 842 行（3.1 命令化后只剩通用 run_from_spec 与界面装配） |
| `tools/check_layering.ps1` | 分层 include 检查 |
| `CONTEXT.md` | 领域词汇表 |
