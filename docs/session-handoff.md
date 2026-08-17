# DataLab 重构会话交接文档

> 本文件记录截至当前会话的完整工作状态，供**新对话**无缝续接。新对话第一条消息建议：
> 「读取 `docs/session-handoff.md` 与 `docs/refactor-plan.md`，继续 DataLab 重构，从阶段 3.1 命令化开始。」

---

## 1. 项目与环境

| 项 | 值 |
|---|---|
| 项目路径 | `D:\QT_CppPrograms\DataLab` |
| 产品 | 模仿 Minitab 的汽车质量分析工具（46 种分析） |
| 技术栈 | Qt 6.11.1 / MinGW 13.1 / C++17 / CMake / SQLite；Python(pandas) 仅用于 Excel 导入桥 |
| 规模 | 约 20,500 行 C++（src+tests），12 个测试目标 |
| 版本控制 | 本地 git 仓库（初始提交 `26ad743` 后 7 个重构提交）；无远端 |

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

### 🔴 最高价值：阶段 3.1 命令化（下一轮主攻）
目标：`AnalysisCatalog` 数据驱动化，删 MainWindow 的 94 行 if/else（`run_analysis`，mainwindow.cpp:849-943）和 45 个 `run_*`（约 1000 行样板），菜单随 `menu_path` 数据化。

已确认的可行性事实（run_* 模式高度统一）：
```
① if (!ensure_data()) return;
② AnalysisSetupDialog dialog(标题, column_labels(), this);
③ dialog.add_role(id, 标签, multi, optional) / add_line_edit(id, 标签, 占位)
④ if (dialog.exec() != QDialog::Accepted) return;
⑤ 读 role_indices/first_role_index/line_text/line_number/line_int，校验（QMessageBox）
⑥ auto configuration = base_configuration();
⑦ 填 analysis_name/chart_type/字段
⑧ publish_page(AnalysisService::xxx(table_, configuration));
```

设计（写于 docs/refactor-plan.md 3.1）：
```cpp
struct AnalysisCommand {
    QString id, title, menu_path;
    std::vector<RoleSpec> roles;       // add_role 参数
    std::vector<InputSpec> inputs;     // add_line_edit 参数
    std::function<void(datalab::domain::AnalysisConfiguration&, AnalysisSetupDialog&,
                       std::optional<QString>& error)> apply;   // ⑤⑥⑦
    std::function<datalab::domain::OutputPage(const datalab::domain::DataTable&,
        const datalab::domain::AnalysisConfiguration&)> run;    // AnalysisService::xxx
};
```
- 建议放 `src/ui/analysis_commands.{h,cpp}`（引用 AnalysisSetupDialog + AnalysisService）
- MainWindow 只留通用 `run_from_spec(id)`；`create_commands` 遍历命令注册菜单（按 menu_path 分组）
- 转换顺序建议：先控制图 11 个（已熟悉）→ 统计检验 10 个 → DOE/MSA/时序/图形 24 个；未转换的先用 if/else 兜底，全部转换后再删 if/else
- 注意：`run_t_power` 不调 ensure_data()；pareto 有阈值校验；msa_type1 有模式归一化——这些特殊校验迁入各自 apply
- 附带收益：3.4 剩余的图标映射表合并（mainwindow.cpp:195-238 vs analysis_setup_dialog.cpp:26-80）随命令化由 AnalysisCatalog 提供图标路径

### 阶段 2 剩余
- **2.3 剩余**：统计检验族方法本身已 30-90 行（薄壳化收益低）；大方法（doe_factorial 292、regression 165、capability 127、sixpack 153、variance_test 130、logistic 125）为定制化多表输出，需逐方法内部抽取（如 doe 内部 3 处 ANOVA 表同构）
- **2.4 文案分离**：4011 个 CJK 字符收进 `messages.h`（改动面大、为 i18n 铺路）

### 阶段 3 剩余
- **3.2 配置构造收敛到工厂**：`base_configuration()` + run_* 字段填充下沉为 per-analysis builder（随 3.1 一起做）
- **3.4 剩余**：图标映射表合并（随 3.1）；output_workspace 与 report_preview_dialog 重复页面渲染抽共享渲染器；行排除纳入 undo 栈

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

---

## 7. 关键文件速查

| 文件 | 作用 |
|---|---|
| `docs/refactor-plan.md` | 分阶段重构计划（各阶段状态已标注 ✅/未完成） |
| `docs/architecture-review.md` | 初始架构评审报告（证据+建议） |
| `docs/adr/0001-core-architecture.md` / `0002-layer-targets.md` | 架构决策记录 |
| `src/application/analysis_service.cpp` | 3486 行的编排类（剩余大方法的薄壳化目标） |
| `src/application/chart_pages.{h,cpp}` | 控制图族共享页面构建器（薄壳化样板） |
| `src/application/output_builder.{h,cpp}` | 数字格式/错误页/通用表格构建器 |
| `src/application/column_assembly.{h,cpp}` | 子组构建/列选择 |
| `src/infrastructure/data_import_service.{h,cpp}` | 导入分派门面 |
| `src/ui/mainwindow.cpp` | 2502 行（3.1 命令化后应降至 ~800） |
| `tools/check_layering.ps1` | 分层 include 检查 |
| `CONTEXT.md` | 领域词汇表 |
