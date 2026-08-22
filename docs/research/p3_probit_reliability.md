# P3：Probit 可靠性（logit 剂量-响应）

> 研究日期：2026-08-22  
> 访问日期：2026-08-22（UTC+8）  
> Wave-3 W4；`formula_reference ≠ golden`。

## 0. 锁定与禁止偷懒

| 命令 | 交付 |
|---|---|
| `probit_reliability` | 二项比例 vs 应力/剂量；logit 链接；系数 + LD50；`ProbitReliabilityFacts` |

**禁止：** 菜单占位；无 LD50 却声称 probit 完整；把 logit 结果写成 Minitab probit golden；忽略 complete-case 对齐。

## 1. 权威来源

| 来源 | URL | 访问 |
|---|---|---|
| Minitab Support | https://support.minitab.com/ | 2026-08-22 |

## 2. 产品锁定

- 三列：事件数、试验数、应力/剂量；complete-case 对齐。  
- 模型：\(\mathrm{logit}(p)=\beta_0+\beta_1\cdot\mathrm{stress}\)；加权 IRLS。  
- 输出：系数表、拟合表、偏差/AIC、LD50 及 delta 法近似 SE/CI（\(\beta_1\neq 0\)）。  
- 图：观测比例 vs 应力 + 拟合曲线。  
- **不做：** 真 probit 链接、正态 tolerance 带、Minitab 逐点 golden。

## 3. 接线

`probit_reliability.cpp` → Facts → Service → 命令/解释/序列化/help。
