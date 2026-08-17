# ARIMA 基础预测样例

文件：`arima_trend.csv`  
完整路径：`d:\QT_CppPrograms\DataLab\samples\time_series\arima\arima_trend.csv`

## 列说明

| 列名 | 角色 | 说明 |
|---|---|---|
| `Period` | 时间列 | 严格递增整数序号 |
| `Value` | 时间序列值 | 带缓慢上升趋势的数值 |

## 导入与操作

1. 打开 DataLab。
2. **文件 → 打开 / 导入**，选择本目录 `arima_trend.csv`。
3. 确认工作表出现 `Period`、`Value`。
4. 菜单：**统计 → ARIMA 基础预测**
5. 弹窗设置：
   - 时间列（可选）：`Period`
   - 时间序列值：`Value`
   - 选模准则：`aicc`（也可用 `aic` / `bic`）
   - 预测期数：`4`
6. 确定后查看输出页。

## 输出对照

| 区块 | 看什么 |
|---|---|
| 候选模型比较 | ARIMA(0,1,0)、AR(1)、MA(1) 的 SSE、AIC、AICc、BIC |
| 模型摘要与预测 | 最优模型、截距/系数、Forecast、Lower、Upper |
| 图形 | 拟合值与预测值散点/序列图 |

首版固定比较三种候选模型，再按准则选择最优模型做递推预测。

## 数据契约

- 若指定时间列，必须与数值列有效行一致，且时间严格递增、无重复。
- 当前版本不自动补齐缺失时间点。
- 值列必须全部为有限数值。
- 预测期数至少为 1。
- 样本量过小会返回诊断，不虚构预测。

## Minitab 对照

- `Stat > Time Series > ARIMA`
- 或 `Stat > Time Series > Forecast with Best ARIMA Model`
