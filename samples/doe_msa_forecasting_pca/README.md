# 下一批统计能力样例

## DOE

DataLab 菜单：`质量工具 → 2 水平全因子设计`。
因子名输入 `Temperature,Pressure`，低水平输入 `180,20`，高水平输入
`220,40`，区组数为 `1`，可打开随机化。该分析生成标准顺序、运行顺序和编码
设计矩阵。

## Nested Gage R&R

导入 `nested_gage_rr.csv`，选择 `Measurement`、`Part`、`Operator`。
要求每个部件只属于一个操作者，数据为平衡嵌套设计。

## 属性一致性

导入 `attribute_agreement.csv`，选择 `Rating`、`Part`、`Appraiser` 和可选的
`Standard`，查看评估者内、评估者间以及与标准的一致率和 Kappa。

## 季节性预测

导入 `seasonal_sales.csv`，菜单选择 `统计 → 季节性预测`，值列选择 `Sales`，
周期输入 `4`，加法模型，预测期数输入 `4`。

## PCA

导入 `pca_measurements.csv`，菜单选择 `统计 → 主成分分析`，选择
`Length,Width,Strength,Weight`，建议先使用 `standardized` 模式。
