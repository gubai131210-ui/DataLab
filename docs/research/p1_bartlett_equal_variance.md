# P1 Bartlett 等方差（k 组）

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab Test for Equal Variances | [Test for Equal Variances methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/test-for-equal-variances/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| NIST Bartlett test | [NIST e-Handbook 1.3.5.7](https://www.itl.nist.gov/div898/handbook/eda/section3/eda357.htm) | 2026-08-20 |
| Bartlett (1937) | 教材：对数方差加权 χ² + 校正因子 | 2026-08-20 |

## 2. 与 F / Levene / Bonett 的关系

| 方法 | `variance_test_method` | 适用 | 本产品角色 |
|---|---|---|---|
| F | `f` | 两样本、近似正态 | 经典方差比（两列路径） |
| Levene（中位数） | `levene` | k≥2、稳健默认 | Minitab 对齐的稳健默认 |
| Levene（均值） | `levene_mean` | 经典 Levene | 可选 |
| Bonett | `bonett` | **仅两样本** SD 比 | 已落地；k>2 诊断 |
| **Bartlett** | `bartlett` | k≥2、**正态假设更敏感** | 新增正态路径 |

- **不**把 Bartlett 伪装成 Levene；**不**改中位数 Levene 默认。  
- 分组路径选 `f` 仍走 Levene 的既有行为不改；仅显式 `bartlett` 走新路径。  
- 解释只陈述 χ²/P 与正态假设未验证，**不写「已证明等方差」**。

## 3. 公式（Bartlett，含校正）

对 \(k\) 组，第 \(i\) 组样本量 \(n_i\)、样本方差 \(s_i^2\)（分母 \(n_i-1\)），\(N=\sum n_i\)，\(ν_i=n_i-1\)，\(ν=\sum ν_i\)。

```text
s_p² = (1/ν) · Σ ν_i s_i²
q    = ν ln(s_p²) − Σ ν_i ln(s_i²)
c    = 1 + (1/(3(k−1))) · [ Σ(1/ν_i) − 1/ν ]
χ²   = q / c
df   = k − 1
P    = P(χ²_df ≥ χ²)   # 双侧等方差备择的常规右尾
```

任一组 \(n_i < 2\) 或 \(s_i^2 ≤ 0\) → 诊断，不伪造。

## 4. 接线范围

- **测量 + 分组列** \(k≥2\)：完整 Bartlett。  
- **两列**：视为 \(k=2\)。  
- 不输出 CI（与 Levene 一致）。

## 5. 表形与 Facts

| 输出 | 合同 |
|---|---|
| 方差检验结果 | 方法=`Bartlett`；N；χ²；DF；P |
| `VarianceFacts` | `method=bartlett`；`statistic`；`p_value`；`group_count` |

## 6. 明确不做

- 改 Levene / Bonett / F 公式  
- Bartlett 功效 / 样本量  
- 假 Minitab golden  

## 7. 测试策略

`# source: formula_reference`：等方差大 P；刻意放大一组尺度后 P 变小；\(n_i<2\) / 零方差诊断；k=3 分组列。
