# P3：CCF（互相关）

> 研究日期：2026-08-21 · 访问 2026-08-21（UTC+8）  
> 配 `acf_pacf`；`formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `ccf` | 两序列互相关；±滞后；白噪声带宽；`CcfFacts` |

## 来源

| URL | 访问 |
|---|---|
| https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-21 |
| https://www.itl.nist.gov/div898/handbook/eda/section3/autocopl.htm | 2026-08-21 |

## 产品

- 两列数值；按行对齐 complete-case；滞后 \(-L..L\)。  
- \(c_k=\frac{\sum(x_t-\bar x)(y_{t+k}-\bar y)}{\sqrt{\sum(x-\bar x)^2\sum(y-\bar y)^2}}\)（有限重叠）。  
- 带宽 \(\pm z/\sqrt{n}\)（独立性近似）。  
- **不做：** 预白化；谱密度。
