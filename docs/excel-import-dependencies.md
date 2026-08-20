# Excel 导入依赖说明

DataLab 应用运行时通过 `ExcelTableImporter` 读取 `.xlsx`，不依赖 Python。

## 构建依赖

| 依赖 | 用途 | 获取方式 |
|------|------|----------|
| Qt 6 Core | XML 解析、文件 IO | 项目已有 |
| miniz | ZIP/DEFLATE 解压（OOXML） | vendored：`third_party/miniz/` |

CMake 通过静态库 `datalab_miniz` 链接，无需系统 zlib 或 Python。

## 支持范围

- `.xlsx`（Office Open XML）：第一个工作表
- 共享字符串、inline string、数字、布尔、公式缓存值
- 空单元格 → 空字符串（与原先 pandas 桥一致）
- **不支持** 老式 `.xls`（BIFF）

## 测试 fixture

- 生成：`python tools/generate_import_fixture.py`
- 路径：`tests/fixtures/import/basic_contract.xlsx`

## 独立 Python 工具

`requirements.txt` 仍服务于 `tools/` 下开发脚本（如 Minitab golden 对照），**不是**应用运行时依赖。
