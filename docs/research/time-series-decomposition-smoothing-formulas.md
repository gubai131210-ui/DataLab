# 时间序列分解与指数平滑输出合同

> 研究日期：2026-08-18  
> 访问日期：2026-08-18（UTC+8）  
> 本文记录 Minitab 风格表形与 DataLab 实现边界。`formula_reference ≠ golden`，未从 Minitab 导出的数值不得写入 golden。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| 时间序列分解 | [Decomposition methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/time-series/how-to/decomposition/methods-and-formals/methods-and-formulas/) | 2026-08-18 |
| 单/双指数平滑 | [Single/Double exponential smoothing](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/time-series/how-to/single-exponential-smoothing/methods-and-formulas/methods-and-formulas/) | 2026-08-18 |
| Holt 线性趋势 | [Double exponential smoothing (Holt)](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/time-series/how-to/double-exponential-smoothing/methods-and-formulas/methods-and-formulas/) | 2026-08-18 |

## 2. 经典分解（Classical Decomposition）

加法模型：

```text
Y_t = Trend_t + Seasonal_t + Error_t
```

乘法模型：

```text
Y_t = Trend_t × Seasonal_t × Error_t
```

DataLab 步骤（与 `statistical-methodology.md` 一致）：

1. 长度为 `m` 的中心移动平均提取趋势分量。
2. 加法：`RawSeasonal_t = Y_t − MA_t`；乘法：`RawSeasonal_t = Y_t / MA_t`。
3. 按季节位置取 raw seasonal 的中位数，加法归一化到均值 0，乘法归一化到均值 1。
4. 对去季节序列按实际时间做线性趋势：`Trend_t = b0 + b1 × Time_t`。
5. 重组拟合值并计算残差；未来期由趋势 + 季节指数外推。

约束：

- 周期 `m` 为正且至少两个完整周期（`n ≥ 2m`）。
- 时间严格递增；非等间隔时间给出 `irregular_time_spacing` 警告。
- 乘法模型要求观测值严格为正。
- 缺失/无效时间或数值返回诊断，不静默修复。

## 3. 指数平滑

单指数平滑（SES）：

```text
S_t = α Y_t + (1 − α) S_(t−1)
```

双指数平滑（Holt，DataLab `double` 方法）：

```text
L_t = α Y_t + (1 − α)(L_(t−1) + T_(t−1))
T_t = γ(L_t − L_(t−1)) + (1 − γ) T_(t−1)
F_(t+m) = L_t + m T_t
```

预测区间（公式参考，非 Minitab golden）：

```text
Half-width ≈ 1.96 × 1.25 × MAD
```

准确度：MAD、MSD、MAPE（%）。

## 4. 服务层输出合同（对齐 ARIMA / 季节预测）

| 方法 | 表 | Facts | method_metadata.estimation_method |
|---|---|---|---|
| 时间序列分解 | 预测准确度 / 拟合与预测明细 / 季节指数 | `ForecastFacts.mape` | `classical_decomposition_cma_trend` |
| 单指数平滑 | 拟合与预测明细 / 预测准确度 | `ForecastFacts.mape` | `single_exponential_ses` |
| 双指数平滑 | 拟合与预测明细 / 预测准确度 | `ForecastFacts.mape` | `holt_linear_des` |

共同字段：

- 明细表含 `原始行`（1-based 工作表行号）。
- `method_metadata`: `parameter_source=estimated`, `valid_count`, `missing_count`, `source_rows`。
- PlotSpec series: `actual`, `fitted`, `forecast`, `confidence_band`（平滑）或 `actual/trend/fitted/remainder/forecast`（分解）。
- 解释层只读 `ForecastFacts`；不写“预测已验证”或“模型最优”。

## 5. 本轮实现边界

| 模块 | 位置 |
|---|---|
| 分解算法 | `src/domain/statistics/time_series_decomposition.*` |
| 平滑算法 | `src/domain/statistics/time_series.*` |
| 服务装配 | `AnalysisService::time_series_decomposition/smoothing` |
| 解释 | `InterpretationService::add_forecast_rules` |
| 测试 | `quality_statistics_test`, `interpretation_service_test` |

未实现：Minitab 分解的 STL/X-11 变体；平滑参数自动寻优；分解预测区间。
