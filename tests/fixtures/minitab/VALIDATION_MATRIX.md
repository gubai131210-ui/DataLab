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
| arima_trend | ARIMA 基础预测 | Period、Value，AICc，4 期预测，max_d=1 | `expected/arima_trend_golden.tsv`；当前最优 ARIMA(2,1,0)（CSS 近似，待 Minitab 导出核对） |
| regression.csv | 线性回归 | Response，Temperature，Pressure | `expected/regression_golden.tsv`；Seq/Adj SS、S、DW |

## Golden 文件与容差

| 文件 | 来源 | 容差 |
|---|---|---|
| `expected/regression_golden.tsv` | OLS 公式参考（`docs/statistical-methodology.md`）；**请用 Minitab 导出替换 `# source` 行** | 系数/SS：相对 ≤1e-4；DW：相对 ≤1e-4 |
| `expected/arima_trend_golden.tsv` | 镜像 `arima.cpp` 候选网格；**请用 Minitab Best ARIMA 导出替换** | AICc：绝对 ≤0.01；Forecast：绝对 ≤0.05 |
| `expected/EXPORT_GUIDE.md` | 导出格式说明 | — |

重新生成公式参考值（非 Minitab 猜测）：

```powershell
.\.venv\Scripts\python.exe tools\dump_minitab_golden_reference.py
```

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

自动化测试在 golden 文件缺失时 `QSKIP`；只有实际从 Minitab 导出的结果才应作为最终 golden 基准。

Johnson 变换、非正态 Z-score、对数正态可靠性、乘法 SARIMA CSS、三参数 Weibull、
Fleiss Kappa、Kendall W/τ、两参数指数、三参数对数正态、PCA 系数/T²Q、非参数 ties
与 Levene 中位数等方差的自动化测试是 **公式参考**（`# source: formula_reference`），
**不是** Minitab 导出。不要把它们登记为数值对齐。

## 本批黄金检查项

- 双因素 ANOVA：`DF`、`SS`、`MS`、`F`、`P-Value`、因子均值和交互均值。
- 回归诊断：`Durbin-Watson`、内部/删除学生化残差、Cook's D、DFITS 和 VIF；ANOVA `Seq SS` / `Adj SS`。
- ARIMA：候选模型名称、AIC/AICc/BIC、Forecast、Lower、Upper。
- 输出页面：长中文标题、11 列以上表格、诊断卡片和多图布局不发生文字重叠。
