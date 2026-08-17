# 测量系统分析样例（Crossed Gage R&R）

## gage_rr_crossed.csv

平衡交叉设计：

- 5 个零件：`P1`–`P5`
- 2 个操作员：`A`、`B`
- 每个零件×操作员组合重复测量 3 次

| 列 | 角色 |
|---|---|
| `Measurement` | 测量值 |
| `Part` | 零件 |
| `Operator` | 操作员 |

### DataLab 操作

1. **文件 → 打开 / 导入** `gage_rr_crossed.csv`
2. 菜单：**质量工具 → Crossed Gage R&R**
3. 弹窗设置：
   - 测量值：`Measurement`
   - 零件：`Part`
   - 操作员：`Operator`
   - LSL（可选）：`9.5`
   - USL（可选）：`12.5`

填写 LSL/USL 后会计算 `%Tolerance`；不填则该列显示为 0。

### 输出对照

- **Gage R&R 方差分析**：Part / Operator / Part*Operator / Repeatability
- **方差分量**：VarComp、StdDev、%Contribution、Study Var、%Study Var、%Tolerance
- **摘要**：零件数、操作员数、重复次数、ndc

### 数据契约注意

- 每个零件×操作员组合重复次数必须一致。
- 至少 2 个零件、2 个操作员、每组合至少 2 次重复。
- 缺失组合或非平衡设计会返回诊断错误，不会强行计算。

Minitab 对照：`Stat > Quality Tools > Gage Study > Gage R&R Study (Crossed)`，方法选 ANOVA。

## msa_type1.csv

这是 Type 1 Gage 的最小样例：

| 列 | 角色 |
|---|---|
| `Measurement` | 重复测量值 |
| `Reference` | 参考值（Bias/Linearity 可用） |
| `Time` | 测量顺序 |

菜单：**质量工具 → MSA Type 1 / Bias / Stability**。模式可填写：
`type1`、`bias_linearity` 或 `stability`。Type 1 还需要填写参考值；公差宽度可选。

## Reliability 数据约定

可靠性分析使用寿命列和失效指示列，失效指示 `1` 表示失效，`0` 表示右删失。
菜单：**统计 → 可靠性分析（Kaplan-Meier / Weibull）**，模型可选
`kaplan_meier`、`weibull` 或 `exponential`。

## t 功效与样本量

菜单：**统计 → t 功效与样本量**。样本量模式使用
`one_sample_sample_size` 或 `two_sample_sample_size`；功效模式使用
`one_sample_power` 或 `two_sample_power`。
