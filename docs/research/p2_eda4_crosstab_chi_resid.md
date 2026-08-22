# C5 EDA 四图 · D2 交叉表/Tally · D3 卡方残差深化

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。

## 0. 锁定与禁止偷懒

| ID | 命令/改动 | 交付 |
|---|---|---|
| C5 | `eda_4plot` | 单页四图：run sequence、lag-1、histogram、normal probability；`EdaPlotFacts.kind=eda_4plot` |
| D2 | `cross_tabulation` | 独立交叉表：观察频数 + 行%/列%/合计%；**不强制**卡方；`CrossTabFacts` |
| D3 | 深化 `chi_square` | 增：行%/列%/合计% 表；调整残差热图；`ChiSquareFacts` 增 max_|adj residual| 与贡献最大单元格；**不改** `chi_square_gof` |

**禁止偷懒：**

- 禁止 mosaic 用错误卡方图糊弄（本轮不做 mosaic）  
- 禁止交叉表破坏/替换现有 `chi_square` 命令  
- 禁止改动 `chi_square` 与 `chi_square_gof` 边界  
- 禁止 EDA 四图写成「过程已证明受控 / 分布已正态」  
- 禁止假 golden  

---

## 1. C5 NIST 4-Plot

| 来源 | URL | 访问 |
|---|---|---|
| NIST 4-Plot | https://www.itl.nist.gov/div898/handbook/eda/section3/eda3332.htm | 2026-08-21 |

四图（同页四 `PlotSpec`）：

1. Run sequence：\(i\) vs \(Y_i\)（`time_series`/`scatter`）  
2. Lag-1：\(Y_{i-1}\) vs \(Y_i\)（`scatter`）  
3. Histogram（现有 `histogram`）  
4. Normal probability（现有 `normal_probability_plot`）  

解释：四图用于检查位置/散布/随机性/分布形态假设；**不是**过程受控证明。

---

## 2. D2 交叉表 / Tally

| 来源 | URL | 访问 |
|---|---|---|
| Minitab Cross Tabulation tabulated stats | https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/how-to/cross-tabulation-and-chi-square/interpret-the-results/all-statistics-and-graphs/tabulated-statistics/ | 2026-08-21 |

- 输入：行分类 + 列分类（complete-case）  
- 输出：观察频数表（含边际合计）；可选显示行%/列%/合计%（默认全出）  
- **不做**卡方检验（用户要检验用 `chi_square`）  
- `CrossTabFacts`：rows、cols、n、missing  

---

## 3. D3 卡方残差深化（现有 `chi_square`）

已有：Pearson/LR、单元格 Observed/Expected/Raw/Std/Adj/Contribution、观察频数热图。

本轮加深：

1. 百分比表：行% / 列% / 合计%（各一张或合并说明）  
2. **调整残差热图**（`PlotKind::heatmap`，标题标明 Adjusted Residual）  
3. Facts：`max_abs_adjusted_residual`、`largest_contribution_cell`（行×列标签）  
4. 解读：指出 |Adj residual| 最大单元格仅为偏离独立假设的描述，不是因果  

公式（已实现，文档对齐 Minitab）：

\[
E_{ij}=n_{i+}n_{+j}/n,\quad
r_{ij}=O_{ij}-E_{ij},\quad
\frac{r_{ij}}{\sqrt{E_{ij}}},\quad
\frac{r_{ij}}{\sqrt{E_{ij}(1-n_{i+}/n)(1-n_{+j}/n)}}
\]

---

## 4. 测试 / 验收

见 brief §5s。
