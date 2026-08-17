# 验证矩阵

第一轮先验证数据完整性、字段映射和算法输入口径；第二轮在用户用 Minitab 打开原始文件并导出结果后，补充数值容差。

| 数据集 | DataLab 分析 | 配置 | 当前验证 |
|---|---|---|---|
| CrankshaftMovement | Xbar-R | A to B Distance，按 Date 分组 | CSV 行数、列名、125 行 |
| CamshaftLength | Xbar-R | Machine 1/2/3，Subgroup ID | CSV 行数、列名、100 行 |
| PistonRingDiameter | Cp/Cpk/Pp/Ppk | 每 5 行一个子组，73.95/74.05，Target 74.00 | CSV 行数、列名、规格记录 |
| PinLength | 分组能力分析 | Length 按 Machine 分组，13/25 | CSV 行数、列名 |
| UnansweredCalls | P 图 | Unanswered Calls / Total Calls | 21 行、两列计数 |
| CableWires | Xbar-R/能力分析 | Diameter、Subgroup，0.50/0.60 | 缺失值保留为 `*` |
| factorial_2x2 | 双因素 ANOVA | Response，FactorA，FactorB | Seq/Adj SS、交互项和秩亏诊断 |
| arima_trend | ARIMA 基础预测 | Period、Value，AICc，4 期预测 | 候选模型、AICc 和预测区间 |
| regression.csv | 线性回归 | Response，Temperature，Pressure | DW、学生化残差、Cook's D、DFITS |

## Minitab 对照记录格式

每个数据集后续应记录：

```text
Minitab 分析路径：
输入列：
子组列：
LSL：
USL：
Target：
缺失值选项：
异常点选项：
Minitab 结果：
DataLab 结果：
绝对误差：
相对误差：
```

当前仓库只自动验证文件转换和输入完整性，不虚构尚未从 Minitab 导出的统计结果。

## 本批黄金检查项

- 双因素 ANOVA：`DF`、`SS`、`MS`、`F`、`P-Value`、因子均值和交互均值。
- 回归诊断：`Durbin-Watson`、内部/删除学生化残差、Cook's D、DFITS 和 VIF。
- ARIMA：候选模型名称、AIC/AICc/BIC、Forecast、Lower、Upper。
- 输出页面：长中文标题、11 列以上表格、诊断卡片和多图布局不发生文字重叠。
