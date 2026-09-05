# Minitab 市场算法对照清单（完成状态）

> 对照来源：[Minitab Feature List](https://www.minitab.com/en-us/products/minitab/features/)（访问 2026-08-21）  
> 产品范围：**主路径**仍为汽车质量工程师桌面工具；**综合轨道**（通用 EDA/图表、判断规则文档化、建模加宽）见 [`comprehensive-analytics-roadmap.md`](comprehensive-analytics-roadmap.md)；**市场/UX/架构演进调研**见 [`product-evolution-market-ux-architecture-research.md`](product-evolution-market-ux-architecture-research.md)。  
> **不做**：Predictive Analytics 全模块、Assistant 向导、Graph Builder **拖拽全量**、宏/Python/R 集成、可旋转 3D。  
> 状态权威：本文件 = 「市场有什么 / 我们到哪了（算法名）」；综合图表与 Track 队列 = roadmap；接线细节见 `algorithm-chart-gap-matrix.md`、`deferred-capability-agreement.md`、`algorithm-session-brief.md`。  
> `formula_reference ≠ golden`：未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX.md`。

## 状态图例

| 标记 | 含义 |
|---|---|
| ✅ 深度闭环 | 命令/领域/服务/Facts/解释/序列化/公式 md 已齐；可列手工验收（acceptance 勾选可能仍开） |
| 🟡 已接入待深化 | 已有入口与主计算，相对 Minitab 仍缺方法选项、表形、图、后比较或诊断深度 |
| ⚪ 已实现待 golden | 产品可用，数值未与 Minitab 导出对齐；**G-Trust 锁表**可另注「ref-golden 已冻」（`reference_implementation`，**≠** vendor ✅） |
| ❌ 未实现 | 市场常见且属产品范围，仓库尚无独立闭环 |
| ⏸ 刻意延后/产品外 | `deferred-capability-agreement.md` §5，或明确不做 |

**深度闭环规则（每项 ✅ 前必须满足）**：research md → domain → `*Facts` → AnalysisService → `analysis_commands` → interpretation（只读 Facts）→ serialization + `# source: formula_reference` 测试 → help catalog → acceptance/gap 状态更新；导入 complete-case / `source_row` / A→B 契约不破坏。

**进度一句话（2026-08-21）**：**§12 P0+P1+P2 主项已 ✅/⚪**；含 DOE/ACF/功效/RSM、规则策略、C1–C5、D2/D3、B1–B5、T²/GV/MEWMA、EMP + Expanded(三因子平衡)；不平衡 Expanded GLM 与 P3 延后。

---

## 1. Basic Statistics

| 市场算法（Minitab） | DataLab 状态 | 说明 / 下一步 |
|---|---|---|
| Descriptive statistics | ✅ | 箱线+个体值；继续按表形小修即可 |
| 1-sample Z | ✅ | 命令 `one_sample_z`；已知 σ |
| 1/2-sample t、paired t | ✅ | 区间图已有；`two_sample_t` **ref-golden 已冻**（≠ vendor_oracle） |
| 1/2 proportions | ✅ | exact/normal/wilson/AC；两比例 Newcombe/AC；不做 Blaker |
| 1/2-sample Poisson rate | ✅ | 含率比；不做 Blaker |
| 1/2 variances / equal variances | ✅ | F / Levene / Bonett / Bartlett |
| Correlation / covariance | ✅ | Pearson/Spearman+散点；协方差矩阵+可选偏相关 |
| Normality test | ✅ | AD 默认 + Ryan–Joiner 可选；`normality_test` **ref-golden 已冻**（≠ vendor_oracle） |
| Outlier test（Grubbs / Dixon r10） | ✅ | `outlier_test` method=`grubbs`\|`dixon_r10` |
| Poisson GOF | ✅ | 命令 `poisson_gof`；独立于 chi_square_gof |

## 2. Nonparametrics

| 市场算法 | 状态 | 说明 / 下一步 |
|---|---|---|
| Sign test | ✅ | 主 P + 中位数 CI（非 NLI 假三区间） |
| Wilcoxon（1-sample / paired） | ✅ | Walsh/HL CI 单样本与配对 |
| Mann–Whitney | ✅ | McKean–Ryan CI |
| Kruskal–Wallis | ✅ | Dunn 默认；Steel–Dwass 可选（近似） |
| Mood’s median | ✅ | χ² + 各组 Sign CI |
| Friedman | ✅ | 可选 Nemenyi（近似）；不做独立 Nemenyi 命令 |
| Runs test | ✅ | 命令 `runs_test` |
| McNemar / Cochran Q | ✅ | DataLab 已有（Minitab 多在 Tables） |

## 3. Equivalence / Tables

| 市场算法 | 状态 | 说明 / 下一步 |
|---|---|---|
| 1/2-sample / paired TOST | ✅ | 含均值比（Fieller/log） |
| 比例 z-TOST | ✅ | 1/2-sample |
| 2×2 crossover TOST | ❌ | 低优先 |
| Chi-square association | ✅ | 百分比表 + 调整残差热图（D3）；mosaic ⏸ |
| Fisher’s exact | ✅ | 独立 `fisher_exact`；两比例仍附带 Fisher P |
| Chi-square GOF | ✅ | 有效性提示已有；不做期望比例假造 |
| Tally / cross tabulation | ✅ | 命令 `cross_tabulation`；与 `chi_square` 分流 |

## 4. ANOVA / Regression

| 市场算法 | 状态 | 说明 / 下一步 |
|---|---|---|
| One-way / Two-way ANOVA | ✅ | Tukey 区间+Grouping；残差图；`one_way_anova` **ref-golden 已冻**（≠ vendor_oracle） |
| GLM / Mixed / MANOVA | ❌ | 大缝；非本阶段默认 |
| Analysis of means (ANOM) | ✅ | 命令 `anom`；正态均值；Nelson 近似限 |
| Equal variances（见上） | ✅ | |
| Linear regression | ✅ | Unusual、Fitted Line、DW 临界、残差契约 |
| Stepwise / Best subsets | ✅ / ✅ | `stepwise_regression` ✅（含 Forward AICc/BIC 表形 2026-08-22）；`best_subsets_regression` ✅ |
| Binary logistic | ✅ | HL/VIF/影响点 + 配对一致率 + 分类表 + 逐步 AIC/BIC（2026-08-22 Wave-4） |
| Ordinal logistic | ✅ | `ordinal_logistic`（比例优势 logit；formula_reference） |
| Nominal logistic | ✅ | `nominal_logistic`（2026-08-22 Wave-2） |
| Poisson 回归 | ✅ | `poisson_regression`（log 链 IRLS）；见 §10 |
| Nonlinear / orthogonal / PLS | ❌ | ⏸ 多数非质量主路径优先 |
| Cox regression（固定协变量 PH） | ✅ | `cox_regression` 窄化（2026-08-22 Wave-4）；不做 counting process |
| Stability studies | ❌ | 制药偏多；汽车可选后置 |

## 5. Quality Tools — 控制图

| 市场算法 | 状态 | 说明 / 下一步 |
|---|---|---|
| Xbar / R / S / Xbar-R / Xbar-S | ✅ | Test 规则+阶段；`xbar_r` **ref-golden 已冻**（≠ vendor_oracle） |
| I / MR / I-MR / I-MR-R/S | ✅ | `imr` **ref-golden 已冻**（≠ vendor_oracle） |
| Zone chart | ✅ | 命令 `zone_chart`；Jaehn 计分 + 个体/得分双图 |
| Z-MR | ✅ | 命令 `z_mr`；Z + MR(Z)；可选分组 |
| P / NP / C / U / Laney P'/U' | ✅ | `p_chart` **ref-golden 已冻**（≠ vendor_oracle） |
| MA（移动平均图） | ✅ | 命令 `moving_average`；窗宽 `ma_window` |
| EWMA / CUSUM | ✅ | |
| Multivariate T² / GV / MEWMA | ✅ | T²+GV+MEWMA；个体 GV 替代 ⏸ |
| G / T rare event | ✅ | |
| Historical / shift-in-process | ✅ | I-MR 历史参数表 + 分阶段估计对照（B4） |
| Run chart | ✅ | 独立命令 `run_chart`；Gage 内嵌 Run Chart 仍在 |
| Pareto chart（缺陷） | ✅ | 命令 `pareto`；勿重做 |
| Cause-and-effect（鱼骨） | ✅ | 命令 `cause_and_effect` |

## 6. Quality Tools — 能力 / 变换 / 容差

| 市场算法 | 状态 | 说明 / 下一步 |
|---|---|---|
| Box-Cox / Johnson | ✅ / ⚪ | Johnson 待 golden |
| Individual distribution ID | ✅ | |
| Normal capability / Sixpack | ✅ | `capability` / `capability_sixpack` **ref-golden 已冻**（≠ vendor_oracle） |
| Nonnormal / between-within | ✅ / ⚪ | `between_within_capability` **ref-golden 已冻**（≠ vendor_oracle）；非正态仍待 vendor/后续 golden |
| Attribute capability（二项/泊松） | ✅ | |
| Batch capability | ✅ | `batch_capability`（2026-08-22） |
| Nonparametric capability | ✅ | `nonparametric_capability`（Wave-2 窄化 + Wave-4 直方图/PPM/Cnp 表形） |
| Automated capability | ❌ | 产品外自动化向导倾向 |
| Tolerance intervals | ✅ | 正态 Howe/Natrella + 非参数；显式 method |
| Acceptance sampling / OC | ✅ | 命令 `acceptance_sampling`；二项 OC |
| Multi-Vari | ✅ | 2～4 因子 |
| Variability chart | ✅ | 命令 `variability_chart`；1～2 因子 |

## 7. MSA / Gage

| 市场算法 | 状态 | 说明 / 下一步 |
|---|---|---|
| Gage R&R Crossed / Nested | ✅ | %Tol、Run Chart、By Part 等；`gage_rr` **ref-golden 已冻**（≠ vendor_oracle） |
| Gage R&R Expanded | ✅/⚪ | 平衡三因子 `expanded_gage_rr` ✅；不平衡/固定/嵌套 GLM ⚪ |
| Gage linearity and bias | ✅ | |
| Type 1 Gage | ✅ | Cg 全公差口径（非 Minitab K=20%） |
| Attribute agreement / Kappa | ⚪ | Fleiss/Cohen/Kendall；待 golden；Weighted κ 非 Minitab AAA |
| EMP Crossed | ✅ | `emp_crossed` |
| Data collection worksheets | ⏸ | UI 模板，非算法核 |

## 8. DOE

| 市场算法 | 状态 | 说明 / 下一步 |
|---|---|---|
| Two-level factorial 分析 | ✅ | Pareto/立方/等值线/残差/优化 |
| 设计生成（全因子/部分/PB/DSD…） | ✅ / ⏸ | 2^k + 2^(k-p) ✅；PB ✅（`doe_plackett_burman`）；DSD ⏸ |
| RSM / Mixture / Taguchi / Split-plot | 🟡 | RSM 分析 ✅；`taguchi_orthogonal_design`+`taguchi_analyze`+`mixture_design` ✅（W5/W6）；Split-plot 仍 ❌ |
| Analyze variability / binary response DOE | ❌ | |
| 响应优化 | ✅ | 多响应 D；精确 PI 优先协方差 |

## 9. Reliability / Survival

| 市场算法 | 状态 | 说明 / 下一步 |
|---|---|---|
| 右删失参数分布（Weibull/指数/对数正态） | ✅ / ⚪ | 含阈值族；待 golden |
| KM / Log-rank / CIF | ✅ | 右删失 KM ✅；K 组 Log-rank ✅；Aalen–Johansen CIF 曲线+表 + Gray 窄化 ✅（2026-08-22 Wave-4）；Fine-Gray IPCW formula_reference |
| Accelerated life / warranty / repairable | ✅ / ✅ | ALT Newton MLE + 使用应力百分位 ✅（2026-08-22）；warranty ✅ |
| Probit / Weibayes / life regression | 🟡 | `probit_reliability` ✅；`weibayes` ✅（W5）；`nhpp_repairable`/`reliability_test_plan` ✅（W6）；全量寿命回归 ❌ |
| Test plans | ❌ | |

## 10. Time Series / Power / Multivariate

| 市场算法 | 状态 | 说明 / 下一步 |
|---|---|---|
| Decomposition / SES-DES / Winters | ✅ | |
| ARIMA / Best ARIMA 候选 | 🟡 / ⏸ | CSS 候选有；TSERIES/Kalman 数值对齐延后 |
| ACF / PACF / CCF / ADF | ✅ | ACF/PACF ✅；ADF ✅；CCF ✅（`ccf`） |
| Trend analysis / MA 平滑 | 🟡/❌ | 部分被分解/平滑覆盖 |
| Power：t / ANOVA / 比例 / 方差 / 泊松 | ✅ | |
| Power：等价 / DOE / 容差样本量 | ✅ | `t_power` mode：`equivalence_*` / `doe_factorial_*` / `tolerance_normal_sample_size` |
| PCA | 🟡 | 经验 T²/Q；Jackson–Mudholkar 解析限延后 |
| Cluster K-Means | ✅ | `kmeans`（欧氏 Lloyd；formula_reference） |
| Cluster Observations（层次） | ✅ | `cluster_observations`（complete linkage） |
| CART 单树（窄化） | ✅ | `cart_tree`；非 TreeNet/RF |
| Isolation Forest（窄化） | ✅ | `isolation_forest`；与 outlier_test 分流 |
| Bootstrap 单均值 CI | ✅ | `bootstrap_mean`（百分位） |
| Bootstrap 两样本均值差 CI | ✅ | `bootstrap_two_sample`（2026-08-22） |
| Poisson 回归 | ✅ | `poisson_regression`（log 链 IRLS） |
| Factor analysis | ⏸ | 见 next-wave；非本批 |
| Discriminant | ✅ | `discriminant`（LDA；非 QDA） |

## 11. 交互与图表优势（非「算法名」但要对齐）

| Minitab 优势 | 状态 | 说明 / 下一步 |
|---|---|---|
| 方法选项可切换 | 🟡 | 比例/正态性/Kruskal 后比较等已有；继续按分析补齐 |
| 小样本 / 假设警告 | 🟡 | 多处诊断；统一文案与 Facts 字段可再深 |
| 表 + 图同页 | 🟡 | 多数分析已有；Pareto / Run Chart / 鱼骨 / Variability 已独立 |
| 图属性编辑 | 🟡 | 侧栏色/线/点/主题/刻度；注释/拖拽/多图拼版延后 |
| 帮助公式可读 | ✅ | 「公式与来源」页签；禁止正文「见 md」 |
| 通用 EDA 图形加宽 | ✅ | C1–C5 ✅（含 `eda_4plot`） |
| 特殊原因测试可选 + 规则文档 | ✅ | Tests 1–8；help Catalog；`minitab_like` vs `all_applicable` |

---

## 11b. 算法判断规则（摘要；详表见 roadmap §2）

| 规则族 | 定义来源 | DataLab | 下一步 |
|---|---|---|---|
| Nelson / Minitab Tests 1–8 | [Minitab Tests](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/basics/using-tests-for-special-causes/)（访问 2026-08-21） | ✅ 实现 + UI 可选；策略 `minitab_like`/`all_applicable` + Catalog | Track B3：Nelson estimate + MSSD ✅ |
| WECO 四规则（NIST） | [NIST pmc32](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm) | 与 Tests 1/5/6/2 **部分重叠** | 帮助中对照说明误报代价；不另造编号系统 |
| Zone Jaehn 1/2/4≥8 | `p1_zone_zmr_ma_charts.md` | ✅ `zone_chart` | 禁止与 Tests 1–8 混写 |
| Run Chart 四模式 | Minitab Run Chart | ✅ `run_chart` | ⚪ golden；不做控制限 |
| ANOM 决策限 | Nelson 近似 | ✅ `anom` | 二项/泊松 ANOM ❌ |
| 能力/SPC 解读约束 | ADR + interpretation | ✅ 禁止合格/已证明稳定等 | Track B 统一 Facts |

**解释硬约束（所有规则共用）**：规则触发 = **调查信号**，不是「过程已失控 / 批次合格 / 已证明稳定」。

---

## 12. 未实现优先队列（产品范围内，供 /goal 消耗）

按汽车质量杠杆排序；每轮锁定 3～4 项竖切。**禁止**把 §13 延后项当成本队列。  
**综合轨道**（图表/规则/建模）同步登记于 [`comprehensive-analytics-roadmap.md`](comprehensive-analytics-roadmap.md) Tracks A–D。

### 12.0 已完成水位（勿重做）

| 水位 | 状态 | 批次 |
|---|---|---|
| P0（Runs / Fisher / Dixon / Run Chart / 鱼骨） | ✅ | brief §5k–§5l |
| P1（Acceptance / Variability / Zone·Z-MR·MA / 容差 method / ANOM / 1-sample Z / Poisson GOF / 协方差偏相关） | ✅ | brief §5l–§5n |

| 优先级 | 项 | 类型 | 备注 |
|---|---|---|---|
| P0 | Runs test | ✅ | 命令 `runs_test` |
| P0 | Fisher exact 表形深化 | ✅ | 命令 `fisher_exact` |
| P0 | Dixon 异常值（可选） | ✅ | `outlier_test` method=`dixon_r10` |
| P0 | 独立 Run Chart | ✅ | 命令 `run_chart` |
| P0 | Cause-and-effect（鱼骨） | ✅ | 命令 `cause_and_effect` |
| P1 | Acceptance sampling + OC | ✅ | |
| P1 | Variability chart | ✅ | |
| P1 | Zone / Z-MR / MA 控制图 | ✅ | |
| P1 | 非参数容差区间 | ✅ | |
| P1 | ANOM | ✅ | |
| P1 | 1-sample Z | ✅ | |
| P1 | Poisson GOF | ✅ | |
| P1 | 协方差/偏相关 | ✅ | |

### 12.1 下一批建议（三选一或组合，每轮仍 3～4 项）

| Track | 焦点 | 首批建议 | 权威细节 |
|---|---|---|---|
| **A 质量 P2** | DOE / 时序诊断 / 多元 SPC / Gage | DOE 设计生成 → ACF/PACF → RSM 或多元 T² | roadmap §5 Track A |
| **B 判断规则** | Tests 默认策略、Nelson estimate、规则百科 | 规则 catalog + 默认策略选项 | roadmap §5 Track B |
| **C 综合图表** | 非仅质量的 EDA | 密度/KDE → Hexbin → Violin → 通用条形 | roadmap §5 Track C |
| **D 建模加宽** | 交叉表 / 有序 Logistic / GLM | 独立交叉表 → 卡方残差深化 | roadmap §5 Track D |

### 12.2 P2 / P3 明细（仍 ❌ / 🟡）

| 优先级 | 项 | 类型 | 备注 |
|---|---|---|---|
| P2 | DOE 设计生成（全/部分析因） | ✅ | Track A1；PB ✅；DSD 仍 ⏸ |
| P2 | RSM 分析（若已有设计表） | ✅ | Track A2；命令 `rsm_response` |
| P2 | Expanded Gage / EMP | ✅/⚪ | EMP `emp_crossed` ✅；平衡三因子 `expanded_gage_rr` ✅；不平衡 GLM ⚪ |
| P2 | 多元控制图 T²/GV/MEWMA | ✅ | `hotelling_t2` / `generalized_variance` / `mewma` |
| P2 | ACF/PACF；容差/等价/DOE 功效 | ✅ | Track A3–A4；命令 `acf_pacf` + `t_power` 扩展 mode |
| P2 | 特殊原因规则 catalog + 默认策略 | ✅ | Track B1–B2；help `special_cause_rules`；`rule_policy` |
| P2 | I 图 Nelson estimate + MSSD | ✅ | Track B3；`use_nelson_estimate` + `sigma_method=mssd` |
| P2 | Historical 参数页 / 规则交叉链接 | ✅ | Track B4–B5；I-MR 历史/阶段表；`special_cause_rules` 交叉链接 |
| P2 | 密度图 / Hexbin / Violin / 通用条形（EDA） | ✅ | Track C1–C4；命令 density/hexbin/violin/bar_chart |
| P2 | EDA 四图打包 | ✅ | Track C5；命令 `eda_4plot` |
| P2 | 交叉表 / Tally 独立工具 | ✅ | Track D2；命令 `cross_tabulation` |
| P2 | 卡方关联残差深化 | ✅ | Track D3；百分比表 + 调整残差热图；mosaic ⏸ |
| P3 | 批次 1–3 + Track H 滚动（stepwise / km_interval / PB） | ✅ | Best subsets ✅（A1 2026-08-22）；TreeNet ⏸ |
| P3 | 左/区间删失；加速寿命等 | ✅ | KM 区间 ✅；ALT 窄化 ✅ `accelerated_life`（2026-08-22） |
| P3 | Best subsets / CCD·BBD 设计生成 | ✅ | Best subsets ✅；CCD·BBD ✅ |

**已完成勿重做（节选）**：brief §4–§5n。详见 `algorithm-session-brief.md`。

## 13. 刻意延后（清空缺口时不算失败）

见 `deferred-capability-agreement.md` §5：

- Blaker / Adjusted Blaker；泊松 Blaker  
- Jackson–Mudholkar T²/Q 解析限  
- 可旋转 3D；图表注释/拖拽/多图拼版；**Graph Builder 全量拖拽**  
- Kalman / TSERIES 数值对齐；无界似然 bias-correction 对齐  
- Nemenyi **独立**命令；精确 studentized-range  
- 重构阶段 5/6（PlotSpec 合一、CI、i18n）  
- Predictive Analytics / Assistant  

## 14. 维护规则

1. 每竖切闭环一项：把本表对应行改为 ✅（或 ⚪），并更新 `quality-algorithms-acceptance.md`、gap-matrix、brief；若属规则/综合图表，**同步** `comprehensive-analytics-roadmap.md`。  
2. 新发现的市场项：先写入本表或 roadmap 再实现，避免只做代码不登记。  
3. `/goal` 终态（质量）= **§12 产品范围内 ❌/🟡 按优先级清空到可接受水位**（P0+P1+P2 主项已清到 ✅/⚪；P3 延后），**不是** Minitab 功能列表 100% 克隆。  
4. `/goal` 终态（综合）= roadmap Track B 规则文档化 + Track C 商定的 EDA 子集。  
5. 访问日期变更时更新文首 Feature List 链接日期。

## 15. 下一波候选池（P3+ / 经典 ML / 开源灵感）

长清单与官方 URL、框架硬约束、建议 Track E–H 见：

**[`next-wave-algorithms-charts-ml-oss.md`](next-wave-algorithms-charts-ml-oss.md)**（2026-08-21）

该文件**不**改变本表已有 ✅ 行；实现前须把选中项登记回本表章节与 §12，再竖切。Predictive Analytics 全模块 / Graph Builder / 嵌入 Python·R 仍属 §13 延后。

### 15.1 算法 Wave-5 锁定（2026-08-23 · ✅）

计划与 Mega `/goal`：[`goal-wave-2026-08-23-algorithm-wave5-plan-and-mega-prompt.md`](goal-wave-2026-08-23-algorithm-wave5-plan-and-mega-prompt.md)  
调研：[`algorithm-wave5-market-formula-research-2026-08-23.md`](algorithm-wave5-market-formula-research-2026-08-23.md)

| Wave-5 | id | 备注 |
|--------|-----|------|
| W5-1 | `random_forest` | Track E2 窄化 ✅ |
| W5-2 | `weibayes` | 可靠性少失效窄化 ✅ |
| W5-3 | `taguchi_orthogonal_design` | DOE 设计生成子集 ✅ |
| W5-4 | `distribution_calculator` | PDF/CDF/分位工具 ✅ |

### 15.2 算法 Wave-6 锁定（2026-08-25 · ✅）

计划与 Mega `/goal`：[`goal-wave-2026-08-25-algorithm-wave6-plan-and-mega-prompt.md`](goal-wave-2026-08-25-algorithm-wave6-plan-and-mega-prompt.md)  
调研：[`algorithm-wave6-market-formula-research-2026-08-25.md`](algorithm-wave6-market-formula-research-2026-08-25.md)  
DoD：[`goal-wave-2026-08-25-algorithm-wave6.md`](goal-wave-2026-08-25-algorithm-wave6.md)

| Wave-6 | id | 备注 |
|--------|-----|------|
| W6-1 | `taguchi_analyze` | Taguchi **分析**（设计已在 W5） ✅ |
| W6-2 | `mixture_design` | Mixture 设计生成（simplex 窄化） ✅ |
| W6-3 | `nhpp_repairable` | 可修复 NHPP 幂律 ✅ |
| W6-4 | `reliability_test_plan` | 可靠性试验/演示样本量 ✅ |

### 15.3 算法 Wave-7 锁定（2026-08-28 · ✅）

计划与 Mega `/goal`：[`goal-wave-2026-08-28-algorithm-wave7-plan-and-mega-prompt.md`](goal-wave-2026-08-28-algorithm-wave7-plan-and-mega-prompt.md)  
调研：[`algorithm-wave7-market-formula-research-2026-08-28.md`](algorithm-wave7-market-formula-research-2026-08-28.md)  
DoD：[`goal-wave-2026-08-28-algorithm-wave7.md`](goal-wave-2026-08-28-algorithm-wave7.md)

| Wave-7 | id | 备注 |
|--------|-----|------|
| W7-1 | `mixture_analyze` | Mixture **分析**（Scheffé；设计已在 W6） ✅ |
| W7-2 | `glm_two_way` | 不平衡双因子 GLM（Type III） ✅ |
| W7-3 | `analyze_variability` | DOE 分散效应（2 水平窄化） ✅ |
| W7-4 | `factor_analysis` | 因子分析（主成分提取窄化） ✅ |

### 15.4 算法 Wave-8 锁定（2026-08-28 · ✅）

计划与 Mega `/goal`：[`goal-wave-2026-08-28-algorithm-wave8-plan-and-mega-prompt.md`](goal-wave-2026-08-28-algorithm-wave8-plan-and-mega-prompt.md)  
调研：[`algorithm-wave8-market-formula-research-2026-08-28.md`](algorithm-wave8-market-formula-research-2026-08-28.md)  
DoD：[`goal-wave-2026-08-28-algorithm-wave8.md`](goal-wave-2026-08-28-algorithm-wave8.md)

| Wave-8 | id | 备注 |
|--------|-----|------|
| W8-1 | `binary_response_doe` | 析因二值响应 Logit IRWLS ✅ |
| W8-2 | `cluster_variables` | 变量层次聚类 dendrogram ✅ |
| W8-3 | `glm_three_factor` | 三因子不平衡 Type III（无 ABC 三阶） ✅ |
| W8-4 | `life_data_regression` | Weibull + 协变量 + 右删失窄化 ✅ |

### 15.5 算法 Wave-9 锁定（2026-08-28 · ✅）

计划与 Mega `/goal`：[`goal-wave-2026-08-28-algorithm-wave9-plan-and-mega-prompt.md`](goal-wave-2026-08-28-algorithm-wave9-plan-and-mega-prompt.md)  
调研：[`algorithm-wave9-market-formula-research-2026-08-28.md`](algorithm-wave9-market-formula-research-2026-08-28.md)  
DoD：[`goal-wave-2026-08-28-algorithm-wave9.md`](goal-wave-2026-08-28-algorithm-wave9.md)

| Wave-9 | id | 备注 |
|--------|-----|------|
| W9-1 | `expanded_gage_unbalanced` | 不平衡 Expanded Gage GLM 方差分量 ✅ |
| W9-2 | `split_plot_analyze` | 裂区 WP/SP 双误差 ANOVA ✅ |
| W9-3 | `mixture_process_variable` | Scheffé + 过程变量 + 可选交互 ✅ |
| W9-4 | `manova_one_way` | 单因子 MANOVA 四检验 ✅ |

### 15.6 算法 Wave-10 锁定（2026-08-31 · ✅）

计划与 Mega `/goal`：[`goal-wave-2026-08-31-algorithm-wave10-plan-and-mega-prompt.md`](goal-wave-2026-08-31-algorithm-wave10-plan-and-mega-prompt.md)  
调研：[`algorithm-wave10-market-formula-research-2026-08-31.md`](algorithm-wave10-market-formula-research-2026-08-31.md)  
DoD：[`goal-wave-2026-08-31-algorithm-wave10.md`](goal-wave-2026-08-31-algorithm-wave10.md)

| Wave-10 | id | 备注 |
|--------|-----|------|
| W10-1 | `general_manova` | General MANOVA Type III SSCP ✅ |
| W10-2 | `mixed_effects_reml` | 单随机 REML 方差分量 ✅ |
| W10-3 | `binary_doe_probit` | Probit/Gompit IRWLS ✅ |
| W10-4 | `life_data_lognormal` | Lognormal MLE + 删失 ✅ |

### 15.7 算法 Wave-11 锁定（2026-09-01 · ✅）

计划与 Mega `/goal`：[`goal-wave-2026-09-01-algorithm-wave11-plan-and-mega-prompt.md`](goal-wave-2026-09-01-algorithm-wave11-plan-and-mega-prompt.md)  
调研：[`algorithm-wave11-market-formula-research-2026-09-01.md`](algorithm-wave11-market-formula-research-2026-09-01.md)  
DoD：[`goal-wave-2026-09-01-algorithm-wave11.md`](goal-wave-2026-09-01-algorithm-wave11.md)

| Wave-11 | id | 备注 |
|--------|-----|------|
| W11-1 | `simple_correspondence` | 简单对应分析（2 列分类）✅ |
| W11-2 | `multiple_correspondence` | 多重对应分析（3～6 列）✅ |
| W11-3 | `nonlinear_regression` | 非线性回归内置模型 GN/LM ✅ |
| W11-4 | `split_plot_design` | 2 水平裂区设计生成 ✅ |
