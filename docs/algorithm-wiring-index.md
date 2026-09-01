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
| 报告可读性 / 数据库导入研究 | [`research/report-format-database-import-research.md`](research/report-format-database-import-research.md) |
| 算法竖切 + 报告产品化计划 | [`research/vertical-slice-algorithms-and-report-product-plan.md`](research/vertical-slice-algorithms-and-report-product-plan.md) |
| Phase 0 报告证据契约 | [`research/phase0-report-evidence-contracts.md`](research/phase0-report-evidence-contracts.md)、`src/domain/report_types.h` |
| 验证矩阵（formula/reference/vendor/golden） | [`research/VALIDATION_MATRIX.md`](research/VALIDATION_MATRIX.md) |
| Phase 2 PDF/A·UA 评估 | [`research/phase2-pdfa-pdfua-assessment.md`](research/phase2-pdfa-pdfua-assessment.md) |
| 特殊原因规则目录 | `src/domain/statistics/special_cause_rule_catalog.*`（8 稳定 ID） |
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
| P2 EDA 四图 / 交叉表 / 卡方残差 | [`research/p2_eda4_crosstab_chi_resid.md`](research/p2_eda4_crosstab_chi_resid.md) |
| P2 T² / MEWMA / Nelson / EMP | [`research/p2_t2_mewma_nelson_emp.md`](research/p2_t2_mewma_nelson_emp.md) |
| P2 GV / Expanded / B4–B5 | [`research/p2_gv_expanded_b4b5.md`](research/p2_gv_expanded_b4b5.md) |
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
| P1 Sign CI | [`research/p1_sign_confidence_interval.md`](research/p1_sign_confidence_interval.md) |
| P1 Mood 组 Sign CI | [`research/p1_mood_group_sign_ci.md`](research/p1_mood_group_sign_ci.md) |
| P1 配对 Wilcoxon Walsh CI | [`research/p1_paired_wilcoxon_walsh_ci.md`](research/p1_paired_wilcoxon_walsh_ci.md) |
| P1 Runs / Fisher / Run Chart / 鱼骨 | [`research/p1_runs_fisher_runchart_fishbone.md`](research/p1_runs_fisher_runchart_fishbone.md) |
| P1 Multi-Vari 第 4 因子 | [`research/p1_multi_vari_fourth_factor.md`](research/p1_multi_vari_fourth_factor.md) |
| P1 Dixon / 1-Sample Z / Variability / 非参数容差 | [`research/p1_dixon_z_variability_nptol.md`](research/p1_dixon_z_variability_nptol.md) |
| P2 DOE 设计生成 / ACF-PACF / 等价·DOE·容差功效 | [`research/p2_doe_design_acf_power.md`](research/p2_doe_design_acf_power.md) |

## 2. 命令 id → 服务 → Facts

| 命令 id | 服务方法 | Facts | 主要测试 |
|---|---|---|---|
| descriptive | descriptive | descriptive | descriptive_statistics_test |
| normality_test | normality_test | normality | quality_statistics_test |
| outlier_test | outlier_test | outlier_test（method=grubbs\|dixon_r10） | grubbs_test |
| correlation | correlation | correlation（含 covariance_available、partial_available） | quality_statistics_test |
| one_sample_t / two_sample_t / paired_t | one_sample_t / two_sample_t / paired_t | t_test | quality_statistics_test |
| one_sample_z | one_sample_z | t_test（kind=one_sample_z） | quality_statistics_test |
| one_proportion / two_proportions | one_proportion / two_proportions | proportion（含 agresti_coull；两比例含 newcombe_wilson / agresti_coull_diff） | proportion_test |
| one_proportion_equivalence / two_proportion_equivalence | one_proportion_equivalence / two_proportion_equivalence | equivalence | equivalence_test |
| one_poisson_rate / two_poisson_rate | one_poisson_rate / two_poisson_rate | poisson_rate | poisson_rate_test |
| one_sample_equivalence / two_sample_equivalence / two_sample_equivalence_ratio / paired_equivalence | 同名 | equivalence（比值 kind=two_sample_ratio；可选 log） | equivalence_test |
| one_way_anova / two_factor_anova | one_way_anova / two_factor_anova | anova（含 tukey_grouping_*） | quality_statistics_test / two_factor_anova_output_test |
| regression | regression | regression | regression_output_test |
| mann_whitney / wilcoxon_signed_rank / sign_test / mood_median / kruskal_wallis / friedman | 同名 | nonparametric（含 dunn/steel_dwass/nemenyi/sign_test/friedman/mood/wilcoxon_one_sample） | quality_statistics_test |
| mcnemar / cochran_q | mcnemar / cochran_q | mcnemar / cochran_q | quality_statistics_test |
| chi_square | chi_square | chi_square（含 percent_tables / residual_heatmap / max_abs_adjusted_residual） | quality_statistics_test / p2_eda4_crosstab_chi_resid_test |
| cross_tabulation | cross_tabulation | cross_tab | p2_eda4_crosstab_chi_resid_test |
| chi_square_gof | chi_square_gof | chi_square_gof（含 validity_status、minimum_expected_count、recommendation） | capability_ci_gof_rare_event_power_test |
| poisson_gof | poisson_goodness_of_fit | chi_square_gof（method=poisson、lambda_hat） | quality_statistics_test |
| acceptance_sampling | acceptance_sampling_binomial | acceptance_sampling | quality_statistics_test |
| anom | analysis_of_means | anom | quality_statistics_test |
| variance_test | variance_test | variance（含 bartlett） | quality_statistics_test |
| logistic_regression | logistic_regression | logistic（含 leverage_threshold、maximum_leverage、maximum_vif） | quality_statistics_test |
| pca | pca | pca | quality_statistics_test |
| kmeans | kmeans | kmeans | p3_batch1_kmeans_cart_adf_test |
| cart_tree | cart_tree | cart_tree | p3_batch1_kmeans_cart_adf_test |
| adf_test | adf_test | adf | p3_batch1_kmeans_cart_adf_test |
| poisson_regression | poisson_regression | poisson_regression | p3_batch2_poisson_iforest_bootstrap_hclust_test |
| isolation_forest | isolation_forest | isolation_forest | p3_batch2_poisson_iforest_bootstrap_hclust_test |
| bootstrap_mean | bootstrap_mean | bootstrap_mean | p3_batch2_poisson_iforest_bootstrap_hclust_test |
| bootstrap_two_sample | bootstrap_two_sample | bootstrap_two_sample | algorithm_wave3_track_test |
| probit_reliability | probit_reliability | probit_reliability | algorithm_wave3_track_test |
| cluster_observations | cluster_observations | hierarchical_cluster | p3_batch2_poisson_iforest_bootstrap_hclust_test |
| ordinal_logistic | ordinal_logistic | ordinal_logistic | p3_batch3_ordinal_lda_ccf_correlogram_test |
| nominal_logistic | nominal_logistic | nominal_logistic | algorithm_wave2_track_test |
| nonparametric_capability | nonparametric_capability | nonparametric_capability | algorithm_wave4_track_test |
| cox_regression | cox_regression | cox_regression | algorithm_wave4_track_test |
| accelerated_life | accelerated_life | accelerated_life | algorithm_wave2_track_test |
| discriminant | discriminant | discriminant | p3_batch3_ordinal_lda_ccf_correlogram_test |
| ccf | ccf | ccf | p3_batch3_ordinal_lda_ccf_correlogram_test |
| correlogram | correlogram | correlogram | p3_batch3_ordinal_lda_ccf_correlogram_test |
| stepwise_regression | stepwise_regression | stepwise_regression | p3_track_h_stepwise_km_pb_test |
| best_subsets_regression | best_subsets_regression | best_subsets_regression | algorithm_batch_2026_08_test |
| batch_capability | batch_capability | batch_capability | algorithm_batch_2026_08_test |
| logistic_regression | logistic_regression | logistic（含 leverage_threshold、maximum_leverage、maximum_vif、stepwise） | quality_statistics_test, algorithm_wave4_track_test |
| km_interval | km_interval | km_interval | p3_track_h_stepwise_km_pb_test |
| doe_plackett_burman | doe_plackett_burman | plackett_burman | p3_track_h_stepwise_km_pb_test |
| random_forest | random_forest | random_forest | algorithm_wave5_track_test |
| weibayes | weibayes | weibayes | algorithm_wave5_track_test |
| taguchi_orthogonal_design | taguchi_orthogonal_design | taguchi_orthogonal | algorithm_wave5_track_test |
| distribution_calculator | distribution_calculator | distribution_calculator | algorithm_wave5_track_test |
| taguchi_analyze | taguchi_analyze | taguchi_analyze | algorithm_wave6_track_test |
| mixture_design | mixture_design | mixture_design | algorithm_wave6_track_test |
| mixture_analyze | mixture_analyze | mixture_analyze | algorithm_wave7_track_test |
| glm_two_way | glm_two_way | glm_two_way | algorithm_wave7_track_test |
| analyze_variability | analyze_variability | analyze_variability | algorithm_wave7_track_test |
| factor_analysis | factor_analysis | factor_analysis | algorithm_wave7_track_test |
| binary_response_doe | binary_response_doe | binary_response_doe | algorithm_wave8_track_test |
| cluster_variables | cluster_variables | cluster_variables | algorithm_wave8_track_test |
| glm_three_factor | glm_three_factor | glm_three_factor | algorithm_wave8_track_test |
| life_data_regression | life_data_regression | life_data_regression | algorithm_wave8_track_test |
| expanded_gage_unbalanced | expanded_gage_unbalanced | expanded_gage_unbalanced | algorithm_wave9_track_test |
| split_plot_analyze | split_plot_analyze | split_plot_analyze | algorithm_wave9_track_test |
| mixture_process_variable | mixture_process_variable | mixture_process_variable | algorithm_wave9_track_test |
| manova_one_way | manova_one_way | manova_one_way | algorithm_wave9_track_test |
| general_manova | general_manova | general_manova | algorithm_wave10_track_test |
| mixed_effects_reml | mixed_effects_reml | mixed_effects_reml | algorithm_wave10_track_test |
| binary_doe_probit | binary_doe_probit | binary_doe_probit | algorithm_wave10_track_test |
| life_data_lognormal | life_data_lognormal | life_data_lognormal | algorithm_wave10_track_test |
| simple_correspondence | simple_correspondence | simple_correspondence | algorithm_wave11_track_test |
| multiple_correspondence | multiple_correspondence | multiple_correspondence | algorithm_wave11_track_test |
| nonlinear_regression | nonlinear_regression | nonlinear_regression | algorithm_wave11_track_test |
| split_plot_design | split_plot_design | split_plot_design | algorithm_wave11_track_test |
| nhpp_repairable | nhpp_repairable | nhpp_repairable | algorithm_wave6_track_test |
| reliability_test_plan | reliability_test_plan | reliability_test_plan | algorithm_wave6_track_test |
| doe_ccd | doe_response_surface_design | design_generation | response_surface_design_phase4_test |
| doe_bbd | doe_response_surface_design | design_generation | response_surface_design_phase4_test |
| multi_vari | multi_vari | multi_vari | multi_vari_test |
| reliability | reliability | reliability（含 evidence_type、log_rank K 组、CIF/Gray） | reliability_msa_power_test / algorithm_wave4_track_test |
| reliability_warranty | reliability_warranty | warranty | reliability_phase5_test |
| t_power | t_power | power（含 actual_power；equivalence / doe_factorial / tolerance mode） | p2_doe_acf_power_test / capability_ci_gof_rare_event_power_test |
| acf_pacf | acf_pacf | acf_pacf | p2_doe_acf_power_test |
| rsm_response | rsm_response | rsm（含 design_source_id / coding_mode / surface_is_static） | p2_rsm_special_cause_test / response_surface_design_phase4_test |
| special_cause_rules（help） | SpecialCauseRuleCatalog / SpcFacts.rules | spc.rules + enabled_special_cause_rule_ids | special_cause_rule_catalog_test / special_cause_rule_test / p2_rsm_special_cause_test |
| import_database（菜单） | DatabaseImportService + SQLite Provider | ConnectionInfo / TableMetadata / ImportPlan / ImportedTable | database_provider_registry_test / database_import_test |
| density_plot | GraphService::density | eda | p2_eda_plots_test |
| hexbin_plot | GraphService::hexbin | eda | p2_eda_plots_test |
| violin_plot | GraphService::violin | eda | p2_eda_plots_test |
| bar_chart | GraphService::bar | eda | p2_eda_plots_test |
| eda_4plot | eda_4plot | eda（kind=eda_4plot） | p2_eda4_crosstab_chi_resid_test |
| hotelling_t2 | hotelling_t2 | multivariate_spc | p2_t2_mewma_nelson_emp_test |
| mewma | mewma | multivariate_spc | p2_t2_mewma_nelson_emp_test |
| generalized_variance | generalized_variance | multivariate_spc | p2_gv_expanded_b4b5_test |
| emp_crossed | emp_crossed | msa（emp_*） | p2_t2_mewma_nelson_emp_test |
| expanded_gage_rr | expanded_gage_rr | msa | p2_gv_expanded_b4b5_test |
| imr | individuals_moving_range | spc（rules / rule_ids / sigma_method / nelson / historical+stage） | special_cause_rule_catalog_test / p2_t2_mewma_nelson_emp_test / p2_gv_expanded_b4b5_test / special_cause_rule_test |
| time_series_smoothing / decomposition / seasonal_forecasting / arima | 同名 | forecast | seasonal_forecasting_output_test |
| imr | individuals_moving_range | spc | special_cause_rule_test / imr_rs_test / xbar_output_test |
| xbar_r / xbar_s | xbar_range / xbar_s | spc | xbar_output_test / quality_statistics_test |
| imr_rs | imr_rs | spc | imr_rs_test |
| p_chart / np_chart / c_chart / u_chart | 同名 | spc | quality_statistics_test |
| laney_p_chart / laney_u_chart | laney_p_chart / laney_u_chart | spc | quality_statistics_test |
| ewma / cusum | ewma / cusum | spc | ewma_cusum_output_test / special_cause_rule_test |
| zone_chart / z_mr / moving_average | zone_chart / z_mr / moving_average | zone_chart / z_mr / moving_average + spc | zone_zmr_ma_output_test |
| g_chart / t_chart | g_chart / t_chart | spc | capability_ci_gof_rare_event_power_test |
| capability / nonnormal_capability | capability | capability | quality_statistics_test |
| between_within_capability | between_within_capability | capability | quality_statistics_test |
| binomial_capability / poisson_capability | 同名 | capability | attribute_capability_test |
| capability_sixpack | capability_sixpack | capability | quality_statistics_test |
| box_cox | box_cox | box_cox | quality_statistics_test |
| distribution_identification | distribution_identification | distribution_identification | quality_statistics_test |
| variability_chart | variability_chart | variability | quality_statistics_test |
| tolerance_intervals | tolerance_intervals | tolerance（含 method_family、achieved_confidence；显式 tolerance_method） | tolerance_interval_test |
| gage_rr | gage_rr | msa | gage_rr_output_test |
| nested_gage_rr | nested_gage_rr | msa | gage_rr_output_test |
| msa_type1（含 bias_linearity / stability） | msa_type1 | msa | reliability_msa_power_test |
| attribute_agreement | attribute_agreement | msa | reliability_msa_power_test |
| doe_factorial / doe_response | doe_factorial | doe（含 design_kind/fraction_p/resolution/run_count、contour_*） | p2_doe_acf_power_test / doe_factorial_output_test |
| response_optimization | response_optimization | doe（含 prediction_interval_available） | doe_response_test |
| 图形族 scatter_plot…pie_plot | GraphService::run | — | graph_service_test |

图形命令不写质量 Facts。解释层只读已填充的 `*Facts`，不写过程合格 / 量具通过 / 已证明正态。

## 3. 导入契约（摘要）

complete-case 行主序 `align_complete_rows`；`parse_numeric_cell` / `is_missing_cell`；保留 `source_row`。重导入 B 后排除行 / 输出页 / undo / 行选择失效（`import_state_reset_test`）。

## 4. 算法与公式帮助中心

- 菜单：**帮助 → 算法、公式与参考资料**（`AlgorithmHelpDialog`）。
- 菜单：**帮助 → 公式注册表**（`FormulaRegistryDialog`）：按命令 id 搜索公式块、Primary URL、research md 路径；可从帮助中心「在公式注册表中打开」跳转。
- 详情区分页签：**方法说明** | **公式与来源**（后者仅公式块 + 官方链接，不含仓库 md / wiring）。
- 内置资源：`resources/help/algorithm_help.json`（`:/help/algorithm_help.json`）。**最终用户不需要仓库或 Markdown 文件**：每条命令都内置用途、输入、缺失值规则、计算步骤、公式、符号、判定条件、不可计算边界和解释限制。
- 官方网页仅作延伸阅读；核心方法说明在程序内离线可读。
- 仓库 `docs/research/*.md` 只出现在详情页底部的维护信息中，供开发者追溯，不是用户说明书。
- `formula_reference` 表示公式参考，**不是** Minitab golden；延后/诊断能力不得标为已实现。
- 自动化：`algorithm_help_catalog_test`（用户字段完整性、命令覆盖、禁止把说明写成“见 md”、公式节点、https 链接）；`algorithm_help_dialog_test`（树、搜索、步骤/符号/判定、复制摘要）。
- 手工验收：无 `docs/` 目录、断网仍能读懂计算方法；100%/150% 缩放与深色主题下公式可读。

## 5. Track G1+G2：公式注册表与输出复制（2026-08-22）

> 研究：[`research/g1-g2-formula-registry-chart-copy.md`](research/g1-g2-formula-registry-chart-copy.md)  
> 验收：[`../samples/product_evolution/g1_g2_manual_acceptance.md`](../samples/product_evolution/g1_g2_manual_acceptance.md)

| 能力 | UI / 模块 | 数据 / 契约 | 测试 |
|---|---|---|---|
| **G1** 公式注册表 | `FormulaRegistryDialog`；菜单 帮助→公式注册表；`AlgorithmHelpDialog`「在公式注册表中打开」 | `resources/help/algorithm_help.json` → `AlgorithmHelpCatalogLoader` | `formula_registry_dialog_test`、`algorithm_help_dialog_test` |
| **G2** 图表复制 PNG+BMP+脚注 | `AnalysisChartWidget::copy_to_clipboard` → `row_visibility_clipboard` | `excluded_count_` / `hidden_count_` / `analysis_n_` / `display_n_`（Phase 7 语义，hidden≠excluded） | `analysis_chart_widget_test`、`row_visibility_clipboard_test` |
| **G2** 表格 TSV/CSV+`#` 注释 | `page_renderer` 右键复制 / Ctrl+C / 导出 CSV | `append_clipboard_footnote_comments` | `row_visibility_clipboard_test` |
| **G2** 多图焦点与 Ctrl+C 路由 | `OutputWorkspace::chart_for_copy`、`QShortcut(Ctrl+C)` → `copy_chart_requested`；`MainWindow::copy_chart` / `copy_selection`（输出表优先 TSV） | 输出页 ScrollArea / 图表 surface 焦点追踪 | `output_workspace_test` |

批量测试：`powershell -File tools/run_g1g2_tests.ps1`（Qt Creator 构建后）。

## 6. UI 菜单信息架构（Menu IA · 2026-08-23）

> 分类：[`research/ui-menu-ia-minitab-taxonomy-2026-08-23.md`](research/ui-menu-ia-minitab-taxonomy-2026-08-23.md)  
> 全量映射：[`research/ui-menu-ia-command-taxonomy-map-2026-08-23.md`](research/ui-menu-ia-command-taxonomy-map-2026-08-23.md)  
> DoD：[`research/goal-wave-2026-08-23-ui-menu-ia-layout.md`](research/goal-wave-2026-08-23-ui-menu-ia-layout.md)

| 能力 | UI / 模块 | 数据 / 契约 | 测试 |
|---|---|---|---|
| **U1–U3** 声明式菜单 | `AnalysisCommand.menu_path` + `menu_group`；`MainWindow` 只渲染字段（深度≤1） | 顶层：统计 / 控制图 / 质量工具 / 图形；help `menu_path` = `{path} > {group}` | `ui_menu_ia_track_test`；`python tools/verify_ui_menu_ia_track.py` |

**不改** AnalysisService / domain 统计公式。Graph Builder（G3）不在本 Track。

## 7. Track G6：命令 Wizard（2026-08-23）

> 调研：[`research/g6-command-wizard-ux-research-2026-08-23.md`](research/g6-command-wizard-ux-research-2026-08-23.md)  
> DoD：[`research/goal-wave-2026-08-23-g6-command-wizard.md`](research/goal-wave-2026-08-23-g6-command-wizard.md)  
> 计划：[`research/goal-wave-2026-08-23-g6-command-wizard-plan-and-mega-prompt.md`](research/goal-wave-2026-08-23-g6-command-wizard-plan-and-mega-prompt.md)

| 能力 | UI / 模块 | 数据 / 契约 | 测试 |
|---|---|---|---|
| **W1** 推荐引擎 | `CommandRecommendationEngine`（`src/application/command_recommendation_engine.*`，无 Qt） | 列 `ColumnType` + `CommandWizardIntent` → Top-N≤8 `Recommendation`（`command_id`/`score`/`reason_key`）+ optional `hint_key` | `g6_command_wizard_track_test`（T01–T15） |
| **W2** Wizard UI | `CommandWizardDialog`（独立 QDialog 三步：选列→意图→推荐） | 信号 `openAnalysisRequested(command_id)`；**不**调用 `AnalysisService::*` | UI smoke + `QSignalSpy` |
| **W3** 接线 / i18n | MainWindow「统计 → 命令向导…」chrome；确认后 `run_from_spec` | `translations/ui_menu_strings.json`（`action.command_wizard` / `intent.*` / `reason.*` / `hint.*`） | `verify_g6_command_wizard_track.py`（Tester） |

**禁止：** Wizard/引擎内跑分析；推荐幽灵 command_id；塞进 MainWindow 单页堆控件。

## 8. Track G9：公式代入 / Show Your Work（2026-08-23）

> 调研：[`research/formula-substitution-show-your-work-research-2026-08-23.md`](research/formula-substitution-show-your-work-research-2026-08-23.md)  
> 计划：[`research/goal-wave-2026-08-23-g9-formula-substitution-plan-and-mega-prompt.md`](research/goal-wave-2026-08-23-g9-formula-substitution-plan-and-mega-prompt.md)  
> DoD：[`research/goal-wave-2026-08-23-g9-formula-substitution.md`](research/goal-wave-2026-08-23-g9-formula-substitution.md)  
> 覆盖矩阵：[`research/g9-formula-substitution-coverage-matrix.md`](research/g9-formula-substitution-coverage-matrix.md)

| 能力 | UI / 模块 | 数据 / 契约 | 测试 |
|---|---|---|---|
| **FS-A～J** 运行时公式代入 | `FormulaSubstitutionDialog`（四页 Stack：列表/变量/代入/出处）；输出页标题行「公式代入」 | `OutputPage.computation_traces` + `analysis_command_id`；`attach_computation_traces`（141 非豁免命令） | `g9_formula_substitution_track_test`；`python tools/verify_g9_formula_substitution_track.py` |

**不合并** G1 `FormulaRegistryDialog`（出处页可 `select_entry` 跳转）。豁免仅 `tests` / `rule_policy`。

### 8.1 Track G9-D：验算轨迹深化（2026-08-24 · 已交付）

> 调研：[`research/g9-show-your-work-deepen-research-2026-08-24.md`](research/g9-show-your-work-deepen-research-2026-08-24.md)  
> 计划+Mega：[`research/goal-wave-2026-08-24-g9-show-your-work-deepen-plan-and-mega-prompt.md`](research/goal-wave-2026-08-24-g9-show-your-work-deepen-plan-and-mega-prompt.md)  
> DoD：[`research/goal-wave-2026-08-24-g9-show-your-work-deepen.md`](research/goal-wave-2026-08-24-g9-show-your-work-deepen.md)  
> 深度矩阵：[`research/g9-show-your-work-depth-matrix.md`](research/g9-show-your-work-depth-matrix.md)

| 能力 | UI / 模块 | 数据 / 契约 | 测试 |
|---|---|---|---|
| **SYW-A** 分步求值模型 | 页3「分步求值」步骤表（序/说明/代入前/代入后/得数） | `ComputationStep.order/expression_* /value`；JSON round-trip | `g9_show_your_work_deepen_track_test` |
| **SYW-B～I** 深度绑定 | 四页不变；页2 真值绑定 | `computation_trace_attach_deep.cpp`（79 原 stub → L3）；`trace_helpers` Facts 优先 | 试点：capability / one_sample_t / imr / regression / gage_rr / weibayes |
| **SYW-J** 门禁 | — | `src` 内「主公式」=0；深度矩阵 143 命令 | `python tools/verify_g9_show_your_work_deepen_track.py` |

**L2 例外（≤15% A 类）：** 见深度矩阵 notes（Bootstrap/树路径/ARIMA 迭代等）。
