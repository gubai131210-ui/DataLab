# ADR 0005：Python 导入脚本随应用运行时提供

- 状态：已接受
- 日期：2026-08-17

## 决策

Excel 导入脚本作为 Qt 资源随应用发布，运行时释放到临时目录后交给 Python 解释器执行。生产代码不依赖编译机的源码路径宏。

Python 与 C++ 之间的 JSON 协议带有 `schema_version`。导入工作在 QtConcurrent 线程中执行，UI 线程只接收完成信号并更新模型。
