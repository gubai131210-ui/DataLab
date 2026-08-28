# 算法 Wave-9 市场对照与公式入口（2026-08-28）

> 访问日期：2026-08-28（UTC+8）  
> 用途：给 **算法 Wave-9 `/goal`** 的 Primary URL、公式口径、Minitab 表形参考；**本文件不写代码、不填 golden**。  
> 状态权威：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md)  
> 执行计划：[`goal-wave-2026-08-28-algorithm-wave9-plan-and-mega-prompt.md`](goal-wave-2026-08-28-algorithm-wave9-plan-and-mega-prompt.md)  
> 前置水位：Wave-8 ✅（`binary_response_doe` / `cluster_variables` / `glm_three_factor` / `life_data_regression`）

---

## §0 一句话水位（给新对话）

| 水位 | 状态 |
|------|------|
| P0–P2 + Wave-2～8 | ✅ / ⚪（勿重做） |
| Wave-8 | 二值 DOE · 变量聚类 · 三因子 GLM · 寿命回归 ✅ |
| Expanded Gage 平衡三因子 | ✅ `expanded_gage_rr`；**不平衡 GLM** ⚪ |
| Mixture 分析（纯组分） | ✅ W7 `mixture_analyze`；**过程变量联合** ❌ |
| Split-plot DOE 分析 | ❌ |
| MANOVA | ❌（General MANOVA 全量延后） |
| **本 Wave 锁定** | **4 项竖切**：不平衡 Expanded Gage · Split-plot 分析 · Mixture+过程变量 · 单因子 MANOVA |

**「实现所有算法」在本产品的正确含义：**  
按 backlog 产品范围内 ❌ **多 Wave 滚动清空**，**不是**单次 Goal 克隆 Minitab Feature List 100%。本 Goal 锁定 **4 项完整竖切**；其余进 §4 候补队列供 Wave-10+。

---

## §1 Minitab 市场对照摘要（2026-08-28 再访）

Primary：[Minitab Feature List](https://www.minitab.com/en-us/products/minitab/features/)（访问 2026-08-28）

### 1.1 本 Wave 要吃的缝

| 模块 | Wave-8 后仍 ❌ / 🟡 | Wave-9 |
|------|---------------------|--------|
| MSA — Expanded Gage R&R | ⚪ 不平衡/固定/嵌套 GLM | ✅ W9-1 `expanded_gage_unbalanced` |
| DOE — Split-plot 分析 | ❌ | ✅ W9-2 `split_plot_analyze` |
| DOE — Mixture + 过程变量 | ❌ | ✅ W9-3 `mixture_process_variable` |
| ANOVA — MANOVA | ❌ | ✅ W9-4 `manova_one_way`（单因子窄化） |

### 1.2 Minitab 可学习的非算法名优势

| 优势 | Wave-9 要求 |
|------|-------------|
| 不平衡 MSA 用 GLM 方差分量 | Expanded Gage 表形：VarComp / %Contribution / %Study Var |
| Split-plot 双误差项 | WP Error vs SP Error；硬改因子 F 用 WP MS |
| Mixture×过程变量交互 | Scheffé + 线性过程项 + 组分×过程交互 |
| MANOVA 四检验 | Wilks / Pillai / LH / Roy；H/E SSCP 矩阵 |
| 帮助公式可读 | 每项 `p9_*.md` + help 禁止「见 md」 |

### 1.3 刻意不做（本 Goal）

见 [`deferred-capability-agreement.md`](deferred-capability-agreement.md)：Mixed REML 全量、General MANOVA 协变量全量、TreeNet/AutoML、嵌 Python/R、Minitab golden、Mixture extreme-vertices 设计生成、Graph Builder 全量、Binary DOE probit/gompit 链扩展、寿命多分布全量、Parallel/Bubble 图（Track G 另开）。

---

## §2 Wave-9 锁定项（W9-1～W9-4）

### W9-1 `expanded_gage_unbalanced` — 不平衡 Expanded Gage R&R（GLM 窄化）

| 字段 | 内容 |
|------|------|
| **动机** | backlog §7 Expanded Gage ⚪「不平衡/固定/嵌套 GLM」；汽车 MSA 常见不等重复 |
| **窄化** | Part + Operator + 可选第 3 随机因子；**不平衡**（不等 Part×Operator 重复）；随机效应 GLM → ANOVA 表 → 方差分量；%GRR / %Study Var / NDC；**≠** 平衡 `expanded_gage_rr` 对话框 |
| **输入** | 测量列；Part；Operator；可选附加因子；complete-case |
| **输出表形** | Gage R&R VarComp 表；%Contribution；Study Var / %Study Var / %Tolerance（可选公差）；NDC |
| **Primary URL** | [Methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/expanded-gage-r-r-study/methods-and-formulas/methods-and-formulas/)（2026-08-28）；[Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/expanded-gage-r-r-study/before-you-start/overview/)；[Unbalanced blog](https://blog.minitab.com/en/blog/quality-data-analysis-and-statistics/unbalanced-designs-and-gage-randr) |
| **公式入口** | Fit GLM → Adj SS → VarComp；随机项 MS 估计；固定项 \( \text{VarComp}=\sum \text{coef}_j^2 \)；%Contribution = VarComp / Total |
| **邻域** | `expanded_gage_rr`（平衡）、`glm_three_factor`（W8）、`gage_rr`、`emp_crossed` |
| **UI** | **页1** Part/Operator/测量/附加因子；**页2** 随机/固定/嵌套声明；**页3** GLM/方差分量方法；**页4** 预览 |

### W9-2 `split_plot_analyze` — 裂区析因分析（窄化）

| 字段 | 内容 |
|------|------|
| **动机** | backlog §8 Split-plot 仍 ❌；汽车 DOE 常见难改/易改因子 |
| **窄化** | 1 个难改因子（whole-plot）+ 1～2 个易改因子（sub-plot）；2 水平编码；**双误差项** WP/SP；平衡或轻度不平衡；主效应 + 可选交互；**不做** 二值响应裂区 |
| **输入** | 响应；难改因子列；易改因子列；whole-plot 指示列（或自动生成）；complete-case + `source_row` |
| **输出表形** | ANOVA（含 WP Error / SP Error 行）；Effects/Coefficients；Fits/Residuals；可选 whole-plot residuals |
| **Primary URL** | [Factorial ANOVA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-factorial-design/methods-and-formulas/analysis-of-variance/)（2026-08-28）；[Model information / WP columns](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/analyze-factorial-design/methods-and-formulas/model-information/)；[Split-plot blog](https://blog.minitab.com/en/blog/how-to-analyze-a-split-plot-experiment) |
| **公式入口** | 平衡裂区：难改因子 F 分母 = MS(WP Error)；易改因子 F 分母 = MS(SP Error)；\( \text{SS}_{WP} \)、\( \text{SS}_{SP} \) 分区 |
| **邻域** | `doe_factorial`、`glm_two_way`、`analyze_variability`、`taguchi_analyze` |
| **UI** | **页1** 响应 + 难改/易改因子 + WP 列；**页2** 模型（主效应/交互）；**页3** 双误差项方法说明；**页4** 预览 |

### W9-3 `mixture_process_variable` — 混合物 + 过程变量分析（窄化）

| 字段 | 内容 |
|------|------|
| **动机** | W7 `mixture_analyze` 仅纯 Scheffé 组分；Minitab Mixture 可含过程变量 |
| **窄化** | 2～4 组分（比例和≈1）；**1 个过程变量**（连续，可标准化）；线性 Scheffé + 线性过程项 + 可选组分×过程交互；**不做** extreme-vertices 设计生成、不做 amount 变量 |
| **输入** | 组分列；响应；过程变量列；complete-case |
| **输出表形** | Coefficients；ANOVA（Adj SS）；R²；Fits/Residuals；诊断 |
| **Primary URL** | [Analyze Mixture Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/mixtures/analyze-mixture-design/before-you-start/overview/)（2026-08-28）；[Model terms / process](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/mixtures/analyze-mixture-design/perform-the-analysis/specify-the-model-terms/)；[ANOVA methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/mixtures/analyze-mixture-design/methods-and-formulas/analysis-of-variance/)；[Example Fondue](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/mixtures/analyze-mixture-design/before-you-start/example/) |
| **公式入口** | Scheffé：\( Y = \sum b_i x_i + \sum\sum b_{ij}x_ix_j + \gamma X_1 + \sum \delta_i x_i X_1 \)；约束 \( \sum x_i = 1 \) |
| **邻域** | `mixture_analyze`（W7）、`mixture_design`（W6）、`rsm_response` |
| **UI** | **页1** 组分 + 响应 + 过程变量；**页2** 模型阶（线性/二次组分 + 过程交互开关）；**页3** Scheffé 方法；**页4** 预览 |

### W9-4 `manova_one_way` — 单因子 MANOVA（窄化）

| 字段 | 内容 |
|------|------|
| **动机** | backlog §4/§10 MANOVA ❌；汽车质量常见多响应同批比较 |
| **窄化** | **2～4 连续响应**；**1 个分类因子**（≥2 水平）；complete-case；Wilks / Pillai / Lawley-Hotelling / Roy；H/E SSCP；**不做** 多因子 General MANOVA、不做协变量 |
| **输入** | 2～4 响应列；1 因子列；complete-case + `source_row` |
| **输出表形** | MANOVA Test Table（四检验 Stat/F/P）；Group Mean Vectors；Eigen analysis（可选）；Univariate ANOVA 附表（每响应一行） |
| **Primary URL** | [Understanding MANOVA](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/supporting-topics/basics/understanding-manova/)（2026-08-28）；[MANOVA tests formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/general-manova/methods-and-formulas/manova-tests/)；[MANOVA test table](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/anova/how-to/general-manova/interpret-the-results/all-statistics-and-graphs/manova-test-table/) |
| **公式入口** | \( \mathbf{H} = \sum_g n_g (\bar{\mathbf{y}}_g - \bar{\mathbf{y}})(\cdot)' \)；\( \mathbf{E} \) 组内 SSCP；\( \lambda_i \) 为 \( \mathbf{E}^{-1}\mathbf{H} \) 特征值；Wilks \( \Lambda = \prod 1/(1+\lambda_i) \) |
| **邻域** | `glm_three_factor`（W8）、`discriminant`、`hotelling_t2`、`one_way_anova` |
| **UI** | **页1** 多响应 + 因子；**页2** 检验选项（四检验默认全开）；**页3** SSCP/MANOVA 方法；**页4** 预览 |

### 备选替换（仅 Planner 证明原项不可行；仍须满 4 项）

| 备选 id | 场景 |
|---------|------|
| `correspondence_analysis` 窄化 | MANOVA 被阻塞时 |
| `binary_doe_probit` 链扩展 | Split-plot 被阻塞时 |
| `life_data_lognormal` 窄化 | Mixture 过程变量被阻塞时 |
| `parallel_plot`（Track G） | Expanded Gage 被阻塞时——**须**另开 Track G Goal |

---

## §3 导入 / 数据衔接硬约束

1. **complete-case**：无效行不进计算；诊断说明排除原因。  
2. **`source_row`**：残差/拟合值/运行表可回溯。  
3. **A→B**：换文件后旧排除行不串到新文件。  
4. **Expanded Gage**：Part/Operator 水平不足报门禁；不平衡须有可估方差分量。  
5. **Split-plot**：WP 指示与难改因子一致；禁止宣称「设计已最优」。  
6. **Mixture**：组分和在容差内（默认 ±0.05，可配置）；过程变量不必归一。  
7. **MANOVA**：响应须数值；每组样本量 ≥2（否则报门禁）。

---

## §4 Wave-10+ 候补队列（本 Goal 不实现）

1. General MANOVA（多因子 + 协变量）  
2. Mixed REML 全量  
3. Binary DOE probit/gompit 链  
4. Life data 多分布（lognormal 等）  
5. Mixture extreme-vertices 设计  
6. Expanded Gage 固定因子 + 嵌套全量  
7. Correspondence analysis  
8. Nonlinear / PLS 回归  
9. Parallel / Bubble 图（Track G 另开）

---

## §5 调研检查清单（Planner 开工前）

- [ ] Feature List 本波相关行已对照  
- [ ] 每项 ≥1 Primary URL + 访问日期写入 `p9_*.md`  
- [ ] command_id 在 `analysis_commands.cpp` 无冲突  
- [ ] Menu IA：`menu_path` / `menu_group` 已规划  
- [ ] UI 线框：每命令 ≥4 页（层次分离）  
- [ ] 导入影响已评估  
- [ ] 与 `expanded_gage_rr` / `mixture_analyze` / `doe_factorial` / `glm_three_factor` **无对话框合并**

---

**文档状态：** 2026-08-28 首版；供 Wave-9 `/goal` 锁定。
