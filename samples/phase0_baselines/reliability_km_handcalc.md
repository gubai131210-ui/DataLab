# KM 手算说明（formula_reference）

数据文件：`reliability_km_handcalc.csv`

有序失效时间，右删失发生在 15 与 25。

| 失效时刻 t | 风险集 | d | 递推 R(t) |
|---|---|---|---|
| 10 | 5 | 1 | 4/5 = 0.8 |
| 20 | 3（15 已删失离开） | 1 | 0.8 × 2/3 ≈ 0.5333 |
| 30 | 1 | 1 | ≈ 0.5333 × 0/1 = 0 |

验收：

- 删失时刻 15、25 **不得**产生 failure step。
- 证据类型：`formula_reference`（NIST product-limit）。
- **不得**标记为 `golden` 或 `vendor_oracle`。
