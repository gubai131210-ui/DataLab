# Laney P' / U' 对照样例

## P' 图：`laney_p.csv`

### Minitab

1. 打开 CSV。
2. 选择 `Stat > Control Charts > Attributes Charts > Laney P′`。
3. 不合格品列选择 `Defectives`，检验数列选择 `Inspected`。
4. 对照 `Sigma Z`、P' 控制限、Test 1 和阶段标签。

### DataLab

菜单路径：**控制图 → Laney P' 图**

设置不合格品数列、检验数列和可选的 `Stage` 阶段列。
特殊原因测试可以输入 `1 2 3 4`；默认只启用 Test 1。

## U' 图：`laney_u.csv`

### Minitab

选择 `Stat > Control Charts > Attributes Charts > Laney U′`，缺陷数列选择
`Defects`，单位数列选择 `Units`。

### DataLab

菜单路径：**控制图 → Laney U' 图**

设置缺陷数列、单位数列和可选阶段列。

## 对照重点

- `Sigma Z = 1`：Laney 图与传统 P/U 图的控制限基本一致。
- `Sigma Z > 1`：过度离散，Laney 控制限更宽。
- `Sigma Z < 1`：欠离散，Laney 控制限更窄。
- 表格会列出每个子组的 Z、MR、控制限和 Test 1–4 状态。
