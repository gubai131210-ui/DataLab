# P3：Bootstrap 双样本均值差置信区间

> 研究日期：2026-08-22  
> 访问日期：2026-08-22（UTC+8）  
> Wave-3 W1；`formula_reference ≠ golden`。

## 0. 锁定与禁止偷懒

| 命令 | 交付 |
|---|---|
| `bootstrap_two_sample` | 两独立样本均值差百分位 bootstrap CI；`BootstrapTwoSampleFacts` |

**禁止：** 只出点估计无区间；未写清百分位 vs BCa（本轮锁定**百分位**）；菜单占位；把两组当成配对差而不说明。

## 1. 权威来源

| 来源 | URL | 访问 |
|---|---|---|
| Minitab Support | https://support.minitab.com/ | 2026-08-22 |
| Minitab Feature List（Simulations / resampling，次级） | https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-22 |

## 2. 产品锁定

- 两列数值；各列 complete-case（有限值）。  
- 两组**独立**有放回重抽样（默认 B=2000）；固定 `seed`。  
- 统计量：\(\bar x_1 - \bar x_2\)；\((1-\alpha)\) 百分位区间取经验分位 \(\alpha/2\) 与 \(1-\alpha/2\)。  
- 输出：两组均值、均值差、CI、bootstrap 均值差直方图；诊断写明非 BCa。  
- **不做：** BCa、配对 bootstrap、中位数差（可后续）。

## 3. 接线

`bootstrap_two_sample.cpp` → Facts → Service → 命令/解释/序列化/help。
