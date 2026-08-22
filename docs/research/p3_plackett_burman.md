# P3：Plackett–Burman 设计生成

> 研究日期：2026-08-21 · 访问 2026-08-21（UTC+8）  
> Track H；配既有 `generate_2_level_factorial`；`formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `doe_plackett_burman` | PB 筛选设计矩阵；运行表；`PlackettBurmanFacts` |

## 来源

| URL | 访问 |
|---|---|
| https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-21 |
| https://www.itl.nist.gov/div898/handbook/pri/section3/pri334.htm | 2026-08-21 |

## 产品

- 因子数 `k`（2…N−1）；运行数取最小 PB `N=4m ≥ k+1`（本轮支持 N∈{8,12,16,20,24}）。  
- 循环生成 ±1 矩阵；可选中心点与随机化。  
- **不做：** CCD/BBD 生成（仍 ❌）；DSD；分析拟合（沿用既有 DOE 分析路径）。
