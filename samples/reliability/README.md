# 可靠性分析样例

导入 `reliability_survival.csv` 后选择：

1. `Reliability Analysis`
2. 寿命/时间列：`Time`
3. 失效指示列：`Event`（`1` 为失效，`0` 为右删失）
4. 分组列：`Group`（可选；Kaplan-Meier 模式会增加 Log-rank 检验）
5. 模型可输入 `kaplan_meier`、`weibull` 或 `exponential`

样例用于验证 Kaplan-Meier Greenwood 置信区间、Log-rank 分组比较，以及右删失
Weibull/Exponential 参数估计。`Event=0` 的记录进入风险集，但不计入失效数。
