# P1 Gage：%Tolerance / Bias 表形对齐

- 访问日期：2026-08-20
- 目标：补 `%Tolerance` 条图、Nested `*` 显示，以及 Bias 表的 `N / SE`
- 口径：优先贴近 Minitab / AIAG MSA 的表形与解释习惯，不改现有方差分量和回归计算核心

## %Tolerance

现有 crossed / nested 结果对象都已经有：

- `percent_tolerance`
- `percent_tolerance_available`

因此本轮只需要补 UI 接线：

1. 方差分量表中，无公差时显示 `*`
2. 有公差时新增 `方差分量 %Tolerance` Pareto
3. 无公差时不输出这张图

图的来源顺序与现有 `%Contribution` / `%Study Var` 一致：

- Repeatability
- Reproducibility
- Part-To-Part

## Nested `*` 规则

Nested Gage R&R 之前在 `%Tolerance` 不可用时仍直接格式化数值，容易出现 `0.0` 伪装成真实结果。

本轮规则：

- `percent_tolerance_available == false` -> 输出 `*`
- 绝不把 `0.0` 当成“不可算”的占位

## Bias 表

`BiasLinearityLevel` 已经包含：

- `valid_count`
- `standard_error`
- `bias`
- `%Bias`
- `t`
- `P`

因此 Bias 表按 Minitab / AIAG 更完整的列型补齐为：

- `Reference`
- `N`
- `Bias`
- `SE Bias`
- `%Bias`
- `t`
- `P`

Average 行：

- `N` = 所有 level 的有效观测总数
- `SE Bias` 由 `average_bias / average_bias_t` 反推（当 t 可用且非 0）

## 不改动的东西

- Crossed / Nested 方差分量公式
- ndc
- Cg / Cgk
- Bias / Linearity 的 OLS 拟合与 p 值

## Minitab / AIAG 参考

1. What is a gage linearity and bias study?  
   https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/supporting-topics/other-gage-studies-and-measures/what-is-a-gage-linearity-and-bias-study/  
   访问日期：2026-08-20

2. Methods and formulas for Gage Bias  
   https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-linearity-and-bias-study/methods-and-formulas/gage-bias/  
   访问日期：2026-08-20

3. All statistics and graphs for Gage Linearity and Bias Study  
   https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-linearity-and-bias-study/interpret-the-results/all-statistics-and-graphs/  
   访问日期：2026-08-20

## 验收口径

- crossed / nested 且有公差：表里 `%Tolerance` 非 `*`，并出现 `%Tolerance` Pareto
- 无公差：不出现 `%Tolerance` Pareto，表里 `%Tolerance` = `*`
- Bias/Linearity：`Gage Bias` 表含 `N`、`SE Bias`，Average 行也有对应列
