# P1 Acceptance / ANOM / Poisson GOF / 协方差偏相关

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。

## 0. 锁定与禁止偷懒

**做：** `acceptance_sampling`（属性一次抽样 + OC）；`anom`（正态均值）；`poisson_gof`；加深 `correlation` 出协方差矩阵 + 可选偏相关。  
**禁止偷懒：** 禁止只画图无表/Facts；禁止改坏 `chi_square_gof` / `correlation` Pearson；禁止 ANSI 全表假 golden；禁止 ANOM 本轮做二项/泊松变体（可诊断引导）；禁止解释「批次合格 / 已证明同均值」。

---

## 1. Acceptance sampling（属性一次抽样）

| 来源 | URL | 访问 |
|---|---|---|
| OC 曲线 | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/acceptance-sampling/supporting-topics/operating-characteristic-oc-curve/ | 2026-08-21 |
| NIST | https://www.itl.nist.gov/div898/handbook/pmc/section2/pmc22.htm | 2026-08-21 |

**产品：** 命令 `acceptance_sampling`；输入：批大小 N（可选，无限批用二项）、样本量 n、接收数 c、AQL/RQL（可选显示风险）。  
二项模型（无限批 / n≪N）：\(P_a(p)=\sum_{k=0}^{c}\binom{n}{k}p^k(1-p)^{n-k}\)。  
有限批可用超几何；本轮锁定**二项 OC**，N 仅作参数摘要。  
输出：计划表；OC 表（p 网格 + Pa）；OC 曲线图。Facts：`AcceptanceSamplingFacts`。

---

## 2. ANOM（正态）

| 来源 | URL | 访问 |
|---|---|---|
| 解读 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/analysis-of-means/interpret-the-results/key-results/ | 2026-08-21 |
| 数据 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/analysis-of-means/perform-the-analysis/enter-your-data/ | 2026-08-21 |

测量 + 分组；总体均值 \(\bar{\bar y}\)；组均值 \(\bar y_i\)；MSE 来自组内；决策限（等样本简化）：

\[
UDL/LDL=\bar{\bar y}\pm h_{\alpha}\,\hat\sigma\sqrt{\frac{k-1}{N}}
\]

（等 n 时 \(\hat\sigma=s_p\)；\(h_\alpha\) 用学生化范围/正态近似产品锁定：等样用 \(z_{1-\alpha/(2k)}\sqrt{(k-1)/k}\) 型 Nelson 近似，诊断标明近似）。  
图：组均值点 + 中心线 + UDL/LDL。Facts：`AnomFacts`。  
**不做：** 二项/泊松 ANOM（本轮）。

---

## 3. Poisson GOF

| 来源 | NIST / 教材卡方拟合 | 2026-08-21 |

单列非负整数计数；\(\hat\lambda=\bar x\)；按观察值合并或按频数组；Pearson \(\chi^2=\sum(O-E)^2/E\)，df=k−1−1（估 λ）。  
命令 `poisson_gof`；不碰 `chi_square_gof`。Facts：挂 `ChiSquareGofFacts` 或 method=`poisson`。

---

## 4. 协方差 / 偏相关

加深 `correlation`：  
- 增出「协方差矩阵」表（complete-case）  
- 可选 `partial=true` 且 ≥3 列：对每对控制其余变量的偏相关（精度矩阵法：\(r_{ij\cdot rest}=-\Omega_{ij}/\sqrt{\Omega_{ii}\Omega_{jj}}\)）  
Facts：`CorrelationFacts` 增 `covariance_available` / `partial_available`。

---

## 5. 测试

`# source: formula_reference`：OC 在 p=0 时 Pa=1；ANOM 极端组出界；泊松全同 λ 时 P 大；协方差对角=方差；偏相关 3 列手算方向。

## G. 已交付窄化（2026-08-21）

- `acceptance_sampling`：二项 OC；N 仅摘要  
- `anom`：正态均值 + Nelson 近似限  
- `poisson_gof`：独立于 `chi_square_gof`  
- `correlation`：协方差矩阵 + 可选 Pearson 偏相关  
