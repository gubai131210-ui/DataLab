# P6：Taguchi 静态分析（S/N + 响应表）

> 研究日期：2026-08-25 · 访问 2026-08-25（UTC+8）  
> Wave-6 W6-1；`taguchi_analyze`；独立于 `taguchi_orthogonal_design`；非动态 Taguchi、非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `taguchi_analyze` | 静态；≥2 种 S/N（larger / smaller，+ nominal）；Means/S/N 响应表 + Delta/Rank；主效应图；complete-case |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/taguchi/analyze-taguchi-design/methods-and-formulas/methods-and-formulas/ | 2026-08-25 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/supporting-topics/taguchi-designs/what-is-the-signal-to-noise-ratio/ | 2026-08-25 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/taguchi/analyze-taguchi-design/interpret-the-results/all-statistics-and-graphs/response-table/ | 2026-08-25 |

## 表形

- Response Table for Means（Level × Factor；Delta；Rank）
- Response Table for Signal to Noise Ratios（同上）
- Diagnostics（缺失水平 / 行对齐）
- Main Effects PlotSpec（Means 与/或 S/N）

## 公式（# source: formula_reference）

对每个内阵组合（控制因子水平组合），用外阵重复响应 \(Y_1,\ldots,Y_n\)：

**Larger is better**

\[
\mathrm{S/N} = -10\log_{10}\left(\frac{1}{n}\sum_{i=1}^{n}\frac{1}{Y_i^2}\right)
\]

**Smaller is better**

\[
\mathrm{S/N} = -10\log_{10}\left(\frac{1}{n}\sum_{i=1}^{n} Y_i^2\right)
\]

**Nominal is best (II)**（本实现选用；方差型）

\[
\mathrm{S/N} = -10\log_{10}(s^2)
\]

其中 \(s\) 为该组合响应的样本标准差（\(n\ge 2\)）。

**Means**：\(\bar Y = n^{-1}\sum Y_i\)。

**响应表**：对每个因子、每个水平，平均该水平下各运行的特性（Mean 或 S/N）；  
\(\mathrm{Delta} = \max(\text{level avg}) - \min(\text{level avg})\)；  
Rank：Delta 从大到小，最大为 1。

## UI 分页

1. 列选择（因子 + 响应）  
2. 方法（S/N 类型）  
3. 方法说明（公式只读）  
4. 预览确认  

禁止在设计生成对话框加勾选交差。

## 明确不做

- 动态 Taguchi、噪声因子全交叉全量、Minitab 数值 golden、宣称「已优化/合格」
