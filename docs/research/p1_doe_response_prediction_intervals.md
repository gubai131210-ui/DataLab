# P1 DOE 响应优化：精确预测区间

- 访问日期：2026-08-20
- 目标：让 `Response Optimization` 在回归协方差可用时输出精确 CI / PI；协方差缺失时明确显示 `*`
- 口径：优先贴近 Minitab Predict / Response Optimizer

## 结论

当前 `predict_response(...)` 已支持：

- `SE Fit = sqrt(x0' Cov(b) x0)`
- `PI SE = sqrt(SE Fit^2 + MSE)`
- `CI = ŷ ± t * SE Fit`
- `PI = ŷ ± t * PI SE`

缺的不是公式，而是 `ResponseModel.coefficient_covariance` 的上游接线。

## 公式

线性回归预测点 `x0`：

- `ŷ = x0' b`
- `Cov(b) = MSE * (X'X)^(-1)`
- `SE Fit = sqrt(x0' Cov(b) x0)`
- `SE Pred = sqrt(SE Fit^2 + MSE)`
- `CI = ŷ ± t_(1-α/2, df) * SE Fit`
- `PI = ŷ ± t_(1-α/2, df) * SE Pred`

其中：

- `MSE` 为残差均方
- `df` 为残差自由度
- `x0` 的项顺序必须与回归系数顺序一致

## DataLab seam

现有回归结果已经有：

- `RegressionResult.xtx_inverse`
- `RegressionResult.error_mean_square`

DOE 响应拟合 `DoeResponseAnalysisResult` 之前只保留了：

- `coefficients`
- `residual_mean_square`
- `residual_degrees_of_freedom`

因此本轮需要把 `xtx_inverse` 继续向上带到 `build_response_model(...)`。

## 实现策略

1. 在 `DoeResponseAnalysisResult` 增加 `xtx_inverse`
2. 从 `fit_linear_regression(...)` 回填到 DOE 结果
3. `build_response_model(...)` 内构造：
   - `coefficient_covariance = residual_mean_square * xtx_inverse`
4. 仅当协方差矩阵维度与项顺序都匹配时，才开放响应优化区间
5. 若缺协方差矩阵：
   - 不走近似 `residual_standard_error / sqrt(n)` 路径
   - 表格 CI / PI 显示 `*`
   - 诊断明确提示“缺少回归系数协方差矩阵”

## 顺序约束

`x0` / `Cov(b)` 的顺序必须一致：

1. 截距
2. 主效应（按 `factor_names` 顺序）
3. 二阶交互（按 `interaction_coefficients` 顺序）

如果顺序不一致，区间虽然“有数”，但会 silently wrong，比直接 `*` 更危险。

## Minitab 参考

1. Methods and formulas for Predict  
   https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/using-fitted-models/how-to/predict/methods-and-formulas/methods-and-formulas/  
   访问日期：2026-08-20

2. Interpret the key results for Response Optimizer  
   https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/using-fitted-models/how-to/response-optimizer/interpret-the-results/key-results/  
   访问日期：2026-08-20

3. All statistics and graphs for Response Optimizer  
   https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/using-fitted-models/how-to/response-optimizer/interpret-the-results/all-statistics-and-graphs/  
   访问日期：2026-08-20

## 验收口径

- 有 `Cov(b)`：响应预测表中 `置信下限 / 置信上限 / 预测下限 / 预测上限` 非 `*`
- 无 `Cov(b)`：上述 4 列全部 `*`，并有诊断解释原因
- Facts：
  - `prediction_interval_available = true` 仅代表精确协方差路径可用
  - 不能把近似路径冒充成精确区间
