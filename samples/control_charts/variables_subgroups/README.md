# Xbar-R / Xbar-S 与正态性对照样例

## `fixed_subgroups.csv`

列 `Measurement` 是测量值，按每 5 行一个子组。

### Minitab

- `Stat > Control Charts > Variables Charts for Subgroups > Xbar-R`
- 测量列选 `Measurement`，子组大小填 `5`
- 再用 `Xbar-S` 重复一次
- 先检查 R/S 图，再解释 Xbar 图；对照 CL、LCL、UCL 和 Test 1。

### DataLab

- 选择“Xbar-R”或“Xbar-S”
- 测量值列选择 `Measurement`
- 子组大小填 `5`
- 不选择子组标识列

## `labelled_subgroups.csv`

列 `Subgroup` 是子组标识，`Measurement` 是测量值。标签首次出现顺序
决定子组顺序。

- Minitab：选择“所有观测值在一列”，测量列为 `Measurement`，子组大小选择
  `Subgroup` 列。
- DataLab：测量值列选择 `Measurement`，子组列选择 `Subgroup`。

## `normality_measurements.csv`

用于描述统计和正态性检验。

- Minitab：`Stat > Basic Statistics > Display Descriptive Statistics`，
  或 `Stat > Basic Statistics > Normality Test`。
- DataLab：分别选择“显示描述性统计”和“正态性检验”。

正态性重点对照 N、Mean、StDev、AD 和 P-Value。概率图 plotting position
需要保持一致：DataLab 默认使用 Blom 位置公式，Minitab 可在概率图选项中设置。
