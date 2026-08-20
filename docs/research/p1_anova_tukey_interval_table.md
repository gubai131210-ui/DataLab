# P1 单因素 ANOVA Tukey 多重比较区间表形

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。数值公式本轮不变。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab One-Way ANOVA 多重比较 | [One-Way ANOVA methods / Comparisons](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/one-way-anova/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| Tukey 同时区间解释 | Minitab 同时置信区间列（Lower / Upper） | 2026-08-20 |
| DataLab 现有近似 | `tukey_multiple_comparisons`：conservative Šidák-t / studentized-range **approximation** | 2026-08-20 |

## 2. 现状与缺口

领域层 `TukeyComparison` 已有 `confidence_lower` / `confidence_upper`。  
服务层将二者压成单列 `同时置信区间` = `[L, U]`。

Minitab 学习点：区间表形通常分 **下限 / 上限** 两列，便于扫读与复制；并可配差值区间图。

## 3. 本轮变更（仅表形 + 可选图）

| 项 | 动作 |
|---|---|
| 「Tukey 同时比较」表 | 将 `同时置信区间` 拆为 **`下限`**、**`上限`** 两列 |
| 其余列 | Difference、SE、q、Adjusted P、显著（含 0 / 不含 0）、族置信水平、误差 DF、MSE、方法 — **保留** |
| 数值 | **不改** Šidák-t 近似算法与诊断码 |
| 图 | 追加「Tukey 差值同时区间」：`PlotKind::interval`，类别=成对标签，须=下限/上限，参考线 y=0（或 x=0 视渲染约定）；无 `source_row`（成对汇总） |
| Facts | 可选记录 `tukey_interval_columns=lower_upper`；`tukey_significant_pairs` 语义不变 |

## 4. 与组均值个体 CI 的区别

- 组均值表 `{CL}% CI`：pooled MSE **个体**区间，不是 Tukey 族同时校正。  
- Tukey 下限/上限：成对均值差的 **族同时**区间（本产品仍为近似）。

## 5. 明确不做

- 精确 Studentized Range 分位替换近似  
- Grouping Information 字母分组  
- 双因素 ANOVA Tukey  
- DOE 实际单位 hold  

## 6. 测试策略

服务层断言表头含 `下限`/`上限`；显著列仍按区间是否含 0；存在差值区间图；旧诊断 `tukey_studentized_range_approximation` 仍在。
