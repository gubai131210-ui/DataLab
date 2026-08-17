# DataLab 重构计划（分阶段）

配套文档：`docs/architecture-review.md`（评审报告）。本文件给出可执行的分阶段实施步骤，每一步都注明目标、改动文件、验证方式和回滚方式。

原则：

- **每步一个可编译、可测试的提交**，不混步。
- **golden 测试是安全网**：`ctest` 全绿是每步的验收门槛（`minitab_formula_golden_test`、`quality_statistics_test`、`output_serialization_test` 等覆盖服务层输出）。
- 所有移动/改名先做机械迁移，再做行为修改。

---

## 阶段 0：地基（半天）

### 0.1 建立版本控制

```powershell
git init
git add .
git commit -m "chore: initial commit"
```

- 先更新 `.gitignore`（见 0.2）再提交，避免把运行产物入库。
- 若无 git 身份，先 `git config user.name` / `user.email`（本地即可，后续可改）。
- 建议尽早建立远端/备份（私有仓库或 NAS），并确认 `build/`、`.venv/` 不入库。

### 0.2 .gitignore 补齐

追加（精确匹配根目录运行产物）：

```gitignore
/test-output.txt
/icon-output.txt
/test-results.xml
```

### 0.3 根目录清理

- `main.cpp`/`mainwindow.*` 移入 `src/ui/`（随阶段 1 的 CMake 拆分一起做）。
- `待修改.md` 是个人笔记，可移入 `docs/notes/` 或保留；`test-output.txt`、`icon-output.txt`、`test-results.xml` 删除或忽略（忽略即可，见 0.2）。

---

## 阶段 1：构建系统分层（1-2 天）

目标：让"分层"在编译期真实存在，违规直接编译失败。

### 1.1 拆分 CMake 目标

现状：`CMakeLists.txt:8-114` 一个 `datalab_domain` 库装四层。

拆成五个静态库 + 一个可执行目标（**已实施**，见 ADR 0002）：

| 目标 | 内容 | 链接 |
|---|---|---|
| `datalab_domain` | `src/domain/**` | 无 Qt（纯 C++） |
| `datalab_application` | `src/application/**` | `datalab_domain` PUBLIC，Qt6::Core |
| `datalab_reporting` | `src/reporting/**`（图表渲染，无 QWidget） | `datalab_domain` PUBLIC，Qt6::Gui |
| `datalab_infrastructure` | `src/infrastructure/**` | `datalab_domain` PUBLIC，Core/Gui；`datalab_reporting` PRIVATE，Qt6::Sql |
| `datalab_ui` | `src/ui/**` + 迁入的 `mainwindow.*` | `datalab_domain` PUBLIC，Widgets；`datalab_application`/`datalab_infrastructure`/`datalab_reporting` PRIVATE |
| `DataLab` | `main.cpp` + 资源 | `datalab_ui` PRIVATE，Widgets/Svg |

- include 根**保持统一为 `src/`**（保留 `domain/...`、`ui/...` 自描述前缀），分层由两层机制约束：
  - 链接期：target 依赖图表达允许方向，反向引用直接链接失败；
  - include 期：`tools/check_layering.ps1` 按层白名单检查 include 前缀（CI 阶段接入）。
  - 备选的"每目标 include 白名单"需要把所有 include 改为层根相对（破坏可读性、改动面大），作为后续强化手段保留（见 ADR 0002）。
- 测试目标改链对应的库：domain 测试链 `datalab_domain`；`worksheet_model_test` 链 `datalab_ui`；`chart_geometry_test` 链 `datalab_reporting`；服务层测试链 `datalab_application`；fixture/序列化/PDF 测试链 `datalab_infrastructure`。

验证：`cmake -S . -B build/... && cmake --build ...` 全绿；`ctest` 全绿。

### 1.2 消除 infrastructure → ui 反向依赖

证据：`pdf_report_writer.cpp:5-6` include `ui/chart_adapter.h`、`ui/chart_renderer.h`。

**已实施**：`ChartRenderer`（928 行，纯 QPainter）、`ChartModel`、`chart_adapter`、`chart_coordinate_mapper` 均不含 QWidget，已从 `src/ui/` 下沉到 `src/reporting/`（新建 reporting 层，见 ADR 0002）：

- include 前缀 `ui/chart_*` → `reporting/chart_*`（12 处消费方同步更新，含 `pdf_report_writer.cpp`、`output_workspace.cpp`、`analysis_chart_widget.*`、`graph_properties_dialog.h`、`report_preview_dialog.cpp`、`chart_geometry_test.cpp`）；
- `pdf_report_writer`（infrastructure）改链 `datalab_reporting`；
- `output_workspace`、`analysis_chart_widget`、`report_preview_dialog`（ui）反向依赖 reporting（方向正常）。

验证：`ctest` 全绿；grep 确认 `src/infrastructure` 下再无 `#include "ui/"`；`tools/check_layering.ps1` 通过。

---

## 阶段 2：拆 `AnalysisService` 管线（2-3 天）

目标：把 4013 行的四合一文件拆成可独立维护的层。保持 `AnalysisService::xxx` 的**公开签名与输出不变**（golden 测试兜底）。

**已完成（首轮）**：`analysis_service.cpp` 4013 → 3625 行。

### 2.1 抽"列装配层"（已完成 ✅）

产出 `src/application/column_assembly.h/.cpp`：`SubgroupInput`、`build_strict_subgroups`、`first_variable`（原 analysis_service.cpp 313-393 行）。
**统一三套数值解析（已完成 ✅）**：pca 的裸 `std::stod`（原 2925-2954 行）改用 `parse_numeric_cell`；`capability_sixpack` 反解析改为 `capability()` 新增可选 out 参数 `capability_indices` 直接捕获结构化值。当前 analysis_service.cpp 已无任何 `std::stod/strtod/stoul/stoi` 残留。
> 剩余：按 by_column 分组（descriptive/one_way_anova 等方法内）仍内联，随 2.4/后续族抽取时下沉；paired_t/regression 的 complete-case 行对齐已下沉 `align_complete_rows`（见 2.3 ✅）。

### 2.2 抽"表格/图表构建器"（已完成 ✅）

产出 `src/application/output_builder.h/.cpp`（280 行）：`format_number`/`format_optional`/`parse_numeric_cell`、`new_id`/`error_page`/`append_diagnostics`、`parse_alternative`/`parse_variance_method`/`alternative_label`/`append_nonnegative_counts`、`t_test_table`/`descriptive_table`/`attribute_chart_table`/`laney_chart_table`/`control_plot`（原 analysis_service.cpp 60-423 行的 18 个匿名辅助）。
> 命名空间注意：这些函数进入 `datalab::application`（非匿名），头文件内 `StatisticTable` 等 domain 类型必须限定为 `domain::StatisticTable`。

### 2.3 45 个方法收敛为薄壳（进行中）

**控制图族已完成 ✅**：新增 `src/application/chart_pages.{h,cpp}`（190 行），三个共享页面构建器：
- `subgroup_dual_chart_page`：Xbar-R / Xbar-S 共用（xbar_range 266→26 行、xbar_s 262→24 行）；
- `attribute_chart_page`：P / NP / C / U 共用（各方法从 ~55 行收敛到 ~40 行规格配置）；
- `laney_chart_page`：Laney P' / Laney U' 共用（从 ~90 行收敛到 ~45 行，含 included_rows→excluded_rows 换算、阶段列处理、参数表）。
方法内的数据装配差异用 `assemble` lambda 表达；页面骨架、表格、图表、错误页全部下沉。`analysis_service.cpp` 3625 → 3486 行。

**大方法薄壳化已完成 ✅**（`57d92c0`/`74e9a55`/`30e14ed`/`1cc6bbd`）：
- `doe_factorial` 292 → 98 行：新增 `doe_pages.{h,cpp}`（响应分析页/设计矩阵页组装），3 处同构 ANOVA 表收敛为 `append_anova_rows`；
- complete-case 行对齐下沉 `column_assembly::align_complete_rows`（行主序：`aligned[i][j]` = 第 i 个对齐观测第 j 列值），paired_t/regression 共用；初版列主序导致 regression 预测变量结构拍平（无测试覆盖未被发现），补 4 条服务层测试后修正；
- `capability` 143 → 65 行（四表+直方图正文下沉 `build_capability_content`）、`capability_sixpack` 138 → 89 行（概率图/最后 25 点图下沉）、`logistic_regression` 125 → 88 行（complete-case 导入下沉 `logistic_import_rows`）；
- 新增服务层测试：`buildsDoeFactorialServiceOutput` / `buildsRegressionServiceOutput` / `buildsLogisticServiceOutput` / `buildsPairedTServiceOutput`（覆盖此前无兜底的抽取路径；测试数据规避完全可分导致的 IRLS 秩亏与零误差自由度退化）。
- `analysis_service.cpp` 3486 → 3313 行；剩余方法多为 bespoke 单表/单图内容（regression 147、arima 149 等），薄壳化收益低，不再强行抽取。

> 剩余：imr/ewma/cusum 已薄（46/25/28 行），无需再处理。

### 2.4 文案与格式分离（未完成）

把 4011 个 CJK 字符收进一个 `src/application/messages.h`（常量/函数），为后续 i18n 或双语维护留口。

### 2.5 顺带消灭"反解析"（已完成 ✅）

- `to_legacy_result` 删除（含 `analysis_service.h` 声明；`AnalysisResult` 结构体仍被 `pdf_report_writer` 旧重载与 `analysis_output_view` 使用，随阶段 3 死代码清理一并处理）；
- `capability_sixpack` 反解析改为结构化捕获（见 2.1）；
- `interpretation_service.cpp` 的死函数 `count_column_values` 删除。

验证：`ctest` 全绿（12/12）；`analysis_service.cpp` 行数 4013 → 3313（薄壳化收敛；剩余 bespoke 内容不再强行抽取）。

---

## 阶段 3：`MainWindow` 减负（2-3 天）

### 3.1 命令化（删 94 行 if/else + 45 个 run_*）

**已完成 ✅**（`47fd2c8`）：实现为 `src/ui/analysis_commands.{h,cpp}`——46 项 `AnalysisCommand` 数据表（id/菜单文字/对话框标题/menu_path/图标/角色与输入规格/`apply`/`run`），`AnalysisCatalog` 随之删除（无调用方）。MainWindow 只留通用 `run_from_spec(id)`，`run_analysis` 的 94 行 if/else 与 45 个 `run_*`（约 1600 行）全部删除；菜单按 `menu_path` 数据化生成（`separator_before` 重现分隔线）；两份图标映射表合并为命令表 `icon_file` 单一来源。`mainwindow.cpp` 2502 → 842 行。doe_response 与 doe_factorial 共用配置构建（doe_apply/doe_run）；t_power 的 `requires_data=false` 保留不调 `ensure_data()`。

原始设计（记录备查）：

```cpp
struct AnalysisSpec {
    QString id;
    QString title;
    QString menu_path;          // 恢复使用（当前为死字段）
    std::vector<RoleSpec> roles;      // add_role 的参数
    std::vector<InputSpec> inputs;    // add_line_edit 的参数
    std::function<OutputPage(const DataTable&, const AnalysisConfiguration&)> run;
    std::function<void(AnalysisConfiguration&, const DialogResult&)> apply; // 配置构建
};
```

- `create_commands` 遍历目录注册菜单动作（`menu_path` 落地）；
- MainWindow 只留一个 `run_from_spec(id)`：`ensure_data()`（按需）→ 建对话框 → 收集选择 → `apply` 填配置 → `publish_page(run(...))`；
- 各分析的特殊校验（pareto 阈值、msa 模式归一化、静默中止等）迁入各自的 `apply` lambda。

### 3.2 配置构造收敛到工厂

`base_configuration()` + 各 run_* 的字段填充下沉为 `AnalysisConfigurationFactory`（application 层），UI 只传"用户选择"（列索引 + 字符串参数）。
> 状态：字段填充已随 3.1 迁入 `analysis_commands` 各 `apply` lambda（ui 层，读 `AnalysisSetupDialog`）；是否进一步下沉为 application 层工厂（纯数据入参）待评估——当前 apply 直接读对话框，UI 耦合已局限在 ui 层，收益有限。

### 3.3 补应用层门面

**DataImportService 已实施 ✅**：`src/infrastructure/data_import_service.{h,cpp}`——扩展名分派（.xlsx/.xls → Python 导入器，其余 → CSV 导入器）与错误传递从 mainwindow.cpp:741-746 下沉；mainwindow 不再直接引用 CsvImporter/PythonTableImporter。新增 2 个测试用例（CSV 经服务导入、文件不存在报错）。
> 说明：`ProjectService`/`ReportService` 暂缓——当前仅是 `ProjectRepository`/`PdfReportWriter` 的一行调用直通，按"删除测试"原则不造浅模块；待 4.3 存储端口（`ProjectStore` 接口 + 迁移）落地时一并引入。

### 3.4 清理

**已实施（首批）✅**：
- 删 `AnalysisDialog`、`AnalysisOutputView`（无调用方）——连同其 CMake 源项一起移除；
- 删 `AnalysisCatalog::menu_path` 死字段（46 处描述符第三项一并剔除）；
- 删 `AnalysisResult` 结构体（`quality_types.h`）与 `PdfReportWriter` 单分析版 `write(file, table, AnalysisResult, ...)` 重载（生产只走多页重载）；空 pages 回退改为内联"暂无分析结果。"；
- `pdf_layout_test` 的图表用例改写为走生产多页重载（保留 PDF+图表冒烟覆盖）。

> 剩余：`output_workspace` 与 `report_preview_dialog` 重复页面渲染抽共享渲染器；行排除纳入 undo 栈。

验证：`ctest` 全绿；`mainwindow.cpp` 从 2502 行降到 842 行（命令表迁往 `analysis_commands.cpp`）；UI 手动过一遍 6 类代表分析（描述统计/单样本 t/Xbar-R/能力/柏拉图/ARIMA）。

---

## 阶段 4：Python 桥与序列化加固（1-2 天）

### 4.1 Python 桥

- `tools/import_table.py` 打进 Qt 资源（`qt_add_resources`）或随 install 安装；运行时 `applicationDirPath()` 解析，**删除 `DATALAB_SOURCE_DIR`**（`python_table_importer.cpp:32-33`、`CMakeLists.txt:123`）；
- 协议加 `schema_version` 字段，C++ 侧校验；stdout 非 JSON 时给出明确错误；
- QProcess 移到 `QtConcurrent`/线程，UI 不阻塞（当前 `waitForFinished(120000)` 冻结界面）；
- 部署：瘦 venv（pandas+openpyxl+xlrd）或 PyInstaller 单文件；`requirements.txt` 拆分 `requirements-runtime.txt`（导入用）与 `requirements-dev.txt`（统计工具预留）。
- **补 `PythonTableImporter` 自动化测试**（当前缺失）：至少覆盖 CSV 回退路径、进程失败/非零退出、stdout 非 JSON、error 字段透传四类用例（可用假的 python 脚本桩替代真实 pandas）。

### 4.2 序列化声明式化

`output_serialization.cpp`（765 行）的 150 个字段改为声明式映射表：

```cpp
struct FieldSpec { QString key; /* 成员访问器 */ };
// 读写/默认值各只出现一次；新增字段只加一行
```

- 补 `output_serialization_test` 的"全字段非默认值 round-trip"用例（现只覆盖子集）。

### 4.3 存储层（配合 ADR #5）

**健壮性修复已实施 ✅**（`project_repository.cpp`）：save() 失败路径补 `removeDatabase`（原 35-41 行，与 load() 对齐）；4 条 `DELETE` 的返回值改为逐一检查、失败即回滚（原 87-90 行）。
> 剩余：抽 `ProjectStore` 端口 + `SQLiteProjectStore` 实现；`raw_rows.values_json` 列名与管道分隔实现不符（106 行）——改为 JSON 需考虑旧 .dlab 兼容，暂缓；补 schema `user_version` 迁移。

验证：`ctest` 全绿；手工导入 xlsx 一次确认不冻 UI。

---

## 阶段 5：模型与解释类型化（2-3 天，可延后）

- **`PlotSpec`/`ChartModel` 合一**：优先低成本方案——把 4 个视图状态字段（`chart_model.h:63-64/82-83`：selected/hovered/zoom/pan）收进 `ChartViewState`，明确"数据 vs 视图"边界；激进方案让渲染器直接消费 domain `PlotSpec`。
- **`InterpretationService` 类型化**：analysis 输出结构化字段（如 `significant_terms`、`shape_parameter`），不再按表头字符串抓取；删除 `count_column_values` 死代码。顺带核对**覆盖不一致**：descriptive / chi_square / regression 等目前只落到兜底文案，类型化时逐分析确认解释覆盖。
- **修已知 bug**：`analysis_chart_widget` 框选 `to_index` 补 zoom/pan 换算（235-241 行 vs 431-435 行）；`run_t_power` 的 `ensure_data()` 语义确认；`analysis_chart_widget::selected_rows`（78-88 行）返回 `static thread_local` 向量引用，改为成员或显式传参。

---

## 阶段 6：工程化收尾（持续）

- **i18n 决策**：全量 `tr()` 走 Qt 翻译，或正式声明简体中文单语并删除空壳 `DataLab_zh_CN.ts`（当前 4 条翻译、0 处 tr()）。
- **CMake 工程化**：测试目标用 `function(add_datalab_test name sources...)` 消除 13 份样板；图标资源清单单源化（当前 DataLab 与 icon_resource_test 各一份）。
- **CI**：GitHub Actions（windows-latest + Qt 6.11 + MinGW）跑 `cmake --build` + `ctest`，把 golden 测试变成提交护栏；CI 中附带 `verification_report.md` 的对照检查（可选）。
- **ADR 补齐**：为本报告的拆分决策（目标分层、Python 桥方案、存储端口）各记一条 ADR。
- **数值差异确认**：核对 `test-results.xml` 快照中的方差差异（9.58 vs 10.0，population/sample 口径）当前是否已修复，补回归测试。

---

## 风险与回滚

| 步骤 | 主要风险 | 回滚 |
|---|---|---|
| 1.x CMake 拆分 | include 路径/automoc 调整遗漏 | 单提交，`git revert` |
| 2.x AnalysisService | 输出格式变化（表头/顺序） | golden 测试失败即停；逐方法迁移 |
| 3.x 命令化 | 个别分析校验差异漏迁 | 逐分析对比手动结果 |
| 4.x Python 桥 | 部署环境差异 | 保留旧路径回退逻辑 |
| 5.x 模型合一 | 渲染回归 | 视觉对比 + 像素级冒烟测试 |

每步完成条件：`cmake --build` 全绿 + `ctest` 全绿 + 代表性手动冒烟。
