# ADR 0002：按层拆分 CMake 目标与图表渲染模块下沉

- 状态：已接受
- 日期：2026-08

## 背景

ADR 0001 确立了 domain / application / infrastructure / ui 四层结构，但构建层面一直把四层全部编进单一静态库 `datalab_domain`（该库还链接 Qt6::Widgets，名为 domain 实为"除 mainwindow 外的一切"），分层没有编译/链接期约束。实际出现的唯一跨层上行依赖是 `infrastructure/pdf_report_writer.cpp` include `ui/chart_adapter.h`、`ui/chart_renderer.h`（infrastructure 反向依赖 ui）。同时 mainwindow.cpp/h/ui 游离在项目根目录，其余 UI 在 src/ui/。

## 决策

1. **按层拆分 CMake 目标**：`datalab_domain`（纯 C++，不链接 Qt）、`datalab_application`（Qt6::Core）、`datalab_reporting`（Qt6::Gui）、`datalab_infrastructure`（Core/Gui/Sql）、`datalab_ui`（Widgets），`DataLab` 可执行目标只保留 main.cpp 与资源。
2. **新增 reporting 层**：把 `chart_model` / `chart_renderer` / `chart_adapter` / `chart_coordinate_mapper` 从 src/ui/ 下沉到 src/reporting/。这四个文件只依赖 QPainter/QPointF 等 QtGui 类型，不依赖任何 QWidget；ui（图表控件）与 infrastructure（PDF 写入）共用该层，从而消除 infrastructure→ui 反向依赖。
3. **mainwindow 迁入 src/ui/**，与其余 UI 文件同层；main.cpp 保持根目录作为可执行入口。
4. **include 根保持统一为 src/**，分层由两层机制约束：
   - 链接期：目标间的 target_link_libraries 依赖图表达允许的依赖方向（如 datalab_infrastructure 不链接 datalab_ui，任何反向符号引用都会链接失败）；
   - include 期：`tools/check_layering.ps1` 按层白名单检查 `#include "层/..."` 前缀（如 domain 不得 include "ui/..."），在 CI 与本地执行。
   - 不做"每目标 include 白名单"的原因：现有代码统一使用 `src/` 根相对的自描述前缀（`domain/...`、`ui/...`），改为层根相对会破坏可读性并引入大量机械改动；链接图 + lint 脚本已能覆盖实际风险（历史唯一违规即 infra→ui，已结构性消除）。

## 备选方案

### 每目标 include 白名单（target_include_directories 只暴露允许的层）

编译期最强约束，但需把所有 `#include "domain/X.h"` 改为 `#include "X.h"` 并逐目标配置 include 目录，改动面大、可读性下降；与链接图相比边际收益有限，作为后续强化手段（配合 CI）保留。

### 图表渲染不建新层，直接并入 infrastructure

技术上可行（都是 QtGui 绘制），但把"展示/渲染"归入"外部依赖适配"语义不符，且 ui 依赖 infrastructure 的方向会让 ui 无法独立于存储/导入模块编译。

## 后果

- 分层在链接期真实存在：新增跨层依赖必须显式声明，违规方向直接链接失败。
- `datalab_domain` 成为无 Qt 依赖的纯 C++ 库，可独立编译与测试。
- include 方向由 lint 脚本把关（CI 阶段接入，见重构计划阶段 6）。
- 后续重构（拆 AnalysisService、命令化、存储端口等）拥有独立的目标边界，可按层推进而不互相牵连。
