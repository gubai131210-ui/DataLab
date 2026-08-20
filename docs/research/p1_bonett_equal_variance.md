# P1 Bonett 等方差（两样本）

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 2 Variances methods | [2 Variances methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/2-variances/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| Bonett vs Levene 选用 | [Bonett's method or Levene's method](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/supporting-topics/tests-of-proportions-and-variances/bonett-s-method-or-levene-s-method/) | 2026-08-20 |
| Minitab white paper（修正 Bonett 2006） | [Bonett's Method (Banga & Fox)](https://support.minitab.com/en-us/minitab/media/pdfs/translate/Bonetts_Method_Two_Variances.pdf) | 2026-08-20 |
| Bonett (2006) | D. G. Bonett, *Computational Statistics & Data Analysis*（比率区间原稿） | 2026-08-20 |

## 2. 与 Levene / F 的关系（产品口径）

| 方法 | `variance_test_method` | 适用 | 本产品角色 |
|---|---|---|---|
| F | `f` | 近似正态 | 经典两样本方差比 |
| Levene（中位数） | `levene`（默认稳健） | 任意连续分布；小样本/重尾更稳 | **Minitab 对齐的稳健默认** |
| Levene（均值） | `levene_mean` | 经典 Levene | 可选 |
| **Bonett** | `bonett` | 任意连续；通常比 Levene 更有功效；极偏/重尾小样本慎用 | **新增第三路径** |

- **不**把 Bonett 伪装成 Levene；**不**改中位数 Levene 默认。  
- 解释只陈述比值区间与 P，**不写「已证明等方差」**。  
- **不做 Bartlett**；不做 Bonett 功效。

## 3. 公式（两样本，修正版）

记 \(n_i\)、样本标准差 \(S_i\)、修整均值 \(m_i\)（trim 比例 \(1/[2\sqrt{n_i-4}]\)，\(n_i\ge 5\)；更小样本退回算术均值并诊断）。

对假设比率 \(\rho = \sigma_1/\sigma_2\)，一致的合并峰度：

```text
γ̂(ρ) = (n1+n2) · [Σ(x1j−m1)⁴ + ρ⁴ Σ(x2j−m2)⁴]
        / [(n1−1)S1² + ρ²(n2−1)S2²]²
```

检验统计量（对 \(\rho_0\)，默认等方差 \(\rho_0=1\)）：

```text
se²(ρ0) = (γ̂(ρ0)−1)/(n1−1) + (γ̂(ρ0)−ρ0 项调整见 white paper)/(n2−1)
         （实现按 Banga–Fox / Minitab：对 SD 比用 ln(S1/S2)−ln(ρ0)）
c_α     = equalizer（不平衡设计小样本校正；平衡时为 1）
|Z|     = |ln(S1/S2) − ln(ρ0)| / (c · se)
P       = 2(1−Φ(|Z|))   # 双侧
```

置信区间：对 \(\rho\) 求根，使检验接受域边界成立（反解 CI）；方差比区间 = SD 比区间的平方。实现用数值括号求根；失败只诊断，不伪造区间。

## 4. 接线范围

- **两列** `variance_first` + `variance_second`：完整 Bonett。  
- **分组列** 且恰 2 组：同两样本。  
- **k>2 组**：诊断 `bonett_requires_two_groups`，不伪造 F/P（用户改用 Levene）。

## 5. 表形与 Facts

| 输出 | 合同 |
|---|---|
| 方差检验结果 | 方法=`Bonett`；N；统计量 Z；P；SD 比置信区间 |
| 可选描述 | 各组 N / StDev / 方差 |
| `VarianceFacts` | `method=bonett`；`statistic`；`p_value`；可选 `ci_lower`/`ci_upper`（SD 比） |

## 6. 明确不做

- Bartlett  
- Bonett 功效 / 样本量  
- k 组多重 Bonett  
- 假 Minitab golden（修正算法与桌面数值可能仍有差）  

## 7. 测试策略

`# source: formula_reference`：两正态等方差样本 P 较大；刻意放大一组尺度后 P 变小；n&lt;2 / 零方差诊断；k=3 分组列拒绝 Bonett。
