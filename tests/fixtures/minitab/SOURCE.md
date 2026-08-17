# Minitab 官方验证数据

这些原始文件来自 Minitab 官方 Data Set Library：

<https://support.minitab.com/en-us/datasets/>

下载日期：2026-08-13。

## 文件与用途

| 原始文件 | 转换文件 | 用途 | 关键配置 |
|---|---|---|---|
| `CrankshaftMovement.MWX` | `converted/CrankshaftMovement.csv` | 汽车发动机曲轴，Xbar-R | 每日子组，测量列 `A to B Distance` |
| `CamshaftLength.MWX` | `converted/CamshaftLength.csv` | 汽车凸轮轴，Xbar-R | `Machine 1/2/3`，子组列 `Subgroup ID` |
| `PistonRingDiameter.MWX` | `converted/PistonRingDiameter.csv` | 正态过程能力 | 每 5 行一个子组，LSL=73.95，USL=74.05，Target=74.00 |
| `PinLength.MWX` | `converted/PinLength.csv` | 多机器过程能力 | `Length` 按 `Machine` 分组，LSL=13，USL=25 |
| `UnansweredCalls.MWX` | `converted/UnansweredCalls.csv` | P 图 | `Unanswered Calls` 为不合格品数，`Total Calls` 为检验总数 |
| `CableWires.MWX` | `converted/CableWires.csv` | 子组、缺失值和过程能力 | `Diameter`，`Subgroup`，规格 0.55 ± 0.05 cm |

## 官方页面

- [Crankshaft movement](https://support.minitab.com/en-us/datasets/control-charts-data-sets/crankshaft-movement-data/)
- [Camshaft length](https://support.minitab.com/en-us/datasets/control-charts-data-sets/camshaft-length-data/)
- [Piston ring diameter](https://support.minitab.com/en-us/datasets/capability-data-sets/piston-ring-diameters/)
- [Connector pin lengths](https://support.minitab.com/en-us/datasets/capability-data-sets/connector-pin-lengths/)
- [Unanswered calls](https://support.minitab.com/en-us/datasets/control-charts-data-sets/unanswered-calls-data/)
- [Cable wire diameters](https://support.minitab.com/en-us/datasets/capability-data-sets/cable-wire-diameters/)

## 转换说明

`.MWX` 是 Minitab 工作表容器。DataLab 不将其当作通用输入格式，而是保留原始文件，并使用 `tools/mwx_to_csv.py` 生成 UTF-8 CSV。转换脚本只读取工作表 JSON，保持原始列顺序和缺失值标记 `*`。

原始文件和转换文件仅用于本项目本地验证；使用时应遵守 Minitab 官方数据集页面的条款和版权说明。
