# P3：Bootstrap 单均值置信区间

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> Track F1；`formula_reference ≠ golden`。

## 0. 锁定与禁止偷懒

| 命令 | 交付 |
|---|---|
| `bootstrap_mean` | 单样本均值百分位 bootstrap CI；`BootstrapMeanFacts` |

**禁止：** 只出点估计无区间；未写清百分位 vs BCa（本轮锁定**百分位**）；菜单占位。

## 1. 权威来源

| 来源 | URL | 访问 |
|---|---|---|
| Minitab Feature List（Simulations / resampling） | https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-21 |
| NIST EDA（bootstrap 背景，次级） | https://www.itl.nist.gov/div898/handbook/eda/eda.htm | 2026-08-21 |

## 2. 产品锁定

- 单列数值；complete-case。  
- \(B\) 次有放回重抽样（默认 2000）；固定 `seed`。  
- 每次算 \(\bar x^*\)；\((1-\alpha)\) 百分位区间取经验分位 \(\alpha/2\) 与 \(1-\alpha/2\)。  
- 输出：点估计、CI、bootstrap 均值分布直方图（或序列）；诊断写明非 BCa。  
- **不做：** 两样本差（F2）、BCa、中位数 bootstrap（可后续）。

## 3. 接线

`bootstrap_mean.cpp` → Facts → Service → 命令/解释/序列化/测试/help。
