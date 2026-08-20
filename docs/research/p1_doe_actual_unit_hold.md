# P1 DOE 等值线/曲面：实际单位 hold

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab Contour Plot | [Create Contour Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/contour-plot/create-a-contour-plot/) | 2026-08-20 |
| Minitab Surface Plot | [Create Surface Plot](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/surface-plot/create-a-surface-plot/) | 2026-08-20 |
| DataLab 现有 | [`p1_doe_contour_factor_hold.md`](p1_doe_contour_factor_hold.md)；`evaluate_coded_grid` | 2026-08-20 |

## 2. 现状与缺口

现网：X/Y 轴可选；其余因子 **编码 hold = 0**。  
缺口：无法按 Minitab「Hold values」用 **实际单位** 固定非轴因子。

## 3. 产品合同

- 保留 `contour_x_factor` / `contour_y_factor`；缺省仍为前两因子。  
- 新增 UI 行：`hold`，格式 **`因子名=实际值;因子名=实际值`**；空 = 全部非轴因子编码 0（兼容）。  
- 配置：`DoeConfiguration::contour_hold_actual`（名→实际值字符串）。  
- X/Y 轴因子若出现在 hold 中 → 忽略该条目并诊断 `hold_ignored_axis_factor`。

### 实际 → 编码

对因子 \(i\)，水平低 \(L\)、高 \(H\)：

1. **数值水平**（\(L,H\) 与 hold 值均可 `parse_numeric_cell`）：  
   `coded = 2*(x−L)/(H−L) − 1`；越界诊断 `hold_out_of_range`，**仍求值并 clamp 到 [−1,1]**。  
2. **非数值水平**：hold 文本精确匹配 \(L\) → −1，匹配 \(H\) → +1；否则 `invalid_hold_value`，该因子 **回退 0**。  
3. \(L=H\)（数值）→ `invalid_hold_levels`，回退 0。

### Facts / 解释

- `held_factor_names`、`held_actual_values`（与 names 等长；空 hold 时 names 仍列非轴因子，actual 可为空串表示编码 0）。  
- 解释：有实际 hold 时写「hold 实际单位 …（编码 …）」；全空仍写 hold=0。

## 4. 明确不做

- 可旋转 3D  
- 改 Pareto / 立方 / 残差 4 图 / 优化器 D  
- 假网格 golden  

## 5. 测试策略

`# source: formula_reference`：3 因子 A×C 轴，B 数值 hold 中点 → 编码≈0 与空 hold 网格接近；B 持 high → 编码≈1 与 hold=0 网格不同；非法 hold 诊断并回退。
