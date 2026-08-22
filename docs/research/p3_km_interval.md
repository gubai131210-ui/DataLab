# P3：左/区间删失 Kaplan–Meier（Turnbull）

> 研究日期：2026-08-21 · 访问 2026-08-21（UTC+8）  
> Track H；配既有右删失 `kaplan_meier`；`formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `km_interval` | 左/区间/右删失统一为区间；Turnbull NPMLE；`KmIntervalFacts` |

## 来源

| URL | 访问 |
|---|---|
| https://www.itl.nist.gov/div898/handbook/apr/section4/apr4.htm | 2026-08-21 |
| https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-21 |

## 产品

- 每行给出区间 `(L, R]`：  
  - 精确失效：`L=R=t`  
  - 右删失：`L=t, R=+∞`（用很大上界或标记）  
  - 左删失：`L=0 或 -∞, R=t`  
  - 区间删失：`L<R` 有限  
- Turnbull 自洽迭代估计质量点；输出生存阶梯与中位寿命（若可辨识）。  
- **不做：** 参数寿命模型；左截断（truncation）。
