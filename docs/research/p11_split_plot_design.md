# P11：2-Level Split-Plot Design（窄化）

> 研究日期：2026-09-01 · 访问 2026-09-01（UTC+8）  
> Wave-11 W11-4；`split_plot_design`；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `split_plot_design` | 2～4 因子、1 HTC；设计矩阵；Whole plot 列 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/create-2-level-split-plot/before-you-start/overview/ | 2026-09-01 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/create-2-level-split-plot/create-the-design/specify-the-design/ | 2026-09-01 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/create-2-level-split-plot/before-you-start/example/ | 2026-09-01 |

## 表形

- Design Summary（因子数、whole plots、runs、复制）
- Design Table（Standard Order, Run Order, Whole Plots, 因子列, Point Type）
- Alias table（若 fractional）

## 契约

输出列命名与 `doe_factorial` 一致；分析走 W9 `split_plot_analyze`（响应列用户后填）。

## UI 分页

1. 因子数 + 名称/水平  
2. 指定 HTC + 选设计（runs/whole plots）  
3. whole-plot 复制 + 随机化  
4. 预览矩阵
