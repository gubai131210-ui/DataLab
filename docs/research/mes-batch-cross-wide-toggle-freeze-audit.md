# 批量跨站宽表切换假死 — 审计与施工单（Agent1）

> **日期**：2026-09-08（UTC+8）  
> **岗**：Agent1 计划（本文仅施工文档；产品代码由 Agent2 修改）  
> **复现（用户拍板）**：勾选「宽表输出」→ 表已转宽 → **再取消勾选** → UI 假死、进程仍在；规模约 **100 SN × 10 任务**  
> **范围**：P2（先修本卡死 + 扫 MES 同类 / DataLab 大表笔记）  
> **产品不变式**：不改结构 / 功能面 / 使用习惯；只改内部实现消假死  

---

## 1. P0 用户路径（1A）

| 步骤 | 操作 | 代码入口 |
|------|------|----------|
| 1 | 查询完成 | `on_finished` |
| 2 | 勾选「宽表输出」 | `wide_output_checkbox_` `toggled` → `reset_result_table_headers` + `refresh_result_table`（宽） |
| 3 | **取消勾选** | 同上 → 长表 `refresh_result_table` | **假死点** |

焦点文件：`Mes_For_Q/ui/batch_cross_station_page.cpp`

---

## 2. 根因

1. `configure_result_table_layout(false)` 将 Header 设为 **`QHeaderView::ResizeToContents` 永久模式** → 每个 `setItem` 触发测宽。  
2. 取消勾选后 `refresh_result_table` 长表：`setRowCount(~1000)` + 逐格 `new QTableWidgetItem` + `resizeColumnsToContents`。  
3. 无 `setUpdatesEnabled(false)`、切换前不清空旧宽表格子。  
4. **主线程 O(行×列×测宽) 假死**，非典型堆泄漏闪退。

次要：

- `update_wide_result_row`：每次 `rowUpdated` 全量 `pivot_cross_station_wide`。  
- `CrossStationRunner`：`QTimer::singleShot` 重试在 `request_stop` 后可能仍触发（generation 未挡 timer）。

---

## 3. 必改函数清单（Agent2）

| 函数 / 位置 | 动作 |
|-------------|------|
| `BatchCrossStationPage::configure_result_table_layout` | 长/宽均 `Interactive`；禁止长表永久 `ResizeToContents` |
| `BatchCrossStationPage::refresh_result_table` | `setUpdatesEnabled(false)` → `setRowCount(0)` → 填表 → 一次轻量列宽 → 恢复 |
| `BatchCrossStationPage::update_wide_result_row` | 单 SN 切片 pivot，禁止每次全量 |
| `cross_station_result_builder` | 新增单行 wide 构建（或等价） |
| `CrossStationRunner::start` / `request_stop` / retry lambda | `job_generation_` 挡 timer |
| `on_finished` | 避免大表 `resizeSections(ResizeToContents)` 卡死 |

---

## 4. P2 扫描目标

| 区域 | 发现 | Agent2 动作 |
|------|------|-------------|
| `BatchQueryPage` ~L1072 | 非宽表完成时 `resizeSections(ResizeToContents)` | 大结果改为 Interactive / 跳过全表测宽 |
| `CrossStationPage` | result/task Header `ResizeToContents`；完成时 resizeSections | 结果表改为 Interactive；避免完成时全量测宽假死（单 SN 规模通常小，仍对齐习惯） |
| DataLab `worksheet_model` / `mes_table_adapter` | 模型 reset，非 QTableWidget 逐格测宽 | **记录不修**（非本复现同构） |
| DataLab 图表 | 大系列布局 | **记录不修** |

---

## 5. 全局禁止偷懒（粘贴）

1. 禁止只删 `resizeColumnsToContents` 却保留 Header `ResizeToContents` 模式  
2. 禁止 `processEvents` 糊弄  
3. 禁止本轮虚拟表大重构替代修复  
4. 禁止把泄漏审计冒充本 bug 修复  
5. 禁止只改 DataLab 不改 Mes_For_Q  
6. 禁止 Agent 跳岗 / 中途换模型  
7. 禁止用小数据测代替 100×10  
8. 禁止升无关 scope（P3、Track A、学习中心）  
9. **禁止改原来程序结构、功能面、使用习惯**  
10. 禁止「顺手重构」MesShell、Batch 页布局、导入工作区交互  

---

## 6. 完成定义

- [x] 取消勾选宽表在 ~100×10 下可继续交互（不永久假死） — *Agent2：去掉 ResizeToContents 永久模式 + 填表停用更新；需人工用 100×10 复验*
- [x] 宽表仍一行一 SN；长表仍一行一任务；勾选框语义不变 — *toggled 连接未改；builder/测试锁定形状*
- [x] 自动化：100×10 假数据 wide↔long 切换断言 — *`cross_station_wide_long_toggle_hundred_by_ten` + `cross_station_wide_row_matches_full_pivot`*
- [x] Agent3/4/5 过关；审计本文补已改文件与 Review

**go/no-go（Agent1）**：**GO** → Agent2

---

## 7. Agent2 已改文件列表（交付）

| 文件 | 变更要点 |
|------|----------|
| `Mes_For_Q/ui/batch_cross_station_page.cpp` / `.h` | 长/宽均 `Interactive`；`refresh_result_table` 关更新→`setRowCount(0)`→填表→默认列宽；禁止全表 `ResizeToContents`；`update_wide_result_row` 改单行 pivot；`on_finished` 去掉大表测宽 |
| `Mes_For_Q/mes/cross_station_result_builder.h` / `.cpp` | 新增 `pivot_cross_station_wide_row`；全量 pivot 复用单行 API；OOR probe column=3 |
| `Mes_For_Q/mes/cross_station_runner.h` / `.cpp` | `job_generation_`；`start`/`request_stop` 递增；`QTimer::singleShot` 与回调入口按 generation 丢弃迟到任务（不再二次 `--active_count_`） |
| `Mes_For_Q/ui/batch_query_page.cpp` | P2：非宽表完成时去掉 `resizeSections(ResizeToContents)` |
| `Mes_For_Q/ui/cross_station_page.cpp` | P2：结果表 Header `Interactive`；刷新/完成避免全表 `ResizeToContents` |
| `Mes_For_Q/tests/test_batch_cross_station.h` / `.cpp` | 单行 pivot 一致性 + 100×10 宽/长形状断言 |
| `Mes_For_Q/tests/test_main.cpp` | `--suite=batch_cross` / `--suite=mes_api` 分流（避免全量 API 挂死） |
| `docs/research/mes-batch-cross-wide-toggle-freeze-audit.md` | 本段交付记录（DataLab） |

**未改**：DataLab 学习中心/算法；MesShell 布局；虚拟表大重构；`processEvents`。

**Agent2 go/no-go**：**GO**

---

## 8. Agent3 静态检查

> **结论**：PASS  
> grep：结果表长表无永久 `ResizeToContents`；`update_wide_result_row` 用单行 pivot；stop 有 generation；无 `processEvents`；P2 BatchQuery/单跨站已定点或记录。

---

## 9. Agent4 测试

### 自动化（已跑）

```text
cd d:\QT_CppPrograms\Mes_For_Q\build-mingw
.\Mes_For_Q_Tests.exe --suite=batch_cross <offline slots...> -o batch_suite_result.txt,txt
```

- **Totals: 12 passed, 0 failed**（含 `cross_station_wide_long_toggle_hundred_by_ten`，约 10–24ms）
- 全量 `Mes_For_Q_Tests.exe`（含 MesApiTest 网络）可能崩溃/挂起 → **已知 harness**，用 `--suite=batch_cross` 测本修复

### 本机人工红环（请你本机复测；中文路径不代跑 package）

1. 启动 DataLab → MES 工具 → **批量跨站**
2. 约 **100 SN × 10 任务**查完（或已有结果）
3. 勾选 **「宽表输出」** → 表变宽（一行一 SN）
4. **再取消勾选** → 应能继续点侧栏/按钮，**不再假死**
5. 可选：查询中点停止；导出 Excel 列含义与现网一致

---

## 10. Agent5 Review（Code Review）

> **日期**：2026-09-08（UTC+8）  
> **结论**：PASS  
> **产品不变式**：成立（宽表勾选/长宽形状/导出查询流未改；仅内部冻屏与同类 P2）

### 验证摘要

- 结果表长/宽均为 Interactive；`refresh_result_table` 使用 setUpdatesEnabled + setRowCount(0) + 默认列宽；无永久 ResizeToContents、无 processEvents、无虚拟表重写。
- `pivot_cross_station_wide_row` 与全量 pivot 一致；OOR probe column=3；`update_wide_result_row` 单 SN 切片。
- `CrossStationRunner::job_generation_` 挡 stop 后 singleShot/直连回调。
- 测试 `cross_station_wide_long_toggle_hundred_by_ten` 为真 100×10；`--suite=batch_cross` 可用。

### 残留（非阻断）

- flex/route/fallback 回调未挂 generation（非本轮 P0 冻屏项）。

### go/no-go（Agent5）

**GO**（待人工 100×10 UI 复验取消勾选宽表）

---

## 11. 版本库说明

- 产品修复在兄弟目录 `d:\QT_CppPrograms\Mes_For_Q`（**无独立 git remote**；由 DataLab `add_subdirectory(../Mes_For_Q)` 消费）。
- DataLab 仓库仅跟踪本审计 md；Mes 源码需你自行备份或另建仓库。