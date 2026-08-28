# P7：Analyze Variability（2 水平分散效应）

> 研究日期：2026-08-28 · 访问 2026-08-28（UTC+8）  
> Wave-7 W7-3；`analyze_variability`；ln(σ) 分散模型；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `analyze_variability` | 2 水平 DOE；重复列→运行标准差；ln(s) LSE；效应 = 2×系数 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-variability/before-you-start/overview/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-variability/methods-and-formulas/model-information/ | 2026-08-28 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/supporting-topics/factorial-and-screening-designs/analyze-location-effects-and-dispersion-effects/ | 2026-08-28 |

## 表形

- Std Dev by Run
- Dispersion Coefficients / Effects
- ANOVA（分散）
- Diagnostics

## 公式（# source: formula_reference）

每运行标准差 \(s_i\)（重复列样本标准差）。

分散模型（±1 编码 \(z_{ij}\)）：

\[
\ln s_i = \sum_j \gamma_j z_{ij}
\]

2 水平效应：\(\text{Effect}_j = 2\hat{\gamma}_j\)。

权重（说明）：\(w_i \propto 1/\hat{\sigma}_i^2\)。

## UI 分页

1. 数据布局（因子 + 重复列）  
2. 估计方法  
3. 方法说明  
4. 预览
