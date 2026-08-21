# DataLab 算法接线索引

> 不改计算。对照 `analysis_commands.cpp` 命令 id、`AnalysisService` / `GraphService` 方法、`OutputPage.facts` 与公式文档。
> 导入契约见 [`research/algorithm-chart-gap-matrix.md`](research/algorithm-chart-gap-matrix.md) §3。
> 访问日期：2026-08-20。

## 1. 公式文档索引

| 主题 | 文档 |
|---|---|
| SPC Test / 能力 / Johnson / Laney 阶段 | [`research/spc-control-charts.md`](research/spc-control-charts.md) §5.1 |
| SPC 直方图 / 卡方 / Grubbs | [`research/spc-capability-chi-grubbs-formulas.md`](research/spc-capability-chi-grubbs-formulas.md) |
| 交叉 Gage By Part / 交互图 | [`research/gage-by-part-interaction-formulas.md`](research/gage-by-part-interaction-formulas.md) |
| Nested Gage By Part | 同上 §2（`append_gage_by_part_plot`） |
| Gage 分量条 / 按操作者 Xbar-R | [`research/poisson-rate-anova-winters-gage-formulas.md`](research/poisson-rate-anova-winters-gage-formulas.md) §5、§5.1 |
| Nested Gage / ARIMA / Kappa | [`research/nested-gage-arima-kappa-anova-formulas.md`](research/nested-gage-arima-kappa-anova-formulas.md) |
| 指数2 / 对数正态3 / 概率图 | [`research/kendall-exp2-lognormal3-formulas.md`](research/kendall-exp2-lognormal3-formulas.md) |
| Weibull3 | [`research/weibull3-fleiss-chart-formulas.md`](research/weibull3-fleiss-chart-formulas.md) |
| 回归 Unusual / Fitted Line | [`research/unusual-obs-multivari-doe-chart-formulas.md`](research/unusual-obs-multivari-doe-chart-formulas.md)、[`research/regression-doe-tolerance-chart-formulas.md`](research/regression-doe-tolerance-chart-formulas.md) |
| 非参数 / McKean–Ryan / 卡方热图 | [`research/pca-nonparametric-variance-chart-formulas.md`](research/pca-nonparametric-variance-chart-formulas.md) §3、§5 |
| Type 1 / Bias Linearity | [`research/type1-bias-twoprop-boxcox-formulas.md`](research/type1-bias-twoprop-boxcox-formulas.md) §2–§3 |
| 能力 CI / 拟合优度 / G·T / 功效 | [`research/capability-ci-gof-rare-event-power-formulas.md`](research/capability-ci-gof-rare-event-power-formulas.md)、[`adr/0008-capability-index-confidence-intervals.md`](adr/0008-capability-index-confidence-intervals.md) |
| 导入 / 分层 | [`algorithm-session-brief.md`](algorithm-session-brief.md) §3、ADR 0001 / 0003 / 0004 / 0007 |
| P0 控制图规则扩展 | [`research/p0_control_chart_special_cause_rules.md`](research/p0_control_chart_special_cause_rules.md) |
| P0 TOST 输出统一 | [`research/p0_equivalence_tost_minitab_alignment.md`](research/p0_equivalence_tost_minitab_alignment.md) |
| P0 GOF 有效性提示 | [`research/p0_chi_square_gof_validity_guidance.md`](research/p0_chi_square_gof_validity_guidance.md) |
| P1 容差区间一致化 | [`research/p1_tolerance_intervals_methods.md`](research/p1_tolerance_intervals_methods.md) |
| P1 Logistic 诊断增强 | [`research/p1_logistic_diagnostics_minitab.md`](research/p1_logistic_diagnostics_minitab.md) |
| P1 配对等价 | [`research/p1_paired_equivalence_tost.md`](research/p1_paired_equivalence_tost.md) |
| P1 方差功效 | [`research/p1_power_variance_sample_size.md`](research/p1_power_variance_sample_size.md) |
| P1 DOE 精确预测区间 | [`research/p1_doe_response_prediction_intervals.md`](research/p1_doe_response_prediction_intervals.md) |
| P1 Gage `%Tolerance` / Bias 表形 | [`research/p1_gage_percent_tolerance_bias_aiag.md`](research/p1_gage_percent_tolerance_bias_aiag.md) |
| P1 比例等价 z-TOST | [`research/p1_proportion_equivalence_z_tost.md`](research/p1_proportion_equivalence_z_tost.md) |
| P1 Weighted Kappa | [`research/p1_weighted_kappa_cohen.md`](research/p1_weighted_kappa_cohen.md) |
| P1 DOE 等值线因子切换 | [`research/p1_doe_contour_factor_hold.md`](research/p1_doe_contour_factor_hold.md) |
| P1 Wilson 比例 CI | [`research/p1_wilson_proportion_ci.md`](research/p1_wilson_proportion_ci.md) |
| P1 Bonett 等方差 | [`research/p1_bonett_equal_variance.md`](research/p1_bonett_equal_variance.md) |
| P1 泊松率功效 | [`research/p1_poisson_rate_power.md`](research/p1_poisson_rate_power.md) |
| P1 ANOVA Tukey 区间表形 | [`research/p1_anova_tukey_interval_table.md`](research/p1_anova_tukey_interval_table.md) |
| P1 Agresti–Coull 比例 CI | [`research/p1_agresti_coull_proportion_ci.md`](research/p1_agresti_coull_proportion_ci.md) |
| P1 Bartlett 等方差 | [`research/p1_bartlett_equal_variance.md`](research/p1_bartlett_equal_variance.md) |
| P1 DOE 实际单位 hold | [`research/p1_doe_actual_unit_hold.md`](research/p1_doe_actual_unit_hold.md) |
| P1 ANOVA Tukey Grouping | [`research/p1_anova_tukey_grouping_letters.md`](research/p1_anova_tukey_grouping_letters.md) |
| P1 泊松率比 | [`research/p1_poisson_rate_ratio.md`](research/p1_poisson_rate_ratio.md) |
| P1 双样本均值比 TOST | [`research/p1_two_sample_mean_ratio_tost.md`](research/p1_two_sample_mean_ratio_tost.md) |
| P1 两比例 Newcombe–Wilson | [`research/p1_two_proportion_newcombe_wilson.md`](research/p1_two_proportion_newcombe_wilson.md) |
| P1 Kruskal Dunn | [`research/p1_kruskal_dunn_posthoc.md`](research/p1_kruskal_dunn_posthoc.md) |
| P1 TOST 对数 | [`research/p1_tost_ratio_log_transform.md`](research/p1_tost_ratio_log_transform.md) |
| P1 两比例 AC | [`research/p1_two_proportion_agresti_coull_ci.md`](research/p1_two_proportion_agresti_coull_ci.md) |
| P1 Steel–Dwass | [`research/p1_kruskal_steel_dwass.md`](research/p1_kruskal_steel_dwass.md) |
| P1 Friedman | [`research/p1_friedman_test.md`](research/p1_friedman_test.md) |
| P1 Friedman Nemenyi | [`research/p1_friedman_nemenyi_posthoc.md`](research/p1_friedman_nemenyi_posthoc.md) |
| P1 McNemar | [`research/p1_mcnemar_test.md`](research/p1_mcnemar_test.md) |
| P1 Sign test | [`research/p1_sign_test.md`](research/p1_sign_test.md) |
| P1 Durbin–Watson 临界 | [`research/p1_durbin_watson_critical.md`](research/p1_durbin_watson_critical.md) |
| P1 Mood 中位数 | [`research/p1_mood_median_test.md`](research/p1_mood_median_test.md) |
| P1 Cochran Q | [`research/p1_cochran_q.md`](research/p1_cochran_q.md) |
| P1 单样本 Wilcoxon | [`research/p1_wilcoxon_one_sample.md`](research/p1_wilcoxon_one_sample.md) |
| P1 帮助公式与来源页签 | [`research/p1_help_formula_sources_tab.md`](research/p1_help_formula_sources_tab.md) |
| P1 Multi-Vari 第 4 因子 | [`research/p1_multi_vari_fourth_factor.md`](research/p1_multi_vari_fourth_factor.md) |

## 2. 命令 id → 服务 → Facts

| 命令 id | 服务方法 | Facts | 主要测试 |
|---|---|---|---|
| descriptive | descriptive | descriptive | descriptive_statistics_test |
| normality_test | normality_test | normality | quality_statistics_test |
| outlier_test | outlier_test | outlier_test | grubbs_test |
| correlation | correlation | correlation | quality_statistics_test |
| one_sample_t / two_sample_t / paired_t | one_sample_t / two_sample_t / paired_t | t_test | quality_statistics_test |
| one_proportion / two_proportions | one_proportion / two_proportions | proportion（含 agresti_coull；两比例含 newcombe_wilson / agresti_coull_diff） | proportion_test |
| one_proportion_equivalence / two_proportion_equivalence | one_proportion_equivalence / two_proportion_equivalence | equivalence | equivalence_test |
| one_poisson_rate / two_poisson_rate | one_poisson_rate / two_poisson_rate | poisson_rate | poisson_rate_test |
| one_sample_equivalence / two_sample_equivalence / two_sample_equivalence_ratio / paired_equivalence | 同名 | equivalence（比值 kind=two_sample_ratio；可选 log） | equivalence_test |
| one_way_anova / two_factor_anova | one_way_anova / two_factor_anova | anova（含 tukey_grouping_*） | quality_statistics_test / two_factor_anova_output_test |
| regression | regression | regression | regression_output_test |
| mann_whitney / wilcoxon_signed_rank / sign_test / mood_median / kruskal_wallis / friedman | 同名 | nonparametric（含 dunn/steel_dwass/nemenyi/sign_test/friedman/mood/wilcoxon_one_sample） | quality_statistics_test |
| mcnemar / cochran_q | mcnemar / cochran_q | mcnemar / cochran_q | quality_statistics_test |
| chi_square | chi_square | chi_square（含 plot_available） | quality_statistics_test |
| chi_square_gof | chi_square_gof | chi_square_gof（含 validity_status、minimum_expected_count、recommendation） | capability_ci_gof_rare_event_power_test |
| variance_test | variance_test | variance（含 bartlett） | quality_statistics_test |
| logistic_regression | logistic_regression | logistic（含 leverage_threshold、maximum_leverage、maximum_vif） | quality_statistics_test |
| pca | pca | pca | quality_statistics_test |
| multi_vari | multi_vari | multi_vari | multi_vari_test |
| reliability | reliability | reliability | reliability_msa_power_test |
| t_power | t_power | power（含 actual_power；支持 one_variance_* / two_variance_*） | capability_ci_gof_rare_event_power_test / reliability_msa_power_test |
| time_series_smoothing / decomposition / seasonal_forecasting / arima | 同名 | forecast | seasonal_forecasting_output_test |
| imr | individuals_moving_range | spc | special_cause_rule_test / imr_rs_test / xbar_output_test |
| xbar_r / xbar_s | xbar_range / xbar_s | spc | xbar_output_test / quality_statistics_test |
| imr_rs | imr_rs | spc | imr_rs_test |
| p_chart / np_chart / c_chart / u_chart | 同名 | spc | quality_statistics_test |
| laney_p_chart / laney_u_chart | laney_p_chart / laney_u_chart | spc | quality_statistics_test |
| ewma / cusum | ewma / cusum | spc | ewma_cusum_output_test / special_cause_rule_test |
| g_chart / t_chart | g_chart / t_chart | spc | capability_ci_gof_rare_event_power_test |
| capability / nonnormal_capability | capability | capability | quality_statistics_test |
| between_within_capability | between_within_capability | capability | quality_statistics_test |
| binomial_capability / poisson_capability | 同名 | capability | attribute_capability_test |
| capability_sixpack | capability_sixpack | capability | quality_statistics_test |
| box_cox | box_cox | box_cox | quality_statistics_test |
| distribution_identification | distribution_identification | distribution_identification | quality_statistics_test |
| multi_vari | multi_vari | multi_vari | multi_vari_test |
| tolerance_intervals | tolerance_intervals | tolerance（含 method_family、achieved_confidence） | tolerance_interval_test |
| gage_rr | gage_rr | msa | gage_rr_output_test |
| nested_gage_rr | nested_gage_rr | msa | gage_rr_output_test |
| msa_type1（含 bias_linearity / stability） | msa_type1 | msa | reliability_msa_power_test |
| attribute_agreement | attribute_agreement | msa | reliability_msa_power_test |
| doe_factorial / doe_response | doe_factorial | doe（含 contour_*、held_actual/coded） | doe_factorial_output_test |
| response_optimization | response_optimization | doe（含 prediction_interval_available） | doe_response_test |
| 图形族 scatter_plot…pie_plot | GraphService::run | — | graph_service_test |

图形命令不写质量 Facts。解释层只读已填充的 `*Facts`，不写过程合格 / 量具通过 / 已证明正态。

## 3. 导入契约（摘要）

complete-case 行主序 `align_complete_rows`；`parse_numeric_cell` / `is_missing_cell`；保留 `source_row`。重导入 B 后排除行 / 输出页 / undo / 行选择失效（`import_state_reset_test`）。

## 4. 算法与公式帮助中心

- 菜单：**帮助 → 算法、公式与参考资料**（`AlgorithmHelpDialog`）。
- 详情区分页签：**方法说明** | **公式与来源**（后者仅公式块 + 官方链接，不含仓库 md / wiring）。
- 内置资源：`resources/help/algorithm_help.json`（`:/help/algorithm_help.json`）。**最终用户不需要仓库或 Markdown 文件**：每条命令都内置用途、输入、缺失值规则、计算步骤、公式、符号、判定条件、不可计算边界和解释限制。
- 官方网页仅作延伸阅读；核心方法说明在程序内离线可读。
- 仓库 `docs/research/*.md` 只出现在详情页底部的维护信息中，供开发者追溯，不是用户说明书。
- `formula_reference` 表示公式参考，**不是** Minitab golden；延后/诊断能力不得标为已实现。
- 自动化：`algorithm_help_catalog_test`（用户字段完整性、命令覆盖、禁止把说明写成“见 md”、公式节点、https 链接）；`algorithm_help_dialog_test`（树、搜索、步骤/符号/判定、复制摘要）。
- 手工验收：无 `docs/` 目录、断网仍能读懂计算方法；100%/150% 缩放与深色主题下公式可读。
