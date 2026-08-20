# Gage By Part / Operator×Part 图（Crossed + Nested By Part）

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Crossed Gage 全部统计与图 | [All statistics and graphs for Crossed Gage R&R Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/crossed-gage-r-r-study/interpret-the-results/all-statistics-and-graphs/) | 2026-08-20 |
| Nested Gage 图 | [Graphs for Nested Gage R&R Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/nested-gage-r-r-study/interpret-the-results/all-statistics-and-graphs/graphs/) | 2026-08-20 |
| 关键结果解读 | [Interpret the key results for Crossed Gage R&R Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/crossed-gage-r-r-study/interpret-the-results/key-results/) | 2026-08-20 |
| AIAG MSA 单元格均值 | [NIST/SEMATECH e-Handbook 2.4.5](https://www.itl.nist.gov/div898/handbook/ppc/section4/ppc45.htm) | 2026-08-20 |

ANOVA 表、方差分量、ndc、负方差截断 **不改**。本轮只补诊断图。

## 2. By Part 图

Minitab：按零件排列全部测量；点为重复测量，圆叉为零件均值，连线连接各零件均值。同一零件上点靠得近表示重复性较小。

DataLab 合同：

```text
对 complete-case 行 i：测量 y_i、零件 p_i、source_row_i
零件均值 ȳ_p = mean{ y_i : p_i = p }
图：散点 (零件序号, y_i)，另加系列「零件均值」连线
每个点保留 source_row；均值点用该零件首次出现的 source_row
```

**Crossed 与 Nested 共用**上述合同。平衡且重复次数 ≥ 2 才出图。不平衡或 replicate<2：只诊断，不出 By Part（Nested 亦不出 Operator×Part）。缺失零件/操作者/`*` 跳过。

## 3. Operator×Part 交互图

Minitab：每个操作者一条线，连接该操作者在各零件上的**单元格均值**。这是 ANOVA 中 `Operator * Part` F 检验的图示。平行/重合表示操作者表现相近；交叉或不平行提示交互。

```text
cell(oper, part) = mean{ y_i : operator_i = oper ∧ part_i = part }
X = 零件序号（与 By Part 同一顺序）
系列 = 操作者；Y = cell 均值
source_row = 该单元格首次观测行
```

解释只读 `MsaFacts.interaction_p_value`：陈述是否提示交互，**不写**量具通过/不通过。交互是否 pooled 不因本图改变。

## 4. 范围边界

- Nested Gage：**已纳入** By Part（复用 `append_gage_by_part_plot`）；**不做** Operator×Part 交互图。
- Crossed Gage：By Part + Operator×Part 均已接入。
- 仍不做：改 ndc / 截断 / 自动 pooled；Weighted Kappa。
