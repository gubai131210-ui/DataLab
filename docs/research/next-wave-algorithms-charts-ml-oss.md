# 下一波算法 / 图表 / 经典 ML / 开源功能调研

> 研究日期：2026-08-21（UTC+8）  
> 访问日期：2026-08-21  
> 用途：给后续长时 `/goal` 的**候选池 + 权威 URL + 框架约束**；**本文件不写代码、不改命令、不填 golden**。  
> 产品边界：汽车质量桌面工具 + 综合 EDA；**不是** Minitab/JMP 100% 克隆，**不是**云端 AutoML。

### 与现有权威源的关系

| 文档 | 职责 |
|---|---|
| [`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) | 市场算法名 ↔ 状态；§12 优先队列 |
| [`comprehensive-analytics-roadmap.md`](comprehensive-analytics-roadmap.md) | Track A–D 与图表矩阵 |
| [`deferred-capability-agreement.md`](deferred-capability-agreement.md) | 刻意延后 |
| **本文件** | **P3+ / 经典 ML / 开源功能灵感** 的一次性大清单与接线约束 |

---

## §0 框架搭配硬约束（执行 agent 必读）

DataLab 竖切必须保持：

```
research md → domain（纯 C++）→ *Facts → AnalysisService → analysis_commands
→ interpretation（只读 Facts）→ serialization → formula_reference 测试
→ help catalog → acceptance / wiring / backlog 状态
```

**禁止破坏：**

1. 分层：`ui → {application, infrastructure, reporting} → domain`；domain **不**依赖 Qt。  
2. 导入契约：complete-case、`source_row`、A→B 换文件失效旧排除行。  
3. 解释层禁用子串（即使否定句）：`过程已失控` / `已证明稳定` / `批次合格` / `分布已正态`。  
4. 帮助正文禁止「见 md」。  
5. `formula_reference ≠ golden`；未导出不得写 `VALIDATION_MATRIX.md`。  
6. 控制图 PlotSpec：`center` / `lower` / `upper`。  
7. 不做：Graph Builder 全量拖拽、可旋转 3D、Assistant、宏/Python/R 嵌入、Predictive Analytics **全模块**（见 §6）。

**经典 ML 落地口径（本仓库）：** 允许 **自研/可审计的经典算法** 放进 `src/domain/statistics/`（或新 `ml/` 子目录仍属 domain），输出表+图+Facts；**禁止**把 scikit-learn / 外部 Python 塞进发行包；sklearn 仅作公式/接口参考。

---

## §1 主要来源（Primary Sources）

| 主题 | URL | 访问 |
|---|---|---|
| Minitab Feature List（全模块） | https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-21 |
| Minitab 回归族总览 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/supporting-topics/basics/regression-analyses-in-minitab/ | 2026-08-21 |
| Minitab 多元分析总览 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/supporting-topics/basics/multivariate-analyses-in-minitab/ | 2026-08-21 |
| Minitab Predictive Analytics 模型类型 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/predictive-analytics/types-of-predictive-analytics-models-in-minitab-statistical-software/ | 2026-08-21 |
| CART 回归节点分裂 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/predictive-analytics/how-to/cart-regression/methods-and-formulas/node-splitting-methods/ | 2026-08-21 |
| Random Forests® 回归方法 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/predictive-analytics/how-to/random-forests-regression/methods-and-formulas/methods/ | 2026-08-21 |
| TreeNet® 回归方法 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/predictive-analytics/how-to/treenet-regression/methods-and-formulas/methods/ | 2026-08-21 |
| Cluster K-Means 公式 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/cluster-k-means/methods-and-formulas/cluster-k-means/ | 2026-08-21 |
| NIST 过程监控总章 | https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm | 2026-08-21 |
| NIST EDA | https://www.itl.nist.gov/div898/handbook/eda/eda.htm | 2026-08-21 |
| NIST 可靠性数据与 Weibull | https://www.itl.nist.gov/div898/handbook/apr/section4/apr4.htm | 2026-08-21 |
| NIST 可靠性总章 | https://www.itl.nist.gov/div898/handbook/apr/apr.htm | 2026-08-21 |
| sklearn DecisionTree | https://scikit-learn.org/stable/modules/generated/sklearn.tree.DecisionTreeClassifier.html | 2026-08-21 |
| sklearn RandomForest | https://scikit-learn.org/stable/modules/generated/sklearn.ensemble.RandomForestClassifier.html | 2026-08-21 |
| sklearn IsolationForest | https://scikit-learn.org/stable/modules/generated/sklearn.ensemble.IsolationForest.html | 2026-08-21 |
| ggplot2 geoms（市场参考） | https://ggplot2.tidyverse.org/reference/index.html | 2026-08-21 |
| JMP Graph Builder（市场参考） | https://www.jmp.com/support/help/en/19.1/jmp/graph-builder.shtml | 2026-08-21 |

---

## §2 候选算法总表（相对 Feature List；对照 DataLab）

图例：`缺口` = backlog 仍 ❌/🟡/⏸ 或 roadmap 未闭环；`已有` = 已 ✅/⚪；`窄化可做` = 可进 /goal 但不克隆 Minitab 商标级全家桶。

### 2.1 回归 / GLM / 生存建模

| 候选 | DataLab | 优先 | 权威公式/概述 URL |
|---|---|---|---|
| Stepwise（p / AICc / BIC） | ✅ | P3 高 | 命令 `stepwise_regression`（α；非 AICc） |
| Best subsets | ❌ | P3 高 | 同上 |
| 有序 Logistic | ✅ | P3 高 | 同上；命令 `ordinal_logistic` |
| 名义 Logistic | ❌ | P3 中 | 同上 |
| Poisson 回归 | ✅ | P3 高（质量计数） | 同上；命令 `poisson_regression` |
| Nonlinear regression | ❌ | P3 中 | 同上 |
| Orthogonal regression | ❌ | P3 低 | 同上 |
| PLS | ❌ | P3 低 / ⏸ | 同上 |
| Cox regression | ❌ | P3 中 | Feature List |
| Stability studies | ❌ | ⏸ 制药偏 | Feature List |
| MARS® | ❌ | ⏸ 商标/PA | Feature List Predictive |
| Binary logistic 深化（逐步/验证集） | 🟡 | P3 | 回归总览 + 现有 HL/VIF |

### 2.2 ANOVA / 混合模型

| 候选 | DataLab | 优先 | 权威 URL |
|---|---|---|---|
| GLM（一般线性，不平衡） | ❌ | P3 高 | Feature List ANOVA |
| Mixed models（REML） | ❌ | P3 中 / 大缝 | Feature List |
| MANOVA | ❌ | P3 低 | Feature List |
| 一般因子 / 多水平 ANOVA 深化 | 🟡 | 按需 | Feature List |

### 2.3 多元统计（非控制图）

| 候选 | DataLab | 优先 | 权威 URL |
|---|---|---|---|
| PCA 深化（解析 T²/Q） | 🟡 / ⏸ Jackson | P3：经验限已有；解析限 ⏸ | 多元总览 https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/supporting-topics/basics/multivariate-analyses-in-minitab/ |
| Factor analysis | ❌ | P3 中 | 同上 |
| Discriminant analysis | ✅ | P3 高（分类诊断） | 同上；命令 `discriminant` |
| Cluster Observations（层次） | ✅ | P3 高 | 同上；命令 `cluster_observations`（complete） |
| Cluster Variables | ❌ | P3 中 | 同上 |
| Cluster K-Means | ✅ | P3 高 | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/cluster-k-means/methods-and-formulas/cluster-k-means/ ；命令 `kmeans` |
| Simple / Multiple Correspondence | ❌ | P3 中 | 多元总览 |
| Item analysis / Cronbach α | ❌ | P3 低（问卷） | 多元总览 |

### 2.4 经典 / 可审计「机器学习」（窄化）

> Minitab 商标级 TreeNet® / Random Forests® / AutoML **不做全模块**；允许 **CART 风格单树**、**小规模 bootstrap 森林（自研）**、**Isolation Forest 风格异常** 作为质量辅助，帮助中标明「非 Minitab TreeNet/RF 数值对齐」。

| 候选 | 建议命令名（草案） | 优先 | 权威 URL |
|---|---|---|---|
| CART® 分类/回归（单树 + 剪枝概念） | `cart_tree` | **Track E1 ✅** | 模型类型 https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/predictive-analytics/types-of-predictive-analytics-models-in-minitab-statistical-software/ ；分裂 https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/predictive-analytics/how-to/cart-regression/methods-and-formulas/node-splitting-methods/ ；sklearn DecisionTree |
| 随机森林（窄化：固定树数、OOB 可选） | `random_forest` | Track E2 | RF 方法 https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/predictive-analytics/how-to/random-forests-regression/methods-and-formulas/methods/ ；sklearn RF |
| 梯度提升 / TreeNet® | — | ⏸ | TreeNet https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/predictive-analytics/how-to/treenet-regression/methods-and-formulas/methods/ |
| AutoML | — | ⏸ | Feature List Predictive |
| Isolation Forest（多元异常） | `isolation_forest` | **Track E3 ✅** | sklearn IsolationForest；与 Grubbs/Dixon **分流**（多变量） |
| k-NN 分类（教学/诊断） | `knn_classify` | Track E4 低 | sklearn KNeighbors（次级）；非 Minitab 主菜单 |
| 朴素贝叶斯 | `naive_bayes` | 低 | sklearn；质量场景少 |

**禁止偷懒（ML）：** 禁止只出准确率无混淆矩阵/变量重要性；禁止声称等同 TreeNet®；禁止把 Python 运行时打进 dist。

### 2.5 时序 / 预测

| 候选 | DataLab | 优先 | 权威 URL |
|---|---|---|---|
| CCF（互相关） | ✅ | P3 | Feature List Time Series；命令 `ccf` |
| ADF（单位根） | ✅ | P3 高 | Feature List（Augmented Dickey-Fuller*）；命令 `adf_test` |
| Best ARIMA 自动选模 | 🟡/⏸ | ⏸ 数值对齐 | Feature List；deferred Kalman/TSERIES |
| Winters / 三次指数平滑深化 | 🟡 | P3 | Feature List |
| Trend analysis 独立命令 | 🟡 | 低 | Feature List |
| Box-Cox（时序路径显式） | 部分有 | 低 | Feature List |

### 2.6 可靠性 / 保修

| 候选 | DataLab | 优先 | 权威 URL |
|---|---|---|---|
| 左/区间删失 KM | ✅ | P3 | 命令 `km_interval`（Turnbull） |
| 加速寿命（Arrhenius 等） | ❌ | P3 | NIST APR；Feature List |
| 可修复系统 / NHPP | ❌ | P3 | NIST APR |
| Probit / Weibayes | ❌ | P3 | Feature List |
| Warranty analysis | ❌ | P3 | Feature List |
| 多失效模式 | ❌ | P3 | Feature List |
| Test plans | ❌ | 低 | Feature List；NIST 试验计划 |

### 2.7 DOE 加宽

| 候选 | DataLab | 优先 | 权威 URL |
|---|---|---|---|
| Plackett–Burman | ✅ | P3 | 命令 `doe_plackett_burman` |
| Definitive screening | ⏸ | P3 | Feature List |
| CCD/BBD **设计生成** | ❌ | P3（分析已有 RSM） | Feature List |
| Mixture | ❌ | P3 | Feature List |
| Taguchi | ❌ | P3 | Feature List |
| Split-plot | ❌ | 低 | Feature List |
| D-optimal | ❌ | 低 | Feature List |
| Analyze variability / binary DOE | ❌ | P3 | Feature List |

### 2.8 质量工具剩余

| 候选 | DataLab | 优先 | 权威 URL |
|---|---|---|---|
| 不平衡 Expanded Gage GLM | ⚪ | P3 | deferred；Minitab Expanded 方法页 |
| Batch capability | ❌ | P3 | Feature List |
| Nonparametric capability | ❌ | P3 | Feature List* |
| Automated capability | ❌ | ⏸ Assistant 向 | Feature List* |
| 2×2 crossover TOST | ❌ | 低 | Feature List Equivalence |
| Historical 全图种参数页 | 部分 I-MR | 中 | Feature List Historical charts |

### 2.9 仿真 / 分布 / Bootstrap

| 候选 | DataLab | 优先 | 权威 URL |
|---|---|---|---|
| PDF/CDF/逆 CDF 计算器 | ❌ | Track F 中 | Feature List Simulations |
| 随机抽样 / RNG | ❌ | Track F 低 | Feature List |
| Bootstrap / randomization tests | ✅ / ❌ | 单均值百分位 ✅（`bootstrap_mean`）；两样本/BCa/随机化检验仍 ❌ |

### 2.10 图表 / EDA（相对 roadmap §4）

| 候选 | DataLab | 优先 | 权威 / 市场参考 |
|---|---|---|---|
| Mosaic / spine | ❌ / ⏸ | Track G | ggplot2；卡方残差已有热图 |
| Correlogram / 相关矩阵热图独立图 | ✅ | Track G | Feature List Graphics correlograms；命令 `correlogram` |
| Parallel coordinates | ❌ | Track G | Feature List parallel plots |
| Bubble plot | ❌ | Track G | Feature List |
| Matrix plot（散点矩阵） | ✅ | Track G 高 | Feature List matrix plots；命令 `matrix_plot` |
| Dotplot | ❌ | Track G | Feature List |
| Forest plot（命名） | ❌ | 低 | 医学 meta；可选 |
| Calendar heatmap | ❌ | 低 | roadmap |
| Donut | ❌ | ⏸ 慎用 | — |
| 可旋转 3D | ⏸ | ⏸ | deferred |
| Graph Builder 拖拽 | ⏸ | ⏸ | deferred |

---

## §3 建议 /goal Track（E–H）— 供长时执行

> Track A–D 已基本闭环（见 roadmap）。新队列从 **E** 起编号。每轮仍 **3–4 竖切**；agent **默认不**跑 cmake/ctest（中文路径）；仅用户要求才 commit/push。

### Track E — 经典可审计 ML（质量辅助）

| ID | 交付 | 证据 |
|---|---|---|
| E1 | `cart_tree`：二叉递归划分；LS/Gini；树表+变量重要性；可选最大深度 | ✅ research `p3_cart_tree.md` + Facts + 测试 |
| E2 | `random_forest` 窄化：固定 `n_trees`、bootstrap、多数表决/均值；OOB 可选 | 披露非 TreeNet/Minitab RF 对齐 |
| E3 | `isolation_forest`：多元异常分数图；与单变量 outlier_test 分流 | ✅ research `p3_isolation_forest.md` + Facts + 测试 |
| E4 | （可选）`knn_classify` 或跳过 | — |

**禁止偷懒：** 禁止无诊断的黑盒；禁止 AutoML；禁止把 sklearn DLL 塞进 dist。

### Track F — 推断加强（Bootstrap / 分布工具）

| ID | 交付 | 状态 |
|---|---|---|
| F1 | 单均值 bootstrap CI（百分位；非 BCa） | ✅ `bootstrap_mean` |
| F2 | 两样本均值差 bootstrap | ❌ |
| F3 | PDF/CDF/分位数计算器（正态/t/χ²/F/Weibull 子集） | ❌ |

权威：Feature List Simulations；NIST 分布章节。

### Track G — 图表加宽（非 Graph Builder）

| ID | 交付 |
|---|---|
| G1 | `matrix_plot` 散点矩阵 | ✅ |
| G2 | `correlogram` / 相关热图独立图 | ✅ |
| G3 | `parallel_plot` 或 `bubble_plot`（择一） |
| G4 | mosaic ⏸ 除非卡方残差已稳定 |

### Track H — 建模 / 多元 / 可靠性（P3 主缝）

按 backlog 优先级滚动，建议顺序：

1. Poisson 回归（缺陷计数） ✅ `poisson_regression`  
2. 有序 Logistic ✅ `ordinal_logistic`  
3. Stepwise ✅ `stepwise_regression`（Best subsets 仍 ❌）  
4. K-Means ✅ + 层次聚类 ✅  
5. Discriminant ✅ `discriminant`  
6. ADF ✅ + CCF ✅  
7. 左/区间删失 KM ✅ `km_interval`  
8. PB 设计生成 ✅ `doe_plackett_burman`（CCD/BBD 仍 ❌）

每项仍要独立 `docs/research/p3_*.md`。

---

## §4 开源项目：可吸收的功能灵感（非抄代码）

> 许可与架构不同；**只借鉴产品能力**，实现必须走 DataLab 竖切。AGPL 项目尤其注意：不要整库并入。

| 项目 | 许可/栈 | 可吸收能力 | 不宜直接搬 |
|---|---|---|---|
| [Cassini](https://github.com/saturnis-io/cassini) | AGPL；Python/React SPC | 实时图、Nelson、MSA 向导、changepoint、Isolation Forest、Show Your Work | 整站架构、AGPL 传染风险 |
| [mfgqc](https://pypi.org/project/mfgqc/) | MIT；Python | 假设守卫文案、可审计 provenance、DOE/MSA 对照 oracle 思路 | Python 依赖进 dist |
| [Tinker DOE](https://github.com/M-Elsaied/Tinker) | OSS；浏览器 DOE | PB/CCD/BBD/D-opt/Mixture 设计矩阵 UX、desirability PDF 报告叙事 | Web 栈 |
| [BASIL](https://github.com/molecularmodelinglab/BASIL) | OSS；Qt/PySide + BayBE | 贝叶斯序贯试验 UX（远景）；参数约束表 | 化学 SMILES、BayBE 运行时 |
| [JASP](https://github.com/jasp-stats/jasp-desktop) | AGPL；Qt/C++ | 模块化分析、CSV 预览、贝叶斯模块（远景）、结果报告体验 | AGPL；R 后端 |
| [LabPlot](https://labplot.org/pages/features/) | GPL；KDE/Qt | 拟合函数库、柱统计子表、Notebook（远景）、多格式导入 | Notebook/多语言内核 |
| [RKWard](https://rkward.kde.org/) + rk.qcc | GPL；R GUI | SPC 插件菜单组织；能力+控制图同屏 | 依赖完整 R |
| quality-analysis-toolkit 类小库 | 多为 MIT 示例 | Xbar-R + Gage 教学流水线 | 深度不足 |

**可落地的「产品功能」增量（开源启发，非算法本身）：**

1. **CSV/Excel 导入预览**（分隔符/小数点确认）— JASP。  
2. **分析假设自检表**（样本量、平衡性、删失比例）— mfgqc/Cassini。  
3. **Show Your Work / 公式页已有**：继续强化「复制摘要」— Cassini。  
4. **DOE 设计向导分步页**（设计→收集→分析）— Tinker。  
5. **变点检测（changepoint）** 作为 I-MR 辅助诊断 — Cassini/ruptures 思路，自研 CUSUM/PELT 窄化。  
6. **项目加密 / 只读包** — JASP（低优先）。  
7. **能力+控制图六包已有**：可补「自动推荐正态/非正态路径」但**不做 Assistant 黑盒**。

---

## §5 与 backlog / roadmap 的登记建议（实现前）

执行时把选中的 ❌ 行写入：

- `minitab-market-algorithm-backlog.md` 对应章节 + §12 新 P3 行  
- `comprehensive-analytics-roadmap.md` Track E–H 状态列  
- `algorithm-session-brief.md` 新 §5v+ 批次说明  

本文件 **不**自动改 ✅；避免未实现却标完成。

---

## §6 明确不做（与 deferred 对齐，扩展声明）

| 项 | 原因 |
|---|---|
| TreeNet® / MARS® / AutoML 全模块 | 商标+复杂度；产品外 |
| 嵌入 Python/R 运行时 | dist 可移植与分层冲突 |
| Graph Builder 拖拽 / 可旋转 3D | deferred |
| Assistant 向导 | Feature List 明确；黑盒风险 |
| 云端实时 MQTT SPC 平台 | Cassini 类；非桌面主路径 |
| 把 AGPL 项目源码并入仓库 | 许可风险 |

---

## §7 给 `/goal` 的推荐首批（可跑很久）

**批次 1（Track E+G 混合，约 4 项）：**

1. K-Means（多元）— ✅ `kmeans`  
2. CART 单树（分类或回归）— ✅ `cart_tree`  
3. `matrix_plot` 散点矩阵 — ✅（既有；文档纠偏）  
4. ADF 单位根（时序诊断，配 ACF）— ✅ `adf_test`

**批次 2：**

1. Poisson 回归 — ✅ `poisson_regression`  
2. Isolation Forest — ✅ `isolation_forest`  
3. Bootstrap 单均值 CI — ✅ `bootstrap_mean`  
4. Cluster Observations（层次，complete）— ✅ `cluster_observations`

**批次 3：**

1. 有序 Logistic — ✅ `ordinal_logistic`  
2. Discriminant — ✅ `discriminant`  
3. CCF — ✅ `ccf`  
4. correlogram — ✅ `correlogram`

每项：独立 research md → 竖切 → 文档同步 → 合规子 agent；**禁止**菜单占位。

---

## §8 禁止偷懒清单（本文件专用）

- 禁止只写调研不列官方 URL  
- 禁止把「市场有」直接标 DataLab ✅  
- 禁止引入 Python/R 依赖完成 ML  
- 禁止声称数值等同 Minitab TreeNet/RF  
- 禁止破坏已闭环 P0–P2 命令  
- 禁止 Graph Builder / 3D / Assistant 借道本文件复活  
- 禁止无 Facts / 无解释约束的黑盒分数  
- 禁止一次 PR 塞满 Track E–H 而不竖切验收  

---

## 附录 A — Feature List 模块速查（2026-08-21）

来源：https://www.minitab.com/en-us/products/minitab/features/

Assistant · Graphics · Basic Statistics · Regression · ANOVA · MSA · Quality Tools · DOE · Reliability/Survival · Power and Sample Size · **Predictive Analytics** · Multivariate · Time Series · Nonparametrics · Equivalence · Tables · Simulations · Macros

## 附录 B — 修订记录

| 日期 | 说明 |
|---|---|
| 2026-08-21 | Track H 滚动：`stepwise_regression` / `km_interval` / `doe_plackett_burman` ✅（Best subsets / CCD 仍 ❌） |
| 2026-08-21 | 批次 3：`ordinal_logistic` / `discriminant` / `ccf` / `correlogram` ✅ |
| 2026-08-21 | 批次 2：`poisson_regression` / `isolation_forest` / `bootstrap_mean` / `cluster_observations` ✅ |
| 2026-08-21 | 批次 1 竖切：`kmeans` / `cart_tree` / `adf_test` ✅；`matrix_plot` 文档纠偏为 ✅ |
| 2026-08-21 | 初版：算法/图表/经典 ML/开源灵感；Track E–H；框架约束 |
