# Minitab：edge_case（缺失 + Other=90）

## 打开数据

1. **File → Open Worksheet…**
2. 导入 `samples/capability/pareto/edge_case/data.csv`  
   （派生文件，无官方 MWX；见 `../sources.md`）

## 分析设置

1. **Stat → Quality Tools → Pareto Chart**
2. **Defects or attribute data in：** `Defect`
3. **Frequencies in：** `Count`
4. **Combine remaining defects into one category after this percent：** 勾选并填 **90**
5. 观察 Minitab 如何处理：
   - `Defect = *`
   - `Count = *`

## 记录以下输出

| 指标 | 记什么 |
|---|---|
| 有效总计数 | 期望 490（若 Minitab 不同，写明规则） |
| 保留的独立柱 | 名称与 Count |
| Other | 计数与 Cum% |
| 缺失处理 | * 行是否进入图/表 |

DataLab 期望（实现口径，待你与 Minitab 对照）：

- 跳过类别缺失、计数缺失。
- 保留到 Cum% 首次 **超过** 90% 的那一类，其后全部进 Other。
- 保留：Missing button(217), Stitching errors(112), Loose thread(67), Hemming errors(43), Fabric flaws(23)。
- Other = 18+4+3+2+1 = **28**。
