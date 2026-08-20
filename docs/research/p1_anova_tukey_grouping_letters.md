# P1 单因素 ANOVA Tukey Grouping 字母

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。数值临界值本轮不变。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab One-Way ANOVA Comparisons | [One-Way ANOVA methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/one-way-anova/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| Minitab Grouping Information | 表形：Level / N / Mean / Grouping | 2026-08-20 |
| Compact Letter Display | Piepho (2004) 风格贪心；本产品只用现有 pairwise `significant` | 2026-08-20 |
| DataLab 现有 | [`p1_anova_tukey_interval_table.md`](p1_anova_tukey_interval_table.md)；`tukey_multiple_comparisons` | 2026-08-20 |

## 2. 产品合同

- **不改** Šidák-t / studentized-range **approximation** 与成对 `significant`（区间是否含 0）。  
- 在显著矩阵上做 CLD：同字母 ⇒ 在本产品规则下 **不显著不同**。  
- 新表「Grouping Information」：水平、N、均值、Grouping（如 `A`、`AB`、`B`）。  
- 保留「Tukey 同时比较」下限/上限 + 差值区间图。  
- `AnovaFacts.tukey_grouping_available`；可选 `grouping_letter_count`。

## 3. CLD 伪代码（确定性）

输入：组标签、均值、样本量；成对 `(i,j)` 的 `significant`（来自已算 Tukey）。

```text
1. 将组按均值降序排序，得到序 index[0..k-1]。
2. 建 k×k 布尔矩阵 sig[i][j] = 两组成对显著（对称；对角 false）。
3. letters = 空列表；每个组的 letter_set 为空。
4. 对每个尚未「覆盖所有非显著邻居需求」的组（按均值序）：
   a. 找最小字母 L，使得：对所有已标 L 的组 g，当前组与 g 不显著；
      若不存在，分配新字母。
   b. 将 L 赋给当前组，并尽可能把 L 扩展到所有与当前组及已标 L 的组
      两两均不显著的尚未标注组（贪心向前扫均值序）。
5. 每个组的 Grouping = 其字母集合按字母序拼接（无分隔符）。
```

实现可等价为：对每个新字母，从最高均值未完全处理的组起，吸收所有与字母内每一成员均不显著的组。

## 4. 明确不做

- 精确 Studentized Range  
- TOST 均值比  
- 双因素 ANOVA grouping  
- 另算一套显著临界值  

## 5. 测试策略

`# source: formula_reference`：三组等均值共享同一字母；两组显著分离则字母不相交；Grouping 与 Tukey 显著列一致。
