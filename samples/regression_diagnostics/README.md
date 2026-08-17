# 回归残差诊断样例

文件：`regression_diagnostics.csv`

用于验证本批新增的回归诊断输出：Durbin–Watson、删除学生化残差、Cook's D、DFITS、
残差与拟合值/顺序/预测变量图。

## 导入与操作

1. 打开 DataLab。
2. **文件 → 打开 / 导入**，选择：
   `samples/regression_diagnostics/regression_diagnostics.csv`
3. 菜单：**统计 → 线性回归**
4. 弹窗设置：
   - 变量列按顺序选择：`Response`、`Temperature`、`Pressure`
   - 第一列是响应，其后是预测变量
   - 置信水平默认 `0.95`

## 输出重点

1. **模型摘要**
   - S、R-sq、R-sq(adj)、R-sq(pred)、PRESS、F、P-Value
   - Durbin-Watson
   - 异常 / 高杠杆 / 影响观测计数
2. **系数表**
   - Coef、SE Coef、T、P-Value、置信区间、VIF
3. **回归方差分析**
4. **拟合与诊断表**
   - 标准化残差、删除学生化残差、杠杆值、Cook 距离、DFITS、诊断标记
5. **诊断图**
   - 残差与拟合值
   - 残差与观测顺序
   - 残差与预测变量（第一预测变量）

## 预期现象

- `Temperature` 与 `Pressure` 高度相关，VIF 可能偏高。
- 最后一行刻意偏高，可能触发影响观测或异常标记。
- Durbin–Watson 接近 2 表示残差自相关不明显；远离 2 时注意顺序自相关。

## 数据契约

- 使用 complete-case：响应或任一预测变量缺失的行会被跳过。
- 至少需要：1 个响应 + 1 个预测变量，且有效行数大于预测变量数 + 1。
- 设计矩阵秩亏时返回诊断，不虚构系数。

Minitab 对照：`Stat > Regression > Regression > Fit Regression Model`
然后查看 Residual Plots、Fits and Diagnostics。
