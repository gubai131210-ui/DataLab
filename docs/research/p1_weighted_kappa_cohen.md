# P1 Weighted Kappa（Cohen linear / quadratic）

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源与产品口径

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab AAA Kappa（未加权 / Fleiss） | [Kappa statistics](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/attribute-agreement-analysis/attribute-agreement-analysis/methods-and-formulas/kappa-statistics/) | 2026-08-20 |
| Minitab 有序评级 | 官方有序路径用 **Kendall W/τ**，**无** linear/quadratic 加权 Kappa | 2026-08-20 |
| Cohen weighted kappa | Cohen, J. (1968). Weighted kappa. *Psychological Bulletin*. | 1968 / 参考 |

**产品口径：** DataLab 在 `kappa_weight_scheme=linear|quadratic` 时计算 **Cohen 加权 Kappa**。这不是 Minitab AAA 默认输出。Minitab 有序一致性仍通过 `ordinal=true` 的 Kendall。帮助与解释必须写明差异；不得标为 Minitab golden。

## 2. 公式

对两评估者（或评估者 vs 标准）的 k×k 列联表，行/列按有序等级 0…k−1 编号：

```text
linear:     w_ij = 1 − |i−j| / (k−1)
quadratic:  w_ij = 1 − [(i−j)/(k−1)]²
（k=1 时权重无定义 → 不可识别）

p_ij = n_ij / N
P_o = Σ_i Σ_j w_ij p_ij
P_e = Σ_i Σ_j w_ij p_i. p_.j
κ_w = (P_o − P_e) / (1 − P_e)
```

`P_e = 1` 时不可识别（与未加权相同）。

SE（本轮 formula_reference，对齐现有 simple_binomial 风格，非 Fleiss 全方差表）：

```text
SE ≈ sqrt( P_o (1−P_o) / (N (1−P_e)²) )
CI = κ_w ± z_(1−α/2) · SE   （截断到 [−1,1]）
```

Fleiss overall **保持未加权**；权重开启时诊断 `fleiss_remains_unweighted`。

## 3. 等级排序

1. 收集表中全部非空评级标签。  
2. 若全部可解析为有限数值 → 按数值升序赋秩。  
3. 否则诊断 `ordinal_ratings_unranked`，**不计算加权**（两两回退未加权 Cohen 并保留诊断）。  
4. `none`：现有 `cohen_unweighted` 不变。

## 4. 配置与边界

- `MsaConfiguration::kappa_weight_scheme`：`none`（默认）/ `linear` / `quadratic`。  
- 未知 scheme → `unknown_kappa_weight_scheme`，回退 `none`。  
- 空评级不进分母。  
- 方法名：`cohen_linear` / `cohen_quadratic`。  
- Facts：`kappa_weight_scheme`、`weighted_kappa_available`。  
- **序列化必须持久化** `kappa_weight_scheme`（配置 + Facts）。

## 5. 明确不做

- 不改 Fleiss overall 公式冒充加权。  
- 不把 Weighted Kappa 写成 Minitab AAA 功能。  
- 不改 Kendall 路径。  
- 无 Minitab 导出数值冒充 golden。
