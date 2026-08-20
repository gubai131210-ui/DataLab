# ADR 0006：原生 C++ XLSX 导入

- 状态：已接受
- 日期：2026-08-20
- 取代：ADR 0005

## 背景

DataLab 曾通过 `PythonTableImporter` + `tools/import_table.py` + 运行时 Python/pandas 读取 Excel。该方案部署脆弱、阻塞 UI、且与“统计核心在 C++”的架构目标不一致。

## 决策

1. 应用运行时不再依赖 Python 解释器或 pandas/openpyxl。
2. 在 `src/infrastructure/` 新增 `ExcelTableImporter`，使用 Qt + vendored miniz 读取 OOXML `.xlsx`。
3. `DataImportService` 分派：`.csv`/`.txt` → `CsvImporter`，`.xlsx` → `ExcelTableImporter`。
4. 不支持老式 BIFF `.xls`；用户需另存为 `.xlsx` 或 `.csv`。
5. 仍只读取第一个工作表，输出 `DataTable` 契约与原先 Python 桥一致。
6. 独立功能性 Python 工具（如 Minitab fixture 转换）保留在 `tools/`，不属于应用运行时。

## 依赖

- **miniz 3.0.2**（vendored：`third_party/miniz/`，MIT）
- **Qt 6 Core**：XML 解析、文件 IO
- 无 QXlsx / pybind / 运行时 venv

## 后果

- Windows 部署不再需要 exe 旁 `.venv` 或系统 Python。
- Excel 导入在 `QtConcurrent` 线程中仍为同步文件 IO，但不再启动外部进程。
- ADR 0005 废止；ADR 0001 中“Python 辅助导入”表述由本 ADR 覆盖。
