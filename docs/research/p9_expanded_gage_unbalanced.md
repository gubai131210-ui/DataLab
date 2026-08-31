# P9：不平衡 Expanded Gage R&R（GLM 窄化）

> 研究日期：2026-08-31 · 访问 2026-08-31（UTC+8）  
> Wave-9 W9-1；`expanded_gage_unbalanced`；GLM 方差分量；非 golden。

## 锁定

| 命令 | 交付 |
|---|---|
| `expanded_gage_unbalanced` | 不平衡 Part×Operator（+ 可选第 3 因子）；GLM → VarComp；%GRR / %Study Var / NDC |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/expanded-gage-r-r-study/methods-and-formulas/methods-and-formulas/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/expanded-gage-r-r-study/before-you-start/overview/ | 2026-08-31 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/expanded-gage-r-r-study/interpret-the-results/key-results/ | 2026-08-31 |
| https://blog.minitab.com/en/blog/quality-data-analysis-and-statistics/unbalanced-designs-and-gage-randr | 2026-08-31 |

## 表形

- Gage R&R VarComp 表（Repeatability / Reproducibility / Part-to-Part / Total）
- %Contribution(of VarComp)
- Study Var / %Study Var / %Tolerance（可选公差）
- Number of Distinct Categories (NDC)

## 公式（# source: formula_reference）

随机效应 VarComp：由 GLM/ANOVA 的 MS 与期望均方估计。

固定效应 VarComp（窄化）：\( \text{VarComp}_\text{fixed} = \sum_{j=1}^{J-1} \text{coef}_j^2 \)（含隐藏参照水平）。

汇总：

\[
\text{%Contribution} = \frac{\text{VarComp}_\text{source}}{\text{VarComp}_\text{Total}}
\]

\[
\text{StdDev} = \sqrt{\text{VarComp}}, \quad \text{Study Var} = k \cdot \text{StdDev}
\]

\[
\text{NDC} = \max\left(1, \left\lfloor \sqrt{2 \cdot \frac{\sigma_{P-P}^2}{\sigma_{GRR}^2}} \right\rfloor \right)
\]

不平衡设计：不等 Part×Operator 重复仍须可估；不可估时 error_page 诊断。

## UI 分页

1. 测量 + Part / Operator / 附加因子  
2. 随机 / 固定 / 嵌套声明  
3. GLM / 方差分量方法  
4. 预览
