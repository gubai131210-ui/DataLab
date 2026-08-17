# 柏拉图（Pareto Chart）验证数据包

算法 ID：`pareto`  
目录：`samples/capability/pareto/`

本目录按项目 skill `data_verification` 组织，用于把 DataLab 的柏拉图结果与 Minitab 对照。**在收到你的 Minitab / DataLab 数值对比前，不把该算法标记为已通过验证。**

## 三套数据

| 角色 | 目录 | 文件 | 布局 | 用途 |
|---|---|---|---|---|
| 官方主参考 | `official_primary/` | `data.csv` | 汇总：`Defect` + `Count` | 服装缺陷计数表 |
| 备选布局 | `alternate_layout/` | `data.csv` | 原始类别列：`Flaws` | 油漆缺陷逐条观测 |
| 边缘案例 | `edge_case/` | `data.csv` | 汇总 + 缺失 `*` + 稀有类别 | 测缺失跳过与 `Other=90` |

原始 `.MWX` 保留在 `raw/`。

## 快速操作

先读：

1. [`sources.md`](sources.md) — 来源与 SHA-256  
2. [`datalab_steps.md`](datalab_steps.md) — DataLab 统一操作  
3. 各子目录的 `minitab_steps.md`  
4. [`verification_report.md`](verification_report.md) — 把两边结果填进表

手算核对可用 [`expected/`](expected/) 中的占位 JSON（不是 Minitab 实测，仅便于对照）。
