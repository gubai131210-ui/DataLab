# 算法 Wave-6 市场对照与公式入口（2026-08-25）

> 访问日期：2026-08-25（UTC+8）  
> 用途：给 **算法 Wave-6 `/goal`** 的 Primary URL、公式口径、Minitab 表形参考；**本文件不写代码、不填 golden**。  
> 状态权威：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md)  
> 候选池：[`next-wave-algorithms-charts-ml-oss.md`](next-wave-algorithms-charts-ml-oss.md)  
> 执行计划：[`goal-wave-2026-08-25-algorithm-wave6-plan-and-mega-prompt.md`](goal-wave-2026-08-25-algorithm-wave6-plan-and-mega-prompt.md)

---

## §0 一句话水位（给新对话）

| 水位 | 状态 |
|------|------|
| P0–P2 主项 + Wave-2～5 | ✅ / ⚪（勿重做） |
| Wave-5 | `random_forest` / `weibayes` / `taguchi_orthogonal_design` / `distribution_calculator` ✅ |
| G9 公式代入 + Show Your Work 深化 | 已落地（验算轨迹 / 分步求值）；本 Wave **不以 G9 为主** |
| 本 Wave 锁定 | **4 项竖切**：Taguchi 分析 · Mixture 设计 · NHPP 可修复 · 可靠性试验计划 |

**现实约束（必须写进 Goal）：**  
「把所有算法都实现」= **多 Wave 清空产品范围内 ❌**，**不是**单次 Goal 克隆 Minitab Feature List 100%。单次 Goal 仍锁定 **4 项完整竖切**；其余进 §4 候补队列供 Wave-7+。

---

## §1 Minitab 市场对照摘要（2026-08-25 再访）

Primary：[Minitab Feature List](https://www.minitab.com/en-us/products/minitab/features/)（访问 2026-08-25）

### 1.1 DataLab 相对 Feature List：仍缺口（产品范围内）

| 模块 | 仍 ❌ / 🟡（精选） | Wave-6 是否吃掉 |
|------|-------------------|-----------------|
| DOE | Mixture；Taguchi **分析**（设计已有）；Analyze variability / binary DOE；Split-plot；D-opt；DSD ⏸ | ✅ Taguchi 分析 + Mixture 设计 |
| Reliability | Test plans；Repairable systems；多失效模式；寿命回归全量；左删失参数化深化 | ✅ Test plan + NHPP |
| ANOVA | GLM 不平衡；Mixed；MANOVA | 候补（缝大） |
| Multivariate | Factor analysis；Cluster Variables；Correspondence | 候补 |
| Regression | Nonlinear / Orthogonal / PLS | 多数 ⏸ 或低优先 |
| Graphics | parallel / bubble / mosaic；Graph Builder 全量 ⏸ | Track G 另开 |
| Predictive | TreeNet / AutoML / 商标级 RF | **不做** |
| Macros | Python/R 嵌入 | **不做** |

### 1.2 Minitab 可学习的「非算法名」优势（产品补齐方向）

| 优势 | DataLab 现状 | Wave-6 要求 |
|------|--------------|-------------|
| 方法选项可切换 + 表形对齐 | 🟡 多数分析有 | 每项至少 1 个方法/阵列选项；输出表对齐 Minitab **结构**（非数值 golden） |
| 帮助公式可读 + 来源链接 | ✅「公式与来源」；G9 代入 | 每项 `p6_*.md` Primary URL；help 禁止「见 md」 |
| 表+图同页 | 🟡 | 有图则 PlotSpec；无可图须显式 L1/notes |
| 小样本 / 假设警告 | 🟡 | 诊断消息进 Facts；解释禁止「已证明稳定/合格」 |
| 设计生成 → 写入工作表 → 再分析 | Taguchi 设计已有 | Mixture 同样必须 **可写入工作表**；Taguchi 分析读已有列 |
| UI 分层 | Menu IA 四顶层 | **对话框多页**：选项 / 结果表 / 图 / 方法说明分页；禁止单页堆控件 |

### 1.3 刻意不做（本 Goal 与后续）

见 [`deferred-capability-agreement.md`](deferred-capability-agreement.md)：Graph Builder 全量、可旋转 3D、Assistant、嵌 Python/R、Automated Capability、Blaker、TreeNet/AutoML、Minitab 数值 golden。

---

## §2 Wave-6 锁定项（W6-1～W6-4）

### W6-1 `taguchi_analyze` — Analyze Taguchi Design（静态窄化）

| 字段 | 内容 |
|------|------|
| **动机** | Wave-5 已有 `taguchi_orthogonal_design`；市场缺口是 **分析**（S/N、均值响应表、主效应） |
| **窄化** | 静态设计；S/N 类型：`larger_is_better` / `smaller_is_better` / `nominal_is_best`（至少 2 种起步）；因子水平均值表 + Delta/Rank；不做动态 Taguchi、不做噪声因子全交叉全量 |
| **输入** | 工作表：响应列（可多重复列或已算均值列）+ 因子列（来自正交表） |
| **输出表形（参考 Minitab）** | Response Table for S/N；Response Table for Means；Estimated Coefficients（可选窄化）；Main Effects 图 |
| **Primary URL** | [Taguchi designs](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/supporting-topics/taguchi-designs/taguchi-designs/)；[Analyze Taguchi example](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/taguchi/analyze-taguchi-design/before-you-start/example-of-analyzing-a-static-design/)（访问 2026-08-25） |
| **公式入口** | Larger-is-better：`-10·log10(mean(1/y²))`；Smaller-is-better：`-10·log10(mean(y²))`；Nominal-is-best（常用）：`10·log10(ȳ²/s²)`（在 research 中写清所选变体） |
| **邻域代码** | `taguchi_orthogonal.*`、`doe_*`、`rsm_response` |
| **UI** | 独立对话框：页1 列选择；页2 S/N 与选项；页3 结果预览说明（可选）——**禁止**把设计生成与分析塞进同一页 |

### W6-2 `mixture_design` — Mixture 设计生成（simplex 窄化）

| 字段 | 内容 |
|------|------|
| **动机** | Feature List DOE「Mixture designs」仍 ❌；汽车/材料配比场景高频 |
| **窄化** | 分量数 q=3～4；**simplex-lattice**（degree 2 或 3 择一）或 **simplex-centroid** 择一实现并文档化；∑xᵢ=1；写入工作表列 `x1..xq` + 可选 RunOrder；不做 extreme-vertices / D-opt / 过程变量联合 |
| **Primary URL** | [NIST 5.5.4 Mixture](https://www.itl.nist.gov/div898/handbook/pri/section5/pri54.htm)；[Simplex-lattice](https://www.itl.nist.gov/div898/handbook/pri/section5/pri542.htm)；Feature List DOE（访问 2026-08-25） |
| **公式/设计点** | Lattice：各分量取 {0, 1/m, …, 1} 且和为 1；Centroid：单纯形顶点、边中点、整体形心等（按所选类型写死点集表） |
| **邻域** | `doe_factorial` / `doe_plackett_burman` / `taguchi_orthogonal` |
| **UI** | 设计生成页：分量数、阵列类型、写入列名前缀；**另页**显示设计矩阵预览；禁止与「分析 Mixture」同对话框（分析可列为 Wave-7） |

### W6-3 `nhpp_repairable` — 可修复系统 NHPP（幂律窄化）

| 字段 | 内容 |
|------|------|
| **动机** | Feature List「Repairable systems」仍 ❌；质量现场维修/ROCOF 常见 |
| **窄化** | Power-law NHPP（Crow–AMSAA / Duane 相关）；输入累积失效时间；估计 β、λ（或 η）；强度/累积均值函数表；可选 Duane 图；不做 HPP 以外多模型全家桶、不做复杂系统 RBD |
| **Primary URL** | [NIST 8.1.7 Repair rate](https://www.itl.nist.gov/div898/handbook/apr/section1/apr17.htm)；[NHPP power law](https://www.itl.nist.gov/div898/handbook/apr/section1/apr172.htm)；[Reliability growth NHPP](https://www.itl.nist.gov/div898/handbook/apr/section1/apr191.htm)（访问 2026-08-25） |
| **公式入口** | λ(t)=λβ t^{β−1}；M(t)=λ t^β；MLE / 最小二乘择一并在 md 写清 |
| **邻域** | `reliability` / `accelerated_life` / `weibayes` / `reliability_warranty` |
| **UI** | 页1：时间列 + 删失/截尾选项；页2：模型与估计方法；结果页展示参数表+图 |

### W6-4 `reliability_test_plan` — 可靠性试验/验证样本量（窄化）

| 字段 | 内容 |
|------|------|
| **动机** | Feature List「Test plans」仍 ❌；与 Weibull/Weibayes 形成「计划→试验→分析」链 |
| **窄化** | 演示型（demonstration）：给定 Weibull 形状 β（已知或假设）、可靠度 R、置信水平、试验时间/截尾规则之一，计算所需样本量 n 或允许失效数；输出计划表；不做全部分布族、不做复杂多阶段贝叶斯计划 |
| **Primary URL** | Feature List Reliability「Test plans」；NIST APR 试验/样本量相关章节（Planner 须在实现前钉死 1～2 条具体 Methods-and-Formulas 或 NIST 页并写入 `p6_reliability_test_plan.md`）；可参考 Minitab Reliability Module 帮助中 Test Plan 输出表形 |
| **邻域** | `t_power`、`weibayes`、`reliability` |
| **UI** | 工具型对话框：目标可靠度 / β / 置信度 / 试验时长；**结果页**单独显示 n 与假设摘要；禁止与分布拟合同页 |

### 备选替换（仅当 Planner 证明原项不可行；仍须满 4 项）

| 备选 id | 场景 |
|---------|------|
| `analyze_variability_doe` | Taguchi 分析被阻塞时 |
| `cluster_variables` | Mixture 设计被阻塞时 |
| `life_data_regression` 窄化 | NHPP 被阻塞时 |
| `bootstrap_randomization` 或 `factor_analysis` 窄化 | Test plan 公式源不足时 |

---

## §3 导入 / 数据衔接硬约束

每项必须满足（Tester 回归）：

1. **complete-case**：无效行不进计算；诊断说明排除原因。  
2. **`source_row`**：点图/残差/运行行可回溯。  
3. **A→B**：换文件后旧排除行不串到新文件。  
4. 设计生成类（Mixture）：写入新列后不破坏既有列类型；随机化种子可复现。  
5. Taguchi 分析：因子列与响应列长度对齐；缺失水平报诊断而非静默。

---

## §4 Wave-7+ 候补队列（本 Goal 不实现，但登记）

按汽车质量杠杆粗排：

1. Mixture **分析**（Scheffé 线性/二次）  
2. GLM 不平衡（窄化 two-way）  
3. Factor analysis / Cluster Variables  
4. Analyze variability / binary response DOE  
5. 多失效模式 / 寿命回归加宽  
6. Parallel / Bubble 图（Track G）  
7. Expanded Gage 不平衡 GLM ⚪→✅  
8. Nonlinear regression（低优先）

---

## §5 开源灵感（只借能力，不抄栈）

| 灵感 | 用于 Wave-6 |
|------|-------------|
| Tinker DOE：设计矩阵预览 UX | Mixture / Taguchi 预览页 |
| mfgqc：假设守卫文案 | NHPP / Test plan 诊断 |
| Cassini：Show Your Work | 已有 G9；本 Wave 仅保证新命令可挂 computation_trace（有公式则 L2/L3） |

---

## §6 调研检查清单（Planner 开工前勾选）

- [ ] Feature List 本波相关行已对照  
- [ ] 每项 ≥1 Primary URL + 访问日期写入对应 `p6_*.md`  
- [ ] command_id 在 `analysis_commands.cpp` 无冲突  
- [ ] Menu IA：`menu_path` / `menu_group` 规划到 Stat/DOE 或 Reliability  
- [ ] UI 线框：每命令 ≥2 页（选项 vs 结果/方法），禁止单页堆叠  
- [ ] 导入影响已评估  

---

**文档状态：** 2026-08-25 首版；供 Wave-6 `/goal` 锁定。
