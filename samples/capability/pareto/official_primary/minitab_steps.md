# Minitab：ClothingDefect（汇总数据）

## 打开数据

1. **File → Open Worksheet…**
2. 选 `samples/capability/pareto/raw/ClothingDefect.MWX`  
   或导入 `official_primary/data.csv`（UTF-8）。

## 分析设置

1. **Stat → Quality Tools → Pareto Chart**
2. **Defects or attribute data in：** `Defect`
3. **Frequencies in：** `Count`（汇总计数列）
4. **BY variable：** 留空
5. **Combine remaining defects…：** **Do not combine**
6. 确认显示百分比与累计线（默认开启）

## 记录以下输出

| 指标 | 记什么 |
|---|---|
| 类别顺序 | 从高到低 |
| Count | 每类计数 |
| Percent | 每类百分比 |
| Cum % | 累计百分比 |
| 总缺陷数 | 所有 Count 之和 |

手算参考（非 Minitab 实测）：Total=480；Missing button 217 → 45.2083%；Cum% 到 Stitching errors 后约 68.5417%。
