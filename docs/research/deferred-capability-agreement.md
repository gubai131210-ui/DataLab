# 延后算法：Minitab golden / 未锁定项

> 研究日期：2026-08-18（2026-08-20 更新状态）
> 状态：Kendall W/τ、两参数指数、三参数对数正态、**Weighted Kappa（Cohen linear/quadratic）**、
> **配对 TOST**、**泊松率比**、**Wilson 比例 CI**、**Bonett 等方差**、**泊松功效**、
> **Tukey 区间表形**、**Agresti–Coull**、**Bartlett**、**DOE 实际单位 hold**、
> **Tukey Grouping 字母**、**双样本均值比 TOST**、**两比例 Newcombe–Wilson**、
> **Kruskal Dunn**、**Multi-Vari 第 4 因子**、**TOST 对数变换**、**两比例 Agresti–Coull**、
> **Steel–Dwass（近似）**、**Friedman** 公式已实现（非 Minitab golden，除非另有导出）。Blaker /
> 可旋转 3D / Jackson–Mudholkar 解析限等仍延后。

## 1. Johnson / 非正态能力

| 项 | 状态 |
|---|---|
| Minitab 参考 | [Johnson transformed data](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/methods-and-formulas/johnson-transformed-data/)；[Z-score method](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/supporting-topics/capability-metrics/z-score-method-for-nonnormal-capability/) |
| 领域接口 | `fit_johnson_transform`、`ProcessCapability::calculate_johnson` / `calculate_nonnormal` |
| 实现 | **公式已实现**（Chou 1998 + AD p>0.10；Weibull/Lognormal Z-score Pp/Ppk）。找不到变换或规格越界只输出诊断，不伪造 Pp。 |
| 与 Minitab | **未对齐** — 等待导出后再写入 `VALIDATION_MATRIX.md`；当前测试为 `# source: formula_reference` |
| 所需 golden | 至少 1 组非正态测量 + LSL/USL + Johnson 变换后 Pp/Ppk；另 1 组非正态 Z-score |

## 2. 三参数 Weibull / 两参数指数 / 三参数对数正态

| 项 | 状态 |
|---|---|
| 领域接口 | `fit_weibull3`、`fit_exponential2`、`fit_lognormal3` |
| 实现 | **公式已实现**（剖面似然）。菜单 `weibull3` / `exponential2` / `lognormal3`。无界似然只诊断。 |
| 与 Minitab | **未对齐** — 不实现 Lockhart–Stephens / Minitab bias-correction 数值 |
| 所需 golden | 含阈值的删失数据集 + Minitab 参数表（Shape/Scale/Threshold 或 Location/Scale/Threshold） |

## 3. Fleiss Kappa、Kendall 与 Weighted Kappa

| 项 | 状态 |
|---|---|
| 当前实现 | 两两 `cohen_unweighted`；≥3 评估者 overall `fleiss`；`ratings_are_ordinal=true` 时 Kendall W/τ |
| 配置 | `MsaConfiguration::ratings_are_ordinal` 默认 false；`kappa_weight_scheme` 默认 `"none"` |
| Weighted Kappa | **已实现（DataLab）** — `linear` / `quadratic` Cohen 加权；配置与 Facts 已序列化；Fleiss overall **仍未加权**并诊断 `fleiss_remains_unweighted` |
| 与 Minitab AAA | **刻意不同** — Minitab 有序评级走 Kendall，无 linear/quadratic Kappa；不得把 DataLab κ_w 当 Minitab golden |
| Kendall W/τ | **公式已实现** — 有序评级官方用 Kendall；等待 Minitab 导出 golden |
| 所需 golden | 多评估者表 + Minitab Fleiss overall；有序表 + Kendall（**不要**用 Minitab Kappa 表对 linear κ） |

详见 [`p1_weighted_kappa_cohen.md`](p1_weighted_kappa_cohen.md)。

## 4. 已落地、勿再当延后项

| 项 | 状态 |
|---|---|
| 配对 TOST `paired_equivalence` | **已落地** — 见 session-brief §5a |
| 泊松率比 `two_poisson_rate` + `comparison=ratio` | **已落地** — 默认仍 `difference`；见 [`p1_poisson_rate_ratio.md`](p1_poisson_rate_ratio.md) |
| 比例 z-TOST | **已落地** — `one_proportion_equivalence` / `two_proportion_equivalence` |
| DOE 等值线 X/Y + hold 0 | **已落地** — 不做可旋转 3D |
| Wilson 单比例 CI | **已落地** — `one_proportion` method=`wilson` |
| Bonett 两样本等方差 | **已落地** — `variance_test_method=bonett`；k>2 诊断 |
| 泊松率功效 | **已落地** — `t_power` `one_poisson_*` / `two_poisson_*` |
| Tukey 区间表形 | **已落地** — 下限/上限列 + 差值区间图；近似算法不变 |
| Agresti–Coull 单比例 CI | **已落地** — `one_proportion` method=`agresti_coull`；见 [`p1_agresti_coull_proportion_ci.md`](p1_agresti_coull_proportion_ci.md) |
| Bartlett 等方差 | **已落地** — `variance_test_method=bartlett`；见 [`p1_bartlett_equal_variance.md`](p1_bartlett_equal_variance.md) |
| DOE 实际单位 hold | **已落地** — `contour_hold_actual`；见 [`p1_doe_actual_unit_hold.md`](p1_doe_actual_unit_hold.md) |
| Tukey Grouping 字母 | **已落地** — Grouping Information CLD；见 [`p1_anova_tukey_grouping_letters.md`](p1_anova_tukey_grouping_letters.md) |
| 双样本均值比 TOST | **已落地** — `two_sample_equivalence_ratio`；非对数；见 [`p1_two_sample_mean_ratio_tost.md`](p1_two_sample_mean_ratio_tost.md) |
| 两比例 Newcombe–Wilson | **已落地** — `two_proportions` method=`wilson`；见 [`p1_two_proportion_newcombe_wilson.md`](p1_two_proportion_newcombe_wilson.md) |
| Kruskal Dunn | **已落地** — Dunn–Bonferroni + Grouping；见 [`p1_kruskal_dunn_posthoc.md`](p1_kruskal_dunn_posthoc.md) |
| Multi-Vari 第 4 因子 | **已落地** — 2～4 因子；见 [`p1_multi_vari_fourth_factor.md`](p1_multi_vari_fourth_factor.md) |
| 均值比 TOST 对数 | **已落地** — `transform=log`；见 [`p1_tost_ratio_log_transform.md`](p1_tost_ratio_log_transform.md) |
| 两比例 Agresti–Coull | **已落地** — method=`agresti_coull`；见 [`p1_two_proportion_agresti_coull_ci.md`](p1_two_proportion_agresti_coull_ci.md) |
| Kruskal Steel–Dwass | **已落地（近似）** — `posthoc=steel_dwass`；见 [`p1_kruskal_steel_dwass.md`](p1_kruskal_steel_dwass.md) |
| Friedman | **已落地** — 命令 `friedman`；见 [`p1_friedman_test.md`](p1_friedman_test.md) |

## 5. 仍延后

- Blaker / Adjusted Blaker（比例区间族；**Wilson / Agresti–Coull / 两比例 Newcombe–Wilson / 两比例 AC 已落地**）
- 泊松 Blaker（**泊松功效已落地**，见 [`p1_poisson_rate_power.md`](p1_poisson_rate_power.md)）
- Jackson–Mudholkar T²/Q 解析限（**Bonett / Bartlett / Tukey 表形与字母已落地**）
- 可旋转 3D 曲面
- Minitab 无界似然 bias-correction 数值对齐
- Kalman 状态空间 MLE；Minitab TSERIES 迭代最小二乘 + back forecast 数值对齐
- 图表注释、拖拽布局、多图拼版
- Nemenyi 独立命令 / Friedman 后比较（**Steel–Dwass 近似与 Friedman 主检验已落地**）
- 重构阶段 5/6（PlotSpec 合一、CI、i18n），除非挡住接线

## 6. 下一批接入条件

Johnson / 非正态 / 三参数 Weibull / 两参数指数 / 三参数对数正态 / Fleiss / Kendall：公式已实现，只需从 Minitab 导出对照后写入 golden，禁止猜测数值。

Weighted Kappa 不作为 Minitab 功能验收。PCA T²/Q 经验分位、非参数 ties、Levene 中位数口径、Wilson / Agresti–Coull / Bonett / Bartlett / 泊松功效 / Tukey 表形与字母是公式参考，不是 Minitab golden。
