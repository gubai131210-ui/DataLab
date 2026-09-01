# 算法 Wave-10 市场对照与公式入口（2026-08-31）

> 访问日期：2026-08-31（UTC+8）  
> 用途：给 **算法 Wave-10 `/goal`** 的 Primary URL、公式口径、Minitab 表形参考；**本文件不写代码、不填 golden**。  
> 状态权威：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md)  
> 执行计划：[`goal-wave-2026-08-31-algorithm-wave10-plan-and-mega-prompt.md`](goal-wave-2026-08-31-algorithm-wave10-plan-and-mega-prompt.md)  
> 前置水位：Wave-9 ✅（`expanded_gage_unbalanced` / `split_plot_analyze` / `mixture_process_variable` / `manova_one_way`）

---

## §0 一句话水位（给新对话）

| 水位 | 状态 |
|------|------|
| P0–P2 + Wave-2～9 | ✅ / ⚪（勿重做） |
| Wave-9 | 不平衡 Gage · 裂区 · Mixture 过程变量 · 单因子 MANOVA ✅ |
| General MANOVA（多因子/协变量） | ❌ |
| Mixed REML（混合效应） | ❌ |
| Binary DOE probit/gompit 链 | 🟡（W8 仅 Logit） |
| 寿命回归多分布（Lognormal 等） | 🟡（W8 仅 Weibull 窄化） |
| **本 Wave 锁定** | **4 项竖切**：General MANOVA · Mixed REML · Binary DOE Probit · Life Lognormal |

**「实现所有算法」在本产品的正确含义：**  
按 backlog 产品范围内 ❌ **多 Wave 滚动清空**，**不是**单次 Goal 克隆 Minitab Feature List 100%。本 Goal 锁定 **4 项完整竖切**；其余进 §4 候补队列供 Wave-11+。

---

## §1 Minitab 市场对照摘要（2026-08-31 再访）

Primary：[Minitab Feature List](https://www.minitab.com/en-us/products/minitab/features/)（访问 2026-08-31）

### 1.1 本 Wave 要吃的缝

| 模块 | Wave-9 后仍 ❌ / 🟡 | Wave-10 |
|------|---------------------|---------|
| ANOVA — General MANOVA | ❌（W9 仅单因子窄化） | ✅ W10-1 `general_manova` |
| ANOVA — Mixed models REML | ❌ | ✅ W10-2 `mixed_effects_reml` |
| DOE — Binary response probit/gompit | 🟡 W8 Logit only | ✅ W10-3 `binary_doe_probit` |
| 可靠性 — Regression with Life Data 多分布 | 🟡 W8 Weibull only | ✅ W10-4 `life_data_lognormal` |

### 1.2 Minitab 可学习的非算法名优势

| 优势 | Wave-10 要求 |
|------|-------------|
| General MANOVA 多响应 + 多因子 + 协变量 | H/E SSCP；四 multivariate 检验；≠ `manova_one_way` 对话框 |
| Mixed REML 随机因子 + 固定因子 | 默认 REML；方差分量；BLUP 可选窄化 |
| Binary DOE 多 link | Logit / Probit(normit) / Gompit；Events/Trials 或 0/1 |
| Life regression 多分布 | Lognormal：log(Y) ~ Normal；MLE + 删失 |

### 1.3 刻意不做（本 Goal）

见 [`deferred-capability-agreement.md`](deferred-capability-agreement.md)：

- Mixed REML **全量**（多随机项嵌套/交叉/全协方差结构）  
- General MANOVA **全量**（任意高阶交互、stored model 预测链）  
- TreeNet / AutoML / 嵌 R/Python  
- Minitab 数值 golden  
- Graph Builder 全量、Parallel/Bubble（Track G 另开）  
- Multiple correspondence analysis 全量  
- Mixture extreme-vertices 设计生成  
- 破坏 Wave-2～9 已 ✅ 算法与导入契约  

---

## §2 Wave-10 锁定项（W10-1～W10-4）

### W10-1 `general_manova` — General MANOVA（窄化）

| 字段 | 内容 |
|------|------|
| **动机** | backlog §4/§10 MANOVA 大缝；W9 `manova_one_way` 仅单因子 |
| **窄化** | **2～4 连续响应**；**1～2 分类固定因子**；**可选 1 连续协变量**；主效应 + 可选因子交互（仅 2 因子时）；Wilks/Pillai/LH/Roy；H/E SSCP；**不做** 3 因子以上、不做 stored model 预测 |
| **输入** | 响应列 2～4；因子列 1～2；协变量列 0～1；complete-case + `source_row` |
| **输出表形** | MANOVA Test Table；Group Mean Vectors；Eigen analysis；Univariate ANOVA 附表（每响应） |
| **Primary URL** | [General MANOVA Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/general-manova/before-you-start/overview/)（2026-08-31）；[MANOVA tests](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/general-manova/methods-and-formulas/manova-tests/)；[Data considerations](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/general-manova/before-you-start/data-considerations/) |
| **公式入口** | 复用 W9：\( \mathbf{H}, \mathbf{E} \)；\( \lambda_i \) of \( \mathbf{E}^{-1}\mathbf{H} \)；多因子时 Type III 分区 SSCP |
| **邻域** | `manova_one_way`（W9）、`glm_three_factor`（W8）、`one_way_anova` |
| **UI** | **页1** 多响应 + 因子（1～2）；**页2** 协变量 + 模型交互；**页3** 四检验 + SSCP 方法；**页4** 预览 |

### W10-2 `mixed_effects_reml` — 混合效应 REML（窄化）

| 字段 | 内容 |
|------|------|
| **动机** | backlog GLM/Mixed ❌；汽车 DOE/MSA 常见随机区组、批次 |
| **窄化** | **1 个随机因子** + **1～2 固定因子** + **可选 1 协变量**；连续响应；**REML** 估计方差分量；固定效应 BLUE；**不做** 多随机项、不做 ML 切换、不做 stored model |
| **输入** | 响应；随机因子列；固定因子列；协变量可选；complete-case |
| **输出表形** | Variance Components；Fixed Effects（Coef/SE/t/P）；ANOVA Type III（固定项）；Random effect BLUP 可选表 |
| **Primary URL** | [Mixed Effects Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/mixed-effects-model/before-you-start/overview/)（2026-08-31）；[Methods REML](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/mixed-effects-model/methods-and-formulas/methods/)；[Variance components](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/mixed-effects-model/methods-and-formulas/variance-components/) |
| **公式入口** | \( y = X\beta + Z\mu + \varepsilon \)；\( V = \sigma^2 I + \sigma_u^2 ZZ' \)；REML 最大化 restricted log-likelihood |
| **邻域** | `glm_three_factor`、`glm_two_way`、`expanded_gage_unbalanced`（随机效应概念） |
| **UI** | **页1** 响应 + 随机因子；**页2** 固定因子 + 协变量；**页3** REML 方法说明；**页4** 预览 |

### W10-3 `binary_doe_probit` — 析因二值响应 Probit/Gompit（窄化）

| 字段 | 内容 |
|------|------|
| **动机** | W8 `binary_response_doe` 仅 Logit；Minitab Binary Response 支持 normit/gompit |
| **窄化** | 2～4 水平因子析因；Events/Trials 或 0/1 展开；**probit** 或 **gompit** link；IRWLS；系数/OR/拟合；**独立**对话框 ≠ `binary_response_doe` |
| **输入** | 因子列；Events+Trials 或 二值响应；complete-case |
| **输出表形** | Coefficients；Odds Ratio（或 probit 边际效应说明）；ANOVA（Adj SS）；Fits；Diagnostics |
| **Primary URL** | [Binary Logistic Methods / link functions](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-binary-logistic-model/methods-and-formulas/methods/)（2026-08-31）；[Analyze Binary Response Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-binary-response/before-you-start/overview/) |
| **公式入口** | Probit：\( g(\mu) = \Phi^{-1}(\mu) \)；Gompit：\( g(\mu) = \log(-\log(1-\mu)) \)；IRWLS 权重 |
| **邻域** | `binary_response_doe`（W8）、`doe_factorial` |
| **UI** | **页1** 因子 + 响应布局；**页2** Link（probit/gompit）；**页3** IRWLS 方法；**页4** 预览 |

### W10-4 `life_data_lognormal` — 寿命数据 Lognormal 回归（窄化）

| 字段 | 内容 |
|------|------|
| **动机** | W8 `life_data_regression` 仅 Weibull；Minitab Regression with Life Data 支持 Lognormal |
| **窄化** | Lognormal 分布；**1～2 协变量** + 截距；右删失/精确失效；MLE；回归表 + 百分位；**独立**对话框 ≠ `life_data_regression` |
| **输入** | 时间列；删失指示；协变量 0～2；complete-case + `source_row` |
| **输出表形** | Regression Table（log scale）；Percentiles；Fits；Distribution parameter（μ, σ on log scale） |
| **Primary URL** | [Regression with Life Data Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/regression-with-life-data/before-you-start/overview/)（2026-08-31）；[Equations / lognormal](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/regression-with-life-data/methods-and-formulas/equations/)；[Regression table](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/regression-with-life-data/interpret-the-results/regression-table/) |
| **公式入口** | \( \log(Y_p) = \beta_0 + \sum \beta_k x_k + \sigma \Phi^{-1}(p) \)；MLE with censoring |
| **邻域** | `life_data_regression`（W8）、`accelerated_life`、`reliability` |
| **UI** | **页1** 时间 + 删失 + 协变量；**页2** Lognormal 选项；**页3** MLE 方法；**页4** 预览 |

### 备选替换（仅 Planner 证明原项不可行；仍须满 4 项）

| 备选 id | 场景 |
|---------|------|
| `correspondence_analysis` 窄化 | General MANOVA 被阻塞 |
| `nested_anova` 窄化 | Mixed REML 被阻塞 |
| `binary_doe_gompit` 仅链 | 与 probit 合并为单命令双 link 时 |
| `life_data_extreme_value` 窄化 | Lognormal 被阻塞 |
| `parallel_plot`（Track G） | 须另开 Track G Goal |

---

## §3 导入 / 数据衔接硬约束

1. **complete-case**：无效行不进计算；诊断说明排除原因。  
2. **`source_row`**：拟合/残差/运行表可回溯。  
3. **A→B**：换文件后旧排除行不串到新文件。  
4. **General MANOVA**：每组每响应样本量 ≥2；响应须连续。  
5. **Mixed REML**：随机因子 ≥2 水平；收敛失败报 diagnostic。  
6. **Binary DOE Probit**：Trials>0；事件≤试验；link 与 logit 命令分离。  
7. **Life Lognormal**：时间>0；删失契约与 W8 一致。  

---

## §4 Wave-11+ 候补队列（本 Goal 不实现）

1. Simple / Multiple Correspondence Analysis  
2. Mixed REML 多随机项 / 嵌套全量  
3. Nonlinear regression / PLS  
4. Mixture extreme-vertices 设计  
5. Expanded Gage 固定+嵌套全量  
6. Binary DOE 与 split-plot 交叉（Minitab 不支持，产品亦不做）  
7. Parallel / Bubble / Mosaic 图（Track G）  
8. Cox regression 全量  
9. Definitive Screening Designs  

---

## §5 调研检查清单（Planner 开工前）

- [ ] Feature List 本波相关行已对照  
- [ ] 每项 ≥1 Primary URL + 访问日期写入 `p10_*.md`  
- [ ] command_id 在 `analysis_commands.cpp` 无冲突  
- [ ] Menu IA：`menu_path` / `menu_group` 已规划  
- [ ] UI 线框：每命令 ≥4 页（层次分离）  
- [ ] 导入影响已评估  
- [ ] 与 W9/W8 邻域命令 **无对话框合并**  

---

**文档状态：** 2026-08-31 初稿；供 Wave-10 `/goal` 锁定。
