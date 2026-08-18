# Minitab Golden 导出指南

本目录存放从 Minitab **实际导出**的对照数值。测试在文件缺失时 `QSKIP`，不会猜测 Minitab 结果。

## 文件清单

| 文件 | 数据集 | Minitab 菜单路径 |
|---|---|---|
| `regression_golden.tsv` | `samples/statistical_inference/regression.csv` | Stat > Regression > Regression > Fit Regression Model |
| `arima_trend_golden.tsv` | `samples/time_series/arima/arima_trend.csv` | Stat > Time Series > Forecast with Best ARIMA Model |

## TSV 格式

- UTF-8，无 BOM，Tab 分隔
- 区块以 `# section: 名称` 开头
- 配置以 `# config: key=value` 开头
- 首行数据为表头

## regression_golden.tsv 配置

```
# config: response_col=Response
# config: predictor_cols=Temperature,Pressure
# config: confidence=0.95
```

区块：`summary`、`anova`、`coefficients`、`diagnostics`

## arima_trend_golden.tsv 配置

```
# config: time_col=Period
# config: value_col=Value
# config: criterion=aicc
# config: forecast_periods=4
# config: max_d=2
```

区块：`candidates`、`forecast`

## 容差（见 VALIDATION_MATRIX.md）

- 系数 / SS：相对误差 ≤ 1e-4
- AICc：绝对误差 ≤ 0.01
- Forecast：绝对误差 ≤ 0.05
