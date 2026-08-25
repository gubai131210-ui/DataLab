# P6：可修复系统幂律 NHPP（Crow–AMSAA）

> 研究日期：2026-08-25 · 访问 2026-08-25（UTC+8）  
> Wave-6 W6-3；`nhpp_repairable`；幂律强度；非 RBD / 非多模型全家桶。

## 锁定

| 命令 | 交付 |
|---|---|
| `nhpp_repairable` | 累积失效时间；β、λ（MLE）；强度/累积均值表；可选 Duane 图；诊断 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://www.itl.nist.gov/div898/handbook/apr/section1/apr17.htm | 2026-08-25 |
| https://www.itl.nist.gov/div898/handbook/apr/section1/apr172.htm | 2026-08-25 |
| https://www.itl.nist.gov/div898/handbook/apr/section1/apr191.htm | 2026-08-25 |

## 表形

- Parameter Estimates（β、λ）
- Intensity / Mean function（t、λ(t)、M(t)）
- Observation Summary（N failures、T）

## 公式（# source: formula_reference）

幂律 NHPP（Crow–AMSAA）：

\[
\lambda(t) = \lambda\,\beta\, t^{\beta-1},\qquad M(t)=\lambda\, t^{\beta}
\]

对时间截尾观测，失效时间 \(0 < t_1 < \cdots < t_n \le T\)（若未给 T，取 \(T=t_n\)），MLE：

\[
\hat\beta = \frac{n}{\sum_{i=1}^{n}\ln(T/t_i)},\qquad
\hat\lambda = \frac{n}{T^{\hat\beta}}
\]

（等价形式：\(\hat\beta = n / \sum_i \ln(T/t_i)\)。）

**Duane 图（可选）**：累积 MTBF \(t_i/i\) 对 \(t_i\) 的 log-log 散点，作趋势参考（非证明「已稳定」）。

## UI 分页

1. 数据（时间列 + 截尾 T）  
2. 方法（幂律 NHPP / MLE）  
3. 结果说明预览  

## 明确不做

- HPP 以外多模型全家桶、系统 RBD、宣称「ROCOF 已合格/已证明稳定」、golden
