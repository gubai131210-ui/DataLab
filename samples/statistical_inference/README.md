# 统计推断对照样例

## correlation.csv

DataLab 菜单：**统计 → 相关分析**。选择 `X`、`Y`、`Monotonic`。
方法可输入 `pearson` 或 `spearman`，默认置信水平为 95%。

Minitab 对照路径：`Stat > Basic Statistics > Correlation`。

## t_tests.csv

- **单样本 t**：选择 `Measurement`，假设均值输入 `10`。
- **双样本 t**：选择 `SampleA` 和 `SampleB`，默认使用 Welch 方法。

Minitab 对照路径：

- `Stat > Basic Statistics > 1-Sample t`
- `Stat > Basic Statistics > 2-Sample t`

## anova.csv

DataLab 菜单：**统计 → 单因素 ANOVA**，响应变量选择 `Response`，因子/分组列
选择 `Group`。

Minitab 对照路径：`Stat > ANOVA > One-Way`。

重点对照 Mean、StDev、SE、DF、P-Value、置信区间以及 ANOVA 表中的
SS、MS、F、P-Value。

## regression.csv

DataLab 菜单：**统计 → 线性回归**。第一列 `Response` 为响应变量，
`Temperature` 和 `Pressure` 为预测变量。

配对 t 检验从同一行的两列构造差值；列联表卡方需要两列分类变量。
两比例检验使用四个汇总计数列，每列选择一行有效的非负整数计数。

## 回归残差诊断（推荐）

更适合检查本批诊断增强的样例：

- `samples/regression_diagnostics/regression_diagnostics.csv`
- 说明：`samples/regression_diagnostics/README.md`

重点查看 Durbin-Watson、删除学生化残差、Cook's D、DFITS 和三张残差图。

## 双因素 ANOVA

见：

- `samples/anova_factorial/factorial_2x2.csv`
- `samples/anova_factorial/README.md`

## 其他批次样例

- `samples/nonparametric/`
- `samples/control_charts/time_weighted/`
- `samples/measurement_system/`
- `samples/time_series/`
- `samples/time_series/arima/`

总索引：`samples/README.md`  
本批逐步说明：`samples/本批使用说明.md`
