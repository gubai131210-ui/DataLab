# P3：CART 单树（可审计经典 ML）

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。  
> **披露：** 非 Minitab TreeNet® / Random Forests® 数值对齐；自研二叉递归划分。

## 0. 本轮锁定与禁止偷懒

**做：**

| 命令 | 交付 |
|---|---|
| `cart_tree` | 分类（Gini）或回归（LS）；最大深度；树结点表；变量重要性；`CartTreeFacts` |

**禁止偷懒：**

- 禁止无混淆矩阵/叶统计的黑盒准确率  
- 禁止声称等同 TreeNet / Minitab RF  
- 禁止把 sklearn/Python 打进 dist  
- 禁止菜单占位  

---

## 1. 权威来源

| 来源 | URL | 访问 |
|---|---|---|
| Minitab：Predictive Analytics 模型类型 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/predictive-analytics/types-of-predictive-analytics-models-in-minitab-statistical-software/ | 2026-08-21 |
| Minitab：CART 回归节点分裂 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/predictive-analytics/how-to/cart-regression/methods-and-formulas/node-splitting-methods/ | 2026-08-21 |
| sklearn DecisionTree（接口/公式参考，非依赖） | https://scikit-learn.org/stable/modules/generated/sklearn.tree.DecisionTreeClassifier.html | 2026-08-21 |

## 2. 产品锁定

- 命令 `cart_tree`；响应 1 列 + ≥1 数值预测列；complete-case。  
- `task`：`classification`（响应按类别标签，Gini）或 `regression`（数值响应，平方误差）。  
- 分裂：对每个候选变量，在排序中点试阈值，选不纯度下降最大者。  
  - 分类：\(Gini=1-\sum p_c^2\)；增益 = 父 − 加权子。  
  - 回归：\(SSE=\sum(y-\bar y)^2\)；增益 = 父 SSE − 子 SSE。  
- 停止：`max_depth`（默认 5）、`min_leaf`（默认 5）、增益≤0、结点纯。  
- **不做成本复杂度剪枝全流程**（本轮仅深度/叶停）；帮助写明。  
- 输出：树结点表（id、父、深度、分裂变量/阈值、n、预测、不纯度）；叶/混淆或残差摘要；变量重要性 = 全树增益合计（归一化）。  
- 图：回归 → 观测 vs 拟合散点；分类 → 不强制混淆热图（表即可）。  
- `CartTreeFacts`：`task`、`n`、`predictor_count`、`max_depth`、`node_count`、`leaf_count`、`train_metric`（分类准确率或回归 RMSE）、`top_variable`。  
- **不做：** 随机森林、TreeNet、AutoML、类别预测列多水平编码以外的复杂因子处理（分类响应用字符串类别）。

## 3. 接线

`cart_tree.cpp` → `CartTreeFacts` → `AnalysisService::cart_tree` → 命令/解释/序列化/测试/help/文档。
