# 过程能力分析示例数据

来源：Minitab Data Set Library（官方验证数据集，已转换为 UTF-8 CSV）。

官方说明：

- [Piston ring diameters](https://support.minitab.com/en-us/datasets/capability-data-sets/piston-ring-diameters/)
- [Cable wire diameters](https://support.minitab.com/en-us/datasets/capability-data-sets/cable-wire-diameters/)
- [Connector pin lengths](https://support.minitab.com/en-us/datasets/capability-data-sets/connector-pin-lengths/)

## 推荐优先用：PistonRingDiameter.csv

Montgomery《Introduction to Statistical Quality Control》活塞环直径数据。

| 项目 | 值 |
|---|---|
| 列名 | `Diameter` |
| 样本量 | 125（25 个子组 × 每组 5 个） |
| LSL | `73.95` |
| USL | `74.05` |
| Target | `74.00` |
| 子组大小 | `5` |

### 在 DataLab 中怎么测

1. 文件 → 导入数据 → 选择本目录下的 `PistonRingDiameter.csv`
2. 质量工具 → 正态过程能力，或 过程能力 Sixpack
3. 变量选 `Diameter`
4. 子组大小填 `5`
5. LSL=`73.95`，USL=`74.05`，Target=`74.00`

预期：过程大致居中，规格内；官方示例中 Cpk / Ppk / Cpm 通常都大于 1.33。

## 备选

### CableWires.csv

金属公司电缆线直径：每小时抽连续 5 根测量，目标规格 `0.55 ± 0.05 cm`。

| 项目 | 值 |
|---|---|
| 列名 | `Diameter`（测量）、`Subgroup`（子组号） |
| 行数 | 100（20 子组 × 设计每组 5 个） |
| 有效观测 | 96（另有 4 个 `*` 缺失） |
| LSL / USL | `0.50` / `0.60` |
| Target | `0.55`（工程目标；能力分析可不填） |
| 子组大小 | `5` |

**缺失值：** 工作表中用 `*` 表示。DataLab 与 Minitab 都会跳过这些单元格，**不要**把它们当成数字。

#### Minitab

1. File → Open Worksheet → 选 `CableWires.csv`（或打开原始 `.MWX`）
2. Stat → Quality Tools → Capability Analysis → Normal…
3. **Data are arranged as** → Single column：`Diameter`
4. **Subgroup size** 填 `5`（或用 Subgroup sizes：列 `Subgroup`）
5. Lower / Upper spec：`0.50` / `0.60`
6. 可选：Capability Sixpack → Normal…，同样参数

#### DataLab

1. 文件 → 导入数据 → `samples/capability/CableWires.csv`
2. 质量工具 → 正态过程能力，或 过程能力 Sixpack
3. 变量选 `Diameter`
4. 子组大小填 `5`
5. LSL=`0.50`，USL=`0.60`

对照时重点看：Sample N≈96、Within/Overall StDev、Cp/Cpk/Pp/Ppk、Observed/Expected PPM。

### PinLength.csv

- 列：`Length`、`Machine`
- 规格：LSL=`13`，USL=`25`
- 适合按机器分组看能力

原始 `.MWX` 仍保留在：

`tests/fixtures/minitab/raw/`

## 柏拉图验证数据

详见独立目录：[`pareto/`](pareto/README.md)（ClothingDefect / PaintFlaws / edge_case）。
