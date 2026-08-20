# ADR 0005：Python 导入脚本随应用运行时提供

- 状态：已废止（2026-08-20，见 ADR 0006）
- 日期：2026-08-17

## 原决策

Excel 导入脚本作为 Qt 资源随应用发布，运行时释放到临时目录后交给 Python 解释器执行。

## 废止原因

已由 `ExcelTableImporter`（C++ + zlib + Qt XML）替代；应用不再需要 Python 运行时。
