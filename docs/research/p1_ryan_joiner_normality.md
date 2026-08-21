# P1 Ryan–Joiner 正态性（接 normality_test）

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。加深 `normality_test`；**默认仍 Anderson–Darling**；不重做 AD 本体。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 正态性方法 | [Normality Test methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/normality-test/methods-and-formulas/methods-and-formulas/) | 2026-08-21 |
| Minitab 统计解释 | [Interpret statistics](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/normality-test/interpret-the-results/all-statistics-and-graphs/) | 2026-08-21 |
| RJ 相关统计量叙述 | [PSU STAT 462 §6.3](https://online.stat.psu.edu/stat462/node/147/) | 2026-08-21 |
| 临界近似 / P 插值（公开复现） | 社区对 Minitab corα 公式的复述（公式参考，非 golden） | 2026-08-21 |

## 2. 产品选型

- 命令 `normality_test` 增加 `method`：`anderson_darling`（默认）| `ryan_joiner`
- 配置字段：`InferenceConfiguration::normality_method`
- Facts：`NormalityFacts.method`；RJ 时填 `ryan_joiner_r`；AD 字段在 RJ 模式下可空
- 概率图 + 直方图保留；能力/Johnson 等调用方继续默认 AD 路径
- 解释禁止「已证明正态」

## 3. 公式（锁定）

### 3.1 统计量

有序样本 \(Y_{(1)}\le\cdots\le Y_{(n)}\)。结：平均秩。正态得分（Blom）：

\[
b_i=\Phi^{-1}\!\left(\frac{r_i-0.375}{n+0.25}\right)
\]

其中 \(r_i\) 为第 \(i\) 个有序观测的平均秩。样本方差 \(s^2\)（除以 \(n-1\)）。

\[
R_p=\frac{\sum_{i=1}^{n} Y_{(i)} b_i}{\sqrt{s^2(n-1)\sum_{i=1}^{n} b_i^2}}
\]

\(R_p\) 接近 1 支持正态。\(n<3\)、零方差 → 诊断、不伪造。

### 3.2 临界值近似（公式参考）

对 \(\alpha\in\{0.10,0.05,0.01\}\)，\(u=n^{-1/2}\)，\(v=n^{-1}\)，\(w=n^{-2}\)：

| α | 临界 \(c_\alpha(n)\) |
|---|---|
| 0.10 | \(1.0071 - 0.1371 u - 0.3682 v + 0.7780 w\) |
| 0.05 | \(1.0063 - 0.1288 u - 0.6118 v + 1.3505 w\) |
| 0.01 | \(0.9963 - 0.0211 u - 1.4106 v + 3.5152 w\) |

（n&lt;8 警告；与 Minitab 表可能略有差异 → 禁止 golden。）

### 3.3 P 值（线性插值，对齐 Minitab 叙述）

令 \(c_{10},c_{05},c_{01}\) 为上表。

- 若 \(R_p > c_{10}\)：报告 \(p>0.10\)（存储 `p_value=0.10` + 诊断 `ryan_joiner_p_gt_0_10`）
- 若 \(R_p \le c_{01}\)：报告 \(p<0.01\)（存储 `p_value=0.01` + 诊断 `ryan_joiner_p_lt_0_01`）
- 若 \(c_{05}<R_p\le c_{10}\)：在 0.05–0.10 间对 \(R_p\) 线性插值
- 若 \(c_{01}<R_p\le c_{05}\)：在 0.01–0.05 间线性插值

判定：相对产品 `alpha`（默认 0.05），若可算 P 则 `reject` ⇔ \(p<\alpha\)；对边界诊断仍用 \(R_p\) 与 \(c_\alpha\) 一致性检查。

## 4. 表形

| method | 表列 |
|---|---|
| AD | 现有 AD / A²* / P / 判定 |
| RJ | 变量 / N / N* / Mean / StDev / RJ(R) / Alpha / P-Value / 判定 |

## 5. 明确不做

Kolmogorov–Smirnov 切换；改 AD 公式；假 Minitab golden；能力页强制改 RJ。

## 6. 测试

近似正态样本 RJ P 不极小；强偏斜 P 小或拒绝；默认 AD 回归；`# source: formula_reference`。
