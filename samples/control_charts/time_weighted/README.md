# 时间加权控制图样例（EWMA / CUSUM）

通用导入步骤：

1. **文件 → 打开 / 导入** 本目录 CSV。
2. 菜单进入 **控制图**，选择 EWMA 或 CUSUM。
3. 测量值列选择 `Value`（`Observation` 仅作序号参考，可不选）。

## ewma.csv

DataLab 菜单：**控制图 → EWMA 控制图**

推荐参数：

| 参数 | 建议值 | 说明 |
|---|---|---|
| 测量值 | `Value` | 个体观测序列 |
| Lambda | `0.2` | 默认平滑系数 |
| 控制限倍数 | `3` | 对应约 3σ |
| 历史均值 | 留空或 `10` | 留空则用样本均值 |

预期现象：

- 前半段大致围绕 10 附近。
- 中段均值上移后，EWMA 曲线逐步抬升并可能越出控制限。
- 输出包含 EWMA 序列、随时间收敛的 LCL/UCL 和信号点。

Minitab 对照：`Stat > Control Charts > Time-weighted Charts > EWMA`

## cusum.csv

DataLab 菜单：**控制图 → CUSUM 控制图**

推荐参数：

| 参数 | 建议值 | 说明 |
|---|---|---|
| 测量值 | `Value` | 个体观测序列 |
| 目标值 | `10` | 过程目标 μ0 |
| 过程 Sigma | `0.3` | 过程标准差估计 |
| 参考值 k | `0.5` | slack，常取 0.5σ |
| 决策间隔 h | `4` | 决策阈值倍数 |

预期现象：

- 中后段持续偏高时，上侧 CUSUM 累积并触发信号。
- 下侧 CUSUM 在偏高阶段接近 0。
- CUSUM 信号不等同于 Shewhart Test 1，请按上下侧信号解释。

Minitab 对照：`Stat > Control Charts > Time-weighted Charts > CUSUM`
