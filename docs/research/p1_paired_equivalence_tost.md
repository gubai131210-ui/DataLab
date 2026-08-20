# P1 配对等价性（Paired TOST）

- 访问日期：2026-08-20
- 目标：在现有 `one_sample_equivalence` / `two_sample_equivalence` 架构上补齐 `paired_equivalence`
- 口径：优先贴近 Minitab「Equivalence Test with Paired Data」，当前只做**差值等价**，不做 ratio / log-transformed ratio

## 公式

设每对观测差值为：

`d_i = X_i - Y_i`

样本均值与标准误：

- `d̄ = (1/n) Σ d_i`
- `s_d = sqrt( Σ(d_i-d̄)^2 / (n-1) )`
- `SE = s_d / sqrt(n)`
- `df = n - 1`

双单侧检验（TOST）：

- `t_L = (d̄ - LEL) / SE`
- `t_U = (d̄ - UEL) / SE`
- `p_L = 1 - F_t(t_L, df)`
- `p_U = F_t(t_U, df)`

判定：

- `both_pvalues_below_alpha = (p_L <= α && p_U <= α)`
- `within_limits = both_pvalues_below_alpha`

当前 DataLab 口径沿用现有均值 TOST：

- `ci_method = tost_1_minus_alpha`
- 区间为 `100(1-2α)% CI = [d̄ - t_(1-α,df) SE, d̄ + t_(1-α,df) SE]`

## 变量定义

- `X_i, Y_i`：第 `i` 对观测
- `d_i`：第 `i` 对差值
- `LEL, UEL`：等价下限 / 上限
- `SE`：配对差值均值标准误
- `df`：自由度 `n-1`
- `α`：单侧显著性水平

## 输入与数据契约

- 两列数值，按 **complete-case** 对齐
- 只有同一原始行两列都有效时，这一对才进入分析
- `source_row` 仍用于图表 / 输出追溯
- 不从工作表删除缺失行，只在分析时跳过

## 输出口径（贴近 Minitab）

- 描述统计表：配对差值 `N / Mean / StDev`
- TOST 表：Difference、Lower t / P、Upper t / P、`CI method`、Equivalence Interval、Conclusion
- 等价区间图：差值区间 + 下/上等价界限
- Facts：沿用 `EquivalenceFacts`，其中 `kind = "paired"`

## 边界与限制

- `LEL >= UEL`：诊断 `invalid_equivalence_limits`
- `SE <= 0` 或 `df <= 0`：诊断 `zero_variance`
- 当前不做：
  - 比值等价
  - 对数变换比值等价
  - 单侧 superiority / inferiority 专用表形

## Minitab 参考

1. Overview for Equivalence Test with Paired Data  
   https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/how-to/equivalence-test-with-paired-data/before-you-start/overview/  
   访问日期：2026-08-20

2. Methods and formulas for Test mean - reference mean for Equivalence Test with Paired Data  
   https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/how-to/equivalence-test-with-paired-data/methods-and-formulas/test-mean---reference-mean/  
   访问日期：2026-08-20

3. Difference (or Ratio) for Equivalence Test with Paired Data  
   https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/how-to/equivalence-test-with-paired-data/interpret-your-results/all-statistics-and-graphs/difference-or-ratio/  
   访问日期：2026-08-20

## 实现备注

- 深 seam 放在 `src/domain/statistics/equivalence_test.cpp` 的 `fill_tost(...)`
- `paired_equivalence` 复用 `paired_t_test(...)` 产出的 `mean_difference / standard_error / df`
- 解释层仍只陈述“区间是否落在界限内”，不写“已证明等价”
- 当前测试全部标注 `# source: formula_reference`，禁止冒充 Minitab golden
