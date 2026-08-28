# 算法 Wave-8 市场对照与公式入口（2026-08-28）

> 访问日期：2026-08-28（UTC+8）  
> 用途：给 **算法 Wave-8 `/goal`** 的 Primary URL、公式口径、Minitab 表形参考；**本文件不写代码、不填 golden**。  
> 状态权威：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md)  
> 执行计划：[`goal-wave-2026-08-28-algorithm-wave8-plan-and-mega-prompt.md`](goal-wave-2026-08-28-algorithm-wave8-plan-and-mega-prompt.md)  
> 前置水位：Wave-7 ✅（`mixture_analyze` / `glm_two_way` / `analyze_variability` / `factor_analysis`）

---

## §0 一句话水位（给新对话）

| 水位 | 状态 |
|------|------|
| P0–P2 + Wave-2～7 | ✅ / ⚪（勿重做） |
| Wave-7 | Mixture 分析 · 双因子 GLM · Analyze Variability · 因子分析 ✅ |
| G9 公式代入 + Show Your Work | 已落地；有公式的新命令应可挂 trace |
| **本 Wave 锁定** | **4 项竖切**：Binary Response DOE · Cluster Variables · 三因子 GLM · 寿命数据回归（窄化） |

**「实现所有算法」在本产品的正确含义：**  
按 backlog 产品范围内 ❌ **多 Wave 滚动清空**，**不是**单次 Goal 克隆 Minitab Feature List 100%。本 Goal 锁定 **4 项完整竖切**；其余进 §4 候补队列供 Wave-9+。

---

## §1 Minitab 市场对照摘要（2026-08-28 再访）

Primary：[Minitab Feature List](https://www.minitab.com/en-us/products/minitab/features/)（访问 2026-08-28）

### 1.1 本 Wave 要吃的缝

| 模块 | Wave-7 后仍 ❌ / 🟡 | Wave-8 |
|------|---------------------|--------|
| DOE — Binary response | ❌ | ✅ W8-1 `binary_response_doe` |
| Multivariate — Cluster Variables | ❌ | ✅ W8-2 `cluster_variables` |
| ANOVA — GLM 三因子 | ❌（W7 仅双因子） | ✅ W8-3 `glm_three_factor` |
| Reliability — Life data regression | ❌（ALT/参数族已有；协变量回归缝） | ✅ W8-4 `life_data_regression` |

### 1.2 Minitab 可学习的非算法名优势

| 优势 | Wave-8 要求 |
|------|-------------|
| DOE 位置效应 vs 分散效应 | W7 分散已做；W8 补 **二值响应** 位置模型 |
| 多元「变量聚类」树状图 | Cluster Variables：dendrogram + similarity |
| 不平衡 GLM 用拟合均值 | W8 三因子延续 W7 Type III + Fitted Means |
| 寿命回归表形 | 协变量 + 删失 + 百分位表；与 `accelerated_life` 区分 |
| 帮助公式可读 | 每项 `p8_*.md` + help 禁止「见 md」 |

### 1.3 刻意不做（本 Goal）

见 [`deferred-capability-agreement.md`](deferred-capability-agreement.md)：Mixed REML 全量、MANOVA、Expanded Gage 不平衡全量、TreeNet/AutoML、嵌 Python/R、Minitab golden、Mixture 过程变量/extreme-vertices、Graph Builder 全量、三因子 **三阶交互** 全量、寿命回归多响应/复杂嵌套。

---

## §2 Wave-8 锁定项（W8-1～W8-4）

### W8-1 `binary_response_doe` — 析因设计二值响应（Logit 窄化）

| 字段 | 内容 |
|------|------|
| **动机** | backlog §8「Analyze variability / binary response DOE」中 binary 仍 ❌；汽车质量常见 pass/fail DOE |
| **窄化** | 2～4 因子（2 水平编码或文本）；响应：**events/trials 两列** 或 **0/1 单列**（trials=1）；主效应 + 可选 AB 交互（不做三阶交互全量）；**logit** 链 IRWLS；系数 + OR + 偏差/拟合 |
| **输入** | 因子列；event 列 + trial 列（或 binary 列）；complete-case |
| **输出表形** | Method（Link=logit）；Coefficients（Coef/SE/Z/P）；Odds Ratios；Goodness-of-Fit；Event/Trial 信息 |
| **Primary URL** | [Methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-binary-response/methods-and-formulas/methods/)（2026-08-28）；[Estimated equation / IRWLS](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-binary-response/methods-and-formulas/estimated-equation/)；[Data considerations](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-binary-response/before-you-start/data-considerations/) |
| **公式入口** | \(\eta_i = X_i\beta\)；\(p_i = \text{logit}^{-1}(\eta_i)\)；IRWLS 迭代至偏差变化 \(<10^{-8}\)；OR \(=\exp(\beta_j)\) |
| **邻域** | `doe_factorial`、`logistic_regression`、`poisson_regression`（IRWLS 模式） |
| **UI** | **页1** 因子+响应布局；**页2** 模型（主效应/交互）；**页3** 方法（logit/IRWLS）；**页4** 预览 |

### W8-2 `cluster_variables` — 变量聚类（层次法窄化）

| 字段 | 内容 |
|------|------|
| **动机** | backlog §10 Cluster Variables ❌；与 `cluster_observations`（观测量聚类）对称 |
| **窄化** | ≥3 数值列；相关距离 \(d_{ij}=1-\rho_{ij}\) 或绝对相关；连结：**Complete / Single / Average / Ward** 择 3～4 种主路径；dendrogram + amalgamation 表；similarity \(s_{ij}=100(1-d_{ij}/d_{max})\) |
| **输入** | 数值变量列；complete-case 行 |
| **输出表形** | Amalgamation Steps；Similarity/Distance；Dendrogram（PlotSpec）；Cluster membership（可选切割 k） |
| **Primary URL** | [Distance measures](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/cluster-variables/methods-and-formulas/distance-measures/)（2026-08-28）；[Linkage methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/cluster-variables/methods-and-formulas/linkage-methods/)；[Similarity](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/cluster-variables/methods-and-formulas/similarity/) |
| **公式入口** | Pearson \(\rho_{ij}\)；\(d_{ij}=1-|\rho_{ij}|\) 或 \(1-\rho_{ij}\)；Ward 最小化簇内 SS |
| **邻域** | `cluster_observations`、`hierarchical_cluster`、`correlation`、`factor_analysis`（W7） |
| **UI** | **页1** 变量选择；**页2** 距离/连结；**页3** 方法说明；**页4** 预览 |

### W8-3 `glm_three_factor` — 三因子 GLM（不平衡窄化）

| 字段 | 内容 |
|------|------|
| **动机** | W7 `glm_two_way` 仅双因子；汽车 DOE/ANOVA 常见三因子不平衡 |
| **窄化** | 连续响应；因子 A/B/C（各 ≥2 水平）；主效应 + **可选 AB/AC/BC 两两交互**（默认开 AB+AC+BC 可关）；**不做 ABC 三阶交互**；Type III Adj SS；Fitted Means；Tukey 显式不做 |
| **输入** | 响应 + 三因子列；complete-case + `source_row` |
| **输出表形** | ANOVA Type III；Coefficients；Fitted Means（按因子边际）；Fits/Residuals；Assumptions |
| **Primary URL** | [Balanced/unbalanced](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/supporting-topics/anova-models/balanced-and-unbalanced-designs/)（2026-08-28）；[Fit GLM methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/fit-general-linear-model/methods-and-formulas/methods/)；[Fitted means 博客](https://blog.minitab.com/en/blog/marilyn-wheatleys-blog/anova-data-means-and-fitted-means-balanced-and-unbalanced-designs/) |
| **公式入口** | \(Y=X\beta+\varepsilon\)；Type III：项在其余项已在模型下的 Adj SS；拟合均值 = 回归预测在因子水平上的最小二乘边际均值 |
| **邻域** | `glm_two_way`（W7）、`two_factor_anova`、`regression` |
| **UI** | **页1** 列选择（三因子+响应）；**页2** 模型（主效应/两两交互）；**页3** 方法（不平衡/拟合均值）；**页4** 预览 |

### W8-4 `life_data_regression` — 寿命数据回归（Weibull + 协变量窄化）

| 字段 | 内容 |
|------|------|
| **动机** | backlog §9 寿命回归仍 ❌；`accelerated_life` 为应力加速；本命令为 **协变量回归** 表形 |
| **窄化** | 右删失；Weibull（默认）；1～2 协变量（连续或分类哑变量）；MLE Newton-Raphson；回归表 + 可选百分位表（p=1%,5%）；与 `cox_regression` 分流 |
| **输入** | 时间列；删失指示；协变量列；complete-case |
| **输出表形** | Regression Table（Coef/SE/Z/P）；Distribution/Shape；Percentile Table（可选）；Diagnostics |
| **Primary URL** | [Equations](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/regression-with-life-data/methods-and-formulas/equations/)（2026-08-28）；[Regression table](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/regression-with-life-data/interpret-the-results/regression-table/)；[Analysis options](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/regression-with-life-data/perform-the-analysis/select-the-analysis-options/) |
| **公式入口** | \(\log Y_p = \beta_0 + \sum \beta_k x_k + \sigma \Phi^{-1}(p)\)；Weibull 下 \(\sigma=1/\text{shape}\)；\(\Phi\) 为标准极值分位 |
| **邻域** | `reliability`、`accelerated_life`、`cox_regression`、`probit_reliability` |
| **UI** | **页1** 时间+删失+协变量；**页2** 分布/百分位选项；**页3** 方法说明；**页4** 预览 |

### 备选替换（仅 Planner 证明原项不可行；仍须满 4 项）

| 备选 id | 场景 |
|---------|------|
| `expanded_gage_unbalanced` 窄化 | 三因子 GLM 被阻塞时 |
| `mixture_process_variable` 窄化 | Binary DOE 被阻塞时 |
| `split_plot_doe` 窄化 | Cluster Variables 被阻塞时 |
| `parallel_plot` / `bubble_plot`（Track G） | 寿命回归被阻塞时——**须**另开 Track G Goal |

---

## §3 导入 / 数据衔接硬约束

1. **complete-case**：无效行不进计算；诊断说明排除原因。  
2. **`source_row`**：残差/拟合值/运行表可回溯。  
3. **A→B**：换文件后旧排除行不串到新文件。  
4. **Binary DOE**：events ≤ trials；trials=0 报门禁；与 `logistic_regression` 命令分流（DOE 因子编码 vs 通用 logistic）。  
5. **Life data**：时间 > 0；删失语义写清；禁止宣称「寿命已达标」。

---

## §4 Wave-9+ 候补队列（本 Goal 不实现）

1. Expanded Gage R&R 不平衡 GLM 全量  
2. Mixture 过程变量联合  
3. GLM 协变量扩展 / Mixed REML  
4. MANOVA / 对应分析  
5. Split-plot DOE  
6. Binary DOE probit/gompit 链扩展  
7. Life data 多分布（lognormal 等）  
8. Parallel / Bubble 图（Track G 另开）  
9. Nonlinear / PLS 回归

---

## §5 调研检查清单（Planner 开工前）

- [ ] Feature List 本波相关行已对照  
- [ ] 每项 ≥1 Primary URL + 访问日期写入 `p8_*.md`  
- [ ] command_id 在 `analysis_commands.cpp` 无冲突  
- [ ] Menu IA：`menu_path` / `menu_group` 已规划  
- [ ] UI 线框：每命令 ≥4 页（层次分离）  
- [ ] 导入影响已评估  
- [ ] 与 W7 `glm_two_way` / `logistic_regression` / `accelerated_life` **无对话框合并**

---

**文档状态：** 2026-08-28 首版；供 Wave-8 `/goal` 锁定。
