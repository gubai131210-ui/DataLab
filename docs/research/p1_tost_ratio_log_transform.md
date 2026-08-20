# P1 双样本均值比 TOST 对数变换

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 2-Sample Equivalence（比值假设） | [Hypotheses](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/how-to/2-sample-equivalence-test/before-you-start/hypotheses/) | 2026-08-20 |
| Minitab Test mean / reference mean | [Methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/how-to/2-sample-equivalence-test/methods-and-formulas/test-mean-reference-mean/) | 2026-08-20 |
| 对数比值 / 生物等效 TOST | Schuirmann (1987)；教材 log-ratio TOST 与 100(1−2α)% CI | 2026-08-20 |
| DataLab 非对数均值比 | [`p1_two_sample_mean_ratio_tost.md`](p1_two_sample_mean_ratio_tost.md) | 2026-08-20 |

## 2. 产品选型

- 深化命令 `two_sample_equivalence_ratio`；配置 `equivalence_ratio_transform`：
  - `none`（默认）：现有非对数 Fieller 合同不变
  - `log`：对数变换路径
- `EquivalenceFacts.kind` 保持 `two_sample_ratio`
- `ci_method`：`tost_ratio_1_minus_alpha`（none）vs `tost_ratio_log_1_minus_alpha`（log）
- `difference` 字段始终存 **比值尺度** 点估计（log 路径为几何均值比 \(\hat\rho_g\)）
- 解释陈述是否落入界限与变换类型；禁止「已证明等价」

## 3. 公式（log 路径）

两侧观测均须 \(x_{ij}>0\)。令 \(y_{ij}=\ln x_{ij}\)。

界限在比值尺度为 \(\delta_1<\delta_2\)（\(\delta_1>0\)），在 log 尺度：

\[
L=\ln\delta_1,\quad U=\ln\delta_2
\]

对 \(y\) 做 **差值 TOST**（复用双样本差值等价合同）：

- \(\hat\theta=\bar y_1-\bar y_2\)
- 100(1−2α)% CI 在 \(\theta\) 上：\([\hat\theta\pm t_{1-\alpha,\nu}\cdot\mathrm{SE}]\)（Welch 或 pooled）
- \(p_\mathrm{lower}\)、\(p_\mathrm{upper}\) 对 H0: \(\theta\le L\) / \(\theta\ge U\)
- `within_limits ⇔ p_lower≤α ∧ p_upper≤α`

回变换到比值尺度：

\[
\hat\rho_g=\exp(\hat\theta),\qquad
\mathrm{CI}_\rho=\bigl[\exp(\mathrm{CI}_L),\,\exp(\mathrm{CI}_U)\bigr]
\]

描述统计仍报告原始尺度均值/SD；参数摘要标明「对数变换」。

## 4. Minitab 表形（产品合同）

| 表/图 | 合同 |
|---|---|
| 描述统计 | 两组 N、均值、标准差（原始尺度） |
| 等价性检验 | 比值估计 \(\hat\rho_g\)、界限、两侧 t/P、比值 CI；方法注 log |
| 区间图 | 横轴比值；须为回变换 CI；竖线为 δ 界限 |
| Facts | `kind=two_sample_ratio`；`ci_method=tost_ratio_log_1_minus_alpha` |

## 5. 明确不做

- 改默认非对数 Fieller 数值合同
- 配对 / 比例 z-TOST 重做
- Blaker；假 Minitab golden

## 6. 测试策略

`# source: formula_reference`：正值小样例核对 \(\hat\rho_g\)、log 界限两侧 p、exp(CI)；含 ≤0 诊断；`transform=none` 与改前一致。
