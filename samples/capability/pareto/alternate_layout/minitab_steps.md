# Minitab：PaintFlaws（原始类别列）

## 打开数据

1. **File → Open Worksheet…**
2. 选 `samples/capability/pareto/raw/PaintFlaws.MWX`  
   或导入 `alternate_layout/data.csv`。

## 分析设置

1. **Stat → Quality Tools → Pareto Chart**
2. **Defects or attribute data in：** `Flaws`
3. **Frequencies in：** 留空（每行一次观测）
4. **BY variable：** 本轮先留空（先对照总表；若要按班次，另做一版填 `Shift`）
5. **Combine remaining defects…：** **Do not combine**

官方示例也曾用 BY=`Shift` 且 Other=95：  
https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/pareto-chart/before-you-start/example-of-a-pareto-chart-with-a-by-variable/

本轮主对照不做 BY，避免 DataLab 尚未提供 BY 多图时无法一对一比较。

## 记录以下输出

| 指标 | 记什么 |
|---|---|
| N / 总计数 | 应为 40 |
| 顺序 | Peel, Scratch, 然后两个 6 次类别 |
| Count / Percent / Cum % | 全表 |
| 同频顺序 | Other vs Smudge 谁在前 |

手算参考：Peel 15=37.5%；Scratch 13=32.5%；两个 6=15%。
