# Minitab 官方验证数据 + G-Trust 输入登记

这些原始文件来自 Minitab 官方 Data Set Library：

<https://support.minitab.com/en-us/datasets/>

下载日期：2026-08-13。

**证据边界**：下列 CSV 仅作**输入**。G-Trust 数值基线来自 `scripts/g_trust_*_reference.py` → `expected/*_ref_golden.tsv`（`reference_implementation`），**不是** vendor_oracle。

## 文件与用途

| 原始文件 | 转换文件 | 用途 | 关键配置 |
|---|---|---|---|
| `CrankshaftMovement.MWX` | `converted/CrankshaftMovement.csv` | 汽车发动机曲轴，Xbar-R | 每日子组，测量列 `A to B Distance` |
| `CamshaftLength.MWX` | `converted/CamshaftLength.csv` | 汽车凸轮轴，Xbar-R | `Machine 1/2/3`，子组列 `Subgroup ID` |
| `PistonRingDiameter.MWX` | `converted/PistonRingDiameter.csv` | 正态过程能力 | 每 5 行一个子组，LSL=73.95，USL=74.05，Target=74.00 |
| `PinLength.MWX` | `converted/PinLength.csv` | 多机器过程能力 | `Length` 按 `Machine` 分组，LSL=13，USL=25 |
| `UnansweredCalls.MWX` | `converted/UnansweredCalls.csv` | P 图 | `Unanswered Calls` 为不合格品数，`Total Calls` 为检验总数 |
| `CableWires.MWX` | `converted/CableWires.csv` | 子组、缺失值和过程能力 | `Diameter`，`Subgroup`，规格 0.55 ± 0.05 cm |

## G-Trust 锁表 10 命令 — 输入登记（预留 / 已填）

| command_id | 输入路径 | 登记说明 | Wave |
|---|---|---|---|
| `imr` | `converted/imr_ref_golden_input.csv` | 由 `expected/imr_synthetic.csv` 同源序列正规化为单列 `Value`（合成；非官方库） | Wave-1 |
| `xbar_r` | `converted/CamshaftLength.csv` | 官方公开集；`Machine 1` 按 `Subgroup ID`（计划备选）。`CrankshaftMovement` 按 Date 子组 n 不等（5/10），与 domain 等 n 约束冲突，故换用 | Wave-1 |
| `p_chart` | `converted/UnansweredCalls.csv` | 官方公开集；`Unanswered Calls` / `Total Calls` | Wave-1 |
| `capability` | `converted/PistonRingDiameter.csv` | LSL=73.95, USL=74.05, Target=74.00；subgroup_size=5；within=`R̄ / d2(n)` | Wave-2 |
| `capability_sixpack` | `converted/PistonRingDiameter.csv` | 同 capability；图契约 min_plots=5 / expected_plots=6 | Wave-2 |
| `between_within_capability` | `converted/cap_between_within.csv` | 自 `tools/learning_data/csv/cap_between_within.csv` 复制（**非**官方库）；列 `子组`/`厚度_um`；规格钉死 LSL=95, USL=105, Target=100（学习 overlay `between_within_capability.json`） | Wave-2 |
| `gage_rr` | `converted/gage_rr_crossed.csv` | 自 `samples/measurement_system/gage_rr_crossed.csv` 复制（**非**官方库）；Part/Operator/Measurement；tolerance=0 | Wave-3 |
| `two_sample_t` | `converted/two_sample_t_ref_golden_input.csv` | 合成宽表 Sample1/Sample2；固定表 2026-09-05；Welch | Wave-3 |
| `normality_test` | `converted/PistonRingDiameter.csv` | Diameter；method=anderson_darling | Wave-3 |
| `one_way_anova` | `converted/one_way_anova_ref_golden_input.csv` | 合成 Factor/Response；固定平衡 3×5（2026-09-05） | Wave-3 |

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
