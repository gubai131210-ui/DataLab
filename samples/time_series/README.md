# 时间序列平滑样例

通用导入步骤：

1. **文件 → 打开 / 导入** 本目录 CSV。
2. 菜单：**统计 → 时间序列平滑**
3. 时间序列列选择数值列（`Sales` 或 `Demand`）。`Period` 仅作序号参考。

## sales_single.csv（单指数）

适合水平波动、趋势不明显的序列。

弹窗设置：

| 参数 | 建议值 |
|---|---|
| 时间序列 | `Sales` |
| 方法 | `single` |
| Alpha | `0.3` |
| Gamma | 可忽略 |
| 预测期数 | `3` |

预期输出：拟合值、未来 3 期预测、预测区间、MAD / MSD / MAPE。

Minitab 对照：`Stat > Time Series > Single Exponential Smoothing`

## sales_double.csv（双指数）

适合带明显上升趋势的序列。

弹窗设置：

| 参数 | 建议值 |
|---|---|
| 时间序列 | `Demand` |
| 方法 | `double` |
| Alpha | `0.3` |
| Gamma | `0.2` |
| 预测期数 | `4` |

预期现象：预测值沿趋势继续上升；单指数在此数据上通常会系统性滞后。

Minitab 对照：`Stat > Time Series > Double Exponential Smoothing`

## ARIMA

ARIMA 基础预测样例在子目录：

- `samples/time_series/arima/arima_trend.csv`
- 使用说明：`samples/time_series/arima/README.md`
- 菜单：**统计 → ARIMA 基础预测**

## 说明

- 当前版本按列中有效数值的行顺序建模，请保证导入后时间顺序正确。
- Alpha / Gamma 必须在 `(0, 1]`；预测期数至少为 1。
- 空值或非数值会按 complete-case 跳过，并影响拟合长度。
