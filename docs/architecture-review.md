# DataLab 架构与可持续发展评审报告

- 评审日期：2026-08
- 评审范围：全仓只读分析（src、tests、docs、samples、tools、CMakeLists.txt、根目录遗留文件）
- 结论：**算法工程质量高，软件工程存在系统性风险**。领域层设计干净、验证纪律优秀；但应用层与 UI 层存在两个"上帝对象"，且**完全没有版本控制**、构建系统不强制分层、Python 桥部署脆弱。所有问题集中在少数文件，重构性价比高。

---

## 1. 项目概况

| 项 | 值 |
|---|---|
| 产品 | DataLab —— 面向汽车制造质量工程师的统计分析工具（Minitab 替代品） |
| 技术栈 | Qt 6.11.1 / C++17 / CMake / MinGW 13.1 / SQLite / 少量 Python |
| 规模 | 约 21,300 行 C++（src + tests），46 种分析，13 个测试目标 |
| 分层意图 | domain / application / infrastructure / ui（见 `docs/adr/0001-core-architecture.md`） |

## 2. 总体框架评估

### 2.1 做对的部分 ✅

| 项 | 证据 |
|---|---|
| `src/domain/` 目录纯净 | 对 domain 全目录 grep `#include <Q`、`ui/`、`application/`、`infrastructure/` **零匹配**，纯 `std::` C++，不依赖 Qt |
| 领域统计模块是典型"深模块" | `ControlCharts`（`src/domain/statistics/control_charts.h`）：类型化 options + 结构化 result，小接口背后 37KB 实现；`RegressionResult` 等结果结构体字段完整 |
| 文档文化 | ADR、CONTEXT.md 领域词汇、28KB 统计方法规范、UI 规范 |
| 数值正确性纪律 | `.cursor/skills/data_verification/SKILL.md`：每个算法要求三份真实数据集 + Minitab 人工对照 + 溯源（URL/SHA-256/许可证/转换步骤）；`samples/` 下有完整验证报告与 golden 测试 |
| 通用 UI 控件 | `AnalysisSetupDialog` 一个角色式对话框服务全部 46 种分析（深模块的正面例子） |

### 2.2 落地走样的部分 ⚠️

1. **`datalab_domain` 库名不副实**（`CMakeLists.txt:8-114`）：名为 domain 的静态库实际编入了 domain + application + infrastructure + **全部 `src/ui/*.cpp`**（即"除 mainwindow 外的一切"），因此 `PUBLIC` 链接 `Qt6::Widgets`。"领域库依赖 Widgets"是分层混乱的直接证据。
2. **mainwindow 游离于根目录**（`main.cpp`/`mainwindow.*` 在项目根，其余 UI 在 `src/ui/`）。它靠 `datalab_domain` 的 PUBLIC include 目录解析 `"ui/..."`、`"application/..."` 头文件——可执行文件源码与库的 include 布局隐式耦合。
3. **无依赖方向护栏**：唯一 include 目录是 `PUBLIC src/`，任何层可 include 任何层。`infrastructure/pdf_report_writer.cpp:5-6` 直接 include `ui/chart_adapter.h`、`ui/chart_renderer.h` —— **infrastructure 反向依赖 ui**，与声明分层相反。
4. `src/ui` 库清单不完整（`worksheet_view.h`、`report_preview_dialog.h`、`graph_properties_dialog.h` 只有 .cpp 入列），清单本身随意。

## 3. 耦合性分析：四个核心病灶

### 3.1 病灶 1：`AnalysisService` —— 4013 行四合一上帝对象

`src/application/analysis_service.cpp`（4013 行）= 45 个静态方法 + 17 个匿名辅助函数，全部遵循同一模板：提取数据 → 调 domain → 拼表格/图 → 造错误页。

- **职责四合一**：行对齐/列映射、统计调用、**中英文案格式化**、错误处理全部内联在单方法内。4011 个 CJK 字符、全部表头（如"置信区间"）、数字格式策略（`format_number` 被调用 **374 次**）、错误消息（`error_page` 70 次）硬编码于此。
- **反解析自己的输出**：`capability_sixpack`（3796-3813 行）从自己产出的表格字符串抓数重组；`to_legacy_result`（3991 行）`std::strtod` 反解析表格——该层缺少结构化结果，只能用文本自噬。
- **与 domain 强耦合**：include 30 个头文件，137 次 `datalab::domain::statistics::` 调用，构造 10+ 种 domain 选项类型。

### 3.2 病灶 2：`MainWindow` —— 2502 行七类职责上帝对象

`mainwindow.cpp`（2502 行）至少承担 7 类职责：命令/菜单构建、布局编排、数据处理、45 个分析分发、剪贴板、undo 栈、事件过滤。

- **45 个 `run_*` 方法完全同构**（945-2441 行）：`ensure_data()` → 建 `AnalysisSetupDialog` 声明角色 → `base_configuration()` → 逐字段填充 → `publish_page(AnalysisService::xxx(table_, configuration))`。
- `run_analysis(id)`（849-943 行）是 **94 行 if/else 字符串分派链**；`CommandRegistry` 只是 QAction 注册表，命令模式只做了一半。
- **UI 直接构造 115 字段的 `AnalysisConfiguration`**（`base_configuration()` 810-815 行）——字段任何改动牵动 45 处，是全项目最深耦合点。
- **UI 绕过 application 直接调 infrastructure**：`ProjectRepository`（711/788 行）、`CsvImporter`/`PythonTableImporter`（745-746 行）、`PdfReportWriter`（2495 行）被 MainWindow 直接实例化；**应用层没有 ProjectService/DataImportService 门面**。

### 3.3 病灶 3：上帝结构体 + 双份镜像模型

- **`AnalysisConfiguration` 约 115 个字段**（`quality_types.h:40-155`）涵盖 DOE/Gage/ARIMA/PCA/功效/季节性等所有分析——新增分析类型要同时动它、动 AnalysisService、动 MainWindow、动序列化。
- **`PlotSpec`（domain）与 `ChartModel`（ui）是 37/39 字段的平行镜像**，`chart_adapter.cpp` 150 行逐字段双向拷贝。每加一个图字段要改三处；**反向函数 `plot_from_chart_model` 是 74 行死代码**（全仓 0 调用）。
- **`quality_types.h` 是中心枢纽头**：所有层都 include 它，膨胀拖慢编译并放大每次改动的爆炸半径。
- `PlotSpec` 含 `show_grid`/`show_legend`/`line_width`/`center_label`、`AnalysisConfiguration::chart_type` —— 展示概念泄漏进领域数据形状（include 层面无泄漏，数据形状层面有）。

### 3.4 病灶 4：隐藏契约与死代码

- **`InterpretationService` 靠字符串抓取反解析输出**（`interpretation_service.cpp`）：按表头 `"P"`/`"Cpk"`/`"Slope"` 和 `method_name` 子串（`"Capability Sixpack"`、`"Kaplan-Meier"`）在格式化后的表格上 `std::stod` 抓数。analysis_service 改表头，interpretation 静默失效——两处存在无文档的隐藏契约。
- **死代码清单**：`AnalysisDialog`、`AnalysisOutputView`（遗留 UI 类）、`AnalysisService::to_legacy_result`、`AnalysisCatalog::menu_path` 字段（从未使用）、`InterpretationService::count_column_values`、`ReportPainter::wrapped_height`（被 `pdf_report_writer.cpp:251-262` 内联重实现替代）、`chart_adapter::plot_from_chart_model`。
- **重复实现**：两份图标映射表（`mainwindow.cpp:195-238` vs `analysis_setup_dialog.cpp:26-80`）；`output_workspace` 与 `report_preview_dialog` 整段重复的页面渲染逻辑；`pdf_report_writer` 两个 `write()` 重载复制约 50 行报告骨架；`series_color`/confidence band 绘制在 `chart_renderer.cpp` 内两处重复。

## 4. 代码分区评估

**✅ 合理**：`src/domain/statistics/` 一分析一文件；tests 独立成目标；samples/docs/tools/translations/resources 各归其位。

**❌ 混乱**：
- 根目录残留 `mainwindow.*` 游离于 src/ui；`待修改.md`（个人笔记）、`icon-output.txt`、`test-output.txt`、`test-results.xml` 等产物躺在仓库根。
- CMakeLists 439 行：13 个测试目标复制 3 行样板；图标资源清单完整写了两遍（DataLab 与 icon_resource_test）。
- **i18n 是空壳**：全仓 UI 代码 **0 处 `tr()`**，`DataLab_zh_CN.ts` 只有 4 条翻译。要么真正国际化，要么正式声明简体中文单语。
- `report_layout_cursor.h` 的类在全局命名空间，`report_painter.h` 在 `datalab::infrastructure`，命名空间不一致。

## 5. 可持续发展问题

### 5.1 🔴 没有版本控制（最紧迫）

`.gitignore` 存在但**整个项目（含父目录）无 `.git`**。无历史、无回滚、无备份、无法 diff/协作。一次误删或磁盘故障即丢失全部（含 samples 中珍贵的 Minitab 对照数据）。**应立刻 `git init` 并提交。**

### 5.2 ~~Python 桥部署脆弱~~ → 已解决（ADR 0006）

原 `PythonTableImporter` + pandas 方案已移除。现由 `ExcelTableImporter`（Qt + zlib）在 infrastructure 层原生读取 `.xlsx`，无需 Python 运行时或 exe 旁 `.venv`。

### 5.3 存储层无 seam、无迁移

ADR 0001 承诺"后续通过存储接口增加 MySQL/SQL Server 适配器"，但 `ProjectRepository` 是 `final` 具体类、无抽象接口、QSQLITE 硬编码（`project_repository.cpp:32/177`）。健壮性缺陷：
- 失败路径不调 `QSqlDatabase::removeDatabase`（35-41 行），连接名残留全局注册表；
- 4 条 `DELETE` 的返回值被忽略（87-90 行），删除失败仍继续插入并 commit；
- `raw_rows.values_json` 列名与管道分隔实现不符（106 行），与 `output_pages.payload` 的 JSON 不一致；
- 无 schema `user_version` 迁移机制，`CREATE TABLE IF NOT EXISTS` 不演进老库。

### 5.4 序列化三处重复

`output_serialization.cpp`（765 行）约 150 个配置字段的 JSON 键在写方向（99-319 行）与读方向（420-673 行）各写一遍，默认值第三遍重复（如 `toDouble(0.2)` 503/513 行、`toString("two_sided")` 492 行）——改 domain 默认值必须同步改这里。

### 5.5 其他

- 无 CI；无打包/安装验证（仅有 `qt_generate_deploy_app_script`）。
- 历史测试快照 `test-results.xml` 显示一个**未决方差数值差异**（9.58 vs 10.0，population/sample 方差口径），需确认当前是否已修复。
- 已知潜在 bug：`analysis_chart_widget` 框选 `to_index` 未考虑 zoom/pan 而 `hit_test` 考虑了（缩放/平移后框选错位）；`run_t_power` 不调 `ensure_data()`、部分方法漏设 `analysis_name`（复制粘贴松散证据）。
- 图表几何**双份实现**：边距魔数 `(58,42,-96,-48)`/`(64,42,-88,-178)` 在 `chart_renderer.cpp:27/33` 与 `analysis_chart_widget.cpp:235/372-374` 各一份，改边距必须同步两处；`chart_geometry_test` 只测了 `ChartCoordinateMapper`，renderer 本体无单测。

## 6. 建议（按优先级）

### P0 —— 立刻做（低风险、高收益）

| # | 动作 | 收益 | 风险 |
|---|---|---|---|
| 1 | **`git init` + 首次提交**（.gitignore 已就绪，补运行产物后提交） | 恢复历史/回滚/备份/diff，可持续性地基 | 无 |
| 2 | **拆 `datalab_domain` 库 + mainwindow 移入 `src/ui/`**：按层拆 `datalab_domain`（纯 C++，去 Widgets 依赖）/ `datalab_application` / `datalab_infrastructure` / `datalab_ui`；`DataLab` 可执行目标只留 `main.cpp`+资源 | 库名与内容一致；domain 可独立编译测试；层违规在编译期暴露 | 低，机械迁移 |
| 3 | **消除 infrastructure→ui 反向依赖**：`ChartRenderer`/`ChartModel`/`chart_adapter`/`chart_coordinate_mapper` 均不含 QWidget（纯 QPainter），下沉为独立渲染模块，`pdf_report_writer` 不再 include `ui/` | 恢复分层正确性；infrastructure 可独立测试 | 低-中 |

### P1 —— 近期做（中风险、消除核心耦合）

| # | 动作 | 收益 | 风险 |
|---|---|---|---|
| 4 | **`AnalysisService` 拆管线**：抽"列装配层"（列映射/子组构建）、"表格构建器"、"文案与数字格式层"，45 个方法收敛为薄壳 | 样板集中一处；文案可独立维护/i18n | 中（golden 测试兜底） |
| 5 | **`run_*` 命令化**：`AnalysisCatalog` 扩展为"参数规格+配置构建器"数据表，MainWindow 只留通用 `run_from_spec(id)` | 删约 1000 行样板；新增分析=加一行数据 | 中，需迁移各方法校验差异 |
| 6 | **配置构造收敛到工厂**：per-analysis builder 取代 `base_configuration()`+逐字段填充 | UI 不再触碰 115 字段结构体 | 中，分批 |
| 7 | **补应用层门面**：`ProjectService`/`DataImportService`/`ReportService` | 导入/存取可脱离 UI 测试 | 低-中 |
| 8 | **Python 桥加固**：脚本入 Qt 资源或随 install 安装（删 `DATALAB_SOURCE_DIR`）；QProcess 异步化；瘦 venv 或 PyInstaller 单文件 | 部署可用、不冻 UI | 中 |
| 9 | **序列化声明式化**：150 字段声明式映射（键+成员+默认值各一次），补全字段 round-trip 测试 | 加字段不再三处改 | 中 |
| 10 | **死代码清理**：删 `AnalysisDialog`、`AnalysisOutputView`、`to_legacy_result`、`menu_path`、`plot_from_chart_model`、`wrapped_height`；合并图标映射表；抽取共享页面渲染器 | 减少维护面 | 极低 |

### P2 —— 中期（结构性优化）

- **`PlotSpec`/`ChartModel` 合一**或至少把视图状态（zoom/pan/选中/悬停）收进独立 `ChartViewState`。
- **`InterpretationService` 类型化**：analysis 输出结构化字段（如 `significant_terms`），消灭字符串抓取的隐藏契约。
- **落地 ADR #5**：抽 `ProjectStore` 端口 + SQLite 适配器（当前只有一个实现——抽出接口但不过度设计）。
- **i18n 决策**：全量 `tr()` 或正式声明单语并删除空壳 .ts。
- **CMake 工程化**：测试目标 `function()` 化；图标清单单源化；清理根目录遗留产物。
- **CI**：GitHub Actions 装 Qt 6.11 + MinGW 跑 `ctest`，golden 测试作为每次提交护栏。
- **修已知缺陷**：框选 zoom/pan 错位、`removeDatabase` 缺失、`DELETE` 返回值忽略、确认方差测试差异。

## 7. 总体判断

架构方向正确（分层意图、领域纯净、验证纪律），主要债务集中在三处：**`analysis_service.cpp`（4013 行）+ `mainwindow.cpp`（2502 行）+ `quality_types.h`（115 字段配置）**，合计约占总代码量三成，是所有耦合的汇聚点。domain 层干净、golden 测试提供重构安全网、文档习惯好——先做 P0 三件事（git、拆库、消反向依赖），再按 P1 逐个拆上帝对象，可持续性即可立起。

具体实施步骤见 `docs/refactor-plan.md`。
