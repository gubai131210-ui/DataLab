# 计数型控制图对照样例

这三组数据用于 DataLab 与 Minitab 的手工对照。它们不是随机生成的“通过证明”，
而是用于核对中心线、控制限、绘制值和 Test 1 标记的固定输入。

## P / NP 图：`p_np_defectives.csv`

### Minitab

1. 打开 CSV。
2. 选择 `Stat > Control Charts > Attributes Charts > P`，不合格品列选
   `Defectives`，检验数列选 `Inspected`。
3. 再选择 `NP`，使用相同两列。
4. 默认只启用 Test 1，比较 `pbar` 或 `npbar`、每个子组的 LCL/UCL 和红色异常点。

### DataLab

选择“P 图”或“NP 图”，不合格品数列为 `Defectives`，检验数列为 `Inspected`。
对照输出表中的子组、绘制值、中心线、LCL、UCL 和 Test 1 状态。

## C 图：`c_defects.csv`

### Minitab

选择 `Stat > Control Charts > Attributes Charts > C`，缺陷数列选 `Defects`。
每个子组单位数固定为 10。比较 `cbar`、LCL/UCL 和 Test 1。

### DataLab

选择“C 图”，缺陷数列为 `Defects`，固定单位数填 `10`。

## U 图：`u_defects.csv`

### Minitab

选择 `Stat > Control Charts > Attributes Charts > U`，缺陷数列选 `Defects`，
单位数列选 `Units`。比较 `ubar`、随单位数变化的 LCL/UCL 和 Test 1。

### DataLab

选择“U 图”，缺陷数列为 `Defects`，单位数列为 `Units`。

## 关键计算口径

- P：`p_i = Defectives_i / Inspected_i`，`pbar = sum(Defectives) / sum(Inspected)`。
- NP：`np_i = Defectives_i`，中心线为 `Inspected_i * pbar`。
- C：`cbar = mean(Defects)`，单位数必须固定。
- U：`u_i = Defects_i / Units_i`，`ubar = sum(Defects) / sum(Units)`。
- 四种图默认使用 3σ Test 1，控制限分别截断到合法的比例或非负范围。
