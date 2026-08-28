# 算法 Wave-7 市场对照与公式入口（2026-08-28）

> 访问日期：2026-08-28（UTC+8）  
> 用途：给 **算法 Wave-7 `/goal`** 的 Primary URL、公式口径、Minitab 表形参考；**本文件不写代码、不填 golden**。  
> 状态权威：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md)  
> 执行计划：[`goal-wave-2026-08-28-algorithm-wave7-plan-and-mega-prompt.md`](goal-wave-2026-08-28-algorithm-wave7-plan-and-mega-prompt.md)

---

## §0 一句话水位（给新对话）

| 水位 | 状态 |
|------|------|
| P0–P2 + Wave-2～6 | ✅ / ⚪（勿重做） |
| Wave-6 | `taguchi_analyze` / `mixture_design` / `nhpp_repairable` / `reliability_test_plan` ✅ |
| G9 公式代入 + Show Your Work | 已落地；本 Wave **不以 G9 为主**，但有公式的新命令应可挂 trace |
| **本 Wave 锁定** | **4 项竖切**：Mixture **分析** · 不平衡 GLM 双因子 · Analyze Variability · 因子分析（窄化） |

**「实现所有算法」在本产品的正确含义：**  
按 backlog 产品范围内 ❌ **多 Wave 滚动清空**，**不是**单次 Goal 克隆 Minitab Feature List 100%。本 Goal 锁定 **4 项完整竖切**；其余进 §4 候补队列供 Wave-8+。

---

## §1 Minitab 市场对照摘要（2026-08-28 再访）

Primary：[Minitab Feature List](https://www.minitab.com/en-us/products/minitab/features/)（访问 2026-08-28）

### 1.1 本 Wave 要吃的缝

| 模块 | 仍 ❌ / 🟡 | Wave-7 |
|------|-----------|--------|
| DOE — Mixture | 设计 ✅（W6）；**分析** ❌ | ✅ W7-1 `mixture_analyze` |
| DOE — Analyze variability | ❌ | ✅ W7-3 `analyze_variability` |
| ANOVA — GLM 不平衡 | ❌（大缝窄化） | ✅ W7-2 `glm_two_way` |
| Multivariate — Factor analysis | ❌ | ✅ W7-4 `factor_analysis` |

### 1.2 Minitab 可学习的非算法名优势

| 优势 | Wave-7 要求 |
|------|-------------|
| 设计→分析分步 | Mixture：W6 设计 + W7 分析 **独立命令/对话框** |
| 位置效应 vs 分散效应 | Analyze Variability：分散模型表 + 可选权重导出说明 |
| 不平衡 ANOVA 用拟合均值 | GLM：输出 Fitted Means，禁止只报原始均值 |
| 多元降维表形 | Factor：Loadings + % Var + Scree |
| 帮助公式可读 | 每项 `p7_*.md` + help 禁止「见 md」 |

### 1.3 刻意不做（本 Goal）

见 [`deferred-capability-agreement.md`](deferred-capability-agreement.md)：Mixed REML 全量、MANOVA、Mixture 过程变量/amount 联合、extreme-vertices、D-opt、binary response DOE 全量、Graph Builder、TreeNet/AutoML、嵌 Python/R、Minitab golden。

---

## §2 Wave-7 锁定项（W7-1～W7-4）

### W7-1 `mixture_analyze` — Analyze Mixture Design（Scheffé 窄化）

| 字段 | 内容 |
|------|------|
| **动机** | W6 已有 `mixture_design` 生成矩阵；市场缺口是 **分析**（系数、ANOVA、残差） |
| **窄化** | 读工作表：分量列 x1..xq（q=3～4）+ 响应列；模型：**线性 Scheffé** 必选，**二次**（AB+AC+BC）可选；**无常数项**；不做过程变量、不做 special cubic 全量 |
| **输入** | 分量比例列（∑≈1 诊断）；响应列；complete-case |
| **输出表形（参考 Minitab）** | Coefficients（Coef/SE/T/P）；ANOVA（Seq/Adj SS）；Fits and Residuals 摘要；Design Info（q、模型阶） |
| **Primary URL** | [Mixture models/terms](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/supporting-topics/mixture-designs/models-terms-and-blending/)（2026-08-28）；[Coefficients](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/mixtures/analyze-mixture-design/methods-and-formulas/coefficients/)；[ANOVA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/mixtures/analyze-mixture-design/methods-and-formulas/analysis-of-variance/) |
| **公式入口** | 无截距：\(Y = \sum_i b_i x_i + \sum_{i<j} b_{ij} x_i x_j\)；OLS：\(\hat b = (X'X)^{-1} X'y\) |
| **邻域** | `mixture_design.*`、`rsm_response`、`doe_factorial` |
| **UI** | **独立**于 `mixture_design`：**页1** 列选择；**页2** 模型阶（linear/quadratic）；**页3** 方法说明；**页4** 预览确认 |

### W7-2 `glm_two_way` — 双因子 GLM（不平衡窄化）

| 字段 | 内容 |
|------|------|
| **动机** | backlog §4 GLM/MANOVA 大缝；汽车质量常见双因子不平衡（缺组合、不等重复） |
| **窄化** | 连续响应；因子 A、B（各自 ≥2 水平）；主效应 + AB 交互；**Type III 调整平方和**；拟合均值（Fitted Means）；Tukey 可选窄化或显式不做并写诊断 |
| **输入** | 响应列 + 两因子列（文本/分类）；complete-case + `source_row` |
| **输出表形** | ANOVA（Adj SS/DF/F/P）；Coefficients；Fitted Means 表；残差诊断 |
| **Primary URL** | [Balanced/unbalanced](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/supporting-topics/anova-models/balanced-and-unbalanced-designs/)（2026-08-28）；[Fit GLM methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/fit-general-linear-model/methods-and-formulas/methods/)；[ANOVA table](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/fit-general-linear-model/interpret-the-results/all-statistics-and-graphs/analysis-of-variance-table/) |
| **公式入口** | \(Y = X\beta + \varepsilon\)；\(\hat\beta=(X'X)^{-1}X'y\)；Type III：项在「其余项已在模型」下的 Adj SS |
| **邻域** | `anova_two_way`（若已有平衡路径）、`linear_regression`、`poisson_regression`（GLM 模式参考） |
| **UI** | **页1** 列选择；**页2** 模型（主效应/交互）；**页3** 方法（不平衡/拟合均值）；**页4** 结果说明 |

### W7-3 `analyze_variability` — DOE 分散效应（2 水平窄化）

| 字段 | 内容 |
|------|------|
| **动机** | Feature List DOE「Analyze variability」仍 ❌；稳健制程需同时看均值与标准差 |
| **窄化** | 2 水平全因子或部分析因；每运行 **重复/再现** 测得响应；先算每运行 \(s_i\)；分散模型 \(\ln\sigma = A\gamma\)（log link）；LSE 加权回归；输出分散效应表；**可选**存储权重说明（供后续析因位置模型，本波可不接自动权重链） |
| **输入** | 因子列（2 水平编码或文本）；响应重复列（多列）或长表（运行+重复）；complete-case |
| **输出表形** | Std Dev by Run；Dispersion Effects/Coefficients；ANOVA（分散）；Diagnostics |
| **Primary URL** | [Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-variability/before-you-start/overview/)（2026-08-28）；[Model information](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-variability/methods-and-formulas/model-information/)；[Location vs dispersion](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/supporting-topics/factorial-and-screening-designs/analyze-location-effects-and-dispersion-effects/) |
| **公式入口** | \( \ln s_i = \sum_j \gamma_j z_{ij}\)；2 水平效应 \(= 2\times\) 系数；权重 \(w_i \propto 1/\hat\sigma_i^2\) |
| **邻域** | `doe_factorial`、`rsm_response`、`taguchi_analyze`（勿混） |
| **UI** | **页1** 因子+重复列布局；**页2** 估计方法（LSE/MLE 择一主路径）；**页3** 方法说明；**页4** 预览 |

### W7-4 `factor_analysis` — 因子分析（主成分提取窄化）

| 字段 | 内容 |
|------|------|
| **动机** | 多元统计缝；与已有 PCA 经验 T² 区分——本命令是 **探索性因子分析表形** |
| **窄化** | 原始数据矩阵；相关矩阵特征分解；**主成分提取**；前 m 因子（Kaiser λ>1 或用户指定）；**Varimax 旋转可选**；Loadings、% Var、Communalities；Scree PlotSpec |
| **输入** | ≥3 数值列；complete-case；标准化相关矩阵 |
| **输出表形** | Factor Loadings；Variance Explained；Communalities；Scree 图 |
| **Primary URL** | [FA methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/factor-analysis/methods-and-formulas/methods-and-formulas/)（2026-08-28）；[PCA vs FA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/supporting-topics/principal-components-and-factor-analysis/differences-between-pca-and-factor-analysis/) |
| **公式入口** | 相关阵 \(R\) 特征对 \((\lambda_i,e_i)\)；载荷 \(L_{:,i}=\sqrt{\lambda_i}\,e_i\)（未旋转）；旋转后重新解释载荷 |
| **邻域** | `pca` / Hotelling、`kmeans`、`discriminant` |
| **UI** | **页1** 变量选择；**页2** 提取/旋转选项；**页3** 方法说明；**页4** 预览 |

### 备选替换（仅 Planner 证明原项不可行；仍须满 4 项）

| 备选 id | 场景 |
|---------|------|
| `cluster_variables` | FA 被阻塞时 |
| `binary_response_doe` 窄化 | Analyze variability 被阻塞时 |
| `life_data_regression` 窄化 | GLM 被阻塞时 |
| `bubble_plot` 或 `parallel_plot`（Track G） | 多元被阻塞时——**须**另开 Track G Goal，不与本算法 Wave 混减项 |

---

## §3 导入 / 数据衔接硬约束

1. **complete-case**：无效行不进计算；诊断说明排除原因。  
2. **`source_row`**：残差/拟合值图可回溯。  
3. **A→B**：换文件后旧排除行不串到新文件。  
4. **Mixture 分析**：分量列 ∑ 偏离 1 报诊断；禁止静默归一化除非用户显式选项。  
5. **Analyze variability**：重复 vs 再现语义须在 help 写清；缺重复报门禁失败。

---

## §4 Wave-8+ 候补队列（本 Goal 不实现）

1. Cluster Variables  
2. Binary response DOE / Logistic DOE 分析  
3. GLM 三因子 / 协变量扩展  
4. Mixture 过程变量联合  
5. Expanded Gage 不平衡 GLM  
6. Parallel / Bubble 图（Track G 另开）  
7. 多失效模式 / 寿命回归加宽  
8. Nonlinear / PLS 回归

---

## §5 调研检查清单（Planner 开工前）

- [ ] Feature List 本波相关行已对照  
- [ ] 每项 ≥1 Primary URL + 访问日期写入 `p7_*.md`  
- [ ] command_id 在 `analysis_commands.cpp` 无冲突  
- [ ] Menu IA：`menu_path` / `menu_group` 已规划  
- [ ] UI 线框：每命令 ≥3 页（选项/方法/结果层次分离）  
- [ ] 导入影响已评估  
- [ ] 与 W6 `mixture_design` 命令 **无对话框合并**

---

**文档状态：** 2026-08-28 首版；供 Wave-7 `/goal` 锁定。
