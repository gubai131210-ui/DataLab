# 双因素 ANOVA 样例

文件：`factorial_2x2.csv`  
完整路径：`d:\QT_CppPrograms\DataLab\samples\anova_factorial\factorial_2x2.csv`

## 列说明

| 列名 | 角色 | 说明 |
|---|---|---|
| `Response` | 响应变量 | 数值 |
| `FactorA` | 因子 A | 分类水平 `A1` / `A2` |
| `FactorB` | 因子 B | 分类水平 `B1` / `B2` |

这是 2×2 交叉设计，每个因子组合都有重复观测。

## 导入与操作

1. 打开 DataLab。
2. **文件 → 打开 / 导入**，选择本目录 `factorial_2x2.csv`。
3. 确认工作表出现三列：`Response`、`FactorA`、`FactorB`。
4. 菜单：**统计 → 双因素 ANOVA**
5. 弹窗设置：
   - 响应变量：`Response`
   - 因子 A：`FactorA`
   - 因子 B：`FactorB`
   - 因子编码：输入 `reference`（也可用 `effect`）
6. 确定后查看输出页。

## 输出对照

| 区块 | 看什么 |
|---|---|
| ANOVA 表 | Seq SS、Adj SS、DF、MS、F、P-Value；来源含 Factor A、Factor B、A*B、Error |
| 因子 A 均值 | 各水平 N 与均值 |
| 因子 B 均值 | 各水平 N 与均值 |
| 交互均值 | 每个 A×B 组合的 N 与均值 |
| 诊断 | 缺失行、非平衡、空单元、秩亏提示 |

## 数据契约

- `Response` 必须是有限数值。
- 两个因子都至少要有 2 个水平。
- 同一行必须同时有响应、因子 A、因子 B；否则按 complete-case 排除。
- 因子组合重复数不一致时会出现非平衡警告，但仍可能给出 Adj SS。

## Minitab 对照

`Stat > ANOVA > General Linear Model > Fit General Linear Model`

- Responses：`Response`
- Factors：`FactorA` `FactorB`
- Model：包含主效应和交互项
